#include "SB/GM/Engine/Game/zSpinner.pool.h"
// zSpinner -- the asset's Create, read from the image with
// tools/disasm.py. It is one shape 33 Sext assets in this tree
// share: take sizeof(zSpinner) from the global heap (heap 0, tag
// 16, no clear), memset it, and place the entity on it with the
// handle and the asset. The entity's constructor is a call, so it
// is declared here and not defined, and the class is padded to the
// size the allocation asks for -- which is the only thing about its
// layout this file knows.
//
// tools/twin_census.py is what paired it with the written ones.
//
// The 2 function(s) below came from tools/gen_accessors.py before
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
namespace Sext { class zSpinnerAsset; }

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

extern xAnimTable* animSpinnerTables;


class zSpinner {
public:
    zSpinner(World::EntityHandleBase* handle,
             Sext::zSpinnerAsset* asset);

    static unsigned int anIdleToSpinCheck(xAnimTransition* a0,
                                          xAnimSingle* a1, void* a2);
    bool IdleToSpinCheck(xAnimTransition* a0, xAnimSingle* a1);
    static unsigned int anSpinToSpinCheck(xAnimTransition* a0,
                                          xAnimSingle* a1, void* a2);
    bool SpinToSpinCheck(xAnimTransition* a0, xAnimSingle* a1);

    static unsigned int anIdleToSpinCB(xAnimTransition* a0,
                                       xAnimSingle* a1, void* a2);
    static unsigned int anSpinToSpinCB(xAnimTransition* a0,
                                       xAnimSingle* a1, void* a2);
    static unsigned int anSpinToRecoverCB(xAnimTransition* a0,
                                          xAnimSingle* a1, void* a2);
    static unsigned int anRecoverToIdleCB(xAnimTransition* a0,
                                          xAnimSingle* a1, void* a2);

    static xAnimTable* CreateAnimTable(unsigned long long id);


    unsigned char _pad0[0x98];
};


unsigned int zSpinner::anIdleToSpinCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2) { return ((zSpinner*)a2)->IdleToSpinCheck(a0, a1); }
unsigned int zSpinner::anSpinToSpinCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2) { return ((zSpinner*)a2)->SpinToSpinCheck(a0, a1); }

namespace Sext {

class zSpinnerAsset {
public:
    static zSpinner* Create(World::EntityHandleBase* handle,
                              zSpinnerAsset* asset);
};

}  // namespace Sext

zSpinner* Sext::zSpinnerAsset::Create(World::EntityHandleBase* handle,
                                    zSpinnerAsset* asset) {
    return new (memset(Memory::AllocGlobalHeap(
                           sizeof(zSpinner), (Memory::GlobalHeapEnum)0,
                           (eMemMgrTag)16, false),
                       0, sizeof(zSpinner))) zSpinner(handle, asset);
}

// zSpinner::CreateAnimTable, read from the image. The module keeps a
// list of the tables it has made, one per id; the node IS an
// xAnimTable, which is why the found one is what comes back.
xAnimTable* zSpinner::CreateAnimTable(unsigned long long id) {
    xAnimTable* table = animSpinnerTables;

    while (table) {
        if (table->id && *table->id == id) {
            break;
        }

        table = table->next;
    }

    if (table == 0) {
        unsigned long long* key =
            (unsigned long long*)Memory::AllocGlobalHeap(
                8, (Memory::GlobalHeapEnum)0, (eMemMgrTag)7, false);

        *key = id;

        table = xAnimTableNew("Spinner", 0, key, &animSpinnerTables);
        xAnimTableNewState(table, "Spinner_Idle", 16, 0, 1.0f, 0, 0, 0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
        xAnimTableNewState(table, "Spinner_Spinning", 64, 0, 1.0f, 0, 0, 0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
        xAnimTableNewState(table, "Spinner_Recover", 32, 0, 1.0f, 0, 0, 0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
        xAnimTableNewTransition(table, "Spinner_Idle", "Spinner_Spinning", 0, zSpinner::anIdleToSpinCheck, zSpinner::anIdleToSpinCB, 0, 0, 0.0f, 0.0f, 1100, 0, 0.15f, 0);
        xAnimTableNewTransition(table, "Spinner_Spinning", "Spinner_Spinning", 0, zSpinner::anSpinToSpinCheck, zSpinner::anSpinToSpinCB, 16, 0, 0.0f, 0.0f, 1101, 0, 0.15f, 0);
        xAnimTableNewTransition(table, "Spinner_Spinning", "Spinner_Recover", 0, 0, zSpinner::anSpinToRecoverCB, 16, 0, 0.0f, 0.0f, 1100, 0, 0.15f, 0);
        xAnimTableNewTransition(table, "Spinner_Recover", "Spinner_Idle", 0, 0, zSpinner::anRecoverToIdleCB, 16, 0, 0.0f, 0.0f, 1100, 0, 0.15f, 0);
    }

    return table;
}
