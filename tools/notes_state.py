"""Rewrite NOTES.md's "State at time of writing" block from the tools.

    python tools/notes_state.py [--check]

That block has drifted three times: it said 390 functions when there were
491, 86 hand-written when there were 92, and a percentage two commits old.
It is the first thing anyone reads and the last thing anyone updates, which
is the definition of a figure that should not be typed.

Every number here comes from somewhere that owns it -- report.json for the
category totals, written_vs_generated.py for the split it refuses to get
wrong, dwarf_data_carve.py for the data tier -- and `--check` exits
non-zero when the file disagrees, so a stale block can fail a build rather
than mislead a reader.
"""

import argparse
import contextlib
import io
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
REPORT = ROOT / "build/R8IE78/report.json"
NOTES = ROOT / "NOTES.md"
NL = chr(10)

sys.path.insert(0, str(ROOT / "tools"))
import written_vs_generated                               # noqa: E402


def comma(v):
    return format(int(v), ",")


def data_tier():
    import dwarf_data_carve
    buf, argv = io.StringIO(), sys.argv
    sys.argv = ["dwarf_data_carve.py", "--survey"]
    try:
        with contextlib.redirect_stdout(buf):
            dwarf_data_carve.main()
    except SystemExit:
        pass
    finally:
        sys.argv = argv
    m = re.search(r"(\d+) unit\(s\) could take their data", buf.getvalue())
    if not m:
        sys.exit("notes_state: dwarf_data_carve.py did not report its total")
    return int(m.group(1))


def block():
    if not REPORT.exists():
        sys.exit("notes_state: %s is missing -- run ninja first." % REPORT)
    rep = json.loads(REPORT.read_text(encoding="utf-8"))
    g = next(c["measures"] for c in rep["categories"]
             if c["name"] == "Game Code")
    s = written_vs_generated.split()
    wu, wf, wb = s["written"]
    _gu, gf, _gb = s["generated"]
    dunits = sum(1 for u in rep["units"]
                 if "game" in (u.get("metadata", {})
                               .get("progress_categories") or [])
                 and int(u["measures"].get("complete_data", 0) or 0))
    dbytes = sum(int(u["measures"].get("complete_data", 0) or 0)
                 for u in rep["units"]
                 if "game" in (u.get("metadata", {})
                               .get("progress_categories") or []))
    am = rep["measures"]
    return NL.join([
        "```",
        "Game Code:  %d of %d files complete  %s / %s bytes  %s / %s fn"
        % (g.get("complete_units", 0), g["total_units"],
           comma(g["matched_code"]), comma(g["total_code"]),
           comma(g["matched_functions"]), comma(g["total_functions"])),
        "            %.4f%% of game code"
        % (100.0 * int(g["matched_code"]) / int(g["total_code"])),
        "",
        "Of those %s functions, %s are GENERATED -- machine-recognised"
        % (comma(g["matched_functions"]), comma(gf)),
        "shapes, not one of which is decompiling. They are real matched",
        "functions and the offsets and constants are recovered fact, but a",
        "count of them is not a count of decompiled code. HAND-WRITTEN IS",
        "%s, across %d units and %s bytes, and that is the figure to"
        % (comma(wf), wu, comma(wb)),
        "compare against earlier ones.",
        "",
        "Data:       %d unit(s) carry their own, %s bytes; %d more could"
        % (dunits, comma(dbytes), data_tier()),
        "All:        %.2f%% matched              main.dol reproduces byte "
        "for byte" % (100.0 * int(am["matched_code"])
                      / int(am["total_code"])),
        "```",
        "",
        "Every number above is written by `python tools/notes_state.py`,",
        "which reads report.json, `written_vs_generated.py` and",
        "`dwarf_data_carve.py`. Do not edit it by hand -- it has drifted",
        "three times, always in the flattering direction. `--check` fails",
        "when it is stale.",
    ])


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()

    text = NOTES.read_text(encoding="utf-8")
    m = re.search(r"(## State at time of writing" + NL + NL + r").*?"
                  r"(" + NL + r"The other categories)", text, re.S)
    if not m:
        sys.exit("notes_state: NOTES.md no longer has the state block where "
                 "expected -- refusing to guess where to write it.")
    want = m.group(1) + block() + m.group(2)
    if text[m.start():m.end()] == want:
        print("  the state block is current")
        return 0
    if args.check:
        print("  NOTES.md's state block is STALE. Run "
              "`python tools/notes_state.py`.")
        return 1
    NOTES.write_text(text[:m.start()] + want + text[m.end():],
                     encoding="utf-8")
    print("  NOTES.md state block rewritten")
    return 0


if __name__ == "__main__":
    sys.exit(main())
