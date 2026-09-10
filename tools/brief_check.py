"""Validate `brief.py` against the tools it joins, on EVERY function.

    python tools/brief_check.py            a sample across the whole image
    python tools/brief_check.py --all      all of them, slowly

`brief.py` re-walks the DWARF itself rather than calling `dwarf_locals`
and `dwarf_lines` in turn, because one walk is the whole point of it.
That is exactly the shape of tool this repository has been burned by
twice: a second copy of a comparison that disagrees with the first, always
in the direction that gets believed. So the copy is checked against the
originals, and one disagreement anywhere fails the run:

  LINES    every (address, file, line) row brief holds for a function is
           the row dwarf_lines holds for it
  LOCALS   every (name, location) pair brief holds is one dwarf_locals
           holds, and vice versa
  COLUMN   every `rN=name` brief attaches to an instruction names a
           register that is literally in that instruction, and an address
           inside that variable's own scope

There is deliberately NO axis comparing brief's disassembly against
disasm's: brief CALLS `disasm.annotate`, so such a check would compare an
expression with itself and pass forever. A check that cannot fail is worse
than no check, because it gets counted as one.

A function that neither tool has an answer for is COUNTED, not skipped in
silence: a validator that quietly measures nothing passes every time.
"""

import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

import brief as B                                         # noqa: E402
import disasm as D                                        # noqa: E402
import dwarf_lines as DL                                  # noqa: E402
import dwarf_locals as DLoc                               # noqa: E402

WORD = re.compile(r"[A-Za-z]\w*")


def locals_reference():
    """-> {low_pc: sorted [(name, location)]} straight from dwarf_locals."""
    L = DLoc.Locals()
    out = {}

    def keep(cu_base, fn, rows):
        al = fn.attributes.get("DW_AT_low_pc")
        if al is None:
            return
        got = []
        for kind, die, ranges in rows:
            if kind == "block":
                continue
            for _b, _e, loc in ranges:
                got.append((DLoc.T.name_of(die) or "?", loc))
        out[al.value] = sorted(got)

    L.walk(keep)
    L.check()
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--all", action="store_true")
    ap.add_argument("--step", type=int, default=97,
                    help="sample every Nth function (default 97)")
    args = ap.parse_args()

    br = B.Brief()
    br.check()
    lines = DL.Lines()
    ref_locals = locals_reference()
    raw, secs, funcs, objs = D.load()

    picked = sorted(br.funcs)
    if not args.all:
        picked = picked[:: args.step]
    if not picked:
        sys.exit("brief_check: no function to check. That is a failure to "
                 "read the DWARF, not a clean run.")

    bad = []
    n_lines = n_locals = n_code = n_named = 0
    no_lines = no_code = uncovered = 0

    for lo in picked:
        fn = br.funcs[lo]
        hi = fn["hi"]

        mine = br.line_rows(lo, hi)
        theirs = lines.rows_in(lo, hi)
        if not theirs:
            no_lines += 1
        if [(a, f, ln) for a, f, ln, _s in mine] != \
           [(a, f, ln) for a, f, ln, _s in theirs]:
            bad.append("LINES  %08X %s: %d row(s) here, %d there"
                       % (lo, fn["name"], len(mine), len(theirs)))
        else:
            n_lines += len(mine)

        mine_v = sorted((v.name, v.loc) for v in fn["vars"])
        their_v = ref_locals.get(lo)
        if their_v is None:
            bad.append("LOCALS %08X %s: dwarf_locals has no such function"
                       % (lo, fn["name"]))
        elif mine_v != their_v:
            bad.append("LOCALS %08X %s: %r here, %r there"
                       % (lo, fn["name"], mine_v[:6], their_v[:6]))
        else:
            n_locals += len(mine_v)

        words, rows, _u = D.annotate(raw, secs, funcs, objs, lo, hi - lo)
        if words is None:
            no_code += 1
            continue
        n_code += len(rows)
        for at, _w, _l, dc, _n in rows:
            if mine and br.line_at(at, mine) is None:
                uncovered += 1
            col = B.var_column(fn, at, dc)
            if not col:
                continue
            present = set(WORD.findall(dc.text))
            for part in col.split("  "):
                if "=" not in part:
                    continue
                reg, nm = part.split("=", 1)
                n_named += 1
                if reg not in present:
                    bad.append("COLUMN %08X %s: names %s, not in %r"
                               % (at, fn["name"], reg, dc.text))
                for one in nm.split("|"):
                    if not any(v.name == one and v.lo <= at < v.hi
                               for v in fn["vars"]):
                        bad.append("SCOPE  %08X %s: names %s outside its "
                                   "scope" % (at, fn["name"], one))

    print("  %d function(s) checked, of %d in the DWARF"
          % (len(picked), len(br.funcs)))
    print("  %d line row(s) and %d local(s) agreed with the tools brief "
          "replaces" % (n_lines, n_locals))
    print("  %d instruction(s) read; %d register-to-variable label(s) "
          "checked against the instruction and the scope" % (n_code, n_named))
    print("  %d function(s) have no line rows at all; %d are in no loaded "
          "section; %d instruction(s) sit before their first line row"
          % (no_lines, no_code, uncovered))
    if bad:
        print("")
        for b in bad[:40]:
            print("  FAIL  %s" % b)
        if len(bad) > 40:
            print("  ... and %d more" % (len(bad) - 40))
        print("")
        print("  %d disagreement(s). brief.py must not be read until they "
              "are gone." % len(bad))
        return 1
    print("  0 disagreement(s).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
