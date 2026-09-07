"""Turn a solved function into its unsolved twin, by naming every hole.

    python tools/transplant.py <target>            the fill sheet
    python tools/transplant.py <target> --from <solved>
    python tools/transplant.py --cluster <any>     who is in the cluster
    python tools/transplant.py --emit <target>     source, if a template fits
    python tools/transplant.py --survey            every mixed cluster

`twin_census.py` answers "which unsolved function has the same shape as
one already written". It stops there, and the rest was being done by
hand: read both listings, notice what differs, retype the source with
the differences substituted. That is a mechanical step and this is it.

WHAT A HOLE IS. Two functions in a twin_census cluster have the same
instruction skeleton, which means every opcode and every register agrees
and only three kinds of field can differ: a branch displacement, a
16-bit immediate, and the halves of a lis/addi address. So walking the
two word arrays in lockstep and printing the words that differ yields
the COMPLETE list of what the source has to change, with nothing else in
it. Everything not on that list is already right in the written twin.

That completeness is the whole value. A near-miss hunted by eye is a
search; a fill sheet is a form.

WHY IT CANNOT BE THE WHOLE ANSWER, stated here so the tool is not
oversold. The skeleton blanks those three fields, so it also HIDES them:
two members can differ in a size, an offset, a constant or which symbol
a call names, and the source has to get each right. This finds and names
them; it does not know which C++ spelling produces them. And a cluster
of two is weaker evidence than a cluster of twenty -- one solved member
may itself have been written in an unusual way that happens to match.

WHAT THE DWARF ADDS, and it is the part that surprised. The retail ELF
is an unstripped CodeWarrior link carrying 4.68 MB of DWARF 2, so for
most of these functions the compiler already recorded the answer to half
the question: `dwarf_lines.py` names the FILE a function was declared in
and the source lines its instructions came from, and `dwarf_locals.py`
names the parameters and which register each occupied. Neither says what
the expressions were -- that is still the bytes' job -- but a target
whose file, line span, statement count and parameter names are all known
is a much smaller search than one where they are not. Every fill sheet
here carries that block when the DWARF has it, and says so plainly when
it does not.

Nothing here decides that anything matches. It emits candidate source;
`unitcmp.py` and a full `ninja` are still the only test, and a template
that fits the skeleton can still produce different bytes.
"""

import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import disasm as D                                        # noqa: E402
import twin_census as TC                                  # noqa: E402

NL = chr(10)


# --------------------------------------------------------------------------
# The mangling, only as far as a class name. mwcc writes a qualified name
# as Q<n><len><name>... and a plain one as <len><name>, so `Init__11z
# HintSphereFPQ24Sext16zHintSphereAsset` names class `zHintSphere` and
# `__vt__11zHintSphere` names the same one. That is all this needs: which
# TYPE a call belongs to. Parameter types are left alone -- a wrong
# demangling of those would be read as fact, and nothing here needs them.
# --------------------------------------------------------------------------

def _take_len_name(s, i):
    """-> (name, next index) for a <len><name> run at i, or (None, i)."""
    j = i
    while j < len(s) and s[j].isdigit():
        j += 1
    if j == i:
        return None, i
    n = int(s[i:j])
    if j + n > len(s):
        return None, i
    return s[j:j + n], j + n


def class_of(sym):
    """The class a mangled member function or vtable symbol belongs to.

    Returns None rather than a guess when the shape is not recognised.
    """
    if sym.startswith("__vt__"):
        body, i = sym[6:], 0
    else:
        m = re.search("__(Q?[0-9])", sym)
        if not m:
            return None
        body, i = sym[m.start() + 2:], 0

    if body[i:i + 1] == "Q":
        # Q<count> then <count> length-prefixed components.
        if not body[i + 1:i + 2].isdigit():
            return None
        count = int(body[i + 1])
        i += 2
        parts = []
        for _ in range(count):
            nm, i = _take_len_name(body, i)
            if nm is None:
                return None
            parts.append(nm)
        return "::".join(parts)

    nm, _i = _take_len_name(body, i)
    return nm


# --------------------------------------------------------------------------


def load_all():
    game, matched, source = TC.population()
    cl, read, unread = TC.clusters(game, matched)
    raw, secs, funcs, objs = D.load()
    addr_of = {}
    for a, (n, _s) in sorted(funcs.items()):
        addr_of.setdefault(n, a)
    return dict(game=game, matched=matched, source=source, cl=cl,
                read=read, unread=unread, raw=raw, secs=secs,
                funcs=funcs, objs=objs, addr_of=addr_of)


def cluster_of(env, name):
    """-> (solved, unsolved) member lists for the cluster holding `name`."""
    for members in env["cl"].values():
        if any(m[0] == name for m in members):
            return (sorted(m for m in members if m[2]),
                    sorted(m for m in members if not m[2]))
    return None, None


def resolved(row):
    """What an instruction NAMES: the symbol a branch reaches, or the
    address a lis/addi pair builds and what lives there. None when it
    names nothing -- never a guess, and never an empty string, because
    an empty string compares equal to another empty string and that
    would silently fold two unrelated holes together."""
    note = row[4]
    if note.startswith("  ->"):
        return note[4:].strip() or None
    if note.startswith("  ="):
        # `  = 806C1560  __vt__11zHintSphere`, or the address alone when
        # nothing is named there. Return the SYMBOL when there is one:
        # two functions that form the same address through different
        # lis/addi halves are still naming the same thing, and the
        # address alone would be compared equal to itself for the wrong
        # reason.
        rest = note[3:].split()
        if len(rest) >= 2:
            return rest[1]
        return rest[0] if rest else None
    return None


# disasm.py names an in-function branch target L1, L2, ... so a
# resolved name of that shape is a LABEL and not a symbol. Both
# readers below skip them: an internal branch calls nothing.
LABEL = re.compile("^L[0-9]+$")


def rows_for(env, name):
    a = env["addr_of"].get(name)
    if a is None:
        return None, None
    _nm, size = env["funcs"][a]
    words, rows, unknown = D.annotate(env["raw"], env["secs"], env["funcs"],
                                      env["objs"], a, size)
    if words is None:
        return None, None
    return rows, unknown


def fill_sheet(env, target, donor, show_same=False):
    trows, tunknown = rows_for(env, target)
    drows, dunknown = rows_for(env, donor)
    if trows is None or drows is None:
        sys.exit("transplant: could not read one of the two functions")
    if len(trows) != len(drows):
        sys.exit("transplant: %d instruction(s) against %d -- these are not "
                 "twins, and a fill sheet across different lengths would be "
                 "an alignment guess" % (len(trows), len(drows)))

    print("  TRANSPLANT")
    print("    into  %s" % target)
    print("    from  %s  (matched)" % donor)
    print("          %s" % (env["source"].get(donor) or "(no source path)"))
    print("")

    # A differing WORD is not a differing SOURCE. Two calls to the same
    # function from two addresses carry two displacements and so two
    # words, and the C++ that produced them is the same line. Folding
    # those away took the first sheet from eight entries to four, and
    # the four are the whole of what a transplant has to decide.
    same_call, holes = [], []
    for i, (trow, drow) in enumerate(zip(trows, drows)):
        if trow[1] == drow[1]:
            continue
        tsym, dsym = resolved(trow), resolved(drow)
        if tsym is not None and tsym == dsym:
            same_call.append((i, dsym))
            continue
        # The high half of a lis/addi pair is not its own hole: the
        # addi below it carries the whole address and the symbol.
        if (drow[3].text.startswith("lis") and i + 1 < len(trows)
                and resolved(trows[i + 1]) is not None
                and trows[i + 1][1] != drows[i + 1][1]):
            continue
        holes.append((i, drow, trow))

    print("  %d of %d instruction(s) are already byte-identical."
          % (sum(1 for t, d in zip(trows, drows) if t[1] == d[1]),
             len(trows)))
    if same_call:
        print("  %d differ only in a branch displacement and reach the SAME "
              "symbol:" % len(same_call))
        for i, sym in same_call:
            print("      +0x%03X  %s" % (i * 4, sym))
        print("  Those are not holes -- the source that produced them is "
              "unchanged.")
    print("  %d hole(s) remain." % len(holes))
    if tunknown or dunknown:
        print("  %d instruction(s) in the target and %d in the donor did not "
              "decode" % (tunknown, dunknown))
        print("  -- a .word is not a no-op, and a hole in one is not "
              "characterised here.")
    print("")

    if not holes:
        print("  No holes. These two are byte-identical, which means the "
              "linker")
        print("  may have FOLDED them onto one another -- check before "
              "writing anything.")
        return holes

    print("  THE HOLES -- everything the source has to change, and nothing "
          "else:")
    print("")
    for i, drow, trow in holes:
        dtxt = (drow[3].text + drow[4]).strip()
        ttxt = (trow[3].text + trow[4]).strip()
        print("    +0x%03X   written  %s" % (i * 4, dtxt))
        print("             TARGET   %s" % ttxt)
        print("")

    kinds = {"call": 0, "immediate": 0, "address": 0}
    for i, drow, trow in holes:
        if trow[3].target is not None:
            kinds["call"] += 1
        elif trow[4].startswith("  ="):
            kinds["address"] += 1
        else:
            kinds["immediate"] += 1
    print("  %d call(s), %d immediate(s), %d formed address(es)."
          % (kinds["call"], kinds["immediate"], kinds["address"]))

    named = set()
    for i, drow, trow in holes:
        if trow[3].target is not None and trow[4].startswith("  ->"):
            sym = trow[4][5:].strip()
            c = class_of(sym)
            if c:
                named.add(c)
        if trow[4].startswith("  ="):
            sym = trow[4].split()[-1]
            c = class_of(sym)
            if c:
                named.add(c)
    if named:
        print("  The holes name these type(s): %s"
              % ", ".join(sorted(named)))
    print("")
    return holes


def dwarf_note(addr):
    """The line-table and locals block for one function, printed by the
    two tools that own those tables. Absence is reported, never
    rendered as an empty success."""
    import subprocess
    got = []
    for tool in ("dwarf_lines.py", "dwarf_locals.py"):
        r = subprocess.run([sys.executable, str(ROOT / "tools" / tool),
                            "0x%08X" % addr], cwd=str(ROOT),
                           capture_output=True, text=True)
        if r.returncode != 0 and not r.stdout.strip():
            got.append("  %s: exit %d, no output -- NOT read"
                       % (tool, r.returncode))
            continue
        body = [ln for ln in r.stdout.splitlines() if ln.strip()]
        if not body:
            got.append("  %s: nothing for this address" % tool)
            continue
        got.extend("  " + ln for ln in body)
    return got


# --------------------------------------------------------------------------
# Templates. A template is the DONOR'S OWN SOURCE with the holes named,
# so registering one is a claim about a shape that has already matched
# rather than a theory about what the compiler does. It is keyed by the
# donor because the donor is what the caller chose; transplanting from a
# different solved twin picks a different template, or none.
#
# `describe` turns a fill sheet into the handful of facts a template
# needs, and refuses rather than guessing when the shape it expects is
# not there. Nothing emitted here is a match until it compiles.
# --------------------------------------------------------------------------


# The one-letter builtin codes. `U` in front of one of these is the
# unsigned prefix, which is why it is NOT in the modifier loop below.
BUILTIN = {"v": "void", "c": "char", "s": "short", "i": "int",
           "l": "long", "x": "long long", "f": "float", "d": "double",
           "b": "bool", "w": "wchar_t"}


def param_start(sym):
    """Index just past the `F` that opens a mangled parameter list.

    A free function is `name__F<params>`; a member is
    `name__<class>F<params>`, and the class is either Q-qualified or a
    single length-prefixed name. Returns None rather than a guess.
    """
    # The separator is the `__` followed by the class or by F --
    # NOT the leading one of `__ct__`, which is what a plain find()
    # returns and what made every constructor parse to nothing.
    m = re.search("__(F|Q[0-9]|[0-9])", sym)
    if m is None:
        return None
    j = m.start() + 2
    if sym[j:j + 1] == "F":
        return j + 1
    if sym[j:j + 1] == "Q":
        if not sym[j + 1:j + 2].isdigit():
            return None
        count, j = int(sym[j + 1]), j + 2
        for _ in range(count):
            nm, j = _take_len_name(sym, j)
            if nm is None:
                return None
    elif sym[j:j + 1].isdigit():
        nm, j = _take_len_name(sym, j)
        if nm is None:
            return None
    else:
        return None
    return j + 1 if sym[j:j + 1] == "F" else None


def params_of(sym):
    """The parameter list of a mangled function, structured.

    Each entry is {text, base, ref, ptr}: `text` is the C++ spelling
    for a declaration and `base` the type a cast has to name. Only the
    forms this family uses are decoded -- P, R, C and U over a
    length-prefixed or Q-qualified name. An encoding not in that list
    returns None for the WHOLE list rather than a partial answer,
    because a parameter list that is right up to the third entry is
    worse than none: it compiles.
    """
    j = param_start(sym)
    if j is None:
        return None
    rest, out = sym[j:], []
    while rest:
        pre, ptr, ref, mods = "", 0, False, 0
        while rest[:1] in ("P", "R", "C"):
            if rest[0] == "P":
                ptr += 1
            elif rest[0] == "R":
                ref = True
            else:
                pre = "const "
            rest, mods = rest[1:], mods + 1
        # `v` with no modifier in front of it is the NO-PARAMETER
        # marker; `v` behind a P is the type void. Only the count of
        # modifiers consumed tells them apart.
        if rest == "v" and not mods:
            return out
        uns = ""
        if rest[:1] == "U" and rest[1:2] in BUILTIN:
            uns, rest = "unsigned ", rest[1:]
        if rest[:1] in BUILTIN:
            base, rest = uns + BUILTIN[rest[0]], rest[1:]
            out.append({"base": pre + base, "raw": base, "ptr": ptr,
                        "ref": ref,
                        "text": pre + base + ("*" * ptr) + ("&" if ref else "")})
            continue
        if uns:
            return None
        if rest[:1] == "Q":
            if not rest[1:2].isdigit():
                return None
            count, k, parts = int(rest[1]), 2, []
            for _ in range(count):
                nm, k = _take_len_name(rest, k)
                if nm is None:
                    return None
                parts.append(nm)
            base, rest = "::".join(parts), rest[k:]
        elif rest[:1].isdigit():
            base, k = _take_len_name(rest, 0)
            if base is None:
                return None
            rest = rest[k:]
        else:
            return None
        out.append({"base": pre + base, "raw": base, "ptr": ptr,
                    "ref": ref,
                    "text": pre + base + ("*" * ptr) + ("&" if ref else "")})
    return out


def describe(env, target, donor, trows, drows):
    """-> the facts the sext_create template needs, or (None, why).

    THE INIT IS NOT ALWAYS A MEMBER, and the instructions cannot say so.
    `entity->Init(asset)` and `zEnvInit(entity, asset)` both compile to
    `mr r3,r31; mr r4,r30; bl X` -- the entity goes in r3 either way,
    as `this` or as the first argument -- so the two live in ONE
    twin_census cluster and are one skeleton. Five of this cluster's
    eight solved members turned out to be the free form, and a first
    version of this function called all eight members and would have
    emitted source that compiles and is wrong for five of them. The
    mangled name is what tells them apart: a member carries `__<class>F`
    and a free function carries `__F`.
    """
    d = {"asset": class_of(target), "size": None, "entity": None,
         "base": None, "init": None, "vt": None, "kind": None,
         "initparams": None}
    if d["asset"] is None:
        return None, "the target's own name did not demangle to a class"

    sizes, initsym = [], None
    for row in trows:
        sym = resolved(row)
        if sym is None:
            m = re.match(r"li      r[35],(-?[0-9]+)$", row[3].text.strip())
            if m:
                sizes.append(int(m.group(1)))
            continue
        if sym.startswith("__ct__"):
            d["base"] = class_of(sym)
        elif sym.startswith("__vt__"):
            d["vt"] = sym
            d["entity"] = class_of(sym)
        elif (sym != "memset" and "Alloc" not in sym
              and not LABEL.match(sym)):
            initsym = sym

    if initsym:
        d["init"] = initsym.split("__", 1)[0]
        owner = class_of(initsym)
        d["initsym"] = initsym
        d["initparams"] = params_of(initsym)
        if owner is not None and owner == d["entity"]:
            d["kind"] = "member"
            want = 1
        elif owner is None and "__F" in initsym:
            d["kind"] = "free"
            want = 2
        else:
            want = None
        if want is not None:
            if not d["initparams"] or len(d["initparams"]) != want:
                return None, ("the init %s did not decode to %d "
                              "parameter(s): %r"
                              % (initsym, want, d["initparams"]))
        if want is None:
            return None, ("the init %s belongs to %r, not to the entity %r"
                          % (initsym, owner, d["entity"]))

    # The allocation size is spelled twice -- once for AllocGlobalHeap and
    # once for memset -- and they must agree. If they do not, this is not
    # the shape the template describes and emitting anyway would produce a
    # confident wrong answer.
    big = [s for s in sizes if s not in (0, 16)]
    if len(big) != 2 or big[0] != big[1]:
        return None, ("expected one allocation size spelled twice, read %r"
                      % (sizes,))
    d["size"] = big[0]

    for k in ("entity", "base", "init", "kind"):
        if not d[k]:
            return None, "could not read the %s from the target's calls" % k
    return d, None


SEXT_CREATE_INIT = NL.join([
    "// GENERATED by tools/transplant.py from a matched twin.",
    "// Do not hand-edit: re-emit, or take the file over and "
    "remove this",
    "// banner -- written_vs_generated.py splits on it.",
    "//",
    "// %(entity)s, transplanted from",
    "// %(donor)s,",
    "// which is matched. The shapes are identical instruction for",
    "// instruction; the holes were %(nholes)d, and every one of them is",
    "// read from the retail image rather than guessed:",
    "//",
    "%(holes)s",
    "//",
    "// The DWARF places this function in %(file)s.",
    "",
    "enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };",
    "",
    "namespace Memory {",
    "enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };",
    "",
    "void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap,"
    " eMemMgrTag tag,",
    "                      bool clear);",
    "}  // namespace Memory",
    "",
    "extern " + chr(34) + "C" + chr(34) + " {",
    "void* memset(void* dst, int c, unsigned long n);",
    "}",
    "",
    "inline void* operator new(unsigned long, void* p) { return p; }",
    "",
    "namespace World {",
    "class EntityHandleBase;",
    "}",
    "",
    "class %(entity)s;",
    "%(fwddecl)s",
    "",
    "// 0x38, and polymorphic: the vtable pointer the constructor stores",
    "// sits at +0, so nothing precedes it.",
    "class xBase {",
    "public:",
    "    virtual void _v0();",
    "",
    "    unsigned char _pad0[0x34 - 0x4];",
    "};",
    "",
    "namespace World {",
    "",
    "class xOGEntity : public xBase {",
    "public:",
    "    xOGEntity(EntityHandleBase* handle);",
    "",
    "    unsigned char _pad0[0x40 - 0x34];",
    "};",
    "",
    "}  // namespace World",
    "",
    "namespace %(ns)s {",
    "",
    "class %(assetleaf)s {",
    "public:",
    "    static ::%(entity)s* Create(World::EntityHandleBase* handle,",
    "%(decindent)s%(assetleaf)s* asset);",
    "};",
    "",
    "}  // namespace %(ns)s",
    "",
    "class %(entity)s : public %(base)s {",
    "public:",
    "    %(entity)s(World::EntityHandleBase* handle) : %(base)s(handle) {}",
    "",
    "    virtual void _v0();",
    "%(nesteddecl)s",
    "%(initdecl)s",
    "%(pad)s",
    "};",
    "%(freedecl)s",
    "%(entity)s* %(asset)s::Create(World::EntityHandleBase* handle,",
    "%(indent)s%(assetleaf)s* asset) {",
    "    %(qent)s* entity = new (memset(",
    "        Memory::AllocGlobalHeap(sizeof(%(qent)s),"
    " (Memory::GlobalHeapEnum)0,",
    "                                (eMemMgrTag)16, false),",
    "        0, sizeof(%(qent)s))) %(qent)s(handle);",
    "",
    "%(initcall)s",
    "",
    "    return entity;",
    "}",
    "",
])

SEXT_CREATE_CTOR = NL.join([
    "// GENERATED by tools/transplant.py from a matched twin.",
    "// Do not hand-edit: re-emit, or take the file over and "
    "remove this",
    "// banner -- written_vs_generated.py splits on it.",
    "//",
    "// %(entity)s, transplanted from",
    "// %(donor)s,",
    "// which is matched. The shapes are identical instruction for",
    "// instruction; the holes were %(nholes)d, and every one of them is",
    "// read from the retail image rather than guessed:",
    "//",
    "%(holes)s",
    "//",
    "// The entity constructor is a CALL, so it is declared and never",
    "// defined and no vtable is stored here. The class is padded to",
    "// the size the allocation asks for, which is the only thing",
    "// about its layout this file knows.",
    "//",
    "// The DWARF places this function in %(file)s.",
    "",
    "enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };",
    "",
    "namespace Memory {",
    "enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };",
    "",
    "void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap,"
    " eMemMgrTag tag,",
    "                      bool clear);",
    "}  // namespace Memory",
    "",
    "extern " + chr(34) + "C" + chr(34) + " {",
    "void* memset(void* dst, int c, unsigned long n);",
    "}",
    "",
    "inline void* operator new(unsigned long, void* p) { return p; }",
    "",
    "namespace World { class EntityHandleBase; }",
    "%(fwddecl)s",
    "",
    "class %(entity)s {",
    "public:",
    "    %(entity)s(World::EntityHandleBase* handle,",
    "%(ctorindent)s%(p1)s asset);",
    "",
    "    unsigned char _pad0[0x%(sizehex)X];",
    "};",
    "",
    "namespace %(ns)s {",
    "",
    "class %(assetleaf)s {",
    "public:",
    "    static ::%(entity)s* Create(World::EntityHandleBase* handle,",
    "%(decindent)s%(assetleaf)s* asset);",
    "};",
    "",
    "}  // namespace %(ns)s",
    "",
    "%(entity)s* %(asset)s::Create(World::EntityHandleBase* handle,",
    "%(indent)s%(assetleaf)s* asset) {",
    "    return new (memset(Memory::AllocGlobalHeap(",
    "                           sizeof(%(qent)s), (Memory::GlobalHeapEnum)0,",
    "                           (eMemMgrTag)16, false),",
    "                       0, sizeof(%(qent)s)))"
    " %(qent)s(handle, %(arg)s);",
    "}",
    "",
])


def render_ctor(env, target, donor, tmpl, trows, holes, where):
    """The 96-byte shape: one constructor call, no vtable, no init."""
    asset = class_of(target)
    if asset is None or "::" not in asset:
        return None, "the target name did not demangle to a qualified " "asset class"

    sizes, ctor = [], None
    for row in trows:
        sym = resolved(row)
        if sym is None:
            m = re.match(r"li      r[35],(-?[0-9]+)$", row[3].text.strip())
            if m:
                sizes.append(int(m.group(1)))
        elif sym.startswith("__ct__"):
            ctor = sym
        elif sym.startswith("__vt__"):
            return None, ("this one stores a vtable (%s), which the "
                          "constructor-call shape does not" % sym)
        elif (sym != "memset" and "Alloc" not in sym
              and not LABEL.match(sym)):
            return None, ("it also calls %s, so it is not the bare "
                          "constructor shape" % sym)
    if ctor is None:
        return None, "no constructor call in the target"

    big = [x for x in sizes if x not in (0, 16)]
    if len(big) != 2 or big[0] != big[1]:
        return None, ("expected one allocation size spelled twice, "
                      "read %r" % (sizes,))
    size = big[0]

    entity = class_of(ctor)
    if entity is None or "::" in entity:
        return None, ("the entity %r is nested or did not demangle; a "
                      "nested type cannot be defined by a qualified "
                      "name" % entity)
    ps = params_of(ctor)
    if not ps or len(ps) != 2:
        return None, ("the constructor %s did not decode to two "
                      "parameters" % ctor)
    p1 = ps[1]

    # The asset reaches the constructor unchanged when the types agree
    # or when it takes void*; otherwise it is a different class and
    # the cast has to name it from the GLOBAL scope, because inside
    # Sext::<Asset>::Create an unqualified name finds the enclosing
    # class first.
    ns, _, leaf = asset.rpartition("::")

    # The entity class comes BEFORE the Sext namespace and its
    # constructor names the asset, so the asset is forward-declared
    # ahead of it. Only the uncommon case -- a constructor taking
    # some other class -- was being declared, so the common one did
    # not compile.
    fwd = [NL, "namespace %s { class %s; }" % (ns, leaf)]
    if p1["raw"] in (asset, "void"):
        arg = "*asset" if p1["ref"] else "asset"
    else:
        t = p1["base"]
        if "::" not in t:
            fwd.append("class %s;" % p1["raw"])
            # The qualifier comes first and the scope belongs on the
            # NAME: "::const X" is not a type, "const ::X" is.
            pre = "const " if t.startswith("const ") else ""
            t = pre + "::" + t[len(pre):]
        else:
            h, _, lf = p1["raw"].rpartition("::")
            fwd.append("namespace %s { class %s; }" % (h, lf))
        if p1["ref"]:
            arg = "*(%s*)asset" % t
        else:
            arg = "(%s%s)asset" % (t, "*" * p1["ptr"])
    fwd = NL + NL.join(fwd)
    hole_text = NL.join(
        "//   +0x%03X  %s" % (i * 4, (t[3].text + t[4]).strip())
        for i, _dr, t in holes)
    body = tmpl % {
        "entity": entity, "qent": "::" + entity,
        "asset": asset, "assetleaf": leaf, "ns": ns,
        "p1": p1["text"], "arg": arg, "fwddecl": fwd,
        "sizehex": size, "donor": donor, "nholes": len(holes),
        "holes": hole_text, "file": where,
        "ctorindent": " " * (len(entity) + 9),
        "decindent": " " * (len(entity) + 22),
        "indent": " " * (len(entity) + len(asset) + 11),
    }
    return body, {"entity": entity, "asset": asset, "size": size,
                  "base": "(none -- the constructor is a call)",
                  "init": ctor.split("__")[0] or "__ct__",
                  "kind": "constructor", "vt": None,
                  "template": "sext_create_ctor"}

TEMPLATES = {
    "Create__Q24Sext11xGroupAssetFPQ25World16EntityHandleBase"
    "PQ24Sext11xGroupAsset": ("sext_create_init", SEXT_CREATE_INIT),
    "Create__Q24Sext11xTimerAssetFPQ25World16EntityHandleBase"
    "PQ24Sext11xTimerAsset": ("sext_create_init", SEXT_CREATE_INIT),
    # The 96-byte shape: 18 solved members, any of which is a donor.
    "Create__Q24Sext10FXInstanceFPQ25World16EntityHandleBase"
    "PQ24Sext10FXInstance": ("sext_create_ctor", SEXT_CREATE_CTOR),
    "Create__Q24Sext13zSpinnerAssetFPQ25World16EntityHandleBase"
    "PQ24Sext13zSpinnerAsset": ("sext_create_ctor", SEXT_CREATE_CTOR),
    "Create__Q24Sext15zPlantTrapAssetFPQ25World16EntityHandleBase"
    "PQ24Sext15zPlantTrapAsset": ("sext_create_ctor", SEXT_CREATE_CTOR),
}


def render(env, target, donor, holes):
    """-> (source text, description) or (None, why it will not fit)."""
    if donor not in TEMPLATES:
        return None, ("no template for donor %s; %d donor(s) have one: %s"
                      % (donor, len(TEMPLATES), ", ".join(sorted(TEMPLATES))))

    tname, tmpl = TEMPLATES[donor]
    trows, _tu = rows_for(env, target)
    drows, _du = rows_for(env, donor)

    import subprocess
    r = subprocess.run([sys.executable, str(ROOT / "tools/dwarf_lines.py"),
                        "0x%08X" % env["addr_of"][target]], cwd=str(ROOT),
                       capture_output=True, text=True)
    m = re.search("declared in " + chr(92) + "S+", r.stdout)
    where = m.group(0)[12:] if m else "no file the line table names"

    if tname == "sext_create_ctor":
        return render_ctor(env, target, donor, tmpl, trows, holes, where)

    d, why = describe(env, target, donor, trows, drows)
    if d is None:
        return None, why

    # The template carries the DONOR'S base scaffolding verbatim -- xBase
    # at 0x38 and World::xOGEntity at 0x40 -- because that is what has
    # matched. Three of this cluster's eight solved members derive from
    # zUI instead and one from zEnt, and emitting the xOGEntity layout
    # for one of those would place the vtable and every offset wrong
    # while compiling cleanly. A different base needs its own template.
    if d["base"] != "World::xOGEntity":
        return None, ("the entity derives from %s and this template only "
                      "carries the World::xOGEntity layout" % d["base"])

    # A nested type cannot be DEFINED by its qualified name -- `class
    # FX::Ribbon::zRibbon : public ...` is not C++ -- and the fix is to
    # wrap the definition in namespaces, which this template does not do.
    if "::" in d["entity"]:
        return None, ("the entity %s is nested, and a nested type cannot be "
                      "defined by a qualified name" % d["entity"])

    ns, _, leaf = d["asset"].rpartition("::")
    if not ns:
        return None, "%s is not a qualified asset name" % d["asset"]

    hole_text = NL.join(
        "//   +0x%03X  %s" % (i * 4, (t[3].text + t[4]).strip())
        for i, _dr, t in holes)

    # DECLARE WHAT RETAIL DECLARES. The template used to spell every
    # init as taking the Sext asset by pointer; five of eleven emitted
    # units then named a symbol that is nowhere in the image, because
    # retail takes a const reference, or a nested asset_type, or an
    # asset class that is not in namespace Sext at all. The bytes
    # matched anyway -- a pointer and a reference are both an address
    # in r4 -- so nothing but reloc_audit.py would ever have said so.
    # The true signature is not guessed: the bl resolves through the
    # retail symbol table, so its mangled parameter list is in hand.
    ent, asset = d["entity"], d["asset"]
    known = set(["xBase", "World::xOGEntity", "World::EntityHandleBase",
                 ent, asset])

    def at_class(p):
        """The type as written INSIDE the entity class, where a type
        nested in that class is reachable by its leaf name."""
        if p["raw"].startswith(ent + "::"):
            return p["text"].replace(ent + "::", "")
        return p["text"]

    def at_create(p):
        """The type as written inside Sext::<Asset>::Create, where an
        unqualified name finds the ENCLOSING asset class first and so
        has to be spelled from the global scope."""
        b = p["base"]
        pre = "const " if b.startswith("const ") else ""
        bare = b[len(pre):]
        return pre + (bare if "::" in bare else "::" + bare)

    def arg(p, expr):
        t = at_create(p)
        if p["ref"]:
            return "*(%s*)%s" % (t, expr)
        return "(%s%s)%s" % (t, "*" * p["ptr"], expr)

    fwd, nested = [], []
    for p in d["initparams"]:
        raw = p["raw"]
        if raw in known:
            continue
        known.add(raw)
        head, _, leafname = raw.rpartition("::")
        if head == ent:
            nested.append("    class %s;" % leafname)
        elif not head:
            fwd.append("class %s;" % raw)
        else:
            fwd.append("namespace %s { class %s; }" % (head, leafname))

    fwddecl = (NL + NL + NL.join(fwd)) if fwd else ""
    nesteddecl = (NL + NL.join(nested)) if nested else ""

    if d["kind"] == "member":
        p = d["initparams"][0]
        initdecl = NL.join(["", "    void %s(%s asset);"
                            % (d["init"], at_class(p)), ""])
        freedecl = ""
        initcall = "    entity->%s(%s);" % (d["init"], arg(p, "asset"))
    else:
        p0, p1 = d["initparams"]
        freedecl = NL.join(["", "void %s(%s base, %s asset);"
                            % (d["init"], p0["text"], p1["text"]), ""])
        initdecl = ""
        initcall = "    %s(%s, %s);" % (d["init"], arg(p0, "entity"),
                                        arg(p1, "asset"))
    body = tmpl % {
        "entity": d["entity"], "qent": "::" + d["entity"],
        "asset": d["asset"], "assetleaf": leaf,
        "ns": ns, "base": d["base"], "init": d["init"],
        "initdecl": initdecl, "freedecl": freedecl, "initcall": initcall,
        "fwddecl": fwddecl, "nesteddecl": nesteddecl,
        "pad": ("    unsigned char _pad0[0x%X - 0x40];" % d["size"]
                if d["size"] > 0x40 else ""),
        "size": d["size"], "sizehex": d["size"], "donor": donor,
        "nholes": len(holes), "holes": hole_text, "file": where,
        "indent": " " * (len(d["entity"]) + len(d["asset"]) + 11),
        "decindent": " " * (len(d["entity"]) + 22),
    }
    d["template"] = tname
    return body, d


def emit(env, target, donor, holes, out=None):
    body, d = render(env, target, donor, holes)
    if body is None:
        print("  TEMPLATE DOES NOT FIT: %s." % d)
        print("  Refusing to emit -- a template forced onto a shape it does")
        print("  not describe produces source that compiles and is wrong.")
        print("  The fill sheet above is still complete.")
        return 1

    print("  TEMPLATE %s FITS. %d hole(s) filled: %s = %d bytes, base %s,"
          % (d["template"], len(holes), d["entity"], d["size"], d["base"]))
    print("  %s init %s, vtable %s." % (d["kind"], d["init"], d["vt"]))
    print("")
    if out:
        p = Path(out)
        if p.exists():
            print("  %s ALREADY EXISTS -- refusing to overwrite it." % out)
            print("  Emitting to stdout instead; merge it by hand.")
        else:
            p.parent.mkdir(parents=True, exist_ok=True)
            p.write_text(body, encoding="utf-8")
            print("  wrote %d bytes to %s" % (len(body), out))
            print("  Re-run `python configure.py` -- a new source file is not "
                  "in the build until you do.")
            return 0
    print("  ---- 8< ---- emitted source, NOT yet compiled ---- 8< ----")
    print("")
    print(body)
    print("  ---- 8< ---- run unitcmp.py and a full ninja ---- 8< ----")
    return 0


def survey(env, limit):
    mixed = []
    for members in env["cl"].values():
        solved = [m for m in members if m[2]]
        todo = [m for m in members if not m[2]]
        if solved and todo:
            mixed.append((sum(m[1] for m in todo), solved, todo))
    mixed.sort(key=lambda x: -x[0])
    print("  %d mixed cluster(s): %d unsolved member(s), %d bytes, of "
          "%d game function(s) clustered"
          % (len(mixed), sum(len(m[2]) for m in mixed),
             sum(m[0] for m in mixed), env["read"]))
    print("")
    print("  %-6s %-7s %-9s  %s" % ("bytes", "each", "solved/todo",
                                    "a donor"))
    for nbytes, solved, todo in mixed[:limit]:
        print("  %-6d %-7d %-9s  %s"
              % (nbytes, todo[0][1], "%d/%d" % (len(solved), len(todo)),
                 sorted(solved)[0][0][:52]))
    return 0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("target", nargs="?", help="the unsolved function")
    ap.add_argument("--from", dest="donor",
                    help="which solved twin to read the shape from")
    ap.add_argument("--cluster", help="list the cluster holding this name")
    ap.add_argument("--survey", action="store_true",
                    help="every cluster with both a solved and an unsolved")
    ap.add_argument("--limit", type=int, default=25)
    ap.add_argument("--no-dwarf", action="store_true",
                    help="skip the line-table and locals block")
    ap.add_argument("--emit", action="store_true",
                    help="also write source, if a template fits the donor")
    ap.add_argument("--out", help="write the emitted source to this path")
    args = ap.parse_args()

    if not args.target and not args.cluster and not args.survey:
        sys.exit(__doc__)

    env = load_all()
    print("")
    print("  %d game function(s), %d matched; %d clustered, %d unreadable"
          % (len(env["game"]), len(env["matched"]), env["read"],
             env["unread"]))
    print("")

    if args.survey:
        return survey(env, args.limit)

    name = args.target or args.cluster
    if name not in env["addr_of"]:
        hits = [n for n in env["addr_of"] if name in n]
        if len(hits) == 1:
            name = hits[0]
            print("  resolved to %s" % name)
            print("")
        elif not hits:
            sys.exit("transplant: no game function is called %r" % name)
        else:
            sys.exit("transplant: %d function names contain %r -- be exact:%s"
                     % (len(hits), name,
                        NL + NL.join("    " + h for h in sorted(hits)[:20])))

    solved, todo = cluster_of(env, name)
    if solved is None:
        sys.exit("transplant: %s is in no cluster (unreadable, or not a "
                 "game function)" % name)

    if args.cluster:
        print("  CLUSTER   %d byte(s) each, %d solved, %d unsolved"
              % ((solved or todo)[0][1], len(solved), len(todo)))
        print("")
        for nm, _sz, _m in solved:
            print("    written  %-58s  %s"
                  % (nm[:58], env["source"].get(nm)))
        for nm, _sz, _m in todo:
            print("    todo     %-58s  %s"
                  % (nm[:58], env["source"].get(nm) or "(no source file)"))
        print("")
        return 0

    if not solved:
        sys.exit("transplant: %s has no solved twin -- nothing to transplant "
                 "from" % name)

    donor = args.donor
    if donor and donor not in env["addr_of"]:
        hits = [m[0] for m in solved if donor in m[0]]
        if len(hits) != 1:
            sys.exit("transplant: --from %r matches %d solved twin(s)"
                     % (donor, len(hits)))
        donor = hits[0]
    if donor is None:
        donor = solved[0][0]

    if donor not in [m[0] for m in solved]:
        sys.exit("transplant: %s is not a solved member of this cluster"
                 % donor)

    holes = fill_sheet(env, name, donor)

    if args.emit:
        rc = emit(env, name, donor, holes, args.out)
        print("")
        if rc:
            return rc

    if not args.no_dwarf:
        print("  WHAT THE COMPILER ITSELF RECORDED (DWARF 2, from the "
              "retail link):")
        for ln in dwarf_note(env["addr_of"][name]):
            print(ln)
        print("")

    return 0


if __name__ == "__main__":
    sys.exit(main())
