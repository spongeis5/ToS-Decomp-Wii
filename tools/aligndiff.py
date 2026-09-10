"""Is a big word count a big difference, or one missing instruction?

    python tools/aligndiff.py <unit> [name substring]

`unitcmp` says how many words differ. That number is read as a measure of
how far the source is from retail's, and for a SHIFT it is nothing of the
kind: xModelUpdatePartsVis differed by 74 of 83 words and the whole of it
was ONE missing instruction at word 11, with every word after it correct
but offset by one. A rule of thumb like "more than ten words means the
control flow is wrong" would have thrown that function away, and it was
336 bytes.

So this asks the question the count cannot: is there a k such that our
word n equals retail's word n+k over a long run? If there is, the report
is not "74 words differ" but

    ALIGNED AFTER A SHIFT: ours[11..82] == retail[12..83]
    We are 1 instruction SHORT at word 11. Retail has 4800010c there.

which names the one place to look. If there is no such k, the
differences really are scattered and the count means what it appears to.

Relocated words are excluded from the comparison, exactly as unitcmp
excludes them, because an unlinked object holds 0 where retail holds the
resolved value. The number excluded is printed: a run that matched only
because everything in it was masked is not an aligned run.
"""

import argparse
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

HEAD = re.compile(r"^\s*(DIFFER|MATCH)\s+(\d+)\s+(\S+)")
ROW = re.compile(r"^\s+(\d+)\s+([0-9a-f]{8})\s+([0-9a-f]{8})\s*(.*)$")


def blocks(unit):
    """-> [(name, size, [(idx, ours, retail, masked)])] from unitcmp -v."""
    r = subprocess.run([sys.executable, "tools/unitcmp.py", unit, "-v"],
                       cwd=str(ROOT), capture_output=True, text=True)
    out, cur = [], None
    for line in (r.stdout + r.stderr).splitlines():
        m = HEAD.match(line)
        if m:
            cur = (m.group(3), int(m.group(2)), [])
            out.append(cur)
            continue
        if cur is None:
            continue
        m = ROW.match(line)
        if m:
            cur[2].append((int(m.group(1)), m.group(2), m.group(3),
                           "reloc" in m.group(4)))
    if not out:
        sys.exit("aligndiff: unitcmp printed no function block for %r. "
                 "That is a failure to run it, not a unit with no "
                 "functions.\n%s" % (unit, (r.stdout + r.stderr)[-800:]))
    return out


def best_shift(rows, limit=6):
    """-> (k, start, length, masked) for the longest aligned run, k != 0."""
    ours = {i: (a, m) for i, a, _b, m in rows}
    theirs = {i: (b, m) for i, _a, b, m in rows}
    best = (0, 0, 0, 0)
    for k in list(range(1, limit + 1)) + list(range(-1, -limit - 1, -1)):
        run = masked = 0
        start = None
        for i in sorted(ours):
            j = i + k
            if j not in theirs:
                run, start = 0, None
                continue
            a, am = ours[i]
            b, bm = theirs[j]
            if a == b or am or bm:
                if start is None:
                    start = i
                run += 1
                if am or bm:
                    masked += 1
                if run - masked > best[2] - best[3]:
                    best = (k, start, run, masked)
            else:
                run, start = 0, None
    return best


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("unit")
    ap.add_argument("what", nargs="?", help="a function name substring")
    args = ap.parse_args()

    shown = 0
    for name, size, rows in blocks(args.unit):
        if args.what and args.what not in name:
            continue
        if not rows:
            continue
        real = [(i, a, b) for i, a, b, m in rows if a != b and not m]
        if not real:
            continue
        shown += 1
        print("")
        print("  %s  (%d bytes, %d word(s))" % (name, size, len(rows)))
        print("    %d word(s) differ and are NOT masked by a relocation"
              % len(real))

        k, start, run, masked = best_shift(rows)
        cover = run - masked
        frac = 100.0 * run / max(1, len(rows))
        if k and cover >= max(6, len(rows) // 4):
            print("")
            label = ("ALIGNED AFTER A SHIFT" if frac >= 50.0
                     else "PARTIALLY ALIGNED AFTER A SHIFT")
            print("    %s: our word[n] == retail's word[n%+d] over %d "
                  "word(s) from %d, %.0f%% of the function (%d masked)"
                  % (label, k, run, start, frac, masked))
            if frac < 50.0:
                print("    LESS THAN HALF the function aligns, so this is a "
                      "shift AND something else. Fixing the one place below "
                      "will not finish it.")
            if k > 0:
                print("    We are %d instruction(s) SHORT at word %d."
                      % (k, start))
                want = [b for i, _a, b, _m in rows if start <= i < start + k]
                extra = []
                print("    Retail has %s there and we do not."
                      % " ".join(want))
            else:
                print("    We have %d instruction(s) TOO MANY at word %d."
                      % (-k, start))
                extra = [a for i, a, _b, _m in rows if start <= i < start - k]
                want = []
                print("    We emit %s there and retail does not."
                      % " ".join(extra))
            # A shift whose missing words are `lis` is the float-base
            # blocker wearing a shift's clothes: retail spells a `lis`
            # per float literal and we reach them all off one base, so
            # we come out N instructions short with everything after
            # aligned. NOTES.md's float-base section owns that, and no
            # source spelling has ever moved it. zDecal::init looked
            # like 3 missing statements and was this.
            # `lis` is 3c/3d, `lfs` is c0/c4, `lfd` is c8/cc.
            floaty = [w for w in (want if k > 0 else extra)
                      if w[:2] in ("3c", "3d", "c0", "c4", "c8", "cc")]
            if floaty:
                print("    %d of the %d word(s) are a `lis` or a float load "
                      "(%s)." % (len(floaty), abs(k), " ".join(floaty)))
                print("    A shift made of those is usually the FLOAT-BASE "
                      "blocker wearing a shift's clothes, NOT a missing "
                      "statement: retail spells a `lis` per float literal "
                      "and we reach them all off one base, so we come out "
                      "N instructions short with everything after aligned. "
                      "Check the surrounding words for `lfs fN,off(rBASE)` "
                      "against retail's `lis`+`lfs` pairs before writing "
                      "any source. See NOTES.md, 'THE FLOAT BASE IS "
                      "SHARED'. zDecal::init looked like 3 missing "
                      "statements and was exactly this.")
            if frac >= 50.0:
                print("    So this is ONE place to look, not %d. The word "
                      "count measures alignment, not distance." % len(real))
            else:
                print("    One place to look of more than one; the word "
                      "count still overstates the distance.")
        else:
            print("    No shift explains it: the longest aligned run at any "
                  "offset from -6 to +6 covers %d unmasked word(s), which is "
                  "not enough to call it a shift. The differences are "
                  "scattered and the count means what it looks like."
                  % max(0, cover))
            for i, a, b in real[:12]:
                print("      word %-4d ours=%s retail=%s" % (i, a, b))
            if len(real) > 12:
                print("      ... and %d more" % (len(real) - 12))

    if shown == 0:
        print("  no function of %s has an unmasked differing word%s"
              % (args.unit, " matching %r" % args.what if args.what else ""))
    return 0


if __name__ == "__main__":
    sys.exit(main())
