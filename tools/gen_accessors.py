"""Generate a unit's member-only, constant and one-global functions.

    python tools/gen_accessors.py --survey
    python tools/gen_accessors.py --unit <unit> <out.cpp>
    python tools/gen_accessors.py <start> <end> <out.cpp>

A function whose whole body touches nothing but its own object's members,
one global, a constant, or its own base constructor. Ten shapes:

    lwz  r3, N(r3) ; blr         int, unsigned int or a pointer   -- get
    lhz / lha / lbz / lfs        unsigned short, short, unsigned char, float
    stw  r4, N(r3) ; blr         void Set(int)                    -- set
    sth / stb / stfs             the same by width
    addi r3, r3, N ; blr         T* Get() { return &member; }     -- ref
    lis/addi r3, K ; blr         a constant return                -- constret
    li   r0, K ; st* r0, A(r3)   member(s) set to one constant    -- constset
      [; st* r0, B(r3) ...] ; blr    each store at its own width
    lis r4 ; addi r4, __vt__C    a constructor storing its own    -- vtable
      ; stw r4, 0(r3) ; blr          vtable pointer, and nothing else
    lis rD ; addi r3, rD, g      return the address of a global   -- gref
      ; blr
    lis rD ; l** r3, g(rD) ; blr return a global                  -- gget
    lis rD ; st** rV, g(rD)      assign to a global; rV is r3 in  -- gset
      ; blr                          a free function, r4 in a member
    lis rD ; addi rD, g          a member set to the address of   -- gstore
      ; stw rD, N(r3) ; blr          a global
    <prologue> ; bl __ct__<Base>     a constructor calling its base,  -- basector
      ; lis/addi r4, __vt__C             then storing its own vtable
      ; stw r4, N(r31) ; <epilogue>      pointer, and nothing else

THESE ARE GENERATED, NOT READ, and the same caveat applies as to the type
ids: real matched functions whose offsets are recovered fact, but a count of
them is not a count of decompiled code. tools/written_vs_generated.py keeps
the two apart.

This writes the FILE for a unit; tools/gen_units.py drives it over every
unit that has candidates. tools/gen_typeids.py wrote the constant-return
shape before this tool could, and is now the codec for it -- its decode,
re-encode and demangle are imported here rather than copied.

`vtable` needs no symbol named by hand: one declared virtual makes mwcc
emit `__vt__<class>` itself, and the constructor store then relocates
against the same name retail uses. What the real class's virtuals WERE is
not in those four words, so the table this object defines has one entry and
retail's has its own. The CONSTRUCTOR is what matches, not the table -- and
the unit is NonMatching either way, so nothing is linked from it.

The `g*` shapes DO need the global named, and a variable carries the same
qualifier a function does: `activeViewport__Q28Graphics7Display`.
CodeWarrior spells that the same whether the scope is a class or a
namespace, so an `extern` inside nested namespaces reproduces the symbol --
except where the scope is a class this file also declares, in which case
the variable goes in as a static member instead.

`basector` reads the base out of the `bl`: its symbol IS `__ct__<Base>`,
and its argument list has to be the same as the caller's. N -- where the
vtable pointer is stored -- says how the base gets declared: at 0 the base
shares that pointer and is declared polymorphic, above 0 it sits in front
and is declared padded to exactly N bytes. A class that has a base
subobject does not have its members where a bare offset says, so where one
class has both a base constructor and a measured layout, the BASE
CONSTRUCTOR is the one dropped.

`gref` and `constret` are the SAME THREE WORDS. `lis r3,HI ; addi r3,r3,LO
; blr` is both "return the address of a global" and "return this 32-bit
constant", and the two are told apart by whether the value lands inside a
loaded section -- the test in loaded_spans(). Without it, 164 constant
returns in WAD02_36 read as global references, found no symbol, and were
dropped.

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

    if n == 24:
        # An animation callback: a static member forwarding to a member
        # function on the object the animation belongs to. All 54 in the
        # image share ONE register assignment, so every word but the two
        # offsets and the call is required exactly -- the same discipline
        # the constructor shape uses. A variant would simply not match.
        if (ws[1] == 0x7C601B78 and ws[2] == 0x7C852378
                and ws[4] == 0x7C040378
                and (ws[0] >> 26) == 32 and ((ws[0] >> 21) & 31) == 6
                and ((ws[0] >> 16) & 31) == 4 and not ws[0] & 0x8000
                and (ws[3] >> 26) == 32 and ((ws[3] >> 21) & 31) == 3
                and ((ws[3] >> 16) & 31) == 6 and not ws[3] & 0x8000
                and (ws[5] >> 26) == 18 and (ws[5] & 3) == 0):
            rel = ws[5] & 0x03FFFFFC
            if rel & 0x02000000:
                rel -= 0x04000000
            return ("animcb", rel, ws[0] & 0xFFFF, ws[3] & 0xFFFF)
        return None

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

    if n == 60:
        # A constructor: prologue, base constructor, vtable store, epilogue.
        # Every word but the call, the two halves of the vtable address and
        # the store offset is fixed, so the whole frame is checked here and
        # only four fields are read out.
        if (list(ws[:5]) == [0x9421FFF0, 0x7C0802A6, 0x90010014, 0x93E1000C,
                             0x7C7F1B78]
                and ws[7] == 0x7FE3FB78
                and list(ws[10:]) == [0x83E1000C, 0x80010014, 0x7C0803A6,
                                      0x38210010, BLR]
                and (ws[5] >> 26) == 18 and (ws[5] & 3) == 1
                and (ws[6] >> 26) == 15 and ((ws[6] >> 21) & 31) == 4
                and ((ws[6] >> 16) & 31) == 0
                and (ws[8] >> 26) == 14 and ((ws[8] >> 21) & 31) == 4
                and ((ws[8] >> 16) & 31) == 4
                and (ws[9] >> 26) == 36 and ((ws[9] >> 21) & 31) == 4
                and ((ws[9] >> 16) & 31) == 31):
            rel = ws[5] & 0x03FFFFFC
            if rel & 0x02000000:
                rel -= 0x04000000
            lo16 = ws[8] & 0xFFFF
            v = (((ws[6] & 0xFFFF) << 16)
                 + (lo16 - 0x10000 if lo16 & 0x8000 else lo16)) & 0xFFFFFFFF
            voff = ws[9] & 0xFFFF
            if not voff & 0x8000:
                return ("basector", rel, v, voff)
        return None

    if n == 12:
        # lis rD, HI ; <one access at LO(rD)> ; blr -- a function whose whole
        # body touches one GLOBAL. Which global is checked in candidates();
        # here it is only decoded. This has to come before the constant
        # return below, whose three-word form is the same instructions.
        if (ws[0] >> 26) == 15 and ((ws[0] >> 16) & 31) == 0:
            d = (ws[0] >> 21) & 31
            op, dd = ws[1] >> 26, (ws[1] >> 21) & 31
            if ((ws[1] >> 16) & 31) == d:
                lo = ws[1] & 0xFFFF
                v = (((ws[0] & 0xFFFF) << 16)
                     + (lo - 0x10000 if lo & 0x8000 else lo)) & 0xFFFFFFFF
                if op == 14 and d == 3 and dd == 3:
                    return ("gref", v)
                if op in LOADS:
                    ctype, _w, isf = LOADS[op]
                    if dd == (1 if isf else 3):
                        return ("gget", ctype, v, d)
                if op in STORES:
                    ctype, _w, isf = STORES[op]
                    # The value is in f1 for a float; otherwise r3 for a
                    # free function and r4 for a member, which the symbol
                    # decides and candidates() checks.
                    ok = (dd == 1) if isf else (dd in (3, 4))
                    if ok:
                        return ("gset", ctype, v, d, dd)

    if n == 16:
        # lis rD, HI ; addi rD, rD, LO ; stw rD, N(r3) ; blr -- a constructor
        # storing its own vtable pointer. Whether the address really is this
        # class's vtable is checked in candidates(), where the symbol table
        # is to hand; here it is only decoded.
        d = (ws[0] >> 21) & 31
        if ((ws[0] >> 26) == 15 and (ws[1] >> 26) == 14
                and (ws[2] >> 26) == 36
                and ((ws[0] >> 16) & 31) == 0
                and ((ws[1] >> 21) & 31) == d and ((ws[1] >> 16) & 31) == d
                and ((ws[2] >> 21) & 31) == d and ((ws[2] >> 16) & 31) == 3):
            off = ws[2] & 0xFFFF
            hi, lo = ws[0] & 0xFFFF, ws[1] & 0xFFFF
            if not off & 0x8000:
                v = ((hi << 16) + (lo - 0x10000 if lo & 0x8000 else lo))
                return ("vtable", v & 0xFFFFFFFF, off, d)

    # li r0, K then one or more stw r0, N(r3)
    li = ws[0]
    if (li >> 26) != 14 or ((li >> 21) & 31) != 0 or ((li >> 16) & 31) != 0:
        return None
    v = li & 0xFFFF
    if v & 0x8000:
        v -= 0x10000
    offs = []
    for w in ws[1:-1]:
        op = w >> 26
        if op not in STORES or ((w >> 21) & 31) != 0 or ((w >> 16) & 31) != 3:
            return None
        ctype, _wd, isf = STORES[op]
        if isf:
            return None              # r0 is not a floating-point register
        off = w & 0xFFFF
        if off & 0x8000:
            return None
        offs.append((ctype, off))
    if not offs:
        return None
    return ("constset", v, tuple(offs))


def reencode(shape):
    kind = shape[0]
    if kind == "animcb":
        rel, holder, slot = shape[1], shape[2], shape[3]
        return struct.pack(">IIIIII",
                           0x80C40000 | holder, 0x7C601B78, 0x7C852378,
                           0x80660000 | slot, 0x7C040378,
                           0x48000000 | (rel & 0x03FFFFFC))
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
        for ctype, off in offs:
            op = next(o for o, (ty, _w, isf) in STORES.items()
                      if ty == ctype and not isf)
            ws.append((op << 26) | (3 << 16) | off)
        ws.append(BLR)
        return struct.pack(">" + "I" * len(ws), *ws)
    if kind == "gref":
        v = shape[1]
        lo, hi = v & 0xFFFF, (v >> 16) & 0xFFFF
        if lo & 0x8000:
            hi = (hi + 1) & 0xFFFF
        return struct.pack(">III", 0x3C600000 | hi, 0x38630000 | lo, BLR)
    if kind in ("gget", "gset"):
        table = LOADS if kind == "gget" else STORES
        ctype, v, d = shape[1], shape[2], shape[3]
        lo, hi = v & 0xFFFF, (v >> 16) & 0xFFFF
        if lo & 0x8000:
            hi = (hi + 1) & 0xFFFF
        for op, (ty, _w, isf) in table.items():
            if ty != ctype:
                continue
            dd = (1 if isf else 3) if kind == "gget" else shape[4]
            return struct.pack(">III", 0x3C000000 | (d << 21) | hi,
                               (op << 26) | (dd << 21) | (d << 16) | lo, BLR)
        return None
    if kind == "basector":
        rel, v, voff = shape[1], shape[2], shape[3]
        lo, hi = v & 0xFFFF, (v >> 16) & 0xFFFF
        if lo & 0x8000:
            hi = (hi + 1) & 0xFFFF
        ws = [0x9421FFF0, 0x7C0802A6, 0x90010014, 0x93E1000C, 0x7C7F1B78,
              (18 << 26) | (rel & 0x03FFFFFC) | 1,
              0x3C800000 | hi, 0x7FE3FB78, 0x38840000 | lo,
              0x909F0000 | voff,
              0x83E1000C, 0x80010014, 0x7C0803A6, 0x38210010, BLR]
        return struct.pack(">" + "I" * 15, *ws)
    if kind == "vtable":
        v, off, d = shape[1], shape[2], shape[3]
        lo, hi = v & 0xFFFF, (v >> 16) & 0xFFFF
        if lo & 0x8000:
            hi = (hi + 1) & 0xFFFF
        return struct.pack(">IIII",
                           0x3C000000 | (d << 21) | hi,
                           0x38000000 | (d << 21) | (d << 16) | lo,
                           0x90000000 | (d << 21) | (3 << 16) | off,
                           BLR)
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
                        and s["st_size"] in (8, 12, 16, 20, 24, 60)):
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


_FUNC_SYMS = None


def function_symbols():
    """address -> name, for functions. The base constructor is found here."""
    global _FUNC_SYMS
    if _FUNC_SYMS is None:
        _FUNC_SYMS = {}
        with open(ELF, "rb") as fh:
            for sec in ELFFile(fh).iter_sections():
                if sec.header["sh_type"] != "SHT_SYMTAB":
                    continue
                for s in sec.iter_symbols():
                    if s["st_info"]["type"] == "STT_FUNC" and s["st_size"]:
                        _FUNC_SYMS.setdefault(s["st_value"], s.name)
    return _FUNC_SYMS


_DATA_SYMS = None


def data_symbols():
    """address -> name, for everything that is not a function.

    A constructor's vtable store is only reproducible if the address it
    builds really is this class's own `__vt__`; mwcc emits that symbol from
    the class declaration, so nothing else has to be named. Any other
    address is somebody else's object and is refused.
    """
    global _DATA_SYMS
    if _DATA_SYMS is None:
        _DATA_SYMS = {}
        with open(ELF, "rb") as fh:
            for sec in ELFFile(fh).iter_sections():
                if sec.header["sh_type"] != "SHT_SYMTAB":
                    continue
                for s in sec.iter_symbols():
                    if s["st_info"]["type"] != "STT_FUNC" and s.name:
                        _DATA_SYMS.setdefault(s["st_value"], s.name)
    return _DATA_SYMS


def split_data_symbol(sym):
    """-> (namespaces, name) for a global we could declare, or None.

    A variable carries the same qualifier a function does -- `name__<qual>`
    -- and CodeWarrior spells it the same whether the scope is a class or a
    namespace, so a namespace reproduces either. Anything with an `@` in it
    is a static local, an anonymous namespace or a string literal, and no
    declaration can name it.
    """
    if "@" in sym:
        return None
    if "__" in sym:
        name, rest = sym.split("__", 1)
        got = parse_type(rest)
        if got is None or got[1] or got[0][0] != "name":
            return None
        if mangle_type(got[0]) != rest:
            return None
        parts = got[0][1]
    else:
        name, parts = sym, ()
    if not name or not usable([name] + list(parts)):
        return None
    return tuple(parts), name


def candidates(raw, base, foff, syms, lo, hi):
    """-> (list of candidate dicts, Counter of reasons for the rest)."""
    good, skipped = [], Counter()
    spans = loaded_spans(raw)
    for addr, size, sym in syms:
        if not (lo <= addr < hi):
            continue
        body = raw[foff + (addr - base): foff + (addr - base) + size]

        shape = decode(body)
        const, glob, base_cls = None, None, None
        if (shape is not None and shape[0] == "gref"
                and not any(a <= shape[1] < b for a, b in spans)):
            # Not an address, so those three words are a constant return
            # and not a reference to anything.
            shape = None
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
        if (cls is None and const is None
                and shape[0] not in ("gref", "gget", "gset")):
            # A free function has no members of its own to touch. Only a
            # constant return and the global-touching shapes can be one.
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
        elif shape[0] == "basector":
            if cls is None or method != "__ct__":
                skipped["only a constructor calls a base constructor"] += 1
                continue
            want = "__vt__" + mangle_type(("name", tuple(ns) + (cls,)))
            if data_symbols().get(shape[2]) != want:
                skipped["the stored address is not this class's own "
                        "vtable"] += 1
                continue
            tgt = (addr + 20 + shape[1]) & 0xFFFFFFFF
            tname = function_symbols().get(tgt)
            if tname is None or not tname.startswith("__ct__"):
                skipped["the call target is not a constructor"] += 1
                continue
            them = split_symbol(tname)
            if them is None or them[1] is None:
                skipped["the base constructor does not name a class"] += 1
                continue
            if them[4] != args:
                skipped["the base constructor takes different "
                        "arguments"] += 1
                continue
            if not usable(list(them[0]) + [them[1]]):
                skipped["the base class name is not a plain "
                        "identifier"] += 1
                continue
            if shape[3] % 4:
                skipped["the vtable offset is not a multiple of four"] += 1
                continue
            base_cls = (tuple(them[0]), them[1])
            if base_cls == (tuple(ns), cls):
                skipped["a class cannot be its own base"] += 1
                continue
        elif shape[0] == "animcb":
            if cls is None or not method.startswith("an") or len(method) < 3:
                skipped["an animation callback is a member whose name "
                        "starts with an"] += 1
                continue
            if args != "P15xAnimTransitionP11xAnimSinglePv":
                skipped["an animation callback takes (xAnimTransition*, "
                        "xAnimSingle*, void*)"] += 1
                continue
            tgt = (addr + 20 + shape[1]) & 0xFFFFFFFF
            tname = function_symbols().get(tgt)
            want = ("%s__%sF%s"
                    % (method[2:], mangle_type(("name", tuple(ns) + (cls,))),
                        args[:-2]))
            if tname != want:
                skipped["the call target is not this class's own %s"
                        % ("<name without an>",)] += 1
                continue
            shape = shape + (method[2:],)
        elif shape[0] == "vtable":
            got_sym = data_symbols().get(shape[1])
            want = ("__vt__" + mangle_type(("name", tuple(ns) + (cls,)))
                    if cls else None)
            if want is not None and got_sym == want:
                if method != "__ct__":
                    skipped["only a constructor stores the vtable "
                            "pointer"] += 1
                    continue
                if shape[2] % 4:
                    skipped["the vtable pointer is not four-aligned"] += 1
                    continue
            else:
                # Not a vtable: a member set to the address of some other
                # global, which is only reachable if that global is
                # nameable.
                glob = split_data_symbol(got_sym or "")
                if glob is None:
                    skipped["the stored address is not a nameable "
                            "global"] += 1
                    continue
                if cls is None:
                    skipped["a free function storing through r3"] += 1
                    continue
                shape = ("gstore", shape[1], shape[2], shape[3])
        elif shape[0] in ("gref", "gget", "gset"):
            glob = split_data_symbol(
                data_symbols().get(shape[1] if shape[0] == "gref"
                                   else shape[2]) or "")
            if glob is None:
                skipped["the address is not a nameable global"] += 1
                continue
            if shape[0] == "gset":
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
                    skipped["a reference argument stored by value"] += 1
                    continue
                isf = shape[1] == "float"
                want_reg = 1 if isf else (3 if cls is None else 4)
                if shape[4] != want_reg:
                    skipped["the stored register is not where this kind of "
                            "function's first argument arrives"] += 1
                    continue
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
                     "params": params, "sym": sym, "global": glob,
                     "base": base_cls})
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

    # A class whose constructor stores a vtable pointer has one at offset
    # 0, put there by the compiler and not by us. Nothing may be declared
    # in those four bytes, or every field after it lands four too far on.
    poly = {(c["ns"], c["cls"]) for c in good
            if c["shape"][0] == "vtable" and not c["shape"][2]}

    # Lay each class out and refuse one that cannot be: an offset that is
    # not aligned for its type, or a field that would overlap the next.
    for key, offs in fields.items():
        end = 4 if key in poly else 0
        for off in sorted(offs):
            ctype = offs[off] or "int"
            if off % WIDTH[ctype]:
                bad.add((key, off))
            if off < end:
                bad.add((key, off))
            end = off + WIDTH[ctype]

    # A class with a base subobject in front of it does not have its members
    # where a bare offset says: the offsets measured here are from the start
    # of the whole object. Rather than guess, the BASE CONSTRUCTOR is the one
    # dropped, so nothing that already matched can be lost this way.
    laid_out = {(c["ns"], c["cls"]) for c in good
                if offsets_of(c["shape"]) or c["shape"][0] == "vtable"}

    keep = []
    for c in good:
        key = (c["ns"], c["cls"])
        if c["shape"][0] == "basector" and key in laid_out:
            skipped["a base constructor on a class whose layout is also "
                    "measured"] += 1
            continue
        if (c["shape"][0] == "vtable" and c["shape"][2]
                and any(offsets_of(o["shape"]) for o in good
                        if (o["ns"], o["cls"]) == key)):
            # The vptr is at N, so N bytes in front of it belong to a base
            # subobject -- and every other offset measured for this class
            # is from the start of the WHOLE object, not from where its own
            # members begin. Which of them fall inside the base is not in
            # these bytes, so the constructor goes rather than a layout be
            # guessed at. Same rule the base constructor gets.
            skipped["a vtable at an offset on a class whose layout is also "
                    "measured"] += 1
            continue
        if any((key, off) in bad for off, _t in offsets_of(c["shape"])):
            skipped["the class cannot be laid out from what was measured"] += 1
            continue
        keep.append(c)
    return fields, keep


def global_types(good, skipped):
    """-> {(namespaces, name): ctype}, having dropped the contradictions.

    Two functions that disagree about the type of one global cannot both be
    right, and nothing here can tell which is, so both go -- the same rule
    the member offsets get.
    """
    types, bad = {}, set()
    for c in good:
        if not c["global"]:
            continue
        key = c["global"]
        ctype = None
        if c["shape"][0] in ("gget", "gset"):
            ctype = c["shape"][1]
        prev = types.setdefault(key, None)
        if ctype is None:
            continue
        if prev is not None and prev != ctype:
            bad.add(key)
        types[key] = ctype

    keep = []
    for c in good:
        if c["global"] in bad:
            skipped["two functions disagree about a global's type"] += 1
            continue
        keep.append(c)
    return types, keep


def offsets_of(shape):
    """-> [(offset, ctype or None)] the shape proves about the class."""
    if shape[0] in ("get", "set"):
        return [(shape[2], shape[1])]
    if shape[0] == "ref":
        return [(shape[1], None)]
    if shape[0] == "constset":
        return [(off, ctype) for ctype, off in shape[2]]
    if shape[0] == "gstore":
        return [(shape[2], "int")]
    return []


def has_vtable(cands):
    return any(c["shape"][0] in ("vtable", "basector") for c in cands)


def vptr_pad(cands):
    """Bytes in front of this class's own vptr, from where it is stored.

    Zero means the vptr is at the start of the object, which is the plain
    case and needs nothing written.
    """
    for c in cands:
        if c["shape"][0] == "vtable" and c["shape"][2]:
            return c["shape"][2]
    return 0


def base_of(cands):
    """The (namespaces, class) this class derives from, or None."""
    for c in cands:
        if c["shape"][0] == "basector":
            return c["base"]
    return None


def class_order(methods):
    """Our classes, a base before anything deriving from it.

    Two of the units hold a chain -- zPlayerLandSB derives from
    zSBPlayerAction, which derives from zCommonPlayerAction -- and a class
    has to be complete before it is used as a base.
    """
    order, seen = [], set()

    def visit(key, path):
        if key in seen or key not in methods:
            return
        if key in path:
            return                      # a cycle; leave the order alone
        b = base_of(methods[key])
        if b is not None:
            visit(b, path | {key})
        seen.add(key)
        order.append(key)

    for key in methods:
        visit(key, set())
    return order


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
    # The RTTID_Fix<T> family. It is a template rather than a per-function
    # shape, so it lives in its own module -- imported here rather than at
    # the top because that module imports this one for the mangler.
    import gen_rttid
    rt_rows, rt_skipped = gen_rttid.family(raw, base, foff, spans)
    skipped += rt_skipped

    if not good and not rt_rows:
        sys.exit("gen_accessors: nothing in %08X..%08X matches a shape -- "
                 "refusing to write an empty file" % (lo, hi))

    fields, good = field_types(good, skipped)
    globs, good = global_types(good, skipped)

    # Named parameter types. A name used by value has to be DEFINED -- the
    # only definition this tool writes is an enum -- and a name used only
    # through a pointer or a reference needs a declaration. Either can
    # collide with a class this file defines: a parameter of type
    # `Graphics::MovieBase` needs `namespace Graphics`, and a class called
    # `Graphics` cannot also be a namespace. A forward declaration of a
    # class this file goes on to define is fine and is not a collision.
    defined = {(c["ns"], c["cls"]) for c in good}
    # A base stub is a class this file declares too, so a parameter type
    # whose scope is that same name cannot also be a namespace.
    # Graphics::Node is both a base class and the scope of
    # Graphics::Node::NodeTypeEnum, and `namespace Node` beside
    # `class Node` does not compile.
    defined |= {c["base"] for c in good if c["shape"][0] == "basector"}

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

    statics = {}
    externs = []
    for key in sorted(globs):
        parts, name = key
        ctype = globs[key] or "int"
        if (parts[:-1], parts[-1]) in defined if parts else False:
            statics.setdefault((parts[:-1], parts[-1]), []).append(
                (name, ctype))
        else:
            line = "extern %s %s;" % (ctype, name)
            for n in reversed(parts):
                line = "namespace %s { %s }" % (n, line)
            externs.append(line)

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
    if externs:
        L += externs
        L.append("")

    # Base classes this file does not otherwise declare, as stubs. The
    # vtable offset says which kind: at 0 the base shares the pointer and is
    # polymorphic, above 0 it sits in front of it and is that many bytes.
    stubs = {}
    for key, cands in methods.items():
        b = base_of(cands)
        if b is None or b in methods:
            continue
        c = next(x for x in cands if x["shape"][0] == "basector")
        stubs.setdefault(b, (c["shape"][3], c["params"]))
    for b in sorted(stubs):
        voff, params = stubs[b]
        bns, bcls = list(b[0]), b[1]
        for n in bns:
            L.append("namespace %s {" % n)
        L.append("")
        L.append("class %s {" % bcls)
        L.append("public:")
        L.append("    %s(%s);" % (bcls, param_decls(params, False)))
        if voff:
            L.append("    unsigned char _pad[0x%X];" % voff)
        else:
            L.append("    virtual void __vtable_anchor();")
        L.append("};")
        L.append("")
        for n in reversed(bns):
            L.append("}  // namespace %s" % n)
        L.append("")

    poly = {(c["ns"], c["cls"]) for c in good
            if c["shape"][0] == "vtable" and not c["shape"][2]}

    cbs = [c for c in good if c["shape"][0] == "animcb"]
    if cbs:
        holder = {c["shape"][2] for c in cbs}
        slot = {c["shape"][3] for c in cbs}
        if len(holder) != 1 or len(slot) != 1:
            sys.exit("gen_accessors: the animation callbacks in this unit "
                     "do not agree on their offsets (%s / %s) -- refusing "
                     "to emit one layout for two"
                     % (sorted(holder), sorted(slot)))
        L.append("// The two dereferences every animation callback makes.")
        L.append("// Nothing in the image NAMES either type, so both are")
        L.append("// spelled as the offsets that were measured. Neither")
        L.append("// struct emits a symbol.")
        L.append("struct AnimCBSlot { unsigned char _pad[0x%X]; void* owner; "
                 "};" % slot.pop())
        L.append("struct AnimCBHolder { unsigned char _pad[0x%X]; "
                 "AnimCBSlot* slot; };" % holder.pop())
        L.append("")

    for key in class_order(methods):
        ns, cls = list(key[0]), key[1]
        b = base_of(methods[key])
        for n in ns:
            L.append("namespace %s {" % n)
        L.append("")
        if b is None:
            L.append("class %s {" % cls)
        else:
            L.append("class %s : public %s {"
                     % (cls, "::".join(list(b[0]) + [b[1]])))
        L.append("public:")
        for name, ctype in sorted(statics.get(key, [])):
            L.append("    static %s %s;" % (ctype, name))
        vpad = vptr_pad(methods[key])
        if has_vtable(methods[key]) and not vpad:
            # One virtual is all it takes to make the compiler emit
            # __vt__<class> and have the constructor store it. Which
            # virtuals the real class had is not in these bytes, so the
            # vtable this object defines holds one entry and retail's holds
            # its own -- the CONSTRUCTOR is what matches, not the table.
            L.append("    virtual void __vtable_anchor();")
        seen = set()
        for c in sorted(methods[key], key=lambda c: (c["method"], c["addr"])):
            decl = declare(c, fields[key])
            if decl in seen:
                continue
            seen.add(decl)
            L.append(decl)
        L.append("")
        if vpad:
            # The vptr is at vpad, so vpad bytes of something precede it.
            # A leading data member puts it there; what those bytes ARE is
            # not in the four words this was read from.
            L.append("    unsigned char _vbase[0x%X];" % vpad)
            L.append("    virtual void __vtable_anchor();")
            pad = vpad + 4
        else:
            pad = 4 if key in poly else 0
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
        seen = set()
        for c in sorted(frees, key=lambda c: c["method"]):
            d = declare_free(c)
            if d not in seen:
                seen.add(d)
                L.append(d)
        L.append("")

    # A base constructor DEFINED in this file gets inlined into the derived
    # constructors that call it, and the call retail makes to the base then
    # emits as a call to the base's OWN base instead. Twelve functions were
    # counted as matched while branching to the wrong constructor, and
    # report.json cannot see it -- a relocated field holds zero on both
    # sides, so objdiff compares nothing. `tools/reloc_audit.py` is what
    # found them and `unitcmp.py` is what can now tell.
    ctors = any(c["shape"][0] == "basector" for c in good)
    if ctors:
        L.append("#pragma dont_inline on")
    for c in good:
        L.append(define(c, fields[(c["ns"], c["cls"])]))
    if ctors:
        L.append("#pragma dont_inline off")

    rt_lines, rt_kept, rt_refused = gen_rttid.render(rt_rows, defined)
    if rt_lines:
        L.append("")
        L += rt_lines

    Path(out).parent.mkdir(parents=True, exist_ok=True)
    Path(out).write_text(NL.join(L) + NL, encoding="utf-8")

    total = len(good) + len(rt_kept) + sum(skipped.values())
    by = Counter(c["shape"][0] for c in good)
    if rt_kept:
        by["rttid"] = len(rt_kept)
    skipped += rt_refused
    print("  %s: %d function(s) emitted of %d short function(s) in "
          "%08X..%08X, %d bytes"
          % (out, len(good) + len(rt_kept), total, lo, hi,
             sum(c["size"] for c in good) + 4 * len(rt_kept)))
    print("    " + ", ".join("%s %d" % (k, n) for k, n in by.most_common()))
    for why, n in skipped.most_common():
        print("    skipped %-54s %d" % (why, n))
    return 0


def ref_type(c, flds):
    """The pointee type a `ref` hands back."""
    t = flds[c["shape"][1]] or "int"
    return ("const " + t) if c["const"] else t


def gname(c):
    """The global this candidate touches, spelled for C++."""
    parts, name = c["global"]
    return "::".join(list(parts) + [name])


def return_type(c):
    """The return type for a shape that reads or points at something."""
    kind = c["shape"][0]
    if kind == "gget":
        return c["shape"][1]
    if kind == "gref":
        return "int*"
    if kind == "get":
        return c["shape"][1]
    return "unsigned int"


def declare_free(c):
    """The file-scope declaration for a free function."""
    kind = c["shape"][0]
    if kind == "gset":
        return "void %s(%s);" % (c["method"],
                                 param_decls(c["params"], True))
    return "%s %s(%s);" % (return_type(c), c["method"],
                           param_decls(c["params"], False))


def declare(c, flds):
    """The in-class declaration for one candidate."""
    kind = c["shape"][0]
    cst = " const" if c["const"] else ""
    if kind in ("gref", "gget"):
        return "    %s %s(%s)%s;" % (return_type(c), c["method"],
                                     param_decls(c["params"], False), cst)
    if kind == "gset":
        return "    void %s(%s)%s;" % (c["method"],
                                       param_decls(c["params"], True), cst)
    if kind == "gstore":
        return "    void %s(%s)%s;" % (c["method"],
                                       param_decls(c["params"], False), cst)
    if kind in ("vtable", "basector"):
        return "    %s(%s);" % (c["cls"], param_decls(c["params"], False))
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
    if kind == "animcb":
        return ("    static void %s(%s);" + chr(10)
                + "    void %s(%s);") % (
            c["method"], param_decls(c["params"], False),
            c["shape"][4], param_decls(c["params"][:-1], False))
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
    if kind in ("gref", "gget", "gset", "gstore"):
        scope = (q + "::") if c["cls"] else ""
        if kind == "gref":
            return "int* %s%s(%s)%s { return &%s; }" % (
                scope, c["method"], param_decls(c["params"], False), cst,
                gname(c))
        if kind == "gget":
            return "%s %s%s(%s)%s { return %s; }" % (
                c["shape"][1], scope, c["method"],
                param_decls(c["params"], False), cst, gname(c))
        if kind == "gset":
            return "void %s%s(%s)%s { %s = %s; }" % (
                scope, c["method"], param_decls(c["params"], True), cst,
                gname(c), stored_value(c["params"][0], c["shape"][1]))
        return "void %s%s(%s)%s { f%X = (int)&%s; }" % (
            scope, c["method"], param_decls(c["params"], False), cst,
            c["shape"][2], gname(c))
    if kind == "vtable":
        # The body IS the vtable store; the compiler writes it because the
        # class is polymorphic, so an empty body is the whole function.
        return "%s::%s(%s) {}" % (q, c["cls"],
                                  param_decls(c["params"], False))
    if kind == "basector":
        args = ", ".join("a%d" % i for i in range(len(c["params"])))
        return "%s::%s(%s) : %s(%s) {}" % (
            q, c["cls"], param_decls(c["params"], False),
            "::".join(list(c["base"][0]) + [c["base"][1]]), args)
    if c["method"] == "__ct__":
        body = " ".join("f%X = %d;" % (off, c["shape"][1])
                        for _ctype, off in c["shape"][2])
        return "%s::%s(%s) { %s }" % (q, c["cls"],
                                      param_decls(c["params"], False), body)
    if kind == "animcb":
        return ("void %s::%s(%s) { ((%s*)((AnimCBHolder*)a1)->slot->owner)"
                "->%s(a0, a1); }"
                % (q, c["method"], param_decls(c["params"], False), q,
                   c["shape"][4]))
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
                        for _ctype, off in c["shape"][2])
        return "void %s::%s(%s)%s { %s }" % (
            q, c["method"], param_decls(c["params"], False), cst, body)
    raise AssertionError(kind)


if __name__ == "__main__":
    sys.exit(main())
