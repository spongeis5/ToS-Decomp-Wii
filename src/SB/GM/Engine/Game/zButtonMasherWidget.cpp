// zButtonMasherMashChecker -- the asset's Create, read from the image with
// tools/disasm.py. It is one shape 33 Sext assets in this tree
// share: take sizeof(zButtonMasherMashChecker) from the global heap (heap 0, tag
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
namespace Sext { class Button_Masher_Widget; }

// The constructor takes this and Create takes the Sext asset: two
// distinct classes, since a typedef would have mangled as the one
// it names. The cast is what the branch target says.
class zButtonMasherWidgetAsset;


class zButtonMasherMashChecker {
public:
    zButtonMasherMashChecker(World::EntityHandleBase* handle,
                             zButtonMasherWidgetAsset* asset);

    unsigned char CycleDone();
    void ResetButton();

    unsigned char _pad0[0x4C];
    unsigned char f4C;
    unsigned char f4D;

    unsigned char _pad1[0x68 - 0x4E];
};


unsigned char zButtonMasherMashChecker::CycleDone() { return f4C; }
void zButtonMasherMashChecker::ResetButton() { f4C = 0; f4D = 0; }

namespace Sext {

class Button_Masher_Widget {
public:
    static zButtonMasherMashChecker* Create(World::EntityHandleBase* handle,
                                              Button_Masher_Widget* asset);
};

}  // namespace Sext

zButtonMasherMashChecker* Sext::Button_Masher_Widget::Create(World::EntityHandleBase* handle,
                                                           Button_Masher_Widget* asset) {
    return new (memset(Memory::AllocGlobalHeap(
                           sizeof(zButtonMasherMashChecker), (Memory::GlobalHeapEnum)0,
                           (eMemMgrTag)16, false),
                       0, sizeof(zButtonMasherMashChecker)))
        zButtonMasherMashChecker(handle,
                                 (zButtonMasherWidgetAsset*)asset);
}
