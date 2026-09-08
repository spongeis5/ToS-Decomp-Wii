// zSlope's Create was emitted by tools/transplant.py; the Init below
// was added by hand, so the generated banner is gone and this file
// counts as written. The Create is unchanged.
//
// zSlope, transplanted from Sext::zEventSpyAsset::Create, which
// is matched. 27 of 34 instructions are byte-identical and two more
// reach the same symbol from a different address; the holes are the
// allocation size, the base constructor, the vtable and the VIRTUAL
// SLOT the init is called through -- (K - 8) / 4 of the
// `lwz r12,K(r12)`, because a CodeWarrior vtable pointer points
// eight bytes past its start. All four were read from the image.
//
// A call through a vtable names no symbol, so the init's parameter
// type is the one thing here the image cannot say; it is spelled as
// the asset, which is what it is called with.
//
// The base carries nothing but its constructor and all the padding
// rides on the derived class: the vptr sits at +0 either way, and
// how sizeof splits between the two changes no code. None of the
// virtuals is defined, which is what leaves __vt__zSlope
// referenced instead of emitted.

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

class xBase;
class LinkAsset;

namespace Sext {
class xBaseAsset;
class EventAny;
}  // namespace Sext

void xBaseInit(xBase* base, const Sext::xBaseAsset* asset);
void InteractionEventWrapper(xBase* from, xBase* to, unsigned int event,
                             Sext::EventAny* any);

namespace World { class EntityHandleBase; }
namespace Sext { class zSlopeAsset; }

class zInteractionUP {
public:
    zInteractionUP(World::EntityHandleBase* handle);
};

class zSlope : public zInteractionUP {
public:
    zSlope(World::EntityHandleBase* handle) : zInteractionUP(handle) {}

    virtual void _v0();  virtual void _v1();  virtual void _v2();
    virtual void _v3();  virtual void _v4();  virtual void _v5();
    virtual void _v6();  virtual void _v7();  virtual void _v8();
    virtual void _v9();  virtual void _v10();  virtual void _v11();
    virtual void _v12();  virtual void _v13();  virtual void _v14();
    virtual void _v15();  virtual void _v16();  virtual void _v17();
    virtual void _v18();  virtual void _v19();  virtual void _v20();
    virtual void _v21();  virtual void _v22();  virtual void _v23();
    virtual void _v24();  virtual void _v25();  virtual void _v26();
    virtual void _v27();  virtual void _v28();  virtual void _v29();
    virtual void _v30();  virtual void _v31();
    virtual void Init(void* asset);

    unsigned char _pad0[0x28 - 0x4];
    LinkAsset* linkArray;
    unsigned char _pad1[0x4];
    void (*eventFunc)(xBase* from, xBase* to, unsigned int event,
                      Sext::EventAny* any);
    unsigned char _pad2[0x68 - 0x34];
    void* asset;
    unsigned char _pad3[0x88 - 0x6C];
};

namespace Sext {

class zSlopeAsset {
public:
    static ::zSlope* Create(World::EntityHandleBase* handle,
                            zSlopeAsset* asset);
};

}  // namespace Sext

zSlope* Sext::zSlopeAsset::Create(World::EntityHandleBase* handle,
                                  zSlopeAsset* asset) {
    ::zSlope* entity = new (memset(
        Memory::AllocGlobalHeap(sizeof(::zSlope), (Memory::GlobalHeapEnum)0,
                                (eMemMgrTag)16, false),
        0, sizeof(::zSlope))) ::zSlope(handle);

    entity->Init(asset);

    return entity;
}

void zSlope::Init(void* asset) {
    xBaseInit((xBase*)this, (const Sext::xBaseAsset*)asset);
    this->asset = asset;
    eventFunc = InteractionEventWrapper;
    linkArray = (LinkAsset*)((char*)asset + 88);
}
