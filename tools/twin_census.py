"""Which unmatched game functions are BYTE-TWINS of ones already written.

    python tools/twin_census.py [--limit N] [--min-bytes N] [--unsolved]

`shape_census.py` asks what the unmatched functions share with EACH
OTHER, by primary opcode, so that a generator can be written for the
biggest shape. This asks a different question, and the difference is the
whole point: put the matched and the unmatched in ONE clustering, on a
much stricter key, and look for clusters that already contain a solved
member.

A cluster like that is a TRANSPLANT rather than a decompilation. The
source is written -- in some other unit, for some other type -- and what
differs between the members is which symbols they name and which
constants they carry. `Sext::xGroupAsset::Create` was written that way
and matched on the first compile: eight other assets in the tree already
had the shape, and only the type, its size and which init runs changed.

THE KEY IS EVERY INSTRUCTION WITH ITS REGISTERS, and only the fields a
relocation or an address computation can move are blanked: the
displacement of a branch, and the low sixteen bits of the
immediate-carrying forms. That is deliberately stricter than an opcode
signature -- it will not put two functions together because they happen
to load and add in the same order -- and deliberately looser than the
bytes, because the members are the same source with different names.

What the key blanks it also HIDES, so a cluster is a lead and not a
proof: two members can differ in a size, an offset or a constant that
the source has to get right. That is the work. What the cluster removes
is the part that costs days, which is not knowing the shape at all.

Reads the retail image through disasm.py and report.json for what is
matched, so nothing here is guessed. Every count states its denominator,
and a function whose bytes cannot be read is dropped and reported rather
than counted as anything.
"""

import argparse
import json
import struct
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
REPORT = ROOT / "build/R8IE78/report.json"
sys.path.insert(0, str(ROOT / "tools"))
import disasm as D                                        # noqa: E402

# The primary opcodes whose low sixteen bits are an immediate that a
# relocation or an address computation can change: the arithmetic and
# logical immediate forms, and every load and store with a displacement.
IMM16 = set([7, 8, 10, 11, 12, 13, 14, 15, 24, 25, 26, 27, 28, 29]
            + list(range(32, 56)))


def skeleton(data):
    """The instruction skeleton: opcodes and registers, addresses blanked."""
    out = bytearray()
    for i in range(0, len(data), 4):
        (w,) = struct.unpack(">I", data[i:i + 4])
        op = w >> 26
        if op == 18:                       # b / bl / ba / bla
            w &= 0xFC000003                # keep AA and LK, drop the target
        elif op == 16:                     # bc
            w &= 0xFFFF0003                # keep BO and BI, drop the target
        elif op in IMM16:
            w &= 0xFFFF0000                # keep the opcode and the registers
        out += struct.pack(">I", w)
    return bytes(out)


def population():
    """-> (names in the game category, matched names, name -> source path)."""
    if not REPORT.exists():
        sys.exit("twin_census: %s is missing -- run `ninja` first" % REPORT)
    rep = json.loads(REPORT.read_text(encoding="utf-8"))
    game, matched, source = set(), set(), {}
    for u in rep["units"]:
        md = u.get("metadata") or {}
        if "game" not in (md.get("progress_categories") or []):
            continue
        for f in (u.get("functions") or []):
            name = f.get("name")
            if not name:
                continue
            game.add(name)
            source[name] = md.get("source_path")
            if float(f.get("fuzzy_match_percent") or 0) >= 100.0:
                matched.add(name)
    return game, matched, source


def clusters(game, matched):
    """-> (skeleton -> [(name, size, solved)], functions read, unread)."""
    raw, secs, funcs, _objs = D.load()
    out, read, unread = defaultdict(list), 0, 0
    for addr, (name, size) in funcs.items():
        if name not in game or not size or size % 4:
            continue
        data = D.read(raw, secs, addr, size)
        if data is None or len(data) != size:
            unread += 1
            continue
        read += 1
        out[skeleton(data)].append((name, size, name in matched))
    return out, read, unread


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--limit", type=int, default=20,
                    help="how many clusters to print (default 20)")
    ap.add_argument("--min-bytes", type=int, default=0,
                    help="skip clusters carrying fewer unsolved bytes")
    ap.add_argument("--unsolved", action="store_true",
                    help="name every unsolved member, not the first three")
    args = ap.parse_args()

    game, matched, source = population()
    print("%d game function(s) named by report.json, %d of them matched"
          % (len(game), len(matched)))

    cl, read, unread = clusters(game, matched)
    print("%d clustered, %d could not be read, %d distinct skeleton(s)"
          % (read, unread, len(cl)))

    mixed = []
    for members in cl.values():
        solved = [m for m in members if m[2]]
        todo = [m for m in members if not m[2]]
        if solved and todo:
            mixed.append((sum(m[1] for m in todo), solved, todo))
    mixed.sort(key=lambda x: -x[0])
    mixed = [m for m in mixed if m[0] >= args.min_bytes]

    print("")
    print("%d cluster(s) hold both a solved function and an unsolved one: "
          "%d unsolved member(s), %d bytes"
          % (len(mixed), sum(len(m[2]) for m in mixed),
             sum(m[0] for m in mixed)))
    print("")

    for nbytes, solved, todo in mixed[:args.limit]:
        print("%d unsolved / %d solved   %d bytes, %d B each"
              % (len(todo), len(solved), nbytes, todo[0][1]))
        for name, _size, _m in solved[:2]:
            print("   written  %-58s  %s" % (name[:58], source.get(name)))
        shown = sorted(todo) if args.unsolved else sorted(todo)[:3]
        for name, size, _m in shown:
            print("   todo     %-58s  %s"
                  % (name[:58], source.get(name) or "(no source file yet)"))
        if not args.unsolved and len(todo) > 3:
            print("   ...      and %d more" % (len(todo) - 3))
        print("")


if __name__ == "__main__":
    main()
