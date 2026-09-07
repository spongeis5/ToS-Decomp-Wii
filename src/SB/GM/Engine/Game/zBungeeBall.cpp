// zBungeeBall -- the asset's Create, read from the image with
// tools/disasm.py. It is one shape 33 Sext assets in this tree
// share: take sizeof(zBungeeBall) from the global heap (heap 0, tag
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
namespace Sext { class zBungeeBallAsset; }


class zBungeeBall {
public:
    zBungeeBall(World::EntityHandleBase* handle,
                Sext::zBungeeBallAsset* asset);

    int* DriveGetCurMat(int a0);
    bool IsReturning();

    unsigned char _pad0[0x80];
    int f80;
    unsigned char _pad1[0x48];
    int fCC;

    unsigned char _pad2[0x158 - 0xD0];
};


bool zBungeeBall::IsReturning() { return fCC == 2; }
int* zBungeeBall::DriveGetCurMat(int a0) { return &f80; }

namespace Sext {

class zBungeeBallAsset {
public:
    static zBungeeBall* Create(World::EntityHandleBase* handle,
                                 zBungeeBallAsset* asset);
};

}  // namespace Sext

zBungeeBall* Sext::zBungeeBallAsset::Create(World::EntityHandleBase* handle,
                                          zBungeeBallAsset* asset) {
    return new (memset(Memory::AllocGlobalHeap(
                           sizeof(zBungeeBall), (Memory::GlobalHeapEnum)0,
                           (eMemMgrTag)16, false),
                       0, sizeof(zBungeeBall))) zBungeeBall(handle, asset);
}
