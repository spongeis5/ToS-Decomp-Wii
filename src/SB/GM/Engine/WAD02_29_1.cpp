// WAD02_29_1.cpp -- zNPCPerception: 43 functions, 7,868 bytes in the
// image. An NPC keeps up to four perception TARGETS, and each target
// carries six PERCEPTION TYPES -- reachable, chargable, attackable,
// visible, shootable, engageable. A type is a list of asset-driven
// NODES, each a shape (sphere, sound sphere, cylinder, angular sphere,
// angular cylinder, or the NPC's own wall net) tested against the
// target's position, optionally behind a line-of-sight check.
//
// Layouts from the DWARF (tools/dwarf_types.py), which covers this file
// completely -- WAD02.cpp is one of the eleven compile units the retail
// link kept debug info for. zNPCPerception 0x1E8, zNPCPerceptionTarget
// 0x74, zPerceptionType 0x10, zLOSCache 0x8; on the asset side
// Sext::NPCPerceptionAsset::PerceptionType 0x10 and PerceptionNode 0x28, whose shape union
// runs from +4 to +0x1B. The entity chain is from the same place:
// xEnt 0xC0, zNPCEntity 0x1D0 on it, zPlayer 0x480 on it and
// zCommonPlayer 0x8A0 on zPlayer -- which is why the geometry
// accessors below cast rather than reading everything off an xEnt.
//
// The float literals are read from .rodata rather than named by address:
// a fresh perception type starts at FLT_MAX (never perceived), goes to
// 0.0f the moment it is, and a target's line-of-sight cache starts at
// -0.1f so the first check always runs.

class xVec3;

class zWallNet {
public:
    bool IsInsideWallNetXZ(const xVec3& p) const;
};

// The type id every scene object carries; the image gives the values
// this file tests against and the order of the tests, not their names.
class xBase {
public:
    unsigned char _pad0[0x20];
    unsigned int baseType;
};

xBase* zSceneFindObject(unsigned long long id);


namespace World {
class xOGModel;
}  // namespace World

class zNPCBase;

class zNPCBound {
public:
    float GetBoundRadiusXZ() const;

    unsigned char _pad0[0x10];
    float radiusY;
};

// 0x1D0 in the DWARF, deriving from xEnt at +0 and from zNPCComponent
// at +0xBC. So the word DisableTarget sends its event from is that
// component's OWNER -- the zNPCBase -- and it reaches zEntEvent as an
// xBase* because a zNPCBase is one. The bound is at +0xF4.
class zNPCEntity {
public:
    void GetBoundCenter(xVec3& out) const;

    unsigned char _pad0[0x34];
    World::xOGModel* model;
    unsigned char _pad1[0xBC - 0x38];
    zNPCBase* owner;
    unsigned char _pad2[0xF4 - 0xC0];
    zNPCBound npcBound;
};

// The angular tests measure the tangent of the angle between the NPC's
// facing and the direction to its target. The sphere form takes the
// SQUARED tangent in three dimensions and compares against tanHAngle
// squared; the cylinder form takes the tangent in the XZ plane and
// compares against tanHAngle itself.
namespace Math {
float rsqrt(float x);
}  // namespace Math

extern "C" double tan(double x);

class zNPCHelper {
public:
    static float GetTanTheta2(const xVec3* from, const xVec3* forward,
                              const xVec3* to);
    static float GetTanThetaXZ(const xVec3* from, const xVec3* forward,
                               const xVec3* to);
};

class xVec3 {
public:
    xVec3& operator=(const xVec3& other);
    void Sub(const xVec3& a, const xVec3& b);
    float length2() const;
    float Distance2XZ(const xVec3& other) const;
    xVec3& operator*=(float s);

    static const xVec3 m_Null;

    float x;
    float y;
    float z;
};


// 0x14 in the DWARF, and its position is at +0, which is why the same
// pointer serves as both the xVec3 and the zWallNetPositionXZ argument.
class zWallNetPositionXZ {
public:
    xVec3 curPos;
    zWallNet* curWallNet;
    int curTriangleId;
};

class zWallNetCollis;

class zIWallNet {
public:
    static bool IntersectsSweptCircle(const zWallNet* wallNet,
                                      const zWallNetPositionXZ* from,
                                      const xVec3* dir, float radius,
                                      float distance,
                                      zWallNetCollis& collis);
};

class xSphere {
public:
    xVec3 center;
    float r;
};

namespace World {

class xOGModel {
public:
    unsigned char _pad0[0x20];
    xVec3 forward;
    unsigned char _pad1[0x30 - 0x2C];
    xVec3 position;
};

}  // namespace World

int xModelGetBoneCount(const World::xOGModel* model);

void xModelGetBoneLocationNoScale(xVec3& out, const World::xOGModel& model,
                                  unsigned long bone);


class xHavokPhysicsObject {
public:
    void GetBoundingSphere(xSphere* out) const;
};

// 0xC0 in the DWARF, and the accessors below reach PAST it through a
// cast, because the field they want belongs to whichever concrete type
// the base id names. The four ids are xBase type constants; the image
// gives the values and the order of the tests, not the names.
class xEnt : public xBase {
public:
    unsigned char _pad1[0x34 - 0x24];
    World::xOGModel* model;
    unsigned char _pad2[0x48 - 0x38];

    // A one-bit field, the eighth from the top of the word at +0x48:
    // retail rotates by 8 and masks bit 31, which normalises it to 0 or
    // 1 the way a bitfield read does and a mask does not. zVar's
    // zVarEntryCB_IsVisible reads this same bit at this same offset,
    // which is where the name comes from.
    unsigned int _bits0 : 7;
    unsigned int visible : 1;
    unsigned int _bits1 : 24;

    unsigned char _pad3[0x80 - 0x4C];
    xHavokPhysicsObject physics;
    unsigned char _pad4[0xC0 - 0x81];
};
// 0x480 in the DWARF, on the same 0xC0 entity base. The three members
// below are the ones this file reaches past an xEnt for, and it only
// does so once the base id has been checked for 0x55.
class zPlayer : public xEnt {
public:
    unsigned char _pad0[0x170 - 0xC0];
    xSphere extraSpheres[5];
    int numExtraSpheres;
    unsigned char _pad1[0x214 - 0x1C4];
    float capRadius;
    unsigned char _pad2[0x480 - 0x218];
};

// 0x8A0, and the only thing on it this file wants is the noise the
// player is making, which is what the sound sphere gates on.
class zCommonPlayer : public zPlayer {
public:
    float GetNoiseLevel();
};

class zWallNetCollis {
public:
    zWallNetCollis() { Reset(); }

    void Reset();

    unsigned char _pad0[0x90];
    unsigned int numberOfCollisions;
    unsigned int closestCollision;
    unsigned char _pad1[0xAC - 0x98];
    zWallNet* wallNet;
    unsigned char calculateDistance : 1;
    unsigned char calculateNormal : 1;
    unsigned char calculateExactPoint : 1;
    unsigned char consumed : 1;
    unsigned char hitIt : 1;
};

namespace Sext {

class EventAny {
public:
    unsigned long long id;
};

enum eCollisionLayer { eCollisionLayer_ = 0x7FFFFFFF };

enum eNPCPerceptionType {
    eNPCPerception_TargetReachable,
    eNPCPerception_TargetChargable,
    eNPCPerception_TargetAttackable,
    eNPCPerception_TargetVisible,
    eNPCPerception_TargetShootable,
    eNPCPerception_TargetEngageable,
    END_eNPCPerception_ENUM,
};

class PercShapeSphere {
public:
    float Radius;
};

class PercShapeSoundSphere {
public:
    float Radius;
    float SoundSphereDetectionNoise;
};

class PercShapeCylinder {
public:
    float Radius;
    float HeightUp;
    float HeightDown;
};

class PercShapeAngularSphere {
public:
    bool UseTargetBounds;
    unsigned char _pad0[0x4 - 0x1];
    float Radius;
    float Angle;
    float tanHAngle;
};

class PercShapeAngularCylinder {
public:
    bool UseTargetBounds;
    unsigned char _pad0[0x4 - 0x1];
    float Radius;
    float HeightUp;
    float HeightDown;
    float Angle;
    float tanHAngle;
};

class PercShapeNPCWalls {
public:
    float HeightUp;
    float HeightDown;
};

class CharacterAsset {
public:
    unsigned char _pad0[0xC0];
    unsigned long long perceptionAssetID;
};

// The uid of the NPC's wall net sits at +0x148; the field's own name
// is not recovered, only its offset and what AllAttached does with it.
class NPCAsset {
public:
    unsigned char _pad0[0x148];
    unsigned long long wallNetID;
};

class NPCPerceptionAsset {
public:
    class PerceptionNode {
    public:
        unsigned char Shape;
        unsigned char _pad0[0x4 - 0x1];

        union {
            PercShapeSphere Sphere;
            PercShapeSoundSphere SoundSphere;
            PercShapeCylinder Cylinder;
            PercShapeAngularSphere AngularSphere;
            PercShapeAngularCylinder AngularCylinder;
            PercShapeNPCWalls InNPCWalls;
        };

        unsigned int flags;
        unsigned int LOSCollisionLayer;
        float HysteresisRatio;
    };

    class PerceptionType {
    public:
        unsigned char Type;
        unsigned char _pad0[0x4 - 0x1];
        float MemoryTime;
        unsigned int nodeCount;
        PerceptionNode* nodes;
    };

    unsigned int typeCount;
    PerceptionType* types;
};

}  // namespace Sext

const Sext::CharacterAsset* zNPCAsset_GetCharacterAsset(
    const Sext::NPCAsset* asset);

namespace World {

class EntityManager {
public:
    // Static: retail passes only the id, in r3:r4, and discards the
    // manager the getter returned.
    static void* FindAsset(unsigned long long id);
};

EntityManager* GetEntityManager();

}  // namespace World

// zEntEvent(from, fromEvent, to, toEvent, param, force) -- the disable
// path sends one event with no parameter and the force flag set.
enum ForceEvent { ForceEvent_ = 0x7FFFFFFF };

void zEntEvent(xBase* from, unsigned int fromEvent, xBase* to,
               unsigned int toEvent, Sext::EventAny* param, ForceEvent force);

class zNPCStatus;
class zNPCSteeringOld;
class zNPCSteering;

// From the DWARF: 0xC0, an xOGEntity base at +0, npcStatus at +0x40,
// npcAsset at +0x60, npcEntity at +0x98 and the two steerings after it.
class zNPCBase {
public:
    void GetPosition(xVec3& out);

    unsigned char _pad0[0x60];
    const Sext::NPCAsset* asset;
    unsigned char _pad1[0x98 - 0x64];
    zNPCEntity* npcEnt;
    zNPCSteeringOld* npcSteeringOld;
    zNPCSteering* npcSteering;
};

// The vtable pointer sits at +4, after owner, which is what declaring
// the virtual BELOW the data member gives. The constructor is here
// rather than in zNPCPerception because retail stores owner = 0 before
// it constructs the target array -- a member array is built between the
// base constructor and the derived one's body.
class zNPCComponent {
public:
    zNPCComponent() { owner = 0; }

    zNPCBase* owner;

    virtual void Update();
};

// 0x48 in the DWARF, a zNPCComponent at +0 -- so the vtable is at +4,
// after the owner. The accessor this file calls sits at vtable +0x4C,
// which is slot 17 counting Update as slot 0; the slots between exist
// only to put it there. zNPCSteering's own entry there is null and the
// implementation is zNPCSingleSteering::GetWallNetPosition.
class zNPCSteering : public zNPCComponent {
public:
    virtual void _v1();   virtual void _v2();   virtual void _v3();
    virtual void _v4();   virtual void _v5();   virtual void _v6();
    virtual void _v7();   virtual void _v8();   virtual void _v9();
    virtual void _v10();  virtual void _v11();  virtual void _v12();
    virtual void _v13();  virtual void _v14();  virtual void _v15();
    virtual void _v16();
    virtual const zWallNetPositionXZ* GetWallNetPosition() const;
};

class zNPCPerception;

class zNPCPerceptionTarget {
public:
    class zPerceptionType {
    public:
        void Update();
        zWallNet* GetNPCWallNet();
        typedef Sext::NPCPerceptionAsset::PerceptionNode Node;

        bool CheckNodePerception(const Node* node);
        bool CheckSpherePerception(const Node* node);
        bool CheckSoundSpherePerception(const Node* node);
        bool CheckCylinderPerception(const Node* node);
        bool CheckAngularSpherePerception(const Node* node);
        bool CheckAngularSpherePerceptionWithTargetBounds(const Node* node);
        bool CheckAngularSpherePerceptionWithoutTargetBounds(const Node* node);
        bool CheckAngularCylinderPerception(const Node* node);
        bool CheckAngularCylinderPerceptionWithTargetBounds(const Node* node);
        bool CheckAngularCylinderPerceptionWithoutTargetBounds(
            const Node* node);
        bool CheckInNPCWallsPerception(const Node* node);
        bool IsInDirectPath(const zNPCEntity* npc, const zWallNet* wallNet);
        void Setup(Sext::NPCPerceptionAsset::PerceptionType* asset, zNPCPerceptionTarget* owner);
        void Cleanup();
        void SetPerceived(bool perceived);
        bool IsPerceived();

        Sext::NPCPerceptionAsset::PerceptionType* typeAsset;
        zNPCPerceptionTarget* ownerTarget;
        float perceivedTimer;
        bool isDirty;
        bool isPerceived;
        unsigned char _pad0[0x10 - 0xE];
    };

    class zLOSCache {
    public:
        bool CheckLineOfSight(const zNPCEntity* npc,
                              zNPCPerceptionTarget* target,
                              Sext::eCollisionLayer layer, bool force);

        float LOSTimer;
        bool isInLOS;
        unsigned char _pad0[0x8 - 0x5];
    };

    zNPCPerceptionTarget();

    void Setup(Sext::NPCPerceptionAsset* asset, zNPCPerception* owner);
    void Cleanup();
    void DisableTarget();
    bool IsPerceived(Sext::eNPCPerceptionType type);
    zNPCEntity* GetNPCEntity();
    void PostUpdate(float dt);
    void GetTargetEntityCenter(xVec3& out);
    float GetTargetEntityRadiusXZ();
    float GetTargetEntityRadiusY();

    xEnt* targetEnt;
    zNPCPerception* ownerNpcPerc;
    zPerceptionType types[6];
    zLOSCache losCache;
    bool perceivedAny;
    unsigned char _pad0[0x74 - 0x71];
};

class zNPCPerception : public zNPCComponent {
public:
    zNPCPerception();

    void SetAssetAuto(const Sext::NPCAsset* asset);
    void SetTarget(unsigned int index, xEnt* ent);
    void RemoveTarget(unsigned int index);
    void Attached(const zNPCStatus* status);
    void AllAttached();
    bool SystemEvent(xBase* from, xBase* to, unsigned int event,
                     Sext::EventAny* param);
    int GetTargetIndexClosestMatching(Sext::eNPCPerceptionType type,
                                      bool perceivedOnly);
    xEnt* GetTargetClosest();
    bool AreTargetsPerceived(unsigned int mask,
                             Sext::eNPCPerceptionType type, bool all);
    // Static: the five arguments go in r3 through r7 with no this.
    static bool CheckLineOfSight(const xVec3* from, const xVec3* to,
                                 const zNPCEntity* npc, const xEnt* ent,
                                 Sext::eCollisionLayer layer);
    void Detached(zNPCStatus* status);
    void PostUpdate(float dt);

    zWallNet* npcWallNet;
    Sext::NPCPerceptionAsset* perceptionAsset;
    zNPCPerceptionTarget targets[4];
    unsigned int targetBitMask;
    int status;
};

zNPCPerceptionTarget::zNPCPerceptionTarget() {
    // A BOTTOM-TESTED pointer walk, and all three parts matter. A
    // counted loop costs one instruction (mwcc puts the trip count in
    // ctr); the same walk written as a while costs five, because the
    // top test needs a guard and a computed trip count; and hoisting
    // &types[6] into a local of its own is eight words out. Written
    // this way there is no counter and no guard, which is retail.
    zPerceptionType* t = types;

    do {
        t->typeAsset = 0;
        t->isDirty = true;
        t++;
    } while (t < &types[6]);

    losCache.LOSTimer = -0.1f;
    losCache.isInLOS = false;
    targetEnt = 0;
}

void zNPCPerceptionTarget::Setup(Sext::NPCPerceptionAsset* asset,
                                 zNPCPerception* owner) {
    // count BEFORE i: retail holds the counter in r29 and the trip
    // count in r30, and declaring them the other way round -- or
    // initialising count where it is declared -- puts them in each
    // other's registers, four or five words out of 32.
    int count;
    int i;

    ownerNpcPerc = owner;
    count = asset->typeCount;

    for (i = 0; i < count; i++) {
        Sext::NPCPerceptionAsset::PerceptionType* t = &asset->types[i];

        if (types[t->Type].typeAsset == 0) {
            types[t->Type].Setup(t, this);
        }
    }

    perceivedAny = false;
}

void zNPCPerceptionTarget::Cleanup() {
    int i;

    ownerNpcPerc = 0;

    for (i = 0; i < 6; i++) {
        types[i].Cleanup();
    }
}

void zNPCPerceptionTarget::PostUpdate(float dt) {
    bool anyPerceived;
    int i;

    if (targetEnt == 0) {
        return;
    }

    anyPerceived = false;
    losCache.LOSTimer = losCache.LOSTimer - dt;

    for (i = 0; i < 6; i++) {
        if (types[i].typeAsset != 0) {
            // += , not x = x + dt: the compound form emits fadds with
            // the accumulator FIRST, the spelled-out form emits it
            // second, and either operand order of the spelled-out form
            // gives the same wrong one. One word of 75.
            types[i].perceivedTimer += dt;
            types[i].isDirty = true;

            if (!anyPerceived && types[i].IsPerceived()) {
                anyPerceived = true;
            }
        }
    }

    if (anyPerceived != perceivedAny) {
        if (perceivedAny) {
            zEntEvent((xBase*)ownerNpcPerc->owner->npcEnt->owner, 0,
                      (xBase*)targetEnt, 0x03EECE48, 0, (ForceEvent)1);
        } else {
            zEntEvent((xBase*)ownerNpcPerc->owner->npcEnt->owner, 0,
                      (xBase*)targetEnt, 0xEE71365E, 0, (ForceEvent)1);
        }

        perceivedAny = anyPerceived;
    }
}

void zNPCPerceptionTarget::DisableTarget() {
    if (targetEnt != 0) {
        zEntEvent((xBase*)ownerNpcPerc->owner->npcEnt->owner, 0,
                  (xBase*)targetEnt, 0x03EECE48, 0, (ForceEvent)1);
    }

    targetEnt = 0;
}

bool zNPCPerceptionTarget::IsPerceived(Sext::eNPCPerceptionType type) {
    if (targetEnt == 0) {
        return false;
    }

    if (types[type].typeAsset != 0) {
        return types[type].IsPerceived();
    }

    return false;
}


void zNPCPerceptionTarget::GetTargetEntityCenter(xVec3& out) {
    xEnt* ent = targetEnt;

    // An if CHAIN here, where the two radius accessors below are
    // switches: retail lays the first body immediately after its own
    // test and branches past it, which is what a chain gives. Written as
    // a switch all four tests come first and the bodies follow, 17 words
    // and four bytes out.
    if (ent->baseType == 0x55) {
        xModelGetBoneLocationNoScale(out, *ent->model, 0);
        return;
    }

    if (ent->baseType == 0x38 || ent->baseType == 0x5A ||
        ent->baseType == 0x56) {
        out = ent->model->position;
        return;
    }

    out = xVec3::m_Null;
}

float zNPCPerceptionTarget::GetTargetEntityRadiusXZ() {
    xEnt* ent = targetEnt;

    switch (ent->baseType) {
    case 0x55:
        return ((zPlayer*)ent)->capRadius;
    case 0x38:
        return ((zNPCEntity*)ent)->npcBound.GetBoundRadiusXZ();
    case 0x5A: {
        xSphere sphere;

        ent->physics.GetBoundingSphere(&sphere);
        return sphere.r;
    }
    case 0x56: {
        xSphere sphere;

        ent->physics.GetBoundingSphere(&sphere);
        return sphere.r;
    }
    }

    return 0.0f;
}

float zNPCPerceptionTarget::GetTargetEntityRadiusY() {
    xEnt* ent = targetEnt;

    switch (ent->baseType) {
    case 0x55:
        return ((zPlayer*)ent)->capRadius;
    case 0x38:
        return ((zNPCEntity*)ent)->npcBound.radiusY;
    case 0x5A: {
        xSphere sphere;

        ent->physics.GetBoundingSphere(&sphere);
        return sphere.r;
    }
    case 0x56: {
        xSphere sphere;

        ent->physics.GetBoundingSphere(&sphere);
        return sphere.r;
    }
    }

    return 0.0f;
}

void zNPCPerceptionTarget::zPerceptionType::Update() {
    int count;
    int i;

    count = typeAsset->nodeCount;

    for (i = 0; i < count; i++) {
        if (CheckNodePerception(&typeAsset->nodes[i])) {
            SetPerceived(true);
            return;
        }
    }

    SetPerceived(false);
}


void zNPCPerceptionTarget::zPerceptionType::Setup(Sext::NPCPerceptionAsset::PerceptionType* asset,
                                                  zNPCPerceptionTarget* owner) {
    typeAsset = asset;
    ownerTarget = owner;
    isDirty = true;
    isPerceived = false;
    perceivedTimer = 3.4028235e38f;
}

void zNPCPerceptionTarget::zPerceptionType::Cleanup() {
    typeAsset = 0;
    ownerTarget = 0;
    isDirty = true;
    isPerceived = false;
    perceivedTimer = 3.4028235e38f;
}

void zNPCPerceptionTarget::zPerceptionType::SetPerceived(bool perceived) {
    if (perceived) {
        perceivedTimer = 0.0f;
    }

    isPerceived = perceived;
    isDirty = false;
}

bool zNPCPerceptionTarget::zPerceptionType::IsPerceived() {
    if (isDirty) {
        isPerceived = perceivedTimer < typeAsset->MemoryTime;
        Update();
    }

    return isPerceived;
}

zNPCPerception::zNPCPerception() {
    // No loop here: the four targets are a member array with a
    // constructor, so mwcc builds them itself, and it emits the same
    // bottom-tested pointer walk the target's own constructor needed.
    npcWallNet = 0;
    perceptionAsset = 0;
    status = 0;
    targetBitMask = 0;
}

void zWallNetCollis::Reset() {
    numberOfCollisions = 0;
    closestCollision = 0;
    calculateDistance = 0;
    calculateNormal = 0;
    calculateExactPoint = 0;
    consumed = 0;
    hitIt = 0;
    wallNet = 0;
}

void zNPCPerception::SetAssetAuto(const Sext::NPCAsset* asset) {
    // id DECLARED first and assigned after: retail keeps its two halves
    // in r31 and r30 and the result in r29, and either order that
    // initialises id where it is declared rotates all three.
    unsigned long long id;
    Sext::NPCPerceptionAsset* found = 0;

    id = zNPCAsset_GetCharacterAsset(asset)->perceptionAssetID;

    if (id != 0) {
        found = (Sext::NPCPerceptionAsset*)World::GetEntityManager()
                    ->FindAsset(id);
    }

    perceptionAsset = found;
}

void zNPCPerception::SetTarget(unsigned int index, xEnt* ent) {
    RemoveTarget(index);
    targets[index].targetEnt = ent;
    targetBitMask = targetBitMask | (1 << index);

    if (perceptionAsset != 0) {
        targets[index].Setup(perceptionAsset, this);
    }
}

void zNPCPerception::RemoveTarget(unsigned int index) {
    zNPCPerceptionTarget* t = &targets[index];

    t->DisableTarget();
    targetBitMask = targetBitMask & ~(1 << index);
    t->Cleanup();
}

void zNPCPerception::AllAttached() {
    // The asset pointer into a local of its own: retail's npcWallNet = 0
    // store sits BETWEEN that load and the uid load, which is where it
    // lands only when the two are separate statements.
    const Sext::NPCAsset* data = owner->asset;
    unsigned long long id;

    npcWallNet = 0;
    id = data->wallNetID;

    if (id != 0) {
        xBase* obj = zSceneFindObject(id);

        if (obj != 0 && obj->baseType == 0x62) {
            npcWallNet = (zWallNet*)obj;
        }
    }
}

void zNPCPerception::Detached(zNPCStatus* status) {
    int i;

    perceptionAsset = 0;

    for (i = 0; i < 4; i++) {
        targets[i].DisableTarget();
    }
}

void zNPCPerception::PostUpdate(float dt) {
    unsigned int i;

    if (perceptionAsset == 0) {
        return;
    }

    for (i = 0; i < 4; i++) {
        targets[i].PostUpdate(dt);
    }
}

bool zNPCPerceptionTarget::zPerceptionType::CheckNodePerception(
    const Node* node) {
    // The flag is on the TARGET ENTITY, not the NPC.
    if (!ownerTarget->targetEnt->visible) {
        return false;
    }

    switch (node->Shape) {
    case 0:
        return CheckSpherePerception(node);
    case 1:
        return CheckSoundSpherePerception(node);
    case 2:
        return CheckCylinderPerception(node);
    case 3:
        return CheckAngularSpherePerception(node);
    case 4:
        return CheckAngularCylinderPerception(node);
    case 5:
        return CheckInNPCWallsPerception(node);
    }

    return false;
}

bool zNPCPerceptionTarget::zPerceptionType::CheckAngularSpherePerception(
    const Node* node) {
    if (node->AngularSphere.UseTargetBounds) {
        return CheckAngularSpherePerceptionWithTargetBounds(node);
    }

    return CheckAngularSpherePerceptionWithoutTargetBounds(node);
}

bool zNPCPerceptionTarget::zPerceptionType::CheckAngularCylinderPerception(
    const Node* node) {
    if (node->AngularCylinder.UseTargetBounds) {
        return CheckAngularCylinderPerceptionWithTargetBounds(node);
    }

    return CheckAngularCylinderPerceptionWithoutTargetBounds(node);
}

xEnt* zNPCPerception::GetTargetClosest() {
    int index =
        GetTargetIndexClosestMatching(Sext::END_eNPCPerception_ENUM, true);

    if (index < 0) {
        return 0;
    }

    return targets[index].targetEnt;
}

bool zNPCPerceptionTarget::zPerceptionType::CheckSpherePerception(
    const Node* node) {
    // Declared in this order because the FIRST takes the highest stack
    // slot: retail has targetCenter at 32, npcCenter at 20, delta at 8.
    xVec3 targetCenter;
    xVec3 npcCenter;
    xVec3 delta;
    float radius = node->Sphere.Radius;
    zNPCEntity* npc;
    zWallNet* wallNet;

    // Already perceived: widen the radius, so a target has to leave by
    // more than it entered by.
    if (isPerceived) {
        radius = radius * node->HysteresisRatio;
    }

    ownerTarget->GetTargetEntityCenter(targetCenter);
    radius = radius * radius;
    npc = ownerTarget->GetNPCEntity();
    npc->GetBoundCenter(npcCenter);
    delta.Sub(targetCenter, npcCenter);

    if (delta.length2() > radius) {
        return false;
    }

    wallNet = GetNPCWallNet();

    if (wallNet != 0) {
        if (node->flags & 2) {
            if (!wallNet->IsInsideWallNetXZ(targetCenter)) {
                return false;
            }
        }

        if (node->flags & 4) {
            if (!IsInDirectPath(npc, wallNet)) {
                return false;
            }
        }
    }

    if (node->flags & 1) {
        if (!ownerTarget->losCache.CheckLineOfSight(
                npc, ownerTarget,
                (Sext::eCollisionLayer)node->LOSCollisionLayer, false)) {
            return false;
        }
    }

    return true;
}


bool zNPCPerceptionTarget::zPerceptionType::CheckSoundSpherePerception(
    const Node* node) {
    xVec3 targetCenter;
    xVec3 npcCenter;
    xVec3 delta;
    float radius;
    zNPCEntity* npc;
    zWallNet* wallNet;

    if (ownerTarget->targetEnt->baseType != 0x55) {
        return false;
    }

    if (((zCommonPlayer*)ownerTarget->targetEnt)->GetNoiseLevel() <
        node->SoundSphere.SoundSphereDetectionNoise) {
        return false;
    }

    radius = node->SoundSphere.Radius;

    if (isPerceived) {
        radius = radius * node->HysteresisRatio;
    }

    ownerTarget->GetTargetEntityCenter(targetCenter);
    radius = radius * radius;
    npc = ownerTarget->GetNPCEntity();
    npc->GetBoundCenter(npcCenter);
    delta.Sub(targetCenter, npcCenter);

    if (delta.length2() > radius) {
        return false;
    }

    wallNet = GetNPCWallNet();

    if (wallNet != 0) {
        if (node->flags & 2) {
            if (!wallNet->IsInsideWallNetXZ(targetCenter)) {
                return false;
            }
        }

        if (node->flags & 4) {
            if (!IsInDirectPath(npc, wallNet)) {
                return false;
            }
        }
    }

    if (node->flags & 1) {
        if (!ownerTarget->losCache.CheckLineOfSight(
                npc, ownerTarget,
                (Sext::eCollisionLayer)node->LOSCollisionLayer, false)) {
            return false;
        }
    }

    return true;
}

bool zNPCPerceptionTarget::zPerceptionType::CheckCylinderPerception(
    const Node* node) {
    xVec3 npcCenter;
    xVec3 targetCenter;
    float radius = node->Cylinder.Radius;
    float heightUp = node->Cylinder.HeightUp;
    float heightDown = node->Cylinder.HeightDown;
    float dy;
    float radius2;
    zNPCEntity* npc;
    zWallNet* wallNet;

    if (isPerceived) {
        radius = radius * node->HysteresisRatio;
        heightUp = heightUp * node->HysteresisRatio;
        heightDown = heightDown * node->HysteresisRatio;
    }

    npc = ownerTarget->GetNPCEntity();
    npc->GetBoundCenter(npcCenter);
    ownerTarget->GetTargetEntityCenter(targetCenter);
    dy = targetCenter.y - npcCenter.y;

    if (dy < -heightDown || dy > heightUp) {
        return false;
    }

    radius2 = radius * radius;

    if (npcCenter.Distance2XZ(targetCenter) > radius2) {
        return false;
    }

    wallNet = GetNPCWallNet();

    if (wallNet != 0) {
        if (node->flags & 2) {
            if (!wallNet->IsInsideWallNetXZ(targetCenter)) {
                return false;
            }
        }

        if (node->flags & 4) {
            if (!IsInDirectPath(npc, wallNet)) {
                return false;
            }
        }
    }

    if (node->flags & 1) {
        if (!ownerTarget->losCache.CheckLineOfSight(
                npc, ownerTarget,
                (Sext::eCollisionLayer)node->LOSCollisionLayer, false)) {
            return false;
        }
    }

    return true;
}
bool zNPCPerceptionTarget::zPerceptionType::CheckInNPCWallsPerception(
    const Node* node) {
    xVec3 npcCenter;
    xVec3 targetCenter;
    float heightUp;
    float heightDown;
    float dy;
    zWallNet* wallNet = GetNPCWallNet();

    if (wallNet == 0) {
        return false;
    }

    heightUp = node->InNPCWalls.HeightUp;
    heightDown = node->InNPCWalls.HeightDown;

    if (isPerceived) {
        heightUp = heightUp * node->HysteresisRatio;
        heightDown = heightDown * node->HysteresisRatio;
    }

    ownerTarget->GetNPCEntity()->GetBoundCenter(npcCenter);
    ownerTarget->GetTargetEntityCenter(targetCenter);
    dy = targetCenter.y - npcCenter.y;

    if (dy < -heightDown || dy > heightUp) {
        return false;
    }

    return wallNet->IsInsideWallNetXZ(targetCenter);
}
bool zNPCPerceptionTarget::zPerceptionType::
    CheckAngularSpherePerceptionWithoutTargetBounds(const Node* node) {
    xVec3 npcCenter;
    xVec3 targetCenter;
    xVec3 delta;
    float tanHAngle = node->AngularSphere.tanHAngle;
    float radius = node->AngularSphere.Radius;
    float tan2;

    // The squared radius into a variable of ITS OWN, not back into
    // radius: reassigning costs seven words, because tanHAngle and
    // radius then swap floating-point registers. Twelve combinations
    // were measured -- three declaration orders, the two hysteresis
    // multiplies in either order, and this.
    float radius2;
    zNPCEntity* npc;
    zWallNet* wallNet;

    if (isPerceived) {
        tanHAngle = tanHAngle * node->HysteresisRatio;
        radius = radius * node->HysteresisRatio;
    }

    npc = ownerTarget->GetNPCEntity();
    npc->GetBoundCenter(npcCenter);
    ownerTarget->GetTargetEntityCenter(targetCenter);
    radius2 = radius * radius;
    delta.Sub(targetCenter, npcCenter);

    if (delta.length2() > radius2) {
        return false;
    }

    tan2 = zNPCHelper::GetTanTheta2(&npcCenter, &npc->model->forward,
                                    &targetCenter);

    if (tan2 < 0.0f) {
        return false;
    }

    if (tan2 > tanHAngle * tanHAngle) {
        return false;
    }

    wallNet = GetNPCWallNet();

    if (wallNet != 0) {
        if (node->flags & 2) {
            if (!wallNet->IsInsideWallNetXZ(targetCenter)) {
                return false;
            }
        }

        if (node->flags & 4) {
            if (!IsInDirectPath(npc, wallNet)) {
                return false;
            }
        }
    }

    if (node->flags & 1) {
        if (!ownerTarget->losCache.CheckLineOfSight(
                npc, ownerTarget,
                (Sext::eCollisionLayer)node->LOSCollisionLayer, false)) {
            return false;
        }
    }

    return true;
}
bool zNPCPerceptionTarget::zPerceptionType::
    CheckAngularCylinderPerceptionWithoutTargetBounds(const Node* node) {
    xVec3 npcCenter;
    xVec3 targetCenter;
    float tanHAngle = node->AngularCylinder.tanHAngle;
    float radius = node->AngularCylinder.Radius;
    float heightUp = node->AngularCylinder.HeightUp;
    float heightDown = node->AngularCylinder.HeightDown;
    float dy;
    float tanXZ;
    float radius2;
    zNPCEntity* npc;
    zWallNet* wallNet;

    if (isPerceived) {
        tanHAngle = tanHAngle * node->HysteresisRatio;
        radius = radius * node->HysteresisRatio;
        heightUp = heightUp * node->HysteresisRatio;
        heightDown = heightDown * node->HysteresisRatio;
    }

    npc = ownerTarget->GetNPCEntity();
    npc->GetBoundCenter(npcCenter);
    ownerTarget->GetTargetEntityCenter(targetCenter);
    dy = targetCenter.y - npcCenter.y;

    if (dy < -heightDown || dy > heightUp) {
        return false;
    }

    radius2 = radius * radius;

    if (npcCenter.Distance2XZ(targetCenter) > radius2) {
        return false;
    }

    tanXZ = zNPCHelper::GetTanThetaXZ(&npcCenter, &npc->model->forward,
                                     &targetCenter);

    if (tanXZ < 0.0f) {
        return false;
    }

    // Not squared: GetTanThetaXZ returns the tangent, where the sphere's
    // GetTanTheta2 returns its square.
    if (tanXZ > tanHAngle) {
        return false;
    }

    wallNet = GetNPCWallNet();

    if (wallNet != 0) {
        if (node->flags & 2) {
            if (!wallNet->IsInsideWallNetXZ(targetCenter)) {
                return false;
            }
        }

        if (node->flags & 4) {
            if (!IsInDirectPath(npc, wallNet)) {
                return false;
            }
        }
    }

    if (node->flags & 1) {
        if (!ownerTarget->losCache.CheckLineOfSight(
                npc, ownerTarget,
                (Sext::eCollisionLayer)node->LOSCollisionLayer, false)) {
            return false;
        }
    }

    return true;
}
bool zNPCPerceptionTarget::zPerceptionType::
    CheckAngularSpherePerceptionWithTargetBounds(const Node* node) {
    // 3 OF 137 WORDS, size exact. The ternary's temporary takes f30
    // where retail takes f31, and the squared distance is compared out
    // of f1 where retail compares a copy in f28. Four declaration orders
    // of the three float locals measure the same.
    xVec3 npcCenter;
    xVec3 targetCenter;
    xVec3 delta;
    float targetRadius;
    float tanHAngle;
    float radius;
    float sumRadius2;
    float dist2;
    float tan2;
    float distance;
    float tanBound;
    float expanded;
    zNPCEntity* npc;
    zWallNet* wallNet;

    // Whichever of the target's two bound radii is larger.
    targetRadius = ownerTarget->GetTargetEntityRadiusXZ() >
                           ownerTarget->GetTargetEntityRadiusY()
                       ? ownerTarget->GetTargetEntityRadiusXZ()
                       : ownerTarget->GetTargetEntityRadiusY();
    tanHAngle = node->AngularSphere.tanHAngle;
    radius = node->AngularSphere.Radius;

    if (isPerceived) {
        tanHAngle = tanHAngle * node->HysteresisRatio;
        radius = radius * node->HysteresisRatio;
    }

    npc = ownerTarget->GetNPCEntity();
    npc->GetBoundCenter(npcCenter);
    ownerTarget->GetTargetEntityCenter(targetCenter);
    sumRadius2 = (radius + targetRadius) * (radius + targetRadius);
    delta.Sub(targetCenter, npcCenter);
    dist2 = delta.length2();

    if (dist2 > sumRadius2) {
        return false;
    }

    tan2 = zNPCHelper::GetTanTheta2(&npcCenter, &npc->model->forward,
                                    &targetCenter);

    if (tan2 < 0.0f) {
        return false;
    }

    // The cone widened by the angle the target's own radius subtends:
    // tan(A + B) = (tanA + tanB) / (1 - tanA tanB).
    distance = dist2 * Math::rsqrt(dist2);
    tanBound = targetRadius / distance;
    expanded = (tanHAngle + tanBound) / (1.0f - tanHAngle * tanBound);

    if (tan2 > expanded * expanded) {
        return false;
    }

    wallNet = GetNPCWallNet();

    if (wallNet != 0) {
        if (node->flags & 2) {
            if (!wallNet->IsInsideWallNetXZ(targetCenter)) {
                return false;
            }
        }

        if (node->flags & 4) {
            if (!IsInDirectPath(npc, wallNet)) {
                return false;
            }
        }
    }

    if (node->flags & 1) {
        if (!ownerTarget->losCache.CheckLineOfSight(
                npc, ownerTarget,
                (Sext::eCollisionLayer)node->LOSCollisionLayer, false)) {
            return false;
        }
    }

    return true;
}
bool zNPCPerceptionTarget::zPerceptionType::
    CheckAngularCylinderPerceptionWithTargetBounds(const Node* node) {
    xVec3 npcCenter;
    xVec3 targetCenter;
    float radiusXZ;
    float radiusY;
    float tanHAngle;
    float radius;
    float heightUp;
    float heightDown;
    float dy;
    float sumRadius2;
    float dist2;
    float tanXZ;
    float distance;
    float tanBound;
    float expanded;
    zNPCEntity* npc;
    zWallNet* wallNet;

    // Both bound radii, each used for its own axis -- where the
    // angular SPHERE takes whichever is larger, the cylinder widens
    // its height band by the Y radius and its footprint by the XZ one.
    radiusXZ = ownerTarget->GetTargetEntityRadiusXZ();
    radiusY = ownerTarget->GetTargetEntityRadiusY();
    tanHAngle = node->AngularCylinder.tanHAngle;
    radius = node->AngularCylinder.Radius;
    heightUp = node->AngularCylinder.HeightUp;
    heightDown = node->AngularCylinder.HeightDown;

    if (isPerceived) {
        tanHAngle = tanHAngle * node->HysteresisRatio;
        radius = radius * node->HysteresisRatio;
        heightUp = heightUp * node->HysteresisRatio;
        heightDown = heightDown * node->HysteresisRatio;
    }

    npc = ownerTarget->GetNPCEntity();
    npc->GetBoundCenter(npcCenter);
    ownerTarget->GetTargetEntityCenter(targetCenter);
    dy = targetCenter.y - npcCenter.y;

    if (dy < -(heightDown + radiusY) || dy > heightUp + radiusY) {
        return false;
    }

    sumRadius2 = (radius + radiusXZ) * (radius + radiusXZ);
    dist2 = npcCenter.Distance2XZ(targetCenter);

    if (dist2 > sumRadius2) {
        return false;
    }

    tanXZ = zNPCHelper::GetTanThetaXZ(&npcCenter, &npc->model->forward,
                                      &targetCenter);

    if (tanXZ < 0.0f) {
        return false;
    }

    // The XZ distance, not the 3D one: the cylinder's whole cone is
    // measured in the plane.
    distance = dist2 * Math::rsqrt(dist2);
    tanBound = radiusXZ / distance;
    expanded = (tanHAngle + tanBound) / (1.0f - tanHAngle * tanBound);

    if (tanXZ > expanded) {
        return false;
    }

    wallNet = GetNPCWallNet();

    if (wallNet != 0) {
        if (node->flags & 2) {
            if (!wallNet->IsInsideWallNetXZ(targetCenter)) {
                return false;
            }
        }

        if (node->flags & 4) {
            if (!IsInDirectPath(npc, wallNet)) {
                return false;
            }
        }
    }

    if (node->flags & 1) {
        if (!ownerTarget->losCache.CheckLineOfSight(
                npc, ownerTarget,
                (Sext::eCollisionLayer)node->LOSCollisionLayer, false)) {
            return false;
        }
    }

    return true;
}
void zNPCPerception::Attached(const zNPCStatus* status) {
    // Declared here and in this order because that is the register
    // order: first declared takes the highest, r28 down to r23, and
    // index shares j's register since their lives do not overlap.
    Sext::NPCPerceptionAsset::PerceptionType* t;
    Sext::NPCPerceptionAsset::PerceptionNode* n;
    int count;
    int i;
    int nodeCount;
    int j;
    unsigned int index;

    if (perceptionAsset == 0) {
        SetAssetAuto(owner->asset);
    }

    count = perceptionAsset->typeCount;

    for (i = 0; i < count; i++) {
        t = &perceptionAsset->types[i];
        nodeCount = t->nodeCount;

        for (j = 0; j < nodeCount; j++) {
            n = &t->nodes[j];

            // The asset stores the cone's FULL angle in degrees; what
            // the checks want is the tangent of half of it, baked in
            // here once rather than computed per test.
            switch (n->Shape) {
            case 3:
                n->AngularSphere.tanHAngle = tan(
                    0.01745329238474369f * (0.5f * n->AngularSphere.Angle));
                break;
            case 4:
                n->AngularCylinder.tanHAngle =
                    tan(0.01745329238474369f *
                        (0.5f * n->AngularCylinder.Angle));
                break;
            }
        }
    }

    for (index = 0; index < 4; index++) {
        if (targets[index].targetEnt != 0) {
            targets[index].Setup(perceptionAsset, this);
        }
    }
}

bool zNPCPerception::SystemEvent(xBase* from, xBase* to,
                                 unsigned int event,
                                 Sext::EventAny* param) {
    switch (event) {
    case 0x6A4E593B: {
        xBase* obj = 0;

        if (param != 0 && param->id != 0) {
            obj = zSceneFindObject(param->id);

            if (obj == 0) {
                // Into a local first: retail loads both halves before
                // it calls GetEntityManager, which is only forced by
                // something that has to stay live across that call.
                unsigned long long id = param->id;

                obj = (xBase*)World::GetEntityManager()->FindAsset(id);
            }
        }

        if (obj == 0 || obj->baseType != 0x62) {
            return true;
        }

        npcWallNet = (zWallNet*)obj;
        return false;
    }
    case 0xF94E58D9:
        status = 2;
        return false;
    case 0xBE13945E:
        status = 1;
        return false;
    case 0xF306E4B9:
        status = 0;
        return false;
    }

    return false;
}
int zNPCPerception::GetTargetIndexClosestMatching(
    Sext::eNPCPerceptionType type, bool perceivedOnly) {
    // Both halves of this declaration block were measured: all 24
    // orders of the four locals, each with i initialised at its
    // declaration and not, and exactly one of the 48 matches. The
    // order gives the registers -- first declared takes the highest,
    // r29 down to r26 -- and initialising i where it is declared is
    // what puts its li above the GetPosition call rather than after.
    xVec3 npcPos;
    xVec3 delta;
    xEnt* ent;
    int i = 0;
    int best;
    bool haveBest;
    float bestDist2;

    best = -1;
    haveBest = false;

    owner->GetPosition(npcPos);

    for (; i < 4; i++) {
        ent = targets[i].targetEnt;

        if (ent == 0) {
            continue;
        }

        // END_eNPCPerception_ENUM means any target will do, so the
        // perception query is skipped entirely rather than asked.
        if (type != Sext::END_eNPCPerception_ENUM) {
            if (AreTargetsPerceived(1 << i, type, false) !=
                perceivedOnly) {
                continue;
            }
        }

        delta.Sub(ent->model->position, npcPos);

        float dist2 = delta.length2();

        if (!haveBest || bestDist2 > dist2) {
            bestDist2 = dist2;
            best = i;
            haveBest = true;
        }
    }

    return best;
}

bool zNPCPerception::AreTargetsPerceived(unsigned int mask,
                                         Sext::eNPCPerceptionType type,
                                         bool all) {
    unsigned int shift = 0;

    while (mask != 0) {
        unsigned int bit = mask & 1;

        if (bit != 0) {
            // (1 << shift) - 1, not shift: that is what the image
            // computes, and the function is byte-identical with it.
            // It agrees with the index only for the low two bits, so
            // a mask naming target 2 or 3 reads past the array. Left
            // as retail has it.
            unsigned int index = (bit << shift) - 1;

            if (status == 0) {
                bool perceived = targets[index].IsPerceived(type);

                if (all && !perceived) {
                    return false;
                }

                if (!all && perceived) {
                    return true;
                }
            } else {
                if (!all && status == 2) {
                    return true;
                }

                if (all && status != 2) {
                    return false;
                }
            }
        }

        mask = mask >> 1;
        shift++;
    }

    return all;
}
bool zNPCPerceptionTarget::zLOSCache::CheckLineOfSight(
    const zNPCEntity* npc, zNPCPerceptionTarget* target,
    Sext::eCollisionLayer layer, bool force) {
    xVec3 npcCenter;
    xVec3 targetCenter;
    xVec3 bonePos;
    xVec3 center;
    xEnt* ent;
    int count;
    int i;

    // THE CACHE NEVER ANSWERS. Every path through this sets the flag
    // to true, so the return below it is unreachable and the ray test
    // runs on every call; the timer is still written and still counted
    // down by PostUpdate. Three li r0,1 and no li r0,0, read out of
    // the image and confirmed against the raw DOL word at 0x800F7718
    // rather than the disassembler. Left as retail has it.
    bool recheck = true;

    if (force) {
        recheck = true;
    } else if (LOSTimer < 0.0f) {
        recheck = true;
    }

    if (!recheck) {
        return isInLOS;
    }

    LOSTimer = 0.5f;
    npc->GetBoundCenter(npcCenter);
    ent = target->targetEnt;

    // Only the player gets the extra spheres and the bone: everything
    // else is one ray at its centre.
    if (ent->baseType == 0x55) {
        target->GetTargetEntityCenter(targetCenter);

        if (zNPCPerception::CheckLineOfSight(&npcCenter, &targetCenter,
                                            npc, ent, layer)) {
            isInLOS = true;
            return true;
        }

        count = ((zPlayer*)ent)->numExtraSpheres;

        for (i = 0; i < count; i++) {
            if (zNPCPerception::CheckLineOfSight(
                    &npcCenter,
                    &((zPlayer*)ent)->extraSpheres[i].center, npc, ent,
                    layer)) {
                isInLOS = true;
                return true;
            }
        }

        if (xModelGetBoneCount(ent->model) > 3) {
            xModelGetBoneLocationNoScale(bonePos, *ent->model, 3);
        } else {
            xModelGetBoneLocationNoScale(bonePos, *ent->model, 0);
        }

        if (zNPCPerception::CheckLineOfSight(&npcCenter, &bonePos, npc,
                                            ent, layer)) {
            isInLOS = true;
            return true;
        }

        isInLOS = false;
        return false;
    }

    target->GetTargetEntityCenter(center);
    isInLOS = zNPCPerception::CheckLineOfSight(&npcCenter, &center, npc,
                                              ent, layer);

    return isInLOS;
}
// 69 WORDS OF RETAIL'S 70, AND THE SOURCE IS NOT WHAT IS WRONG. This
// function reads three constants out of .rodata -- 0.0f, 1e-05f and
// 1.0f -- and mwcc anchors a base register at the SECTION and bakes
// the displacements in, which costs an addi and takes r31, so pos
// lands in r30 and every register below it shifts. Retail materialises
// a high half per reference instead. The difference is not in the
// source: compiled with 36,000 bytes of .rodata placed AHEAD of the
// constants, so that no signed 16-bit displacement can reach them from
// the section base, this function is byte-identical at 280 -- measured,
// not argued. So retail's WAD02.cpp carries at least 32KB of .rodata
// before these three, and our split of it carries 0x18 bytes in total.
// That is the same class of thing as the units that reproduce OFFSETS
// rather than a placeable layout, and padding the section to fake it
// would put data here that the manifest does not name.
bool zNPCPerceptionTarget::zPerceptionType::IsInDirectPath(
    const zNPCEntity* npc, const zWallNet* wallNet) {
    xVec3 targetCenter;
    xVec3 delta;
    const zWallNetPositionXZ* pos;
    float radius;
    float distance;

    ownerTarget->GetTargetEntityCenter(targetCenter);
    pos = npc->owner->npcSteering->GetWallNetPosition();
    radius = npc->npcBound.GetBoundRadiusXZ();
    delta.Sub(targetCenter, pos->curPos);

    // Flattened: the sweep is a circle on the wall net, so the height
    // difference plays no part in either the direction or the length.
    delta.y = 0.0f;
    distance = delta.length2();
    distance = distance * Math::rsqrt(distance);

    if (distance < 1e-05f) {
        return true;
    }

    delta *= 1.0f / distance;

    zWallNetCollis collis;

    collis.Reset();

    return !zIWallNet::IntersectsSweptCircle(wallNet, pos, &delta, radius,
                                             distance, collis);
}
// DEFINED here, below every caller: with the bodies above them the
// auto-inliner takes both into the perception checks, where retail
// calls them (NOTES, where the body sits in the file). Anything written
// AFTER them inlines them again -- keep them last.
zWallNet* zNPCPerceptionTarget::zPerceptionType::GetNPCWallNet() {
    return ownerTarget->ownerNpcPerc->npcWallNet;
}

zNPCEntity* zNPCPerceptionTarget::GetNPCEntity() {
    return ownerNpcPerc->owner->npcEnt;
}
