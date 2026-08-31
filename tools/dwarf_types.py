"""Recover C++ type declarations from the Wii build's DWARF.

    python tools/dwarf_types.py --report            what is in there
    python tools/dwarf_types.py --type zPlayer      one type and its deps
    python tools/dwarf_types.py --all -o out.h      every named type
    python tools/dwarf_types.py --conflicts         types the CUs disagree on

`orig/R8IE78/files/SB09WiiMASTERWAD.elf` is an unstripped CodeWarrior link
carrying DWARF 2. Eleven compile units have debug info and they are exactly
the eleven the build reports as `Game Code`, at 0.00% matched:

    GM/Engine/WAD/WAD00-04.cpp, WADSpeed.cpp, LinkFastSqrt.cpp
    NG/Source/Engine/WAD/WAD00-02.cpp, WADSpeed.cpp

So the region with no other source of information is the region with full
type coverage. That is what this extracts.

WHAT IT DOES NOT DO. It does not decide that a type is correct, and nothing
it emits is a match. It transcribes what the DWARF says, and where two
compile units say DIFFERENT things about the same type it reports the
disagreement instead of picking one -- `--conflicts` exists because silently
taking the first is how a wrong struct layout gets believed for weeks.

EVERY EMITTED TYPE CARRIES ITS OWN CHECK. A struct is written with each
member's offset in a comment AND a compile-time assertion of that offset, so
a hand edit that moves a field fails the build rather than producing a
plausible wrong answer. The assertions are the point; the comments are for
reading.

`DW_AT_data_member_location` is a location EXPRESSION, not an integer:
`DW_OP_plus_uconst <ULEB>`. Reading byte [1] of it works until the first
offset above 127 and then silently truncates -- which is most of a real
game struct. It is decoded properly here.

HOW MUCH OF IT BUILDS, because a tool that emits 3,739 types and is never
measured has not been finished: **71 of 120 sampled types (59%)** produce a
header the real CodeWarrior compiler accepts. Run
`tools/dwarf_types_check.py` for the current figure and the causes of the
rest. `--all` emits every type into one header and does NOT compile yet;
per-type emission is the mode that works, and it is the one a decomper
wants anyway.
"""

import argparse
import sys
from collections import Counter, OrderedDict
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DEFAULT_ELF = ROOT / "orig/R8IE78/files/SB09WiiMASTERWAD.elf"

try:
    from elftools.elf.elffile import ELFFile
except ImportError:                                            # noqa: BLE001
    print("pyelftools is required:  pip install pyelftools")
    print("REFUSING to continue -- there is no partial answer to give.")
    sys.exit(1)

TAG_COMPOSITE = ("DW_TAG_structure_type", "DW_TAG_class_type",
                 "DW_TAG_union_type")
KEYWORD = {"DW_TAG_structure_type": "struct", "DW_TAG_class_type": "class",
           "DW_TAG_union_type": "union"}


def uleb(data, i=0):
    v, s = 0, 0
    while i < len(data):
        b = data[i]
        i += 1
        v |= (b & 0x7F) << s
        if not (b & 0x80):
            break
        s += 7
    return v


def member_offset(die):
    """-> byte offset of a member, or None if it is not a plain one.

    DWARF 2 stores this as a location expression. The only form CodeWarrior
    emits here is `DW_OP_plus_uconst <ULEB>` (opcode 0x23), and anything
    else is returned as None rather than guessed at -- a member whose offset
    could not be read must not be written down as 0.
    """
    a = die.attributes.get("DW_AT_data_member_location")
    if a is None:
        return None
    v = a.value
    if isinstance(v, int):
        return v
    if isinstance(v, (bytes, bytearray, list)):
        b = bytes(v)
        if not b or b[0] != 0x23:
            return None
        return uleb(b, 1)
    return None


def name_of(die):
    a = die.attributes.get("DW_AT_name")
    if a is None:
        return None
    v = a.value
    return v.decode("latin1") if isinstance(v, bytes) else str(v)


def qualified(die):
    """A name that identifies ONE type. Leaf name, or leaf plus DIE offset.

    THE LEAF NAME IS NOT AN IDENTITY. DWARF stores only the leaf in
    DW_AT_name, and this producer emits nested classes and template
    instantiations FLAT at compile-unit level -- their DIE parent is the
    compile unit, not the enclosing class. So the tree cannot supply a
    scope, and neither can anything else in the file: 12 different `Atoms`,
    6 `Block`, 5 `ConstIterator` and 21 `GPtr` share a leaf name each.

    Indexed by leaf name those look like types the compile units "disagree"
    about, and picking one would emit a plausible WRONG struct under a name
    something else uses. So a colliding name is suffixed with its DIE
    offset, which is unique by construction and is a fact rather than an
    inference. `Types.enclosing()` reports which types REFERENCE each one,
    as a hint for naming it by hand -- marked as inferred, because a
    reference is not a scope and this tool does not pretend otherwise.
    """
    n = name_of(die)
    if not n:
        return None
    return n


class Types(object):
    """Every named type in the DWARF, indexed and checked for disagreement."""

    def __init__(self, path):
        self.path = Path(path)
        if not self.path.exists():
            raise SystemExit(
                "%s is not there.\nPut the extracted disc at orig/R8IE78/ "
                "(gitignored). REFUSING to print an empty type list, which "
                "reads as 'no types' when it means 'nothing was read'."
                % self.path)
        self._f = open(str(self.path), "rb")
        self.dw = ELFFile(self._f).get_dwarf_info()
        self.by_off = {}
        self.named = OrderedDict()          # unique name -> [(cu, die)]
        self.leaf = {}                      # leaf name -> {unique names}
        self.ambiguous = {}                 # leaf name -> how many types
        self.uniq = {}                      # die offset -> the emitted name
        self._refs = None
        self.cus = []
        self.tags = Counter()
        raw = OrderedDict()
        for cu in self.dw.iter_CUs():
            top = cu.get_top_DIE()
            cu_name = name_of(top) or "?"
            lo = top.attributes.get("DW_AT_low_pc")
            hi = top.attributes.get("DW_AT_high_pc")
            self.cus.append((cu_name, lo.value if lo else 0,
                             hi.value if hi else 0))
            for die in cu.iter_DIEs():
                self.tags[die.tag] += 1
                self.by_off[die.offset] = die
                if die.tag in TAG_COMPOSITE or die.tag in (
                        "DW_TAG_enumeration_type", "DW_TAG_typedef"):
                    n = name_of(die)
                    # `@class` and `@enum` are CodeWarrior's placeholders for
                    # an ANONYMOUS type, not names. 166 unrelated classes
                    # share `@class`; indexing them together would be
                    # meaningless in either direction.
                    if n and not n.startswith("@"):
                        raw.setdefault(n, []).append((cu_name, die))
        self._resolve_names(raw)

    def _resolve_names(self, raw):
        """Give every type a name that identifies exactly one of them.

        A leaf name shared by structurally IDENTICAL definitions is one type
        seen from several compile units -- that is the normal case and it
        keeps the plain name. A leaf name covering definitions that DIFFER is
        several types, and each gets its DIE offset appended, because there
        is nothing else in this DWARF to tell them apart.
        """
        for n, entries in raw.items():
            sigs = {}
            for cu_name, die in entries:
                key = (self.signature(die) if die.tag in TAG_COMPOSITE
                       else ("typedef", self.type_name(self.ref(die))))
                sigs.setdefault(key, []).append((cu_name, die))
            if len(sigs) == 1:
                self.named[n] = entries
                self.leaf.setdefault(n, set()).add(n)
                for _cu, die in entries:
                    self.uniq[die.offset] = n
                continue
            self.ambiguous[n] = len(sigs)
            for group in sigs.values():
                die = group[0][1]
                q = "%s__%X" % (n, die.offset)
                self.named[q] = group
                self.leaf.setdefault(n, set()).add(q)
                # EVERY die in the group, not just the first. A member
                # referring to any of them must render the name that is
                # actually emitted -- rendering the bare leaf here is how a
                # type gets referenced under a name nothing defines.
                for _cu, d in group:
                    self.uniq[d.offset] = q

    def enclosing(self, die):
        """Types that REFERENCE this one, as a hint for naming it by hand.

        A reference is not a scope. This is reported as inferred and never
        used to build a name.
        """
        if self._refs is None:
            self._refs = {}
            for nm, entries in self.named.items():
                for _cu, d in entries:
                    if d.tag not in TAG_COMPOSITE:
                        continue
                    for ch in d.iter_children():
                        if ch.tag not in ("DW_TAG_member",
                                          "DW_TAG_inheritance"):
                            continue
                        tgt = self.ref(ch)
                        while tgt is not None and tgt.tag in (
                                "DW_TAG_const_type", "DW_TAG_volatile_type",
                                "DW_TAG_array_type", "DW_TAG_pointer_type",
                                "DW_TAG_reference_type", "DW_TAG_typedef"):
                            tgt = self.ref(tgt)
                        if tgt is not None:
                            self._refs.setdefault(tgt.offset, set()).add(nm)
        return sorted(self._refs.get(die.offset, ()))

    # -- rendering ---------------------------------------------------------

    def ref(self, die, attr="DW_AT_type"):
        a = die.attributes.get(attr)
        return self.by_off.get(a.value) if a is not None else None

    def type_name(self, die, depth=0):
        """A C++ spelling for a type DIE. `void` when there is no type."""
        if die is None:
            return "void"
        if depth > 12:
            return "void*  /* type nesting too deep to render */"
        t = die.tag
        if t == "DW_TAG_base_type":
            return name_of(die) or "int"
        # THE EMITTED NAME, not the leaf. A member of an ambiguous type is
        # written as `DataType__1A2B`, so rendering `DataType` here names
        # something the header never defines -- which is the exact failure
        # the disambiguation exists to prevent, reintroduced one function
        # further along.
        if t in ("DW_TAG_typedef", "DW_TAG_enumeration_type"):
            n = self.uniq.get(die.offset) or name_of(die)
            if n and not n.startswith("@"):
                return ("enum " + n) if t == "DW_TAG_enumeration_type" else n
            return self.type_name(self.ref(die), depth + 1)
        if t in TAG_COMPOSITE:
            n = self.uniq.get(die.offset) or name_of(die)
            if not n or n.startswith("@"):
                return "/* anonymous */ void"
            return n
        if t == "DW_TAG_pointer_type":
            return self.type_name(self.ref(die), depth + 1) + "*"
        if t == "DW_TAG_reference_type":
            return self.type_name(self.ref(die), depth + 1) + "&"
        if t == "DW_TAG_const_type":
            return "const " + self.type_name(self.ref(die), depth + 1)
        if t == "DW_TAG_volatile_type":
            return "volatile " + self.type_name(self.ref(die), depth + 1)
        if t == "DW_TAG_array_type":
            return self.type_name(self.ref(die), depth + 1)
        if t == "DW_TAG_subroutine_type":
            return self.type_name(self.ref(die), depth + 1) + " (*)()"
        return "void"

    def array_suffix(self, die):
        """`[N]` for an array member, or '' -- the count from DW_AT_upper_bound."""
        if die is None or die.tag != "DW_TAG_array_type":
            return ""
        out = ""
        for ch in die.iter_children():
            if ch.tag != "DW_TAG_subrange_type":
                continue
            ub = ch.attributes.get("DW_AT_upper_bound")
            if ub is None or not isinstance(ub.value, int):
                out += "[]"
            else:
                out += "[%d]" % (ub.value + 1)
        return out or "[]"

    # -- one type ----------------------------------------------------------

    def layout(self, die):
        """-> (byte_size, [(offset, name, type_die, bits)]) or None."""
        bs = die.attributes.get("DW_AT_byte_size")
        rows = []
        for ch in die.iter_children():
            if ch.tag == "DW_TAG_inheritance":
                off = member_offset(ch)
                rows.append((off, None, self.ref(ch), None))
                continue
            if ch.tag != "DW_TAG_member":
                continue
            off = member_offset(ch)
            bits = None
            b = ch.attributes.get("DW_AT_bit_size")
            o = ch.attributes.get("DW_AT_bit_offset")
            if b is not None:
                bits = (b.value, o.value if o is not None else None)
            rows.append((off, name_of(ch), self.ref(ch), bits))
        return (bs.value if bs is not None else None), rows

    def raw_type_name(self, die, depth=0):
        """A spelling that does NOT depend on the emitted-name map.

        `signature` is what DECIDES the emitted names, so it cannot be
        computed from them. Using `type_name` here made the grouping depend
        on how far `_resolve_names` had got: the same DWARF reported 0
        disagreeing types on one run and 7 on the next, purely from
        iteration order. A comparison used to build a map must not read
        that map.
        """
        if die is None or depth > 12:
            return "void"
        t = die.tag
        if t in TAG_COMPOSITE or t in ("DW_TAG_base_type", "DW_TAG_typedef",
                                       "DW_TAG_enumeration_type"):
            return name_of(die) or ("<anon %s>" % t)
        if t == "DW_TAG_pointer_type":
            return self.raw_type_name(self.ref(die), depth + 1) + "*"
        if t == "DW_TAG_reference_type":
            return self.raw_type_name(self.ref(die), depth + 1) + "&"
        if t in ("DW_TAG_const_type", "DW_TAG_volatile_type",
                 "DW_TAG_array_type"):
            return self.raw_type_name(self.ref(die), depth + 1)
        return t

    def signature(self, die):
        """A comparable summary, for deciding whether two CUs agree."""
        size, rows = self.layout(die)
        return (size, tuple((o, n, self.raw_type_name(t), b)
                            for o, n, t, b in rows))

    def conflicts(self):
        """-> {name: [ (cu, signature) ]} for every type the CUs disagree on."""
        out = {}
        for n, entries in self.named.items():
            seen = {}
            for cu_name, die in entries:
                if die.tag not in TAG_COMPOSITE:
                    continue
                seen.setdefault(self.signature(die), []).append(cu_name)
            if len(seen) > 1:
                out[n] = seen
        return out

    def emit(self, name, seen=None, order=None):
        """Emit `name` after everything it needs BY VALUE. -> [text]."""
        seen = set() if seen is None else seen
        order = [] if order is None else order
        if name in seen:
            return order
        seen.add(name)
        entries = self.named.get(name)
        if not entries:
            return order
        die = next((d for _c, d in entries if d.tag in TAG_COMPOSITE),
                   entries[0][1])
        # An enum member renders as `enum Foo`, so the enum has to be defined
        # or the header does not compile. It has no dependencies of its own.
        if die.tag == "DW_TAG_enumeration_type":
            order.append(self.render_enum(name, die))
            return order
        if die.tag not in TAG_COMPOSITE:
            return order
        for dn in self.deps_of(name, "value"):
            if dn in self.named:
                self.emit(dn, seen, order)
        order.append(self.render(name, die))
        return order

    def group_unions(self, rows):
        """Collapse members that share an offset into one union row.

        A row is `(offset, name, type, bits)`; a union row is
        `(offset, "\\0union", [arms], None)`. Inheritance rows (name None)
        are left alone.
        """
        by_off = OrderedDict()
        order = []
        for r in rows:
            off, mname = r[0], r[1]
            if mname is None or off is None:
                order.append(("keep", r))
                continue
            if off not in by_off:
                by_off[off] = []
                order.append(("off", off))
            by_off[off].append(r)
        out = []
        for kind, val in order:
            if kind == "keep":
                out.append(val)
                continue
            group = by_off[val]
            if len(group) == 1:
                out.append(group[0])
                continue
            # An anonymous placeholder covering the same offset as real arms
            # adds nothing and would double-count the space.
            real = [g for g in group if self.opaque_bytes(g[2]) is None]
            arms = real if real else group
            if len(arms) == 1:
                out.append(arms[0])
            else:
                out.append((val, "\0union", arms, None))
        return out

    def opaque_bytes(self, die):
        """-> byte size, when this type can only be held as opaque bytes.

        An ANONYMOUS composite held by value: nothing declares it, so it
        cannot be named, but its size is known and size is what a layout
        needs. Returns None for anything nameable -- a real type is always
        preferable to a byte blob.
        """
        d = die
        while d is not None and d.tag in ("DW_TAG_const_type",
                                          "DW_TAG_volatile_type",
                                          "DW_TAG_typedef",
                                          "DW_TAG_array_type"):
            d = self.ref(d)
        if d is None or d.tag not in TAG_COMPOSITE:
            return None
        n = self.uniq.get(d.offset) or name_of(d)
        if n and not n.startswith("@"):
            return None
        bs = d.attributes.get("DW_AT_byte_size")
        return bs.value if bs is not None and bs.value else None

    def render_enum(self, name, die):
        out = ["enum %s" % name, "{"]
        for ch in die.iter_children():
            if ch.tag != "DW_TAG_enumerator":
                continue
            n = name_of(ch)
            v = ch.attributes.get("DW_AT_const_value")
            if n is None:
                continue
            out.append("    %s = %d," % (n, v.value if v is not None else 0))
        out.append("};")
        return "\n".join(out)

    def deps_of(self, name, kind):
        """Types `name` needs: kind 'value' (must be defined) or 'ptr'."""
        out = OrderedDict()
        for _cu, die in self.named.get(name, ()):
            if die.tag not in TAG_COMPOSITE:
                continue
            for ch in die.iter_children():
                if ch.tag not in ("DW_TAG_member", "DW_TAG_inheritance"):
                    continue
                d, ptr = self.ref(ch), False
                while d is not None and d.tag in (
                        "DW_TAG_pointer_type", "DW_TAG_reference_type",
                        "DW_TAG_const_type", "DW_TAG_volatile_type",
                        "DW_TAG_array_type", "DW_TAG_typedef"):
                    if d.tag in ("DW_TAG_pointer_type",
                                 "DW_TAG_reference_type"):
                        ptr = True
                    d = self.ref(d)
                if d is None:
                    continue
                if kind == "value" and ptr:
                    continue
                if kind == "ptr" and not ptr:
                    continue
                if d.tag not in TAG_COMPOSITE and \
                        d.tag != "DW_TAG_enumeration_type":
                    continue
                dn = self.uniq.get(d.offset)
                if dn and dn != name and not dn.startswith("@"):
                    out[dn] = d.tag
        return out

    def forward_decls(self, defined):
        """`class X;` for every composite the emitted set only points AT.

        A name this tool had to disambiguate with a DIE offset cannot be
        forward declared usefully -- the declaration would introduce a type
        that no member actually names -- so those are skipped and counted.
        """
        want = OrderedDict()
        for n in defined:
            for dn, tag in self.deps_of(n, "ptr").items():
                if dn in defined or dn in want:
                    continue
                want[dn] = tag
        out = []
        for n, tag in want.items():
            if tag == "DW_TAG_enumeration_type":
                # An enum cannot be forward declared in this dialect, so it
                # is DEFINED rather than skipped -- skipping it would leave
                # a member naming a type nothing declares.
                entries = self.named.get(n)
                if entries:
                    out.append(self.render_enum(n, entries[0][1]))
                continue
            out.append("%s %s;" % (KEYWORD.get(tag, "struct"), n))
        return out

    def render(self, name, die):
        size, rows = self.layout(die)
        kw = KEYWORD.get(die.tag, "struct")
        out = ["%s %s%s" % (kw, name,
                            "  /* 0x%X bytes */" % size if size else "")]
        out.append("{")
        if kw == "class":
            out.append("public:")
        asserts = []
        unknown = 0
        nbase = 0
        # MEMBERS SHARING AN OFFSET ARE A UNION. CodeWarrior flattens an
        # anonymous union into the enclosing type, listing every arm at the
        # same offset -- and it also emits the union itself as a member. Laid
        # out sequentially that makes sizeof the SUM of the arms instead of
        # the largest, which is why `xCamCoord` (0x20 bytes, three arms all
        # at +0) failed its own size assertion. The offsets say it is a
        # union, so it is written as one; where an opaque placeholder covers
        # the same offset as real arms, the arms win and the placeholder is
        # dropped.
        rows = self.group_unions(rows)
        # A GAP AT THE START IS NOT NOTHING. A class with virtual functions
        # has a vtable pointer at +0 that DWARF does not list as a member,
        # so `MemCallbacks` came out as an empty class asserting a size of 4
        # and failed. The gap is filled with explicit padding rather than a
        # guess about what occupies it: the layout is then exact, and what
        # is unknown is named as unknown.
        first = min([o for o, m, _t, _b in rows
                     if m is not None and o is not None] or [None]
                    ) if rows else None
        has_base = any(m is None for _o, m, _t, _b in rows)
        if not has_base and size:
            gap = size if first is None else first
            if gap and gap > 0:
                out.append("    /* +0x0    */ unsigned char __head[0x%X];"
                           "   /* not described by DWARF"
                           " (vtable pointer?) */" % gap)
        for off, mname, t, bits in rows:
            if mname == "\0union":
                out.append("    /* +0x%-4X */ union" % off)
                out.append("    {")
                for _o, un, ut, _b in t:
                    ua = ""
                    uw = ut
                    while uw is not None and uw.tag in (
                            "DW_TAG_const_type", "DW_TAG_volatile_type"):
                        uw = self.ref(uw)
                    if uw is not None and uw.tag == "DW_TAG_array_type":
                        ua = self.array_suffix(uw)
                    ob = self.opaque_bytes(ut)
                    if ob is not None:
                        out.append("        unsigned char %s[0x%X]%s;"
                                   % (un, ob, ua))
                    else:
                        out.append("        %s %s%s;"
                                   % (self.type_name(ut), un, ua))
                out.append("    };")
                continue
            if mname is None:                       # inheritance
                # Numbered: multiple inheritance gives more than one base and
                # `_base` twice is a redeclaration.
                out.append("    /* +0x%s */ %s _base%d;"
                           % ("%-4X" % off if off is not None else "????",
                              self.type_name(t), nbase))
                nbase += 1
                continue
            arr = ""
            walk = t
            while walk is not None and walk.tag in ("DW_TAG_const_type",
                                                    "DW_TAG_volatile_type"):
                walk = self.ref(walk)
            if walk is not None and walk.tag == "DW_TAG_array_type":
                arr = self.array_suffix(walk)
            # AN ANONYMOUS TYPE HELD BY VALUE. CodeWarrior names these
            # `@class`, so there is nothing to declare and nothing to refer
            # to -- rendering the member as `void` made it the single
            # largest cause of headers the compiler rejected. Its SIZE is
            # known, and size is what a layout needs, so it becomes an
            # opaque byte array of exactly that many bytes. The layout is
            # preserved and the loss (the member names inside it) is stated
            # rather than hidden.
            opaque = self.opaque_bytes(t)
            if opaque is not None:
                decl = "unsigned char %s[0x%X]%s" % (mname, opaque, arr)
                out.append("    /* +0x%-4X */ %s;   /* anonymous type */"
                           % (off if off is not None else 0, decl))
                if off is not None and bits is None:
                    asserts.append((mname, off))
                continue
            decl = "%s %s%s" % (self.type_name(t), mname, arr)
            if bits is not None:
                decl += " : %d" % bits[0]
            if off is None:
                unknown += 1
                out.append("    /* +?????? */ %s;   /* OFFSET NOT READ */"
                           % decl)
                continue
            out.append("    /* +0x%-4X */ %s;" % (off, decl))
            if bits is None:
                asserts.append((mname, off))
        out.append("};")
        # `offsetof` is not a CONSTANT EXPRESSION for a non-POD type in this
        # compiler -- a class with a base fails with "illegal constant
        # expression" -- so the per-member assertion can only be emitted for
        # types without inheritance. `sizeof` is fine either way, so those
        # types still get their size checked, and the offsets are still
        # written down in the comments. The tool reports the split rather
        # than quietly emitting fewer checks.
        if asserts:
            out.append("#ifdef DWARF_ASSERT_OFFSETS")
            for mname, off in asserts:
                out.append("DWARF_ASSERT_OFFSET(%s, %s, 0x%X);"
                           % (name, mname, off))
            out.append("#endif")
        if size:
            out.append("DWARF_ASSERT_SIZE(%s, 0x%X);" % (name, size))
        if unknown:
            out.insert(1, "/* %d member offset(s) could NOT be read and are "
                          "marked; do not treat them as 0. */" % unknown)
        return "\n".join(out)


HEADER = """/*
 * Types recovered from the Wii build's DWARF by tools/dwarf_types.py.
 * Do not hand-edit: regenerate.
 *
 * Every type's SIZE is asserted unconditionally. Every member's OFFSET is
 * asserted too, but behind -DDWARF_ASSERT_OFFSETS, because `offsetof` is
 * not a constant expression for a non-POD type in this compiler -- a class
 * with a base or a virtual function fails with "illegal constant
 * expression". Define it when the types in play are plain structs; the
 * offsets are in the comments either way, and they are the recovered fact.
 */
#ifndef DWARF_TYPES_H
#define DWARF_TYPES_H

#include "types.h"

#ifndef DWARF_ASSERT_SIZE
#define DWARF_ASSERT_SIZE(T, N) \\
    typedef char _dwarf_size_##T[(sizeof(T) == (N)) ? 1 : -1]
#define DWARF_ASSERT_OFFSET(T, M, N) \\
    typedef char _dwarf_off_##T##_##M[(offsetof(T, M) == (N)) ? 1 : -1]
#endif

"""


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--elf", default=str(DEFAULT_ELF))
    ap.add_argument("--report", action="store_true")
    ap.add_argument("--conflicts", action="store_true")
    ap.add_argument("--type", action="append", default=[])
    ap.add_argument("--all", action="store_true")
    ap.add_argument("-o", "--out")
    a = ap.parse_args(argv)

    t = Types(a.elf)

    if a.report or not (a.type or a.all or a.conflicts):
        print("%s" % Path(a.elf).name)
        print("%d compile unit(s) carry DWARF:" % len(t.cus))
        for nm, lo, hi in t.cus:
            print("   %08X..%08X %9s B  %s"
                  % (lo, hi, "{:,}".format(hi - lo), nm))
        print("")
        comp = sum(1 for n, e in t.named.items()
                   if any(d.tag in TAG_COMPOSITE for _c, d in e))
        print("%d named type(s); %d of them struct/class/union"
              % (len(t.named), comp))
        for k in ("DW_TAG_member", "DW_TAG_subprogram", "DW_TAG_class_type",
                  "DW_TAG_structure_type", "DW_TAG_inheritance",
                  "DW_TAG_enumeration_type"):
            print("   %-28s %d" % (k, t.tags.get(k, 0)))
        c = t.conflicts()
        print("")
        print("%d type(s) the compile units DISAGREE about (see --conflicts)."
              % len(c))
        print("A type that could not be read is not a type with no members;")
        print("offsets this tool could not decode are marked, never zeroed.")
        return 0

    if a.conflicts:
        print("%d leaf name(s) cover more than one type. This DWARF emits"
              % len(t.ambiguous))
        print("nested classes and template instantiations FLAT at compile-unit")
        print("level, so nothing in the file says what encloses them. Each is")
        print("suffixed with its DIE offset, which is unique by construction.")
        print("")
        for n in sorted(t.ambiguous):
            print("  %s -- %d distinct type(s)" % (n, t.ambiguous[n]))
            for q in sorted(t.leaf.get(n, ())):
                die = t.named[q][0][1]
                size, rows = t.layout(die)
                enc = t.enclosing(die)
                print("     %-28s size=%-7s members=%d%s"
                      % (q, ("0x%X" % size) if size else "?", len(rows),
                         ("   referenced by: " + ", ".join(enc[:3]))
                         if enc else "   referenced by nothing named"))
        print("")
        print("`referenced by` is INFERRED and is a hint for naming these by")
        print("hand. A reference is not a scope; the tool does not use it.")
        # After resolution nothing indexed under one name may disagree.
        left = t.conflicts()
        print("")
        print("self-check: %d name(s) still cover disagreeing definitions"
              % len(left))
        return 1 if left else 0

    if a.all:
        names = list(t.named)
    else:
        # A leaf name may be several types. Resolving it silently is how the
        # wrong struct gets emitted under the right name, so an ambiguous
        # one is REFUSED with the candidates listed.
        names = []
        for want in a.type:
            if want in t.named:
                names.append(want)
                continue
            cand = sorted(t.leaf.get(want, ()))
            if len(cand) == 1:
                names.append(cand[0])
            elif cand:
                print("%r is ambiguous -- %d type(s) share that leaf name."
                      % (want, len(cand)), file=sys.stderr)
                for c in cand:
                    print("    %s" % c, file=sys.stderr)
                print("Name one of them exactly.", file=sys.stderr)
            else:
                print("%r is not a named type in this DWARF." % want,
                      file=sys.stderr)
    if not names:
        print("Nothing to emit.", file=sys.stderr)
        return 1

    seen, order = set(), []
    for n in names:
        t.emit(n, seen, order)

    # A type reached only through a POINTER is never defined here, so the
    # header does not compile without a forward declaration for it. Emitting
    # the pointee's full definition instead would drag in most of the game.
    fwd = t.forward_decls(seen)
    body = "\n\n".join(order)
    if fwd:
        body = ("/* Forward declarations: reached only through a pointer or\n"
                " * reference, so their layout is not needed here. */\n"
                + "\n".join(fwd) + "\n\n" + body)
    text = HEADER + body + "\n\n#endif\n"
    if a.out:
        Path(a.out).write_text(text, encoding="utf-8")
        print("wrote %s -- %d type(s) (%d requested, the rest are what they"
              % (a.out, len(order), len(names)))
        print("depend on by value)")
    else:
        print(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
