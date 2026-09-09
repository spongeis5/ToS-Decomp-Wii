"""Write the README's progress badges from the build's own report.

    python tools/badges.py            regenerate progress/*.json
    python tools/badges.py --check    fail if any file is out of date

The badges the README carried before this pointed at decomp.dev's copy
of bfbbdecomp/tssm -- the UPSTREAM repository -- so they rendered
upstream's progress under this fork's name. decomp.dev has no entry for
this fork (its API returns 404), so pointing the same URLs at it would
render broken images instead. Either way the number on the page would
not be the number the build measured.

So the badges are generated here, from `build/R8IE78/report.json`, the
same file `configure.py` writes and `report.py` reads. Each one becomes
a shields.io endpoint document under `progress/`, and the README links
img.shields.io at the raw file. That keeps the rule the rest of the
project keeps: a figure in a document is generated or it rots.

A missing or unreadable report is an ERROR, not an empty badge. A
percentage that could not be measured is not zero, and a badge reading
0.00% would be believed.
"""

import argparse
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
REPORT = ROOT / "build/R8IE78/report.json"
OUT = ROOT / "progress"

# (filename, label, category id or None for the whole image, measure)
#
# `None` is the whole image -- report.json keeps those measures at the
# top level rather than in a category, which is what decomp.dev calls
# `category=all`.
BADGES = [
    ("game_code", "Game Code", "game", "matched_code_percent"),
    ("game_fuzzy", "Game Fuzzy", "game", "fuzzy_match_percent"),
    ("image_code", "Image", None, "matched_code_percent"),
    ("image_fuzzy", "Image Fuzzy", None, "fuzzy_match_percent"),
    ("linked", "Linked", None, "complete_code_percent"),
    ("data", "Data", None, "matched_data_percent"),
]

COLOR = "007ec6"


def measures(report, category):
    """The measure block for one category, or the whole image."""
    if category is None:
        return report["measures"]
    for c in report.get("categories", []):
        if c.get("id") == category:
            return c["measures"]
    raise SystemExit(
        "no category %r in %s -- the report has %s"
        % (category, REPORT,
           ", ".join(repr(c.get("id")) for c in report.get("categories", []))))


def build():
    if not REPORT.is_file():
        raise SystemExit(
            "%s does not exist -- run `python configure.py && ninja` first. "
            "Refusing to write badges from a report that was not measured."
            % REPORT)

    report = json.loads(REPORT.read_text(encoding="utf-8"))
    want = {}
    for name, label, category, measure in BADGES:
        m = measures(report, category)
        if measure not in m:
            raise SystemExit(
                "%s has no measure %r for category %r -- it has %s"
                % (REPORT, measure, category, ", ".join(sorted(m))))
        want[name] = {
            "schemaVersion": 1,
            "label": label,
            "message": "%.2f%%" % m[measure],
            "color": COLOR,
        }
    return want


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--check", action="store_true",
                    help="fail if any badge file is out of date")
    args = ap.parse_args()

    want = build()
    OUT.mkdir(exist_ok=True)

    stale = []
    for name, doc in sorted(want.items()):
        path = OUT / ("%s.json" % name)
        text = json.dumps(doc, indent=2) + "\n"
        old = path.read_text(encoding="utf-8") if path.is_file() else None
        if old == text:
            continue
        stale.append((name, doc["message"]))
        if not args.check:
            path.write_text(text, encoding="utf-8")

    if args.check and stale:
        for name, msg in stale:
            print("  STALE %-12s should be %s" % (name, msg))
        print()
        print("  %d of %d badge(s) out of date -- run `python tools/badges.py`"
              % (len(stale), len(want)))
        return 1

    for name, doc in sorted(want.items()):
        print("  %-12s %-12s %s" % (name, doc["label"], doc["message"]))
    print()
    print("  %d badge(s) written of %d, %d changed"
          % (len(want), len(want), len(stale)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
