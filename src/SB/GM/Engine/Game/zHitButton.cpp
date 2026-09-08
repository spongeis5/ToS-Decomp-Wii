#include "SB/GM/Engine/Game/zHitButton.pool.h"

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

namespace zHitButtonNS { extern xAnimTable* animButtonHammerStrengthTables; }
// zHitButton -- the asset's Create, read from the image with
// tools/disasm.py. It is one shape 33 Sext assets in this tree
// share: take sizeof(zHitButton) from the global heap (heap 0, tag
// 16, no clear), memset it, and place the entity on it with the
// handle and the asset. The entity's constructor is a call, so it
// is declared here and not defined, and the class is padded to the
// size the allocation asks for -- which is the only thing about its
// layout this file knows.
//
// tools/twin_census.py is what paired it with the written ones.
//
// The 3 function(s) below came from tools/gen_accessors.py before
// this file was written by hand, and are unchanged. The generator's
// banner is gone because gen_units.py overwrites any file that
// still carries it.

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
namespace Sext { class zHitButtonAsset; }

class xAnimSingle;
class xAnimTransition;


class zHitButton {
public:
    static xAnimTable* CreateAnimTable(unsigned long long id);
    static unsigned long long animSetHitButtonID;
    static unsigned int anResetCheck(xAnimTransition*, xAnimSingle*, void*);
    zHitButton(World::EntityHandleBase* handle,
               Sext::zHitButtonAsset* asset);

    static unsigned int anStrengthOneCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2);
    bool StrengthOneCheck(xAnimTransition* a0, xAnimSingle* a1);
    static unsigned int anStrengthThreeCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2);
    bool StrengthThreeCheck(xAnimTransition* a0, xAnimSingle* a1);
    static unsigned int anStrengthTwoCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2);
    bool StrengthTwoCheck(xAnimTransition* a0, xAnimSingle* a1);


    unsigned char _pad0[0xB0];
};


unsigned int zHitButton::anStrengthOneCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2) { return ((zHitButton*)a2)->StrengthOneCheck(a0, a1); }
unsigned int zHitButton::anStrengthTwoCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2) { return ((zHitButton*)a2)->StrengthTwoCheck(a0, a1); }
unsigned int zHitButton::anStrengthThreeCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2) { return ((zHitButton*)a2)->StrengthThreeCheck(a0, a1); }

namespace Sext {

class zHitButtonAsset {
public:
    static zHitButton* Create(World::EntityHandleBase* handle,
                                zHitButtonAsset* asset);
};

}  // namespace Sext

zHitButton* Sext::zHitButtonAsset::Create(World::EntityHandleBase* handle,
                                        zHitButtonAsset* asset) {
    return new (memset(Memory::AllocGlobalHeap(
                           sizeof(zHitButton), (Memory::GlobalHeapEnum)0,
                           (eMemMgrTag)16, false),
                       0, sizeof(zHitButton))) zHitButton(handle, asset);
}

// zHitButton::CreateAnimTable, read from the image. This one keeps the
// id in a static of its own rather than taking eight bytes for it,
// and hands the table that address.
xAnimTable* zHitButton::CreateAnimTable(unsigned long long id) {
    xAnimTable* table = zHitButtonNS::animButtonHammerStrengthTables;

    while (table) {
        if (table->id && *table->id == id) {
            break;
        }

        table = table->next;
    }

    if (table == 0) {
        animSetHitButtonID = id;

        table = xAnimTableNew("ButtonHammerStrength", 0, &animSetHitButtonID,
                              &zHitButtonNS::animButtonHammerStrengthTables);
        xAnimTableNewState(table, "ButtonIdle", 16, 0, 1.0f, 0, 0, 0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
        xAnimTableNewState(table, "ButtonStrengthOne", 32, 0, 1.0f, 0, 0, 0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
        xAnimTableNewState(table, "ButtonStrengthTwo", 32, 0, 1.0f, 0, 0, 0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
        xAnimTableNewState(table, "ButtonStrengthThree", 32, 0, 1.0f, 0, 0, 0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
        xAnimTableNewState(table, "ButtonActivated", 16, 0, 1.0f, 0, 0, 0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
        xAnimTableNewTransition(table, "ButtonIdle", "ButtonStrengthOne", 0, zHitButton::anStrengthOneCheck, 0, 0, 0, 0.0f, 0.0f, 1100, 0, 0.15f, 0);
        xAnimTableNewTransition(table, "ButtonIdle", "ButtonStrengthTwo", 0, zHitButton::anStrengthTwoCheck, 0, 0, 0, 0.0f, 0.0f, 1100, 0, 0.15f, 0);
        xAnimTableNewTransition(table, "ButtonIdle", "ButtonStrengthThree", 0, zHitButton::anStrengthThreeCheck, 0, 0, 0, 0.0f, 0.0f, 1100, 0, 0.15f, 0);
        xAnimTableNewTransition(table, "ButtonStrengthOne", "ButtonIdle", 0, 0, 0, 16, 0, 0.0f, 0.0f, 1100, 0, 0.15f, 0);
        xAnimTableNewTransition(table, "ButtonStrengthTwo", "ButtonIdle", 0, 0, 0, 16, 0, 0.0f, 0.0f, 1100, 0, 0.15f, 0);
        xAnimTableNewTransition(table, "ButtonStrengthThree", "ButtonActivated", 0, 0, 0, 16, 0, 0.0f, 0.0f, 1100, 0, 0.15f, 0);
        xAnimTableNewTransition(table, "ButtonActivated", "ButtonIdle", 0, zHitButton::anResetCheck, 0, 0, 0, 0.0f, 0.0f, 1100, 0, 0.15f, 0);
    }

    return table;
}
