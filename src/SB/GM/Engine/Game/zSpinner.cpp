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


class zSpinner {
public:
    zSpinner(World::EntityHandleBase* handle,
             Sext::zSpinnerAsset* asset);

    static bool anIdleToSpinCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2);
    bool IdleToSpinCheck(xAnimTransition* a0, xAnimSingle* a1);
    static bool anSpinToSpinCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2);
    bool SpinToSpinCheck(xAnimTransition* a0, xAnimSingle* a1);


    unsigned char _pad0[0x98];
};


bool zSpinner::anIdleToSpinCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2) { return ((zSpinner*)a2)->IdleToSpinCheck(a0, a1); }
bool zSpinner::anSpinToSpinCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2) { return ((zSpinner*)a2)->SpinToSpinCheck(a0, a1); }

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
