"""Generate the assets' `Fix(long)` -- the load-time relocation pass.

    python tools/gen_assetfix.py --survey
    python tools/gen_assetfix.py --validate
    python tools/gen_assetfix.py <unit>

THE FAMILY IS NOT TWO TEMPLATES, IT IS A SHORT VOCABULARY. The first
version of this tool checked a candidate's tail word for word against one
of two bodies written by hand, and read six of the eighty-four left. What
the other seventy-eight do is the same five things in different orders:

    CustomFix(base);                    the base class's own fixup
    m1C = (void*)((long)m1C + base);    a pointer member relocated
    Sext::FixWmlType(base, m08, m0C);   a (type, pointer) pair fixed
    m50.Fix(base);                      a sub-object at a known offset
    if (m10 == 0) { ... }               a member tested against zero

plus one loop: a cursor over an array whose FILE OFFSET is a member and
whose length is another, walked to the end and advanced by a constant
stride. Inside the loop the body is the same five things again, applied
to the cursor instead of to `this`.

So this reads the program out of the bytes rather than matching a
silhouette. A register file carries five symbolic values -- `this`,
`base`, a word loaded from a member, that word plus base, and a count
times a stride -- and every instruction either moves one of those or is
refused BY NAME. Nothing is dropped quietly: the old version `continue`d
past everything whose prologue differed, which is why it reported six and
not eighty-four.

THE VALIDATION IS THE SEVENTY-EIGHT THAT ALREADY MATCH. They are
known-good answers, and a reader that cannot read those has no business
writing the others -- `--validate` decodes each one, renders it, and
looks for the rendering in the file, so an emitter bug shows up against
bytes that are already proven rather than against bytes nobody has
compiled.

A LOOP IS WRITTEN IN BYTES, NOT IN ELEMENTS, wherever the element type is
a stub. `p + n` needs `sizeof(T)` to BE the stride, and the sub-object
stubs are deliberately EMPTY -- one byte -- because the offsets of every
member after them are measured against that size. `(T*)((char*)p + n *
S)` needs nothing from the type and emits the same multiply and add, so
the two facts stop fighting. The EventLinkNew loop keeps the element
form: that type is fully declared, 40 bytes, and its 77 bodies are
already matched in that spelling.

`end` IS DECLARED BEFORE THE CURSOR, and that is not cosmetic: with the
cursor first, retail's r30 and r31 come out swapped in fourteen of the
forty words. Three spellings were compiled to settle it.

AND THE BODIES GO INSIDE `#pragma dont_inline`. Without it mwcc takes
Util::RTTID_Fix<T> -- it is one line -- and the branch reaches T::Fix
directly where retail reaches the wrapper. Every word is still equal, so
only reloc_audit sees it, and reloc_audit is what caught it. A wrapper
for a T new to the unit is instantiated OUTSIDE that block, or the
inlining it DOES want does not happen either.

The unit must already declare the class as a `Fix(long)` stub, and the
type of each sub-object as one too. Anything missing is named and the
merge refuses; a member whose Fix is not in the unit is not invented.
"""
import argparse
import json
import re
import struct
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

import disasm as D                                          # noqa: E402

NL = chr(10)
PRELUDE = "class EventLinkNew {"
THIS = ("this",)
BASE = ("base",)

# The EventLinkNew loop body as the ops it decodes to: 77 of the 78
# already-matched bodies are this and nothing else.
EVENTLINK = ("rttid", "get", "fixwml", "rttid", "get", "fixwml")
EVENTLINK_STRIDE = 40


class Refuse(Exception):
    pass


def qualified(sym):
    """A mangled qualifier back to C++. A single one is written bare and
    two or more take Q<n>, so both spellings have to be handled."""
    m = re.match(r"^Q(\d)(.*)$", sym)
    parts, rest = [], m.group(2) if m else sym
    want = int(m.group(1)) if m else 1
    for _ in range(want):
        g = re.match(r"(\d+)", rest)
        if not g:
            return None
        n = int(g.group(1))
        rest = rest[g.end():]
        parts.append(rest[:n])
        rest = rest[n:]
    return "::".join(parts) if rest == "" else None


class Fn(object):
    def __init__(self, raw, secs, funcs, objs, addr, name, size):
        self.a, self.name, self.size = addr, name, size
        self.funcs, self.objs = funcs, objs
        b = D.read(raw, secs, addr, size)
        if b is None:
            raise Refuse("bytes outside every section")
        self.w = list(struct.unpack(">" + "I" * (size // 4), b))

    def tgt(self, i):
        w = self.w[i]
        d = w & 0x03FFFFFC
        if d & 0x02000000:
            d -= 0x04000000
        return D.name_at(self.funcs, self.objs,
                         d if (w & 2) else self.a + 4 * i + d)

    def bdest(self, i):
        w = self.w[i]
        op = w >> 26
        if op == 18:
            d = w & 0x03FFFFFC
            if d & 0x02000000:
                d -= 0x04000000
        elif op == 16:
            d = w & 0xFFFC
            if d & 0x8000:
                d -= 0x10000
        else:
            return None
        if w & 2:
            return None
        j = i + d // 4
        return j if 0 <= j <= len(self.w) else None


def prologue(f):
    """(first body word, frame size). A leaf has neither."""
    w = f.w
    if (w[0] >> 26) != 37 or ((w[0] >> 21) & 31) != 1 \
            or ((w[0] >> 16) & 31) != 1:
        return 0, 0
    frame = 0x10000 - (w[0] & 0xFFFF)
    if w[1] != 0x7C0802A6:
        raise Refuse("stwu is not followed by mflr")
    if (w[2] >> 26) != 36 or ((w[2] >> 16) & 31) != 1 \
            or ((w[2] >> 21) & 31) != 0 or (w[2] & 0xFFFF) != frame + 4:
        raise Refuse("the return address is not stored at frame+4")
    i = 3
    while i < len(w):
        op = w[i] >> 26
        if op == 47 and ((w[i] >> 16) & 31) == 1:            # stmw
            i += 1
            continue
        if op == 36 and ((w[i] >> 16) & 31) == 1 \
                and ((w[i] >> 21) & 31) >= 13:               # stw rN,M(r1)
            i += 1
            continue
        break
    return i, frame


def epilogue(f, frame):
    """One past the last body word."""
    w, n = f.w, len(f.w)
    if w[n - 1] == 0x4E800020:
        n -= 1
    elif (w[n - 1] >> 26) == 18 and not (w[n - 1] & 1):
        return n                          # a tail call: nothing to unwind
    else:
        raise Refuse("does not end in blr or a tail branch")
    if not frame:
        return n
    if (w[n - 1] >> 26) == 14 and ((w[n - 1] >> 21) & 31) == 1 \
            and (w[n - 1] & 0xFFFF) == frame:
        n -= 1
    else:
        raise Refuse("the frame is not popped with addi r1,r1,frame")
    if w[n - 1] == 0x7C0803A6:
        n -= 1
    else:
        raise Refuse("no mtlr in the epilogue")
    if (w[n - 1] >> 26) == 32 and ((w[n - 1] >> 16) & 31) == 1:
        n -= 1
    else:
        raise Refuse("the return address is not reloaded")
    while n and ((w[n - 1] >> 26) == 46
                 or ((w[n - 1] >> 26) == 32 and ((w[n - 1] >> 16) & 31) == 1
                     and ((w[n - 1] >> 21) & 31) >= 13)):
        n -= 1
    return n


CONDNAME = {(12, 0): "lt", (12, 1): "gt", (12, 2): "eq",
            (4, 0): "ge", (4, 1): "le", (4, 2): "ne"}
# The branch SKIPS the block, so the source condition is its negation.
NEGATE = {"lt": ">=", "gt": "<=", "eq": "!=",
          "ge": "<", "le": ">", "ne": "=="}


class Reader(object):
    """One function's words, read into the program they carry."""

    def __init__(self, f, lo, hi):
        self.f, self.lo, self.hi = f, lo, hi
        self.v = {3: THIS, 4: BASE}
        self.cmp = None

    def kill(self):
        for r in (0, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12):
            self.v[r] = None

    # -- one instruction ------------------------------------------------
    def step(self, i, ops):
        w = self.f.w[i]
        op = w >> 26
        d, a, b = (w >> 21) & 31, (w >> 16) & 31, (w >> 11) & 31
        imm = w & 0xFFFF
        simm = imm - 0x10000 if imm & 0x8000 else imm
        ext = (w >> 1) & 0x3FF

        if op == 31 and ext == 444:                          # or / mr
            self.v[a] = self.v.get(d) if d == b else None
            return
        if op in (32, 34, 40):                               # lwz lbz lhz
            src = self.v.get(a)
            k = {32: "w", 34: "b", 40: "h"}[op]
            if src == THIS:
                self.v[d] = ("mem", imm, k)
            elif src is not None and src[0] in ("cursor", "elem"):
                self.v[d] = ("cmem", src[1], imm, k)
            else:
                self.v[d] = None
            return
        if op == 31 and ext == 266:                          # add
            x, y = self.v.get(a), self.v.get(b)
            if not (y == BASE or (y is not None and y[0] == "scaled")):
                x, y = y, x
            if y == BASE and x is not None and x[0] in ("mem", "cmem"):
                self.v[d] = ("fixed",) + x[1:]
            elif y is not None and y[0] == "scaled" and x is not None \
                    and x[0] in ("fixed", "fixedat", "cursor"):
                self.v[d] = ("end", y[1], y[2], y[3])
            elif y is not None and y[0] == "idx" and x == THIS:
                self.v[d] = ("elem", 0)
            elif x is not None and x[0] == "idx" and y == THIS:
                self.v[d] = ("elem", 0)
            else:
                self.v[d] = None
            return
        if op == 14:                                         # addi
            src = self.v.get(a)
            if a == 0:
                self.v[d] = ("const", simm)
            elif src == THIS:
                self.v[d] = ("addr", simm)
            elif src is not None and src[0] == "cursor" and d == a:
                ops.append(("advance", simm))
            elif src is not None and src[0] == "cursor":
                self.v[d] = ("caddr", src[1], simm)
            elif src is not None and src[0] == "idx" and d == a:
                ops.append(("advance", simm))
            elif src is not None and src[0] == "ctr" and d == a:
                ops.append(("bump", simm))
            else:
                self.v[d] = None
            return
        if op == 7:                                          # mulli
            s = self.v.get(a)
            self.v[d] = ("scaled", s[1], simm, s[2]) if s is not None \
                and s[0] == "mem" else None
            return
        if op == 21:                                         # rlwinm
            sh, mb, me = (w >> 11) & 31, (w >> 6) & 31, (w >> 1) & 31
            s = self.v.get(a)
            self.v[d] = ("scaled", s[1], 1 << sh, s[2]) if (
                s is not None and s[0] == "mem" and mb == 0
                and me == 31 - sh) else None
            return
        if op in (36, 38, 44):                               # stw stb sth
            dst, src = self.v.get(a), self.v.get(d)
            if src is None:
                raise Refuse("a store of a value this reader cannot name")
            if dst == THIS and src[0] == "fixed" and src[1] == imm:
                ops.append(("reloc", imm))
                self.v[d] = ("fixedat", imm)
                return
            if dst is not None and dst[0] in ("cursor", "elem") \
                    and src[0] == "fixed" and src[1] == dst[1] \
                    and src[2] == imm:
                ops.append(("ereloc", imm))
                return
            raise Refuse("a store this reader cannot account for")
        if op == 18 and (w & 1):                             # bl
            self.call(self.f.tgt(i), ops)
            return
        if op == 11:                                         # cmpwi
            self.cmp = ("i", self.v.get(a), simm, True)
            return
        if op == 10:                                         # cmplwi
            self.cmp = ("i", self.v.get(a), imm, False)
            return
        if op == 31 and ext in (0, 32):                      # cmpw cmplw
            self.cmp = ("r", self.v.get(a), self.v.get(b), a, b)
            return
        raise Refuse("instruction 0x%08X is not in the vocabulary" % w)

    # -- one call -------------------------------------------------------
    def call(self, name, ops):
        name = name or "<unresolved>"
        if re.match(r"^CustomFix__", name):
            if self.v.get(3) != THIS:
                raise Refuse("CustomFix is not called on this")
            ops.append(("customfix", name))
            self.kill()
            return
        if name == "FixWmlType__4SextFliPv":
            t, p = self.v.get(4), self.v.get(5)
            if self.v.get(3) != BASE:
                raise Refuse("FixWmlType's first argument is not base")
            if t is None or p is None or t[0] not in ("mem", "cmem") \
                    or p[0] not in ("fixed", "fixedat", "got"):
                raise Refuse("FixWmlType's arguments do not read")
            ops.append(("fixwml", tuple(t[1:]), tuple(p[1:])))
            self.kill()
            return
        m = re.match(r"^Fix__(Q\d.*|\d+\w*)Fl$", name)
        if m:
            ty = qualified(m.group(1))
            if ty is None:
                raise Refuse("a Fix target that does not demangle")
            if self.v.get(4) != BASE:
                raise Refuse("a Fix call whose second argument is not base")
            r3 = self.v.get(3)
            if r3 is not None and r3[0] == "addr":
                ops.append(("memberfix", r3[1], ty))
            elif r3 is not None and r3[0] == "cursor":
                ops.append(("elemfix", 0, ty))
            elif r3 is not None and r3[0] == "caddr":
                ops.append(("elemfix", r3[2], ty))
            elif r3 == THIS:
                ops.append(("selffix", ty))
            else:
                raise Refuse("a Fix call on something this reader cannot name")
            self.kill()
            return
        m = re.match(r"^RTTID_Fix<(.+)>__4UtilFPvl_v$", name)
        if m:
            ty = qualified(m.group(1))
            r3 = self.v.get(3)
            if ty is None or self.v.get(4) != BASE:
                raise Refuse("an RTTID_Fix whose arguments do not read")
            if r3 is not None and r3[0] == "caddr":
                ops.append(("rttid", r3[2], ty))
            elif r3 is not None and r3[0] == "cursor":
                ops.append(("rttid", 0, ty))
            else:
                raise Refuse("an RTTID_Fix this reader cannot name")
            self.kill()
            return
        m = re.match(r"^Get__\d+Pointer32<(.+)>CFv$", name)
        if m:
            r3 = self.v.get(3)
            if r3 is None or r3[0] not in ("caddr", "cursor"):
                raise Refuse("a Pointer32::Get this reader cannot name")
            off = r3[2] if r3[0] == "caddr" else 0
            ops.append(("get", off))
            self.kill()
            self.v[3] = ("got", off)
            return
        raise Refuse("a call to %s" % name.split("__")[0])

    # -- structure ------------------------------------------------------
    def block(self, lo, hi):
        f, ops, i = self.f, [], lo
        while i < hi:
            w = f.w[i]
            op = w >> 26
            if op == 18 and not (w & 1):
                dest = f.bdest(i)
                if dest is None or dest >= hi:
                    if i != hi - 1:
                        raise Refuse("a branch out of the middle of a body")
                    self.call(f.tgt(i), ops)
                    i += 1
                    continue
                if dest <= i:
                    raise Refuse("a backward unconditional branch")
                latch = self.latch_of(i + 1, dest, hi)
                if latch is None:
                    raise Refuse("a forward branch that starts no loop")
                ops.append(self.loop(i, dest, latch))
                i = latch + 1
                continue
            if op == 16:
                dest = f.bdest(i)
                if dest is None or dest <= i or dest > hi:
                    raise Refuse("a conditional branch out of the body")
                ops.append(self.guard(i, dest))
                i = dest
                continue
            self.step(i, ops)
            i += 1
        return ops

    def latch_of(self, body_lo, cmp_at, hi):
        for j in range(cmp_at, min(hi, cmp_at + 4)):
            if (self.f.w[j] >> 26) == 16 and self.f.bdest(j) == body_lo:
                return j
        return None

    def guard(self, i, dest):
        w = self.f.w[i]
        name = CONDNAME.get(((w >> 21) & 31, ((w >> 16) & 31) & 3))
        c = self.cmp
        if name is None or c is None:
            raise Refuse("a conditional this reader cannot name")
        if c[0] != "i" or c[1] is None or c[1][0] != "mem":
            raise Refuse("a guard that is not a member tested against a "
                         "constant")
        return ("guard", c[1][1], c[1][2], name, self.block(i + 1, dest),
                c[2], c[3])

    def loop(self, bidx, cmp_at, latch):
        f = self.f
        save = dict(self.v)
        tmp = []
        for j in range(cmp_at, latch):
            self.step(j, tmp)
        if tmp:
            raise Refuse("a loop latch that does more than compare")
        c = self.cmp
        w = f.w[latch]
        cond = CONDNAME.get(((w >> 21) & 31, ((w >> 16) & 31) & 3))
        self.v = save
        if c is None or c[0] != "r":
            raise Refuse("a loop whose bound this reader cannot name")
        if cond == "lt":
            return self.idxloop(bidx, cmp_at, c)
        if cond != "ne":
            raise Refuse("a loop that does not close on %s" % cond)
        x, y, ra, rb = c[1], c[2], c[3], c[4]
        if x is not None and x[0] in ("fixed", "fixedat"):
            cur, other = ra, y
        else:
            cur, other = rb, x
        ev = self.v.get(cur)
        if ev is None or ev[0] not in ("fixed", "fixedat") \
                or other is None or other[0] != "end":
            raise Refuse("a cursor loop whose bounds do not read")
        arr = ev[1]
        self.v[cur] = ("cursor", arr)
        body = self.block(bidx + 1, cmp_at)
        adv = [o for o in body if o[0] == "advance"]
        if len(adv) != 1:
            raise Refuse("a cursor loop with %d advances" % len(adv))
        self.v[cur] = None
        return ("loop", arr, other[1], other[2], adv[0][1],
                [o for o in body if o[0] != "advance"], other[3])



    def idxloop(self, bidx, cmp_at, c):
        """A counted loop: `i` from zero to a member, reloaded each
        iteration, with the element addressed as `this + i * stride`.
        The counter is the register the compare names; the byte
        offset is the OTHER register still holding a literal zero,
        and there has to be exactly one of those."""
        x, y, ra, rb = c[1], c[2], c[3], c[4]
        if x is not None and x[0] == 'const' and x[1] == 0 \
                and y is not None and y[0] == 'mem':
            ctr, cnt = ra, y
        elif y is not None and y[0] == 'const' and y[1] == 0 \
                and x is not None and x[0] == 'mem':
            ctr, cnt = rb, x
        else:
            raise Refuse('an index loop whose bounds do not read')
        offs = [r for r, v in self.v.items()
                if v == ('const', 0) and r != ctr]
        if len(offs) != 1:
            raise Refuse('an index loop with %d offset register(s)'
                         % len(offs))
        self.v[ctr] = ('ctr',)
        self.v[offs[0]] = ('idx',)
        body = self.block(bidx + 1, cmp_at)
        adv = [o for o in body if o[0] == 'advance']
        bump = [o for o in body if o[0] == 'bump']
        if len(adv) != 1 or len(bump) != 1 or bump[0][1] != 1:
            raise Refuse('an index loop that does not advance once')
        self.v[ctr] = self.v[offs[0]] = None
        return ('idxloop', cnt[1], cnt[2], adv[0][1],
                [o for o in body if o[0] not in ('advance', 'bump')])


def read_one(raw, secs, funcs, objs, a, nm, sz):
    f = Fn(raw, secs, funcs, objs, a, nm, sz)
    lo, frame = prologue(f)
    hi = epilogue(f, frame)
    return Reader(f, lo, hi).block(lo, hi)


# ---------------------------------------------------------------------
# rendering

INT = {"w": ("int", 4), "h": ("unsigned short", 2), "b": ("unsigned char", 1)}


def names_of(ops):
    """What to call each member. The already-matched bodies say `links`,
    `linkCount` and `other`, and keeping those names is what lets
    --validate compare a rendering with the file rather than only with
    itself."""
    nm = {}
    loose = []
    for o in each(ops, "loop", []):
        try:
            if elem_type(o[5], o[4]) == "EventLinkNew":
                nm[o[1]], nm[o[2]] = "links", "linkCount"
        except Refuse:
            pass
    for o in each(ops, "reloc", []):
        if o[1] not in nm:
            loose.append(o[1])
    if len(set(loose)) == 1:
        nm[loose[0]] = "other"
    return nm


def fold(ops):
    """A reloc of an array is the loop's own preamble, not a statement of
    its own: `p = (T*)((long)links + base); links = p;` is both."""
    out, i = [], 0
    while i < len(ops):
        o = ops[i]
        if o[0] == "reloc" and i + 1 < len(ops) and ops[i + 1][0] == "loop" \
                and ops[i + 1][1] == o[1]:
            i += 1
            continue
        if o[0] == "loop":
            o = o[:5] + (fold(o[5]),) + tuple(o[6:])
        elif o[0] == "idxloop":
            o = o[:4] + (fold(o[4]),)
        elif o[0] == "guard":
            o = o[:4] + (fold(o[4]),) + tuple(o[5:])
        out.append(o)
        i += 1
    return out


def at(cur, off):
    """A byte offset from the cursor. The cursor of every loop but the
    EventLinkNew one is a `char*`, so the element type never has to be a
    real type and the sub-object stubs stay one byte."""
    return cur if off == 0 else "(%s + %d)" % (cur, off)


class Cursors(object):
    """A fresh (cursor, end) pair per loop.

    ONE pair shared by every loop cannot reproduce these bodies:
    BehaviorTree has four loops and VehicleConfiguration two, and in both
    retail gives the FIRST loop the opposite register of the rest --
    cursor r28/end r29 where the others take r29/r28. Two variables
    cannot hold two allocations, so the source has a pair per loop, and
    mwcc coalesces the disjoint live ranges the way the bytes show.
    `end` is still declared before its cursor: with the cursor first,
    retail's r30 and r31 come out swapped in fourteen of the forty words
    of the EventLinkNew body."""

    def __init__(self, kinds):
        self.kinds = kinds
        self.i = 0

    def names(self, i):
        return ("p" if i == 0 else "p%d" % (i + 1),
                "end" if i == 0 else "end%d" % (i + 1))

    def next(self):
        cur, fin = self.names(self.i)
        self.i += 1
        return cur, fin

    def declare(self):
        out = []
        for i, k in enumerate(self.kinds):
            cur, fin = self.names(i)
            ty = "EventLinkNew*" if k == "EventLinkNew" else "char*"
            out += ["    %s %s;" % (ty, fin), "    %s %s;" % (ty, cur)]
        return out


def elem_type(body, stride):
    """The element type of one loop, or a refusal naming its body. Only
    the EventLinkNew loop has one: everything else walks bytes."""
    sig = tuple(o[0] for o in body)
    if sig == EVENTLINK and stride == EVENTLINK_STRIDE:
        return "EventLinkNew"
    if sig and all(k in ("elemfix", "ereloc", "fixwml") for k in sig):
        return "char"
    raise Refuse("a loop body of %s this writer cannot spell" % " ".join(sig))


class Fields(object):
    """The members one program touches, each at the offset its bytes name."""

    def __init__(self):
        self.at = {}

    def put(self, off, ctype, name, size):
        old = self.at.get(off)
        if old is not None:
            if old == (ctype, name, size):
                return
            if old[2] == size and {old[0], ctype} == {
                    "int", "unsigned int"}:
                self.at[off] = ("unsigned int", name, size)
                return
            raise Refuse("member 0x%X reads as both %s and %s"
                         % (off, old[0], ctype))
        self.at[off] = (ctype, name, size)

    def render(self, cls, base):
        out = ["class %s%s {" % (cls, " : public %s" % base if base else ""),
               "public:", "    void Fix(long base);", ""]
        at, n = 0, 0
        for off in sorted(self.at):
            ctype, name, size = self.at[off]
            if off < at:
                raise Refuse("member 0x%X overlaps the one before it" % off)
            if off > at:
                out.append("    unsigned char _pad%d[0x%X];" % (n, off - at))
                n += 1
            out.append("    %s %s;" % (ctype, name))
            at = off + size
        out.append("};")
        return NL.join(out)


def collect(ops, fl, names, sizes):
    """Every member a program touches, with the type its use implies. A
    sub-object stub is ONE byte -- see Fields.render -- and that is what
    the offsets of every member after it are measured against."""
    for o in ops:
        k = o[0]
        if k == "reloc":
            fl.put(o[1], "void*", names(o[1]), 4)
        elif k == "fixwml":
            t = o[1]
            if len(t) == 3:                       # through the cursor
                continue
            ct, sz = INT[t[1]]
            fl.put(t[0], ct, names(t[0]), sz)
        elif k == "memberfix":
            fl.put(o[1], o[2], names(o[1]),
                   sizes.get(o[2].split("::")[-1], 1))
        elif k == "guard":
            ct, sz = INT[o[2]]
            if not o[6] and ct == "int":
                ct = "unsigned int"
            fl.put(o[1], ct, names(o[1]), sz)
            collect(o[4], fl, names, sizes)
        elif k == "idxloop":
            ct, sz = INT[o[2]]
            fl.put(o[1], ct, names(o[1]), sz)
        elif k == "loop":
            arr, cnt, _mul, stride, body = o[1], o[2], o[3], o[4], o[5]
            t = elem_type(body, stride)
            ct, csz = INT[o[6]]
            fl.put(cnt, ct, names(cnt), csz)
            fl.put(arr, "%s*" % t, names(arr), 4)
        elif k in ("customfix", "selffix"):
            pass
        else:
            raise Refuse("no member rule for %s" % k)


def emit(ops, indent, names, elem=None, cursors=None, cur="p"):
    out, pad = [], " " * indent
    for o in ops:
        k = o[0]
        if k == "customfix":
            out += [pad + "CustomFix(base);", ""]
        elif k == "reloc":
            n = names(o[1])
            out.append(pad + "%s = (void*)((long)%s + base);" % (n, n))
        elif k == "fixwml":
            t, p = o[1], o[2]
            if len(t) == 3 and elem == "EventLinkNew":
                out.append(pad + "Sext::FixWmlType(base, %s->%s, "
                           "%s->%s.Get());"
                           % (cur, "srcType" if t[1] == 0 else "dstType",
                              cur, "src" if p[0] == 4 else "dst"))
            elif len(t) == 3:
                out.append(pad + "Sext::FixWmlType(base, *(int*)%s, "
                           "*(void**)%s);" % (at(cur, t[1]), at(cur, p[1])))
            else:
                out.append(pad + "Sext::FixWmlType(base, %s, %s);"
                           % (names(t[0]), names(p[0])))
        elif k == "memberfix":
            out.append(pad + "%s.Fix(base);" % names(o[1]))
        elif k == "selffix":
            # `bl Fix__T` with r3 still `this`: T is a base at offset
            # zero. Written as a CAST rather than as inheritance, because
            # inheriting would put T's declared size in front of every
            # offset this class's own bytes name, and the offsets are
            # measured from the object start.
            out.append(pad + "((%s*)this)->Fix(base);" % o[1])
        elif k == "rttid":
            out.append(pad + "Util::RTTID_Fix<%s>(&%s->%s, base);"
                       % (o[2], cur, "src" if o[1] == 4 else "dst"))
        elif k == "get":
            pass                          # spelled inside the FixWmlType line
        elif k == "elemfix":
            out.append(pad + "((%s*)%s)->Fix(base);"
                       % (o[2], at(cur, o[1])))
        elif k == "ereloc":
            out.append(pad + "*(long*)%s += base;" % at(cur, o[1]))
        elif k == "guard":
            out.append(pad + "if (%s %s %d) {"
                       % (names(o[1]), NEGATE[o[3]], o[5]))
            out += emit(o[4], indent + 4, names, elem, cursors, cur)
            out.append(pad + "}")
        elif k == "idxloop":
            cnt, stride, body = o[1], o[3], o[4]
            out += ["",
                    pad + "for (i = 0; i < %s; i++) {" % names(cnt),
                    pad + "    char* e = (char*)this + i * %d;" % stride,
                    ""]
            out += emit(body, indent + 4, names, "char", cursors, "e")
            out.append(pad + "}")
        elif k == "loop":
            arr, cnt, mul, stride, body = o[1], o[2], o[3], o[4], o[5]
            t = elem_type(body, stride)
            cur, fin = cursors.next()
            if t == "EventLinkNew":
                out += [pad + "%s = (EventLinkNew*)((long)%s + base);"
                        % (cur, names(arr)),
                        pad + "%s = %s;" % (names(arr), cur),
                        pad + "%s = %s + %s;" % (fin, cur, names(cnt))]
            else:
                out += [pad + "%s = (char*)((long)%s + base);"
                        % (cur, names(arr)),
                        pad + "%s = %s;" % (names(arr), cur),
                        pad + "%s = %s + %s * %d;"
                        % (fin, cur, names(cnt), mul)]
            out += ["", pad + "while (%s != %s) {" % (cur, fin)]
            out += emit(body, indent + 4, names, t, cursors, cur)
            out.append(pad + "    %s++;" % cur if t == "EventLinkNew"
                       else pad + "    %s += %d;" % (cur, stride))
            out.append(pad + "}")
        else:
            raise Refuse("no source rule for %s" % k)
    return out


def dependency_order(fresh):
    """Where each fresh class must sit so that every class an inline body
    NAMES is complete before it. A forward declaration covers a pointer
    member; a cast through a type does not."""
    names = {s for _n, s, _d in fresh}
    need = {}
    for _n, s, d in fresh:
        body = d[d.find("void Fix(long base) {"):] \
            if "void Fix(long base) {" in d else ""
        need[s] = {t for t in names
                   if t != s and re.search(r"" + chr(92) + "b%s" % t, body)}
    order, placed = {}, set()
    while len(placed) < len(names):
        ready = sorted(n for n in names
                       if n not in placed and not (need[n] - placed))
        if not ready:                    # a cycle: leave the rest as-is
            ready = sorted(names - placed)
        for n in ready:
            order[n] = len(placed)
            placed.add(n)
    return order


def has_inline_fix(text, short):
    """Does the file already declare this class with a Fix BODY in it?
    Any spelling counts: the question is whether the wrapper is already
    accounted for, not whether it is spelled the way this tool spells
    it."""
    i = text.find("class %s {" % short + NL)
    if i < 0:
        i = text.find("class %s " % short)
        if i < 0:
            return False
    j = text.find(NL + "};", i)
    return j > 0 and "void Fix(long base) {" in text[i:j]


def declared_as(text, decl):
    """Is this declaration the one in the file? A nested stub the merge
    injected -- `class __srcEvent__ ...` inside LinkAssetBaseNew -- is an
    ADDED line and not a changed one, so the test is that every line the
    renderer produces appears in the file's block, in order."""
    if decl in text:
        return True
    lines = decl.splitlines()
    i = text.find(lines[0] + NL)
    if i < 0:
        return False
    block = text[i:text.find(NL + "};", i) + 3].splitlines()
    j = 0
    for line in block:
        if j < len(lines) and line == lines[j]:
            j += 1
    return j == len(lines)


def each(ops, kind, out):
    for o in ops:
        if o[0] == kind:
            out.append(o)
        if o[0] == "loop":
            each(o[5], kind, out)
        elif o[0] == "idxloop":
            each(o[4], kind, out)
        elif o[0] == "guard":
            each(o[4], kind, out)
    return out


def render(cls, ops, pinned, inline=False, sizes={}):
    """(declaration, body) for one asset. A class in `pinned` is one some
    OTHER asset already holds as a sub-object at a fixed offset, so its
    size is load-bearing and must stay at one byte -- it gets no
    declaration back, and its members are addressed off `this`."""
    ops = fold(ops)
    base = None
    for o in ops:
        if o[0] != "customfix":
            continue
        m = re.match(r"^CustomFix__(Q\d.*|\d+\w*)Fl$", o[1])
        b = qualified(m.group(1)) if m else None
        if b is None:
            raise Refuse("a CustomFix whose class does not demangle")
        base = b.split("::")[-1]

    kinds = [elem_type(o[5], o[4]) for o in each(ops, "loop", [])]
    counted = each(ops, "idxloop", [])

    if cls in pinned:
        raise Refuse("this class is a sub-object of another, so its size "
                     "may not change")

    nmap = names_of(ops)

    def names(off):
        return nmap.get(off, "m%X" % off)

    fl = Fields()
    collect(ops, fl, names, sizes)
    decl = fl.render(cls.split("::")[-1], base)

    cursors = Cursors(kinds)
    head = ["void %s::Fix(long base) {" % cls]
    if kinds or counted:
        head += cursors.declare()
        if counted:
            # UNSIGNED: retail compares the counter with `cmplw`, and a
            # signed `i` makes it `cmpw` whatever the member's own type
            # is, because the conversion goes the other way.
            head.append("    unsigned int i;")
        head.append("")
    body = emit(ops, 4, names, None, cursors)
    while body and body[-1] == "":
        body.pop()
    while body and body[0] == "":
        body.pop(0)
    if inline:
        inner = ["    void Fix(long base) {"]
        if kinds:
            inner += ["    " + l for l in cursors.declare()] + [""]
        inner += ["    " + l if l else "" for l in body]
        inner += ["    }"]
        lines = decl.splitlines()
        cut = lines.index("    void Fix(long base);")
        return NL.join(lines[:cut] + inner + lines[cut + 1:]), None
    return decl, NL.join(head + body + ["}"])


# ---------------------------------------------------------------------
# the population


def population():
    raw, secs, funcs, objs = D.load()
    matched = set()
    rep = json.loads((ROOT / "build/R8IE78/report.json").read_text())
    for u in rep.get("units", []):
        for fn in u.get("functions", []):
            if fn.get("fuzzy_match_percent", 0) >= 100:
                matched.add(fn["name"])

    ranges = D.unit_ranges()

    def unit_of(addr):
        for u, rs in ranges:
            for lo, hi in rs:
                if lo <= addr < hi:
                    return u
        return None

    rows, refused = [], defaultdict(list)
    for a in sorted(funcs):
        nm, sz = funcs[a]
        kind = None
        if re.match(r"^Fix__(Q\d|\d)", nm) and nm.endswith("Fl"):
            kind, m = "fix", re.match(r"^Fix__(Q\d.*|\d+\w*)Fl$", nm)
        elif nm.endswith(">__4UtilFPvl_v") and sz > 4:
            # A wrapper bigger than a tail call has T::Fix INSIDE it.
            kind, m = "inline", re.match(r"^RTTID_Fix<(.+)>__4UtilFPvl_v$", nm)
        if kind is None:
            continue
        cls = qualified(m.group(1)) if m else None
        if cls is None:
            refused["the class does not demangle"].append((nm, sz))
            continue
        try:
            ops = read_one(raw, secs, funcs, objs, a, nm, sz)
        except Refuse as e:
            refused[str(e)].append((nm, sz))
            continue
        rows.append((nm, cls, sz, unit_of(a), ops, nm in matched, kind))
    return rows, refused


SIZEOF = {"int": 4, "unsigned int": 4, "unsigned short": 2,
          "unsigned char": 1, "long": 4, "void*": 4, "char*": 4}


def sizes_from_file(text):
    """Every class the file declares with fields, and how many bytes it
    takes. A bare stub is absent and therefore one byte."""
    out = {}
    for m in re.finditer(r"^class (\w+)[^{]*\{\n(.*?)^\};", text,
                         re.S | re.M):
        name, block = m.group(1), m.group(2)
        at = 0
        for line in block.splitlines():
            g = re.match(r"^    unsigned char _pad\d+\[0x([0-9A-Fa-f]+)\];$",
                         line)
            if g:
                at += int(g.group(1), 16)
                continue
            g = re.match(r"^    ([A-Za-z_][A-Za-z0-9_:* ]*?)\s+"
                         r"(m[0-9A-F]+|links|linkCount|other)\;$", line)
            if g:
                ty = g.group(1).strip()
                at += SIZEOF.get(ty, 4 if ty.endswith("*") else 1)
        if at:
            out[name] = at
    return out


def pinned_classes(rows):
    """Classes some asset holds as a sub-object AT A FIXED OFFSET. Their
    stubs are empty -- one byte -- and every member after them in the
    holder is placed against that, so giving one of them members of its
    own would move the holder's offsets. The holders are already written
    and already match, so the size is not ours to change."""
    out = set()
    for row in rows:
        for o in each(row[4], "memberfix", []):
            out.add(o[2])
    return out


def merge(unit, rows, pinned):
    path = ROOT / "src" / unit
    if not path.exists():
        raise SystemExit("gen_assetfix: %s has no source file" % unit)
    text = path.read_text(encoding="utf-8")
    sizes = sizes_from_file(text)
    pinned = {c for c in pinned if c.split("::")[-1] not in sizes}
    if PRELUDE not in text:
        raise SystemExit("gen_assetfix: %s has no EventLinkNew; add the "
                         "prelude by hand (WAD00_32.cpp is the model)" % unit)

    bodies, problems, rt, left = [], [], set(), defaultdict(int)
    named = set()
    fresh = []                      # (namespace, declaration) with no stub
    for nm, cls, sz, _u, ops, _done, kind in sorted(rows, key=lambda r: r[1]):
        try:
            decl, body = render(cls, ops, pinned, kind == "inline", sizes)
        except Refuse as e:
            left[str(e)] += 1
            continue
        if kind == "inline":
            if has_inline_fix(text, cls.split("::")[-1]):
                left["already in the file"] += 1
                continue
        elif body in text:
            # Already written -- by an earlier run of this tool, or by
            # the version before it. The merge is keyed on the FILE, not
            # on report.json: keying it on what already matches made the
            # result depend on when the report was last generated, and
            # re-running after a build then rewrote three bodies and
            # dropped forty-five.
            left["already in the file"] += 1
            continue
        parts = cls.split("::")
        short = parts[-1]
        if len(parts) > 2:
            left["a nested class this writer cannot declare"] += 1
            continue
        stub = "class %s { public: void Fix(long); };" % short
        if stub not in text and (("class %s {" % short + NL) in text
                                 or ("class %s :" % short) in text):
            # Already DEFINED here, by another generator and for another
            # reason -- World::ShaderCodeBlobAsset carries a Create. Its
            # members are not ours to add to, and a second definition
            # does not compile.
            left["already defined in this unit for another reason"] += 1
            continue
        if stub in text:
            missing = []
            for line in decl.splitlines():
                m = re.match(r"^    ([A-Za-z_][A-Za-z0-9_:]*)\*? m[0-9A-F]+;$",
                             line)
                if not m or m.group(1) in ("int", "void", "unsigned", "char"):
                    continue
                t = m.group(1).split("::")[-1]
                if t == short:
                    continue
                if ("class %s " % t) not in text \
                        and ("class %s;" % t) not in text \
                        and ("class %s {" % t) not in text:
                    missing.append(t)
            if missing:
                # Not a refusal of the whole unit: a type this file has
                # no declaration for is one row that cannot be written,
                # and the other rows are unaffected. It IS counted --
                # nothing here is dropped without a reason and a number.
                problems.append("%s holds %s, which this unit does not "
                                "declare" % (cls, ", ".join(sorted(set(missing)))))
                continue
            text = text.replace(stub, decl, 1)
        else:
            # No stub: gen_rttid only writes one for a class some
            # RTTID_Fix<T> names, and not every Fix has a wrapper. The
            # declaration goes in this tool's own block instead, with a
            # forward declaration ahead of it so the order of the block
            # does not have to be a dependency order.
            fresh.append((parts[0] if len(parts) > 1 else "", short, decl))
        if body is not None:
            bodies.append(body)
        if kind == "inline":
            # Only AFTER every refusal: instantiating a wrapper whose
            # class was not written asks mwcc for a Fix that is not there.
            rt.add(cls)
        for o in each(ops, "rttid", []):
            rt.add(o[2])
        for opkind, ix in (("elemfix", 2), ("memberfix", 2), ("selffix", 1)):
            for o in each(ops, opkind, []):
                named.add(o[ix])

    for p in sorted(set(problems)):
        print("  not written: %s" % p)
    if not bodies and not fresh:
        raise SystemExit("gen_assetfix: nothing writable in %s" % unit)

    # A type a WRITTEN body names -- a loop's element, a sub-object, a
    # base -- has to be declared even when its own Fix could not be read.
    # A bare stub is the one-byte declaration the offsets everywhere else
    # are already measured against, so adding one moves nothing.
    have = {s for _n, s, _d in fresh}
    for cls in sorted(named):
        parts = cls.split("::")
        if len(parts) > 2:
            # A nested type -- `Sext::LinkAssetBaseNew::__srcEvent__`.
            # Its stub goes INSIDE the outer class, which only works if
            # this tool is the one declaring that class.
            outer, inner = parts[-2], parts[-1]
            for i, (ns, s, d) in enumerate(fresh):
                if s != outer or "public:" not in d:
                    continue
                if ("class %s " % inner) in d:
                    break
                fresh[i] = (ns, s, d.replace(
                    "public:",
                    "public:" + NL
                    + "    class %s { public: void Fix(long); };" % inner, 1))
                break
            else:
                problems.append("%s is nested in a class this unit declares "
                                "elsewhere, so its stub cannot be added" % cls)
            continue
        short = parts[-1]
        if short in have or ("class %s " % short) in text \
                or ("class %s;" % short) in text \
                or ("class %s {" % short) in text:
            continue
        have.add(short)
        fresh.append((parts[0] if len(parts) > 1 else "", short,
                      "class %s { public: void Fix(long); };" % short))

    head = []
    order = dependency_order(fresh)
    for ns in sorted({n for n, _s, _d in fresh}):
        rows_ns = [(s, d) for n, s, d in fresh if n == ns]
        open_ns = ["namespace %s {" % ns] if ns else []
        close_ns = ["}  // namespace %s" % ns] if ns else []
        head += open_ns
        head += ["class %s;" % s for s, _d in sorted(rows_ns)]
        head += [""]
        for _s, d in sorted(rows_ns, key=lambda r: order[r[0]]):
            head += [d, ""]
        head += close_ns + [""]

    inst = [t for t in sorted(rt)
            if ("Util::RTTID_Fix<%s>(void*, long);" % t) not in text]
    block = NL.join(
        head
        + (["#pragma always_inline on"] if inst else [])
        + ["template void Util::RTTID_Fix<%s>(void*, long);" % t for t in inst]
        + (["#pragma always_inline off"] if inst else [])
        + ["", "#pragma dont_inline on"]
        + [(NL + NL).join(bodies)]
        + ["#pragma dont_inline off"]) if bodies else NL.join(
        head
        + (["#pragma always_inline on"] if inst else [])
        + ["template void Util::RTTID_Fix<%s>(void*, long);" % t
           for t in inst]
        + (["#pragma always_inline off"] if inst else []))
    path.write_text(text.rstrip(NL) + NL + NL + block + NL, encoding="utf-8")
    print("  merged %d Fix bodies into %s (%d class(es) newly declared)"
          % (len(bodies), unit, len(fresh)))
    for k, n in sorted(left.items(), key=lambda kv: -kv[1]):
        print("  left x%-3d %s" % (n, k))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("unit", nargs="?")
    ap.add_argument("--survey", action="store_true")
    ap.add_argument("--validate", action="store_true")
    ap.add_argument("--show", default=None)
    args = ap.parse_args()

    rows, refused = population()
    pinned = pinned_classes(rows)
    sizes = {}
    nref = sum(len(v) for v in refused.values())

    if args.show:
        for nm, cls, sz, u, ops, done, kind in rows:
            if args.show not in nm:
                continue
            print("// %s -- %d bytes, %s, %s"
                  % (nm, sz, u, "MATCHED" if done else "to write"))
            for o in ops:
                print("//    %s" % (o,))
            decl, body = render(cls, ops, pinned,
                                kind == "inline", sizes)
            print(decl)
            if body is not None:
                print(body)
            print("")
        return

    if args.validate:
        path = ROOT / "src/SB/GM/Engine/WAD00_32.cpp"
        text = path.read_text(encoding="utf-8")
        sizes = sizes_from_file(text)
        pinned = {c for c in pinned if c.split("::")[-1] not in sizes}
        good, byhand, bad = 0, 0, []
        for nm, cls, sz, u, ops, done, kind in rows:
            if not done:
                continue
            try:
                decl, body = render(cls, ops, pinned, kind == "inline", sizes)
            except Refuse as e:
                bad.append((cls, "cannot be rendered: %s" % e))
                continue
            if body is None:
                # An inline row: the class carries the body. A spelling
                # this tool did not write still counts as written --
                # DTRMovieSettings is `offset += base` by hand -- so say
                # so rather than calling a matched function a failure.
                if declared_as(text, decl):
                    good += 1
                elif has_inline_fix(text, cls.split("::")[-1]):
                    byhand += 1
                else:
                    bad.append((cls, "renders differently from the file"))
            elif body in text and declared_as(text, decl):
                good += 1
            else:
                bad.append((cls, "renders differently from the file"))
        for cls, why in bad:
            print("  %-52s %s" % (cls, why))
        print("%d of %d already-matched Fix bodies render exactly as they "
              "are written; %d more are the same program in a spelling "
              "written by hand; %d do not"
              % (good, good + byhand + len(bad), byhand, len(bad)))
        return

    todo = [r for r in rows if not r[5]]
    done = [r for r in rows if r[5]]
    by = defaultdict(list)
    for r in rows:
        by[r[3]].append(r)

    if args.survey or not args.unit:
        print("  %d Fix function(s) named in the image; %d read, %d refused"
              % (len(rows) + nref, len(rows), nref))
        print("  of the read: %d already match, %d do not (%d bytes)"
              % (len(done), len(todo), sum(r[2] for r in todo)))
        for k, v in sorted(refused.items(), key=lambda kv: -len(kv[1])):
            print("  refused x%-3d %6d B  %s"
                  % (len(v), sum(s for _, s in v), k))
        print("")
        left_by = defaultdict(list)
        for r in todo:
            left_by[r[3]].append(r)
        for u, v in sorted(left_by.items(),
                           key=lambda kv: -sum(r[2] for r in kv[1])):
            ok, errs = 0, defaultdict(int)
            okb = 0
            for r in v:
                try:
                    render(r[1], r[4], pinned, r[6] == "inline",
                           sizes)
                    ok += 1
                    okb += r[2]
                except Refuse as e:
                    errs[str(e)] += 1
            print("  %-40s %3d left, %3d render (%d B)"
                  % (u, len(v), ok, okb))
            for k, n in sorted(errs.items(), key=lambda kv: -kv[1]):
                print("        x%-3d %s" % (n, k))
        return

    if args.unit not in by:
        raise SystemExit("gen_assetfix: nothing left to write in %s"
                         % args.unit)
    merge(args.unit, by[args.unit], pinned)


if __name__ == "__main__":
    main()
