// zUIImage.cpp -- hand-owned since the asset's Create was added.
// The accessor part below was written by tools/gen_accessors.py and
// is kept as it was; the generator's banner is gone because
// gen_units.py overwrites any file that still carries it.
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

class zUIImage;
namespace Sext { class zUI; }

// The base is a CALL, so its layout is never read here. It is
// declared with the vtable pointer the derived constructor stores
// at +0 and nothing else, and zUIImage carries the whole
// allocation as padding.
class zUI {
public:
    zUI(World::EntityHandleBase* handle);

    virtual void _v0();

    void Invisible();
    void Visible();
};

void zUI_Init(zUI* ui, Sext::zUI* asset);

namespace Sext {

class UI_Image {
public:
    static ::zUIImage* Create(World::EntityHandleBase* handle,
                              UI_Image* asset);
};

}  // namespace Sext



class zUIImage : public zUI {
public:
    zUIImage(World::EntityHandleBase* handle) : zUI(handle) {}

    virtual void _v0();

    void Invisible();
    void Visible();

    unsigned char _pad0[0x150 - 0x4];
};

zUIImage* Sext::UI_Image::Create(World::EntityHandleBase* handle,
                                 UI_Image* asset) {
    ::zUIImage* entity = new (memset(
        Memory::AllocGlobalHeap(sizeof(::zUIImage),
                                (Memory::GlobalHeapEnum)0,
                                (eMemMgrTag)16, false),
        0, sizeof(::zUIImage))) ::zUIImage(handle);

    zUI_Init((::zUI*)entity, (Sext::zUI*)asset);

    return entity;
}


void zUIImage::Visible() { zUI::Visible(); }
void zUIImage::Invisible() { zUI::Invisible(); }
