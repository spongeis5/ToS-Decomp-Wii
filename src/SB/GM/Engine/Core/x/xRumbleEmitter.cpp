// xRumbleEmitter -- the asset Create(s) below are one shape 33 Sext assets in
// this tree share, read from the image with tools/disasm.py: take
// sizeof(T) from the global heap (heap 0, tag 16, no clear), memset
// it, and place the entity on it with the handle and the asset. Each
// entity's constructor is a CALL, so it is declared and not defined,
// and each class is padded to the size its allocation asks for --
// which is the only thing about its layout known here.
//
// tools/twin_census.py is what paired them with the written ones.
//
// What is above the first Create came from tools/gen_accessors.py
// before this file was written by hand, and is unchanged but for the
// padding a sizeof needs. The generator's banner is gone because
// gen_units.py overwrites any file that still carries it.

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
namespace World { class EntityHandleBase; }

namespace World {

class xOGEntity {
public:
    xOGEntity(World::EntityHandleBase* a0);
    virtual void __vtable_anchor();
};

}  // namespace World

namespace xRumble {

class emitterBase : public World::xOGEntity {
public:
    virtual void __vtable_anchor();
    emitterBase(World::EntityHandleBase* a0);

};

}  // namespace xRumble

#pragma dont_inline on
xRumble::emitterBase::emitterBase(World::EntityHandleBase* a0) : World::xOGEntity(a0) {}
#pragma dont_inline off

namespace xRumble {

class effectAsset;
class sphericalEmitterAsset;
class boxEmitterAsset;

class effect {
public:
    effect(World::EntityHandleBase* handle, effectAsset* asset);

    unsigned char _pad0[0x40];
};

class sphericalEmitter {
public:
    sphericalEmitter(World::EntityHandleBase* handle,
                     sphericalEmitterAsset* asset);

    unsigned char _pad0[0x48];
};

class boxEmitter {
public:
    boxEmitter(World::EntityHandleBase* handle,
               boxEmitterAsset* asset);

    unsigned char _pad0[0x48];
};

}  // namespace xRumble

namespace Sext {

class Rumble {
public:
    static xRumble::effect* Create(World::EntityHandleBase* handle,
                                     Rumble* asset);
};

class Rumble_Spherical_Emitter {
public:
    static xRumble::sphericalEmitter* Create(World::EntityHandleBase* handle,
                                               Rumble_Spherical_Emitter* asset);
};

class Rumble_Box_Emitter {
public:
    static xRumble::boxEmitter* Create(World::EntityHandleBase* handle,
                                         Rumble_Box_Emitter* asset);
};

}  // namespace Sext

xRumble::effect* Sext::Rumble::Create(World::EntityHandleBase* handle,
                                    Rumble* asset) {
    return new (memset(Memory::AllocGlobalHeap(
                           sizeof(xRumble::effect), (Memory::GlobalHeapEnum)0,
                           (eMemMgrTag)16, false),
                       0, sizeof(xRumble::effect)))
        xRumble::effect(handle, (xRumble::effectAsset*)asset);
}

xRumble::sphericalEmitter* Sext::Rumble_Spherical_Emitter::Create(World::EntityHandleBase* handle,
                                                                Rumble_Spherical_Emitter* asset) {
    return new (memset(Memory::AllocGlobalHeap(
                           sizeof(xRumble::sphericalEmitter), (Memory::GlobalHeapEnum)0,
                           (eMemMgrTag)16, false),
                       0, sizeof(xRumble::sphericalEmitter)))
        xRumble::sphericalEmitter(handle, (xRumble::sphericalEmitterAsset*)asset);
}

xRumble::boxEmitter* Sext::Rumble_Box_Emitter::Create(World::EntityHandleBase* handle,
                                                    Rumble_Box_Emitter* asset) {
    return new (memset(Memory::AllocGlobalHeap(
                           sizeof(xRumble::boxEmitter), (Memory::GlobalHeapEnum)0,
                           (eMemMgrTag)16, false),
                       0, sizeof(xRumble::boxEmitter)))
        xRumble::boxEmitter(handle, (xRumble::boxEmitterAsset*)asset);
}
