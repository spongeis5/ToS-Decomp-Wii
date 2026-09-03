// PoolManager.cpp -- seven functions, read from the image with
// tools/disasm.py. A pool manager owns four fixed-block allocators, for
// blocks of 4, 16, 32 and 48 bytes, and hands a small allocation to the
// first one whose block is big enough. Create takes the four buffers
// from physical memory, clears the four debug buffers, takes a
// FixedAllocator for each and gives it its buffer, its block size, the
// tracker and thread safety; each allocator is then marked
// self-destructing. Destroy runs the four allocators' destructors in
// reverse and gives the eight blocks back. The constructor is Create and
// the destructor is Destroy. GetMemorySize and Free ask each allocator
// in turn whether the block lies inside its buffer; Alloc walks the four
// size classes and stops at the first that answers.
//
// Layouts from the DWARF (tools/dwarf_types.py): PoolManager is 0x34 --
// unsigned char* fixedBuffer[4] at +0, unsigned char* debugBuffer[4] at
// +0x10, FixedAllocator* falloc[4] at +0x20 and MemTracker* memTracker
// at +0x30, which nothing in this unit writes. FixedAllocator is 0x38
// with a 0x20-byte CriticalSection first, selfDestruct at +0x24,
// bufferStart at +0x30 and bufferEnd at +0x34 -- so `sizeof` is the 56
// the four allocator allocations and the four frees pass. The pool enum
// is Memory::PhysicalMemoryPool, PHYS_MEM_MAIN 0.
//
// Four shapes the bytes fixed.
//
// The range test is an inline one-expression predicate, not a bool
// local: its value is materialised in r5 (li 0, li 1) and then tested
// with `cmpwi r5,0`, which is what an inlined `return a && b;` gives.
// Both compares are UNSIGNED (`cmplw`), because both sides are
// pointers, and the block is the LEFT operand of each -- `mem >=
// bufferStart` and `mem < bufferEnd`, in that order.
//
// GetMemorySize and Free read `falloc[i]` ONCE for the test and again
// for the call that follows, and the compiler folds the two into one
// load (r6), which is the member-read lever; spelling a local instead
// changes nothing but is not what the source can be shown to have had.
// Create is the opposite case and it is a real lever: there a `bl` sits
// between the test and the use, so the member spelling RELOADS
// `falloc[i]` after the allocator's Create and the store lands through
// r3, where retail keeps the pointer in r31 across the call. Written as
// a local assigned from the member, all four blocks fall in and the
// function goes from 39 of 113 words differing to 0 -- that one line was
// the whole of this unit's miss.
// GetMemorySize's four calls are tail calls (`b`), so it returns the
// allocator's answer directly; Free's are ordinary calls, because Free
// returns a bool of its own -- and the `return false` is written after
// the last test, which is why `li r3,0 ; b` sits ahead of the `li r3,1`
// that falls into the epilogue.
//
// Alloc keeps its running result in r0 across the whole body, never
// across a call, which is a plain local initialised to zero and
// re-assigned in each arm. The first arm has no `!= 0` test, only the
// size test; the other three test the result first, so the source is
// `if (mem == 0 && size <= K)` three times over.
//
// Destroy's four destructor calls are `bl __dt__12hkBaseObjectFv` with
// the don't-delete flag -1. That symbol at 0x80007520 is the folded
// empty destructor -- a null-this test and delete-if-asked and nothing
// else -- that every empty destructor in the image collapsed onto, so
// the name FixedAllocator's own destructor had is gone from the image
// the way zCamPool's +28 camera slot is. The call is spelled here as an
// explicit destructor call through that class, which is the symbol the
// relocation has to name; retail's source will have written
// `falloc[i]->~FixedAllocator()`.
//
// NO NEAR MISS. `python tools/unitcmp.py
// SB/NG/Source/Engine/Memory/PoolManager.cpp` reports 7 of 7
// byte-identical, 1,456 bytes, which is every function the split holds.
// That is unitcmp's answer and not the oracle's: this unit has not been
// through ninja, is not marked Matching in configure.py, and nothing
// here has been linked or placed.

class hkBaseObject;

// The empty destructor at 0x80007520, under the name the image gives
// it. Everything about it that these bytes depend on is the symbol.
class hkBaseObject {
public:
    ~hkBaseObject();
};

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

class OSThread;

struct OSMutex {
    unsigned char _bytes[0x18];
};

namespace System {

class CriticalSection {
public:
    int refcount;
    OSMutex mutex;
    OSThread* owner;
};

}  // namespace System

namespace Memory {

class MemTracker;

enum PhysicalMemoryPool { PHYS_MEM_MAIN = 0, PHYS_MEM_SECONDARY = 1 };

void* AllocPhysicalMemory(int size, int align, PhysicalMemoryPool pool);
void FreePhysicalMemory(void* block, int size, PhysicalMemoryPool pool);

class FixedHeader;

class FixedAllocator {
public:
    void Create(int poolSize, int size, void* buffer, void* bufferLimit,
                MemTracker* tracker, bool threadSafe);
    void* Alloc(int size, eMemMgrTag tag);
    int GetMemorySize(void* mem);
    void Free(void* mem);

    bool IsInRange(void* mem) {
        return (unsigned char*)mem >= bufferStart &&
               (unsigned char*)mem < bufferEnd;
    }

    System::CriticalSection memMutex;
    FixedHeader* freeList;
    bool selfDestruct;
    bool beThreadSafe;
    int numBlocks;
    int blockSize;
    unsigned char* bufferStart;
    unsigned char* bufferEnd;
};

class PoolManager {
public:
    PoolManager(MemTracker* tracker, PhysicalMemoryPool pool);
    ~PoolManager();

    void Create(MemTracker* tracker, PhysicalMemoryPool pool);
    void Destroy();

    int GetMemorySize(void* mem);
    void* Alloc(int size, eMemMgrTag tag);
    bool Free(void* mem);

    unsigned char* fixedBuffer[4];
    unsigned char* debugBuffer[4];
    FixedAllocator* falloc[4];
    MemTracker* memTracker;
};

PoolManager::PoolManager(MemTracker* tracker, PhysicalMemoryPool pool) {
    Create(tracker, pool);
}

PoolManager::~PoolManager() {
    Destroy();
}

void PoolManager::Create(MemTracker* tracker, PhysicalMemoryPool pool) {
    FixedAllocator* alloc;

    fixedBuffer[0] = (unsigned char*)AllocPhysicalMemory(0x2000, 16, pool);
    fixedBuffer[1] = (unsigned char*)AllocPhysicalMemory(0x8000, 16, pool);
    fixedBuffer[2] = (unsigned char*)AllocPhysicalMemory(0x10200, 16, pool);
    fixedBuffer[3] = (unsigned char*)AllocPhysicalMemory(0x24300, 16, pool);

    debugBuffer[0] = 0;
    debugBuffer[1] = 0;
    debugBuffer[2] = 0;
    debugBuffer[3] = 0;

    falloc[0] = (FixedAllocator*)AllocPhysicalMemory(sizeof(FixedAllocator), 16,
                                                     pool);
    falloc[1] = (FixedAllocator*)AllocPhysicalMemory(sizeof(FixedAllocator), 16,
                                                     pool);
    falloc[2] = (FixedAllocator*)AllocPhysicalMemory(sizeof(FixedAllocator), 16,
                                                     pool);
    falloc[3] = (FixedAllocator*)AllocPhysicalMemory(sizeof(FixedAllocator), 16,
                                                     pool);

    alloc = falloc[0];
    if (alloc) {
        alloc->Create(0x2000, 4, fixedBuffer[0], debugBuffer[0], tracker, true);
        alloc->selfDestruct = true;
    }

    alloc = falloc[1];
    if (alloc) {
        alloc->Create(0x8000, 16, fixedBuffer[1], debugBuffer[1], tracker,
                      true);
        alloc->selfDestruct = true;
    }

    alloc = falloc[2];
    if (alloc) {
        alloc->Create(0x10200, 32, fixedBuffer[2], debugBuffer[2], tracker,
                      true);
        alloc->selfDestruct = true;
    }

    alloc = falloc[3];
    if (alloc) {
        alloc->Create(0x24300, 48, fixedBuffer[3], debugBuffer[3], tracker,
                      true);
        alloc->selfDestruct = true;
    }
}

void PoolManager::Destroy() {
    ((hkBaseObject*)falloc[3])->~hkBaseObject();
    ((hkBaseObject*)falloc[2])->~hkBaseObject();
    ((hkBaseObject*)falloc[1])->~hkBaseObject();
    ((hkBaseObject*)falloc[0])->~hkBaseObject();

    FreePhysicalMemory(falloc[3], sizeof(FixedAllocator), PHYS_MEM_MAIN);
    FreePhysicalMemory(falloc[2], sizeof(FixedAllocator), PHYS_MEM_MAIN);
    FreePhysicalMemory(falloc[1], sizeof(FixedAllocator), PHYS_MEM_MAIN);
    FreePhysicalMemory(falloc[0], sizeof(FixedAllocator), PHYS_MEM_MAIN);

    FreePhysicalMemory(fixedBuffer[3], 0x24300, PHYS_MEM_MAIN);
    FreePhysicalMemory(fixedBuffer[2], 0x10200, PHYS_MEM_MAIN);
    FreePhysicalMemory(fixedBuffer[1], 0x8000, PHYS_MEM_MAIN);
    FreePhysicalMemory(fixedBuffer[0], 0x2000, PHYS_MEM_MAIN);
}

int PoolManager::GetMemorySize(void* mem) {
    if (falloc[0]->IsInRange(mem)) {
        return falloc[0]->GetMemorySize(mem);
    }

    if (falloc[1]->IsInRange(mem)) {
        return falloc[1]->GetMemorySize(mem);
    }

    if (falloc[2]->IsInRange(mem)) {
        return falloc[2]->GetMemorySize(mem);
    }

    if (falloc[3]->IsInRange(mem)) {
        return falloc[3]->GetMemorySize(mem);
    }

    return 0;
}

void* PoolManager::Alloc(int size, eMemMgrTag tag) {
    void* mem = 0;

    if (size <= 4) {
        mem = falloc[0]->Alloc(size, tag);
    }

    if (mem == 0 && size <= 16) {
        mem = falloc[1]->Alloc(size, tag);
    }

    if (mem == 0 && size <= 32) {
        mem = falloc[2]->Alloc(size, tag);
    }

    if (mem == 0 && size <= 48) {
        mem = falloc[3]->Alloc(size, tag);
    }

    return mem;
}

bool PoolManager::Free(void* mem) {
    if (falloc[0]->IsInRange(mem)) {
        falloc[0]->Free(mem);
    } else if (falloc[1]->IsInRange(mem)) {
        falloc[1]->Free(mem);
    } else if (falloc[2]->IsInRange(mem)) {
        falloc[2]->Free(mem);
    } else if (falloc[3]->IsInRange(mem)) {
        falloc[3]->Free(mem);
    } else {
        return false;
    }

    return true;
}

}  // namespace Memory
