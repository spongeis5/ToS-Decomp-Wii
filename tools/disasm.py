"""Read one retail function, with its symbols resolved.

    python tools/disasm.py GetOwner__16zNPCBTClientBaseCFv
    python tools/disasm.py 0x80101A30
    python tools/disasm.py --unit SB/GM/Engine/Game/zSoundReverb

Nothing here disassembled anything. `unitcmp -v` falls back to printing hex
words when no PowerPC disassembler is importable, and none is, so reading a
130-byte function meant decoding it by hand -- which is exactly the part of
writing a unit that should not be done by hand.

What it adds beyond a mnemonic:

  * BRANCH TARGETS become names. `bl 0x8002A6B0` is unreadable; `bl
    getRootCollidable__9hkpCdBodyCFv` is the call the source has to make.
  * `lis` PAIRS become addresses. A `lis rX,HI` and the next instruction to
    use rX are folded into the value they build, and that value is looked
    up in the symbol table, so a float pool or a global arrives named.
  * The FRAME is summarised: which registers are saved, how big the stack
    frame is, whether lr is spilled -- the shape a source has to produce.

UNKNOWN ENCODINGS ARE PRINTED AS `.word` AND COUNTED, and the count is
stated at the end. A disassembler that quietly renders an instruction it
does not know as something plausible is worse than one that stops, because
the wrong mnemonic is then reasoned from. Every count here states its
denominator: `104 of 108 bytes decoded`.
"""

import argparse
import re
import struct
import sys
from pathlib import Path

from elftools.elf.elffile import ELFFile

ROOT = Path(__file__).resolve().parent.parent
ELF = ROOT / "orig/R8IE78/files/SB09WiiMASTERWAD.elf"
SPLITS = ROOT / "config/R8IE78/splits.txt"

# --------------------------------------------------------------------------
# Encodings. Anything not named here prints as .word rather than as a guess.
# --------------------------------------------------------------------------

D_FORM = {
    32: "lwz", 33: "lwzu", 34: "lbz", 35: "lbzu", 40: "lhz", 41: "lhzu",
    42: "lha", 43: "lhau", 36: "stw", 37: "stwu", 38: "stb", 39: "stbu",
    44: "sth", 45: "sthu", 46: "lmw", 47: "stmw",
    48: "lfs", 49: "lfsu", 50: "lfd", 51: "lfdu",
    52: "stfs", 53: "stfsu", 54: "stfd", 55: "stfdu",
}
FLOAT_D = {48, 49, 50, 51, 52, 53, 54, 55}

ARITH_D = {14: "addi", 15: "addis", 12: "addic", 13: "addic.",
           8: "subfic", 7: "mulli"}
LOGIC_D = {24: "ori", 25: "oris", 26: "xori", 27: "xoris",
           28: "andi.", 29: "andis."}

X31 = {
    266: "add", 10: "addc", 138: "adde", 234: "addme", 202: "addze",
    40: "subf", 8: "subfc", 136: "subfe", 232: "subfme", 200: "subfze",
    104: "neg", 235: "mullw", 75: "mulhw", 11: "mulhwu", 491: "divw",
    459: "divwu", 28: "and", 60: "andc", 444: "or", 412: "orc",
    316: "xor", 124: "nor", 476: "nand", 284: "eqv", 26: "cntlzw",
    954: "extsb", 922: "extsh", 24: "slw", 536: "srw", 792: "sraw",
    824: "srawi", 0: "cmp", 32: "cmpl", 19: "mfcr", 144: "mtcrf",
    339: "mfspr", 467: "mtspr", 371: "mftb",
    23: "lwzx", 55: "lwzux", 87: "lbzx", 119: "lbzux", 279: "lhzx",
    311: "lhzux", 343: "lhax", 375: "lhaux", 151: "stwx", 183: "stwux",
    215: "stbx", 247: "stbux", 407: "sthx", 439: "sthux",
    535: "lfsx", 567: "lfsux", 599: "lfdx", 663: "stfsx", 695: "stfsux",
    727: "stfdx", 4: "tw", 598: "sync", 982: "icbi", 86: "dcbf",
    54: "dcbst", 246: "dcbtst", 278: "dcbt", 1014: "dcbz",
    # 150 is stwcx. here; isync is opcode 19, not 31. Writing "isync" in
    # this table shadowed nothing, but a placeholder written beside it as
    # `150 + 1: None` DID -- it silently replaced stwx, and 2,615
    # instructions stopped decoding. The sweep is what said so.
    150: "stwcx.", 20: "lwarx", 146: "mtmsr", 83: "mfmsr", 595: "mfsr",
    210: "mtsr", 918: "sthbrx", 534: "lwbrx", 790: "lhbrx", 662: "stwbrx",
    470: "dcbi", 306: "tlbie", 566: "tlbsync", 370: "tlbia",
}
X31_FLOAT_D = {535, 567, 599, 663, 695, 727}

X19 = {16: "bclr", 528: "bcctr", 50: "rfi", 150: "isync", 0: "mcrf",
       33: "crnor", 129: "crandc", 193: "crxor", 225: "crnand",
       257: "crand", 289: "creqv", 417: "crorc", 449: "cror"}

A59 = {18: "fdivs", 20: "fsubs", 21: "fadds", 22: "fsqrts", 24: "fres",
       25: "fmuls", 28: "fmsubs", 29: "fmadds", 30: "fnmsubs",
       31: "fnmadds"}
A63 = {18: "fdiv", 20: "fsub", 21: "fadd", 22: "fsqrt", 23: "fsel",
       25: "fmul", 26: "frsqrte", 28: "fmsub", 29: "fmadd", 30: "fnmsub",
       31: "fnmadd"}
X63 = {0: "fcmpu", 32: "fcmpo", 12: "frsp", 14: "fctiw", 15: "fctiwz",
       40: "fneg", 72: "fmr", 136: "fnabs", 264: "fabs", 583: "mffs",
       711: "mtfsf", 64: "mcrfs", 38: "mtfsb1", 70: "mtfsb0",
       134: "mtfsfi"}

# Gekko paired singles, opcode 4. A-form (5-bit extended opcode) first;
# it is disjoint from the 10-bit table, and the indexed quantised forms
# start at 6, so no two of the three tables collide.
PS_A = {10: "ps_sum0", 11: "ps_sum1", 12: "ps_muls0", 13: "ps_muls1",
        14: "ps_madds0", 15: "ps_madds1", 18: "ps_div", 20: "ps_sub",
        21: "ps_add", 23: "ps_sel", 24: "ps_res", 25: "ps_mul",
        26: "ps_rsqrte", 28: "ps_msub", 29: "ps_madd", 30: "ps_nmsub",
        31: "ps_nmadd"}
PS_X10 = {0: "ps_cmpu0", 32: "ps_cmpo0", 40: "ps_neg", 64: "ps_cmpu1",
          72: "ps_mr", 96: "ps_cmpo1", 136: "ps_nabs", 264: "ps_abs",
          528: "ps_merge00", 560: "ps_merge01", 592: "ps_merge10",
          624: "ps_merge11", 1014: "dcbz_l"}
# Read off the image rather than off a manual: xo6 = 7 appears in
# prologues beside `stfd f31,N(r1)` and xo6 = 6 in epilogues beside
# `lfd f31,N(r31)`, which is the two-halves save of a paired-single
# register. 6 is the load, 7 the store; the update forms are 32 higher.
PS_IDX = {6: "psq_lx", 7: "psq_stx", 38: "psq_lux", 39: "psq_stux"}
PSQ_D = {56: "psq_l", 57: "psq_lu", 60: "psq_st", 61: "psq_stu"}

SPR = {1: "xer", 8: "lr", 9: "ctr"}
CONDS = {(12, 0): "blt", (4, 0): "bge", (12, 1): "bgt", (4, 1): "ble",
         (12, 2): "beq", (4, 2): "bne", (12, 3): "bso", (4, 3): "bns"}


def signed(v, bits=16):
    return v - (1 << bits) if v & (1 << (bits - 1)) else v


class Decoded(object):
    def __init__(self, text, target=None, base_reg=None, disp=None,
                 sets_hi=None, uses=None, known=True):
        self.text = text
        self.target = target        # a branch destination
        self.base_reg = base_reg    # a D-form base register
        self.disp = disp            # its displacement
        self.sets_hi = sets_hi      # (register, value<<16) for lis
        self.known = known


def decode(w, addr):
    op = w >> 26
    d = (w >> 21) & 31
    a = (w >> 16) & 31
    b = (w >> 11) & 31
    imm = w & 0xFFFF

    if op == 18:                                     # b / bl / ba / bla
        li = w & 0x03FFFFFC
        if li & 0x02000000:
            li -= 0x04000000
        tgt = li if w & 2 else addr + li
        return Decoded("%-8s" % ("bla" if w & 3 == 3 else
                                 "ba" if w & 2 else
                                 "bl" if w & 1 else "b"), target=tgt)

    if op == 16:                                     # bc
        bd = w & 0xFFFC
        if bd & 0x8000:
            bd -= 0x10000
        tgt = bd if w & 2 else addr + bd
        bo, bi = d, a
        name = CONDS.get((bo & ~1 if bo in (13, 5) else bo, bi & 3))
        if bo in (16, 18):
            name = "bdnz" if bo == 16 else "bdz"
        if name is None:
            return Decoded("bc      %d,%d," % (bo, bi), target=tgt)
        if bi >= 4:
            name += "  cr%d," % (bi >> 2)
        if w & 1:
            name += "l"
        return Decoded("%-8s" % name, target=tgt)

    if op == 17:
        return Decoded("sc")

    if op == 19:
        xo = (w >> 1) & 0x3FF
        nm = X19.get(xo)
        if nm == "bclr":
            if d == 20:
                return Decoded("blr")
            c = CONDS.get((d, a & 3))
            return Decoded("%slr" % c if c else "bclr   %d,%d" % (d, a))
        if nm == "bcctr":
            if d == 20:
                return Decoded("bctrl" if w & 1 else "bctr")
            return Decoded("bcctr  %d,%d" % (d, a))
        if nm and nm.startswith("cr"):
            return Decoded("%-8s%d,%d,%d" % (nm, d, a, b))
        if nm:
            return Decoded(nm)
        return Decoded(".word   0x%08X" % w, known=False)

    if op in ARITH_D:
        nm = ARITH_D[op]
        if nm == "addi" and a == 0:
            return Decoded("%-8sr%d,%d" % ("li", d, signed(imm)))
        if nm == "addis" and a == 0:
            return Decoded("%-8sr%d,0x%04X" % ("lis", d, imm),
                           sets_hi=(d, imm << 16))
        return Decoded("%-8sr%d,r%d,%d" % (nm, d, a, signed(imm)),
                       base_reg=a, disp=signed(imm))

    if op in LOGIC_D:
        return Decoded("%-8sr%d,r%d,0x%04X" % (LOGIC_D[op], a, d, imm))

    if op in (10, 11):                                # cmpli / cmpi
        nm = "cmpli" if op == 10 else "cmpi"
        val = imm if op == 10 else signed(imm)
        crf = d >> 2
        if crf == 0:
            return Decoded("%-8sr%d,%s" % (nm[:-1] + "wi" if op == 11
                                           else "cmplwi", a,
                                           "0x%X" % val if op == 10
                                           else str(val)))
        return Decoded("%-8scr%d,r%d,%d" % (nm, crf, a, val))

    if op in D_FORM:
        nm = D_FORM[op]
        reg = ("f%d" if op in FLOAT_D else "r%d") % d
        return Decoded("%-8s%s,%d(r%d)" % (nm, reg, signed(imm), a),
                       base_reg=a, disp=signed(imm))

    if op in (20, 21, 23):                            # rlwimi/rlwinm/rlwnm
        nm = {20: "rlwimi", 21: "rlwinm", 23: "rlwnm"}[op]
        sh, mb, me = b, (w >> 6) & 31, (w >> 1) & 31
        dot = "." if w & 1 else ""
        if nm == "rlwinm" and mb == 0 and me == 31 - sh:
            return Decoded("%-8sr%d,r%d,%d" % ("slwi" + dot, a, d, sh))
        if nm == "rlwinm" and me == 31 and mb == 32 - sh and sh:
            return Decoded("%-8sr%d,r%d,%d" % ("srwi" + dot, a, d, 32 - sh))
        return Decoded("%-8sr%d,r%d,%d,%d,%d" % (nm + dot, a, d, sh, mb, me))

    if op == 31:
        xo = (w >> 1) & 0x3FF
        nm = X31.get(xo)
        if nm is None:
            return Decoded(".word   0x%08X" % w, known=False)
        dot = "." if (w & 1) and nm not in ("cmp", "cmpl", "mtspr",
                                            "mfspr", "mtcrf") else ""
        if nm in ("mtspr", "mfspr"):
            spr = ((w >> 16) & 0x1F) | (((w >> 11) & 0x1F) << 5)
            name = SPR.get(spr, "spr%d" % spr)
            if nm == "mtspr":
                return Decoded("%-8sr%d" % ("mt" + name, d))
            return Decoded("%-8sr%d" % ("mf" + name, d))
        if nm in ("cmp", "cmpl"):
            crf = d >> 2
            base = "cmpw" if nm == "cmp" else "cmplw"
            if crf == 0:
                return Decoded("%-8sr%d,r%d" % (base, a, b))
            return Decoded("%-8scr%d,r%d,r%d" % (base, crf, a, b))
        if nm == "or" and a != d and d == b:
            return Decoded("%-8sr%d,r%d" % ("mr" + dot, a, d))
        if nm in ("cntlzw", "extsb", "extsh"):
            return Decoded("%-8sr%d,r%d" % (nm + dot, a, d))
        if nm == "neg":
            return Decoded("%-8sr%d,r%d" % (nm + dot, d, a))
        if nm == "srawi":
            return Decoded("%-8sr%d,r%d,%d" % (nm + dot, a, d, b))
        if nm == "mfcr":
            return Decoded("%-8sr%d" % (nm, d))
        if nm in ("sync", "isync"):
            return Decoded(nm)
        if nm.startswith(("lf", "stf")):
            return Decoded("%-8sf%d,r%d,r%d" % (nm, d, a, b))
        if nm.startswith(("l", "st")):
            return Decoded("%-8sr%d,r%d,r%d" % (nm, d, a, b))
        if nm in ("and", "andc", "or", "orc", "xor", "nor", "nand", "eqv",
                  "slw", "srw", "sraw"):
            return Decoded("%-8sr%d,r%d,r%d" % (nm + dot, a, d, b))
        return Decoded("%-8sr%d,r%d,r%d" % (nm + dot, d, a, b))

    if op in (59, 63):
        xo5 = (w >> 1) & 0x1F
        table = A59 if op == 59 else A63
        dot = "." if w & 1 else ""
        if op == 63:
            xo10 = (w >> 1) & 0x3FF
            if xo10 in X63:
                nm = X63[xo10]
                if nm in ("fmr", "fneg", "fabs", "fnabs", "frsp",
                          "fctiw", "fctiwz"):
                    return Decoded("%-8sf%d,f%d" % (nm + dot, d, b))
                if nm in ("fcmpu", "fcmpo"):
                    return Decoded("%-8scr%d,f%d,f%d" % (nm, d >> 2, a, b))
                return Decoded("%-8sf%d" % (nm + dot, d))
        if xo5 in table:
            nm = table[xo5]
            c = (w >> 6) & 31
            if nm in ("fmadds", "fmsubs", "fnmadds", "fnmsubs", "fmadd",
                      "fmsub", "fnmadd", "fnmsub", "fsel"):
                return Decoded("%-8sf%d,f%d,f%d,f%d"
                               % (nm + dot, d, a, c, b))
            if nm in ("fmuls", "fmul"):
                return Decoded("%-8sf%d,f%d,f%d" % (nm + dot, d, a, c))
            if nm in ("fres", "frsqrte", "fsqrt", "fsqrts"):
                return Decoded("%-8sf%d,f%d" % (nm + dot, d, b))
            return Decoded("%-8sf%d,f%d,f%d" % (nm + dot, d, a, b))
        return Decoded(".word   0x%08X" % w, known=False)

    if op in PSQ_D:
        # psq_l  frD, d(rA), W, I  -- a 12-bit displacement, then W and I.
        disp = w & 0xFFF
        if disp & 0x800:
            disp -= 0x1000
        return Decoded("%-8sf%d,%d(r%d),%d,%d"
                       % (PSQ_D[op], d, disp, a, (w >> 15) & 1,
                          (w >> 12) & 7))

    if op == 4:
        xo5 = (w >> 1) & 0x1F
        xo10 = (w >> 1) & 0x3FF
        xo6 = (w >> 1) & 0x3F
        c = (w >> 6) & 31
        dot = "." if w & 1 else ""
        if xo5 in PS_A:
            nm = PS_A[xo5]
            if nm in ("ps_madd", "ps_msub", "ps_nmadd", "ps_nmsub",
                      "ps_sel", "ps_sum0", "ps_sum1", "ps_madds0",
                      "ps_madds1"):
                return Decoded("%-8sf%d,f%d,f%d,f%d" % (nm + dot, d, a, c, b))
            if nm in ("ps_mul", "ps_muls0", "ps_muls1"):
                return Decoded("%-8sf%d,f%d,f%d" % (nm + dot, d, a, c))
            if nm in ("ps_res", "ps_rsqrte"):
                return Decoded("%-8sf%d,f%d" % (nm + dot, d, b))
            return Decoded("%-8sf%d,f%d,f%d" % (nm + dot, d, a, b))
        if xo10 in PS_X10:
            nm = PS_X10[xo10]
            if nm.startswith("ps_cmp"):
                return Decoded("%-8scr%d,f%d,f%d" % (nm, d >> 2, a, b))
            if nm == "dcbz_l":
                return Decoded("%-8sr%d,r%d" % (nm, a, b))
            if nm.startswith("ps_merge"):
                return Decoded("%-8sf%d,f%d,f%d" % (nm + dot, d, a, b))
            return Decoded("%-8sf%d,f%d" % (nm + dot, d, b))
        if xo6 in PS_IDX:
            return Decoded("%-8sf%d,r%d,r%d,%d,%d"
                           % (PS_IDX[xo6], d, a, b, (w >> 10) & 1,
                              (w >> 7) & 7))
        return Decoded(".word   0x%08X" % w, known=False)

    return Decoded(".word   0x%08X" % w, known=False)


# --------------------------------------------------------------------------


def load():
    raw = ELF.read_bytes()
    secs, funcs, objs = [], {}, {}
    with open(ELF, "rb") as fh:
        elf = ELFFile(fh)
        for s in elf.iter_sections():
            secs.append((s.header["sh_addr"], s.header["sh_offset"],
                         s.header["sh_size"], s.name))
        for sec in elf.iter_sections():
            if sec.header["sh_type"] != "SHT_SYMTAB":
                continue
            for s in sec.iter_symbols():
                if not s.name:
                    continue
                if s["st_info"]["type"] == "STT_FUNC" and s["st_size"]:
                    funcs[s["st_value"]] = (s.name, s["st_size"])
                else:
                    objs.setdefault(s["st_value"], s.name)
    return raw, secs, funcs, objs


def read(raw, secs, addr, n):
    for base, off, size, _nm in secs:
        if base and base <= addr and addr + n <= base + size:
            return raw[off + addr - base: off + addr - base + n]
    return None


def unit_ranges():
    out, cur, rs = [], None, []
    for line in SPLITS.read_text(encoding="utf-8").splitlines():
        if line and not line[0].isspace() and line.rstrip().endswith(":"):
            if cur:
                out.append((cur, rs))
            cur, rs = line.rstrip()[:-1], []
            continue
        m = re.match(r"\s+\.text\s+start:(0x[0-9A-Fa-f]+)\s+"
                     r"end:(0x[0-9A-Fa-f]+)", line)
        if m and cur:
            rs.append((int(m.group(1), 16), int(m.group(2), 16)))
    if cur:
        out.append((cur, rs))
    return out


def name_at(funcs, objs, addr):
    if addr in funcs:
        return funcs[addr][0]
    if addr in objs:
        return objs[addr]
    for a, (nm, sz) in funcs.items():
        if a < addr < a + sz:
            return "%s+0x%X" % (nm, addr - a)
    return None


def show(raw, secs, funcs, objs, addr, size, name):
    body = read(raw, secs, addr, size)
    if body is None:
        sys.exit("disasm: %08X is in no loaded section" % addr)
    words = struct.unpack(">" + "I" * (size // 4), body)

    print("  %s" % name)
    print("  %08X..%08X   %d bytes, %d instruction(s)"
          % (addr, addr + size, size, len(words)))
    print("")

    hi = {}
    unknown = 0
    labels = {}
    for i, w in enumerate(words):
        dc = decode(w, addr + 4 * i)
        if dc.target is not None and addr <= dc.target < addr + size:
            # setdefault, not assignment: a target branched to twice was
            # being RENUMBERED on the second branch, so two different
            # addresses printed the same label and one label named two
            # places. Numbered in address order so the listing reads down.
            labels.setdefault(dc.target, None)
    for i, t in enumerate(sorted(labels)):
        labels[t] = "L%d" % (i + 1)

    for i, w in enumerate(words):
        at = addr + 4 * i
        dc = decode(w, at)
        if not dc.known:
            unknown += 1
        note = ""

        if dc.target is not None:
            if dc.target in labels:
                note = "  -> %s" % labels[dc.target]
            else:
                nm = name_at(funcs, objs, dc.target)
                note = "  -> %s" % (nm or "%08X" % dc.target)

        if dc.base_reg is not None and dc.base_reg in hi:
            full = (hi[dc.base_reg] + dc.disp) & 0xFFFFFFFF
            nm = name_at(funcs, objs, full)
            note = "  = %08X%s" % (full, "  %s" % nm if nm else "")

        if dc.sets_hi:
            hi[dc.sets_hi[0]] = dc.sets_hi[1]
        elif dc.base_reg is None:
            # Any write to a register invalidates a pending lis in it.
            m = re.match(r"\S+\s+r(\d+)", dc.text)
            if m and int(m.group(1)) in hi:
                del hi[int(m.group(1))]

        lab = labels.get(at, "")
        print("  %08X  %08X  %-4s %-34s%s"
              % (at, w, lab + ":" if lab else "", dc.text, note))

    print("")
    print("  %d of %d instruction(s) decoded; %d printed as .word"
          % (len(words) - unknown, len(words), unknown))
    if unknown:
        print("  A .word is an encoding this tool does not know. It is NOT")
        print("  a no-op and NOT a guess -- decode it before writing source.")
    return unknown


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("what", nargs="?", help="a symbol name or 0xADDRESS")
    ap.add_argument("--unit", help="every unmatched function of a unit")
    args = ap.parse_args()
    if not args.what and not args.unit:
        sys.exit(__doc__)

    raw, secs, funcs, objs = load()

    if args.unit:
        rs = [r for u, r in unit_ranges()
              if u == args.unit or u == args.unit + ".cpp"]
        if not rs:
            sys.exit("disasm: splits.txt has no unit %r" % args.unit)
        picked = sorted((a, nm, sz) for a, (nm, sz) in funcs.items()
                        if any(lo <= a < hi for lo, hi in rs[0]))
        if not picked:
            sys.exit("disasm: %s holds no function symbol" % args.unit)
        for a, nm, sz in picked:
            show(raw, secs, funcs, objs, a, sz, nm)
            print("")
        return 0

    if args.what.lower().startswith("0x"):
        addr = int(args.what, 16)
        if addr not in funcs:
            sys.exit("disasm: no function symbol starts at %08X" % addr)
        nm, sz = funcs[addr]
    else:
        hits = [(a, v) for a, v in funcs.items() if v[0] == args.what]
        if not hits:
            sys.exit("disasm: no function is called %r" % args.what)
        if len(hits) > 1:
            sys.exit("disasm: %d functions are called %r -- give an address: "
                     "%s" % (len(hits), args.what,
                             ", ".join("0x%08X" % a for a, _v in hits)))
        addr, (nm, sz) = hits[0]
    return 1 if show(raw, secs, funcs, objs, addr, sz, nm) else 0


if __name__ == "__main__":
    sys.exit(main())
