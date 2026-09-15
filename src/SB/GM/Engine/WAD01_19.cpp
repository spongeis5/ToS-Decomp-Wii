#include "SB/GM/Engine/WAD01_19.pool.h"

// The strings of the WAD01 pool (@stringBase0 0x80689B28) ahead of this
// file's one, "SBB1" at +6926, in pool order, so the compiled offset is
// retail's. Data read from the image (the rows of the generated WAD01_28
// table up to that string); retail has the files in front instead.
// gen_poolprefix counts only addi references to the pool, and the tag is
// read with lbz displacements, so the generated header here carries none.
static const char* const kUnityPoolStrings[] = {
    "Achievements Manager",  // +0
    "SpongeBob_set_ref",  // +21
    "SB_GooSplashFX_Ref",  // +39
    "SB_InvincibilityFX_Ref",  // +58
    "SB_InvincibilityMovingFX_Ref",  // +81
    "NormalTurnFactor",  // +110
    "StoppedTurnSpeedUp",  // +127
    "AttackTurnFactor",  // +146
    "AirTurnFactor",  // +163
    "MoveSpeedNormal",  // +177
    "MoveSpeedJump",  // +193
    "MoveAcceleration",  // +207
    "MoveDeceleration",  // +224
    "SkidStopDeceleration",  // +241
    "SkidStopDecelTime",  // +262
    "SkidStopLerpTime",  // +280
    "MinimumRunTime",  // +297
    "UseMinimumWalkSpeed",  // +312
    "SpongeBob_Puck_Aimer_Ref",  // +332
    "SpongeBob_GooSplash",  // +357
    "Do not step on board yet",  // +377
    "SpongeBob_Invincibility",  // +402
    "SpongeBob_InvincibilityMoving",  // +426
    "UID_A_BOOT_MusicHappy",  // +456
    "DefaultAnimPackage_Ref",  // +478
    "SpongebuffAnimPackage_Ref",  // +501
    "SpinPowerupAnimPackage_Ref",  // +527
    "HammerPowerupAnimPackage_Ref",  // +554
    "PuckPowerupAnimPackage_Ref",  // +583
    "GooAnimPackage_Ref",  // +610
    "DeathModelSwapAnimPackage_Ref",  // +629
    "HammerHitAnimPackage_Ref",  // +659
    "InvincibilityPowerupAnimPackage_Ref",  // +684
    "SL05",  // +720
    "Idle01",  // +725
    "IdleExtra01",  // +732
    "IdleExtra02",  // +744
    "IdleExtra03",  // +756
    "IdleExtra04",  // +768
    "IdleExtra05",  // +780
    "IdleExtra06",  // +792
    "IdleExtra07",  // +804
    "IdleMoveStart01",  // +816
    "IdleSlippery01",  // +832
    "IdleSlipperyMoveStart01",  // +847
    "IdleLowHealth01",  // +871
    "IdleCold01",  // +887
    "Idle#",  // +898
    "IdleExtra#",  // +904
    "IdleCold#",  // +915
    "IdleMoveStart01 IdleSlipperyMoveStart01",  // +925
    "Run01",  // +965
    "Walk01",  // +971
    "WalkStart01",  // +978
    "WalkStop01",  // +990
    "WalkSlipperyStart01",  // +1001
    "WalkSlippery01",  // +1021
    "WalkSlipperyStop01",  // +1036
    "WalkStart01 Walk01 WalkStop01",  // +1055
    "WalkSlippery*",  // +1085
    "WalkStop01 WalkSlipperyStop01",  // +1099
    "RunStart01",  // +1129
    "RunStop01",  // +1140
    "RunSlipperyStart01",  // +1150
    "RunSlippery01",  // +1169
    "RunSlipperyStop01",  // +1183
    "RunBraveStart01",  // +1201
    "RunBrave01",  // +1217
    "RunBraveStop01",  // +1228
    "RunSuccessStart01",  // +1243
    "RunSuccess01",  // +1261
    "RunSuccessStop01",  // +1274
    "RunBrave*",  // +1291
    "RunSuccess*",  // +1301
    "RunStart01 Run01 RunStop01",  // +1313
    "RunSlippery*",  // +1340
    "RunStop01 RunSlipperyStop01 RunBraveStop01 RunSuccessStop01",  // +1353
    "Turn180",  // +1413
    "JumpStartIdle01",  // +1421
    "JumpCycleIdle01",  // +1437
    "JumpStartMoving01",  // +1453
    "JumpCycleMoving01",  // +1471
    "FallIdle01",  // +1489
    "FallMoving01",  // +1500
    "LandIdle01",  // +1513
    "LandMoving01",  // +1524
    "SpinPowerupAttack",  // +1537
    "Spray01",  // +1555
    "SlamStart01",  // +1563
    "SlamFall01",  // +1575
    "SlamLand01",  // +1586
    "Burst01",  // +1597
    "FallHighIdle01",  // +1605
    "FallHighMoving01",  // +1620
    "FallDoubleJumpIdle01",  // +1637
    "FallDoubleJumpMoving01",  // +1658
    "FallSprBoIdle01",  // +1681
    "FallSprBoMoving01",  // +1697
    "HighFallLandIdle01",  // +1715
    "HighFallLandMoving01",  // +1734
    "LandDoubleJumpIdle01",  // +1755
    "LandDoubleJumpMoving01",  // +1776
    "LandSprBoIdle01",  // +1799
    "LandSprBoMoving01",  // +1815
    "HighFallLandGetUp01",  // +1833
    "DoubleJumpInIdle01",  // +1853
    "DoubleJumpInMoving01",  // +1872
    "DoubleJumpCycleIdle01",  // +1893
    "DoubleJumpCycleMoving01",  // +1915
    "SprBoStartIdle01",  // +1939
    "SprBoStartMoving01",  // +1956
    "SpinAttackIn01",  // +1975
    "SpinAttackCycle01",  // +1990
    "SpinAttackCycleMoving01",  // +2008
    "SpinAttackOut01",  // +2032
    "SpinSpongebuffCycle01",  // +2048
    "SpinSpongebuffCycleMoving01",  // +2070
    "SpinAttack* SpinSpongebuff*",  // +2098
    "SpinAttackCycle*",  // +2126
    "SpinSpongebuffCycle*",  // +2143
    "HammerAttack01",  // +2164
    "HammerAttackRecoilHigh",  // +2179
    "HammerAttackRecoilMed",  // +2202
    "HammerAttackAirIn01",  // +2224
    "HammerAttackAirCycle01",  // +2244
    "HammerAttackAirOut01",  // +2267
    "HammerSpongebuff01",  // +2288
    "HammerSpongebuffRecover01",  // +2307
    "HammerSpongebuffAirIn01",  // +2333
    "HammerSpongebuffAirCycle01",  // +2357
    "HammerSpongebuffAirOut01",  // +2384
    "HammerAttack* HammerSpongebuff*",  // +2409
    "PuckAttackIn01",  // +2441
    "PuckAttackCharging01",  // +2456
    "PuckAttackShoot01",  // +2477
    "PuckAttackRecover01",  // +2495
    "LedgeUp01",  // +2515
    "HitFront01",  // +2525
    "HitBack01",  // +2536
    "HitSpinFront01",  // +2546
    "HitSpinBack01",  // +2561
    "HitPuckFront01",  // +2575
    "HitPuckBack01",  // +2590
    "HitGooFront01",  // +2604
    "HitGooBack01",  // +2618
    "HitElectricArc01",  // +2631
    "HitPowerup01",  // +2648
    "HammerHitIn01",  // +2661
    "HammerHitIn02",  // +2675
    "HammerHitCycle01",  // +2689
    "HammerHitCycle02",  // +2706
    "HammerHitOut01",  // +2723
    "HammerHitOut02",  // +2738
    "HammerHitRecover01",  // +2753
    "HammerHitOut#",  // +2772
    "LaunchFront01",  // +2786
    "LaunchLandFront01",  // +2800
    "LaunchBack01",  // +2818
    "LaunchLandBack01",  // +2831
    "LaunchKnockbackFront01",  // +2848
    "LaunchLandKnockbackFront01",  // +2871
    "LaunchKnockbackBack01",  // +2898
    "LaunchLandKnockbackBack01",  // +2920
    "LaunchGooFront01",  // +2946
    "LaunchLandGooFront01",  // +2963
    "LaunchGooBack01",  // +2984
    "LaunchLandGooBack01",  // +3000
    "FillWithGooTurn01",  // +3020
    "FillWithGooIn01",  // +3038
    "DrainGooIn01",  // +3054
    "GainPowerup_Spongebuff",  // +3067
    "GainPowerup_Invincibility",  // +3090
    "GainPowerup_In",  // +3116
    "GainPowerup_Prop_Spin",  // +3131
    "GainPowerup_Prop_Hammer",  // +3153
    "GainPowerup_Prop_Puck",  // +3177
    "GainPowerup_Sidekick_Spin",  // +3199
    "GainPowerup_Sidekick_Hammer",  // +3225
    "GainPowerup_Sidekick_Puck",  // +3253
    "GainPowerup_Spongebuff GainPowerup_Invincibility",  // +3279
    "SpinPowerupAttack_Moving",  // +3328
    "HammerPowerupAttack_Idle_Lift",  // +3353
    "HammerPowerupAttack_Idle_Hit",  // +3383
    "HammerPowerupAttack_Walk_Lift_L",  // +3412
    "HammerPowerupAttack_Walk_Lift_R",  // +3444
    "HammerPowerupAttack_Walk_Hit_L",  // +3476
    "HammerPowerupAttack_Walk_Hit_R",  // +3507
    "HammerPowerupAttack_Run_Lift",  // +3538
    "HammerPowerupAttack_Run_Hit",  // +3567
    "PuckPowerupAttack_Idle_Charge",  // +3595
    "PuckPowerupAttack_Idle_Shoot",  // +3625
    "PuckPowerupAttack_Walk_Charge_L",  // +3654
    "PuckPowerupAttack_Walk_Charge_R",  // +3686
    "PuckPowerupAttack_Walk_Shoot_L",  // +3718
    "PuckPowerupAttack_Walk_Shoot_R",  // +3749
    "PuckPowerupAttack_Run_Charge",  // +3780
    "PuckPowerupAttack_Run_Shoot",  // +3809
    "LosePowerup",  // +3837
    "CandyGetOn",  // +3849
    "CandyIdle",  // +3860
    "CandyWalk",  // +3870
    "CandyGetOff",  // +3880
    "CandyBuffGetOn",  // +3892
    "CandyBuffIdle",  // +3907
    "CandyBuffWalk",  // +3921
    "CandyBuffGetOff",  // +3935
    "Candy*",  // +3951
    "QuicksandJump",  // +3958
    "QuicksandStuckIdle",  // +3972
    "QuicksandStuckWalk",  // +3991
    "BungeeEnter",  // +4010
    "BungeeTransfer",  // +4022
    "BungeeReturned",  // +4037
    "BungeeBallIdle",  // +4052
    "BungeeBallFling",  // +4067
    "BungeeBallBuffFling",  // +4083
    "BungeeBallReturn",  // +4103
    "BungeeSpecialHit",  // +4120
    "BungeeSpecialDefeated",  // +4137
    "Bungee*",  // +4159
    "BungeeBall*",  // +4167
    "BungeeEnter BungeeTransfer BungeeReturned",  // +4179
    "Cheat01",  // +4221
    "*",  // +4229
    "KelpTrapStart",  // +4231
    "KelpTrappedIdle",  // +4245
    "KelpRelease",  // +4261
    "dialog",  // +4273
    "DefeatedBeginStand01",  // +4280
    "DefeatedBeginStand02",  // +4301
    "DefeatedBeginStand03",  // +4322
    "DefeatedGravestoneSpecial01",  // +4343
    "DefeatedBeginLava01",  // +4371
    "DefeatedBeginGoo01",  // +4391
    "DefeatedBeginFrozenGoo01",  // +4410
    "DefeatedFrozenGooSpecial01",  // +4435
    "Celebration01",  // +4462
    "SlideBegin01",  // +4476
    "SlideIdle01",  // +4489
    "SlideEnd01",  // +4501
    "SlideHit01",  // +4512
    "SlideJumpEnter01",  // +4523
    "SlideJumpCycle01",  // +4540
    "SlideFallEnter01",  // +4557
    "SlideJumpLand01",  // +4574
    "x = %1.3f y = %1.3f weight = %3.1f",  // +4590
    "BreakawayIdle",  // +4625
    "BreakawayWarn",  // +4639
    "BreakawayAppear",  // +4653
    "Breakaway",  // +4669
    "BreakawayDisappear",  // +4679
    "BreakawayGone",  // +4698
    "zNPCBTActiveCondition",  // +4712
    "zNPCBTAliveCondition",  // +4734
    "zNPCBTCheckPlayerTypeCondition",  // +4755
    "zNPCBTLastLiveEnemyCondition",  // +4786
    "zNPCBTCounterFailCondition",  // +4815
    "zNPCBTCounterNoneCondition",  // +4842
    "zNPCBTCounterSuccessCondition",  // +4869
    "zNPCBTCurrPlayerIsHumanCondition",  // +4899
    "zNPCBTCurrPlayerIsPlayerCondition",  // +4932
    "zNPCBTDefeatedCondition",  // +4966
    "zNPCBTDistanceFromFloatingObjectCondition",  // +4990
    "zNPCBTEnteredTrapCondition",  // +5032
    "zNPCBTExitedTrapCondition",  // +5059
    "zNPCBTHitCondition",  // +5085
    "zNPCBTHitPointCountCondition",  // +5104
    "zNPCBTInHearingAidRangeCondition",  // +5133
    "zNPCBTIsFloatingObjectActiveCondition",  // +5166
    "zNPCBTIsFloatingObjectValidCondition",  // +5204
    "zNPCBTIsPlayerValidCondition",  // +5241
    "zNPCBTIsToLockedPlayerSideCondition",  // +5270
    "zNPCBTLockedPlayerIsHumanCondition",  // +5306
    "zNPCBTNeedCombatCleanupCondition",  // +5341
    "zNPCBTNeedCombatTargetingCleanupCondition",  // +5374
    "zNPCBTOffScreenCondition",  // +5416
    "zNPCBTPlayerIsHumanCondition",  // +5441
    "zNPCBTPlayerIsPlayerCondition",  // +5470
    "zNPCBTPlayerIsTargetableCondition",  // +5500
    "zNPCBTPlayerIsBeingAttackedCondition",  // +5534
    "zNPCBTSpecialAbilityOnCondition",  // +5571
    "zNPCBTSwarmKilledByPlayerCondition",  // +5603
    "zNPCBTIsOnGroundCondition",  // +5638
    "zNPCBTMovingCondition",  // +5664
    "zNPCBT_GenericSpawner_NonstopSpawn_Condition",  // +5686
    "zNPCBT_GenericSpawner_CanSpawn_Condition",  // +5731
    "zNPCBT_GenericSpawner_CanSpawnLater_Condition",  // +5772
    "zNPCBTBlockedCondition",  // +5818
    "zNPCBTBlockedTypeCondition",  // +5841
    "zNPCBTDamagedCondition",  // +5868
    "zNPCBTDamagedByTypeCondition",  // +5891
    "zNPCBTDamagedByKnockbackCondition",  // +5920
    "zNPCBTInRPSAttackStateCondition",  // +5954
    "zNPCBTIsPlanktonShakingCondition",  // +5986
    "zNPCBTPokedCondition",  // +6019
    "zNPCBTNearbyProjectileCondition",  // +6040
    "zNPCBTBossSquidwardBlockPlayer",  // +6072
    "zNPCBTBossSquidwardDestroyCover",  // +6103
    "zNPCBTSBPlayerIsBuff",  // +6135
    "zNPCBTFacingPerceptionTargetCondition",  // +6156
    "zNPCBTCheckPerceptionCondition",  // +6194
    "zNPCBTCheckPerceptionTargetStatusCondition",  // +6225
    "zNPCBTCheckPerceptionTargetChangedCondition",  // +6268
    "zNPCBTCheckPerceptionTargetInWallnetCondition",  // +6312
    "zNPCBTIsInPuppetModeCondition",  // +6358
    "zNPCBTIsInWallnetCondition",  // +6388
    "SB_BungeeBallCordImmediateModel_Ref",  // +6415
    "Bungee_Ball_Pop",  // +6451
    "UID_BungeeCylinderGeom",  // +6467
    "TtzCam2Player_update",  // +6490
    "Et",  // +6511
    "",  // +6514
    "TtCastCameraRay2",  // +6515
    "Camera|Curve:POI_0|",  // +6532
    "Camera|Curve:POI_1|",  // +6552
    "Camera|Curve:POI_2|",  // +6572
    "Camera|Curve:POI_3|",  // +6592
    "Camera|Curve:POI_4|",  // +6612
    "Camera|Curve:POI_5|",  // +6632
    "Camera|Curve:Rail_0|",  // +6652
    "Camera|Curve:Rail_1|",  // +6673
    "Camera|Curve:Rail_2|",  // +6694
    "Camera|Curve:Rail_3|",  // +6715
    "Camera|Curve:Rail_4|",  // +6736
    "Camera|Curve:Rail_5|",  // +6757
    "GlobalPuckAttackCounterReference",  // +6778
    "GlobalSpinAttackCounterReference",  // +6811
    "COLLECTIBLE_HAPPY_POINT_COUNTER_reference",  // +6844
    "%6.4f",  // +6886
    "%8.4f  %6d  00:%02d:%02d:%02d  %s",  // +6892
    "SBB1",  // +6926
};


// zCombat.cpp in the WAD01 unity build, with the inlines the image kept here:
// zNPCCombat.h's IsDead, zProjectile.h's explosion queries and
// hkpAllCdBodyPairCollector.inl's reset and destructor.

extern "C" void* memset(void* dst, int val, unsigned long n);

namespace Sext {

enum eHitSource {
    eHitSourceEVENT = 0,
    eHitSourceGENERAL = 1,
    eHitSourceEXPLOSION = 2,
    eHitSourceZERO_POINT = 3,
    eHitSourceJUGGLE = 4,
    eHitSourceLASER_PLAYER = 5,
    eHitSourceLASER_ENEMY = 6,
    eHitSourceSURFACE = 7,
    eHitSourceDEATHPLANE = 8,
    eHitSourceKNOCKBACK = 9,
    eHitSourceENEMY_HEAVY = 10,
    eHitSourceENEMY_LIGHT = 11,
    eHitSourceENEMY_ELECTRIFY = 12,
    eHitSourceEVENT_KILL = 14,
    eHitSourceDOT = 16,
    eHitSourceDOT_DROWN = 19,
    eHitSourceDEATHPLANE_DROWN = 20,
    eHitSourceDEATHPLANE_FALL = 21,
    eHitSourceCAVE_WORM = 23,
    eHitSourceDART_GUN = 24,
    eHitSourceSNARE = 25,
    eHitSourcePLANT = 26,
    eHitSourceSPIN_ATTACK = 27,
    eHitSourceSPIN_NOHAMMER_ATTACK = 28,
    eHitSourceHAMMER_ATTACK = 29,
    eHitSourceHAMMER_CRUSHING_ATTACK = 30,
    eHitSourcePUCK_ATTACK = 31,
    eHitSourceSPIN_SPONGEBUFF_ATTACK = 32,
    eHitSourceHAMMER_SPONGEBUFF_ATTACK = 33,
    eHitSourcePUCK_SPONGEBUFF_ATTACK = 34,
    eHitSourceSPIN_POWERUP_ATTACK = 35,
    eHitSourceHAMMER_POWERUP_ATTACK = 36,
    eHitSourcePUCK_POWERUP_ATTACK = 37,
    eHitSourceTURRET1 = 38,
    eHitSourceSPLASH_SPIN = 39,
    eHitSourceSPLASH_HAMMER = 40,
    eHitSourceSPLASH_PUCK = 41,
    eHitSourceHIGH_FALL_SB = 42,
    eHitSourceSPLASH_NPCSpawner = 43,
    eHitSourcePROJECTILE = 44,
    eHitSourceSB_PROJECTILE_NPC_BOMB_EXPLOSION = 45,
    eHitSourcePLAYER_FLUID = 46,
    eHitSourcePLANKTON_STUN = 47,
    eHitSourcePLANKTON_DAMAGE = 48,
    eHitSourceSPONGEBUFF_CANDY = 49,
    eHitSourceBUNGEE = 50,
    eHitSourceSPONGEBUFF_BUNGEE = 51,
    eHitSourceSPLASHBACK = 52,
    eHitSourceJOSE_KNOCKBACK = 53,
    eHitSourceSB_ELECTRIC_ARC = 54,
    eHitSourceRUBBER_BAND = 55,
    eHitSourcePLAYER = 56,
    eHitSourceBUG_CATCH = 57,
    eHitSourceBIPLANE_PLAYER1 = 58,
    eHitSourceBIPLANE_PLAYER2 = 59,
    eHitSourceBIPLANE_PLAYER3 = 60,
    eHitSourceBIPLANE_PLAYER4 = 61,
    END_eHitSourceENUM = 62,
    eHitSource_FORCE_INT = 0x7FFFFFFF
};

enum eRPSAttackTypes {
    eRPSAttackType_None = 0,
    eRPSAttackType_Hammer = 1,
    eRPSAttackType_Spin = 2,
    eRPSAttackType_Puck = 3,
    eRPSAttackType_Miniboss_Hammer = 4,
    eRPSAttackType_Miniboss_Spin = 5,
    eRPSAttackType_Miniboss_Puck = 6,
    END_eRPSAttackType_ENUM = 7,
    eRPSAttackTypes_FORCE_INT = 0x7FFFFFFF
};

}  // namespace Sext

enum zHitTarget {
    zHT_GENERAL = 0,
    zHT_FRONT = 1,
    zHT_BACK = 2,
    zHitTarget_FORCE_INT = 0x7FFFFFFF
};

enum ePlayerName {
    PLAYER_SPONGEBOB = 6,
    PLAYER_PATRICK = 7,
    PLAYER_PLANKTON = 8,
    ePlayerName_FORCE_INT = 0x7FFFFFFF
};

// ---------------------------------------------------------------------------
// Math and models

namespace Math {

float rsqrt(float x);

}  // namespace Math

class xVec3 {
public:
    float length2() const;

    // The length as retail spells it: the squared length times its inverse
    // square root.
    float length() const {
        float len2 = length2();
        return len2 * Math::rsqrt(len2);
    }

    xVec3& assign(const xVec3& v) {
        x = v.x;
        y = v.y;
        z = v.z;
        return *this;
    }

    float x;
    float y;
    float z;
};

xVec3 operator-(const xVec3& a, const xVec3& b);

// xVec3's assignment, called out of line: reached by its symbol so this unit
// emits no copy of it and xVec3 stays a plain aggregate. The source by
// reference, so a returned temporary is passed as it is, as retail does.
extern "C" xVec3* __as__5xVec3FRC5xVec3(xVec3* self, const xVec3& other);

// A vector built in place from three floats: the image names Math::Vector's
// constructor for it, the one the linker kept.
extern "C" void __ct__Q24Math6VectorFfff(void* self, float x, float y, float z);

// Its arguments load z, y, x, as retail's do.
inline void xVec3Set(xVec3* v, float x, float y, float z) {
    __ct__Q24Math6VectorFfff(v, x, y, z);
}

class xMat3x3 {
public:
    xVec3 right;
    int flags;
    xVec3 up;
    unsigned int pad1;
    xVec3 at;
    unsigned int pad2;
};

class xMat4x3 : public xMat3x3 {
public:
    xVec3 pos;
    unsigned int pad3;
};

class xSphere {
public:
    xVec3 center;
    float r;
};

extern "C" xSphere* __as__7xSphereFRC7xSphere(xSphere* self, const xSphere* other);

void xMat3x3RMulVec(xVec3* o, const xMat3x3* m, const xVec3* v);
void xMat3x3GetScale(const xMat3x3* m, xVec3* scale);
void v3add(xVec3* c, xVec3* a, xVec3* b);

inline void xMat4x3Toworld(xVec3* o, const xMat4x3* m, const xVec3* v) {
    xVec3 t;
    xMat3x3RMulVec(&t, m, v);
    v3add(o, &t, (xVec3*)&m->pos);
}

class xAnimState {
public:
    unsigned char _pad0[0x10];
    unsigned int ID;
    unsigned int Flags;
    unsigned int UserFlags;
};

class xAnimSingle {
public:
    unsigned int SingleFlags;
    xAnimState* State;
    float Time;
    float CurrentSpeed;
};

class xAnimPlay {
public:
    unsigned char _pad0[0xC];
    xAnimSingle* Single;
};

namespace World {

class xOGModel {
public:
    xMat4x3 Mat;
    xVec3 Scale;
    xAnimPlay* Anim;
};

}  // namespace World

void xModelGetBoneMatScaled(xMat4x3& mat, const World::xOGModel& model, unsigned long bone);
void xModelGetBoneLocationNoScale(xVec3& loc, const World::xOGModel& model, unsigned long bone);

// ---------------------------------------------------------------------------
// Havok

enum HK_MEMORY_CLASS {
    HK_MEMORY_CLASS_AGENT = 32,
    HK_MEMORY_CLASS_FORCE_INT = 0x7FFFFFFF
};

class hkThreadMemory {
public:
    void deallocateChunkConstSize(void* p, int nbytes, HK_MEMORY_CLASS cl);
};

extern hkThreadMemory* hkThreadMemory__s_threadMemoryInstance;

class hkBool {
public:
    char m_bool;
};

class hkVector4 {
public:
    float dot3(const hkVector4& v) const;

    float x __attribute__((aligned(16)));
    float y;
    float z;
    float w;
};

// The dot products are hkVector4's, called on twelve-byte xVec3s: the linker
// folded xVec3's onto it.
inline float xVec3Dot3(const xVec3& a, const xVec3& b) {
    return ((const hkVector4*)&a)->dot3(*(const hkVector4*)&b);
}

template <class T>
class hkArray {
public:
    ~hkArray();

    T* m_data;
    int m_size;
    int m_capacityAndFlags;
};

class hkpShape;
class hkpCollidable;

class hkpCdBody {
public:
    const hkpCollidable* getRootCollidable() const;

    const hkpShape* m_shape;
    unsigned int m_shapeKey;
    void* m_motion;
    const hkpCdBody* m_parent;
};

class hkpShape {
public:
    unsigned char _pad0[0x10];
};

class hkMotionState;

class hkpCollidable : public hkpCdBody {
public:
    hkpCollidable(const hkpShape* shape, const hkMotionState* motionState, int type);

    signed char m_ownerOffset;
    unsigned char m_forceCollideOntoPpu;
    unsigned short m_shapeSizeOnSpu;
    unsigned int m_broadPhaseId;
    signed char m_broadPhaseType;
    signed char m_broadPhaseOwnerOffset;
    signed char m_objectQualityType;
    unsigned int m_collisionFilterInfo;
    unsigned char _pad20[0x50 - 0x20];
};

class hkpPropertyValue {
public:
    int getInt() const { return (int)m_data; }

    unsigned long long m_data;
};

class hkpWorldObject {
public:
    enum MtChecks {
        MULTI_THREADING_CHECKS_ENABLE = 0,
        MULTI_THREADING_CHECKS_IGNORE = 1,
        MtChecks_FORCE_INT = 0x7FFFFFFF
    };

    hkpPropertyValue getProperty(unsigned int key,
                                 MtChecks mtCheck = MULTI_THREADING_CHECKS_ENABLE) const;

    unsigned char _pad0[0xC];
    unsigned long m_userData;
};

class hkpRigidBody : public hkpWorldObject {};
class hkpPhantom : public hkpWorldObject {};

hkpRigidBody* hkGetRigidBody(const hkpCollidable* collidable);
hkpPhantom* hkGetPhantom(const hkpCollidable* collidable);

class hkpRootCdBodyPair {
public:
    const hkpCollidable* m_rootCollidableA;
    unsigned int m_shapeKeyA;
    const hkpCollidable* m_rootCollidableB;
    unsigned int m_shapeKeyB;
};

// hkInplaceArray<hkpRootCdBodyPair, 16>. The image folded the destructor of
// hkArray<hkpRootCdBodyPair> onto hkArray<hkVector4>'s (both elements 16
// bytes), and the collector destructor's branch names that one. A template,
// so its implicit destructor is not emitted where nothing calls it.
template <class T, int N>
class hkInplaceArrayFolded : public hkArray<hkVector4> {
public:
    T m_storage[N];
};

class hkpCdBodyPairCollector {
public:
    hkpCdBodyPairCollector() { reset(); }
    virtual ~hkpCdBodyPairCollector() {}
    virtual void addCdBodyPair(const hkpCdBody& bodyA, const hkpCdBody& bodyB) = 0;
    virtual void reset();

    hkBool m_earlyOut;
};

class hkpAllCdBodyPairCollector : public hkpCdBodyPairCollector {
public:
    // Its own size: the destructors of the collectors derived from it pass
    // 0x114 too.
    void operator delete(void* p) {
        if (p) {
            hkThreadMemory__s_threadMemoryInstance->deallocateChunkConstSize(
                p, sizeof(hkpAllCdBodyPairCollector), HK_MEMORY_CLASS_AGENT);
        }
    }

    hkpAllCdBodyPairCollector() {
        m_hits.m_data = (hkVector4*)m_hits.m_storage;
        m_hits.m_size = 0;
        m_hits.m_capacityAndFlags = (int)(16 | 0x80000000);
        reset();
    }

    // Declared first and defined elsewhere, so the table stays where retail
    // keeps it.
    virtual void addCdBodyPair(const hkpCdBody& bodyA, const hkpCdBody& bodyB);
    virtual ~hkpAllCdBodyPairCollector();
    virtual void reset();

    hkInplaceArrayFolded<hkpRootCdBodyPair, 16> m_hits;
};

class hkpConvexShape : public hkpShape {
public:
    float m_radius;
};

class hkpSphereShape : public hkpConvexShape {
public:
    hkpSphereShape(float radius);

    unsigned int m_pad16[3];
};

class hkpCapsuleShape : public hkpConvexShape {
public:
    hkpCapsuleShape(const hkVector4& vertexA, const hkVector4& vertexB, float radius);

    hkVector4 m_vertexA;
    hkVector4 m_vertexB;
};

extern "C" void __ct__Q24Math8Matrix33Fv(void* self);

class hkQuaternion {
public:
    hkVector4 m_vec;
};

// Its constructor is the empty one the linker folded onto Matrix33's.
class hkMotionState {
public:
    hkMotionState() { __ct__Q24Math8Matrix33Fv(this); }

    void initMotionState(const hkVector4& position, const hkQuaternion& rotation);

    hkVector4 m_data[11];
};

namespace Math {

// The four-float vector the Havok temporaries are built with: retail passes
// on the reference Assign returns.
class Vector4 {
public:
    Vector4() {}

    Vector4& Assign(float x, float y, float z, float w);

    float x __attribute__((aligned(16)));
    float y;
    float z;
    float w;
};

}  // namespace Math

class hkpCollisionInput {};

class hkpWorld {
public:
    void getPenetrations(const hkpCollidable* collA, const hkpCollisionInput& input,
                         hkpCdBodyPairCollector& collector) const;

    unsigned char _pad0[0x74];
    hkpCollisionInput* m_collisionInput;
};

hkpWorld* xHavok_GetWorld();
void xHavok_GetPenetrationInformation(const hkpCollidable* collA, const hkpCollidable* collB,
                                      hkVector4& hitLoc, hkVector4& hitNorm);

// ---------------------------------------------------------------------------
// Entities

class xBase {
public:
    unsigned char _pad0[0x20];
    unsigned int baseType;
    unsigned char UNUSED_linkCount;
    unsigned char assertFlags;
    unsigned short baseFlags;
    unsigned char _pad28[0x34 - 0x28];
};

class xEnt : public xBase {
public:
    World::xOGModel* model;
    unsigned char _pad38[0xBC - 0x38];
};

xVec3 xEntGetCenter(const xEnt* ent);
xBase* zSceneFindObject(unsigned long long id);

class zPlayerInput;

namespace zPlayerInputNS {

zPlayerInput* GetPadAtGamePort(int port);

}  // namespace zPlayerInputNS

namespace xRumble {

class emitterBase {
public:
    virtual void _v0();
    virtual void _v1();
    virtual void _v2();
    virtual void _v3();
    virtual void _v4();
    virtual void _v5();
    virtual void _v6();
    virtual void _v7();
    virtual void _v8();
    virtual void _v9();
    virtual void _v10();
    virtual void _v11();
    virtual void _v12();
    virtual void _v13();
    virtual void _v14();
    virtual void _v15();
    virtual void _v16();
    virtual void _v17();
    virtual void _v18();
    virtual void _v19();
    virtual void _v20();
    virtual void _v21();
    virtual void _v22();
    virtual void SetScale(float scale);
};

class Manager {
public:
    static Manager* Get();

    void Add(unsigned int flags, zPlayerInput* input, emitterBase* emitter);
};

}  // namespace xRumble

class zCombat;
class zNPCCombat;

class zPlayerActionManager {
public:
    unsigned int GetCurrentActionID() const;

    unsigned char _pad0[0x38];
};

namespace Graphics {

// zPlayerActionManager's action lookup, which the linker folded onto this.
class ModelPrototype {
public:
    void* GetBuilder(int i);
};

}  // namespace Graphics

class zPlayer : public xEnt {
public:
    zCombat* GetCombat();

    unsigned char _padBC[0xC0 - 0xBC];
    zPlayerActionManager actionManager;
    unsigned char _padF8[0x2EC - 0xF8];
    ePlayerName eName;
};

class zBoardPlayer : public zPlayer {
public:
    static zBoardPlayer* GetInstance();

    unsigned char _pad2F0[0x8B0 - 0x2F0];
    int powerupModelState;
};

class zSBPlayerHammerAttack {
public:
    void ReportHit(xBase* target, xVec3 hitLoc);
};

class zHitButton : public xBase {
public:
    bool IsWinding();

    unsigned char _pad34[0x4C - 0x34];
    bool pressed;
};

class NPCTemplate {
public:
    unsigned char _pad0[0x80];
    bool hammerReportsHit;
};

class zNPCTemplate {
public:
    NPCTemplate* templateAsset;
};

class zNPCBase {
public:
    unsigned char _pad0[0x78];
    zNPCTemplate* npcTemplate;
    unsigned char _pad7C[0xA8 - 0x7C];
    zNPCCombat* npcCombat;
};

class zNPCEntity : public xEnt {
public:
    zNPCBase* owner;
};

class zScene {
public:
    unsigned int sceneID;
};

class zGlobals {
public:
    unsigned char _pad0[0x43C];
    zScene* sceneCur;
};

extern zGlobals globals;

// A scene's four-letter tag, spelled out of the string at run time.
inline unsigned int BoardSceneTag(const char* s) {
    return (s[0] << 24) | (s[1] << 16) | (s[2] << 8) | s[3];
}

// ---------------------------------------------------------------------------
// Combat

class xHierarchyBoundInitData;

class xHierarchyNode {
public:
    xSphere sphere;
    unsigned short userData;
    signed char bone;
};

class xHierarchyBound {
public:
    xSphere master;
    xHierarchyNode* nodes;
    unsigned char count;
    unsigned char maxCount;
    signed char masterBone;
};

int xHierarchyBoundSetup(xHierarchyBound* bound, const xHierarchyBoundInitData* initData,
                         unsigned int count, void* data);
int xHierarchyBoundSetup(xHierarchyBound* bound, unsigned int count, void* data);
void xHierarchyBoundInit(xHierarchyBound* bound, const xHierarchyBoundInitData* initData,
                         float radius, int count, float scale);

class EventActionHit {
public:
    float damage;
    Sext::eHitSource hitSource;
    float knockback[3];
    bool SentByDestructable;
};

class Time {
public:
    long long ticks;
};

class sphereInfo {
public:
    xSphere sphere;
    xSphere previousSphere;
};

class hitBoneInfo {
public:
    float radius;
    unsigned short bone;
    xVec3 boneOffset;
};

class RumbleEffectParams {
public:
    float startTime;
    unsigned long long emitterID;
    float SB_NMEHammerRumbleMax;
    float SB_NMEHammerRumbleMin;
    float SB_NMEHammerRumbleMaxDist;
    float SB_NMEHammerRumbleMinDist;
};

class zCombatAttack {
public:
    unsigned int state;
    float attackStart;
    float attackEnd;
    float attackRadius;
    hitBoneInfo hitBones[6];
    float damage;
    int hitFilter;
    unsigned short flags;
    Sext::eHitSource source;
    bool hitsBSP;
    float impact;
    unsigned short effect;
    unsigned short hitEffect;
    float effectStart;
    float effectEnd;
    unsigned int chainStateID;
    RumbleEffectParams rumbleEffect;
    void (*hitCB)();
};

class zCombatHitSpot {
public:
    void Update(unsigned char profile);

    // Retail tests the facing as a bool value (mfcr), not a branch.
    bool FacesHit(const xVec3& hitNorm) const {
        return xVec3Dot3(hitNorm, direction) > cosAngle;
    }

    void* asset;
    xEnt* owner;
    xVec3 boneOffset;
    xVec3 localDirection;
    float cosAngle;
    bool active;
    xVec3 position;
    xVec3 direction;
    const hkpWorldObject* havokPhantom;
};

class zCombat {
public:
    class hkpCombatCdBodyPairCollector : public hkpAllCdBodyPairCollector {
    public:
        virtual void __vtable_anchor();
        hkpCombatCdBodyPairCollector();
        virtual ~hkpCombatCdBodyPairCollector();
        virtual void addCdBodyPair(const hkpCdBody& bodyA, const hkpCdBody& bodyB);
    };

    class hkpCombatCdBodyPairCollectorNoBSP : public hkpCombatCdBodyPairCollector {
    public:
        virtual void __vtable_anchor();
        hkpCombatCdBodyPairCollectorNoBSP();
        virtual ~hkpCombatCdBodyPairCollectorNoBSP();
        virtual void addCdBodyPair(const hkpCdBody& bodyA, const hkpCdBody& bodyB);
    };

    int Setup(const xHierarchyBoundInitData* initData, unsigned int count, void* data);
    void Init(const xHierarchyBoundInitData* initData, float boundRadius, int boundCount,
              const zCombatAttack* stateTable, unsigned short stateTableSize,
              zCombatHitSpot* hitSpots, unsigned short hitSpotCount, float hitPoints,
              float boundScale, const char* modelName, const char** boneNames,
              int boneCount);
    bool ShouldRunEffect();
    void ClearRunningAttack(xEnt* attacker);
    void StartNewAttack(xEnt* attacker, unsigned int animationState);
    float GetRunningAttackLastTime();
    zCombatAttack* FindAttackState(unsigned int state);
    void UpdateSphere(xSphere* sphere, int bone, World::xOGModel* model,
                      const xVec3* boneOffset);
    void PostUpdate(xEnt* ent, float dt);
    void CheckForHit(xEnt* attacker);
    void ProcessHitCollection(xEnt* attacker, hkpAllCdBodyPairCollector* collector);
    void SendObjectHit(xEnt* attacker, xBase* target, zHitTarget hitTarget,
                       const xVec3* hitLoc, const xVec3* hitNorm, bool report);
    bool CheckHitValid(const xBase* attackerBase, const hkpWorldObject* attacked,
                       const xBase* attackedBase, const xVec3& hit_loc,
                       const xVec3& hit_norm) const;
    bool CheckHitSpotActive(const hkpWorldObject* attacked) const;
    bool CheckHitSpots(const hkpWorldObject* attacked, const xVec3& hit_norm) const;

    xHierarchyBound bounds;
    unsigned int lastBoundUpdateTime;
    EventActionHit collParams;
    float currentHitPoints;
    float maximumHitPoints;
    unsigned short stateTableSize;
    zCombatAttack* stateTable;
    xAnimState* animationState;
    zCombatAttack* runningAttack;
    float runningAttackTimer;
    xEnt* lastNPCDamaged;
    bool disableMovement;
    bool runningEffect;
    bool runningBlur;
    bool hitting;
    bool hitEnv;
    bool hitObject;
    bool forceReset;
    unsigned char hitObjectCount;
    signed char firstValidHitObjectIndex;
    signed char lastValidHitObjectIndex;
    xBase* hitObjects[24];
    Time hitObjectsTime[24];
    float currentDamage;
    float lastHitDamage;
    Sext::eHitSource lastHitSource;
    zHitTarget lastHitTarget;
    sphereInfo location[6];
    unsigned int effectParam;
    zCombatHitSpot* hitSpots;
    unsigned short hitSpotCount;
    unsigned short hitProfile;
};

class zNPCCombat {
public:
    bool IsDead() const;

    unsigned char _pad0[0x10];
    zCombat baseCombat;
};

zHitTarget zCombatGetHitTarget(xVec3* pos, xEnt* target);
zHitTarget zCombatGetHitTarget(xEnt* attacker, xEnt* target);
Sext::eHitSource zCombatGetBaseAttackSB(Sext::eHitSource source);
Sext::eHitSource zCombatGetBaseAttackSB(Sext::eRPSAttackTypes attackType);

// ---------------------------------------------------------------------------
// Projectiles

class ProjectileAsset {
public:
    unsigned char _pad0[0x10C];
    int explosionType;
    float explosionRadius;
    float explosionDamage;
};

class zProjectile {
public:
    // Data first: mwcc puts the table pointer where the first virtual is
    // declared, and retail has it at +0x190.
    unsigned char _pad0[0xC];
    ProjectileAsset* projectileAsset;
    unsigned char _pad10[0x190 - 0x10];

    // The slots in front of IsExplosive, declared and defined nowhere here so
    // the table stays where retail keeps it.
    virtual void _v0();
    virtual void _v1();
    virtual void _v2();
    virtual void _v3();
    virtual void _v4();
    virtual void _v5();
    virtual void _v6();
    virtual bool IsExplosive();
    virtual float GetExplosiveDamage();

    unsigned int GetExplosionHitSource();
};

// ---------------------------------------------------------------------------
// The collector destructors used in line below. The Havok collector's is
// defined at the foot so every one of these still branches to it.

zCombat::hkpCombatCdBodyPairCollector::~hkpCombatCdBodyPairCollector() {}

// ---------------------------------------------------------------------------
// zCombat.cpp

#define COMBAT_MAX(a, b) ((a) > (b) ? (a) : (b))
#define COMBAT_MIN(a, b) ((a) < (b) ? (a) : (b))

// PostUpdate, CheckForHit and ProcessHitCollection sit ahead of every function
// of this file they call: retail branches to each of them.
void zCombat::PostUpdate(xEnt* ent, float dt) {
    for (int i = 0; i < bounds.count; i++) {
        xHierarchyNode& node = bounds.nodes[i];
        xModelGetBoneLocationNoScale(node.sphere.center, *ent->model, node.bone);
    }

    if (bounds.masterBone == -1) {
        __as__5xVec3FRC5xVec3(&bounds.master.center, xEntGetCenter(ent));
    } else {
        xModelGetBoneLocationNoScale(bounds.master.center, *ent->model, bounds.masterBone);
    }

    hitting = false;
    dt *= ent->model->Anim->Single->CurrentSpeed;

    if (runningAttack && runningAttack->rumbleEffect.emitterID != 0 &&
        runningAttackTimer < runningAttack->rumbleEffect.startTime &&
        runningAttackTimer + dt >= runningAttack->rumbleEffect.startTime) {
        xRumble::emitterBase* rumble =
            (xRumble::emitterBase*)zSceneFindObject(runningAttack->rumbleEffect.emitterID);

        if (zCombatGetBaseAttackSB(runningAttack->source) == Sext::eHitSourceHAMMER_ATTACK) {
            if (ent->baseType == 0x55) {
                rumble->SetScale(0.15f);
            } else if (ent->baseType == 0x38) {
                xVec3 PlayerToNPC =
                    ent->model->Mat.pos - zBoardPlayer::GetInstance()->model->Mat.pos;
                float distAway = PlayerToNPC.length() -
                                 runningAttack->rumbleEffect.SB_NMEHammerRumbleMaxDist;
                static float distanceRange =
                    runningAttack->rumbleEffect.SB_NMEHammerRumbleMinDist -
                    runningAttack->rumbleEffect.SB_NMEHammerRumbleMaxDist;
                float percentageRumble = 1.0f - distAway / distanceRange;

                percentageRumble = COMBAT_MAX(0.0f, COMBAT_MIN(percentageRumble, 1.0f));

                float rumbleRange = runningAttack->rumbleEffect.SB_NMEHammerRumbleMax -
                                    runningAttack->rumbleEffect.SB_NMEHammerRumbleMin;

                rumble->SetScale(percentageRumble * rumbleRange +
                                 runningAttack->rumbleEffect.SB_NMEHammerRumbleMin);
            }
        }

        xRumble::Manager::Get()->Add(0, zPlayerInputNS::GetPadAtGamePort(0), rumble);
    }

    bool resetSphere = false;
    xAnimState* curState = ent->model->Anim->Single->State;

    if (animationState != curState || forceReset) {
        animationState = curState;

        if (curState->UserFlags & 0x08000000) {
            if (!runningAttack) {
                resetSphere = true;
            }

            zCombatAttack* temp = FindAttackState(curState->ID);

            if (!runningAttack || temp) {
                runningAttackTimer = ent->model->Anim->Single->Time;
                runningAttack = temp;
                currentDamage = runningAttack->damage;
            } else {
                runningAttackTimer += dt;
            }
        } else {
            if (curState->UserFlags & 0x04000000) {
                StartNewAttack(ent, curState->ID);
            } else {
                ClearRunningAttack(ent);
            }

            resetSphere = true;
        }
    } else if ((animationState->UserFlags & 0x08000000) && runningAttack &&
               animationState->ID != runningAttack->state) {
        runningAttackTimer += dt;
    } else {
        float newTime = ent->model->Anim->Single->Time;

        if (runningAttackTimer > newTime) {
            if (runningAttack && (runningAttack->flags & 0x8)) {
                if (runningAttack->state != animationState->ID) {
                    StartNewAttack(ent, animationState->ID);
                    resetSphere = true;
                } else {
                    hitObjectCount = 0;
                    firstValidHitObjectIndex = -1;
                    lastValidHitObjectIndex = -1;
                    hitEnv = false;
                    hitObject = false;
                    lastNPCDamaged = 0;
                }
            }
        } else if (runningAttack && runningAttack->chainStateID != 0 &&
                   newTime > GetRunningAttackLastTime()) {
            StartNewAttack(ent, runningAttack->chainStateID);
            resetSphere = true;
        }

        runningAttackTimer = ent->model->Anim->Single->Time;
    }

    if (hitSpots && hitSpotCount) {
        for (unsigned int i = 0; i < hitSpotCount; i++) {
            hitSpots[i].Update(hitProfile);
        }
    }

    if (runningAttack) {
        for (int i = 0; i < 6; i++) {
            if (runningAttack->hitBones[i].bone != 0xFFFF) {
                __as__7xSphereFRC7xSphere(&location[i].previousSphere, &location[i].sphere);

                if (runningAttack->hitBones[i].radius > 0.0f) {
                    location[i].sphere.r = runningAttack->hitBones[i].radius;
                } else {
                    location[i].sphere.r = runningAttack->attackRadius;
                }

                UpdateSphere(&location[i].sphere, runningAttack->hitBones[i].bone, ent->model,
                             &runningAttack->hitBones[i].boneOffset);

                if (resetSphere) {
                    __as__7xSphereFRC7xSphere(&location[i].previousSphere,
                                              &location[i].sphere);
                }
            }
        }

        if (runningAttackTimer >= runningAttack->attackStart &&
            (runningAttack->attackEnd <= runningAttack->attackStart ||
             runningAttackTimer <= runningAttack->attackEnd)) {
            if (runningAttack->damage >= 0.0f) {
                CheckForHit(ent);
            }
        }

        if (ShouldRunEffect()) {
            if (!runningEffect) {
                runningEffect = true;
            }
        } else if (runningEffect) {
            runningEffect = false;
        }
    }
}

void zCombat::CheckForHit(xEnt* attacker) {
    for (int i = 0; i < 6; i++) {
        if (runningAttack->hitBones[i].bone != 0xFFFF) {
            float radius = location[i].previousSphere.r;

            if (location[i].sphere.r > radius) {
                radius = location[i].sphere.r;
            }

            hkpWorld* havokWorld = xHavok_GetWorld();
            hkpCombatCdBodyPairCollectorNoBSP noBSPcollector;
            hkpCombatCdBodyPairCollector BSPcollector;
            float diffLen2 =
                (location[i].previousSphere.center - location[i].sphere.center).length2();

            if (diffLen2 < 1e-10f) {
                hkpSphereShape shape(radius);
                hkMotionState motionState;

                motionState.initMotionState(
                    (const hkVector4&)Math::Vector4().Assign(
                        location[i].previousSphere.center.x, location[i].previousSphere.center.y,
                        location[i].previousSphere.center.z, 0.0f),
                    (const hkQuaternion&)Math::Vector4().Assign(0.0f, 0.0f, 0.0f, 1.0f));

                hkpCollidable collidable(&shape, &motionState, 0);

                collidable.m_collisionFilterInfo = runningAttack->hitFilter;

                const hkpCollisionInput* input = havokWorld->m_collisionInput;

                if (runningAttack->hitsBSP) {
                    havokWorld->getPenetrations(&collidable, *input, BSPcollector);
                    ProcessHitCollection(attacker, &BSPcollector);
                } else {
                    havokWorld->getPenetrations(&collidable, *input, noBSPcollector);
                    ProcessHitCollection(attacker, &noBSPcollector);
                }
            } else {
                hkpCapsuleShape shape(
                    (const hkVector4&)Math::Vector4().Assign(
                        location[i].previousSphere.center.x, location[i].previousSphere.center.y,
                        location[i].previousSphere.center.z, 0.0f),
                    (const hkVector4&)Math::Vector4().Assign(
                        location[i].sphere.center.x, location[i].sphere.center.y,
                        location[i].sphere.center.z, 0.0f),
                    radius);
                hkMotionState motionState;

                motionState.initMotionState(
                    (const hkVector4&)Math::Vector4().Assign(0.0f, 0.0f, 0.0f, 0.0f),
                    (const hkQuaternion&)Math::Vector4().Assign(0.0f, 0.0f, 0.0f, 1.0f));

                hkpCollidable collidable(&shape, &motionState, 0);

                collidable.m_collisionFilterInfo = runningAttack->hitFilter;

                const hkpCollisionInput* input = havokWorld->m_collisionInput;

                if (runningAttack->hitsBSP) {
                    havokWorld->getPenetrations(&collidable, *input, BSPcollector);
                    ProcessHitCollection(attacker, &BSPcollector);
                } else {
                    havokWorld->getPenetrations(&collidable, *input, noBSPcollector);
                    ProcessHitCollection(attacker, &noBSPcollector);
                }
            }
        }
    }
}

void zCombat::ProcessHitCollection(xEnt* attacker, hkpAllCdBodyPairCollector* collector) {
    for (int i = 0; i < collector->m_hits.m_size; i++) {
        hkpRootCdBodyPair hit = ((hkpRootCdBodyPair*)collector->m_hits.m_data)[i];
        hkpRigidBody* hitRBA = hkGetRigidBody(hit.m_rootCollidableA);
        hkpRigidBody* hitRBB = hkGetRigidBody(hit.m_rootCollidableB);
        hkpPhantom* hitPhantomA = hkGetPhantom(hit.m_rootCollidableA);
        hkpPhantom* hitPhantomB = hkGetPhantom(hit.m_rootCollidableB);
        xBase* hitRBABase = 0;
        xBase* hitRBBBase = 0;
        xBase* hitPhantomABase = 0;
        xBase* hitPhantomBBase = 0;

        if (hitRBA) {
            hitRBABase = (xBase*)hitRBA->m_userData;

            if (hitRBABase == attacker) {
                hitRBABase = 0;
            }
        } else if (hitPhantomA) {
            hitPhantomABase = (xBase*)hitPhantomA->m_userData;

            if (hitPhantomABase == attacker) {
                hitPhantomABase = 0;
            }
        }

        if (hitRBB) {
            hitRBBBase = (xBase*)hitRBB->m_userData;

            if (hitRBBBase == attacker) {
                hitRBBBase = 0;
            }
        } else if (hitPhantomB) {
            hitPhantomBBase = (xBase*)hitPhantomB->m_userData;

            if (hitPhantomBBase == attacker) {
                hitPhantomBBase = 0;
            }
        }

        xVec3 hit_loc;
        xVec3 hit_norm;

        if (hitRBABase || hitRBBBase || hitPhantomABase || hitPhantomBBase) {
            hkVector4 hk_hit_loc;
            hkVector4 hk_hit_norm;

            xHavok_GetPenetrationInformation(hit.m_rootCollidableA, hit.m_rootCollidableB,
                                             hk_hit_loc, hk_hit_norm);
            xVec3Set(&hit_loc, hk_hit_loc.x, hk_hit_loc.y, hk_hit_loc.z);
            xVec3Set(&hit_norm, hk_hit_norm.x, hk_hit_norm.y, hk_hit_norm.z);
        }

        if (hitRBABase && CheckHitValid(attacker, hitRBA, hitRBABase, hit_loc, hit_norm)) {
            zHitTarget hitTarget;

            if ((hitRBABase->baseFlags >> 5) & 1) {
                hitTarget = zCombatGetHitTarget(attacker, (xEnt*)hitRBABase);
            } else {
                hitTarget = zHT_GENERAL;
            }

            SendObjectHit(attacker, hitRBABase, hitTarget, &hit_loc, &hit_norm, true);
        } else if (hitPhantomABase &&
                   CheckHitValid(attacker, hitPhantomA, hitPhantomABase, hit_loc, hit_norm)) {
            zHitTarget hitTarget;

            if ((hitPhantomABase->baseFlags >> 5) & 1) {
                hitTarget = zCombatGetHitTarget(attacker, (xEnt*)hitPhantomABase);
            } else {
                hitTarget = zHT_GENERAL;
            }

            SendObjectHit(attacker, hitPhantomABase, hitTarget, &hit_loc, &hit_norm, true);
        }

        if (hitRBBBase && CheckHitValid(attacker, hitRBB, hitRBBBase, hit_loc, hit_norm)) {
            zHitTarget hitTarget;

            if ((hitRBBBase->baseFlags >> 5) & 1) {
                hitTarget = zCombatGetHitTarget(attacker, (xEnt*)hitRBBBase);
            } else {
                hitTarget = zHT_GENERAL;
            }

            SendObjectHit(attacker, hitRBBBase, hitTarget, &hit_loc, &hit_norm, true);
        } else if (hitPhantomBBase &&
                   CheckHitValid(attacker, hitPhantomB, hitPhantomBBase, hit_loc, hit_norm)) {
            zHitTarget hitTarget;

            if ((hitPhantomBBase->baseFlags >> 5) & 1) {
                hitTarget = zCombatGetHitTarget(attacker, (xEnt*)hitPhantomBBase);
            } else {
                hitTarget = zHT_GENERAL;
            }

            SendObjectHit(attacker, hitPhantomBBase, hitTarget, &hit_loc, &hit_norm, true);
        }
    }
}

int zCombat::Setup(const xHierarchyBoundInitData* initData, unsigned int count, void* data) {
    return initData ? xHierarchyBoundSetup(&bounds, initData, 0, data)
                    : xHierarchyBoundSetup(&bounds, count, data);
}

void zCombat::Init(const xHierarchyBoundInitData* initData, float boundRadius, int boundCount,
                   const zCombatAttack* stateTable, unsigned short stateTableSize,
                   zCombatHitSpot* hitSpots, unsigned short hitSpotCount, float hitPoints,
                   float boundScale, const char* modelName, const char** boneNames,
                   int boneCount) {
    if (initData) {
        xHierarchyBoundInit(&bounds, initData, boundRadius, boundCount, boundScale);
    } else {
        memset(&bounds, 0, sizeof(xHierarchyBound));
    }

    this->stateTable = (zCombatAttack*)stateTable;
    this->stateTableSize = stateTableSize;
    this->hitSpots = hitSpots;
    this->hitSpotCount = hitSpotCount;
    hitProfile = 0;
    currentHitPoints = hitPoints;
    maximumHitPoints = hitPoints;
    animationState = 0;
    runningAttack = 0;
    forceReset = false;
}

// The timer by reference: retail reads it at each comparison.
inline bool AttackEffectStarted(const zCombatAttack* attack, const float& timer) {
    return attack->effectStart != attack->effectEnd && timer >= attack->effectStart;
}

inline bool AttackEffectNotEnded(const zCombatAttack* attack, const float& timer) {
    return attack->effectEnd <= attack->effectStart || timer <= attack->effectEnd;
}

#pragma push
#pragma always_inline on
bool zCombat::ShouldRunEffect() {
    return AttackEffectStarted(runningAttack, runningAttackTimer) &&
           AttackEffectNotEnded(runningAttack, runningAttackTimer);
}
#pragma pop

void zCombat::ClearRunningAttack(xEnt* attacker) {
    runningAttackTimer = 0.0f;
    hitObjectCount = 0;
    firstValidHitObjectIndex = -1;
    lastValidHitObjectIndex = -1;
    hitEnv = false;
    hitObject = false;
    runningAttack = 0;
    runningEffect = false;
    runningBlur = false;
    lastNPCDamaged = 0;
    forceReset = false;
}

#pragma dont_inline on
void zCombat::StartNewAttack(xEnt* attacker, unsigned int animationState) {
    ClearRunningAttack(attacker);

    runningAttackTimer = attacker->model->Anim->Single->Time;
    runningAttack = FindAttackState(animationState);

    if (runningAttack) {
        currentDamage = runningAttack->damage;
    }
}
#pragma dont_inline off

float zCombat::GetRunningAttackLastTime() {
    if (runningAttack) {
        return COMBAT_MAX(runningAttack->attackEnd,
                          COMBAT_MAX(runningAttack->effectEnd,
                                     runningAttack->rumbleEffect.startTime));
    }

    return -3.4028235e+38f;
}

// NEAR MISS: 11 of 15 words differ; ours computes &stateTable[i] ahead of the
// compare and indexes (index, base), retail indexes (base, index) and adds
// only inside the if. Six spellings of the test and the address gave the same.
zCombatAttack* zCombat::FindAttackState(unsigned int state) {
    zCombatAttack* result = 0;

    for (int i = 0; i < stateTableSize; i++) {
        if (state == stateTable[i].state) {
            result = &stateTable[i];
        }
    }

    return result;
}

unsigned int zCombatGetEvent(Sext::eHitSource source) {
    unsigned int toEvent;

    switch (source) {
    case Sext::eHitSourceSPIN_ATTACK:
    case Sext::eHitSourceSPIN_NOHAMMER_ATTACK:
    case Sext::eHitSourceSPIN_SPONGEBUFF_ATTACK:
    case Sext::eHitSourceSPIN_POWERUP_ATTACK:
        toEvent = 0xA6F1954E;
        break;
    case Sext::eHitSourceHAMMER_ATTACK:
    case Sext::eHitSourceHAMMER_SPONGEBUFF_ATTACK:
    case Sext::eHitSourceHAMMER_POWERUP_ATTACK:
        toEvent = 0x8EF11AF2;
        break;
    case Sext::eHitSourcePUCK_ATTACK:
    case Sext::eHitSourcePUCK_SPONGEBUFF_ATTACK:
    case Sext::eHitSourcePUCK_POWERUP_ATTACK:
        toEvent = 0xBE49B97D;
        break;
    case Sext::eHitSourceTURRET1:
        toEvent = 0x5466E841;
        break;
    case Sext::eHitSourceSPLASH_HAMMER:
        toEvent = 0xB11B8E5E;
        break;
    case Sext::eHitSourceSPLASH_PUCK:
        toEvent = 0x5621F781;
        break;
    case Sext::eHitSourceSPONGEBUFF_CANDY:
        toEvent = 0x81A27AAB;
        break;
    case Sext::eHitSourceSPONGEBUFF_BUNGEE:
        toEvent = 0xB9B37786;
        break;
    default:
        toEvent = 0x00130037;
        break;
    }

    return toEvent;
}

Sext::eHitSource zCombatGetBaseAttackSB(Sext::eHitSource source) {
    Sext::eHitSource baseSource = source;

    switch (source) {
    case Sext::eHitSourceSPIN_ATTACK:
    case Sext::eHitSourceSPIN_NOHAMMER_ATTACK:
    case Sext::eHitSourceSPIN_SPONGEBUFF_ATTACK:
    case Sext::eHitSourceSPIN_POWERUP_ATTACK:
        baseSource = Sext::eHitSourceSPIN_ATTACK;
        break;
    case Sext::eHitSourceHAMMER_ATTACK:
    case Sext::eHitSourceHAMMER_CRUSHING_ATTACK:
    case Sext::eHitSourceHAMMER_SPONGEBUFF_ATTACK:
    case Sext::eHitSourceHAMMER_POWERUP_ATTACK:
        baseSource = Sext::eHitSourceHAMMER_ATTACK;
        break;
    case Sext::eHitSourcePUCK_ATTACK:
    case Sext::eHitSourcePUCK_SPONGEBUFF_ATTACK:
    case Sext::eHitSourcePUCK_POWERUP_ATTACK:
    case Sext::eHitSourceTURRET1:
        baseSource = Sext::eHitSourcePUCK_ATTACK;
        break;
    case Sext::eHitSourceSPONGEBUFF_CANDY:
        baseSource = Sext::eHitSourceSPONGEBUFF_CANDY;
        break;
    case Sext::eHitSourceSPONGEBUFF_BUNGEE:
        baseSource = Sext::eHitSourceSPONGEBUFF_BUNGEE;
        break;
    }

    return baseSource;
}

Sext::eHitSource zCombatGetBaseAttackSB(Sext::eRPSAttackTypes attackType) {
    Sext::eHitSource baseSource;

    switch (attackType) {
    case Sext::eRPSAttackType_Spin:
    case Sext::eRPSAttackType_Miniboss_Spin:
        baseSource = Sext::eHitSourceSPIN_ATTACK;
        break;
    case Sext::eRPSAttackType_Hammer:
    case Sext::eRPSAttackType_Miniboss_Hammer:
        baseSource = Sext::eHitSourceHAMMER_ATTACK;
        break;
    case Sext::eRPSAttackType_Puck:
    case Sext::eRPSAttackType_Miniboss_Puck:
        baseSource = Sext::eHitSourcePUCK_ATTACK;
        break;
    default:
        baseSource = Sext::eHitSourceGENERAL;
        break;
    }

    return baseSource;
}

bool zNPCCombat::IsDead() const {
    return baseCombat.currentHitPoints <= 0.0f;
}

zHitTarget zCombatGetHitTarget(xEnt* attacker, xEnt* target) {
    zHitTarget hitTarget = zHT_GENERAL;

    if (attacker) {
        hitTarget = zCombatGetHitTarget(&attacker->model->Mat.pos, target);
    }

    return hitTarget;
}

zCombat* zCombatGetFrom(xEnt* ent) {
    if (ent->baseType == 0x55) {
        return ((zPlayer*)ent)->GetCombat();
    }

    if (ent->baseType == 0x38) {
        zNPCCombat* npcCombat = ((zNPCEntity*)ent)->owner->npcCombat;

        if (npcCombat) {
            return &npcCombat->baseCombat;
        }
    }

    return 0;
}

void zCombat::UpdateSphere(xSphere* sphere, int bone, World::xOGModel* model,
                           const xVec3* boneOffset) {
    xMat4x3 boneMat;

    xModelGetBoneMatScaled(boneMat, *model, bone);
    xMat4x3Toworld(&sphere->center, &boneMat, boneOffset);

    xVec3 boneScale;

    xMat3x3GetScale(&boneMat, &boneScale);
    sphere->r *= 0.333333f * (boneScale.x + boneScale.y + boneScale.z);
}

bool zProjectile::IsExplosive() {
    return projectileAsset->explosionType == 0;
}

float zProjectile::GetExplosiveDamage() {
    return IsExplosive() ? projectileAsset->explosionDamage : 0.0f;
}

#pragma dont_inline on
unsigned int zProjectile::GetExplosionHitSource() { return 0x00000002u; }
zCombat::hkpCombatCdBodyPairCollectorNoBSP::hkpCombatCdBodyPairCollectorNoBSP() : zCombat::hkpCombatCdBodyPairCollector() {}
#pragma dont_inline off

zCombat::hkpCombatCdBodyPairCollectorNoBSP::~hkpCombatCdBodyPairCollectorNoBSP() {}

// Retail has both base constructors in line.
#pragma push
#pragma always_inline on
zCombat::hkpCombatCdBodyPairCollector::hkpCombatCdBodyPairCollector() {}
#pragma pop

#pragma dont_inline on
void hkpAllCdBodyPairCollector::reset() {
    m_hits.m_size = 0;
    m_earlyOut.m_bool = 0;
}
#pragma dont_inline off

// NEAR MISS: 7 of 94 words differ; npcCombat and the useHitSpots flag take r28
// and r29 the other way round. Tried: the flag through a bool inline, declared
// first, npcCombat declared in the condition or not a local at all. The
// region puts the vector assign in line (retail's x->f2, z->f0 float moves).
#pragma push
#pragma always_inline on
bool zCombat::CheckHitValid(const xBase* attackerBase, const hkpWorldObject* attacked,
                            const xBase* attackedBase, const xVec3& hit_loc,
                            const xVec3& hit_norm) const {
    if ((attackedBase->baseFlags & 0x20) && attackedBase->baseType == 0x38) {
        zNPCCombat* npcCombat = ((const zNPCEntity*)attackedBase)->owner->npcCombat;

        if (npcCombat) {
            bool validHit = true;
            bool useHitSpots = attacked && attacked->getProperty(888).getInt() == 1;

            if (useHitSpots) {
                validHit = npcCombat->baseCombat.CheckHitSpots(attacked, hit_norm);
            } else if (npcCombat->baseCombat.hitProfile != 0) {
                validHit = false;
            }

            if (!validHit && attackerBase && attackerBase->baseType == 0x55 &&
                ((const zPlayer*)attackerBase)->eName == PLAYER_SPONGEBOB) {
                zBoardPlayer* sbPlayer = (zBoardPlayer*)attackerBase;

                if (sbPlayer->actionManager.GetCurrentActionID() == 0x15) {
                    const zNPCEntity* npcEnt = (const zNPCEntity*)attackedBase;

                    if (npcEnt->owner->npcTemplate->templateAsset->hammerReportsHit) {
                        xVec3 hitLoc;
                        hitLoc.assign(hit_loc);

                        ((zSBPlayerHammerAttack*)((Graphics::ModelPrototype*)&sbPlayer
                                                      ->actionManager)
                             ->GetBuilder(sbPlayer->actionManager.GetCurrentActionID()))
                            ->ReportHit((xBase*)attackedBase, hitLoc);
                    }
                }
            }

            return validHit;
        }
    }

    return true;
}
#pragma pop

// The scene tag and the facing test in line, as retail has them.
// NEAR MISS (CheckHitSpotActive): 5 of 57 words differ; the globals and
// @stringBase0 bases take r30 and r31 the other way round. Tried: either
// operand order of the tag test, globals declared ahead of the pool strings,
// the test through an inline.
#pragma push
#pragma always_inline on
bool zCombat::CheckHitSpotActive(const hkpWorldObject* attacked) const {
    for (unsigned int i = 0; i < hitSpotCount; i++) {
        if (BoardSceneTag("SBB1") == globals.sceneCur->sceneID && i == 0) {
            zBoardPlayer* sbPlayer = zBoardPlayer::GetInstance();

            if (sbPlayer->powerupModelState != 1) {
                continue;
            }
        }

        if (hitSpots[i].active && attacked == hitSpots[i].havokPhantom) {
            return true;
        }
    }

    return false;
}

bool zCombat::CheckHitSpots(const hkpWorldObject* attacked, const xVec3& hit_norm) const {
    const zCombatHitSpot* spot;

    for (unsigned int i = 0; i < hitSpotCount; i++) {
        spot = &hitSpots[i];

        if (spot->active && attacked == spot->havokPhantom && spot->FacesHit(hit_norm)) {
            return true;
        }
    }

    return false;
}
#pragma pop

void zCombat::hkpCombatCdBodyPairCollector::addCdBodyPair(const hkpCdBody& bodyA,
                                                          const hkpCdBody& bodyB) {
    const hkpCollidable* rootCollidable = bodyB.getRootCollidable();
    hkpRigidBody* body = hkGetRigidBody(rootCollidable);

    if (body) {
        xBase* bodyOwner = (xBase*)body->m_userData;

        if (bodyOwner) {
            if (bodyOwner->baseType == 0xE4 || bodyOwner->baseType == 0x6D) {
                return;
            }

            if (bodyOwner->baseType == 0x9A) {
                if (!(bodyOwner->baseFlags & 1)) {
                    return;
                }

                zHitButton* button = (zHitButton*)bodyOwner;

                if (button->pressed || button->IsWinding()) {
                    return;
                }
            }
        }
    }

    hkpAllCdBodyPairCollector::addCdBodyPair(bodyA, bodyB);
}

#pragma dont_inline on
void zCombat::hkpCombatCdBodyPairCollectorNoBSP::addCdBodyPair(const hkpCdBody& bodyA,
                                                               const hkpCdBody& bodyB) {
    const hkpCollidable* rootCollidable = bodyB.getRootCollidable();
    hkpRigidBody* body = hkGetRigidBody(rootCollidable);

    if (body) {
        xBase* bodyOwner = (xBase*)body->m_userData;

        if (bodyOwner && bodyOwner->baseType == 0x45) {
            return;
        }
    }

    hkpCombatCdBodyPairCollector::addCdBodyPair(bodyA, bodyB);
}
#pragma dont_inline off

// hkpAllCdBodyPairCollector.inl. Last, so each collector destructor above
// branches to it.
hkpAllCdBodyPairCollector::~hkpAllCdBodyPairCollector() {}
