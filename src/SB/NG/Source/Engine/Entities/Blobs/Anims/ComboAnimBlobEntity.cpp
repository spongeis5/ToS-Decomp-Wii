// ComboAnimBlobEntity.cpp -- two functions, read from the image with
// tools/disasm.py. The constructor runs the blob entity base, stores
// the vtable, sets the type id to 30 and keeps the asset. Create takes
// 28 bytes from the global heap (heap 0, tag 53) and constructs the
// entity there when the block is not null, returning what placement
// new returns. Layouts from the DWARF (ComboAnimBlobEntity 0x1C on
// BlobEntity 0x18 on Entity 0x18); the undefined virtual ahead of the
// constructor keeps the vtable's home in the blob that has it.

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {
enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };

void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap, eMemMgrTag tag,
                      bool clear);
}  // namespace Memory

inline void* operator new(unsigned long, void* p) { return p; }

namespace World {

class EntityHandleBase;
class ComboAnimBlobAssetV2;

class Entity {
public:
    virtual void __key();

    unsigned char ogSceneNode[0x8];
    int ogUpdateIdx;
    unsigned int typeID;
    EntityHandleBase* handle;
};

class BlobEntity : public Entity {
public:
    BlobEntity(EntityHandleBase* handle);
};

class ComboAnimBlobEntity : public BlobEntity {
public:
    ComboAnimBlobEntity(EntityHandleBase* handle,
                        const ComboAnimBlobAssetV2* asset);

    ComboAnimBlobAssetV2* v2data;
};

class ComboAnimBlobAssetV2 {
public:
    static ComboAnimBlobEntity* Create(EntityHandleBase* handle,
                                       ComboAnimBlobAssetV2* asset);
};

}  // namespace World

World::ComboAnimBlobEntity::ComboAnimBlobEntity(
    EntityHandleBase* handle, const ComboAnimBlobAssetV2* asset)
    : BlobEntity(handle) {
    typeID = 30;
    v2data = (ComboAnimBlobAssetV2*)asset;
}

World::ComboAnimBlobEntity* World::ComboAnimBlobAssetV2::Create(
    EntityHandleBase* handle, ComboAnimBlobAssetV2* asset) {
    return new (Memory::AllocGlobalHeap(sizeof(ComboAnimBlobEntity),
                                        (Memory::GlobalHeapEnum)0,
                                        (eMemMgrTag)53, false))
        ComboAnimBlobEntity(handle, asset);
}
