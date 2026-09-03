// MemoryUtil.cpp -- eight functions, read from the image with
// tools/disasm.py. The physical memory pools are the two Wii arenas:
// InitializePhysicalMemory builds an expanding heap over each of MEM1
// and MEM2, sets both to their first allocation mode and marks the pair
// initialized. AllocPhysicalMemory initializes them if that flag is
// clear and then hands the request to the pool's own allocator; Free
// picks the pool the same way and returns without doing anything for a
// pool it does not know. The two free-size queries ask the heap for its
// total free size and for its largest allocatable block, and both take
// 64 MB off the MEM2 answer when the console has the extended memory a
// development kit has. IsUsingExtendedMemory is that test: the MEM2
// heap's own span against 64 MB, which the compiler computes without a
// branch. __sys_alloc and __sys_free are the runtime's hooks, each a
// tail call into the global heap with the arguments filled in.
//
// The three globals are Memory's own (config/R8IE78/symbols.txt names
// them at 8077B8EC, 8077B8F0 and 8077B8F4), and the heap handle's span
// is read at +0x1C of MEMiHeapHead, which is where its end pointer sits.
//
// Four shapes the bytes fixed.
//
// The pool dispatch is an if/else if with no final else, so an unknown
// pool falls out of the bottom: Free's is a conditional return (bnelr)
// and Alloc's leaves the result register untouched.
//
// The extended-memory test is a SIGNED greater-than against 64 MB. The
// unsigned form is a different sequence entirely -- subfc, subfe, neg,
// nine words -- while the signed one is the xoris, srawi, and, subf and
// srwi the bytes have, ten. Neither has a branch.
//
// Each arena's high address is read into a NAMED local before the
// subtraction: retail moves it to r0 and then subtracts, where the call
// used directly in the expression subtracts from r3 and loses two
// words.
//
// And the three globals are addressed from ONE base with displacements,
// so the base is the unity unit's .bss anchor and not our first
// variable: the flag's store is at 6524 off it in retail and at 0
// without the padding array below, which is one word and the only one
// this unit ever missed.

namespace Memory {

enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };
enum PhysicalMemoryPool { PhysicalMemoryPool_ = 0x7FFFFFFF };

}  // namespace Memory

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {

void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap,
                      unsigned long align, eMemMgrTag tag, bool clear);
void FreeGlobalHeap(void* block, GlobalHeapEnum heap);

}  // namespace Memory

// The expanding-heap handle, of which only the end pointer is read.
class MEMiHeapHead {
public:
    unsigned char _pad0[0x1C];
    void* end;
};

extern "C" {

void* MEMCreateExpHeapEx(void* start, unsigned long size, unsigned short opt);
void MEMSetAllocModeForExpHeap(void* heap, int mode);
unsigned long MEMGetTotalFreeSizeForExpHeap(void* heap);
unsigned long MEMGetAllocatableSizeForExpHeapEx(void* heap, int align);

void* OSGetMEM1ArenaLo();
void* OSGetMEM1ArenaHi();
void* OSGetMEM2ArenaLo();
void* OSGetMEM2ArenaHi();

void* __sys_alloc(unsigned long size);
void __sys_free(void* block);

}

// The unity unit's .bss ahead of these three, measured from the image
// (memCB_domDir at 80779F70 to IsHeapInitialized at 8077B8EC). The
// compiler addresses the group from one base, so without it the flag
// sits at displacement 0 where retail has 6524. Referenced by nothing
// and holds nothing.
static unsigned char kUnityBssAhead[6524];

namespace Memory {

bool IsHeapInitialized;
void* physicalMEM1Handle;
void* physicalMEM2Handle;

int AllocPhysicalMemory(int size, int align, PhysicalMemoryPool pool);
void FreePhysicalMemory(void* block, int size, PhysicalMemoryPool pool);
int GetPhysicalMemoryFreeMax(PhysicalMemoryPool pool);
int GetPhysicalMemoryFreeMin(PhysicalMemoryPool pool);
void InitializePhysicalMemory();
bool IsUsingExtendedMemory();
int AllocPhysicalMemory1(int size, int align);
int AllocPhysicalMemory2(int size, int align);
void FreePhysicalMemory1(void* block);
void FreePhysicalMemory2(void* block);

}  // namespace Memory

void* __sys_alloc(unsigned long size) {
    return Memory::AllocGlobalHeap(size, (Memory::GlobalHeapEnum)0, 16,
                                   (eMemMgrTag)74, false);
}

void __sys_free(void* block) {
    Memory::FreeGlobalHeap(block, (Memory::GlobalHeapEnum)0);
}

int Memory::AllocPhysicalMemory(int size, int align, PhysicalMemoryPool pool) {
    if (!IsHeapInitialized) {
        InitializePhysicalMemory();
    }

    if (pool == 0) {
        return AllocPhysicalMemory1(size, align);
    } else if (pool == 1) {
        return AllocPhysicalMemory2(size, align);
    }
}

void Memory::FreePhysicalMemory(void* block, int size,
                                PhysicalMemoryPool pool) {
    if (pool == 0) {
        FreePhysicalMemory1(block);
    } else if (pool == 1) {
        FreePhysicalMemory2(block);
    }
}

void Memory::InitializePhysicalMemory() {
    if (!IsHeapInitialized) {
        void* lo = OSGetMEM1ArenaLo();
        void* hi = OSGetMEM1ArenaHi();

        physicalMEM1Handle = MEMCreateExpHeapEx(lo, (char*)hi - (char*)lo, 0);

        MEMSetAllocModeForExpHeap(physicalMEM1Handle, 0);

        lo = OSGetMEM2ArenaLo();
        hi = OSGetMEM2ArenaHi();

        physicalMEM2Handle = MEMCreateExpHeapEx(lo, (char*)hi - (char*)lo, 0);

        MEMSetAllocModeForExpHeap(physicalMEM2Handle, 0);

        IsHeapInitialized = true;
    }
}

int Memory::GetPhysicalMemoryFreeMax(PhysicalMemoryPool pool) {
    if (pool == 0) {
        return MEMGetTotalFreeSizeForExpHeap(physicalMEM1Handle);
    } else if (pool == 1) {
        int free = MEMGetTotalFreeSizeForExpHeap(physicalMEM2Handle);

        if (IsUsingExtendedMemory()) {
            free -= 0x04000000;
        }

        return free;
    }

    return -1;
}

int Memory::GetPhysicalMemoryFreeMin(PhysicalMemoryPool pool) {
    if (pool == 0) {
        return MEMGetAllocatableSizeForExpHeapEx(physicalMEM1Handle, 4);
    } else if (pool == 1) {
        int free = MEMGetAllocatableSizeForExpHeapEx(physicalMEM2Handle, 4);

        if (IsUsingExtendedMemory()) {
            free -= 0x04000000;
        }

        return free;
    }

    return -1;
}

bool Memory::IsUsingExtendedMemory() {
    MEMiHeapHead* heap = (MEMiHeapHead*)physicalMEM2Handle;

    return (int)((char*)heap->end - (char*)heap) > 0x04000000;
}
