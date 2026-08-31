"""Which recovered units can never match while they are split out.

    python tools/anon_blocked.py [--list]

CodeWarrior mangles an anonymous namespace with the name of the
TRANSLATION UNIT it compiled, not the file the namespace was written in.
Every unit here was carved out of a unity blob, so a function that lives in
an anonymous namespace carries `@unnamed@WAD02_cpp@` in retail and would
carry `@unnamed@<our file>_cpp@` in ours. The bytes can be identical and the
symbols still will not pair.

This is not a thing to work around, and two attempts are already excluded:

  * `#line 1 "WAD02.cpp"` does NOT move it. The mangler reads the real input
    filename, not __FILE__ -- measured, not assumed.
  * Naming our source WAD02.cpp would produce the right symbol and collide
    with the parent unit, which dtk refuses (`Duplicate object path`), and
    would be one file for five blobs' worth of units in any case.

So the honest answer is that these units are reachable only as part of the
whole blob, and the target list should not offer them as if they were one
good afternoon's work. `Sort.cpp` -- the largest no-data unit at 2,860
bytes -- is one of them, which is worth knowing BEFORE writing it.
"""
import re
import sys
from pathlib import Path

from elftools.elf.elffile import ELFFile

ROOT = Path(__file__).resolve().parent.parent
ELF = ROOT / "orig/R8IE78/files/SB09WiiMASTERWAD.elf"
SPLITS = ROOT / "config/R8IE78/splits.txt"


def units():
    """-> [(unit_name, [(start, end), ...])] from splits.txt."""
    out, cur, ranges = [], None, []
    for line in SPLITS.read_text(encoding="utf-8").splitlines():
        if line and not line[0].isspace() and line.rstrip().endswith(":"):
            if cur:
                out.append((cur, ranges))
            cur, ranges = line.rstrip()[:-1], []
            continue
        m = re.match(r"\s+\.text\s+start:(0x[0-9A-Fa-f]+)\s+end:(0x[0-9A-Fa-f]+)",
                     line)
        if m and cur:
            ranges.append((int(m.group(1), 16), int(m.group(2), 16)))
    if cur:
        out.append((cur, ranges))
    return out


def main():
    syms = []
    with open(ELF, "rb") as fh:
        f = ELFFile(fh)
        for sec in f.iter_sections():
            if sec.header["sh_type"] != "SHT_SYMTAB":
                continue
            for s in sec.iter_symbols():
                if s["st_info"]["type"] == "STT_FUNC" and s["st_size"]:
                    syms.append((s["st_value"], s["st_size"], s.name or ""))
    syms.sort()

    all_units = units()
    blocked, clean, sizes = [], [], {}
    for name, ranges in all_units:
        if not ranges:
            continue
        total = sum(e - b for b, e in ranges)
        sizes[name] = total
        hit = [n for a, z, n in syms
               if "@unnamed@" in n
               and any(b <= a < e for b, e in ranges)]
        (blocked if hit else clean).append((total, name, len(hit)))

    blocked.sort(reverse=True)
    clean.sort(reverse=True)

    print("  %d unit(s) with .text in splits.txt" % len(sizes))
    print("  %d BLOCKED by anonymous-namespace mangling (%s bytes)"
          % (len(blocked), format(sum(b for b, _, _ in blocked), ",")))
    print("  %d reachable (%s bytes)"
          % (len(clean), format(sum(b for b, _, _ in clean), ",")))
    print("")
    print("  largest BLOCKED:")
    for total, name, n in blocked[:8]:
        print("    %7s B  %2d anon symbol(s)  %s" % (format(total, ","), n,
                                                     name))
    if "--list" in sys.argv:
        print("")
        print("  largest reachable:")
        for total, name, _ in clean[:12]:
            print("    %7s B  %s" % (format(total, ","), name))
    return 0


if __name__ == "__main__":
    sys.exit(main())
