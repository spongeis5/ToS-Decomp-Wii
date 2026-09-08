// WAD02_4 -- the asset Create(s) below are one shape 33 Sext assets in
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

void operator delete(void* mem);

class hkBaseObject {
public:
    virtual ~hkBaseObject();
};

namespace World {

// The base carries nothing but its constructor: the vptr zFXSpawn's
// own virtual creates sits at +0 either way, so how sizeof splits
// between the two changes no code.
class xOGEntity {
public:
    xOGEntity(EntityHandleBase* handle);
    virtual ~xOGEntity();
};

class xOGModelRefPtr {
public:
    ~xOGModelRefPtr();
};

}  // namespace World

namespace FX {

// The destructor destroys the model reference at +360 with the
// don't-delete flag and then the base at +0 with the flag clear;
// that second flag is the whole of what says base rather than
// member.
class zFXSpawn : public World::xOGEntity {
public:
    zFXSpawn(World::EntityHandleBase* a0);
    virtual void __vtable_anchor();
    ~zFXSpawn();

    unsigned char _pad0[0x168 - 0x4];
    World::xOGModelRefPtr model;
};

}  // namespace FX

namespace FX {

class zFXSpawnWithSoundAssetMultiple : public FX::zFXSpawn {
public:
    virtual void __vtable_anchor();
    zFXSpawnWithSoundAssetMultiple(World::EntityHandleBase* a0);

    ~zFXSpawnWithSoundAssetMultiple();
};

}  // namespace FX

#pragma dont_inline on
FX::zFXSpawnWithSoundAssetMultiple::zFXSpawnWithSoundAssetMultiple(World::EntityHandleBase* a0) : FX::zFXSpawn(a0) {}
#pragma dont_inline off

namespace Sext {
class FXInstance;
class FXParticleSystem;
class FXSpawn;
}  // namespace Sext

namespace FX {

class zFXInstance {
public:
    zFXInstance(World::EntityHandleBase* handle,
                Sext::FXInstance* asset);

    unsigned char _pad0[0x188];
};

class zFXSpawnObject {
public:
    zFXSpawnObject(World::EntityHandleBase* handle,
                   Sext::FXSpawn* asset);

    unsigned char _pad0[0x180];
};

namespace Particles {

class zFXParticleSystem {
public:
    zFXParticleSystem(World::EntityHandleBase* handle,
                      Sext::FXParticleSystem* asset);

    unsigned char _pad0[0xC0];
};

}  // namespace Particles

}  // namespace FX

namespace Sext {

class FXInstance {
public:
    static FX::zFXInstance* Create(World::EntityHandleBase* handle,
                                     FXInstance* asset);
};

class FXParticleSystem {
public:
    static FX::Particles::zFXParticleSystem* Create(World::EntityHandleBase* handle,
                                                      FXParticleSystem* asset);
};

class FXSpawn {
public:
    static FX::zFXSpawnObject* Create(World::EntityHandleBase* handle,
                                        FXSpawn* asset);
};

}  // namespace Sext

FX::zFXInstance* Sext::FXInstance::Create(World::EntityHandleBase* handle,
                                        FXInstance* asset) {
    return new (memset(Memory::AllocGlobalHeap(
                           sizeof(FX::zFXInstance), (Memory::GlobalHeapEnum)0,
                           (eMemMgrTag)16, false),
                       0, sizeof(FX::zFXInstance)))
        FX::zFXInstance(handle, asset);
}

FX::Particles::zFXParticleSystem* Sext::FXParticleSystem::Create(World::EntityHandleBase* handle,
                                                               FXParticleSystem* asset) {
    return new (memset(Memory::AllocGlobalHeap(
                           sizeof(FX::Particles::zFXParticleSystem), (Memory::GlobalHeapEnum)0,
                           (eMemMgrTag)16, false),
                       0, sizeof(FX::Particles::zFXParticleSystem)))
        FX::Particles::zFXParticleSystem(handle, asset);
}

FX::zFXSpawnObject* Sext::FXSpawn::Create(World::EntityHandleBase* handle,
                                        FXSpawn* asset) {
    return new (memset(Memory::AllocGlobalHeap(
                           sizeof(FX::zFXSpawnObject), (Memory::GlobalHeapEnum)0,
                           (eMemMgrTag)16, false),
                       0, sizeof(FX::zFXSpawnObject)))
        FX::zFXSpawnObject(handle, asset);
}

// The manager's slot holds a trivial object at +0 and a spawn at
// +72, destroyed in reverse declaration order as C++ specifies.
class zFXScriptSpawnPtMgr {
public:
    class SpawnSlot {
    public:
        ~SpawnSlot();

        hkBaseObject head;
        unsigned char _pad0[0x48 - 0x4];
        FX::zFXSpawn spawn;
    };
};

FX::zFXSpawn::~zFXSpawn() {}
zFXScriptSpawnPtMgr::SpawnSlot::~SpawnSlot() {}

// The 80-byte base-only destructor, the compiler's own: the
// null-this test, the BASE's destructor on `this` with the flag
// CLEAR -- r4 = 0 is a base subobject where r4 = -1 is a complete
// one -- then operator delete when the CALLER's flag is positive,
// and `return this`. No member is destroyed.
FX::zFXSpawnWithSoundAssetMultiple::~zFXSpawnWithSoundAssetMultiple() {}
