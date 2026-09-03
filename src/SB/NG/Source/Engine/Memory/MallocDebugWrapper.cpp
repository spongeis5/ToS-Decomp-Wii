// MallocDebugWrapper.cpp -- six functions, read from the image with
// tools/disasm.py. The wrapper is a critical section and the number of
// the physical pool it serves. Its constructor initialises the mutex and
// records the pool, ignoring the heap it is handed. Alloc refuses a
// zero size, takes the lock, negates the alignment when the caller asks
// for the top of the heap (which is how MEMAllocFromExpHeapEx is told to
// allocate downwards), calls whichever of the two pool allocators the
// pool number names, and releases the lock. Resize takes the lock and
// resizes the block in whatever heap contains it -- both arms of its
// pool test are the same three calls, which is what retail emits and is
// not folded. Free ignores a null block and otherwise frees through the
// pool's own routine under the lock.
//
// The two pool allocators live here rather than in MemoryUtil.cpp:
// AllocPhysicalMemory1 tries MEM1 and falls back to MEM2, and
// AllocPhysicalMemory2 tries MEM2 and falls back to MEM1. Either way a
// failure prints the request and then the two pools' free totals and
// largest free blocks. A zero alignment becomes four.
//
// Layouts from the DWARF (tools/dwarf_types.py): MallocDebugWrapper is
// 0x24 -- a System::CriticalSection at 0 and the pool enum at 0x20 --
// and CriticalSection is 0x20 with its OSMutex four bytes in, which is
// why the constructor hands OSInitMutex `this + 4` and why Enter and
// Exit are called with `this` unchanged. PhysicalMemoryPool's own
// enumerators are PHYS_MEM_MAIN 0 and PHYS_MEM_SECONDARY 1.
//
// Five shapes the bytes fixed.
//
// The first is the string pooling, and it is written out at the pragma
// and the padding array below rather than here, because the two lines
// it takes are the shape.
//
// The mutex is initialised by two body statements, not by a member's
// constructor: MemTracker.cpp beside this one holds the same
// CriticalSection and initialises it at the END of its constructor,
// after two calls, which a member's constructor could not do.
//
// The failure reports read their two pools in argument order but the
// calls come out right to left -- pool 1's free size is computed into a
// callee-saved register before pool 0's -- because mwcc evaluates a
// call's arguments from the right. Each of the three format strings
// forms its own base with a `lis`, since OSReport clobbers the volatile
// register the previous one used.
//
// The zero-size refusal is an ordinary `if (size == 0) return 0;`: the
// branch that skips it is the one taken, and the returned zero is the
// fall-through.
//
// Resize's two arms are IDENTICAL instruction for instruction and
// retail keeps both, with the pool test and the branch between them.
// Written with the two arms spelled the SAME WAY mwcc deletes the test
// outright -- not a tail merge, the member read and the compare go too
// -- and the function comes out 23 words against retail's 34. The fold
// is at the statement-tree level, so any textual difference between the
// arms that emits the same eight instructions defeats it: a named local
// for the heap in one arm does, and so does a named local for the
// resize's result. Eight spellings that do NOT, seven of them 23 words:
// the compare written the other way round, a switch, a conditional
// expression, `> 0` against `!= 0` in one arm, a no-op `(unsigned long)`
// cast on the size in one arm, the test on the secondary pool instead,
// an early return out of the first arm, and a volatile read of the pool
// -- which keeps the load, drops the branch, and is 24 words.
//
// NEAR MISS -- none. All six functions are byte-identical.

// Retail's six format strings are SEPARATE objects, four-aligned, and
// the two identical "Largest Contiguous Block Free" texts are not
// folded together -- so this translation unit's strings were not pooled
// and each reference builds its own address. Pooled, mwcc has one
// @stringBase0 to share and keeps it in a callee-saved register across
// the OSReport calls, which costs a register (r28-r31 against retail's
// r29-r31) and 25 of 58 words in each allocator.
#pragma pool_strings off

// And unpooled is not enough on its own: under 32 KB into .rodata mwcc
// still addresses the separate objects SECTION-RELATIVE off one shared
// base and the two allocators come out 46 of 58. This is the same
// distance rule tools/gen_poolprefix.py measures for float literals --
// it declines to write a header here because the unit loads none -- and
// the array is its 32 KB floor. Referenced by nothing and holds nothing;
// 32,768 and 65,536 give the same bytes.
static const unsigned char kUnityRodataAhead[32768] = {1};

class OSThread;
class OSMutex;

class OSThreadQueue {
public:
    OSThread* head;
    OSThread* tail;
};

class OSMutexLink {
public:
    OSMutex* next;
    OSMutex* prev;
};

class OSMutex {
public:
    OSThreadQueue queue;
    OSThread* thread;
    long count;
    OSMutexLink link;
};

extern "C" {

void OSInitMutex(OSMutex* mutex);
void OSReport(const char* format, ...);

void* MEMAllocFromExpHeapEx(void* heap, unsigned long size, int align);
void* MEMFindContainHeap(const void* block);
unsigned long MEMResizeForMBlockExpHeap(void* heap, void* block,
                                        unsigned long size);
}

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace System {

class CriticalSection {
public:
    void Enter();
    void Exit();

    int refcount;
    OSMutex mutex;
    OSThread* owner;
};

}  // namespace System

namespace Memory {

enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };

enum PhysicalMemoryPool { PHYS_MEM_MAIN = 0, PHYS_MEM_SECONDARY = 1 };

extern void* physicalMEM1Handle;
extern void* physicalMEM2Handle;

int GetPhysicalMemoryFreeMax(PhysicalMemoryPool pool);
int GetPhysicalMemoryFreeMin(PhysicalMemoryPool pool);

void* AllocPhysicalMemory1(int size, int align);
void* AllocPhysicalMemory2(int size, int align);
void FreePhysicalMemory1(void* block);
void FreePhysicalMemory2(void* block);

class MallocDebugWrapper {
public:
    MallocDebugWrapper(GlobalHeapEnum heap, PhysicalMemoryPool pool);

    void* Alloc(int size, eMemMgrTag tag, int align, bool fromTop);
    bool Resize(void* block, int size);
    void Free(void* block);

    System::CriticalSection memMutex;
    PhysicalMemoryPool pool;
};

}  // namespace Memory

Memory::MallocDebugWrapper::MallocDebugWrapper(GlobalHeapEnum heap,
                                               PhysicalMemoryPool poolID) {
    memMutex.refcount = 0;
    OSInitMutex(&memMutex.mutex);

    pool = poolID;
}

void* Memory::MallocDebugWrapper::Alloc(int size, eMemMgrTag tag, int align,
                                        bool fromTop) {
    void* block;

    if (size == 0) {
        return 0;
    }

    memMutex.Enter();

    if (fromTop) {
        align = -align;
    }

    if (pool == PHYS_MEM_MAIN) {
        block = AllocPhysicalMemory1(size, align);
    } else {
        block = AllocPhysicalMemory2(size, align);
    }

    memMutex.Exit();

    return block;
}

void* Memory::AllocPhysicalMemory1(int size, int align) {
    if (align == 0) {
        align = 4;
    }

    void* block = MEMAllocFromExpHeapEx(physicalMEM1Handle, size, align);

    if (block == 0) {
        block = MEMAllocFromExpHeapEx(physicalMEM2Handle, size, align);
    }

    if (block == 0) {
        OSReport("Out of MEM1 memory - Size requested: %d   Align: %d\n", size,
                 align);
        OSReport("Physical Memory Free:\n\tMEM1: %d bytes\n\tMEM2: %d bytes\n",
                 GetPhysicalMemoryFreeMax(PHYS_MEM_MAIN),
                 GetPhysicalMemoryFreeMax(PHYS_MEM_SECONDARY));
        OSReport("Largest Contiguous Block Free:\n\tMEM1: %d bytes\n\tMEM2: %d "
                 "bytes\n",
                 GetPhysicalMemoryFreeMin(PHYS_MEM_MAIN),
                 GetPhysicalMemoryFreeMin(PHYS_MEM_SECONDARY));
    }

    return block;
}

void* Memory::AllocPhysicalMemory2(int size, int align) {
    if (align == 0) {
        align = 4;
    }

    void* block = MEMAllocFromExpHeapEx(physicalMEM2Handle, size, align);

    if (block == 0) {
        block = MEMAllocFromExpHeapEx(physicalMEM1Handle, size, align);
    }

    if (block == 0) {
        OSReport("Out of MEM2 memory - Size requested: %d   Align: %d\n", size,
                 align);
        OSReport("Physical Memory Free:\n\tMEM1: %d bytes\n\tMEM2: %d bytes\n",
                 GetPhysicalMemoryFreeMax(PHYS_MEM_MAIN),
                 GetPhysicalMemoryFreeMax(PHYS_MEM_SECONDARY));
        OSReport("Largest Contiguous Block Free:\n\tMEM1: %d bytes\n\tMEM2: %d "
                 "bytes\n",
                 GetPhysicalMemoryFreeMin(PHYS_MEM_MAIN),
                 GetPhysicalMemoryFreeMin(PHYS_MEM_SECONDARY));
    }

    return block;
}

bool Memory::MallocDebugWrapper::Resize(void* block, int size) {
    bool resized;

    memMutex.Enter();

    if (pool == PHYS_MEM_MAIN) {
        void* heap = MEMFindContainHeap(block);

        resized = MEMResizeForMBlockExpHeap(heap, block, size) != 0;
    } else {
        resized =
            MEMResizeForMBlockExpHeap(MEMFindContainHeap(block), block, size) !=
            0;
    }

    memMutex.Exit();

    return resized;
}

void Memory::MallocDebugWrapper::Free(void* block) {
    if (block) {
        memMutex.Enter();

        if (pool == PHYS_MEM_MAIN) {
            FreePhysicalMemory1(block);
        } else {
            FreePhysicalMemory2(block);
        }

        memMutex.Exit();
    }
}
