// zProjectileEntity.cpp -- three functions, read from the image with
// tools/disasm.py. The render half of a projectile. LoadModel is the
// same routine zPhysicsObjectEntity carries, on this class's members:
// it keeps the instance asset, creates the model from it with simple
// shadows (eShadow 2), switches its updater to the engine's do-nothing
// one, shows it, sets the bound radius to the largest of the collision
// mesh's three AABB extents -- or 0.25 when the asset resolves to no
// model prototype entity, or that entity carries no collision mesh
// blob -- and then sets bit 3 of the model's simple shadow cache flags
// if it got a cache. FreeModel defers the model's destruction and
// clears both pointers. Setup records the owning projectile, loads the
// model, copies the matrix into the entity and from there into the
// previous matrix, copies it on into the model and updates the render
// state.
//
// Layouts from the DWARF (tools/dwarf_types.py): zProjectileEntity is
// 0x98 bytes -- the matrix at +0, prevMatrix at +0x40, the instance
// asset at +0x80, the model at +0x84, the owning projectile at +0x88,
// boundRadius at +0x8C, attachedEnt at +0x90 and the two bitfields in
// the word at +0x94; xMat4x3 0x40 on xMat3x3; xOGModel 0x178 with
// mModelArt at +0xC4, whose simpleShadowCachePtr is +4 into it (the
// +0xC8 the bytes load); ShadowSimpleCache with its unsigned short
// flags at +0x14; ModelPrototypeEntity 0x8C with collmeshBlob at +0x6C;
// CollisionMeshBlobEntity 0x4C with dxTriMeshData at +0x18, whose
// AABBExtents is +0xC into it (the +0x24, +0x28 and +0x2C the bytes
// read). The two literals are read out of the image at the addresses
// the loads build: @254997 (0x8068D2D0) is 0.25f and @254625
// (0x8068D29C) is 1.0f. The unit loads float literals and its split
// carries no .rodata, so the generated padding header goes first --
// gen_poolprefix.py measures 65,032 bytes of the unity unit's .rodata
// ahead of the first of them.
//
// Four shapes the bytes fixed. The radius is a MAX MACRO nested in
// itself: retail computes max(e0, e1), compares it against e2 and, on
// the greater side, COMPUTES max(e0, e1) A SECOND TIME, which is the
// macro's two expansions of its first argument and not an inline
// function or a named local. The two refusals are a nested if and not
// an `||`, each with its own store and its own branch to the join.
// Setup's two matrix assignments are CHAINED, `prevMatrix = matrix =
// mat`: retail passes the first assignment's returned reference
// straight into the second (`mr r4,r3`), where two statements would
// re-form the address. And the model pointer is reloaded before each
// call that follows one, because a call is between them.

#include "SB/GM/Engine/Game/zProjectileEntity.pool.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))

class zProjectile;
class xEnt;

class xVec3 {
public:
    float x;
    float y;
    float z;
};

class xMat3x3 {
public:
    xVec3 left;
    int flags;
    xVec3 up;
    unsigned int pad1;
    xVec3 at;
    unsigned int pad2;
};

class xMat4x3 : public xMat3x3 {
public:
    // The image's own out-of-line implicit assignment: declared, never
    // defined here, so the unit emits no copy of it.
    xMat4x3& operator=(const xMat4x3& other);

    xVec3 pos;
    unsigned int pad3;
};

class ShadowSimpleCache {
public:
    unsigned char _pad0[0x14];
    unsigned short flags;
    unsigned char _pad1[0xD4 - 0x16];
};

class TriangleInfo;

class dxTriMeshData {
public:
    float AABBCenter[3];
    float AABBExtents[3];
    TriangleInfo* triInfo;
};

namespace World {

class Entity {
public:
    unsigned char _pad0[0x18];
};

class BlobEntity : public Entity {};

class CollisionMeshBlobEntity : public BlobEntity {
public:
    dxTriMeshData triMesh;
    unsigned char _pad0[0x4C - 0x34];
};

class ModelPrototypeEntity : public Entity {
public:
    unsigned char _pad0[0x6C - 0x18];
    CollisionMeshBlobEntity* collmeshBlob;
    unsigned char _pad1[0x8C - 0x70];
};

class ModelInstanceAsset;
class xOGModelUpdater;

extern xOGModelUpdater g_modelUpdateNone;

class ModelInstanceArticle {
public:
    enum eShadow { eShadow_ = 0x7FFFFFFF };

    unsigned int projectShadowCache;
    ShadowSimpleCache* simpleShadowCachePtr;
    unsigned char _pad0[0x98 - 0x8];
};

class xOGModel {
public:
    static xOGModel* Create(const ModelInstanceAsset& asset,
                            ModelInstanceArticle::eShadow shadow);

    void UpdaterSwitch(xOGModelUpdater* updater, void* parent);
    void Show();
    void DeferDestroy();
    void UpdateRender();

    unsigned char _pad0[0xC4];
    ModelInstanceArticle mModelArt;
    unsigned char _pad1[0x178 - 0x15C];
};

}  // namespace World

class xOGRenderHelper {
public:
    static World::ModelPrototypeEntity* GetModelPrototypeEntity(
        const World::ModelInstanceAsset* asset);
};

class zProjectileEntity {
public:
    void LoadModel(World::ModelInstanceAsset* asset);
    void FreeModel();
    void Setup(zProjectile* projectile, const xMat4x3& mat,
               World::ModelInstanceAsset* asset);

    xMat4x3 matrix;
    xMat4x3 prevMatrix;
    World::ModelInstanceAsset* modelInstanceAsset;
    World::xOGModel* modelRenderData;
    zProjectile* ownerProjectile;
    float boundRadius;
    xEnt* attachedEnt;
    int attachedEntBone : 31;
    bool visible : 1;
};

void zProjectileEntity::LoadModel(World::ModelInstanceAsset* asset) {
    modelInstanceAsset = asset;

    modelRenderData = World::xOGModel::Create(
        *asset, (World::ModelInstanceArticle::eShadow)2);

    modelRenderData->UpdaterSwitch(&World::g_modelUpdateNone, 0);
    modelRenderData->Show();

    World::ModelPrototypeEntity* protoEnt =
        xOGRenderHelper::GetModelPrototypeEntity(modelInstanceAsset);

    if (protoEnt == 0) {
        boundRadius = 0.25f;
    } else {
        World::CollisionMeshBlobEntity* collmesh = protoEnt->collmeshBlob;

        if (collmesh == 0) {
            boundRadius = 0.25f;
        } else {
            boundRadius = MAX(MAX(collmesh->triMesh.AABBExtents[0],
                                  collmesh->triMesh.AABBExtents[1]),
                              collmesh->triMesh.AABBExtents[2]);
        }
    }

    ShadowSimpleCache* cache = modelRenderData->mModelArt.simpleShadowCachePtr;

    if (cache) {
        cache->flags |= 8;
    }
}

void zProjectileEntity::FreeModel() {
    modelRenderData->DeferDestroy();

    modelRenderData = 0;
    modelInstanceAsset = 0;
}

void zProjectileEntity::Setup(zProjectile* projectile, const xMat4x3& mat,
                              World::ModelInstanceAsset* asset) {
    ownerProjectile = projectile;

    LoadModel(asset);

    prevMatrix = matrix = mat;

    *(xMat4x3*)modelRenderData = matrix;

    modelRenderData->UpdateRender();
}
