"""Read animation tables off the image and merge them into a unit.

    python tools/gen_animtables.py <unit.cpp> <symbol> [<symbol> ...]
    python tools/gen_animtables.py --calls <symbol> [<callee-prefix>]

395 functions in the image carry `AnimTable` in the name and 314 of
them are BRANCHLESS: a straight run of calls to xAnimTableNewState
(sixteen parameters) or xAnimTableNewTransition (fourteen) whose only
variable parts are a string, a flag word or two, a float, and the
callbacks. One forward walk over such a function, keeping the constant
GPRs, the FPRs loaded from a constant base, and the outgoing stack
slots, recovers every argument of every call. `--calls` prints that.

The merge writes those calls as source INTO the unit's existing file,
which the accessor generator will usually have written already -- the
weak `an...Check` callbacks the tables name are its animcb shape. Four
rules, each found by one diff and each worth a function:

  * the tables are `void` -- the mangled name carries no return type,
    and a `return 0;` is one `li r3,0` too many;
  * a callback passed to xAnimTableNewTransition returns `unsigned int`
    (the parameter is `PF..._Ui`; a bool function pointer does not
    convert), so the generated forwarders are retyped, declaration and
    definition, scoped to their own class;
  * a pointer that NAMES a symbol is that symbol: `@STRING@GetIdleString
    __13zPlayerIdleSBFv` is the literal an inlined GetIdleString()
    returns, and the table passes GetIdleString();
  * the unit's pool header must carry the WHOLE pool
    (gen_poolprefix.py --whole), or the unit's own strings fall in our
    order and every offset after the first disagreement is wrong.

Anything unresolved names the call and the slot and stops the merge;
nothing is guessed. A table with a branch in it is refused: that is a
different shape. Callee-saved FPRs survive a call -- `fmr f3,f31` is
how a table passes a float it keeps -- and the DWARF names such a
float (`AgingIdleBlendTime`); that local is added by hand afterwards.

The second shape dispatches through `bctrl`: `lwz r3,0(this)`,
`lwz r3,0(r3)`, `lwz r3,4k(r3)`, vptr at +12, slot 1, 2 or 3 -- that
is `manager->actions[k]->AddStandardTransitions / AddDefaultTransitions
/ AddTransitions(...)` by the order zPlayerAction.cpp declares them,
and the two `...From(table, name)` functions call the slot on `this`.
The walk keeps a symbolic `this` and follows loads from it, so the
object each call is made on is READ, not assumed; a chain it cannot
spell stops the merge. A class that makes such a call is derived from
the unit's zPlayerAction stub (three members, then the virtuals, so
the vptr lands at +12) and its leading padding shrinks by the base.

Three helpers zPlayerAction.cpp defines were inlined by retail's unity
build and change the order hoisted values come out in, so the merger
spells the calls through them and the unit defines them `inline`:
`AddActionTransition` (a direct transition whose third callback is
ActionChange with the fixed zeros), the manager's `AddTransitionsTo`
family (every slot call on `manager->actions[k]`), and `NewState` (a
direct state whose owner is `this` with a zero `m`). Each was found
by one diff and each is worth a table; the docstrings of the rules in
`emit()` give the word counts.
"""
import re
import struct
import sys
from collections import Counter, defaultdict
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import disasm as D                                       # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
NL = chr(10)

raw, secs, funcs, objs = D.load()
BYADDR = {a: nm for a, (nm, sz) in funcs.items() if nm}
STR_SECS = [(s[0], s[0] + s[2]) for s in secs if s[3] in (".rodata", ".data")]

STATE_SIG = ("unsigned int xAnimTableNewState(xAnimTable* table, "
             "const char* name, unsigned int a, unsigned int b, float c, "
             "float* d, float* e, float f, unsigned short* g, void* h, "
             "void (*i)(xAnimPlay*, xAnimState*, void*), "
             "void (*j)(xAnimPlay*, xAnimState*, void*), "
             "void (*k)(xAnimState*, xAnimSingle*, void*), "
             "void (*l)(xAnimPlay*, xQuat*, xVec3*, xVec3*, int), "
             "unsigned long long m, unsigned int n);")
TRANS_SIG = ("unsigned int xAnimTableNewTransition(xAnimTable* table, "
             "const char* from, const char* to, "
             "unsigned int (*a)(xAnimTransition*, xAnimSingle*, void*), "
             "unsigned int (*b)(xAnimTransition*, xAnimSingle*, void*), "
             "unsigned int (*c)(xAnimTransition*, xAnimSingle*, void*), "
             "unsigned int d, unsigned int e, float f, float g, "
             "unsigned short h, unsigned short i, float j, "
             "unsigned short* k);")
CB = {"P15xAnimTransitionP11xAnimSinglePv":
      ("unsigned int", "xAnimTransition*, xAnimSingle*, void*"),
      "P9xAnimPlayP10xAnimStatePv":
      ("void", "xAnimPlay*, xAnimState*, void*"),
      "P10xAnimStateP11xAnimSinglePv":
      ("void", "xAnimState*, xAnimSingle*, void*"),
      "P9xAnimPlayP5xQuatP5xVec3P5xVec3i":
      ("void", "xAnimPlay*, xQuat*, xVec3*, xVec3*, int")}
FORWARD = ["class xAnimTable;", "class xAnimState;", "class xAnimPlay;",
           "class xQuat;", "class xVec3;", "class xAnimSingle;",
           "class xAnimTransition;"]
ACTION_CHANGE = next(a for a, nm in BYADDR.items()
                     if nm.startswith("ActionChange__13zPlayerAction"))
HELPER = "inline unsigned int zPlayerAction::AddActionTransition("
MGR_HELPER = "inline void zPlayerActionManager::AddTransitionsTo("
NEWSTATE_HELPER = "inline unsigned int zPlayerAction::NewState("


# ---- reading ----------------------------------------------------------

def cstr(a, cap=1024):
    # The transition lists run long: zSBPlayerHammerAttack's
    # GetTransitionString() literal is past 80 characters, and an 80-
    # character cap read it as an unnamed function pointer.
    if not any(lo <= a < hi for lo, hi in STR_SECS):
        return None
    out = []
    for k in range(cap):
        b = D.read(raw, secs, a + k, 1)[0]
        if b == 0:
            return "".join(out)
        if not 32 <= b < 127:
            return None
        out.append(chr(b))
    return None


def f32(a):
    return struct.unpack(">f", D.read(raw, secs, a, 4))[0]


THIS = ("this",)
BCTRL = 0x4E800421


def clobber(regs, fprs):
    for r in [r for r in regs if r <= 12]:
        del regs[r]
    for k in [k for k in fprs if k <= 13]:               # f14-f31 survive
        del fprs[k]


def walk(sym, callee):
    """-> (addr, size, branches,
           [(call_addr, target, gprs, fprs, stack)]).

    target is the callee's name for a `bl`, or ("vslot", n, object) for
    a `bctrl` through vtable slot n of the object expression.  Register
    values are ints, or tuples: THIS, ("arg", n) for an argument register
    never written, ("ld", base, off) for a word loaded from one."""
    hit = [(a, sz) for a, (nm, sz) in funcs.items() if nm == sym]
    if not hit:
        raise SystemExit("gen_animtables: no symbol %s" % sym)
    addr, size = hit[0]
    ws = struct.unpack(">" + "I" * (size // 4), D.read(raw, secs, addr, size))
    branches = sum(1 for w in ws if (w >> 26) == 16
                   or ((w >> 26) == 18 and not (w & 1)))
    regs = {3: THIS, 4: ("arg", 4), 5: ("arg", 5)}
    fprs, stack, calls = {}, {}, []
    frame = 0
    for i, w in enumerate(ws):
        a = addr + 4 * i
        op = w >> 26
        if op == 18 and (w & 1):
            d = w & 0x03FFFFFC
            if d & 0x02000000:
                d -= 0x04000000
            # _savegpr_N is an interior label of one routine, with no
            # size, so the sized-function map has no name for it and the
            # symbol table has to be asked; it touches r11 and the stack
            # only, and `this` is still in r3 when the table copies it
            # out afterwards.
            nm = BYADDR.get(a + d) or D.name_at(funcs, objs, a + d) or ""
            if nm.startswith("_savegpr") or nm.startswith("_restgpr"):
                continue
            # Every other call is recorded, whatever it is: one the
            # emitter cannot spell must stop the merge, not vanish from
            # the body and leave the size to say something was missed.
            calls.append((a, nm, dict(regs), dict(fprs), dict(stack)))
            clobber(regs, fprs)
        elif op == 37 and ((w >> 21) & 31) == 1 and ((w >> 16) & 31) == 1:
            frame = 0x10000 - (w & 0xFFFF)                # stwu r1,-N(r1)
        elif w == BCTRL:
            t = regs.get(12)
            if isinstance(t, tuple) and t[0] == "ld" and t[1][0] == "ld" \
                    and t[1][2] == 12 and t[2] >= 8 and t[2] % 4 == 0:
                target = ("vslot", (t[2] - 8) // 4, t[1][1])
            else:
                target = ("vslot", None, t)
            calls.append((a, target, dict(regs), dict(fprs), dict(stack)))
            clobber(regs, fprs)
        elif op == 15:
            d, s = (w >> 21) & 31, (w >> 16) & 31
            imm = (w & 0xFFFF) << 16
            if s == 0:
                regs[d] = imm
            elif isinstance(regs.get(s), int):
                regs[d] = (regs[s] + imm) & 0xFFFFFFFF
            else:
                regs.pop(d, None)
        elif op == 14:
            d, s = (w >> 21) & 31, (w >> 16) & 31
            imm = w & 0xFFFF
            imm = imm - 0x10000 if imm & 0x8000 else imm
            if s == 0:
                regs[d] = imm & 0xFFFFFFFF
            elif isinstance(regs.get(s), int):
                regs[d] = (regs[s] + imm) & 0xFFFFFFFF
            else:
                regs.pop(d, None)
        elif op == 21:                                       # rlwinm
            s, d = (w >> 21) & 31, (w >> 16) & 31
            sh, mb, me = (w >> 11) & 31, (w >> 6) & 31, (w >> 1) & 31
            if isinstance(regs.get(s), int):
                v = ((regs[s] << sh) | (regs[s] >> (32 - sh))) & 0xFFFFFFFF
                mask = 0
                k = mb
                while True:
                    mask |= 1 << (31 - k)
                    if k == me:
                        break
                    k = (k + 1) & 31
                regs[d] = v & mask
            else:
                regs.pop(d, None)
        elif op == 31 and ((w >> 1) & 0x3FF) == 444 \
                and ((w >> 21) & 31) == ((w >> 11) & 31):     # mr
            d, s = (w >> 16) & 31, (w >> 21) & 31
            if s in regs:
                regs[d] = regs[s]
            else:
                regs.pop(d, None)
        elif op == 31 and ((w >> 1) & 0x3FF) == 467:         # mtspr
            pass
        elif op == 36:                                       # stw
            s, base = (w >> 21) & 31, (w >> 16) & 31
            off = w & 0xFFFF
            off = off - 0x10000 if off & 0x8000 else off
            if base == 1 and off != frame + 4:               # not the LR save
                stack[off] = regs.get(s, "?")
        elif op == 32:                                       # lwz
            d, base = (w >> 21) & 31, (w >> 16) & 31
            off = w & 0xFFFF
            off = off - 0x10000 if off & 0x8000 else off
            if isinstance(regs.get(base), tuple):
                regs[d] = ("ld", regs[base], off)
            else:
                regs.pop(d, None)
        elif op == 48:                                       # lfs
            d, base = (w >> 21) & 31, (w >> 16) & 31
            off = w & 0xFFFF
            off = off - 0x10000 if off & 0x8000 else off
            fprs[d] = f32((regs[base] + off) & 0xFFFFFFFF) \
                if isinstance(regs.get(base), int) else "?"
        elif op == 63 and ((w >> 1) & 0x3FF) == 72:          # fmr
            fprs[(w >> 21) & 31] = fprs.get((w >> 11) & 31, "?")
        elif op == 31:
            regs.pop((w >> 21) & 31, None)
        elif op in (33, 34, 35, 40, 41, 42, 43, 46):
            regs.pop((w >> 21) & 31, None)
    return addr, size, branches, calls


# ---- spelling ---------------------------------------------------------

class Merge(object):
    def __init__(self):
        self.class_decls = defaultdict(set)
        self.retype = defaultdict(set)
        self.free_decls = set()
        self.base_needed = set()
        self.helper_needed = False
        self.mgr_needed = False
        self.newstate_needed = False
        self.problems = []

    @staticmethod
    def split_sym(sym):
        m = re.match(r"^([A-Za-z_]\w*)__(\d+)(\w+)$", sym or "")
        if m:
            name, n, rest = m.group(1), int(m.group(2)), m.group(3)
            cls, params = rest[:n], rest[n:]
            return (name, cls, params[1:]) if params.startswith("F") else None
        m = re.match(r"^([A-Za-z_]\w*)__F(\w+)$", sym or "")
        return (m.group(1), None, m.group(2)) if m else None

    def fn_ref(self, v, where):
        if v == 0:
            return "0"
        sp = self.split_sym(BYADDR.get(v))
        if sp is None or sp[2] not in CB:
            self.problems.append("%s: function pointer %08X (%s)"
                                 % (where, v, BYADDR.get(v)))
            return "?"
        name, cls, params = sp
        ret, args = CB[params]
        if cls is None:
            self.free_decls.add("%s %s(%s);" % (ret, name, args))
            return name
        self.class_decls[cls].add("    static %s %s(%s);" % (ret, name, args))
        if ret == "unsigned int":
            self.retype[cls].add(name)
        return "%s::%s" % (cls, name)

    def lit(self, v, where):
        if v == ("arg", 4):
            return "table"
        if v == ("arg", 5):
            return "name"
        if v == ("ld", THIS, 0):
            return "manager"
        if v == ("ld", THIS, 4):
            return "player"
        if isinstance(v, (str, tuple)):
            self.problems.append("%s: unresolved register %r" % (where, v))
            return "?"
        nm = D.name_at(funcs, objs, v) if v >= 0x80000000 else None
        if nm and nm.startswith("@STRING@"):
            sp = self.split_sym(nm[len("@STRING@"):])
            text = cstr(v)
            if sp is None or text is None:
                self.problems.append("%s: %s" % (where, nm))
                return "?"
            fname, cls, _p = sp
            self.class_decls[cls].add(
                '    static const char* %s() { return "%s"; }' % (fname, text))
            return "%s::%s()" % (cls, fname)
        s = cstr(v)
        if s is not None:
            return '"%s"' % s
        if v >= 0x80000000:
            return self.fn_ref(v, where)
        return "%d" % v if v < 65536 else "0x%X" % v

    def flt(self, v, where):
        if v == "?":
            self.problems.append("%s: unresolved float" % where)
            return "?"
        if float(v) == int(v):
            return "%d.0f" % int(v)
        want = struct.unpack(">f", struct.pack(">f", v))[0]
        for digits in range(1, 10):
            text = "%.*g" % (digits, v)
            if struct.unpack(">f", struct.pack(">f", float(text)))[0] == want:
                return text + "f"
        return ("%r" % v) + "f"

    PARAMS = {"P10xAnimTable": "xAnimTable* table",
              "P10xAnimTablePCc": "xAnimTable* table, const char* name"}
    SLOTS = {1: "AddStandardTransitions", 2: "AddDefaultTransitions",
             3: "AddTransitions"}

    SCALAR = {"Us": "unsigned short", "Ui": "unsigned int",
              "Ul": "unsigned long", "Uc": "unsigned char", "i": "int",
              "s": "short", "c": "char", "b": "bool", "l": "long",
              "f": "float", "d": "double"}

    @classmethod
    def parse_params(cls, s):
        """CW mangled parameter list -> [(C++ type, kind)], kind 'g' for
        a GPR/stack word, 'f' for an FPR, 'cb' for a callback pointer.
        None when a token is not understood."""
        out, i = [], 0
        while i < len(s):
            rest = s[i:]
            hit = None
            for key, (ret, args) in CB.items():
                tok = "PF" + key + "_" + ("Ui" if ret == "unsigned int"
                                          else "v")
                if rest.startswith(tok):
                    hit = ("%s (*)(%s)" % (ret, args), "cb", len(tok))
            if hit is None and rest.startswith("PCc"):
                hit = ("const char*", "g", 3)
            if hit is None and rest.startswith("Pv"):
                hit = ("void*", "g", 2)
            if hit is None:
                m = re.match(r"P(\d+)", rest)
                if m:
                    n = int(m.group(1))
                    j = m.end() + n
                    hit = (rest[m.end():j] + "*", "g", j)
            if hit is None:
                for k in ("Us", "Ui", "Ul", "Uc", "i", "s", "c", "b", "l",
                          "f", "d"):
                    if rest.startswith(k):
                        hit = (cls.SCALAR[k], "f" if k in "fd" else "g",
                               len(k))
                        break
            if hit is None:
                return None
            out.append(hit[:2])
            i += hit[2]
        return out

    def free_call(self, sp, r, f, st, where):
        """A free function with a mangled signature: its arguments sit
        in r3.., f1.. and stack slots 8.. by the ABI, so the walk's
        snapshot names each one. Declared `void`: a discarded result
        compiles the same whatever the type."""
        name, _cls, params = sp
        types = self.parse_params(params)
        if types is None:
            self.problems.append("%s: cannot read the signature of %s__F%s"
                                 % (where, name, params))
            return "?"
        gpr, fpr, slot, args = 3, 1, 8, []
        for ty, kind in types:
            if kind == "f":
                args.append(self.flt(f.get(fpr, "?"), where))
                fpr += 1
                continue
            if gpr <= 10:
                v = r.get(gpr, "?")
                gpr += 1
            else:
                v = st.get(slot, "?")
                slot += 4
            args.append(self.fn_ref(v, where) if kind == "cb" and v != "?"
                        else self.lit(v, where))
        self.free_decls.add("void %s(%s);" % (name, ", ".join(
            t for t, _k in types)))
        return "%s(%s);" % (name, ", ".join(args))

    def obj_src(self, e, where):
        """The object a virtual call is made on, as source, from the load
        chain the walk read: `this`, or this->manager->actions[k]."""
        if e == THIS:
            return ""
        if isinstance(e, tuple) and e[0] == "ld" \
                and e[1] == ("ld", ("ld", THIS, 0), 0) and e[2] % 4 == 0:
            return "manager->actions[%d]->" % (e[2] // 4)
        self.problems.append("%s: virtual call on %r" % (where, e))
        return "?"

    def emit(self, sym):
        name, cls, params = self.split_sym(sym)
        if params not in self.PARAMS:
            raise SystemExit("gen_animtables: %s takes (%s); not a table "
                             "signature" % (sym, params))
        psrc = self.PARAMS[params]
        addr, size, branches, calls = walk(sym, "xAnimTableNew")
        if branches:
            raise SystemExit("gen_animtables: %s has %d branch(es); not a "
                             "table" % (sym, branches))
        lines = ["void %s::%s(%s) {" % (cls, name, psrc)]
        for a, callee, r, f, st in calls:
            where = "%s @%08X" % (name, a)
            L, F, P = self.lit, self.flt, self.fn_ref
            if isinstance(callee, tuple):
                _v, slot, obj = callee
                if slot not in self.SLOTS:
                    self.problems.append("%s: bctrl through %r" % (where, obj))
                    continue
                self.base_needed.add(cls)
                o = self.obj_src(obj, where)
                if slot == 3:
                    args = [L(r.get(4, "?"), where), L(r.get(5, "?"), where),
                            P(r.get(6, 0), where), P(r.get(7, 0), where),
                            L(r.get(8, "?"), where), F(f.get(1, "?"), where),
                            L(r.get(9, "?"), where), L(r.get(10, "?"), where),
                            "(zPlayerAction::SpecialActions)%s"
                            % L(st.get(8, "?"), where)]
                else:
                    args = [L(r.get(4, "?"), where), L(r.get(5, "?"), where)]
                if o.startswith("manager->actions["):
                    # Spelled through the manager's inlined helper --
                    # zPlayerActionManager::AddTransitionsTo and its two
                    # siblings, `actions[id]->...` in zPlayerAction.cpp,
                    # same translation unit -- because the direct call
                    # creates the pool-string temp before the load chain
                    # and retail did it the other way round: RunSB 5 of
                    # 299 words, PuckAttack 9 of 164, both 0 once inlined.
                    k = o[len("manager->actions["):-len("]->")]
                    self.mgr_needed = True
                    lines.append("    manager->%sTo(%s, %s);"
                                 % (self.SLOTS[slot], k, ", ".join(args)))
                else:
                    lines.append("    %s%s(%s);" % (o, self.SLOTS[slot],
                                                    ", ".join(args)))
            elif callee.startswith("xAnimTableNewState") \
                    and r.get(10) == THIS and st.get(24, 0) == 0 \
                    and st.get(28, 0) == 0:
                # zPlayerAction::NewState, inlined: the owner is `this`
                # and the helper supplies the zero `m`. Spelled direct,
                # the hoisted values come out permuted (zPlayerHitSB
                # 250 of 385 words; 11 through the helper, all of them
                # the member stores around the calls).
                args = ["table", L(r.get(4, "?"), where), L(r.get(5, "?"), where),
                        L(r.get(6, "?"), where), F(f.get(1, "?"), where),
                        L(r.get(7, "?"), where), L(r.get(8, "?"), where),
                        F(f.get(2, "?"), where), L(r.get(9, "?"), where),
                        P(st.get(8, 0), where), P(st.get(12, 0), where),
                        P(st.get(16, 0), where), P(st.get(20, 0), where),
                        L(st.get(32, 0), where)]
                self.newstate_needed = True
                lines.append("    NewState(%s);" % ", ".join(args))
            elif callee.startswith("xAnimTableNewState"):
                owner = "this" if r.get(10, "?") in ("?", THIS) \
                    else L(r[10], where)
                args = ["table", L(r.get(4, "?"), where), L(r.get(5, "?"), where),
                        L(r.get(6, "?"), where), F(f.get(1, "?"), where),
                        L(r.get(7, "?"), where), L(r.get(8, "?"), where),
                        F(f.get(2, "?"), where), L(r.get(9, "?"), where), owner,
                        P(st.get(8, 0), where), P(st.get(12, 0), where),
                        P(st.get(16, 0), where), P(st.get(20, 0), where),
                        L(st.get(28, 0), where) if st.get(24, 0) == 0 else "?",
                        L(st.get(32, 0), where)]
                lines.append("    xAnimTableNewState(%s);" % ", ".join(args))
            elif callee.startswith("xAnimTableNewTransition") \
                    and r.get(8) == ACTION_CHANGE and f.get(1) == 0.0 \
                    and f.get(2) == 0.0 and st.get(12, 0) == 0 \
                    and st.get(16, 0) == 0:
                # zPlayerAction::AddActionTransition, inlined: the source
                # passed 0 as the third callback and the helper's
                # `c == 0` test folded to ActionChange. Spelling the call
                # it as the direct one is one register pair the other way
                # round (zPlayerCheatSB, 21 of 150 words).
                args = ["table", L(r.get(4, "?"), where), L(r.get(5, "?"), where),
                        P(r.get(6, 0), where), P(r.get(7, 0), where), "0",
                        L(st.get(8, 0), where), F(f.get(3, "?"), where),
                        L(r.get(9, "?"), where), L(r.get(10, "?"), where)]
                self.helper_needed = True
                lines.append("    zPlayerAction::AddActionTransition(%s);"
                             % ", ".join(args))
            elif callee.startswith("xAnimTableNewTransition"):
                args = ["table", L(r.get(4, "?"), where), L(r.get(5, "?"), where),
                        P(r.get(6, 0), where), P(r.get(7, 0), where),
                        P(r.get(8, 0), where), L(r.get(9, "?"), where),
                        L(r.get(10, "?"), where), F(f.get(1, "?"), where),
                        F(f.get(2, "?"), where), L(st.get(8, 0), where),
                        L(st.get(12, 0), where), F(f.get(3, "?"), where),
                        L(st.get(16, 0), where)]
                lines.append("    xAnimTableNewTransition(%s);"
                             % ", ".join(args))
            else:
                sp = self.split_sym(callee)
                if sp is None or sp[1] is not None:
                    self.problems.append("%s: calls %s" % (where, callee))
                    continue
                lines.append("    " + self.free_call(sp, r, f, st, where))
        lines.append("}")
        self.class_decls[cls].add("    void %s(%s);" % (name, psrc))
        return cls, name, NL.join(lines), len(calls), psrc

    def into(self, unit, bodies):
        path = ROOT / "src" / unit
        text = path.read_text(encoding="utf-8")
        stem = Path(unit).name[:-4]
        inc = '#include "%s.pool.h"' % (Path(unit).with_suffix("").as_posix())
        old_banner = "// GENERATED by tools/gen_accessors.py from the retail image."
        if old_banner in text:
            text = text.replace(old_banner, NL.join([
                "// %s -- hand-owned since the animation tables were added."
                % Path(unit).name,
                "//",
                "// The accessor part below was generated by",
                "// tools/gen_accessors.py and is kept as it was written;",
                "// gen_units.py now HOLDS this file rather than regenerating",
                "// it, so a new accessor candidate is merged in by hand. The",
                "// tables were merged by tools/gen_animtables.py.",
                "//",
                "// This file is a fragment of a unity build: the generated",
                "// header puts the whole string pool in front, so the string",
                "// offsets baked into the code come out as retail has them.",
                inc,
                "",
                "// -- generated accessor part (gen_accessors.py) ------------",
            ]), 1)
        elif inc not in text:
            text = inc + NL + text
        if STATE_SIG not in text:
            text = text.replace(inc, inc + NL + NL + NL.join(FORWARD) + NL
                                + STATE_SIG + NL + TRANS_SIG, 1)
        for d in sorted(self.free_decls):
            if d in text:
                continue
            # A class the declaration points at must be declared before
            # it, and the unit's own forward declaration may sit later.
            at = text.index(TRANS_SIG)
            fwd = [n for n in re.findall(r"\b([A-Za-z_]\w*)\*", d)
                   if n not in ("char", "void", "int", "float", "short",
                                "long", "unsigned", "bool", "double")
                   and "class %s;" % n not in text[:at]]
            text = text.replace(TRANS_SIG, TRANS_SIG + NL + NL.join(
                ["class %s;" % n for n in sorted(set(fwd))] + [d]), 1)
        for cls, decls in self.class_decls.items():
            # The generated stub may carry a base clause -- `class
            # zBoardPlayerHammerPowerupAttack : public zPlayerWalk {` --
            # and a search for `class X {` misses it, appends a second
            # stub, and retypes a definition whose declaration it never
            # found: "redeclared".
            hm = re.search(r"(?m)^class %s\b[^{;]*\{" % re.escape(cls), text)
            head = hm.group(0) if hm else "class %s {" % cls
            if hm:
                i = hm.start()
                j = text.index(NL + "};", i)
                present = text[i:j]
                add = [d for d in sorted(decls)
                       if d.strip() not in present and not any(
                           d.strip().split("(")[0].split()[-1] + "(" in ln
                           for ln in present.splitlines())]
                if add:
                    text = text[:j] + NL + NL.join(add) + text[j:]
            else:
                text = text.rstrip(NL) + NL + NL + head + NL + "public:" + NL \
                    + NL.join(sorted(decls)) + NL + "};" + NL
            for nm in self.retype.get(cls, ()):
                if head in text:
                    i = text.index(head)
                    j = text.index(NL + "};", i)
                    blk = text[i:j].replace("    static bool %s(" % nm,
                                            "    static unsigned int %s(" % nm)
                    text = text[:i] + blk + text[j:]
                text = re.sub(r"(?m)^bool %s::%s\(" % (re.escape(cls), nm),
                              "unsigned int %s::%s(" % (cls, nm), text)
        # A class that calls a slot needs the vptr at +12 and `manager`
        # at 0: it derives from the unit's zPlayerAction stub, which is
        # written by hand ONCE (zPlayerAction.cpp is the model) and must
        # already be there, and its leading padding shrinks by the base.
        if self.base_needed:
            bm = re.search(r"(?m)^class zPlayerAction \{.*?^\};", text, re.S)
            if not bm or "virtual void AddTransitions(" not in bm.group(0):
                raise SystemExit("gen_animtables: %s has no zPlayerAction "
                                 "stub with the virtuals; add it by hand "
                                 "(three members, then _v0, AddStandard"
                                 "Transitions, AddDefaultTransitions, "
                                 "AddTransitions) before the first table"
                                 % unit)
        if self.helper_needed and HELPER not in text:
            raise SystemExit("gen_animtables: a table inlines zPlayerAction::"
                             "AddActionTransition and %s has no inline "
                             "definition of it; copy the one from "
                             "zSBPlayerActions.cpp, with its comment" % unit)
        if self.newstate_needed and NEWSTATE_HELPER not in text:
            raise SystemExit("gen_animtables: a table inlines zPlayerAction::"
                             "NewState and %s has no inline definition of "
                             "it; copy the one from zSBPlayerActions.cpp, "
                             "with its comment" % unit)
        if self.newstate_needed:
            for cls, _n, _b, _c, _p in bodies:
                self.base_needed.add(cls)
        if self.mgr_needed and MGR_HELPER not in text:
            raise SystemExit("gen_animtables: a table inlines the manager's "
                             "AddTransitionsTo family and %s has no inline "
                             "definitions of them; copy the block from "
                             "zSBPlayerActions.cpp, with its comment" % unit)
        for cls in sorted(self.base_needed):
            hm = re.search(r"(?m)^class %s\b([^{;]*)\{" % re.escape(cls), text)
            if hm is None:
                raise SystemExit("gen_animtables: no stub for %s" % cls)
            if ":" in hm.group(1):
                if "zPlayerAction" not in hm.group(1):
                    raise SystemExit("gen_animtables: %s derives from %s, "
                                     "not zPlayerAction" % (cls, hm.group(1)))
                continue
            head = "class %s : public zPlayerAction {" % cls
            text = text[:hm.start()] + head + text[hm.end():]
            i = text.index(head)
            j = text.index(NL + "};", i)
            pm = re.search(r"(?m)^    unsigned char _pad0\[0x([0-9A-Fa-f]+)\];"
                           + NL + "?", text[i:j])
            if pm:
                n = int(pm.group(1), 16)
                if n < 16:
                    raise SystemExit("gen_animtables: %s has 0x%X byte(s) "
                                     "before its first member, the base "
                                     "takes 16; rewrite that stub by hand"
                                     % (cls, n))
                repl = "" if n == 16 else \
                    "    unsigned char _pad0[0x%X];" % (n - 16) + NL
                text = text[:i + pm.start()] + repl + text[i + pm.end():]
            elif re.search(r"(?m)^    (?!static )\w[^;(]*;", text[i:j]):
                # A declaration without a parameter list is a data member
                raise SystemExit("gen_animtables: %s has data members but "
                                 "no leading _pad0; rewrite that stub by "
                                 "hand" % cls)
        marker = "// -- the animation tables, read from the image"
        if marker not in text:
            text = text.rstrip(NL) + NL + NL + marker \
                + " ------------------" + NL
        for cls, name, body, ncalls, psrc in bodies:
            head = "void %s::%s(%s) {" % (cls, name, psrc)
            if head in text:
                i = text.index(head)
                j = text.index(NL + "}", i) + 2
                text = text[:i] + body + text[j:]
            else:
                text += NL + "// %s::%s: %d call(s)" % (cls, name, ncalls) \
                    + NL + body + NL
        path.write_text(text, encoding="utf-8")
        print("  merged %d table(s), %d call(s), into %s"
              % (len(bodies), sum(b[3] for b in bodies), unit))
        for cls, names in self.retype.items():
            print("  %s: %d callback(s) retyped bool -> unsigned int"
                  % (cls, len(names)))


def show_calls(sym, callee):
    addr, size, branches, calls = walk(sym, callee)
    print("  %s  %08X  %d bytes  %d branch(es)  %d call(s) to %s*"
          % (sym.split("__")[0], addr, size, branches, len(calls), callee))
    unresolved = 0
    for a, target, r, f, st in calls:
        parts = []
        if isinstance(target, tuple):
            parts.append("vslot%s on %r" % (target[1], target[2]))
        else:
            parts.append(target.split("__")[0])
        for k in range(3, 11):
            v = r.get(k, "?")
            s = cstr(v) if isinstance(v, int) else None
            parts.append("r%d=%s" % (k, ('"%s"' % s) if s else v))
            unresolved += v == "?"
        for k in sorted(f):
            parts.append("f%d=%s" % (k, f[k]))
            unresolved += f[k] == "?"
        for k in sorted(st):
            # Outgoing argument slots only. The caller's own link-register
            # save is a stack store too (stw r0,52(r1)) and counted as an
            # unresolved argument until this said which slots are which.
            if 8 <= k < 36:
                parts.append("st%d=%s" % (k, st[k]))
                unresolved += st[k] == "?"
        print("    %08X  %s" % (a, "  ".join(parts)))
    print("  %d unresolved argument(s) over %d call(s)" % (unresolved,
                                                          len(calls)))


def main():
    if len(sys.argv) >= 3 and sys.argv[1] == "--calls":
        show_calls(sys.argv[2], sys.argv[3] if len(sys.argv) > 3
                   else "xAnimTableNew")
        return 0
    if len(sys.argv) < 3:
        print(__doc__.split(NL + NL)[1])
        return 2
    unit, syms = sys.argv[1], sys.argv[2:]
    m = Merge()
    bodies = [m.emit(s) for s in syms]
    if m.problems:
        print("  %d unresolved:" % len(m.problems))
        for p in m.problems[:12]:
            print("    " + p)
        raise SystemExit("gen_animtables: refusing to write")
    m.into(unit, bodies)
    return 0


if __name__ == "__main__":
    sys.exit(main())
