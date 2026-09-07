// xLight -- the asset Create(s) below are one shape 33 Sext assets in
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

namespace Graphics {

class LightPointSpot {
public:
    void SetLightMask(unsigned short value);
    void SetViewportVisibleMask(unsigned short value);

    unsigned char _pad0[0x12];
    unsigned short f12;
    unsigned short f14;
};

}  // namespace Graphics

void Graphics::LightPointSpot::SetViewportVisibleMask(unsigned short value) { f12 = value; }
void Graphics::LightPointSpot::SetLightMask(unsigned short value) { f14 = value; }

class xLightAsset;

class xLightEnt {
public:
    xLightEnt(World::EntityHandleBase* handle, xLightAsset* asset);

    unsigned char _pad0[0xD0];
};

namespace Sext {

class LightAsset {
public:
    static xLightEnt* Create(World::EntityHandleBase* handle,
                               LightAsset* asset);
};

}  // namespace Sext

xLightEnt* Sext::LightAsset::Create(World::EntityHandleBase* handle,
                                  LightAsset* asset) {
    return new (memset(Memory::AllocGlobalHeap(
                           sizeof(xLightEnt), (Memory::GlobalHeapEnum)0,
                           (eMemMgrTag)16, false),
                       0, sizeof(xLightEnt)))
        xLightEnt(handle, (xLightAsset*)asset);
}
