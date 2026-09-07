// zBTConditionBuilder.cpp -- the behaviour-tree condition factory,
// read from the image with tools/disasm.py. The same shape as
// WAD01_1_1's action factory: one function template per owner, 60
// instantiations, a switch that turns a type id into a condition, and a
// Destroy that gives one back.
//
// EVERY FACT HERE IS ONE THE BYTES SAY: the `li r4,N` that is sizeof(T),
// the `stw` that puts the vtable pointer at +8, the slot each caller
// reads, and the constant each compare builds in front of its `beq`.
// What the members ARE is not in the image, so they are padding.
//
// The NPC side's Create takes the condition's NAME, and retail reaches
// it as one pooled base plus a baked-in offset -- so this unit needs the
// string pool its unity build had in front of it, which is what the
// generated .pool.h supplies. Without it every offset is small and
// wrong.
//
// Two things the bytes fixed. THE PLACEMENT NEW IS A CONDITIONAL
// EXPRESSION WITH THE NULL CASE FIRST, as everywhere else in this tree.
// And the asset setter Build calls is named zBTAction's: the condition's
// own would be `stw r4,0(r3); blr`, byte for byte the action's, and the
// image has one function under that name -- so the call is written
// through it.

#include "SB/GM/Engine/Game/zBTConditionBuilder.pool.h"

namespace Sext {
class ActionBase;
class ConditionBase;
}  // namespace Sext

class zBTClient;

namespace Memory {

enum eFactoryMemType { eFactoryMemType_ = 0x7FFFFFFF };

class Factory {
public:
    void* AllocMem(unsigned int size, eFactoryMemType type);
    void DeallocMem(void* block);
};

}  // namespace Memory

inline void* operator new(unsigned long, void* p) { return p; }

// The one the linker folded the condition's own asset setter onto.
class zBTAction {
public:
    void SetAsset(const Sext::ActionBase* value);
};

// Two words in front of the vtable pointer at +8: the asset the builder
// sets, and the client.
class zBTConditionData {
public:
    const Sext::ConditionBase* asset;
    zBTClient* btClient;
};

class zBTCondition : public zBTConditionData {
public:
    virtual int _v0();
    virtual unsigned int _v1();
    virtual void _v2();
    virtual void _v3();
    virtual void _v4();
    virtual void _v5();
    virtual void _v6();

    void SetBTClient(zBTClient* value);

    static zBTCondition gConditionTrue;
    static zBTCondition gConditionFalse;
};

class zBTFactory {
public:
    static Memory::Factory factory;

    template <class T> static T* Create();
};

template <class T>
T* zBTFactory::Create() {
    void* mem = factory.AllocMem(sizeof(T), (Memory::eFactoryMemType)14);

    return !mem ? 0 : new (mem) T();
}

class zNPCBTCondition : public zBTCondition {
public:
    virtual void _v2();

    template <class T> static T* Create(const char* name);

    unsigned char _pad0[0x14 - 0xC];
};

template <class T>
T* zNPCBTCondition::Create(const char* name) {
    void* mem =
        zBTFactory::factory.AllocMem(sizeof(T),
                                     (Memory::eFactoryMemType)14);

    T* condition = !mem ? 0 : new (mem) T();

    condition->_v5();

    return condition;
}

class zBTConditionBuilder {
public:
    zBTCondition* Build(int type, Sext::ConditionBase* asset) const;
    void Destroy(zBTCondition* condition) const;

    zBTClient* btClient;
};

// Build CALLS this rather than folding it in, so it is compiled
// where nothing can inline it -- the same guard gen_accessors.py
// emits round a setter a caller in the same unit uses.
#pragma dont_inline on

void zBTCondition::SetBTClient(zBTClient* value) { btClient = value; }

#pragma dont_inline off

class zBTConditionCheckConditional : public zBTCondition {
public:
    virtual void _v2();
};

class zBTConditionCheckVariable : public zBTCondition {
public:
    virtual void _v2();
};

class zBTConditionDistanceFromClosestPlayer : public zBTCondition {
public:
    virtual void _v2();

    unsigned char _pad0[0x8];
};

class zBTConditionDistanceFromPlayer : public zBTCondition {
public:
    virtual void _v2();

    unsigned char _pad0[0xC];
};

class zBTConditionIsAnyPlayerInCombat : public zBTCondition {
public:
    virtual void _v2();
};

class zBTConditionPlayerIsInAction : public zBTCondition {
public:
    virtual void _v2();

    unsigned char _pad0[0x4];
};

class zNPCBTActiveCondition : public zNPCBTCondition {
public:
    virtual void _v2();
};

class zNPCBTAliveCondition : public zNPCBTCondition {
public:
    virtual void _v2();
};

class zNPCBTBlockedCondition : public zNPCBTCondition {
public:
    virtual void _v2();
};

class zNPCBTBlockedTypeCondition : public zNPCBTCondition {
public:
    virtual void _v2();

    unsigned char _pad0[0x4];
};

class zNPCBTBossSquidwardBlockPlayer : public zNPCBTCondition {
public:
    virtual void _v2();
};

class zNPCBTBossSquidwardDestroyCover : public zNPCBTCondition {
public:
    virtual void _v2();
};

class zNPCBTCheckPerceptionCondition : public zNPCBTCondition {
public:
    virtual void _v2();

    unsigned char _pad0[0xC];
};

class zNPCBTCheckPerceptionTargetChangedCondition : public zNPCBTCondition {
public:
    virtual void _v2();

    unsigned char _pad0[0x4];
};

class zNPCBTCheckPerceptionTargetInWallnetCondition : public zNPCBTCondition {
public:
    virtual void _v2();

    unsigned char _pad0[0x4];
};

class zNPCBTCheckPerceptionTargetStatusCondition : public zNPCBTCondition {
public:
    virtual void _v2();

    unsigned char _pad0[0x8];
};

class zNPCBTCheckPlayerTypeCondition : public zNPCBTCondition {
public:
    virtual void _v2();

    unsigned char _pad0[0x8];
};

class zNPCBTCounterFailCondition : public zNPCBTCondition {
public:
    virtual void _v2();
};

class zNPCBTCounterNoneCondition : public zNPCBTCondition {
public:
    virtual void _v2();
};

class zNPCBTCounterSuccessCondition : public zNPCBTCondition {
public:
    virtual void _v2();
};

class zNPCBTCurrPlayerIsHumanCondition : public zNPCBTCondition {
public:
    virtual void _v2();
};

class zNPCBTCurrPlayerIsPlayerCondition : public zNPCBTCondition {
public:
    virtual void _v2();

    unsigned char _pad0[0x4];
};

class zNPCBTDamagedByKnockbackCondition : public zNPCBTCondition {
public:
    virtual void _v2();
};

class zNPCBTDamagedByTypeCondition : public zNPCBTCondition {
public:
    virtual void _v2();

    unsigned char _pad0[0x4];
};

class zNPCBTDamagedCondition : public zNPCBTCondition {
public:
    virtual void _v2();

    unsigned char _pad0[0x4];
};

class zNPCBTDefeatedCondition : public zNPCBTCondition {
public:
    virtual void _v2();
};

class zNPCBTDistanceFromFloatingObjectCondition : public zNPCBTCondition {
public:
    virtual void _v2();
};

class zNPCBTEnteredTrapCondition : public zNPCBTCondition {
public:
    virtual void _v2();
};

class zNPCBTExitedTrapCondition : public zNPCBTCondition {
public:
    virtual void _v2();
};

class zNPCBTFacingPerceptionTargetCondition : public zNPCBTCondition {
public:
    virtual void _v2();

    unsigned char _pad0[0x8];
};

class zNPCBTHitCondition : public zNPCBTCondition {
public:
    virtual void _v2();
};

class zNPCBTHitPointCountCondition : public zNPCBTCondition {
public:
    virtual void _v2();
};

class zNPCBTInHearingAidRangeCondition : public zNPCBTCondition {
public:
    virtual void _v2();
};

class zNPCBTInRPSAttackStateCondition : public zNPCBTCondition {
public:
    virtual void _v2();

    unsigned char _pad0[0x4];
};

class zNPCBTIsFloatingObjectActiveCondition : public zNPCBTCondition {
public:
    virtual void _v2();
};

class zNPCBTIsFloatingObjectValidCondition : public zNPCBTCondition {
public:
    virtual void _v2();
};

class zNPCBTIsInPuppetModeCondition : public zNPCBTCondition {
public:
    virtual void _v2();
};

class zNPCBTIsInWallnetCondition : public zNPCBTCondition {
public:
    virtual void _v2();

    unsigned char _pad0[0x8];
};

class zNPCBTIsOnGroundCondition : public zNPCBTCondition {
public:
    virtual void _v2();
};

class zNPCBTIsPlanktonShakingCondition : public zNPCBTCondition {
public:
    virtual void _v2();
};

class zNPCBTIsPlayerValidCondition : public zNPCBTCondition {
public:
    virtual void _v2();
};

class zNPCBTIsToLockedPlayerSideCondition : public zNPCBTCondition {
public:
    virtual void _v2();

    unsigned char _pad0[0x4];
};

class zNPCBTLastLiveEnemyCondition : public zNPCBTCondition {
public:
    virtual void _v2();
};

class zNPCBTLockedPlayerIsHumanCondition : public zNPCBTCondition {
public:
    virtual void _v2();
};

class zNPCBTMovingCondition : public zNPCBTCondition {
public:
    virtual void _v2();
};

class zNPCBTNearbyProjectileCondition : public zNPCBTCondition {
public:
    virtual void _v2();

    unsigned char _pad0[0x4];
};

class zNPCBTNeedCombatCleanupCondition : public zNPCBTCondition {
public:
    virtual void _v2();
};

class zNPCBTNeedCombatTargetingCleanupCondition : public zNPCBTCondition {
public:
    virtual void _v2();
};

class zNPCBTOffScreenCondition : public zNPCBTCondition {
public:
    virtual void _v2();
};

class zNPCBTPlayerIsBeingAttackedCondition : public zNPCBTCondition {
public:
    virtual void _v2();
};

class zNPCBTPlayerIsHumanCondition : public zNPCBTCondition {
public:
    virtual void _v2();
};

class zNPCBTPlayerIsPlayerCondition : public zNPCBTCondition {
public:
    virtual void _v2();

    unsigned char _pad0[0x4];
};

class zNPCBTPlayerIsTargetableCondition : public zNPCBTCondition {
public:
    virtual void _v2();
};

class zNPCBTPokedCondition : public zNPCBTCondition {
public:
    virtual void _v2();

    unsigned char _pad0[0x4];
};

class zNPCBTSBPlayerIsBuff : public zNPCBTCondition {
public:
    virtual void _v2();
};

class zNPCBTSpecialAbilityOnCondition : public zNPCBTCondition {
public:
    virtual void _v2();

    unsigned char _pad0[0x4];
};

class zNPCBTSwarmKilledByPlayerCondition : public zNPCBTCondition {
public:
    virtual void _v2();

    unsigned char _pad0[0x4];
};

class zNPCBT_GenericSpawner_CanSpawnLater_Condition : public zNPCBTCondition {
public:
    virtual void _v2();
};

class zNPCBT_GenericSpawner_CanSpawn_Condition : public zNPCBTCondition {
public:
    virtual void _v2();
};

class zNPCBT_GenericSpawner_NonstopSpawn_Condition : public zNPCBTCondition {
public:
    virtual void _v2();
};

zBTCondition* zBTConditionBuilder::Build(
    int type, Sext::ConditionBase* asset) const {
    zBTCondition* condition;

    switch (type) {
    case 0x89176D7D:
        condition = &zBTCondition::gConditionTrue;
        break;
    case 0x2EF3935C:
        condition = &zBTCondition::gConditionFalse;
        break;
    case 0xD57AD404:
        condition = zBTFactory::Create<zBTConditionCheckVariable>();
        break;
    case 0xB80413D6:
        condition = zBTFactory::Create<zBTConditionCheckConditional>();
        break;
    case 0xBE5CF04F:
        condition = zBTFactory::Create<zBTConditionDistanceFromClosestPlayer>();
        break;
    case 0xD91878D8:
        condition = zBTFactory::Create<zBTConditionDistanceFromPlayer>();
        break;
    case 0x9D1EA338:
        condition = zBTFactory::Create<zBTConditionIsAnyPlayerInCombat>();
        break;
    case 0x4EC7C3F6:
        condition = zBTFactory::Create<zBTConditionPlayerIsInAction>();
        break;
    case 0xDAF925F4:
        condition = zNPCBTCondition::Create<zNPCBTActiveCondition>(
            "zNPCBTActiveCondition");
        break;
    case 0xB2BE860B:
        condition = zNPCBTCondition::Create<zNPCBTAliveCondition>(
            "zNPCBTAliveCondition");
        break;
    case 0x75BF4415:
        condition = zNPCBTCondition::Create<zNPCBTCheckPlayerTypeCondition>(
            "zNPCBTCheckPlayerTypeCondition");
        break;
    case 0x0E9F347C:
        condition = zNPCBTCondition::Create<zNPCBTLastLiveEnemyCondition>(
            "zNPCBTLastLiveEnemyCondition");
        break;
    case 0x9312DBFC:
        condition = zNPCBTCondition::Create<zNPCBTCounterFailCondition>(
            "zNPCBTCounterFailCondition");
        break;
    case 0x9428F5DA:
        condition = zNPCBTCondition::Create<zNPCBTCounterNoneCondition>(
            "zNPCBTCounterNoneCondition");
        break;
    case 0x59010875:
        condition = zNPCBTCondition::Create<zNPCBTCounterSuccessCondition>(
            "zNPCBTCounterSuccessCondition");
        break;
    case 0xDA75C0E2:
        condition = zNPCBTCondition::Create<zNPCBTCurrPlayerIsHumanCondition>(
            "zNPCBTCurrPlayerIsHumanCondition");
        break;
    case 0x06D347B0:
        condition = zNPCBTCondition::Create<zNPCBTCurrPlayerIsPlayerCondition>(
            "zNPCBTCurrPlayerIsPlayerCondition");
        break;
    case 0xAD08633C:
        condition = zNPCBTCondition::Create<zNPCBTDefeatedCondition>(
            "zNPCBTDefeatedCondition");
        break;
    case 0xDBC285E2:
        condition = zNPCBTCondition::Create<zNPCBTDistanceFromFloatingObjectCondition>(
            "zNPCBTDistanceFromFloatingObjectCondition");
        break;
    case 0xE3FFDDC6:
        condition = zNPCBTCondition::Create<zNPCBTEnteredTrapCondition>(
            "zNPCBTEnteredTrapCondition");
        break;
    case 0xCB47802C:
        condition = zNPCBTCondition::Create<zNPCBTExitedTrapCondition>(
            "zNPCBTExitedTrapCondition");
        break;
    case 0x6BCBFC65:
        condition = zNPCBTCondition::Create<zNPCBTHitCondition>(
            "zNPCBTHitCondition");
        break;
    case 0x5936688C:
        condition = zNPCBTCondition::Create<zNPCBTHitPointCountCondition>(
            "zNPCBTHitPointCountCondition");
        break;
    case 0x0E4569BE:
        condition = zNPCBTCondition::Create<zNPCBTInHearingAidRangeCondition>(
            "zNPCBTInHearingAidRangeCondition");
        break;
    case 0x3A58D11B:
        condition = zNPCBTCondition::Create<zNPCBTIsFloatingObjectActiveCondition>(
            "zNPCBTIsFloatingObjectActiveCondition");
        break;
    case 0x0F158907:
        condition = zNPCBTCondition::Create<zNPCBTIsFloatingObjectValidCondition>(
            "zNPCBTIsFloatingObjectValidCondition");
        break;
    case 0xF4BFED0F:
        condition = zNPCBTCondition::Create<zNPCBTIsPlayerValidCondition>(
            "zNPCBTIsPlayerValidCondition");
        break;
    case 0xDA0CC2F9:
        condition = zNPCBTCondition::Create<zNPCBTIsToLockedPlayerSideCondition>(
            "zNPCBTIsToLockedPlayerSideCondition");
        break;
    case 0xCFBB434E:
        condition = zNPCBTCondition::Create<zNPCBTLockedPlayerIsHumanCondition>(
            "zNPCBTLockedPlayerIsHumanCondition");
        break;
    case 0x1836CF18:
        condition = zNPCBTCondition::Create<zNPCBTNeedCombatCleanupCondition>(
            "zNPCBTNeedCombatCleanupCondition");
        break;
    case 0x3A61E1A3:
        condition = zNPCBTCondition::Create<zNPCBTNeedCombatTargetingCleanupCondition>(
            "zNPCBTNeedCombatTargetingCleanupCondition");
        break;
    case 0xFA817831:
        condition = zNPCBTCondition::Create<zNPCBTOffScreenCondition>(
            "zNPCBTOffScreenCondition");
        break;
    case 0x18EE9D20:
        condition = zNPCBTCondition::Create<zNPCBTPlayerIsHumanCondition>(
            "zNPCBTPlayerIsHumanCondition");
        break;
    case 0xFEABFB6A:
        condition = zNPCBTCondition::Create<zNPCBTPlayerIsPlayerCondition>(
            "zNPCBTPlayerIsPlayerCondition");
        break;
    case 0xFD3BCC1C:
        condition = zNPCBTCondition::Create<zNPCBTPlayerIsTargetableCondition>(
            "zNPCBTPlayerIsTargetableCondition");
        break;
    case 0x352422D9:
        condition = zNPCBTCondition::Create<zNPCBTPlayerIsBeingAttackedCondition>(
            "zNPCBTPlayerIsBeingAttackedCondition");
        break;
    case 0x7C563A4A:
        condition = zNPCBTCondition::Create<zNPCBTSpecialAbilityOnCondition>(
            "zNPCBTSpecialAbilityOnCondition");
        break;
    case 0xD045A852:
        condition = zNPCBTCondition::Create<zNPCBTSwarmKilledByPlayerCondition>(
            "zNPCBTSwarmKilledByPlayerCondition");
        break;
    case 0x199C5DC2:
        condition = zNPCBTCondition::Create<zNPCBTIsOnGroundCondition>(
            "zNPCBTIsOnGroundCondition");
        break;
    case 0x7815D244:
        condition = zNPCBTCondition::Create<zNPCBTMovingCondition>(
            "zNPCBTMovingCondition");
        break;
    case 0x5FC62F38:
        condition = zNPCBTCondition::Create<zNPCBT_GenericSpawner_NonstopSpawn_Condition>(
            "zNPCBT_GenericSpawner_NonstopSpawn_Condition");
        break;
    case 0x9C10E73B:
        condition = zNPCBTCondition::Create<zNPCBT_GenericSpawner_CanSpawn_Condition>(
            "zNPCBT_GenericSpawner_CanSpawn_Condition");
        break;
    case 0x61A01F7D:
        condition = zNPCBTCondition::Create<zNPCBT_GenericSpawner_CanSpawnLater_Condition>(
            "zNPCBT_GenericSpawner_CanSpawnLater_Condition");
        break;
    case 0x41426C6A:
        condition = zNPCBTCondition::Create<zNPCBTBlockedCondition>(
            "zNPCBTBlockedCondition");
        break;
    case 0xAAD297BC:
        condition = zNPCBTCondition::Create<zNPCBTBlockedTypeCondition>(
            "zNPCBTBlockedTypeCondition");
        break;
    case 0xB897522F:
        condition = zNPCBTCondition::Create<zNPCBTDamagedCondition>(
            "zNPCBTDamagedCondition");
        break;
    case 0x456DE2D8:
        condition = zNPCBTCondition::Create<zNPCBTDamagedByTypeCondition>(
            "zNPCBTDamagedByTypeCondition");
        break;
    case 0x9FE9DCB5:
        condition = zNPCBTCondition::Create<zNPCBTDamagedByKnockbackCondition>(
            "zNPCBTDamagedByKnockbackCondition");
        break;
    case 0x69DC34BB:
        condition = zNPCBTCondition::Create<zNPCBTInRPSAttackStateCondition>(
            "zNPCBTInRPSAttackStateCondition");
        break;
    case 0xDD16B40E:
        condition = zNPCBTCondition::Create<zNPCBTIsPlanktonShakingCondition>(
            "zNPCBTIsPlanktonShakingCondition");
        break;
    case 0xBA739AF9:
        condition = zNPCBTCondition::Create<zNPCBTPokedCondition>(
            "zNPCBTPokedCondition");
        break;
    case 0x72888246:
        condition = zNPCBTCondition::Create<zNPCBTNearbyProjectileCondition>(
            "zNPCBTNearbyProjectileCondition");
        break;
    case 0xFC46AB83:
        condition = zNPCBTCondition::Create<zNPCBTBossSquidwardBlockPlayer>(
            "zNPCBTBossSquidwardBlockPlayer");
        break;
    case 0x4E88847E:
        condition = zNPCBTCondition::Create<zNPCBTBossSquidwardDestroyCover>(
            "zNPCBTBossSquidwardDestroyCover");
        break;
    case 0x3FF49CAD:
        condition = zNPCBTCondition::Create<zNPCBTSBPlayerIsBuff>(
            "zNPCBTSBPlayerIsBuff");
        break;
    case 0x174B72DA:
        condition = zNPCBTCondition::Create<zNPCBTFacingPerceptionTargetCondition>(
            "zNPCBTFacingPerceptionTargetCondition");
        break;
    case 0x098F1B17:
        condition = zNPCBTCondition::Create<zNPCBTCheckPerceptionCondition>(
            "zNPCBTCheckPerceptionCondition");
        break;
    case 0xCF3BD772:
        condition = zNPCBTCondition::Create<zNPCBTCheckPerceptionTargetStatusCondition>(
            "zNPCBTCheckPerceptionTargetStatusCondition");
        break;
    case 0xF570755C:
        condition = zNPCBTCondition::Create<zNPCBTCheckPerceptionTargetChangedCondition>(
            "zNPCBTCheckPerceptionTargetChangedCondition");
        break;
    case 0x8CCE56E2:
        condition = zNPCBTCondition::Create<zNPCBTCheckPerceptionTargetInWallnetCondition>(
            "zNPCBTCheckPerceptionTargetInWallnetCondition");
        break;
    case 0x0B8B3028:
        condition = zNPCBTCondition::Create<zNPCBTIsInPuppetModeCondition>(
            "zNPCBTIsInPuppetModeCondition");
        break;
    case 0xE61F36DE:
        condition = zNPCBTCondition::Create<zNPCBTIsInWallnetCondition>(
            "zNPCBTIsInWallnetCondition");
        break;
    default:
        condition = &zBTCondition::gConditionFalse;
        break;
    }

    condition->SetBTClient(btClient);
    ((zBTAction*)condition)->SetAsset((const Sext::ActionBase*)asset);

    return condition;
}

void zBTConditionBuilder::Destroy(zBTCondition* condition) const {
    condition->_v4();

    switch (condition->_v0()) {
    case 0: {
        unsigned int id = condition->_v1();

        if (id != 0x89176D7D && id != 0x2EF3935C) {
            zBTFactory::factory.DeallocMem(condition);
        }

        break;
    }
    case 3:
        condition->_v6();
        zBTFactory::factory.DeallocMem(condition);
        break;
    case 1:
    case 2:
    case 4:
    case 5:
        zBTFactory::factory.DeallocMem(condition);
        break;
    }
}

template zBTConditionCheckConditional* zBTFactory::Create<zBTConditionCheckConditional>();
template zBTConditionCheckVariable* zBTFactory::Create<zBTConditionCheckVariable>();
template zBTConditionDistanceFromClosestPlayer* zBTFactory::Create<zBTConditionDistanceFromClosestPlayer>();
template zBTConditionDistanceFromPlayer* zBTFactory::Create<zBTConditionDistanceFromPlayer>();
template zBTConditionIsAnyPlayerInCombat* zBTFactory::Create<zBTConditionIsAnyPlayerInCombat>();
template zBTConditionPlayerIsInAction* zBTFactory::Create<zBTConditionPlayerIsInAction>();
template zNPCBTActiveCondition* zNPCBTCondition::Create<zNPCBTActiveCondition>(const char*);
template zNPCBTAliveCondition* zNPCBTCondition::Create<zNPCBTAliveCondition>(const char*);
template zNPCBTBlockedCondition* zNPCBTCondition::Create<zNPCBTBlockedCondition>(const char*);
template zNPCBTBlockedTypeCondition* zNPCBTCondition::Create<zNPCBTBlockedTypeCondition>(const char*);
template zNPCBTBossSquidwardBlockPlayer* zNPCBTCondition::Create<zNPCBTBossSquidwardBlockPlayer>(const char*);
template zNPCBTBossSquidwardDestroyCover* zNPCBTCondition::Create<zNPCBTBossSquidwardDestroyCover>(const char*);
template zNPCBTCheckPerceptionCondition* zNPCBTCondition::Create<zNPCBTCheckPerceptionCondition>(const char*);
template zNPCBTCheckPerceptionTargetChangedCondition* zNPCBTCondition::Create<zNPCBTCheckPerceptionTargetChangedCondition>(const char*);
template zNPCBTCheckPerceptionTargetInWallnetCondition* zNPCBTCondition::Create<zNPCBTCheckPerceptionTargetInWallnetCondition>(const char*);
template zNPCBTCheckPerceptionTargetStatusCondition* zNPCBTCondition::Create<zNPCBTCheckPerceptionTargetStatusCondition>(const char*);
template zNPCBTCheckPlayerTypeCondition* zNPCBTCondition::Create<zNPCBTCheckPlayerTypeCondition>(const char*);
template zNPCBTCounterFailCondition* zNPCBTCondition::Create<zNPCBTCounterFailCondition>(const char*);
template zNPCBTCounterNoneCondition* zNPCBTCondition::Create<zNPCBTCounterNoneCondition>(const char*);
template zNPCBTCounterSuccessCondition* zNPCBTCondition::Create<zNPCBTCounterSuccessCondition>(const char*);
template zNPCBTCurrPlayerIsHumanCondition* zNPCBTCondition::Create<zNPCBTCurrPlayerIsHumanCondition>(const char*);
template zNPCBTCurrPlayerIsPlayerCondition* zNPCBTCondition::Create<zNPCBTCurrPlayerIsPlayerCondition>(const char*);
template zNPCBTDamagedByKnockbackCondition* zNPCBTCondition::Create<zNPCBTDamagedByKnockbackCondition>(const char*);
template zNPCBTDamagedByTypeCondition* zNPCBTCondition::Create<zNPCBTDamagedByTypeCondition>(const char*);
template zNPCBTDamagedCondition* zNPCBTCondition::Create<zNPCBTDamagedCondition>(const char*);
template zNPCBTDefeatedCondition* zNPCBTCondition::Create<zNPCBTDefeatedCondition>(const char*);
template zNPCBTDistanceFromFloatingObjectCondition* zNPCBTCondition::Create<zNPCBTDistanceFromFloatingObjectCondition>(const char*);
template zNPCBTEnteredTrapCondition* zNPCBTCondition::Create<zNPCBTEnteredTrapCondition>(const char*);
template zNPCBTExitedTrapCondition* zNPCBTCondition::Create<zNPCBTExitedTrapCondition>(const char*);
template zNPCBTFacingPerceptionTargetCondition* zNPCBTCondition::Create<zNPCBTFacingPerceptionTargetCondition>(const char*);
template zNPCBTHitCondition* zNPCBTCondition::Create<zNPCBTHitCondition>(const char*);
template zNPCBTHitPointCountCondition* zNPCBTCondition::Create<zNPCBTHitPointCountCondition>(const char*);
template zNPCBTInHearingAidRangeCondition* zNPCBTCondition::Create<zNPCBTInHearingAidRangeCondition>(const char*);
template zNPCBTInRPSAttackStateCondition* zNPCBTCondition::Create<zNPCBTInRPSAttackStateCondition>(const char*);
template zNPCBTIsFloatingObjectActiveCondition* zNPCBTCondition::Create<zNPCBTIsFloatingObjectActiveCondition>(const char*);
template zNPCBTIsFloatingObjectValidCondition* zNPCBTCondition::Create<zNPCBTIsFloatingObjectValidCondition>(const char*);
template zNPCBTIsInPuppetModeCondition* zNPCBTCondition::Create<zNPCBTIsInPuppetModeCondition>(const char*);
template zNPCBTIsInWallnetCondition* zNPCBTCondition::Create<zNPCBTIsInWallnetCondition>(const char*);
template zNPCBTIsOnGroundCondition* zNPCBTCondition::Create<zNPCBTIsOnGroundCondition>(const char*);
template zNPCBTIsPlanktonShakingCondition* zNPCBTCondition::Create<zNPCBTIsPlanktonShakingCondition>(const char*);
template zNPCBTIsPlayerValidCondition* zNPCBTCondition::Create<zNPCBTIsPlayerValidCondition>(const char*);
template zNPCBTIsToLockedPlayerSideCondition* zNPCBTCondition::Create<zNPCBTIsToLockedPlayerSideCondition>(const char*);
template zNPCBTLastLiveEnemyCondition* zNPCBTCondition::Create<zNPCBTLastLiveEnemyCondition>(const char*);
template zNPCBTLockedPlayerIsHumanCondition* zNPCBTCondition::Create<zNPCBTLockedPlayerIsHumanCondition>(const char*);
template zNPCBTMovingCondition* zNPCBTCondition::Create<zNPCBTMovingCondition>(const char*);
template zNPCBTNearbyProjectileCondition* zNPCBTCondition::Create<zNPCBTNearbyProjectileCondition>(const char*);
template zNPCBTNeedCombatCleanupCondition* zNPCBTCondition::Create<zNPCBTNeedCombatCleanupCondition>(const char*);
template zNPCBTNeedCombatTargetingCleanupCondition* zNPCBTCondition::Create<zNPCBTNeedCombatTargetingCleanupCondition>(const char*);
template zNPCBTOffScreenCondition* zNPCBTCondition::Create<zNPCBTOffScreenCondition>(const char*);
template zNPCBTPlayerIsBeingAttackedCondition* zNPCBTCondition::Create<zNPCBTPlayerIsBeingAttackedCondition>(const char*);
template zNPCBTPlayerIsHumanCondition* zNPCBTCondition::Create<zNPCBTPlayerIsHumanCondition>(const char*);
template zNPCBTPlayerIsPlayerCondition* zNPCBTCondition::Create<zNPCBTPlayerIsPlayerCondition>(const char*);
template zNPCBTPlayerIsTargetableCondition* zNPCBTCondition::Create<zNPCBTPlayerIsTargetableCondition>(const char*);
template zNPCBTPokedCondition* zNPCBTCondition::Create<zNPCBTPokedCondition>(const char*);
template zNPCBTSBPlayerIsBuff* zNPCBTCondition::Create<zNPCBTSBPlayerIsBuff>(const char*);
template zNPCBTSpecialAbilityOnCondition* zNPCBTCondition::Create<zNPCBTSpecialAbilityOnCondition>(const char*);
template zNPCBTSwarmKilledByPlayerCondition* zNPCBTCondition::Create<zNPCBTSwarmKilledByPlayerCondition>(const char*);
template zNPCBT_GenericSpawner_CanSpawnLater_Condition* zNPCBTCondition::Create<zNPCBT_GenericSpawner_CanSpawnLater_Condition>(const char*);
template zNPCBT_GenericSpawner_CanSpawn_Condition* zNPCBTCondition::Create<zNPCBT_GenericSpawner_CanSpawn_Condition>(const char*);
template zNPCBT_GenericSpawner_NonstopSpawn_Condition* zNPCBTCondition::Create<zNPCBT_GenericSpawner_NonstopSpawn_Condition>(const char*);
