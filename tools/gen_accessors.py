"""Generate the member-only functions of a unit from the retail image.

    python tools/gen_accessors.py --survey
    python tools/gen_accessors.py <start> <end> <out.cpp>

A sibling of tools/gen_typeids.py for the shapes below it: a function whose
whole body touches nothing but its own object's members.

    lwz  r3, N(r3) ; blr         int, unsigned int or a pointer   -- get
    lhz / lha / lbz / lfs        unsigned short, short, unsigned char, float
    stw  r4, N(r3) ; blr         void Set(int)                    -- set
    sth / stb / stfs             the same by width
    addi r3, r3, N ; blr         T* Get() { return &member; }     -- ref
    addi r3, r0, K ; blr         a constant return, WITH arguments -- constret
    li   r0, K ; stw r0, A(r3)   member(s) set to one constant    -- constset
      [; stw r0, B(r3) ...] ; blr

THESE ARE GENERATED, NOT READ, and the same caveat applies as to the type
ids: real matched functions whose offsets are recovered fact, but a count of
them is not a count of decompiled code.

`constret` is deliberately restricted to functions that TAKE ARGUMENTS.
tools/gen_typeids.py owns the argument-free ones and refuses the rest, so
the two tools partition that shape between them rather than both emitting
it into different files for the same unit.

EVERY CANDIDATE IS RE-ENCODED AND COMPARED against the image before it is
emitted, and anything unrecognised is skipped and counted with its reason.
These are refused rather than guessed at:

  * a parameter list this file's mangled-type parser cannot RE-MANGLE back
    to the bytes it was read from, because a wrong parameter type produces
    a different symbol and so a silent miss;
  * a class where two functions disagree about the type at one offset, or
    where the offsets overlap, or where an offset is not aligned for the
    type it holds -- in each case the layout the compiler would build is
    not the layout that was measured, and only one of them can be right;
  * a member function on a class in an anonymous namespace, which needs the
    file named after its unity blob (see tools/anon_blocked.py);
  * a by-value class-or-enum parameter whose name is also a class this file
    defines, since it cannot be both.

A by-value named parameter is emitted as an ENUM with one large enumerator,
which fixes its width at four bytes whatever the -enum setting is.
CodeWarrior mangles a nested name the same whether the outer scope is a
class or a namespace, so a namespace is used for every qualified name and
the symbol comes out the same either way.
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

sys.path.insert(0, str(ROOT / "tools"))
import gen_typeids as G                                  # noqa: E402

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


# --------------------------------------------------------------------------
# Mangled parameter types.
#
# A node is one of ('basic', name) ('name', (parts...)) ('ptr', node)
# ('ref', node) ('const', node). Everything read is mangled back and the
# result compared with what was read; a type that does not round-trip is
# refused, because it would be emitted as a symbol nobody is looking for.
# --------------------------------------------------------------------------

BASIC = {"v": "void", "c": "char", "s": "short", "i": "int", "l": "long",
         "f": "float", "d": "double", "b": "bool", "x": "long long"}
UNSIGNED = {"c": "unsigned char", "s": "unsigned short", "i": "unsigned int",
            "l": "unsigned long", "x": "unsigned long long"}
SIGNED = {"c": "signed char"}

REV = {}
for _k, _v in BASIC.items():
    REV[_v] = _k
for _k, _v in UNSIGNED.items():
    REV[_v] = "U" + _k
for _k, _v in SIGNED.items():
    REV[_v] = "S" + _k

# Widths for the parameter types a `set` may take. A pointer, a reference
# and a named by-value type are all four bytes here.
PARAM_WIDTH = {"char": 1, "signed char": 1, "unsigned char": 1, "bool": 1,
               "short": 2, "unsigned short": 2, "int": 4, "unsigned int": 4,
               "long": 4, "unsigned long": 4, "float": 4}


def parse_type(s):
    """-> (node, rest) or None."""
    if not s:
        return None
    c = s[0]
    if c in "PRC":
        inner = parse_type(s[1:])
        if inner is None:
            return None
        node, rest = inner
        return ({"P": "ptr", "R": "ref", "C": "const"}[c], node), rest
    if c == "U":
        if len(s) < 2 or s[1] not in UNSIGNED:
            return None
        return ("basic", UNSIGNED[s[1]]), s[2:]
    if c == "S":
        if len(s) < 2 or s[1] not in SIGNED:
            return None
        return ("basic", SIGNED[s[1]]), s[2:]
    if c == "Q":
        m = re.match(r"Q(\d)(.*)$", s)
        if not m:
            return None
        rest, parts = m.group(2), []
        for _ in range(int(m.group(1))):
            m2 = re.match(r"(\d+)(.*)$", rest)
            if not m2:
                return None
            ln = int(m2.group(1))
            if len(m2.group(2)) < ln:
                return None
            parts.append(m2.group(2)[:ln])
            rest = m2.group(2)[ln:]
        return ("name", tuple(parts)), rest
    if c.isdigit():
        m = re.match(r"(\d+)(.*)$", s)
        ln = int(m.group(1))
        if len(m.group(2)) < ln:
            return None
        return ("name", (m.group(2)[:ln],)), m.group(2)[ln:]
    if c in BASIC:
        return ("basic", BASIC[c]), s[1:]
    return None


def mangle_type(node):
    kind = node[0]
    if kind == "basic":
        return REV[node[1]]
    if kind == "name":
        parts = node[1]
        body = "".join("%d%s" % (len(p), p) for p in parts)
        return body if len(parts) == 1 else "Q%d%s" % (len(parts), body)
    return {"ptr": "P", "ref": "R", "const": "C"}[kind] + mangle_type(node[1])


def render_type(node):
    kind = node[0]
    if kind == "basic":
        return node[1]
    if kind == "name":
        return "::".join(node[1])
    if kind == "ptr":
        return render_type(node[1]) + "*"
    if kind == "ref":
        return render_type(node[1]) + "&"
    # const of a pointer or reference is a TOP-LEVEL const and has to be
    # written after it; `const Foo*` mangles as PC7Foo, not CP7Foo.
    inner = node[1]
    if inner[0] in ("ptr", "ref"):
        return render_type(inner) + " const"
    return "const " + render_type(inner)


def value_width(node):
    """Bytes a by-value parameter of this type occupies, or None."""
    if node[0] == "basic":
        return PARAM_WIDTH.get(node[1])
    if node[0] in ("ptr", "ref", "name"):
        return 4
    if node[0] == "const":
        return value_width(node[1])
    return None


def base_kind(node):
    """The node kind with const peeled off."""
    return base_kind(node[1]) if node[0] == "const" else node[0]


def stored_value(node, ctype):
    """The expression a `set` assigns, cast when the types differ.

    A pointer or an enum stored with `stw` lands in an `int` member; the
    cast is what makes that legal C++ and it emits the same store.
    """
    return "value" if base_kind(node) == "basic" else "(%s)value" % ctype


def type_names(node, indirect, out):
    kind = node[0]
    if kind == "name":
        out.append((node[1], not indirect))
    elif kind in ("ptr", "ref"):
        type_names(node[1], True, out)
    elif kind == "const":
        type_names(node[1], indirect, out)


def parse_params(args):
    """-> list of nodes, or None if the list does not round-trip."""
    if args == "v":
        return []
    out, rest = [], args
    while rest:
        got = parse_type(rest)
        if got is None:
            return None
        node, rest = got
        out.append(node)
    if "".join(mangle_type(n) for n in out) != args:
        return None
    for n in out:
        parts = []
        type_names(n, False, parts)
        if not all(usable(list(p)) for p, _v in parts):
            return None
    return out


def split_symbol(sym):
    """-> (namespaces, class or None, method, is_const, arg string) or None.

    A class of None is a free function. Only the constant-return shape
    accepts one: a free function storing through r3 is writing through a
    pointer argument, which is a different thing from an accessor.
    """
    if "@unnamed@" in sym:
        return None
    if sym.startswith("__ct__"):
        method, rest = "__ct__", sym[6:]
    else:
        if "__" not in sym:
            return None
        method, rest = sym.split("__", 1)
        if not method:
            return None
    parts = []
    if rest.startswith("F"):
        return (), None, method, False, rest[1:]
    if rest.startswith("Q"):
        m = re.match(r"Q(\d)(.*)$", rest)
        if not m:
            return None
        rest = m.group(2)
        for _ in range(int(m.group(1))):
            m2 = re.match(r"(\d+)(.*)$", rest)
            if not m2 or len(m2.group(2)) < int(m2.group(1)):
                return None
            ln = int(m2.group(1))
            parts.append(m2.group(2)[:ln])
            rest = m2.group(2)[ln:]
    elif rest[:1].isdigit():
        m = re.match(r"(\d+)(.*)$", rest)
        ln = int(m.group(1))
        if len(m.group(2)) < ln:
            return None
        parts.append(m.group(2)[:ln])
        rest = m.group(2)[ln:]
    else:
        return None
    if not parts:
        return None
    is_const = rest.startswith("C")
    if is_const:
        rest = rest[1:]
    if not rest.startswith("F"):
        return None
    return parts[:-1], parts[-1], method, is_const, rest[1:]


# --------------------------------------------------------------------------
# The bodies.
# --------------------------------------------------------------------------

def decode(body):
    """-> a shape tuple, or None.

    ("get", ctype, off) / ("set", ctype, off) / ("ref", off)
    ("constret", value) / ("constset", value, (off, ...))
    """
    n = len(body)
    if n % 4 or n < 8:
        return None
    ws = [struct.unpack_from(">I", body, i)[0] for i in range(0, n, 4)]
    if ws[-1] != BLR:
        return None

    if n == 8:
        a = ws[0]
        op, d, base, imm = a >> 26, (a >> 21) & 31, (a >> 16) & 31, a & 0xFFFF
        if op == 14 and d == 3 and base == 3 and not (imm & 0x8000) and imm:
            return ("ref", imm)
        if imm & 0x8000:
            return None                  # negative offsets are not members
        if base != 3:
            return None
        if op in LOADS:
            ctype, _w, isf = LOADS[op]
            if d != (1 if isf else 3):   # f1 for float, r3 otherwise
                return None
            return ("get", ctype, imm)
        if op in STORES:
            ctype, _w, isf = STORES[op]
            if d != (1 if isf else 4):   # f1 for float, r4 otherwise
                return None
            return ("set", ctype, imm)
        return None

    # li r0, K then one or more stw r0, N(r3)
    li = ws[0]
    if (li >> 26) != 14 or ((li >> 21) & 31) != 0 or ((li >> 16) & 31) != 0:
        return None
    v = li & 0xFFFF
    if v & 0x8000:
        v -= 0x10000
    offs = []
    for w in ws[1:-1]:
        if (w >> 26) != 36 or ((w >> 21) & 31) != 0 or ((w >> 16) & 31) != 3:
            return None
        off = w & 0xFFFF
        if off & 0x8000:
            return None
        offs.append(off)
    if not offs:
        return None
    return ("constset", v, tuple(offs))


def reencode(shape):
    kind = shape[0]
    if kind == "ref":
        return struct.pack(">II", 0x38630000 | shape[1], BLR)
    if kind in ("get", "set"):
        table = LOADS if kind == "get" else STORES
        ctype, off = shape[1], shape[2]
        for op, (t, _w, isf) in table.items():
            if t != ctype:
                continue
            d = (1 if isf else (3 if kind == "get" else 4))
            return struct.pack(">II",
                               (op << 26) | (d << 21) | (3 << 16) | off, BLR)
        return None
    if kind == "constset":
        v, offs = shape[1], shape[2]
        ws = [0x38000000 | (v & 0xFFFF)]
        ws += [0x90030000 | o for o in offs]
        ws.append(BLR)
        return struct.pack(">" + "I" * len(ws), *ws)
    return None


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
    """Function symbols short enough to hold one of the shapes."""
    out = []
    with open(ELF, "rb") as fh:
        f = ELFFile(fh)
        for sec in f.iter_sections():
            if sec.header["sh_type"] != "SHT_SYMTAB":
                continue
            for s in sec.iter_symbols():
                if (s["st_info"]["type"] == "STT_FUNC"
                        and s["st_size"] in (8, 12, 16, 20)):
                    out.append((s["st_value"], s["st_size"], s.name))
    out.sort()
    return out


def loaded_spans(raw):
    """[(start, end)] of every section the image loads at an address.

    A constant return whose value lands in one of these is an ADDRESS, and
    retail reaches it through a relocation; writing the number reproduces
    the instruction word but not the relocation, so the object differs by
    that field. unitcmp masks relocated fields and calls such a function
    identical -- report.json does not, and report.json is right.

    Measured across all 245 constant returns in the game code: 230 land
    outside a section and every one of them matches; 15 land inside and not
    one of them does. The test separates them exactly.
    """
    return [(sh[3], sh[3] + sh[5]) for sh in sections(raw) if sh[3] and sh[5]]


def candidates(raw, base, foff, syms, lo, hi):
    """-> (list of candidate dicts, Counter of reasons for the rest)."""
    good, skipped = [], Counter()
    spans = loaded_spans(raw)
    for addr, size, sym in syms:
        if not (lo <= addr < hi):
            continue
        body = raw[foff + (addr - base): foff + (addr - base) + size]

        shape = decode(body)
        const = None
        if shape is None:
            # The constant-return shape. Its decoder lives in gen_typeids,
            # which is where it was written and measured; this imports it
            # rather than keeping a second copy that could drift.
            const = G.decode(body) if size in (8, 12) else None
            if const is None:
                skipped["body is not one of the shapes"] += 1
                continue
            if G.reencode(const, size == 12) != body:
                skipped["re-encoding does not reproduce the bytes"] += 1
                continue
        elif reencode(shape) != body:
            skipped["re-encoding does not reproduce the bytes"] += 1
            continue

        dm = split_symbol(sym)
        if dm is None:
            skipped["symbol is not a member of a nameable class"] += 1
            continue
        ns, cls, method, is_const, args = dm
        if cls is None and const is None:
            # A free function is not a member, so it has no members to
            # touch; only the constant return can be one.
            skipped["a free function with a member-shaped body"] += 1
            continue
        names = (list(ns) + ([cls] if cls else [])
                 + ([] if method == "__ct__" else [method]))
        if not usable(names):
            skipped["class or method name is not a plain identifier"] += 1
            continue
        params = parse_params(args)
        if params is None:
            skipped["parameter list does not re-mangle to the same bytes"] += 1
            continue

        if const is not None:
            if any(a <= const < b for a, b in spans):
                skipped["the constant is an address and needs a "
                        "relocation"] += 1
                continue
            shape = ("constret", const)
        elif shape[0] == "set":
            if len(params) != 1:
                skipped["a setter whose body stores one value takes one "
                        "argument"] += 1
                continue
            w = value_width(params[0])
            want = {t: wd for t, wd, _f in STORES.values()}
            if w is None or w != want[shape[1]]:
                skipped["argument width does not match the store"] += 1
                continue
            if base_kind(params[0]) == "ref":
                # `stw r4` on a reference argument stores the address, and
                # writing that is a different statement from a plain
                # assignment. Refused rather than spelled two ways.
                skipped["a reference argument stored by value"] += 1
                continue
        good.append({"addr": addr, "size": size, "ns": tuple(ns), "cls": cls,
                     "method": method, "const": is_const, "shape": shape,
                     "params": params, "sym": sym})
    return good, skipped


def from_report(tool):
    if not REPORT.exists():
        sys.exit("%s: %s is missing -- run ninja first. Without it every "
                 "already-matched function counts as remaining work."
                 % (tool, REPORT))
    rep = json.loads(REPORT.read_text(encoding="utf-8"))
    game, matched = set(), set()
    for u in rep["units"]:
        meta = u.get("metadata", {})
        if "game" not in (meta.get("progress_categories") or []):
            continue
        nm = u["name"].split("/", 1)[1] if "/" in u["name"] else u["name"]
        game.add(nm + ".cpp")
        for fn in u.get("functions", []):
            if fn.get("fuzzy_match_percent") == 100.0:
                matched.add(fn["name"])
    if not game:
        sys.exit("%s: report.json names no game unit -- the category filter "
                 "found nothing, which is not a survey of no work left."
                 % tool)
    return game, matched


def survey():
    raw, base, foff, ranges = load_image()
    # The survey answers "what is LEFT", so report.json is required, not
    # optional: it supplies both the game-unit filter and the set of
    # functions already matching. Without the second the survey counts work
    # already done -- which is what it did until 2026-08-31, in both this
    # tool and gen_survey.py.
    game, matched = from_report("gen_accessors")
    syms = [(a, s, n) for a, s, n in symbols() if n not in matched]

    rows, kinds = [], Counter()
    for unit, rs in ranges:
        if unit not in game:
            continue
        n = b = 0
        for lo, hi in rs:
            g, _ = candidates(raw, base, foff, syms, lo, hi)
            n += len(g)
            b += sum(c["size"] for c in g)
            for c in g:
                kinds[c["shape"][0]] += 1
        if n:
            rows.append((n, b, unit))
    rows.sort(reverse=True)
    print("  %-5s %-6s %s" % ("N", "BYTES", "UNIT"))
    for n, b, unit in rows:
        print("  %-5d %-6d %s" % (n, b, unit))
    print("")
    for k, n in kinds.most_common():
        print("    %-10s %d" % (k, n))
    print("")
    print("  %d game unit(s) hold a shape; %d function(s), %d bytes"
          % (len(rows), sum(r[0] for r in rows), sum(r[1] for r in rows)))
    return 0


WIDTH = {"int": 4, "unsigned int": 4, "long": 4, "unsigned long": 4,
         "float": 4, "short": 2, "unsigned short": 2,
         "char": 1, "unsigned char": 1, "bool": 1}


def field_types(good, skipped):
    """-> {(ns, cls): {off: ctype}}, having dropped what cannot be laid out.

    A `ref` names an offset without saying what is there; it takes whatever
    another function proves, and `int` when nothing does.
    """
    fields, bad = {}, set()
    for c in good:
        key = (c["ns"], c["cls"])
        shape = c["shape"]
        # A class whose only shape is a constant return proves no offset at
        # all, and still has to appear here or it has no layout to emit.
        fields.setdefault(key, {})
        for off, ctype in offsets_of(shape):
            prev = fields.setdefault(key, {}).get(off)
            if ctype is None:
                fields[key].setdefault(off, None)
                continue
            if prev is not None and prev != ctype:
                bad.add((key, off))
            fields[key][off] = ctype

    # Lay each class out and refuse one that cannot be: an offset that is
    # not aligned for its type, or a field that would overlap the next.
    for key, offs in fields.items():
        end = 0
        for off in sorted(offs):
            ctype = offs[off] or "int"
            if off % WIDTH[ctype]:
                bad.add((key, off))
            if off < end:
                bad.add((key, off))
            end = off + WIDTH[ctype]

    keep = []
    for c in good:
        key = (c["ns"], c["cls"])
        if any((key, off) in bad for off, _t in offsets_of(c["shape"])):
            skipped["the class cannot be laid out from what was measured"] += 1
            continue
        keep.append(c)
    return fields, keep


def offsets_of(shape):
    """-> [(offset, ctype or None)] the shape proves about the class."""
    if shape[0] in ("get", "set"):
        return [(shape[2], shape[1])]
    if shape[0] == "ref":
        return [(shape[1], None)]
    if shape[0] == "constset":
        return [(off, "int") for off in shape[2]]
    return []


def param_decls(params, setter):
    if not params:
        return ""
    if setter:
        return "%s value" % render_type(params[0])
    return ", ".join("%s a%d" % (render_type(p), i)
                     for i, p in enumerate(params))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--survey", action="store_true")
    ap.add_argument("--unit", help="a unit name from splits.txt; covers ALL "
                                   "of its .text ranges, which an address "
                                   "pair cannot when a unit is split around "
                                   "an interior one")
    ap.add_argument("rest", nargs="*")
    args = ap.parse_args()
    if args.survey:
        return survey()

    raw, base, foff, ranges = load_image()
    if args.unit:
        if len(args.rest) != 1:
            sys.exit(__doc__)
        out = args.rest[0]
        rs = [r for u, r in ranges if u == args.unit]
        if not rs:
            sys.exit("gen_accessors: splits.txt has no unit %r" % args.unit)
        spans = rs[0]
        lo, hi = min(a for a, _b in spans), max(b for _a, b in spans)
    else:
        if len(args.rest) != 3:
            sys.exit(__doc__)
        lo = int(args.rest[0], 16)
        hi = int(args.rest[1], 16)
        out = args.rest[2]
        spans = [(lo, hi)]

    syms = symbols()
    good, skipped = [], Counter()
    for a, b in spans:
        g, s = candidates(raw, base, foff, syms, a, b)
        good += g
        skipped += s
    if not good:
        sys.exit("gen_accessors: nothing in %08X..%08X matches a shape -- "
                 "refusing to write an empty file" % (lo, hi))

    fields, good = field_types(good, skipped)

    # Named parameter types. A name used by value has to be DEFINED -- the
    # only definition this tool writes is an enum -- and a name used only
    # through a pointer or a reference needs a declaration. Either can
    # collide with a class this file defines: a parameter of type
    # `Graphics::MovieBase` needs `namespace Graphics`, and a class called
    # `Graphics` cannot also be a namespace. A forward declaration of a
    # class this file goes on to define is fine and is not a collision.
    defined = {(c["ns"], c["cls"]) for c in good}

    def collides(parts, is_value):
        scopes = [parts[:i + 1] for i in range(len(parts) - 1)]
        if is_value:
            scopes.append(parts)
        return any((s[:-1], s[-1]) in defined for s in scopes)

    keep = []
    for c in good:
        got = []
        for p in c["params"]:
            type_names(p, False, got)
        if any(collides(parts, v) for parts, v in got):
            skipped["a parameter type collides with a class this file "
                    "defines"] += 1
            continue
        keep.append(c)
    good = keep

    byvalue, byref = {}, {}
    for c in good:
        for p in c["params"]:
            got = []
            type_names(p, False, got)
            for parts, is_value in got:
                (byvalue if is_value else byref)[parts] = True
    if not good:
        sys.exit("gen_accessors: every candidate was dropped")

    methods, frees = {}, []
    for c in good:
        if c["cls"] is None:
            frees.append(c)
        else:
            methods.setdefault((c["ns"], c["cls"]), []).append(c)

    NL = chr(10)
    L = ["// GENERATED by tools/gen_accessors.py from the retail image.",
         "// Do not hand-edit: regenerate.",
         "//",
         "// Every function here touches nothing but its own members or a",
         "// constant: one load, one store, the address of a member, a",
         "// constant return, or members set to one constant. Each body was",
         "// decoded from the image and re-encoded back to the same bytes,",
         "// and each parameter list re-mangled back to the same symbol,",
         "// before being written. GENERATED, not read: real matched",
         "// functions whose offsets are recovered fact, but a count of them",
         "// is not a count of decompiled code.",
         "//",
         "// Members are non-virtual, and the padding is padding -- only the",
         "// offsets each function touches are known, not the fields between.",
         ""]

    for parts in sorted(byvalue):
        for n in parts[:-1]:
            L.append("namespace %s {" % n)
        L.append("enum %s { %s_ = 0x7FFFFFFF };" % (parts[-1], parts[-1]))
        for n in reversed(parts[:-1]):
            L.append("}  // namespace %s" % n)
    for parts in sorted(byref):
        if parts in byvalue:
            continue
        line = "class %s;" % parts[-1]
        for n in reversed(parts[:-1]):
            line = "namespace %s { %s }" % (n, line)
        L.append(line)
    if byvalue or byref:
        L.append("")

    for key in methods:
        ns, cls = list(key[0]), key[1]
        for n in ns:
            L.append("namespace %s {" % n)
        L.append("")
        L.append("class %s {" % cls)
        L.append("public:")
        seen = set()
        for c in sorted(methods[key], key=lambda c: (c["method"], c["addr"])):
            decl = declare(c, fields[key])
            if decl in seen:
                continue
            seen.add(decl)
            L.append(decl)
        L.append("")
        pad = 0
        for i, off in enumerate(sorted(fields[key])):
            if off > pad:
                L.append("    unsigned char _pad%d[0x%X];" % (i, off - pad))
            ctype = fields[key][off] or "int"
            L.append("    %s f%X;" % (ctype, off))
            pad = off + WIDTH[ctype]
        L.append("};")
        L.append("")
        for n in reversed(ns):
            L.append("}  // namespace %s" % n)
        L.append("")

    if frees:
        for c in sorted(frees, key=lambda c: c["method"]):
            L.append("unsigned int %s(%s);"
                     % (c["method"], param_decls(c["params"], False)))
        L.append("")

    for c in good:
        L.append(define(c, fields[(c["ns"], c["cls"])]))

    Path(out).parent.mkdir(parents=True, exist_ok=True)
    Path(out).write_text(NL.join(L) + NL, encoding="utf-8")

    total = len(good) + sum(skipped.values())
    by = Counter(c["shape"][0] for c in good)
    print("  %s: %d function(s) emitted of %d short function(s) in "
          "%08X..%08X, %d bytes"
          % (out, len(good), total, lo, hi, sum(c["size"] for c in good)))
    print("    " + ", ".join("%s %d" % (k, n) for k, n in by.most_common()))
    for why, n in skipped.most_common():
        print("    skipped %-54s %d" % (why, n))
    return 0


def ref_type(c, flds):
    """The pointee type a `ref` hands back."""
    t = flds[c["shape"][1]] or "int"
    return ("const " + t) if c["const"] else t


def declare(c, flds):
    """The in-class declaration for one candidate."""
    kind = c["shape"][0]
    cst = " const" if c["const"] else ""
    if c["method"] == "__ct__":
        return "    %s(%s);" % (c["cls"], param_decls(c["params"], False))
    if kind == "get":
        return "    %s %s(%s)%s;" % (c["shape"][1], c["method"],
                                     param_decls(c["params"], False), cst)
    if kind == "set":
        return "    void %s(%s)%s;" % (c["method"],
                                       param_decls(c["params"], True), cst)
    if kind == "ref":
        # A const member cannot hand out a pointer to its own member
        # without the const; the return type is not mangled, so this is
        # free.
        t = ref_type(c, flds)
        return "    %s* %s(%s)%s;" % (t, c["method"],
                                      param_decls(c["params"], False), cst)
    if kind == "constret":
        return "    unsigned int %s(%s)%s;" % (
            c["method"], param_decls(c["params"], False), cst)
    if kind == "constset":
        return "    void %s(%s)%s;" % (c["method"],
                                       param_decls(c["params"], False), cst)
    raise AssertionError(kind)


def define(c, flds):
    q = "::".join(list(c["ns"]) + ([c["cls"]] if c["cls"] else []))
    kind = c["shape"][0]
    cst = " const" if c["const"] else ""
    if c["method"] == "__ct__":
        body = " ".join("f%X = %d;" % (off, c["shape"][1])
                        for off in c["shape"][2])
        return "%s::%s(%s) { %s }" % (q, c["cls"],
                                      param_decls(c["params"], False), body)
    if kind == "get":
        return "%s %s::%s(%s)%s { return f%X; }" % (
            c["shape"][1], q, c["method"], param_decls(c["params"], False),
            cst, c["shape"][2])
    if kind == "set":
        return "void %s::%s(%s)%s { f%X = %s; }" % (
            q, c["method"], param_decls(c["params"], True), cst,
            c["shape"][2], stored_value(c["params"][0], c["shape"][1]))
    if kind == "ref":
        t = ref_type(c, flds)
        return "%s* %s::%s(%s)%s { return &f%X; }" % (
            t, q, c["method"], param_decls(c["params"], False), cst,
            c["shape"][1])
    if kind == "constret":
        if c["cls"] is None:
            return "unsigned int %s(%s) { return 0x%08Xu; }" % (
                c["method"], param_decls(c["params"], False), c["shape"][1])
        return "unsigned int %s::%s(%s)%s { return 0x%08Xu; }" % (
            q, c["method"], param_decls(c["params"], False), cst,
            c["shape"][1])
    if kind == "constset":
        body = " ".join("f%X = %d;" % (off, c["shape"][1])
                        for off in c["shape"][2])
        return "void %s::%s(%s)%s { %s }" % (
            q, c["method"], param_decls(c["params"], False), cst, body)
    raise AssertionError(kind)


if __name__ == "__main__":
    sys.exit(main())
