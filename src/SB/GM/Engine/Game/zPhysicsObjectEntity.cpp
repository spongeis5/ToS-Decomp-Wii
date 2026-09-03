// zPhysicsObjectEntity.cpp -- three functions, read from the image with
// tools/disasm.py. The render half of a physics object. LoadModel keeps
// the instance asset, creates the model from it with simple shadows
// (eShadow 2), switches its updater to the engine's do-nothing one,
// shows it, and works out a bound radius: the largest of the collision
// mesh's three AABB extents, or 0.25 when the asset resolves to no
// model prototype entity or that entity carries no collision mesh
// blob; then, if the model got a simple shadow cache, it sets bit 3 of
// the cache's flags. FreeModel defers the model's destruction and
// clears both pointers. Render copies the entity's matrix into the
// model, updates the render state and sets the colour multiplier to
// white at the entity's alpha.
//
// Layouts from the DWARF (tools/dwarf_types.py): zPhysicsObjectEntity
// is 0x64 bytes -- the model at +0, boundRadius at +4, alpha at +8, a
// one-bit renderEnabled at +0xC, the matrix at +0x10, origPos at +0x50,
// the instance asset at +0x5C and the article at +0x60; xMat4x3 0x40 on
// xMat3x3; xOGModel 0x178 with mModelArt at +0xC4, whose
// simpleShadowCachePtr is +4 into it (so the +0xC8 the bytes load);
// ShadowSimpleCache with its unsigned short flags at +0x14;
// ModelPrototypeEntity 0x8C with collmeshBlob at +0x6C;
// CollisionMeshBlobEntity 0x4C with dxTriMeshData at +0x18, whose
// AABBExtents is +0xC into it (the +0x24, +0x28 and +0x2C the bytes
// read). The two literals are read out of the image at the addresses
// the loads build: @254997 (0x8068D2D0) is 0.25f and @254625
// (0x8068D29C) is 1.0f. The unit loads float literals and its split
// carries no .rodata, so the generated padding header goes first.
//
// Four shapes the bytes fixed. The radius is a MAX MACRO nested in
// itself, not a function: retail computes max(e0, e1), compares it
// against e2 and, on the greater side, COMPUTES max(e0, e1) A SECOND
// TIME -- which is what the macro's two expansions of its first
// argument give and what an inline function or a named local does not.
// The two refusals are a nested if and not an `||`: each has its own
// store of 0.25f and its own branch to the join, where one condition
// would give a single refusal block. Create takes the PARAMETER while
// everything after it reads the member back, because a call is between
// them. And the model pointer is stored and then used without a reload
// for UpdaterSwitch -- nothing is in between, so the store forwards --
// while every later use reloads it across the calls.

#include "SB/GM/Engine/Game/zPhysicsObjectEntity.pool.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))

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
    void SetColorMultiplier(float r, float g, float b, float a);

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

class zPhysicsObjectEntity {
public:
    void LoadModel(World::ModelInstanceAsset* asset);
    void FreeModel();
    void Render();

    World::xOGModel* modelRenderData;
    float boundRadius;
    float alpha;
    bool renderEnabled : 1;
    xMat4x3 matrix;
    xVec3 origPos;
    World::ModelInstanceAsset* modelInstanceAsset;
    World::ModelInstanceArticle* modelInstanceArticle;
};

void zPhysicsObjectEntity::LoadModel(World::ModelInstanceAsset* asset) {
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

void zPhysicsObjectEntity::FreeModel() {
    modelRenderData->DeferDestroy();

    modelRenderData = 0;
    modelInstanceAsset = 0;
}

void zPhysicsObjectEntity::Render() {
    *(xMat4x3*)modelRenderData = matrix;

    modelRenderData->UpdateRender();
    modelRenderData->SetColorMultiplier(1.0f, 1.0f, 1.0f, alpha);
}
