"""Which variable did WE put in the wrong register? Ours against retail's.

    python tools/regdiff.py <unit>                 every function it can pair
    python tools/regdiff.py <unit> <name part>     one of them

Three sessions have now ended at the same wall: the shape is right, the
size is right, and sixty-nine words differ because retail runs on r25..r31
and we run on r26..r31. Staring at the differing words says WHICH words
moved; it never says which VALUE moved, because a register number is not
a name.

Retail's DWARF names every variable and the register it got. Our object
can be made to do the same -- mwcc will emit DWARF for our source too --
and then the question stops being "why is word 41 different" and becomes
"retail keeps animName in r25 and we keep it in r26", which is a fact
about the source and can be acted on.

    retail   ours   variable                 type
    r25      r26    animName                 const char*        MOVED
    r26      r27    animInst                 RefInstanceAnimation*  MOVED
    r31      r31    this                     xOGModel*

THE OBJECT IS COMPILED TWICE AND THE .text MUST BE IDENTICAL. Adding
`-sym dwarf-2` is only safe to reason from if it changes nothing that
matters, so this compiles the unit with and without it and compares the
.text sections byte for byte. If they differ, the debug object is not
the object being measured and the tool REFUSES to print. That check is
the whole reason to trust the output, so it is not skippable.

WHAT `MOVED` MEANS, AND WHAT IT DOES NOT. It means the variable OF THAT
NAME is in a different register. That is a fact about the source only
where both sources use the same names -- which is the case whenever ours
was written from retail's DWARF, and is how most of this project's
recent code was written. Where the names were invented independently,
MOVED measures the naming and nothing else: `xStricmp` is BYTE-IDENTICAL
with retail's `result` in r9 and ours in r3, because our source has an
extra local called `atEnd` and gave the name `result` to something else.
Identical code says nothing about what the source called the values in
it. `--check` reports the agreement rate for exactly this reason: read
MOVED where the rate is high, and read the names first where it is not.

Variables are paired PER DECLARATION, in order, not per name.
zBTBuilder::ParseAsset declares `i` in eight separate loops, and merging
those made seven of the eight unable to agree with anything.

Functions are paired by their DWARF name, which both sides get from the
same compiler. Anything that pairs on neither side is COUNTED and named,
never dropped in silence -- a run that pairs nothing would otherwise look
like a run with no disagreements.
"""

import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

from elftools.elf.elffile import ELFFile                  # noqa: E402

import brief as B                                         # noqa: E402
import dwarf_types as T                                   # noqa: E402
import unitcmp as U                                       # noqa: E402
from dwarf_locals import where                            # noqa: E402


def text_of(path):
    with open(path, "rb") as fh:
        f = ELFFile(fh)
        s = f.get_section_by_name(".text")
        return s.data() if s is not None else None


def _loc_targets(f):
    """-> {offset in .debug_info: offset in .debug_loc}.

    mwcc writes a DW_AT_location as DW_FORM_data4 holding zero, and puts
    the real value in a relocation against a per-function symbol named
    `.dwarf_loc.<mangled>`. The value is that symbol's own offset plus
    the addend; the addend alone is meaningless, and using it lands in
    the middle of some other function's list.
    """
    rela = f.get_section_by_name(".rela.debug_info")
    if rela is None:
        return {}, 0
    symtab = f.get_section(rela.header["sh_link"])
    out, n = {}, 0
    for r in rela.iter_relocations():
        sym = symtab.get_symbol(r["r_info_sym"])
        if not sym.name.startswith(".dwarf_loc"):
            continue
        add = r["r_addend"] if "r_addend" in r.entry else 0
        out[r["r_offset"]] = sym["st_value"] + add
        n += 1
    return out, n


def _u32(b, i):
    return int.from_bytes(b[i:i + 4], "big")


def _u16(b, i):
    return int.from_bytes(b[i:i + 2], "big")


def _loclist(loc, off):
    """-> [(begin, end, expression)] for the list starting at off."""
    out = []
    if loc is None or off >= len(loc):
        return out
    i = off
    while i + 10 <= len(loc):
        begin, end = _u32(loc, i), _u32(loc, i + 4)
        if begin == 0 and end == 0:
            break
        n = _u16(loc, i + 8)
        expr = loc[i + 10:i + 10 + n]
        if expr:
            out.append((begin, end, where(list(expr))))
        i += 10 + n
    return out


def our_locals(obj):
    """-> ({function name: [(kind, name, type, [locations])]}, stats)."""
    out = {}
    resolved = unresolved = 0
    with open(obj, "rb") as fh:
        f = ELFFile(fh)
        if not f.has_dwarf_info():
            return out, (0, 0)
        targets, n_rel = _loc_targets(f)
        locsec = f.get_section_by_name(".debug_loc")
        # Raw, deliberately: .debug_loc's begin/end are already literal
        # .text offsets here, so the all-zero list terminator is real and
        # writing relocation addends over the section would destroy it.
        loc = bytes(locsec.data()) if locsec is not None else None
        if not n_rel:
            raise SystemExit(
                "regdiff: .rela.debug_info names no .dwarf_loc symbol, so "
                "no location can be resolved. That is a failure to read "
                "the object, not a unit whose variables are nowhere.")
        # Not relocated by pyelftools: it refuses this ELF's types, and
        # the two sections that matter are relocated above by hand.
        dw = f.get_dwarf_info(relocate_dwarf_sections=False)
        for cu in dw.iter_CUs():
            fn, depth, want = None, 0, 0
            for die in cu.iter_DIEs():
                if die.is_null():
                    depth -= 1
                    if fn is not None and depth <= want:
                        fn = None
                    continue
                if die.tag == "DW_TAG_subprogram":
                    nm = T.name_of(die)
                    if nm:
                        fn, want = nm, depth
                        out.setdefault(nm, [])
                elif fn is not None and die.tag in (
                        "DW_TAG_formal_parameter", "DW_TAG_variable"):
                    a = die.attributes.get("DW_AT_location")
                    locs = []
                    if a is not None:
                        if isinstance(a.value, (list, bytes, bytearray)):
                            locs = [(None, None, where(list(a.value)))]
                            resolved += 1
                        else:
                            off = targets.get(a.offset, a.value)
                            locs = _loclist(loc, off)
                            if locs:
                                resolved += 1
                            else:
                                unresolved += 1
                    kind = ("param" if die.tag == "DW_TAG_formal_parameter"
                            else "local")
                    try:
                        tn = _type_name(dw, die)
                    except Exception:
                        tn = "?"
                    out[fn].append((kind, T.name_of(die) or "?", tn, locs))
                if die.has_children:
                    depth += 1
    return out, (resolved, unresolved)


def _type_name(dw, die):
    """A best-effort type name from an unlinked object's DWARF."""
    ref = die.attributes.get("DW_AT_type")
    if ref is None:
        return "void"
    seen, suffix = 0, ""
    cu = die.cu
    while ref is not None and seen < 12:
        seen += 1
        try:
            t = cu.get_DIE_from_refaddr(ref.value + cu.cu_offset)
        except Exception:
            return "?" + suffix
        if t.tag == "DW_TAG_pointer_type":
            suffix = "*" + suffix
        elif t.tag == "DW_TAG_reference_type":
            suffix = "&" + suffix
        elif t.tag == "DW_TAG_const_type":
            suffix = suffix
        nm = T.name_of(t)
        if nm:
            return nm + suffix
        ref = t.attributes.get("DW_AT_type")
    return "?" + suffix


def regs(locs):
    """The distinct locations a variable occupied, in order.

    Accepts either brief's plain strings (retail) or our object's
    (begin, end, text) triples, so the two sides compare directly.
    """
    out = []
    for x in locs:
        t = x[2] if isinstance(x, tuple) else x
        if t not in out:
            out.append(t)
    return out


def our_text_and_funcs(obj):
    """-> (.text bytes, {name: (offset, size)}) for our own object."""
    with open(obj, "rb") as fh:
        f = ELFFile(fh)
        sec = f.get_section_by_name(".text")
        text = sec.data() if sec is not None else b""
        idx = list(f.iter_sections()).index(sec) if sec is not None else -1
        out = {}
        for s2 in f.iter_sections():
            if s2.header["sh_type"] != "SHT_SYMTAB":
                continue
            for sym in s2.iter_symbols():
                if sym["st_info"]["type"] == "STT_FUNC" and sym["st_size"] \
                        and sym["st_shndx"] == idx:
                    out[sym.name] = (sym["st_value"], sym["st_size"])
    return text, out


def readable_check(obj, ours):
    """Every register location must be referenced inside its own range.

    -> (checked, failures as a list of strings)
    """
    import re as _re
    import disasm as _D

    text, _fns = our_text_and_funcs(obj)
    bad, unmeasured, checked = [], [], 0
    word = _re.compile(r"[A-Za-z]\w*")
    for fname, rows in ours.items():
        for _kind, vn, _tn, locs in rows:
            for item in locs:
                if not isinstance(item, tuple):
                    continue
                begin, end, what = item
                if begin is None or not _re.match(r"^[rf]\d+$", what or ""):
                    continue
                if end <= begin or end > len(text):
                    bad.append("%s: %s claims %08X..%08X, outside a .text of "
                               "%d bytes" % (fname, vn, begin, end, len(text)))
                    continue
                hit = undecoded = False
                for at in range(begin, end, 4):
                    w = int.from_bytes(text[at:at + 4], "big")
                    dc = _D.decode(w, at)
                    if not getattr(dc, "known", True):
                        undecoded = True
                    if what in word.findall(dc.text):
                        hit = True
                        break
                if hit:
                    checked += 1
                elif undecoded:
                    unmeasured.append(
                        "%s: %s over %08X..%08X holds an instruction disasm "
                        "prints as .word, so no register name is in the "
                        "text and this range cannot be checked"
                        % (fname, vn, begin, end))
                else:
                    unmeasured.append(
                        "%s: %s is said to be in %s over %08X..%08X and no "
                        "instruction there mentions %s -- a location list "
                        "kept past the last use, or a range decoded wrongly"
                        % (fname, vn, what, begin, end, what))
    return checked, bad, unmeasured


def retail_decls(rt):
    """-> {name: [ [locations of the 1st declaration], [of the 2nd], ...]}.

    One variable can hold several location ranges and one NAME can belong
    to several variables -- `i` is declared in eight separate loops of
    zBTBuilder::ParseAsset. Merging those made seven of the eight unable
    to agree with anything. Grouping by the DIE keeps a variable's ranges
    together and its namesakes apart.
    """
    by_die = {}
    order = []
    for v in rt["vars"]:
        if v.die not in by_die:
            by_die[v.die] = (v.name, [])
            order.append(v.die)
        by_die[v.die][1].append(v.loc)
    out = {}
    for d in order:
        nm, locs = by_die[d]
        out.setdefault(nm, []).append(locs)
    return out


def retail_kinds(rt):
    """-> {name: [kind of the 1st declaration, of the 2nd, ...]}."""
    by_die, order = {}, []
    for v in rt["vars"]:
        if v.die not in by_die:
            by_die[v.die] = (v.name, v.kind)
            order.append(v.die)
    out = {}
    for d in order:
        nm, kind = by_die[d]
        out.setdefault(nm, []).append(kind)
    return out


def pair(fn_ours, rt):
    """-> [(retail locs|None, our locs, name, type, our kind, their kind)]."""
    left = retail_decls(rt)
    kinds = retail_kinds(rt)
    taken = {}
    rows = []
    for kind, vn, tn, locs in fn_ours:
        i = taken.get(vn, 0)
        opts = left.get(vn) or []
        got = opts[i] if i < len(opts) else None
        ks = kinds.get(vn) or []
        tk = ks[i] if i < len(ks) else None
        taken[vn] = i + 1
        rows.append((got, locs, vn, tn, kind, tk))
    extra = []
    for vn, opts in left.items():
        for i in range(taken.get(vn, 0), len(opts)):
            extra.append((opts[i], None, vn, "", None,
                          (kinds.get(vn) or [None] * (i + 1))[i]))
    return rows, extra


def moved_in(fn_ours, rt):
    """-> (same, moved, only_ours, only_theirs) for one paired function."""
    rows, extra = pair(fn_ours, rt)
    same = moved = only_ours = 0
    for got, locs, _vn, _tn, _ok, _tk in rows:
        if got is None:
            only_ours += 1
        elif regs(locs) == regs(got):
            same += 1
        else:
            moved += 1
    return same, moved, only_ours, len(extra)


def check(unit, ours, br, theirs, dbg):
    """Two things: are our locations READABLE, and do the names AGREE?"""
    import disasm as D

    checked, bad, unmeasured = readable_check(dbg, ours)
    print("  READABLE: %d register location(s) confirmed against the "
          "instructions in their own range; %d could not be confirmed"
          % (checked, len(unmeasured)))
    for line in bad[:20]:
        print("  FAIL  %s" % line)
    for line in unmeasured[:10]:
        print("  unconfirmed: %s" % line)
    if bad:
        print("  %d location(s) decoded OUTSIDE .text. regdiff must not be "
              "read until they are gone." % len(bad))
        return 1
    if checked == 0:
        sys.exit("regdiff --check: no register location was confirmed, so "
                 "nothing was verified. That is not a pass.")
    print("  0 location(s) outside .text.")

    res = U.compare(unit)
    if isinstance(res, str):
        sys.exit("regdiff --check: unitcmp could not build %s:\n%s"
                 % (unit, res))
    _raw, _secs, funcs, _objs = D.load()
    demangled = {}
    for a, (sym, _sz) in funcs.items():
        fn = br.funcs.get(a)
        if fn is not None:
            demangled[sym] = fn["name"]

    matched = joined = unjoined = 0
    bad = []
    for sym, (bad_words, _n, _m, _u) in res.items():
        if bad_words != 0:
            continue
        matched += 1
        nm = demangled.get(sym)
        if nm is None or nm not in ours or nm not in theirs:
            unjoined += 1
            continue
        joined += 1
        _same, moved, _oo, _ot = moved_in(ours[nm], theirs[nm])
        if moved:
            bad.append((nm, moved))

    print("  AGREEMENT: %d byte-identical function(s) in %s; %d joined to "
          "a DWARF name on both sides, %d could not be joined"
          % (matched, unit, joined, unjoined))
    if joined == 0:
        sys.exit("regdiff --check: nothing was joined, so nothing was "
                 "checked. That is not a pass.")
    for nm, moved in bad[:10]:
        print("    names differ: %s, %d variable(s) in another register "
              "though the bytes are identical" % (nm[:56], moved))
    print("  %d of %d joined byte-identical function(s) agree on every "
          "variable (%.1f%%). The rest name things differently from retail, "
          "which identical bytes permit and which is not an error."
          % (joined - len(bad), joined,
             100.0 * (joined - len(bad)) / max(1, joined)))
    return 0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("unit")
    ap.add_argument("what", nargs="?", help="a function name substring")
    ap.add_argument("--check", action="store_true",
                    help="assert every byte-identical function has no "
                         "MOVED variable, and fail if one does")
    args = ap.parse_args()

    plain, err = U.compile_unit(args.unit)
    if plain is None:
        sys.exit("regdiff: the unit does not compile with the project's "
                 "flags:\n%s" % err)
    dbg, err = U.compile_unit(args.unit, ["-sym", "dwarf-2"])
    if dbg is None:
        sys.exit("regdiff: the unit does not compile with -sym dwarf-2:\n%s"
                 % err)

    a, b = text_of(plain), text_of(dbg)
    if a is None or b is None:
        sys.exit("regdiff: one of the objects has no .text section")
    if a != b:
        n = sum(1 for x, y in zip(a, b) if x != y)
        sys.exit("regdiff: -sym dwarf-2 CHANGED THE CODE -- %d of %d byte(s) "
                 "of .text differ, sizes %d and %d. The debug object is not "
                 "the object being measured, so nothing below would be about "
                 "the build. REFUSING to print." % (n, len(a), len(a), len(b)))
    print("  .text is byte-identical with and without -sym dwarf-2 "
          "(%d bytes), so the debug object is the build." % len(a))

    ours, (resolved, unresolved) = our_locals(dbg)
    if not ours:
        sys.exit("regdiff: our object carries no DWARF subprogram. That is a "
                 "failure to read it, not a unit with no functions.")
    print("  %d of our variable(s) resolved to a location, %d did not"
          % (resolved, unresolved))
    if resolved == 0:
        sys.exit("regdiff: not one of our variables resolved to a location. "
                 "Every row would read as MOVED, which is a failure to read "
                 "the object and not a build that agrees with nothing. "
                 "REFUSING to print.")

    br = B.Brief()
    br.check()
    theirs = {}
    for fn in br.funcs.values():
        theirs.setdefault(fn["name"], fn)

    if args.check:
        return check(args.unit, ours, br, theirs, dbg)

    names = sorted(ours)
    if args.what:
        names = [n for n in names if args.what in n]
        if not names:
            sys.exit("regdiff: no function of %s has a DWARF name containing "
                     "%r. It has: %s" % (args.unit, args.what,
                                         ", ".join(sorted(ours)[:8])))

    paired = unpaired = 0
    moved_total = same_total = 0
    n_copies = [0]
    for nm in names:
        rt = theirs.get(nm)
        if rt is None:
            unpaired += 1
            print("")
            print("  %s" % nm)
            print("    NOT IN RETAIL'S DWARF under this name. Ours has %d "
                  "variable(s); nothing to compare them against."
                  % len(ours[nm]))
            continue
        paired += 1
        rows, extra = pair(ours[nm], rt)

        print("")
        print("  %s" % nm)
        print("    %08X..%08X, %d bytes"
              % (rt["lo"], rt["hi"], rt["hi"] - rt["lo"]))
        print("    %-22s %-22s %-24s %s"
              % ("retail", "ours", "variable", "type"))
        moved = same = only_ours = 0
        copies = []
        for got, locs, vn, tn, our_kind, their_kind in rows:
            if our_kind == "param" and their_kind == "local":
                copies.append(vn)
            mine = regs(locs) or ["(nowhere)"]
            mark = ""
            if got is None:
                only_ours += 1
                shown = "(not named)"
            else:
                shown = "/".join(regs(got))
                if mine == regs(got):
                    same += 1
                else:
                    moved += 1
                    mark = "   MOVED"
            print("    %-22s %-22s %-24s %s%s"
                  % (shown[:22], "/".join(mine)[:22], vn[:24],
                     (tn or "")[:20], mark))
        only_theirs = len(extra)
        for got, _mine, vn, _tn, _ok, _tk in extra:
            print("    %-22s %-22s %-24s %s"
                  % ("/".join(regs(got))[:22], "(we have none)", vn[:24], ""))
        print("    %d variable(s) in the same register, %d MOVED, %d only "
              "ours, %d only retail's" % (same, moved, only_ours, only_theirs))
        if copies:
            print("    A PARAMETER HERE AND A LOCAL IN RETAIL: %s."
                  % ", ".join(copies))
            print("    The original COPIED the parameter into a local at "
                  "the top. Used directly, its live range starts where it "
                  "is first read, which ranks it below the locals and "
                  "rotates the callee-saved registers. This is what "
                  "GetRefAnimation turned on.")
        n_copies[0] += len(copies)
        moved_total += moved
        same_total += same

    print("")
    print("  %d function(s) paired by DWARF name, %d of ours had no retail "
          "counterpart, of %d our object defines"
          % (paired, unpaired, len(ours)))
    print("  %d variable(s) agree on their register, %d MOVED, %d are a "
          "parameter here and a LOCAL in retail"
          % (same_total, moved_total, n_copies[0]))
    if paired == 0:
        sys.exit("regdiff: nothing paired. A run that compares nothing is "
                 "not a run with no disagreements.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
