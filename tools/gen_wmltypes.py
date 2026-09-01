"""Generate src/SB/GM/Engine/Core/x/xWMLTypes.cpp from the image.

Sext::FixWmlType is 11,944 bytes and the whole unit. It relocates a
loaded WML object: `l` is the delta to add to every pointer inside it,
`type` is the hash of the object's type. mwcc compiles the switch to a
binary search, so half the function is dispatch over 307 case values,
and those values are DATA -- they are read out of the image here, not
transcribed.

Three things about reading them cost a day between them, and each is a
rule that generalises:

  * Resolve a compared value along the CONTROL-FLOW PATH, not by walking
    backwards through the instruction stream. mwcc hoists a `lis` into
    r6/r7 and shares it across a subtree, so the textually nearest `lis`
    can be one that never executes on the path that reaches the `addi`.
    One value in 307 came out as FA880572 when it is FB510572, and that
    single wrong value moved the median of a nine-case subtree and put
    four comparisons in a different order.

  * A body runs to the start of the NEXT body, not to the first opcode-18
    instruction -- opcode 18 is `bl` as well as `b`, so ending there
    truncates every body that makes more than one call. That was 109 of
    the function's instructions, and it also explained the register
    allocation: a body that still needs `p` after a call is exactly what
    makes mwcc keep p in a callee-saved register.

  * mwcc DELETES a case whose body is empty, because it cannot be
    distinguished from the default. Three of these cases run nothing, and
    written `case X: break;` they vanish from the search, the value set
    drops from 307 to 304, and every pivot at or below the median moves:
    63 of 307 comparisons agreed. Written `case X: return;` the emitted
    code is the same lone branch to the epilogue and the case survives.

Two cases carry no equality test at all: their subtree had narrowed to a
single candidate and mwcc proved it with a pair of range tests instead.
They are recovered from the interval, and this refuses to guess when an
interval holds more than one value.

STATE: the dispatch matches retail exactly, all 307 comparisons in
order, and 2,979 of retail's 2,986 instructions align. It is NOT
byte-identical yet -- see NOTES.md for the seven that differ and for
what has been ruled out.
"""
import argparse
import struct
import sys
from collections import defaultdict
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import disasm as D                                       # noqa: E402
import gen_accessors as A                                # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "src/SB/GM/Engine/Core/x/xWMLTypes.cpp"
NL = chr(10)

ADDR, SIZE = 0x80049320, 11944
EPI = 0x8004C1B4
P_REGS = (5, 31)            # p is the third argument, or a saved copy
L_REGS = (3, 30)            # l is the first argument, or a saved copy

sv = lambda x: x - 0x100000000 if x & 0x80000000 else x        # noqa: E731


def writes(w):
    """The GPR this instruction writes, or None (conservatively)."""
    op = w >> 26
    if op in (14, 15, 12, 13, 8, 7):
        return (w >> 21) & 31
    if op in (24, 25, 26, 27, 28, 29):
        return (w >> 16) & 31
    if op in (32, 33, 34, 35, 40, 41, 42, 43, 46):
        return (w >> 21) & 31
    if op == 31:
        xo = (w >> 1) & 0x3FF
        if xo in (444, 28, 60, 316, 124, 476, 284, 412, 24, 536, 792, 824):
            return (w >> 16) & 31
        if xo in (266, 40, 10, 138, 8, 136, 235, 75, 11, 491, 459, 104,
                  26, 954, 922, 202, 234, 200, 232, 339, 371, 19,
                  23, 55, 87, 119, 279, 311, 343, 375):
            return (w >> 21) & 31
    if op in (20, 21, 23):
        return (w >> 16) & 31
    return None


def branch(w, a):
    """(kind, target) for a branch, else None. target is None for bl/blr."""
    op = w >> 26
    if op == 16:
        bo, bi = (w >> 21) & 31, (w >> 16) & 31
        bd = w & 0xFFFC
        if bd & 0x8000:
            bd -= 0x10000
        k = {(12, 2): "beq", (4, 2): "bne", (4, 0): "bge",
             (12, 0): "blt", (12, 1): "bgt", (4, 1): "ble"}.get((bo, bi))
        return None if k is None else (k, a + bd)
    if op == 18:
        if w & 1:
            return ("bl", None)
        d = w & 0x03FFFFFC
        if d & 0x02000000:
            d -= 0x04000000
        return ("b", a + d)
    if w == 0x4E800020:
        return ("blr", None)
    return None


def extract(words, base):
    """-> (nodes, unresolved).

    nodes[addr] = (value, beq_target, greater_target, less_target), over
    exactly the blocks the search reaches through its ge/lt edges. Each
    block is entered with the constant registers that the path carries,
    so a hoisted `lis` is read from the branch that actually executed.
    """
    nodes, unres = {}, []
    seen = set()
    stack = [(base, ())]
    while stack:
        addr, regs = stack.pop()
        if (addr, regs) in seen:
            continue
        seen.add((addr, regs))
        r = dict(regs)
        i = (addr - base) // 4
        val = None
        beq = ge = lt = None
        saw_branch = uncond = False
        while 0 <= i < len(words):
            a = base + 4 * i
            w = words[i]
            op = w >> 26
            b = branch(w, a)
            if b is not None:
                kind, tgt = b
                if kind in ("bl", "blr"):
                    break
                saw_branch = True
                if kind == "beq" and beq is None:
                    beq = tgt
                elif kind in ("bge", "bgt"):
                    ge = tgt
                elif kind in ("blt", "ble"):
                    lt = tgt
                elif kind == "b":
                    if lt is None:
                        lt = tgt
                    uncond = True
                    i += 1
                    break
                i += 1
                continue
            if saw_branch:
                break
            if op == 15:                                   # lis / addis
                d, s = (w >> 21) & 31, (w >> 16) & 31
                imm = (w & 0xFFFF) << 16
                if s == 0:
                    r[d] = imm
                elif s in r:
                    r[d] = (r[s] + imm) & 0xFFFFFFFF
                else:
                    r.pop(d, None)
            elif op == 14:                                 # addi
                d, s = (w >> 21) & 31, (w >> 16) & 31
                imm = w & 0xFFFF
                imm = imm - 0x10000 if imm & 0x8000 else imm
                if s == 0:
                    r[d] = imm & 0xFFFFFFFF
                elif s in r:
                    r[d] = (r[s] + imm) & 0xFFFFFFFF
                else:
                    r.pop(d, None)
            elif op == 31 and ((w >> 1) & 0x3FF) == 0:     # cmpw
                if ((w >> 16) & 31) == 4 and val is None:
                    rb = (w >> 11) & 31
                    if rb in r:
                        val = r[rb]
                    else:
                        unres.append(a)
                        break
            elif op in (10, 11):                           # cmplwi / cmpwi
                if ((w >> 16) & 31) == 4 and val is None:
                    v = w & 0xFFFF
                    val = ((v - 0x10000) if (v & 0x8000 and op == 11)
                           else v) & 0xFFFFFFFF
            else:
                d = writes(w)
                if d is not None:
                    r.pop(d, None)
            i += 1
        if val is None:
            continue
        # The fallthrough is whichever side has no branch of its own:
        # `beq`+`bge` falls through to less-than, `beq`+`blt` falls
        # through to greater, and a block ending in `b` has none at all.
        if saw_branch and not uncond:
            nxt = base + 4 * i
            if lt is None and ge is not None:
                lt = nxt
            elif ge is None and lt is not None:
                ge = nxt
            elif lt is None and ge is None:
                lt = nxt
        nodes[addr] = (val, beq, ge, lt)
        frozen = tuple(sorted(r.items()))
        for t in (ge, lt):
            if t is not None and base <= t < base + 4 * len(words):
                stack.append((t, frozen))
    return nodes, unres


def case_set(nodes, root):
    """-> {signed case value: body address}. Raises rather than guess."""
    cases, ambiguous, seen = {}, [], set()

    def visit(addr, lo, hi):
        if addr not in nodes or (addr, lo, hi) in seen:
            return
        seen.add((addr, lo, hi))
        v, beq, ge, lt = nodes[addr]
        s = sv(v)
        if beq is not None:
            cases[s] = beq
        glo = max(lo, s if beq is None else s + 1)
        if ge is not None:
            if ge in nodes:
                visit(ge, glo, hi)
            elif ge != EPI:
                (cases.__setitem__(glo, ge) if glo == hi
                 else ambiguous.append((glo, hi, ge)))
        if lt is not None:
            lhi = min(hi, s - 1)
            if lt in nodes:
                visit(lt, lo, lhi)
            elif lt != EPI:
                (cases.__setitem__(lo, lt) if lo == lhi
                 else ambiguous.append((lo, lhi, lt)))

    visit(root, -0x80000000, 0x7FFFFFFF)
    if ambiguous:
        raise SystemExit(
            "gen_wmltypes: %d range leaves span more than one value; the "
            "case they guard cannot be read: %s"
            % (len(ambiguous),
               ", ".join("%08X..%08X" % (a & 0xFFFFFFFF, b & 0xFFFFFFFF)
                         for a, b, _t in ambiguous)))
    return cases


REL0 = "*(long*)p += l;"


def rel(n):
    return REL0 if n == 0 else "*(long*)((char*)p + 0x%X) += l;" % n


# The bodies that are none of the mechanical shapes, written from their
# full disassembly. Keyed by body address.
HAND = {
    0x8004AB1C: [rel(0x0), rel(0x4)],
    0x8004AC60: [rel(0x4), rel(0xC)],
    0x8004AC7C: [rel(0x4), rel(0xC)],
    0x8004B980: [rel(0x0), rel(0x4)],
    0x8004BA5C: [rel(0xC), rel(0x14)],
    0x8004AD10: [rel(0x14), rel(0x1C), rel(0x24), rel(0x30), rel(0x38)],

    0x8004AEE4: [rel(0x0),
                 "((Sext::SoundBankSource*)((char*)p + 0x4))->Fix(l);"],

    0x8004B8C8: [rel(0x10),
                 "FixWmlType(l, *(int*)((char*)p + 0xC),",
                 "           *(void**)((char*)p + 0x10));"],
    0x8004B8E4: [rel(0x4),
                 "FixWmlType(l, *(int*)p, *(void**)((char*)p + 0x4));"],
    0x8004ABB8: [rel(0x4),
                 "FixWmlType(l, *(int*)p, *(void**)((char*)p + 0x4));",
                 rel(0xC),
                 "FixWmlType(l, *(int*)((char*)p + 0x8),",
                 "           *(void**)((char*)p + 0xC));"],

    0x8004C120: ["if (*(int*)p == 4) {",
                 "    ((World::ModelInstanceAsset*)((char*)p + 0x20))"
                 "->Fix(l);",
                 "}"],

    0x8004ACE8: ["((Sext::Whatever2*)((char*)p + 0x1C))->Fix(l);",
                 "if (*(long*)((char*)p + 0x34)) {",
                 "    *(long*)((char*)p + 0x34) += l;",
                 "}"],

    0x8004AD50: [rel(0x28),
                 "if (*(int*)((char*)p + 0x24) == 2) {",
                 "    char* e = *(char**)((char*)p + 0x28);",
                 "    if (*(long*)(e + 4)) {",
                 "        *(long*)(e + 4) += l;",
                 "        ((Sext::Whatever2*)(*(char**)(e + 4) + 12))"
                 "->Fix(l);",
                 "    }",
                 "}"],

    # FOUR independent ifs, not a nest: each `beq` lands on the next
    # test, never on the epilogue.
    0x8004ADB0: [rel(0x4), rel(0xC), rel(0x14),
                 "if (*(long*)((char*)p + 0x18)) {",
                 "    *(long*)((char*)p + 0x18) += l;",
                 "}",
                 "if (*(long*)((char*)p + 0x1C)) {",
                 "    *(long*)((char*)p + 0x1C) += l;",
                 "}",
                 "if (*(long*)((char*)p + 0x20)) {",
                 "    *(long*)((char*)p + 0x20) += l;",
                 "}",
                 rel(0x24)],

    # The first test is signed against zero, the rest unsigned against
    # 1, 2 and 3, so the discriminant is an unsigned field.
    0x8004B0A8: ["if (*(unsigned int*)((char*)p + 0x28) == 0) {",
                 "    *(long*)((char*)p + 0xC) += l;",
                 "}",
                 "if (*(unsigned int*)((char*)p + 0x28) == 1) {",
                 "    *(long*)((char*)p + 0x14) += l;",
                 "}",
                 "if (*(unsigned int*)((char*)p + 0x28) == 2) {",
                 "    *(long*)((char*)p + 0x1C) += l;",
                 "}",
                 "if (*(unsigned int*)((char*)p + 0x28) == 3) {",
                 "    *(long*)((char*)p + 0x24) += l;",
                 "}"],

    0x8004ABEC: ["{",
                 "    char* e = (*(char**)((char*)p + 0x4) += l);",
                 "    char* end = e + *(int*)p * 40;",
                 "",
                 "    while (e != end) {",
                 "        Util::RTTID_Fix<Sext::DTRMovieSettings>(e + 4, l);",
                 "        FixWmlType(l, *(int*)e,",
                 "                   ((Pointer32<Sext::EventAny*>*)"
                 "(e + 4))->Get());",
                 "        Util::RTTID_Fix<Sext::DTRMovieSettings>(e + 12, l);",
                 "        FixWmlType(l, *(int*)(e + 8),",
                 "                   ((Pointer32<Sext::EventAny*>*)"
                 "(e + 12))->Get());",
                 "        e += 40;",
                 "    }",
                 "}"],

    0x8004AE5C: ["((Sext::xBaseAsset*)p)->CustomFix(l);",
                 "{",
                 "    char* e = (*(char**)((char*)p + 0x18) += l);",
                 "    char* end = e + *(int*)((char*)p + 0x14) * 40;",
                 "",
                 "    while (e != end) {",
                 "        ((Sext::LinkAssetBaseNew::__srcEvent__*)e)"
                 "->Fix(l);",
                 "        ((Sext::LinkAssetBaseNew::__srcEvent__*)(e + 8))"
                 "->Fix(l);",
                 "        e += 40;",
                 "    }",
                 "}"],

    0x8004B2BC: ["((Sext::xBaseAsset*)p)->CustomFix(l);",
                 "{",
                 "    long* a = (long*)(*(char**)((char*)p + 0x14) += l);",
                 "    long* aend = a + *(int*)((char*)p + 0x10);",
                 "",
                 "    while (a != aend) {",
                 "        *a += l;",
                 "        a += 1;",
                 "    }",
                 "}",
                 "{",
                 "    char* e = (*(char**)((char*)p + 0x1C) += l);",
                 "    char* end = e + *(int*)((char*)p + 0x18) * 40;",
                 "",
                 "    while (e != end) {",
                 "        *(long*)(e + 4) += l;",
                 "        FixWmlType(l, *(int*)e, *(void**)(e + 4));",
                 "        *(long*)(e + 12) += l;",
                 "        FixWmlType(l, *(int*)(e + 8), *(void**)(e + 12));",
                 "        e += 40;",
                 "    }",
                 "}"],
}

# Classes named only by a hand-written body, so the stub is still
# declared. Same mangled spelling the mechanical path parses.
EXTRA_SYMS = [
    "CustomFix__Q24Sext10xBaseAssetFl",
    "Fix__Q34Sext16LinkAssetBaseNew12__srcEvent__Fl",
    "Fix__Q24Sext9Whatever2Fl",
    "Fix__Q24Sext15SoundBankSourceFl",
    "Fix__Q25World18ModelInstanceAssetFl",
    "Fix__Q24Sext16DTRMovieSettingsFl",
]

# Classes whose method is DEFINED, not merely declared. RTTID_Fix<T> is
# instantiated in this unit and retail's instantiation is four
# instructions -- lwz, add, stw, blr -- so T::Fix is inlined into it and
# its body is one relocation at offset zero. A stub would emit a call.
DEFINED = {
    ("Sext", "DTRMovieSettings"):
        "class DTRMovieSettings { public: long mOffset;"
        " void Fix(long l) { mOffset += l; } };",
}


class Gen(object):
    def __init__(self):
        raw, secs, self.funcs, self.objs = D.load()
        self.words = list(struct.unpack(">" + "I" * (SIZE // 4),
                                        D.read(raw, secs, ADDR, SIZE)))
        nodes, unres = extract(self.words, ADDR)
        if unres:
            raise SystemExit(
                "gen_wmltypes: %d comparisons could not be resolved: %s"
                % (len(unres), ", ".join("%08X" % a for a in unres)))
        self.nodes = nodes
        self.cases = case_set(nodes, ADDR)
        self.bytarget = defaultdict(list)
        for v, t in self.cases.items():
            self.bytarget[t].append(v & 0xFFFFFFFF)
        self.starts = sorted(t for t in self.bytarget if t != EPI)
        self.classes = {}

    def extent(self, t):
        nxt = next((s for s in self.starts if s > t), EPI)
        return t, min(nxt, EPI)

    def words_at(self, t):
        lo, hi = self.extent(t)
        return self.words[(lo - ADDR) // 4:(hi - ADDR) // 4]

    def callee(self, t, k, w):
        disp = w & 0x03FFFFFC
        if disp & 0x02000000:
            disp -= 0x04000000
        return self.funcs.get(t + 4 * k + disp, (None, 0))[0]

    def cpp_class(self, sym):
        name, rest = sym.split("__", 1)
        if not rest.endswith("Fl"):
            raise SystemExit("gen_wmltypes: %s is not a (long) method" % sym)
        node, tail = A.parse_type(rest[:-2])
        if tail or node[0] != "name":
            raise SystemExit("gen_wmltypes: cannot parse %s" % sym)
        q = node[1]
        self.classes.setdefault(q, set()).add(name)
        return "::".join(q), name

    def translate(self, t):
        """The C++ for this body, or None if some instruction is not a
        known shape. Never a partial body."""
        if t == EPI:
            # NOT an empty body: mwcc deletes a case that runs nothing,
            # and deleting it moves the search tree's median.
            return ["return;"]
        w = self.words_at(t)
        out, i = [], 0

        def mr_pair(x):
            if (x >> 26) == 31 and ((x >> 1) & 0x3FF) == 444 \
                    and ((x >> 21) & 31) == ((x >> 11) & 31):
                return ((x >> 16) & 31, (x >> 21) & 31)
            return None

        while i < len(w):
            a = w[i]
            b = w[i + 1] if i + 1 < len(w) else 0
            c = w[i + 2] if i + 2 < len(w) else 0
            # lwz rD,N(p) ; add rD,rD,l ; stw rD,N(p)
            if ((a >> 26) == 32 and ((a >> 16) & 31) in P_REGS
                    and (b >> 26) == 31 and ((b >> 1) & 0x3FF) == 266
                    and (c >> 26) == 36
                    and ((c >> 16) & 31) == ((a >> 16) & 31)
                    and (a & 0xFFFF) == (c & 0xFFFF)
                    and ((b >> 16) & 31) == ((a >> 21) & 31)
                    and ((b >> 11) & 31) in L_REGS):
                out.append(rel(a & 0xFFFF))
                i += 3
                continue
            m1, m2 = mr_pair(a), mr_pair(b)
            # mr r3,p ; mr r4,l ; bl
            if m1 in ((3, 5), (3, 31)) and m2 is not None \
                    and m2[0] == 4 and m2[1] in L_REGS \
                    and (c >> 26) == 18 and (c & 1):
                sym = self.callee(t, i + 2, c)
                if sym is None:
                    return None
                cls, meth = self.cpp_class(sym)
                out.append("((%s*)p)->%s(l);" % (cls, meth))
                i += 3
                continue
            # mr r4,l ; (mr r3,p | addi r3,p,K) ; bl
            if m1 is not None and m1[0] == 4 and m1[1] in L_REGS \
                    and (c >> 26) == 18 and (c & 1):
                sym = self.callee(t, i + 2, c)
                if sym is None:
                    return None
                cls, meth = self.cpp_class(sym)
                if m2 in ((3, 5), (3, 31)):
                    out.append("((%s*)p)->%s(l);" % (cls, meth))
                elif (b >> 26) == 14 and ((b >> 21) & 31) == 3 \
                        and ((b >> 16) & 31) in P_REGS and not b & 0x8000:
                    out.append("((%s*)((char*)p + 0x%X))->%s(l);"
                               % (cls, b & 0xFFFF, meth))
                else:
                    return None
                i += 3
                continue
            if (a >> 26) == 18 and not (a & 1):              # b -> epilogue
                d = a & 0x03FFFFFC
                if d & 0x02000000:
                    d -= 0x04000000
                if t + 4 * i + d != EPI:
                    return None
                i += 1
                continue
            return None
        return out

    def build(self):
        bodies, missing, stale = {}, [], []
        for t in sorted(self.bytarget):
            s = self.translate(t)
            if s is None:
                if t in HAND:
                    bodies[t] = HAND[t]
                else:
                    missing.append(t)
            else:
                bodies[t] = s
        for t in HAND:
            if self.translate(t) is not None:
                stale.append(t)
        if missing:
            raise SystemExit("gen_wmltypes: no hand body for %s"
                             % ", ".join("%08X" % t for t in missing))
        if stale:
            raise SystemExit("gen_wmltypes: %s translate mechanically now; "
                             "drop their hand bodies"
                             % ", ".join("%08X" % t for t in stale))
        for sym in EXTRA_SYMS:
            self.cpp_class(sym)

        byns = defaultdict(dict)
        for q, meths in self.classes.items():
            byns[q[:-1]][q[-1]] = sorted(meths)

        L = [
            "// Sext::FixWmlType -- one function, 11,944 bytes, and the "
            "whole unit.",
            "//",
            "// GENERATED by tools/gen_wmltypes.py; edit that, not this.",
            "//",
            "// The case table is DATA, read out of the image. Each",
            "// compared value is resolved along the control-flow path",
            "// that reaches it, because mwcc hoists a `lis` into r6/r7",
            "// and shares it across a subtree, so the textually nearest",
            "// one need not be the one that executed.",
            "//",
            "// Two cases carry no equality test: their subtree had",
            "// narrowed to one candidate and mwcc proved it with a pair",
            "// of range tests. Three more run nothing, and are written",
            "// `return;` rather than left empty, because mwcc DELETES a",
            "// case whose body is empty and deleting it moves the median",
            "// of the whole search.",
            "//",
            "// The classes below are STUBS: each carries the method the",
            "// call needs to name, and nothing else about it is known.",
            "",
        ]
        for ns in sorted(byns):
            for n in ns:
                L.append("namespace %s {" % n)
            for cls in sorted(byns[ns]):
                if ns + (cls,) in DEFINED:
                    L.append(DEFINED[ns + (cls,)])
                    continue
                decls = " ".join("void %s(long);" % mth
                                 for mth in byns[ns][cls])
                L.append("class %s { public: %s };" % (cls, decls))
            for n in reversed(ns):
                L.append("}  // namespace %s" % n)
            L.append("")

        L += [
            "namespace Sext { class EventAny; }",
            "",
            "template <class T>",
            "class Pointer32 {",
            "public:",
            "    T Get() const;",
            "};",
            "",
            "namespace Util {",
            "template <class T> void RTTID_Fix(void* p, long l) "
            "{ ((T*)p)->Fix(l); }",
            "}  // namespace Util",
            "",
            "namespace Sext {",
            "",
            "void FixWmlType(long l, int type, void* p) {",
            "    switch (type) {",
        ]
        for t in sorted(self.bytarget):
            for v in sorted(self.bytarget[t], key=sv):
                L.append("    case %d:" % sv(v))
            L += ["        " + ln if ln else "" for ln in bodies[t]]
            if bodies[t][-1].strip() != "return;":
                L.append("        break;")
            L.append("")
        L += ["    }", "}", "", "}  // namespace Sext", ""]
        return L, bodies


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--check", action="store_true",
                    help="report what would be written; write nothing")
    args = ap.parse_args()

    g = Gen()
    L, bodies = g.build()
    hand = sum(1 for t in bodies if t in HAND)
    empty = sum(1 for t in g.cases.values() if t == EPI)
    print("  %d case values, %d distinct bodies, %d comparison blocks"
          % (len(g.cases), len(g.bytarget), len(g.nodes)))
    print("  %d bodies translated mechanically, %d written by hand, "
          "%d run nothing" % (len(bodies) - hand, hand, empty))
    print("  %d classes declared" % len(g.classes))
    if args.check:
        old = OUT.read_text(encoding="utf-8") if OUT.exists() else None
        now = NL.join(L)
        print("  %s" % ("up to date" if old == now
                        else "OUT OF DATE -- rerun without --check"))
        return 0 if old == now else 1
    OUT.write_text(NL.join(L), encoding="utf-8")
    print("  wrote %s (%d lines)"
          % (OUT.relative_to(ROOT).as_posix().replace("/", "/"), len(L)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
