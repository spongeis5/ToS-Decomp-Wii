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
// runs from +4 to +0x1B.
//
// The float literals are read from .rodata rather than named by address:
// a fresh perception type starts at FLT_MAX (never perceived), goes to
// 0.0f the moment it is, and a target's line-of-sight cache starts at
// -0.1f so the first check always runs.

class xBase;
class zWallNet;

// DisableTarget sends its event from the NPC's own base entity, which
// the disassembly reaches at +0xBC of the zNPCEntity.
class zNPCEntity {
public:
    unsigned char _pad0[0xBC];
    xBase* baseEnt;
};

class xVec3 {
public:
    xVec3& operator=(const xVec3& other);

    static const xVec3 m_Null;

    float x;
    float y;
    float z;
};

class xSphere {
public:
    xVec3 center;
    float r;
};

namespace World {

class xOGModel {
public:
    unsigned char _pad0[0x30];
    xVec3 position;
};

}  // namespace World

void xModelGetBoneLocationNoScale(xVec3& out, const World::xOGModel& model,
                                  unsigned long bone);

class zNPCBound {
public:
    float GetBoundRadiusXZ() const;

    unsigned char _pad0[0x10];
    float radiusY;
};

class xHavokPhysicsObject {
public:
    void GetBoundingSphere(xSphere* out) const;
};

// The four ids the geometry accessors switch on are xBase type
// constants; the image gives the values and the order of the tests, not
// the names.
class xEnt {
public:
    unsigned char _pad0[0x20];
    unsigned int baseType;
    unsigned char _pad1[0x34 - 0x24];
    World::xOGModel* model;
    unsigned char _pad2[0x80 - 0x38];
    xHavokPhysicsObject physics;
    unsigned char _pad3[0xF4 - 0x81];
    zNPCBound npcBound;
    unsigned char _pad4[0x214 - 0x108];
    float radius;
};

class zWallNetCollis {
public:
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

class EventAny;

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

// zEntEvent(from, fromEvent, to, toEvent, param, force) -- the disable
// path sends one event with no parameter and the force flag set.
enum ForceEvent { ForceEvent_ = 0x7FFFFFFF };

void zEntEvent(xBase* from, unsigned int fromEvent, xBase* to,
               unsigned int toEvent, Sext::EventAny* param, ForceEvent force);

class zNPCBase {
public:
    unsigned char _pad0[0x98];
    zNPCEntity* npcEnt;
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

class zNPCPerception;

class zNPCPerceptionTarget {
public:
    class zPerceptionType {
    public:
        void Update();
        zWallNet* GetNPCWallNet();
        bool CheckNodePerception(
            const Sext::NPCPerceptionAsset::PerceptionNode* node);
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

void zNPCPerceptionTarget::DisableTarget() {
    if (targetEnt != 0) {
        zEntEvent((xBase*)ownerNpcPerc->owner->npcEnt->baseEnt, 0,
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

zNPCEntity* zNPCPerceptionTarget::GetNPCEntity() {
    return ownerNpcPerc->owner->npcEnt;
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
        return ent->radius;
    case 0x38:
        return ent->npcBound.GetBoundRadiusXZ();
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
        return ent->radius;
    case 0x38:
        return ent->npcBound.radiusY;
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

zWallNet* zNPCPerceptionTarget::zPerceptionType::GetNPCWallNet() {
    return ownerTarget->ownerNpcPerc->npcWallNet;
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
