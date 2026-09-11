#include "SB/GM/Engine/WAD02_31.pool.h"

// WAD02_31.cpp -- zNPCSteering.cpp and zNPCSteeringControls.cpp as they
// sit in the WAD02 unity unit: the steering component an NPC owns (one
// entity, or a swarm of up to 64), its two Havok collision handlers, and
// the steering controls it runs. Read from the image with tools/disasm.py;
// the layouts are the DWARF's.
//
// zNPCSteering is polymorphic from +4: zNPCComponent keeps its owner in
// front of the vptr and declares slots 0-9, zNPCSteering adds 10-21
// (SetControl 10, RemoveControl 11, ApplySteering 14, SwitchWallNet 16,
// GetWallNetPosition 17). zNPCSteeringControl's vptr follows its members
// at +0x48: Enter 0, Exit 1, CalculateAcc 2, CalculateVelConstraints 3,
// GetSeekAcc 4, GetArriveAcc 5. The virtual calls go through placeholder
// slots; every class's first virtual is defined nowhere, so this object
// references the vtables and does not define them.
//
// xVec3's operator= is out of line, so every assignment is a `bl`.

class xBase {
public:
    unsigned char _pad0[0x20];
    unsigned int baseType;
};

class zWallNet;
class zNPCStatus;
class zNPCBase;
class zNPCEntity;
class zNPCSteering;
class zNPCSteeringControl;

namespace Sext {
class EventAny;
}

class xVec3 {
public:
    xVec3& operator=(const xVec3& v);
    xVec3& operator*=(float s);
    void AddScale(const xVec3& v, float s);
    void Scale(const xVec3& v, float s);
    void Add(const xVec3& a, const xVec3& b);
    void Sub(const xVec3& a, const xVec3& b);
    float length2XZ() const;
    float NormalizeSafe();

    float x;
    float y;
    float z;

    static const xVec3 m_Null;
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

namespace Math {
float rsqrt(float x);
}

float xatan2(float y, float x);
xBase* zSceneFindObject(unsigned long long id);

namespace World {

class EntityManager {
public:
    static xBase* FindAsset(unsigned long long id);
};

EntityManager* GetEntityManager();

}  // namespace World

class EventActionSwitchWallNet {
public:
    unsigned long long WallNet;
};

// ---------------------------------------------------------------------------
// Havok

enum HK_MEMORY_CLASS { HK_MEMORY_CLASS_BASE = 22 };

class hkThreadMemory {
public:
    void* allocateChunk(int nbytes, HK_MEMORY_CLASS cls);
    void deallocateChunk(void* p, int nbytes, HK_MEMORY_CLASS cls);
};

extern hkThreadMemory* hkThreadMemory__s_threadMemoryInstance;

class hkBaseObject {
public:
    virtual ~hkBaseObject() {}
};

class hkReferencedObject : public hkBaseObject {
public:
    void operator delete(void* p) {
        hkThreadMemory__s_threadMemoryInstance->deallocateChunk(
            p, static_cast<hkReferencedObject*>(p)->m_memSizeAndFlags,
            HK_MEMORY_CLASS_BASE);
    }

    unsigned short m_memSizeAndFlags;
    short m_referenceCount;
};

class hkpCharacterProxyListener {
public:
    virtual ~hkpCharacterProxyListener() {}
};

class hkpCollisionListener {
public:
    virtual ~hkpCollisionListener() {}
};

class zNPCCollisionListener : public hkpCharacterProxyListener {
public:
    zNPCCollisionListener();
    virtual ~zNPCCollisionListener();

    zNPCBase* npc;
};

class zNPCRigidBodyCollisionListener : public hkpCollisionListener {
public:
    virtual ~zNPCRigidBodyCollisionListener();

    zNPCBase* npc;
    void* body;
};

class zNPCSteeringCollisionHandler : public hkReferencedObject,
                                     public zNPCCollisionListener {
public:
    virtual void _h0();

    int currentNumberOfUserEdgesAdded;
    bool checkForWallnetEdges;
    bool collidedWithWallnetEdge;
};

class hkArrayBase {
public:
    void* m_data;
    int m_size;
    int m_capacityAndFlags;
};

class zNPCSteeringRigidBodyCollisionHandler
    : public hkReferencedObject,
      public zNPCRigidBodyCollisionListener {
public:
    virtual void _h0();

    int currentNumberOfUserEdgesAdded;
    bool checkForWallnetEdges;
    bool collidedWithWallnetEdge;
    hkArrayBase m_verticalContactPoints;
};

// ---------------------------------------------------------------------------
// Springs, wall nets

class xSpringyF32 {
public:
    xSpringyF32();
    void Reset();

    unsigned char _pad0[0xC];
    float mVelocity;
    float mGoal;
    float mCurrent;
};

class xSpringyAngle : public xSpringyF32 {
public:
    void Update(float dt);
};

class zWallNetPositionXZ {
public:
    zWallNetPositionXZ();
    ~zWallNetPositionXZ();

    void SwitchWallNet(const zWallNet* wallNet);

    xVec3 curPos;
    zWallNet* curWallNet;
    int curTriangleId;
};

class zWallNetDestinationXZ {
public:
    void SetupPosition(const xVec3& pos, const zWallNet* wallNet,
                       float radius);

    zWallNetPositionXZ wantedDestination;
    zWallNetPositionXZ modifiedDestination;
    bool isWantedModified;
    bool isWantedReachable;
    bool isValid;
};

// ---------------------------------------------------------------------------
// The NPC

class zNPCBound {
public:
    float GetBoundRadiusXZ() const;

    unsigned char _pad0[0x30];
};

class zNPCCollisionListener;
class zNPCRigidBodyCollisionListener;

class NPCTemplate {
public:
    unsigned char _pad0[0x90];
    unsigned char movementStyle;
};

class zNPCTemplate {
public:
    NPCTemplate* templateAsset;
};

class zNPCEntity {
public:
    void UnregisterCollisionListener(zNPCCollisionListener* listener);
    void UnregisterCollisionListener(
        zNPCRigidBodyCollisionListener* listener);

    unsigned char _pad0[0x34];
    xMat4x3* model;
    unsigned char _pad1[0xBC - 0x38];
    zNPCBase* owner;
    unsigned char _pad2[0xF4 - 0xC0];
    zNPCBound npcBound;
};

class zNPCBase {
public:
    unsigned char _pad0[0x78];
    zNPCTemplate* npcTemplate;
    unsigned char _pad1[0x98 - 0x7C];
    zNPCEntity* npcEntity;
};

// ---------------------------------------------------------------------------
// zNPCSteering

class zNPCComponentData {
public:
    zNPCBase* owner;
};

class zNPCComponent : public zNPCComponentData {
public:
    zNPCComponent() { owner = 0; }

    virtual void _c0();
    virtual void _c1(zNPCStatus* status);
    virtual void _c2();
    virtual void _c3();
    virtual void _c4();
    virtual void _c5(float dt);
    virtual void _c6();
    virtual void _c7();
    virtual void _c8();
    virtual bool _c9(xBase* from, xBase* to, unsigned int toEvent,
                     Sext::EventAny* params);
};

class zNPCSteering : public zNPCComponent {
public:
    zNPCSteering();

    bool SystemEvent(xBase* from, xBase* to, unsigned int toEvent,
                     Sext::EventAny* genericParams);
    void IntegrateAcc(const xVec3& acc, const xVec3& oldVel,
                      float speedLimitXZ, float speedLimitY, xVec3* vel,
                      float dt);
    void ApplyExternalForce(const xVec3* force);
    void ApplyExternalImpulse(const xVec3* impulse);
    void SwitchWallNet(zWallNet* value);
    void SetCheckForWallNetEdges(bool check);

    virtual void _s10(zNPCSteeringControl* control);
    virtual void _s11(zNPCSteeringControl* control);
    virtual void _s12();
    virtual void _s13();
    virtual void _s14(float dt);
    virtual void _s15();
    virtual void _s16(zWallNet* wallNet);
    virtual zWallNetPositionXZ* _s17() const;

    zNPCSteeringCollisionHandler* steeringCollisionListener;
    zNPCSteeringRigidBodyCollisionHandler* steeringCollisionRigidBodyListener;
    xVec3 externalAccAccum;
    xVec3 externalVelocityAccum;
    xVec3 externalMomentumlessVelAccum;
    zWallNet* wallNet;
    float turnSpringK;
    float headingCalcInteropRatio;
    float gravity;
    int steeringNeedsUpdate;
};

class zNPCSingleSteering : public zNPCSteering {
public:
    zNPCSingleSteering();

    void Detached(zNPCStatus* status);
    void SetControl(zNPCSteeringControl* control);
    void RemoveControl(zNPCSteeringControl* control);
    void SnapTurnSpring();
    void SwitchWallNet(zWallNet* newWallNet);

    virtual void _c0();

    xVec3 supportVel;
    xVec3 wantedVel;
    xVec3 wantedHeading;
    xSpringyAngle turnSpring;
    zWallNetPositionXZ wallNetPosition;
    float curTurnRate;
    float timeSinceLastHoverRayCast;
    float estimatedGroundHeight;
    zNPCSteeringControl* curControl;
};

class zNPCSwarmSteering : public zNPCSteering {
public:
    zNPCSwarmSteering();

    void SwitchWallNet(zWallNet* newWallNet);

    virtual void _c0();

    xVec3 supportVels[64];
    xVec3 wantedVels[64];
    xVec3 wantedHeadings[64];
    xSpringyAngle turnSprings[64];
    zWallNetPositionXZ wallNetPositions[64];
    float curTurnRates[64];
    zNPCSteeringControl* curControls[64];
};

// ---------------------------------------------------------------------------
// The controls

class zNPCSteeringVec {
public:
    xVec3 dir;
    float mag;
};

class zNPCSteeringAccumulator {
public:
    xVec3 curAccumulatedVec;
    float curAccumulatedMag;
};

class zNPCSteeringDest {
public:
    zNPCSteeringDest() { pDest = 0; }

    xVec3 dest;
    xVec3* pDest;
};

class zNPCSteeringVecLimiter {
public:
    zNPCSteeringVecLimiter() {
        limitV.x = 1.0f;
        limitV.y = 1.0f;
        limitV.z = 1.0f;
        isSphere = 1;
    }

    void Set(float x, float y, float z) { limitV.x = x; limitV.y = y; limitV.z = z; isSphere = 0; }

    xVec3 limitV;
    int isSphere;
};

enum eHeadingCalcType {
    eHeadingCalcType_0 = 0,
    eHeadingCalcType_1 = 1,
};

class zNPCSteeringControl {
public:
    class zWanderData {
    public:
        zWanderData();

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

    zNPCSteeringControl();

    void SetMaxAcc(float acc);
    void SetMaxAcc(const xVec3& acc);
    static bool ApplyFrictionVel(float friction, const xVec3& vel,
                                 const xVec3& wantedVel, float dt,
                                 xVec3* outVel);
    bool GetVelCorrectionAcc(const xVec3& wantedVel, const xVec3& dest,
                             zNPCSteeringVecLimiter* limiter,
                             zNPCSteeringVec* acc, float dt, bool ignoreY);

    zNPCSteering* steering;
    zNPCBase* npcBase;
    zNPCEntity* npcEntity;
    zWallNetPositionXZ* wallNetPosition;
    eHeadingCalcType headingCalcType;
    zNPCSteeringDest customHeading;
    float speedLimitXZ;
    float speedLimitY;
    bool gravityEnabled : 1;
    bool forceApplyGravity : 1;
    bool turnSpringEnabled : 1;
    bool ignoreYComponent : 1;
    unsigned char movementStyle;
    float maxAcc;
    zNPCSteeringVecLimiter accLimiter;

    virtual void _v0(const zNPCSteeringControl* prev);
    virtual void _v1();
    virtual void _v2(xVec3* acc, float dt);
    virtual bool _v3(const xVec3& vel, const xVec3& wantedVel, float dt,
                     xVec3* outVel);
    virtual bool _v4(zNPCSteeringVec* acc, float dt);
    virtual bool _v5(zNPCSteeringVec* acc, float dt);
};

class zNPCSteeringStopControl : public zNPCSteeringControl {
public:
    bool CalculateVelConstraints(const xVec3& vel, const xVec3& wantedVel,
                                 float dt, xVec3* outVel);
};

class zNPCSteeringMoveToControl : public zNPCSteeringControl {
public:
    void Enter(const zNPCSteeringControl* prev);
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
    zWallNetDestinationXZ wallNetDestination;
};

class zNPCSteeringFleeControl : public zNPCSteeringControl {
public:
    bool GetFleeAcc(zNPCSteeringVec* fleeAcc, float dt);

    zWanderData wanderData;
    zWallAvoidanceData wallAvoidanceData;
    zNPCSteeringDest fleeDestination;
    zNPCSteeringAccumulator accAccum;
};

class zNPCSteeringFollowPlayerControl : public zNPCSteeringMoveToControl {
public:
    void Enter(const zNPCSteeringControl* prev);
};

class zNPCSteeringJumpControl : public zNPCSteeringControl {
public:
    void SetDestination(const xVec3* dest);
    void Enter(const zNPCSteeringControl* prev);

    float apexHeight;
    float launchVelXZ;
    float maxLaunchVelY;
    zNPCSteeringDest destination;
    zNPCSteeringAccumulator accAccum;
    bool useHeight;
    bool jumped;
};

class zNPCSteeringFloatControl : public zNPCSteeringControl {
public:
    void Enter(const zNPCSteeringControl* prev);
    void CalculateAcc(xVec3* acc, float dt);

    xVec3 floatAcc;
};

// ===========================================================================
// zNPCSteering.cpp

zNPCSteering::zNPCSteering() {
    steeringCollisionListener = 0;
    steeringCollisionRigidBodyListener = 0;
    externalAccAccum = xVec3::m_Null;
    externalVelocityAccum = xVec3::m_Null;
    externalMomentumlessVelAccum = xVec3::m_Null;
    wallNet = 0;
    turnSpringK = 10.0f;
    headingCalcInteropRatio = 0.5f;
    gravity = 19.6f;
}

// The one event is a wall-net switch; its parameter names the wall net by
// id, found in the scene first and in the entity manager after.
bool zNPCSteering::SystemEvent(xBase* from, xBase* to, unsigned int toEvent,
                               Sext::EventAny* genericParams) {
    if (toEvent == 0x6A4E593B) {
        EventActionSwitchWallNet* params =
            (EventActionSwitchWallNet*)genericParams;
        xBase* to_param_widget = 0;

        if (params != 0 && params->WallNet != 0) {
            to_param_widget = zSceneFindObject(params->WallNet);

            if (to_param_widget == 0) {
                to_param_widget =
                    World::GetEntityManager()->FindAsset(params->WallNet);
            }
        }

        if (to_param_widget == 0 || to_param_widget->baseType != 0x62) {
            return false;
        }

        _s16((zWallNet*)to_param_widget);
        return false;
    }

    return false;
}

void zNPCSteering::IntegrateAcc(const xVec3& acc, const xVec3& oldVel,
                                float speedLimitXZ, float speedLimitY,
                                xVec3* vel, float dt) {
    xVec3 dVel;

    dVel.Scale(acc, dt);
    vel->Add(oldVel, dVel);

    float speed2XZ = vel->length2XZ();

    if (speed2XZ > speedLimitXZ * speedLimitXZ) {
        float speedXZ = speed2XZ * Math::rsqrt(speed2XZ);
        float scaleXZ = speedLimitXZ / speedXZ;

        vel->x *= scaleXZ;
        vel->z *= scaleXZ;
    }

    if (vel->y < -speedLimitY) {
        vel->y = -speedLimitY;
    }

    if (vel->y > speedLimitY) {
        vel->y = speedLimitY;
    }
}

void zNPCSteering::ApplyExternalForce(const xVec3* force) {
    float npcMass = 1000.0f;

    externalAccAccum.AddScale(*force, 1.0f / npcMass);
}

void zNPCSteering::ApplyExternalImpulse(const xVec3* impulse) {
    float npcMass = 1000.0f;

    externalVelocityAccum.AddScale(*impulse, 1.0f / npcMass);
}

void zNPCSteering::SwitchWallNet(zWallNet* value) { wallNet = value; }

void zNPCSteering::SetCheckForWallNetEdges(bool check) {
    if (steeringCollisionListener != 0) {
        steeringCollisionListener->checkForWallnetEdges = check;
    } else if (steeringCollisionRigidBodyListener != 0) {
        steeringCollisionRigidBodyListener->checkForWallnetEdges = check;
    }
}

// ---------------------------------------------------------------------------
// zNPCSingleSteering

zNPCSingleSteering::zNPCSingleSteering() {
    curTurnRate = 0.0f;
    curControl = 0;
}

void zNPCSingleSteering::Detached(zNPCStatus* status) {
    if (steeringCollisionListener != 0) {
        owner->npcEntity->UnregisterCollisionListener(
            steeringCollisionListener);
        delete steeringCollisionListener;
    }

    if (steeringCollisionRigidBodyListener != 0) {
        owner->npcEntity->UnregisterCollisionListener(
            steeringCollisionRigidBodyListener);
        delete steeringCollisionRigidBodyListener;
    }
}

// The outgoing control is told it is leaving and let go of; the new one is
// told who drives it and is handed the control it replaces.
void zNPCSingleSteering::SetControl(zNPCSteeringControl* control) {
    if (control != 0 && control == curControl) {
        return;
    }

    if (curControl != 0) {
        curControl->_v1();
        curControl->steering = 0;
        curControl->npcBase = 0;
        curControl->npcEntity = 0;
        curControl->wallNetPosition = 0;
    }

    zNPCSteeringControl* oldControl = curControl;
    curControl = control;

    curControl->steering = this;
    curControl->npcBase = owner;
    curControl->npcEntity = owner->npcEntity;
    curControl->wallNetPosition = &wallNetPosition;
    curControl->_v0(oldControl);
}

void zNPCSingleSteering::RemoveControl(zNPCSteeringControl* control) {
    if (curControl == 0 || curControl != control) {
        return;
    }

    curControl->_v1();
    curControl->steering = 0;
    curControl->npcBase = 0;
    curControl = 0;
}

void zNPCSingleSteering::SnapTurnSpring() {
    xMat4x3* mat = owner->npcEntity->model;

    turnSpring.mGoal = xatan2(mat->at.x, mat->at.z);
    turnSpring.Reset();
    curTurnRate = 0.0f;
}

void zNPCSingleSteering::SwitchWallNet(zWallNet* newWallNet) {
    wallNet = newWallNet;
    wallNetPosition.SwitchWallNet(newWallNet);
}

// ---------------------------------------------------------------------------
// zNPCSwarmSteering

zNPCSwarmSteering::zNPCSwarmSteering() {
    for (int i = 0; i < 64; i++) {
        curTurnRates[i] = 0.0f;
        curControls[i] = 0;
    }
}

void zNPCSwarmSteering::SwitchWallNet(zWallNet* newWallNet) {
    wallNet = newWallNet;

    for (int i = 0; i < 64; i++) {
        wallNetPositions[i].SwitchWallNet(newWallNet);
    }
}

// ===========================================================================
// zNPCSteeringControls.cpp

zNPCSteeringControl::zNPCSteeringControl() {
    steering = 0;
    npcBase = 0;
    npcEntity = 0;
    wallNetPosition = 0;
    movementStyle = 0;
    headingCalcType = eHeadingCalcType_1;
    speedLimitXZ = 8.0f;
    speedLimitY = 19.0f;
    gravityEnabled = true;
    forceApplyGravity = false;
    turnSpringEnabled = true;
    ignoreYComponent = true;
}

void zNPCSteeringControl::SetMaxAcc(float acc) {
    maxAcc = acc;
    accLimiter.Set(acc, acc, acc);
}

// The largest component, through a macro that reads the inner one twice.
void zNPCSteeringControl::SetMaxAcc(const xVec3& acc) {
    float max = (acc.x > ((acc.y > acc.z) ? acc.y : acc.z))
                    ? acc.x
                    : ((acc.y > acc.z) ? acc.y : acc.z);

    maxAcc = max;
    accLimiter.Set(acc.x, acc.y, acc.z);
}

bool zNPCSteeringStopControl::CalculateVelConstraints(const xVec3& vel,
                                                      const xVec3& wantedVel,
                                                      float dt,
                                                      xVec3* outVel) {
    return ApplyFrictionVel(maxAcc, vel, wantedVel, dt, outVel);
}

void zNPCSteeringMoveToControl::Enter(const zNPCSteeringControl* prev) {
    movementStyle = npcEntity->owner->npcTemplate->templateAsset->movementStyle;
    wallNetDestination.isValid = false;

    if (destination.pDest != 0 && steering->wallNet != 0 && useWallNet) {
        wallNetDestination.SetupPosition(*destination.pDest,
                                         steering->wallNet,
                                         npcEntity->npcBound.GetBoundRadiusXZ());
    }
}

xVec3* zNPCSteeringMoveToControl::GetReachableDest() {
    if (wallNetDestination.isValid) {
        return wallNetDestination.isWantedModified
                   ? &wallNetDestination.modifiedDestination.curPos
                   : &wallNetDestination.wantedDestination.curPos;
    }

    return destination.pDest;
}

bool zNPCSteeringFleeControl::GetFleeAcc(zNPCSteeringVec* fleeAcc, float dt) {
    if (fleeDestination.pDest == 0) {
        return false;
    }

    xVec3 desiredVelocity;

    desiredVelocity.Sub(npcEntity->model->pos, *fleeDestination.pDest);
    desiredVelocity.y = 0.0f;
    desiredVelocity.NormalizeSafe();
    desiredVelocity *= speedLimitXZ;

    return GetVelCorrectionAcc(desiredVelocity, *fleeDestination.pDest,
                               &accLimiter, fleeAcc, dt, true);
}

void zNPCSteeringFollowPlayerControl::Enter(const zNPCSteeringControl* a0) {
    zNPCSteeringMoveToControl::Enter(a0);
}

void zNPCSteeringJumpControl::SetDestination(const xVec3* dest) {
    if (dest != 0) {
        destination.pDest = &destination.dest; destination.dest = *dest;
    }
}

void zNPCSteeringJumpControl::Enter(const zNPCSteeringControl* prev) {
    movementStyle = npcEntity->owner->npcTemplate->templateAsset->movementStyle;
    jumped = false;
}

void zNPCSteeringFloatControl::Enter(const zNPCSteeringControl* prev) {
    ignoreYComponent = false;
    gravityEnabled = false;
}

void zNPCSteeringFloatControl::CalculateAcc(xVec3* acc, float dt) {
    *acc = floatAcc;
    *acc *= dt;
}
