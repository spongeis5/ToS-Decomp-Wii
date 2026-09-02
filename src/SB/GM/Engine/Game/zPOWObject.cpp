// zPOWObject.cpp -- three functions, read from the image with
// tools/disasm.py. Sext::zPOWObjectAsset::Create takes 64 bytes from
// the global heap (heap 0, tag 16), clears them, constructs a
// zPOWObject there when the block is not null (the placement new's own
// test on memset's return), and calls its Init through the vtable.
// Init runs xBaseInit, keeps the asset, installs the direction event
// wrapper as the entity's event function and points the link array at
// the asset's event links. HandleEvent, on either of two event ids,
// tail-calls the virtual after Init. Layouts from the DWARF (zPOWObject
// 0x40 on xOGEntity, zPOWObjectAsset 0x78 with the links at +0x70;
// xBase's link array at +0x28 and event function at +0x30). The event
// wrapper is a file static in retail, so this unit matches and does
// not link.

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {
enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };

void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap, eMemMgrTag tag,
                      bool clear);
}  // namespace Memory

extern "C" void* memset(void* dst, int c, unsigned long n);

inline void* operator new(unsigned long, void* p) { return p; }

class xBase;
class LinkAsset;

namespace Sext {
class xBaseAsset;
class EventAny;
class zPOWObjectAsset;
}  // namespace Sext

void xBaseInit(xBase* base, const Sext::xBaseAsset* asset);
void DirectionEventWrapper(xBase* from, xBase* to, unsigned int event,
                           Sext::EventAny* any);

namespace World {

class EntityHandleBase;

class xOGEntity {
public:
    xOGEntity(EntityHandleBase* handle);
};

}  // namespace World

class zPOWObject : public World::xOGEntity {
public:
    zPOWObject(World::EntityHandleBase* handle) : World::xOGEntity(handle) {}

    virtual void _v0();   virtual void _v1();   virtual void _v2();
    virtual void _v3();   virtual void _v4();   virtual void _v5();
    virtual void _v6();   virtual void _v7();   virtual void _v8();
    virtual void _v9();   virtual void _v10();  virtual void _v11();
    virtual void _v12();  virtual void _v13();  virtual void _v14();
    virtual void _v15();  virtual void _v16();  virtual void _v17();
    virtual void _v18();  virtual void _v19();
    virtual void Init(Sext::zPOWObjectAsset* asset);
    virtual void OnTriggered();

    void HandleEvent(xBase* from, unsigned int event, Sext::EventAny* any);

    unsigned char _pad0[0x24];
    LinkAsset* linkArray;
    unsigned char _pad1[0x4];
    void (*eventFunc)(xBase* from, xBase* to, unsigned int event,
                      Sext::EventAny* any);
    unsigned char _pad2[0x8];
    Sext::zPOWObjectAsset* asset;
};

namespace Sext {

class zPOWObjectAsset {
public:
    static zPOWObject* Create(World::EntityHandleBase* handle,
                              zPOWObjectAsset* asset);

    unsigned char _pad0[0x70];
    unsigned char EventLinksNew[0x8];
};

}  // namespace Sext

zPOWObject* Sext::zPOWObjectAsset::Create(World::EntityHandleBase* handle,
                                          zPOWObjectAsset* asset) {
    zPOWObject* object = new (memset(
        Memory::AllocGlobalHeap(sizeof(zPOWObject), (Memory::GlobalHeapEnum)0,
                                (eMemMgrTag)16, false),
        0, sizeof(zPOWObject))) zPOWObject(handle);

    object->Init(asset);

    return object;
}

void zPOWObject::Init(Sext::zPOWObjectAsset* powAsset) {
    xBaseInit((xBase*)this, (const Sext::xBaseAsset*)powAsset);
    asset = powAsset;
    eventFunc = DirectionEventWrapper;
    linkArray = (LinkAsset*)&powAsset->EventLinksNew;
}

void zPOWObject::HandleEvent(xBase* from, unsigned int event,
                             Sext::EventAny* any) {
    if (event == 0x389E01C0 || event == 0xA8B93047) {
        OnTriggered();
    }
}
