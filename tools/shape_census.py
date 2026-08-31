"""What SHAPES do the unmatched game functions have, as a population.

    python tools/shape_census.py [--max-bytes N] [--limit N] [--shape SIG]

Both generators were written after a shape was noticed by accident, and each
time the population turned out to be far larger than the accident. This asks
the question directly: of the game functions that do NOT match today, how
many share an instruction-opcode signature, and how many bytes do they carry.

A signature is the sequence of primary opcodes, with the extended opcode
appended for the X/XO forms (primary 31) and the branch forms (19), because
primary 31 alone covers everything from `or` to `divw`. It is deliberately
coarser than a disassembly: it groups `lwz r3,4(r4)` with `lwz r5,8(r6)`,
which is the grouping a generator cares about.

`--shape SIG` lists the functions carrying one signature, so a promising row
can be read before anything is written.

Counts are of functions NOT already matching, out of every function the
splits assign to a game unit; report.json supplies both facts and the tool
refuses to run without it.
"""

import argparse
import json
import re
import struct
import sys
from collections import Counter, defaultdict
from pathlib import Path

from elftools.elf.elffile import ELFFile

ROOT = Path(__file__).resolve().parent.parent
ELF = ROOT / "orig/R8IE78/files/SB09WiiMASTERWAD.elf"
SPLITS = ROOT / "config/R8IE78/splits.txt"
REPORT = ROOT / "build/R8IE78/report.json"


def unit_ranges():
    out, cur, ranges = [], None, []
    for line in SPLITS.read_text(encoding="utf-8").splitlines():
        if line and not line[0].isspace() and line.rstrip().endswith(":"):
            if cur:
                out.append((cur, ranges))
            cur, ranges = line.rstrip()[:-1], []
            continue
        m = re.match(r"\s+\.text\s+start:(0x[0-9A-Fa-f]+)\s+"
                     r"end:(0x[0-9A-Fa-f]+)", line)
        if m and cur:
            ranges.append((int(m.group(1), 16), int(m.group(2), 16)))
    if cur:
        out.append((cur, ranges))
    return out


def report():
    if not REPORT.exists():
        sys.exit("shape_census: %s is missing -- run ninja first. Without it "
                 "every already-matched function would be counted as work "
                 "remaining." % REPORT)
    rep = json.loads(REPORT.read_text(encoding="utf-8"))
    game, matched = set(), set()
    for u in rep["units"]:
        meta = u.get("metadata", {})
        if "game" not in (meta.get("progress_categories") or []):
            continue
        name = u["name"].split("/", 1)[1] if "/" in u["name"] else u["name"]
        game.add(name + ".cpp")
        for fn in u.get("functions", []):
            if fn.get("fuzzy_match_percent") == 100.0:
                matched.add(fn["name"])
    if not game:
        sys.exit("shape_census: report.json names no game units -- the "
                 "category filter found nothing, which is not a census of "
                 "zero remaining work.")
    return game, matched


# Primary opcodes worth naming; anything else prints as its number.
NAMES = {
    3: "twi", 7: "mulli", 8: "subfic", 10: "cmpli", 11: "cmpi", 12: "addic",
    13: "addic.", 14: "addi", 15: "addis", 16: "bc", 17: "sc", 18: "b",
    20: "rlwimi", 21: "rlwinm", 23: "rlwnm", 24: "ori", 25: "oris",
    26: "xori", 27: "xoris", 28: "andi.", 29: "andis.", 32: "lwz",
    33: "lwzu", 34: "lbz", 35: "lbzu", 36: "stw", 37: "stwu", 38: "stb",
    39: "stbu", 40: "lhz", 41: "lhzu", 42: "lha", 43: "lhau", 44: "sth",
    45: "sthu", 46: "lmw", 47: "stmw", 48: "lfs", 49: "lfsu", 50: "lfd",
    51: "lfdu", 52: "stfs", 53: "stfsu", 54: "stfd", 55: "stfdu",
}

X31 = {
    0: "cmp", 8: "subfc", 10: "addc", 11: "mulhwu", 19: "mfcr", 23: "lwzx",
    24: "slw", 26: "cntlzw", 28: "and", 32: "cmpl", 40: "subf", 55: "lwzux",
    60: "andc", 75: "mulhw", 87: "lbzx", 104: "neg", 119: "lbzux",
    124: "nor", 136: "subfe", 138: "adde", 144: "mtcrf", 200: "subfze",
    202: "addze", 234: "addme", 235: "mullw", 266: "add", 279: "lhzx",
    284: "eqv", 316: "xor", 339: "mfspr", 343: "lhax", 371: "mftb",
    407: "sthx", 412: "orc", 439: "sthux", 444: "or", 459: "divwu",
    467: "mtspr", 476: "nand", 491: "divw", 512: "mcrxr", 533: "lswx",
    535: "lfsx", 536: "srw", 567: "lfsux", 663: "stfsx", 695: "stfsux",
    792: "sraw", 824: "srawi", 922: "extsh", 954: "extsb", 151: "stwx",
    183: "stwux", 86: "dcbf", 982: "icbi", 598: "sync", 4: "tw",
}

X19 = {16: "bclr", 528: "bcctr", 50: "rfi", 150: "isync", 33: "crnor",
       257: "crand", 449: "cror", 193: "crxor", 225: "crnand", 289: "creqv",
       417: "crorc", 481: "crandc", 0: "mcrf"}

X59 = {18: "fdivs", 20: "fsubs", 21: "fadds", 22: "fsqrts", 24: "fres",
       25: "fmuls", 28: "fmsubs", 29: "fmadds", 30: "fnmsubs", 31: "fnmadds"}

X63 = {12: "frsp", 14: "fctiw", 15: "fctiwz", 18: "fdiv", 20: "fsub",
       21: "fadd", 22: "fsqrt", 23: "fsel", 25: "fmul", 26: "frsqrte",
       28: "fmsub", 29: "fmadd", 30: "fnmsub", 31: "fnmadd", 0: "fcmpu",
       32: "fcmpo", 40: "fneg", 72: "fmr", 136: "fnabs", 264: "fabs",
       583: "mffs", 711: "mtfsf"}


def mnemonic(w):
    op = (w >> 26) & 0x3F
    if op == 31:
        xo = (w >> 1) & 0x3FF
        return X31.get(xo, "31/%d" % xo)
    if op == 19:
        xo = (w >> 1) & 0x3FF
        return X19.get(xo, "19/%d" % xo)
    if op == 59:
        return X59.get((w >> 1) & 0x1F, "59/%d" % ((w >> 1) & 0x1F))
    if op == 63:
        xo5 = (w >> 1) & 0x1F
        if xo5 in X63 and xo5 >= 18:
            return X63[xo5]
        return X63.get((w >> 1) & 0x3FF, "63/%d" % ((w >> 1) & 0x3FF))
    if op == 4:
        return "ps/%d" % ((w >> 1) & 0x3FF)
    return NAMES.get(op, "op%d" % op)


def sections(raw):
    (shoff, shentsize, shnum, _shstrndx) = (
        struct.unpack(">I", raw[0x20:0x24])[0],
        struct.unpack(">H", raw[0x2E:0x30])[0],
        struct.unpack(">H", raw[0x30:0x32])[0],
        struct.unpack(">H", raw[0x32:0x34])[0])
    out = []
    for i in range(shnum):
        o = shoff + i * shentsize
        f = struct.unpack(">IIIIIIIIII", raw[o:o + 40])
        out.append(f)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--max-bytes", type=int, default=32,
                    help="longest function to include (default 32)")
    ap.add_argument("--limit", type=int, default=25)
    ap.add_argument("--shape", help="list the functions with this signature")
    ap.add_argument("--dump", action="append", default=[],
                    help="print the words of the function at this address")
    args = ap.parse_args()

    game, matched = report()
    raw = ELF.read_bytes()
    ranges = [(u, rs) for u, rs in unit_ranges() if u in game]
    if not ranges:
        sys.exit("shape_census: splits.txt and report.json share no unit "
                 "name -- the two are not being joined on the same key.")

    probe = next(rs[0][0] for _u, rs in ranges if rs)
    base = foff = None
    for sh in sections(raw):
        if sh[3] and sh[3] <= probe < sh[3] + sh[5]:
            base, foff = sh[3], sh[4]
            break
    if base is None:
        sys.exit("shape_census: cannot locate the section holding .text")

    syms = []
    with open(ELF, "rb") as fh:
        for sec in ELFFile(fh).iter_sections():
            if sec.header["sh_type"] != "SHT_SYMTAB":
                continue
            for s in sec.iter_symbols():
                if s["st_info"]["type"] == "STT_FUNC" and s["st_size"]:
                    syms.append((s["st_value"], s["st_size"], s.name))
    syms.sort()

    owner = {}
    for unit, rs in ranges:
        for lo, hi in rs:
            owner[(lo, hi)] = unit

    total = seen = 0
    shapes = Counter()
    shape_bytes = Counter()
    members = defaultdict(list)
    for addr, size, name in syms:
        unit = None
        for (lo, hi), u in owner.items():
            if lo <= addr < hi:
                unit = u
                break
        if unit is None:
            continue
        total += 1
        if name in matched:
            continue
        seen += 1
        if size > args.max_bytes or size % 4:
            continue
        body = raw[foff + (addr - base): foff + (addr - base) + size]
        sig = " ".join(mnemonic(struct.unpack(">I", body[i:i + 4])[0])
                       for i in range(0, size, 4))
        shapes[sig] += 1
        shape_bytes[sig] += size
        members[sig].append((unit, name, addr, size))

    if args.dump:
        want = {int(a, 16) for a in args.dump}
        found = set()
        for addr, size, name in syms:
            if addr not in want:
                continue
            found.add(addr)
            print("  %08X  %s  (%d bytes)" % (addr, name, size))
            for i in range(0, size, 4):
                w = struct.unpack(
                    ">I", raw[foff + (addr - base) + i:
                              foff + (addr - base) + i + 4])[0]
                print("    %08X  %08X  %-8s D=%-2d A=%-2d imm=%-6d "
                      "(0x%04X)"
                      % (addr + i, w, mnemonic(w), (w >> 21) & 31,
                         (w >> 16) & 31,
                         (w & 0xFFFF) - 0x10000 if w & 0x8000 else w & 0xFFFF,
                         w & 0xFFFF))
            print("")
        missing = want - found
        if missing:
            sys.exit("shape_census: no function symbol at %s"
                     % ", ".join("%08X" % a for a in sorted(missing)))
        return 0

    if args.shape:
        rows = members.get(args.shape)
        if not rows:
            sys.exit("shape_census: no unmatched function has the signature "
                     "%r" % args.shape)
        print("  %d function(s) with %r" % (len(rows), args.shape))
        for unit, name, addr, size in sorted(rows):
            print("  %08X  %3d  %-38s %s" % (addr, size, unit, name))
        return 0

    covered = sum(shape_bytes.values())
    print("  %d game function(s) in the splits; %d not matching; %d of those "
          "are <= %d bytes" % (total, seen, sum(shapes.values()),
                               args.max_bytes))
    print("  those short ones carry %d bytes across %d distinct shape(s)"
          % (covered, len(shapes)))
    print("")
    print("  %-5s %-7s %s" % ("N", "BYTES", "SIGNATURE"))
    for sig, n in shapes.most_common(args.limit):
        print("  %-5d %-7d %s" % (n, shape_bytes[sig], sig))
    shown = sum(n for _s, n in shapes.most_common(args.limit))
    print("")
    print("  %d of %d short function(s) shown, %d of %d bytes"
          % (shown, sum(shapes.values()),
             sum(shape_bytes[s] for s, _n in shapes.most_common(args.limit)),
             covered))
    return 0


if __name__ == "__main__":
    sys.exit(main())
