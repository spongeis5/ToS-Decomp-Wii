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
    # The second unit to carry its own data, and the first written
    # from scratch with it. One function, 264 bytes of .bss.
    "SB/GM/Engine/Core/Wii/Env/WAD00": (1, 1),
    # A recorded near miss, listed at 0 of 1 on purpose. If it ever reads
    # 1 of 1 this check must say so rather than quietly agreeing.
    "SB/NG/Source/Tools/Havok/source/Common/Base/keycode.cxx": (0, 1),
    # Both functors match; the three sorts are a recorded near miss.
    "SB/NG/Source/Engine/Util/Sort/WAD02.cpp": (2, 5),
    # Generated, not read: constant returns, plus a few accessors, of the
    # 178 and 135 functions those two chunks hold. The rest is real code
    # and is not written.
    "SB/GM/Engine/WAD02_36": (170, 170),
    "SB/GM/Engine/WAD01_26": (67, 67),
    # Five of the unit's eight functions are written; the other three
    # reach further into the graphics types than has been recovered.
    "SB/NG/Source/Engine/Graphics/Builders/StaticBuilder": (5, 5),
    # Five of seven written; xStricmp and the length-limited hash are
    # recorded near misses.  The unit has 14 functions in all.
    "SB/GM/Engine/Core/x/xString": (5, 7),
    "SB/GM/Engine/Core/x/xGameInterface": (3, 3),
    "SB/GM/Engine/Core/x/xGroup": (1, 1),
    "SB/GM/Engine/Game/zButtonMasherWidget": (2, 2),
    "SB/GM/Engine/Game/zDecal": (1, 1),
    "SB/GM/Engine/Game/zJawFlapper": (2, 2),
    "SB/GM/Engine/Game/zPIDController": (1, 1),
    "SB/GM/Engine/WAD00": (8, 8),
    "SB/GM/Engine/WAD00_1": (2, 2),
    "SB/GM/Engine/WAD00_12": (2, 2),
    "SB/GM/Engine/WAD00_2": (10, 10),
    "SB/GM/Engine/WAD00_7": (3, 3),
    "SB/GM/Engine/WAD01_13": (4, 4),
    "SB/GM/Engine/WAD01_21": (8, 8),
    "SB/GM/Engine/WAD02_27": (2, 2),
    "SB/GM/Engine/WAD02_38": (4, 4),
    "SB/GM/Engine/WAD03_16": (3, 3),
    "SB/GM/Engine/WAD03_17": (1, 1),
    "SB/GM/Engine/WAD03_2": (1, 1),
    # Three pins were LOWERED by hand on 2026-08-31 -- these two and
    # TRCMessageBox below -- and this is the only reason one ever is here:
    # the generator WITHDREW a function, so the object legitimately defines
    # one fewer. Each held a constant return whose value is an ADDRESS.
    # unitcmp masks relocated fields and called it byte-identical;
    # report.json never agreed, and report.json was right.
    # tools/unitcmp_pins.py refuses to make this edit on its own, which is
    # why it is written out here instead.
    "SB/GM/Engine/WAD03_3": (12, 12),
    "SB/GM/Engine/WAD03_37": (8, 8),
    "SB/GM/Engine/WAD03_43": (22, 22),
    "SB/GM/Engine/WAD04_6": (4, 4),
    "SB/GM/Engine/WAD04_8": (5, 5),
    "SB/NG/Engine/WAD00_11": (4, 4),
    "SB/NG/Engine/WAD00_12": (11, 11),
    "SB/NG/Engine/WAD00_17": (12, 12),
    "SB/NG/Engine/WAD01_12": (3, 3),
    "SB/NG/Engine/WAD01_17": (3, 3),
    "SB/NG/Engine/WAD01_2": (1, 1),
    "SB/NG/Engine/WAD02_15": (2, 2),
    "SB/NG/Engine/WADSpeed": (18, 18),
    "SB/NG/Source/Engine/TRC/TRCPadManager": (5, 5),
    "SB/GM/Engine/Core/x/xUpdateCull": (1, 1),
    "SB/GM/Engine/Game/zBTDepot": (1, 1),
    "SB/GM/Engine/Game/zEventSpy": (1, 1),
    "SB/GM/Engine/Game/zWaterWheel": (1, 1),
    "SB/GM/Engine/WAD00_24": (3, 3),
    "SB/GM/Engine/WAD00_31": (2, 2),
    "SB/GM/Engine/WAD00_6": (1, 1),
    "SB/GM/Engine/WAD00_8": (1, 1),
    "SB/GM/Engine/WAD01": (18, 18),
    "SB/GM/Engine/WAD01_1": (8, 8),
    "SB/GM/Engine/WAD01_10": (1, 1),
    "SB/GM/Engine/WAD01_11": (1, 1),
    "SB/GM/Engine/WAD01_18": (2, 2),
    "SB/GM/Engine/WAD01_19": (3, 3),
    "SB/GM/Engine/WAD02_11": (5, 5),
    "SB/GM/Engine/WAD02_22": (4, 4),
    "SB/GM/Engine/WAD02_24": (3, 3),
    "SB/GM/Engine/WAD02_31": (1, 1),
    "SB/GM/Engine/WAD02_4": (2, 2),
    "SB/GM/Engine/WAD03_1": (2, 2),
    "SB/GM/Engine/WAD03_36": (4, 4),
    "SB/GM/Engine/WAD03_40": (1, 1),
    "SB/GM/Engine/WAD04_13": (1, 1),
    "SB/NG/Engine/WAD00_2": (2, 2),
    "SB/NG/Engine/WAD01_10": (1, 1),
    "SB/NG/Engine/WAD02_1": (2, 2),
    "SB/NG/Engine/WAD02_37": (4, 4),
    "SB/NG/Source/Engine/IO/VirtualKeyboard/VirtualKeyboard": (1, 1),
    # Lowered by hand for the reason given above WAD03_3.
    "SB/NG/Source/Engine/TRC/TRCMessageBox": (1, 1),
    "SB/NG/Source/Engine/TRC/TRCModule": (1, 1),
    "SB/GM/Engine/Game/zNPCCommonCombatBTActions": (1, 1),
    "SB/GM/Engine/Game/zPlayerInputAI": (1, 1),
    "SB/GM/Engine/WAD03_32": (2, 2),
    "SB/NG/Engine/WAD01_15": (1, 1),
    "SB/GM/Engine/Game/zGameState": (4, 4),
    "SB/GM/Engine/Game/zMenu": (1, 1),
    "SB/GM/Engine/Game/zPlayerInputPadMgr": (1, 1),
    "SB/GM/Engine/Game/zProjectileManager": (1, 1),
    "SB/GM/Engine/Game/zSound": (1, 1),
    "SB/GM/Engine/Game/zSoundReverb": (1, 1),
    "SB/GM/Engine/WAD00_18": (2, 2),
    "SB/GM/Engine/WAD01_12": (1, 1),
    "SB/GM/Engine/WAD02_13": (2, 2),
    "SB/GM/Engine/WAD02_20": (2, 2),
    "SB/GM/Engine/WAD02_29": (1, 1),
    "SB/GM/Engine/WAD03_22": (4, 4),
    "SB/GM/Engine/WAD03_24": (1, 1),
    "SB/GM/Engine/WAD03_5": (3, 3),
    "SB/GM/Engine/WAD03_9": (1, 1),
    "SB/NG/Engine/WAD02_12": (1, 1),
    "SB/NG/Engine/WAD02_13": (1, 1),
    "SB/NG/Source/Engine/Graphics/Display": (2, 2),
    "SB/NG/Source/Engine/Graphics/Util/ScreenShot": (1, 1),
    "SB/NG/Source/Engine/IO/File/SystemCache": (1, 1),
    "SB/NG/Source/Engine/TRC/PowerControl": (1, 1),
    "SB/GM/Engine/WAD02_1": (1, 1),
    "SB/GM/Engine/WAD04_3": (2, 2),
    "SB/NG/Engine/WAD02_28": (1, 1),
    "SB/NG/Source/Engine/Entities/ShaderEntity": (1, 1),
    "SB/NG/Source/Engine/System/GameWindow": (1, 1),
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
