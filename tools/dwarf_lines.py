"""The line table: which source line each instruction came from, and so
WHICH FILE -- which is a map of what the inliner did.

    python tools/dwarf_lines.py --report          the population
    python tools/dwarf_lines.py 0x801ECDB0        one function, line by line
    python tools/dwarf_lines.py --inlines         foreign code, by unit
    python tools/dwarf_lines.py --unit <unit>     every function in a unit

`.debug_line` is 990 KB of the retail ELF and nothing read it. Two things
are in there that are in no other source:

  * THE ORIGINAL'S STATEMENT STRUCTURE. A row marked `stmt` is a statement
    boundary, so a function's instructions group into the lines that wrote
    them. hkGetKeyValue turns out to be seven lines with the loop's three
    increments all on the CONDITION line, not the body -- which says the
    original is a `for` with an increment clause and not a `while` with
    three statements in it.

  * WHERE CODE WAS INLINED FROM. A row inside a function that names a
    different file is code the compiler expanded there. Those bytes cannot
    be matched by writing that function alone, however right the source is;
    they belong to a header. Nothing else here can tell you that, and
    telling a near miss apart from an impossible one is worth knowing
    before spending a day on it.

The file index in a row is an index into that compile unit's own file
table, so it is resolved per CU and never across; a row whose index is out
of range is counted, not guessed at.
"""

import argparse
import bisect
import re
import sys
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ELF = ROOT / "orig/R8IE78/files/SB09WiiMASTERWAD.elf"
SPLITS = ROOT / "config/R8IE78/splits.txt"

sys.path.insert(0, str(ROOT / "tools"))
import dwarf_types as T                                   # noqa: E402


def unit_ranges():
    out, cur, ranges = [], None, []
    for line in SPLITS.read_text(encoding="utf-8").splitlines():
        if line and not line[0].isspace() and line.rstrip().endswith(":"):
            if cur:
                out.append((cur, ranges))
            cur, ranges = line.rstrip()[:-1], []
            continue
        m = re.match(r"\s+\.text\s+start:(0x[0-9A-Fa-f]+)\s+"
                     r"end:(0x[0-9A-Fa-f]+)", line)
        if m and cur:
            ranges.append((int(m.group(1), 16), int(m.group(2), 16)))
    if cur:
        out.append((cur, ranges))
    return out


def base(name):
    return str(name).replace(chr(92), "/").rsplit("/", 1)[-1]


class Lines(object):
    """Every line row, and every function, indexed by address."""

    def __init__(self):
        self.ty = T.Types(ELF)
        self.rows = []            # (addr, file name, line, is_stmt)
        self.funcs = []           # (lo, hi, name, own file name)
        self.unknown_file = 0
        for cu in self.ty.dw.iter_CUs():
            prog = self.ty.dw.line_program_for_CU(cu)
            names = []
            if prog is not None:
                for fe in prog.header.get("file_entry", []):
                    n = fe.name
                    names.append(n.decode("utf-8", "replace")
                                 if isinstance(n, bytes) else n)

            def fname(i):
                if 1 <= i <= len(names):
                    return base(names[i - 1])
                self.unknown_file += 1
                return "?%d" % i

            if prog is not None:
                for e in prog.get_entries():
                    s = e.state
                    if s is None or s.end_sequence:
                        continue
                    self.rows.append((s.address, fname(s.file), s.line,
                                      bool(s.is_stmt)))
            for die in cu.iter_DIEs():
                if die.is_null() or die.tag != "DW_TAG_subprogram":
                    continue
                lo = die.attributes.get("DW_AT_low_pc")
                hi = die.attributes.get("DW_AT_high_pc")
                df = die.attributes.get("DW_AT_decl_file")
                if lo is None or hi is None:
                    continue
                self.funcs.append((lo.value, hi.value,
                                   T.name_of(die) or "?",
                                   fname(df.value) if df else "?"))
        self.rows.sort()
        self.funcs.sort()
        self._addrs = [r[0] for r in self.rows]
        if not self.rows:
            sys.exit("dwarf_lines: the line table is empty. That is a "
                     "failure to read it, not a program with no lines.")

    def rows_in(self, lo, hi):
        i = bisect.bisect_left(self._addrs, lo)
        out = []
        while i < len(self.rows) and self.rows[i][0] < hi:
            out.append(self.rows[i])
            i += 1
        return out

    def find(self, addr):
        for lo, hi, nm, own in self.funcs:
            if lo == addr:
                return (lo, hi, nm, own)
        return None


def show(L, fn):
    lo, hi, nm, own = fn
    rows = L.rows_in(lo, hi)
    print("")
    print("  %s" % nm)
    print("  %08X..%08X, %d bytes, declared in %s" % (lo, hi, hi - lo, own))
    if not rows:
        print("    (no line rows -- the table does not cover this function)")
        return
    lines = sorted({(f, ln) for _a, f, ln, _s in rows})
    foreign = sorted({f for _a, f, _l, _s in rows if f != own})
    print("    %d row(s) over %d distinct source line(s)"
          % (len(rows), len(lines)))
    if foreign:
        print("    INLINED FROM: %s" % ", ".join(foreign))
    print("")
    for a, f, ln, stmt in rows:
        print("    %08X  %-34s line %-6d %s"
              % (a, f[-34:], ln, "stmt" if stmt else ""))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--report", action="store_true")
    ap.add_argument("--inlines", action="store_true")
    ap.add_argument("--unit")
    ap.add_argument("--limit", type=int, default=25)
    ap.add_argument("addr", nargs="?")
    args = ap.parse_args()

    L = Lines()

    if args.report or args.inlines:
        with_rows = clean = mixed = 0
        foreign_bytes = own_bytes = 0
        by_file = Counter()
        for lo, hi, nm, own in L.funcs:
            rows = L.rows_in(lo, hi)
            if not rows:
                continue
            with_rows += 1
            # A row owns the bytes from it to the next row.
            for i, (a, f, _l, _s) in enumerate(rows):
                end = rows[i + 1][0] if i + 1 < len(rows) else hi
                n = max(0, end - a)
                if f == own:
                    own_bytes += n
                else:
                    foreign_bytes += n
                    by_file[f] += n
            if any(f != own for _a, f, _l, _s in rows):
                mixed += 1
            else:
                clean += 1

        print("  %d row(s) in the line table; %d file index(es) out of range"
              % (len(L.rows), L.unknown_file))
        print("  %d function(s) have rows, of %d in the DWARF"
              % (with_rows, len(L.funcs)))
        print("  %d are entirely their own file's code; %d hold code from "
              "somewhere else" % (clean, mixed))
        print("  %s byte(s) of own code, %s byte(s) INLINED from elsewhere "
              "(%.1f%%)"
              % ("{:,}".format(own_bytes), "{:,}".format(foreign_bytes),
                 100.0 * foreign_bytes / max(1, own_bytes + foreign_bytes)))
        print("")
        print("  where the inlined bytes came from:")
        for f, n in by_file.most_common(args.limit):
            print("    %-46s %s" % (f[-46:], "{:,}".format(n)))
        print("    %d file(s) in total contributed inlined code"
              % len(by_file))
        return 0

    if args.unit:
        rs = [r for u, r in unit_ranges() if u == args.unit]
        if not rs:
            sys.exit("dwarf_lines: splits.txt has no unit %r" % args.unit)
        hits = 0
        for fn in L.funcs:
            if any(lo <= fn[0] < hi for lo, hi in rs[0]):
                show(L, fn)
                hits += 1
        if not hits:
            sys.exit("dwarf_lines: no function of %r is in the DWARF, which "
                     "is not the same as one with no lines." % args.unit)
        print("")
        print("  %d function(s) in %s" % (hits, args.unit))
        return 0

    if not args.addr:
        sys.exit(__doc__)
    fn = L.find(int(args.addr, 16))
    if fn is None:
        sys.exit("dwarf_lines: no function starts at %s. A function absent "
                 "from the DWARF is not a function with no lines."
                 % args.addr)
    show(L, fn)
    return 0


if __name__ == "__main__":
    sys.exit(main())
