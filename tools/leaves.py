"""Unwritten functions with NO calls -- the ones that cannot fail to link.

    python tools/leaves.py <unit-substring> [max-bytes]

Picking the next function to write by SIZE wastes the afternoon. Three
were opened in one session and abandoned because each calls something
our source does not define -- ExtraIdleCheck, ResetRandomAnims,
GetPlayerPosition. A caller that names an undefined symbol compiles fine
and fails the LINK, which is the trap HANDBOOK records against
zCamSplineCommonMix.

A LEAF -- no `bl` anywhere in its disassembly -- has no callee to write
first. It is the only class of unwritten function guaranteed to be
writable right now, so it is the one to pick.

zSBPlayerSpinAttack::FinishedQueueCheck came off this list and matched
at 160 bytes on the first attempt.

A NOTE ON THE FILTER, because the first version of it lied. disasm.py
ends every listing with `N printed as .word`, so testing `".word" in
output` matches the FOOTER of every function and excludes all of them.
It reported `0 leaves of 106` -- a filter returning nothing, dressed as
a measurement. The count has to be parsed, not searched for.
"""
import argparse
import json
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
REPORT = ROOT / "build/R8IE78/report.json"

WORD_COUNT = re.compile(r"(\d+) printed as \.word")
DECODED = re.compile(r"(\d+) of (\d+) instruction\(s\) decoded")
CALL = re.compile(r"\bbl\s")


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("unit", help="substring of the unit name")
    ap.add_argument("max_bytes", nargs="?", type=int, default=200)
    args = ap.parse_args()

    if not REPORT.is_file():
        raise SystemExit("%s does not exist -- run `python configure.py && "
                         "ninja` first." % REPORT)

    R = json.loads(REPORT.read_text(encoding="utf-8"))
    rows, units = [], 0
    for u in R["units"]:
        if args.unit not in u["name"]:
            continue
        units += 1
        for f in (u.get("functions") or []):
            if f.get("fuzzy_match_percent", 0.0) != 0.0:
                continue
            if int(f["size"]) > args.max_bytes:
                continue
            rows.append((int(f["size"]), f["name"]))
    rows.sort()

    leaves, calls, undecoded, failed = [], 0, 0, 0
    for size, name in rows:
        r = subprocess.run([sys.executable, "tools/disasm.py", name],
                           cwd=ROOT, capture_output=True, text=True)
        out = r.stdout + r.stderr
        m = WORD_COUNT.search(out)
        d = DECODED.search(out)
        if not m or not d:
            failed += 1
            print("  %6d  NOT DISASSEMBLED  %s" % (size, name[:60]))
            continue
        if int(m.group(1)) != 0:
            undecoded += 1
            continue
        if CALL.search(out):
            calls += 1
            continue
        leaves.append((size, name))
        print("  %6d  %s" % (size, name[:72]))

    print()
    print("  %d leaf(s), %d byte(s), of %d unwritten function(s) at or under"
          " %d bytes in %d unit(s)"
          % (len(leaves), sum(s for s, _ in leaves), len(rows),
             args.max_bytes, units))
    print("  excluded: %d with calls, %d with undecoded words, %d that would"
          " not disassemble" % (calls, undecoded, failed))
    return 0


if __name__ == "__main__":
    sys.exit(main())
