"""Every local and parameter of a function: its type, its declaration line,
and WHICH REGISTER OR FRAME SLOT the compiler put it in.

    python tools/dwarf_locals.py 0x800FE540        one function, by address
    python tools/dwarf_locals.py Setup             by name substring
    python tools/dwarf_locals.py --unit <unit>     every function in a unit
    python tools/dwarf_locals.py --report          the whole population

The four recorded near-misses in NOTES.md are all register allocation or
instruction scheduling, and the lever named for those is DECLARATION ORDER.
The DWARF does not have to be guessed at on either count: it records the
declaration line of every local and a location list saying which register
or frame slot it occupied over which range of the function.

29,455 of the 29,875 locals and parameters in the game library carry a
location, and about 22,500 of those name an exact register.

WHAT THIS IS NOT. It is a measurement of what the compiler DID, not of what
source text produces it. Two orders that allocate the same way are still
two orders, and the bytes are still the only test. It shortens the search;
it does not answer it.

THE RANGES ARE CHECKED, NOT ASSUMED. A DWARF 2 location list holds offsets
from the compile unit's own low_pc, not addresses, and reading them as
addresses gives plausible small numbers that are wrong. Every resolved
range is asserted to lie inside the function that owns it, and the tool
refuses to print if any does not -- a location that lands outside its
function means the base is wrong and every register named beside it is
suspect.
"""

import argparse
import re
import sys
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ELF = ROOT / "orig/R8IE78/files/SB09WiiMASTERWAD.elf"
SPLITS = ROOT / "config/R8IE78/splits.txt"

sys.path.insert(0, str(ROOT / "tools"))
import dwarf_types as T                                   # noqa: E402


def uleb(data, i=0):
    out, shift = 0, 0
    while i < len(data):
        b = data[i]
        out |= (b & 0x7F) << shift
        i += 1
        if not b & 0x80:
            break
        shift += 7
    return out


def sleb(data, i=0):
    out, shift, b = 0, 0, 0
    while i < len(data):
        b = data[i]
        out |= (b & 0x7F) << shift
        shift += 7
        i += 1
        if not b & 0x80:
            break
    if b & 0x40 and shift < 64:
        out -= 1 << shift
    return out


def regname(n):
    """DWARF register number -> the PowerPC name.

    0-31 are the GPRs and 32-63 the FPRs. Printing an FPR as `r32` is not a
    typo with no consequence: this tool exists to say which register a
    value is in, and a float in f0 is a different fact from an integer in a
    register that does not exist.
    """
    if n < 32:
        return "r%d" % n
    if n < 64:
        return "f%d" % (n - 32)
    return "dwarf reg %d" % n


def where(expr):
    """-> a short description of one DWARF location expression."""
    if not expr:
        return "(empty)"
    op = expr[0]
    if 0x50 <= op <= 0x6F:
        return regname(op - 0x50)
    if op == 0x90:                       # DW_OP_regx
        return regname(uleb(expr, 1))
    if 0x70 <= op <= 0x8F:
        return "%s %+d" % (regname(op - 0x70), sleb(expr, 1))
    if op == 0x91:                       # DW_OP_fbreg
        return "frame %+d" % sleb(expr, 1)
    if op == 0x92:                       # DW_OP_bregx
        return "regx"
    if op == 0x03:                       # DW_OP_addr
        return "%08X" % int.from_bytes(bytes(expr[1:5]), "big")
    return "op 0x%02X" % op


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


class Locals(object):
    def __init__(self):
        self.ty = T.Types(ELF)
        self.lists = self.ty.dw.location_lists()
        self.bad_range = 0
        self.no_loc = 0
        self.seen = 0

    def resolve(self, die, cu_base, lo, hi):
        """-> [(begin, end, description)], addresses fixed to the CU base."""
        a = die.attributes.get("DW_AT_location")
        if a is None:
            self.no_loc += 1
            return []
        self.seen += 1
        if isinstance(a.value, list):
            return [(lo, hi, where(a.value))]
        out = []
        for e in self.lists.get_location_list_at_offset(a.value):
            ex = getattr(e, "loc_expr", None)
            if ex is None:
                continue
            b = cu_base + e.begin_offset
            n = cu_base + e.end_offset
            if not (lo <= b <= hi and lo <= n <= hi):
                self.bad_range += 1
            out.append((b, n, where(ex)))
        return out

    def walk(self, keep):
        """Call `keep(cu_base, fn_die, rows)` for every function it accepts.

        `rows` is a list of ("param"/"local"/"block", die, ranges).
        """
        for cu in self.ty.dw.iter_CUs():
            top = cu.get_top_DIE()
            a = top.attributes.get("DW_AT_low_pc")
            cu_base = a.value if a else 0
            depth, fn, lo, hi, rows = 0, None, 0, 0, []
            for die in cu.iter_DIEs():
                if die.is_null():
                    depth -= 1
                    if fn is not None and depth <= fn[1]:
                        keep(cu_base, fn[0], rows)
                        fn, rows = None, []
                    continue
                if die.tag == "DW_TAG_subprogram":
                    if fn is not None:
                        keep(cu_base, fn[0], rows)
                    al = die.attributes.get("DW_AT_low_pc")
                    ah = die.attributes.get("DW_AT_high_pc")
                    lo = al.value if al else 0
                    hi = ah.value if ah else 0
                    fn, rows = (die, depth), []
                elif fn is not None and die.tag in (
                        "DW_TAG_formal_parameter", "DW_TAG_variable"):
                    kind = ("param" if die.tag == "DW_TAG_formal_parameter"
                            else "local")
                    rows.append((kind, die,
                                 self.resolve(die, cu_base, lo, hi)))
                elif fn is not None and die.tag == "DW_TAG_lexical_block":
                    bl = die.attributes.get("DW_AT_low_pc")
                    bh = die.attributes.get("DW_AT_high_pc")
                    if bl is not None and bh is not None:
                        rows.append(("block", die,
                                     [(bl.value, bh.value, "")]))
                if die.has_children:
                    depth += 1
            if fn is not None:
                keep(cu_base, fn[0], rows)

    def check(self):
        if self.bad_range:
            sys.exit("dwarf_locals: %d location range(s) of %d fall outside "
                     "the function that owns them. The compile unit base is "
                     "wrong, and every register named beside one of those is "
                     "suspect. REFUSING to print." % (self.bad_range,
                                                      self.seen))


def show(L, cu_base, fn, rows):
    nm = T.name_of(fn) or "?"
    al = fn.attributes.get("DW_AT_low_pc")
    ah = fn.attributes.get("DW_AT_high_pc")
    lo = al.value if al else 0
    hi = ah.value if ah else 0
    print("")
    print("  %s" % nm)
    print("  %08X..%08X, %d bytes" % (lo, hi, hi - lo))
    if not rows:
        print("    (no parameters, locals or blocks in the debug info)")
        return
    for kind, die, ranges in rows:
        if kind == "block":
            b, e, _ = ranges[0]
            print("    block  %-26s %08X..%08X" % ("", b, e))
            continue
        line = die.attributes.get("DW_AT_decl_line")
        # The array SUFFIX is separate from the element type in this
        # producer, and printing `char` where the variable is `char[16]`
        # is not a cosmetic slip: it is the difference between a character
        # and a buffer, and that difference is what this tool is read for.
        ref = L.ty.ref(die)
        tn = L.ty.type_name(ref) + L.ty.array_suffix(ref)
        text = "; ".join("%08X..%08X %s" % r for r in ranges) or "(nowhere)"
        print("    %-6s line %-5s %-22s %-28s %s"
              % (kind, line.value if line else "?", T.name_of(die) or "?",
                 tn[:28], text))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--unit", help="a unit name from splits.txt")
    ap.add_argument("--report", action="store_true")
    ap.add_argument("what", nargs="?", help="an address, or a name substring")
    args = ap.parse_args()

    L = Locals()

    if args.report:
        kinds, regs, fns, with_any = Counter(), Counter(), [0], [0]

        def note(cu_base, fn, rows):
            fns[0] += 1
            vars_ = [r for r in rows if r[0] != "block"]
            if vars_:
                with_any[0] += 1
            for _k, _d, ranges in vars_:
                if not ranges:
                    kinds["no location at all"] += 1
                    continue
                seen = {w for _b, _e, w in ranges}

                def isreg(w):
                    return w[:1] in ("r", "f") and w[1:].isdigit()

                if all(isreg(w) for w in seen):
                    kinds["a register throughout" if len(seen) == 1
                          else "different registers over its life"] += 1
                    for w in seen:
                        regs[w] += 1
                elif all(w.startswith("frame") for w in seen):
                    kinds["a frame slot"] += 1
                elif any(isreg(w) for w in seen):
                    kinds["a register for part of its life"] += 1
                else:
                    kinds["somewhere else"] += 1

        L.walk(note)
        L.check()
        print("  %d function(s) in the DWARF; %d declare a parameter or a "
              "local" % (fns[0], with_any[0]))
        print("  %d location(s) read, %d variable(s) had none"
              % (L.seen, L.no_loc))
        print("")
        for k, n in kinds.most_common():
            print("    %-40s %d" % (k, n))
        print("")
        print("  the registers they land in:")
        for k, n in regs.most_common(12):
            print("    %-6s %d" % (k, n))
        return 0

    if args.unit:
        rs = [r for u, r in unit_ranges() if u == args.unit]
        if not rs:
            sys.exit("dwarf_locals: splits.txt has no unit %r" % args.unit)
        spans = rs[0]
        hits = [0]

        def pick(cu_base, fn, rows):
            a = fn.attributes.get("DW_AT_low_pc")
            if a is None:
                return
            if any(lo <= a.value < hi for lo, hi in spans):
                hits[0] += 1
                show(L, cu_base, fn, rows)

        L.walk(pick)
        L.check()
        if not hits[0]:
            sys.exit("dwarf_locals: no function of %r is in the DWARF -- "
                     "that is not the same as a function with no locals."
                     % args.unit)
        print("")
        print("  %d function(s) in %s" % (hits[0], args.unit))
        return 0

    if not args.what:
        sys.exit(__doc__)
    addr = None
    if re.match(r"^(0x)?[0-9A-Fa-f]{8}$", args.what):
        addr = int(args.what, 16)
    hits = [0]

    def pick(cu_base, fn, rows):
        a = fn.attributes.get("DW_AT_low_pc")
        nm = T.name_of(fn) or ""
        if (addr is not None and a is not None and a.value == addr) or \
                (addr is None and args.what in nm):
            hits[0] += 1
            show(L, cu_base, fn, rows)

    L.walk(pick)
    L.check()
    if not hits[0]:
        sys.exit("dwarf_locals: nothing matches %r. A function that is not "
                 "in the DWARF is not a function with no locals." % args.what)
    print("")
    print("  %d function(s) matched %r" % (hits[0], args.what))
    return 0


if __name__ == "__main__":
    sys.exit(main())
