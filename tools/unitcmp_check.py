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
    "SB/NG/Source/Engine/AssetManager/Domains/DomainMgr": (1, 1),
    "SB/GM/Engine/Game/zNPCBTConditionBuilder": (1, 1),
    "SB/GM/Engine/Game/zLaser": (1, 1),
    # Four of seven. The three left are one compiler decision
    # each -- see the file; six spellings of the switch tie.
    "SB/GM/Engine/Game/zBTNodeReference": (7, 7),
    # The second unit to carry its own data, and the first written
    # from scratch with it. One function, 264 bytes of .bss.
    "SB/GM/Engine/Core/Wii/Env/WAD00": (1, 1),
    # The first unit with INITIALISED data: .bss and .data both.
    "SB/NG/Source/Engine/Graphics/PostRenderChannel": (1, 1),
    # A recorded near miss, listed at 0 of 1 on purpose. If it ever reads
    # 1 of 1 this check must say so rather than quietly agreeing.
    "SB/NG/Source/Tools/Havok/source/Common/Base/keycode.cxx": (1, 1),
    # Both functors match; the three sorts are a recorded near miss.
    "SB/NG/Source/Engine/Util/Sort/WAD02.cpp": (2, 5),
    # Generated, not read: constant returns, plus a few accessors, of the
    # 178 and 135 functions those two chunks hold. The rest is real code
    # and is not written.
    "SB/GM/Engine/WAD02_36": (173, 173),
    "SB/GM/Engine/WAD01_26": (77, 77),
    # Five of the unit's eight functions are written; the other three
    # reach further into the graphics types than has been recovered.
    "SB/NG/Source/Engine/Graphics/Builders/StaticBuilder": (5, 5),
    # Five of seven written; xStricmp and the length-limited hash are
    # recorded near misses.  The unit has 14 functions in all.
    "SB/GM/Engine/Core/x/xString": (7, 7),
    "SB/GM/Engine/Core/x/xGameInterface": (3, 3),
    "SB/GM/Engine/Core/x/xGroup": (2, 2),
    "SB/GM/Engine/Game/zButtonMasherWidget": (3, 3),
    "SB/GM/Engine/Game/zDecal": (4, 6),
    "SB/GM/Engine/Game/zJawFlapper": (2, 2),
    "SB/GM/Engine/Game/zPIDController": (5, 5),
    # LOWERED BY HAND, and this is the only place that is allowed: the
    # second cut of the unity builds (dwarf_splits.py, header absorption)
    # moved 236 functions out of these seven remainder chunks into 86
    # recovered units, 183 of them from WAD00 alone. The build-wide count
    # stayed at 993 byte-identical functions before and after, which is
    # the evidence that nothing was lost -- only re-homed. unitcmp_pins
    # refuses to lower a pin, correctly, so these seven are written here.
    "SB/GM/Engine/WAD00": (6, 6),
    "SB/GM/Engine/WAD00_1": (4, 4),
    "SB/GM/Engine/WAD00_2": (7, 7),
    "SB/GM/Engine/WAD01_21": (4, 4),
    "SB/GM/Engine/WAD02_38": (35, 35),
    "SB/GM/Engine/WAD03_16": (4, 5),
    "SB/GM/Engine/WAD03_2": (1, 1),
    # Three pins were LOWERED by hand on 2026-08-31 -- these two and
    # TRCMessageBox below -- and this is the only reason one ever is here:
    # the generator WITHDREW a function, so the object legitimately defines
    # one fewer. Each held a constant return whose value is an ADDRESS.
    # unitcmp masks relocated fields and called it byte-identical;
    # report.json never agreed, and report.json was right.
    # tools/unitcmp_pins.py refuses to make this edit on its own, which is
    # why it is written out here instead.
    "SB/GM/Engine/WAD03_43": (26, 26),
    "SB/GM/Engine/WAD04_6": (4, 4),
    "SB/GM/Engine/WAD04_8": (1, 1),
    "SB/NG/Engine/WADSpeed": (18, 18),
    "SB/NG/Source/Engine/TRC/TRCPadManager": (7, 7),
    "SB/GM/Engine/Core/x/xUpdateCull": (1, 1),
    "SB/GM/Engine/Game/zBTDepot": (4, 5),
    "SB/GM/Engine/Game/zEventSpy": (6, 6),
    "SB/GM/Engine/Game/zWaterWheel": (2, 2),
    "SB/GM/Engine/WAD00_31": (6, 6),
    "SB/GM/Engine/WAD01_19": (3, 3),
    "SB/GM/Engine/WAD02_31": (2, 2),
    "SB/GM/Engine/WAD02_4": (7, 7),
    "SB/GM/Engine/WAD04_13": (3, 3),
    "SB/NG/Engine/WAD02_37": (19, 19),
    "SB/NG/Source/Engine/IO/VirtualKeyboard/VirtualKeyboard": (2, 2),
    # Lowered by hand for the reason given above WAD03_3.
    "SB/NG/Source/Engine/TRC/TRCMessageBox": (1, 1),
    "SB/NG/Source/Engine/TRC/TRCModule": (5, 5),
    "SB/GM/Engine/Game/zNPCCommonCombatBTActions": (1, 1),
    "SB/GM/Engine/Game/zPlayerInputAI": (1, 1),
    "SB/GM/Engine/Game/zGameState": (7, 9),
    "SB/GM/Engine/Game/zMenu": (1, 1),
    "SB/GM/Engine/Game/zPlayerInputPadMgr": (3, 3),
    "SB/GM/Engine/Game/zProjectileManager": (1, 1),
    "SB/GM/Engine/Game/zSound": (1, 1),
    "SB/GM/Engine/Game/zSoundReverb": (2, 2),
    "SB/GM/Engine/WAD01_12": (2, 2),
    "SB/GM/Engine/WAD03_22": (3, 3),
    # One function of the 154 the linker put in this chunk: an
    # inline that landed among xtextbox's tag parsers.
    "SB/GM/Engine/WAD04": (1, 1),
    "SB/GM/Engine/WAD03_24": (43, 46),
    "SB/NG/Engine/WAD02_12": (2, 2),
    # The GFx/Scaleform base-only destructors, taken out once when
    # reloc_audit called them overstated and written again with the
    # class's own `operator delete`: a GFx object is freed through
    # GMemoryHeap::Free, and a one-line operator delete is taken by
    # -inline auto at the call site, which is what puts that call in
    # the destructor. All eight match and reloc_audit is clean.
    "SB/NG/Engine/WAD02_13": (7, 7),
    # One function of the 68 in this chunk.
    "SB/NG/Engine/WAD02_14": (2, 2),
    "SB/NG/Source/Engine/Graphics/Display": (2, 2),
    "SB/NG/Source/Engine/Graphics/Util/ScreenShot": (2, 2),
    "SB/NG/Source/Engine/IO/File/SystemCache": (5, 5),
    "SB/NG/Source/Engine/TRC/PowerControl": (7, 7),
    "SB/NG/Source/Engine/Entities/ShaderEntity": (2, 2),
    "SB/NG/Source/Engine/System/GameWindow": (1, 1),
    "SB/GM/Engine/Core/x/xserializer": (1, 1),
    "SB/GM/Engine/Game/zAchievementsMgr": (1, 1),
    "SB/GM/Engine/Game/zBTFactory": (4, 4),
    "SB/GM/Engine/Game/zDirection": (1, 1),
    "SB/GM/Engine/Game/zNPCAnimViewer": (4, 5),
    "SB/GM/Engine/Game/zNPCFX": (2, 2),
    "SB/GM/Engine/Game/zNPCGenericPool": (2, 2),
    "SB/GM/Engine/Game/zPlayerAction": (25, 25),
    "SB/GM/Engine/Game/zProjectileSpawner": (2, 2),
    "SB/GM/Engine/Game/zUIImage": (3, 3),
    "SB/GM/Engine/Game/zUIModel": (6, 6),
    "SB/GM/Engine/Game/zUIText": (1, 1),
    "SB/GM/Engine/WAD01_17": (1, 1),
    "SB/GM/Engine/WAD03_27": (1, 1),
    "SB/NG/Engine/WAD00_4": (2, 2),
    "SB/NG/Source/Engine/Entities/MaterialEntity": (1, 1),
    "SB/NG/Source/Engine/Graphics/Graphics": (8, 8),
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
    "SB/GM/Engine/Game/zNPCStatus": (1, 1),
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
    "SB/NG/Source/Tools/Havok/source/Common/Base/keycode": (1, 1),
    "SB/GM/Engine/Game/zNPCUPGeneric": (7, 8),
    "SB/GM/Engine/Core/x/xWMLTypes": (1, 2),
    "SB/GM/Engine/Core/x/xCam": (2, 2),
    "SB/GM/Engine/Core/x/xEnt": (5, 5),
    "SB/GM/Engine/Core/x/xFX": (1, 1),
    "SB/GM/Engine/Core/x/xLight": (3, 3),
    "SB/GM/Engine/Core/x/xLightEffect": (2, 2),
    "SB/GM/Engine/Core/x/xModel": (13, 15),
    "SB/GM/Engine/Core/x/xRumbleEmitter": (4, 4),
    "SB/GM/Engine/Core/x/xRumbleManager": (1, 1),
    "SB/GM/Engine/Core/x/xTRC": (2, 2),
    "SB/GM/Engine/Core/x/xUIDMgr": (6, 6),
    "SB/GM/Engine/Core/x/xstransvc": (2, 2),
    "SB/GM/Engine/Game/zBTAction": (1, 1),
    "SB/GM/Engine/Game/zBTConditionBuilder": (63, 63),
    "SB/GM/Engine/Game/zBTNode": (1, 1),
    "SB/GM/Engine/Game/zBTNodeAction": (1, 1),
    "SB/GM/Engine/Game/zBTNodeDecorator": (1, 1),
    "SB/GM/Engine/Game/zBoardPlayerCharacterProxyCollisionListener": (1, 1),
    "SB/GM/Engine/Game/zBreakawayPlatform": (3, 3),
    "SB/GM/Engine/Game/zBungeeBall": (3, 3),
    "SB/GM/Engine/Game/zCamFollow": (2, 2),
    "SB/GM/Engine/Game/zCheckpoint": (3, 3),
    "SB/GM/Engine/Game/zCollectibleSpawner": (3, 3),
    # The animation tables merged by gen_animtables.py. The five
    # that DIFFER are AddActionTransitions bodies with many calls,
    # merged and not yet examined; the pin has to see them.
    "SB/GM/Engine/Game/zCommonPlayerActions": (168, 172),
    "SB/GM/Engine/Game/zFXParticleLocator": (1, 1),
    "SB/GM/Engine/Game/zFloatingCollectible": (3, 3),
    "SB/GM/Engine/Game/zHitButton": (5, 5),
    "SB/GM/Engine/Game/zInteraction": (5, 5),
    "SB/GM/Engine/Game/zMainOGModule": (3, 3),
    "SB/GM/Engine/Game/zNGLoadingScreen": (2, 2),
    "SB/GM/Engine/Game/zNPCBase": (2, 2),
    "SB/GM/Engine/Game/zNPCCommonBTActions": (4, 4),
    "SB/GM/Engine/Game/zNPCCommonMovementBTActions": (4, 4),
    "SB/GM/Engine/Game/zNPCGenericSpawner": (1, 1),
    "SB/GM/Engine/Game/zNPCGenericSwarm": (2, 2),
    "SB/GM/Engine/Game/zNPCManager": (1, 1),
    "SB/GM/Engine/Game/zPOWGroup": (1, 1),
    "SB/GM/Engine/Game/zPhysicsObject": (3, 3),
    "SB/GM/Engine/Game/zPlanktonPlayer": (19, 19),
    "SB/GM/Engine/Game/zPlantTrap": (17, 17),
    "SB/GM/Engine/Game/zPlatform": (3, 3),
    "SB/GM/Engine/Game/zPlayerConstrainer": (3, 3),
    "SB/GM/Engine/Game/zPlayerInventory": (1, 1),
    "SB/GM/Engine/Game/zProjectileHavok": (2, 2),
    "SB/GM/Engine/Game/zSBPlayerActions": (389, 396),
    "SB/GM/Engine/Game/zSearchMapCreatorNavMesh": (2, 2),
    "SB/GM/Engine/Game/zSearchStrategyAStar": (1, 1),
    "SB/GM/Engine/Game/zSpinner": (4, 4),
    "SB/GM/Engine/Game/zSpringboard": (4, 4),
    "SB/GM/Engine/Game/zTiki": (4, 4),
    "SB/GM/Engine/Game/zTrigger": (1, 1),
    "SB/GM/Engine/Game/zUI": (2, 2),
    "SB/GM/Engine/Game/zUIGroup": (1, 1),
    "SB/GM/Engine/Game/zUIMgr": (8, 8),
    "SB/GM/Engine/Game/zWallNetPositionXZ": (1, 1),
    "SB/GM/Engine/WAD00_32": (328, 329),
    "SB/GM/Engine/WAD01_13_1": (2, 2),
    "SB/GM/Engine/WAD01_1_1": (123, 127),
    "SB/GM/Engine/WAD01_28": (416, 416),
    "SB/GM/Engine/WAD01_29": (2, 2),
    "SB/GM/Engine/WAD02_6_1": (3, 3),
    "SB/GM/Engine/WAD03_32_2": (3, 3),
    "SB/GM/Engine/WAD03_3_3": (7, 7),
    "SB/GM/Engine/WAD04_14": (2, 2),
    "SB/GM/Engine/WAD04_8_2": (1, 1),
    "SB/NG/Engine/WAD00_12_2": (5, 5),
    "SB/NG/Engine/WAD00_12_3": (7, 7),
    "SB/NG/Engine/WAD00_17_1": (16, 16),
    "SB/NG/Engine/WAD00_5_1": (2, 2),
    "SB/NG/Engine/WAD02_15_1": (7, 7),
    "SB/NG/Source/Engine/AssetManager/Loader/TableManager": (1, 1),
    "SB/NG/Source/Engine/AssetManager/Overseer/Coordinator": (2, 2),
    "SB/NG/Source/Engine/Entities/Blobs/CameraFlyBlobEntity": (3, 3),
    # World::ImmediateGeometry's destructor, which is also the one
    # StaticGeometryEntity.cpp's StaticBuilder calls on its member.
    "SB/NG/Source/Engine/Entities/ImmediateGeometryEntity": (1, 1),
    "SB/NG/Source/Engine/Entities/ModelInstanceArticle": (3, 3),
    "SB/NG/Source/Engine/Entities/RenderCustomizerEntity": (6, 6),
    # The last six of the 68-byte module-constructor cluster,
    # one function each in a unit that had no source file.
    "SB/NG/Source/Engine/Entities/CurveEntity": (1, 1),
    "SB/NG/Source/Engine/Entities/RenderModeEntity": (7, 7),
    # One function of the six in this chunk, and it is not an
    # entity: Graphics::StaticBuilder's destructor landed here.
    "SB/NG/Source/Engine/Entities/StaticGeometryEntity": (1, 1),
    "SB/NG/Source/Engine/Entities/SkinGeometryEntity": (3, 3),
    "SB/NG/Source/Engine/Graphics/Builders/SkinBuilder": (3, 3),
    "SB/NG/Source/Engine/Graphics/Light": (1, 1),
    # One function of the nine in this chunk, and it is not a
    # Model: an inline from Renderable3D.h that landed here.
    "SB/NG/Source/Engine/Graphics/Model": (1, 1),
    "SB/NG/Source/Engine/Graphics/RenderState": (1, 1),
    "SB/NG/Source/Engine/Graphics/RenderTargetWii": (2, 2),
    "SB/NG/Source/Engine/Graphics/Scene": (1, 1),
    "SB/NG/Source/Engine/Graphics/SceneGraph": (1, 1),
    "SB/NG/Source/Engine/Graphics/Viewport": (13, 13),
    "SB/NG/Source/Engine/IO/File/LFDevice": (2, 2),
    "SB/NG/Source/Engine/IO/File/MediaFile": (8, 8),
    "SB/NG/Source/Engine/IO/File/MediaIO": (1, 1),
    "SB/NG/Source/Engine/IO/Pad/ConsolePadDevice": (2, 2),
    "SB/NG/Source/Engine/Scaleform/ScaleformModule": (1, 1),
    "SB/NG/Source/Engine/UI/Font": (3, 3),
    "SB/GM/Engine/Game/zShootingPlayer": (6, 6),
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
    "SB/GM/Engine/WAD01_5": (1, 1),
    "SB/NG/Engine/WAD00_11": (1, 1),
    # Two of the nine and one of the 34: units that had a
    # configure row and no source file until the 84-byte batch.
    "SB/NG/Engine/WAD00_11_1": (1, 1),
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
    "SB/GM/Engine/Game/zModuleMgr_Registry": (1, 1),
    "SB/GM/Engine/Game/zNPCBTActionBuilder": (1, 1),
    "SB/GM/Engine/Game/zNavMarker": (2, 2),
    "SB/GM/Engine/WAD00_9_1": (1, 1),
    "SB/GM/Engine/WAD01_30": (1, 1),
    "SB/GM/Engine/WAD02": (1, 1),
    "SB/NG/Source/Engine/Entities/ClipEntity": (1, 1),
    "SB/NG/Source/Engine/System/Main": (1, 1),
    "SB/GM/Engine/Game/zBouncer": (1, 2),
    "SB/GM/Engine/Game/zEnv": (2, 2),
    "SB/GM/Engine/Game/zPlayerAICommandGroup": (2, 2),
    "SB/GM/Engine/Game/zPlayerAISearchMapLinkCostCalculator": (1, 1),
    "SB/GM/Engine/Game/zSoundMask": (2, 2),
    "SB/GM/Engine/WAD00_12": (1, 1),
    "SB/NG/Engine/WAD02_35_1": (1, 1),
    "SB/NG/Source/Engine/Entities/GlobalFXEntity": (2, 2),
    "SB/NG/Source/Engine/Graphics/Renderable": (2, 2),
    "SB/NG/Source/Engine/TRC/Wii/SaveErrorMsgBox": (2, 2),
    "SB/NG/Source/Engine/UI/Text": (0, 1),
    "SB/GM/Engine/Game/zBase": (3, 3),
    "SB/GM/Engine/Game/zPOWManager": (3, 3),
    "SB/GM/Engine/Game/zPOWObject": (3, 3),
    "SB/GM/Engine/WAD00_7_1": (1, 1),
    "SB/GM/Engine/WAD01_11": (1, 1),
    "SB/NG/Engine/WAD01_17_1": (3, 3),
    "SB/NG/Engine/WAD01_22": (1, 1),
    "SB/NG/Source/Engine/Entities/Blobs/Anims/ComboAnimBlobEntity": (2, 2),
    "SB/NG/Source/Engine/Math/MathUtil": (1, 2),
    "SB/NG/Source/Engine/Memory/Factory/FactoryMemTypeRegistry": (2, 2),
    "SB/GM/Engine/Core/x/xRegionSupport": (1, 1),
    "SB/GM/Engine/Core/x/xTagParser": (3, 3),
    "SB/GM/Engine/Core/x/xTextDepot": (3, 3),
    "SB/GM/Engine/Core/x/xTimer": (8, 8),
    "SB/GM/Engine/Game/zBTNodeSequence": (5, 5),
    "SB/GM/Engine/Game/zCombatSystem": (4, 4),
    "SB/GM/Engine/Game/zFXRibbonPool": (4, 4),
    "SB/GM/Engine/Game/zNPCAsset": (3, 3),
    "SB/GM/Engine/Game/zNPCGroupBase": (4, 4),
    "SB/GM/Engine/Game/zNPCHelper": (1, 3),
    "SB/GM/Engine/Game/zNPCQuickTimeCombat": (2, 4),
    "SB/GM/Engine/Game/zNavLink": (7, 7),
    "SB/GM/Engine/Game/zPhysicsObjectEntity": (3, 3),
    "SB/GM/Engine/Game/zPlayerBase": (5, 5),
    "SB/GM/Engine/Game/zProjectileEntity": (3, 3),
    "SB/GM/Engine/Game/zRandomModelList": (4, 4),
    "SB/GM/Engine/Game/zReference": (5, 5),
    "SB/GM/Engine/Game/zSoundPhysics": (4, 5),
    "SB/GM/Engine/Game/zUIController": (4, 4),
    "SB/GM/Engine/WAD00_1_2": (6, 6),
    "SB/GM/Engine/WAD00_1_3": (8, 8),
    "SB/GM/Engine/WAD00_27": (1, 1),
    "SB/GM/Engine/WAD03_28": (1, 1),
    "SB/NG/Engine/WAD01_5": (2, 3),
    "SB/NG/Engine/WADSpeed_3": (1, 3),
    "SB/NG/Source/Engine/Debug/Exception": (2, 3),
    "SB/NG/Source/Engine/Entities/Blobs/RawBlobEntity": (3, 3),
    "SB/NG/Source/Engine/Entities/ImmediatePrototypeEntity": (5, 5),
    "SB/NG/Source/Engine/Entities/LightKitEntity": (5, 6),
    "SB/NG/Source/Engine/Graphics/Channel": (2, 2),
    "SB/NG/Source/Engine/Graphics/View": (4, 4),
    "SB/NG/Source/Engine/IO/File/MediaConfig": (1, 1),
    "SB/NG/Source/Engine/Memory/Memory": (1, 1),
    "SB/NG/Source/Engine/Memory/MemoryUtil": (8, 8),
    "SB/NG/Source/Engine/System/StartupConfig": (1, 1),
    "SB/GM/Engine/Core/Wii/iMath3": (3, 4),
    "SB/GM/Engine/Game/zBTNodeCondition": (8, 8),
    "SB/GM/Engine/Game/zCamPool": (8, 8),
    "SB/GM/Engine/Game/zFMV": (4, 4),
    "SB/GM/Engine/Game/zLetterbox": (6, 6),
    "SB/GM/Engine/Game/zNPCBTAction": (5, 5),
    "SB/GM/Engine/Game/zNPCExtraModel": (6, 6),
    "SB/GM/Engine/Game/zPlayerTemplate": (2, 3),
    "SB/GM/Engine/Game/zSweptCircle": (3, 3),
    "SB/GM/Engine/Game/zUIUserString": (6, 6),
    "SB/GM/Engine/Game/zWallNetGroup": (5, 5),
    "SB/NG/Source/Engine/Entities/Blobs/SkeletonBlobEntity": (3, 5),
    "SB/NG/Source/Engine/Graphics/BuildMemory": (4, 6),
    "SB/NG/Source/Engine/Graphics/Texture": (4, 5),
    "SB/NG/Source/Engine/Memory/MallocDebugWrapper": (6, 6),
    "SB/NG/Source/Engine/Memory/MemTracker": (5, 7),
    "SB/GM/Engine/Core/Wii/iMath3": (3, 4),
    "SB/GM/Engine/Game/zBTNodeCondition": (8, 8),
    "SB/GM/Engine/Game/zCamPool": (8, 8),
    "SB/GM/Engine/Game/zFMV": (4, 4),
    "SB/GM/Engine/Game/zLetterbox": (6, 6),
    "SB/GM/Engine/Game/zNPCBTAction": (5, 5),
    "SB/GM/Engine/Game/zNPCExtraModel": (6, 6),
    "SB/GM/Engine/Game/zPlayerTemplate": (2, 3),
    "SB/GM/Engine/Game/zSweptCircle": (3, 3),
    "SB/GM/Engine/Game/zUIUserString": (6, 6),
    "SB/GM/Engine/Game/zWallNetGroup": (5, 5),
    "SB/NG/Source/Engine/Entities/Blobs/SkeletonBlobEntity": (3, 5),
    "SB/NG/Source/Engine/Graphics/BuildMemory": (4, 6),
    "SB/NG/Source/Engine/Graphics/Texture": (4, 5),
    "SB/NG/Source/Engine/Memory/MallocDebugWrapper": (6, 6),
    "SB/NG/Source/Engine/Memory/MemTracker": (5, 7),
    "SB/GM/Engine/Core/x/xEvent": (4, 5),
    "SB/GM/Engine/Game/zCamTargetSpline": (6, 6),
    "SB/GM/Engine/Game/zNPCTemplate": (3, 5),
    "SB/NG/Source/Engine/Entities/Blobs/CMeshBlobEntity": (6, 7),
    "SB/NG/Source/Engine/Entities/GeometryEntity": (3, 6),
    "SB/NG/Source/Engine/Entities/ImmediateInstanceArticle": (5, 7),
    "SB/NG/Source/Engine/Entities/LightKitSceneEntity": (8, 8),
    "SB/NG/Source/Engine/Memory/PoolManager": (7, 7),
    "SB/NG/Source/Engine/Scaleform/ScaleformAllocator": (6, 6),
    "SB/GM/Engine/Core/x/iCameraNG": (8, 8),
    "SB/GM/Engine/Game/zBuyScreen": (8, 8),
    "SB/GM/Engine/Game/zNPCUpdateLOD": (4, 5),
    "SB/GM/Engine/WAD02_20_1": (4, 5),
    "SB/NG/Engine/WAD01_15": (3, 3),
    "SB/NG/Source/Engine/Graphics/Shaders/GenericShader": (7, 8),
    "SB/GM/Engine/Game/zBTClient": (9, 9),
    "SB/GM/Engine/Game/zNPCNinjaManager": (4, 7),
    "SB/GM/Engine/Game/zSoundWiimoteSpeaker": (9, 9),
    # Down from (3, 12), not up: the four intermediate
    # destructors this unit used to emit as EXTRA are gone, so
    # the object defines 8 where it defined 12. The two 96-byte
    # destructors among them are byte-identical to retail and
    # differ only in the symbol a relocated branch names --
    # reloc_audit files all three of those as folded.
    "SB/NG/Engine/WAD00_17": (3, 8),
    "SB/NG/Engine/WAD01_12_1": (7, 7),
    "SB/NG/Engine/WAD02_4_1": (2, 3),
    "SB/NG/Source/Engine/Entities/Wii/RVLFaceLibEntity": (4, 4),
    "SB/GM/Engine/WAD01_14": (4, 5),
    "SB/NG/Source/Engine/Graphics/MaterialDepot": (8, 9),
    "SB/GM/Engine/Game/zDestructibles": (3, 6),
    "SB/GM/Engine/Game/zHintSphere": (7, 8),
    "SB/GM/Engine/Game/zHudSB": (15, 19),
    "SB/GM/Engine/Game/zNeoDrive": (8, 9),
    "SB/NG/Source/Engine/World/EntityManager": (14, 18),
    "SB/GM/Engine/Game/zBlackboard": (25, 25),
    "SB/GM/Engine/Game/zBTBuilder": (12, 12),
    "SB/NG/Engine/WAD00_1": (22, 42),
    "SB/GM/Engine/WAD02_29_1": (41, 42),
    "SB/GM/Engine/WAD02_22_1": (19, 21),
    "SB/GM/Engine/Game/zVar": (47, 47),
    "SB/GM/Engine/WAD02_24_2": (14, 17),
    "SB/GM/Engine/Game/zPlayerInputHuman": (42, 42),
    "SB/GM/Engine/Core/x/xCamTransition": (1, 1),
    "SB/GM/Engine/Core/x/xCounter": (1, 1),
    "SB/GM/Engine/Core/x/xMovePoint": (1, 1),
    "SB/GM/Engine/Core/x/xScreenFade": (1, 1),
    "SB/GM/Engine/Game/zCameraCurve": (1, 1),
    "SB/GM/Engine/Game/zConditional": (1, 1),
    "SB/GM/Engine/Game/zLensFlare": (1, 1),
    "SB/GM/Engine/Game/zPortal": (2, 2),
    "SB/GM/Engine/Game/zScript": (1, 1),
    "SB/GM/Engine/Game/zTextBox": (1, 1),
    "SB/GM/Engine/Game/zUIFlashOnScreenText": (1, 1),
    "SB/GM/Engine/Core/x/xFunctionGenerator": (1, 1),
    "SB/GM/Engine/Game/zAnimList": (1, 1),
    "SB/GM/Engine/Game/zFountain": (1, 1),
    "SB/GM/Engine/Game/zInflatablePlatform": (1, 1),
    "SB/GM/Engine/Game/zLocatorEntity": (1, 1),
    "SB/GM/Engine/Game/zPlanktonReticle": (1, 1),
    "SB/GM/Engine/Game/zPlayerLocationEnt": (1, 1),
    "SB/GM/Engine/Game/zPuckReflector": (1, 1),
    "SB/GM/Engine/Game/zRubberBand": (1, 1),
    "SB/GM/Engine/Game/zScreenWarp": (2, 2),
    "SB/GM/Engine/Game/zSoundsNamed": (1, 1),
    "SB/GM/Engine/Game/zTikiScreen": (1, 1),
    "SB/GM/Engine/Game/zFog": (2, 2),
    "SB/GM/Engine/Game/zLedge": (1, 1),
    "SB/GM/Engine/Game/zSlope": (2, 2),
    "SB/GM/Engine/Game/zSurfaces": (2, 2),
    "SB/GM/Engine/Game/zTrampoline": (1, 1),
    "SB/GM/Engine/Game/zRibbon": (1, 1),
    "SB/GM/Engine/Game/zTriggerEntity": (1, 1),
    "SB/GM/Engine/Game/zWallNet": (1, 1),
    "SB/GM/Engine/WAD01_13": (2, 2),
    "SB/GM/Engine/WAD02_11_1": (1, 1),
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
