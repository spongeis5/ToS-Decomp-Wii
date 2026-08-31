"""Validate tools/unitcmp.py against EVERY known-good answer it has, and
prove its flag-drift guard actually fires.

    python tools/unitcmp_check.py

A guard that has never been seen to fire is not known to work, and one that
fires on correct input is worse than none -- it teaches you to reach past
guards.  Both directions are checked here.  The second case is not
hypothetical: the first version of the drift guard read '-W all' out of a
COMMENTED-OUT line in configure.py and refused to run at all.

EXPECT is a per-unit (byte-identical, defined) pair, not a single number,
so a unit that starts defining an extra function is a failure rather than a
silent pass.  xOGModelRefPtr is deliberately listed at 3 of 4: it is a
recorded near miss, and if it ever reads 4 of 4 this check must say so.
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import unitcmp as U

EXPECT = {
    "SB/GM/Engine/Core/x/xBase": (6, 6),
    "SB/GM/Engine/Core/x/xOGModelRefPtr": (3, 4),
    "SB/GM/Engine/Game/zPathFinderNodeFinder": (3, 3),
    "SB/NG/Source/Engine/IO/File/MediaObject": (1, 1),
    "SB/NG/Source/Engine/Math/Collide": (4, 4),
    "SB/NG/Source/Engine/Memory/FixedAllocator": (5, 5),
    "SB/NG/Source/Engine/Util/Containers": (10, 10),
    "SB/GM/Engine/Game/zCameraCurveAsset": (1, 1),
    "SB/GM/Engine/Game/zNPCInfoNode": (1, 1),
    "SB/GM/Engine/Core/x/xSpringy": (1, 1),
    "SB/NG/Source/Engine/Graphics/Scaleform": (5, 5),
    "SB/GM/Engine/Game/zWallNetPosition": (3, 3),
    "SB/NG/Source/Engine/AssetManager/Domains/Blobloids": (9, 9),
    "SB/GM/Engine/Game/zNPCBTConditionBuilder": (1, 1),
    "SB/GM/Engine/Game/zLaser": (1, 1),
    # A recorded near miss, listed at 0 of 1 on purpose. If it ever reads
    # 1 of 1 this check must say so rather than quietly agreeing.
    "SB/NG/Source/Tools/Havok/source/Common/Base/keycode.cxx": (0, 1),
    # Both functors match; the three sorts are a recorded near miss.
    "SB/NG/Source/Engine/Util/Sort/WAD02.cpp": (2, 5),
    # Generated, not read. 164 constant returns of the 177 functions
    # in the chunk; the other 13 are real code and are not written.
    "SB/GM/Engine/WAD02_36": (164, 164),
    "SB/GM/Engine/WAD01_26": (54, 54),
}


def main():
    fails = 0
    for unit, (want_ok, want_total) in sorted(EXPECT.items()):
        res = U.compare(unit)
        if isinstance(res, str):
            print("  FAIL %-40s did not build" % unit.split("/")[-1])
            fails += 1
            continue
        ok = sum(1 for v in res.values() if v[0] == 0)
        good = (ok, len(res)) == (want_ok, want_total)
        fails += 0 if good else 1
        print("  %-4s %-40s %2d/%-2d  (expected %d/%d)"
              % ("ok" if good else "FAIL", unit.split("/")[-1],
                 ok, len(res), want_ok, want_total))

    print("")
    src = (Path(__file__).resolve().parent / "unitcmp.py").read_text()
    mutated = src.replace('GAME_EXTRA = ["-O4,s"', 'GAME_EXTRA = ["-O4,q"')
    if mutated == src:
        print("  FAIL could not mutate GAME_EXTRA -- the guard is untested")
        return 1
    ns = {"__name__": "unitcmp_mutant",
          "__file__": str(Path(__file__).resolve().parent / "unitcmp.py")}
    try:
        exec(compile(mutated, "unitcmp_mutant", "exec"), ns)
        print("  FAIL drift guard did NOT fire on a mutated flag list")
        fails += 1
    except SystemExit as e:
        print("  ok   drift guard fired: %s"
              % str(e).splitlines()[0][:80])

    print("")
    print("  %d failure(s) of %d check(s)" % (fails, len(EXPECT) + 1))
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
