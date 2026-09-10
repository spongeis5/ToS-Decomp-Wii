"""Compile several spellings of ONE source file and score each with unitcmp.

    python tools/sweep_src.py <src> <unit> <variant.cpp> [more.cpp ...]

`unitcmp.py` answers in about half a second, and the loop around it has
always been by hand: edit, run, read, undo. That makes a four-variant
sweep a five-minute job and a forty-eight-variant sweep something nobody
does -- which is why the notes beside several near misses say "eight
combinations measured" and stop there.

This puts each candidate file in place, runs `unitcmp` on the unit, reads
the per-function word counts out of it, and restores the original whether
it passed, failed or raised. The original is restored in a `finally`, and
its bytes are compared afterwards: a sweep that leaves the tree modified
is a sweep that has silently rewritten the thing being measured.

It reports differing words per function, and it prints the BASELINE first
so a variant can be read as better or worse rather than as a bare number.
A variant that fails to compile is printed as FAILED with the compiler's
first error, never as a score -- a spelling that did not build has not
been measured.

This does NOT decide whether anything matches: `unitcmp` does that, and
`ninja` is the oracle. Nothing here is a reason to flip a unit Matching.
"""

import argparse
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

ROW = re.compile(r"^\s*(DIFFER|MATCH|EXTRA|MISSING)\s+(\d+)\s+(\S+)\s*(.*)$")
DIFF = re.compile(r"(\d+) of (\d+) words differ")


def score(unit, extra=None):
    """-> (rows, raw text). A row is (state, size, name, differ, total)."""
    cmd = [sys.executable, "tools/unitcmp.py", unit]
    if extra:
        cmd += ["--extra", extra]
    r = subprocess.run(cmd,
                       cwd=str(ROOT), capture_output=True, text=True)
    text = r.stdout + r.stderr
    rows = []
    for line in text.splitlines():
        m = ROW.match(line)
        if not m:
            continue
        state, size, name, rest = m.groups()
        d = DIFF.search(rest)
        rows.append((state, int(size), name,
                     int(d.group(1)) if d else 0,
                     int(d.group(2)) if d else 0))
    return rows, text


def show(tag, rows, base=None):
    if not rows:
        print("  %-22s NO FUNCTION ROWS -- not a score" % tag)
        return
    tot = sum(r[3] for r in rows)
    bits = []
    for state, size, name, d, t in sorted(rows, key=lambda r: -r[1]):
        short = name.split("__", 1)[0][:22]
        if state == "MATCH":
            bits.append("%s ok" % short)
        elif state == "DIFFER":
            bits.append("%s %d/%d" % (short, d, t))
        else:
            bits.append("%s %s" % (short, state))
    delta = ""
    if base is not None:
        delta = "  (%+d)" % (tot - base)
    print("  %-22s %4d differing word(s)%s" % (tag, tot, delta))
    for b in bits:
        print("      %s" % b)
    return tot


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("src", help="the source file the unit builds from")
    ap.add_argument("unit", help="the unit name for unitcmp")
    ap.add_argument("variants", nargs="+", help="candidate .cpp files")
    ap.add_argument("--extra", help="extra cflags for every compile here, "
                    "quoted; echoed by unitcmp on every run")
    args = ap.parse_args()

    src = Path(args.src)
    if not src.is_file():
        sys.exit("sweep_src: %s does not exist" % src)
    original = src.read_bytes()

    print("  baseline is the file on disk; %d variant(s) to try"
          % len(args.variants))
    rows, _t = score(args.unit, args.extra)
    base = show("BASELINE", rows)
    if base is None:
        sys.exit("sweep_src: the baseline produced no function rows. "
                 "Measuring variants against nothing is not a sweep.")
    print("")

    results = []
    try:
        for v in args.variants:
            p = Path(v)
            if not p.is_file():
                print("  %-22s NOT FOUND" % p.name)
                continue
            shutil.copyfile(p, src)
            rows, text = score(args.unit, args.extra)
            if not rows:
                first = next((ln.strip() for ln in text.splitlines()
                              if "rror" in ln or "Error" in ln), "")
                print("  %-22s FAILED: %s" % (p.name, first[:70]))
                results.append((None, p.name))
                continue
            tot = show(p.name, rows, base)
            results.append((tot, p.name))
    finally:
        src.write_bytes(original)
        if src.read_bytes() != original:
            sys.exit("sweep_src: FAILED TO RESTORE %s. The tree is "
                     "modified; do not trust anything above." % src)

    print("")
    print("  restored %s to its original %d bytes" % (src, len(original)))
    ok = [r for r in results if r[0] is not None]
    print("  %d of %d variant(s) compiled" % (len(ok), len(results)))
    if ok:
        best = min(ok)
        print("  best is %s at %d differing word(s); baseline was %d"
              % (best[1], best[0], base))
        if best[0] >= base:
            print("  NOTHING BEAT THE BASELINE. The file on disk is "
                  "unchanged and still the best measured.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
