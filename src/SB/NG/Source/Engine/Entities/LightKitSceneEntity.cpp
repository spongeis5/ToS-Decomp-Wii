// LightKitSceneEntity.cpp -- seven functions, read from the image with
// tools/disasm.py. A light-kit scene entity holds five light-kit entities,
// one per kit slot, and the four uids the asset named. The asset's Create
// takes one 80-byte block from the global heap (heap 0, tag 20), places the
// entity on it, clears all five slots, activates slots 1 to 4 from the
// asset's four uids and then keeps a copy of the asset. ActivateOneKit
// releases whatever is in a slot, looks the uid up and -- when the uid is
// non-zero and the lookup finds a handle -- activates it and keeps its
// entity, storing null in every other case. DeactivateOneKit releases the
// slot's kit through its own handle. Deactivate releases all five, runs the
// virtual destructor and frees the block to the entity pool through
// DeleteArray, whose pool enum comes by reference (the static temporary
// retail loads at @50329). SetNewLightkit puts ONE uid into whichever of
// the four slots its four flags select, and Reset re-activates all four
// from the copy the entity kept. The constructor is the Entity base, the
// entity vtable and the type id 20.
//
// Layouts from the DWARF (tools/dwarf_types.py): Entity 0x18 with the type
// id at +0x10 and the handle at +0x14; EntityHandleBase 0x48 with its
// entity at +0x38; LightKitSceneEntity 0x50 with lightKits[5] at +0x18 and
// the asset at +0x30 -- the four-byte hole at +0x2C is the array's end
// rounded up to the asset's 8-alignment, and it is why Deactivate's
// DeleteArray size is the constant 80; LightKitSceneAsset 0x20, four uids
// named NPC, Player, Object and Environment. The DWARF wraps each of those
// in a one-member `uid` class, but the mangled name of ActivateOneKit is
// `FiUx`, so the value that crosses the call is the raw 64-bit integer and
// the members are spelled as one here. The destructor is left undefined so
// the vtable's home stays where retail has it, and the DeleteArray instance
// this unit emits is a weak copy of the one the image already holds.
//
// Four shapes the bytes fixed.
//
// ActivateOneKit's null store is written TWICE, once in each else. Retail
// emits it at the fall-through of the inner test and again at the far L2 the
// outer test branches to, and the inner test is spelled `handle == 0` first:
// `bne` to the activating block is what an `if (x == 0) A else B` gives,
// where a `!= 0` test with the blocks the other way round swaps the two
// bodies. Folding the two stores into one at the top costs the branch pair.
//
// Create's five-slot clear is a COUNTED loop -- `mtctr 5` and `bdnz` -- and
// Deactivate's five-slot release is not, because its body makes a call and
// the counter has to survive it; there the loop keeps both an `int i` for
// the signed `cmpwi 5` and the strength-reduced byte offset beside it.
//
// The allocation's null test belongs to the placement new, not to an `if`:
// `cmpwi r3,0 ; mr r31,r3 ; beq` right after AllocGlobalHeap is what
// `new (AllocGlobalHeap(...)) LightKitSceneEntity(handle)` emits by itself,
// and the block pointer is already the result on the null path.
//
// And each of the four asset copies is an eight-byte AGGREGATE copy, not a
// 64-bit integer assignment. Both are four instructions -- load, load, store,
// store -- and both put the second-loaded register in the first store; what
// differs is which half goes first, and retail takes the LOW word first and
// stores ASCENDING where a `unsigned long long` assignment takes the high
// word first and stores descending, 16 of 67 words. What reproduces retail
// is a struct holding `unsigned int w[2]`, whose copy the compiler cannot
// scalarise. Measured and ruled out, each compiled and read back: the plain
// 64-bit assignment (the DWARF's own type, which wraps the value in a
// one-member class -- that class's implicit operator= gives the identical 16
// words, so the wrapper is not the lever); the same through a local, through
// a const reference, through a cast, with `+ 0` and `| 0` folded onto it,
// and through a pointer to the destination struct; signed `long long`
// members; a struct wrapping one `unsigned long long`; `double`; a struct of
// two NAMED words, `hi` and `lo`, which is shorter and 22 of 63; eight
// explicit 32-bit assignments in both word orders and both groupings, which
// reach 12 of 67 because the scheduler will not interleave them; and
// `entity->asset = *asset` as one struct assignment, which is 54 words.

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

// A uid as its two words. Retail's four asset copies are eight-byte
// AGGREGATE copies, not 64-bit integer assignments -- see the note above --
// and this is the view that spells one.
struct uidWords {
    unsigned int w[2];
};

namespace World {

class EntityHandleBase;
class LightKitSceneEntity;

}  // namespace World

namespace Sext {

// The DWARF's LightKitSceneAsset, 0x20 bytes: four uids in slot order.
class LightKitSceneAsset {
public:
    static World::LightKitSceneEntity* Create(World::EntityHandleBase* handle,
                                              LightKitSceneAsset* asset);

    uid NPC;
    uid Player;
    uid Object;
    uid Environment;
};

}  // namespace Sext

namespace World {

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

class LightKitEntity : public Entity {};

class LightKitSceneEntity : public Entity {
public:
    LightKitSceneEntity(EntityHandleBase* handle);

    virtual ~LightKitSceneEntity();
    virtual void Deactivate();

    void ActivateOneKit(int index, uid id);
    void DeactivateOneKit(int index);
    void SetNewLightkit(uid id, bool npc, bool player, bool object,
                        bool environment);
    void Reset();

    LightKitEntity* lightKits[5];
    Sext::LightKitSceneAsset asset;
};

}  // namespace World

World::LightKitSceneEntity::LightKitSceneEntity(EntityHandleBase* handle)
    : Entity(handle) {
    typeID = 20;
}

void World::LightKitSceneEntity::ActivateOneKit(int index, uid id) {
    DeactivateOneKit(index);

    if (id != 0) {
        EntityHandleBase* found = EntityManager::FindHandle(id);

        if (found == 0) {
            lightKits[index] = 0;
        } else {
            Util::Referrer referrer;

            found->Activate(referrer);
            lightKits[index] = (LightKitEntity*)found->entity;
        }
    } else {
        lightKits[index] = 0;
    }
}

void World::LightKitSceneEntity::DeactivateOneKit(int index) {
    if (lightKits[index]) {
        Util::Referrer referrer;

        lightKits[index]->handle->Deactivate(referrer);
    }
}

void World::LightKitSceneEntity::Deactivate() {
    for (int i = 0; i < 5; i++) {
        if (lightKits[i]) {
            Util::Referrer referrer;

            lightKits[i]->handle->Deactivate(referrer);
        }
    }

    this->~LightKitSceneEntity();

    DeleteArray(Memory::EntityPool, (unsigned char*)this,
                sizeof(LightKitSceneEntity));
}

void World::LightKitSceneEntity::SetNewLightkit(uid id, bool npc, bool player,
                                                bool object,
                                                bool environment) {
    if (npc) {
        ActivateOneKit(1, id);
    }

    if (player) {
        ActivateOneKit(2, id);
    }

    if (object) {
        ActivateOneKit(3, id);
    }

    if (environment) {
        ActivateOneKit(4, id);
    }
}

void World::LightKitSceneEntity::Reset() {
    ActivateOneKit(1, asset.NPC);
    ActivateOneKit(2, asset.Player);
    ActivateOneKit(3, asset.Object);
    ActivateOneKit(4, asset.Environment);
}

World::LightKitSceneEntity* Sext::LightKitSceneAsset::Create(
    World::EntityHandleBase* handle, LightKitSceneAsset* asset) {
    World::LightKitSceneEntity* entity = new (Memory::AllocGlobalHeap(
        sizeof(World::LightKitSceneEntity), (Memory::GlobalHeapEnum)0,
        (eMemMgrTag)20, false)) World::LightKitSceneEntity(handle);

    for (int i = 0; i < 5; i++) {
        entity->lightKits[i] = 0;
    }

    entity->ActivateOneKit(1, asset->NPC);
    entity->ActivateOneKit(2, asset->Player);
    entity->ActivateOneKit(3, asset->Object);
    entity->ActivateOneKit(4, asset->Environment);

    *(uidWords*)&entity->asset.NPC = *(uidWords*)&asset->NPC;
    *(uidWords*)&entity->asset.Player = *(uidWords*)&asset->Player;
    *(uidWords*)&entity->asset.Object = *(uidWords*)&asset->Object;
    *(uidWords*)&entity->asset.Environment = *(uidWords*)&asset->Environment;

    return entity;
}
