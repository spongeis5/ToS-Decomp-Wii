// ImmediatePrototypeEntity.cpp -- four functions, read from the image with
// tools/disasm.py. An immediate-prototype entity owns a run of entity
// handles, one per geometry the asset names: the asset's Create sizes one
// block for the entity and the asset's uid list (32 bytes plus eight per
// geometry, global heap, tag 60), hands it to a static stack allocator on
// the stack, takes the entity and then a four-byte handle per geometry from
// it, keeps the block size, walks the uid list (which begins at the first
// eight-aligned byte after the asset's two-byte header), looks each uid up
// and activates it, and finally stores the count. The constructor is the
// Entity base, the vtable and the type id 29. Deactivate deactivates every
// handle in that run and then calls Destroy through a pointer to member.
// Destroy reads the block size, runs the virtual destructor and frees the
// block to the entity pool through DeleteArray, whose pool enum comes by
// reference (the static temporary retail loads at @50158).
//
// Layouts from the DWARF (tools/dwarf_types.py): Entity 0x18 with the type
// id at +0x10 and the handle at +0x14; ImmediatePrototypeEntity 0x20 on
// that base with memSize at +0x18 and geomCount at +0x1C, so the handle run
// starts at `this + 1`; ImmediatePrototypeAsset is TWO bytes, one unsigned
// short geomCount, which is why the uid list is reached as (asset + 9) & ~7.
// EntityHandleBase 0x48. StaticStackAllocator {mark, buffer, end}. The
// destructor is left undefined so the vtable's home stays where retail has
// it, and the DeleteArray instance this unit emits is a weak copy of the
// one the image already holds, byte-identical to it.
//
// Four shapes the bytes fixed.
//
// The block is sized `count * sizeof(uid) + sizeof(entity)` and only
// `count * sizeof(handle)` of the tail is ever handed out -- retail shifts
// the count left three for the size and left two for the run, and both
// shifts are in the image, so the over-allocation is retail's and not a
// misread.
//
// DECLARATION ORDER picks the six callee-saved registers, and it was the
// whole of Create's first miss: written with the pointers first the body
// was already identical and 19 of 57 words were a register number, with
// the two parameters holding r27/r28 where retail holds r24/r25. The three
// ints declared AHEAD of the pointers puts them back. Twelve orders were
// measured; two reach it -- `size, handleBytes, count` and `handleBytes,
// size, count`, both with the pointers and the loop counter after -- so
// the bytes do not say which was written. The rest run 11 to 19 words out,
// and none of them changes an instruction, only a register.
//
// The lookup's result is a NAMED LOCAL: retail stores it and then activates
// through the value still in r3, where storing into the run and reading the
// element back for the call reloads it.
//
// Deactivate re-reads geomCount from the member every iteration -- retail's
// loop test loads +0x1C each time -- so the count is not hoisted into a
// local, while the run's base (`this + 1`) is computed once ahead of it.
//
// The referrer is a NAMED default-initialised local, not a temporary: a
// temporary of an empty class is value-initialised and would zero its byte
// on the stack, and retail writes nothing to the slot it passes.

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {
enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };
enum EntityPoolEnum { EntityPool = 0 };

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

namespace Memory {

void FreeGlobalHeap(void* block, GlobalHeapEnum heap);

// The entity pool frees to the global heap; the image's instance of
// DeleteArray for it is a null test and this tail call.
inline void Free(void* block, EntityPoolEnum) {
    FreeGlobalHeap(block, (GlobalHeapEnum)0);
}

}  // namespace Memory

template <class H, class T>
void DeleteArray(const H& heap, T* array, unsigned long count) {
    if (array) {
        Memory::Free(array, heap);
    }
}

namespace Util {

// Empty, and passed as a named default-initialised local.
class Referrer {};

}  // namespace Util

typedef unsigned long long uid;

namespace World {

class EntityHandleBase {
public:
    void Activate(const Util::Referrer& referrer);
    void Deactivate(const Util::Referrer& referrer);
};

class EntityManager {
public:
    static EntityHandleBase* FindHandle(uid id);
};

class Entity {
public:
    Entity(EntityHandleBase* handle);

    virtual ~Entity();
    virtual void Deactivate();

    unsigned char _pad0[0xC];
    unsigned int typeID;
    EntityHandleBase* handle;
};

class ImmediatePrototypeEntity : public Entity {
public:
    ImmediatePrototypeEntity(EntityHandleBase* handle);

    virtual ~ImmediatePrototypeEntity();
    virtual void Deactivate();

    void Destroy();

    int memSize;
    int geomCount;
};

class ImmediatePrototypeAsset {
public:
    static ImmediatePrototypeEntity* Create(EntityHandleBase* handle,
                                            ImmediatePrototypeAsset* asset);

    unsigned short geomCount;
};

}  // namespace World

World::ImmediatePrototypeEntity::ImmediatePrototypeEntity(
    EntityHandleBase* handle)
    : Entity(handle) {
    typeID = 29;
}

World::ImmediatePrototypeEntity* World::ImmediatePrototypeAsset::Create(
    EntityHandleBase* handle, ImmediatePrototypeAsset* asset) {
    int size;
    int handleBytes;
    int count;
    ImmediatePrototypeEntity* entity;
    EntityHandleBase** handles;
    const uid* geoms;
    int i;

    count = asset->geomCount;
    size = count * sizeof(uid) + sizeof(ImmediatePrototypeEntity);
    handleBytes = count * sizeof(EntityHandleBase*);

    Memory::StaticStackAllocator alloc;

    alloc.Create(Memory::AllocGlobalHeap(size, (Memory::GlobalHeapEnum)0,
                                         (eMemMgrTag)60, false),
                 size);

    entity = new (alloc.Alloc(sizeof(ImmediatePrototypeEntity)))
        ImmediatePrototypeEntity(handle);

    entity->memSize = size;

    geoms = (const uid*)(((unsigned int)(asset + 1) + 7) & ~7);
    handles = (EntityHandleBase**)alloc.Alloc(handleBytes);

    Util::Referrer referrer;

    for (i = 0; i < count; i++) {
        EntityHandleBase* found = EntityManager::FindHandle(geoms[i]);

        handles[i] = found;
        found->Activate(referrer);
    }

    entity->geomCount = count;

    return entity;
}

void World::ImmediatePrototypeEntity::Deactivate() {
    EntityHandleBase** handles = (EntityHandleBase**)(this + 1);
    int i;

    Util::Referrer referrer;

    for (i = 0; i < geomCount; i++) {
        handles[i]->Deactivate(referrer);
    }

    void (ImmediatePrototypeEntity::*destroy)() =
        &ImmediatePrototypeEntity::Destroy;

    (this->*destroy)();
}

void World::ImmediatePrototypeEntity::Destroy() {
    unsigned long size = memSize;

    this->~ImmediatePrototypeEntity();

    DeleteArray(Memory::EntityPool, (unsigned char*)this, size);
}
