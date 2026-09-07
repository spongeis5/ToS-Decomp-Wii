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

class xEnt;
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
    float x;
    float y;
    float z;
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

class zNPCComponent {
public:
    zNPCBase* owner;
    unsigned char _pad0[0x8 - 0x4];
};

class zNPCPerception;

class zNPCPerceptionTarget {
public:
    class zPerceptionType {
    public:
        void Update();
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

    xEnt* targetEnt;
    zNPCPerception* ownerNpcPerc;
    zPerceptionType types[6];
    zLOSCache losCache;
    bool perceivedAny;
    unsigned char _pad0[0x74 - 0x71];
};

class zNPCPerception : public zNPCComponent {
public:
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

