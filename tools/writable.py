"""Unwritten functions whose CALLEES all already exist -- writable now.

    python tools/writable.py <unit-substring> [max-bytes]

tools/leaves.py takes the strict line: no `bl` at all, so nothing to
write first. That is guaranteed safe but rare -- zCommonPlayerActions
has 0 leaves among 15 unwritten, and every one is excluded for having a
call.

The real requirement is weaker. A function is writable now when every
symbol it branches to is ALREADY in the image we build: either a
function our object defines (report.json scores it above zero) or one
this unit itself defines. Then nothing is left dangling and the link
holds.

The trap this exists to avoid: a caller naming an undefined symbol
compiles fine and fails the LINK, which HANDBOOK records against
zCamSplineCommonMix. Three functions were opened and abandoned to it in
one session -- ExtraIdleCheck, ResetRandomAnims, GetPlayerPosition.

Every count states its denominator, and a function whose disassembly
could not be read is reported, not silently dropped.
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
# disasm.py renders a resolved branch as `bl  -> name`
CALL = re.compile(r"\bbl\s+(?:->\s*)?(\S+)?")
ARROW = re.compile(r"\bbl\s+->\s+(\S+)")


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("unit")
    ap.add_argument("max_bytes", nargs="?", type=int, default=200)
    args = ap.parse_args()

    if not REPORT.is_file():
        raise SystemExit("%s does not exist -- build first." % REPORT)
    R = json.loads(REPORT.read_text(encoding="utf-8"))

    # Every symbol our object already defines, anywhere in the image.
    have = set()
    for u in R["units"]:
        for f in (u.get("functions") or []):
            if f.get("fuzzy_match_percent", 0.0) > 0.0:
                have.add(f["name"])

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

    ok, blocked, unreadable = [], {}, 0
    for size, name in rows:
        r = subprocess.run([sys.executable, "tools/disasm.py", name],
                           cwd=ROOT, capture_output=True, text=True)
        out = r.stdout + r.stderr
        m = WORD_COUNT.search(out)
        if not m:
            unreadable += 1
            print("  %6d  NOT DISASSEMBLED  %s" % (size, name[:58]))
            continue
        if int(m.group(1)) != 0:
            blocked["undecoded words"] = blocked.get("undecoded words", 0) + 1
            continue
        targets = set(ARROW.findall(out))
        # An unresolved `bl` with no name is a call we cannot account for.
        raw = len(re.findall(r"\bbl\s", out))
        if raw != len(ARROW.findall(out)):
            blocked["unnamed call"] = blocked.get("unnamed call", 0) + 1
            continue
        missing = [t for t in targets if t not in have]
        if missing:
            blocked["missing callee"] = blocked.get("missing callee", 0) + 1
            continue
        ok.append((size, name, len(targets)))
        print("  %6d  %-64s %d call(s)" % (size, name[:64], len(targets)))

    print()
    print("  %d writable now, %d byte(s), of %d unwritten at or under %d"
          " bytes in %d unit(s)"
          % (len(ok), sum(s for s, _, _ in ok), len(rows), args.max_bytes,
             units))
    for why, n in sorted(blocked.items()):
        print("  excluded %4d for %s" % (n, why))
    if unreadable:
        print("  %d could NOT be disassembled and were not judged"
              % unreadable)
    return 0


if __name__ == "__main__":
    sys.exit(main())
