// zPortal's Create was emitted by tools/transplant.py; the Init below
// was added by hand and needed the class restructured, so the
// generated banner is gone and this file counts as written.
//
// zPortal, transplanted from
// Create__Q24Sext11xGroupAssetFPQ25World16EntityHandleBasePQ24Sext11xGroupAsset,
// which is matched. The shapes are identical instruction for
// instruction; the holes were 3, and every one of them is
// read from the retail image rather than guessed:
//
//   +0x020  li      r3,64
//   +0x030  li      r5,64
//   +0x050  addi    r3,r3,27032  = 806C6998  __vt__7zPortal
//
// The DWARF places this function in zPortal.cpp.

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

class zPortal;
class xBase;
class LinkAsset;

namespace Sext {
class xBaseAsset;
class EventAny;
}  // namespace Sext

void xBaseInit(xBase* base, const Sext::xBaseAsset* asset);
void zPortalEventCB(xBase* from, xBase* to, unsigned int event,
                    Sext::EventAny* any);

// The entity is 0x40 and the three members the Init stores to sit
// at +0x28, +0x30 and +0x3C -- inside what the xBase/xOGEntity
// scaffolding the Create was emitted with already occupied. Spelled
// as zPOWObject.cpp does, which matches: the base carries nothing
// but its constructor and every offset rides on the entity.
namespace World {

class xOGEntity {
public:
    xOGEntity(EntityHandleBase* handle);
};

}  // namespace World

namespace Sext {

class xPortalAsset {
public:
    static zPortal* Create(World::EntityHandleBase* handle,
                           xPortalAsset* asset);
};

}  // namespace Sext

class zPortal : public World::xOGEntity {
public:
    zPortal(World::EntityHandleBase* handle) : World::xOGEntity(handle) {}

    virtual void _v0();

    unsigned char _pad0[0x28 - 0x4];
    LinkAsset* linkArray;
    unsigned char _pad1[0x4];
    void (*eventFunc)(xBase* from, xBase* to, unsigned int event,
                      Sext::EventAny* any);
    unsigned char _pad2[0x8];
    Sext::xPortalAsset* asset;
};

void zPortalInit(zPortal* base, Sext::xPortalAsset* asset);

zPortal* Sext::xPortalAsset::Create(World::EntityHandleBase* handle,
                                    xPortalAsset* asset) {
    zPortal* entity = new (memset(
        Memory::AllocGlobalHeap(sizeof(zPortal), (Memory::GlobalHeapEnum)0,
                                (eMemMgrTag)16, false),
        0, sizeof(zPortal))) zPortal(handle);

    zPortalInit((::zPortal*)entity, (Sext::xPortalAsset*)asset);

    return entity;
}

void zPortalInit(zPortal* base, Sext::xPortalAsset* asset) {
    xBaseInit((xBase*)base, (const Sext::xBaseAsset*)asset);
    base->asset = asset;
    base->eventFunc = zPortalEventCB;
    base->linkArray = (LinkAsset*)((char*)asset + 56);
}
