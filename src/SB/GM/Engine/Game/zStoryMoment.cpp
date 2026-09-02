// zStoryMoment.cpp -- one function, read from the image with
// tools/disasm.py: GetInstance makes the singleton on first use. The
// allocation is `new` through a class operator new on the global heap
// (heap 0, tag 67, one byte -- the DWARF's zStoryMoment is a single
// bool), and the compare after the call with nothing to construct is
// the null check `new` leaves behind. The instance is a static member.

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {
enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };

void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap, eMemMgrTag tag,
                      bool clear);
}  // namespace Memory

class zStoryMoment {
public:
    static zStoryMoment* instance;

    void* operator new(unsigned long size) {
        return Memory::AllocGlobalHeap(size, (Memory::GlobalHeapEnum)0,
                                       (eMemMgrTag)67, false);
    }

    zStoryMoment() {}

    static zStoryMoment* GetInstance();

    bool active;
};

zStoryMoment* zStoryMoment::GetInstance() {
    if (instance == 0) {
        instance = new zStoryMoment;
    }

    return instance;
}
