// ClipEntity.cpp -- one function, read from the image with
// tools/disasm.py: Sext::Clip::Create takes 28 bytes from the global
// heap (heap 0, tag 48), hands them to a static stack allocator on the
// stack, takes the entity's 28 back from it (the inline Alloc: read
// the mark, advance it), constructs a ClipEntity there when the block
// is not null -- the Entity base, the vtable, the clip cleared and the
// type id set to 32, both in the body -- then stores
// the asset and returns. Layouts from the DWARF (Entity 0x18,
// StaticStackAllocator 0xC); ClipEntity itself is not named there and
// is the 28 bytes the allocation says. An undefined virtual ahead of
// the constructor keeps the vtable's home in the WAD00 blob, and
// the always-inline pragma is what puts the constructor in line.

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

// The constructor below inlines into Create only under this pragma:
// -inline auto takes it with one store in its body and declines it
// with two, and retail has it inlined, so the original was forced.
#pragma always_inline on

namespace Sext {
class Clip;
}

namespace World {

class EntityHandleBase;

class EmbeddedListNode {
public:
    EmbeddedListNode* next;
    EmbeddedListNode* prev;
};

class Entity {
public:
    Entity(EntityHandleBase* handle);

    virtual void __key();

    EmbeddedListNode ogSceneNode;
    int ogUpdateIdx;
    unsigned int typeID;
    EntityHandleBase* handle;
};

class ClipEntity : public Entity {
public:
    ClipEntity(EntityHandleBase* handle) : Entity(handle) {
        clip = 0;
        typeID = 32;
    }

    Sext::Clip* clip;
};

}  // namespace World

namespace Sext {

class Clip {
public:
    static World::ClipEntity* Create(World::EntityHandleBase* handle,
                                     Clip* asset);
};

}  // namespace Sext

World::ClipEntity* Sext::Clip::Create(World::EntityHandleBase* handle,
                                      Clip* asset) {
    Memory::StaticStackAllocator alloc;

    alloc.Create(Memory::AllocGlobalHeap(sizeof(World::ClipEntity),
                                         (Memory::GlobalHeapEnum)0,
                                         (eMemMgrTag)48, false),
                 sizeof(World::ClipEntity));

    World::ClipEntity* entity = new (alloc.Alloc(sizeof(World::ClipEntity)))
        World::ClipEntity(handle);

    entity->clip = asset;

    return entity;
}
