// SkeletonBlobEntity.cpp -- four functions, read from the image with
// tools/disasm.py. A skeleton blob is an entity wrapping a
// Graphics::Skeleton node. The asset's Create sizes one block for the
// entity and one byte per evaluator (global heap, tag 52), hands it to a
// static stack allocator on the stack, takes the entity and then the
// evaluator callbacks from it, zeroes the callbacks, and builds the
// skeleton from the arrays that follow the asset -- the evaluators, the
// groups after them, the skin clusters after those, and the
// transform-to-group map after all three -- before attaching the node.
// The constructor is the blob entity base, this class's vtable, the node
// (Node type 6), the skeleton's own vtable and the type id 7. Deactivate
// detaches the node and calls Destroy through a pointer to member;
// Destroy runs the virtual destructor and frees the block to the entity
// pool, its size recomputed as the evaluator count plus the entity.
//
// Layouts from the DWARF (tools/dwarf_types.py): SkeletonBlobEntity 0x58
// on BlobEntity 0x18 with the skeleton at +0x18; Graphics::Skeleton 0x40
// on Node 0xC, with its evaluator count at +0x10, which is +0x28 of the
// entity and where Destroy reads it; SkeletonBlobAsset 0xC -- three byte
// counts, a pad, two shorts and a word -- and the three arrays follow it,
// four bytes an evaluator, four a group and thirty-two a skin cluster,
// which is what the three shifts in the bytes are.
//
// The first virtual is left undefined so the two vtables' homes stay in
// the unity unit's data, and the pointer-to-member constant Deactivate
// copies to the stack is this unit's own data, read at 806CA30C: delta
// zero, index -1, and Destroy's address.
//
// Two shapes the bytes fixed. BlobEntity DECLARES a destructor it does
// not define: an intermediate class with none of its own under a
// virtual one makes the compiler emit an implicit 80-byte destructor
// that retail has nowhere. And the skeleton's constructor is INLINE in
// its class, because retail has no such symbol -- it is folded into the
// entity's constructor as the node constructor and a vtable store.
//
// NEAR MISS, two of the five. Create is 42 of 68 words and the
// constructor 23 of 24, and both are register assignment rather than
// shape. Create's structure lines up -- allocator, placement new, the
// zeroing loop, the four array pointers, the skeleton and the attach --
// but retail keeps the ASSET in r31 and the size and entity in r30
// where ours has them the other way round, and where retail reloads
// asset->evalCount for the array arithmetic ours reuses the byte the
// loop bound already loaded and masks it. Reading the skin count into a
// local at the top is what got it from 42 to 66 of the 68 words being
// the right instructions, and declaring that local before or after the
// size changes nothing. The constructor's one word is the same kind:
// retail holds &skel in a second callee-saved register across the node
// constructor and stores the skeleton's vtable through it, where ours
// recomputes this+24. What is left to find is what makes the compiler
// keep those values rather than recompute them.

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {

enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };
enum EntityPoolEnum { EntityPool = 0 };

void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap, eMemMgrTag tag,
                      bool clear);
void FreeGlobalHeap(void* block, GlobalHeapEnum heap);

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

// The entity pool frees to the global heap; the image's instance of
// DeleteArray for it is a null test and this tail call.
inline void Free(void* block, EntityPoolEnum) {
    FreeGlobalHeap(block, (GlobalHeapEnum)0);
}

}  // namespace Memory

inline void* operator new(unsigned long, void* p) { return p; }

template <class H, class T>
void DeleteArray(const H& heap, T* array, unsigned long count) {
    if (array) {
        Memory::Free(array, heap);
    }
}

namespace Graphics {

class Node {
public:
    enum NodeTypeEnum { NodeTypeEnum_ = 0x7FFFFFFF };

    Node(NodeTypeEnum type);

    virtual void _v0();

    void Attach();
    void Detach();

    NodeTypeEnum type;
    unsigned int flags;
};

class Skeleton : public Node {
public:
    class EvaluatorInfo {
    public:
        unsigned int word;
    };

    class EvaluatorCallback {
    public:
        signed char byte;
    };

    class GroupNode {
    public:
        unsigned int word;
    };

    class SkinCluster {
    public:
        unsigned char _pad0[0x20];
    };

    // Inline: retail has no such function, it is folded into the entity's
    // constructor as the node constructor and a vtable store.
    Skeleton() : Node((NodeTypeEnum)6) {}

    void Create(const EvaluatorInfo* evals, int evalCount,
                const EvaluatorCallback* evalCBs, const GroupNode* groups,
                int groupCount, const SkinCluster* skins, int skinCount,
                int skinJointTotal, const signed char* transformToGroupMap,
                int morphWeightTotal);

    EvaluatorInfo* evals;
    int evalCount;
    EvaluatorCallback* evalCBs;
    int totalTransformCount;
    GroupNode* groups;
    int groupCount;
    SkinCluster* skins;
    int skinCount;
    int skinJointTotal;
    int morphWeightTotal;
    signed char* transformToGroupMap;
    bool userScale;
    unsigned char _pad0[0x3];
    void* hackEvalResults;
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

class BlobEntity : public Entity {
public:
    BlobEntity(EntityHandleBase* handle);

    // Declared, not defined: an intermediate class with none of its own
    // under a virtual destructor makes the compiler emit an implicit one
    // here, which retail does not have.
    virtual ~BlobEntity();
};

class SkeletonBlobEntity : public BlobEntity {
public:
    SkeletonBlobEntity(EntityHandleBase* handle);

    virtual ~SkeletonBlobEntity();
    virtual void Deactivate();

    void Destroy();

    Graphics::Skeleton skel;
};

class SkeletonBlobAsset {
public:
    static SkeletonBlobEntity* Create(EntityHandleBase* handle,
                                      SkeletonBlobAsset* asset);

    unsigned char evalCount;
    unsigned char groupCount;
    unsigned char skinCount;
    unsigned char pad;
    unsigned short skinJointTotal;
    unsigned short totalTransformCount;
    unsigned int morphWeightTotal;
};

}  // namespace World

World::SkeletonBlobEntity::SkeletonBlobEntity(EntityHandleBase* handle)
    : BlobEntity(handle) {
    typeID = 7;
}

World::SkeletonBlobEntity* World::SkeletonBlobAsset::Create(
    EntityHandleBase* handle, SkeletonBlobAsset* asset) {
    SkeletonBlobEntity* entity;
    Graphics::Skeleton::EvaluatorCallback* callbacks;
    int skinCount;
    int size;
    int i;

    skinCount = asset->skinCount;
    size = asset->evalCount + sizeof(SkeletonBlobEntity);

    Memory::StaticStackAllocator alloc;

    alloc.Create(Memory::AllocGlobalHeap(size, (Memory::GlobalHeapEnum)0,
                                         (eMemMgrTag)52, false),
                 size);

    entity = new (alloc.Alloc(sizeof(SkeletonBlobEntity)))
        SkeletonBlobEntity(handle);

    callbacks = (Graphics::Skeleton::EvaluatorCallback*)alloc.Alloc(
        asset->evalCount);

    for (i = 0; i < asset->evalCount; i++) {
        ((signed char*)callbacks)[i] = 0;
    }

    const Graphics::Skeleton::EvaluatorInfo* evals =
        (const Graphics::Skeleton::EvaluatorInfo*)(asset + 1);
    const Graphics::Skeleton::GroupNode* groups =
        (const Graphics::Skeleton::GroupNode*)(evals + asset->evalCount);
    const Graphics::Skeleton::SkinCluster* skins =
        (const Graphics::Skeleton::SkinCluster*)(groups + asset->groupCount);

    entity->skel.Create(evals, asset->evalCount, callbacks, groups,
                        asset->groupCount, skins, skinCount,
                        asset->skinJointTotal,
                        (const signed char*)(skins + asset->skinCount),
                        asset->morphWeightTotal);

    entity->skel.Attach();

    return entity;
}

void World::SkeletonBlobEntity::Deactivate() {
    skel.Detach();

    void (SkeletonBlobEntity::*destroy)() = &SkeletonBlobEntity::Destroy;

    (this->*destroy)();
}

void World::SkeletonBlobEntity::Destroy() {
    unsigned long size = skel.evalCount + sizeof(SkeletonBlobEntity);

    this->~SkeletonBlobEntity();

    DeleteArray(Memory::EntityPool, (unsigned char*)this, size);
}
