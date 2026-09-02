// zEnv.cpp -- two functions, read from the image with tools/disasm.py.
// Sext::xEnvAsset::Create takes 64 bytes from the global heap (heap 0,
// tag 16), clears them, constructs a zEnv there when the block is not
// null (the placement new's own test on memset's return), and hands it
// to zEnvInit. zEnvInit runs xBaseInit, stores the asset, and if the
// current scene has no environment yet makes this one it, hands Havok
// the world bounds as two four-vectors built from the asset's min and
// max with a zero w, and clears the environment light kit. The two
// vectors are 16-byte aligned locals, which is the dynamic frame
// alignment in the prologue. Layouts from the DWARF (zEnv 0x40 on
// xOGEntity, xEnvAsset 0xA0 with MinBounds at +0x64 and MaxBounds at
// +0x70); the scene pointer at globals+0x43C and the environment slot
// at scene+0xCD0 are the loads.

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {
enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };

void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap, eMemMgrTag tag,
                      bool clear);
}  // namespace Memory

extern "C" void* memset(void* dst, int c, unsigned long n);

inline void* operator new(unsigned long, void* p) { return p; }

class xBase;
class zEnv;

namespace World {
class EntityHandleBase;
}  // namespace World

namespace Sext {
class xBaseAsset;
class xEnvAsset;
}  // namespace Sext

void xBaseInit(xBase* base, const Sext::xBaseAsset* asset);

class hkVector4 {
public:
    float x;
    float y;
    float z;
    float w;
};

namespace Math {

// Sixteen-byte aligned, which is the dynamic frame alignment in the
// prologue of a function with one on its stack.
class Vector4 {
public:
    void Assign(float x, float y, float z, float w);

    operator const hkVector4&() const { return *(const hkVector4*)this; }

    float v[4] __attribute__((aligned(16)));
};

}  // namespace Math

void xHavok_SetWorldSize(const hkVector4& min, const hkVector4& max);

class vec3 {
public:
    float x;
    float y;
    float z;
};

namespace Sext {

class xEnvAsset {
public:
    unsigned char _pad0[0x64];
    vec3 MinBounds;
    vec3 MaxBounds;
    unsigned char _pad1[0x24];

    static zEnv* Create(World::EntityHandleBase* handle, xEnvAsset* asset);
};

}  // namespace Sext

namespace World {

class EntityHandleBase;

class xOGEntity {
public:
    xOGEntity(EntityHandleBase* handle);
};

}  // namespace World

class zEnv : public World::xOGEntity {
public:
    zEnv(World::EntityHandleBase* handle) : World::xOGEntity(handle) {}

    virtual void __key();

    unsigned char _pad0[0x38];
    Sext::xEnvAsset* envAsset;
};

class xScene {
public:
    unsigned char _pad0[0xCD0];
    zEnv* env;
};

class zGlobals {
public:
    unsigned char _pad0[0x43C];
    xScene* sceneCur;
};

extern zGlobals globals;

namespace Globals {
extern void* g_EnviromentLightKit;
}  // namespace Globals

void zEnvInit(xBase* base, Sext::xEnvAsset* asset);

zEnv* Sext::xEnvAsset::Create(World::EntityHandleBase* handle,
                              xEnvAsset* asset) {
    zEnv* env = new (memset(
        Memory::AllocGlobalHeap(sizeof(zEnv), (Memory::GlobalHeapEnum)0,
                                (eMemMgrTag)16, false),
        0, sizeof(zEnv))) zEnv(handle);

    zEnvInit((xBase*)env, asset);

    return env;
}

void zEnvInit(xBase* base, Sext::xEnvAsset* asset) {
    xBaseInit(base, (const Sext::xBaseAsset*)asset);
    ((zEnv*)base)->envAsset = asset;

    if (globals.sceneCur->env == 0) {
        globals.sceneCur->env = (zEnv*)base;

        Math::Vector4 min;
        Math::Vector4 max;

        min.Assign(asset->MinBounds.x, asset->MinBounds.y, asset->MinBounds.z,
                   0.0f);
        max.Assign(asset->MaxBounds.x, asset->MaxBounds.y, asset->MaxBounds.z,
                   0.0f);
        xHavok_SetWorldSize(min, max);

        Globals::g_EnviromentLightKit = 0;
    }
}
