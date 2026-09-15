#include "SB/GM/Engine/Game/zNPCCommonMovementBTActions.pool.h"

// zNPCCommonMovementBTActions.cpp -- the behaviour-tree actions that move an
// NPC: move to a destination (with or without path finding), follow a
// player or a projectile, escort, flee, flutter, jump, follow a path of move
// points, stop, face, teleport, orbit, fly and snap to the floor. Read from
// the image with tools/disasm.py; the layouts are the DWARF's, and the
// strings and float literals come out of WAD02.cpp's pool, which the header
// in front reproduces.
//
// Every action keeps its asset, a resume callback and its client in front
// of the vtable pointer at +0xC, and the NPC it drives at +0x10. The move-to
// family adds two virtuals: slot 10 re-reads the destination and slot 11
// hands out the steering control. The NPC's steering is polymorphic from +4:
// slot 10 takes a control, slot 11 gives it back, slot 14 steps it and
// slot 15 re-reads the model's matrix. An Update returns 1 while running,
// 3 when done and 4 when it cannot run.
//
// Virtuals are declared under slot names (_vN, _sN, _pN) and never defined,
// so no vtable lands here; the functions this unit defines are declared
// non-virtual.

namespace Sext {

class ActionBase {};

enum eNPCMoveType {
    eNPCMoveType_Mosey = 0,
    eNPCMoveType_Custom = 7,
};

// The movement block of every moving action's asset. The speeds and the
// animation name are only read for eNPCMoveType_Custom.
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

class Action_NPC_MoveTo : public ActionBase {
public:
    NPC_Action_MovementData MovementData;
    bool UsePathFinding;
    bool EnableObjectAvoidance;
    bool CheckForWallNetEdges;
    bool ExitWhenAnimationIsDone;
    bool DontExitBeforeAnimationIsDone;
    bool FastMove;

    bool GetEnableObjectAvoidance() const {
        return EnableObjectAvoidance;
    }
};

class Action_NPC_Flee : public Action_NPC_MoveTo {
public:
    unsigned int PlayerVariable;
    bool UseFleePoint;
    bool UseFleeDirection;
};

class Action_NPC_Flutter : public Action_NPC_MoveTo {
public:
    float MinYOffset;
    float MaxYOffset;
};

class Action_NPC_Orbit : public Action_NPC_MoveTo {
public:
    unsigned int PlayerVariable;
};

// Not in the DWARF: FollowPlayer's Begin reads a player variable at +0x2C,
// the same place Flee and Orbit keep theirs.
class Action_NPC_FollowPlayer : public Action_NPC_MoveTo {
public:
    unsigned int PlayerVariable;
};

class Action_NPC_Jump : public ActionBase {
public:
    unsigned int DestinationVariable;
    float Height;
    float VelocityXZ;
    float MaxVelocityY;
    bool CheckDestination;
    bool UseLaunchVelXZ;
};

class Action_NPC_PathFollowMP : public ActionBase {
public:
    float PathWidth;
    unsigned int MovementVariable;
    NPC_Action_MovementData MovementData;
};

class Action_NPC_Stop : public ActionBase {
public:
    float Acceleration;
    int Heading;
};

class Action_NPC_FaceFromEvent : public ActionBase {
public:
    unsigned int Animation;
    float Acceleration;
    unsigned int FaceEventVar;
};

class Action_NPC_SnapToFloor : public ActionBase {
public:
    float HeightOffset;
};

// Event payloads the jump, path-follow, face and teleport actions read: an
// object's uid, after a move type for a path.
class EventActionUid {
public:
    unsigned long long uid;
};

class EventActionFollowPath {
public:
    eNPCMoveType MoveType;
    unsigned long long uid;
};

class EventAny;

}  // namespace Sext

class xBase;
class zBTClient;
class zNPCBase;
class zNPCEntity;
class zNPCSteering;
class zNPCSteeringControl;
class zNPCSteeringMoveToControl;

// ---------------------------------------------------------------------------
// Vectors and matrices. Assignment and equality are out of line (every
// `a = b` on a vector is a call); a copy made at a declaration is not.

class xVec3 {
public:
    xVec3& operator=(const xVec3& other);
    bool operator==(const xVec3& other) const;
    bool operator!=(const xVec3& other) const { return !(*this == other); }
    xVec3& operator*=(float s);

    float Distance2XZ(const xVec3& other) const;
    float length2() const;
    xVec3& Sub(const xVec3& a, const xVec3& b);
    xVec3& Add(const xVec3& a, const xVec3& b);
    xVec3& AddScale(const xVec3& a, const xVec3& b, float s);
    xVec3& AddScale(const xVec3& v, float s);
    float NormalizeSafe();
    xVec3& safe_normalize(const xVec3& fallback);

    static const xVec3 m_Null;
    static const xVec3 m_UnitAxisX;
    static const xVec3 m_UnitAxisY;

    float x;
    float y;
    float z;
};

xVec3 operator-(const xVec3& a, const xVec3& b);
xVec3 operator+(const xVec3& a, const xVec3& b);
xVec3 operator*(const xVec3& v, float s);
float xVec3Dist2(const xVec3* a, const xVec3* b);
void xVec3RotateOnAxis(xVec3& out, const xVec3& v, const xVec3& axis,
                       float angle);

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

void xMat3x3RMulRotY(xMat3x3* o, const xMat3x3* m, float t);

float xrand_GenRandFloat();
float xrand_RandomFloatRange(float lo, float hi);
int xrand_RandomRange(int lo, int hi);

namespace Math {

float rsqrt(float x);

class Vector4 {
public:
    Vector4& Assign(float x, float y, float z, float w);

    float x;
    float y;
    float z;
    float w;
};

}  // namespace Math

// An empty function the linker folded onto Math::Matrix33's constructor:
// the image names every call to it `__ct__Q24Math8Matrix33Fv`, so that is
// the name called.
extern "C" void __ct__Q24Math8Matrix33Fv(void* self);

// ---------------------------------------------------------------------------
// The engine's objects as the actions reach them: xBase's flags, an
// entity's model at +0x34, a move point's position at +0x40.

class xOGModel {
public:
    xMat4x3 Mat;
};

class xBase {
public:
    unsigned char _pad0[0x26];
    unsigned short baseFlags;
    unsigned char _pad1[0x34 - 0x28];
    xOGModel* model;
};

class xMovePoint : public xBase {
public:
    int NetworkGetNumberOfMPs();
    int NetworkFillMovepoints(xMovePoint** out, int max);
    void NetworkGetNextOnPath_Null(const xMovePoint* dest, xMovePoint*& next);
    xVec3 GetPosShifted(const xVec3* prevPos, const xMovePoint* prevMP,
                        const xVec3* nextPos, const xMovePoint* nextMP,
                        float shift) const;

    unsigned char _pad2[0x40 - 0x38];
    xVec3* pos;
};

xBase* zSceneFindObject(unsigned long long id);

enum eWallNetUserType {
    eWallNetUserType_Player = 0,
    eWallNetUserType_NPC = 1,
};

class zWallNetEdge {
public:
    unsigned char srcVertex;
    unsigned char dstVertex;
    unsigned short flags;
    float lengthXZ;
};

class zWallNetTriangle {
public:
    unsigned short edges[3];
    unsigned short flags;
};

class zWallNetAsset {
public:
    unsigned char _pad0[0x20];
    int numBoundEdges;
    int numTriangles;
    unsigned char _pad1[0x38 - 0x28];
    xVec3* vertices;
    zWallNetEdge* edges;
    zWallNetTriangle* triangles;
};

class zWallNet {
public:
    unsigned char _pad0[0x3C];
    zWallNetAsset* wallNetAsset;
};

class zIWallNet {
public:
    static zWallNet* FindWallNet(const xVec3& pos, eWallNetUserType type,
                                 xBase* user, int* triangleID, bool clamp);
};

// zPlayer is polymorphic from +0; slot 105 says whether the computer drives
// it.
class zPlayerVirtuals {
public:
    virtual void _p0(); virtual void _p1(); virtual void _p2();
    virtual void _p3(); virtual void _p4(); virtual void _p5();
    virtual void _p6(); virtual void _p7(); virtual void _p8();
    virtual void _p9(); virtual void _p10(); virtual void _p11();
    virtual void _p12(); virtual void _p13(); virtual void _p14();
    virtual void _p15(); virtual void _p16(); virtual void _p17();
    virtual void _p18(); virtual void _p19(); virtual void _p20();
    virtual void _p21(); virtual void _p22(); virtual void _p23();
    virtual void _p24(); virtual void _p25(); virtual void _p26();
    virtual void _p27(); virtual void _p28(); virtual void _p29();
    virtual void _p30(); virtual void _p31(); virtual void _p32();
    virtual void _p33(); virtual void _p34(); virtual void _p35();
    virtual void _p36(); virtual void _p37(); virtual void _p38();
    virtual void _p39(); virtual void _p40(); virtual void _p41();
    virtual void _p42(); virtual void _p43(); virtual void _p44();
    virtual void _p45(); virtual void _p46(); virtual void _p47();
    virtual void _p48(); virtual void _p49(); virtual void _p50();
    virtual void _p51(); virtual void _p52(); virtual void _p53();
    virtual void _p54(); virtual void _p55(); virtual void _p56();
    virtual void _p57(); virtual void _p58(); virtual void _p59();
    virtual void _p60(); virtual void _p61(); virtual void _p62();
    virtual void _p63(); virtual void _p64(); virtual void _p65();
    virtual void _p66(); virtual void _p67(); virtual void _p68();
    virtual void _p69(); virtual void _p70(); virtual void _p71();
    virtual void _p72(); virtual void _p73(); virtual void _p74();
    virtual void _p75(); virtual void _p76(); virtual void _p77();
    virtual void _p78(); virtual void _p79(); virtual void _p80();
    virtual void _p81(); virtual void _p82(); virtual void _p83();
    virtual void _p84(); virtual void _p85(); virtual void _p86();
    virtual void _p87(); virtual void _p88(); virtual void _p89();
    virtual void _p90(); virtual void _p91(); virtual void _p92();
    virtual void _p93(); virtual void _p94(); virtual void _p95();
    virtual void _p96(); virtual void _p97(); virtual void _p98();
    virtual void _p99(); virtual void _p100(); virtual void _p101();
    virtual void _p102(); virtual void _p103(); virtual void _p104();
    virtual bool IsAI() const;
};

class zPlayer : public zPlayerVirtuals {
public:
    unsigned char _pad0[0x34 - 0x4];
    xOGModel* model;
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

class zProjectile {
public:
    const xVec3& GetPos() const;
};

class zProjectileManager {
public:
    static zProjectile* sFindProjectileFromID(unsigned int id);
};

// ---------------------------------------------------------------------------
// The NPC

class zNPCBound {
public:
    float GetBoundRadiusXZ() const;
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
    bool DoesAnimExist(unsigned int animID);
    xVec3& GetPos() { return model->Mat.pos; }
    float GetBoundHeight() { return npcBound.extent.y; }
    bool DoesAnimExist(const char* name);
    bool IsAnimationStopped(unsigned int animID);
    void KillVelocity();
    void UpdateWithTeleport(float dt);

    unsigned char _pad0[0x34];
    xOGModel* model;
    unsigned char _pad1[0x58 - 0x38];
    xEntFrame* frame;
    unsigned char _pad2[0xF4 - 0x5C];
    zNPCBound npcBound;
    unsigned char _pad3[0x1AF - 0x124];
    bool floorCollision;
};

class MoveData {
public:
    float maxSpeed;
    float maxAcceleration;
    float turningRadius;
    float turnSpring;
};

class NPCTemplate {
public:
    unsigned char _pad0[0x38];
    unsigned int flags;
};

class zNPCTemplate {
public:
    NPCTemplate* templateAsset;
    MoveData moveData[8];
};

class zNPCBase {
public:
    void GetPosition(xVec3& pos);
    zNPCSteering* GetSteering() { return npcSteering; }
    bool IsFlying() { return flying; }

    unsigned char _pad0[0x71];
    bool infoNodesUpdating : 1;
    bool quietKill : 1;
    bool updating : 1;
    bool flying : 1;
    bool collectible : 1;
    unsigned char _pad1[0x78 - 0x72];
    zNPCTemplate* npcTemplate;
    void* btClient;
    xMovePoint* npcMovePoint;
    xMovePoint* npcMovePointNetwork;
    unsigned char _pad2[0x98 - 0x88];
    zNPCEntity* npcEntity;
    void* npcSteeringOld;
    zNPCSteering* npcSteering;
};

// The steering component: the owner, then the vtable pointer at +4.
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
    void SetCheckForWallNetEdges(bool check);

    virtual void _s10(zNPCSteeringControl* control);
    virtual void _s11(zNPCSteeringControl* control);
    virtual void _s12();
    virtual void _s13();
    virtual void _s14(float dt);
    virtual void _s15();

    unsigned char _pad0[0x34 - 0x8];
    zWallNet* wallNet;
    float turnSpringK;
};

// ---------------------------------------------------------------------------
// Steering controls

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
    void Set(float x, float y, float z);

    xVec3 limitV;
    int isSphere;
};

class zNPCSteeringControl {
public:
    void SetMaxAcc(float acc);
    void SetMaxAcc(const xVec3& acc);
    void SetCustomHeading(zNPCSteeringDest& dest);
    void SetIgnoreYComponent(bool ignore) { ignoreYComponent = ignore; }
    void SetSpeedLimitXZ(float limit) { speedLimitXZ = limit; }
    void SetSpeedLimitY(float limit) { speedLimitY = limit; }
    // Inline where retail inlines it (Escort's Begin); the out-of-line
    // SetMaxAcc(float) is the one other callers reach.
    void SetMaxAccLimits(float acc) {
        maxAcc = acc;
        accLimiter.Set(acc, acc, acc);
    }

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

class zNPCSteeringStopControl : public zNPCSteeringControl {};

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
    void SetArriveToDest(int arrive) { arriveToDest = arrive; }
    void SetWanderIgnoreY(bool ignore) { wanderData.ignoreY = ignore; }
    void SetObjectAvoidance(bool enable) {
        objectAvoidanceData.enabled = enable;
    }
    void SetArriveTolerance(float tol) { arriveTolerance2 = tol * tol; }

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

class zNPCSteeringEscortControl : public zNPCSteeringMoveToControl {
public:
    unsigned char _pad0[0x248 - 0x20C];
};

class zNPCSteeringJumpControl : public zNPCSteeringControl {
public:
    void SetDestination(const xVec3* dest);

    float apexHeight;
    float launchVelXZ;
    float maxLaunchVelY;
    zNPCSteeringDest destination;
    zNPCSteeringAccumulator accAccum;
    bool useHeight;
    bool jumped;
};

class zSteeringPath;

class zNPCSteeringFollowPathControl : public zNPCSteeringControl {
public:
    zWanderData wanderData;
    zNPCSteeringAccumulator accAccum;
    zSteeringPath* path;
};

// A path keeps two floats in front of its vtable pointer at +8; slot 4 says
// whether the end has been reached.
class zSteeringPathData {
public:
    float halfWidth;
    float halfWidthSqr;
};

class zSteeringPath : public zSteeringPathData {
public:
    void SetHalfWidth(float hw);

    virtual void _p0();
    virtual void _p1();
    virtual void _p2();
    virtual void _p3();
    virtual bool _p4() const;
};

class zSteeringPathMovePoints : public zSteeringPath {
public:
    void UpdateToNextMP(const xVec3& pos);
    void SetCurrMP(xMovePoint* mp, const xVec3& pos) {
        currMP = mp;
        UpdateToNextMP(pos);
    }
    bool IsPathEnd() const;

    xMovePoint* currMP;
    xMovePoint* prevMP;
    unsigned char linePath[0x30];
    unsigned char splinePath[0x10];
    zSteeringPath* currPath;
    float arriveTol2;
};

// ---------------------------------------------------------------------------
// Path finding

enum eNavLinkActionCode {
    eNavLinkActionCode_None = 0,
    eNavLinkActionCode_TraverseWallnet = 1,
    eNavLinkActionCode_Jump = 2,
    eNavLinkActionCode_Swing = 3,
};

enum ePathEvalResult {
    ePathEvalResult_Ok = 0,
    ePathEvalResult_Searching = 1,
    ePathEvalResult_NoPath_OutsideOfWallNet = 2,
    ePathEvalResult_NoPath_NoConnection = 3,
    ePathEvalResult_NoPath_InvalidSetup = 4,
    ePathEvalResult_Pause = 5,
    ePathEvalResult_PathEnd = 6,
};

class zPathFinderSearchMapLinkCostCalculator {
public:
    unsigned char _pad0[0x10];
};

class zNPCSearchMapLinkCostCalculator
    : public zPathFinderSearchMapLinkCostCalculator {
public:
    zNPCBase* npcBase;
};

class zPathFinder {
public:
    void Setup(xBase* owner);
    void SetCostCalculator(zPathFinderSearchMapLinkCostCalculator* calc);
    void CancelSearch();
    void Cleanup();
    ePathEvalResult EvaluatePath(const xVec3* pos, xVec3* dest, float radius,
                                 eNavLinkActionCode* code);
    void FindPath(const xVec3* from, const xVec3* to, float radius,
                  eWallNetUserType type);

    xBase* owner;
    unsigned char _pad0[0x2A8 - 0x4];
};

// ---------------------------------------------------------------------------
// The blackboard, as zBlackboard.cpp spells it. Every Read and Write is
// that unit's.

namespace Util {

template <class T>
class DelegateP0 {
public:
    template <class C, T (C::*M)()>
    static T InvokeMember(void* obj) {
        return (((C*)obj)->*M)();
    }

    void* object;
    T (*func)(void*);
};

}  // namespace Util

class zVariableEventData {
public:
    zVariableEventData() : eventID(0), dataPointer(0) {}

    unsigned int GetEventID() const { return eventID; }

    unsigned int eventID;
    Sext::EventAny* dataPointer;
    char eventDataBuffer[64];
};

class zBlackboard {
public:
    void RegisterVariableObserver(unsigned int id, Util::DelegateP0<void>* obs);
    void UnregisterVariableObserver(unsigned int id,
                                    Util::DelegateP0<void>* obs);

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

// The variable ids are set once, at start-up, and never written again; read
// as const, their loads may move ahead of the stores in front of them, the
// way retail's do (UpdateDestination loads the id before saving r31).
extern const unsigned int NPC_VAR_CURRENT_PLAYER;
extern const unsigned int NPC_VAR_CURRENT_DESTINATION;
extern const unsigned int NPC_VAR_TARGET_MP;
extern const unsigned int NPC_VAR_FLEE_POS;
extern const unsigned int NPC_VAR_FLEE_DIR;
extern const unsigned int NPC_VAR_CURRENT_PROJECTILE_ID;
extern const unsigned int NPC_VAR_PATH_SHIFT_PERCENT;

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
    void Init(unsigned int id, float blend, float start);
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
    virtual void _v10();
    virtual zNPCSteeringMoveToControl* _v11();
};

class zNPCBTAction : public zBTAction {
public:
    zNPCBase* GetNPCBase() { return npcBase; }

    zNPCBase* npcBase;
};

namespace BT_Utility {

void SetMovementLimits(const zNPCBase* npcBase, zNPCSteering* steeringComponent,
                       zNPCSteeringControl* control, Sext::eNPCMoveType moveType,
                       float customMaxSpeed, float customMaxAcc,
                       float customTurningRadius, float customTurnSpring);

}  // namespace BT_Utility

class zNPCBTMoveToAction : public zNPCBTAction {
public:
    void Initialize();
    void Setup(const Sext::ActionBase* a);
    void InitPathFinder();
    void SetMoveToDestination(const xVec3& dest);
    zNPCSteeringMoveToControl* SteeringControl();
    void InitMovement();
    void Begin();
    void SetDestination(const xVec3& dest);
    bool IsArrived();
    bool IsStuck();
    bool EvaluatePath();
    void UpdateFinalDestination(const xVec3& newDest);
    void UpdateDestination();
    void UpdateMoveTo(float dt);
    void UpdatePathFinding(float dt);
    eTaskState Update(float dt);
    void End();
    void Cleanup();

    const Sext::Action_NPC_MoveTo* asset;
    zNPCSteeringMoveToControl moveToControl;
    zNPCBTActionAnim actionAnim;
    zNPCBTStuckRangeMultiplier stuckRangeMultiplier;
    bool endWhenArrived;
    zPathFinder pathFinder;
    xVec3 currentDest;
    xVec3 finalDest;
    float updateDestTimer;
    float updatePathTimer;
    eNavLinkActionCode actionCode;
    ePathEvalResult pathResult;
    bool pathFinderSetupDone;
    zNPCSearchMapLinkCostCalculator costCalculator;
};

class zNPCBTPathThruMPsShiftedAction : public zNPCBTMoveToAction {
public:
    bool IsWalkable(const xVec3* start, const xVec3* end);
    xMovePoint* NetworkGetClosestWalkable(xMovePoint* network,
                                          const xVec3* pos);
    bool InitializeMPs();
    void Begin();
    eTaskState Update(float dt);
    void End();

    xVec3 pathThruDest;
    xMovePoint* startMP;
    xMovePoint* endMP;
    xMovePoint* currentMP;
    xMovePoint* nextMP;
};

class zNPCBTJumpAction : public zNPCBTAction {
public:
    void Initialize();
    void Begin();
    void SwitchAnim(const char* name);
    eTaskState Update(float dt);

    zNPCSteeringJumpControl jumpControl;
    zNPCBTActionAnim actionAnim;
    unsigned int jumpState;
    bool useJumpStartAnim;
    bool useJumpEndAnim;
    const char* jumpAnimName;
    const char* jumpStartAnimName;
    const char* jumpEndAnimName;
};

class zNPCBTFollowPlayerAction : public zNPCBTMoveToAction {
public:
    void Begin();
    void End();
    void Cleanup();
    zPlayer* GetPlayer();
    void UpdateDestination();
    void UpdatePlayer();

    Util::DelegateP0<void> observer;
    unsigned int playerVariable;
    zPlayer* player;
};

class zNPCBTEscortAction : public zNPCBTMoveToAction {
public:
    void UpdateDestination();
    void Begin();
    zNPCSteeringEscortControl* SteeringControl();
    eTaskState Update(float dt);

    zNPCSteeringEscortControl escortControl;
};

class zNPCBTFleeAction : public zNPCBTMoveToAction {
public:
    void Begin();
    void UpdateDestination();

    zPlayer* player;
};

class zNPCBTFlutterAction : public zNPCBTMoveToAction {
public:
    void Begin();
    void UpdateDestination();
    eTaskState Update(float dt);
};

class zNPCBTFollowProjectileAction : public zNPCBTMoveToAction {
public:
    void Begin();
    void UpdateDestination();
    eTaskState Update(float dt);

    zProjectile* projectile;
};

class zNPCBTPathFollowMPAction : public zNPCBTAction {
public:
    void Begin();
    bool ArrivedAtMP();
    bool IsStuck();
    eTaskState Update(float dt);
    void End();

    zNPCSteeringFollowPathControl followControl;
    zNPCBTActionAnim actionAnim;
    zNPCBTStuckRangeMultiplier stuckRangeMultiplier;
    zSteeringPathMovePoints path;
    float pathHalfWidth;
    float maxSpeed;
    float arriveTol2;
    Sext::eNPCMoveType moveType;
    int headingCalcType;
    bool arriveToDest;
    float customMaxSpeed;
    float customMaxAcc;
    const char* customAnimName;
};

class zNPCBTStopAction : public zNPCBTAction {
public:
    void Setup(const Sext::ActionBase* a);
    void Begin();
    eTaskState Update(float dt);

    zNPCSteeringStopControl stopControl;
    int headingCalcType;
    float acceleration;
};

class zNPCBTFaceFromEventAction : public zNPCBTAction {
public:
    void Setup(const Sext::ActionBase* a);
    void Begin();
    void End();

    unsigned int animID;
    unsigned int eventVarID;
    zNPCBTActionAnim actionAnim;
    bool valid;
    zNPCSteeringDest target;
    zNPCSteeringStopControl stopControl;
    float acceleration;
};

class zNPCBTTeleportAction : public zNPCBTAction {
public:
    void Setup(const Sext::ActionBase* a);
    eTaskState Update(float dt);

    xMat4x3 destMat;
    unsigned int destVar;
};

class zNPCBTOrbitAction : public zNPCBTMoveToAction {
public:
    void Begin();
    void UpdateOrbitDirection(float dt);
    void UpdateOrbitDestination(float dt);
    eTaskState Update(float dt);

    zNPCBTActionAnim actionAnim;
    zPlayer* player;
    bool orbitLeft;
    float orbitTimer;
    float orbitExpireTime;
    float orbitRadius;
    unsigned int playerVarID;
};

class zNPCBTSetFlyingAction : public zNPCBTAction {
public:
    eTaskState Update(float dt);
};

class zNPCBTSnapToFloorAction : public zNPCBTAction {
public:
    eTaskState Update(float dt);

    xMat4x3 destMat;
};

// ---------------------------------------------------------------------------
// zNPCBTPathThruMPsShiftedAction: move to the destination through the move
// point network, walking each point shifted sideways into its own lane.

#define xmin(a, b) ((a) < (b) ? (a) : (b))

// Whether segments a0-a1 and b0-b1 cross in XZ. Inline in retail: all of it
// sits on one source line of IsWalkable.
inline bool SegmentsIntersectXZ(const xVec3& a0, const xVec3& a1,
                                const xVec3& b0, const xVec3& b1) {
    float dx = a1.x - a0.x;
    float dz = a1.z - a0.z;
    float ex = b1.x - b0.x;
    float ez = b1.z - b0.z;
    float denom = dx * ez - dz * ex;

    if (denom == 0.0f) {
        return false;
    }

    float sx = a0.x - b0.x;
    float sz = a0.z - b0.z;
    float t = (sz * dx - sx * dz) / denom;

    if (t >= 0.0f && t <= 1.0f) {
        float u = (sz * ex - sx * ez) / denom;

        if (u >= 0.0f && u <= 1.0f) {
            return true;
        }
    }

    return false;
}

// Walkable: both ends on the same wallnet, and the segment crosses none of
// its boundary edges.
// NEAR MISS: 71 of 113 words. The intersection test must be in line:
// always_inline below, or the helper comes out as an EXTRA copy. What
// is left: retail re-reads startWN->wallNetAsset at the top of every
// iteration where ours hoists it out of the loop, and the float
// temporaries land in other registers. An inline zWallNet member that
// reads its own asset, nested or flat, gives the same 71 words.
#pragma push
#pragma always_inline on
bool zNPCBTPathThruMPsShiftedAction::IsWalkable(const xVec3* start,
                                                const xVec3* end) {
    if (start == 0 || end == 0 || npcBase == 0) {
        return false;
    }

    zWallNet* startWN = zIWallNet::FindWallNet(*start, eWallNetUserType_NPC,
                                               (xBase*)npcBase, 0, true);
    zWallNet* endWN = zIWallNet::FindWallNet(*end, eWallNetUserType_NPC,
                                             (xBase*)npcBase, 0, true);

    if (startWN == 0 || endWN == 0) {
        return false;
    }

    if (startWN != endWN) {
        return false;
    }

    bool intersect = false;

    for (int j = 0; j < startWN->wallNetAsset->numBoundEdges; j++) {
        if (SegmentsIntersectXZ(
                *start, *end,
                startWN->wallNetAsset
                    ->vertices[startWN->wallNetAsset->edges[j].srcVertex],
                startWN->wallNetAsset
                    ->vertices[startWN->wallNetAsset->edges[j].dstVertex])) {
            intersect = true;
            break;
        }
    }

    return !intersect;
}
#pragma pop

xMovePoint* zNPCBTPathThruMPsShiftedAction::NetworkGetClosestWalkable(
    xMovePoint* network, const xVec3* pos) {
    xMovePoint* closestWalkable = 0;

    if (network != 0 && pos != 0 && npcBase != 0) {
        int numberOfMPs = network->NetworkGetNumberOfMPs();

        if (numberOfMPs < 200) {
            xMovePoint* movePointsArray[200];
            int movePointsLength =
                network->NetworkFillMovepoints(movePointsArray, 200);
            float closestDist2 = 3.4028235e+38f;

            for (int i = 0; i < movePointsLength; i++) {
                if (IsWalkable(pos, movePointsArray[i]->pos)) {
                    float dist2 = pos->Distance2XZ(*movePointsArray[i]->pos);

                    if (dist2 < closestDist2) {
                        closestDist2 = dist2;
                        closestWalkable = movePointsArray[i];
                    }
                }
            }
        }
    }

    return closestWalkable;
}

void zNPCBTPathThruMPsShiftedAction::Begin() {
    InitializeMPs();
    zNPCBTMoveToAction::Begin();
}

void zNPCBTPathThruMPsShiftedAction::End() { zNPCBTMoveToAction::End(); }

// On arriving at a move point, step to the next one on the path to the end
// point -- skipping the last when the destination is already walkable past
// it -- and aim at its lane-shifted position.
eTaskState zNPCBTPathThruMPsShiftedAction::Update(float dt) {
    eTaskState result = zNPCBTMoveToAction::Update(dt);

    if (result == eTaskState_Complete) {
        if (currentMP != 0) {
            if (nextMP == endMP) {
                xVec3 currToNext = *nextMP->pos - *currentMP->pos;
                xVec3 NextToDest = pathThruDest - *nextMP->pos;
                float dotXZ = currToNext.x * NextToDest.x +
                              currToNext.z * NextToDest.z;

                if (dotXZ < 0.0f) {
                    if (IsWalkable(currentMP->pos, &pathThruDest)) {
                        nextMP = 0;
                    }
                }
            }

            xMovePoint* prevMP = currentMP;

            currentMP = nextMP;

            if (currentMP != 0) {
                currentMP->NetworkGetNextOnPath_Null(endMP, nextMP);

                float shiftPercent = 0.0f;

                btClient->blackboard.Read(NPC_VAR_PATH_SHIFT_PERCENT,
                                          shiftPercent);

                xVec3 npcPos;

                npcBase->GetPosition(npcPos);

                xVec3 mpPos = currentMP->GetPosShifted(
                    &npcPos, prevMP, &pathThruDest, nextMP, shiftPercent);

                btClient->blackboard.Write(NPC_VAR_CURRENT_DESTINATION, mpPos);
            } else {
                btClient->blackboard.Write(NPC_VAR_CURRENT_DESTINATION,
                                           pathThruDest);
            }

            if (asset->UsePathFinding) {
                pathFinder.CancelSearch();
            }

            updateDestTimer = 0.0f;
            updatePathTimer = 0.0f;
            _v10();

            return eTaskState_Running;
        }

        return eTaskState_Complete;
    }

    return result;
}

// ---------------------------------------------------------------------------
// zNPCBTMoveToAction

void zNPCBTMoveToAction::Initialize() { endWhenArrived = true; }

void zNPCBTMoveToAction::Setup(const Sext::ActionBase* a) {
    asset = (const Sext::Action_NPC_MoveTo*)a;
    pathFinderSetupDone = false;
}

void zNPCBTMoveToAction::InitPathFinder() {
    if (asset->UsePathFinding) {
        pathFinder.Setup((xBase*)npcBase);
        costCalculator.npcBase = npcBase;
        pathFinder.SetCostCalculator(&costCalculator);
        actionCode = eNavLinkActionCode_None;
        updateDestTimer = 0.0f;
        updatePathTimer = 0.0f;
        SetMoveToDestination(npcBase->npcEntity->model->Mat.pos);
        pathFinderSetupDone = true;
    }
}

// Weak in retail (the class's header) and called from InitPathFinder above,
// so it is defined below it; SetDestination and UpdateMoveTo, further down,
// take it in line.
inline void zNPCBTMoveToAction::SetMoveToDestination(const xVec3& dest) {
    _v11()->destination.Set(dest);
}

zNPCSteeringMoveToControl* zNPCBTMoveToAction::SteeringControl() {
    return &moveToControl;
}

// NEAR MISS: 8 of 98 words, all in the arrive-tolerance store: retail
// loads the tolerance before SteeringControl() and squares it after
// the call, ours squares it first. A tolerance local, the direct
// product and an inline setter (left out of line: an EXTRA copy) were
// measured. The stored asset member, not a local, and a local for the
// control in the last store are what brought it from 81 words to 8.
void zNPCBTMoveToAction::InitMovement() {
    asset = (const Sext::Action_NPC_MoveTo*)actionAsset;

    BT_Utility::SetMovementLimits(
        GetNPCBase(), npcBase->GetSteering(), _v11(),
        asset->MovementData.MoveType, asset->MovementData.MaxSpeed,
        asset->MovementData.MaxAcceleration, asset->MovementData.TurningRadius,
        asset->MovementData.TurnSpring);

    _v11()->headingCalcType = asset->MovementData.Heading;
    _v11()->arriveTolerance2 = asset->MovementData.ArriveTolerance *
                               asset->MovementData.ArriveTolerance;

    if (asset->MovementData.Arrive) {
        _v11()->SetArriveToDest(1);
    } else {
        _v11()->SetArriveToDest(0);
    }

    stuckRangeMultiplier.curMultiplier = 1.0f;

    bool checkForWallNetEdges = asset->CheckForWallNetEdges;

    _v11()->wallAvoidanceData.enabled = checkForWallNetEdges;
    _v11()->useWallNet = checkForWallNetEdges;
    npcBase->npcSteering->SetCheckForWallNetEdges(checkForWallNetEdges);

    zNPCSteeringMoveToControl* control = _v11();

    control->objectAvoidanceData.enabled = asset->EnableObjectAvoidance;
}

// The speeds come from the asset for a custom move type and from the NPC's
// template otherwise; -1 means "leave it". With a turning radius, the
// acceleration across the heading is capped at v^2 / r.
// NEAR MISS: 4 of 82 words. The template pointer is in r3 and the scaled
// index in r0 where retail has them the other way round; a local, no
// local and a MoveData reference all give the same bytes.
void BT_Utility::SetMovementLimits(const zNPCBase* npcBase,
                                   zNPCSteering* steeringComponent,
                                   zNPCSteeringControl* control,
                                   Sext::eNPCMoveType moveType,
                                   float customMaxSpeed, float customMaxAcc,
                                   float customTurningRadius,
                                   float customTurnSpring) {
    if (control == 0) {
        return;
    }

    float maxSpeed = -1.0f;
    float maxAcc = -1.0f;
    float turningRadius = -1.0f;
    float turnSpring = -1.0f;

    if (moveType == Sext::eNPCMoveType_Custom) {
        maxSpeed = customMaxSpeed;
        maxAcc = customMaxAcc;
        turningRadius = customTurningRadius;
        turnSpring = customTurnSpring;
    } else {
        zNPCTemplate* npcTemplate = npcBase->npcTemplate;

        if (npcTemplate != 0) {
            maxSpeed = npcTemplate->moveData[moveType].maxSpeed;
            maxAcc = npcTemplate->moveData[moveType].maxAcceleration;
            turningRadius = npcTemplate->moveData[moveType].turningRadius;
            turnSpring = npcTemplate->moveData[moveType].turnSpring;
        }
    }

    if (maxSpeed > 0.0f) {
        control->speedLimitXZ = maxSpeed;
    }

    if (maxAcc > 0.0f) {
        if (turningRadius > 0.0f) {
            float turningAcceleration = control->speedLimitXZ;

            turningAcceleration =
                turningAcceleration * turningAcceleration / turningRadius;

            xVec3 limits;

            limits.x = turningAcceleration;
            limits.y = maxAcc;
            limits.z = maxAcc;

            control->SetMaxAcc(limits);
        } else {
            control->maxAcc = maxAcc;
            control->accLimiter.Set(maxAcc, maxAcc, maxAcc);
        }
    } else if (turningRadius > 0.0f) {
        float turningAcceleration = control->speedLimitXZ;

        maxAcc = control->maxAcc;
        turningAcceleration =
            turningAcceleration * turningAcceleration / turningRadius;

        xVec3 limits;

        limits.x = turningAcceleration;
        limits.y = maxAcc;
        limits.z = maxAcc;

        control->SetMaxAcc(limits);
    }

    if (turnSpring != -1.0f) {
        steeringComponent->turnSpringK = turnSpring;
    }
}

void zNPCBTMoveToAction::Begin() {
    actionAnim.SetAnimation(asset->MovementData.MoveType,
                            asset->MovementData.AnimationName);
    InitMovement();
    InitPathFinder();
    _v10();

    if (npcBase->flying) {
        _v11()->SetIgnoreYComponent(false);
        _v11()->SetWanderIgnoreY(false);
    }

    actionAnim.StartOnNPC(npcBase->npcEntity, false);
    npcBase->GetSteering()->_s10(_v11());
}

// Without path finding the destination goes straight to the steering; with
// it, it is where the path finder is sent.
void zNPCBTMoveToAction::SetDestination(const xVec3& dest) {
    if (!asset->UsePathFinding) {
        SetMoveToDestination(dest);
    } else {
        UpdateFinalDestination(dest);
    }
}

// Arrived: at the final destination when path finding, and within the NPC's
// radius plus the tolerance of the reachable destination -- in 3D for a
// flier (its radius capped by its half height), in XZ otherwise.
// NEAR MISS: 72 of 98 words. Ours keeps npcBase from the flying test in
// a saved register across the calls where retail reloads it, and takes
// the else branch's position after the calls where retail takes it
// first. Inline position and bound-height accessors, a local for the
// reachable destination, and GetNPCBase() were measured; none matches.
bool zNPCBTMoveToAction::IsArrived() {
    bool isFinalDest = true;

    if (asset->UsePathFinding) {
        zNPCSteeringMoveToControl* control = _v11();

        isFinalDest = finalDest == *control->destination.pDest;
    }

    if (!isFinalDest) {
        return false;
    }

    if (npcBase->flying) {
        xVec3 diff;

        diff.Sub(*_v11()->GetReachableDest(),
                 npcBase->npcEntity->model->Mat.pos);

        float dist2 = diff.length2();
        float bound = xmin(npcBase->npcEntity->npcBound.GetBoundMinRadiusXZ(),
                           npcBase->npcEntity->npcBound.extent.y);
        float arriveTolerance = asset->MovementData.ArriveTolerance;

        if (dist2 < bound * bound + arriveTolerance * arriveTolerance) {
            return true;
        }
    } else {
        float dist2 = npcBase->npcEntity->model->Mat.pos.Distance2XZ(
            *_v11()->GetReachableDest());
        float boundXZ = npcBase->npcEntity->npcBound.GetBoundMinRadiusXZ();
        float arriveTolerance = asset->MovementData.ArriveTolerance;

        if (dist2 < boundXZ * boundXZ + arriveTolerance * arriveTolerance) {
            return true;
        }
    }

    return false;
}

// Stuck: still near the destination's radius scaled by the stuck-range
// multiplier, or the multiplier has run out.
// NEAR MISS: 99 of 102 words. Retail reuses the loaded multiplier for the
// equality test but materialises it (mfcr) as a bool; ours does one or
// the other: an inline IsMaxed() gives the mfcr and loses the reuse
// (85 of 106), the plain comparison keeps the reuse and branches.
bool zNPCBTMoveToAction::IsStuck() {
    xVec3 dest = *_v11()->GetReachableDest();

    if (asset->UsePathFinding && *_v11()->destination.pDest != finalDest) {
        dest = finalDest;
    }

    if (npcBase->flying) {
        xVec3 diff;

        diff.Sub(dest, npcBase->npcEntity->model->Mat.pos);

        float dist2 = diff.length2();
        float bound = xmin(npcBase->npcEntity->npcBound.GetBoundMinRadiusXZ(),
                           npcBase->npcEntity->npcBound.extent.y);

        return dist2 < bound * bound * (stuckRangeMultiplier.curMultiplier *
                                        stuckRangeMultiplier.curMultiplier) ||
               stuckRangeMultiplier.curMultiplier ==
                   stuckRangeMultiplier.maxMultiplier;
    } else {
        float dist2 = npcBase->npcEntity->model->Mat.pos.Distance2XZ(dest);
        float boundXZ = npcBase->npcEntity->npcBound.GetBoundMinRadiusXZ();

        return dist2 < boundXZ * boundXZ *
                           (stuckRangeMultiplier.curMultiplier *
                            stuckRangeMultiplier.curMultiplier) ||
               stuckRangeMultiplier.curMultiplier ==
                   stuckRangeMultiplier.maxMultiplier;
    }
}

bool zNPCBTMoveToAction::EvaluatePath() {
    eNavLinkActionCode newCode;
    float npcRadius = npcBase->npcEntity->npcBound.GetBoundRadiusXZ();

    pathResult = pathFinder.EvaluatePath(&npcBase->npcEntity->model->Mat.pos,
                                         &currentDest, npcRadius, &newCode);

    switch (pathResult) {
    case ePathEvalResult_Searching:
        return false;
    case ePathEvalResult_PathEnd:
        currentDest = finalDest;
        actionCode = eNavLinkActionCode_None;
        break;
    case ePathEvalResult_NoPath_OutsideOfWallNet:
    case ePathEvalResult_NoPath_NoConnection:
    case ePathEvalResult_NoPath_InvalidSetup:
        currentDest = finalDest;
        actionCode = eNavLinkActionCode_None;
        break;
    default:
        actionCode = newCode;
        break;
    }

    return true;
}

void zNPCBTMoveToAction::UpdateFinalDestination(const xVec3& newDest) {
    if (updateDestTimer > 0.0f) {
        return;
    }

    if (actionCode != eNavLinkActionCode_None) {
        return;
    }

    float npcRadius = npcBase->npcEntity->npcBound.GetBoundRadiusXZ();

    finalDest = newDest;
    pathFinder.FindPath(&npcBase->npcEntity->model->Mat.pos, &finalDest,
                        npcRadius, eWallNetUserType_NPC);
    updateDestTimer = 0.5f;
    updatePathTimer = 2.0f;
}

void zNPCBTMoveToAction::UpdateDestination() {
    xVec3 dest;

    btClient->blackboard.Read(NPC_VAR_CURRENT_DESTINATION, dest);
    SetDestination(dest);
}

void zNPCBTMoveToAction::UpdateMoveTo(float dt) {
    SetMoveToDestination(currentDest);

    if (pathResult != ePathEvalResult_PathEnd) {
        _v11()->SetArriveToDest(0);
    } else if (asset->MovementData.Arrive) {
        _v11()->SetArriveToDest(1);
    } else {
        _v11()->SetArriveToDest(0);
    }
}

void zNPCBTMoveToAction::UpdatePathFinding(float dt) {
    if (asset->UsePathFinding) {
        if (updatePathTimer >= 0.0f) {
            updatePathTimer -= dt;

            if (updatePathTimer < 0.0f) {
                float npcRadius =
                    npcBase->npcEntity->npcBound.GetBoundRadiusXZ();

                pathFinder.FindPath(&npcBase->npcEntity->model->Mat.pos,
                                    &finalDest, npcRadius,
                                    eWallNetUserType_NPC);
                updatePathTimer = 2.0f;
            }
        }

        if (updateDestTimer > 0.0f) {
            updateDestTimer -= dt;
        }

        __ct__Q24Math8Matrix33Fv(&pathFinder);

        if (EvaluatePath()) {
            UpdateMoveTo(dt);
        }
    }
}

eTaskState zNPCBTMoveToAction::Update(float dt) {
    _v10();
    UpdatePathFinding(dt);
    npcBase->npcSteering->_s14(dt);
    stuckRangeMultiplier.Update(npcBase->npcEntity->frame->vel, dt);

    if (asset->ExitWhenAnimationIsDone &&
        npcBase->npcEntity->IsAnimationStopped(actionAnim.animStateID)) {
        if (asset->MovementData.StopAtExit) {
            npcBase->npcEntity->frame->vel = xVec3::m_Null;
        }

        return eTaskState_Complete;
    }

    if (endWhenArrived) {
        if (IsArrived()) {
            if (asset->MovementData.Arrive || asset->MovementData.StopAtExit) {
                npcBase->npcEntity->frame->vel = xVec3::m_Null;
            }

            if (!asset->DontExitBeforeAnimationIsDone ||
                npcBase->npcEntity->IsAnimationStopped(
                    actionAnim.animStateID)) {
                return eTaskState_Complete;
            }
        } else if (IsStuck()) {
            return eTaskState_Fail;
        }
    }

    return eTaskState_Running;
}

void zNPCBTMoveToAction::End() {
    if (asset->UsePathFinding) {
        pathFinder.CancelSearch();
    }

    zNPCSteering* steering = npcBase->npcSteering;

    steering->_s11(_v11());
}

void zNPCBTMoveToAction::Cleanup() {
    if (asset->UsePathFinding) {
        pathFinder.Cleanup();
    }

    if (!asset->CheckForWallNetEdges) {
        npcBase->npcSteering->SetCheckForWallNetEdges(true);
    }
}

// ---------------------------------------------------------------------------
// zNPCBTJumpAction

void zNPCBTJumpAction::Initialize() {
    jumpControl.headingCalcType = 2;
    jumpAnimName = "JUMP";
    jumpStartAnimName = "JUMP_START";
    jumpEndAnimName = "JUMP_END";
}

// The launch uses the asset's XZ speed or its apex height; the destination
// is a move point named by an event in the asset's variable, or the current
// destination.
void zNPCBTJumpAction::Begin() {
    const Sext::Action_NPC_Jump* asset =
        (const Sext::Action_NPC_Jump*)actionAsset;
    bool checkDestination = asset->CheckDestination;
    bool useLaunchVelXZ = asset->UseLaunchVelXZ;
    unsigned int destVar = asset->DestinationVariable;

    useJumpStartAnim = npcBase->npcEntity->DoesAnimExist(jumpStartAnimName);
    useJumpEndAnim = npcBase->npcEntity->DoesAnimExist(jumpEndAnimName);

    if (useJumpStartAnim) {
        actionAnim.Init(jumpStartAnimName, 0.2f, 0.0f);
    } else {
        actionAnim.Init(jumpAnimName, 0.2f, 0.0f);
    }

    actionAnim.StartOnNPC(npcBase->npcEntity, false);

    if (useLaunchVelXZ) {
        jumpControl.launchVelXZ = asset->VelocityXZ;
        jumpControl.useHeight = false;
    } else {
        jumpControl.apexHeight = asset->Height;
        jumpControl.useHeight = true;
    }

    jumpControl.maxLaunchVelY = asset->MaxVelocityY;

    if (checkDestination) {
        xVec3 dest;

        if (destVar != 0) {
            zVariableEventData eventData;

            if (btClient->blackboard.Read(destVar, eventData) &&
                eventData.eventID == 0x49DA9FC3 && eventData.dataPointer != 0) {
                xMovePoint* destMP = (xMovePoint*)zSceneFindObject(
                    ((const Sext::EventActionUid*)eventData.dataPointer)->uid);

                dest = *destMP->pos;
            } else {
                btClient->blackboard.Read(NPC_VAR_CURRENT_DESTINATION, dest);
            }
        } else {
            btClient->blackboard.Read(NPC_VAR_CURRENT_DESTINATION, dest);
        }

        jumpControl.SetDestination(&dest);
    }

    npcBase->npcSteering->_s10(&jumpControl);
    jumpState = 0;
}

// Take-off (after the start animation), airborne until the floor, landing
// (until the end animation is done).
eTaskState zNPCBTJumpAction::Update(float dt) {
    switch (jumpState) {
    case 0:
        if (!useJumpStartAnim) {
            jumpState = 1;
            break;
        }

        if (npcBase->npcEntity->IsAnimationStopped(actionAnim.animStateID)) {
            SwitchAnim(jumpAnimName);
            jumpState = 1;
        }
        break;
    case 1:
        if (npcBase->npcEntity->floorCollision) {
            if (useJumpEndAnim) {
                SwitchAnim(jumpEndAnimName);
            }

            jumpState = 2;
        }
        break;
    case 2:
        npcBase->npcEntity->KillVelocity();

        if (!useJumpEndAnim) {
            return eTaskState_Complete;
        }

        if (npcBase->npcEntity->IsAnimationStopped(actionAnim.animStateID)) {
            return eTaskState_Complete;
        }
        break;
    }

    npcBase->npcSteering->_s14(dt);

    return eTaskState_Running;
}

void zNPCBTJumpAction::SwitchAnim(const char* name) {
    actionAnim.Init(name, 0.2f, 0.0f);
    actionAnim.StartOnNPC(npcBase->npcEntity, false);
}

// ---------------------------------------------------------------------------
// zNPCBTFollowPlayerAction: the player comes from a blackboard variable,
// re-read whenever the variable changes.

// The player variable is the asset's, or the current player's when the asset
// names none; the delegate re-reads the player whenever it changes.
void zNPCBTFollowPlayerAction::Begin() {
    actionAnim.SetAnimation(asset->MovementData.MoveType,
                            asset->MovementData.AnimationName);
    InitMovement();
    InitPathFinder();

    playerVariable =
        ((const Sext::Action_NPC_FollowPlayer*)actionAsset)->PlayerVariable != 0
            ? ((const Sext::Action_NPC_FollowPlayer*)actionAsset)->PlayerVariable
            : NPC_VAR_CURRENT_PLAYER;
    UpdatePlayer();

    observer.object = this;
    observer.func =
        &Util::DelegateP0<void>::InvokeMember<
            zNPCBTFollowPlayerAction, &zNPCBTFollowPlayerAction::UpdatePlayer>;
    btClient->blackboard.RegisterVariableObserver(playerVariable, &observer);

    _v10();
    npcBase->GetSteering()->_s10(_v11());
    actionAnim.StartOnNPC(npcBase->npcEntity, false);
}

void zNPCBTFollowPlayerAction::End() {
    zNPCBTMoveToAction::End();
    btClient->blackboard.UnregisterVariableObserver(playerVariable, &observer);
}

void zNPCBTFollowPlayerAction::Cleanup() {
    zNPCBTMoveToAction::Cleanup();
    btClient->blackboard.UnregisterVariableObserver(playerVariable, &observer);
}

// The first human player found, whatever its distance.
zPlayer* zNPCBTFollowPlayerAction::GetPlayer() {
    zPlayer* closestPlayer = 0;
    float minDist2 = 3.4028235e+38f;

    for (int i = 0; i < xglobals->players.numPlayers; i++) {
        if (!xglobals->players.playerArray[i]->IsAI()) {
            xVec3 playerPos = xglobals->players.playerArray[i]->model->Mat.pos;
            float dist2 =
                playerPos.Distance2XZ(npcBase->npcEntity->model->Mat.pos);

            if (dist2 < minDist2) {
                minDist2 = dist2;
                closestPlayer = xglobals->players.playerArray[i];
            }

            break;
        }
    }

    return closestPlayer;
}

void zNPCBTFollowPlayerAction::UpdateDestination() {
    if (GetPlayer() != 0) {
        xVec3 dest = GetPlayer()->model->Mat.pos;

        SetDestination(dest);
    }
}

void zNPCBTFollowPlayerAction::UpdatePlayer() {
    btClient->blackboard.Read(playerVariable, player);
}

// ---------------------------------------------------------------------------
// zNPCBTEscortAction: go where the first player is.

void zNPCBTEscortAction::UpdateDestination() {
    xVec3 dest = xglobals->players.playerArray[0]->model->Mat.pos;

    SetDestination(dest);
}

void zNPCBTEscortAction::Begin() {
    actionAnim.SetAnimation(asset->MovementData.MoveType,
                            asset->MovementData.AnimationName);
    actionAnim.StartOnNPC(npcBase->npcEntity, false);
    InitMovement();
    InitPathFinder();
    _v10();

    if (npcBase->flying) {
        _v11()->SetIgnoreYComponent(false);
        _v11()->SetWanderIgnoreY(false);
    }

    _v11()->SetSpeedLimitXZ(100.0f);
    _v11()->SetSpeedLimitY(100.0f);
    _v11()->SetMaxAccLimits(100.0f);
    npcBase->GetSteering()->_s10(_v11());
    endWhenArrived = false;
}

zNPCSteeringEscortControl* zNPCBTEscortAction::SteeringControl() {
    return &escortControl;
}

eTaskState zNPCBTEscortAction::Update(float dt) {
    return zNPCBTMoveToAction::Update(dt);
}

// ---------------------------------------------------------------------------
// zNPCBTFleeAction: away from the flee point, along the flee direction, or
// away from the nearest human player.

void zNPCBTFleeAction::Begin() {
    actionAnim.SetAnimation(asset->MovementData.MoveType,
                            asset->MovementData.AnimationName);
    InitMovement();
    InitPathFinder();

    const Sext::Action_NPC_Flee* asset =
        (const Sext::Action_NPC_Flee*)actionAsset;

    player = 0;

    if (asset->PlayerVariable != 0) {
        btClient->blackboard.Read(asset->PlayerVariable, player);
    }

    if (player == 0) {
        btClient->blackboard.Read(NPC_VAR_CURRENT_PLAYER, player);
    }

    _v10();
    actionAnim.StartOnNPC(npcBase->npcEntity, false);
    npcBase->GetSteering()->_s10(_v11());
}

void zNPCBTFleeAction::UpdateDestination() {
    xVec3 dest = xVec3::m_Null;
    const Sext::Action_NPC_Flee* asset =
        (const Sext::Action_NPC_Flee*)actionAsset;

    if (asset->UseFleePoint) {
        btClient->blackboard.Read(NPC_VAR_FLEE_POS, dest);
    } else if (asset->UseFleeDirection) {
        xVec3 fleeDir;

        if (btClient->blackboard.Read(NPC_VAR_FLEE_DIR, fleeDir)) {
            dest.AddScale(npcBase->npcEntity->model->Mat.pos, fleeDir, 20.0f);
        }
    } else {
        if (player == 0) {
            float minDist2 = 3.4028235e+38f;

            for (int i = 0; i < xglobals->players.numPlayers; i++) {
                if (!xglobals->players.playerArray[i]->IsAI()) {
                    float dist2 = xVec3Dist2(
                        &npcBase->npcEntity->model->Mat.pos,
                        &xglobals->players.playerArray[i]->model->Mat.pos);

                    if (dist2 < minDist2) {
                        minDist2 = dist2;
                        player = xglobals->players.playerArray[i];
                    }
                }
            }
        }

        xVec3 playerToNPC;

        playerToNPC.Sub(npcBase->npcEntity->model->Mat.pos,
                        player->model->Mat.pos);
        playerToNPC.NormalizeSafe();
        playerToNPC *= 20.0f;
        dest.Add(npcBase->npcEntity->model->Mat.pos, playerToNPC);
    }

    SetDestination(dest);
}

// ---------------------------------------------------------------------------
// zNPCBTFlutterAction

void zNPCBTFlutterAction::Begin() {
    actionAnim.SetAnimation(asset->MovementData.MoveType,
                            asset->MovementData.AnimationName);
    InitMovement();
    InitPathFinder();

    zNPCTemplate* npcTemplate = npcBase->npcTemplate;

    if (npcTemplate != 0 && (npcTemplate->templateAsset->flags & 1)) {
        _v11()->SetIgnoreYComponent(false);
    }

    _v10();
    actionAnim.StartOnNPC(npcBase->npcEntity, false);
    npcBase->GetSteering()->_s10(_v11());
}

eTaskState zNPCBTFlutterAction::Update(float dt) {
    npcBase->npcSteering->_s14(dt);
    UpdatePathFinding(dt);
    stuckRangeMultiplier.Update(npcBase->npcEntity->frame->vel, dt);

    if (npcBase->npcEntity->IsAnimationStopped(actionAnim.animStateID)) {
        return eTaskState_Complete;
    }

    if (IsArrived() || IsStuck()) {
        _v10();
    }

    return eTaskState_Running;
}

// ---------------------------------------------------------------------------
// zNPCBTFollowProjectileAction

void zNPCBTFollowProjectileAction::Begin() {
    projectile = 0;

    int projectileID;

    if (btClient->blackboard.Read(NPC_VAR_CURRENT_PROJECTILE_ID,
                                  projectileID)) {
        projectile = zProjectileManager::sFindProjectileFromID(projectileID);
    }

    zNPCBTMoveToAction::Begin();
}

void zNPCBTFollowProjectileAction::UpdateDestination() {
    xVec3 dest = xVec3::m_Null;

    if (projectile != 0) {
        dest = projectile->GetPos();
    }

    SetDestination(dest);
}

eTaskState zNPCBTFollowProjectileAction::Update(float dt) {
    int projectileID;

    if (!btClient->blackboard.Read(NPC_VAR_CURRENT_PROJECTILE_ID,
                                   projectileID)) {
        return eTaskState_Fail;
    }

    projectile = zProjectileManager::sFindProjectileFromID(projectileID);

    return projectile != 0 ? zNPCBTMoveToAction::Update(dt) : eTaskState_Fail;
}

// ---------------------------------------------------------------------------
// zNPCBTPathFollowMPAction

// The path starts at a move point named by an event in the asset's
// variable (which may also change the move type), the target move point, or
// the NPC's own.
void zNPCBTPathFollowMPAction::Begin() {
    const Sext::Action_NPC_PathFollowMP* asset =
        (const Sext::Action_NPC_PathFollowMP*)actionAsset;

    arriveTol2 = asset->MovementData.ArriveTolerance *
                 asset->MovementData.ArriveTolerance;

    unsigned int destVar = asset->MovementVariable;
    Sext::eNPCMoveType moveType = asset->MovementData.MoveType;
    xMovePoint* mp = 0;

    if (destVar != 0) {
        zVariableEventData eventData;

        if (btClient->blackboard.Read(destVar, eventData) &&
            eventData.eventID == 0x9A75D58B && eventData.dataPointer != 0) {
            const Sext::EventActionFollowPath* followPathData =
                (const Sext::EventActionFollowPath*)eventData.dataPointer;

            mp = (xMovePoint*)zSceneFindObject(followPathData->uid);
            moveType = followPathData->MoveType;
        } else {
            btClient->blackboard.Read(NPC_VAR_TARGET_MP, mp);
        }
    }

    if (mp == 0) {
        btClient->blackboard.Read(NPC_VAR_TARGET_MP, mp);
    }

    if (mp == 0) {
        mp = npcBase->npcMovePoint;
    }

    path.SetCurrMP(mp, npcBase->npcEntity->model->Mat.pos);
    path.SetHalfWidth(0.5f * asset->PathWidth);
    path.arriveTol2 = asset->MovementData.ArriveTolerance *
                      asset->MovementData.ArriveTolerance;
    followControl.path = &path;
    followControl.wanderData.enabled = true;
    followControl.wanderData.wanderFOV = 1.0f;

    if (npcBase->flying) {
        followControl.ignoreYComponent = false;
        followControl.wanderData.ignoreY = false;
    }

    BT_Utility::SetMovementLimits(npcBase, npcBase->npcSteering, &followControl,
                                  moveType, asset->MovementData.MaxSpeed,
                                  asset->MovementData.MaxAcceleration,
                                  asset->MovementData.TurningRadius,
                                  asset->MovementData.TurnSpring);
    followControl.headingCalcType = asset->MovementData.Heading;
    actionAnim.SetAnimation(moveType, asset->MovementData.AnimationName);
    actionAnim.StartOnNPC(npcBase->npcEntity, false);
    npcBase->npcSteering->_s10(&followControl);
    stuckRangeMultiplier.curMultiplier = 1.0f;
}

// Defined above ArrivedAtMP and IsStuck, which retail calls rather than
// takes in line.
eTaskState zNPCBTPathFollowMPAction::Update(float dt) {
    if (path._p4()) {
        return eTaskState_Fail;
    }

    if (ArrivedAtMP()) {
        path.UpdateToNextMP(npcBase->npcEntity->model->Mat.pos);
    }

    if (path._p4()) {
        return eTaskState_Complete;
    }

    npcBase->npcSteering->_s14(dt);

    if (path._p4()) {
        return eTaskState_Complete;
    }

    stuckRangeMultiplier.Update(npcBase->npcEntity->frame->vel, dt);

    return IsStuck() ? eTaskState_Fail : eTaskState_Running;
}

void zSteeringPath::SetHalfWidth(float hw) {
    halfWidth = hw;
    halfWidthSqr = hw * hw;
}

bool zNPCBTPathFollowMPAction::ArrivedAtMP() {
    float dist2 =
        npcBase->npcEntity->model->Mat.pos.Distance2XZ(*path.currMP->pos);

    return dist2 < arriveTol2;
}

bool zNPCBTPathFollowMPAction::IsStuck() {
    if (path._p4()) {
        return true;
    }

    return stuckRangeMultiplier.curMultiplier ==
           stuckRangeMultiplier.maxMultiplier;
}

bool zSteeringPathMovePoints::IsPathEnd() const { return currMP == 0; }

// The move point reached last is handed back through the blackboard.
void zNPCBTPathFollowMPAction::End() {
    npcBase->npcSteering->_s11(&followControl);

    xMovePoint* mp = path.prevMP;

    btClient->blackboard.Write(NPC_VAR_TARGET_MP, mp);
}

// ---------------------------------------------------------------------------
// zNPCBTStopAction

void zNPCBTStopAction::Setup(const Sext::ActionBase* a) {
    const Sext::Action_NPC_Stop* asset = (const Sext::Action_NPC_Stop*)a;

    acceleration = asset->Acceleration;
    headingCalcType = asset->Heading;
}

void zNPCBTStopAction::Begin() {
    stopControl.SetMaxAcc(acceleration);
    stopControl.headingCalcType = headingCalcType;
    npcBase->npcSteering->_s10(&stopControl);
}

eTaskState zNPCBTStopAction::Update(float dt) {
    npcBase->npcSteering->_s14(dt);

    return eTaskState_Running;
}

// ---------------------------------------------------------------------------
// zNPCBTFaceFromEventAction

void zNPCBTFaceFromEventAction::Setup(const Sext::ActionBase* a) {
    const Sext::Action_NPC_FaceFromEvent* asset =
        (const Sext::Action_NPC_FaceFromEvent*)a;

    animID = asset->Animation;
    eventVarID = asset->FaceEventVar;
    acceleration = asset->Acceleration;
}

// Face along the heading, or towards an object named by an event in the
// variable; invalid when the event names nothing visible or the animation
// does not exist.
void zNPCBTFaceFromEventAction::Begin() {
    valid = true;
    stopControl.SetMaxAcc(acceleration);
    npcBase->npcSteering->_s10(&stopControl);

    if (eventVarID != 0) {
        zVariableEventData eventData;

        if (btClient->blackboard.Read(eventVarID, eventData)) {
            if (eventData.GetEventID() == 0x057AE2C7) {
                stopControl.headingCalcType = 0;
            } else if (eventData.GetEventID() == 0x7FBBF77F &&
                       eventData.dataPointer != 0) {
                stopControl.headingCalcType = 8;

                xBase* faceObject = zSceneFindObject(
                    ((const Sext::EventActionUid*)eventData.dataPointer)->uid);

                if (faceObject != 0 && (faceObject->baseFlags & 0x20)) {
                    target.pDest = &faceObject->model->Mat.pos;
                    stopControl.SetCustomHeading(target);
                } else {
                    valid = false;
                }
            } else {
                valid = false;
            }
        }
    }

    valid &= npcBase->npcEntity->DoesAnimExist(animID);
    actionAnim.Init(animID, 0.2f, 0.0f);
    actionAnim.StartOnNPC(npcBase->npcEntity, false);
}

void zNPCBTFaceFromEventAction::End() {
    npcBase->npcSteering->_s11(&stopControl);
}

// ---------------------------------------------------------------------------
// zNPCBTTeleportAction

void zNPCBTTeleportAction::Setup(const Sext::ActionBase* a) {
    destVar = *(const unsigned int*)a;
}

// To a move point named by an event in the variable, or to the current
// destination, keeping the NPC's orientation.
eTaskState zNPCBTTeleportAction::Update(float dt) {
    xVec3 currentDest;

    if (destVar != 0) {
        zVariableEventData eventData;

        if (!btClient->blackboard.Read(destVar, eventData) ||
            eventData.eventID != 0x6A29EB24 || eventData.dataPointer == 0) {
            return eTaskState_Fail;
        }

        xMovePoint* destMP = (xMovePoint*)zSceneFindObject(
            ((const Sext::EventActionUid*)eventData.dataPointer)->uid);

        currentDest = *destMP->pos;
    } else {
        btClient->blackboard.Read(NPC_VAR_CURRENT_DESTINATION, currentDest);
    }

    destMat = npcBase->npcEntity->model->Mat;
    destMat.pos = currentDest;
    npcBase->npcEntity->model->Mat = destMat;
    npcBase->npcSteering->_s15();
    npcBase->npcEntity->KillVelocity();
    npcBase->npcEntity->UpdateWithTeleport(dt);

    return eTaskState_Complete;
}

// ---------------------------------------------------------------------------
// zNPCBTOrbitAction

eTaskState zNPCBTOrbitAction::Update(float dt) {
    npcBase->npcSteering->_s14(dt);
    UpdateOrbitDirection(dt);
    UpdateOrbitDestination(dt);

    bool animDone =
        npcBase->npcEntity->IsAnimationStopped(actionAnim.animStateID);

    return animDone ? eTaskState_Complete : eTaskState_Running;
}

// ---------------------------------------------------------------------------
// zNPCBTSetFlyingAction

eTaskState zNPCBTSetFlyingAction::Update(float dt) {
    npcBase->flying = *(const bool*)actionAsset;

    return eTaskState_Complete;
}
