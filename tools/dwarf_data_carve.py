"""Give a unit the data it declares: the definitions, the split, the
force_active entries.

    python tools/dwarf_data_carve.py --survey        every candidate, ranked
    python tools/dwarf_data_carve.py --unit <unit>   what it would emit
    python tools/dwarf_data_carve.py --unit <unit> --apply

Two units carry their own data now and both were done by hand. The recipe
turned out to be three edits, and only one of them is source:

  1. the DEFINITIONS, in address order, with a type chosen for its SIZE and
     ALIGNMENT and not for what the data means;
  2. the SPLIT -- and where the range sits inside a parent chunk, the parent
     is CUT, with the upper half named after the chunk that FOLLOWS in text
     order. Leaving both halves under the parent gives "Cyclic dependency
     encountered while resolving link order", because it would have to come
     both before and after;
  3. `force_active` in config.yml for every symbol nothing references. The
     linker dead-strips those, everything after them moves, and the object
     is CORRECT while it happens -- which makes the failure read as a
     layout mistake when it is not one.

THE TYPE IS A GUESS AND THE LAYOUT IS NOT. The element type of an array is
not recoverable and does not matter; the size and the alignment are
recovered facts and they decide where everything lands. `collBSP` starts
eight bytes after a four-byte int, not four, so it is 8-aligned and the
compiler leaves the hole itself -- `double[16]` reproduces that where
`float[32]` moves everything after it.

A NAMESPACE IS ENOUGH, and that is measured rather than assumed.
`s_clamp__Q24Math11QuickCull20` could be a static member of a class or a
variable in a namespace; CodeWarrior spells both the same way, so this
always emits namespaces and never has to decide which the original was.
Compiled and checked: `namespace Math { namespace QuickCull20 { double
s_clamp[2]; } }` produces exactly that symbol.

AN ANONYMOUS NAMESPACE IS AN INSTRUCTION, NOT A REFUSAL. Its mangled name
carries the unity blob's basename, so a file called Env.cpp can never
produce `collBSPCount__19@unnamed@WAD00_cpp@` -- but a file called
WAD00.cpp at a different path can, which is the trick Util/Sort/WAD02.cpp
uses for text and Core/Wii/Env/WAD00.cpp now uses for data. This says which
name the file needs. Those symbols are LOCAL, so config.yml's force_active
cannot hold them (the linker says "is either not a global symbol or doesn't
exist. Ignored.") and `#pragma force_active on` around the definitions does
it instead.

REFUSED RATHER THAN GUESSED AT:
  * a symbol with `@` in it that is NOT an anonymous namespace -- a static
    local, a string literal. Nothing can name those. See anon_blocked.py.
  * a section start that is not 8-aligned. mwcc emits no data section
    below align 8 -- measured over every object here -- so the link would
    move it. 39 units are blocked this way and 72 are not.
  * a layout no run of types can reproduce, rather than inventing a
    padding symbol retail does not have.
"""

import argparse
import re
import sys
from collections import defaultdict
from pathlib import Path

from elftools.elf.elffile import ELFFile

ROOT = Path(__file__).resolve().parent.parent
SPLITS = ROOT / "config/R8IE78/splits.txt"
CONFIG = ROOT / "config/R8IE78/config.yml"

sys.path.insert(0, str(ROOT / "tools"))
import dwarf_data_decl as D                               # noqa: E402
import gen_accessors as A                                 # noqa: E402

# (alignment, size) -> the C spelling that produces exactly that.
SCALAR = {(8, 8): "double", (4, 4): "int", (2, 2): "short", (1, 1): "char"}


def spell(size, align):
    """A declaration type of exactly `size` bytes and `align` alignment."""
    for a, unit in ((8, "double"), (4, "int"), (2, "short"), (1, "char")):
        if a != align:
            continue
        if size == a:
            return unit, ""
        if size % a == 0:
            return unit, "[%d]" % (size // a)
    return None, None


ANON = re.compile(r"^(\d+)@unnamed@(.+?)_cpp@$")


def qualifier(sym):
    """-> (parts, name) for any data symbol, or None.

    An anonymous-namespace part comes back as `<anonymous>`; every other
    part has to be a plain identifier. This is the ONLY place the enclosing
    scopes are read for such a symbol, and dropping them would put the
    definition at the wrong scope and mangle it to something else.
    """
    if "__" not in sym:
        return ((), sym) if A.IDENT.match(sym) else None
    name, qual = sym.split("__", 1)
    if not name:
        return None
    m = re.match(r"^Q(\d)(.*)$", qual)
    rest = m.group(2) if m else qual
    count = int(m.group(1)) if m else 1
    parts = []
    for _ in range(count):
        m2 = re.match(r"^(\d+)(.*)$", rest)
        if not m2 or len(m2.group(2)) < int(m2.group(1)):
            return None
        ln = int(m2.group(1))
        parts.append(m2.group(2)[:ln])
        rest = m2.group(2)[ln:]
    if rest:
        return None
    out = []
    for part in parts:
        if re.match(r"^@unnamed@(.+?)_cpp@$", part):
            out.append("<anonymous>")
        elif A.IDENT.match(part):
            out.append(part)
        else:
            return None
    return tuple(out), name


def anon_blob(sym):
    """-> the blob basename an anonymous-namespace symbol needs, or None."""
    if "__" not in sym:
        return None
    qual = sym.split("__", 1)[1]
    m = re.match(r"^Q(\d)(.*)$", qual)
    parts = []
    rest = m.group(2) if m else qual
    count = int(m.group(1)) if m else 1
    for _ in range(count):
        m2 = re.match(r"^(\d+)(.*)$", rest)
        if not m2 or len(m2.group(2)) < int(m2.group(1)):
            return None
        ln = int(m2.group(1))
        parts.append(m2.group(2)[:ln])
        rest = m2.group(2)[ln:]
    if rest:
        return None
    for part in parts:
        m3 = re.match(r"^@unnamed@(.+?)_cpp@$", part)
        if m3:
            return m3.group(1)
    return None


def read_splits():
    """-> (ordered unit names, {unit: {section: [(lo, hi)]}})."""
    order, ranges = [], defaultdict(lambda: defaultdict(list))
    cur = None
    for line in SPLITS.read_text(encoding="utf-8").splitlines():
        if line and not line[0].isspace() and line.rstrip().endswith(":"):
            cur = line.rstrip()[:-1]
            order.append(cur)
            continue
        m = re.match(r"\s+\.(\w+)\s+start:(0x[0-9A-Fa-f]+)\s+"
                     r"end:(0x[0-9A-Fa-f]+)", line)
        if m and cur:
            ranges[cur]["." + m.group(1)].append(
                (int(m.group(2), 16), int(m.group(3), 16)))
    return order, ranges


def undefined_symbols():
    """Every symbol name some carved object refers to but does not define."""
    out = set()
    for p in (ROOT / "build/R8IE78/obj").rglob("*.o"):
        try:
            with open(p, "rb") as fh:
                for s in ELFFile(fh).iter_sections():
                    if s.header["sh_type"] != "SHT_SYMTAB":
                        continue
                    for y in s.iter_symbols():
                        if y.name and y["st_shndx"] == "SHN_UNDEF":
                            out.add(y.name)
                    break
        except Exception:
            continue
    return out


class Carve(object):
    def __init__(self):
        self.d = D.Decl()
        self.order, self.ranges = read_splits()
        self.names = {}
        with open(D.ELF, "rb") as fh:
            for s in ELFFile(fh).iter_sections():
                if s.header["sh_type"] != "SHT_SYMTAB":
                    continue
                for y in s.iter_symbols():
                    if y.name and y["st_size"]:
                        self.names.setdefault(y["st_value"], y.name)
        self.by_base = defaultdict(list)
        for u in self.order:
            self.by_base[u.replace(chr(92), "/").rsplit("/", 1)[-1]].append(u)

    def plan(self, unit):
        """-> (sections, problems). sections is {sec: (lo, hi, [rows])}."""
        base = unit.replace(chr(92), "/").rsplit("/", 1)[-1]
        mine = defaultdict(list)
        for a, (f, _n) in self.d.vars.items():
            if f == base:
                mine[self.d.section_of(a)].append(a)
        if not mine:
            return None, ["the DWARF gives this unit no data"]

        out, bad = {}, []
        for sec, addrs in sorted(mine.items()):
            addrs.sort()
            lo, hi = addrs[0], addrs[-1] + self.d.sizes[addrs[-1]]
            # THE TRAILING PADDING IS THIS UNIT'S. A one-byte variable at
            # the end leaves the range ending odd, and dtk rejects the
            # remainder that would then start there:
            #   Invalid alignment for split: ... .bss 8:0x80779F99
            # Rounding up to 4 is what it accepts, and the range must not
            # reach whatever is declared next.
            end4 = (hi + 3) // 4 * 4
            nxt = min([a for a in self.d.vars
                       if a >= hi and self.d.section_of(a) == sec] or [end4])
            hi = min(end4, nxt)
            al = 1
            while al < 8 and lo % (al * 2) == 0:
                al *= 2
            if lo % 8:
                bad.append("%s starts at %08X, aligned to %d -- mwcc emits "
                           "no data section below align 8" % (sec, lo, al))
                continue
            rows, cur, ok = [], 0, True
            needs_blob = [None]
            for a in addrs:
                nm = self.names.get(a)
                if nm is None:
                    bad.append("%s %08X has no symbol" % (sec, a))
                    ok = False
                    break
                blob = anon_blob(nm)
                if blob is None and "@" in nm:
                    bad.append("%s is a static local or a literal, which "
                               "nothing can name: %s" % (sec, nm))
                    ok = False
                    break
                if qualifier(nm) is None:
                    bad.append("%s cannot be spelled: %s" % (sec, nm))
                    ok = False
                    break
                if blob is not None:
                    if needs_blob[0] not in (None, blob):
                        bad.append("%s names two different blobs, %s and "
                                   "%s, so one file cannot hold both"
                                   % (sec, needs_blob[0], blob))
                        ok = False
                        break
                    needs_blob[0] = blob
                off = a - lo
                want = 1
                while want < 8 and off % (want * 2) == 0:
                    want *= 2
                placed = None
                for align in (8, 4, 2, 1):
                    if align > want:
                        continue
                    if (cur + align - 1) // align * align != off:
                        continue
                    ty, arr = spell(self.d.sizes[a], align)
                    if ty is None:
                        continue
                    placed = (align, ty, arr)
                    break
                if placed is None:
                    bad.append("%s %s at offset %d cannot be reached from "
                               "%d by any type of %d byte(s)"
                               % (sec, nm, off, cur, self.d.sizes[a]))
                    ok = False
                    break
                rows.append((a, nm, self.d.sizes[a], placed[1], placed[2]))
                cur = off + self.d.sizes[a]
            if ok:
                out[sec] = (lo, hi, rows, needs_blob[0])
        return out, bad

    def owner(self, sec, lo, hi):
        for u in self.order:
            for s, e in self.ranges[u].get(sec, []):
                if s <= lo and hi <= e:
                    return u, s, e
        return None, None, None

    def following(self, unit):
        i = self.order.index(unit)
        return self.order[i + 1] if i + 1 < len(self.order) else None


def render(c, unit, sections, undef):
    """-> (source text, splits edits, force_active names)."""
    lines, forced, edits = [], [], []
    for sec, (lo, hi, rows, blob) in sorted(sections.items()):
        lines.append("// %s, %08X..%08X, %d byte(s). Types chosen for SIZE"
                     % (sec, lo, hi, hi - lo))
        lines.append("// and ALIGNMENT, which are the recovered facts; what "
                     "the data")
        lines.append("// holds is not recoverable and does not decide the "
                     "layout.")
        if blob:
            lines.append("// ANONYMOUS NAMESPACE: this file has to be called")
            lines.append("// %s.cpp, at whatever path says what it really"
                         % blob)
            lines.append("// is, and the definitions need #pragma "
                         "force_active on")
            lines.append("// around them -- they are LOCAL symbols and "
                         "force_active in")
            lines.append("// config.yml cannot hold one.")
        seen_ns = None
        for _a, nm, size, ty, arr in rows:
            ns, name = qualifier(nm)
            if ns != seen_ns:
                if seen_ns:
                    for n in reversed(seen_ns):
                        lines.append("}  // namespace"
                                     if n == "<anonymous>"
                                     else "}  // namespace %s" % n)
                for n in ns:
                    lines.append("namespace {" if n == "<anonymous>"
                                 else "namespace %s {" % n)
                seen_ns = ns
            lines.append("%s %s%s;   // %08X, %d byte(s)"
                         % (ty, name, arr, _a, size))
            if nm not in undef and not blob:
                forced.append(nm)
        if seen_ns:
            for n in reversed(seen_ns):
                lines.append("}  // namespace" if n == "<anonymous>"
                             else "}  // namespace %s" % n)
        lines.append("")

        own, s, e = c.owner(sec, lo, hi)
        if own is None:
            edits.append((sec, lo, hi, None, None, None))
        elif own == unit:
            continue
        elif s == lo and hi == e:
            edits.append((sec, lo, hi, own, None, None))
        elif s == lo:
            edits.append((sec, lo, hi, own, (hi, e), None))
        elif hi == e:
            edits.append((sec, lo, hi, own, (s, lo), None))
        else:
            edits.append((sec, lo, hi, own, (s, lo), (hi, e)))
    return chr(10).join(lines), edits, forced


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--survey", action="store_true")
    ap.add_argument("--unit")
    ap.add_argument("--apply", action="store_true")
    args = ap.parse_args()

    c = Carve()

    if args.survey:
        undef = undefined_symbols()
        kinds = defaultdict(list)
        for unit in c.order:
            secs, bad = c.plan(unit)
            if secs is None:
                continue
            if bad and not secs:
                kinds["refused: " + bad[0].split(" -- ")[0]
                      .split(":")[0]].append(unit)
                continue
            total = sum(hi - lo for lo, hi, _r, _b in secs.values())
            interior = any(
                c.owner(sec, lo, hi)[0] not in (None, unit)
                and c.owner(sec, lo, hi)[1] != lo
                and c.owner(sec, lo, hi)[2] != hi
                for sec, (lo, hi, _r, _b) in secs.items())
            k = ("needs the parent cut" if interior
                 else "no cut needed")
            kinds[k].append((total, unit))
        print("  %d unit(s) could take their data:"
              % sum(len(v) for k, v in kinds.items()
                    if not k.startswith("refused")))
        for k in sorted(kinds):
            v = kinds[k]
            print("    %-46s %d" % (k, len(v)))
        for k in ("no cut needed", "needs the parent cut"):
            rows = sorted(kinds.get(k, []), reverse=True)[:10]
            if not rows:
                continue
            print("")
            print("  %s, biggest first:" % k)
            for total, unit in rows:
                print("    %6d byte(s)  %s" % (total, unit))
        return 0

    if not args.unit:
        sys.exit(__doc__)
    secs, bad = c.plan(args.unit)
    if secs is None:
        sys.exit("dwarf_data_carve: %s" % bad[0])
    for b in bad:
        print("  REFUSED  %s" % b)
    if not secs:
        sys.exit("dwarf_data_carve: nothing left to emit for %s" % args.unit)

    undef = undefined_symbols()
    src, edits, forced = render(c, args.unit, secs, undef)
    print("")
    print("  ---- definitions ----")
    print(src)
    print("  ---- splits.txt ----")
    for sec, lo, hi, own, lower, upper in edits:
        print("    %s: add   %-8s start:0x%08X end:0x%08X"
              % (args.unit, sec, lo, hi))
        if own is None:
            print("      (no unit owns that range -- nothing to cut)")
            continue
        print("      %s: replace its %s range with:" % (own, sec))
        if lower:
            print("        start:0x%08X end:0x%08X" % lower)
        if upper:
            nxt = c.following(args.unit)
            print("        start:0x%08X end:0x%08X   -> give this to %s"
                  % (upper[0], upper[1], nxt))
    print("")
    print("  ---- force_active (nothing references these) ----")
    for f in forced:
        print("    - %s" % f)
    if args.apply:
        sys.exit("dwarf_data_carve: --apply is not implemented. The three "
                 "edits above touch a source file, splits.txt and "
                 "config.yml, and each has been wrong once in a way the "
                 "build reported as something else. They are printed to be "
                 "read before they are made.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
