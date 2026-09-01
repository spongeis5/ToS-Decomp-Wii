"""The `RTTID_Fix<T>` family: 176 four-byte functions from ONE template.

    python tools/gen_rttid.py --survey
    python tools/gen_rttid.py --unit SB/GM/Engine/WAD00.cpp

`tools/shape_census.py` says the single commonest shape among the unmatched
game functions is a bare `b` -- 261 of them, 1,044 bytes -- and that 176 of
those 261 are called `RTTID_Fix<...>__4UtilFPvl_v`. Every one is four bytes
and every one tail-calls a `Fix__<class>Fl`, which is what

    namespace Util {
    template <class T> void RTTID_Fix(void* p, long l) { ((T*)p)->Fix(l); }
    }

compiles to once `T::Fix` is declared and not defined. So the whole family
is one template plus a one-line class declaration each, and the argument
list is read out of the symbol table rather than guessed.

THE TARGET IS NOT ALWAYS `T`'s OWN Fix, and that is the whole difficulty.
137 of the 176 branch to `Fix__<T>Fl`; the other 39 branch to some other
class's, because either T inherits Fix or the linker folded two identical
bodies together. Both cases are reproduced the same way and neither needs
guessing which it was: declare T as deriving from the class that owns the
symbol retail branches to, and the call emits that symbol. Single
inheritance at offset 0 leaves r3 alone, so the body stays four bytes.

Nothing here is decompiling. The template body is recovered fact -- the
branch target says which method, the symbol says the argument types -- but
the CLASSES are stubs, and they carry a `Fix` because the call needs one,
not because the layout is known.

Every name this writes is mangled back and compared with the symbol it was
read from, and a row that does not round-trip is refused rather than
emitted, because a symbol nobody is looking for cannot be measured.

`gen_accessors.py` owns the output file and appends this block; running
this tool on its own only reports.
"""

import argparse
import re
import struct
import sys
from collections import Counter, defaultdict
from pathlib import Path

from elftools.elf.elffile import ELFFile

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import gen_accessors as A                                # noqa: E402

ELF = ROOT / "orig/R8IE78/files/SB09WiiMASTERWAD.elf"

NAME = re.compile(r"^RTTID_Fix<(.+)>__4UtilFPvl_v$")
TARGET = re.compile(r"^Fix__(.+)Fl$")

TEMPLATE = [
    "namespace Util {",
    "",
    "// Recovered from the branch target and the two symbols: the call is",
    "// T::Fix(long) on the pointer, tail-called, so the body is one word.",
    "template <class T>",
    "void RTTID_Fix(void* p, long l) {",
    "    ((T*)p)->Fix(l);",
    "}",
    "",
    "}  // namespace Util",
]


_ALL = None


def all_functions():
    """-> sorted [(addr, size, name)] for every function symbol.

    gen_accessors.symbols() filters to the sizes ITS shapes can be, which
    does not include four, so this family is invisible to it.

    Cached: the survey asks per unit, and re-reading the symbol table 500
    times turned a two-second answer into a two-minute one.
    """
    global _ALL
    if _ALL is not None:
        return _ALL
    out = []
    with open(ELF, "rb") as fh:
        for sec in ELFFile(fh).iter_sections():
            if sec.header["sh_type"] != "SHT_SYMTAB":
                continue
            for s in sec.iter_symbols():
                if s["st_info"]["type"] == "STT_FUNC" and s["st_size"]:
                    out.append((s["st_value"], s["st_size"], s.name))
    out.sort()
    _ALL = out
    return out


def qualified(mangled):
    """-> tuple of scope parts for a mangled class name, or None.

    Refuses anything that does not mangle back to exactly what was read.
    """
    got = A.parse_type(mangled)
    if got is None:
        return None
    node, rest = got
    if rest or node[0] != "name":
        return None
    if A.mangle_type(node) != mangled:
        return None
    parts = node[1]
    if not A.usable(list(parts)):
        return None
    return tuple(parts)


def family(raw, base, foff, spans):
    """-> (rows, Counter of reasons for the rest).

    A row is {addr, sym, cls, owner}: the class the template names, and the
    class whose `Fix` the branch actually reaches. They differ for 39 of
    the 176 and the difference is what the derivation reproduces.
    """
    syms = all_functions()
    byaddr = {a: n for a, _s, n in syms}
    rows, skipped = [], Counter()

    for addr, size, sym in syms:
        if not any(lo <= addr < hi for lo, hi in spans):
            continue
        m = NAME.match(sym)
        if not m:
            continue
        if size != 4:
            skipped["RTTID_Fix with an inlined Fix (%d bytes)" % size] += 1
            continue

        word = struct.unpack(">I", raw[foff + (addr - base):
                                       foff + (addr - base) + 4])[0]
        if (word >> 26) != 18 or word & 3:
            skipped["RTTID_Fix whose one word is not a plain branch"] += 1
            continue
        disp = word & 0x03FFFFFC
        if disp & 0x02000000:
            disp -= 0x04000000
        target = byaddr.get(addr + disp)
        if target is None:
            skipped["RTTID_Fix branching where no symbol is"] += 1
            continue
        mt = TARGET.match(target)
        if not mt:
            skipped["branches to %s, which is not a Fix(long)" % target] += 1
            continue

        cls = qualified(m.group(1))
        owner = qualified(mt.group(1))
        if cls is None or owner is None:
            skipped["a class name that does not round-trip"] += 1
            continue
        # The round trip, both ways, against the bytes that were read.
        if ("RTTID_Fix<%s>__4UtilFPvl_v"
                % A.mangle_type(("name", cls))) != sym:
            skipped["the template name does not re-mangle"] += 1
            continue
        if ("Fix__%sFl" % A.mangle_type(("name", owner))) != target:
            skipped["the target name does not re-mangle"] += 1
            continue
        rows.append({"addr": addr, "sym": sym, "cls": cls, "owner": owner})

    return rows, skipped


def render(rows, defined):
    """-> (lines, emitted rows, Counter of refusals).

    `defined` is the set of (namespaces, class) this file already declares
    for another shape. A stub with the same name would be a redefinition,
    so the row is refused and reported rather than merged: merging a stub
    into somebody else's class is how a generated file starts lying about
    what it knows.
    """
    refused = Counter()

    def split(parts):
        return tuple(parts[:-1]), parts[-1]

    owners = {r["owner"] for r in rows}
    keep = []
    for r in rows:
        if split(r["cls"]) in defined or split(r["owner"]) in defined:
            refused["the class is already declared for another shape"] += 1
            continue
        keep.append(r)
    if not keep:
        return [], [], refused

    owners = {r["owner"] for r in keep}
    derived = {r["cls"] for r in keep if r["cls"] != r["owner"]}
    # A class that owns a Fix cannot also be declared as somebody's stub.
    derived -= owners

    L = ["// The RTTID_Fix<T> family -- see tools/gen_rttid.py. Each class",
         "// below is a STUB: it carries a Fix(long) because the tail call",
         "// needs one to name, and nothing else about it is known.",
         ""]

    def block(names, line_of):
        by_ns = defaultdict(list)
        for parts in names:
            by_ns[parts[:-1]].append(parts[-1])
        for ns in sorted(by_ns):
            for n in ns:
                L.append("namespace %s {" % n)
            for cls in sorted(by_ns[ns]):
                L.append(line_of(ns, cls))
            for n in reversed(ns):
                L.append("}  // namespace %s" % n)
            L.append("")

    block(sorted(owners),
          lambda ns, cls: "class %s { public: void Fix(long); };" % cls)

    base_of = {}
    for r in keep:
        if r["cls"] in derived:
            base_of[r["cls"]] = r["owner"]
    if derived:
        L.append("// These branch to another class's Fix -- T inherits it,")
        L.append("// or the linker folded two identical bodies into one.")
        L.append("// Either way the derivation emits the symbol retail has.")
        block(sorted(derived),
              lambda ns, cls: "class %s : public %s { };"
              % (cls, "::".join(base_of[ns + (cls,)])))

    L += TEMPLATE
    L.append("")
    for r in sorted(keep, key=lambda r: r["addr"]):
        L.append("template void Util::RTTID_Fix<%s>(void*, long);"
                 % "::".join(r["cls"]))
    L.append("")
    return L, keep, refused


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--survey", action="store_true")
    ap.add_argument("--unit")
    args = ap.parse_args()
    if not args.survey and not args.unit:
        sys.exit(__doc__)

    raw, base, foff, ranges = A.load_image()
    game, matched = A.from_report("gen_rttid")

    if args.unit:
        rs = [r for u, r in ranges if u == args.unit]
        if not rs:
            sys.exit("gen_rttid: splits.txt has no unit %r" % args.unit)
        wanted = [(args.unit, rs[0])]
    else:
        wanted = [(u, r) for u, r in ranges if u in game]

    total = done = 0
    per_unit = Counter()
    skipped = Counter()
    for unit, spans in wanted:
        rows, sk = family(raw, base, foff, spans)
        skipped += sk
        if not rows:
            continue
        per_unit[unit] = len(rows)
        total += len(rows)
        done += sum(1 for r in rows if r["sym"] in matched)

    print("  %d RTTID_Fix<T> row(s) across %d unit(s); %d already matching"
          % (total, len(per_unit), done))
    for unit, n in per_unit.most_common():
        print("    %-52s %4d function(s), %d bytes" % (unit, n, n * 4))
    for why, n in skipped.most_common():
        print("    skipped %-56s %d" % (why, n))
    if not total:
        sys.exit("gen_rttid: no row was found -- that is a failure to "
                 "report, not a result")
    return 0


if __name__ == "__main__":
    sys.exit(main())
