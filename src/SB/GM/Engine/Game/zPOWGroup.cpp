#include "SB/GM/Engine/Game/zPOWGroup.pool.h"

// zPOWGroup.cpp -- a group of physics objects spawned in a box, sphere or
// point volume. Read from the image with tools/disasm.py and tools/brief.py;
// the layouts are the DWARF's (zPOWGroup 0x100, zPOWGroupAsset 0xA0,
// zPOWObjectAsset 0x78, zPhysicsObject 0x60), the virtual slots the image's
// (__vt__9zPOWGroup: Init 20, Reset 21, DebugReset 22).
//
// The unit was a tools/gen_accessors.py stub holding SceneExit; it is kept.
//
// Not written, sorted from the unit's listing:
// - UpdatePositions (9 distinct float literals) and Update (8): the
//   four-literal wall.
// - Init loads FloatingCollectibleEventWrapper from WAD02.cpp's anonymous
//   namespace, which a fragment cannot name.
//
// Order: the functions that need #pragma always_inline (Create, HandleEvent,
// Settle, Activate) sit above every ordinary function they call, because the
// pragma inlines those too and retail calls them.

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {

enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };
enum eFactoryMemType { eFactoryMemType_ = 0x7FFFFFFF };

void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap, eMemMgrTag tag,
                      bool clear);

class Factory {
public:
    void* AllocMem(unsigned int size, eFactoryMemType type);
    void DeallocMem(void* block);

    unsigned char _pad0[0x3C];
};

}  // namespace Memory

extern "C" {
void* memset(void* dst, int c, unsigned long n);
}

inline void* operator new(unsigned long, void* p) { return p; }

void* xMemAlloc(Memory::GlobalHeapEnum heap, unsigned int size, int align,
                eMemMgrTag tag);

// ---------------------------------------------------------------------------
// Vectors and matrices

class xVec3 {
public:
    xVec3& operator=(const xVec3& other);
    xVec3& operator*=(float s);
    void Sub(const xVec3& a, const xVec3& b);
    float length2() const;
    void normalize();

    static const xVec3 m_UnitAxisX;
    static const xVec3 m_UnitAxisY;
    static const xVec3 m_UnitAxisZ;

    float x;
    float y;
    float z;
};

class xMat3x3 {
public:
    xVec3 right;
    int flags;
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

extern const xMat4x3 g_I3;

void xMat4x3Invert(xMat4x3* o, const xMat4x3* m);
void xMat4x3Mul(xMat4x3* o, const xMat4x3* a, const xMat4x3* b);

// Linker-folded names the image's calls carry.
extern "C" void __ct__Q24Math6VectorFfff(void* v, float x, float y, float z);
extern "C" void __ct__Q24Math8Matrix33Fv(void* self);

namespace Math {

class Vector4POD {
public:
    float data[4];
};

class Vector4 {
public:
    // Weak in the image and called out of line: defined at the foot.
    Vector4(const Vector4POD& v);

    Vector4& Assign(float x, float y, float z, float w);

    float data[4];
};

float sqrt(float x);

}  // namespace Math

// ---------------------------------------------------------------------------
// Havok

class hkBool {
public:
    operator bool() const { return m_bool != 0; }

    char m_bool;
};

class hkVector4 {
public:
    float x __attribute__((aligned(16)));
    float y;
    float z;
    float w;
};

class hkVector4Init : public hkVector4 {
public:
    hkVector4Init(const xVec3& v) {
        ((Math::Vector4*)this)->Assign(v.x, v.y, v.z, 0.0f);
    }
};

class hkpEntity {
public:
    hkBool isActive() const;
    void activate();
    void deactivate();
};

class hkpRigidBody : public hkpEntity {};

void xHavok_ApplyLinearImpulse(hkpRigidBody* body, const hkVector4& impulse);

// ---------------------------------------------------------------------------
// World

class xBase;
class xEnt;
class xEntFrame;
class zPOWGroup;
class zPhysicsObject;
class zTrigger;

class dxTriMeshData {
public:
    float AABBCenter[3];
    float AABBExtents[3];
    void* triInfo;
};

namespace World {

class Entity;
class EntityHandleBase;

class ModelInstanceAsset {
public:
    unsigned char _pad0[0x40];
};

class BlobEntity {
public:
    unsigned char _pad0[0x18];
};

class CollisionMeshBlobEntity : public BlobEntity {
public:
    dxTriMeshData triMesh;
};

class ModelPrototypeEntity {
public:
    unsigned char _pad0[0x6C];
    CollisionMeshBlobEntity* collmeshBlob;
};

class EntityManager {
public:
    static void* FindAsset(unsigned long long id);
};

EntityManager* GetEntityManager();

class xOGModel {
public:
    xMat4x3 Mat;
};

}  // namespace World

class xOGRenderHelper {
public:
    static World::ModelPrototypeEntity* GetModelPrototypeEntity(
        const World::ModelInstanceAsset* asset);
};

class UIDModelReference {
public:
    unsigned int nameHash;
    unsigned int dummy1;
    unsigned char _pad0[0x10 - 0x8];
    World::ModelInstanceAsset mia;
};

// ---------------------------------------------------------------------------
// Assets

namespace Sext {

class EventAny {};

class uid {
public:
    operator unsigned long long() const { return internalUid; }

    unsigned long long internalUid;
};

enum magCharge { Repel = -1, Neutral = 0, Attract = 1, END_magChargeENUM = 2 };
enum eCollisionLayer { eCollisionLayer_ = 0x7FFFFFFF };

class xBaseAsset {
public:
    uid id;
    unsigned int baseType;
    unsigned short linkCount;
    unsigned short baseFlags;
};

class xBaseScene : public xBaseAsset {};

class LinkAsset {
public:
    unsigned int count;
    void* data;
};

class LinkEvent {
public:
    int type;
    void* v;
};

class LinkAssetBaseNew {
public:
    LinkEvent srcEvent;
    LinkEvent dstEvent;
    uid dstAssetID;
    uid chkAssetID;
    bool chkSourceParams;
    bool disabled;
    unsigned int chkSourceMask;
};

class EventActionNew : public EventAny {};

class EventActionDrivenBy : public EventActionNew {
public:
    bool param0;
    bool param1;
    bool param2;
    bool cam;
    uid specificPassenger;
    int bone;
};

class vec3 {
public:
    float x;
    float y;
    float z;
};

class Matrix43POD {
public:
    Math::Vector4POD v[3];
};

class Box_Type {
public:
    Matrix43POD Transform;
    int NumX;
    int NumY;
    int NumZ;
};

class Sphere_Type {
public:
    vec3 Center;
    float Radius;
};

class Point_Type {
public:
    vec3 Position;
};

class PhysicsDataStruct {
public:
    float mass;
    float friction;
    float elasticity;
    float linearDamping;
    float angularDamping;
    float settleTimer;
    unsigned char magnetic_Charge;
    unsigned char pad1;
    unsigned char pad2;
    unsigned char pad3;
};

class zPOWObjectAsset : public xBaseAsset {
public:
    uid ModelInstanceReference;
    bool InfiniteLife;
    PhysicsDataStruct PhysicsData;
};

class zPOWGroupAsset : public xBaseScene {
public:
    static World::Entity* Create(World::EntityHandleBase* handle,
                                 zPOWGroupAsset* inAsset);

    LinkAsset EventLinksNew;
    int NumObj;
    uid POWObj;
    unsigned char SpawnType;
    unsigned char VolumeType;
    uid DestructibleObj;
    unsigned int shadowColor;
    float shadowMaxDepth;
    float shadowStartDepth;
    unsigned int shadowMinBlur;
    unsigned int shadowMaxBlur;
    LinkAsset triggers;
    unsigned int initialCollisionFilter;
    unsigned char _pad0[0x60 - 0x58];
    union {
        Box_Type Box;
        Sphere_Type Sphere;
        Point_Type Point;
    };
};

}  // namespace Sext

// ---------------------------------------------------------------------------
// Entities

class EmbeddedListNode {
public:
    EmbeddedListNode* next;
    EmbeddedListNode* prev;
};

// The slots of __vt__9zPOWGroup this unit calls. Data-free, so the vtable
// pointer lands at +0.
class EntityVirtuals {
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
    virtual void _v11();
    virtual void _v12();
    virtual void _v13();
    virtual void _v14();
    virtual void _v15();
    virtual void _v16();
    virtual void _v17();
    virtual void _v18();
    virtual void _v19();
    virtual void Init(Sext::zPOWGroupAsset* asset);
    virtual void Reset();
    virtual void DebugReset();
};

// Packed to four: the id's eight-byte alignment would otherwise round xBase
// up past the model handle the DWARF puts at +0x34.
#pragma pack(push, 4)

namespace World {

class Entity : public EntityVirtuals {
public:
    EmbeddedListNode ogSceneNode;
    int ogUpdateIdx;
    unsigned int typeID;
    EntityHandleBase* handle;
};

}  // namespace World

class xBase : public World::Entity {
public:
    unsigned long long id;
    unsigned int baseType;
    unsigned char UNUSED_linkCount;
    unsigned char assertFlags;
    unsigned short baseFlags;
    Sext::LinkAsset* linkArray;
    void* templateParent;
    void* eventFunc;
};

namespace World {

class xOGModelRef {
public:
    xOGModel* data;
    void* autoptr;
};

class xOGModelHandle : public xOGModelRef {};

class xOGEntity : public xBase {
public:
    xOGEntity(EntityHandleBase* handle);

    xOGModelHandle ogModel;
};

}  // namespace World

#pragma pack(pop)

class xEnt : public World::xOGEntity {
public:
    unsigned char _pad0[0x58 - 0x3C];
    xEntFrame* frame;
};

class zDestructible {
public:
    bool active;
};

class zEntSimpleObj {
public:
    unsigned char _pad0[0xCC];
    zDestructible* destructible;
};

class zPhysicsObjectEntity {
public:
    void* modelRenderData;
    float boundRadius;
    float alpha;
    unsigned char renderEnabled;
    xMat4x3 matrix;
    xVec3 origPos;
    World::ModelInstanceAsset* modelInstanceAsset;
    void* modelInstanceArticle;
};

class HavokSimObj {
public:
    hkpRigidBody* pRigidBody;
};

class zPhysicsObject : public xBase {
public:
    zPhysicsObject(World::EntityHandleBase* handle);

    bool SetupPhysicsObject(zPOWGroup* owner, World::ModelInstanceAsset* asset,
                            xVec3* pos, float linearDamping,
                            float angularDamping, float elasticity,
                            float friction, float mass, bool settle,
                            Sext::magCharge charge, bool collisionDisabled,
                            Sext::eCollisionLayer layer);
    bool Reset(zPOWGroup* owner, World::ModelInstanceAsset* asset, xVec3* pos,
               float linearDamping, float angularDamping, float elasticity,
               float friction, float mass, bool settle, Sext::magCharge charge,
               bool collisionDisabled, Sext::eCollisionLayer layer);
    void CreateComponents();
    void CleanupPhysicsObject();
    void DestroyComponents();
    void RemoveFromSimWorld();

    xVec3* GetPos() { return entity ? &entity->matrix.pos : 0; }

    zPhysicsObject* nextPhysicsObject;
    zPOWGroup* owner;
    int initialCollisionLayer;
    float lifetime;
    zPhysicsObjectEntity* entity;
    unsigned int collFilterInfo;
    int count;
    HavokSimObj havokSimObj;
    unsigned char flags;
    void* soundSourcesPhysics;
    unsigned char _pad0[0x60 - 0x5C];
};

class zPOWManager {
public:
    static Memory::Factory factory;
};

// The handle is bound before the allocation, as a parameter is.
inline zPhysicsObject* zPOWCreatePhysicsObject(World::EntityHandleBase* handle) {
    void* mem = zPOWManager::factory.AllocMem(sizeof(zPhysicsObject),
                                              (Memory::eFactoryMemType)24);

    return !mem ? 0 : new (mem) zPhysicsObject(handle);
}

// Each kept as a flag, as retail builds them.
inline bool zPOWHasRigidBody(zPhysicsObject* obj) {
    return obj != 0 && obj->havokSimObj.pRigidBody != 0;
}

inline bool zPOWIsActive(zPhysicsObject* obj) {
    return zPOWHasRigidBody(obj) && obj->havokSimObj.pRigidBody->isActive();
}

inline bool zPOWIsInactive(zPhysicsObject* obj) {
    return zPOWHasRigidBody(obj) && !obj->havokSimObj.pRigidBody->isActive();
}

class zPlayerContainer {
public:
    World::xOGEntity* playerArray[4];
    int numPlayers;
};

class xGlobals {
public:
    unsigned char _pad0[0x428];
    zPlayerContainer players;
};

extern xGlobals* xglobals;

// Combat reads the player's position through it: retail loads this chain
// before the object-position test, which the chain spelled in place does not.
inline const xVec3& zPOWPlayerPos() {
    return xglobals->players.playerArray[0]->ogModel.data->Mat.pos;
}

void xBaseReset(xBase* b, Sext::xBaseAsset* asset);
xBase* zSceneFindEntity(unsigned long long id);

// ---------------------------------------------------------------------------
// The group

class zPOWGroup : public World::xOGEntity {
public:
    enum positionDestination { CurrentPosition = 0, OriginalPosition = 1 };

    zPOWGroup(World::EntityHandleBase* handle)
        : World::xOGEntity(handle), asset(0), driver(0), driveMode(false),
          powObjAsset(0) {}

    // First among the virtuals and not defined here, so the table is not
    // emitted with this unit's definitions.
    virtual void Init(Sext::zPOWGroupAsset* asset);
    virtual void Reset();
    virtual void DebugReset();

    void InitPositions(dxTriMeshData* mesh);
    void SetMatrix(dxTriMeshData* mesh);
    void UpdatePositions(positionDestination dest, xMat4x3* newMat);
    void Setup();
    void Activate();
    void ResetPhysObj();
    void Combat(xVec3* center, float radiusSq);
    void GetBoundSphere(xVec3& center, float& radius);
    void Update(float dt);
    void RemoveFromSimWorld();
    void Unactivate();
    void Remove(zPhysicsObject* obj);
    void SceneExit();
    void HandleEvent(xBase* from, unsigned int toEvent,
                     Sext::EventAny* params);
    void Settle();
    void SetupDrivenBy();

    Sext::zPOWGroupAsset* asset;
    zEntSimpleObj* destObj;
    bool collisionDisabled;
    int countdownToActivation;
    xMat4x3 mat;
    float moveToTime;
    zPhysicsObject** objPtrs;
    int numObjs;
    int collidedWithPlayer;
    int collidedWithPlayerHandled;
    int lifetimeSet;
    int infiniteLife;
    float autoSettleTimer;
    zTrigger** triggers;
    xEnt* driver;
    xMat4x3 driveInfoMat;
    bool driveMode;
    bool wasEnabled;
    Sext::zPOWObjectAsset* powObjAsset;
    unsigned char _pad0[0x100 - 0xFC];
};

// ---------------------------------------------------------------------------

#pragma push
#pragma always_inline on
World::Entity* Sext::zPOWGroupAsset::Create(World::EntityHandleBase* handle,
                                            zPOWGroupAsset* inAsset) {
    zPOWGroup* entity = new (memset(
        Memory::AllocGlobalHeap(sizeof(zPOWGroup), (Memory::GlobalHeapEnum)0,
                                (eMemMgrTag)16, false),
        0, sizeof(zPOWGroup))) zPOWGroup(handle);

    entity->Init(inAsset);

    return entity;
}
#pragma pop

// NEAR MISS: 8 of 85 words differ; objPtrs[i] and the inner has-body flag take
// r4 and r3 where retail has r3 and r4 (tried: return a && b, bool ret = false
// with an if, a member HasRigidBody).
#pragma push
#pragma always_inline on
void zPOWGroup::HandleEvent(xBase* from, unsigned int toEvent,
                            Sext::EventAny* params) {
    switch (toEvent) {
    case 0x389E01C0:
        DebugReset();
        break;

    case 0xA8B93047:
        Reset();
        break;

    case 0x2C9D0683:
        if (!wasEnabled) {
            wasEnabled = true;
            baseFlags |= 1;
            Activate();
        }
        break;

    case 0x8662F06A:
        Unactivate();
        break;

    case 0x90A29826:
        for (int i = 0; i < numObjs; i++) {
            if (zPOWIsInactive(objPtrs[i])) {
                objPtrs[i]->havokSimObj.pRigidBody->activate();
            }
        }
        break;

    case 0xCE1BA8CF:
        Settle();
        break;
    }
}
#pragma pop

// NEAR MISS: 8 of 42 words differ; the same r3/r4 swap as HandleEvent.
#pragma push
#pragma always_inline on
void zPOWGroup::Settle() {
    for (int i = 0; i < numObjs; i++) {
        if (zPOWIsActive(objPtrs[i])) {
            objPtrs[i]->havokSimObj.pRigidBody->deactivate();
        }
    }
}
#pragma pop

void zPOWGroup::Setup() {
    // The base Setup: an empty function the linker folded.
    __ct__Q24Math8Matrix33Fv(this);

    powObjAsset = (Sext::zPOWObjectAsset*)World::GetEntityManager()->FindAsset(
        asset->POWObj);

    UIDModelReference* MIAsset =
        (UIDModelReference*)World::GetEntityManager()->FindAsset(
            powObjAsset->ModelInstanceReference);

    xOGRenderHelper::GetModelPrototypeEntity(&MIAsset->mia);

    destObj = 0;

    unsigned int destructibleAssetID = asset->DestructibleObj;

    if (destructibleAssetID) {
        xEnt* ent = (xEnt*)zSceneFindEntity(destructibleAssetID);

        if (ent) {
            destObj = (zEntSimpleObj*)ent;
        }
    }

    if (baseFlags & 1) {
        Activate();
    }
}

// NEAR MISS: 40 of 138 words differ; the mesh pointer takes r30 and this r29
// where retail has this in r30 and the mesh in r26, and the temporaries between
// sit one register lower (tried: pointer local, reference local, declared at
// the top, a CollisionMeshBlobEntity local, no mesh local, no proto local).
#pragma push
#pragma always_inline on
void zPOWGroup::Activate() {
    UIDModelReference* MIAsset =
        (UIDModelReference*)World::GetEntityManager()->FindAsset(
            powObjAsset->ModelInstanceReference);

    World::ModelInstanceAsset* modelInstanceAsset = &MIAsset->mia;

    World::ModelPrototypeEntity* proto =
        xOGRenderHelper::GetModelPrototypeEntity(modelInstanceAsset);

    dxTriMeshData* triMesh = &proto->collmeshBlob->triMesh;

    collidedWithPlayer = 0;
    collidedWithPlayerHandled = 0;
    infiniteLife = powObjAsset->InfiniteLife;
    lifetimeSet = 0;
    autoSettleTimer = powObjAsset->PhysicsData.settleTimer;

    collisionDisabled = false;
    if (destObj && destObj->destructible && destObj->destructible->active) {
        collisionDisabled = true;
    }

    for (int i = 0; i < numObjs; i++) {
        zPhysicsObject* obj = zPOWCreatePhysicsObject(handle);

        if (obj) {
            obj->id = asset->POWObj;
            obj->CreateComponents();
            objPtrs[i] = obj;
        }
    }

    InitPositions(triMesh);
    SetupDrivenBy();

    for (int i = 0; i < numObjs; i++) {
        if (objPtrs[i]) {
            bool success = objPtrs[i]->SetupPhysicsObject(
                this, modelInstanceAsset, &objPtrs[i]->entity->matrix.pos,
                powObjAsset->PhysicsData.linearDamping,
                powObjAsset->PhysicsData.angularDamping,
                powObjAsset->PhysicsData.elasticity,
                powObjAsset->PhysicsData.friction,
                powObjAsset->PhysicsData.mass,
                powObjAsset->PhysicsData.settleTimer == 0.0f,
                (Sext::magCharge)powObjAsset->PhysicsData.magnetic_Charge,
                collisionDisabled,
                (Sext::eCollisionLayer)asset->initialCollisionFilter);

            if (!success) {
                zPhysicsObject* curObj = objPtrs[i];
                curObj->CleanupPhysicsObject();
                curObj->DestroyComponents();
                zPOWManager::factory.DeallocMem(curObj);
                objPtrs[i] = 0;
            }
        }
    }

    countdownToActivation = -1;
}
#pragma pop

void zPOWGroup::InitPositions(dxTriMeshData* mesh) {
    SetMatrix(mesh);
    UpdatePositions(CurrentPosition, 0);

    for (int i = 0; i < numObjs; i++) {
        objPtrs[i]->entity->origPos = objPtrs[i]->entity->matrix.pos;
    }
}

void zPOWGroup::SetMatrix(dxTriMeshData* mesh) {
    if (numObjs != 0) {
        if (asset->VolumeType == 0) {
            Math::Vector4 row_1(asset->Box.Transform.v[0]);
            Math::Vector4 row_2(asset->Box.Transform.v[1]);
            Math::Vector4 row_3(asset->Box.Transform.v[2]);

            __ct__Q24Math6VectorFfff(&mat.right, row_1.data[0], row_2.data[0],
                                     row_3.data[0]);
            __ct__Q24Math6VectorFfff(&mat.up, row_1.data[1], row_2.data[1],
                                     row_3.data[1]);
            __ct__Q24Math6VectorFfff(&mat.at, row_1.data[2], row_2.data[2],
                                     row_3.data[2]);
            __ct__Q24Math6VectorFfff(&mat.pos, row_1.data[3], row_2.data[3],
                                     row_3.data[3]);
        } else if (asset->VolumeType == 1) {
            __ct__Q24Math6VectorFfff(&mat.right, xVec3::m_UnitAxisX.x,
                                     xVec3::m_UnitAxisX.y,
                                     xVec3::m_UnitAxisX.z);
            __ct__Q24Math6VectorFfff(&mat.up, xVec3::m_UnitAxisY.x,
                                     xVec3::m_UnitAxisY.y,
                                     xVec3::m_UnitAxisY.z);
            __ct__Q24Math6VectorFfff(&mat.at, xVec3::m_UnitAxisZ.x,
                                     xVec3::m_UnitAxisZ.y,
                                     xVec3::m_UnitAxisZ.z);
            __ct__Q24Math6VectorFfff(&mat.pos, asset->Sphere.Center.x,
                                     asset->Sphere.Center.y,
                                     asset->Sphere.Center.z);
        } else if (asset->VolumeType == 2) {
            __ct__Q24Math6VectorFfff(&mat.right, xVec3::m_UnitAxisX.x,
                                     xVec3::m_UnitAxisX.y,
                                     xVec3::m_UnitAxisX.z);
            __ct__Q24Math6VectorFfff(&mat.up, xVec3::m_UnitAxisY.x,
                                     xVec3::m_UnitAxisY.y,
                                     xVec3::m_UnitAxisY.z);
            __ct__Q24Math6VectorFfff(&mat.at, xVec3::m_UnitAxisZ.x,
                                     xVec3::m_UnitAxisZ.y,
                                     xVec3::m_UnitAxisZ.z);
            __ct__Q24Math6VectorFfff(&mat.pos, asset->Point.Position.x,
                                     asset->Point.Position.y,
                                     asset->Point.Position.z);
        }
    }
}

void zPOWGroup::Reset() {
    xBaseReset(this, asset);
    RemoveFromSimWorld();

    autoSettleTimer = powObjAsset->PhysicsData.settleTimer;
}

void zPOWGroup::DebugReset() {
    asset = (Sext::zPOWGroupAsset*)World::GetEntityManager()->FindAsset(
        asset->id);
    linkArray = &asset->EventLinksNew;
    powObjAsset = (Sext::zPOWObjectAsset*)World::GetEntityManager()->FindAsset(
        asset->POWObj);

    Reset();
    Unactivate();

    int oldNumObjs = numObjs;
    numObjs = asset->NumObj;

    if (numObjs > oldNumObjs) {
        objPtrs = (zPhysicsObject**)xMemAlloc(
            (Memory::GlobalHeapEnum)0, numObjs * sizeof(zPhysicsObject*), 0,
            (eMemMgrTag)76);
    }

    memset(objPtrs, 0, numObjs * sizeof(zPhysicsObject*));

    Activate();
}

void zPOWGroup::ResetPhysObj() {
    UIDModelReference* MIAsset =
        (UIDModelReference*)World::GetEntityManager()->FindAsset(
            powObjAsset->ModelInstanceReference);

    World::ModelInstanceAsset* modelInstanceAsset = &MIAsset->mia;

    World::ModelPrototypeEntity* proto =
        xOGRenderHelper::GetModelPrototypeEntity(modelInstanceAsset);

    InitPositions(&proto->collmeshBlob->triMesh);

    collidedWithPlayer = 0;
    collidedWithPlayerHandled = 0;
    infiniteLife = powObjAsset->InfiniteLife;
    lifetimeSet = 0;

    collisionDisabled = false;
    if (destObj && destObj->destructible && destObj->destructible->active) {
        collisionDisabled = true;
    }

    for (int i = 0; i < numObjs; i++) {
        bool success = objPtrs[i]->Reset(
            this, modelInstanceAsset, &objPtrs[i]->entity->matrix.pos,
            powObjAsset->PhysicsData.linearDamping,
            powObjAsset->PhysicsData.angularDamping,
            powObjAsset->PhysicsData.elasticity,
            powObjAsset->PhysicsData.friction, powObjAsset->PhysicsData.mass,
            powObjAsset->PhysicsData.settleTimer == 0.0f,
            (Sext::magCharge)powObjAsset->PhysicsData.magnetic_Charge,
            collisionDisabled,
            (Sext::eCollisionLayer)asset->initialCollisionFilter);

        if (!success) {
            zPhysicsObject* curObj = objPtrs[i];
            curObj->CleanupPhysicsObject();
            curObj->DestroyComponents();
            zPOWManager::factory.DeallocMem(curObj);
            objPtrs[i] = 0;
        }
    }
}

void zPOWGroup::Combat(xVec3* center, float radiusSq) {
    xVec3 delta;

    if (!collisionDisabled) {
        for (int i = 0; i < numObjs; i++) {
            zPhysicsObject* curObj = objPtrs[i];

            if (curObj) {
                delta.Sub(*center, *curObj->GetPos());

                if (delta.length2() < radiusSq) {
                    xVec3 playerDelta;

                    playerDelta.Sub(*curObj->GetPos(), zPOWPlayerPos());
                    playerDelta.y = 0.0f;
                    playerDelta.normalize();
                    playerDelta.y = 0.5f;
                    playerDelta *= 500.0f;

                    hkVector4Init impulse(playerDelta);

                    xHavok_ApplyLinearImpulse(curObj->havokSimObj.pRigidBody,
                                              impulse);
                    curObj->owner->collidedWithPlayer = 1;
                }
            }
        }
    }
}

void zPOWGroup::GetBoundSphere(xVec3& center, float& radius) {
    if (asset->VolumeType == 0) {
        xVec3 up, at, right;

        Math::Vector4 row_1(asset->Box.Transform.v[0]);
        Math::Vector4 row_2(asset->Box.Transform.v[1]);
        Math::Vector4 row_3(asset->Box.Transform.v[2]);

        __ct__Q24Math6VectorFfff(&right, row_1.data[0], row_2.data[0],
                                 row_3.data[0]);
        __ct__Q24Math6VectorFfff(&up, row_1.data[1], row_2.data[1],
                                 row_3.data[1]);
        __ct__Q24Math6VectorFfff(&at, row_1.data[2], row_2.data[2],
                                 row_3.data[2]);

        center.x = row_1.data[3];
        center.y = row_2.data[3];
        center.z = row_3.data[3];

        radius = Math::sqrt(right.length2());

        float upLen = Math::sqrt(up.length2());
        if (upLen > radius) {
            radius = upLen;
        }

        float atLen = Math::sqrt(at.length2());
        if (atLen > radius) {
            radius = atLen;
        }

        radius *= 0.5f;
    } else if (asset->VolumeType == 1) {
        __ct__Q24Math6VectorFfff(&center, asset->Sphere.Center.x,
                                 asset->Sphere.Center.y,
                                 asset->Sphere.Center.z);
        radius = asset->Sphere.Radius;
    } else if (asset->VolumeType == 2) {
        __ct__Q24Math6VectorFfff(&center, asset->Point.Position.x,
                                 asset->Point.Position.y,
                                 asset->Point.Position.z);
        radius = 0.5f;
    }
}

void zPOWGroup::RemoveFromSimWorld() {
    for (int i = 0; i < numObjs; i++) {
        zPhysicsObject* curObj = objPtrs[i];

        if (curObj) {
            curObj->RemoveFromSimWorld();
        }
    }

    countdownToActivation = 2;
}

void zPOWGroup::Unactivate() {
    for (int i = 0; i < numObjs; i++) {
        zPhysicsObject* curObj = objPtrs[i];

        if (curObj) {
            curObj->CleanupPhysicsObject();
            curObj->DestroyComponents();
            zPOWManager::factory.DeallocMem(curObj);
            objPtrs[i] = 0;
        }
    }
}

void zPOWGroup::Remove(zPhysicsObject* obj) {
    for (int i = 0; i < numObjs; i++) {
        if (objPtrs[i] == obj) {
            objPtrs[i] = 0;
            return;
        }
    }
}

void zPOWGroup::SceneExit() { Unactivate(); }

void zPOWGroup::SetupDrivenBy() {
    driveInfoMat = g_I3;

    if (linkArray && linkArray->count) {
        // Declared ahead of i, so i takes r29 and the link r30, as retail's.
        Sext::LinkAssetBaseNew* link;

        for (unsigned int i = 0; i < linkArray->count; i++) {
            link = &((Sext::LinkAssetBaseNew*)linkArray->data)[i];

            if (!link->disabled && link->dstEvent.type == 0x3FE52B13) {
                xEnt* dent = (xEnt*)zSceneFindEntity(link->dstAssetID);

                if (dent) {
                    driveMode = ((Sext::EventActionDrivenBy*)link->srcEvent.v)
                                    ->param0 != 0;
                    driver = dent;

                    xMat4x3 invMat;

                    if (dent->frame) {
                        xMat4x3Invert(&invMat, &dent->ogModel.data->Mat);
                        xMat4x3Mul(&driveInfoMat, &mat, &invMat);
                    }
                }
            }
        }
    }
}

// Weak in the image, called out of line by SetMatrix and GetBoundSphere:
// defined below them.
inline Math::Vector4::Vector4(const Math::Vector4POD& v) {
    Assign(v.data[0], v.data[1], v.data[2], v.data[3]);
}
