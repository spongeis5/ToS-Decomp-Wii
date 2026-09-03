"""Which untouched units are actually REACHABLE as a fragment.

    python tools/unit_triage.py [--bytes N] [--functions N] [--all]

next_functions.py ranks what is left; this says which of it can be written
at all. Two things make a unit unreachable before a line of source exists,
and both are visible in the image:

  * a function that loads FOUR OR MORE distinct float literals. Past 32 KB
    of .rodata mwcc forms one `addis` base for them where retail spells a
    `lis` per literal, and no compiler setting tried moves that line -- see
    the float-base section of NOTES.md. Three or fewer is fine.
  * a symbol in a unity unit's unnamed namespace. `@unnamed@WAD00_cpp@`
    is not a name any C++ identifier can produce from another file, so a
    fragment can neither DEFINE one nor CALL one. Defining is fatal;
    calling is too, because the branch would have to name it.

Both are counted per unit and printed beside the size, so a batch can be
picked without compiling anything. A unit that only READS such a symbol's
data is not blocked: the fragment can declare its own static and the text
still matches, which is why only definitions and branch targets count here.

Reads report.json for what is already matched and disassembles each
candidate once; no compile, and nothing is written.
"""

import argparse
import json
import os
import re
import subprocess
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
REPORT = ROOT / "build/R8IE78/report.json"

SYMBOL = re.compile(r"^[A-Za-z_@][A-Za-z0-9_$<>,:*&()@.-]*__[A-Za-z0-9_$<>,:*&()@.-]*$")
LITERAL = re.compile(r"= ([0-9A-F]{8})\s+(@[0-9]+)")


def candidates(max_bytes, max_functions, everything):
    if not REPORT.exists():
        sys.exit("unit_triage: %s is not there -- run ninja first" % REPORT)

    report = json.load(open(REPORT))
    out = []
    for unit in report.get("units", []):
        name = unit["name"]
        if not name.startswith("main/SB/"):
            continue
        fns = [f for f in unit.get("functions", [])
               if not f["name"].startswith("pad_")]
        if not fns:
            continue
        if not everything and any(f.get("fuzzy_match_percent") == 100.0
                                  for f in fns):
            continue
        rel = name.replace("main/", "")
        if not everything and os.path.exists(ROOT / ("src/" + rel + ".cpp")):
            continue
        total = sum(int(f.get("size", 0)) for f in fns)
        if len(fns) <= max_functions and total <= max_bytes:
            out.append((total, len(fns), rel))
    out.sort()
    return out


def blockers(unit):
    """(worst distinct literal count, defines anon, calls anon) or None
    when the disassembler printed nothing -- which is not zero."""
    run = subprocess.run([sys.executable, str(ROOT / "tools/disasm.py"),
                          "--unit", unit + ".cpp"],
                         cwd=str(ROOT), capture_output=True, text=True)
    text = run.stdout + run.stderr
    if not text.strip():
        return None

    per = defaultdict(set)
    name = "?"
    defines = calls = False
    for line in text.splitlines():
        stripped = line.strip()
        if SYMBOL.match(stripped):
            name = stripped
            if "@unnamed@" in stripped:
                defines = True
            continue
        if "-> " in line and "@unnamed@" in line:
            calls = True
        if " lfs " in line or " lfd " in line:
            hit = LITERAL.search(line)
            if hit:
                per[name].add(hit.group(2))
    return max((len(v) for v in per.values()), default=0), defines, calls


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--bytes", type=int, default=1200)
    ap.add_argument("--functions", type=int, default=8)
    ap.add_argument("--all", action="store_true",
                    help="units with source, and matched ones, as well")
    args = ap.parse_args()

    rows = candidates(args.bytes, args.functions, args.all)
    print("  %d unit(s) at most %d bytes and %d function(s):"
          % (len(rows), args.bytes, args.functions))

    clear = 0
    for total, count, unit in rows:
        answer = blockers(unit)
        if answer is None:
            print("  %-52s %5d B %2d fn  disasm printed nothing" % (unit, total, count))
            continue
        worst, defines, calls = answer
        why = []
        if worst >= 4:
            why.append("%d literals" % worst)
        if defines:
            why.append("defines anon")
        if calls:
            why.append("calls anon")
        if not why:
            clear += 1
        print("  %-52s %5d B %2d fn  %s"
              % (unit, total, count, ", ".join(why) if why else "clear"))

    print("  %d of %d clear of both blockers" % (clear, len(rows)))


if __name__ == "__main__":
    main()
