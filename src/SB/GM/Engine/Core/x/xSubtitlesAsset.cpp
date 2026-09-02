// xSubtitlesAsset.cpp -- one function, read from the image with
// tools/disasm.py: Sext::xSubtitlesAsset::Create takes 64 bytes from
// the global heap (heap 0, tag 16), clears them, and constructs an
// xSubtitles there when the block is not null -- the base entity's
// constructor, then the subtitles vtable. The null test is the one
// the placement new inserts itself on the pointer memset returns; an
// if around the new adds a second. The asset argument is not used;
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

class xSubtitles : public World::xOGEntity {
public:
    xSubtitles(World::EntityHandleBase* handle) : World::xOGEntity(handle) {}

    virtual void __vtable_anchor();

    unsigned char _pad0[0x3C];
};

namespace Sext {

class xSubtitlesAsset {
public:
    static xSubtitles* Create(World::EntityHandleBase* handle,
                              xSubtitlesAsset* asset);
};

}  // namespace Sext

xSubtitles* Sext::xSubtitlesAsset::Create(World::EntityHandleBase* handle,
                                          xSubtitlesAsset* asset) {
    return new (memset(Memory::AllocGlobalHeap(sizeof(xSubtitles),
                                               (Memory::GlobalHeapEnum)0,
                                               (eMemMgrTag)16, false),
                       0, sizeof(xSubtitles))) xSubtitles(handle);
}
