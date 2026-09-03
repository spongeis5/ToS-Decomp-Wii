// RawBlobEntity.cpp -- three functions, read from the image with
// tools/disasm.py. A raw blob is an asset handed to the game whole: the
// asset's Create takes one 40-byte block from the global heap (tag 66),
// places the entity on it, and fills the asset pointer, the size taken
// from the handle's blobloid header, an empty owner list and a clear
// defer flag. The constructor is the BlobEntity base, the vtable, the
// owner list and type id 4. Deactivate either destroys through a
// pointer to member (the 12-byte constant copied to the stack and
// __ptmf_scall, as RenderModeEntity does) or, when the defer flag is
// clear, calls the base's own destroy directly.
//
// Layouts from the DWARF (tools/dwarf_types.py): Entity 0x18 with the
// scene node at +4, the update index at +0xC, the type id at +0x10 and
// the handle at +0x14; BlobEntity 0x18, adding nothing; RawBlobEntity
// 0x28 with the asset at +0x18, its size at +0x1C, the owner list at
// +0x20 and the defer flag at +0x24; Blobloid 0x20, the head of
// EntityHandleBase, with the blob size at +0xC. EntityHandleBase is
// 0x48 in the DWARF and only its Blobloid head is read here, so the
// rest is not declared -- it is reached through a pointer and its size
// never enters the bytes. The destructor is declared and left undefined
// so the vtable's home stays in the unity unit that has it, and
// BlobEntity's is declared for the same reason on top of another: an
// intermediate class with no declared destructor and a virtual one in
// its base gets an 80-byte implicit destructor emitted and counted as
// EXTRA, which is what it did here before the declaration went in. No
// float or string literal is loaded, so there is no pool header.
//
// Two things the bytes fixed, and one they cannot say. The allocation is
// a placement new on the allocator's result, which emits the null test
// that skips only the constructor -- the four member stores that follow
// run on either path, as retail has them. And the direct call in the
// else branch reaches 0x80051460, which the image names
// xOGEntity::Deactivate. Read there, that body is a `delete this` -- the
// first virtual called with -1, then FreeGlobalHeap on a non-null this
// -- and identical weak bodies of that shape fold, so what the original
// wrote here cannot be recovered from the image and the call is made by
// the mangled symbol the image does have, the way zUIModel calls the
// folded empty Matrix33 constructor. The third word of retail's
// pointer-to-member constant (@130773: 0, -1, the function) resolves to
// that SAME address, so the member it named is gone the same way; it is
// declared here as Destroy and left undefined, and the 12-byte constant
// is data this unit emits for itself.

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {
enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };

void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap, eMemMgrTag tag,
                      bool clear);
}  // namespace Memory

extern "C" {
// The shared body every `delete this` folded onto; the image names it
// after xOGEntity.
void Deactivate__Q25World9xOGEntityFv(void* entity);
}

inline void* operator new(unsigned long, void* p) { return p; }

class EmbeddedListNode {
public:
    EmbeddedListNode* next;
    EmbeddedListNode* prev;
};

namespace World {

class Owner;
class MemHandle;

// The blobloid header every handle begins with; only the blob size is
// read here.
class Blobloid {
public:
    unsigned long long blobUID;
    int wmlTypeID;
    int blobSize;
    unsigned char subType;
    unsigned char blobFlags;
    unsigned short langID;
    MemHandle* memHandle;
    unsigned short domRefMaskList;
    unsigned short memOwnerRefMask;
};

class EntityHandleBase : public Blobloid {};

class Entity {
public:
    Entity(EntityHandleBase* handle);

    virtual ~Entity();
    virtual void Deactivate();

    EmbeddedListNode ogSceneNode;
    int ogUpdateIdx;
    unsigned int typeID;
    EntityHandleBase* handle;
};

class BlobEntity : public Entity {
public:
    BlobEntity(EntityHandleBase* handle);

    // Declared and not defined: an implicit one is generated and emitted
    // (80 bytes, EXTRA in unitcmp), since the base's is virtual.
    virtual ~BlobEntity();
};

class RawBlobAsset;

class RawBlobEntity : public BlobEntity {
public:
    RawBlobEntity(EntityHandleBase* handle);

    virtual ~RawBlobEntity();
    virtual void Deactivate();

    void Destroy();

    RawBlobAsset* asset;
    int assetSize;
    Owner* owners;
    bool deferDestroy;
};

class RawBlobAsset {
public:
    static RawBlobEntity* Create(EntityHandleBase* handle,
                                 RawBlobAsset* asset);

    unsigned char data[1];
};

}  // namespace World

World::RawBlobEntity::RawBlobEntity(EntityHandleBase* handle)
    : BlobEntity(handle) {
    owners = 0;
    typeID = 4;
}

World::RawBlobEntity* World::RawBlobAsset::Create(EntityHandleBase* handle,
                                                  RawBlobAsset* asset) {
    RawBlobEntity* entity = new (Memory::AllocGlobalHeap(
        sizeof(RawBlobEntity), (Memory::GlobalHeapEnum)0, (eMemMgrTag)66,
        false)) RawBlobEntity(handle);

    entity->asset = asset;
    entity->assetSize = handle->blobSize;
    entity->owners = 0;
    entity->deferDestroy = false;

    return entity;
}

void World::RawBlobEntity::Deactivate() {
    if (deferDestroy) {
        void (RawBlobEntity::*destroy)() = &RawBlobEntity::Destroy;

        (this->*destroy)();
    } else {
        Deactivate__Q25World9xOGEntityFv(this);
    }
}
