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
    # Four of seven. The three left are one compiler decision
    # each -- see the file; six spellings of the switch tie.
    "SB/GM/Engine/Game/zBTNodeReference": (4, 7),
    # The second unit to carry its own data, and the first written
    # from scratch with it. One function, 264 bytes of .bss.
    "SB/GM/Engine/Core/Wii/Env/WAD00": (1, 1),
    # The first unit with INITIALISED data: .bss and .data both.
    "SB/NG/Source/Engine/Graphics/PostRenderChannel": (1, 1),
    # A recorded near miss, listed at 0 of 1 on purpose. If it ever reads
    # 1 of 1 this check must say so rather than quietly agreeing.
    "SB/NG/Source/Tools/Havok/source/Common/Base/keycode.cxx": (0, 1),
    # Both functors match; the three sorts are a recorded near miss.
    "SB/NG/Source/Engine/Util/Sort/WAD02.cpp": (2, 5),
    # Generated, not read: constant returns, plus a few accessors, of the
    # 178 and 135 functions those two chunks hold. The rest is real code
    # and is not written.
    "SB/GM/Engine/WAD02_36": (172, 172),
    "SB/GM/Engine/WAD01_26": (70, 70),
    # Five of the unit's eight functions are written; the other three
    # reach further into the graphics types than has been recovered.
    "SB/NG/Source/Engine/Graphics/Builders/StaticBuilder": (5, 5),
    # Five of seven written; xStricmp and the length-limited hash are
    # recorded near misses.  The unit has 14 functions in all.
    "SB/GM/Engine/Core/x/xString": (7, 7),
    "SB/GM/Engine/Core/x/xGameInterface": (3, 3),
    "SB/GM/Engine/Core/x/xGroup": (1, 1),
    "SB/GM/Engine/Game/zButtonMasherWidget": (2, 2),
    "SB/GM/Engine/Game/zDecal": (1, 1),
    "SB/GM/Engine/Game/zJawFlapper": (2, 2),
    "SB/GM/Engine/Game/zPIDController": (1, 1),
    # LOWERED BY HAND, and this is the only place that is allowed: the
    # second cut of the unity builds (dwarf_splits.py, header absorption)
    # moved 236 functions out of these seven remainder chunks into 86
    # recovered units, 183 of them from WAD00 alone. The build-wide count
    # stayed at 993 byte-identical functions before and after, which is
    # the evidence that nothing was lost -- only re-homed. unitcmp_pins
    # refuses to lower a pin, correctly, so these seven are written here.
    "SB/GM/Engine/WAD00": (6, 6),
    "SB/GM/Engine/WAD00_1": (1, 1),
    "SB/GM/Engine/WAD00_2": (6, 6),
    "SB/GM/Engine/WAD01_21": (4, 4),
    "SB/GM/Engine/WAD02_38": (12, 12),
    "SB/GM/Engine/WAD03_16": (3, 3),
    "SB/GM/Engine/WAD03_2": (1, 1),
    # Three pins were LOWERED by hand on 2026-08-31 -- these two and
    # TRCMessageBox below -- and this is the only reason one ever is here:
    # the generator WITHDREW a function, so the object legitimately defines
    # one fewer. Each held a constant return whose value is an ADDRESS.
    # unitcmp masks relocated fields and called it byte-identical;
    # report.json never agreed, and report.json was right.
    # tools/unitcmp_pins.py refuses to make this edit on its own, which is
    # why it is written out here instead.
    "SB/GM/Engine/WAD03_43": (23, 23),
    "SB/GM/Engine/WAD04_6": (4, 4),
    "SB/GM/Engine/WAD04_8": (1, 1),
    "SB/NG/Engine/WADSpeed": (18, 18),
    "SB/NG/Source/Engine/TRC/TRCPadManager": (7, 7),
    "SB/GM/Engine/Core/x/xUpdateCull": (1, 1),
    "SB/GM/Engine/Game/zBTDepot": (1, 1),
    "SB/GM/Engine/Game/zEventSpy": (1, 1),
    "SB/GM/Engine/Game/zWaterWheel": (1, 1),
    "SB/GM/Engine/WAD00_31": (4, 4),
    "SB/GM/Engine/WAD01_19": (3, 3),
    "SB/GM/Engine/WAD02_31": (2, 2),
    "SB/GM/Engine/WAD02_4": (1, 1),
    "SB/GM/Engine/WAD04_13": (1, 1),
    "SB/NG/Engine/WAD02_37": (13, 13),
    "SB/NG/Source/Engine/IO/VirtualKeyboard/VirtualKeyboard": (2, 2),
    # Lowered by hand for the reason given above WAD03_3.
    "SB/NG/Source/Engine/TRC/TRCMessageBox": (1, 1),
    "SB/NG/Source/Engine/TRC/TRCModule": (1, 1),
    "SB/GM/Engine/Game/zNPCCommonCombatBTActions": (1, 1),
    "SB/GM/Engine/Game/zPlayerInputAI": (1, 1),
    "SB/GM/Engine/Game/zGameState": (4, 4),
    "SB/GM/Engine/Game/zMenu": (1, 1),
    "SB/GM/Engine/Game/zPlayerInputPadMgr": (3, 3),
    "SB/GM/Engine/Game/zProjectileManager": (1, 1),
    "SB/GM/Engine/Game/zSound": (1, 1),
    "SB/GM/Engine/Game/zSoundReverb": (2, 2),
    "SB/GM/Engine/WAD01_12": (2, 2),
    "SB/GM/Engine/WAD03_22": (3, 3),
    "SB/GM/Engine/WAD03_24": (1, 1),
    "SB/NG/Engine/WAD02_12": (1, 1),
    "SB/NG/Engine/WAD02_13": (2, 2),
    "SB/NG/Source/Engine/Graphics/Display": (2, 2),
    "SB/NG/Source/Engine/Graphics/Util/ScreenShot": (2, 2),
    "SB/NG/Source/Engine/IO/File/SystemCache": (1, 1),
    "SB/NG/Source/Engine/TRC/PowerControl": (1, 1),
    "SB/NG/Source/Engine/Entities/ShaderEntity": (2, 2),
    "SB/NG/Source/Engine/System/GameWindow": (1, 1),
    "SB/GM/Engine/Core/x/xserializer": (1, 1),
    "SB/GM/Engine/Game/zAchievementsMgr": (1, 1),
    "SB/GM/Engine/Game/zBTFactory": (4, 4),
    "SB/GM/Engine/Game/zDirection": (1, 1),
    "SB/GM/Engine/Game/zNPCAnimViewer": (1, 1),
    "SB/GM/Engine/Game/zNPCFX": (2, 2),
    "SB/GM/Engine/Game/zNPCGenericPool": (1, 1),
    "SB/GM/Engine/Game/zPlayerAction": (24, 25),
    "SB/GM/Engine/Game/zProjectileSpawner": (1, 1),
    "SB/GM/Engine/Game/zUIImage": (2, 2),
    "SB/GM/Engine/Game/zUIModel": (1, 1),
    "SB/GM/Engine/Game/zUIText": (1, 1),
    "SB/GM/Engine/WAD01_17": (1, 1),
    "SB/GM/Engine/WAD03_27": (1, 1),
    "SB/NG/Engine/WAD00_4": (1, 1),
    "SB/NG/Source/Engine/Entities/MaterialEntity": (1, 1),
    "SB/NG/Source/Engine/Graphics/Graphics": (2, 2),
    "SB/NG/Source/Engine/TRC/Wii/SaveLoadWii": (2, 2),
    "SB/NG/Source/Engine/UI/Strings": (1, 1),
    "Havok/src/hkpCpuShapeRaycastJob": (0, 0),
    "MSL_C/MSL_Common/FILE_POS": (0, 0),
    "MSL_C/MSL_Common/alloc": (0, 10),
    "MSL_C/MSL_Common/buffer_io": (0, 0),
    "MSL_C/MSL_Common/direct_io": (0, 0),
    "MSL_C/MSL_Common/errno": (0, 0),
    "MSL_C/MSL_Common/float": (0, 0),
    "MSL_C/MSL_Common/locale": (0, 0),
    "MSL_C/MSL_Common/math_api": (1, 3),
    "MSL_C/MSL_Common/math_ppc": (0, 1),
    "MSL_C/MSL_Common/mem_funcs": (3, 4),
    "MSL_C/MSL_Common/misc_io": (0, 1),
    "MSL_C/MSL_Common/printf": (0, 0),
    "MSL_C/MSL_Common/signal": (0, 1),
    "MSL_C/MSL_Common/strtold": (0, 0),
    "MSL_C/MSL_Common/wctype": (0, 0),
    "MSL_C/MSL_Common/wstring": (0, 5),
    "MSL_C/MSL_Common_Embedded/Math/Double_precision/e_acos": (0, 1),
    "MSL_C/MSL_Common_Embedded/Math/Double_precision/e_atan2": (0, 1),
    "MSL_C/MSL_Common_Embedded/Math/Double_precision/e_fmod": (0, 1),
    "MSL_C/MSL_Common_Embedded/Math/Double_precision/e_log": (0, 1),
    "MSL_C/MSL_Common_Embedded/Math/Double_precision/e_pow": (0, 1),
    "MSL_C/MSL_Common_Embedded/Math/Double_precision/e_rem_pio2": (0, 1),
    "MSL_C/MSL_Common_Embedded/Math/Double_precision/e_sqrt": (0, 1),
    "MSL_C/MSL_Common_Embedded/Math/Double_precision/k_cos": (0, 1),
    "MSL_C/MSL_Common_Embedded/Math/Double_precision/k_rem_pio2": (0, 1),
    "MSL_C/MSL_Common_Embedded/Math/Double_precision/k_sin": (0, 1),
    "MSL_C/MSL_Common_Embedded/Math/Double_precision/k_tan": (0, 1),
    "MSL_C/MSL_Common_Embedded/Math/Double_precision/s_atan": (0, 1),
    "MSL_C/MSL_Common_Embedded/Math/Double_precision/s_ceil": (0, 1),
    "MSL_C/MSL_Common_Embedded/Math/Double_precision/s_copysign": (0, 1),
    "MSL_C/MSL_Common_Embedded/Math/Double_precision/s_floor": (0, 1),
    "MSL_C/MSL_Common_Embedded/Math/Double_precision/s_frexp": (0, 1),
    "MSL_C/MSL_Common_Embedded/math_sun": (1, 1),
    "MetroTRK/debugger/embedded/MetroTRK/Portable/nubevent": (0, 6),
    "Revolution/src/BTE/hci/src/hcisu_h2": (0, 0),
    "Runtime/__va_arg": (0, 1),
    "Runtime/global_destructor_chain": (0, 2),
    "Runtime/ptmf": (0, 3),
    "SB/GM/Engine/Core/Wii/iSystem": (1, 1),
    "SB/GM/Engine/Core/Wii/iTime": (2, 2),
    "SB/GM/Engine/Core/x/xOGEntity": (6, 6),
    "SB/GM/Engine/Core/x/xOGRenderHelperInfo": (1, 1),
    "SB/GM/Engine/Core/x/xScene": (1, 1),
    "SB/GM/Engine/Core/x/xTextAsset": (2, 2),
    "SB/GM/Engine/Game/zCamSplineCommonMix": (2, 2),
    "SB/GM/Engine/Game/zCharacterAsset": (1, 1),
    "SB/GM/Engine/Game/zCombatAttack": (1, 1),
    "SB/GM/Engine/Game/zHitParameters": (1, 1),
    "SB/GM/Engine/Game/zLaserScanner": (1, 1),
    "SB/GM/Engine/Game/zModuleDebugMetrics": (1, 1),
    "SB/GM/Engine/Game/zNPCSearchMapLinkCostCalculator": (0, 1),
    "SB/GM/Engine/Game/zNPCStatus": (0, 2),
    "SB/GM/Engine/Game/zNPCType": (0, 1),
    "SB/GM/Engine/Game/zPerformanceDisplay": (2, 2),
    "SB/GM/Engine/Game/zPlayerContainer": (1, 1),
    "SB/GM/Engine/Game/zSearchPath": (1, 1),
    "SB/GM/Engine/WAD03": (1, 1),
    "SB/GM/Engine/WADSpeed": (0, 0),
    "SB/NG/Engine/WAD00": (0, 0),
    "SB/NG/Engine/WAD01": (0, 0),
    "SB/NG/Source/Engine/Util/RTTID": (1, 1),
    "SB/NG/Source/Engine/Util/Sort/WAD02": (2, 5),
    "SB/NG/Source/Tools/Havok/source/Common/Base/keycode": (0, 1),
    "SB/GM/Engine/Game/zNPCUPGeneric": (7, 8),
    "SB/GM/Engine/Core/x/xWMLTypes": (1, 2),
    "SB/GM/Engine/Core/x/xCam": (2, 2),
    "SB/GM/Engine/Core/x/xEnt": (5, 5),
    "SB/GM/Engine/Core/x/xFX": (1, 1),
    "SB/GM/Engine/Core/x/xLight": (2, 2),
    "SB/GM/Engine/Core/x/xLightEffect": (1, 1),
    "SB/GM/Engine/Core/x/xModel": (1, 1),
    "SB/GM/Engine/Core/x/xRumbleEmitter": (1, 1),
    "SB/GM/Engine/Core/x/xRumbleManager": (1, 1),
    "SB/GM/Engine/Core/x/xTRC": (2, 2),
    "SB/GM/Engine/Core/x/xUIDMgr": (1, 1),
    "SB/GM/Engine/Core/x/xstransvc": (2, 2),
    "SB/GM/Engine/Game/zBTAction": (1, 1),
    "SB/GM/Engine/Game/zBTConditionBuilder": (1, 1),
    "SB/GM/Engine/Game/zBTNode": (1, 1),
    "SB/GM/Engine/Game/zBTNodeAction": (1, 1),
    "SB/GM/Engine/Game/zBTNodeDecorator": (1, 1),
    "SB/GM/Engine/Game/zBoardPlayerCharacterProxyCollisionListener": (1, 1),
    "SB/GM/Engine/Game/zBreakawayPlatform": (3, 3),
    "SB/GM/Engine/Game/zBungeeBall": (2, 2),
    "SB/GM/Engine/Game/zCamFollow": (2, 2),
    "SB/GM/Engine/Game/zCheckpoint": (2, 2),
    "SB/GM/Engine/Game/zCollectibleSpawner": (2, 2),
    "SB/GM/Engine/Game/zCommonPlayerActions": (26, 26),
    "SB/GM/Engine/Game/zFXParticleLocator": (1, 1),
    "SB/GM/Engine/Game/zFloatingCollectible": (2, 2),
    "SB/GM/Engine/Game/zHitButton": (3, 3),
    "SB/GM/Engine/Game/zInteraction": (5, 5),
    "SB/GM/Engine/Game/zMainOGModule": (2, 2),
    "SB/GM/Engine/Game/zNGLoadingScreen": (2, 2),
    "SB/GM/Engine/Game/zNPCBase": (2, 2),
    "SB/GM/Engine/Game/zNPCCommonBTActions": (4, 4),
    "SB/GM/Engine/Game/zNPCCommonMovementBTActions": (4, 4),
    "SB/GM/Engine/Game/zNPCGenericSpawner": (1, 1),
    "SB/GM/Engine/Game/zNPCGenericSwarm": (1, 1),
    "SB/GM/Engine/Game/zNPCManager": (1, 1),
    "SB/GM/Engine/Game/zPOWGroup": (1, 1),
    "SB/GM/Engine/Game/zPhysicsObject": (2, 2),
    "SB/GM/Engine/Game/zPlanktonPlayer": (7, 7),
    "SB/GM/Engine/Game/zPlantTrap": (14, 14),
    "SB/GM/Engine/Game/zPlatform": (1, 1),
    "SB/GM/Engine/Game/zPlayerConstrainer": (3, 3),
    "SB/GM/Engine/Game/zPlayerInventory": (1, 1),
    "SB/GM/Engine/Game/zProjectileHavok": (1, 1),
    "SB/GM/Engine/Game/zSBPlayerActions": (93, 95),
    "SB/GM/Engine/Game/zSearchMapCreatorNavMesh": (2, 2),
    "SB/GM/Engine/Game/zSearchStrategyAStar": (1, 1),
    "SB/GM/Engine/Game/zSpinner": (2, 2),
    "SB/GM/Engine/Game/zSpringboard": (3, 3),
    "SB/GM/Engine/Game/zTiki": (4, 4),
    "SB/GM/Engine/Game/zTrigger": (1, 1),
    "SB/GM/Engine/Game/zUI": (2, 2),
    "SB/GM/Engine/Game/zUIGroup": (1, 1),
    "SB/GM/Engine/Game/zUIMgr": (8, 8),
    "SB/GM/Engine/Game/zWallNetPositionXZ": (1, 1),
    "SB/GM/Engine/WAD00_32": (177, 177),
    "SB/GM/Engine/WAD01_13_1": (2, 2),
    "SB/GM/Engine/WAD01_1_1": (2, 2),
    "SB/GM/Engine/WAD01_28": (57, 57),
    "SB/GM/Engine/WAD01_29": (2, 2),
    "SB/GM/Engine/WAD02_6_1": (1, 1),
    "SB/GM/Engine/WAD03_32_2": (1, 1),
    "SB/GM/Engine/WAD03_3_3": (7, 7),
    "SB/GM/Engine/WAD04_14": (2, 2),
    "SB/GM/Engine/WAD04_8_2": (1, 1),
    "SB/NG/Engine/WAD00_12_2": (2, 2),
    "SB/NG/Engine/WAD00_12_3": (7, 7),
    "SB/NG/Engine/WAD00_17_1": (15, 15),
    "SB/NG/Engine/WAD00_5_1": (1, 1),
    "SB/NG/Engine/WAD02_15_1": (2, 2),
    "SB/NG/Source/Engine/AssetManager/Loader/TableManager": (1, 1),
    "SB/NG/Source/Engine/AssetManager/Overseer/Coordinator": (1, 1),
    "SB/NG/Source/Engine/Entities/Blobs/CameraFlyBlobEntity": (2, 2),
    "SB/NG/Source/Engine/Entities/ModelInstanceArticle": (2, 2),
    "SB/NG/Source/Engine/Entities/RenderCustomizerEntity": (3, 3),
    "SB/NG/Source/Engine/Entities/RenderModeEntity": (1, 1),
    "SB/NG/Source/Engine/Entities/SkinGeometryEntity": (2, 2),
    "SB/NG/Source/Engine/Graphics/Builders/SkinBuilder": (3, 3),
    "SB/NG/Source/Engine/Graphics/Light": (1, 1),
    "SB/NG/Source/Engine/Graphics/Scene": (1, 1),
    "SB/NG/Source/Engine/Graphics/Viewport": (2, 2),
    "SB/NG/Source/Engine/IO/File/LFDevice": (2, 2),
    "SB/NG/Source/Engine/IO/File/MediaFile": (2, 2),
    "SB/NG/Source/Engine/IO/File/MediaIO": (1, 1),
    "SB/NG/Source/Engine/IO/Pad/ConsolePadDevice": (1, 1),
    "SB/NG/Source/Engine/Scaleform/ScaleformModule": (1, 1),
    "SB/NG/Source/Engine/UI/Font": (1, 1),
    "SB/GM/Engine/Game/zShootingPlayer": (3, 3),
    "SB/GM/Engine/WAD02_26": (1, 1),
    "SB/GM/Engine/WAD00_26": (1, 2),
    "SB/GM/Engine/WAD02_24": (1, 1),
    "SB/GM/Engine/WAD03_33": (1, 1),
    "SB/GM/Engine/WAD03_34": (2, 2),
    "SB/GM/Engine/WAD04_6_1": (1, 1),
    "SB/NG/Engine/WAD00_5_2": (1, 1),
    "SB/NG/Engine/WAD02_9": (1, 1),
    "SB/NG/Source/Engine/Graphics/Material": (2, 2),
    "SB/NG/Source/Engine/Math/Quaternion": (1, 1),
    "SB/NG/Source/Engine/Math/Random": (1, 1),
    "SB/GM/Engine/Game/zEmbeddedStartupIcon": (2, 2),
    "SB/GM/Engine/Game/zPlayerInputBase": (1, 1),
    "SB/GM/Engine/Game/zStoryMoment": (1, 1),
    "SB/GM/Engine/WAD01_5": (0, 1),
    "SB/NG/Engine/WAD00_11": (1, 1),
    "SB/NG/Engine/WAD00_11_3": (1, 1),
    "SB/NG/Engine/WAD00_9": (1, 1),
    "SB/NG/Engine/WAD02_1_1": (1, 1),
    "SB/NG/Engine/WAD02_7_1": (1, 1),
    "SB/NG/Source/Engine/Globals": (1, 1),
    "SB/NG/Source/Engine/Graphics/Primitive": (1, 1),
    "SB/NG/Source/Engine/TestSuite/TestSuite": (1, 1),
    "SB/GM/Engine/Core/LinkFastSqrt": (2, 2),
    "SB/GM/Engine/Core/x/xFMV": (2, 2),
    "SB/GM/Engine/Core/x/xSubtitlesAsset": (1, 1),
    "SB/GM/Engine/Game/zUPQuestCard": (1, 1),
    "SB/GM/Engine/WAD00_8": (1, 1),
    "SB/NG/Engine/WAD00_12_1": (1, 1),
    "SB/NG/Engine/WAD00_16": (1, 1),
}


# A unit whose object is all tail calls, so the branch-target check is the
# ONLY thing measuring it. Before that check existed this read 183 of 183
# with 175 of the words entirely masked. That was WAD00; the second cut
# of the unity builds moved the RTTID_Fix forwarders into WAD00_32, 200
# of them, and left WAD00 six functions with no relocated branch at all
# -- at which point this guard reported itself DEAD, correctly.
BRANCH_UNIT = "SB/GM/Engine/WAD00_32"


def branch_guard():
    """Prove the relocated-branch comparison is live. -> failures."""
    before = U.compare(BRANCH_UNIT)
    if isinstance(before, str):
        print("  FAIL branch guard: %s did not build" % BRANCH_UNIT)
        return 1
    ok_before = sum(1 for v in before.values() if v[0] == 0)

    # ROTATE the names, do not shift the addresses: shifting both moved
    # the function's base and its targets by the same four bytes and every
    # name still resolved, so the mutation cancelled itself and the guard
    # could not fail. Giving each address the NEXT address's names makes
    # every resolved target name wrong, which is the thing being tested.
    byname, byaddr = U.retail_addrs()
    keys = sorted(byaddr)
    moved = (byname,
             {a: byaddr[keys[(i + 1) % len(keys)]]
              for i, a in enumerate(keys)})
    real = U.retail_addrs
    U.retail_addrs = lambda: moved
    try:
        after = U.compare(BRANCH_UNIT)
    finally:
        U.retail_addrs = real
    if isinstance(after, str):
        print("  FAIL branch guard: %s did not build under mutation"
              % BRANCH_UNIT)
        return 1
    ok_after = sum(1 for v in after.values() if v[0] == 0)

    if ok_after >= ok_before:
        print("  FAIL branch-target check is DEAD: %d/%d byte-identical "
              "either way, so a branch to the wrong symbol would read as a "
              "match" % (ok_before, ok_after))
        print("       (%d relocated branch(es) were resolvable at all)"
              % sum(v[2] - v[3] for v in before.values()))
        return 1
    print("  ok   branch-target check fired: %d byte-identical, %d once "
          "every address is given the next one's names"
          % (ok_before, ok_after))
    return 0


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
    fails += branch_guard()

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
    print("  %d failure(s) of %d check(s)" % (fails, len(EXPECT) + 2))
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
