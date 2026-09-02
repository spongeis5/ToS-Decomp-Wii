// RenderModeEntity.cpp -- six functions, read from the image with
// tools/disasm.py. A render-mode entity wraps a Graphics::RenderMode
// node: the asset's Create sizes one block for the entity and its
// state deltas (56 bytes plus eight per delta, global heap, tag 62),
// hands it to a static stack allocator on the stack, takes the entity
// and then the deltas from it, copies the deltas in from the asset
// (aligned up past its four-byte header), fills the node's layer,
// sort order, states and count, and attaches the node. The entity's
// constructor is the Entity base, the vtable, the node (Node type 7)
// and the type id 21; the node's constructor is Node and a vtable.
// Deactivate defers; DeferDestroy detaches the node and calls Destroy
// through a pointer to member; Destroy reads the block size, runs the
// virtual destructor and frees the block to the entity pool through
// DeleteArray, whose pool enum comes by reference (the static
// temporary retail loads).
//
// Layouts from the DWARF (tools/dwarf_types.py): Entity 0x18 with the
// type id at +0x10, RenderMode 0x1C on Node (vptr, type, three
// flags), the entity's block size at +0x18 and the node at +0x1C;
// StaticStackAllocator {mark, buffer, end}; RenderModeAsset four
// bytes. The destructor is left undefined here so the vtable's home
// stays where retail has it (the next unit); the DeleteArray instance
// this unit emits is a weak copy of the one the image already holds,
// byte-identical to it. The two data words are retail's WAD02 data,
// so the unit matches and does not link.
//
// Create's registers come from its declarations: every local declared
// up front, the entity and data pointers first, the byte size before
// the count; and the asset's two signed bytes are read into locals
// before the stores, since a char-typed read after an int store cannot
// be hoisted past it and retail loads both first.

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

extern "C" void* memcpy(void* dst, const void* src, unsigned long n);

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

namespace Graphics {

class StateDelta {
public:
    unsigned short id;
    unsigned char _pad0[0x6];
};

class Node {
public:
    enum NodeTypeEnum { NodeTypeEnum_ = 0x7FFFFFFF };

    Node(NodeTypeEnum type);

    virtual void _v0();

    void Attach();
    void Detach();

    NodeTypeEnum type;
    bool attached;
    bool detachPending;
    bool updateThread;
};

class RenderMode : public Node {
public:
    enum SpaceLayer { SpaceLayer_ = 0x7FFFFFFF };

    RenderMode();

    SpaceLayer spaceLayer;
    int sortOrder;
    StateDelta* renderStates;
    int renderStateCount;
};

}  // namespace Graphics

namespace World {

class EntityHandleBase;

class Entity {
public:
    Entity(EntityHandleBase* handle);

    virtual ~Entity();
    virtual void Deactivate();

    unsigned char _pad0[0xC];
    int typeID;
    unsigned char _pad1[0x4];
};

class RenderModeEntity : public Entity {
public:
    RenderModeEntity(EntityHandleBase* handle);

    virtual ~RenderModeEntity();
    virtual void Deactivate();

    void DeferDestroy();
    void Destroy();

    int dataSize;
    Graphics::RenderMode renderMode;
};

class RenderModeAsset {
public:
    static RenderModeEntity* Create(EntityHandleBase* handle,
                                    RenderModeAsset* asset);

    signed char spaceLayer;
    signed char sortOrder;
    unsigned char renderStateCount;
    unsigned char pad[1];
};

}  // namespace World

World::RenderModeEntity::RenderModeEntity(EntityHandleBase* handle)
    : Entity(handle) {
    typeID = 21;
}

Graphics::RenderMode::RenderMode() : Node((NodeTypeEnum)7) {
}

World::RenderModeEntity* World::RenderModeAsset::Create(EntityHandleBase* handle,
                                                        RenderModeAsset* asset) {
    RenderModeEntity* entity;
    Graphics::StateDelta* data;
    const Graphics::StateDelta* states;
    int stateBytes;
    unsigned int count;
    int size;

    states = (const Graphics::StateDelta*)(((unsigned int)((const char*)asset + 4) + 3) & ~3);
    count = asset->renderStateCount;
    stateBytes = count * sizeof(Graphics::StateDelta);
    size = stateBytes + sizeof(RenderModeEntity);

    Memory::StaticStackAllocator alloc;

    alloc.Create(Memory::AllocGlobalHeap(size, (Memory::GlobalHeapEnum)0,
                                         (eMemMgrTag)62, false),
                 size);

    entity = new (alloc.Alloc(sizeof(RenderModeEntity))) RenderModeEntity(handle);
    data = (Graphics::StateDelta*)alloc.Alloc(stateBytes);

    entity->dataSize = size;

    memcpy(data, states, stateBytes);

    int spaceLayer = asset->spaceLayer;
    int sortOrder = asset->sortOrder;

    entity->renderMode.spaceLayer = (Graphics::RenderMode::SpaceLayer)spaceLayer;
    entity->renderMode.sortOrder = sortOrder;
    entity->renderMode.renderStates = data;
    entity->renderMode.renderStateCount = count;
    entity->renderMode.Attach();

    return entity;
}

void World::RenderModeEntity::Deactivate() {
    DeferDestroy();
}

void World::RenderModeEntity::DeferDestroy() {
    renderMode.Detach();

    void (RenderModeEntity::*destroy)() = &RenderModeEntity::Destroy;

    (this->*destroy)();
}

void World::RenderModeEntity::Destroy() {
    unsigned long size = dataSize;

    this->~RenderModeEntity();

    DeleteArray(Memory::EntityPool, (unsigned char*)this, size);
}
