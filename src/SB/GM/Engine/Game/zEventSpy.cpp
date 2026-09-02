// zEventSpy.cpp -- six functions, read from the image with
// tools/disasm.py. An event spy is an entity that watches events:
// its asset's Create takes 80 bytes from the global heap (tag 16),
// clears them, constructs the spy there and calls its Init through
// the vtable; Init runs xBaseInit, keeps the asset and installs the
// event wrapper as the entity's event function; the wrapper hands the
// event to the spy's HandleEvent through the vtable. HandleEvent sets
// the entity's first flag bit on one event id, clears it on another,
// and on two more tail-calls the virtual after Init. Setup clears the
// trap count and DestroySpyList clears the two class statics.
//
// Layout from the DWARF (zEventSpy 0x50 on xOGEntity 0x3C, the asset
// at +0x3C and the count at +0x40; xBase's event function at +0x30 and
// its flag halfword at +0x26). The vtable slots are zPOWObject's: Init
// at 20, the triggered virtual at 21, HandleEvent at 23. The statics
// are data of this unit's own, so it matches and does not link.

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {
enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };

void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap, eMemMgrTag tag,
                      bool clear);
}  // namespace Memory

extern "C" void* memset(void* dst, int c, unsigned long n);

inline void* operator new(unsigned long, void* p) { return p; }

class xBase;

namespace Sext {
class xBaseAsset;
class EventAny;
class zEventSpyAsset;
}  // namespace Sext

void xBaseInit(xBase* base, const Sext::xBaseAsset* asset);
void EventSpyEventWrapper(xBase* from, xBase* to, unsigned int event,
                          Sext::EventAny* any);

namespace World {

class EntityHandleBase;

class xOGEntity {
public:
    xOGEntity(EntityHandleBase* handle);
};

}  // namespace World

// The entity, with the three virtuals this unit dispatches through at
// their slots; the slots before them exist only to put them there.
class xBase {
public:
    virtual void _v0();   virtual void _v1();   virtual void _v2();
    virtual void _v3();   virtual void _v4();   virtual void _v5();
    virtual void _v6();   virtual void _v7();   virtual void _v8();
    virtual void _v9();   virtual void _v10();  virtual void _v11();
    virtual void _v12();  virtual void _v13();  virtual void _v14();
    virtual void _v15();  virtual void _v16();  virtual void _v17();
    virtual void _v18();  virtual void _v19();
    virtual void Init(Sext::zEventSpyAsset* asset);
    virtual void OnTriggered();
    virtual void _v22();
    virtual void HandleEvent(xBase* from, unsigned int event,
                             Sext::EventAny* any);

    unsigned char _pad0[0x22];
    unsigned short baseFlags;
    unsigned char _pad1[0x8];
    void (*eventFunc)(xBase* from, xBase* to, unsigned int event,
                      Sext::EventAny* any);
};

class zEventSpy : public World::xOGEntity {
public:
    zEventSpy(World::EntityHandleBase* handle) : World::xOGEntity(handle) {}

    virtual void _v0();   virtual void _v1();   virtual void _v2();
    virtual void _v3();   virtual void _v4();   virtual void _v5();
    virtual void _v6();   virtual void _v7();   virtual void _v8();
    virtual void _v9();   virtual void _v10();  virtual void _v11();
    virtual void _v12();  virtual void _v13();  virtual void _v14();
    virtual void _v15();  virtual void _v16();  virtual void _v17();
    virtual void _v18();  virtual void _v19();
    virtual void Init(Sext::zEventSpyAsset* asset);
    virtual void OnTriggered();
    virtual void _v22();
    virtual void HandleEvent(xBase* from, unsigned int event,
                             Sext::EventAny* any);

    void Setup();

    static void DestroySpyList();

    static int spyCount;
    static zEventSpy* spyList;

    // The DWARF puts the asset at +0x3C, inside xOGEntity's 0x40: the
    // base's tail padding, which the compiler reuses. The entity is
    // 8-aligned, so the spy is 0x50 and not 0x4C; the alignment rides
    // on the count.
    unsigned char _pad0[0x38];
    Sext::zEventSpyAsset* asset;
    int timesTrapped __attribute__((aligned(8)));
    bool FlagLog;
    bool FlagLogStack;
    bool FlagAssert;
    bool FlagAssertOnce;
    bool FlagSound;
    bool FlagSkip;
};

int zEventSpy::spyCount;
zEventSpy* zEventSpy::spyList;

namespace Sext {

class zEventSpyAsset {
public:
    static zEventSpy* Create(World::EntityHandleBase* handle,
                             zEventSpyAsset* asset);
};

}  // namespace Sext

zEventSpy* Sext::zEventSpyAsset::Create(World::EntityHandleBase* handle,
                                        zEventSpyAsset* asset) {
    zEventSpy* spy = new (memset(
        Memory::AllocGlobalHeap(sizeof(zEventSpy), (Memory::GlobalHeapEnum)0,
                                (eMemMgrTag)16, false),
        0, sizeof(zEventSpy))) zEventSpy(handle);

    spy->Init(asset);

    return spy;
}

void EventSpyEventWrapper(xBase* from, xBase* to, unsigned int event,
                          Sext::EventAny* any) {
    to->HandleEvent(from, event, any);
}

void zEventSpy::Init(Sext::zEventSpyAsset* spyAsset) {
    xBaseInit((xBase*)this, (const Sext::xBaseAsset*)spyAsset);
    asset = spyAsset;
    ((xBase*)this)->eventFunc = EventSpyEventWrapper;
}

void zEventSpy::Setup() {
    timesTrapped = 0;
}

void zEventSpy::HandleEvent(xBase* from, unsigned int event,
                            Sext::EventAny* any) {
    switch (event) {
    case 0x2C9D0683:
        ((xBase*)this)->baseFlags |= 1;
        break;
    case 0x71E42988:
        ((xBase*)this)->baseFlags &= ~1;
        break;
    case 0x389E01C0:
    case 0xA8B93047:
        OnTriggered();
        break;
    }
}

void zEventSpy::DestroySpyList() {
    spyCount = 0;
    spyList = 0;
}
