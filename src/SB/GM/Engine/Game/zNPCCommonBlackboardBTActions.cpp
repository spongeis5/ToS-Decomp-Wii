#include "SB/GM/Engine/Game/zNPCCommonBlackboardBTActions.pool.h"

// zNPCCommonBlackboardBTActions.cpp -- the behaviour-tree actions that
// compute something about an NPC's world and write it to a blackboard
// variable: hit points, a position inside the wallnet, the closest or best
// player, a patrol or network move point, a wander or player position.
// Read from the image with tools/disasm.py; layouts are the DWARF's.
//
// Most of them share one shape: a virtual at slot 10 of the action finds the
// value, and the action reports 3 when the write succeeded and 4 when there
// was nothing to write. The blackboard's Read and Write are zBlackboard.cpp's
// templates; the four instances retail keeps in this unit are specialized
// here, and every other one is only declared.

namespace Sext {

class ActionBase {};

struct vec3 {
    float x;
    float y;
    float z;
};

class uid {
public:
    unsigned long long internalUid;
};

// How a flying NPC's height is chosen: none, a fixed offset, an offset from
// the wallnet under it, or its spawn height.
class FlyingNPCHeightAdjustment {
public:
    float offset;
    unsigned int HeightAdjustmentType;
};

class Action_NPC_WriteVariable : public ActionBase {
public:
    unsigned int VariableName;
};

class Action_NPC_Write_InsideWallnet : public ActionBase {
public:
    unsigned int VariableName;
    float DistanceFromEdge;
};

class Action_NPC_FlyingWrite_InsideWallnet
    : public Action_NPC_Write_InsideWallnet {
public:
    FlyingNPCHeightAdjustment HeightAdjustment;
};

class Action_NPC_FlyingWrite_WanderPosition : public ActionBase {
public:
    float Radius;
    FlyingNPCHeightAdjustment HeightAdjustment;
};

class Action_NPC_Write_TargetPlayer : public ActionBase {
public:
    float UpdatePeriod;
    bool CheckInWallNet;
    bool CheckSwitch;
    bool NotifyPlayer;
    float DistanceWeight;
    float DistanceRange;
    bool DistanceExclude;
    float AttackingWeight;
    int AttackingEnemy;
    float TargetingWeight;
    int TargetingEnemy;
    float HumanAIWeight;
    bool HumanAIExclude;
};

class Action_NPC_Write_PlayerPosition : public ActionBase {
public:
    int OffsetType;
    vec3 Offset;
    bool IgnoreHeight;
    unsigned int PlayerVariable;
};

class Action_NPC_FlyingWrite_PlayerPosition
    : public Action_NPC_Write_PlayerPosition {
public:
    FlyingNPCHeightAdjustment HeightAdjustment;
};

class Action_NPC_SetFlag : public ActionBase {
public:
    bool Value;
};

class Action_NPC_Write_NetworkMovePoint : public ActionBase {
public:
    int Type;
};

class Action_NPC_Write_CurrentPosition : public ActionBase {
public:
    bool ConstrainToWallnet;
    vec3 Offset;
};

class Action_NPC_FlyingWrite_CurrentPosition
    : public Action_NPC_Write_CurrentPosition {
public:
    FlyingNPCHeightAdjustment HeightAdjustment;
};

class EventAny;

}  // namespace Sext

class xBase;
class zCommonPlayer;

enum ForceEvent { ForceEvent_ = 0x7FFFFFFF };

void zEntEvent(xBase* from, unsigned int fromEvent, xBase* to,
               unsigned int toEvent, Sext::EventAny* args, ForceEvent force);

class xVec3 {
public:
    xVec3& operator+=(const xVec3& other);

    static const xVec3 m_Null;

    float x;
    float y;
    float z;
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

void xMat3x3RMulVec(xVec3* o, const xMat3x3* m, const xVec3* v);
void v3add(xVec3* o, xVec3* a, xVec3* b);
float xVec3Dist2(const xVec3* a, const xVec3* b);

namespace Math {

// Twelve bytes, as zNPCCommonBTActions.cpp explains: retail's locals built
// by this constructor are twelve bytes apart from what follows them.
class Vector {
public:
    Vector(float x, float y, float z);

    float x;
    float y;
    float z;
};

}  // namespace Math

namespace World {

class xOGModel {
public:
    xMat4x3 Mat;
};

}  // namespace World

class xBase {
public:
    unsigned char _pad0[0x20];
    unsigned int baseType;
    unsigned char _pad1[0x34 - 0x24];
    World::xOGModel* model;
};

xBase* zSceneFindObject(unsigned long long id);

class xMovePoint : public xBase {
public:
    int NetworkGetNumberOfMPs();
    xMovePoint* NetworkGetClosestXZ(const xVec3& pos);
    xMovePoint* NetworkGetFarthestXZ(const xVec3& pos);
    xMovePoint* NetworkGetRandom(bool unused);
    void GetRandomChildren(xMovePoint** out, const xMovePoint* exclude,
                           xVec3* pos) const;

    unsigned char _pad2[0x40 - 0x38];
    xVec3* pos;
};

// Slot 105 of the player's vtable is IsAI (tools/vtable.py on __vt__7zPlayer).
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
    unsigned char _pad0[0x34 - 0x4];
    World::xOGModel* model;
    unsigned char _pad1[0x2EC - 0x38];
    int eName;
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

class zWallNetTriangle {
public:
    unsigned char _pad0[0x8];
};

class zWallNetAsset {
public:
    unsigned char _pad0[0x40];
    zWallNetTriangle* triangles;
};

class zWallNet {
public:
    bool IsInsideWallNetXZ(const xVec3& pos) const;
    int FindTriangleIDXZ(const xVec3& pos) const;
    void FindYOnTriangleFromXZ(const zWallNetTriangle* tri, float x, float z,
                               float& y) const;

    unsigned char _pad0[0x3C];
    zWallNetAsset* wallNetAsset;
};

class zWallNetPositionXZ {
public:
    zWallNetPositionXZ();
    ~zWallNetPositionXZ();

    void SetupPosition(const xVec3& pos, const zWallNet* wallNet);
    bool IsRInsideXZ(float r) const;
    bool MakeValidByMovingInside(float r);

    xVec3 curPos;
    zWallNet* curWallNet;
    int curTriangleId;
};

class zNPCBound {
public:
    float GetBoundRadiusXZ() const;

    xVec3 center;
    xVec3 extent;
};

class zNPCEntity {
public:
    unsigned char _pad0[0x34];
    World::xOGModel* model;
    unsigned char _pad1[0xF4 - 0x38];
    zNPCBound npcBound;
};

class zNPCSteering {
public:
    unsigned char _pad0[0x34];
    zWallNet* wallNet;
};

class zNPCPerception {
public:
    unsigned char _pad0[0x10];
    xBase* target;
};

class zNPCCombat {
public:
    unsigned char _pad0[0x44];
    float curHitPoints;
    float maxHitPoints;
};

class NPCAsset {
public:
    unsigned char _pad0[0x40];
    float spawnHeight;
};

class zNPCBase {
public:
    void GetPosition(xVec3& pos);
    zPlayer* GetClosestPlayerOnWallnet(bool ignoreHeight);

    unsigned char _pad0[0x60];
    NPCAsset* npcAsset;
    unsigned char _pad1[0x80 - 0x64];
    xMovePoint* npcMovePoint;
    unsigned char _pad2[0x98 - 0x84];
    zNPCEntity* npcEntity;
    void* npcSteeringOld;
    zNPCSteering* npcSteering;
    zNPCPerception* npcPerception;
    zNPCCombat* npcCombat;
};

namespace BT_Utility {

float CalcFlyingNPCsAdjustedY(const Sext::FlyingNPCHeightAdjustment* adj,
                              const zNPCBase* npc, const xVec3& pos);

}  // namespace BT_Utility

// ---------------------------------------------------------------------------
// The blackboard, as zBlackboard.cpp spells it.

enum eVarType {
    eVarType_Invalid = 0,
    eVarType_S32 = 1,
    eVarType_F32 = 2,
    eVarType_xVec3 = 3,
    eVarType_xBasePtr = 4,
    eVarType_zInteractionUpPtr = 5,
    eVarType_zPlayerPtr = 6,
    eVarType_xMovePointPtr = 7,
    eVarType_UID = 8,
    eVarType_zUpFloatingObjectPtr = 9,
    eVarType_zVariableEventData = 10
};

namespace Util {
template <class T>
class DelegateP0 {
public:
    T Invoke() const { return func(object); }

    void* object;
    T (*func)(void*);
};
}  // namespace Util

class zVariableBase {
public:
    unsigned int id;
    eVarType type;
    unsigned int flags;
    Util::DelegateP0<void>* observers[8];

    virtual void OnReset();
};

template <class T>
class zVariable : public zVariableBase {
public:
    T GetValue() const { return value; }

    T defaultValue;
    T value;
};

class zVariableDynamicCast {
public:
    template <class T>
    static void Cast(zVariableBase* v, zVariable<T>*& out);
};

class zBlackboard {
public:
    unsigned int size;
    zVariableBase** variables;

    zVariableBase* Find(unsigned int id) const;

    template <class T>
    bool Write(unsigned int id, const T& value);
    template <class T>
    bool Read(unsigned int id, T& out) const;
};

template <>
bool zBlackboard::Read<xVec3>(unsigned int id, xVec3& out) const;
template <>
bool zBlackboard::Read<xMovePoint*>(unsigned int id, xMovePoint*& out) const;
template <>
bool zBlackboard::Write<xMovePoint*>(unsigned int id,
                                     xMovePoint* const& value);

class zBTClient {
public:
    unsigned char _pad0[0x88];
    zBlackboard blackboard;
};

extern const unsigned int NPC_VAR_LOCKED_PLAYER;
extern const unsigned int NPC_VAR_CURRENT_PLAYER;
extern const unsigned int NPC_VAR_TARGET_MP;
extern const unsigned int NPC_VAR_CURRENT_DESTINATION;
extern const unsigned int NPC_VAR_TRAP_POS;
extern const unsigned int NPC_VAR_NEED_COMBAT_CLEANUP;
extern const unsigned int NPC_VAR_NEED_COMBAT_TARGETING_CLEANUP;

// ---------------------------------------------------------------------------
// The actions

enum eTaskState {
    eTaskState_Unknown = 0,
    eTaskState_Running = 1,
    eTaskState_Suspend = 2,
    eTaskState_Complete = 3,
    eTaskState_Fail = 4,
    eTaskState_Abort = 5
};

class zBTActionData {
public:
    const Sext::ActionBase* actionAsset;
    void* resumeCB;
    zBTClient* btClient;
};

class zBTAction : public zBTActionData {
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
    virtual bool _v10(xVec3& out);
};

class zNPCBTAction : public zBTAction {
public:
    zNPCBase* npcBase;
};

class zNPCBTWriteCurHitPointsAction : public zNPCBTAction {
public:
    eTaskState Update(float dt);
};

class zNPCBTWriteMaxHitPointsAction : public zNPCBTAction {
public:
    eTaskState Update(float dt);
};

class zNPCBTWriteInsideWallnetAction : public zNPCBTAction {
public:
    bool FindInsideWNPos(xVec3& pos);
    eTaskState Update(float dt);
};

class zNPCFlyingBTWriteInsideWallnetAction
    : public zNPCBTWriteInsideWallnetAction {
public:
    eTaskState Update(float dt);
};

class zNPCBTWriteNumberOfMovepointsAction : public zNPCBTAction {
public:
    eTaskState Update(float dt);
};

class zNPCBTWriteLockedPlayerAction : public zNPCBTAction {
public:
    eTaskState Update(float dt);
};

class zNPCBTWriteClosestPlayerAction : public zNPCBTAction {
public:
    eTaskState Update(float dt);
};

class zNPCBTWriteTargetPlayerAction : public zNPCBTAction {
public:
    float FindWeightedDistance(const xVec3& pos);
    float FindWeightedNumAttacking(const zPlayer* player);
    float FindWeightedNumTargeting(const zPlayer* player);
    float FindWeightedPlayerStatus(const zPlayer* player);
    void Begin();
    eTaskState Update(float dt);

    float timer;
};

class zNPCBTWritePatrolMovePointAction : public zNPCBTAction {
public:
    eTaskState Update(float dt);
};

class zNPCBTWriteWanderPositionAction : public zNPCBTAction {
public:
    eTaskState Update(float dt);
};

class zNPCFlyingBTWriteWanderPositionAction
    : public zNPCBTWriteWanderPositionAction {
public:
    eTaskState Update(float dt);
};

class zNPCBTWritePlayerPositionAction : public zNPCBTAction {
public:
    bool FindPlayerPos(xVec3& pos);
};

class zNPCFlyingBTWritePlayerPositionAction
    : public zNPCBTWritePlayerPositionAction {
public:
    eTaskState Update(float dt);
};

class zNPCBTWriteTrapPositionAction : public zNPCBTAction {
public:
    eTaskState Update(float dt);
};

class zNPCBTResetCurrentPlayerAction : public zNPCBTAction {
public:
    eTaskState Update(float dt);
};

class zNPCBTSetNeedCombatCleanupAction : public zNPCBTAction {
public:
    eTaskState Update(float dt);
};

class zNPCBTSetNeedCombatTargetingCleanupAction : public zNPCBTAction {
public:
    eTaskState Update(float dt);
};

class zNPCBTWriteNetworkMovePointAction : public zNPCBTAction {
public:
    eTaskState Update(float dt);
};

class zNPCBTWriteBlackboardUidPosition : public zNPCBTAction {
public:
    eTaskState Update(float dt);
};

class zNPCBTWriteCurrentPosition : public zNPCBTAction {
public:
    bool FindCurrentPos(xVec3& pos);
};

class zNPCFlyingBTWriteCurrentPosition : public zNPCBTWriteCurrentPosition {
public:
    eTaskState Update(float dt);
};

#define xmin(a, b) ((a) < (b) ? (a) : (b))
#define xmax(a, b) ((a) > (b) ? (a) : (b))

// ---------------------------------------------------------------------------
// Hit points

eTaskState zNPCBTWriteCurHitPointsAction::Update(float dt) {
    unsigned int var =
        ((const Sext::Action_NPC_WriteVariable*)actionAsset)->VariableName;
    zBlackboard& blackboard = btClient->blackboard;

    if (npcBase->npcCombat == 0) {
        return eTaskState_Fail;
    }

    float curHitPoints = npcBase->npcCombat->curHitPoints;

    return blackboard.Write(var, curHitPoints) ? eTaskState_Complete
                                               : eTaskState_Fail;
}

eTaskState zNPCBTWriteMaxHitPointsAction::Update(float dt) {
    unsigned int var =
        ((const Sext::Action_NPC_WriteVariable*)actionAsset)->VariableName;
    zBlackboard& blackboard = btClient->blackboard;

    if (npcBase->npcCombat == 0) {
        return eTaskState_Fail;
    }

    float maxHitPoints = npcBase->npcCombat->maxHitPoints;

    return blackboard.Write(var, maxHitPoints) ? eTaskState_Complete
                                               : eTaskState_Fail;
}

// ---------------------------------------------------------------------------
// Inside the wallnet: the position a variable holds, moved inside the NPC's
// wallnet by the NPC's radius and the asset's distance from the edge, or by
// the radius alone when that cannot be done.

bool zNPCBTWriteInsideWallnetAction::FindInsideWNPos(xVec3& pos) {
    const Sext::Action_NPC_Write_InsideWallnet* asset =
        (const Sext::Action_NPC_Write_InsideWallnet*)actionAsset;
    unsigned int var = asset->VariableName;
    float dist = asset->DistanceFromEdge;

    zBlackboard& blackboard = btClient->blackboard;

    zWallNet* wallNet = npcBase->npcSteering->wallNet;

    if (wallNet == 0) {
        return false;
    }

    if (!blackboard.Read(var, pos)) {
        return false;
    }

    zWallNetPositionXZ wallNetPosition;

    wallNetPosition.SetupPosition(pos, wallNet);

    float boundRadiusXZ =
        npcBase->npcEntity != 0
            ? npcBase->npcEntity->npcBound.GetBoundRadiusXZ()
            : 1.0f;

    if (!wallNetPosition.IsRInsideXZ(boundRadiusXZ + dist)) {
        if (!wallNetPosition.MakeValidByMovingInside(boundRadiusXZ + dist) &&
            !wallNetPosition.MakeValidByMovingInside(boundRadiusXZ)) {
            return false;
        }

        pos = wallNetPosition.curPos;
    }

    return true;
}

// Weak in retail and called from the function above, so defined below it.
inline float zNPCBound::GetBoundRadiusXZ() const {
    return extent.x > extent.z ? extent.x : extent.z;
}

template <>
bool zBlackboard::Read<xVec3>(unsigned int id, xVec3& out) const {
    zVariableBase* v = Find(id);

    if (v == 0) {
        return false;
    }

    zVariable<xVec3>* result;

    zVariableDynamicCast::Cast(v, result);

    zVariable<xVec3>* var = result;

    if (var != 0) {
        out = var->GetValue();
        return true;
    }

    return false;
}

eTaskState zNPCBTWriteInsideWallnetAction::Update(float dt) {
    xVec3 pos;
    unsigned int variable =
        ((const Sext::Action_NPC_Write_InsideWallnet*)actionAsset)
            ->VariableName;

    if (!_v10(pos)) {
        return eTaskState_Fail;
    }

    return btClient->blackboard.Write(variable, pos) ? eTaskState_Complete : eTaskState_Fail;
}

eTaskState zNPCFlyingBTWriteInsideWallnetAction::Update(float dt) {
    xVec3 pos;
    const Sext::Action_NPC_FlyingWrite_InsideWallnet* asset =
        (const Sext::Action_NPC_FlyingWrite_InsideWallnet*)actionAsset;
    unsigned int variable = asset->VariableName;

    if (!_v10(pos)) {
        return eTaskState_Fail;
    }

    pos.y = BT_Utility::CalcFlyingNPCsAdjustedY(&asset->HeightAdjustment,
                                                npcBase, pos);

    return btClient->blackboard.Write(variable, pos) ? eTaskState_Complete : eTaskState_Fail;
}

float BT_Utility::CalcFlyingNPCsAdjustedY(
    const Sext::FlyingNPCHeightAdjustment* adj, const zNPCBase* npc,
    const xVec3& pos) {
    switch ((int)adj->HeightAdjustmentType) {
    case 0:
        return pos.y;
    case 1:
        return pos.y + adj->offset;
    case 2: {
        zWallNet* wallNet = 0;

        if (npc->npcSteering != 0) {
            wallNet = npc->npcSteering->wallNet;
        }

        if (wallNet == 0) {
            return pos.y;
        }

        int tri = wallNet->FindTriangleIDXZ(pos);

        if (tri == 255) {
            return pos.y;
        }

        float y;

        wallNet->FindYOnTriangleFromXZ(&wallNet->wallNetAsset->triangles[tri],
                                       pos.x, pos.z, y);

        return y + adj->offset;
    }
    case 3:
        return npc->npcAsset->spawnHeight;
    }

    return pos.y;
}

// ---------------------------------------------------------------------------
// Move points and players

eTaskState zNPCBTWriteNumberOfMovepointsAction::Update(float dt) {
    unsigned int var =
        ((const Sext::Action_NPC_WriteVariable*)actionAsset)->VariableName;
    zBlackboard& blackboard = btClient->blackboard;
    int numberOfMovepoints =
        npcBase->npcMovePoint != 0
            ? npcBase->npcMovePoint->NetworkGetNumberOfMPs()
            : 0;

    if (!blackboard.Write(var, numberOfMovepoints)) {
        return eTaskState_Fail;
    }

    return eTaskState_Complete;
}

eTaskState zNPCBTWriteLockedPlayerAction::Update(float dt) {
    zPlayer* closest = 0;
    float best = 3.4028235e+38f;

    for (int i = 0; i < xglobals->players.numPlayers; i++) {
        if (!xglobals->players.playerArray[i]->IsAI()) {
            float d = xVec3Dist2(
                &npcBase->npcEntity->model->Mat.pos,
                &xglobals->players.playerArray[i]->model->Mat.pos);

            if (d < best) {
                best = d;
                closest = xglobals->players.playerArray[i];
            }
        }
    }

    if (closest == 0) {
        return eTaskState_Fail;
    }

    return btClient->blackboard.Write(NPC_VAR_LOCKED_PLAYER, closest) ? eTaskState_Complete : eTaskState_Fail;
}

eTaskState zNPCBTWriteClosestPlayerAction::Update(float dt) {
    zPlayer* closest = 0;
    float best = 3.4028235e+38f;

    for (int i = 0; i < xglobals->players.numPlayers; i++) {
        if (!xglobals->players.playerArray[i]->IsAI()) {
            float d = xVec3Dist2(
                &npcBase->npcEntity->model->Mat.pos,
                &xglobals->players.playerArray[i]->model->Mat.pos);

            if (d < best) {
                best = d;
                closest = xglobals->players.playerArray[i];
            }
        }
    }

    if (closest == 0) {
        return eTaskState_Fail;
    }

    return btClient->blackboard.Write(NPC_VAR_CURRENT_PLAYER, closest) ? eTaskState_Complete : eTaskState_Fail;
}

// ---------------------------------------------------------------------------
// zNPCBTWriteTargetPlayerAction: every so often, score each player -- how
// near, how many enemies attack and target it (compiled to nothing), and
// whether it is human -- and target the best.

float zNPCBTWriteTargetPlayerAction::FindWeightedDistance(const xVec3& pos) {
    const Sext::Action_NPC_Write_TargetPlayer* asset =
        (const Sext::Action_NPC_Write_TargetPlayer*)actionAsset;
    float range = asset->DistanceRange;
    bool exclude = asset->DistanceExclude;
    float weight = asset->DistanceWeight;
    float d2 = xVec3Dist2(&npcBase->npcEntity->model->Mat.pos, &pos);

    if (d2 > range * range && exclude) {
        return -1.0f;
    }

    if (range >= -1e-5f && range <= 1e-5f) {
        return 0.0f;
    }

    float t = 1.0f - d2 / (range * range);

    return xmax(0.0f, xmin(t, 1.0f)) * weight;
}

float zNPCBTWriteTargetPlayerAction::FindWeightedNumAttacking(
    const zPlayer* player) {
    if (((const Sext::Action_NPC_Write_TargetPlayer*)actionAsset)
            ->AttackingEnemy == 0) {
        return 0.0f;
    }

    return 0.0f;
}

float zNPCBTWriteTargetPlayerAction::FindWeightedNumTargeting(
    const zPlayer* player) {
    if (((const Sext::Action_NPC_Write_TargetPlayer*)actionAsset)
            ->TargetingEnemy == 0) {
        return 0.0f;
    }

    return 0.0f;
}

float zNPCBTWriteTargetPlayerAction::FindWeightedPlayerStatus(
    const zPlayer* player) {
    const Sext::Action_NPC_Write_TargetPlayer* asset =
        (const Sext::Action_NPC_Write_TargetPlayer*)actionAsset;
    float playerStatusWeight = asset->HumanAIWeight;

    if (player->IsAI() && asset->HumanAIExclude) {
        return -1.0f;
    }

    float statusVal = player->IsAI() ? 0.0f : 1.0f;

    return statusVal * playerStatusWeight;
}

void zNPCBTWriteTargetPlayerAction::Begin() { timer = -1.0f; }

eTaskState zNPCBTWriteTargetPlayerAction::Update(float dt) {
    const Sext::Action_NPC_Write_TargetPlayer* asset =
        (const Sext::Action_NPC_Write_TargetPlayer*)actionAsset;
    float period = asset->UpdatePeriod;
    bool checkIfInWallNet = asset->CheckInWallNet;
    bool checkIfSwitched = asset->CheckSwitch;
    bool notifyPlayer = asset->NotifyPlayer;
    bool runOnce = period < 0.0f;

    if (timer > 0.0f) {
        timer -= dt;
        return eTaskState_Running;
    }

    timer = period;

    float maxPriority = 0.0f;
    zPlayer* player = 0;

    for (int i = 0; i < xglobals->players.numPlayers; i++) {
        zPlayer* p = xglobals->players.playerArray[i];

        if (p->eName == 2 || p->eName == 3) {
            continue;
        }

        xVec3& curPos = p->model->Mat.pos;

        if (checkIfInWallNet && npcBase->npcSteering->wallNet != 0 &&
            !npcBase->npcSteering->wallNet->IsInsideWallNetXZ(curPos)) {
            continue;
        }

        float priority = FindWeightedNumAttacking(p) +
                         FindWeightedDistance(curPos) +
                         FindWeightedNumTargeting(p) +
                         FindWeightedPlayerStatus(p);

        if (priority > maxPriority) {
            maxPriority = priority;
            player = p;
        }
    }

    zPlayer* currentPlayer;

    btClient->blackboard.Read(NPC_VAR_CURRENT_PLAYER, currentPlayer);

    if (currentPlayer != 0 && checkIfSwitched) {
        if (player == currentPlayer) {
            return runOnce ? eTaskState_Complete : eTaskState_Running;
        }

        if (notifyPlayer) {
            zEntEvent((xBase*)npcBase, 0, (xBase*)currentPlayer, 0xBACE84F5,
                      0, (ForceEvent)1);
        }
    }

    if (!btClient->blackboard.Write(NPC_VAR_CURRENT_PLAYER, player)) {
        return eTaskState_Fail;
    }

    if (player == 0) {
        return eTaskState_Fail;
    }

    if (notifyPlayer) {
        zEntEvent((xBase*)npcBase, 0, (xBase*)player, 0x9659DB59, 0,
                  (ForceEvent)1);
    }

    return runOnce ? eTaskState_Complete : eTaskState_Running;
}

// ---------------------------------------------------------------------------
// Patrol: the next move point is a random child of the current target, or
// the NPC's own move point when there is no target yet.

eTaskState zNPCBTWritePatrolMovePointAction::Update(float dt) {
    xMovePoint* currMP = 0;

    btClient->blackboard.Read(NPC_VAR_TARGET_MP, currMP);

    if (currMP == 0) {
        currMP = npcBase->npcMovePoint;

        if (currMP == 0) {
            return eTaskState_Fail;
        }

        btClient->blackboard.Write(NPC_VAR_TARGET_MP, currMP);

        xVec3 mpPos = *currMP->pos;

        btClient->blackboard.Write(NPC_VAR_CURRENT_DESTINATION, mpPos);
    } else {
        xMovePoint* nextMP = 0;

        currMP->GetRandomChildren(&nextMP, 0, 0);

        if (nextMP != 0) {
            btClient->blackboard.Write(NPC_VAR_TARGET_MP, nextMP);

            xVec3 mpPos = *nextMP->pos;

            btClient->blackboard.Write(NPC_VAR_CURRENT_DESTINATION, mpPos);
        }
    }

    return eTaskState_Complete;
}

template <>
bool zBlackboard::Write<xMovePoint*>(unsigned int id,
                                     xMovePoint* const& value) {
    zVariableBase* v = Find(id);

    if (v == 0) {
        return false;
    }

    zVariable<xMovePoint*>* result;

    zVariableDynamicCast::Cast(v, result);

    unsigned int i;
    void* p = result;
    zVariable<xMovePoint*>* var = (zVariable<xMovePoint*>*)p;

    if (var != 0) {
        var->value = value;

        for (i = 0; i < 8; i++) {
            if (var->observers[i] != 0) {
                var->observers[i]->Invoke();
            }
        }

        return true;
    }

    return false;
}

template <>
bool zBlackboard::Read<xMovePoint*>(unsigned int id, xMovePoint*& out) const {
    zVariableBase* v = Find(id);

    if (v == 0) {
        return false;
    }

    zVariable<xMovePoint*>* result;

    zVariableDynamicCast::Cast(v, result);

    zVariable<xMovePoint*>* var = result;

    if (var != 0) {
        out = var->GetValue();
        return true;
    }

    return false;
}

// ---------------------------------------------------------------------------
// Wander and player positions

eTaskState zNPCBTWriteWanderPositionAction::Update(float dt) {
    xVec3 pos;

    if (!_v10(pos)) {
        return eTaskState_Fail;
    }

    return btClient->blackboard.Write(NPC_VAR_CURRENT_DESTINATION, pos) ? eTaskState_Complete : eTaskState_Fail;
}

eTaskState zNPCFlyingBTWriteWanderPositionAction::Update(float dt) {
    xVec3 pos;

    _v10(pos);
    pos.y = BT_Utility::CalcFlyingNPCsAdjustedY(
        &((const Sext::Action_NPC_FlyingWrite_WanderPosition*)actionAsset)
             ->HeightAdjustment,
        npcBase, pos);

    return btClient->blackboard.Write(NPC_VAR_CURRENT_DESTINATION, pos) ? eTaskState_Complete : eTaskState_Fail;
}

// The player a variable names, or the closest one; its position, at the
// NPC's own height when asked, plus an offset that is world-space, in the
// NPC's frame or in the player's.
bool zNPCBTWritePlayerPositionAction::FindPlayerPos(xVec3& pos) {
    const Sext::Action_NPC_Write_PlayerPosition* asset =
        (const Sext::Action_NPC_Write_PlayerPosition*)actionAsset;
    int offsetType = asset->OffsetType;
    Math::Vector offset(asset->Offset.x, asset->Offset.y, asset->Offset.z);
    zPlayer* player = 0;

    if (asset->PlayerVariable != 0) {
        btClient->blackboard.Read(asset->PlayerVariable, player);
    } else {
        player = npcBase->GetClosestPlayerOnWallnet(asset->IgnoreHeight);
    }

    if (player == 0) {
        return false;
    }

    pos = player->model->Mat.pos;

    if (asset->IgnoreHeight) {
        pos.y = npcBase->npcEntity->model->Mat.pos.y;
    }

    switch (offsetType) {
    case 1:
        pos += *(const xVec3*)&offset;
        break;
    case 2:
        xMat3x3RMulVec((xVec3*)&offset, &npcBase->npcEntity->model->Mat,
                       (const xVec3*)&offset);
        pos += *(const xVec3*)&offset;
        break;
    case 3:
        xMat3x3RMulVec((xVec3*)&offset, &player->model->Mat,
                       (const xVec3*)&offset);
        pos += *(const xVec3*)&offset;
        break;
    }

    return true;
}

eTaskState zNPCFlyingBTWritePlayerPositionAction::Update(float dt) {
    xVec3 pos;

    if (!_v10(pos)) {
        return eTaskState_Fail;
    }

    pos.y = BT_Utility::CalcFlyingNPCsAdjustedY(
        &((const Sext::Action_NPC_FlyingWrite_PlayerPosition*)actionAsset)
             ->HeightAdjustment,
        npcBase, pos);

    return btClient->blackboard.Write(NPC_VAR_CURRENT_DESTINATION, pos) ? eTaskState_Complete : eTaskState_Fail;
}

eTaskState zNPCBTWriteTrapPositionAction::Update(float dt) {
    xVec3 pos;

    if (!btClient->blackboard.Read(NPC_VAR_TRAP_POS, pos)) {
        return eTaskState_Fail;
    }

    return btClient->blackboard.Write(NPC_VAR_CURRENT_DESTINATION, pos) ? eTaskState_Complete : eTaskState_Fail;
}

eTaskState zNPCBTResetCurrentPlayerAction::Update(float dt) {
    zPlayer* none = 0;

    return btClient->blackboard.Write(NPC_VAR_CURRENT_PLAYER, none) ? eTaskState_Complete : eTaskState_Fail;
}

eTaskState zNPCBTSetNeedCombatCleanupAction::Update(float dt) {
    int value = ((const Sext::Action_NPC_SetFlag*)actionAsset)->Value;

    if (!btClient->blackboard.Write(NPC_VAR_NEED_COMBAT_CLEANUP, value)) {
        return eTaskState_Fail;
    }

    return eTaskState_Complete;
}

eTaskState zNPCBTSetNeedCombatTargetingCleanupAction::Update(float dt) {
    int value = ((const Sext::Action_NPC_SetFlag*)actionAsset)->Value;

    if (!btClient->blackboard.Write(NPC_VAR_NEED_COMBAT_TARGETING_CLEANUP,
                                    value)) {
        return eTaskState_Fail;
    }

    return eTaskState_Complete;
}

// The move point on the NPC's network nearest to or farthest from its
// perception target or itself, or a random one.
enum MovePointType {
    eMovePointType_Closest = 0,
    eMovePointType_Farthest = 1,
    eMovePointType_Random = 2,
    eMovePointType_ClosestToMe = 3,
    eMovePointType_FarthestToMe = 4,
    END_eMovePointType_ENUM = 5
};

eTaskState zNPCBTWriteNetworkMovePointAction::Update(float dt) {
    MovePointType type =
        (MovePointType)((const Sext::Action_NPC_Write_NetworkMovePoint*)
                            actionAsset)
            ->Type;

    xMovePoint* movePoint = npcBase->npcMovePoint;
    xMovePoint* foundMovePoint = 0;

    if (movePoint == 0) {
        return eTaskState_Fail;
    }

    switch (type) {
    case eMovePointType_Closest: {
        zNPCPerception* perception = npcBase->npcPerception;

        if (perception != 0) {
            xBase* target = perception->target;

            if (target != 0) {
                foundMovePoint =
                    movePoint->NetworkGetClosestXZ(target->model->Mat.pos);
            }
        }
        break;
    }
    case eMovePointType_Farthest: {
        zNPCPerception* perception = npcBase->npcPerception;

        if (perception != 0) {
            xBase* target = perception->target;

            if (target != 0) {
                foundMovePoint =
                    movePoint->NetworkGetFarthestXZ(target->model->Mat.pos);
            }
        }
        break;
    }
    case eMovePointType_Random:
        foundMovePoint = movePoint->NetworkGetRandom(false);
        break;
    case eMovePointType_ClosestToMe: {
        xVec3 npcPos;

        npcBase->GetPosition(npcPos);
        foundMovePoint = movePoint->NetworkGetClosestXZ(npcPos);
        break;
    }
    case eMovePointType_FarthestToMe: {
        xVec3 npcPos;

        npcBase->GetPosition(npcPos);
        foundMovePoint = movePoint->NetworkGetFarthestXZ(npcPos);
        break;
    }
    }

    if (foundMovePoint == 0) {
        return eTaskState_Fail;
    }

    xVec3 targetPos = *foundMovePoint->pos;

    if (btClient->blackboard.Write(NPC_VAR_TARGET_MP, foundMovePoint) &&
        btClient->blackboard.Write(NPC_VAR_CURRENT_DESTINATION, targetPos)) {
        return eTaskState_Complete;
    }

    return eTaskState_Fail;
}

eTaskState zNPCBTWriteBlackboardUidPosition::Update(float dt) {
    Sext::uid id;

    if (btClient->blackboard.Read(
            ((const Sext::Action_NPC_WriteVariable*)actionAsset)->VariableName,
            id)) {
        if (id.internalUid == 0) {
            return eTaskState_Fail;
        }

        xBase* obj = zSceneFindObject(id.internalUid);
        xVec3 pos;

        if (obj->baseType == 0x51) {
            pos = *((xMovePoint*)obj)->pos;
        } else {
            return eTaskState_Fail;
        }

        btClient->blackboard.Write(NPC_VAR_CURRENT_DESTINATION, pos);

        return eTaskState_Complete;
    }

    return eTaskState_Fail;
}

// The NPC's own position plus an offset in its frame, dropped onto the
// wallnet when the asset asks.
// The math library's local-to-world transform, inline: retail holds the
// matrix in a register across the first call and the DWARF has no local
// for it or for the rotated offset.
inline void xMat4x3Toworld(xVec3* o, const xMat4x3* m, const xVec3* v) {
    xVec3 temp;

    xMat3x3RMulVec(&temp, m, v);
    v3add(o, &temp, (xVec3*)&m->pos);
}

bool zNPCBTWriteCurrentPosition::FindCurrentPos(xVec3& targetPosition) {
    const Sext::Action_NPC_Write_CurrentPosition* asset =
        (const Sext::Action_NPC_Write_CurrentPosition*)actionAsset;
    Math::Vector offset(asset->Offset.x, asset->Offset.y, asset->Offset.z);

    xMat4x3Toworld(&targetPosition, &npcBase->npcEntity->model->Mat,
                   (const xVec3*)&offset);

    if (asset->ConstrainToWallnet) {
        zWallNet* wn = npcBase->npcSteering->wallNet;
        int triangleID;

        if (wn != 0 &&
            (triangleID = wn->FindTriangleIDXZ(targetPosition)) != 255) {
            wn->FindYOnTriangleFromXZ(
                &wn->wallNetAsset->triangles[triangleID], targetPosition.x,
                targetPosition.z, targetPosition.y);
        } else {
            return false;
        }
    }

    return true;
}

eTaskState zNPCFlyingBTWriteCurrentPosition::Update(float dt) {
    xVec3 pos;

    if (!_v10(pos)) {
        return eTaskState_Fail;
    }

    const Sext::Action_NPC_FlyingWrite_CurrentPosition* asset =
        (const Sext::Action_NPC_FlyingWrite_CurrentPosition*)actionAsset;

    if (!asset->ConstrainToWallnet) {
        pos.y = BT_Utility::CalcFlyingNPCsAdjustedY(&asset->HeightAdjustment,
                                                    npcBase, pos);
    }

    return btClient->blackboard.Write(NPC_VAR_CURRENT_DESTINATION, pos) ? eTaskState_Complete : eTaskState_Fail;
}

// Defined last: retail CALLS it from the Read and Write above, and read any
// earlier the auto-inliner would take it into them.
template <>
void zVariableDynamicCast::Cast<xMovePoint*>(zVariableBase* v,
                                             zVariable<xMovePoint*>*& out) {
    if (v->type == eVarType_xMovePointPtr) {
        out = (zVariable<xMovePoint*>*)v;
    } else {
        out = 0;
    }
}
