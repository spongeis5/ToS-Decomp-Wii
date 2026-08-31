"""Attribute .data/.bss/.rodata to units by WHO REFERENCES IT.

    python tools/dwarf_data.py                 the attribution, with counts
    python tools/dwarf_data.py --unit X        proposed split lines for one

The 258 units `dwarf_splits.py` recovered carry `.text` only. A unit whose
file owns statics therefore cannot match: its data still sits in the parent
chunk. This is the evidence for putting it back.

THE DWARF WILL NOT DO IT, and that is measured rather than assumed.
DW_AT_decl_file placed all 10,064 functions; it places no data at all:

  * 18,594 DW_TAG_variable DIEs have a DW_AT_location, and every one is a
    DW_FORM_data4 offset into `.debug_loc` holding a REGISTER location list
    (DW_OP_reg17, DW_OP_regx 32, ...). Those DIEs are locals.
  * 33 have the file-scope shape -- named, external, no location. 31 of
    them join a data symbol by name, and NONE of the 33 carries a
    DW_AT_decl_file.

So the code is the only evidence, and it is good evidence: a data address
formed by functions in exactly ONE unit belongs to that unit. Measured over
the recovered units: **2,321 distinct data addresses referenced from 1,893
functions, 1,972 of them by exactly one unit (85.0%), across 205 units**.

References are found by tracking `lis`/`addis` into a register and pairing
it with the next `addi`/`ori`/load/store on that register. The `ori` case
matters more than it looks: a first version had it nested inside the D-form
branch, where `ori` never reaches, and that one branch cost 1,444 of the
2,321 addresses -- it reported 877 and 82% with no sign anything was
missing.

FALSE POSITIVES ARE POSSIBLE AND ARE NOT FILTERED AWAY. A `lis`/`addi` pair
is only probably an address; some are ordinary constants. Those show up as
addresses with no symbol and sometimes without even 4-byte alignment
(`80680003`), and they are left visible rather than quietly dropped, so the
proposed lines get read before they get used.

SMALL DATA DOES NOT APPEAR, which is worth stating so nobody adds the
handling twice. r13/r2 are set by `__init_registers` to 0x80821920 and
0x808245E0 -- each its section start plus 0x8000, which cross-checks -- and
of 7,054 references from 1,893 functions in the recovered units, **0** use
either base. The game code is not built with small-data addressing.

WHAT THIS DOES NOT DO. It reports and proposes; it does not write
splits.txt. An address referenced by several units is SHARED and stays
where it is -- attributing it to one of them would be a guess, and the 18%
that are shared are reported as shared rather than assigned.
"""

import argparse
import re
import struct
import sys
from collections import Counter, defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import dwarf_splits as D                                       # noqa: E402
import dwarf_targets as T                                      # noqa: E402

# Read out of __init_registers rather than guessed:
#   lis r2, 0x8082 ; ori r2, r2, 0x45e0
#   lis r13,0x8082 ; ori r13,r13,0x1920
SDA = 0x80821920
SDA2 = 0x808245E0

# D-form opcodes whose RA is a base register, plus addi.
DFORM = {14, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45,
         48, 49, 50, 51, 52, 53, 54, 55}


def symbols():
    out = {}
    pat = re.compile(r"^(\S+) = \.(\w+):0x([0-9A-Fa-f]+);"
                     r".*?size:0x([0-9A-Fa-f]+)")
    for line in (ROOT / "config/R8IE78/symbols.txt").read_text(
            encoding="utf-8").splitlines():
        m = pat.match(line.strip())
        if m:
            out.setdefault(int(m.group(3), 16),
                           (m.group(1), m.group(2), int(m.group(4), 16)))
    return out


def refs_of(d, secs, lo, hi):
    """Absolute addresses this function forms. -> set."""
    blob = T.read(d, secs, lo, hi - lo)
    out = set()
    if blob is None:
        return out
    hi_reg = {}
    for i in range(len(blob) // 4):
        w = struct.unpack_from(">I", blob, i * 4)[0]
        op = w >> 26
        rd, ra = (w >> 21) & 31, (w >> 16) & 31
        imm = w & 0xFFFF
        simm = imm - 0x10000 if imm >= 0x8000 else imm
        if op == 15:
            if ra == 0:
                hi_reg[rd] = imm << 16
            else:
                hi_reg.pop(rd, None)
            continue
        if op == 24 and rd in hi_reg:                  # ori completes a pair
            out.add((hi_reg[rd] | imm) & 0xFFFFFFFF)
            continue
        if op in DFORM:
            if ra in hi_reg:
                out.add((hi_reg[ra] + simm) & 0xFFFFFFFF)
            elif ra == 13:
                out.add((SDA + simm) & 0xFFFFFFFF)
            elif ra == 2:
                out.add((SDA2 + simm) & 0xFFFFFFFF)
    return out


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--unit")
    ap.add_argument("--limit", type=int, default=16)
    a = ap.parse_args(argv)

    d, secs = T.image()
    funcs = D.dwarf_functions()
    known = set(D.parse_splits())
    syms = symbols()

    text_lo, text_hi = 0x80006760, 0x8067CE40
    by_addr = defaultdict(set)
    nfn = 0
    for lo, hi, src in funcs:
        u = D.src_path(src)
        if u not in known:
            continue
        nfn += 1
        for x in refs_of(d, secs, lo, hi):
            if text_lo <= x < text_hi:
                continue                                # a code address
            by_addr[x].add(u)

    excl = {x: next(iter(us)) for x, us in by_addr.items() if len(us) == 1}
    shared = len(by_addr) - len(excl)
    per_unit = defaultdict(list)
    for x, u in excl.items():
        per_unit[u].append(x)

    if a.unit:
        hits = [u for u in per_unit if u.endswith(a.unit)]
        if not hits:
            print("no recovered unit ending %r owns data exclusively."
                  % a.unit)
            print("That is not the same as owning none -- it may reference")
            print("only addresses other units reference too.")
            return 1
        for u in hits:
            addrs = sorted(per_unit[u])
            print("%s -- %d exclusively referenced address(es)"
                  % (u, len(addrs)))
            for x in addrs:
                s = syms.get(x)
                print("   %08X  %-10s %s"
                      % (x, s[1] if s else "?", s[0] if s else "<no symbol>"))
            print("")
            print("proposed split lines (VERIFY before use -- these are the")
            print("addresses referenced, not proof of the section extents):")
            runs = []
            for x in addrs:
                s = syms.get(x)
                if not s:
                    continue
                if runs and runs[-1][0] == s[1] and runs[-1][2] == x:
                    runs[-1][2] = x + s[2]
                else:
                    runs.append([s[1], x, x + s[2]])
            for sec, lo, hi in runs:
                print("\t.%-10s start:0x%08X end:0x%08X" % (sec, lo, hi))
        return 0

    print("%d function(s) in recovered units, %d distinct data address(es)"
          % (nfn, len(by_addr)))
    print("%d referenced by exactly ONE unit (%.1f%%), %d shared"
          % (len(excl), 100.0 * len(excl) / max(1, len(by_addr)), shared))
    print("%d unit(s) would gain data" % len(per_unit))
    print("")
    print("A shared address stays where it is. Assigning it to one of its")
    print("referrers would be a guess, and there are %d of them." % shared)
    print("")
    for u, xs in sorted(per_unit.items(), key=lambda kv: -len(kv[1]))[:a.limit]:
        print("   %4d addr(s)  %s" % (len(xs), u))
    return 0


if __name__ == "__main__":
    sys.exit(main())
