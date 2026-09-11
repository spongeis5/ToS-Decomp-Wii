#include "SB/GM/Engine/WAD02_24_2.pool.h"

// WAD02_24_2.cpp -- the NPC behaviour-tree CONDITIONS, and after them
// the entity and bound geometry: 111 functions, 19,880 bytes.
//
// A condition is a tiny object: the asset it was built from, the client
// that owns it, and the NPC it asks about. Evaluate() reads one thing
// off one of the NPC's components and answers yes or no, which is why
// most of them are under fifty bytes.
//
// Layouts from the Wii build's DWARF (tools/dwarf_types.py), which
// covers this file: zNPCBTCondition 0x14 on a zBTCondition of 0xC, and
// each condition adds its own fields after it. The Sext:: asset structs
// the Setup functions read are NOT in the DWARF -- only the offsets
// each one touches are recovered, and they are named for the condition
// that reads them.
//
// The pool header in front reproduces WAD02.cpp's string pool ahead of
// this file's own strings, and the .rodata ahead of its float literals.

extern "C" double atan2(double y, double x);

class xBase;
class zBTClient;
class zNPCEntity;
class zNPCBase;
class zNPCSteering;

namespace Sext {

class ConditionBase;
class EventAny;

enum eHitSource { eHitSourceEVENT = 0, END_eHitSourceENUM = 62 };

enum eRPSAttackTypes {
    eRPSAttackType_None = 0,
    eRPSAttackType_Hammer = 1,
    eRPSAttackType_Spin = 2,
    eRPSAttackType_Puck = 3,
    eRPSAttackType_Miniboss_Hammer = 4,
    eRPSAttackType_Miniboss_Spin = 5,
    eRPSAttackType_Miniboss_Puck = 6,
    END_eRPSAttackType_ENUM = 7,
};

enum eNPCPerceptionType {
    eNPCPerception_TargetReachable = 0,
    eNPCPerception_TargetChargable = 1,
    eNPCPerception_TargetAttackable = 2,
    eNPCPerception_TargetVisible = 3,
    eNPCPerception_TargetShootable = 4,
    eNPCPerception_TargetEngageable = 5,
    END_eNPCPerception_ENUM = 6,
};

enum eCollisionLayer { eCollisionLayer_Player = 13 };

// Only the fields each Setup reads; the structs themselves are not
// described by the debug info.
class DamagedCondition {
public:
    bool nonzero;
};

class InRPSAttackStateCondition {
public:
    unsigned char state;
};

// The flag is copied raw into the condition's bool: it is a bool here
// too, or the copy normalises it (addic/subfe), which retail does not.
class CheckPerceptionCondition {
public:
    unsigned int targets;
    unsigned char perceptionType;
    bool anded;
};

// The target indices are one-based in the assets.
class CheckPerceptionTargetStatusCondition {
public:
    unsigned char target;
    unsigned char status;
};

class IsInWallnetCondition {
public:
    unsigned int check;
    unsigned char target;
};

class FacingPerceptionTargetCondition {
public:
    unsigned char target;
    float tolerance;
};

}  // namespace Sext

// ---------------------------------------------------------------------------
// Math

class xVec3 {
public:
    xVec3& operator=(const xVec3& other);
    void Sub(const xVec3& a, const xVec3& b);

    static const xVec3 m_Null;
    static const xVec3 m_Ones;
    static const xVec3 m_UnitAxisY;

    float x;
    float y;
    float z;
};

class xMat3x3 {
public:
    xMat3x3& operator=(const xMat3x3& other);

    xVec3 right;
    unsigned int flags;
    xVec3 up;
    unsigned int pad1;
    xVec3 at;
    unsigned int pad2;
};

class xMat4x3 : public xMat3x3 {
public:
    xMat4x3& operator=(const xMat4x3& other);

    xVec3 pos;
    unsigned int pad3;
};

float xClampAngle0_2PI(float angle);
unsigned int xStrHash(const char* s);

namespace std {

inline float fabs(float x) { return (float)__fabs(x); }

// Retail calls this one: it is emitted weak after its only caller, and
// defined there so that nothing can inline it.
inline float atan2(float y, float x);

}  // namespace std

// ---------------------------------------------------------------------------
// The model: an xOGModel starts with the instance's matrix.

class xAnimPlay;

namespace World {

class xOGModel {
public:
    xMat4x3 Mat;
    xVec3 Scale;
    xAnimPlay* Anim;
};

}  // namespace World

void xModelGetBoneMatNoScale(xMat4x3& mat, const World::xOGModel& model,
                             unsigned long bone);

// ---------------------------------------------------------------------------
// Entities. Every one of them has its vtable pointer at +0, in front of
// the xBase words; the slots are declared where a call needs one.

class xEntVirtuals {
public:
    virtual void _v0();
};

// 0xC0 in the DWARF, of which 0xBC is data: the NPC entity puts its
// component right after it. The 64-bit id at +0x18 is two words here so
// that the class stays four-aligned.
class xEnt : public xEntVirtuals {
public:
    unsigned char _pad0[0x20 - 0x4];
    unsigned int baseType;
    unsigned char _pad1[0x34 - 0x24];
    World::xOGModel* model;
    unsigned char _pad2[0xBC - 0x38];
};

xVec3 xEntGetCenter(const xEnt* ent);

// 0x1D0 in the DWARF.
class zNPCEntity : public xEnt {
public:
    zNPCBase* owner;
    unsigned char _pad3[0x181 - 0xC0];
    bool pokedA;
    bool pokedB;
};

class zNPCBaseVirtuals {
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
    virtual int SystemEvent(xBase* from, xBase* to, unsigned int event,
                            Sext::EventAny* args);
};

// A pickup, destructible or the like: type 0x55, asked for its health
// through its vtable.
class zHealthEnt : public xEnt {
public:
    virtual void _h1();
    virtual void _h2();
    virtual void _h3();
    virtual void _h4();
    virtual void _h5();
    virtual void _h6();
    virtual void _h7();
    virtual void _h8();
    virtual void _h9();
    virtual void _h10();
    virtual void _h11();
    virtual void _h12();
    virtual void _h13();
    virtual void _h14();
    virtual void _h15();
    virtual void _h16();
    virtual void _h17();
    virtual void _h18();
    virtual void _h19();
    virtual void _h20();
    virtual void _h21();
    virtual void _h22();
    virtual void _h23();
    virtual void _h24();
    virtual void _h25();
    virtual void _h26();
    virtual void _h27();
    virtual void _h28();
    virtual void _h29();
    virtual void _h30();
    virtual void _h31();
    virtual void _h32();
    virtual void _h33();
    virtual void _h34();
    virtual void _h35();
    virtual void _h36();
    virtual void _h37();
    virtual void _h38();
    virtual void _h39();
    virtual void _h40();
    virtual void _h41();
    virtual void _h42();
    virtual void _h43();
    virtual void _h44();
    virtual void _h45();
    virtual void _h46();
    virtual void _h47();
    virtual void _h48();
    virtual void _h49();
    virtual void _h50();
    virtual void _h51();
    virtual void _h52();
    virtual void _h53();
    virtual void _h54();
    virtual void _h55();
    virtual void _h56();
    virtual void _h57();
    virtual void _h58();
    virtual void _h59();
    virtual void _h60();
    virtual void _h61();
    virtual void _h62();
    virtual void _h63();
    virtual void _h64();
    virtual void _h65();
    virtual void _h66();
    virtual void _h67();
    virtual void _h68();
    virtual void _h69();
    virtual void _h70();
    virtual void _h71();
    virtual void _h72();
    virtual void _h73();
    virtual void _h74();
    virtual void _h75();
    virtual void _h76();
    virtual void _h77();
    virtual void _h78();
    virtual void _h79();
    virtual void _h80();
    virtual void _h81();
    virtual float GetHealth();
};

// The hit points the other two target types carry: what is left is the
// total less what has been taken.
class zHitPoints {
public:
    unsigned char _pad0[0x18];
    unsigned int total;
    unsigned char _pad1[0x2C - 0x1C];
    unsigned int taken;
};

class zHitPointsEnt56 : public xEnt {
public:
    unsigned char _pad3[0x178 - 0xBC];
    zHitPoints* hitPoints;
};

class zHitPointsEnt5A : public xEnt {
public:
    unsigned char _pad3[0xCC - 0xBC];
    zHitPoints* hitPoints;
};

// 0x2C in the DWARF; this file reads the knockback flag and the damage.
class zNPCGetsDamageInfo {
public:
    unsigned int flags;
    unsigned char _pad0[0xC - 0x4];
    float damageHP;
    unsigned char _pad1[0x2C - 0x10];
};

class zNPCCombat {
public:
    bool HitByType(Sext::eHitSource source) const;
    bool BlockedType(Sext::eHitSource source) const;

    zNPCGetsDamageInfo* GetDamageInfo(unsigned int i) {
        return (i < damageListSize) ? &damageList[i] : 0;
    }

    unsigned char _pad0[0x294];
    Sext::eRPSAttackTypes attackState;
    unsigned char _pad1[0x29C - 0x298];
    zNPCGetsDamageInfo damageList[6];
    unsigned char _pad2[0x3BC - 0x3A4];
    unsigned int damageListSize;
    unsigned char _pad3[0x4E0 - 0x3C0];
    unsigned int blockListSize;
};

class zNPCPerceptionTarget {
public:
    xEnt* targetEnt;
    unsigned char _pad0[0x74 - 0x4];
};

class zNPCPerception {
public:
    bool AreTargetsPerceived(unsigned int mask, Sext::eNPCPerceptionType type,
                             bool all);
    static bool CheckLineOfSight(const xVec3* from, const xVec3* to,
                                 const zNPCEntity* self, const xEnt* ignore,
                                 Sext::eCollisionLayer layer);

    unsigned char _pad0[0x10];
    zNPCPerceptionTarget targets[4];
};

class zWallNet {
public:
    bool IsInsideWallNetXZ(const xVec3& pos) const;
    bool IsInsideWallNet(const xVec3& pos) const;
};

class zNPCSteering {
public:
    unsigned char _pad0[0x34];
    zWallNet* wallNet;
};

class zPlanktonShakeManager {
public:
    static bool IsBeingShaken(const xEnt* ent);
};

// The word at +0x10 is inside the entity base; only its offset and the
// value tested against are recovered. The flag byte at +0x70 is a run
// of one-bit flags, puppetMode first.
class zNPCBase : public zNPCBaseVirtuals {
public:
    unsigned char _pad0[0x10 - 0x4];
    unsigned int typeID;
    unsigned char _pad1[0x70 - 0x14];
    bool puppetMode : 1;
    bool alive : 1;
    bool present : 1;
    bool activated : 1;
    bool spawned : 1;
    bool paused : 1;
    bool updateInCinematicAlways : 1;
    bool updateInCinematicNever : 1;
    unsigned char _pad2[0x98 - 0x71];
    zNPCEntity* npcEntity;
    void* npcSteeringOld;
    zNPCSteering* npcSteering;
    zNPCPerception* npcPerception;
    zNPCCombat* npcCombat;
};

class zPlayerActionManager {
public:
    unsigned int GetCurrentActionID() const;

    unsigned char _pad0[0x38];
};

class zPlayerCommon {
public:
    unsigned char _pad0[0x34];
    World::xOGModel* model;
    unsigned char _pad1[0xC0 - 0x38];
    zPlayerActionManager actionManager;
    unsigned char _pad2[0x8B0 - 0xF8];
    int spongeBuffState;
};

class xGlobals {
public:
    unsigned char _pad0[0x428];
    zPlayerCommon* player;
};

extern xGlobals* xglobals;

class zVariableBase;

class zBlackboard {
public:
    template <class T>
    bool Read(unsigned int id, T& out) const;

    unsigned int size;
    zVariableBase** variables;
};

class zBTClient {
public:
    unsigned char _pad0[0x88];
    zBlackboard blackboard;
};

// zBTCondition is 0xC in the DWARF with two members named, so the word
// left over is the vtable pointer; nothing here dispatches through it.

class zNPCBTBlockedCondition {
public:
    bool Evaluate() const;

    Sext::ConditionBase* conditionAsset;
    zBTClient* btClient;
    unsigned char _vtable[4];
    zNPCBase* npcBase;
    Sext::ConditionBase* asset;
};

class zNPCBTBlockedTypeCondition {
public:
    bool Evaluate() const;

    Sext::ConditionBase* conditionAsset;
    zBTClient* btClient;
    unsigned char _vtable[4];
    zNPCBase* npcBase;
    Sext::ConditionBase* asset;
    Sext::eHitSource type;
};

class zNPCBTDamagedByTypeCondition {
public:
    bool Evaluate() const;

    Sext::ConditionBase* conditionAsset;
    zBTClient* btClient;
    unsigned char _vtable[4];
    zNPCBase* npcBase;
    Sext::ConditionBase* asset;
    Sext::eHitSource type;
};

class zNPCBTDamagedCondition {
public:
    void Setup(const Sext::ConditionBase* condition);
    bool Evaluate() const;

    Sext::ConditionBase* conditionAsset;
    zBTClient* btClient;
    unsigned char _vtable[4];
    zNPCBase* npcBase;
    Sext::ConditionBase* asset;
    bool nonzero;
};

class zNPCBTDamagedByKnockbackCondition {
public:
    bool Evaluate() const;

    Sext::ConditionBase* conditionAsset;
    zBTClient* btClient;
    unsigned char _vtable[4];
    zNPCBase* npcBase;
    Sext::ConditionBase* asset;
};

class zNPCBTPokedCondition {
public:
    bool Evaluate() const;

    Sext::ConditionBase* conditionAsset;
    zBTClient* btClient;
    unsigned char _vtable[4];
    zNPCBase* npcBase;
    Sext::ConditionBase* asset;
    int pokeType;
};

class zNPCBTInRPSAttackStateCondition {
public:
    void Setup(const Sext::ConditionBase* condition);
    bool Evaluate() const;

    Sext::ConditionBase* conditionAsset;
    zBTClient* btClient;
    unsigned char _vtable[4];
    zNPCBase* npcBase;
    Sext::ConditionBase* asset;
    Sext::eRPSAttackTypes state;
};

class zNPCBTIsPlanktonShakingCondition {
public:
    bool Evaluate() const;

    Sext::ConditionBase* conditionAsset;
    zBTClient* btClient;
    unsigned char _vtable[4];
    zNPCBase* npcBase;
    Sext::ConditionBase* asset;
};

class zNPCBTSBPlayerIsBuff {
public:
    bool Evaluate() const;

    Sext::ConditionBase* conditionAsset;
    zBTClient* btClient;
    unsigned char _vtable[4];
    zNPCBase* npcBase;
    Sext::ConditionBase* asset;
};

class zNPCBTIsInPuppetModeCondition {
public:
    bool Evaluate() const;

    Sext::ConditionBase* conditionAsset;
    zBTClient* btClient;
    unsigned char _vtable[4];
    zNPCBase* npcBase;
    Sext::ConditionBase* asset;
};

class zNPCBTCheckPerceptionCondition {
public:
    void Setup(const Sext::ConditionBase* condition);
    bool Evaluate() const;

    Sext::ConditionBase* conditionAsset;
    zBTClient* btClient;
    unsigned char _vtable[4];
    zNPCBase* npcBase;
    Sext::ConditionBase* asset;
    Sext::eNPCPerceptionType perceptionType;
    unsigned int targets;
    bool anded;
};

enum eNPCPerceptionTargetStatusType {
    eNPCPerceptionTargetStatus_TargetAlive = 0,
    eNPCPerceptionTargetStatus_TargetDead = 1,
    eNPCPerceptionTargetStatus_TargetFriendly = 2,
    eNPCPerceptionTargetStatus_TargetHostile = 3,
    END_eNPCPerceptionTargetStatus_ENUM = 4,
};

class zNPCBTCheckPerceptionTargetStatusCondition {
public:
    void Setup(const Sext::ConditionBase* condition);
    bool Evaluate() const;
    bool IsTargetAlive() const;

    Sext::ConditionBase* conditionAsset;
    zBTClient* btClient;
    unsigned char _vtable[4];
    zNPCBase* npcBase;
    Sext::ConditionBase* asset;
    eNPCPerceptionTargetStatusType perceptionTargetStatusType;
    unsigned int target;
};

class zNPCBTCheckPerceptionTargetChangedCondition {
public:
    void Setup(const Sext::ConditionBase* condition);
    bool Evaluate() const;

    Sext::ConditionBase* conditionAsset;
    zBTClient* btClient;
    unsigned char _vtable[4];
    zNPCBase* npcBase;
    Sext::ConditionBase* asset;
    xEnt* originalTarget;
};

class zNPCBTCheckPerceptionTargetInWallnetCondition {
public:
    bool Evaluate() const;

    Sext::ConditionBase* conditionAsset;
    zBTClient* btClient;
    unsigned char _vtable[4];
    zNPCBase* npcBase;
    Sext::ConditionBase* asset;
    unsigned int target;
};

enum CheckType {
    PerceptionTarget = 0,
    Self = 1,
    END_CheckTypeENUM = 2,
};

class zNPCBTIsInWallnetCondition {
public:
    void Setup(const Sext::ConditionBase* condition);
    bool Evaluate() const;

    Sext::ConditionBase* conditionAsset;
    zBTClient* btClient;
    unsigned char _vtable[4];
    zNPCBase* npcBase;
    Sext::ConditionBase* asset;
    CheckType check;
    unsigned int target;
};

class zNPCBTFacingPerceptionTargetCondition {
public:
    void Setup(const Sext::ConditionBase* condition);
    bool Evaluate() const;

    Sext::ConditionBase* conditionAsset;
    zBTClient* btClient;
    unsigned char _vtable[4];
    zNPCBase* npcBase;
    Sext::ConditionBase* asset;
    unsigned char target;
    float tolerance;
};

class zNPCBTBossSquidwardBlockPlayer {
public:
    bool Evaluate() const;

    Sext::ConditionBase* conditionAsset;
    zBTClient* btClient;
    unsigned char _vtable[4];
    zNPCBase* npcBase;
    Sext::ConditionBase* asset;
};

class zNPCBTBossSquidwardDestroyCover {
public:
    bool Evaluate() const;

    Sext::ConditionBase* conditionAsset;
    zBTClient* btClient;
    unsigned char _vtable[4];
    zNPCBase* npcBase;
    Sext::ConditionBase* asset;
};

bool zNPCBTBlockedCondition::Evaluate() const {
    zNPCCombat* combat = npcBase->npcCombat;

    if (combat != 0) {
        return combat->blockListSize != 0;
    }

    return false;
}

// The asset byte is read first and the pointer stored before it: retail
// has the load scheduled over the pointer's store.
void zNPCBTInRPSAttackStateCondition::Setup(
    const Sext::ConditionBase* condition) {
    asset = (Sext::ConditionBase*)condition;
    state = (Sext::eRPSAttackTypes)(
        (const Sext::InRPSAttackStateCondition*)condition)
                ->state;
}

bool zNPCBTInRPSAttackStateCondition::Evaluate() const {
    zNPCCombat* combat = npcBase->npcCombat;

    if (combat != 0) {
        return state == combat->attackState;
    }

    return false;
}

bool zNPCBTBlockedTypeCondition::Evaluate() const {
    zNPCCombat* combat = npcBase->npcCombat;

    if (combat != 0) {
        return combat->BlockedType(type);
    }

    return false;
}

bool zNPCBTDamagedByTypeCondition::Evaluate() const {
    zNPCCombat* combat = npcBase->npcCombat;

    if (combat != 0 && combat->HitByType(type)) {
        return true;
    }

    return false;
}

void zNPCBTDamagedCondition::Setup(const Sext::ConditionBase* condition) {
    asset = (Sext::ConditionBase*)condition;
    nonzero = ((const Sext::DamagedCondition*)condition)->nonzero;
}

// Without the flag any damage this frame counts; with it only damage
// that actually took hit points.
bool zNPCBTDamagedCondition::Evaluate() const {
    zNPCCombat* combat = npcBase->npcCombat;

    if (combat != 0) {
        if (!nonzero) {
            return combat->damageListSize != 0;
        }

        for (unsigned int i = 0; i < combat->damageListSize; i++) {
            if (combat->GetDamageInfo(i)->damageHP > 0.0f) {
                return true;
            }
        }
    }

    return false;
}

bool zNPCBTIsPlanktonShakingCondition::Evaluate() const {
    return zPlanktonShakeManager::IsBeingShaken((const xEnt*)npcBase->npcEntity);
}

bool zNPCBTDamagedByKnockbackCondition::Evaluate() const {
    zNPCCombat* combat = npcBase->npcCombat;

    if (combat != 0) {
        for (unsigned int i = 0; i < combat->damageListSize; i++) {
            if (combat->GetDamageInfo(i)->flags & 1) {
                return true;
            }
        }
    }

    return false;
}

bool zNPCBTPokedCondition::Evaluate() const {
    switch (pokeType) {
    case 0:
        return npcBase->npcEntity->pokedA;
    case 1:
        return npcBase->npcEntity->pokedB;
    }

    return false;
}

bool zNPCBTSBPlayerIsBuff::Evaluate() const {
    return xglobals->player->spongeBuffState == 1;
}

void zNPCBTCheckPerceptionCondition::Setup(
    const Sext::ConditionBase* condition) {
    const Sext::CheckPerceptionCondition* c =
        (const Sext::CheckPerceptionCondition*)condition;

    targets = c->targets;
    perceptionType = (Sext::eNPCPerceptionType)c->perceptionType;
    anded = c->anded;
}

bool zNPCBTCheckPerceptionCondition::Evaluate() const {
    zNPCPerception* perception = npcBase->npcPerception;

    if (perception != 0) {
        return perception->AreTargetsPerceived(targets, perceptionType,
                                               anded);
    }

    return false;
}

void zNPCBTCheckPerceptionTargetStatusCondition::Setup(
    const Sext::ConditionBase* condition) {
    const Sext::CheckPerceptionTargetStatusCondition* c =
        (const Sext::CheckPerceptionTargetStatusCondition*)condition;

    target = c->target - 1;
    perceptionTargetStatusType = (eNPCPerceptionTargetStatusType)c->status;
}

bool zNPCBTCheckPerceptionTargetStatusCondition::Evaluate() const {
    switch (perceptionTargetStatusType) {
    case eNPCPerceptionTargetStatus_TargetAlive:
        return IsTargetAlive();
    case eNPCPerceptionTargetStatus_TargetDead:
        return !IsTargetAlive();
    case eNPCPerceptionTargetStatus_TargetFriendly:
        return false;
    case eNPCPerceptionTargetStatus_TargetHostile:
        return true;
    }

    return false;
}

// An NPC is alive by its flag; the three other kinds of target are
// alive while they have health left.
bool zNPCBTCheckPerceptionTargetStatusCondition::IsTargetAlive() const {
    zNPCPerception* perception = npcBase->npcPerception;

    if (perception != 0) {
        xEnt* ent = perception->targets[target].targetEnt;

        if (ent == 0) {
            return false;
        }

        switch (ent->baseType) {
        case 0x38:
            return ((zNPCEntity*)ent)->owner->alive;
        case 0x55:
            return !(((zHealthEnt*)ent)->GetHealth() <= 0.0f);
        case 0x56: {
            zHitPoints* hp = ((zHitPointsEnt56*)ent)->hitPoints;

            if (hp != 0) {
                return !((float)(hp->total - hp->taken) <= 0.0f);
            }

            return false;
        }
        case 0x5A: {
            zHitPoints* hp = ((zHitPointsEnt5A*)ent)->hitPoints;

            if (hp != 0) {
                return !((float)(hp->total - hp->taken) <= 0.0f);
            }

            return false;
        }
        }

        return false;
    }

    return false;
}

void zNPCBTCheckPerceptionTargetChangedCondition::Setup(
    const Sext::ConditionBase* condition) {
    originalTarget = 0;

    zNPCPerception* perception = npcBase->npcPerception;

    if (perception != 0) {
        originalTarget = perception->targets[0].targetEnt;
    }
}

bool zNPCBTCheckPerceptionTargetChangedCondition::Evaluate() const {
    zNPCPerception* perception = npcBase->npcPerception;

    if (perception != 0 &&
        originalTarget != perception->targets[0].targetEnt) {
        return true;
    }

    return false;
}

// Only the player-sized kinds of entity -- NPCs and the three with
// health -- can be in a wall net.
bool zNPCBTCheckPerceptionTargetInWallnetCondition::Evaluate() const {
    zNPCBase* npc = npcBase;

    if (npc->typeID != 0xF0) {
        return false;
    }

    zNPCPerception* perception = npc->npcPerception;

    if (perception == 0) {
        return false;
    }

    xEnt* ent = perception->targets[target].targetEnt;

    if (ent == 0) {
        return false;
    }

    zWallNet* wallNet = npc->npcSteering->wallNet;

    if (wallNet == 0) {
        return false;
    }

    unsigned int type = ent->baseType;

    if (type == 0x55 || type == 0x56 || type == 0x38 || type == 0x5A) {
        return wallNet->IsInsideWallNetXZ(xEntGetCenter(ent));
    }

    return false;
}

bool zNPCBTIsInPuppetModeCondition::Evaluate() const {
    zNPCBase* npc = npcBase;

    if (npc->typeID != 0xF0) {
        return false;
    }

    return npc->puppetMode;
}

void zNPCBTIsInWallnetCondition::Setup(const Sext::ConditionBase* condition) {
    const Sext::IsInWallnetCondition* c =
        (const Sext::IsInWallnetCondition*)condition;

    check = (CheckType)c->check;
    target = c->target - 1;
}

bool zNPCBTIsInWallnetCondition::Evaluate() const {
    zNPCBase* npc = npcBase;

    if (npc->typeID != 0xF0) {
        return false;
    }

    zWallNet* wallNet = npc->npcSteering->wallNet;

    if (wallNet == 0) {
        return false;
    }

    xEnt* ent = 0;

    switch (check) {
    case PerceptionTarget: {
        zNPCPerception* perception = npc->npcPerception;

        if (perception != 0) {
            ent = perception->targets[target].targetEnt;
        }

        break;
    }
    case Self:
        ent = npc->npcEntity;
        break;
    }

    if (ent == 0) {
        return false;
    }

    unsigned int type = ent->baseType;

    if (type == 0x55 || type == 0x56 || type == 0x38 || type == 0x5A) {
        return wallNet->IsInsideWallNet(xEntGetCenter(ent));
    }

    return false;
}

// The asset gives the tolerance in degrees.
void zNPCBTFacingPerceptionTargetCondition::Setup(
    const Sext::ConditionBase* condition) {
    const Sext::FacingPerceptionTargetCondition* c =
        (const Sext::FacingPerceptionTargetCondition*)condition;

    target = c->target - 1;
    tolerance = 0.017453292f * c->tolerance;
}

// Headings are compared on the ground plane, both clamped to [0, 2pi).
bool zNPCBTFacingPerceptionTargetCondition::Evaluate() const {
    zNPCPerception* perception = npcBase->npcPerception;

    if (perception != 0) {
        xVec3 toTarget;

        toTarget.Sub(perception->targets[target].targetEnt->model->Mat.pos,
                     npcBase->npcEntity->model->Mat.pos);

        float facing = xClampAngle0_2PI(
            std::atan2(npcBase->npcEntity->model->Mat.at.z,
                       npcBase->npcEntity->model->Mat.at.x));
        float angle = xClampAngle0_2PI(std::atan2(toTarget.z, toTarget.x));

        if (std::fabs(angle - facing) < tolerance) {
            return true;
        }
    }

    return false;
}

inline float std::atan2(float y, float x) { return ::atan2(y, x); }

// Action 0x23 is the player's; the blackboard can veto the block.
bool zNPCBTBossSquidwardBlockPlayer::Evaluate() const {
    if (xglobals->player->actionManager.GetCurrentActionID() == 0x23) {
        unsigned int id = xStrHash("DO_NOT_DISTURB");
        int value = -1;

        btClient->blackboard.Read(id, value);

        return value == 0;
    }

    return false;
}

// Cover is destroyed when bone 19 cannot see the player.
bool zNPCBTBossSquidwardDestroyCover::Evaluate() const {
    xVec3 playerPos = xglobals->player->model->Mat.pos;
    xMat4x3 boneMat;

    xModelGetBoneMatNoScale(boneMat, *npcBase->npcEntity->model, 19);

    xVec3 bonePos = boneMat.pos;

    return !zNPCPerception::CheckLineOfSight(&bonePos, &playerPos,
                                             npcBase->npcEntity, 0,
                                             Sext::eCollisionLayer_Player);
}

// FIVE ASSET CREATES THAT ARE ONE TAIL CALL EACH: `li r5,<id>; b`
// into the NPC manager, with the id in the third argument. A
// non-template symbol does not carry its return type, so what these
// hand back is only known to be whatever the manager returns.

namespace World { class EntityHandleBase; }

namespace Sext {
class NPCAsset;
class NPCGroupAsset;

class AnimViewer {
public:
    static xBase* Create(World::EntityHandleBase* handle,
                     AnimViewer* asset);
};

class NPCGeneric {
public:
    static xBase* Create(World::EntityHandleBase* handle,
                     NPCGeneric* asset);
};

class GenericSpawner {
public:
    static xBase* Create(World::EntityHandleBase* handle,
                     GenericSpawner* asset);
};

class GenericSwarm {
public:
    static xBase* Create(World::EntityHandleBase* handle,
                     GenericSwarm* asset);
};

class NPCGroupCircle {
public:
    static xBase* Create(World::EntityHandleBase* handle,
                     NPCGroupCircle* asset);
};

}  // namespace Sext

class zNPCManager {
public:
    static xBase* CreateNPC(World::EntityHandleBase* handle,
                            Sext::NPCAsset* asset, unsigned int type);
    static xBase* CreateNPCGroup(World::EntityHandleBase* handle,
                                 Sext::NPCGroupAsset* asset,
                                 unsigned int type);
};

xBase* Sext::AnimViewer::Create(World::EntityHandleBase* handle,
                                   AnimViewer* asset) {
    return zNPCManager::CreateNPC(handle, (Sext::NPCAsset*)asset,
                                  400);
}

xBase* Sext::NPCGeneric::Create(World::EntityHandleBase* handle,
                                   NPCGeneric* asset) {
    return zNPCManager::CreateNPC(handle, (Sext::NPCAsset*)asset,
                                  432);
}

xBase* Sext::GenericSpawner::Create(World::EntityHandleBase* handle,
                                       GenericSpawner* asset) {
    return zNPCManager::CreateNPC(handle, (Sext::NPCAsset*)asset,
                                  448);
}

xBase* Sext::GenericSwarm::Create(World::EntityHandleBase* handle,
                                     GenericSwarm* asset) {
    return zNPCManager::CreateNPC(handle, (Sext::NPCAsset*)asset,
                                  416);
}

xBase* Sext::NPCGroupCircle::Create(World::EntityHandleBase* handle,
                                    NPCGroupCircle* asset) {
    return zNPCManager::CreateNPCGroup(
        handle, (Sext::NPCGroupAsset*)asset, 32);
}
