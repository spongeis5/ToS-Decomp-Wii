#include "SB/GM/Engine/Game/zNPCCommonCombatBTActions.pool.h"

// zNPCCommonCombatBTActions.cpp -- the behaviour-tree actions of an NPC in a
// fight: the boss meter, being defeated, stunned, hit or striking, damaging
// the player on contact or in range, killing and removing itself, the
// invulnerable and undamageable flags, the ninja manager's attack slots, hit
// points and the hit profile. Read from the image with tools/brief.py; the
// layouts are the DWARF's (tools/dwarf_types.py).
//
// An action keeps its asset, resume callback and client in front of the
// vtable pointer at +0xC, and the NPC at +0x10. Only InflictPlayerDamage is
// declared virtual, because the contact listener calls it through the
// vtable; the other members are declared non-virtual.

class xBase;
class zBTClient;
class zNPCBase;
class zNPCEntity;
class hkpCharacterProxy;
class hkContactPoint;

inline void* operator new(unsigned long size, void* mem) { return mem; }

extern "C" void* memset(void* dst, int c, unsigned long n);

namespace Sext {

class ActionBase {};
class EventAny {};
class EventActionNew : public EventAny {};

enum eHitSource {
    eHitSourceEVENT = 0,
    END_eHitSourceENUM = 62
};

enum eBossMeterType {
    eBossMeterType_Miniboss1 = 0,
    END_eBossMeterType_ENUM = 6
};

class EventActionShowBossMeter : public EventActionNew {
public:
    eBossMeterType BossType;
};

class EventActionOneFloat : public EventActionNew {
public:
    float param0;
};

class EventActionDamagePlayer : public EventActionNew {
public:
    float param0;
    eHitSource param1;
    bool param2;
};

// The assets of the boss meter, defeated, attack-request and flag actions
// are not in the DWARF; their field names are ours.
class Action_NPC_BossMeterShow : public ActionBase {
public:
    eBossMeterType BossType;
};

class Action_NPC_Defeated : public ActionBase {
public:
    bool KillImmediately;
};

class Action_NPC_DamagePlayerOnContact : public ActionBase {
public:
    float damage;
    eHitSource SourceType;
};

class Action_NPC_DamagePlayerInRange : public ActionBase {
public:
    unsigned int options;
    float range;
    float damage;
    eHitSource SourceType;
};

class Action_NPC_RequestAttack : public ActionBase {
public:
    float AttackTime;
    unsigned int PlayerVariable;
};

class Action_NPC_ReleaseAttack : public ActionBase {
public:
    unsigned int PlayerVariable;
    float Delay;
};

// The new hit points: the maximum (0), a constant (1) or a blackboard
// variable (2).
class Action_NPC_Set_CombatCurHitPoints : public ActionBase {
public:
    union {
        float value;
        unsigned int variable;
    } UnionCurHitPoints;
    unsigned char newCurHitPoints;
};

class Action_NPC_SetHitProfile : public ActionBase {
public:
    unsigned char HitProfile;
};

class Action_NPC_SetInvulnerable : public ActionBase {
public:
    bool Invulnerable;
};

class Action_NPC_SetUndamageable : public ActionBase {
public:
    bool Undamageable;
};

class vec3 {
public:
    float x;
    float y;
    float z;
};

class uid {
public:
    unsigned long long internalUid;
};

enum eShootTo {
    eShootTo_Direction = 0,
    eShootTo_Target = 1,
    END_eShootTo_ENUM = 2
};

// What the projectile is aimed along. The union's arms are not in the
// DWARF: a vector in the NPC's space and which space (4) or axis (0 at,
// 1 up, 2 right, 3 a bone's at) to use, the axis's sign flag or the bone,
// or an offset from the target's centre.
class __shootToUnion__ {
public:
    eShootTo type;
    union {
        struct {
            vec3 vector;
            unsigned char space;
        } direction;
        unsigned char flag;
    } u;
};

class Action_NPC_Shoot : public ActionBase {
public:
    int bone;
    uid projectileAsset;
    union {
        float speed;
    } unionLaunchSpeed;
    unsigned char launchSpeedOption;
    vec3 launchPositionOffset;
    eShootTo shootTo;
    __shootToUnion__ shootToUnion;
};

class ProjectileAsset {
public:
    unsigned char _pad0[0x108];
    float launchSpeed;
};

}  // namespace Sext

enum ForceEvent {
    FE_YES = 0,
    FE_NO = 1
};

enum zHitTarget {
    zHT_GENERAL = 0,
    zHT_COUNT = 8
};

enum eTaskState {
    eTaskState_Unknown = 0,
    eTaskState_Running = 1,
    eTaskState_Suspend = 2,
    eTaskState_Complete = 3,
    eTaskState_Fail = 4,
    eTaskState_Abort = 5
};

void zEntEvent(xBase* from, unsigned int fromEvent, xBase* to,
               unsigned int toEvent, Sext::EventAny* param, ForceEvent force);
void zEntEventAllOfType(xBase* from, unsigned int fromEvent,
                        unsigned int toEvent, Sext::EventAny* param,
                        unsigned int type, ForceEvent force);

// ---------------------------------------------------------------------------
// The engine's objects as the actions reach them

class xVec3 {
public:
    float length2() const;
    float NormalizeSafe();
    xVec3& operator=(const xVec3& other);
    xVec3& operator+=(const xVec3& other);
    xVec3& operator*=(float s);
    void negate();
    xVec3& safe_normalize(const xVec3& fallback);

    static const xVec3 m_Null;

    float x;
    float y;
    float z;
};

xVec3 operator-(const xVec3& a, const xVec3& b);
xVec3 operator+(const xVec3& a, const xVec3& b);
xVec3 operator*(const xVec3& v, float s);

// An xVec3 built from three floats is a call to this; the linker folded
// the vector constructor into Math::Vector's identical body.
extern "C" void __ct__Q24Math6VectorFfff(void* v, float x, float y, float z);

class xMat3x3 {
public:
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

namespace World {

class xOGModel {
public:
    xMat4x3 Mat;
};

}  // namespace World

namespace Domains {

class Blobloid {
public:
    void* BlobData() const;
};

}  // namespace Domains

namespace World {

class EntityHandleBase : public Domains::Blobloid {};

class EntityManager {
public:
    static EntityHandleBase* FindHandle(unsigned long long id);
};

}  // namespace World

class xEnt {
public:
    unsigned char _pad0[0x34];
    World::xOGModel* model;
};

void xMat3x3RMulVec(xVec3* o, const xMat3x3* m, const xVec3* v);
void xMat3x3LookVec(xMat3x3* m, const xVec3* at);
int xModelGetBoneCount(const World::xOGModel* model);
void xModelGetBoneMatNoScale(xMat4x3& mat, const World::xOGModel& model,
                             unsigned long bone);
void xEntGetCenterFromAABB(const xEnt* ent, xVec3& center);

class zProjectileLaunchData {
public:
    zProjectileLaunchData();
    void SetupLaunchToDirection(xMat4x3 mat, const xVec3& dir, float speed);

    unsigned char _pad0[0x6C];
};

class zProjectileManager {
public:
    static void sEmit(World::EntityHandleBase* handle,
                      const zProjectileLaunchData& data, xBase* owner);
};

class xBase {
public:
    unsigned char _pad0[0x20];
    unsigned int baseType;
};

namespace Memory {

enum eFactoryMemType { eFactoryMemType_ = 0x7FFFFFFF };

class Factory {
public:
    void* AllocMem(unsigned int size, eFactoryMemType type);
    void DeallocMem(void* mem);
};

}  // namespace Memory

class zNPCManager {
public:
    static Memory::Factory factory;
};

class zCombatDamageInfo {
public:
    zCombatDamageInfo(xBase* from, float damage, Sext::eHitSource source,
                      zHitTarget target, const xVec3& knockback,
                      const xVec3& hitLocation, const xVec3& hitNormal,
                      int attackID, int flags);

    int flags;
    xBase* from;
    float damage;
    Sext::eHitSource source;
    zHitTarget target;
    xVec3 knockback;
    xVec3 hitLocation;
    xVec3 hitNormal;
    int attackID;
};

// zPlayer is polymorphic from +0; slot 57 is Damage (tools/vtslot.py
// __vt__7zPlayer 236).
#define V10(p, a) virtual void p##a##0(); virtual void p##a##1(); \
    virtual void p##a##2(); virtual void p##a##3(); \
    virtual void p##a##4(); virtual void p##a##5(); \
    virtual void p##a##6(); virtual void p##a##7(); \
    virtual void p##a##8(); virtual void p##a##9();

class zPlayerVirtuals {
public:
    V10(_p, 0) V10(_p, 1) V10(_p, 2) V10(_p, 3) V10(_p, 4)
    virtual void _p50();
    virtual void _p51();
    virtual void _p52();
    virtual void _p53();
    virtual void _p54();
    virtual void _p55();
    virtual void _p56();
    virtual void Damage(const zCombatDamageInfo& info);
};

class zPlayer : public zPlayerVirtuals {
public:
    unsigned char _pad0[0x34 - 0x4];
    World::xOGModel* model;
    unsigned char _pad1[0xF8 - 0x38];
    int playerIndex;
};

class zPlayerContainer {
public:
    // Read through this, the player is loaded before the damage info it is
    // handed is built, as retail loads it.
    zPlayer* GetPlayer(int i) { return playerArray[i]; }

    zPlayer* playerArray[4];
    int numPlayers;
};

class xGlobals {
public:
    unsigned char _pad0[0x428];
    zPlayerContainer players;
};

extern xGlobals* xglobals;

// Havok's character proxy and the listener the contact action registers on
// the NPC's.
class hkpWorldObjectData {
public:
    unsigned char _pad0[0xC];
    unsigned long m_userData;
};

class hkpShapePhantom : public hkpWorldObjectData {};

class hkpCharacterProxy {
public:
    hkpShapePhantom* getShapePhantom();
};

class hkpCharacterProxyListener {
public:
    virtual void _h0();
};

class zNPCCollisionListener : public hkpCharacterProxyListener {
public:
    zNPCCollisionListener();

    zNPCBase* npc;
};

class zNPCBTDamagePlayerOnContactAction;

class PlayerCollisionListener : public zNPCCollisionListener {
public:
    virtual void characterInteractionCallback(hkpCharacterProxy* proxy,
                                              hkpCharacterProxy* otherProxy,
                                              const hkContactPoint& point);

    zNPCBTDamagePlayerOnContactAction* parentAction;
};

class zNPCEntity {
public:
    bool IsAnimationStopped(unsigned int animID);
    void KillVelocity();
    void RegisterCollisionListener(zNPCCollisionListener* listener);
    void UnregisterCollisionListener(zNPCCollisionListener* listener);

    unsigned char _pad0[0x34];
    World::xOGModel* model;
};

// A component: the owner, then the vtable pointer at +4.
class zNPCComponent {
public:
    zNPCBase* owner;

    virtual void _c0();
    virtual void _c1();
    virtual void _c2();
    virtual void _c3();
    virtual void _c4();
    virtual void _c5();
    virtual void _c6();
    virtual void _c7();
    virtual void _c8();
    virtual void _c9();
};

class zNPCSteeringControl {
public:
    unsigned char _pad0[0x4C];
};

class zNPCSteeringStopControl : public zNPCSteeringControl {};

// Slots 10, 11 and 14 are folded bodies in every steering vtable in the
// image, so they carry the names the movement unit gives them.
class zNPCSteering : public zNPCComponent {
public:
    virtual void _s10(zNPCSteeringControl* control);
    virtual void _s11(zNPCSteeringControl* control);
    virtual void _s12();
    virtual void _s13();
    virtual void _s14(float dt);
};

class zCombat {
public:
    unsigned char _pad0[0x34];
    float GetCurHitPoints() const { return currentHitPoints; }
    float GetMaxHitPoints() const { return maximumHitPoints; }

    float currentHitPoints;
    float maximumHitPoints;
    unsigned char _pad1[0x25A - 0x3C];
    unsigned short hitProfile;
    unsigned char _pad2[0x260 - 0x25C];
};

class zNPCCombat : public zNPCComponent {
public:
    void SetCurHitPoints(float hp);

    int attackID;
    unsigned char _pad0[0x10 - 0xC];
    zCombat baseCombat;
    unsigned char _pad1[0x284 - 0x270];
    bool hitsDisabled;
    bool damageDisabled;

    // Our names for these. The division has to be an inline's reading both
    // through accessors, or mwcc stores once after the branch and loads the
    // two hit points the other way round.
    float GetHitPointsFraction() const {
        return baseCombat.GetCurHitPoints() / baseCombat.GetMaxHitPoints();
    }
};

class zNPCPerceptionTarget {
public:
    xEnt* targetEnt;
    unsigned char _pad0[0x74 - 0x4];
};

class zNPCPerception : public zNPCComponent {
public:
    unsigned char _pad0[0x10 - 0x8];
    zNPCPerceptionTarget targets[4];
};

// zNPCBase is polymorphic from +0; slots 28 and 29 are named by the swarm's
// overrides (tools/vtslot.py __vt__16zNPCGenericSwarm 120 124).
class zNPCBaseVirtuals {
public:
    V10(_n, 0) V10(_n, 1)
    virtual void _n20();
    virtual void _n21();
    virtual void _n22();
    virtual void _n23();
    virtual void _n24();
    virtual void _n25();
    virtual void _n26();
    virtual void _n27();
    virtual int GetNumberOfChildren();
    virtual zNPCEntity* GetChild(int index) const;
};

#undef V10

class zNPCBase : public zNPCBaseVirtuals {
public:
    void Kill(bool quiet);
    void Remove();

    unsigned char _pad0[0x98 - 0x4];
    zNPCEntity* npcEntity;
    void* npcSteeringOld;
    zNPCSteering* npcSteering;
    zNPCPerception* npcPerception;
    zNPCCombat* npcCombat;
};

class zNPCNinjaManager;
extern zNPCNinjaManager gNPCNinjaManager;

// The underscored members are reached through static inlines; called
// directly, the float argument of _ReleaseAttack is loaded last instead of
// first.
class zNPCNinjaManager {
public:
    static bool RequestAttack(const zNPCBase* npc, int playerIndex,
                              float attackTime);
    static void ReleaseAttack(const zNPCBase* npc, int playerIndex,
                              float delay);

    bool _RequestAttack(const zNPCBase* npc, int playerIndex, float attackTime);
    void _ReleaseAttack(const zNPCBase* npc, int playerIndex, float delay);
};

inline bool zNPCNinjaManager::RequestAttack(const zNPCBase* npc,
                                            int playerIndex,
                                            float attackTime) {
    return gNPCNinjaManager._RequestAttack(npc, playerIndex, attackTime);
}

inline void zNPCNinjaManager::ReleaseAttack(const zNPCBase* npc,
                                            int playerIndex, float delay) {
    gNPCNinjaManager._ReleaseAttack(npc, playerIndex, delay);
}

// ---------------------------------------------------------------------------
// The blackboard, as zBlackboard.cpp spells it; every Read and Write is
// zBlackboard.cpp's.

class zBlackboard {
public:
    template <class T>
    bool Write(unsigned int id, const T& value);
    template <class T>
    bool Read(unsigned int id, T& out) const;

    unsigned int size;
    void** variables;
};

class zBTClient {
public:
    unsigned char _pad0[0x88];
    zBlackboard blackboard;
};

// File statics of the unity build (symbols.txt: scope:local).
extern const unsigned int NPC_VAR_CURRENT_PLAYER;
extern const unsigned int NPC_VAR_HIT_COUNT;

// ---------------------------------------------------------------------------
// The actions

class zNPCBTActionAnim {
public:
    void Init(const char* name, float blend, float start);
    void StartOnNPC(zNPCEntity* npc, bool force);

    const char* animationName;
    unsigned int animStateID;
    float blendTime;
    float animStartTime;
    float leanMaxAngle;
    bool enabled;
};

class zBTAction {
public:
    const Sext::ActionBase* actionAsset;
    void* resumeCB;
    zBTClient* btClient;

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
};

class zNPCBTAction : public zBTAction {
public:
    zNPCBase* npcBase;
};

class zNPCBTActionBossMeterShow : public zNPCBTAction {
public:
    eTaskState Update(float dt);
};

class zNPCBTActionBossMeterHide : public zNPCBTAction {
public:
    eTaskState Update(float dt);
};

class zNPCBTActionBossMeterSet : public zNPCBTAction {
public:
    eTaskState Update(float dt);
};

class zNPCBTDefeatedAction : public zNPCBTAction {
public:
    void Begin();
    eTaskState Update(float dt);

    zNPCSteeringStopControl stopControl;
    zNPCBTActionAnim actionAnim;
};

class zNPCBTStunAction : public zNPCBTAction {
public:
    void Begin();
    eTaskState Update(float dt);

    zNPCSteeringStopControl stopControl;
    zNPCBTActionAnim actionAnim;
};

class zNPCBTHitAction : public zNPCBTAction {
public:
    void Begin();
    void End();

    zNPCSteeringStopControl stopControl;
    zNPCBTActionAnim actionAnim;
};

// Slot 10 is InflictPlayerDamage (tools/vtslot.py
// __vt__33zNPCBTDamagePlayerOnContactAction 48).
class zNPCBTDamagePlayerOnContactAction : public zNPCBTAction {
public:
    void Begin();
    void Cleanup();
    virtual void InflictPlayerDamage(xBase* base);

    zNPCCollisionListener* collisionListener;
};

class zNPCBTDamagePlayerInRangeAction : public zNPCBTAction {
public:
    bool CheckNPCEntityAgainstPlayers(const zNPCEntity* npcEnt);
    void Begin();
    eTaskState Update(float dt);

    float damageTimers[4];
};

class zNPCBTShootAction : public zNPCBTAction {
public:
    eTaskState Update(float dt);
};

class zNPCBTKillAction : public zNPCBTAction {
public:
    eTaskState Update(float dt);
};

class zNPCBTRemoveAction : public zNPCBTAction {
public:
    eTaskState Update(float dt);
};

class zNPCBTSetInvulnerableAction : public zNPCBTAction {
public:
    eTaskState Update(float dt);
};

class zNPCBTSetUndamageableAction : public zNPCBTAction {
public:
    eTaskState Update(float dt);
};

class zNPCBTStrikeAction : public zNPCBTAction {
public:
    void Begin();

    zNPCSteeringStopControl stopControl;
    zNPCBTActionAnim actionAnim;
};

class zNPCBTRequestAttackAction : public zNPCBTAction {
public:
    eTaskState Update(float dt);
};

class zNPCBTReleaseAttackAction : public zNPCBTAction {
public:
    eTaskState Update(float dt);
};

class zNPCBTSetCurrentHitPointsAction : public zNPCBTAction {
public:
    eTaskState Update(float dt);
};

class zNPCBTSetHitProfileAction : public zNPCBTAction {
public:
    eTaskState Update(float dt);
};

// ---------------------------------------------------------------------------
// The boss meter

eTaskState zNPCBTActionBossMeterShow::Update(float dt) {
    Sext::EventActionShowBossMeter params;

    params.BossType =
        ((const Sext::Action_NPC_BossMeterShow*)actionAsset)->BossType;
    zEntEventAllOfType(0, 0, 0x18E5FFCB, &params, 219, FE_NO);

    return eTaskState_Complete;
}

eTaskState zNPCBTActionBossMeterHide::Update(float dt) {
    zEntEventAllOfType(0, 0, 0xA06364B2, 0, 219, FE_NO);

    return eTaskState_Complete;
}

// The meter shows the fraction of hit points left, or full for an NPC with
// no combat component.
eTaskState zNPCBTActionBossMeterSet::Update(float dt) {
    zNPCCombat* combat = npcBase->npcCombat;
    Sext::EventActionOneFloat params;

    if (combat != 0) {
        params.param0 = combat->GetHitPointsFraction();
    } else {
        params.param0 = 1.0f;
    }

    zEntEventAllOfType(0, 0, 0x765DDA2E, &params, 219, FE_NO);

    return eTaskState_Complete;
}

// ---------------------------------------------------------------------------
// Defeated, stunned, hit

void zNPCBTDefeatedAction::Begin() {
    if (!((const Sext::Action_NPC_Defeated*)actionAsset)->KillImmediately) {
        actionAnim.Init("DEFEATED", 0.2f, 0.0f);
        actionAnim.StartOnNPC(npcBase->npcEntity, false);
    }

    npcBase->npcSteering->_s10(&stopControl);
}

eTaskState zNPCBTDefeatedAction::Update(float dt) {
    bool killImmediately =
        ((const Sext::Action_NPC_Defeated*)actionAsset)->KillImmediately;

    npcBase->npcSteering->_s14(dt);

    if (killImmediately ||
        npcBase->npcEntity->IsAnimationStopped(actionAnim.animStateID)) {
        npcBase->Kill(false);
        npcBase->Remove();

        return eTaskState_Complete;
    }

    return eTaskState_Running;
}

void zNPCBTStunAction::Begin() {
    zEntEvent(0, 0, (xBase*)npcBase->npcEntity, 0xFBE70787, 0, FE_NO);

    npcBase->npcEntity->KillVelocity();

    actionAnim.Init("STUN", 0.2f, 0.0f);
    actionAnim.StartOnNPC(npcBase->npcEntity, false);
    npcBase->npcSteering->_s10(&stopControl);
}

eTaskState zNPCBTStunAction::Update(float dt) {
    npcBase->npcSteering->_s14(dt);

    return npcBase->npcEntity->IsAnimationStopped(actionAnim.animStateID)
               ? eTaskState_Complete
               : eTaskState_Running;
}

void zNPCBTHitAction::Begin() {
    actionAnim.Init("HIT", 0.2f, 0.0f);
    actionAnim.StartOnNPC(npcBase->npcEntity, false);
    npcBase->npcSteering->_s10(&stopControl);
}

// Counts the hit on the blackboard.
void zNPCBTHitAction::End() {
    npcBase->npcSteering->_s11(&stopControl);

    int hitCount;

    btClient->blackboard.Read(NPC_VAR_HIT_COUNT, hitCount);
    hitCount++;
    btClient->blackboard.Write(NPC_VAR_HIT_COUNT, hitCount);
}

// ---------------------------------------------------------------------------
// Damaging the player

// A player's proxy touching the NPC's: base type 0x55 is the player.
void PlayerCollisionListener::characterInteractionCallback(
    hkpCharacterProxy* proxy, hkpCharacterProxy* otherProxy,
    const hkContactPoint& point) {
    hkpShapePhantom* pPhantom = otherProxy->getShapePhantom();

    if (pPhantom != 0) {
        xBase* base = (xBase*)pPhantom->m_userData;

        if (base != 0 && base->baseType == 0x55) {
            parentAction->InflictPlayerDamage(base);
        }
    }
}

// The allocation is declared first so that the listener, not the entity,
// is the value left in r31.
void zNPCBTDamagePlayerOnContactAction::Begin() {
    void* mem;
    zNPCEntity* npcEnt = npcBase->npcEntity;

    if (npcEnt != 0) {
        mem = zNPCManager::factory.AllocMem(
            sizeof(PlayerCollisionListener), (Memory::eFactoryMemType)12);

        collisionListener = !mem ? 0 : new (mem) PlayerCollisionListener;
        ((PlayerCollisionListener*)collisionListener)->parentAction = this;
        npcEnt->RegisterCollisionListener(collisionListener);
    } else {
        collisionListener = 0;
    }
}

zNPCCollisionListener::zNPCCollisionListener() {}

void zNPCBTDamagePlayerOnContactAction::Cleanup() {
    zNPCEntity* npcEnt = npcBase->npcEntity;

    if (npcEnt != 0 && collisionListener != 0) {
        npcEnt->UnregisterCollisionListener(collisionListener);
        zNPCManager::factory.DeallocMem(collisionListener);
        collisionListener = 0;
    }
}

void zNPCBTDamagePlayerOnContactAction::InflictPlayerDamage(xBase* base) {
    const Sext::Action_NPC_DamagePlayerOnContact* damageInfo =
        (const Sext::Action_NPC_DamagePlayerOnContact*)actionAsset;
    Sext::EventActionDamagePlayer params;

    params.param0 = damageInfo->damage;
    params.param1 = damageInfo->SourceType;
    params.param2 = false;

    zEntEvent((xBase*)npcBase->npcEntity, 0, base, 0x07C60BE0, &params, FE_NO);
}

// Every player in range whose timer has run out is damaged, knocked away
// from the NPC in the horizontal plane, and timed out for 0.2 seconds.
bool zNPCBTDamagePlayerInRangeAction::CheckNPCEntityAgainstPlayers(
    const zNPCEntity* npcEnt) {
    const Sext::Action_NPC_DamagePlayerInRange* asset =
        (const Sext::Action_NPC_DamagePlayerInRange*)actionAsset;
    float range = asset->range;
    float damage = asset->damage;
    float check2 = range * range;

    bool hit = false;

    for (int i = 0; i < xglobals->players.numPlayers; i++) {
        if (damageTimers[xglobals->players.GetPlayer(i)->playerIndex] >
            0.0f) {
            continue;
        }

        xVec3 diff = npcEnt->model->Mat.pos -
                     xglobals->players.GetPlayer(i)->model->Mat.pos;
        float dist2 = diff.length2();

        if (dist2 < check2) {
            diff.y = 0.0f;
            diff.NormalizeSafe();
            diff *= -1.0f;
            xglobals->players.GetPlayer(i)->Damage(zCombatDamageInfo(
                (xBase*)npcBase, damage, asset->SourceType, zHT_GENERAL, diff,
                xVec3::m_Null, xVec3::m_Null, 0, -1));
            hit = true;
            damageTimers[xglobals->players.GetPlayer(i)->playerIndex] = 0.2f;
        }
    }

    return hit;
}

void zNPCBTDamagePlayerInRangeAction::Begin() {
    memset(damageTimers, 0, sizeof(damageTimers));
}

eTaskState zNPCBTDamagePlayerInRangeAction::Update(float dt) {
    for (int i = 0; i < 4; i++) {
        if (damageTimers[i] > 0.0f) {
            damageTimers[i] -= dt;
        }
    }

    int numChildren = npcBase->GetNumberOfChildren();

    if (numChildren == 0) {
        CheckNPCEntityAgainstPlayers(npcBase->npcEntity);
    } else {
        for (int i = 0; i < numChildren; i++) {
            zNPCEntity* npcEnt = npcBase->GetChild(i);

            CheckNPCEntityAgainstPlayers(npcEnt);
        }
    }

    if (((const Sext::Action_NPC_DamagePlayerInRange*)actionAsset)->options &
        1) {
        return eTaskState_Complete;
    }

    return eTaskState_Running;
}

// ---------------------------------------------------------------------------
// Shooting

eTaskState zNPCBTShootAction::Update(float dt) {
    const Sext::Action_NPC_Shoot* asset =
        (const Sext::Action_NPC_Shoot*)actionAsset;
    World::EntityHandleBase* projHandle =
        World::EntityManager::FindHandle(asset->projectileAsset.internalUid);
    const Sext::ProjectileAsset* projectileAsset =
        (const Sext::ProjectileAsset*)projHandle->BlobData();
    zProjectileLaunchData projectileLaunchData;

    // From the NPC's own transform or from one of its bones, moved by the
    // asset's offset along that transform's axes.
    xMat4x3 launchTransform;

    if (asset->bone < 0) {
        launchTransform = npcBase->npcEntity->model->Mat;
    } else {
        xModelGetBoneMatNoScale(launchTransform, *npcBase->npcEntity->model,
                                asset->bone);
    }

    launchTransform.pos +=
        launchTransform.right * asset->launchPositionOffset.x +
        launchTransform.up * asset->launchPositionOffset.y +
        launchTransform.at * asset->launchPositionOffset.z;

    xVec3 shootDir;

    if (asset->shootToUnion.type == Sext::eShootTo_Direction) {
        switch (asset->shootToUnion.u.direction.space) {
        case 4: {
            xVec3 shootDirLocalSpace;

            __ct__Q24Math6VectorFfff(&shootDirLocalSpace,
                                     asset->shootToUnion.u.direction.vector.x,
                                     asset->shootToUnion.u.direction.vector.y,
                                     asset->shootToUnion.u.direction.vector.z);
            shootDirLocalSpace.safe_normalize(xVec3::m_Null);
            xMat3x3RMulVec(&shootDir, &npcBase->npcEntity->model->Mat,
                           &shootDirLocalSpace);
            break;
        }
        case 0:
            shootDir = npcBase->npcEntity->model->Mat.at;
            if (!asset->shootToUnion.u.flag) {
                shootDir.negate();
            }
            break;
        case 1:
            shootDir = npcBase->npcEntity->model->Mat.up;
            if (!asset->shootToUnion.u.flag) {
                shootDir.negate();
            }
            break;
        case 2:
            shootDir = npcBase->npcEntity->model->Mat.right;
            if (!asset->shootToUnion.u.flag) {
                shootDir.negate();
            }
            break;
        case 3:
            if (asset->shootToUnion.u.flag <
                xModelGetBoneCount(npcBase->npcEntity->model)) {
                xMat4x3 temp;

                xModelGetBoneMatNoScale(temp, *npcBase->npcEntity->model,
                                        asset->bone);
                shootDir = temp.at;
                break;
            }
        default:
            shootDir = npcBase->npcEntity->model->Mat.at;
            break;
        }
    } else if (asset->shootToUnion.type == Sext::eShootTo_Target) {
        xEnt* target = npcBase->npcPerception->targets[0].targetEnt;

        if (target != 0) {
            xVec3 targetLocalSpaceOffset;
            xVec3 targetCenterPos;

            __ct__Q24Math6VectorFfff(&targetLocalSpaceOffset,
                                     asset->shootToUnion.u.direction.vector.x,
                                     asset->shootToUnion.u.direction.vector.y,
                                     asset->shootToUnion.u.direction.vector.z);
            xMat3x3RMulVec(&targetLocalSpaceOffset, &target->model->Mat,
                           &targetLocalSpaceOffset);
            xEntGetCenterFromAABB(target, targetCenterPos);
            shootDir =
                targetCenterPos + targetLocalSpaceOffset - launchTransform.pos;
            shootDir.NormalizeSafe();
        } else {
            shootDir = npcBase->npcEntity->model->Mat.at;
        }
    } else {
        shootDir = npcBase->npcEntity->model->Mat.at;
    }

    float launchSpeed;

    launchSpeed = asset->launchSpeedOption == 1
                      ? asset->unionLaunchSpeed.speed
                      : projectileAsset->launchSpeed;

    xVec3 negatedShootDir;

    negatedShootDir.x = -shootDir.x;
    negatedShootDir.y = -shootDir.y;
    negatedShootDir.z = -shootDir.z;
    xMat3x3LookVec(&launchTransform, &negatedShootDir);

    projectileLaunchData.SetupLaunchToDirection(launchTransform, shootDir,
                                                launchSpeed);
    zProjectileManager::sEmit(projHandle, projectileLaunchData,
                              (xBase*)npcBase->npcEntity);

    return eTaskState_Complete;
}

// ---------------------------------------------------------------------------
// Kill, remove, flags

eTaskState zNPCBTKillAction::Update(float dt) {
    npcBase->Kill(false);

    return eTaskState_Complete;
}

eTaskState zNPCBTRemoveAction::Update(float dt) {
    npcBase->Remove();

    return eTaskState_Complete;
}

eTaskState zNPCBTSetInvulnerableAction::Update(float dt) {
    bool setTo =
        ((const Sext::Action_NPC_SetInvulnerable*)actionAsset)->Invulnerable;

    zNPCCombat* combat = npcBase->npcCombat;
    if (combat != 0) {
        if (setTo) combat->hitsDisabled = true; else combat->hitsDisabled = false;

        return eTaskState_Complete;
    }

    return eTaskState_Fail;
}

eTaskState zNPCBTSetUndamageableAction::Update(float dt) {
    bool setTo =
        ((const Sext::Action_NPC_SetUndamageable*)actionAsset)->Undamageable;

    zNPCCombat* combat = npcBase->npcCombat;
    if (combat != 0) {
        if (setTo) combat->damageDisabled = true; else combat->damageDisabled = false;

        return eTaskState_Complete;
    }

    return eTaskState_Fail;
}

void zNPCBTStrikeAction::Begin() {
    actionAnim.Init("STRIKE", 0.2f, 0.0f);
    actionAnim.StartOnNPC(npcBase->npcEntity, false);
    npcBase->npcSteering->_s10(&stopControl);
}

// ---------------------------------------------------------------------------
// The ninja manager's attack slots

// The player is the asset's variable's, or the current one.
eTaskState zNPCBTRequestAttackAction::Update(float dt) {
    float attackTime =
        ((const Sext::Action_NPC_RequestAttack*)actionAsset)->AttackTime;

    zPlayer* player = 0;

    if (((const Sext::Action_NPC_RequestAttack*)actionAsset)->PlayerVariable !=
        0) {
        btClient->blackboard.Read(
            ((const Sext::Action_NPC_RequestAttack*)actionAsset)->PlayerVariable,
            player);
    }

    if (player == 0) {
        btClient->blackboard.Read(NPC_VAR_CURRENT_PLAYER, player);
    }

    if (player == 0) {
        return eTaskState_Fail;
    }

    return zNPCNinjaManager::RequestAttack(npcBase, player->playerIndex,
                                           attackTime)
               ? eTaskState_Complete
               : eTaskState_Fail;
}

eTaskState zNPCBTReleaseAttackAction::Update(float dt) {
    zPlayer* player = 0;
    const Sext::Action_NPC_ReleaseAttack* asset =
        (const Sext::Action_NPC_ReleaseAttack*)actionAsset;

    if (asset->PlayerVariable != 0) {
        btClient->blackboard.Read(asset->PlayerVariable, player);
    }

    if (player == 0) {
        btClient->blackboard.Read(NPC_VAR_CURRENT_PLAYER, player);
    }

    if (player == 0) {
        return eTaskState_Fail;
    }

    zNPCNinjaManager::ReleaseAttack(npcBase, player->playerIndex, asset->Delay);

    return eTaskState_Complete;
}

// ---------------------------------------------------------------------------
// Hit points and the hit profile

eTaskState zNPCBTSetCurrentHitPointsAction::Update(float dt) {
    const Sext::Action_NPC_Set_CombatCurHitPoints* asset =
        (const Sext::Action_NPC_Set_CombatCurHitPoints*)actionAsset;

    if (asset->newCurHitPoints == 0) {
        npcBase->npcCombat->SetCurHitPoints(
            npcBase->npcCombat->baseCombat.maximumHitPoints);
    } else if (asset->newCurHitPoints == 1) {
        npcBase->npcCombat->SetCurHitPoints(asset->UnionCurHitPoints.value);
    } else if (asset->newCurHitPoints == 2) {
        int hitpointValue = 0;

        btClient->blackboard.Read(asset->UnionCurHitPoints.variable,
                                  hitpointValue);
        npcBase->npcCombat->SetCurHitPoints(hitpointValue);
    }

    return eTaskState_Complete;
}

// The asset counts hit profiles from 1.
eTaskState zNPCBTSetHitProfileAction::Update(float dt) {
    const Sext::Action_NPC_SetHitProfile* asset =
        (const Sext::Action_NPC_SetHitProfile*)actionAsset;

    zNPCCombat* combat = npcBase->npcCombat;

    if (combat == 0) {
        return eTaskState_Fail;
    }

    combat->baseCombat.hitProfile = asset->HitProfile - 1;

    return eTaskState_Complete;
}
