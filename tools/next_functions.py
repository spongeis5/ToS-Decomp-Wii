"""Rank what is left by FUNCTIONS, not by bytes.

    python tools/next_functions.py [--limit N]

dtk counts matched functions PER UNIT even when the unit does not link, so
a unit blocked on its data still yields every function that matches inside
it -- iTime and zPerformanceDisplay each read 2 of 2 while still partial.
That makes "which unit is complete" the wrong question when the goal is a
function count, and tools/dwarf_targets.py answers a different one again:
it ranks by what a unit's CODE needs, and prints only its top tier.

Units whose symbols live in an anonymous namespace are excluded unless the
source file is already named after the blob, because their symbols cannot
pair otherwise -- see tools/anon_blocked.py.
"""

import argparse
import io
import json
import re
import sys
from contextlib import redirect_stdout
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
REPORT = ROOT / "build/R8IE78/report.json"
sys.path.insert(0, str(ROOT / "tools"))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--limit", type=int, default=30)
    ap.add_argument("--all", action="store_true",
                    help="include units blocked by anonymous-namespace "
                         "mangling")
    args = ap.parse_args()

    if not REPORT.exists():
        sys.exit("next_functions: build/R8IE78/report.json is missing -- "
                 "run `ninja` first")
    rep = json.loads(REPORT.read_text(encoding="utf-8"))

    import anon_blocked
    buf = io.StringIO()
    with redirect_stdout(buf):
        anon_blocked.main()
    blocked = set()
    for line in buf.getvalue().splitlines():
        m = re.match(r"\s+[\d,]+ B\s+\d+ anon symbol\(s\)\s+(.*)$", line)
        if m:
            blocked.add(m.group(1).strip())
    if not blocked:
        sys.exit("next_functions: anon_blocked.py listed nothing -- refusing "
                 "to rank targets without knowing which are unreachable")

    rows = []
    for u in rep["units"]:
        meta = u.get("metadata", {})
        if "game" not in (meta.get("progress_categories") or []):
            continue
        m = u["measures"]
        tot = m.get("total_functions", 0)
        got = m.get("matched_functions", 0)
        left = tot - got
        if left <= 0:
            continue
        # dtk prefixes unit names with the DOL name.
        name = u["name"].split("/", 1)[1] if "/" in u["name"] else u["name"]
        src = meta.get("source_path") or ""
        is_blocked = any(name in b or b.endswith(name) for b in blocked)
        if is_blocked and not src and not args.all:
            continue
        rows.append((left, tot, got, int(m.get("total_code", 0)),
                     name, bool(src), is_blocked))

    rows.sort(reverse=True)
    print("  %-5s %-9s %-9s %s" % ("LEFT", "OF", "BYTES", "UNIT"))
    shown = 0
    for left, tot, got, code, name, written, is_blocked in rows:
        if shown >= args.limit:
            break
        mark = ""
        if is_blocked:
            mark = "  [anon]"
        elif written:
            mark = "  [started]"
        print("  %-5d %-9s %-9s %s%s"
              % (left, "%d" % tot, format(code, ","), name, mark))
        shown += 1

    total_left = sum(r[0] for r in rows)
    print("")
    print("  %d unit(s) with unmatched functions, %d function(s) left in "
          "game code" % (len(rows), total_left))
    return 0


if __name__ == "__main__":
    sys.exit(main())
