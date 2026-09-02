"""Which compiler versions reproduce the retail bytes, and how many of them?

    python tools/compiler_sweep.py [--lib PREFIX] [version ...]

Rebuilds every unit with a source file under the prefix (default `SB/`,
the game) with each CodeWarrior for Wii version in build/compilers/Wii,
through unitcmp's own compile-and-compare -- its flags, its masking, its
branch-name check -- with nothing swapped but the compiler path, and
counts the functions that come out byte-identical. A unit that will not
build under a version is counted as a build failure, never as a match
or as zero matches, and functions the object defines that retail does
not have are counted as `extra`.

This exists because "is it the compiler the game was built with?" is a
measurement, not a recollection. Measured on 2026-09-02 over 244 game
units and 1,167 functions: 1.1 and 1.3 reproduce 1,151 and no unit
tells them apart; the 1.0 family reproduces 1,149; 1.5 to 1.7 reproduce
1,107 and emit 16 functions retail lacks, because they stop inlining.
The disc's debug link carries `CW for Wii v1.1` in its DWARF include
paths, which is what separates 1.1 from 1.3. The retail SDK stamps
read 0x4302_145, version 4.3 build 145, which is Wii 1.0: Nintendo's
prebuilt libraries were compiled with the version before the game's.
"""
import re
import sys
import time
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(REPO / "tools"))
import unitcmp as U                                     # noqa: E402


def main():
    args = sys.argv[1:]
    prefix = "SB/"
    if "--lib" in args:
        i = args.index("--lib")
        prefix = args[i + 1]
        del args[i:i + 2]
    versions = args or sorted(
        p.name for p in (REPO / "build/compilers/Wii").iterdir()
        if (p / "mwcceppc.exe").exists())

    cfg = (REPO / "configure.py").read_text(encoding="utf-8")
    units = []
    for m in re.finditer(r'Object\((?:Matching|NonMatching), "([^"]+)"\)', cfg):
        name = m.group(1)
        if not name.startswith(prefix):
            continue
        unit = name[:-4] if name.endswith(".cpp") else name
        if (REPO / U.source_of(unit)).exists():
            units.append(unit)
    print("%d unit(s) with source under %s, %d compiler(s): %s"
          % (len(units), prefix, len(versions), " ".join(versions)))
    if not units:
        raise SystemExit("nothing to measure")

    # retail() and retail_addrs() read the image; do that once.
    retail = U.retail()
    addrs = U.retail_addrs()
    U.retail = lambda: retail
    U.retail_addrs = lambda: addrs

    rows = []
    for ver in versions:
        U.CC = "./build/compilers/Wii/%s/mwcceppc.exe" % ver
        t0 = time.time()
        funcs = exact = failed = extra = exact_bytes = 0
        failed_units = []
        per_unit = {}
        for unit in units:
            res = U.compare(unit)
            if isinstance(res, str):
                failed += 1
                failed_units.append(unit)
                continue
            got = total = 0
            for name, (bad, n, masked, unmeasured) in res.items():
                if bad < 0:
                    extra += 1
                    continue
                funcs += 1
                total += 1
                if bad == 0 and n > 0:
                    exact += 1
                    got += 1
                    exact_bytes += 4 * n
            per_unit[unit] = (got, total)
        rows.append((ver, per_unit))
        print("  %-10s exact %4d of %4d function(s), %6d bytes; "
              "%d unit(s) failed to build, %d extra; %.0fs"
              % (ver, exact, funcs, exact_bytes, failed, extra,
                 time.time() - t0))
        for u in failed_units[:6]:
            print("             failed: %s" % u)

    base = next((per for ver, per in rows if ver == "1.1"), None)
    if base is None:
        return
    print("\nUnits where a version differs from 1.1 "
          "(exact of defined, version vs 1.1):")
    for ver, per in rows:
        if ver == "1.1":
            continue
        diff = [(u, per.get(u), base.get(u)) for u in units
                if per.get(u) != base.get(u)]
        print("  %-10s %d unit(s) differ" % (ver, len(diff)))
        for u, a, b in diff[:12]:
            print("             %-52s %s vs %s" % (u, a, b))


if __name__ == "__main__":
    main()
