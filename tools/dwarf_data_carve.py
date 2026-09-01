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

REFUSED RATHER THAN GUESSED AT:
  * a symbol with `@` in it -- an anonymous namespace or a static local.
    Its mangled name carries the unity blob's basename, so no file at this
    path can reproduce it. See tools/anon_blocked.py.
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
            al = 1
            while al < 8 and lo % (al * 2) == 0:
                al *= 2
            if lo % 8:
                bad.append("%s starts at %08X, aligned to %d -- mwcc emits "
                           "no data section below align 8" % (sec, lo, al))
                continue
            rows, cur, ok = [], 0, True
            for a in addrs:
                nm = self.names.get(a)
                if nm is None:
                    bad.append("%s %08X has no symbol" % (sec, a))
                    ok = False
                    break
                if "@" in nm:
                    bad.append("%s is an anonymous namespace or a static "
                               "local: %s" % (sec, nm))
                    ok = False
                    break
                if A.split_data_symbol(nm) is None:
                    bad.append("%s cannot be spelled: %s" % (sec, nm))
                    ok = False
                    break
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
                out[sec] = (lo, hi, rows)
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
    for sec, (lo, hi, rows) in sorted(sections.items()):
        lines.append("// %s, %08X..%08X, %d byte(s). Types chosen for SIZE"
                     % (sec, lo, hi, hi - lo))
        lines.append("// and ALIGNMENT, which are the recovered facts; what "
                     "the data")
        lines.append("// holds is not recoverable and does not decide the "
                     "layout.")
        seen_ns = None
        for _a, nm, size, ty, arr in rows:
            ns, name = A.split_data_symbol(nm)
            if ns != seen_ns:
                if seen_ns:
                    for n in reversed(seen_ns):
                        lines.append("}  // namespace %s" % n)
                for n in ns:
                    lines.append("namespace %s {" % n)
                seen_ns = ns
            lines.append("%s %s%s;   // %08X, %d byte(s)"
                         % (ty, name, arr, _a, size))
            if nm not in undef:
                forced.append(nm)
        if seen_ns:
            for n in reversed(seen_ns):
                lines.append("}  // namespace %s" % n)
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
            total = sum(hi - lo for lo, hi, _r in secs.values())
            interior = any(
                c.owner(sec, lo, hi)[0] not in (None, unit)
                and c.owner(sec, lo, hi)[1] != lo
                and c.owner(sec, lo, hi)[2] != hi
                for sec, (lo, hi, _r) in secs.items())
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
