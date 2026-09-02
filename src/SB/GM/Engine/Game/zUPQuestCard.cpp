// zUPQuestCard.cpp -- one function, read from the image with
// tools/disasm.py: Sext::zUpQuestCardAsset::Create takes 64 bytes from
// the global heap (heap 0, tag 16), clears them, and constructs a
// zUpQuestCardDummy there when the block is not null -- the base
// entity's constructor, then the dummy's vtable. The null test is the
// one the placement new inserts itself on the pointer memset returns;
// an if around the new adds a second. The asset argument is not used;
// the handle is.

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {
enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };

void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap, eMemMgrTag tag,
                      bool clear);
}  // namespace Memory

extern "C" void* memset(void* dst, int c, unsigned long n);

inline void* operator new(unsigned long, void* p) { return p; }

namespace World {

class EntityHandleBase;

class xOGEntity {
public:
    xOGEntity(EntityHandleBase* handle);
};

}  // namespace World

class zUpQuestCardDummy : public World::xOGEntity {
public:
    zUpQuestCardDummy(World::EntityHandleBase* handle)
        : World::xOGEntity(handle) {}

    virtual void __vtable_anchor();

    unsigned char _pad0[0x3C];
};

namespace Sext {

class zUpQuestCardAsset {
public:
    static zUpQuestCardDummy* Create(World::EntityHandleBase* handle,
                                     zUpQuestCardAsset* asset);
};

}  // namespace Sext

zUpQuestCardDummy* Sext::zUpQuestCardAsset::Create(
    World::EntityHandleBase* handle, zUpQuestCardAsset* asset) {
    return new (memset(Memory::AllocGlobalHeap(sizeof(zUpQuestCardDummy),
                                               (Memory::GlobalHeapEnum)0,
                                               (eMemMgrTag)16, false),
                       0, sizeof(zUpQuestCardDummy)))
        zUpQuestCardDummy(handle);
}
