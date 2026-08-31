"""Rank the recovered source-file units by how reachable they look.

    python tools/dwarf_targets.py              easiest first
    python tools/dwarf_targets.py --file X     the functions in one unit

`tools/dwarf_splits.py` turned the WAD unity builds into 257 units that are
real source files. Every one has a name, a signature and a declaration line
from the DWARF, and none has any source yet. This says which to write first.

The ranking is by what the CODE needs, not by size alone:

  DATA   the unit touches an absolute address (`lis`/`addis`) or the small
         data area (r2/r13). Its `.data`/`.bss`/`.rodata` are still inside
         the parent chunk, so it CANNOT match until those are attributed --
         `dwarf_splits.py` writes `.text` only, and says so.
  CALLS  the unit calls out of itself. Fine, and normal, but each callee is
         a name that has to be declared to compile.
  LEAF   neither. Nothing but registers and its own arguments -- which is
         exactly what xOGEntity.cpp was, and it matched 6 of 6 first try.

A unit is only as reachable as its hardest function, so the flags are the
union over the file.
"""

import argparse
import re
import struct
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
import dwarf_splits as D                                       # noqa: E402


def image():
    d = D.ELF.read_bytes()
    e_shoff, = struct.unpack_from(">I", d, 32)
    e_shentsize, e_shnum, _x = struct.unpack_from(">HHH", d, 46)
    secs = [struct.unpack_from(">IIIIIIIIII", d, e_shoff + i * e_shentsize)
            for i in range(e_shnum)]
    return d, secs


def read(d, secs, va, n):
    for sh in secs:
        addr, off, size = sh[3], sh[4], sh[5]
        if addr and addr <= va and va + n <= addr + size and sh[1] != 8:
            return d[off + (va - addr):off + (va - addr) + n]
    return None


def classify(d, secs, lo, hi):
    """-> (uses_data, calls_out, nwords) for one function."""
    blob = read(d, secs, lo, hi - lo)
    if blob is None:
        return True, True, 0
    data = calls = False
    for i in range(len(blob) // 4):
        w = struct.unpack_from(">I", blob, i * 4)[0]
        op = w >> 26
        if op == 15:                                   # lis / addis
            data = True
        elif op in (32, 34, 36, 38, 40, 44, 48, 50, 52, 54):
            a = (w >> 16) & 31
            if a in (2, 13):                           # r2/r13 small data
                data = True
        elif op == 18 and (w & 1):                     # bl
            calls = True
        elif op == 18:
            li = w & 0x03FFFFFC
            if li & 0x02000000:
                li -= 0x04000000
            tgt = (lo + i * 4 + li) & 0xFFFFFFFF
            if not (lo <= tgt < hi):                   # tail call out
                calls = True
    return data, calls, len(blob) // 4


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--file")
    ap.add_argument("--limit", type=int, default=30)
    a = ap.parse_args(argv)

    funcs = D.dwarf_functions()
    runs = D.runs_of(funcs)
    units = D.parse_splits()
    known = set(units)

    d, secs = image()
    by_src = defaultdict(list)
    for lo, hi, src in funcs:
        by_src[D.src_path(src)].append((lo, hi))

    if a.file:
        hits = [k for k in by_src if k.endswith(a.file)]
        if not hits:
            print("no unit ends with %r" % a.file)
            return 1
        for k in hits:
            print("%s -- %d function(s)" % (k, len(by_src[k])))
            for lo, hi in sorted(by_src[k]):
                dat, cal, n = classify(d, secs, lo, hi)
                print("   %08X..%08X %5d B %3d w  %s%s"
                      % (lo, hi, hi - lo, n,
                         "DATA " if dat else "     ",
                         "CALLS" if cal else ""))
        return 0

    rows = []
    for unit, fs in by_src.items():
        if unit not in known:
            continue
        tot = sum(hi - lo for lo, hi in fs)
        dat = cal = False
        for lo, hi in fs:
            a1, a2, _n = classify(d, secs, lo, hi)
            dat = dat or a1
            cal = cal or a2
        rows.append((dat, cal, tot, len(fs), unit))

    rows.sort(key=lambda r: (r[0], r[1], r[2]))
    leaf = [r for r in rows if not r[0] and not r[1]]
    nodata = [r for r in rows if not r[0]]
    print("%d recovered source-file unit(s) in splits.txt" % len(rows))
    print("   %d need NO data attribution  (%s bytes)"
          % (len(nodata), "{:,}".format(sum(r[2] for r in nodata))))
    print("   %d of those also call nothing out -- the xOGEntity shape"
          % len(leaf))
    print("")
    print("   %-6s %-6s %7s %4s  %s" % ("data", "calls", "bytes", "fn", "unit"))
    for dat, cal, tot, n, unit in rows[:a.limit]:
        print("   %-6s %-6s %7d %4d  %s"
              % ("DATA" if dat else "-", "CALLS" if cal else "-",
                 tot, n, unit))
    return 0


if __name__ == "__main__":
    sys.exit(main())
