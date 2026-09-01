"""Reproduce the string-pool PREFIX a unit inherits from its unity build.

Retail compiled the game as unity translation units -- the WAD blobs --
and each one got a single `@stringBase0` pool, filled in order of first
appearance across every file it included. A function reaches a string as
`addi rD, rBASE, K` with K baked into the instruction, so K is the
string's offset in THAT pool: zNPCUPGeneric's `IDLE` is at +2982
because thirty files' worth of strings precede it, and `IDLE` itself was
first used by an earlier file.

dtk cuts the blob back into per-file units, so a unit compiled on its
own starts an empty pool and every K comes out small and wrong. This
writes `<unit>.pool.h`: a file-scope table holding, in pool order, every
string that precedes the unit's own first new string. Included at the
top of the unit, it puts the same strings in the same order ahead of
the unit's, and `-str reuse` then folds the unit's references onto them.

The table is DATA READ FROM THE IMAGE, not source: retail has no such
table, it has the files in front. It exists so a fragment can be
compared byte for byte; a fragment that is to be LINKED needs the unity
unit rebuilt instead. Every count below states its denominator.
"""
import argparse
import re
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import disasm as D                                       # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
CFG = ROOT / "config/R8IE78"
NL = chr(10)
BS = chr(92)


def units():
    """[(text_start, text_end, unit_name)] from splits.txt, sorted."""
    out, cur = [], None
    for ln in (CFG / "splits.txt").read_text(encoding="utf-8").splitlines():
        m = re.match(r"^(\S.*):\s*$", ln)
        if m:
            cur = m.group(1)
            continue
        m = re.match(r"^\s+\.text\s+start:0x([0-9A-Fa-f]+)\s+end:0x([0-9A-Fa-f]+)",
                     ln)
        if m and cur:
            out.append((int(m.group(1), 16), int(m.group(2), 16), cur))
    return sorted(out)


def pools():
    """[(addr, size)] of every @stringBase0 in symbols.txt."""
    out = []
    for ln in (CFG / "symbols.txt").read_text(encoding="utf-8").splitlines():
        m = re.match(r"^@stringBase0 = \.rodata:0x([0-9A-Fa-f]+); // "
                     r"type:object size:0x([0-9A-Fa-f]+)", ln)
        if m:
            out.append((int(m.group(1), 16), int(m.group(2), 16)))
    return sorted(out)


def pool_strings(raw, secs, base, size):
    data = D.read(raw, secs, base, size)
    out, i = [], 0
    while i < size:
        j = data.find(b"\0", i)
        if j < 0:
            break
        out.append((i, data[i:j]))
        i = j + 1
    return out


def first_referrers(raw, secs, funcs, base, size, lo, hi):
    """pool offset -> lowest function address in [lo,hi) that builds
    base+offset. Only functions in the TU are scanned."""
    first = {}
    for a, (nm, sz) in funcs.items():
        if not sz or not (lo <= a < hi):
            continue
        ws = struct.unpack(">" + "I" * (sz // 4), D.read(raw, secs, a, sz))
        regs = {}
        for w in ws:
            op = w >> 26
            if op == 15 and ((w >> 16) & 31) == 0:
                regs[(w >> 21) & 31] = (w & 0xFFFF) << 16
            elif op == 14:
                d, s = (w >> 21) & 31, (w >> 16) & 31
                imm = w & 0xFFFF
                imm = imm - 0x10000 if imm & 0x8000 else imm
                if s in regs:
                    v = (regs[s] + imm) & 0xFFFFFFFF
                    if base <= v < base + size:
                        off = v - base
                        if off not in first or a < first[off]:
                            first[off] = a
                    regs[d] = v
                else:
                    regs.pop(d, None)
            elif op == 18 and (w & 1):
                for r in [r for r in regs if r <= 12]:
                    del regs[r]
            elif op == 31:
                regs.pop((w >> 16) & 31, None)
            elif op in (32, 33, 34, 35, 40, 41, 42, 43, 46, 48, 50):
                regs.pop((w >> 21) & 31, None)
    return first


def c_string(b):
    out = []
    for ch in b:
        c = chr(ch)
        if c == '"' or c == BS:
            out.append(BS + c)
        elif c == chr(9):
            out.append(BS + "t")
        elif c == chr(10):
            out.append(BS + "n")
        elif 32 <= ch < 127:
            out.append(c)
        else:
            out.append(BS + "x%02X" % ch)
    return '"' + "".join(out) + '"'


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("unit", help="e.g. SB/GM/Engine/Game/zNPCUPGeneric.cpp")
    ap.add_argument("--check", action="store_true",
                    help="report; write nothing")
    args = ap.parse_args()

    us = units()
    me_s, me_e = next((s, e) for s, e, n in us if n == args.unit)
    raw, secs, funcs, objs = D.load()

    # The TU's pool is the @stringBase0 the UNIT'S OWN functions build.
    # The blob names do not bound a translation unit: dtk's WAD02 range
    # holds two pools, because a TU whose first file was fully recovered
    # has no remainder chunk at its head to carry the WADnn name.
    mine = None
    for base, size in pools():
        ref = first_referrers(raw, secs, funcs, base, size, me_s, me_e)
        if ref:
            if mine is not None:
                raise SystemExit("gen_poolprefix: %s builds two pools, "
                                 "%08X and %08X" % (args.unit, mine[0], base))
            mine = (base, size)
    if mine is None:
        raise SystemExit("gen_poolprefix: no function in %s builds a "
                         "@stringBase0; nothing to do" % args.unit)
    base, size = mine
    # Now every referrer of that pool, anywhere: they ARE the TU.
    ref = first_referrers(raw, secs, funcs, base, size, 0, 0xFFFFFFFF)
    tu_lo, tu_hi = min(ref.values()), max(ref.values())
    wads = sorted({n for s, _e, n in us if tu_lo <= s <= tu_hi
                   and re.search(r"/WAD\d+(_\d+)?\.cpp$", n)})
    wad = (wads[0].rsplit("/", 1)[1].split("_")[0] + ".cpp") if wads \
        else "(no WAD chunk)"
    lo, hi = tu_lo, tu_hi
    strings = pool_strings(raw, secs, base, size)
    by_off = {off: s for off, s in strings}

    # Strings this unit is the FIRST to reference; the prefix is
    # everything in the pool before the earliest of them.
    own = sorted(off for off, a in ref.items() if me_s <= a < me_e)
    if not own:
        raise SystemExit("gen_poolprefix: %s references no pool string; "
                         "nothing to do" % args.unit)
    cut = own[0]
    unresolved = [off for off in own if off not in by_off]
    if unresolved:
        raise SystemExit("gen_poolprefix: %d referenced offset(s) fall "
                         "inside a string, not at one: %s"
                         % (len(unresolved),
                            ", ".join("+%d" % o for o in unresolved)))
    prefix = [(off, s) for off, s in strings if off < cut]

    print("  unit %s: text %08X..%08X, in %s (%08X..%08X)"
          % (args.unit, me_s, me_e, wad, lo, hi))
    print("  pool @stringBase0 at %08X, %d bytes, %d strings; %d of them "
          "are referenced by code in the TU"
          % (base, size, len(strings), len(ref)))
    print("  this unit is first to reference %d string(s); the earliest is "
          "+%d %s" % (len(own), cut, by_off[cut].decode("ascii", "replace")))
    print("  prefix: %d string(s), %d bytes, must precede it"
          % (len(prefix), cut))

    out = ROOT / "src" / (args.unit[:-4] + ".pool.h")
    body = [
        "// GENERATED by tools/gen_poolprefix.py; regenerate, do not edit.",
        "//",
        "// %s is a fragment of the unity translation unit %s." % (
            Path(args.unit).name, Path(wad).name),
        "// That unit's string pool (@stringBase0 at 0x%08X, %d bytes)"
        % (base, size),
        "// is filled in order of first appearance across every file it",
        "// included, and a reference bakes the string's pool OFFSET into",
        "// the instruction. The %d strings below are the ones that come"
        % len(prefix),
        "// before this file's own, in that order, so that compiled alone",
        "// it gets the same offsets. This is data read from the image;",
        "// retail has no such table, it has the files in front.",
        "",
        "static const char* const kUnityPoolPrefix[] = {",
    ]
    for off, s in prefix:
        body.append("    %s,  // +%d" % (c_string(s), off))
    body += ["};", ""]
    text = NL.join(body)
    if args.check:
        old = out.read_text(encoding="utf-8") if out.exists() else None
        print("  %s" % ("up to date" if old == text else "OUT OF DATE"))
        return 0 if old == text else 1
    out.write_text(text, encoding="utf-8")
    print("  wrote %s" % out.relative_to(ROOT).as_posix())
    print('  include it FIRST in the unit: #include "%s"' % out.relative_to(ROOT / "src").as_posix())
    return 0


if __name__ == "__main__":
    sys.exit(main())
