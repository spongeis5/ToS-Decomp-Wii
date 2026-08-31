"""Generate two-instruction accessors for a unit from the image itself.

    python tools/gen_accessors.py --survey
    python tools/gen_accessors.py <start> <end> <out.cpp>

A sibling of tools/gen_typeids.py for the next shape down: a function whose
whole body is one load or one store and a return.

    lwz r3, N(r3) ; blr          int, unsigned int or a pointer
    lhz / lha / lbz / lfs        unsigned short, short, unsigned char, float
    stw r4, N(r3) ; blr          void Set(int)
    sth / stb / stfs             the same by width

THESE ARE GENERATED, NOT READ, and the same caveat applies as to the type
ids: real matched functions whose offsets are recovered fact, but a count of
them is not a count of decompiled code.

EVERY CANDIDATE IS RE-ENCODED AND COMPARED against the image before it is
emitted, and anything unrecognised is skipped and counted with its reason.
Three things are refused rather than guessed at:

  * a symbol whose parameter list is not one type from a small allowlist,
    because a wrong parameter type produces a different symbol and so a
    silent miss;
  * a class where two accessors disagree about the type at one offset, since
    only one of them can be right and the tool cannot tell which;
  * an accessor on a class in an anonymous namespace, which needs the file
    named after its unity blob (see tools/anon_blocked.py).
"""

import argparse
import json
import re
import struct
import sys
from collections import Counter
from pathlib import Path

from elftools.elf.elffile import ELFFile

ROOT = Path(__file__).resolve().parent.parent
ELF = ROOT / "orig/R8IE78/files/SB09WiiMASTERWAD.elf"
SPLITS = ROOT / "config/R8IE78/splits.txt"
REPORT = ROOT / "build/R8IE78/report.json"

# opcode -> (C type, width, is_float). The register fields are checked
# separately; only these opcodes are recognised at all.
LOADS = {
    32: ("int", 4, False),             # lwz
    40: ("unsigned short", 2, False),  # lhz
    42: ("short", 2, False),           # lha
    34: ("unsigned char", 1, False),   # lbz
    48: ("float", 4, True),            # lfs
}
STORES = {
    36: ("int", 4, False),             # stw
    44: ("unsigned short", 2, False),  # sth
    38: ("unsigned char", 1, False),   # stb
    52: ("float", 4, True),            # stfs
}

# CodeWarrior parameter encodings this tool will reproduce.
PARAM = {"i": "int", "Ui": "unsigned int", "l": "long", "Ul": "unsigned long",
         "f": "float", "s": "short", "Us": "unsigned short",
         "c": "char", "Uc": "unsigned char", "b": "bool"}

BLR = 0x4E800020



IDENT = re.compile(r'^[A-Za-z_][A-Za-z0-9_]*$')


def usable(names):
    """Every part must be a plain identifier.

    A length-prefixed CodeWarrior name can be a template instantiation
    -- `ImmediateContext<Q24Sext8VertexUV>` -- which is not a class name
    that can be declared. Refused rather than emitted: a generator that
    writes what it cannot reproduce is one step from one that writes
    something wrong and silent."""
    return all(IDENT.match(n) for n in names if n)

def sections(raw):
    off, = struct.unpack_from(">I", raw, 32)
    es, num, _si = struct.unpack_from(">HHH", raw, 46)
    return [struct.unpack_from(">IIIIIIIIII", raw, off + i * es)
            for i in range(num)]


def decode(body):
    """-> (kind, ctype, offset) or None. kind is 'get' or 'set'."""
    if len(body) != 8:
        return None
    a, b = struct.unpack(">II", body)
    if b != BLR:
        return None
    op = a >> 26
    d = (a >> 21) & 31
    base = (a >> 16) & 31
    imm = a & 0xFFFF
    if imm & 0x8000:
        return None                      # negative offsets are not members
    if base != 3:
        return None
    if op in LOADS:
        ctype, _w, isf = LOADS[op]
        if d != (1 if isf else 3):       # f1 for float, r3 otherwise
            return None
        return ("get", ctype, imm)
    if op in STORES:
        ctype, _w, isf = STORES[op]
        if d != (1 if isf else 4):       # f1 for float, r4 otherwise
            return None
        return ("set", ctype, imm)
    return None


def reencode(kind, ctype, off):
    table = LOADS if kind == "get" else STORES
    for op, (t, _w, isf) in table.items():
        if t != ctype:
            continue
        d = (1 if isf else (3 if kind == "get" else 4))
        return struct.pack(">II", (op << 26) | (d << 21) | (3 << 16) | off,
                           BLR)
    return None


def demangle(sym, kind):
    """-> (namespaces, class, method, is_const, param) or None."""
    if "__" not in sym or "@unnamed@" in sym:
        return None
    method, rest = sym.split("__", 1)
    if not method:
        return None
    parts = []
    if rest.startswith("Q"):
        m = re.match(r"Q(\d)(.*)$", rest)
        if not m:
            return None
        count, rest = int(m.group(1)), m.group(2)
        for _ in range(count):
            m2 = re.match(r"(\d+)(.*)$", rest)
            if not m2:
                return None
            ln = int(m2.group(1))
            parts.append(m2.group(2)[:ln])
            rest = m2.group(2)[ln:]
    else:
        m = re.match(r"(\d+)(.*)$", rest)
        if not m:
            return None
        ln = int(m.group(1))
        parts.append(m.group(2)[:ln])
        rest = m.group(2)[ln:]
    if not parts:
        return None
    is_const = rest.startswith("C")
    if is_const:
        rest = rest[1:]
    if not rest.startswith("F"):
        return None
    args = rest[1:]
    if kind == "get":
        if args != "v":
            return None
        return parts[:-1], parts[-1], method, is_const, None
    if args not in PARAM:
        return None
    return parts[:-1], parts[-1], method, is_const, PARAM[args]


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


def load_image():
    raw = ELF.read_bytes()
    ranges = unit_ranges()
    probe = next(rs[0][0] for _u, rs in ranges if rs)
    for sh in sections(raw):
        if sh[3] and sh[3] <= probe < sh[3] + sh[5]:
            return raw, sh[3], sh[4], ranges
    sys.exit("gen_accessors: cannot locate the section holding .text")


def symbols():
    out = []
    with open(ELF, "rb") as fh:
        f = ELFFile(fh)
        for sec in f.iter_sections():
            if sec.header["sh_type"] != "SHT_SYMTAB":
                continue
            for s in sec.iter_symbols():
                if s["st_info"]["type"] == "STT_FUNC" and s["st_size"] == 8:
                    out.append((s["st_value"], s.name))
    out.sort()
    return out


def candidates(raw, base, foff, syms, lo, hi):
    good, skipped = [], Counter()
    for addr, sym in syms:
        if not (lo <= addr < hi):
            continue
        body = raw[foff + (addr - base): foff + (addr - base) + 8]
        d = decode(body)
        if d is None:
            skipped["body is not a single load or store"] += 1
            continue
        kind, ctype, off = d
        if reencode(kind, ctype, off) != body:
            skipped["re-encoding does not reproduce the bytes"] += 1
            continue
        dm = demangle(sym, kind)
        if dm is None:
            skipped["symbol is not a plain member with a known signature"] += 1
            continue
        ns, cls, method, is_const, param = dm
        if not usable(list(ns) + [cls, method]):
            skipped["class or method name is not a plain identifier"] += 1
            continue
        good.append((addr, ns, cls, method, is_const, kind, ctype, off, param))
    return good, skipped


def survey():
    raw, base, foff, ranges = load_image()
    syms = symbols()
    game = None
    if REPORT.exists():
        rep = json.loads(REPORT.read_text(encoding="utf-8"))
        game = set()
        for u in rep["units"]:
            meta = u.get("metadata", {})
            if "game" in (meta.get("progress_categories") or []):
                nm = u["name"].split("/", 1)[1] if "/" in u["name"] \
                    else u["name"]
                game.add(nm + ".cpp")
    rows = []
    for unit, rs in ranges:
        if game is not None and unit not in game:
            continue
        n = 0
        for lo, hi in rs:
            g, _ = candidates(raw, base, foff, syms, lo, hi)
            n += len(g)
        if n:
            rows.append((n, unit))
    rows.sort(reverse=True)
    for n, unit in rows:
        print("  %-5d %s" % (n, unit))
    print("")
    print("  %d game unit(s) hold the shape; %d accessor(s), %d bytes"
          % (len(rows), sum(r[0] for r in rows), 8 * sum(r[0] for r in rows)))
    return 0


WIDTH = {"int": 4, "unsigned int": 4, "long": 4, "unsigned long": 4,
         "float": 4, "short": 2, "unsigned short": 2,
         "char": 1, "unsigned char": 1, "bool": 1}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--survey", action="store_true")
    ap.add_argument("rest", nargs="*")
    args = ap.parse_args()
    if args.survey:
        return survey()
    if len(args.rest) != 3:
        sys.exit(__doc__)
    lo = int(args.rest[0], 16)
    hi = int(args.rest[1], 16)
    out = args.rest[2]

    raw, base, foff, _ranges = load_image()
    good, skipped = candidates(raw, base, foff, symbols(), lo, hi)
    if not good:
        sys.exit("gen_accessors: nothing in %08X..%08X matches the shape -- "
                 "refusing to write an empty file" % (lo, hi))

    # One field per (class, offset). A disagreement means one of them is
    # wrong and the tool cannot tell which, so BOTH accessors are dropped.
    fields, bad_off = {}, set()
    for _a, ns, cls, _m, _c, _k, ctype, off, _p in good:
        key = (tuple(ns), cls)
        prev = fields.setdefault(key, {}).get(off)
        if prev is not None and prev != ctype:
            bad_off.add((key, off))
        fields[key][off] = ctype

    emitted = []
    for row in good:
        key = (tuple(row[1]), row[2])
        if (key, row[7]) in bad_off:
            skipped["two accessors disagree about the type at one offset"] += 1
            continue
        emitted.append(row)
    if not emitted:
        sys.exit("gen_accessors: every candidate was dropped")

    methods = {}
    for _a, ns, cls, m, c, k, ctype, off, p in emitted:
        methods.setdefault((tuple(ns), cls), []).append((m, c, k, ctype, p))

    NL = chr(10)
    L = ["// GENERATED by tools/gen_accessors.py from the retail image.",
         "// Do not hand-edit: regenerate.",
         "//",
         "// Every function here is one load or one store and a return. Each",
         "// offset was decoded from the image and re-encoded back to the",
         "// same bytes before being written. GENERATED, not read: real",
         "// matched functions whose offsets are recovered fact, but a count",
         "// of them is not a count of decompiled code.",
         "//",
         "// Members are non-virtual, and the padding is padding -- only the",
         "// offsets each accessor touches are known, not the fields between.",
         ""]

    for key in methods:
        ns, cls = list(key[0]), key[1]
        for n in ns:
            L.append("namespace %s {" % n)
        L.append("")
        L.append("class %s {" % cls)
        L.append("public:")
        for m, c, k, ctype, p in sorted(set(methods[key])):
            if k == "get":
                L.append("    %s %s()%s;" % (ctype, m, " const" if c else ""))
            else:
                L.append("    void %s(%s value);" % (m, p))
        L.append("")
        pad = 0
        for i, off in enumerate(sorted(fields[key])):
            if off > pad:
                L.append("    unsigned char _pad%d[0x%X];" % (i, off - pad))
            ctype = fields[key][off]
            L.append("    %s f%X;" % (ctype, off))
            pad = off + WIDTH[ctype]
        L.append("};")
        L.append("")
        for n in reversed(ns):
            L.append("}  // namespace %s" % n)
        L.append("")

    for _a, ns, cls, m, c, k, ctype, off, p in emitted:
        q = "::".join(list(ns) + [cls])
        if k == "get":
            L.append("%s %s::%s()%s { return f%X; }"
                     % (ctype, q, m, " const" if c else "", off))
        else:
            L.append("void %s::%s(%s value) { f%X = value; }"
                     % (q, m, p, off))

    Path(out).parent.mkdir(parents=True, exist_ok=True)
    Path(out).write_text(NL.join(L) + NL, encoding="utf-8")

    total = len(emitted) + sum(skipped.values())
    print("  %s: %d accessor(s) emitted of %d eight-byte function(s) in "
          "%08X..%08X" % (out, len(emitted), total, lo, hi))
    for why, c in skipped.most_common():
        print("    skipped %-52s %d" % (why, c))
    return 0


if __name__ == "__main__":
    sys.exit(main())
