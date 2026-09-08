// zPlantTrap -- the asset Create(s) below are one shape 33 Sext assets in
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
namespace Sext { class zPlantTrapAsset; }

class xAnimSingle;

class zPlantTrap;

// The behaviour's base is nested in zPlantTrap, is 0x14 bytes and is
// NOT polymorphic: the vptr the constructor stores therefore lands
// after it at +0x14, and not at +0 the way an entity's does. That
// offset is the whole of what says which.
class zSBKelpTrapBehavior;
class xAnimTransition;


class zPlantTrap {
public:
    zPlantTrap(World::EntityHandleBase* handle,
               Sext::zPlantTrapAsset* asset);

    // Nested, 0x14 bytes and NOT polymorphic, which is why the vptr
    // the derived constructor stores lands after it at +0x14 rather
    // than at +0 the way an entity's does.
    class CustomBehavior {
    public:
        CustomBehavior(zPlantTrap* trap);

        unsigned char _pad0[0x14];
    };

    int* GetAttachDPos();
    bool HitCheck(xAnimTransition* a0, xAnimSingle* a1);
    static bool anDoneChewCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2);
    bool DoneChewCheck(xAnimTransition* a0, xAnimSingle* a1);
    static bool anEatCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2);
    bool EatCheck(xAnimTransition* a0, xAnimSingle* a1);
    static bool anGoToSpitCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2);
    bool GoToSpitCheck(xAnimTransition* a0, xAnimSingle* a1);
    static bool anHitCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2);
    static bool anIdleCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2);
    bool IdleCheck(xAnimTransition* a0, xAnimSingle* a1);
    static bool anProneCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2);
    static bool anProneOutCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2);
    bool ProneOutCheck(xAnimTransition* a0, xAnimSingle* a1);
    static bool anReleaseCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2);
    bool ReleaseCheck(xAnimTransition* a0, xAnimSingle* a1);
    static bool anSnapCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2);
    bool SnapCheck(xAnimTransition* a0, xAnimSingle* a1);
    static bool anSnapEntityCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2);
    bool SnapEntityCheck(xAnimTransition* a0, xAnimSingle* a1);
    static bool anSpitCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2);
    bool SpitCheck(xAnimTransition* a0, xAnimSingle* a1);
    static bool anWarnCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2);
    bool WarnCheck(xAnimTransition* a0, xAnimSingle* a1);

    unsigned char _pad0[0x5C];
    int f5C;
    unsigned char _pad1[0x30];
    int f90;
    unsigned char _pad2[0xA8 - 0x94];
};


int* zPlantTrap::GetAttachDPos() { return &f90; }
bool zPlantTrap::anWarnCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2) { return ((zPlantTrap*)a2)->WarnCheck(a0, a1); }
bool zPlantTrap::anProneOutCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2) { return ((zPlantTrap*)a2)->ProneOutCheck(a0, a1); }
bool zPlantTrap::anSnapCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2) { return ((zPlantTrap*)a2)->SnapCheck(a0, a1); }
bool zPlantTrap::anSnapEntityCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2) { return ((zPlantTrap*)a2)->SnapEntityCheck(a0, a1); }
bool zPlantTrap::anReleaseCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2) { return ((zPlantTrap*)a2)->ReleaseCheck(a0, a1); }
bool zPlantTrap::anEatCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2) { return ((zPlantTrap*)a2)->EatCheck(a0, a1); }
bool zPlantTrap::anHitCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2) { return ((zPlantTrap*)a2)->HitCheck(a0, a1); }
bool zPlantTrap::anGoToSpitCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2) { return ((zPlantTrap*)a2)->GoToSpitCheck(a0, a1); }
bool zPlantTrap::anDoneChewCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2) { return ((zPlantTrap*)a2)->DoneChewCheck(a0, a1); }
bool zPlantTrap::anProneCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2) { return ((zPlantTrap*)a2)->f5C == 9; }
bool zPlantTrap::anSpitCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2) { return ((zPlantTrap*)a2)->SpitCheck(a0, a1); }
bool zPlantTrap::anIdleCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2) { return ((zPlantTrap*)a2)->IdleCheck(a0, a1); }
bool zPlantTrap::HitCheck(xAnimTransition* a0, xAnimSingle* a1) { return f5C == 5; }

namespace Sext {

class zPlantTrapAsset {
public:
    static zPlantTrap* Create(World::EntityHandleBase* handle,
                                zPlantTrapAsset* asset);
};

}  // namespace Sext

zPlantTrap* Sext::zPlantTrapAsset::Create(World::EntityHandleBase* handle,
                                        zPlantTrapAsset* asset) {
    return new (memset(Memory::AllocGlobalHeap(
                           sizeof(zPlantTrap), (Memory::GlobalHeapEnum)0,
                           (eMemMgrTag)16, false),
                       0, sizeof(zPlantTrap)))
        zPlantTrap(handle, asset);
}

class zSBKelpTrapBehavior : public zPlantTrap::CustomBehavior {
public:
    zSBKelpTrapBehavior(zPlantTrap* trap);

    virtual void _v0();

    unsigned char _pad0[0x1C - 0x18];
    int f1C;
};

zSBKelpTrapBehavior::zSBKelpTrapBehavior(zPlantTrap* trap)
    : zPlantTrap::CustomBehavior(trap) {
    f1C = 0;
}
