// EntityManager.cpp -- twenty-seven functions, 5,348 bytes, read from
// the image with tools/disasm.py. The entity manager owns a pool of
// EntityHandleBase and three AVL trees over them, and most of the unit's
// bytes are those trees: EmbeddedTreeAVL is instantiated TWICE, at node
// offsets 28 and 36, so Delete, BalanceLeft, BalanceRight and
// AuxiliaryDelete each appear in the image twice -- 3,064 bytes between
// them, and every byte of it is one template written once.
//
// Layouts from the DWARF (tools/dwarf_types.py): EntityHandleBase 0x48
// on Blobloid 0x1C, with three EmbeddedTreeNode at +0x1C, +0x24 and
// +0x2C -- 28, 36 and 44, which is exactly the template's third argument
// -- then typeID at +0x34, the entity at +0x38, a reference counter at
// +0x3C and the flags from +0x40. EntityMgrModule 0x9C on System::Module
// with the manager at +0x98. EntityManager 0x1C, a PoolList.
//
// MEASURED: 2 of the 7 functions this object defines are
// byte-identical, 84 bytes -- GetEntityManager and the module's
// constructor. This unit is a stub against the twenty it does not yet
// have, and it is committed for the layout work and the near misses
// rather than for its bytes.
//
// WHERE THE BYTES ARE, so the next attempt aims at the right thing:
// EmbeddedTreeAVL is instantiated TWICE, at node offsets 28 and 36,
// and Delete, BalanceLeft, BalanceRight and AuxiliaryDelete each appear
// in the image once per instantiation -- 3,064 bytes, 57% of the unit,
// from ONE template written once. That is the best bytes-per-function
// in the twelve largest blocker-free units and it is the reason to come
// back here.
//
// Two shapes the bytes fixed. System::Module's vtable pointer lands at
// +0x14, so the first virtual is declared there and the derived
// module's constructor gets its vtable store implicitly -- spelled as a
// plain member the store names a symbol that does not exist. And
// EntityManager::g_handleTypeTrees is a static member POINTER, not an
// array: retail loads it and then indexes, two loads rather than one.
//
// NEAR MISSES, none swept: Startup 9 of 20 words and one instruction
// short, Shutdown 13 of 21 and one short, EntityHandleCmp::operator()
// 10 of 16 -- the 64-bit three-way compare, where retail's subfc/subfe
// pair runs in the opposite operand order from ours -- FindEntityByType
// 3 of 13, and CountEntityByType 3 of 5.

typedef unsigned long long uid;

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {
enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };

void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap,
                      unsigned long align, eMemMgrTag tag, bool clear);
void FreeGlobalHeap(void* p, GlobalHeapEnum heap);
}  // namespace Memory

class Entity;

namespace System {

// The vtable pointer lands at +0x14, where the first virtual is
// declared -- the same spelling zSoundPhysics.cpp carries for
// zSoundAsset.
class Module {
public:
    Module();

    unsigned char _pad0[0x4];
    int state;
    unsigned char _pad1[0x14 - 0x8];

    virtual void _v0();

    unsigned char _pad2[0x98 - 0x18];
};

}  // namespace System

namespace World {

class EntityHandleBase {
public:
    uid id;
    unsigned char _pad0[0x34 - 0x8];
    unsigned int typeID;
    Entity* entity;
    unsigned char _pad1[0x48 - 0x3C];
};

class EntityHandleCmp {
public:
    int operator()(const EntityHandleBase* a, const EntityHandleBase* b) const;
};

class PoolList {
public:
    unsigned char _pad0[0x1C];
};

class TypeTree;

class EntityManager {
public:
    void Init();
    void Done();

    static EntityHandleBase* FindEntityHandleByType(unsigned int type,
                                                    unsigned int index);
    static Entity* FindEntityByType(unsigned int type, unsigned int index);
    static unsigned int CountEntityByType(unsigned int type);

    static TypeTree* g_handleTypeTrees;

    PoolList handleList;
};

class TypeTree {
public:
    unsigned int count;
    void* root;
};

class EntityMgrModule : public System::Module {
public:
    EntityMgrModule();

    virtual void Startup(int phase);
    virtual void Shutdown(int phase);

    EntityManager* entityMgr;
};

extern EntityMgrModule entityMgrMod;

EntityManager* GetEntityManager();

}  // namespace World

World::EntityManager* World::GetEntityManager() {
    return entityMgrMod.entityMgr;
}

unsigned int World::EntityManager::CountEntityByType(unsigned int type) {
    return g_handleTypeTrees[type].count;
}

Entity* World::EntityManager::FindEntityByType(unsigned int type,
                                               unsigned int index) {
    EntityHandleBase* handle = FindEntityHandleByType(type, index);

    if (handle == 0) {
        return 0;
    }

    return handle->entity;
}

int World::EntityHandleCmp::operator()(const EntityHandleBase* a,
                                       const EntityHandleBase* b) const {
    if (a->id < b->id) {
        return -1;
    }

    return b->id < a->id;
}

World::EntityMgrModule::EntityMgrModule() {
    state = 3;
}

void World::EntityMgrModule::Startup(int phase) {
    if (phase == 0) {
        entityMgr = (EntityManager*)Memory::AllocGlobalHeap(
            sizeof(EntityManager), (Memory::GlobalHeapEnum)0, 16,
            (eMemMgrTag)69, false);

        entityMgr->Init();
    }
}

void World::EntityMgrModule::Shutdown(int phase) {
    if (phase == 0) {
        entityMgr->Done();

        if (entityMgr != 0) {
            Memory::FreeGlobalHeap(entityMgr, (Memory::GlobalHeapEnum)0);
        }

        entityMgr = 0;
    }
}
