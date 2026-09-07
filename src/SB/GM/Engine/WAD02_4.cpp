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

namespace FX {

class zFXSpawn {
public:
    zFXSpawn(World::EntityHandleBase* a0);
    virtual void __vtable_anchor();
};

}  // namespace FX

namespace FX {

class zFXSpawnWithSoundAssetMultiple : public FX::zFXSpawn {
public:
    virtual void __vtable_anchor();
    zFXSpawnWithSoundAssetMultiple(World::EntityHandleBase* a0);

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
