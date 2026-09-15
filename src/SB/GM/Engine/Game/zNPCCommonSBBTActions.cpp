#include "SB/GM/Engine/Game/zNPCCommonSBBTActions.pool.h"

// zNPCCommonSBBTActions.cpp -- the behaviour-tree actions of this game's own
// NPCs: Plankton's shake, bounce, spin vortex, perception targets, the squid
// block timer, the turret and the chumbot fist. Read from the image with
// tools/brief.py; the layouts are the DWARF's, and the strings and float
// literals come out of WAD02.cpp's pool, which the header in front
// reproduces.
//
// Every action keeps its asset, a resume callback and its client in front
// of the vtable pointer at +0xC, and the NPC it drives at +0x10. An Update
// returns 1 while running, 3 when done and 4 when it cannot run. Virtuals
// are declared under slot names and never defined, so no vtable lands here;
// the functions this unit defines are declared non-virtual.

#define xmin(a, b) ((a) < (b) ? (a) : (b))

extern "C" double sin(double x);
extern "C" double cos(double x);

namespace Sext {

class ActionBase {};
class EventAny;

enum eRPSAttackTypes {
    eRPSAttackTypes_None = 0,
    END_eRPSAttackTypes_ENUM = 0x7FFFFFFF
};

enum eNPCPerceptionType {
    eNPCPerceptionType_First = 0,
    END_eNPCPerceptionType_ENUM = 0x7FFFFFFF
};

enum eNPCMoveType {
    eNPCMoveType_Mosey = 0,
    eNPCMoveType_Custom = 7,
};

// The movement block of every moving action's asset.
class NPC_Action_MovementData {
public:
    float MaxSpeed;
    float MaxAcceleration;
    float TurningRadius;
    float TurnSpring;
    const char* AnimationName;
    eNPCMoveType MoveType;
    int Heading;
    bool StopAtExit;
    bool Arrive;
    float ArriveTolerance;
};

class Action_NPC_SetRPSAttackState : public ActionBase {
public:
    unsigned char State;
    float Delay;
};

class Rotation3 {
public:
    float yaw;
    float pitch;
    float roll;
};

class Action_NPC_Bounce : public ActionBase {
public:
    float MinBounce;
    float MaxBounce;
    Rotation3 Orientation;
};

class Action_NPC_GenerateSpinVortex : public ActionBase {
public:
    float Radius;
    float Acceleration;
};

class Action_NPC_WriteSquidBossBlockTime : public ActionBase {
public:
    unsigned long long StageCounterUID;
    unsigned int TimeVariable;
    unsigned int IndexVariable;
    unsigned int MultiplierVariable;
    bool Block;
};

// Not in the DWARF: Setup reads one float, the flash's length.
class Action_NPC_ChumbotFistFlash : public ActionBase {
public:
    float Duration;
};

class Action_NPC_FollowPerceptionTarget : public ActionBase {
public:
    unsigned char Target;
    NPC_Action_MovementData MovementData;
    bool EndActionWhenArrived;
};

// How a flying NPC's height is chosen.
class FlyingNPCHeightAdjustment {
public:
    float offset;
    unsigned int HeightAdjustmentType;
};

class Action_NPC_Write_PerceptionTargetPosition : public ActionBase {
public:
    unsigned char Target;
};

class Action_NPC_FlyingWrite_PerceptionTargetPosition
    : public Action_NPC_Write_PerceptionTargetPosition {
public:
    FlyingNPCHeightAdjustment HeightAdjustment;
};

// Not in the DWARF: the shoot action's asset names its projectile at +8.
class Action_NPC_Shoot : public ActionBase {
public:
    unsigned char _pad0[0x8];
    unsigned long long ProjectileID;
};

// The generic NPC asset's variant data (DWARF __variantUnion__): a type,
// then a turret's rate of fire or a bomb's fuse, twelve bytes after a type
// word of their own, and the word after them.
class NPCGenericTurretData {
public:
    int ShootCycleCount;
    float ShootCycleDelay;
    float ShootDelay;
};

class NPCGenericFuseData {
public:
    float fuseDuration;
    float redStateDuration;
    float yellowStateDuration;
};

class NPCGeneric {
public:
    unsigned char _pad0[0x190];
    int variantType;
    int variantDataType;
    union {
        NPCGenericTurretData turret;
        NPCGenericFuseData fuse;
    };
    unsigned int fuseDurationOverride;
};

}  // namespace Sext

class xBase;
class zNPCBase;
class zNPCEntity;
class zNPCSteering;
class zNPCSteeringControl;

enum ForceEvent { ForceEvent_ = 0x7FFFFFFF };

void zEntEvent(xBase* from, unsigned int fromEvent, xBase* to,
               unsigned int toEvent, Sext::EventAny* args, ForceEvent force);

xBase* zSceneFindObject(unsigned long long id);

// ---------------------------------------------------------------------------
// Vectors and matrices. Assignment and the compound operators are out of
// line; a copy made at a declaration is not.

// sin and cos through by-value float helpers: retail loads the angle for
// the second call before it rounds the first result, which a plain
// `(float)sin(fRadians)` does not do.
inline float isin(float x) { return (float)sin(x); }
inline float icos(float x) { return (float)cos(x); }

class xVec3 {
public:
    xVec3& operator=(const xVec3& other);
    xVec3& operator+=(const xVec3& other);
    xVec3& operator*=(float s);

    float Distance2XZ(const xVec3& other) const;
    float length2() const;
    float NormalizeSafe();

    xVec3& rotateX(const float& fRadians) {
        float fSin, fCos, fTemp1, fTemp2;

        fSin = isin(fRadians);
        fCos = icos(fRadians);

        fTemp1 = y * fCos - z * fSin;
        fTemp2 = y * fSin + z * fCos;

        y = fTemp1;
        z = fTemp2;

        return *this;
    }

    // Weak in retail and called, not taken in line, from the bounce's
    // Begin; defined below it.
    xVec3& rotateY(const float& fRadians);

    xVec3& rotateZ(const float& fRadians) {
        float fSin, fCos, fTemp1, fTemp2;

        fSin = isin(fRadians);
        fCos = icos(fRadians);

        fTemp1 = x * fCos - y * fSin;
        fTemp2 = x * fSin + y * fCos;

        x = fTemp1;
        y = fTemp2;

        return *this;
    }

    static const xVec3 m_Null;

    float x;
    float y;
    float z;
};

void v3sub(xVec3* o, xVec3* a, xVec3* b);
float xatan2(float y, float x);

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

namespace World {

class xOGModel {
public:
    void SetColorMultiplier(float r, float g, float b, float a);

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

class ProjectileAsset {
public:
    unsigned char _pad0[0xD8];
    float fuseDuration;
    float redStateDuration;
    float yellowStateDuration;
};

// ---------------------------------------------------------------------------
// The engine's objects as the actions reach them

class xEntFrame {
public:
    unsigned char _pad0[0x88];
    xVec3 vel;
};

// The low half of the word at +0x44 is a sixteen-bit field: every change to
// it takes those bits out, sets or clears one and puts them back.
class xEnt {
public:
    unsigned char _pad0[0x34];
    World::xOGModel* model;
    unsigned char _pad1[0x44 - 0x38];
    unsigned int moreFlagsHi : 16;
    unsigned int moreFlags : 16;
    unsigned char _pad2[0x58 - 0x48];
    xEntFrame* frame;
};

class zNPCBound {
public:
    float GetBoundMinRadiusXZ() const;

    xVec3 center;
    xVec3 extent;
    unsigned char _pad0[0x30 - 0x18];
};

class zNPCEntity : public xEnt {
public:
    xVec3& GetPos() { return model->Mat.pos; }
    // Weak in retail and called, not taken in line, from the shake's
    // Begin; defined below it.
    float GetYaw() const;
    bool DoesAnimExist(unsigned int animID);
    bool IsAnimationStopped(unsigned int animID);

    unsigned char _pad3[0xF4 - 0x5C];
    zNPCBound npcBound;
};

class xMovePoint {
public:
    xMovePoint* NetworkGetClosestXZ(const xVec3& pos);
    xMovePoint* NetworkGetNextOnPath(const xMovePoint* dest);
    void GetRandomChildren(xMovePoint** out, const xMovePoint* exclude,
                           xVec3* pos) const;

    unsigned char _pad0[0x40];
    xVec3* pos;
};

class xSpringy {
public:
    float mResponse;
};

class xSpringyVec3 : public xSpringy {
public:
    void Update(float dt);

    xVec3 mVelocity;
    xVec3 mGoal;
    xVec3 mCurrent;
};

class xSpringyF32 : public xSpringy {
public:
    void Reset();

    float mVelocitySaveMax;
    float mVelocityMax;
    float mVelocity;
    float mGoal;
    float mCurrent;
};

class xSpringyAngle : public xSpringyF32 {
public:
    void Update(float dt);
};

class xCounter {
public:
    unsigned char _pad0[0x40];
    short count;
};

// An NPC component: the owner, then the vtable pointer at +4.
class zNPCComponentData {
public:
    zNPCBase* owner;
};

class zNPCComponent : public zNPCComponentData {
public:
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

class zNPCSteering : public zNPCComponent {
public:
    virtual void _s10(zNPCSteeringControl* control);
    virtual void _s11(zNPCSteeringControl* control);
    virtual void _s12();
    virtual void _s13();
    virtual void _s14(float dt);

    unsigned char _pad0[0x10 - 0x8];
    xVec3 externalAccAccum;
    xVec3 externalVelocityAccum;
};

class zNPCGetsDamageInfo {
public:
    unsigned int flags;
    unsigned char _pad0[0x20 - 0x4];
    xVec3 knockback;
};

class zNPCCombat {
public:
    void SetAttackState(Sext::eRPSAttackTypes type, float time);

    // Through the getter the bound is re-read every pass.
    unsigned int GetDamageCount() const { return damageListSize; }
    zNPCGetsDamageInfo* GetDamageInfo(unsigned int i) {
        return (i < GetDamageCount()) ? &damageList[i] : 0;
    }

    unsigned char _pad0[0x29C];
    zNPCGetsDamageInfo damageList[6];
    unsigned char _pad1[0x3BC - 0x3A4];
    unsigned int damageListSize;
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

    unsigned char _pad0[0x10];
    zNPCPerceptionTarget targets[4];
    unsigned int targetBitMask;
};

enum eNPCInfoNodeType {
    eNPCInfoNodeType_Bouncer = 3,
    END_eNPCInfoNodeType_ENUM = 0x7FFFFFFF
};

class zNPCInfoNode;

class zNPCBase {
public:
    zNPCInfoNode* GetInfoNode(eNPCInfoNodeType type);
    void AttachInfoNode(zNPCInfoNode* node, eNPCInfoNodeType type);
    void DetachInfoNode(eNPCInfoNodeType type);

    unsigned char _pad0[0x60];
    Sext::NPCGeneric* npcAsset;
    unsigned char _pad1[0x80 - 0x64];
    xMovePoint* npcMovePoint;
    unsigned char _pad2[0x98 - 0x84];
    zNPCEntity* npcEntity;
    void* npcSteeringOld;
    zNPCSteering* npcSteering;
    zNPCPerception* npcPerception;
    zNPCCombat* npcCombat;
};

class zBouncer {
public:
    void Setup(xBase* ent, const xVec3& n, float minB, float maxB);

    xBase* owner;
    xVec3 normal;
    float minBounce;
    float maxBounce;
    float maxBounceAngle;
};

// The info node keeps three words in front of its vtable pointer at +0xC.
// Its vtable is weak in retail and not this unit's, so the bouncer node's
// constructor is spelled out against the table's symbol.
extern "C" char __vt__19zNPCInfoNodeBouncer[];

class zNPCInfoNode {
public:
    zNPCInfoNode* next;
    zNPCBase* owner;
    eNPCInfoNodeType infoNodeType;
    void* vtable;
};

class zNPCInfoNodeBouncer : public zNPCInfoNode {
public:
    zNPCInfoNodeBouncer() {
        owner = 0;
        vtable = __vt__19zNPCInfoNodeBouncer;
        bouncer = 0;
    }

    zBouncer* bouncer;
};

namespace Memory {

enum eFactoryMemType { eFactoryMemType_ = 0x7FFFFFFF };

class Factory {
public:
    void* AllocMem(unsigned int size, eFactoryMemType type);
};

}  // namespace Memory

inline void* operator new(unsigned long, void* p) { return p; }

class zNPCManager {
public:
    static Memory::Factory factory;
};

// zSBPlayer is polymorphic from +0; slot 48 is IsDead and slot 111
// ImpartAcceleration (tools/vtslot.py __vt__9zSBPlayer 200 452).
#define V10(a) virtual void _v##a##0(); virtual void _v##a##1(); \
    virtual void _v##a##2(); virtual void _v##a##3(); \
    virtual void _v##a##4(); virtual void _v##a##5(); \
    virtual void _v##a##6(); virtual void _v##a##7(); \
    virtual void _v##a##8(); virtual void _v##a##9();

class zSBPlayerVirtuals {
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
    V10(1) V10(2) V10(3)
    virtual void _v40();
    virtual void _v41();
    virtual void _v42();
    virtual void _v43();
    virtual void _v44();
    virtual void _v45();
    virtual void _v46();
    virtual void _v47();
    virtual bool IsDead() const;
    virtual void _v49();
    V10(5) V10(6) V10(7) V10(8) V10(9)
    virtual void _v100();
    virtual void _v101();
    virtual void _v102();
    virtual void _v103();
    virtual void _v104();
    virtual void _v105();
    virtual void _v106();
    virtual void _v107();
    virtual void _v108();
    virtual void _v109();
    virtual void _v110();
    virtual void ImpartAcceleration(const xVec3& acc);
};

#undef V10

class zSBPlayer : public zSBPlayerVirtuals {
public:
    void StopMinibossLyleVortexFX();
    void UpdateMinibossLyleVortexFX(xVec3 dir);

    unsigned char _pad0[0x34 - 0x4];
    World::xOGModel* model;
};

class zBoardPlayer {
public:
    static zSBPlayer* GetInstance();
};

// ---------------------------------------------------------------------------
// Steering controls, as zNPCCommonMovementBTActions.cpp lays them out

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

class zAvoidanceData {
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
    zAvoidanceData wallAvoidanceData;
    zAvoidanceData playerAvoidanceData;
    zAvoidanceData objectAvoidanceData;
    zNPCSteeringDest destination;
    unsigned char pidController[0x11C];
    float arriveTolerance2;
    int arriveToDest;
    bool useWallNet;
    zNPCSteeringAccumulator accAccum;
    unsigned char wallNetDestination[0x2C];
};

namespace BT_Utility {

void SetMovementLimits(const zNPCBase* npcBase, zNPCSteering* steeringComponent,
                       zNPCSteeringControl* control, Sext::eNPCMoveType moveType,
                       float customMaxSpeed, float customMaxAcc,
                       float customTurningRadius, float customTurnSpring);

float CalcFlyingNPCsAdjustedY(const Sext::FlyingNPCHeightAdjustment* adj,
                              const zNPCBase* npc, const xVec3& pos);

}  // namespace BT_Utility

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

// The turret variant action reaches its four variables the way retail
// does, from the unity unit's first .bss object (0x807340F0), 0x1F0C0
// bytes ahead of them, and the zero and one it writes from the .data
// label, 0x644 bytes ahead of those temporaries. The two arrays are that
// distance, referenced by nothing and holding nothing.
// The variables are the unity unit's file statics (scope:local), defined
// here for that reason; a fragment to be LINKED needs the unity unit.
static unsigned char kUnityDataAhead[0x644] = {1};
static unsigned char kUnityBssAhead[0x1F0C0];
static unsigned int NPC_VAR_TURRET_VARIANT_ROF_ENABLED;
static unsigned int NPC_VAR_TURRET_SHOOT_CYCLE_COUNT;
static unsigned int NPC_VAR_TURRET_SHOOT_CYCLE_DELAY;
static unsigned int NPC_VAR_TURRET_SHOOT_DELAY;

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
    void Init(const char* name, float blend, float start);
    void SetAnimation(Sext::eNPCMoveType type, const char* name);
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
    // IsStuck's spelling: through these the equality comes back as a bool
    // (mfcr) and reuses the load the squared multiplier made.
    float GetMultiplier2() const { return curMultiplier * curMultiplier; }
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
    virtual bool _v10(xVec3& out);
};

class zNPCBTAction : public zBTAction {
public:
    zNPCBase* npcBase;
};

class zNPCBTSetRPSAttackStateAction : public zNPCBTAction {
public:
    void Setup(const Sext::ActionBase* a);
    eTaskState Update(float dt);

    Sext::eRPSAttackTypes state;
    float delay;
};

class zNPCBTClearDamageInfoAction : public zNPCBTAction {
public:
    eTaskState Update(float dt);
};

class zNPCBTWriteGopherNextMovepointAction : public zNPCBTAction {
public:
    eTaskState Update(float dt);
};

class zNPCBTPlanktonShakeAction : public zNPCBTAction {
public:
    void Begin();
    eTaskState Update(float dt);
    void End();
    void UpdateShakeBlend(const xVec3& vel, float dt);

    zNPCBTActionAnim dropAnim;
    zNPCBTActionAnim slamPrepAnim;
    zNPCBTActionAnim slamAnim;
    zNPCBTActionAnim shakeAnim;
    xVec3 initialPos;
    xSpringyVec3 posSpring;
    xSpringyAngle angleSpring;
    float blendx;
    float blendy;
    bool released;
    bool hitGround;
    bool performingSlam;
    bool performingSlamPrep;
    bool damaged;
    float height;
    float amplitude;
};

class zNPCBTSetPlanktonShakableAction : public zNPCBTAction {
public:
    eTaskState Update(float dt);

    bool setTo;
};

class zNPCBTBounceAction : public zNPCBTAction {
public:
    void Setup(const Sext::ActionBase* a);
    void Begin();
    eTaskState Update(float dt);
    void End();

    bool attached;
    const Sext::Action_NPC_Bounce* bounceAsset;
    zBouncer bouncer;
    zNPCInfoNodeBouncer* infoNode;
};

class zNPCBTRespondToKnockbackAction : public zNPCBTAction {
public:
    eTaskState Update(float dt);
};

class zNPCBTGenerateSpinVortexAction : public zNPCBTAction {
public:
    void Setup(const Sext::ActionBase* a);
    void End();
    eTaskState Update(float dt);

    float radiusSq;
    float acceleration;
};

// The events sent when a perception type starts and stops being perceived.
class EventMapNode {
public:
    unsigned int percievedEvent;
    unsigned int notPercievedEvent;
};

class zNPCBTMonitorPerceptionAction : public zNPCBTAction {
public:
    void Begin();
    eTaskState Update(float dt);

    static const EventMapNode eventMap[6];

    bool perceptionStates[6];
};

class zNPCBTShootAction : public zNPCBTAction {
public:
    eTaskState Update(float dt);
};

class zNPCBT_Bomb_Shoot_Action : public zNPCBTShootAction {
public:
    eTaskState Update(float dt);
};

class zNPCBT_Turret_GetVariantData_Action : public zNPCBTAction {
public:
    eTaskState Update(float dt);
};

class zNPCBTWriteSquidBlockTimeAction : public zNPCBTAction {
public:
    eTaskState Update(float dt);
    void Setup(const Sext::ActionBase* assetData);

    unsigned int indexIntegerVariable;
    unsigned int timeFloatVariable;
    unsigned int multiplierFloatVariable;
    xCounter* stageCounter;
    bool block;
};

class zNPCBTChumbotFistFlashAction : public zNPCBTAction {
public:
    void Setup(const Sext::ActionBase* a);
    eTaskState Update(float dt);

    float flashDuration;
    float flashTime;
};

class zNPCBTFollowPerceptionTargetAction : public zNPCBTAction {
public:
    void Initialize();
    void Setup(const Sext::ActionBase* a);
    void Begin();
    bool IsArrived();
    bool IsStuck();
    void UpdateDestination();
    eTaskState Update(float dt);

    // Through the accessor the destination's pointer is stored off the
    // destination itself, as retail has it, and not off the action.
    zNPCSteeringMoveToControl* SteeringControl() { return &moveToControl; }

    zNPCSteeringMoveToControl moveToControl;
    zNPCBTActionAnim actionAnim;
    zNPCBTStuckRangeMultiplier stuckRangeMultiplier;
    float arriveRangeMultiplier2;
    bool endWhenArrived;
    unsigned int targetNum;
};

class zNPCBTWritePerceptionTargetPositionAction : public zNPCBTAction {
public:
    void Setup(const Sext::ActionBase* a);
    bool FindPerceptionTargetPos(xVec3& pos);

    unsigned int targetNum;
};

class zNPCFlyingBTWritePerceptionTargetPositionAction
    : public zNPCBTWritePerceptionTargetPositionAction {
public:
    eTaskState Update(float dt);
};

// ---------------------------------------------------------------------------
// Rock-paper-scissors attack state, damage info, gopher move points

void zNPCBTSetRPSAttackStateAction::Setup(const Sext::ActionBase* a) {
    const Sext::Action_NPC_SetRPSAttackState* asset =
        (const Sext::Action_NPC_SetRPSAttackState*)a;

    state = (Sext::eRPSAttackTypes)asset->State;
    delay = asset->Delay;
}

eTaskState zNPCBTSetRPSAttackStateAction::Update(float dt) {
    zNPCCombat* combat = npcBase->npcCombat;

    if (combat != 0) {
        combat->SetAttackState(state, delay);

        return eTaskState_Complete;
    }

    return eTaskState_Fail;
}

eTaskState zNPCBTClearDamageInfoAction::Update(float dt) {
    zNPCCombat* combat = npcBase->npcCombat;

    if (combat != 0) {
        combat->damageListSize = 0;
        return eTaskState_Complete;
    }

    return eTaskState_Fail;
}

// The next move point on the path from the one nearest the NPC to the
// target, or a random child of it when the path goes nowhere.
eTaskState zNPCBTWriteGopherNextMovepointAction::Update(float dt) {
    xMovePoint* start;
    xMovePoint* target;
    xMovePoint* next;

    if (!btClient->blackboard.Read(NPC_VAR_TARGET_MP, target)) {
        return eTaskState_Fail;
    }

    xMovePoint* network = npcBase->npcMovePoint;

    if (network == 0) {
        return eTaskState_Fail;
    }

    start = network->NetworkGetClosestXZ(npcBase->npcEntity->model->Mat.pos);

    next = start->NetworkGetNextOnPath(target);

    if (start == next) {
        start->GetRandomChildren(&next, 0, 0);
    }

    if (next == 0) {
        return eTaskState_Fail;
    }

    xVec3 targetPos = *next->pos;

    if (btClient->blackboard.Write(NPC_VAR_TARGET_MP, next) &&
        btClient->blackboard.Write(NPC_VAR_CURRENT_DESTINATION, targetPos)) {
        return eTaskState_Complete;
    }

    return eTaskState_Fail;
}

// ---------------------------------------------------------------------------
// zNPCBTPlanktonShakeAction

// Everything starts at rest where the NPC stands, facing its own yaw, and
// the NPC is marked as held.
//
// NEAR MISS: 16 of 93 words (retail 388 B, ours 372 B); five distinct
// float literals (0, 1, 0.5, 0.1, 0.2), the four-literal wall (NOTES.md):
// retail spells a lis per literal and this compiler forms one addis base
// past the pool padding. Written for GetYaw, which retail emits after it.
void zNPCBTPlanktonShakeAction::Begin() {
    zNPCEntity* ent = npcBase->npcEntity;

    blendx = blendy = 0.0f;
    released = hitGround = performingSlam = performingSlamPrep = damaged =
        false;

    height = 1.0f;
    amplitude = 0.5f;

    initialPos = ent->model->Mat.pos;
    posSpring.mGoal = initialPos;
    posSpring.mCurrent = posSpring.mGoal;
    posSpring.mVelocity.x = posSpring.mVelocity.y = posSpring.mVelocity.z =
        0.0f;
    angleSpring.mGoal = ent->GetYaw();
    angleSpring.Reset();

    dropAnim.Init("SHAKE_DROP", 0.1f, 0.0f);
    slamPrepAnim.Init("SHAKE_SLAMPREP", 0.1f, 0.0f);

    if (ent->DoesAnimExist(slamPrepAnim.animStateID)) {
        slamAnim.Init("SHAKE_SLAM", 0.0f, 0.0f);
    } else {
        slamAnim.Init("SHAKE_SLAM", 0.2f, 0.0f);
    }

    shakeAnim.Init("SHAKE", 0.2f, 0.0f);
    shakeAnim.StartOnNPC(ent, false);

    ent->moreFlags |= 0x800;
}

inline float zNPCEntity::GetYaw() const {
    return xatan2(model->Mat.at.x, model->Mat.at.z);
}

// Not written: Update (1,796 B) and UpdateShakeBlend (696 B) here, and
// zNPCBT_SplashDamage_Action::Update (108 B) further down -- seven, seven
// and four distinct float literals, the four-literal wall.

// The masks are sixteen-bit constants: `~0x800` would clear the bit with a
// rotate mask where retail ANDs the field with 0xF7FF.
void zNPCBTPlanktonShakeAction::End() {
    npcBase->npcEntity->moreFlags &= 0xF7FF;
}

eTaskState zNPCBTSetPlanktonShakableAction::Update(float dt) {
    xEnt* ent = npcBase->npcEntity;

    if (setTo) {
        ent->moreFlags |= 0x400;
    } else {
        ent->moreFlags &= 0xFBFF;
    }

    return eTaskState_Complete;
}

// ---------------------------------------------------------------------------
// zNPCBTBounceAction

void zNPCBTBounceAction::Setup(const Sext::ActionBase* a) {
    bounceAsset = (const Sext::Action_NPC_Bounce*)a;
    infoNode = 0;
}

// The bouncer's normal is the NPC's up axis turned by the asset's
// orientation; the NPC carries it in a bouncer info node unless it already
// has one.
// rotateX, rotateZ and the node's constructor are in line in retail;
// without the pragma each comes out as a copy of its own.
#pragma push
#pragma always_inline on
void zNPCBTBounceAction::Begin() {
    xVec3 normal = npcBase->npcEntity->model->Mat.up;

    normal.rotateX(bounceAsset->Orientation.pitch);
    normal.rotateY(bounceAsset->Orientation.yaw);
    normal.rotateZ(bounceAsset->Orientation.roll);

    bouncer.Setup((xBase*)npcBase, normal, bounceAsset->MinBounce,
                  bounceAsset->MaxBounce);

    if (npcBase->GetInfoNode(eNPCInfoNodeType_Bouncer) != 0) {
        attached = false;
    } else {
        void* mem = zNPCManager::factory.AllocMem(
            sizeof(zNPCInfoNodeBouncer), (Memory::eFactoryMemType)11);

        infoNode = !mem ? 0 : new (mem) zNPCInfoNodeBouncer();

        if (infoNode != 0) {
            infoNode->bouncer = &bouncer;
            npcBase->AttachInfoNode(infoNode, eNPCInfoNodeType_Bouncer);
            attached = true;
        } else {
            attached = false;
        }
    }
}
#pragma pop

inline xVec3& xVec3::rotateY(const float& fRadians) {
    float fSin, fCos, fTemp10, fTemp20;

    fSin = isin(fRadians);
    fCos = icos(fRadians);

    fTemp10 = z * fSin + x * fCos;
    fTemp20 = z * fCos - x * fSin;

    x = fTemp10;
    z = fTemp20;

    return *this;
}

eTaskState zNPCBTBounceAction::Update(float dt) {
    return attached ? eTaskState_Running : eTaskState_Fail;
}

void zNPCBTBounceAction::End() {
    npcBase->DetachInfoNode(eNPCInfoNodeType_Bouncer);
    infoNode = 0;
}

// The knockback of every hit that carried one, added to the steering's
// external velocity.
eTaskState zNPCBTRespondToKnockbackAction::Update(float dt) {
    zNPCCombat* combat = npcBase->npcCombat;
    zNPCSteering* steering = npcBase->npcSteering;

    if (combat != 0 && steering != 0) {
        xVec3 knockback = xVec3::m_Null;

        unsigned int damageCount = combat->GetDamageCount();

        for (unsigned int i = 0; i < damageCount; i++) {
            zNPCGetsDamageInfo* info = combat->GetDamageInfo(i);

            if (info->flags & 1) {
                knockback += info->knockback;
            }
        }

        steering->externalVelocityAccum += knockback;

        return eTaskState_Complete;
    }

    return eTaskState_Fail;
}

// ---------------------------------------------------------------------------
// zNPCBTGenerateSpinVortexAction: pull the SpongeBob player in while he is
// inside the radius, with the miniboss's vortex effect on him.

void zNPCBTGenerateSpinVortexAction::Setup(const Sext::ActionBase* a) {
    const Sext::Action_NPC_GenerateSpinVortex* asset =
        (const Sext::Action_NPC_GenerateSpinVortex*)a;

    radiusSq = asset->Radius * asset->Radius;
    acceleration = asset->Acceleration;
}

void zNPCBTGenerateSpinVortexAction::End() {
    zBoardPlayer::GetInstance()->StopMinibossLyleVortexFX();
}

eTaskState zNPCBTGenerateSpinVortexAction::Update(float dt) {
    xVec3* npcPos = &npcBase->npcEntity->model->Mat.pos;
    zSBPlayer* sbPlayer = zBoardPlayer::GetInstance();

    if (!sbPlayer->IsDead()) {
        xVec3 displacement;

        v3sub(&displacement, npcPos, &sbPlayer->model->Mat.pos);

        float distanceSq = displacement.length2();

        if (distanceSq <= radiusSq) {
            displacement.NormalizeSafe();
            displacement *= acceleration;

            sbPlayer->ImpartAcceleration(displacement);

            sbPlayer->UpdateMinibossLyleVortexFX(displacement);
        } else {
            sbPlayer->StopMinibossLyleVortexFX();
        }
    } else {
        sbPlayer->StopMinibossLyleVortexFX();
    }

    return eTaskState_Running;
}

// ---------------------------------------------------------------------------
// zNPCBTMonitorPerceptionAction: an event to the NPC whenever a perception
// type starts or stops being perceived.

void zNPCBTMonitorPerceptionAction::Begin() {
    zNPCPerception* perception = npcBase->npcPerception;

    if (perception != 0) {
        for (unsigned int i = 0; i < 6; i++) {
            perceptionStates[i] = perception->AreTargetsPerceived(
                perception->targetBitMask, (Sext::eNPCPerceptionType)i, false);
        }
    }
}

eTaskState zNPCBTMonitorPerceptionAction::Update(float dt) {
    zNPCPerception* perception = npcBase->npcPerception;

    if (perception != 0) {
        for (unsigned int i = 0; i < 6; i++) {
            bool perceived = perception->AreTargetsPerceived(
                perception->targetBitMask, (Sext::eNPCPerceptionType)i, false);

            if (perceived != perceptionStates[i]) {
                if (perceived) {
                    zEntEvent(0, 0, (xBase*)npcBase,
                              eventMap[i].percievedEvent, 0, (ForceEvent)1);
                } else {
                    zEntEvent(0, 0, (xBase*)npcBase,
                              eventMap[i].notPercievedEvent, 0,
                              (ForceEvent)1);
                }

                perceptionStates[i] = perceived;
            }
        }

        return eTaskState_Running;
    }

    return eTaskState_Fail;
}

// ---------------------------------------------------------------------------
// zNPCBT_Bomb_Shoot_Action and zNPCBT_Turret_GetVariantData_Action: the
// generic NPC asset's variant data, handed to the projectile or written to
// the blackboard.

eTaskState zNPCBT_Bomb_Shoot_Action::Update(float dt) {
    const Sext::NPCGeneric* npcAsset = npcBase->npcAsset;

    World::EntityHandleBase* projHandle = World::EntityManager::FindHandle(
        ((const Sext::Action_NPC_Shoot*)actionAsset)->ProjectileID);
    ProjectileAsset* projectileAsset =
        (ProjectileAsset*)projHandle->BlobData();

    if (npcAsset->fuseDurationOverride == 1) {
        projectileAsset->fuseDuration = npcAsset->fuse.fuseDuration;
        projectileAsset->redStateDuration = npcAsset->fuse.redStateDuration;
        projectileAsset->yellowStateDuration =
            npcAsset->fuse.yellowStateDuration;
    }

    return zNPCBTShootAction::Update(dt);
}

// NEAR MISS: 9 of 67 words; the .bss base (the four variables) and the
// .data base (the zero and one) land in r5 and r6 the other way round
// from retail. The padding arrays and file statics above take it from 69
// differing words to 9; the .data array declared after the statics, or
// after this function, moves nothing.
eTaskState zNPCBT_Turret_GetVariantData_Action::Update(float dt) {
    const Sext::NPCGeneric* npcAsset = npcBase->npcAsset;

    if (npcAsset->variantType == 0) {
        btClient->blackboard.Write(NPC_VAR_TURRET_VARIANT_ROF_ENABLED, 0);
        return eTaskState_Complete;
    }

    if (npcAsset->variantType == 2) {
        if (npcAsset->variantDataType == 1) {
            btClient->blackboard.Write(NPC_VAR_TURRET_VARIANT_ROF_ENABLED, 1);
            btClient->blackboard.Write(NPC_VAR_TURRET_SHOOT_CYCLE_COUNT,
                                       npcAsset->turret.ShootCycleCount);
            btClient->blackboard.Write(NPC_VAR_TURRET_SHOOT_CYCLE_DELAY,
                                       npcAsset->turret.ShootCycleDelay);
            btClient->blackboard.Write(NPC_VAR_TURRET_SHOOT_DELAY,
                                       npcAsset->turret.ShootDelay);
            return eTaskState_Complete;
        }

        btClient->blackboard.Write(NPC_VAR_TURRET_VARIANT_ROF_ENABLED, 0);
        return eTaskState_Complete;
    }

    btClient->blackboard.Write(NPC_VAR_TURRET_VARIANT_ROF_ENABLED, 0);
    return eTaskState_Complete;
}

// ---------------------------------------------------------------------------
// zNPCBTWriteSquidBlockTimeAction: the next block time of the boss squid's
// stage, from the stage's table, scaled by the multiplier unless blocking.

// NEAR MISS: 33 of 116 words (retail 476 B, ours 464 B); the three table
// initialisers, 0.0f and 1.0f are five .rodata references, and past the
// pool padding this compiler forms one addis base for four or more where
// retail spells a lis each: the four-literal wall (NOTES.md).
eTaskState zNPCBTWriteSquidBlockTimeAction::Update(float dt) {
    float stage1[12] = {1.0f, 1.75f, 0.5f, 1.25f, 1.0f, 1.75f,
                        1.0f, 1.75f, 0.5f, 1.25f, 1.0f, 1.75f};
    float stage2[14] = {0.35f, 0.35f, 0.35f, 0.35f, 1.0f, 1.75f, 1.0f,
                        1.75f, 0.2f,  0.2f,  1.0f,  1.75f, 1.0f, 1.75f};
    float stage3[12] = {0.75f, 1.5f,  0.25f, 0.25f, 0.75f, 1.5f,
                        0.25f, 0.25f, 0.25f, 0.25f, 0.75f, 1.5f};
    float* blockArray = 0;
    int maxSize = 0;

    switch (stageCounter->count) {
    case 1:
        blockArray = stage1;
        maxSize = 12;
        break;
    case 2:
        blockArray = stage2;
        maxSize = 14;
        break;
    case 3:
        blockArray = stage3;
        maxSize = 12;
        break;
    }

    float mult = 0.0f;

    btClient->blackboard.Read(multiplierFloatVariable, mult);

    if (mult <= 0.0f) {
        mult = 1.0f;
    }

    int idx = -1;

    btClient->blackboard.Read(indexIntegerVariable, idx);

    float timeValue = blockArray[idx];

    if (!block && mult != 1.0f) {
        timeValue *= mult;
    }

    btClient->blackboard.Write(timeFloatVariable, timeValue);

    idx++;

    if (idx >= maxSize) {
        idx = 0;
    }

    btClient->blackboard.Write(indexIntegerVariable, idx);

    return eTaskState_Complete;
}

void zNPCBTWriteSquidBlockTimeAction::Setup(
    const Sext::ActionBase* assetData) {
    const Sext::Action_NPC_WriteSquidBossBlockTime* asset =
        (const Sext::Action_NPC_WriteSquidBossBlockTime*)assetData;

    stageCounter = (xCounter*)zSceneFindObject(asset->StageCounterUID);
    indexIntegerVariable = asset->IndexVariable;
    timeFloatVariable = asset->TimeVariable;
    multiplierFloatVariable = asset->MultiplierVariable;
    block = asset->Block;
}

// ---------------------------------------------------------------------------
// zNPCBTChumbotFistFlashAction: the fist's colour runs from red to white
// over the flash.

void zNPCBTChumbotFistFlashAction::Setup(const Sext::ActionBase* a) {
    flashDuration = ((const Sext::Action_NPC_ChumbotFistFlash*)a)->Duration;
    flashTime = 0.0f;
}

eTaskState zNPCBTChumbotFistFlashAction::Update(float dt) {
    flashTime = xmin(flashTime + dt, flashDuration);

    float colorMult = flashTime / flashDuration;

    npcBase->npcEntity->model->SetColorMultiplier(1.0f, colorMult, colorMult,
                                                  -1.0f);

    return eTaskState_Running;
}

// ---------------------------------------------------------------------------
// zNPCBTFollowPerceptionTargetAction: move to where the asset's perception
// target (numbered from one) is, or stay put without one.

void zNPCBTFollowPerceptionTargetAction::Initialize() {
    endWhenArrived = true;
    arriveRangeMultiplier2 = 1.0f;
    moveToControl.headingCalcType = 4;
}

void zNPCBTFollowPerceptionTargetAction::Setup(const Sext::ActionBase* a) {
    const Sext::Action_NPC_FollowPerceptionTarget* asset =
        (const Sext::Action_NPC_FollowPerceptionTarget*)a;

    endWhenArrived = asset->EndActionWhenArrived;
    targetNum = asset->Target - 1;
}

void zNPCBTFollowPerceptionTargetAction::Begin() {
    const Sext::Action_NPC_FollowPerceptionTarget* asset =
        (const Sext::Action_NPC_FollowPerceptionTarget*)actionAsset;

    actionAnim.SetAnimation(asset->MovementData.MoveType,
                            asset->MovementData.AnimationName);
    BT_Utility::SetMovementLimits(
        npcBase, npcBase->npcSteering, &moveToControl,
        asset->MovementData.MoveType, asset->MovementData.MaxSpeed,
        asset->MovementData.MaxAcceleration, asset->MovementData.TurningRadius,
        asset->MovementData.TurnSpring);

    moveToControl.headingCalcType = asset->MovementData.Heading;

    actionAnim.StartOnNPC(npcBase->npcEntity, false);

    UpdateDestination();

    if (asset->MovementData.Arrive) {
        moveToControl.arriveToDest = 1;
    } else {
        moveToControl.arriveToDest = 0;
    }

    stuckRangeMultiplier.curMultiplier = 1.0f;
    npcBase->npcSteering->_s10(&moveToControl);
}

bool zNPCBTFollowPerceptionTargetAction::IsArrived() {
    float dist2 = npcBase->npcEntity->GetPos().Distance2XZ(
        *moveToControl.GetReachableDest());
    float boundXZ = npcBase->npcEntity->npcBound.GetBoundMinRadiusXZ();

    return dist2 < arriveRangeMultiplier2 * (boundXZ * boundXZ);
}

bool zNPCBTFollowPerceptionTargetAction::IsStuck() {
    float dist2 = npcBase->npcEntity->GetPos().Distance2XZ(
        *moveToControl.GetReachableDest());
    float boundXZ = npcBase->npcEntity->npcBound.GetBoundMinRadiusXZ();

    return dist2 < boundXZ * boundXZ * stuckRangeMultiplier.GetMultiplier2() ||
           stuckRangeMultiplier.IsMaxed();
}

void zNPCBTFollowPerceptionTargetAction::UpdateDestination() {
    xVec3 dest = npcBase->npcEntity->model->Mat.pos;

    zNPCPerception* perception = npcBase->npcPerception;

    if (perception != 0) {
        xEnt* target = perception->targets[targetNum].targetEnt;

        if (target != 0 && target->frame != 0) {
            dest = target->model->Mat.pos;
        }
    }

    SteeringControl()->destination.Set(dest);
}

eTaskState zNPCBTFollowPerceptionTargetAction::Update(float dt) {
    UpdateDestination();

    npcBase->npcSteering->_s14(dt);

    if (npcBase->npcEntity->IsAnimationStopped(actionAnim.animStateID)) {
        return eTaskState_Fail;
    }

    stuckRangeMultiplier.Update(npcBase->npcEntity->frame->vel, dt);

    if (endWhenArrived) {
        if (IsArrived()) {
            return eTaskState_Complete;
        }

        if (IsStuck()) {
            return eTaskState_Fail;
        }
    }

    return eTaskState_Running;
}

// ---------------------------------------------------------------------------
// zNPCBTWritePerceptionTargetPositionAction: the position of the asset's
// perception target (numbered from one).

void zNPCBTWritePerceptionTargetPositionAction::Setup(
    const Sext::ActionBase* a) {
    targetNum =
        ((const Sext::Action_NPC_Write_PerceptionTargetPosition*)a)->Target - 1;
}

bool zNPCBTWritePerceptionTargetPositionAction::FindPerceptionTargetPos(
    xVec3& pos) {
    zNPCPerception* perception = npcBase->npcPerception;

    if (perception != 0) {
        xEnt* target = perception->targets[targetNum].targetEnt;

        if (target != 0 && target->frame != 0) {
            pos = target->model->Mat.pos;
            return true;
        }
    }

    return false;
}

eTaskState zNPCFlyingBTWritePerceptionTargetPositionAction::Update(float dt) {
    xVec3 pos;

    if (!_v10(pos)) {
        return eTaskState_Fail;
    }

    pos.y = BT_Utility::CalcFlyingNPCsAdjustedY(
        &((const Sext::Action_NPC_FlyingWrite_PerceptionTargetPosition*)
              actionAsset)
             ->HeightAdjustment,
        npcBase, pos);

    return btClient->blackboard.Write(NPC_VAR_CURRENT_DESTINATION, pos)
               ? eTaskState_Complete
               : eTaskState_Fail;
}
