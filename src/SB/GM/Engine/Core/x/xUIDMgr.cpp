// xUIDMgr.cpp -- six functions, read from the image with
// tools/disasm.py. UID assets map a 32-bit id onto an entity handle:
// the reference asset's Create takes 24 bytes from the global heap
// (tag 16), clears them and constructs a stub entity there, returning
// what the placement new returns; the stub's constructor is the Entity
// base and a vtable. SceneInit resets the next id to 0x80000.
// FindUID walks the scene's UID assets (type 95) for the id and hands
// back the 64-bit handle stored after it; FindModelInstanceAsset does
// the same over type 96 and returns the asset's tail; FindAsset turns
// the handle into an entity through the entity manager, reports the
// entity's word at +0xC when asked, and returns the entity's blob
// data. The next id is a file static of the unity unit's unnamed
// namespace, defined here as the source has it (see TRCModule.cpp).

namespace World {
class EntityHandleBase;

class Entity {
public:
    Entity(EntityHandleBase* handle);

    virtual void __vtable_anchor();

    unsigned char _pad0[0x14];  // 0x18 in the DWARF, the stub adds nothing
};

class xOGStubEntity : public Entity {
public:
    virtual void __vtable_anchor();

    xOGStubEntity(EntityHandleBase* handle);
};

class EntityManager {
public:
    static Entity* FindHandle(unsigned long long handle);
};

}  // namespace World

namespace Domains {

class Blobloid {
public:
    void* BlobData() const;
};

}  // namespace Domains

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {
enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };

void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap, eMemMgrTag tag,
                      bool clear);
}  // namespace Memory

extern "C" void* memset(void* dst, int c, unsigned long n);

inline void* operator new(unsigned long, void* p) { return p; }

void* xSTFindAssetByType(unsigned int type, int index, unsigned int* size);
int xSTAssetCountByType(unsigned int type);

namespace Sext {

class UIDReference {
public:
    static World::xOGStubEntity* Create(World::EntityHandleBase* handle,
                                        UIDReference* asset);
};

}  // namespace Sext

// A UID asset: the id, then the 64-bit handle it stands for.
struct xUIDAsset {
    unsigned int uid;
    unsigned int _pad0;
    unsigned long long handle;
};

// A model-instance UID asset: the id, then 12 more bytes, then the
// model instance asset FindModelInstanceAsset hands back.
struct xUIDModelInstanceAsset {
    unsigned int uid;
    unsigned char _pad0[0xC];
    unsigned char asset[1];
};

namespace {
unsigned long long nextUID;
}  // namespace

void xUIDMgrSceneInit();
unsigned long long xUIDMgrFindUID(unsigned int uid);
void* xUIDMgrFindModelInstanceAsset(unsigned int uid);
void* xUIDMgrFindAsset(unsigned int uid, unsigned int* size);

World::xOGStubEntity* Sext::UIDReference::Create(World::EntityHandleBase* handle,
                                                 UIDReference*) {
    return new (memset(
        Memory::AllocGlobalHeap(sizeof(World::xOGStubEntity),
                                (Memory::GlobalHeapEnum)0, (eMemMgrTag)16,
                                false),
        0, sizeof(World::xOGStubEntity))) World::xOGStubEntity(handle);
}

#pragma dont_inline on
World::xOGStubEntity::xOGStubEntity(EntityHandleBase* handle)
    : Entity(handle) {}
#pragma dont_inline off

void xUIDMgrSceneInit() {
    nextUID = 0x80000;
}

unsigned long long xUIDMgrFindUID(unsigned int uid) {
    int count = xSTAssetCountByType(95);

    for (int i = 0; i < count; i++) {
        xUIDAsset* asset = (xUIDAsset*)xSTFindAssetByType(95, i, 0);

        if (asset->uid == uid) {
            return asset->handle;
        }
    }

    return 0;
}

void* xUIDMgrFindModelInstanceAsset(unsigned int uid) {
    int count = xSTAssetCountByType(96);

    for (int i = 0; i < count; i++) {
        xUIDModelInstanceAsset* asset =
            (xUIDModelInstanceAsset*)xSTFindAssetByType(96, i, 0);

        if (asset->uid == uid) {
            return asset->asset;
        }
    }

    return 0;
}

void* xUIDMgrFindAsset(unsigned int uid, unsigned int* size) {
    unsigned long long handle = xUIDMgrFindUID(uid);

    if (handle != 0) {
        World::Entity* entity = World::EntityManager::FindHandle(handle);

        if (size) {
            *size = *(unsigned int*)((char*)entity + 0xC);
        }

        return ((Domains::Blobloid*)entity)->BlobData();
    }

    return 0;
}
