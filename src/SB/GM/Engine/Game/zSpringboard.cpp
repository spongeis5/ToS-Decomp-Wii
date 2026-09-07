// zSpringboard -- the asset's Create, read from the image with
// tools/disasm.py. It is one shape 33 Sext assets in this tree
// share: take sizeof(zSpringboard) from the global heap (heap 0, tag
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
namespace Sext { class Springboard; }

class xAnimSingle;
class xAnimTransition;


class zSpringboard {
public:
    zSpringboard(World::EntityHandleBase* handle,
                 Sext::Springboard* asset);

    static bool anCompressCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2);
    static bool anFailCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2);
    static bool anLaunchCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2);

    unsigned char _pad0[0xC0];
    int fC0;

    unsigned char _pad1[0xD0 - 0xC4];
};


bool zSpringboard::anCompressCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2) { return ((zSpringboard*)a2)->fC0 == 1; }
bool zSpringboard::anLaunchCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2) { return ((zSpringboard*)a2)->fC0 == 3; }
bool zSpringboard::anFailCheck(xAnimTransition* a0, xAnimSingle* a1, void* a2) { return ((zSpringboard*)a2)->fC0 == 4; }

namespace Sext {

class Springboard {
public:
    static zSpringboard* Create(World::EntityHandleBase* handle,
                                  Springboard* asset);
};

}  // namespace Sext

zSpringboard* Sext::Springboard::Create(World::EntityHandleBase* handle,
                                      Springboard* asset) {
    return new (memset(Memory::AllocGlobalHeap(
                           sizeof(zSpringboard), (Memory::GlobalHeapEnum)0,
                           (eMemMgrTag)16, false),
                       0, sizeof(zSpringboard))) zSpringboard(handle, asset);
}
