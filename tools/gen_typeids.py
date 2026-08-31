"""Generate the constant-returning members of a unit from the image itself.

    python tools/gen_typeids.py <start> <end> <out.cpp>

One shape only: a function whose whole body is a 32-bit constant into r3
and a return --

    lis   r3, HI
    addi  r3, r3, LO        (or ori r3, r3, LO)
    blr

or the two-instruction `li r3, N` / `blr`. WAD02_36 is 177 functions in
2,700 bytes and 164 of them are that, one `GetTypeID` per behaviour-tree
node, each returning a distinct hash.

THESE ARE GENERATED, NOT READ. They are worth having -- they are real
matched functions and the constants are recovered fact -- but a count of
them is not a count of decompiled code, and anything reporting progress
should say which it is.

REFUSES ANYTHING IT DOES NOT RECOGNISE. Every candidate is decoded back to
bytes and compared against the image before it is emitted; a function whose
body is not exactly this shape is skipped and counted, never guessed at.
The count of skipped functions is printed, because "we emitted 164" means
nothing without "of 177 in the range".

The members are declared NON-VIRTUAL. CodeWarrior mangles a virtual and a
non-virtual member identically, so the symbol is the same either way, and a
virtual declaration would make the compiler emit a vtable this unit does
not have.
"""

import re
import struct
import sys
from pathlib import Path

from elftools.elf.elffile import ELFFile

ROOT = Path(__file__).resolve().parent.parent
ELF = ROOT / "orig/R8IE78/files/SB09WiiMASTERWAD.elf"


def sections(raw):
    off, = struct.unpack_from(">I", raw, 32)
    es, num, si = struct.unpack_from(">HHH", raw, 46)
    hdrs = [struct.unpack_from(">IIIIIIIIII", raw, off + i * es)
            for i in range(num)]
    return hdrs


def demangle(sym):
    """-> (namespaces, class, method) or None.

    CodeWarrior: `name__<qualifier><flags>`. The qualifier is either
    `<len><name>` for a plain class or `Q<n><len><name>...` for a nested
    one. Anything else is refused rather than guessed at -- a wrong class
    name produces a different symbol, which is a silent miss.
    """
    if "__" not in sym:
        return None
    method, rest = sym.split("__", 1)
    if not rest:
        return None
    if rest.startswith("Q"):
        m = re.match(r"Q(\d)(.*)$", rest)
        if not m:
            return None
        count, rest = int(m.group(1)), m.group(2)
        parts = []
        for _ in range(count):
            m2 = re.match(r"(\d+)(.*)$", rest)
            if not m2:
                return None
            ln = int(m2.group(1))
            parts.append(m2.group(2)[:ln])
            rest = m2.group(2)[ln:]
        if not rest.startswith("CFv"):
            return None
        return parts[:-1], parts[-1], method
    m = re.match(r"(\d+)(.*)$", rest)
    if not m:
        return None
    ln = int(m.group(1))
    cls, rest = m.group(2)[:ln], m.group(2)[ln:]
    if rest != "CFv":
        return None
    return [], cls, method


def decode(body):
    """-> the 32-bit constant this body returns, or None."""
    ws = [struct.unpack_from(">I", body, i)[0] for i in range(0, len(body), 4)]
    if len(ws) == 3:
        hi, lo, ret = ws
        if (hi >> 26) != 15 or ((hi >> 21) & 31) != 3 or ((hi >> 16) & 31) != 0:
            return None                      # not lis r3, x
        if ret != 0x4E800020:
            return None
        himm = hi & 0xFFFF
        if (lo >> 26) == 14 and ((lo >> 21) & 31) == 3 and ((lo >> 16) & 31) == 3:
            simm = lo & 0xFFFF               # addi r3, r3, imm  (signed)
            if simm & 0x8000:
                simm -= 0x10000
            return ((himm << 16) + simm) & 0xFFFFFFFF
        if (lo >> 26) == 24 and ((lo >> 21) & 31) == 3 and ((lo >> 16) & 31) == 3:
            return ((himm << 16) | (lo & 0xFFFF)) & 0xFFFFFFFF   # ori
        return None
    if len(ws) == 2:
        li, ret = ws
        if (li >> 26) != 14 or ((li >> 21) & 31) != 3 or ((li >> 16) & 31) != 0:
            return None
        if ret != 0x4E800020:
            return None
        v = li & 0xFFFF
        if v & 0x8000:
            v -= 0x10000
        return v & 0xFFFFFFFF
    return None


def reencode(value, three):
    """Build the bytes back, so nothing is emitted that was not verified."""
    if not three:
        v = value if value < 0x8000 else value - 0x10000
        return struct.pack(">II", 0x38600000 | (v & 0xFFFF), 0x4E800020)
    lo = value & 0xFFFF
    hi = (value >> 16) & 0xFFFF
    if lo & 0x8000:
        hi = (hi + 1) & 0xFFFF
    return struct.pack(">III",
                       0x3C600000 | hi,
                       0x38630000 | lo,
                       0x4E800020)


def main():
    if len(sys.argv) != 4:
        sys.exit(__doc__)
    lo_a, hi_a, out = int(sys.argv[1], 16), int(sys.argv[2], 16), sys.argv[3]

    raw = ELF.read_bytes()
    f = ELFFile(open(ELF, "rb"))

    text = None
    for sh in sections(raw):
        if sh[3] and sh[3] <= lo_a < sh[3] + sh[5]:
            text = (sh[3], sh[4], sh[5])
            break
    if text is None:
        sys.exit("gen_typeids: %08X is not inside a loaded section" % lo_a)
    base, foff, _ = text

    rows = []
    for sec in f.iter_sections():
        if sec.header["sh_type"] != "SHT_SYMTAB":
            continue
        for s in sec.iter_symbols():
            if s["st_info"]["type"] != "STT_FUNC" or not s["st_size"]:
                continue
            v = s["st_value"]
            if lo_a <= v < hi_a:
                rows.append((v, s["st_size"], s.name))
    rows.sort()

    emitted, skipped, classes = [], [], {}
    for addr, size, sym in rows:
        if size not in (8, 12):
            skipped.append((sym, "%d bytes" % size))
            continue
        body = raw[foff + (addr - base): foff + (addr - base) + size]
        val = decode(body)
        if val is None:
            skipped.append((sym, "body is not a constant return"))
            continue
        if reencode(val, size == 12) != body:
            skipped.append((sym, "re-encoding does not reproduce the bytes"))
            continue
        d = demangle(sym)
        if d is None:
            skipped.append((sym, "symbol does not demangle to Class::m() const"))
            continue
        ns, cls, method = d
        key = (tuple(ns), cls)
        classes.setdefault(key, []).append(method)
        emitted.append((ns, cls, method, val, addr))

    if not emitted:
        sys.exit("gen_typeids: nothing in %08X..%08X matches the shape -- "
                 "refusing to write an empty file" % (lo_a, hi_a))

    NL = chr(10)
    L = []
    L.append("// GENERATED by tools/gen_typeids.py from the retail image.")
    L.append("// Do not hand-edit: regenerate.")
    L.append("//")
    L.append("// Every function here is one shape -- a 32-bit constant into")
    L.append("// r3 and a return -- and every constant was decoded from the")
    L.append("// image and re-encoded back to the same bytes before being")
    L.append("// written. These are GENERATED, not read: real matched")
    L.append("// functions whose constants are recovered fact, but a count")
    L.append("// of them is not a count of decompiled code.")
    L.append("//")
    L.append("// Members are declared NON-VIRTUAL on purpose. CodeWarrior")
    L.append("// mangles a virtual and a non-virtual member identically, so")
    L.append("// the symbol is the same either way, and a virtual")
    L.append("// declaration would emit a vtable this unit does not have.")
    L.append("")

    # Declarations, grouped by namespace, in first-use order.
    seen_ns = []
    for (ns, cls), methods in classes.items():
        if list(ns) not in seen_ns:
            seen_ns.append(list(ns))
    for ns in seen_ns:
        for n in ns:
            L.append("namespace %s {" % n)
        for (cns, cls), methods in classes.items():
            if list(cns) != ns:
                continue
            L.append("")
            L.append("class %s {" % cls)
            L.append("public:")
            for m in sorted(set(methods)):
                L.append("    unsigned int %s() const;" % m)
            L.append("};")
        L.append("")
        for n in reversed(ns):
            L.append("}  // namespace %s" % n)
        L.append("")

    # Definitions, in the image's own address order.
    for ns, cls, method, val, addr in emitted:
        q = "::".join(list(ns) + [cls])
        if ns:
            L.append("unsigned int %s::%s() const { return 0x%08Xu; }"
                     % (q, method, val))
        else:
            L.append("unsigned int %s::%s() const { return 0x%08Xu; }"
                     % (cls, method, val))

    Path(out).parent.mkdir(parents=True, exist_ok=True)
    Path(out).write_text(NL.join(L) + NL, encoding="utf-8")

    print("  %s: %d function(s) emitted of %d in %08X..%08X"
          % (out, len(emitted), len(rows), lo_a, hi_a))
    print("  %d skipped:" % len(skipped))
    why = {}
    for sym, reason in skipped:
        why[reason] = why.get(reason, 0) + 1
    for reason, c in sorted(why.items(), key=lambda kv: -kv[1]):
        print("    %-46s %d" % (reason, c))
    return 0


if __name__ == "__main__":
    sys.exit(main())
