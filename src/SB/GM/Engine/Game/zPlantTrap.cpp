#include "SB/GM/Engine/Game/zPlantTrap.pool.h"

class xAnimSingle;
class xAnimTransition;
class xAnimPlay;
class xAnimState;
class xQuat;
class xVec3;

// The table's own list: `next` at +0 and the id it was made for at
// +0x1C, which is what the search reads. Nothing else about it is
// known here.
class xAnimTable {
public:
    xAnimTable* next;
    unsigned char _pad0[0x1C - 0x4];
    unsigned long long* id;
};

xAnimTable* xAnimTableNew(const char* name, unsigned int a, void* owner,
                          xAnimTable** list);
unsigned int xAnimTableNewState(xAnimTable* table, const char* name,
                                unsigned int a, unsigned int b, float c,
                                float* d, float* e, float f,
                                unsigned short* g, void* h,
                                void (*i)(xAnimPlay*, xAnimState*, void*),
                                void (*j)(xAnimPlay*, xAnimState*, void*),
                                void (*k)(xAnimState*, xAnimSingle*, void*),
                                void (*l)(xAnimPlay*, xQuat*, xVec3*,
                                          xVec3*, int),
                                unsigned long long m, unsigned int n);
unsigned int xAnimTableNewTransition(
    xAnimTable* table, const char* from, const char* to,
    unsigned int (*a)(xAnimTransition*, xAnimSingle*, void*),
    unsigned int (*b)(xAnimTransition*, xAnimSingle*, void*),
    unsigned int (*c)(xAnimTransition*, xAnimSingle*, void*),
    unsigned int d, unsigned int e, float f, float g, unsigned short h,
    unsigned short i, float j, unsigned short* k);

namespace zPlantTrapNS { extern xAnimTable* animPlantTables; }
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
    static xAnimTable* CreateAnimTable(unsigned long long id);
    static unsigned int anWarnEndCheck(xAnimTransition*, xAnimSingle*, void*);
    static unsigned int anNotProneCheck(xAnimTransition*, xAnimSingle*, void*);
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
    static unsigned int anDoneChewCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2);
    bool DoneChewCheck(xAnimTransition* a0, xAnimSingle* a1);
    static unsigned int anEatCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2);
    bool EatCheck(xAnimTransition* a0, xAnimSingle* a1);
    static unsigned int anGoToSpitCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2);
    bool GoToSpitCheck(xAnimTransition* a0, xAnimSingle* a1);
    static unsigned int anHitCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2);
    static unsigned int anIdleCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2);
    bool IdleCheck(xAnimTransition* a0, xAnimSingle* a1);
    static unsigned int anProneCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2);
    static unsigned int anProneOutCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2);
    bool ProneOutCheck(xAnimTransition* a0, xAnimSingle* a1);
    static unsigned int anReleaseCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2);
    bool ReleaseCheck(xAnimTransition* a0, xAnimSingle* a1);
    static unsigned int anSnapCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2);
    bool SnapCheck(xAnimTransition* a0, xAnimSingle* a1);
    static unsigned int anSnapEntityCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2);
    bool SnapEntityCheck(xAnimTransition* a0, xAnimSingle* a1);
    static unsigned int anSpitCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2);
    bool SpitCheck(xAnimTransition* a0, xAnimSingle* a1);
    static unsigned int anWarnCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2);
    bool WarnCheck(xAnimTransition* a0, xAnimSingle* a1);

    unsigned char _pad0[0x5C];
    int f5C;
    unsigned char _pad1[0x30];
    int f90;
    unsigned char _pad2[0xA8 - 0x94];
};


int* zPlantTrap::GetAttachDPos() { return &f90; }
unsigned int zPlantTrap::anWarnCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2) { return ((zPlantTrap*)a2)->WarnCheck(a0, a1); }
unsigned int zPlantTrap::anProneOutCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2) { return ((zPlantTrap*)a2)->ProneOutCheck(a0, a1); }
unsigned int zPlantTrap::anSnapCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2) { return ((zPlantTrap*)a2)->SnapCheck(a0, a1); }
unsigned int zPlantTrap::anSnapEntityCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2) { return ((zPlantTrap*)a2)->SnapEntityCheck(a0, a1); }
unsigned int zPlantTrap::anReleaseCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2) { return ((zPlantTrap*)a2)->ReleaseCheck(a0, a1); }
unsigned int zPlantTrap::anEatCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2) { return ((zPlantTrap*)a2)->EatCheck(a0, a1); }
unsigned int zPlantTrap::anHitCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2) { return ((zPlantTrap*)a2)->HitCheck(a0, a1); }
unsigned int zPlantTrap::anGoToSpitCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2) { return ((zPlantTrap*)a2)->GoToSpitCheck(a0, a1); }
unsigned int zPlantTrap::anDoneChewCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2) { return ((zPlantTrap*)a2)->DoneChewCheck(a0, a1); }
unsigned int zPlantTrap::anProneCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2) { return ((zPlantTrap*)a2)->f5C == 9; }
unsigned int zPlantTrap::anSpitCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2) { return ((zPlantTrap*)a2)->SpitCheck(a0, a1); }
unsigned int zPlantTrap::anIdleCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2) { return ((zPlantTrap*)a2)->IdleCheck(a0, a1); }
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

// zPlantTrap::CreateAnimTable, read from the image. The module keeps a
// list of the tables it has made, one per id; the node IS an
// xAnimTable, which is why the found one is what comes back.
xAnimTable* zPlantTrap::CreateAnimTable(unsigned long long id) {
    xAnimTable* table = zPlantTrapNS::animPlantTables;

    while (table) {
        if (table->id && *table->id == id) {
            break;
        }

        table = table->next;
    }

    if (table == 0) {
        unsigned long long* key =
            (unsigned long long*)Memory::AllocGlobalHeap(
                8, (Memory::GlobalHeapEnum)0, (eMemMgrTag)7,
                false);

        *key = id;

        table = xAnimTableNew("Plant", 0, key, &zPlantTrapNS::animPlantTables);
        xAnimTableNewState(table, "PlantClosed", 16, 0, 1.0f, 0, 0, 0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
        xAnimTableNewState(table, "PlantReady", 16, 0, 1.0f, 0, 0, 0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
        xAnimTableNewState(table, "PlantWarn", 32, 0, 1.0f, 0, 0, 0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
        xAnimTableNewState(table, "PlantSnap", 32, 0, 1.0f, 0, 0, 0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
        xAnimTableNewState(table, "PlantSnapEntity", 32, 0, 1.0f, 0, 0, 0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
        xAnimTableNewState(table, "PlantChewPlayer", 16, 0, 1.0f, 0, 0, 0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
        xAnimTableNewState(table, "PlantChewEntity", 64, 0, 1.0f, 0, 0, 0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
        xAnimTableNewState(table, "PlantHurt", 32, 0, 1.0f, 0, 0, 0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
        xAnimTableNewState(table, "PlantSpit", 32, 0, 1.0f, 0, 0, 0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
        xAnimTableNewState(table, "PlantRelease", 32, 0, 1.0f, 0, 0, 0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
        xAnimTableNewState(table, "PlantReleaseQuick", 32, 0, 1.0f, 0, 0, 0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
        xAnimTableNewTransition(table, "PlantReady", "PlantWarn", 0, zPlantTrap::anWarnCheck, 0, 0, 0, 0.0f, 0.0f, 1100, 0, 0.15f, 0);
        xAnimTableNewTransition(table, "PlantReady", "PlantReleaseQuick", 0, zPlantTrap::anProneOutCheck, 0, 0, 0, 0.0f, 0.0f, 1100, 0, 0.15f, 0);
        xAnimTableNewTransition(table, "PlantClosed", "PlantReleaseQuick", 0, zPlantTrap::anProneOutCheck, 0, 0, 0, 0.0f, 0.0f, 1100, 0, 0.15f, 0);
        xAnimTableNewTransition(table, "PlantReleaseQuick", "PlantReady", 0, zPlantTrap::anWarnEndCheck, 0, 16, 0, 0.0f, 0.0f, 1100, 0, 0.15f, 0);
        xAnimTableNewTransition(table, "PlantReady", "PlantSnap", 0, zPlantTrap::anSnapCheck, 0, 0, 0, 0.0f, 0.0f, 1100, 0, 0.15f, 0);
        xAnimTableNewTransition(table, "PlantWarn", "PlantSnap", 0, zPlantTrap::anSnapCheck, 0, 0, 0, 0.0f, 0.0f, 1100, 0, 0.15f, 0);
        xAnimTableNewTransition(table, "PlantReady", "PlantSnapEntity", 0, zPlantTrap::anSnapEntityCheck, 0, 0, 0, 0.0f, 0.0f, 1100, 0, 0.15f, 0);
        xAnimTableNewTransition(table, "PlantWarn", "PlantSnapEntity", 0, zPlantTrap::anSnapEntityCheck, 0, 0, 0, 0.0f, 0.0f, 1100, 0, 0.15f, 0);
        xAnimTableNewTransition(table, "PlantWarn", "PlantReady", 0, 0, 0, 16, 0, 0.0f, 0.0f, 1100, 0, 0.15f, 0);
        xAnimTableNewTransition(table, "PlantSnapEntity", "PlantRelease", 0, zPlantTrap::anReleaseCheck, 0, 16, 0, 0.0f, 0.0f, 1100, 0, 0.15f, 0);
        xAnimTableNewTransition(table, "PlantSnapEntity", "PlantChewPlayer", 0, zPlantTrap::anEatCheck, 0, 16, 0, 0.0f, 0.0f, 1100, 0, 0.15f, 0);
        xAnimTableNewTransition(table, "PlantSnapEntity", "PlantClosed", 0, 0, 0, 16, 0, 0.0f, 0.0f, 1100, 0, 0.15f, 0);
        xAnimTableNewTransition(table, "PlantSnap", "PlantRelease", 0, zPlantTrap::anReleaseCheck, 0, 16, 0, 0.0f, 0.0f, 1100, 0, 0.15f, 0);
        xAnimTableNewTransition(table, "PlantSnap", "PlantChewPlayer", 0, zPlantTrap::anEatCheck, 0, 16, 0, 0.0f, 0.0f, 1100, 0, 0.15f, 0);
        xAnimTableNewTransition(table, "PlantSnap", "PlantClosed", 0, 0, 0, 16, 0, 0.0f, 0.0f, 1100, 0, 0.15f, 0);
        xAnimTableNewTransition(table, "PlantChewPlayer", "PlantHurt", 0, zPlantTrap::anHitCheck, 0, 0, 0, 0.0f, 0.0f, 1100, 0, 0.15f, 0);
        xAnimTableNewTransition(table, "PlantChewPlayer", "PlantRelease", 0, zPlantTrap::anReleaseCheck, 0, 0, 0, 0.0f, 0.0f, 1100, 0, 0.15f, 0);
        xAnimTableNewTransition(table, "PlantClosed", "PlantChewEntity", 0, zPlantTrap::anGoToSpitCheck, 0, 0, 0, 0.0f, 0.0f, 1100, 0, 0.15f, 0);
        xAnimTableNewTransition(table, "PlantChewEntity", "PlantSpit", 0, zPlantTrap::anDoneChewCheck, 0, 16, 0, 0.0f, 0.0f, 1100, 0, 0.15f, 0);
        xAnimTableNewTransition(table, "PlantChewEntity", "PlantChewEntity", 0, 0, 0, 16, 0, 0.0f, 0.0f, 1100, 0, 0.15f, 0);
        xAnimTableNewTransition(table, "PlantClosed", "PlantRelease", 0, zPlantTrap::anNotProneCheck, 0, 0, 0, 0.0f, 0.0f, 1100, 0, 0.15f, 0);
        xAnimTableNewTransition(table, "PlantClosed", "PlantClosed", 0, 0, 0, 16, 0, 0.0f, 0.0f, 1100, 0, 0.15f, 0);
        xAnimTableNewTransition(table, "PlantWarn", "PlantClosed", 0, zPlantTrap::anProneCheck, 0, 0, 0, 0.0f, 0.0f, 1100, 0, 0.15f, 0);
        xAnimTableNewTransition(table, "PlantReady", "PlantClosed", 0, zPlantTrap::anProneCheck, 0, 0, 0, 0.0f, 0.0f, 1100, 0, 0.15f, 0);
        xAnimTableNewTransition(table, "PlantSpit", "PlantReady", 0, zPlantTrap::anSpitCheck, 0, 0, 0, 0.0f, 0.0f, 1100, 0, 0.15f, 0);
        xAnimTableNewTransition(table, "PlantSpit", "PlantReady", 0, 0, 0, 16, 0, 0.0f, 0.0f, 1100, 0, 0.15f, 0);
        xAnimTableNewTransition(table, "PlantHurt", "PlantRelease", 0, zPlantTrap::anReleaseCheck, 0, 0, 0, 0.0f, 0.0f, 1100, 0, 0.15f, 0);
        xAnimTableNewTransition(table, "PlantHurt", "PlantChewPlayer", 0, zPlantTrap::anEatCheck, 0, 16, 0, 0.0f, 0.0f, 1100, 0, 0.15f, 0);
        xAnimTableNewTransition(table, "PlantRelease", "PlantReady", 0, zPlantTrap::anIdleCheck, 0, 16, 0, 0.0f, 0.0f, 1100, 0, 0.15f, 0);
    }

    return table;
}
