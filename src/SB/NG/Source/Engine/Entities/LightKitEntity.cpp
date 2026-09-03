// LightKitEntity.cpp -- five functions, read from the image with
// tools/disasm.py. A light-kit entity wraps a Graphics::LightKit node: the
// asset's Create takes one 168-byte block from the global heap (heap 0, tag
// 20), places the entity on it, and -- when the asset names a lookup
// texture -- finds that uid's handle, activates it, and keeps its entity;
// then it builds the kit from the asset, passing the lookup entity's
// texture when there is one and null when there is not, and attaches the
// node. The constructor is the Entity base, the entity vtable, the kit (a
// Node of type 50, its own vtable, and its lookup, next and prev cleared),
// the type id 19 and a null lookup entity. Deactivate releases the lookup
// texture's handle and defers; DeferDestroy detaches the node and calls
// Destroy through a pointer to member; Destroy runs the virtual destructor
// and frees the block to the entity pool through DeleteArray, whose pool
// enum comes by reference (the static temporary retail loads at @50259).
//
// Layouts from the DWARF (tools/dwarf_types.py): Entity 0x18 with the type
// id at +0x10 and the handle at +0x14; Node 0xC {vptr, type, three flags};
// LightKitData 0x80 with lookup, next and prev at +0x70, +0x74 and +0x78;
// LightKit 0x8C on Node with that data at +0xC; LightKitEntity 0xA8 with
// the kit at +0x18 and the lookup entity at +0xA4, which is why Destroy's
// size is the constant 168 and not a member; LightKitAsset 0x90 with the
// texture uid at +0; EntityHandleBase 0x48 with its entity at +0x38;
// TextureResourceEntity 0x30 with its container at +0x18, and
// TextureContainer 0x4C whose Texture sits at +4 -- which is the whole of
// retail's `lwz r5,24(r5); addi r5,r5,4`. The destructor is left undefined
// so the vtable's home stays where retail has it, and the DeleteArray
// instance this unit emits is a weak copy of the one the image already
// holds, byte-identical to it.
//
// Three shapes the bytes fixed.
//
// The kit's constructor is INLINE and forced, and the pragma that forces it
// acts at the CALL SITE. Retail's entity constructor calls Node's, stores
// the kit's vtable and clears three pointers all in line, and -inline auto
// declines a constructor with that many stores (the same wall `ClipEntity`
// hit), so `#pragma always_inline on` covers the one function that
// constructs a kit; left on past it, the entity constructor is inlined into
// Create as well, where retail has a `bl` -- 43 of 45 words. Put around the
// kit constructor's DEFINITION instead the pragma does nothing at all: the
// kit comes out as a separate 80-byte `__ct__Q28Graphics8LightKitFv` that
// retail does not have. That cost four sweeps measuring the wrong file, and
// the tell was the EXTRA line in unitcmp, which is the tool saying so.
//
// The lookup texture is read back THROUGH THE MEMBER after the branch that
// sets it: retail loads +0xA4 again to test it for the kit's argument,
// where a local kept across the `if` would still be in a register.
//
// The texture argument is a conditional EXPRESSION with the null side last
// -- retail tests, falls into the two loads, and jumps over a trailing
// `li r5,0` -- and the referrer is a named default-initialised local, since
// a temporary of an empty class would zero its byte on the stack.
//
// NEAR MISS -- the constructor, 18 of 29 words aligned, ours 28 words.
// Retail keeps the KIT's address in a callee-saved register of its own:
// `addi r31,r30,24` once, then the Node call takes `mr r3,r31` and the four
// stores that follow (its vtable and the three cleared pointers) go through
// r31 at +0, +124, +128 and +132. Ours folds that address away -- the
// argument is built straight into r3 and the same four stores become
// +24, +148, +152 and +156 off the entity -- which is one word shorter and
// costs a register; with retail's `this` in r30 and ours in r31, every word
// naming a register differs after it. The rest of the function is
// instruction for instruction the same, and the DWARF line table
// (tools/dwarf_lines.py) confirms the source shape: lines 11, 13, 15 and 17,
// with all of the kit's construction attributed to the constructor's own
// line 11, which is where this producer puts inlined code.
//
// Eleven spellings and ten pragmas were measured against a base where the
// kit really is inlined, and every one gives the same 28 words. The
// spellings: the stores through a local `LightKit*`, through a
// `LightKitData&`, through `this->`, through a
// `(LightKit*)((char*)this + 24)` cast, chained (`a = b = 0`), forwarded to
// an inline `Clear()` member, moved into the entity's own body both by the
// member path and through a named local, handed to an inline free helper
// taking a pointer, the base initialiser written as a real enumerator
// rather than a cast, and the plain form kept here. The pragmas:
// `opt_propagation off`, `opt_common_subs off`, `opt_lifetimes off`,
// `opt_dead_assignments off`, `opt_strength_reduction off`,
// `inline_bottom_up on`, `inline_depth(1)` and `(2)` change nothing, while
// `peephole off` and `scheduling off` break Create instead (38 and 12 words
// out) without moving this one. What is left is the compiler's own choice
// to address an inlined member constructor's `this` as a displacement of
// the enclosing object, and no spelling tried reaches it.

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {
enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };
enum EntityPoolEnum { EntityPool = 0 };

void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap, eMemMgrTag tag,
                      bool clear);

void FreeGlobalHeap(void* block, GlobalHeapEnum heap);

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

namespace Util {

// Empty, and passed as a named default-initialised local.
class Referrer {};

}  // namespace Util

typedef unsigned long long uid;

namespace World {
class LightKitAsset;
class TextureResourceEntity;
}  // namespace World

namespace Graphics {

class LightKitData;

// Only its ADDRESS is taken here, but the container holds one by value, so
// the DWARF's 0x48 stands in for the members this unit never touches.
class Texture {
public:
    unsigned char _pad0[0x48];
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

class LightKitData {
public:
    unsigned char _pad0[0x70];
    Texture* lookup;
    LightKitData* next;
    LightKitData* prev;
    unsigned char index;
};

class LightKit : public Node {
public:
    LightKit() : Node((NodeTypeEnum)50) {
        data.lookup = 0;
        data.next = 0;
        data.prev = 0;
    }

    void Create(const World::LightKitAsset& asset, Texture* lookup);

    LightKitData data;
};

}  // namespace Graphics

namespace World {

class EntityHandleBase;

class TextureContainer {
public:
    unsigned char _pad0[0x4];
    Graphics::Texture texture;
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

class EntityHandleBase {
public:
    void Activate(const Util::Referrer& referrer);
    void Deactivate(const Util::Referrer& referrer);

    unsigned char _pad0[0x38];
    Entity* entity;
};

class EntityManager {
public:
    static EntityHandleBase* FindHandle(uid id);
};

class TextureResourceEntity : public Entity {
public:
    TextureContainer* texMem;
};

class LightKitEntity : public Entity {
public:
    LightKitEntity(EntityHandleBase* handle);

    virtual ~LightKitEntity();
    virtual void Deactivate();

    void DeferDestroy();
    void Destroy();

    Graphics::LightKit lightKit;
    TextureResourceEntity* lookupTexEnt;
};

class LightKitAsset {
public:
    static LightKitEntity* Create(EntityHandleBase* handle,
                                  LightKitAsset* asset);

    uid lookupTex;
    unsigned char _pad0[0x90 - 0x8];
};

}  // namespace World

// The kit's constructor is inlined HERE and nowhere else: the pragma acts
// at the CALL SITE, and left on it would inline this constructor into
// Create too, which retail calls out of line.
#pragma always_inline on

World::LightKitEntity::LightKitEntity(EntityHandleBase* handle)
    : Entity(handle) {
    typeID = 19;
    lookupTexEnt = 0;
}

#pragma always_inline off

World::LightKitEntity* World::LightKitAsset::Create(EntityHandleBase* handle,
                                                    LightKitAsset* asset) {
    LightKitEntity* entity =
        new (Memory::AllocGlobalHeap(sizeof(LightKitEntity),
                                     (Memory::GlobalHeapEnum)0, (eMemMgrTag)20,
                                     false)) LightKitEntity(handle);

    if (asset->lookupTex != 0) {
        EntityHandleBase* found = EntityManager::FindHandle(asset->lookupTex);

        Util::Referrer referrer;

        found->Activate(referrer);
        entity->lookupTexEnt = (TextureResourceEntity*)found->entity;
    }

    entity->lightKit.Create(*asset,
                            entity->lookupTexEnt
                                ? &entity->lookupTexEnt->texMem->texture
                                : 0);
    entity->lightKit.Attach();

    return entity;
}

void World::LightKitEntity::Deactivate() {
    if (lookupTexEnt) {
        Util::Referrer referrer;

        lookupTexEnt->handle->Deactivate(referrer);
    }

    DeferDestroy();
}

void World::LightKitEntity::DeferDestroy() {
    lightKit.Detach();

    void (LightKitEntity::*destroy)() = &LightKitEntity::Destroy;

    (this->*destroy)();
}

void World::LightKitEntity::Destroy() {
    this->~LightKitEntity();

    DeleteArray(Memory::EntityPool, (unsigned char*)this,
                sizeof(LightKitEntity));
}
