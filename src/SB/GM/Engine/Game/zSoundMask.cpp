// zSoundMask.cpp -- two functions, read from the image with
// tools/disasm.py. Sext::zSoundMaskAsset::Create takes 64 bytes from
// the global heap (heap 0, tag 16), clears them, constructs a
// zSoundMask there when the block is not null (the placement new's own
// test on memset's return), runs xBaseInit on it with the asset, stores
// the asset and returns. zSoundMask_SceneExit releases the FMOD
// geometry if there is one and forgets it. The class is xOGEntity plus
// the asset (0x40 bytes, the allocation); the undefined virtual keeps
// the vtable's home elsewhere.

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
class zSoundMaskAsset;
}  // namespace Sext

void xBaseInit(xBase* base, const Sext::xBaseAsset* asset);

namespace FMOD {

class Geometry {
public:
    int release();
};

}  // namespace FMOD

extern FMOD::Geometry* FmodGeometry;

namespace World {

class EntityHandleBase;

class xOGEntity {
public:
    xOGEntity(EntityHandleBase* handle);
};

}  // namespace World

class zSoundMask : public World::xOGEntity {
public:
    zSoundMask(World::EntityHandleBase* handle) : World::xOGEntity(handle) {}

    virtual void __key();

    unsigned char _pad0[0x38];
    Sext::zSoundMaskAsset* asset;
};

namespace Sext {

class zSoundMaskAsset {
public:
    static zSoundMask* Create(World::EntityHandleBase* handle,
                              zSoundMaskAsset* asset);
};

}  // namespace Sext

zSoundMask* Sext::zSoundMaskAsset::Create(World::EntityHandleBase* handle,
                                          zSoundMaskAsset* asset) {
    zSoundMask* mask = new (memset(
        Memory::AllocGlobalHeap(sizeof(zSoundMask), (Memory::GlobalHeapEnum)0,
                                (eMemMgrTag)16, false),
        0, sizeof(zSoundMask))) zSoundMask(handle);

    xBaseInit((xBase*)mask, (const Sext::xBaseAsset*)asset);
    mask->asset = asset;

    return mask;
}

void zSoundMask_SceneExit() {
    if (FmodGeometry) {
        FmodGeometry->release();
        FmodGeometry = 0;
    }
}
