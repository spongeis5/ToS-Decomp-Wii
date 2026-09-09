"""Every WRITTEN function that is not byte-identical, biggest first.

    python nearmiss.py [min-fuzzy]

A function with a fuzzy score strictly between 0 and 100 is one whose
object we build and whose bytes disagree -- work already done that
counts for nothing until it lands. Fixing one pays its WHOLE size, not
the missing fraction, so the ranking is by size and the fuzzy score is
shown to say how far off it is.

Game Code only, because that is the figure being chased. Every count
states its denominator.
"""
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
REPORT = ROOT / "build/R8IE78/report.json"
if not REPORT.is_file():
    print("%s does not exist -- run `python configure.py && ninja` first."
          % REPORT)
    sys.exit(1)

R = json.loads(REPORT.read_text(encoding="utf-8"))
floor = float(sys.argv[1]) if len(sys.argv) > 1 else 0.0

rows = []
game_units = 0
for u in R["units"]:
    if "game" not in (u.get("metadata") or {}).get("progress_categories", []):
        continue
    game_units += 1
    for f in (u.get("functions") or []):
        p = f.get("fuzzy_match_percent", 0.0)
        if 0.0 < p < 100.0 and p >= floor:
            rows.append((int(f["size"]), p, u["name"], f["name"]))

rows.sort(reverse=True)
total = sum(r[0] for r in rows)
print("%d near-miss function(s) in %d game unit(s), %d byte(s) in total"
      % (len(rows), game_units, total))
print("(a near miss counts ZERO until it lands, so its whole size is the"
      " payout)")
print()
for size, p, unit, name in rows[:40]:
    print("  %6d B  %6.2f%%  %-42s %s"
          % (size, p, unit.split("/")[-1], name[:64]))
if len(rows) > 40:
    print("  ... %d more, %d byte(s)"
          % (len(rows) - 40, sum(r[0] for r in rows[40:])))
