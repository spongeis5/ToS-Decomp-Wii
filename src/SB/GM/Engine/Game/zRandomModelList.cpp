// zRandomModelList.cpp -- four functions, read from the image with
// tools/disasm.py. A random model list is an entity that holds an asset
// listing models with weights: the asset's Create takes one 72-byte
// block from the global heap (tag 16), places the entity on it and runs
// Init THROUGH THE VTABLE; Init runs xBaseInit, keeps the asset, installs
// the shared entity-connector event wrapper, points the link array at
// the asset's event links, and sums the list's weights into the entity.
// DebugReset looks the asset up again by its own uid and resets the base
// with it. HandleEvent answers two ids and tail-branches to each.
//
// Layouts from the DWARF (tools/dwarf_types.py): xBase 0x38 with the
// link array at +0x28 and the event function at +0x30; xOGEntity 0x40;
// zRandomModelList 0x48 with the asset at +0x3C, inside the base's tail
// padding, and the total weight at +0x40, so the object is 72 bytes and
// not 68 -- the alignment rides on that float. zRandomModelListAsset
// 0x20 with the model list's count and data at +0x10 and +0x14 and the
// event links at +0x18. The list's entries are 80 bytes with the weight
// at +0x40; the DWARF describes the array as a count and an untyped
// pointer, so the entry is declared here as that stride and that float.
//
// The vtable's own slots are recovered fact: the image's __vt at
// 806C3068 holds two leading zero words, then the virtuals, so Init is
// the 21st virtual (slot 22, offset 88, which is the offset Create
// calls) and HandleEvent the 22nd. The slots before them exist only to
// put them there, and the first is left undefined so the vtable's home
// stays in the unity unit's data.
//
// Two shapes the bytes fixed. DebugReset reads the asset's uid into a
// local before the manager call, since retail loads it while `this` is
// still in r3.
//
// And the weight loop reads this->asset ONCE an iteration. Retail
// loads it in the test and the body uses that same register for the
// entry array; written the obvious way -- the count in the condition
// and the array in the body, both through the member -- the compiler
// emits a second load at the body's head and the function is 37 of 38
// words. A while loop, the array in a local, the whole asset in a
// local inside the body, and a for(;;) with a break all leave that
// load where it is (the last is worse, 33). Assigning the local in the
// CONDITION, before the comparison, is what gives the test and the
// body one value. The count is signed: retail compares with cmpw, and
// an unsigned count and index give cmplw.

typedef unsigned long long uid;

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {
enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };

void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap, eMemMgrTag tag,
                      bool clear);
}  // namespace Memory

extern "C" void* memset(void* dst, int c, unsigned long n);

inline void* operator new(unsigned long, void* p) { return p; }

class TemplateEntity;
class xBase;
class zRandomModelList;

namespace World {
class EntityHandleBase;
}

namespace Sext {

class EventAny;

class uidAsset {
public:
    uid id;
};

class xBaseAsset {
public:
    uid id;
    unsigned int baseType;
    unsigned short linkCount;
    unsigned short baseFlags;
};

class LinkAsset {
public:
    unsigned int count;
    unsigned int data;
};

// The entries the model list points at: 80 bytes each, and only the
// weight at +0x40 is read here.
class RandomModel {
public:
    unsigned char _pad0[0x40];
    float weight;
    unsigned char _pad1[0xC];
};

class zRandomModelListAsset : public xBaseAsset {
public:
    static zRandomModelList* Create(World::EntityHandleBase* handle,
                                    zRandomModelListAsset* asset);

    int count;
    RandomModel* models;
    LinkAsset EventLinksNew;
};

}  // namespace Sext

namespace World {

class EntityManager {
public:
    static Sext::xBaseAsset* FindAsset(uid id);
};

EntityManager* GetEntityManager();

}  // namespace World

void xBaseInit(xBase* base, const Sext::xBaseAsset* asset);
void xBaseReset(xBase* base, Sext::xBaseAsset* asset);

// The wrapper this entity installs belongs to another unit.
void zPlayerLocationEntConnectorEventWrapper(xBase* from, xBase* to,
                                             unsigned int event,
                                             Sext::EventAny* any);

class xBase {
public:
    virtual void _v0();

    unsigned char _pad0[0x24];
    Sext::LinkAsset* linkArray;
    TemplateEntity* templateParent;
    void (*eventFunc)(xBase* from, xBase* to, unsigned int event,
                      Sext::EventAny* any);
};

namespace World {

class xOGModelHandle {
public:
    void* model;
    unsigned int _pad0;
};

class xOGEntity : public xBase {
public:
    xOGEntity(EntityHandleBase* handle);

    xOGModelHandle ogModel;
};

}  // namespace World

class zRandomModelList : public World::xOGEntity {
public:
    zRandomModelList(World::EntityHandleBase* handle)
        : World::xOGEntity(handle) {}

    virtual void _v0();   virtual void _v1();   virtual void _v2();
    virtual void _v3();   virtual void _v4();   virtual void _v5();
    virtual void _v6();   virtual void _v7();   virtual void _v8();
    virtual void _v9();   virtual void _v10();  virtual void _v11();
    virtual void _v12();  virtual void _v13();  virtual void _v14();
    virtual void _v15();  virtual void _v16();  virtual void _v17();
    virtual void _v18();  virtual void _v19();
    virtual void Init(void* asset);
    virtual void HandleEvent(xBase* from, unsigned int event,
                             Sext::EventAny* any);

    void DebugReset();

    Sext::zRandomModelListAsset* asset;
    float totalWeight __attribute__((aligned(8)));
};

zRandomModelList* Sext::zRandomModelListAsset::Create(
    World::EntityHandleBase* handle, zRandomModelListAsset* asset) {
    zRandomModelList* list = new (memset(
        Memory::AllocGlobalHeap(sizeof(zRandomModelList),
                                (Memory::GlobalHeapEnum)0, (eMemMgrTag)16,
                                false),
        0, sizeof(zRandomModelList))) zRandomModelList(handle);

    list->Init(asset);

    return list;
}

void zRandomModelList::Init(void* assetData) {
    Sext::zRandomModelListAsset* a = (Sext::zRandomModelListAsset*)assetData;
    int i;
    Sext::zRandomModelListAsset* list;

    xBaseInit(this, a);

    asset = a;
    eventFunc = zPlayerLocationEntConnectorEventWrapper;
    linkArray = &a->EventLinksNew;
    totalWeight = 0.0f;

    for (i = 0; list = asset, i < list->count; i++) {
        totalWeight += list->models[i].weight;
    }
}

void zRandomModelList::DebugReset() {
    uid id = asset->id;

    Sext::zRandomModelListAsset* found =
        (Sext::zRandomModelListAsset*)World::GetEntityManager()->FindAsset(id);

    asset = found;
    linkArray = &found->EventLinksNew;

    xBaseReset(this, found);
}

void zRandomModelList::HandleEvent(xBase* from, unsigned int event,
                                   Sext::EventAny* any) {
    switch (event) {
    case 0x389E01C0:
        DebugReset();
        break;
    case 0xA8B93047:
        xBaseReset(this, asset);
        break;
    }
}
