#include "SB/GM/Engine/Game/zNPCCommonBTConditions.pool.h"

// zNPCCommonBTConditions.cpp -- the behaviour-tree conditions an NPC tests:
// blackboard flags (combat cleanup, hits, traps), who the current or locked
// player is and where they stand, hit and hit-point counts, whether the NPC
// is active, alive, moving, on the ground or off every human's screen, and
// whether a generic spawner can spawn. Read from the image with
// tools/brief.py; the engine layouts are the DWARF's (tools/dwarf_types.py).
//
// A condition keeps its asset and client in front of the vtable pointer at
// +8, and the NPC and its own asset pointer at +0xC and +0x10. Evaluate and
// Setup are virtual in retail; they are declared non-virtual here, so no
// vtable lands in this unit.
//
// Of the Sext assets only Condition_NPC_HitCount and
// Condition_NPC_HitPointCount are in the DWARF; the other assets' field
// names are ours, their offsets the image's.

namespace Sext {

class ConditionBase {};

// The player a condition reads, from the named blackboard variable or the
// current player's when the asset names none.
class Condition_NPC_PlayerVariable : public ConditionBase {
public:
    unsigned int PlayerVariable;
};

class Condition_NPC_PlayerIsPlayer : public ConditionBase {
public:
    int Player;
    unsigned int PlayerVariable;
};

class Condition_NPC_CurrPlayerIsPlayer : public ConditionBase {
public:
    unsigned char Player;
};

class Condition_NPC_HitCount : public ConditionBase {
public:
    int numHits;
    unsigned int compOperator;
};

// The count is a constant or a blackboard variable, and `hitCount` says
// which: 0 for the constant.
class Condition_NPC_HitPointCount : public ConditionBase {
public:
    struct ValueStruct {
        int hitCount;
    };
    struct VariableStruct {
        unsigned int VariableName;
    };

    union {
        ValueStruct Value;
        VariableStruct Variable;
    };
    unsigned int hitCount;
    unsigned int compOperator;
};

class Condition_NPC_SpecialAbilityOn : public ConditionBase {
public:
    unsigned int type;
};

class Condition_NPC_SwarmKilledByPlayer : public ConditionBase {
public:
    int player;
};

class Condition_NPC_Moving : public ConditionBase {
public:
    float tolerance;
};

}  // namespace Sext

enum ePlayerName {
    PLAYER_CARL = 0,
    PLAYER_RUSSELL = 1,
    PLAYER_DUG = 2,
    PLAYER_KEVIN = 3,
    PLAYER_HOUSE = 4,
    PLAYER_BIPLANE = 5,
    PLAYER_SPONGEBOB = 6,
    PLAYER_PATRICK = 7,
    PLAYER_PLANKTON = 8,
    PLAYER_MASTERMIND = 9,
    PLAYER_ALIEN = 10,
    PLAYER_BOARD_SPONGEBOB = 11,
    PLAYER_SHOOTING = 12,
    PLAYER_UNKNOWN = 13
};

enum eBoolOperators {
    BoolOperator_EqualTo = 0,
    BoolOperator_Greater = 1,
    BoolOperator_Less = 2,
    BoolOperator_GreaterEq = 3,
    BoolOperator_LessEq = 4,
    BoolOperator_NotEq = 5
};

// How the NPC's last quick-time attack ended. A counter the player landed is
// an attack that failed.
enum eNPCCombatAttackStatus {
    eNPCCombatAttackStatus_None = 0,
    eNPCCombatAttackStatus_Success = 1,
    eNPCCombatAttackStatus_Fail = 2
};

// ---------------------------------------------------------------------------
// The engine's objects as the conditions reach them

class xVec3 {
public:
    float length2() const;
    float NormalizeSafe();
    float dotXZ(const xVec3& v) const;

    float x;
    float y;
    float z;
};

xVec3 operator-(const xVec3& a, const xVec3& b);

class xVec2 {
public:
    float x;
    float y;
};

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
    xVec3 pos;
    unsigned int pad3;
};

class xOGModel {
public:
    xMat4x3 Mat;
};

bool zViewportWorldToScreen(int viewport, const xVec3& pos, xVec2& screen,
                            float& iw, bool clip);

// zPlayer is polymorphic from +0; slot 105 is IsAI (tools/vtslot.py
// __vt__7zPlayer 428).
#define V10(a) virtual void _v##a##0(); virtual void _v##a##1(); \
    virtual void _v##a##2(); virtual void _v##a##3(); \
    virtual void _v##a##4(); virtual void _v##a##5(); \
    virtual void _v##a##6(); virtual void _v##a##7(); \
    virtual void _v##a##8(); virtual void _v##a##9();

class zPlayerVirtuals {
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
    V10(1) V10(2) V10(3) V10(4) V10(5) V10(6) V10(7) V10(8) V10(9)
    virtual void _v100();
    virtual void _v101();
    virtual void _v102();
    virtual void _v103();
    virtual void _v104();
    virtual bool IsAI() const;
};

#undef V10

class zPlayer : public zPlayerVirtuals {
public:
    // Our name for it. The compare must read eName through an inline, or
    // mwcc subtracts the other way round.
    ePlayerName GetName() const { return eName; }

    unsigned char _pad0[0x34 - 0x4];
    xOGModel* model;
    unsigned char _pad1[0xFC - 0x38];
    int viewportIndex;
    unsigned char _pad2[0x2EC - 0x100];
    ePlayerName eName;
};

class zPlayerContainer {
public:
    zPlayer* playerArray[4];
    int numPlayers;
};

class xGlobals {
public:
    unsigned char _pad0[0x428];
    zPlayerContainer players;
};

extern xGlobals* xglobals;

class zWallNet {
public:
    bool IsInsideWallNetXZ(const xVec3& pos) const;
};

class xEntFrame {
public:
    unsigned char _pad0[0x88];
    xVec3 vel;
};

class zNPCEntity {
public:
    unsigned char _pad0[0x34];
    xOGModel* model;
    unsigned char _pad1[0x58 - 0x38];
    xEntFrame* frame;
    unsigned char _pad2[0x1AF - 0x5C];
    bool floorCollision;
};

class zNPCSteering {
public:
    unsigned char _pad0[0x34];
    zWallNet* wallNet;
};

class zNPCBase;
class zNPCStatus;

// An NPC component: the owner, then the vtable pointer at +4. Slot 0 is
// Attached and slot 11 IsDead (tools/vtslot.py __vt__10zNPCCombat 8 52).
class zNPCComponent {
public:
    zNPCBase* owner;

    virtual void Attached(const zNPCStatus* status);
    virtual void _c1();
    virtual void _c2();
    virtual void _c3();
    virtual void _c4();
    virtual void _c5();
    virtual void _c6();
    virtual void _c7();
    virtual void _c8();
    virtual void _c9();
    virtual void _c10();
    virtual bool IsDead() const;
};

class zCombat {
public:
    unsigned char _pad0[0x34];
    float currentHitPoints;
};

class zNPCCombat : public zNPCComponent {
public:
    int attackID;
    unsigned char _pad0[0x10 - 0xC];
    zCombat baseCombat;
};

class zNPCQuickTimeCombat : public zNPCComponent {
public:
    unsigned char _pad0[0xB4 - 0x8];
    eNPCCombatAttackStatus attackStatus;
};

class zNPCBase {
public:
    unsigned char _pad0[0x70];
    bool puppetMode : 1;
    bool alive : 1;
    bool present : 1;
    bool activated : 1;
    bool spawned : 1;
    bool paused : 1;
    bool updateInCinematicAlways : 1;
    bool updateInCinematicNever : 1;
    unsigned char _pad1[0x98 - 0x71];
    zNPCEntity* npcEntity;
    void* npcSteeringOld;
    zNPCSteering* npcSteering;
    void* npcPerception;
    zNPCCombat* npcCombat;
    zNPCQuickTimeCombat* npcQuickTimeCombat;
};

class zNPCGeneric;

class zNPCGenericPool {
public:
    zNPCGeneric* PeekNextInactiveNPC(unsigned int* index) const;
};

class zNPCGenericSpawner : public zNPCBase {
public:
    unsigned char _pad2[0x1D0 - 0xB0];
    zNPCGenericPool* genericPool;
    unsigned char _pad3[0x1E0 - 0x1D4];
    bool doNonstopSpawn;
    bool spawningEnabled;
};

// ---------------------------------------------------------------------------
// The blackboard, as zBlackboard.cpp spells it; every Read is
// zBlackboard.cpp's.

class zBlackboard {
public:
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

// File statics of the unity build (symbols.txt: scope:local), read as memory
// nothing here writes.
extern const unsigned int NPC_VAR_LOCKED_PLAYER;
extern const unsigned int NPC_VAR_CURRENT_PLAYER;
extern const unsigned int NPC_VAR_HIT_EVENT_RECEIVED;
extern const unsigned int NPC_VAR_TRAP_ENTER_EVENT_RECEIVED;
extern const unsigned int NPC_VAR_TRAP_EXIT_EVENT_RECEIVED;
extern const unsigned int NPC_VAR_NEED_COMBAT_CLEANUP;
extern const unsigned int NPC_VAR_NEED_COMBAT_TARGETING_CLEANUP;
extern const unsigned int NPC_VAR_HIT_COUNT;

// ---------------------------------------------------------------------------
// The conditions

class zBTCondition {
public:
    const Sext::ConditionBase* conditionAsset;
    zBTClient* btClient;

    virtual void _cv0();
};

class zNPCBTCondition : public zBTCondition {
public:
    zNPCBase* npcBase;
    const Sext::ConditionBase* asset;
};

class zNPCBTNeedCombatCleanupCondition : public zNPCBTCondition {
public:
    bool Evaluate() const;
};

class zNPCBTNeedCombatTargetingCleanupCondition : public zNPCBTCondition {
public:
    bool Evaluate() const;
};

class zNPCBTCounterSuccessCondition : public zNPCBTCondition {
public:
    bool Evaluate() const;
};

class zNPCBTCounterFailCondition : public zNPCBTCondition {
public:
    bool Evaluate() const;
};

class zNPCBTCounterNoneCondition : public zNPCBTCondition {
public:
    bool Evaluate() const;
};

class zNPCBTLockedPlayerIsHumanCondition : public zNPCBTCondition {
public:
    bool Evaluate() const;
};

class zNPCBTCurrPlayerIsHumanCondition : public zNPCBTCondition {
public:
    bool Evaluate() const;
};

class zNPCBTCurrPlayerIsPlayerCondition : public zNPCBTCondition {
public:
    void Setup(const Sext::ConditionBase* a);
    bool Evaluate() const;

    ePlayerName playerName;
};

class zNPCBTPlayerIsHumanCondition : public zNPCBTCondition {
public:
    bool Evaluate() const;
};

class zNPCBTPlayerIsTargetableCondition : public zNPCBTCondition {
public:
    bool Evaluate() const;
};

class zNPCBTPlayerIsPlayerCondition : public zNPCBTCondition {
public:
    bool Evaluate() const;

    ePlayerName playerName;
};

class zNPCBTDefeatedCondition : public zNPCBTCondition {
public:
    bool Evaluate() const;
};

class zNPCBTActiveCondition : public zNPCBTCondition {
public:
    bool Evaluate() const;
};

class zNPCBTHitCondition : public zNPCBTCondition {
public:
    bool Evaluate() const;
};

class zNPCBTHitCountCondition : public zNPCBTCondition {
public:
    void Setup(const Sext::ConditionBase* a);
    bool Evaluate() const;

    int numHits;
    unsigned int op;
};

class zNPCBTHitPointCountCondition : public zNPCBTCondition {
public:
    bool Evaluate() const;
};

class zNPCBTEnteredTrapCondition : public zNPCBTCondition {
public:
    bool Evaluate() const;
};

class zNPCBTExitedTrapCondition : public zNPCBTCondition {
public:
    bool Evaluate() const;
};

class zNPCBTSpecialAbilityOnCondition : public zNPCBTCondition {
public:
    void Setup(const Sext::ConditionBase* a);

    unsigned int type;
};

class zNPCBTSwarmKilledByPlayerCondition : public zNPCBTCondition {
public:
    void Setup(const Sext::ConditionBase* a);

    int checkPlayer;
};

class zNPCBTAliveCondition : public zNPCBTCondition {
public:
    bool Evaluate() const;
};

// Where the NPC is from the locked player: along the player's at axis (0),
// against it (1), along the right axis (2), against it (3), above (4) or
// below (5).
class zNPCBTIsToLockedPlayerSideCondition : public zNPCBTCondition {
public:
    bool Evaluate() const;

    unsigned int side;
};

class zNPCBTCheckPlayerTypeCondition : public zNPCBTCondition {
public:
    bool Evaluate() const;

    unsigned int playerType;
    unsigned int varID;
};

class zNPCBTIsPlayerValidCondition : public zNPCBTCondition {
public:
    bool Evaluate() const;
};

class zNPCBTIsOnGroundCondition : public zNPCBTCondition {
public:
    bool Evaluate() const;
};

class zNPCBTMovingCondition : public zNPCBTCondition {
public:
    bool Evaluate() const;
};

class zNPCBTOffScreenCondition : public zNPCBTCondition {
public:
    bool Evaluate() const;
};

class zNPCBT_GenericSpawner_NonstopSpawn_Condition : public zNPCBTCondition {
public:
    bool Evaluate() const;
};

class zNPCBT_GenericSpawner_CanSpawn_Condition : public zNPCBTCondition {
public:
    bool Evaluate() const;
};

class zNPCBT_GenericSpawner_CanSpawnLater_Condition : public zNPCBTCondition {
public:
    bool Evaluate() const;
};

// ---------------------------------------------------------------------------
// Blackboard flags

// NEAR MISS: 5 of 17 words, only for being the first function in the file.
// The first function mwcc builds loads the variable's high half before
// btClient and keeps btClient in r3; retail's unity build had functions
// ahead of it. Any function with a load placed in front (moved there, or an
// unreferenced static) makes it byte-identical and still matches itself; an
// empty one does not. No different: the read over two lines, the extern
// declared first, the other flag's variable, Read<int> declared as an
// explicit specialization.
bool zNPCBTNeedCombatCleanupCondition::Evaluate() const {
    int needCombatCleanup;

    btClient->blackboard.Read(NPC_VAR_NEED_COMBAT_CLEANUP, needCombatCleanup);

    return needCombatCleanup == 1;
}

bool zNPCBTNeedCombatTargetingCleanupCondition::Evaluate() const {
    int needCombatCleanup;

    btClient->blackboard.Read(NPC_VAR_NEED_COMBAT_TARGETING_CLEANUP,
                              needCombatCleanup);

    return needCombatCleanup == 1;
}

// ---------------------------------------------------------------------------
// The last quick-time attack

bool zNPCBTCounterSuccessCondition::Evaluate() const {
    zNPCQuickTimeCombat* qtc = npcBase->npcQuickTimeCombat;

    if (qtc != 0) {
        return qtc->attackStatus == eNPCCombatAttackStatus_Fail;
    }

    return false;
}

bool zNPCBTCounterFailCondition::Evaluate() const {
    zNPCQuickTimeCombat* qtc = npcBase->npcQuickTimeCombat;

    if (qtc != 0) {
        return qtc->attackStatus == eNPCCombatAttackStatus_Success;
    }

    return false;
}

bool zNPCBTCounterNoneCondition::Evaluate() const {
    zNPCQuickTimeCombat* qtc = npcBase->npcQuickTimeCombat;

    if (qtc != 0) {
        return qtc->attackStatus == eNPCCombatAttackStatus_None;
    }

    return false;
}

// ---------------------------------------------------------------------------
// Players

bool zNPCBTLockedPlayerIsHumanCondition::Evaluate() const {
    zPlayer* p;

    btClient->blackboard.Read(NPC_VAR_LOCKED_PLAYER, p);

    return p != 0 ? !p->IsAI() : false;
}

bool zNPCBTCurrPlayerIsHumanCondition::Evaluate() const {
    zPlayer* p;

    btClient->blackboard.Read(NPC_VAR_CURRENT_PLAYER, p);

    return p != 0 ? !p->IsAI() : false;
}

// The asset's player, in the order the asset lists them, as a player name.
void zNPCBTCurrPlayerIsPlayerCondition::Setup(const Sext::ConditionBase* a) {
    asset = a;

    switch (((const Sext::Condition_NPC_CurrPlayerIsPlayer*)a)->Player) {
    case 1:
        playerName = PLAYER_CARL;
        break;
    case 0:
        playerName = PLAYER_RUSSELL;
        break;
    case 3:
        playerName = PLAYER_DUG;
        break;
    case 2:
        playerName = PLAYER_KEVIN;
        break;
    default:
        playerName = PLAYER_UNKNOWN;
        break;
    }
}

bool zNPCBTCurrPlayerIsPlayerCondition::Evaluate() const {
    zPlayer* p;

    btClient->blackboard.Read(NPC_VAR_CURRENT_PLAYER, p);

    if (p == 0) {
        return false;
    }

    return p->GetName() == playerName;
}

bool zNPCBTPlayerIsHumanCondition::Evaluate() const {
    zPlayer* player = 0;

    btClient->blackboard.Read(
        ((const Sext::Condition_NPC_PlayerVariable*)conditionAsset)
                    ->PlayerVariable != 0
            ? ((const Sext::Condition_NPC_PlayerVariable*)conditionAsset)
                  ->PlayerVariable
            : NPC_VAR_CURRENT_PLAYER,
        player);

    if (player == 0) {
        return false;
    }

    return !player->IsAI();
}

// Everything after the read compiled to nothing in retail.
bool zNPCBTPlayerIsTargetableCondition::Evaluate() const {
    zPlayer* player = 0;

    btClient->blackboard.Read(
        ((const Sext::Condition_NPC_PlayerVariable*)conditionAsset)
                    ->PlayerVariable != 0
            ? ((const Sext::Condition_NPC_PlayerVariable*)conditionAsset)
                  ->PlayerVariable
            : NPC_VAR_CURRENT_PLAYER,
        player);

    return false;
}

bool zNPCBTPlayerIsPlayerCondition::Evaluate() const {
    zPlayer* p;

    btClient->blackboard.Read(
        ((const Sext::Condition_NPC_PlayerIsPlayer*)conditionAsset)
            ->PlayerVariable,
        p);

    if (p == 0) {
        return false;
    }

    return p->GetName() == playerName;
}

// ---------------------------------------------------------------------------
// Hits and hit points

bool zNPCBTDefeatedCondition::Evaluate() const {
    int hitCount;

    btClient->blackboard.Read(NPC_VAR_HIT_COUNT, hitCount);

    return hitCount >= 3;
}

bool zNPCBTActiveCondition::Evaluate() const { return npcBase->activated; }

bool zNPCBTHitCondition::Evaluate() const {
    int hitEventReceived;

    btClient->blackboard.Read(NPC_VAR_HIT_EVENT_RECEIVED, hitEventReceived);

    return hitEventReceived == 1;
}

void zNPCBTHitCountCondition::Setup(const Sext::ConditionBase* a) {
    const Sext::Condition_NPC_HitCount* myAsset =
        (const Sext::Condition_NPC_HitCount*)a;

    numHits = myAsset->numHits;
    op = myAsset->compOperator;
}

bool zNPCBTHitCountCondition::Evaluate() const {
    int hitCount;

    btClient->blackboard.Read(NPC_VAR_HIT_COUNT, hitCount);

    bool ret = false;

    switch (op) {
    case BoolOperator_EqualTo:
        ret = hitCount == numHits;
        break;
    case BoolOperator_Greater:
        ret = hitCount > numHits;
        break;
    case BoolOperator_Less:
        ret = hitCount < numHits;
        break;
    case BoolOperator_GreaterEq:
        ret = hitCount >= numHits;
        break;
    case BoolOperator_LessEq:
        ret = hitCount <= numHits;
        break;
    case BoolOperator_NotEq:
        ret = hitCount != numHits;
        break;
    }

    return ret;
}

bool zNPCBTHitPointCountCondition::Evaluate() const {
    const Sext::Condition_NPC_HitPointCount* myAsset =
        (const Sext::Condition_NPC_HitPointCount*)asset;
    // Declared ahead of assetHitNumber, not where it is first assigned:
    // mwcc hands f0 to the float declared first, and retail has hitCount
    // there.
    float hitCount;
    float assetHitNumber;

    if (myAsset->hitCount == 0) {
        assetHitNumber = myAsset->Value.hitCount;
    } else {
        int blackboardVal = 0;

        btClient->blackboard.Read(myAsset->Variable.VariableName,
                                  blackboardVal);
        assetHitNumber = blackboardVal;
    }

    eBoolOperators op = (eBoolOperators)myAsset->compOperator;
    hitCount = npcBase->npcCombat->baseCombat.currentHitPoints;
    bool ret = false;

    switch (op) {
    case BoolOperator_EqualTo:
        ret = hitCount == assetHitNumber;
        break;
    case BoolOperator_Greater:
        ret = hitCount > assetHitNumber;
        break;
    case BoolOperator_Less:
        ret = hitCount < assetHitNumber;
        break;
    case BoolOperator_GreaterEq:
        ret = hitCount >= assetHitNumber;
        break;
    case BoolOperator_LessEq:
        ret = hitCount <= assetHitNumber;
        break;
    case BoolOperator_NotEq:
        ret = hitCount != assetHitNumber;
        break;
    }

    return ret;
}

// ---------------------------------------------------------------------------
// Traps, abilities, swarms

bool zNPCBTEnteredTrapCondition::Evaluate() const {
    int trapEnterEventReceived;

    btClient->blackboard.Read(NPC_VAR_TRAP_ENTER_EVENT_RECEIVED,
                              trapEnterEventReceived);

    return trapEnterEventReceived == 1;
}

bool zNPCBTExitedTrapCondition::Evaluate() const {
    int trapExitEventReceived;

    btClient->blackboard.Read(NPC_VAR_TRAP_EXIT_EVENT_RECEIVED,
                              trapExitEventReceived);

    return trapExitEventReceived == 1;
}

void zNPCBTSpecialAbilityOnCondition::Setup(const Sext::ConditionBase* a) {
    type = ((const Sext::Condition_NPC_SpecialAbilityOn*)a)->type;
}

void zNPCBTSwarmKilledByPlayerCondition::Setup(const Sext::ConditionBase* a) {
    asset = a;
    checkPlayer = ((const Sext::Condition_NPC_SwarmKilledByPlayer*)a)->player;
}

// Alive unless the combat component says dead; alive without one.
bool zNPCBTAliveCondition::Evaluate() const {
    zNPCCombat* combat = npcBase->npcCombat;

    return combat != 0 ? !combat->IsDead() : true;
}

// Within about 45 degrees of the side asked for, or clearly above or below.
//
// NEAR MISS: 107 of 113 words, the four-literal wall (NOTES.md). The four
// distinct literals share an addis base where retail spells a lis per load;
// that base, and the second saved register it costs, is the whole
// difference. Pool padding of 65 to 262 KB forms the same base.
bool zNPCBTIsToLockedPlayerSideCondition::Evaluate() const {
    zPlayer* player;

    btClient->blackboard.Read(NPC_VAR_LOCKED_PLAYER, player);

    if (player == 0) {
        return false;
    }

    xVec3 toPlayer =
        player->model->Mat.pos - npcBase->npcEntity->model->Mat.pos;
    toPlayer.NormalizeSafe();

    switch (side) {
    case 0: {
        float dot = toPlayer.dotXZ(player->model->Mat.at);
        return dot <= -0.7f;
    }
    case 1: {
        float dot = toPlayer.dotXZ(player->model->Mat.at);
        return dot >= 0.7f;
    }
    case 2: {
        float dot = toPlayer.dotXZ(player->model->Mat.right);
        return dot <= -0.7f;
    }
    case 3: {
        float dot = toPlayer.dotXZ(player->model->Mat.right);
        return dot >= 0.7f;
    }
    case 4:
        return toPlayer.y <= -0.8f;
    case 5:
        return toPlayer.y >= 0.8f;
    }

    return false;
}

// Everything after the read compiled to nothing in retail.
bool zNPCBTCheckPlayerTypeCondition::Evaluate() const {
    zPlayer* player;

    btClient->blackboard.Read(varID, player);

    return false;
}

bool zNPCBTIsPlayerValidCondition::Evaluate() const {
    zPlayer* player = 0;

    btClient->blackboard.Read(
        ((const Sext::Condition_NPC_PlayerVariable*)conditionAsset)
                    ->PlayerVariable != 0
            ? ((const Sext::Condition_NPC_PlayerVariable*)conditionAsset)
                  ->PlayerVariable
            : NPC_VAR_CURRENT_PLAYER,
        player);

    if (player == 0) {
        return false;
    }

    return npcBase->npcSteering->wallNet->IsInsideWallNetXZ(
        player->model->Mat.pos);
}

// ---------------------------------------------------------------------------
// The NPC itself

bool zNPCBTIsOnGroundCondition::Evaluate() const {
    return npcBase->npcEntity->floorCollision;
}

bool zNPCBTMovingCondition::Evaluate() const {
    float tolerance =
        ((const Sext::Condition_NPC_Moving*)conditionAsset)->tolerance;

    return npcBase->npcEntity->frame->vel.length2() > tolerance * tolerance;
}

// Off screen in the viewport of the last human player.
bool zNPCBTOffScreenCondition::Evaluate() const {
    int viewport = 0;

    for (int i = 0; i < xglobals->players.numPlayers; i++) {
        if (!xglobals->players.playerArray[i]->IsAI()) viewport = xglobals->players.playerArray[i]->viewportIndex;
    }

    xVec2 screenPos;
    float iw;

    return !zViewportWorldToScreen(viewport, npcBase->npcEntity->model->Mat.pos,
                                   screenPos, iw, true);
}

// ---------------------------------------------------------------------------
// Generic spawners

bool zNPCBT_GenericSpawner_NonstopSpawn_Condition::Evaluate() const {
    return ((zNPCGenericSpawner*)npcBase)->doNonstopSpawn;
}

bool zNPCBT_GenericSpawner_CanSpawn_Condition::Evaluate() const {
    zNPCGenericSpawner* spawner = (zNPCGenericSpawner*)npcBase;

    if (spawner->spawningEnabled == false) {
        return false;
    }

    return spawner->genericPool->PeekNextInactiveNPC(0) != 0;
}

bool zNPCBT_GenericSpawner_CanSpawnLater_Condition::Evaluate() const {
    return ((zNPCGenericSpawner*)npcBase)->spawningEnabled;
}
