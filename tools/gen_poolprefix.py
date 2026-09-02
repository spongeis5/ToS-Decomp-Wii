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

The header also carries the SIZE of the `.rodata` the files in front
put ahead of this unit's float literals, as an unreferenced const
array. mwcc addresses a function's float literals by a fixed cost
rule: under 32 KB into `.rodata` it shares one base for three or more
literals (`lis` once, section-relative offsets), past 32 KB it gives
each literal its own `lis` up to three and forms an `addis` base for
four or more. Retail's literals for a game file sit tens of KB into
its unity unit's `.rodata`, so retail never shares under three; a
fragment compiled alone puts them at 12 KB and shares, one word short
per function (zPlayerFallSB, WalkSB, RunSB, HammerAttack, and the
one-word misses of WAD01_28 and zCommonPlayerActions). Measured, not
reasoned: probes at 0..131072 bytes of leading `.rodata`, every Wii
mwcc on disk, -O levels, -pooldata, -sdata2, -sym, and prior emission
of the literals all left the rule where it is. The four-literal case
(zPlayerWalkSB::AddActionTransitions) is still out of reach: retail
spells four `lis` there and the rule forms a base.
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
                    # A reference is an offset added to a register that
                    # holds the pool's EXACT base. Accepting any folded
                    # address that lands inside the pool counted seven
                    # unrelated constants from other units as referrers,
                    # three of them mid-string and one at an empty slot.
                    if regs[s] == base and 0 <= imm < size:
                        if imm not in first or a < first[imm]:
                            first[imm] = a
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


def rodata_ahead(raw, secs, funcs, tu_lo, tu_hi, me_s, me_e):
    """-> (tu_lowest, first_literal, offset): the lowest .rodata address
    any function of the TU forms, the lowest float literal a function
    of the unit loads, and the distance between them. None when the
    unit loads no float literal."""
    ro = [(s[0], s[0] + s[2]) for s in secs if s[3] == ".rodata"]

    def scan(lo, hi):
        addrs, lits = set(), set()
        for a, (nm, sz) in funcs.items():
            if not sz or not (lo <= a < hi):
                continue
            ws = struct.unpack(">" + "I" * (sz // 4),
                               D.read(raw, secs, a, sz))
            regs = {}
            for w in ws:
                op = w >> 26
                if op == 15 and ((w >> 16) & 31) == 0:
                    regs[(w >> 21) & 31] = (w & 0xFFFF) << 16
                elif op in (14, 32, 34, 40, 48, 50):
                    d, s = (w >> 21) & 31, (w >> 16) & 31
                    imm = w & 0xFFFF
                    imm = imm - 0x10000 if imm & 0x8000 else imm
                    if s in regs:
                        v = (regs[s] + imm) & 0xFFFFFFFF
                        if any(x <= v < y for x, y in ro):
                            addrs.add(v)
                            if op == 48:
                                lits.add(v)
                        if op == 14:
                            regs[d] = v
                            continue
                    regs.pop(d, None)
                elif op == 18 and (w & 1):
                    for r in [r for r in regs if r <= 12]:
                        del regs[r]
                elif op == 31:
                    regs.pop((w >> 21) & 31, None)
        return addrs, lits

    tu_addrs, _l = scan(tu_lo, tu_hi)
    _a, unit_lits = scan(me_s, me_e)
    if not tu_addrs or not unit_lits:
        return None
    lowest, first = min(tu_addrs), min(unit_lits)
    return lowest, first, first - lowest


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
    ap.add_argument("--whole", action="store_true",
                    help="every string in the pool, not only the prefix: "
                         "a unit written a function at a time needs its own "
                         "strings at their retail offsets too, and those "
                         "depend on functions not written yet")
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
    wad = (wads[0].rsplit("/", 1)[1][:-4].split("_")[0] + ".cpp") if wads \
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
    prefix = [(off, s) for off, s in strings
              if args.whole or off < cut]

    print("  unit %s: text %08X..%08X, in %s (%08X..%08X)"
          % (args.unit, me_s, me_e, wad, lo, hi))
    print("  pool @stringBase0 at %08X, %d bytes, %d strings; %d of them "
          "are referenced by code in the TU"
          % (base, size, len(strings), len(ref)))
    print("  this unit is first to reference %d string(s); the earliest is "
          "+%d %s" % (len(own), cut, by_off[cut].decode("ascii", "replace")))
    print("  prefix: %d string(s), %d bytes, must precede it"
          % (len(prefix), cut))
    ahead = rodata_ahead(raw, secs, funcs, lo, hi, me_s, me_e)
    if ahead:
        print("  .rodata ahead: the TU's lowest .rodata address is %08X (%s),"
              " this unit's first float literal %08X (%s), %d bytes apart"
              % (ahead[0], D.name_at(funcs, objs, ahead[0]), ahead[1],
                 D.name_at(funcs, objs, ahead[1]), ahead[2]))
    else:
        print("  .rodata ahead: this unit loads no float literal; no padding")

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
    ]
    if ahead:
        body += [
            "// The files in front also put .rodata ahead of this unit's",
            "// float literals: in the image the unit's first literal",
            "// (0x%08X) lies %d bytes past the lowest .rodata address the"
            % (ahead[1], ahead[2]),
            "// unit's translation unit forms (0x%08X). mwcc shares one base"
            % ahead[0],
            "// among a function's literals only when they sit under 32 KB",
            "// into .rodata, so a fragment compiled with nothing ahead is",
            "// one word short per table. This array is that distance,",
            "// measured; it is referenced by nothing and holds nothing.",
            "static const unsigned char kUnityRodataAhead[%d] = {1};"
            % ahead[2],
            "",
        ]
    body += [
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
