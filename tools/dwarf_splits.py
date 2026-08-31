"""Propose splits that cut the WAD unity builds back into real source files.

    python tools/dwarf_splits.py                 what it would do, and why
    python tools/dwarf_splits.py --apply         write splits.txt + configure.py
    python tools/dwarf_splits.py --limit 20      take at most 20 source files

WAD00..WAD04 are unity builds. WAD04 was one 137 KB `.text` split, so nothing
inside it could be matched without matching all of it -- which is why `Game
Code` sat at 0 of 11 files while every function in it already had a name.

The DWARF says which source file each function was written in, and it says
it for ALL of them: 10,064 subprograms, 819 files, 100%. The linker mostly
respected that order, so most files occupy one contiguous run of addresses.

WHY THE REMAINDER NEEDS A NAME. Carving a file out of the MIDDLE leaves the
parent with text on both sides of the hole, and dtk refuses that -- it is a
genuine cycle:

    Cyclic dependency encountered while resolving link order:
    SB/GM/Engine/WAD00.cpp -> SB/GM/Engine/Core/Wii/iTime.cpp

Peeling only at the EDGES avoids it and gets almost nowhere: every unity
unit's first and last run belongs to a file whose functions are scattered
through it, so the fixed point is 7 units and 10,252 bytes. The contiguous
files are nearly all interior.

But it is a cycle only because both sides carry the same NAME. So each unity
unit becomes an ALTERNATING SEQUENCE in address order -- recovered file,
remainder, recovered file -- and the remainders get `<unit>_N.cpp`. The order
is linear again and the interior opens up: **257 source files, 351,856 bytes,
1,887 functions**, against 7 and 10,252 by peeling.

WHAT IT DOES NOT DO, stated because a silent gap here would be believed:

  * `.text` ONLY. A file with its own statics also owns `.data`/`.bss`, and
    the DWARF's function records do not say which. Such a unit will build
    and FAIL to match, visibly, rather than quietly.
  * NO HEADERS. An inline emitted out-of-line belongs to the .cpp that used
    it, not to the .h it was written in; a `Foo.h` unit is a fiction, and
    dtk rejects it outright when `Foo.cpp` exists (same object path).
  * A file is taken only when it has exactly ONE run across ALL unity units.
    Two runs would need two units with the same name.
  * `<unit>_N.cpp` is a placeholder for a stretch nothing has been
    attributed to yet. It is not a claim that such a file existed.
  * It proposes; `--apply` writes; `ninja` decides. Nothing here claims a
    match, and every unit starts NonMatching.

Verified after applying: `main.dol` still reproduces byte for byte, and the
one already-matched unit is still 100%.
"""

import argparse
import re
import sys
from collections import OrderedDict, defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ELF = ROOT / "orig/R8IE78/files/SB09WiiMASTERWAD.elf"
SPLITS = ROOT / "config/R8IE78/splits.txt"
CONFIGURE = ROOT / "configure.py"
TAB = chr(9)
BS = chr(92)

try:
    from elftools.elf.elffile import ELFFile
except ImportError:                                            # noqa: BLE001
    print("pyelftools is required:  pip install pyelftools")
    sys.exit(1)


def dwarf_functions():
    """-> [(low, high, source_path)] for every function with an address."""
    if not ELF.exists():
        raise SystemExit(
            "%s is not there. Put the extracted disc at orig/R8IE78/.\n"
            "REFUSING to propose an empty split list, which reads as "
            "'nothing to do' when it means 'nothing was read'." % ELF)
    dw = ELFFile(open(str(ELF), "rb")).get_dwarf_info()
    out = []
    for cu in dw.iter_CUs():
        lp = dw.line_program_for_CU(cu)
        files = lp.header.get("file_entry") if lp else []
        dirs = lp.header.get("include_directory") if lp else []

        def fname(i):
            if not files or not (1 <= i <= len(files)):
                return None
            fe = files[i - 1]
            nm = fe.name.decode("latin1")
            di = fe.dir_index
            if di and dirs and 1 <= di <= len(dirs):
                nm = (dirs[di - 1].decode("latin1").replace(BS, "/")
                      + "/" + nm)
            return nm.replace(BS, "/").replace("//", "/")

        for die in cu.iter_DIEs():
            if die.tag != "DW_TAG_subprogram":
                continue
            lo = die.attributes.get("DW_AT_low_pc")
            hi = die.attributes.get("DW_AT_high_pc")
            df = die.attributes.get("DW_AT_decl_file")
            if lo is None or hi is None or df is None:
                continue
            f = fname(df.value)
            if f:
                out.append((lo.value, hi.value, f))
    out.sort()
    return out


def runs_of(funcs):
    """Contiguous stretches of one source file, in address order."""
    runs = []
    cur = None
    for lo, hi, f in funcs:
        if cur is not None and cur[2] == f and lo <= cur[1] + 64:
            cur[1] = max(cur[1], hi)
            cur[3] += 1
        else:
            cur = [lo, hi, f, 1]
            runs.append(cur)
    return runs


_IMG = None

def read_pointer(va):
    """The 32-bit word the image holds at `va`, or None."""
    import struct as _s
    global _IMG
    if _IMG is None:
        import sys as _sys
        _sys.path.insert(0, str(ROOT / "tools"))
        d = ELF.read_bytes()
        e_shoff, = _s.unpack_from(">I", d, 32)
        e_shentsize, e_shnum, e_shstrndx = _s.unpack_from(">HHH", d, 46)
        secs = [_s.unpack_from(">IIIIIIIIII", d, e_shoff + i * e_shentsize)
                for i in range(e_shnum)]
        _IMG = (d, secs)
    d, secs = _IMG
    for sh in secs:
        addr, off, size = sh[3], sh[4], sh[5]
        if addr and addr <= va < addr + size and sh[1] != 8:
            return _s.unpack_from(">I", d, off + (va - addr))[0]
    return None


def parse_splits():
    """-> OrderedDict unit -> [(section, lo, hi, raw_line)] in file order."""
    units = OrderedDict()
    unit = None
    pat = re.compile(r"^[ " + TAB + r"]+(\S+)[ " + TAB
                     + r"]+start:0x([0-9A-Fa-f]+)[ " + TAB
                     + r"]+end:0x([0-9A-Fa-f]+)")
    for line in SPLITS.read_text(encoding="utf-8").splitlines():
        if line and not line.startswith((" ", TAB)) and line.rstrip().endswith(":"):
            unit = line.rstrip()[:-1].split(":")[0]
            units.setdefault(unit, [])
            continue
        m = pat.match(line)
        if m and unit:
            units[unit].append((m.group(1), int(m.group(2), 16),
                                int(m.group(3), 16), line))
    return units


# The unity builds. Only these are peeled; everything else is already split.
UNITY = ("WAD00.cpp", "WAD01.cpp", "WAD02.cpp", "WAD03.cpp", "WAD04.cpp",
         "WADSpeed.cpp")


def src_path(dwarf_path):
    """The repo-relative unit name for a DWARF source path."""
    p = dwarf_path
    for anchor, prefix in (("/GM/", "SB/GM/"), ("/NG/", "SB/NG/")):
        i = p.find(anchor)
        if i >= 0:
            return prefix + p[i + len(anchor):]
    return "SB/" + p.split("/")[-1]


def plan(units, runs, limit):
    """-> [(parent, lo, hi, nfuncs, source_or_None)] covering each unity unit.

    Every unity unit becomes an ALTERNATING SEQUENCE of units in address
    order: a recovered source file, then whatever is left before the next
    one, and so on. `source_or_None` is None for a remainder chunk, which is
    named `<unit>_N.cpp` -- a placeholder for a stretch nothing has been
    attributed to yet, not a claim that such a file exists.

    That is what makes the MIDDLE reachable. Peeling only at the edges stops
    almost immediately: every unity unit's first and last run belongs to a
    file whose functions are scattered through it, so nothing comes off. The
    569 files that ARE contiguous are all interior, and an interior file
    leaves the parent with text on both sides -- which is a cycle only
    because both sides carry the same NAME. Give the second side its own
    name and the order is linear again.

    A file is only taken when it has exactly ONE run across ALL unity units:
    two runs would need two units with the same name.
    """
    inside = OrderedDict()
    for name, secs in units.items():
        if not any(name.endswith(u) for u in UNITY):
            continue
        for sec, lo, hi, _raw in secs:
            if sec == ".text":
                inside.setdefault(name, []).append([lo, hi])

    bounds = [(lo, hi, name) for name, rs in inside.items() for lo, hi in rs]

    def parent_of(r):
        for lo, hi, name in bounds:
            if lo <= r[0] and r[1] <= hi:
                return name
        return None

    count = defaultdict(int)
    for r in runs:
        if parent_of(r) is not None:
            count[r[2]] += 1

    out = []
    for name, ranges in inside.items():
        for lo, hi in ranges:
            here = sorted(r for r in runs if lo <= r[0] and r[1] <= hi)
            cursor = lo
            for r in here:
                if count[r[2]] != 1:
                    continue
                # A HEADER IS NOT A TRANSLATION UNIT. An inline function
                # emitted out-of-line belongs to whichever .cpp used it, not
                # to the .h it was written in, so making `Foo.h` a unit is a
                # fiction -- and dtk rejects it outright when a .cpp of the
                # same stem exists, because both want the same object path:
                #
                #   Duplicate object path: zCamTargetSpline.cpp and
                #   zCamTargetSpline.h both resolve to zCamTargetSpline.o
                #
                # They stay in the remainder chunks, where they belong.
                if not r[2].lower().endswith((".c", ".cpp", ".cxx", ".cc")):
                    continue
                if r[0] < cursor:
                    continue
                if len([g for g in out if g[4]]) >= limit:
                    break
                if r[0] > cursor:
                    out.append((name, cursor, r[0], 0, None))
                out.append((name, r[0], r[1], r[3], r[2]))
                cursor = r[1]
            if cursor < hi:
                out.append((name, cursor, hi, 0, None))
    return out, inside


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--apply", action="store_true")
    ap.add_argument("--limit", type=int, default=10000)
    a = ap.parse_args(argv)

    funcs = dwarf_functions()
    runs = runs_of(funcs)
    units = parse_splits()
    got, inside = plan(units, runs, a.limit)

    named = [g for g in got if g[4]]
    chunks = [g for g in got if not g[4]]
    print("%d function(s) placed by DWARF, %d contiguous run(s)"
          % (len(funcs), len(runs)))
    print("%d source file(s) recovered as their own unit, plus %d unattributed"
          % (len(named), len(chunks)))
    print("remainder chunk(s) named <unit>_N.cpp:")
    print("")
    for name, lo, hi, n, src in named[:25]:
        print("   %-16s %08X..%08X %7d B %4d fn  %s"
              % (name.split("/")[-1], lo, hi, hi - lo, n, src_path(src)))
    if len(named) > 25:
        print("   ... and %d more" % (len(named) - 25))
    total_b = sum(hi - lo for _n, lo, hi, _f, s in named if s)
    total_f = sum(f for _n, _l, _h, f, s in named if s)
    print("")
    print("%s byte(s) and %d function(s) become individually matchable."
          % ("{:,}".format(total_b), total_f))
    print("Every one starts NonMatching: this proposes units, not matches.")
    print("`.text` only -- a file with its own statics also owns .data/.bss,")
    print("which the DWARF's function records do not give, and such a unit")
    print("will fail to match visibly rather than quietly.")

    if not a.apply:
        print("")
        print("nothing written; pass --apply")
        return 0

    # Name every segment. The PARENT keeps its own name and its non-.text
    # sections, and takes the first remainder chunk -- its .rodata/.data/.bss
    # have not been attributed to anything and must not be orphaned. Later
    # chunks become <parent>_N.cpp.
    by_parent = OrderedDict()
    for name, lo, hi, n, src in got:
        by_parent.setdefault(name, []).append((lo, hi, n, src))

    ordered = OrderedDict()
    for parent, segs in by_parent.items():
        segs.sort()
        used_parent = False
        idx = 0
        rows = []
        for lo, hi, n, src in segs:
            if src:
                rows.append((src_path(src), lo, hi, True))
                continue
            if not used_parent:
                rows.append((parent, lo, hi, False))
                used_parent = True
            else:
                idx += 1
                rows.append(("%s_%d.cpp" % (parent[:-4], idx), lo, hi, False))
        if not used_parent:
            print("   %s has no remainder chunk; its data sections would be"
                  % parent)
            print("   orphaned. REFUSING to apply.")
            return 1
        ordered[parent] = rows

    text = SPLITS.read_text(encoding="utf-8")
    for parent, rows in ordered.items():
        # Replace the parent's WHOLE original block in one operation.
        #
        # The first version re-emitted the non-.text lines inside the new
        # block and then deleted the originals with a separate
        # `text.replace(raw, "", 1)` pass. Those strings are IDENTICAL, so
        # the deletion removed the copy it had just written and left the
        # original attached to whatever unit came next in the file. dtk
        # caught it -- "Mismatched splits for .ctors 4:0x8067CE4C
        # (WAD02_75.cpp) and function 3:0x80102A60 (WAD02_73.cpp)" -- which
        # is the kind of thing a hash check would not have noticed, because
        # the DOL still reproduced.
        # `.ctors` HOLDS A POINTER TO A FUNCTION, and dtk requires the entry
        # and its target to be in the same unit:
        #
        #   Mismatched splits for .ctors 4:0x8067CE44 (WAD00.cpp)
        #   and function 3:0x800517B0 (WAD00_56.cpp)
        #
        # So it goes wherever the static initialiser it names ended up, not
        # on the first chunk with everything else.
        other = []
        ctors_for = {}
        for sec, slo, shi, raw in units[parent]:
            if sec == ".text":
                continue
            if sec in (".ctors", ".dtors"):
                tgt = read_pointer(slo)
                owner = None
                for unit, lo, hi, _s in rows:
                    if tgt is not None and lo <= tgt < hi:
                        owner = unit
                        break
                if owner is not None:
                    ctors_for.setdefault(owner, []).append(raw)
                    continue
            other.append(raw)
        old_block = "%s:\n%s" % (parent,
                                 "\n".join(raw for _s, _l, _h, raw
                                           in units[parent]))
        block = []
        for unit, lo, hi, _is_src in rows:
            block.append("%s:" % unit)
            block.append("%s.text       start:0x%08X end:0x%08X"
                         % (TAB, lo, hi))
            if unit == parent:
                block.extend(other)
            block.extend(ctors_for.get(unit, []))
            block.append("")
        assert text.count(old_block) == 1, (
            "%s: expected exactly one block, found %d"
            % (parent, text.count(old_block)))
        text = text.replace(old_block, "\n".join(block).rstrip("\n"), 1)
    SPLITS.write_text(text, encoding="utf-8")
    print("")
    print("splits.txt: %d unit(s) written across %d unity build(s)"
          % (sum(len(r) for r in ordered.values()), len(ordered)))

    cfg = CONFIGURE.read_text(encoding="utf-8")
    added = 0
    for parent, rows in ordered.items():
        anchor = '            Object(NonMatching, "%s"),' % parent
        if anchor not in cfg:
            print("   could not find %s in configure.py" % parent)
            continue
        lines = []
        for unit, _lo, _hi, _s in rows:
            if unit == parent:
                lines.append(anchor)
            else:
                lines.append('            Object(NonMatching, "%s"),' % unit)
                added += 1
        cfg = cfg.replace(anchor, "\n".join(lines), 1)
    CONFIGURE.write_text(cfg, encoding="utf-8")
    print("configure.py: %d new object(s) registered" % added)
    print("")
    print("Now run `python configure.py && ninja`. A unit that needs data")
    print("will build and not match -- that is the honest failure, not a")
    print("reason to leave it out.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
