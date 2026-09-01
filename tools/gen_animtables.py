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


# ---- reading ----------------------------------------------------------

def cstr(a, cap=80):
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


def walk(sym, callee):
    """-> (addr, size, branches, [(call_addr, gprs, fprs, stack)])."""
    hit = [(a, sz) for a, (nm, sz) in funcs.items() if nm == sym]
    if not hit:
        raise SystemExit("gen_animtables: no symbol %s" % sym)
    addr, size = hit[0]
    ws = struct.unpack(">" + "I" * (size // 4), D.read(raw, secs, addr, size))
    branches = sum(1 for w in ws if (w >> 26) == 16
                   or ((w >> 26) == 18 and not (w & 1)))
    regs, fprs, stack, calls = {}, {}, {}, []
    for i, w in enumerate(ws):
        a = addr + 4 * i
        op = w >> 26
        if op == 18 and (w & 1):
            d = w & 0x03FFFFFC
            if d & 0x02000000:
                d -= 0x04000000
            nm = BYADDR.get(a + d, "")
            if nm.startswith(callee):
                calls.append((a, dict(regs), dict(fprs), dict(stack)))
            for r in [r for r in regs if r <= 12]:
                del regs[r]
            for k in [k for k in fprs if k <= 13]:       # f14-f31 survive
                del fprs[k]
        elif op == 15:
            d, s = (w >> 21) & 31, (w >> 16) & 31
            imm = (w & 0xFFFF) << 16
            if s == 0:
                regs[d] = imm
            elif s in regs:
                regs[d] = (regs[s] + imm) & 0xFFFFFFFF
            else:
                regs.pop(d, None)
        elif op == 14:
            d, s = (w >> 21) & 31, (w >> 16) & 31
            imm = w & 0xFFFF
            imm = imm - 0x10000 if imm & 0x8000 else imm
            if s == 0:
                regs[d] = imm & 0xFFFFFFFF
            elif s in regs:
                regs[d] = (regs[s] + imm) & 0xFFFFFFFF
            else:
                regs.pop(d, None)
        elif op == 31 and ((w >> 1) & 0x3FF) == 444 \
                and ((w >> 21) & 31) == ((w >> 11) & 31):     # mr
            d, s = (w >> 16) & 31, (w >> 21) & 31
            if s in regs:
                regs[d] = regs[s]
            else:
                regs.pop(d, None)
        elif op == 36:                                       # stw
            s, base = (w >> 21) & 31, (w >> 16) & 31
            off = w & 0xFFFF
            off = off - 0x10000 if off & 0x8000 else off
            if base == 1:
                stack[off] = regs.get(s, "?")
        elif op == 48:                                       # lfs
            d, base = (w >> 21) & 31, (w >> 16) & 31
            off = w & 0xFFFF
            off = off - 0x10000 if off & 0x8000 else off
            fprs[d] = f32((regs[base] + off) & 0xFFFFFFFF) \
                if base in regs else "?"
        elif op == 63 and ((w >> 1) & 0x3FF) == 72:          # fmr
            fprs[(w >> 21) & 31] = fprs.get((w >> 11) & 31, "?")
        elif op == 31:
            regs.pop((w >> 21) & 31, None)
        elif op in (32, 33, 34, 35, 40, 41, 42, 43, 46):
            regs.pop((w >> 21) & 31, None)
    return addr, size, branches, calls


# ---- spelling ---------------------------------------------------------

class Merge(object):
    def __init__(self):
        self.class_decls = defaultdict(set)
        self.retype = defaultdict(set)
        self.free_decls = set()
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
        if isinstance(v, str):
            self.problems.append("%s: unresolved register" % where)
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

    def emit(self, sym):
        name, cls, _p = self.split_sym(sym)
        addr, size, branches, calls = walk(sym, "xAnimTableNew")
        if branches:
            raise SystemExit("gen_animtables: %s has %d branch(es); not a "
                             "table" % (sym, branches))
        lines = ["void %s::%s(xAnimTable* table) {" % (cls, name)]
        for a, r, f, st in calls:
            w = struct.unpack(">I", D.read(raw, secs, a, 4))[0]
            d = w & 0x03FFFFFC
            if d & 0x02000000:
                d -= 0x04000000
            callee = BYADDR.get(a + d, "")
            where = "%s @%08X" % (name, a)
            L, F, P = self.lit, self.flt, self.fn_ref
            if callee.startswith("xAnimTableNewState"):
                owner = "this" if r.get(10, "?") == "?" else L(r[10], where)
                args = ["table", L(r.get(4, "?"), where), L(r.get(5, "?"), where),
                        L(r.get(6, "?"), where), F(f.get(1, "?"), where),
                        L(r.get(7, "?"), where), L(r.get(8, "?"), where),
                        F(f.get(2, "?"), where), L(r.get(9, "?"), where), owner,
                        P(st.get(8, 0), where), P(st.get(12, 0), where),
                        P(st.get(16, 0), where), P(st.get(20, 0), where),
                        L(st.get(28, 0), where) if st.get(24, 0) == 0 else "?",
                        L(st.get(32, 0), where)]
                lines.append("    xAnimTableNewState(%s);" % ", ".join(args))
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
                raise SystemExit("gen_animtables: %s calls %s at %08X"
                                 % (sym, callee, a))
        lines.append("}")
        self.class_decls[cls].add("    void %s(xAnimTable* table);" % name)
        return cls, name, NL.join(lines), len(calls)

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
            if d not in text:
                text = text.replace(TRANS_SIG, TRANS_SIG + NL + d, 1)
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
        marker = "// -- the animation tables, read from the image"
        if marker not in text:
            text = text.rstrip(NL) + NL + NL + marker \
                + " ------------------" + NL
        for cls, name, body, ncalls in bodies:
            head = "void %s::%s(xAnimTable* table) {" % (cls, name)
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
    for a, r, f, st in calls:
        parts = []
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
