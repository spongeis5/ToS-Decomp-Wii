"""Which already-matched functions branch somewhere retail does not?

    python tools/reloc_audit.py [--unit U] [--quiet]

report.json is this project's oracle and it is BLIND TO RELOCATION
TARGETS. Measured, not assumed: re-base one of the RTTID_Fix stubs on the
wrong class so its single instruction branches to a different function
entirely, rebuild, and report.json still calls it 100% and the byte count
does not move. A relocated field is compared by the bits the object holds,
which are zero on both sides.

So a function can be counted as matched while calling the wrong thing, and
nothing in the pipeline would say so. `unitcmp.py` now resolves both sides
by NAME -- our relocation names the symbol, retail's displacement lands on
one -- and this walks every unit report.json credits with a matched
function, asking that stricter question of each.

A disagreement is not automatically our bug. Two readings, and they are
told apart by looking the name up:

  * the name exists at another address -- a real wrong target, and the
    match is overstated;
  * no symbol of that name is anywhere in the image -- the linker folded
    two identical bodies and only one name survived, and nothing in the
    linked image can say which was written.

Both are printed, separately, because they need different answers.
"""

import argparse
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import unitcmp as U                                      # noqa: E402

REPORT = ROOT / "build/R8IE78/report.json"


def units_with_matches():
    """-> {unit: {names report.json calls 100%}}, from the oracle itself."""
    if not REPORT.exists():
        sys.exit("reloc_audit: %s is missing -- run ninja first. Without it "
                 "this would audit nothing and report no problem." % REPORT)
    rep = json.loads(REPORT.read_text(encoding="utf-8"))
    out = {}
    for u in rep["units"]:
        hit = {f["name"] for f in u.get("functions", [])
               if f.get("fuzzy_match_percent") == 100.0}
        if not hit:
            continue
        name = u["name"].split("/", 1)[1] if "/" in u["name"] else u["name"]
        out[name] = hit
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--unit")
    ap.add_argument("--quiet", action="store_true")
    args = ap.parse_args()

    todo = units_with_matches()
    if args.unit:
        todo = {k: v for k, v in todo.items() if k == args.unit}
        if not todo:
            sys.exit("reloc_audit: report.json credits %r with no matched "
                     "function" % args.unit)

    byname, _byaddr = U.retail_addrs()
    wrong, folded, unbuilt = [], [], []
    checked = 0

    for unit in sorted(todo):
        if not any((ROOT / ("src/" + unit + e)).exists()
                   for e in U.SOURCE_EXTS):
            continue
        res = U.compare(unit)
        if isinstance(res, str):
            unbuilt.append(unit)
            continue
        obj, err = U.compile_unit(unit)
        if err:
            unbuilt.append(unit)
            continue
        mine = U.load(obj, True)
        for name in sorted(todo[unit]):
            if name not in res:
                continue
            checked += 1
            if res[name][0] == 0:
                continue
            # Which relocation disagrees, and does its name exist elsewhere?
            got, masks, brs = mine[name]
            here = byname.get(name)
            want = U.retail()[name][0]
            b = U.words(want)
            for i in sorted(brs):
                if i >= len(b) or here is None:
                    continue
                tgt = U.branch_target(here + 4 * i, b[i])
                if tgt is None:
                    continue
                names = _byaddr.get(tgt)
                if names and brs[i][0] in names:
                    continue
                row = (unit, name, brs[i][0],
                       sorted(names)[0] if names else "(nothing named)")
                if brs[i][0] in byname:
                    wrong.append(row)
                else:
                    folded.append(row)
        if not args.quiet:
            print("  checked %-52s %d" % (unit, len(todo[unit])))

    print("")
    print("  %d function(s) checked, of %d report.json calls matched in "
          "%d unit(s) with source" % (checked,
                                      sum(len(v) for v in todo.values()),
                                      len(todo)))
    print("")
    print("  %d branch(es) to a symbol that EXISTS ELSEWHERE -- the match is"
          " overstated:" % len(wrong))
    for unit, fn, ours, theirs in wrong:
        print("    %-40s %s" % (unit, fn))
        print("      ours -> %s   retail -> %s" % (ours, theirs))
    print("")
    print("  %d branch(es) whose symbol is nowhere in the image -- folded, "
          "and the linked image cannot say which was written:" % len(folded))
    for unit, fn, ours, theirs in folded:
        print("    %-40s %s" % (unit, fn))
        print("      ours -> %s   retail -> %s" % (ours, theirs))
    if unbuilt:
        print("")
        print("  %d unit(s) did not build and were NOT audited: %s"
              % (len(unbuilt), ", ".join(unbuilt)))
    return 1 if wrong else 0


if __name__ == "__main__":
    sys.exit(main())
