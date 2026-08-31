"""Attribute .data/.bss/.rodata to source files by DECLARATION.

    python tools/dwarf_data_decl.py --report        coverage and contiguity
    python tools/dwarf_data_decl.py --file X.cpp    one file's data
    python tools/dwarf_data_decl.py --conflicts     addresses two files claim

`dwarf_data.py` attributes data by WHO REFERENCES IT, and says in its own
docstring that it has to, because "DW_AT_decl_file placed all 10,064
functions; it places no data at all". That measurement is wrong, and the
way it is wrong is worth keeping: it counted 18,594 DW_TAG_variable DIEs
with a DW_AT_location, saw location lists holding REGISTER locations, and
generalised. 15,741 of those are locals and do hold registers. The other
**2,853 are at file scope, and their location lists hold DW_OP_addr** -- a
fixed address -- and every one of them carries a DW_AT_decl_file.

So the DWARF does place data, for the game library:

    section      of section covered     files   contiguous
    .bss              434,758  41.5%      272      230 (85%)
    .data             124,482  28.8%      153      148 (97%)
    .rodata             6,846   3.3%       65       63 (97%)

against the 62% / 60% / 74% that reference-attribution manages. The two
disagree because they answer different questions: a static belonging to
file A but only ever touched through A's code inlined into B is REFERENCED
by B and DECLARED by A, and only one of those is what a split needs.

WHAT IT DOES NOT COVER, which is the whole reason the other tool still
matters. Anonymous data -- string literals, floating-point pools, jump
tables -- has no DIE and never will; that is most of `.rodata`, hence 3.3%.
And only the eleven compile units with DWARF are covered, which is the game
library; the SDK, Havok and FMOD have no debug info at all, and their data
is inside these same sections, so the percentages of a WHOLE section
understate what is covered of the part that has any evidence.

SIZES COME FROM THE SYMBOL TABLE, not from walking the type. Walking is
wrong in both directions -- a `Foo*` has no DW_AT_byte_size so the walk
reaches the pointed-to struct and reports ITS size, and `int a[100]`
reaches the element and reports 4. Doing it that way summed 1,347,263
bytes into a 1,048,584 byte section, with no duplicate addresses at all.
Every address is required to have a sized symbol and the tool refuses to
report if one does not.
"""

import argparse
import sys
from collections import defaultdict
from pathlib import Path

from elftools.elf.elffile import ELFFile

ROOT = Path(__file__).resolve().parent.parent
ELF = ROOT / "orig/R8IE78/files/SB09WiiMASTERWAD.elf"

sys.path.insert(0, str(ROOT / "tools"))
import dwarf_types as T                                   # noqa: E402
import dwarf_lines as DL                                  # noqa: E402


def sections_and_sizes():
    secs, sizes = [], {}
    with open(ELF, "rb") as fh:
        f = ELFFile(fh)
        for s in f.iter_sections():
            h = s.header
            if h["sh_addr"] and h["sh_size"]:
                secs.append((h["sh_addr"], h["sh_addr"] + h["sh_size"],
                             s.name))
            if h["sh_type"] == "SHT_SYMTAB":
                for sym in s.iter_symbols():
                    if sym["st_info"]["type"] != "STT_FUNC" \
                            and sym["st_size"]:
                        a = sym["st_value"]
                        sizes[a] = max(sizes.get(a, 0), sym["st_size"])
    return secs, sizes


class Decl(object):
    def __init__(self):
        self.ty = T.Types(ELF)
        self.loclists = self.ty.dw.location_lists()
        self.secs, self.sizes = sections_and_sizes()
        self.vars = {}          # address -> (file, name)
        self.claims = defaultdict(set)
        self._read()
        if not self.vars:
            sys.exit("dwarf_data_decl: no file-scope variable carries both "
                     "an address and a declaring file. That is a failure to "
                     "read the DWARF, not a program with no data.")
        missing = [a for a in self.vars if a not in self.sizes]
        if missing:
            sys.exit("dwarf_data_decl: %d of %d address(es) have no sized "
                     "symbol, so their extent is unknown and every coverage "
                     "figure below them would be invented. REFUSING."
                     % (len(missing), len(self.vars)))

    def _addr(self, loc):
        exprs = [loc.value] if isinstance(loc.value, list) else [
            e.loc_expr
            for e in self.loclists.get_location_list_at_offset(loc.value)
            if getattr(e, "loc_expr", None)]
        for ex in exprs:
            if ex and ex[0] == 0x03 and len(ex) >= 5:
                return int.from_bytes(bytes(ex[1:5]), "big")
        return None

    def _read(self):
        for cu in self.ty.dw.iter_CUs():
            prog = self.ty.dw.line_program_for_CU(cu)
            names = []
            if prog is not None:
                for fe in prog.header.get("file_entry", []):
                    n = fe.name
                    names.append(n.decode("utf-8", "replace")
                                 if isinstance(n, bytes) else n)
            depth, in_fn = 0, None
            for die in cu.iter_DIEs():
                if die.is_null():
                    depth -= 1
                    if in_fn is not None and depth <= in_fn:
                        in_fn = None
                    continue
                if die.tag == "DW_TAG_subprogram" and in_fn is None:
                    in_fn = depth
                elif die.tag == "DW_TAG_variable" and in_fn is None:
                    loc = die.attributes.get("DW_AT_location")
                    df = die.attributes.get("DW_AT_decl_file")
                    a = self._addr(loc) if loc is not None else None
                    if a is not None and df is not None:
                        f = (DL.base(names[df.value - 1])
                             if 1 <= df.value <= len(names)
                             else "?%d" % df.value)
                        self.vars[a] = (f, T.name_of(die) or "?")
                        self.claims[a].add(f)
                if die.has_children:
                    depth += 1

    def section_of(self, a):
        for lo, hi, n in self.secs:
            if lo <= a < hi:
                return n
        return "?"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--report", action="store_true")
    ap.add_argument("--conflicts", action="store_true")
    ap.add_argument("--file")
    args = ap.parse_args()

    d = Decl()

    if args.conflicts:
        bad = {a: fs for a, fs in d.claims.items() if len(fs) > 1}
        print("  %d of %d address(es) are claimed by more than one file"
              % (len(bad), len(d.vars)))
        for a in sorted(bad):
            print("    %08X  %s" % (a, ", ".join(sorted(bad[a]))))
        return 0

    if args.file:
        rows = sorted((a, n) for a, (f, n) in d.vars.items()
                      if f == args.file)
        if not rows:
            sys.exit("dwarf_data_decl: no variable is declared in %r. A file "
                     "absent from the DWARF is not a file with no data."
                     % args.file)
        print("  %s declares %d variable(s)" % (args.file, len(rows)))
        for a, n in rows:
            print("    %08X  %-9s %6d  %s"
                  % (a, d.section_of(a), d.sizes[a], n))
        return 0

    print("  %d file-scope variable(s) with an address and a declaring file,"
          " over %d file(s)"
          % (len(d.vars), len({f for f, _n in d.vars.values()})))
    bad = sum(1 for fs in d.claims.values() if len(fs) > 1)
    print("  %d address(es) claimed by more than one file" % bad)
    print("")
    print("  %-10s %10s %10s %8s  %6s %6s %6s"
          % ("SECTION", "SIZE", "COVERED", "PERCENT", "VARS", "FILES",
             "CONTIG"))
    for lo, hi, nm in sorted(d.secs, key=lambda s: -(s[1] - s[0])):
        here = [(a, d.sizes[a]) for a in d.vars if lo <= a < hi]
        if not here:
            continue
        iv = sorted((a, min(a + max(z, 1), hi)) for a, z in here)
        merged, cur = [], None
        for s, e in iv:
            if cur and s <= cur[1]:
                cur = (cur[0], max(cur[1], e))
            else:
                if cur:
                    merged.append(cur)
                cur = (s, e)
        if cur:
            merged.append(cur)
        cov = sum(e - s for s, e in merged)

        spans = defaultdict(lambda: [1 << 32, 0])
        for a, z in here:
            v = spans[d.vars[a][0]]
            v[0] = min(v[0], a)
            v[1] = max(v[1], a + max(z, 1))
        clean = sum(1 for f, (s, e) in spans.items()
                    if not any(s <= a < e and d.vars[a][0] != f
                               for a, _z in here))
        print("  %-10s %10s %10s %7.1f%%  %6d %6d %5d"
              % (nm, "{:,}".format(hi - lo), "{:,}".format(cov),
                 100.0 * cov / (hi - lo), len(here), len(spans), clean))
    print("")
    print("  CONTIG counts files whose variables form one run with no other")
    print("  file's variable inside it. Anonymous data -- literals, pools --")
    print("  has no DIE and is not counted anywhere above.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
