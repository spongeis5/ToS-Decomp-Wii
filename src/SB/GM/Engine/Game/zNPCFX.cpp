#include "SB/GM/Engine/Game/zNPCFX.pool.h"

// zNPCFX.cpp -- an NPC's effects: one node per effect its character asset
// lists, created by type from a table, then run, stopped, paused and resumed
// by name hash; and the three node types, which spawn an FX script once, keep
// one running on a bone, or draw cards turning around the NPC. Read from the
// image with tools/brief.py; the layouts are the DWARF's
// (tools/dwarf_types.py), the node's virtual slots the image's
// (tools/vtslot.py).

class xBase;
class xEffectAttachIntf;
class zNPCFX;
class zNPCStatus;

namespace Sext {
class EventAny;
}

namespace Memory {

class Factory {
public:
    void DeallocMem(void* mem);
};

}  // namespace Memory

class zNPCManager {
public:
    static Memory::Factory factory;
};

// ---------------------------------------------------------------------------
// Vectors and matrices

class xVec3 {
public:
    xVec3& operator=(const xVec3& other);

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
    xVec3 pos;
    unsigned int pad3;
};

void xMat3x3Euler(xMat3x3* m, float yaw, float pitch, float roll);
void xMat4x3Mul(xMat4x3* out, const xMat4x3* a, const xMat4x3* b);

namespace Math {

class Matrix43 {
public:
    Matrix43& operator=(const Matrix43& other);

    float m[12];
};

class Vector4 {
public:
    float x;
    float y;
    float z;
    float w;
};

}  // namespace Math

// ---------------------------------------------------------------------------
// Rendering

namespace Graphics {

class Scene;
class PreallocImmediateGeomMem;

enum PrimitiveType {
    PrimitiveType_Triangles = 0x90
};

class Renderable3DTransform {
public:
    Math::Matrix43 worldMat;
    Math::Vector4 recipScale;
};

class Renderable3D {
public:
    void SetTransform(Renderable3DTransform* xform, bool owned);
};

}  // namespace Graphics

namespace Sext {

class vec3 {
public:
    float x;
    float y;
    float z;
};

class Rotation3 {
public:
    float yaw;
    float pitch;
    float roll;
};

class uid {
public:
    unsigned long long internalUid;
};

class ImmediateVertex {
public:
    void Assign(float x, float y, float z, unsigned int color, float tu,
                float tv);

    float x;
    float y;
    float z;
    unsigned int color;
    float tu;
    float tv;
};

}  // namespace Sext

namespace World {

class Entity;
class ImmediateGeometry;
class ImmediateGeometryEntity;
class ImmediatePrototypeEntity;
class xOGModel;
class xOGModelRef;

class EntityHandleBase {
public:
    unsigned char _pad0[0x38];
    Entity* entity;
};

template <class T>
class EntityHandle : public EntityHandleBase {
public:
    T* Get() const { return (T*)entity; }
};

class EntityManager {
public:
    static EntityHandleBase* FindHandle(unsigned long long id);
};

class ImmediateInstanceAsset {
public:
    unsigned long long immediatePrototypeID;
    unsigned short rendCount;
    unsigned short pad;
    unsigned int rendSettings;
};

class ImmediateGeometryContext {
public:
    unsigned char _pad0[0xC];
};

template <class T>
class ImmediateContext {
public:
    enum StateEnum {
        STATE_ERROR = 0xFFFFFFFF,
        STATE_Constructed = 0,
        STATE_Committed = 1,
        STATE_Added = 2
    };

    ImmediateContext(ImmediatePrototypeEntity* proto, int firstVertex,
                     int vertexCount, int primCount, int indexCount,
                     bool preallocated,
                     Graphics::PreallocImmediateGeomMem* prealloc,
                     Graphics::Renderable3DTransform** xform,
                     Graphics::PrimitiveType primType);

    Graphics::Renderable3D* Commit();
    void AddToScene(Graphics::Scene* scene, Graphics::Renderable3D* rend);

    T* verts;
    unsigned short* indices;
    ImmediateGeometryEntity* geomEnt;
    ImmediateGeometry* geom;
    int vertexCount;
    int primCount;
    int indexCount;
    StateEnum state;
    ImmediateGeometryContext igeomCtxt;
};

class WorldPrivate {
public:
    static Graphics::Scene* primaryScene;
};

}  // namespace World

xBase* zSceneFindObject(unsigned long long id);
void xModelGetBoneMatScaled(xMat4x3& mat, const World::xOGModel& model,
                            unsigned long bone);

// ---------------------------------------------------------------------------
// FX scripts

namespace FX {

class zFXSpawn {
public:
    void Init(zFXSpawn* script, xEffectAttachIntf* attach,
              World::xOGModelRef* modelRef, int bone, const xVec3* offset,
              const xVec3* orientation, const xVec3* pos, const xMat3x3* mat,
              bool oneShot);
    void SetMat(const xMat4x3& mat);
    void SetPaused(bool paused);

    unsigned char _pad0[0xB4];
    unsigned int mFlags;
};

}  // namespace FX

class zFXScriptSpawnPtMgr {
public:
    static FX::zFXSpawn* GetNewPoolSpawnPoint(const char* name);
    static void ReturnPoolSpawnPoint(FX::zFXSpawn* spawn);
};

// ---------------------------------------------------------------------------
// The character asset's effect list

enum eNPCFXType {
    eNPCFXTypeOneShot = 0,
    eNPCFXTypeLoop = 1,
    eNPCFXTypeImmediateInstanceLoop = 2,
    END_eNPCFXTypeENUM = 3
};

class FXTypeFXScript {
public:
    int AttachedBone;
    Sext::vec3 AttachOffset;
    Sext::Rotation3 AttachOrientation;
    Sext::uid FXSpawnID;
};

class FXTypeImmediateInstance {
public:
    World::ImmediateInstanceAsset Texture1;
    World::ImmediateInstanceAsset Texture2;
    World::ImmediateInstanceAsset Texture3;
    World::ImmediateInstanceAsset Texture4;
};

namespace Sext {

class CharacterAssets {
public:
    // One entry of the effect list, 0x50 bytes: its name and type, then the
    // type's own description (no DWARF member names that part).
    class NPCFXNode {
    public:
        unsigned int nameHash;
        eNPCFXType type;
        unsigned char _pad0[0x10 - 0x8];
        union {
            FXTypeFXScript fxScript;
            FXTypeImmediateInstance immediateInstance;
        };
    };
};

}  // namespace Sext

class __NPCFXs__ {
public:
    unsigned int count;
    Sext::CharacterAssets::NPCFXNode* data;
};

class zCharacterAsset {
public:
    unsigned char _pad0[0xB8];
    __NPCFXs__ NPCFXs;
};

// ---------------------------------------------------------------------------
// The NPC side

class zNPCEntity {
public:
    unsigned char _pad0[0x34];
    World::xOGModel* model;
};

class zNPCBase {
public:
    unsigned char _pad0[0x68];
    zCharacterAsset* characterAsset;
    unsigned char _pad1[0x98 - 0x6C];
    zNPCEntity* npcEntity;
};

class zNPCComponent {
public:
    zNPCBase* owner;

    virtual void Attached(const zNPCStatus* status);
    virtual void Detached(zNPCStatus* status);
    virtual void Reset(const zNPCStatus* status);
    virtual void AllAttached();
    virtual void PreUpdate(float dt);
    virtual void PostUpdate(float dt);
    virtual void Render();
    virtual void Paused();
    virtual void Resumed();
    virtual bool SystemEvent(xBase* from, xBase* to, unsigned int to_event,
                             Sext::EventAny* params);
};

class zNPCFXParams {
public:
    float stunnedCardScale;
    float stunnedCardTurnRadius;
    float stunnedCardTurnRate;
    float stunnedCardTurnRateVarFreq;
    Sext::vec3 stunnedNPCOffset;
    unsigned int stunnedNumberOfCards;
    float stunnedFadeInTime;
    float stunnedDelayTime;
};

// A node keeps its vtable pointer at +0xC, after its members. Every slot of
// the base's own table is a shared empty body (Update's returns false).
class zNPCFXNode {
public:
    unsigned int nameHash;
    eNPCFXType fxType;
    zNPCFX* owner;

    virtual void Setup(Sext::CharacterAssets::NPCFXNode* node);
    virtual void Destroy();
    virtual bool Update(zNPCEntity* ent, float dt);
    virtual void Run(zNPCEntity* ent);
    virtual void Stop();
    virtual void Pause();
    virtual void Resume();
};

class zNPCFXOneShotFXScript : public zNPCFXNode {
public:
    virtual void Setup(Sext::CharacterAssets::NPCFXNode* node);
    virtual void Run(zNPCEntity* ent);

    FXTypeFXScript* asset;
    FX::zFXSpawn* script;
};

class zNPCFXLoopFXScript : public zNPCFXNode {
public:
    virtual void Setup(Sext::CharacterAssets::NPCFXNode* node);
    virtual bool Update(zNPCEntity* ent, float dt);
    virtual void Run(zNPCEntity* ent);
    virtual void Stop();
    virtual void Pause();
    virtual void Resume();

    FXTypeFXScript* asset;
    FX::zFXSpawn* script;
    FX::zFXSpawn* spawnPoint;
};

class zNPCFXImmediateInstanceLoop : public zNPCFXNode {
public:
    virtual void Setup(Sext::CharacterAssets::NPCFXNode* node);
    virtual bool Update(zNPCEntity* ent, float dt);
    virtual void Run(zNPCEntity* ent);

    World::ImmediatePrototypeEntity* FindImmProtEnt(
        World::ImmediateInstanceAsset* immInstAsset);
    void RenderCard(World::ImmediatePrototypeEntity* texture, float size,
                    Math::Matrix43* mat);

    FXTypeImmediateInstance* asset;
    World::ImmediatePrototypeEntity* textures[4];
    unsigned int numberOfTextures;
    float curFXAngle;
    float curFXTime;
};

// Indexed by eNPCFXType.
class zNPCFXTypeInfo {
public:
    eNPCFXType type;
    zNPCFXNode* (*fxNodeCreator)(Memory::Factory* factory);
};

extern zNPCFXTypeInfo gNPCFXTypeInfo[];
extern zNPCFXParams gDefaultNPCFXParams;

class zNPCFX : public zNPCComponent {
public:
    virtual void Attached(const zNPCStatus* status);
    virtual void Detached(zNPCStatus* status);
    virtual void Reset(const zNPCStatus* status);
    virtual void PostUpdate(float dt);
    virtual void Paused();
    virtual void Resumed();

    void StopAllFX();
    void PauseAllFX();
    void ResumeAllFX();
    bool RunFX(unsigned int nameHash);
    bool IsFXRunning(unsigned int nameHash);
    void StopFX(unsigned int nameHash);

    zNPCFXNode* FXs[8];
    bool isRunning[8];
    unsigned int numberOfFXs;
    zNPCFXParams* params;
    zNPCEntity* customEnt;
};

// ---------------------------------------------------------------------------
// The component

// One node per listed effect, at most eight; an unknown type leaves its
// slot empty.
void zNPCFX::Attached(const zNPCStatus* status) {
    if (params == 0) {
        params = &gDefaultNPCFXParams;
    }

    const zCharacterAsset& characterAsset = *owner->characterAsset;

    numberOfFXs = characterAsset.NPCFXs.count;
    if (numberOfFXs > 8) {
        numberOfFXs = 8;
    }

    for (unsigned int i = 0; i < numberOfFXs; i++) {
        unsigned int nameHash = characterAsset.NPCFXs.data[i].nameHash;
        eNPCFXType type = characterAsset.NPCFXs.data[i].type;

        // Unsigned, as retail tests it: `type >= END` compares signed.
        if ((unsigned int)type > eNPCFXTypeImmediateInstanceLoop) {
            FXs[i] = 0;
            continue;
        }

        FXs[i] = gNPCFXTypeInfo[type].fxNodeCreator(&zNPCManager::factory);

        FXs[i]->nameHash = nameHash;
        FXs[i]->fxType = type;
        FXs[i]->owner = this;

        FXs[i]->Setup(&characterAsset.NPCFXs.data[i]);
        isRunning[i] = false;
    }
}

void zNPCFX::Detached(zNPCStatus* status) {
    StopAllFX();

    for (unsigned int i = 0; i < 8; i++) {
        if (FXs[i] != 0) {
            FXs[i]->Destroy();
            zNPCManager::factory.DeallocMem(FXs[i]);
            FXs[i] = 0;
        }

        isRunning[i] = false;
    }

    numberOfFXs = 0;
}

void zNPCFX::Reset(const zNPCStatus* status) {
    StopAllFX();
}

// A running node reports whether it is still running.
void zNPCFX::PostUpdate(float dt) {
    zNPCEntity* npcEnt = customEnt;

    npcEnt = npcEnt != 0 ? npcEnt : owner->npcEntity;

    for (unsigned int i = 0; i < numberOfFXs; i++) {
        if (FXs[i] != 0) {
            if (isRunning[i]) {
                isRunning[i] = FXs[i]->Update(npcEnt, dt);
            }
        }
    }
}

void zNPCFX::Paused() {
    PauseAllFX();
}

void zNPCFX::Resumed() {
    ResumeAllFX();
}

void zNPCFX::StopAllFX() {
    for (unsigned int i = 0; i < numberOfFXs; i++) {
        if (FXs[i] != 0) {
            FXs[i]->Stop();
            isRunning[i] = false;
        }
    }
}

void zNPCFX::PauseAllFX() {
    for (unsigned int i = 0; i < numberOfFXs; i++) {
        if (FXs[i] != 0) {
            FXs[i]->Pause();
        }
    }
}

void zNPCFX::ResumeAllFX() {
    for (unsigned int i = 0; i < numberOfFXs; i++) {
        if (FXs[i] != 0) {
            FXs[i]->Resume();
        }
    }
}

// Runs every node with the name; a one-shot is not left marked running.
bool zNPCFX::RunFX(unsigned int nameHash) {
    bool started = false;

    zNPCEntity* npcEnt = customEnt;

    npcEnt = npcEnt != 0 ? npcEnt : owner->npcEntity;

    for (unsigned int i = 0; i < numberOfFXs; i++) {
        if (FXs[i] != 0) {
            if (nameHash == FXs[i]->nameHash) {
                FXs[i]->Run(npcEnt);

                if (FXs[i]->fxType != eNPCFXTypeOneShot) {
                    isRunning[i] = true;
                }

                started = true;
            }
        }
    }

    return started;
}

bool zNPCFX::IsFXRunning(unsigned int nameHash) {
    for (unsigned int i = 0; i < numberOfFXs; i++) {
        if (FXs[i] != 0) {
            if (nameHash == FXs[i]->nameHash) {
                if (isRunning[i]) {
                    return true;
                }
            }
        }
    }

    return false;
}

void zNPCFX::StopFX(unsigned int nameHash) {
    for (unsigned int i = 0; i < numberOfFXs; i++) {
        if (FXs[i] != 0) {
            if (nameHash == FXs[i]->nameHash) {
                FXs[i]->Stop();
                isRunning[i] = false;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// A script spawned once, at a bone

void zNPCFXOneShotFXScript::Setup(Sext::CharacterAssets::NPCFXNode* node) {
    asset = &node->fxScript;

    if (node->fxScript.FXSpawnID.internalUid == 0) {
        script = 0;
    } else {
        script = (FX::zFXSpawn*)zSceneFindObject(node->fxScript.FXSpawnID.internalUid);
    }
}

void zNPCFXOneShotFXScript::Run(zNPCEntity* ent) {
    FX::zFXSpawn* spawnPoint =
        zFXScriptSpawnPtMgr::GetNewPoolSpawnPoint("zNPCFXOneShotFXScript::Run");

    if (spawnPoint != 0) {
        xMat4x3 fxMatLocal;
        xMat3x3Euler(&fxMatLocal, asset->AttachOrientation.yaw,
                     asset->AttachOrientation.pitch,
                     asset->AttachOrientation.roll);
        fxMatLocal.pos = (const xVec3&)asset->AttachOffset;

        xMat4x3 fxMatWorld;
        xModelGetBoneMatScaled(fxMatWorld, *ent->model, asset->AttachedBone);

        xMat4x3 fxMatFinal;
        xMat4x3Mul(&fxMatFinal, &fxMatLocal, &fxMatWorld);

        spawnPoint->Init(script, 0, 0, -1, 0, 0, &fxMatFinal.pos, &fxMatFinal,
                         true);
    }
}

// ---------------------------------------------------------------------------
// A script kept running, following a bone

void zNPCFXLoopFXScript::Setup(Sext::CharacterAssets::NPCFXNode* node) {
    asset = &node->fxScript;

    if (node->fxScript.FXSpawnID.internalUid == 0) {
        script = 0;
    } else {
        script = (FX::zFXSpawn*)zSceneFindObject(node->fxScript.FXSpawnID.internalUid);
        spawnPoint = 0;
    }
}

// Moves the spawn point to the bone; false once the script has ended.
bool zNPCFXLoopFXScript::Update(zNPCEntity* ent, float dt) {
    if (spawnPoint == 0) {
        return false;
    }

    xMat4x3 fxMatLocal;
    xMat3x3Euler(&fxMatLocal, asset->AttachOrientation.yaw,
                 asset->AttachOrientation.pitch, asset->AttachOrientation.roll);
    fxMatLocal.pos = (const xVec3&)asset->AttachOffset;

    xMat4x3 fxMatWorld;
    xModelGetBoneMatScaled(fxMatWorld, *ent->model, asset->AttachedBone);

    xMat4x3 fxMatFinal;
    xMat4x3Mul(&fxMatFinal, &fxMatLocal, &fxMatWorld);

    spawnPoint->SetMat(fxMatFinal);

    return spawnPoint->mFlags & 1;
}

void zNPCFXLoopFXScript::Run(zNPCEntity* ent) {
    if (spawnPoint != 0) {
        zFXScriptSpawnPtMgr::ReturnPoolSpawnPoint(spawnPoint);
        spawnPoint = 0;
    }

    spawnPoint =
        zFXScriptSpawnPtMgr::GetNewPoolSpawnPoint("zNPCFXLoopFXScript::Run");

    if (spawnPoint != 0) {
        xMat4x3 fxMatLocal;
        xMat3x3Euler(&fxMatLocal, asset->AttachOrientation.yaw,
                     asset->AttachOrientation.pitch,
                     asset->AttachOrientation.roll);
        fxMatLocal.pos = (const xVec3&)asset->AttachOffset;

        xMat4x3 fxMatWorld;
        xModelGetBoneMatScaled(fxMatWorld, *ent->model, asset->AttachedBone);

        xMat4x3 fxMatFinal;
        xMat4x3Mul(&fxMatFinal, &fxMatLocal, &fxMatWorld);

        spawnPoint->Init(script, 0, 0, -1, 0, 0, &fxMatFinal.pos, &fxMatFinal,
                         false);
    }
}

void zNPCFXLoopFXScript::Stop() {
    if (spawnPoint != 0) {
        zFXScriptSpawnPtMgr::ReturnPoolSpawnPoint(spawnPoint);
        spawnPoint = 0;
    }
}

void zNPCFXLoopFXScript::Pause() {
    if (spawnPoint != 0) {
        spawnPoint->SetPaused(true);
    }
}

void zNPCFXLoopFXScript::Resume() {
    if (spawnPoint != 0) {
        spawnPoint->SetPaused(false);
    }
}

// ---------------------------------------------------------------------------
// Cards turning around the NPC, drawn as immediate geometry

// Keeps each of the four textures that resolves, in order.
void zNPCFXImmediateInstanceLoop::Setup(Sext::CharacterAssets::NPCFXNode* node) {
    asset = &node->immediateInstance;

    numberOfTextures = 0;
    World::ImmediatePrototypeEntity* texture;

    texture = FindImmProtEnt(&asset->Texture1);
    if (texture != 0) {
        textures[numberOfTextures++] = texture;
    }

    texture = FindImmProtEnt(&asset->Texture2);
    if (texture != 0) {
        textures[numberOfTextures++] = texture;
    }

    texture = FindImmProtEnt(&asset->Texture3);
    if (texture != 0) {
        textures[numberOfTextures++] = texture;
    }

    texture = FindImmProtEnt(&asset->Texture4);
    if (texture != 0) {
        textures[numberOfTextures++] = texture;
    }

    curFXAngle = 0.0f;
    curFXTime = 0.0f;
}

World::ImmediatePrototypeEntity* zNPCFXImmediateInstanceLoop::FindImmProtEnt(
    World::ImmediateInstanceAsset* immInstAsset) {
    if (immInstAsset->immediatePrototypeID == 0) {
        return 0;
    }

    World::EntityHandle<World::ImmediatePrototypeEntity>* handle =
        (World::EntityHandle<World::ImmediatePrototypeEntity>*)
            World::EntityManager::FindHandle(immInstAsset->immediatePrototypeID);

    return handle != 0 ? handle->Get() : 0;
}

// One card: a quad of the given half-size, placed by the matrix.
void zNPCFXImmediateInstanceLoop::RenderCard(
    World::ImmediatePrototypeEntity* texture, float size, Math::Matrix43* mat) {
    Graphics::Renderable3DTransform* xform;
    World::ImmediateContext<Sext::ImmediateVertex> immCtxt(
        texture, 0, 4, 2, 6, false, 0, &xform,
        Graphics::PrimitiveType_Triangles);

    if (immCtxt.state !=
        World::ImmediateContext<Sext::ImmediateVertex>::STATE_ERROR) {
        immCtxt.verts[0].Assign(-size, -size, 0.0f, 0x80FFFFFF, 0.0f, 0.0f);
        immCtxt.verts[1].Assign(-size, size, 0.0f, 0x80FFFFFF, 0.0f, 1.0f);
        immCtxt.verts[2].Assign(size, size, 0.0f, 0x80FFFFFF, 1.0f, 1.0f);
        immCtxt.verts[3].Assign(size, -size, 0.0f, 0x80FFFFFF, 1.0f, 0.0f);

        immCtxt.indices[0] = 0;
        immCtxt.indices[1] = 1;
        immCtxt.indices[2] = 2;
        immCtxt.indices[3] = 0;
        immCtxt.indices[4] = 2;
        immCtxt.indices[5] = 3;

        Graphics::Renderable3D* rend = immCtxt.Commit();
        xform->worldMat = *mat;
        rend->SetTransform(xform, true);
        immCtxt.AddToScene(World::WorldPrivate::primaryScene, rend);
    }
}

// Update is not written: six distinct float literals in one function, each
// with its own lis in retail -- the four-literal wall (NOTES.md).

void zNPCFXImmediateInstanceLoop::Run(zNPCEntity* ent) {
    curFXTime = 0.0f;
}
