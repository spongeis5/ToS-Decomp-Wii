// zNavMarker.cpp -- two functions, read from the image with
// tools/disasm.py. Sext::NavMarker::Create takes 88 bytes from the
// global heap (heap 0, tag 16), clears them, constructs a zNavMarker
// there when the block is not null (the placement new's own test on
// memset's return), runs xBaseInit on it with the asset, stores the
// asset and returns. IsOn is the wall net's flag when there is a wall
// net. Layout from the DWARF (zNavMarker 0x58 on xOGEntity 0x3C);
// the undefined virtual ahead of the constructor keeps the vtable's
// home elsewhere.

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
class NavMarker;
}  // namespace Sext

void xBaseInit(xBase* base, const Sext::xBaseAsset* asset);

class xVec3 {
public:
    float x;
    float y;
    float z;
};

class zWallNet {
public:
    unsigned char _pad0[0x40];
    bool on;
};

namespace World {

class EntityHandleBase;

class xOGEntity {
public:
    xOGEntity(EntityHandleBase* handle);
};

}  // namespace World

class zNavMarker : public World::xOGEntity {
public:
    zNavMarker(World::EntityHandleBase* handle) : World::xOGEntity(handle) {}

    virtual void __key();

    bool IsOn() const;

    unsigned char _pad0[0x38];
    Sext::NavMarker* asset;
    xVec3 pos;
    zWallNet* wallNet;
    int triID;
    unsigned char _pad1[0x4];
};

namespace Sext {

class NavMarker {
public:
    static zNavMarker* Create(World::EntityHandleBase* handle,
                              NavMarker* asset);
};

}  // namespace Sext

zNavMarker* Sext::NavMarker::Create(World::EntityHandleBase* handle,
                                    NavMarker* asset) {
    zNavMarker* marker = new (memset(
        Memory::AllocGlobalHeap(sizeof(zNavMarker), (Memory::GlobalHeapEnum)0,
                                (eMemMgrTag)16, false),
        0, sizeof(zNavMarker))) zNavMarker(handle);

    xBaseInit((xBase*)marker, (const Sext::xBaseAsset*)asset);
    marker->asset = asset;

    return marker;
}

bool zNavMarker::IsOn() const { return wallNet != 0 && wallNet->on; }
