"""Write a module's CreateAnimTable(unsigned long long) from the image.

    python tools/gen_createanimtable.py <unit.cpp> <symbol>
    python tools/gen_createanimtable.py --calls <symbol>

Seven modules keep a LIST of the animation tables they have made, one
per id, and hand out the one whose id matches:

    xAnimTable* zSpinner::CreateAnimTable(unsigned long long id) {
        xAnimTable* table = animSpinnerTables;

        while (table) {
            if (table->id && *table->id == id) break;
            table = table->next;
        }

        if (table == 0) {
            <eight bytes for the id, xAnimTableNew, then the calls>
        }

        return table;
    }

The list node IS an xAnimTable -- `next` at +0 and the id pointer at
+0x1C -- which is why the found node is what comes back.

gen_animtables refuses these: it refuses a table with a branch in it,
and this one has a real search. The calls are still ITS reading; only
the head is a template here, and the template is checked word for word
against the head of a matched one before anything is written.

THE POOL HEADER IS NOT OPTIONAL. Written without it, zSpinner's came
out 684 bytes of 688 with 148 of 171 words differing -- the strings
fall at this unit's own offsets rather than the unity build's, and the
register allocation follows the addressing. With
`gen_poolprefix.py --whole` it is byte-identical. So this refuses to
write a unit that has no pool header.
"""
import argparse
import re
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import disasm as D                                        # noqa: E402
import gen_animtables as G                                # noqa: E402

NL = chr(10)

# The head, as the matched zSpinner has it: everything from the
# prologue to the call that makes the table. Read as a mask -- a word
# that carries an address or a displacement is wildcarded, and every
# other word has to agree exactly.
HEAD = [
    (0x94210000, 0xFFFF0000),   # stwu r1,-N(r1)
    (0x7C0802A6, 0xFFFFFFFF),   # mflr r0
    (0x90010000, 0xFFFF0000),   # stw r0,N(r1)
    (0x39610000, 0xFFFF0000),   # addi r11,r1,N
    (0x48000001, 0xFC000003),   # bl _savegpr_26
    (0x3C000000, 0xFC1F0000),   # lis rA,hi
    (0x7C000378, 0xFC0007FE),   # mr rD,rS
    (0x80000000, 0xFC000000),   # lwz rT,lo(rA)   -- the list
    (0x7C000378, 0xFC0007FE),   # mr rD,rS
    (0x48000000, 0xFC000003),   # b -> the loop test
    (0x8000001C, 0xFC00FFFF),   # lwz rK,0x1C(rT) -- the id pointer
    (0x2C000000, 0xFC00FFFF),   # cmpwi rK,0
    (0x41820000, 0xFFFF0000),   # beq -> next
    (0x80000000, 0xFC00FFFF),   # lwz rD,0(rK)
    (0x80000004, 0xFC00FFFF),   # lwz rK,4(rK)
    (0x7C000278, 0xFC0007FE),   # xor
    (0x7C000278, 0xFC0007FE),   # xor
    (0x7C000379, 0xFC0007FF),   # or.
    (0x41820000, 0xFFFF0000),   # beq -> found
    (0x80000000, 0xFC00FFFF),   # lwz rT,0(rT)
    (0x2C000000, 0xFC00FFFF),   # cmpwi rT,0
    (0x40820000, 0xFFFF0000),   # bne -> the loop body
    (0x2C000000, 0xFC00FFFF),   # cmpwi rT,0
    (0x40820000, 0xFFFF0000),   # bne -> the return
]

BODY = NL.join([
    "",
    "// %(cls)s::CreateAnimTable, read from the image. The module keeps a",
    "// list of the tables it has made, one per id; the node IS an",
    "// xAnimTable, which is why the found one is what comes back.",
    "xAnimTable* %(cls)s::CreateAnimTable(unsigned long long id) {",
    "    xAnimTable* table = %(list)s;",
    "",
    "    while (table) {",
    "        if (table->id && *table->id == id) {",
    "            break;",
    "        }",
    "",
    "        table = table->next;",
    "    }",
    "",
    "    if (table == 0) {",
    "        unsigned long long* key =",
    "            (unsigned long long*)Memory::AllocGlobalHeap(",
    "                %(size)d, (Memory::GlobalHeapEnum)0, (eMemMgrTag)%(tag)d,",
    "                false);",
    "",
    "        *key = id;",
    "",
    "        table = xAnimTableNew(%(name)s, %(flags)s, key, &%(list)s);",
    "%(calls)s",
    "    }",
    "",
    "    return table;",
    "}"])

BODY_STATIC = NL.join([
    "",
    "// %(cls)s::CreateAnimTable, read from the image. This one keeps the",
    "// id in a static of its own rather than taking eight bytes for it,",
    "// and hands the table that address.",
    "xAnimTable* %(cls)s::CreateAnimTable(unsigned long long id) {",
    "    xAnimTable* table = %(list)s;",
    "",
    "    while (table) {",
    "        if (table->id && *table->id == id) {",
    "            break;",
    "        }",
    "",
    "        table = table->next;",
    "    }",
    "",
    "    if (table == 0) {",
    "        %(key)s = id;",
    "",
    "        table = xAnimTableNew(%(name)s, %(flags)s, &%(key)s,",
    "                              &%(list)s);",
    "%(calls)s",
    "    }",
    "",
    "    return table;",
    "}"])

DECLS = NL.join([
    "class xAnimSingle;",
    "class xAnimTransition;",
    "class xAnimPlay;",
    "class xAnimState;",
    "class xQuat;",
    "class xVec3;",
    "",
    "// The table's own list: `next` at +0 and the id it was made for at",
    "// +0x1C, which is what the search reads. Nothing else about it is",
    "// known here.",
    "class xAnimTable {",
    "public:",
    "    xAnimTable* next;",
    "    unsigned char _pad0[0x1C - 0x4];",
    "    unsigned long long* id;",
    "};",
    "",
    "xAnimTable* xAnimTableNew(const char* name, unsigned int a, void* owner,",
    "                          xAnimTable** list);",
    "unsigned int xAnimTableNewState(xAnimTable* table, const char* name,",
    "                                unsigned int a, unsigned int b, float c,",
    "                                float* d, float* e, float f,",
    "                                unsigned short* g, void* h,",
    "                                void (*i)(xAnimPlay*, xAnimState*, void*),",
    "                                void (*j)(xAnimPlay*, xAnimState*, void*),",
    "                                void (*k)(xAnimState*, xAnimSingle*,"
    " void*),",
    "                                void (*l)(xAnimPlay*, xQuat*, xVec3*,",
    "                                          xVec3*, int),",
    "                                unsigned long long m, unsigned int n);",
    "unsigned int xAnimTableNewTransition(",
    "    xAnimTable* table, const char* from, const char* to,",
    "    unsigned int (*a)(xAnimTransition*, xAnimSingle*, void*),",
    "    unsigned int (*b)(xAnimTransition*, xAnimSingle*, void*),",
    "    unsigned int (*c)(xAnimTransition*, xAnimSingle*, void*),",
    "    unsigned int d, unsigned int e, float f, float g, unsigned short h,",
    "    unsigned short i, float j, unsigned short* k);"])


# The second shape keeps the id in a static of its own rather than
# on the heap, so the two `mr` that hold it are not there.
HEAD_STATIC = [w for i, w in enumerate(HEAD) if i not in (6, 8)]


def head_ok(ws):
    """-> (which head fitted, None) or (None, what disagreed).

    Both are tried, and the reason given is the FIRST head's: it
    is the one the seven were read from."""
    for tmpl in (HEAD, HEAD_STATIC):
        if len(ws) < len(tmpl):
            continue
        bad = None
        for i, (want, mask) in enumerate(tmpl):
            if (ws[i] & mask) != (want & mask):
                bad = ("word %d is %08X, not the head's %08X "
                       "under %08X" % (i, ws[i], want, mask))
                break
        if bad is None:
            return tmpl, None
        if tmpl is HEAD:
            first = bad
    return None, first


def read(sym):
    hit = [(a, sz) for a, (nm, sz) in G.funcs.items() if nm == sym]
    if not hit:
        raise SystemExit("gen_createanimtable: no symbol %s" % sym)
    addr, size = hit[0]
    ws = struct.unpack(">" + "I" * (size // 4),
                       D.read(G.raw, G.secs, addr, size))
    tmpl, why = head_ok(ws)
    if tmpl is None:
        raise SystemExit("gen_createanimtable: %s is not the shape this "
                         "writes -- %s" % (sym, why))
    return addr, size, ws, tmpl


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--calls", action="store_true")
    ap.add_argument("unit", nargs="?")
    ap.add_argument("sym")
    args = ap.parse_args()

    sym = args.sym
    m = re.match(r"^CreateAnimTable__(\d+)(\w+)FUx$", sym)
    if not m:
        raise SystemExit("gen_createanimtable: %s is not "
                         "CreateAnimTable__<n><class>FUx" % sym)
    cls = m.group(2)[:int(m.group(1))]

    addr, size, ws, tmpl = read(sym)
    _a, _s, _b, calls, _st, _ml, _h = G.walk(sym, "xAnimTable")

    # The list: the lwz at word 7, whose base was formed by the lis at 5.
    lis, lwz = ws[5], ws[7 if tmpl is HEAD else 6]
    lo = lwz & 0xFFFF
    lo = lo - 0x10000 if lo & 0x8000 else lo
    listaddr = (((lis & 0xFFFF) << 16) + lo) & 0xFFFFFFFF
    listname = D.name_at(G.funcs, G.objs, listaddr)
    # The list is often in a namespace, and sometimes in an ANONYMOUS
    # one -- mangled with the unity build's filename, which a split-out
    # unit cannot spell (see anon_blocked.py). Refused rather than
    # written as something that names nothing.
    ns = []
    if listname and "__" in listname:
        var, _u, tail = listname.partition("__")
        if "@" in tail:
            raise SystemExit(
                "gen_createanimtable: %s keeps its tables in an "
                "anonymous namespace (%s), which a split-out unit "
                "cannot name" % (cls, listname))
        if tail.startswith("Q"):
            raise SystemExit("gen_createanimtable: %s's list %s is nested "
                             "deeper than this spells" % (cls, listname))
        m2 = re.match(r"^(\d+)(\w+)$", tail)
        if not m2:
            raise SystemExit("gen_createanimtable: cannot read %s" % listname)
        ns = [m2.group(2)[:int(m2.group(1))]]
        listname = "::".join(ns + [var])
    if not listname:
        raise SystemExit("gen_createanimtable: nothing is named at %08X, "
                         "which is the list this walks" % listaddr)

    mg = G.Merge()
    mg.argnames, mg.fargnames = {}, {}
    L, F, P = mg.lit, mg.flt, mg.fn_ref

    alloc = [c for c in calls if c[1].startswith("AllocGlobalHeap")]
    tnew = [c for c in calls if c[1].startswith("xAnimTableNew__")]
    if len(tnew) != 1 or len(alloc) != (1 if tmpl is HEAD else 0):
        raise SystemExit("gen_createanimtable: %s makes %d allocation(s) and "
                         "%d table(s), which is neither shape"
                         % (sym, len(alloc), len(tnew)))
    ar = alloc[0][2] if alloc else {}
    nr = tnew[0][2]

    lines = []
    for a, callee, r, f, st in calls:
        where = "%s @%08X" % (cls, a)
        if callee.startswith("AllocGlobalHeap") or \
                callee.startswith("xAnimTableNew__"):
            continue
        if callee.startswith("xAnimTableNewState"):
            aa = ["table", L(r.get(4, "?"), where), L(r.get(5, "?"), where),
                    L(r.get(6, "?"), where), F(f.get(1, "?"), where),
                    L(r.get(7, "?"), where), L(r.get(8, "?"), where),
                    F(f.get(2, "?"), where), L(r.get(9, "?"), where),
                    L(r.get(10, "?"), where),
                    P(st.get(8, 0), where), P(st.get(12, 0), where),
                    P(st.get(16, 0), where), P(st.get(20, 0), where),
                    L(st.get(28, 0), where) if st.get(24, 0) == 0 else "?",
                    L(st.get(32, 0), where)]
            lines.append("        xAnimTableNewState(%s);" % ", ".join(aa))
        elif callee.startswith("xAnimTableNewTransition"):
            aa = ["table", L(r.get(4, "?"), where), L(r.get(5, "?"), where),
                    P(r.get(6, 0), where), P(r.get(7, 0), where),
                    P(r.get(8, 0), where), L(r.get(9, "?"), where),
                    L(r.get(10, "?"), where), F(f.get(1, "?"), where),
                    F(f.get(2, "?"), where), L(st.get(8, 0), where),
                    L(st.get(12, 0), where), F(f.get(3, "?"), where),
                    L(st.get(16, 0), where)]
            lines.append("        xAnimTableNewTransition(%s);"
                         % ", ".join(aa))
        else:
            mg.problems.append("%s: calls %s" % (where, callee))

    if mg.problems:
        print("  %d unresolved:" % len(mg.problems))
        for p in mg.problems:
            print("    %s" % p)
        raise SystemExit("gen_createanimtable: refusing to write")

    keyname = None
    if tmpl is not HEAD:
        keyaddr = nr.get(5)
        if not isinstance(keyaddr, int):
            raise SystemExit("gen_createanimtable: %s hands the table "
                             "%r, which is not an address"
                             % (cls, keyaddr))
        keyname = D.name_at(G.funcs, G.objs, keyaddr)
        if not keyname:
            raise SystemExit("gen_createanimtable: nothing is named at "
                             "%08X, which is %s's id"
                             % (keyaddr, cls))
        keyname = keyname.partition("__")[0]

    body = (BODY if tmpl is HEAD else BODY_STATIC) % {
        "cls": cls, "list": listname, "name": L(nr.get(3, "?"), cls),
        "flags": L(nr.get(4, "?"), cls),
        "size": ar.get(3, 0), "tag": ar.get(5, 0),
        "key": keyname or "",
        "calls": NL.join(lines)}

    if args.calls or not args.unit:
        print("  %s  %08X  %d bytes  %d call(s), list %s"
              % (cls, addr, size, len(lines), listname))
        print(body)
        return 0

    path = ROOT / "src" / args.unit
    text = path.read_text(encoding="utf-8")
    inc = '#include "%s.pool.h"' % args.unit[:-4]
    if inc not in text:
        raise SystemExit(
            "gen_createanimtable: %s has no pool header. Written without "
            "one, zSpinner's came out 684 bytes of 688 with 148 of 171 "
            "words differing: the strings fall at this unit's own offsets "
            "rather than the unity build's. Run `python "
            "tools/gen_poolprefix.py --whole %s` and include it FIRST."
            % (args.unit, args.unit))
    ext = "extern xAnimTable* %s;" % listname.rpartition("::")[2]
    for n2 in reversed(ns):
        ext = "namespace %s { %s }" % (n2, ext)
    if ext not in text:
        text = text.replace(inc + NL, inc + NL + NL + ext + NL, 1)
    if DECLS.splitlines()[7] not in text:
        text = text.replace(inc + NL, inc + NL + NL + DECLS + NL, 1)
    # Into the class's PUBLIC part, and only what is not there: the
    # declarations went in right after `class X {`, where they are
    # private and the definitions outside cannot reach them, and a
    # name the stub already carries came out twice.
    def declare(text, decl):
        want = re.search(r"(\w+)\(", decl)
        hm = re.search(r"(?m)^class %s\b[^{;]*\{" % re.escape(cls), text)
        if hm is None:
            raise SystemExit("gen_createanimtable: no stub for %s" % cls)
        end = text.index(NL + "};", hm.end())
        blk = text[hm.end():end]
        if want and re.search(r"[^A-Za-z0-9_]%s\(" % re.escape(want.group(1)), blk):
            return text
        pm = re.search(r"(?m)^public:$", blk)
        at = hm.end() + pm.end() if pm else hm.end()
        if not pm:
            decl = NL + "public:" + NL + decl
        return text[:at] + NL + decl + text[at:]

    for nm in sorted(mg.class_decls.get(cls, ())):
        text = declare(text, nm)
    if keyname:
        # The id's static is a member of the class -- the mangled name
        # is <var>__<len><class> and a single qualifier does not say
        # class from namespace, so it is declared where it cannot
        # collide with the class that is already here.
        d = "    static unsigned long long %s;" % keyname
        if d not in text:
            text = declare(text, d)
    for nm in mg.retype.get(cls, ()):
        text = re.sub(r"(?m)^(    )static bool (%s)\(" % re.escape(nm),
                      r"\1static unsigned int \2(", text)
        text = re.sub(r"(?m)^bool %s::%s\(" % (re.escape(cls), re.escape(nm)),
                      "unsigned int %s::%s(" % (cls, nm), text)
    head = "xAnimTable* %s::CreateAnimTable(unsigned long long id) {" % cls
    if head in text:
        i = text.index(head)
        j = text.index(NL + "}", i) + 2
        text = text[:i] + body.split(head)[0].split(NL, 1)[1] + head \
            + body.split(head, 1)[1] + text[j:]
    else:
        text = text.rstrip(NL) + NL + body + NL
        text = declare(text, "    static xAnimTable* CreateAnimTable(unsigned long long id);")
    path.write_text(text, encoding="utf-8")
    print("  wrote %s::CreateAnimTable, %d call(s), into %s"
          % (cls, len(lines), args.unit))
    return 0


if __name__ == "__main__":
    sys.exit(main())
