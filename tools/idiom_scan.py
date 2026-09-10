"""Which retail functions carry a given instruction idiom?

    python tools/idiom_scan.py aim          # the 28-byte two-return shape
    python tools/idiom_scan.py temp         # cr-set ; li rX,0 ; conditional branch
    python tools/idiom_scan.py --words 2c000000/ffffffff 38000000/fc1fffff 41820000/ff800000
    python tools/idiom_scan.py --check      # the presets must find their known members

A near miss whose bytes have a recognisable SHAPE -- a temp initialised
between a compare and its branch, two returns each with their own blr --
is best answered by a MATCHED function with the same shape, because its
source is the spelling. This asks the image for every function holding
the idiom, so the matched one can be found and read. It found
zWallNetGroup::GetWallNet for HitBuffFrontCheck's temp, and showed that
AimPuckCheck's seven-word shape occurs exactly twice in 23,338 functions.

A pattern is a list of word/mask pairs matched at every offset of every
retail function; `--whole` requires the function to be exactly the
pattern's length. Register fields are masked by giving a mask that
clears them. Bodies come from unitcmp.retail(), so masks and names are
the ones every other tool uses.
"""

import argparse
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import unitcmp  # noqa: E402

PRESETS = {
    # load r0 ; cmpwi r0,0 ; bne +0xc ; li r3,0 ; blr ; li r3,1 ; blr
    "aim": dict(whole=True, words=[
        (0x80030000, 0xf7ff0000),   # lwz/lbz r0,X(r3)  (0x80.. or 0x88..)
        (0x2c000000, 0xffffffff),
        (0x4082000c, 0xffffffff),
        (0x38600000, 0xffffffff),
        (0x4e800020, 0xffffffff),
        (0x38600001, 0xffffffff),
        (0x4e800020, 0xffffffff)]),
    # something setting cr0 ; li rX,0 ; a conditional branch
    "temp": dict(whole=False, words=[
        (0x54000001, 0xfc000001),   # rlwinm.  (or see ALT below)
        (0x38000000, 0xfc1fffff),   # li rX,0
        (0x40000000, 0xfc000000)]), # bc
}
# `temp` accepts cmpwi as the cr-setter too; a preset is a list of
# alternatives for its first word.
ALT_FIRST = {"temp": [(0x2c000000, 0xffe00000)]}

KNOWN = {
    "aim": ["AimPuckCheck__19zSBPlayerPuckAttackFP15xAnimTransitionP11xAnimSingle",
            "AimPuckCheck__22zBoardPlayerPuckAttackFP15xAnimTransitionP11xAnimSingle"],
    "temp": ["HitBuffFrontCheck__12zPlayerHitSBFP15xAnimTransitionP11xAnimSingle",
             "HitBuffBackCheck__12zPlayerHitSBFP15xAnimTransitionP11xAnimSingle",
             "GetWallNet__13zWallNetGroupCFi"],
}


def words_of(body):
    n = len(body) - len(body) % 4
    return [struct.unpack(">I", body[i:i + 4])[0] for i in range(0, n, 4)]


def matches_at(w, i, pat, alt_first):
    if i + len(pat) > len(w):
        return False
    v, m = pat[0]
    first_ok = (w[i] & m) == v or any((w[i] & am) == av for av, am in alt_first)
    if not first_ok:
        return False
    return all((w[i + k] & m) == v for k, (v, m) in enumerate(pat) if k)


def scan(pat, whole, alt_first=()):
    """-> [(address, name, nwords, offset)] over every retail function."""
    bodies = unitcmp.retail()
    byname, _byaddr = unitcmp.retail_addrs()
    hits = []
    for name, v in bodies.items():
        w = words_of(v[0])
        if whole and len(w) != len(pat):
            continue
        for i in range(len(w) - len(pat) + 1):
            if matches_at(w, i, pat, alt_first):
                hits.append((byname.get(name, 0), name, len(w), i))
                break
    return sorted(hits), len(bodies)


def parse_words(specs):
    out = []
    for s in specs:
        v, _, m = s.partition("/")
        out.append((int(v, 16), int(m, 16) if m else 0xffffffff))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("preset", nargs="?", choices=sorted(PRESETS))
    ap.add_argument("--words", nargs="+", help="hex word[/hex mask] ...")
    ap.add_argument("--whole", action="store_true",
                    help="the function must be exactly the pattern's length")
    ap.add_argument("--max-words", type=int, default=0,
                    help="list only functions of at most this many words")
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()

    if args.check:
        bad = 0
        for name, p in PRESETS.items():
            hits, total = scan(p["words"], p["whole"], ALT_FIRST.get(name, ()))
            names = {h[1] for h in hits}
            missing = [k for k in KNOWN[name] if k not in names]
            print("  %-5s %d hit(s) of %d function(s); %d of %d known member(s) found"
                  % (name, len(hits), total, len(KNOWN[name]) - len(missing),
                     len(KNOWN[name])))
            for k in missing:
                print("    MISSING %s" % k)
            bad += len(missing)
        if bad:
            sys.exit("idiom_scan: %d known member(s) not found -- refusing to "
                     "report anything else" % bad)
        print("  idiom_scan: every known member found")
        return 0

    if args.preset:
        p = PRESETS[args.preset]
        pat, whole, alt = p["words"], p["whole"], ALT_FIRST.get(args.preset, ())
    elif args.words:
        pat, whole, alt = parse_words(args.words), args.whole, ()
    else:
        ap.error("give a preset or --words")

    hits, total = scan(pat, whole, alt)
    shown = [h for h in hits if not args.max_words or h[2] <= args.max_words]
    print("  %d of %d retail function(s) carry the idiom; %d listed%s"
          % (len(hits), total, len(shown),
             " (at most %d words)" % args.max_words if args.max_words else ""))
    for addr, name, n, i in shown:
        print("  %08x  %4d words  at word %3d  %s" % (addr, n, i, name))
    print("  A hit is a shape, not a match: read the function whose SOURCE "
          "exists and is byte-identical, and its spelling is the lever.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
