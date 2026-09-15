#include "SB/GM/Engine/Game/zNPCCommonSwarmBTActions.pool.h"

// zNPCCommonSwarmBTActions.cpp -- the behaviour-tree actions of swarm NPCs:
// move-to, wander, flock, flutter and path following for a swarm's members.
// Read from the image with tools/brief.py; the layouts are the DWARF's, and
// the strings and float literals come out of WAD02.cpp's pool, which the
// header in front reproduces.
//
// A swarm action drives every member of the swarm at once: it keeps one
// steering control, animation and stuck-range multiplier per member in
// arrays it allocates from zBTFactory, and hands the controls to the swarm's
// steering in one call. The swarm is polymorphic from +0: slot 28 counts its
// members and slot 29 returns one. Its steering is polymorphic from +4: slot
// 12 takes the controls, slot 13 gives them back and slot 14 steps them.
// The move-to family's slots 10 and 11 are IsArrived and IsStuck. An Update
// returns 1 while running, 3 when done and 4 when it cannot run.
//
// Virtuals are declared under slot names and never defined, so no vtable
// lands here; the functions this unit defines are declared non-virtual.

namespace Sext {

class ActionBase {};

class vec3 {
public:
    float x;
    float y;
    float z;
};

class Action_NPC_SwarmMoveTo : public ActionBase {
public:
    bool allArrive;
    float MaxSpeed;
};

class Action_NPC_SwarmFlutter : public Action_NPC_SwarmMoveTo {
public:
    float MinYOffset;
    float MaxYOffset;
    float GlideProbability;
};

class Action_NPC_Swarm_BugCollected : public ActionBase {
public:
    unsigned char BugType;
    unsigned char BugColor;
};

class Action_NPC_SwarmPathFollowMP : public ActionBase {
public:
    float PathWidth;
    float MaxSpeed;
};

class Action_NPC_SwarmPathFollowCircle : public ActionBase {
public:
    vec3 offset;
    float radius;
    float PathWidth;
    float MaxSpeed;
};

}  // namespace Sext

class zBTClient;
class zNPCBase;
class zNPCEntity;
class zNPCSteering;
class zNPCSteeringControl;
class zWallNet;
class NPCAsset;

// ---------------------------------------------------------------------------
// The factory the actions allocate from

namespace Memory {

enum eFactoryMemType { eFactoryMemType_14 = 14 };

class Factory {
public:
    void* AllocMem(unsigned int size, eFactoryMemType type);
};

}  // namespace Memory

inline void* operator new(unsigned long, void* p) { return p; }

class zBTFactory {
public:
    static void* Allocate(unsigned int size);
    static void Destroy(void* p);
    template <class T>
    static T* Create();

    static Memory::Factory factory;
};

template <class T>
T* zBTFactory::Create() {
    void* mem = factory.AllocMem(sizeof(T), Memory::eFactoryMemType_14);

    return !mem ? 0 : new (mem) T;
}

// ---------------------------------------------------------------------------
// Vectors and matrices. Assignment is out of line (every `a = b` on a vector
// is a call); a copy made at a declaration is not.

class xVec3 {
public:
    xVec3& operator=(const xVec3& other);

    float Distance2XZ(const xVec3& other) const;
    float length2() const;
    xVec3& Sub(const xVec3& a, const xVec3& b);
    xVec3& AddScale(const xVec3& v, float s);
    xVec3& AddScale(const xVec3& a, const xVec3& b, float s);
    float NormalizeSafe();
    void normalizeFast();

    static const xVec3 m_Null;
    static const xVec3 m_UnitAxisY;

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

void v3sub(xVec3* out, xVec3* a, xVec3* b);
void xVec3RotateOnAxis(xVec3& out, const xVec3& v, const xVec3& axis,
                       float angle);
void xLine3VecClosestPoint(const xVec3* a, const xVec3* b, const xVec3* p,
                           xVec3* out);

extern "C" double cos(double x);

float xrand_GenRandFloat();
float xrand_RandomFloatRange(float lo, float hi);

// A random value from lo to hi, in line.
inline float xrandf(float lo, float hi) {
    float t = xrand_GenRandFloat();

    return t * hi + (1.0f - t) * lo;
}

inline float xsqr(float x) { return x * x; }

// xVec3's dot product is folded onto hkVector4::dot3 in the image, and the
// calls are named by that symbol.
class hkVector4 {
public:
    float dot3(const hkVector4& other) const;
};

class xPlane {
public:
    xVec3 norm;
    float d;
};

// ---------------------------------------------------------------------------
// The engine's objects as the actions reach them

class xOGModel {
public:
    xMat4x3 Mat;
};

class xMovePoint {
public:
    void GetRandomChildren(xMovePoint** out, const xMovePoint* exclude,
                           xVec3* pos) const;

    unsigned char _pad0[0x40];
    xVec3* pos;
};

class zNPCBound {
public:
    float GetBoundMinRadiusXZ() const;

    xVec3 center;
    xVec3 extent;
    unsigned char _pad0[0x30 - 0x18];
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
    unsigned char _pad2[0xF4 - 0x5C];
    zNPCBound npcBound;
};

enum eNPCType {
    eNPCType_GenericSwarm = 5,
};

class zNPCType {
public:
    eNPCType npcTypeEnum;
};

// The NPC, polymorphic from +0: a swarm counts and hands out its members.
class zNPCBaseVirtuals {
public:
    virtual void _n0(); virtual void _n1(); virtual void _n2();
    virtual void _n3(); virtual void _n4(); virtual void _n5();
    virtual void _n6(); virtual void _n7(); virtual void _n8();
    virtual void _n9(); virtual void _n10(); virtual void _n11();
    virtual void _n12(); virtual void _n13(); virtual void _n14();
    virtual void _n15(); virtual void _n16(); virtual void _n17();
    virtual void _n18(); virtual void _n19(); virtual void _n20();
    virtual void _n21(); virtual void _n22(); virtual void _n23();
    virtual void _n24(); virtual void _n25(); virtual void _n26();
    virtual void _n27();
    virtual int GetNumberOfChildren();
    virtual zNPCEntity* GetChild(int i) const;
};

class zNPCBase : public zNPCBaseVirtuals {
public:
    unsigned char _pad0[0x5C - 0x4];
    zNPCType* type;
    NPCAsset* npcAsset;
    unsigned char _pad1[0x71 - 0x64];
    bool infoNodesUpdating : 1;
    bool quietKill : 1;
    bool updating : 1;
    bool flying : 1;
    bool collectible : 1;
    unsigned char _pad2[0x80 - 0x72];
    xMovePoint* npcMovePoint;
    xMovePoint* npcMovePointNetwork;
    unsigned char _pad3[0xA0 - 0x88];
    zNPCSteering* npcSteering;
    unsigned char _pad4[0xC0 - 0xA4];
};

class zNPCGenericSwarm : public zNPCBase {
public:
    unsigned char _pad5[0x264 - 0xC0];
    xVec3 hidePoint;
    int numberOfKilledMembers;
    unsigned int killedMembers;
};

// The steering component: the owner, then the vtable pointer at +4.
class zNPCComponentData {
public:
    zNPCBase* owner;
};

class zNPCComponent : public zNPCComponentData {
public:
    virtual void _c0(); virtual void _c1(); virtual void _c2();
    virtual void _c3(); virtual void _c4(); virtual void _c5();
    virtual void _c6(); virtual void _c7(); virtual void _c8();
    virtual void _c9();
};

class zNPCSteering : public zNPCComponent {
public:
    virtual void _s10();
    virtual void _s11();
    virtual void SetControls(zNPCSteeringControl** controls, int count);
    virtual void RemoveControls(zNPCSteeringControl** controls, int count);
    virtual void ApplySteering(float dt);

    unsigned char _pad0[0x34 - 0x8];
    zWallNet* wallNet;
};

// ---------------------------------------------------------------------------
// Steering controls and paths

class zNPCSteeringDest {
public:
    // Aims the pointer at the destination it holds, then fills it in.
    void Set(const xVec3& d) {
        pDest = &dest;
        dest = d;
    }

    xVec3 dest;
    xVec3* pDest;
};

class zNPCSteeringVecLimiter {
public:
    xVec3 limitV;
    int isSphere;
};

class zNPCSteeringControl {
public:
    zNPCSteering* steering;
    zNPCBase* npcBase;
    zNPCEntity* npcEntity;
    void* wallNetPosition;
    int headingCalcType;
    zNPCSteeringDest customHeading;
    float speedLimitXZ;
    float speedLimitY;
    // Four one-bit flags in a four-byte slot: a bool bitfield takes one
    // byte, so the other three are spelled out.
    bool gravityEnabled : 1;
    bool forceApplyGravity : 1;
    bool turnSpringEnabled : 1;
    bool ignoreYComponent : 1;
    unsigned char _pad2D[3];
    unsigned char movementStyle;
    float maxAcc;
    zNPCSteeringVecLimiter accLimiter;

    virtual void _v0();
};

class zWanderData {
public:
    xVec3 prevWanderDir;
    float wanderAcc;
    float wanderFOV;
    float wanderJitterPerSec;
    bool ignoreY;
    bool enabled;
};

class zWallAvoidanceData {
public:
    float avoidanceMaxAcc;
    float avoidanceTime;
    float avoidanceMinDist;
    bool enabled;
};

class zPlayerAvoidanceData {
public:
    float avoidanceAcc;
    float avoidanceMaxAcc;
    float avoidanceMaxDist;
    bool ignoreY;
    bool enabled;
};

class zObjectAvoidanceData {
public:
    float avoidanceMaxAcc;
    float avoidanceTime;
    float avoidanceMinDist;
    bool enabled;
};

class zNPCSteeringAccumulator {
public:
    xVec3 curAccumulatedVec;
    float curAccumulatedMag;
};

class zNPCSteeringMoveToControl : public zNPCSteeringControl {
public:
    xVec3* GetReachableDest();

    zWanderData wanderData;
    zWallAvoidanceData wallAvoidanceData;
    zPlayerAvoidanceData playerAvoidanceData;
    zObjectAvoidanceData objectAvoidanceData;
    zNPCSteeringDest destination;
    unsigned char pidController[0x11C];
    float arriveTolerance2;
    int arriveToDest;
    bool useWallNet;
    zNPCSteeringAccumulator accAccum;
    unsigned char wallNetDestination[0x2C];
};

class zSteeringPath;

class zNPCSteeringFollowPathControl : public zNPCSteeringControl {
public:
    zNPCSteeringFollowPathControl();

    zWanderData wanderData;
    zNPCSteeringAccumulator accAccum;
    zSteeringPath* path;
};

// A path keeps two floats in front of its vtable pointer at +8.
class zSteeringPathData {
public:
    float halfWidth;
    float halfWidthSqr;
};

class zSteeringPath : public zSteeringPathData {
public:
    void SetHalfWidth(float hw);

    virtual void _p0();
};

class zSteeringPathMovePoints : public zSteeringPath {
public:
    void UpdateToNextMP(const xVec3& pos);
    void SetupNextMP(const xMovePoint* mp, const xVec3& pos);

    xMovePoint* currMP;
    xMovePoint* prevMP;
    unsigned char linePath[0x30];
    unsigned char splinePath[0x10];
    zSteeringPath* currPath;
    float arriveTol2;
};

class zSteeringPathCircle : public zSteeringPath {
public:
    xVec3 center;
    xPlane plane;
    float radius;
    bool clockwise;
};

// ---------------------------------------------------------------------------
// The blackboard, as zBlackboard.cpp spells it. Every Read and Write is
// that unit's.

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

// File statics of the unity build (symbols.txt: scope:local), read as memory
// nothing here writes.
extern const unsigned int NPC_VAR_CURRENT_DESTINATION;
extern const unsigned int NPC_VAR_TARGET_MP;

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

class zNPCBTActionAnim {
public:
    zNPCBTActionAnim()
        : animationName(0), blendTime(0.2f), animStartTime(0.0f),
          enabled(true) {}

    void Init(const char* name, float blend, float start);
    void StartOnNPC(zNPCEntity* npc, bool force);

    const char* animationName;
    unsigned int animStateID;
    float blendTime;
    float animStartTime;
    float leanMaxAngle;
    bool enabled;
};

class zNPCBTStuckRangeMultiplier {
public:
    void Update(const xVec3& vel, float dt);
    bool IsMaxed() const { return curMultiplier == maxMultiplier; }

    float stuckVel2;
    float multiplierRate;
    float maxMultiplier;
    float curMultiplier;
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
    virtual bool _v10(const zNPCEntity* npcEnt, const xVec3* dest);
    virtual bool _v11(const zNPCEntity* npcEnt,
                      const zNPCBTStuckRangeMultiplier* stuckRangeMultiplier,
                      const xVec3* dest);
};

class zNPCBTAction : public zBTAction {
public:
    zNPCBase* npcBase;
};

class zNPCBTSwarmMoveToAction : public zNPCBTAction {
public:
    void Setup(const Sext::ActionBase* a);
    void Begin();
    bool IsArrived(const zNPCEntity* npcEnt, const xVec3* dest);
    bool IsStuck(const zNPCEntity* npcEnt,
                 const zNPCBTStuckRangeMultiplier* stuckRangeMultiplier,
                 const xVec3* dest);
    eTaskState Update(float dt);
    void Cleanup();
    void SetMaxSpeed(float maxSpeed);
    void SetSeekToDest();
    inline void SetDestination(const xVec3& dest, int i);

    zNPCSteeringMoveToControl** moveToControls;
    zNPCBTActionAnim** actionAnims;
    zNPCBTStuckRangeMultiplier** stuckRangeMultipliers;
    float arriveTolerance;
    bool exitWhenAllArrive;
    bool setupDone;
};

class zNPCBTSwarmWanderAction : public zNPCBTSwarmMoveToAction {
public:
    void Begin();
    xVec3 GetWanderDestination(const zNPCEntity* npcEnt);
    eTaskState Update(float dt);
};

class zNPCBTSwarmFlockAction : public zNPCBTSwarmMoveToAction {
public:
    void InitMP();
    void Begin();
    eTaskState Update(float dt);
    void NextMP(int i);
    xVec3 GetFlockDestination(const zNPCEntity* npcEnt, xMovePoint* prevMP,
                              xMovePoint* curMP);
    bool IsDestAtMP(const xVec3* dest, xMovePoint* prevMP, xMovePoint* curMP);

    xMovePoint* prevMPs[64];
    xMovePoint* curMPs[64];
    float RANDOM_ANGLE_RANGE;
    float ARRIVED_COS_TOLERANCE;
};

class zNPCBTSwarmFlutterAction : public zNPCBTSwarmMoveToAction {
public:
    void Begin();
    xVec3 GetFlutterDestination(const zNPCEntity* npcEnt);
    xVec3 GetGlideDestination(const zNPCEntity* npcEnt);
    eTaskState Update(float dt);
    void Cleanup();

    bool* gliding;
};

class zNPCBTWriteSwarmHidePointAction : public zNPCBTAction {
public:
    eTaskState Update(float dt);
};

class zNPCBTSwarmBugCollectedAction : public zNPCBTAction {
public:
    void Setup(const Sext::ActionBase* a);

    int bugType;
    int bugColor;
};

class zNPCBTSwarmResetKilledMembersAction : public zNPCBTAction {
public:
    eTaskState Update(float dt);
};

class zNPCBTSwarmPathFollowMPAction : public zNPCBTAction {
public:
    void Setup(const Sext::ActionBase* a);
    void Begin();
    bool ArrivedAtMP(int member);
    eTaskState Update(float dt);
    void End();
    void Cleanup();

    zNPCSteeringFollowPathControl** followControls;
    zNPCBTActionAnim** actionAnims;
    zSteeringPathMovePoints** paths;
    float pathHalfWidth;
    float maxSpeed;
    bool setupDone;
};

class zNPCBTSwarmPathFollowCircleAction : public zNPCBTAction {
public:
    void Begin();
    eTaskState Update(float dt);
    void Cleanup();

    zNPCSteeringFollowPathControl** followControls;
    zNPCBTActionAnim** actionAnims;
    zSteeringPathCircle** paths;
    bool setupDone;
};

#define xmin(a, b) ((a) < (b) ? (a) : (b))

// ---------------------------------------------------------------------------
// zNPCBTSwarmMoveToAction

void zNPCBTSwarmMoveToAction::Setup(const Sext::ActionBase* a) {
    exitWhenAllArrive = ((const Sext::Action_NPC_SwarmMoveTo*)a)->allArrive;
    setupDone = false;
}

bool zNPCBTSwarmMoveToAction::IsArrived(const zNPCEntity* npcEnt,
                                        const xVec3* dest) {
    float dist2 = npcEnt->model->Mat.pos.Distance2XZ(*dest);
    float boundXZ = npcEnt->npcBound.GetBoundMinRadiusXZ();

    return dist2 < boundXZ * boundXZ + arriveTolerance * arriveTolerance;
}

// Stuck: within the member's radius scaled by its stuck-range multiplier --
// in 3D for a flier (the radius capped by the half height), in XZ otherwise
// -- or the multiplier has run out.
bool zNPCBTSwarmMoveToAction::IsStuck(
    const zNPCEntity* npcEnt,
    const zNPCBTStuckRangeMultiplier* stuckRangeMultiplier,
    const xVec3* dest) {
    if (npcBase->flying) {
        xVec3 diff;

        diff.Sub(*dest, npcEnt->model->Mat.pos);

        float dist2 = diff.length2();
        float bound = xmin(npcEnt->npcBound.GetBoundMinRadiusXZ(),
                           npcEnt->npcBound.extent.y);

        return dist2 < bound * bound *
                           xsqr(stuckRangeMultiplier->curMultiplier) ||
               stuckRangeMultiplier->IsMaxed();
    } else {
        float dist2 = npcEnt->model->Mat.pos.Distance2XZ(*dest);
        float boundXZ = npcEnt->npcBound.GetBoundMinRadiusXZ();

        return dist2 < boundXZ * boundXZ *
                           xsqr(stuckRangeMultiplier->curMultiplier) ||
               stuckRangeMultiplier->IsMaxed();
    }
}

// Complete as soon as one member arrives or sticks, unless every member has
// to; `allArrived` is cleared by every member the loop passes, so with
// members it never reports all of them arrived.
eTaskState zNPCBTSwarmMoveToAction::Update(float dt) {
    npcBase->npcSteering->ApplySteering(dt);

    bool allArrived = true;
    int numberOfChildren = npcBase->GetNumberOfChildren();

    for (int i = 0; i < numberOfChildren; i++) {
        zNPCEntity* npcEnt = npcBase->GetChild(i);

        stuckRangeMultipliers[i]->Update(npcEnt->frame->vel, dt);

        if ((_v10(npcEnt, moveToControls[i]->GetReachableDest()) ||
             _v11(npcEnt, stuckRangeMultipliers[i],
                  moveToControls[i]->GetReachableDest())) &&
            !exitWhenAllArrive) {
            return eTaskState_Complete;
        }

        allArrived = false;
    }

    return allArrived ? eTaskState_Complete : eTaskState_Running;
}

void zNPCBTSwarmMoveToAction::Cleanup() {
    if (setupDone) {
        int numChildren = npcBase->GetNumberOfChildren();

        npcBase->npcSteering->RemoveControls(
            (zNPCSteeringControl**)moveToControls, numChildren);

        for (int i = 0; i < numChildren; i++) {
            zBTFactory::Destroy(moveToControls[i]);
            zBTFactory::Destroy(actionAnims[i]);
            zBTFactory::Destroy(stuckRangeMultipliers[i]);
        }

        zBTFactory::Destroy(moveToControls);
        moveToControls = 0;
        zBTFactory::Destroy(actionAnims);
        actionAnims = 0;
        zBTFactory::Destroy(stuckRangeMultipliers);
        stuckRangeMultipliers = 0;
    }
}

void zNPCBTSwarmMoveToAction::SetMaxSpeed(float maxSpeed) {
    int numberOfChildren = npcBase->GetNumberOfChildren();

    for (int i = 0; i < numberOfChildren; i++) {
        moveToControls[i]->speedLimitXZ = maxSpeed;
    }
}

// Seek rather than arrive: every member runs through its destination.
void zNPCBTSwarmMoveToAction::SetSeekToDest() {
    int numberOfChildren = npcBase->GetNumberOfChildren();

    for (int i = 0; i < numberOfChildren; i++) {
        moveToControls[i]->arriveToDest = 0;
    }
}

// ---------------------------------------------------------------------------
// zNPCBTSwarmWanderAction: every member wanders to a random point, and to a
// new one whenever it arrives.

void zNPCBTSwarmWanderAction::Begin() {
    zNPCBTSwarmMoveToAction::Begin();

    int numberOfChildren = npcBase->GetNumberOfChildren();

    for (int i = 0; i < numberOfChildren; i++) {
        zNPCEntity* npcEnt = npcBase->GetChild(i);

        SetDestination(GetWanderDestination(npcEnt), i);
    }

    SetMaxSpeed(2.0f);
    exitWhenAllArrive = false;
}

eTaskState zNPCBTSwarmWanderAction::Update(float dt) {
    npcBase->npcSteering->ApplySteering(dt);

    int numChildren = npcBase->GetNumberOfChildren();

    for (int i = 0; i < numChildren; i++) {
        zNPCEntity* npcEnt = npcBase->GetChild(i);
        xVec3* dest = moveToControls[i]->GetReachableDest();

        if (_v10(npcEnt, dest)) {
            SetDestination(GetWanderDestination(npcEnt), i);
        }
    }

    return eTaskState_Running;
}

// ---------------------------------------------------------------------------
// zNPCBTSwarmFlockAction: every member flies from move point to move point
// through the network, each along its own random line between the two.

void zNPCBTSwarmFlockAction::InitMP() {
    xMovePoint* npcMP = npcBase->npcMovePoint;

    for (int i = 0; i < 64; i++) {
        if (npcMP == 0) {
            prevMPs[i] = 0;
            curMPs[i] = 0;
        } else {
            prevMPs[i] = npcMP;
            npcMP->GetRandomChildren(&curMPs[i], 0, 0);
        }
    }
}

void zNPCBTSwarmFlockAction::Begin() {
    zNPCBTSwarmMoveToAction::Begin();
    InitMP();
    SetMaxSpeed(2.0f);
    SetSeekToDest();
    exitWhenAllArrive = true;

    int numberOfChildren = npcBase->GetNumberOfChildren();

    for (int i = 0; i < numberOfChildren; i++) {
        zNPCEntity* npcEnt = npcBase->GetChild(i);

        SetDestination(GetFlockDestination(npcEnt, prevMPs[i], curMPs[i]), i);
    }
}

// Complete when a member runs out of move points, unless every member has
// to.
eTaskState zNPCBTSwarmFlockAction::Update(float dt) {
    npcBase->npcSteering->ApplySteering(dt);

    bool allArrived = true;
    int numberOfChildren = npcBase->GetNumberOfChildren();

    for (int i = 0; i < numberOfChildren; i++) {
        zNPCEntity* npcEnt = npcBase->GetChild(i);
        xVec3* dest = moveToControls[i]->GetReachableDest();

        if (_v10(npcEnt, dest)) {
            if (IsDestAtMP(dest, prevMPs[i], curMPs[i])) {
                NextMP(i);
            }

            SetDestination(GetFlockDestination(npcEnt, prevMPs[i], curMPs[i]),
                           i);
        }

        if (curMPs[i] == 0) {
            if (!exitWhenAllArrive) {
                return eTaskState_Complete;
            }
        } else {
            allArrived = false;
        }
    }

    return allArrived ? eTaskState_Complete : eTaskState_Running;
}

void zNPCBTSwarmFlockAction::NextMP(int i) {
    if (curMPs[i] != 0) {
        prevMPs[i] = curMPs[i];
        prevMPs[i]->GetRandomChildren(&curMPs[i], 0, 0);
    }
}

// A random point on a line from the previous move point, turned by a random
// angle about the vertical from the line to the current one, between as far
// along as the member already is and as far as the current one.
xVec3 zNPCBTSwarmFlockAction::GetFlockDestination(const zNPCEntity* npcEnt,
                                                  xMovePoint* prevMP,
                                                  xMovePoint* curMP) {
    xVec3 dest = xVec3::m_Null;

    if (prevMP == 0 || curMP == 0) {
        dest = prevMP != 0 ? *prevMP->pos : npcEnt->model->Mat.pos;

        return dest;
    }

    xVec3* prevPos = prevMP->pos;
    xVec3* curPos = curMP->pos;
    xVec3 prevToCur;

    v3sub(&prevToCur, curPos, prevPos);

    float mpDist = prevToCur.NormalizeSafe();

    if (mpDist < 0.00001f) {
        return *curMP->pos;
    }

    float randomAngle =
        xrand_RandomFloatRange(-RANDOM_ANGLE_RANGE, RANDOM_ANGLE_RANGE);
    xVec3 prevToCurRotated;

    xVec3RotateOnAxis(prevToCurRotated, prevToCur, xVec3::m_UnitAxisY,
                      randomAngle);
    prevToCurRotated.normalizeFast();

    float minDist;
    float maxDist;
    float cosAngle = cos(randomAngle);

    maxDist = cosAngle < 0.00001f ? mpDist : mpDist / cosAngle;

    xVec3 maxDest;

    maxDest.AddScale(*prevPos, prevToCurRotated, maxDist);

    xVec3 npcPosProj;

    xLine3VecClosestPoint(prevPos, curPos, &npcEnt->model->Mat.pos,
                          &npcPosProj);

    xVec3 prevMPToNPCProj;

    prevMPToNPCProj.Sub(npcPosProj, *prevPos);

    float traveled = prevMPToNPCProj.NormalizeSafe();
    float curDist = cosAngle < 0.00001f ? maxDist : traveled / cosAngle;

    minDist = maxDist - curDist < 1.0f ? maxDist : curDist;

    float randomDist = xrand_RandomFloatRange(minDist, maxDist);

    dest.AddScale(*prevPos, prevToCurRotated, randomDist);

    return dest;
}

// At the move point: no line to measure against, on the point already, or
// off the line from the previous point by more than the tolerance allows.
bool zNPCBTSwarmFlockAction::IsDestAtMP(const xVec3* dest, xMovePoint* prevMP,
                                        xMovePoint* curMP) {
    if (prevMP == 0 || curMP == 0) {
        return false;
    }

    xVec3* curPos = curMP->pos;
    xVec3 curToPrevMP;

    v3sub(&curToPrevMP, prevMP->pos, curPos);

    float dist = curToPrevMP.NormalizeSafe();

    if (dist < 0.00001f) {
        return true;
    }

    xVec3 curToDest;

    v3sub(&curToDest, (xVec3*)dest, curPos);
    dist = curToDest.NormalizeSafe();

    if (dist < 0.00001f) {
        return true;
    }

    float cosAngle = (*(const hkVector4*)&curToPrevMP)
                         .dot3(*(const hkVector4*)&curToDest);

    cosAngle = __fabs(cosAngle);

    return cosAngle < ARRIVED_COS_TOLERANCE;
}

// ---------------------------------------------------------------------------
// zNPCBTSwarmFlutterAction: every member flutters about, and now and then
// glides down.

void zNPCBTSwarmFlutterAction::Begin() {
    zNPCBTSwarmMoveToAction::Begin();

    int numberOfChildren = npcBase->GetNumberOfChildren();

    gliding = (bool*)zBTFactory::Allocate(numberOfChildren);

    for (int i = 0; i < numberOfChildren; i++) {
        zNPCEntity* npcEnt = npcBase->GetChild(i);

        SetDestination(GetFlutterDestination(npcEnt), i);
        moveToControls[i]->arriveToDest = 0;
        moveToControls[i]->arriveTolerance2 = 0.25f;
        moveToControls[i]->playerAvoidanceData.enabled = true;
        moveToControls[i]->playerAvoidanceData.ignoreY = false;
        moveToControls[i]->playerAvoidanceData.avoidanceAcc = 10.0f;
        moveToControls[i]->playerAvoidanceData.avoidanceMaxAcc = 10.0f;
        moveToControls[i]->wallAvoidanceData.avoidanceTime = 0.4f;
        gliding[i] = false;
    }
}

// A member that arrives or sticks glides down now and then, never twice in a
// row, and flutters on otherwise.
eTaskState zNPCBTSwarmFlutterAction::Update(float dt) {
    npcBase->npcSteering->ApplySteering(dt);

    const Sext::Action_NPC_SwarmFlutter* asset =
        (const Sext::Action_NPC_SwarmFlutter*)actionAsset;
    int numChildren = npcBase->GetNumberOfChildren();

    for (int i = 0; i < numChildren; i++) {
        zNPCEntity* npcEnt = npcBase->GetChild(i);
        xVec3* dest = moveToControls[i]->GetReachableDest();

        stuckRangeMultipliers[i]->Update(npcEnt->frame->vel, dt);

        if (_v10(npcEnt, dest) ||
            _v11(npcEnt, stuckRangeMultipliers[i], dest)) {
            float prob = xrandf(0.0f, 1.0f);

            if (!gliding[i] && prob < asset->GlideProbability) {
                actionAnims[i]->Init("GLIDE", 0.2f, 0.0f);
                SetDestination(GetGlideDestination(npcEnt), i);
                gliding[i] = true;
            } else {
                actionAnims[i]->Init("MOVE", 0.2f, 0.0f);
                SetDestination(GetFlutterDestination(npcEnt), i);
                gliding[i] = false;
            }

            actionAnims[i]->StartOnNPC(npcEnt, false);
        }
    }

    return eTaskState_Running;
}

void zNPCBTSwarmFlutterAction::Cleanup() {
    zNPCBTSwarmMoveToAction::Cleanup();

    if (gliding != 0) {
        zBTFactory::Destroy(gliding);
        gliding = 0;
    }
}

// Weak in retail, and called, not taken in line, by every Wander, Flock and
// Flutter function above: defined below them.
inline void zNPCBTSwarmMoveToAction::SetDestination(const xVec3& dest, int i) {
    moveToControls[i]->destination.Set(dest);
}

// ---------------------------------------------------------------------------
// zNPCBTWriteSwarmHidePointAction, zNPCBTSwarmBugCollectedAction,
// zNPCBTSwarmResetKilledMembersAction

eTaskState zNPCBTWriteSwarmHidePointAction::Update(float dt) {
    if (npcBase->type->npcTypeEnum != eNPCType_GenericSwarm) {
        return eTaskState_Fail;
    }

    xVec3 hidePoint = ((zNPCGenericSwarm*)npcBase)->hidePoint;

    btClient->blackboard.Write(NPC_VAR_CURRENT_DESTINATION, hidePoint);

    return eTaskState_Complete;
}

void zNPCBTSwarmBugCollectedAction::Setup(const Sext::ActionBase* a) {
    const Sext::Action_NPC_Swarm_BugCollected* asset =
        (const Sext::Action_NPC_Swarm_BugCollected*)a;

    bugType = asset->BugType;
    bugColor = asset->BugColor;
}

eTaskState zNPCBTSwarmResetKilledMembersAction::Update(float dt) {
    zNPCGenericSwarm* swarm = (zNPCGenericSwarm*)npcBase;

    if (swarm->type->npcTypeEnum != eNPCType_GenericSwarm) {
        return eTaskState_Fail;
    }

    swarm->numberOfKilledMembers = swarm->killedMembers = 0;

    return eTaskState_Complete;
}

// ---------------------------------------------------------------------------
// zNPCBTSwarmPathFollowMPAction

void zNPCBTSwarmPathFollowMPAction::Setup(const Sext::ActionBase* a) {
    const Sext::Action_NPC_SwarmPathFollowMP* asset =
        (const Sext::Action_NPC_SwarmPathFollowMP*)a;

    pathHalfWidth = 0.5f * asset->PathWidth;
    maxSpeed = asset->MaxSpeed;
    setupDone = false;
}

bool zNPCBTSwarmPathFollowMPAction::ArrivedAtMP(int member) {
    zNPCEntity* npcEnt = npcBase->GetChild(member);
    float dist2 =
        npcEnt->model->Mat.pos.Distance2XZ(*paths[member]->currMP->pos);

    return dist2 < 0.01f;
}

eTaskState zNPCBTSwarmPathFollowMPAction::Update(float dt) {
    npcBase->npcSteering->ApplySteering(dt);

    int numChildren = npcBase->GetNumberOfChildren();

    for (int i = 0; i < numChildren; i++) {
        zNPCEntity* npcEnt = npcBase->GetChild(i);

        if (ArrivedAtMP(i)) {
            paths[i]->UpdateToNextMP(npcEnt->model->Mat.pos);
        }
    }

    return eTaskState_Running;
}

// The first member's current move point is handed back through the
// blackboard.
void zNPCBTSwarmPathFollowMPAction::End() {
    if (setupDone) {
        xMovePoint* mp = paths[0]->currMP;

        btClient->blackboard.Write(NPC_VAR_TARGET_MP, mp);
    }
}

void zNPCBTSwarmPathFollowMPAction::Cleanup() {
    if (setupDone) {
        int numChildren = npcBase->GetNumberOfChildren();

        npcBase->npcSteering->RemoveControls(
            (zNPCSteeringControl**)followControls, numChildren);

        for (int i = 0; i < numChildren; i++) {
            zBTFactory::Destroy(followControls[i]);
            zBTFactory::Destroy(actionAnims[i]);
            zBTFactory::Destroy(paths[i]);
        }

        zBTFactory::Destroy(followControls);
        followControls = 0;
        zBTFactory::Destroy(actionAnims);
        actionAnims = 0;
        zBTFactory::Destroy(paths);
        paths = 0;
    }
}

// ---------------------------------------------------------------------------
// zNPCBTSwarmPathFollowCircleAction

// Each member is fetched and nothing is done with it.
eTaskState zNPCBTSwarmPathFollowCircleAction::Update(float dt) {
    npcBase->npcSteering->ApplySteering(dt);

    int numChildren = npcBase->GetNumberOfChildren();

    for (int i = 0; i < numChildren; i++) {
        npcBase->GetChild(i);
    }

    return eTaskState_Running;
}

void zNPCBTSwarmPathFollowCircleAction::Cleanup() {
    if (setupDone) {
        int numChildren = npcBase->GetNumberOfChildren();

        npcBase->npcSteering->RemoveControls(
            (zNPCSteeringControl**)followControls, numChildren);

        for (int i = 0; i < numChildren; i++) {
            zBTFactory::Destroy(followControls[i]);
            zBTFactory::Destroy(actionAnims[i]);
            zBTFactory::Destroy(paths[i]);
        }

        zBTFactory::Destroy(followControls);
        followControls = 0;
        zBTFactory::Destroy(actionAnims);
        actionAnims = 0;
        zBTFactory::Destroy(paths);
        paths = 0;
    }
}

// The factory instances retail keeps out of line in this unit. Their only
// callers are Begins past the four-literal wall, not written, so they are
// instantiated here; always_inline takes zNPCBTActionAnim's constructor in
// line, as retail does, where the explicit instantiation alone calls it.
//
// Not written, each past the four-literal wall (retail spells a lis per
// literal where mwcc forms a shared addis base): zNPCBTSwarmMoveToAction::
// Begin (12 distinct literals), zNPCBTSwarmWanderAction::
// GetWanderDestination (5), zNPCBTSwarmFlutterAction::GetFlutterDestination
// (5) and GetGlideDestination (6), zNPCBTSwarmPathFollowMPAction::Begin (4)
// and zNPCBTSwarmPathFollowCircleAction::Begin (4).
#pragma push
#pragma always_inline on
template zNPCBTActionAnim* zBTFactory::Create<zNPCBTActionAnim>();
template zNPCSteeringFollowPathControl*
zBTFactory::Create<zNPCSteeringFollowPathControl>();
#pragma pop
