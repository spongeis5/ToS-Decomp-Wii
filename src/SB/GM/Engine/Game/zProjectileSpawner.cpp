// zProjectileSpawner.cpp -- hand-owned since the asset's Create was
// added. The accessor part below was written by
// tools/gen_accessors.py and is kept as it was; the generator's
// banner is gone because gen_units.py overwrites any file that
// still carries it.
//
// Every function here touches nothing but its own members or a
// constant: one load, one store, the address of a member, a
// constant return, or members set to one constant. Each body was
// decoded from the image and re-encoded back to the same bytes,
// and each parameter list re-mangled back to the same symbol,
// before being written. GENERATED, not read: real matched
// functions whose offsets are recovered fact, but a count of them
// is not a count of decompiled code.
//
// Members are non-virtual, and the padding is padding -- only the
// offsets each function touches are known, not the fields between.

namespace Sext { class EventAny; }

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {
enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };

void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap, eMemMgrTag tag,
                      bool clear);
}  // namespace Memory

extern "C" {
void* memset(void* dst, int c, unsigned long n);
}

inline void* operator new(unsigned long, void* p) { return p; }

namespace World {
class EntityHandleBase;
}

class zProjectileSpawner;

// 0x38, and polymorphic: the vtable pointer the constructor stores
// sits at +0, so nothing precedes it.
class xBase {
public:
    virtual void _v0();

    unsigned char _pad0[0x34 - 0x4];
};

namespace World {

class xOGEntity : public xBase {
public:
    xOGEntity(EntityHandleBase* handle);

    unsigned char _pad0[0x40 - 0x34];
};

}  // namespace World

namespace Sext {

class ProjectileSpawner {
public:
    static ::zProjectileSpawner* Create(World::EntityHandleBase* handle,
                                        ProjectileSpawner* asset);
};

}  // namespace Sext


class zProjectileSpawner : public World::xOGEntity {
public:
    zProjectileSpawner(World::EntityHandleBase* handle)
        : World::xOGEntity(handle) {}

    virtual void _v0();

    void Init(Sext::ProjectileSpawner* asset);

    static void StaticHandleEvent(xBase* a0, xBase* a1, unsigned int a2, Sext::EventAny* a3);
    void HandleEvent(xBase* a0, unsigned int a1, Sext::EventAny* a2);

    unsigned char _pad0[0x70 - 0x40];
};

zProjectileSpawner* Sext::ProjectileSpawner::Create(
    World::EntityHandleBase* handle, ProjectileSpawner* asset) {
    ::zProjectileSpawner* entity = new (memset(
        Memory::AllocGlobalHeap(sizeof(::zProjectileSpawner),
                                (Memory::GlobalHeapEnum)0,
                                (eMemMgrTag)16, false),
        0, sizeof(::zProjectileSpawner))) ::zProjectileSpawner(handle);

    entity->Init(asset);

    return entity;
}


void zProjectileSpawner::StaticHandleEvent(xBase* a0, xBase* a1, unsigned int a2, Sext::EventAny* a3) { ((zProjectileSpawner*)a1)->HandleEvent(a1, a2, a3); }
