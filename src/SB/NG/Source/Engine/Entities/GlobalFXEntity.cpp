// GlobalFXEntity.cpp -- two functions, read from the image with
// tools/disasm.py. Sext::GlobalFX::Create takes 60 bytes from the
// global heap (heap 0, tag 48) through a static stack allocator on the
// stack, constructs a GlobalFXEntity there when the block is not null
// (the Entity base, the vtable, the asset cleared and the type id set
// to 35, in line under the always-inline pragma), stores the asset,
// clears the eight prototype entities, and if the asset names a screen
// quad finds its handle, activates it, keeps its entity, and starts the
// post-process with the display size. Deactivate returns the screen
// quad's handle. Layouts from the DWARF (GlobalFXEntity 0x3C on Entity
// 0x18, GlobalFX 0x48, EntityHandleBase 0x48 with the entity at +0x38,
// StaticStackAllocator 0xC).

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {
enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };

void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap, eMemMgrTag tag,
                      bool clear);

class StaticStackAllocator {
public:
    void Create(void* buffer, int size);

    void* Alloc(int size) {
        void* p = mark;
        mark += size;
        return p;
    }

    unsigned char* mark;
    unsigned char* buffer;
    unsigned char* bufferEnd;
};

}  // namespace Memory

inline void* operator new(unsigned long, void* p) { return p; }
#pragma always_inline on

namespace Util {

// Empty, and passed as a named default-initialised local: a temporary
// `Referrer()` is value-initialised, which zeroes its one byte on the
// stack, and retail passes the address of nothing written.
class Referrer {};

}  // namespace Util

class uid {
public:
    unsigned long long id;
};

namespace Sext {
class GlobalFX;
}

namespace Graphics {

class Display {
public:
    static int GetScreenWidth();
    static int GetScreenHeight();
};

}  // namespace Graphics

namespace World {

class GlobalFXEntity;
class EmbeddedListNode;

class EntityHandleBase {
public:
    void Activate(const Util::Referrer& referrer);
    void Deactivate(const Util::Referrer& referrer);

    unsigned char _pad0[0x38];
    class Entity* entity;
};

class EntityManager {
public:
    static EntityHandleBase* FindHandle(unsigned long long id);
};

class Entity {
public:
    Entity(EntityHandleBase* handle);

    virtual void __key();

    unsigned char ogSceneNode[0x8];
    int ogUpdateIdx;
    unsigned int typeID;
    EntityHandleBase* handle;
};

class ImmediatePrototypeEntity : public Entity {};

class GlobalFXEntity : public Entity {
public:
    GlobalFXEntity(EntityHandleBase* handle) : Entity(handle) {
        mAsset = 0;
        typeID = 35;
    }

    void Deactivate();

    ImmediatePrototypeEntity* HDRImmProtEnt;
    ImmediatePrototypeEntity* DoFImmProtEnt;
    ImmediatePrototypeEntity* MotionBlurImmProtEnt;
    ImmediatePrototypeEntity* PostProcessImmProtEnt;
    ImmediatePrototypeEntity* ShadowReceiveImmProtEnt;
    ImmediatePrototypeEntity* DecalImmProtEnt;
    ImmediatePrototypeEntity* DoF_HDR_BlurImmProtEnt;
    ImmediatePrototypeEntity* ScreenQuad;
    Sext::GlobalFX* mAsset;
};

}  // namespace World

namespace Graphics {

class PostProcess {
public:
    void Init(int width, int height, World::GlobalFXEntity* entity);
};

extern PostProcess g_PostProcess;

}  // namespace Graphics

namespace Sext {

class GlobalFX {
public:
    static World::GlobalFXEntity* Create(World::EntityHandleBase* handle,
                                         GlobalFX* asset);

    uid HDRImmediateModel;
    uid DepthOfFieldImmediateModel;
    uid MotionBlurImmediateModel;
    uid PostProcessImmediateModel;
    uid OutlineImmediateModel;
    uid ShadowReceiveImmediateModel;
    uid DecalImmediateModel;
    uid DoF_HDR_BlurImmediateModel;
    uid ScreenQuad;
};

}  // namespace Sext

World::GlobalFXEntity* Sext::GlobalFX::Create(World::EntityHandleBase* handle,
                                              GlobalFX* asset) {
    Memory::StaticStackAllocator alloc;

    alloc.Create(Memory::AllocGlobalHeap(sizeof(World::GlobalFXEntity),
                                         (Memory::GlobalHeapEnum)0,
                                         (eMemMgrTag)48, false),
                 sizeof(World::GlobalFXEntity));

    World::GlobalFXEntity* entity =
        new (alloc.Alloc(sizeof(World::GlobalFXEntity)))
            World::GlobalFXEntity(handle);

    entity->mAsset = asset;
    entity->HDRImmProtEnt = 0;
    entity->DoFImmProtEnt = 0;
    entity->MotionBlurImmProtEnt = 0;
    entity->PostProcessImmProtEnt = 0;
    entity->ShadowReceiveImmProtEnt = 0;
    entity->DecalImmProtEnt = 0;
    entity->DoF_HDR_BlurImmProtEnt = 0;
    entity->ScreenQuad = 0;

    if (asset->ScreenQuad.id != 0) {
        World::EntityHandleBase* quad =
            World::EntityManager::FindHandle(asset->ScreenQuad.id);

        if (quad) {
            Util::Referrer referrer;

            quad->Activate(referrer);
            entity->ScreenQuad = (World::ImmediatePrototypeEntity*)quad->entity;

            int width = Graphics::Display::GetScreenWidth();
            int height = Graphics::Display::GetScreenHeight();

            Graphics::g_PostProcess.Init(width, height, entity);
        }
    }

    return entity;
}

void World::GlobalFXEntity::Deactivate() {
    if (ScreenQuad) {
        Util::Referrer referrer;

        ScreenQuad->handle->Deactivate(referrer);
    }
}
