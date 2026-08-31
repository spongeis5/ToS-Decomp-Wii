// C:/branches/SB09/main/NG/Source/Engine/Memory/FixedAllocator.cpp
//
// Layout from the Wii build's DWARF (tools/dwarf_types.py):
//
//   class FixedAllocator /* 0x38 */ {
//       /* +0x0  */ CriticalSection memMutex;   // 0x20
//       /* +0x20 */ FixedHeader* freeList;
//       /* +0x24 */ bool selfDestruct;
//       /* +0x25 */ bool beThreadSafe;
//       /* +0x28 */ int numBlocks;
//       /* +0x2C */ int blockSize;
//       /* +0x30 */ unsigned char* bufferStart;
//       /* +0x34 */ unsigned char* bufferEnd;
//   };
//   class FixedHeader /* 0x4 */ { FixedHeader* next; };
//
// CriticalSection is constructed by Create, NOT by the constructor: the ctor
// at 801EA880 is three instructions and touches only +0x24. So the member's
// setup (refcount = 0, OSInitMutex(&mutex)) is not a C++ constructor here --
// it is an Init() the caller invokes, and Create invokes it only when the
// allocator was asked to be thread safe. MediaObject.cpp spells the same two
// instructions as a constructor because there it runs unconditionally; a
// constructor would emit here too, and nothing does.
//
// The free-list build loop RELOADS bufferEnd every iteration and compares
// UNSIGNED (`cmplw`), which is what a pointer comparison against a member
// looks like when the body stores through a pointer that may alias it.
// blockSize stays in a register across the loop because the loop is written
// against the PARAMETER, not the member it was just stored into.
//
// Same distinction decides the last word of Create. `if (beThreadSafe)`, read
// back from the bool member, emits `clrlwi. r0, r9, 0x18` -- mwcc forwards the
// store but still truncates to the member's byte. Retail has a bare
// `cmpwi r9, 0`, which is the PARAMETER being tested. The member is stored
// either way; only the test differs.

struct OSMutex {
    unsigned char _bytes[0x18];
};

class OSThread;

extern "C" void OSInitMutex(OSMutex* mutex);

namespace System {

class CriticalSection {
public:
    void Init() {
        refcount = 0;
        OSInitMutex(&mutex);
    }

    void Enter();
    void Exit();

    int refcount;
    OSMutex mutex;
    OSThread* owner;
};

}  // namespace System

// Abridged: only the name is load-bearing here -- it reaches the object as
// `10eMemMgrTag` in Alloc's mangled name, and the argument is never read.
enum eMemMgrTag {
    eMemMgrTag_2D = 0,
    eMemMgrTag_NumTags = 103
};

namespace Memory {

class MemTracker;

class FixedHeader {
public:
    FixedHeader* next;
};

class FixedAllocator {
public:
    FixedAllocator();

    void Create(int poolSize, int size, void* buffer, void* bufferLimit,
                MemTracker* tracker, bool threadSafe);
    void* Alloc(int size, eMemMgrTag tag);
    int GetMemorySize(void* mem);
    void Free(void* mem);

    System::CriticalSection memMutex;
    FixedHeader* freeList;
    bool selfDestruct;
    bool beThreadSafe;
    int numBlocks;
    int blockSize;
    unsigned char* bufferStart;
    unsigned char* bufferEnd;
};

FixedAllocator::FixedAllocator() {
    selfDestruct = false;
}

void FixedAllocator::Create(int poolSize, int size, void* buffer,
                            void* bufferLimit, MemTracker* tracker,
                            bool threadSafe) {
    unsigned char* p;

    blockSize = size;
    bufferStart = (unsigned char*)buffer;
    freeList = (FixedHeader*)buffer;
    numBlocks = poolSize / size;
    bufferEnd = bufferStart + numBlocks * size;

    p = (unsigned char*)buffer;
    while (p < bufferEnd - size) {
        unsigned char* next = p + size;
        *(unsigned char**)p = next;
        p = next;
    }
    *(unsigned char**)p = 0;

    beThreadSafe = threadSafe;
    if (threadSafe) {
        memMutex.Init();
    }
}

void* FixedAllocator::Alloc(int size, eMemMgrTag tag) {
    FixedHeader* block;

    if (size == 0) {
        return 0;
    }

    if (beThreadSafe) {
        memMutex.Enter();
    }

    block = freeList;
    if (block == 0) {
        if (beThreadSafe) {
            memMutex.Exit();
        }
        return 0;
    }

    freeList = block->next;

    if (beThreadSafe) {
        memMutex.Exit();
    }

    return block;
}

int FixedAllocator::GetMemorySize(void* mem) {
    if (mem == 0) {
        return -1;
    }

    return blockSize;
}

void FixedAllocator::Free(void* mem) {
    if (mem == 0) {
        return;
    }

    if (beThreadSafe) {
        memMutex.Enter();
    }

    ((FixedHeader*)mem)->next = freeList;
    freeList = (FixedHeader*)mem;

    if (beThreadSafe) {
        memMutex.Exit();
    }
}

}  // namespace Memory
