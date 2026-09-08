// The accessor part below was written by tools/gen_accessors.py and is
// kept unchanged; Graphics::Renderable::AmendColorMulAlpha at the foot
// was added by hand, so the generator's banner is gone -- gen_units.py
// overwrites any file that still carries it.
//
// Members are non-virtual, and the padding is padding -- only the
// offsets each function touches are known, not the fields between.
//
// AmendColorMulAlpha is in this unit and not in a renderer one because
// that is where the linker put it: the DWARF says it was DEFINED IN
// Renderable.h, at line 467, so it is an inline that some translation
// unit in this run emitted out of line, and this is the run. Nothing
// about it belongs to zPlatform.
//
// The body is its own SetColorMulAlpha called through a pointer to
// member -- the twelve-byte constant onto the stack, r12 pointed at it,
// __ptmf_scall. The constant is in the image: delta 0, vtable offset -1,
// and a third word that is SetColorMulAlpha's address. The float passes
// through untouched, because __ptmf_scall adjusts r3 and jumps.

namespace World { class EntityHandleBase; }

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

class zPlatform;
namespace Sext { class xEntAsset; }

namespace Sext {

class xPlatformAsset {
public:
    static ::zPlatform* Create(World::EntityHandleBase* handle,
                               xPlatformAsset* asset);
};

}  // namespace Sext


class xEnt {
public:
    xEnt(World::EntityHandleBase* a0);
    virtual void __vtable_anchor();
};



class zEnt : public xEnt {
public:
    virtual void __vtable_anchor();
    zEnt(World::EntityHandleBase* a0);

};


namespace Graphics {

class Renderable {
public:
    void SetColorMulAlpha(float alpha);
    void AmendColorMulAlpha(float alpha);
};

}  // namespace Graphics


#pragma dont_inline on
zEnt::zEnt(World::EntityHandleBase* a0) : xEnt(a0) {}
#pragma dont_inline off

void Graphics::Renderable::AmendColorMulAlpha(float alpha) {
    void (Renderable::*set)(float) = &Renderable::SetColorMulAlpha;

    (this->*set)(alpha);
}

// The asset's Create: one 0x1D0 block from the global heap (heap 0,
// tag 16, no clear), memset, the entity placed on it, then the free
// zPlatform_Init. The base's layout is not known here -- only that
// its constructor is a call and the vtable pointer it stores is at
// +0 -- so the padding runs from the four bytes that takes.
class zPlatform : public zEnt {
public:
    zPlatform(World::EntityHandleBase* handle) : zEnt(handle) {}

    virtual void _v0();

    unsigned char _pad0[0x1D0 - 0x4];
};

void zPlatform_Init(zPlatform* base, Sext::xEntAsset* asset);

zPlatform* Sext::xPlatformAsset::Create(World::EntityHandleBase* handle,
                                        xPlatformAsset* asset) {
    ::zPlatform* entity = new (memset(
        Memory::AllocGlobalHeap(sizeof(::zPlatform),
                                (Memory::GlobalHeapEnum)0,
                                (eMemMgrTag)16, false),
        0, sizeof(::zPlatform))) ::zPlatform(handle);

    zPlatform_Init((::zPlatform*)entity, (Sext::xEntAsset*)asset);

    return entity;
}
