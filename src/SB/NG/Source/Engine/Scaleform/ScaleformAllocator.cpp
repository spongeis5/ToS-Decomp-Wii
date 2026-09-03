// ScaleformAllocator.cpp -- six functions, read from the image with
// tools/disasm.py. Scaleform's GFx library takes its memory through a
// GSysAlloc; this is the Wii one. HeapAllocatorWii IS a GSysAllocStatic
// (the slow, large segment) and CONTAINS a second one (the fast, small
// segment in MEM1), and every request goes to the fast allocator first
// when the heap it names is one of the three fast-memory heap ids, then
// falls back to the base. The constructor builds both allocators empty,
// clears the six counters and publishes itself in Scaleform::sfAllocator.
// Create gives the base a 5.25 MB segment out of global heap 2 and the
// fast allocator a 1 MB segment out of heap 1, takes heap 2's memory
// tracker and initialises the lock. Alloc, Free and ReallocInPlace are
// the three GSysAlloc virtuals: each asks ShouldBeInFastMem, tries the
// fast allocator, falls back to GSysAllocStatic's own, and then keeps
// per-segment counts -- a count, a running total and a peak, indexed 0
// for fast and 1 for the fallback. A failed Alloc prints the request.
//
// Layouts from the DWARF (tools/dwarf_types.py):
// Scaleform::HeapAllocatorWii is 0x1AC on GSysAllocStatic 0xB4, with
// int sfAllocCount[2] at +0xB4, sfAllocUsageCurrent[2] at +0xBC,
// sfAllocUsagePeak[2] at +0xC4, MemTracker* memTracker at +0xCC, the
// second GSysAllocStatic fastMemAlloc at +0xD0, bool mSpam at +0x184,
// GlobalHeapEnum mHeap at +0x188 and a 0x20-byte CriticalSection
// heapLock at +0x18C. GSysAllocStatic is 0xB4 on GSysAlloc (0x4, the
// vtable pointer alone) with MinSize +0x4, NumSegments +0x8,
// pAllocator +0xC, PrivateData[8] +0x10 and TotalSpace +0xB0; the
// DWARF names `Segments` at +0x30 but its four elements span 0x80
// bytes, so it is written here as the byte count the two offsets fix.
// GMemoryHeap is 0x54 with HeapInfo Info at +0x18 and HeapDesc Desc at
// +0 of it, whose HeapId is at +0x18 -- which is the +0x30 that
// ShouldBeInFastMem reads.
//
// The vtable slots are recovered fact and the names come from the
// mangled symbols. mwcc puts the Nth virtual at 8 + 4N, and the calls
// on fastMemAlloc go through +12, +16 and +20: Alloc, Free and
// ReallocInPlace are virtuals 1, 2 and 3, so one virtual ahead of them
// is declared and never defined. A call on the member dispatches
// through the vtable even though its type is exact; a call on the base
// is qualified (`GSysAllocStatic::Alloc`) and is a direct branch, which
// is what keeps it out of infinite recursion.
//
// Four shapes the bytes fixed.
//
// The counters are cleared with CHAINED assignments. Retail stores
// index 1 before index 0 in all three pairs -- +0xB8 then +0xB4, +0xC0
// then +0xBC, +0xC8 then +0xC4 -- which is `a[0] = a[1] = 0;`
// evaluated right to left; two separate statements come out the other
// way round.
//
// ShouldBeInFastMem's three ids are one range and one equality:
// `addi r0,r3,-5 ; cmplwi r0,1 ; ble` for 5 and 6, then `cmplwi r3,3`.
// The null heap is refused by an early `return false` whose branch is
// the one that SKIPS it, so it is written `if (heap == 0) return
// false;` and not as the fall-through.
//
// ReallocInPlace picks its counter index with a second call to
// ShouldBeInFastMem and a `cntlzw`/`rlwinm` pair rather than a branch:
// that is mwcc folding a conditional expression over PLAIN INTS, so
// the index is `ShouldBeInFastMem(heap) ? 0 : 1` and not an if.
//
// Alloc and Free carry the index in a variable set to 1 only on the
// fallback path, and the fast path leaves the zero it was initialised
// with -- there is no `= 0` store inside the fast arm at all. Their two
// locals are declared in OPPOSITE orders, which is the whole of what
// Free was out by: with the result declared first, Free came out 7 of
// 51 words wrong and every one of the seven was r30 against r31, the
// two locals in the other registers. Alloc wants the result first
// (r30, index r29) and Free wants the index first (r31, result r30).
// The initialisation order in the bytes is the same either way, so the
// registers are the only thing that says so.
//
// NO NEAR MISS. `python tools/unitcmp.py
// SB/NG/Source/Engine/Scaleform/ScaleformAllocator.cpp` reports 6 of 6
// byte-identical, 1,064 bytes, which is every function the split holds.
// That is unitcmp's answer and not the oracle's: this unit has not been
// through ninja, is not marked Matching in configure.py, and nothing
// here has been linked or placed. It carries a generated pool header,
// so it cannot link as a fragment in any case.

#include "SB/NG/Source/Engine/Scaleform/ScaleformAllocator.pool.h"

class OSThread;

struct OSMutex {
    unsigned char _bytes[0x18];
};

extern "C" {
void OSInitMutex(OSMutex* mutex);
void OSReport(const char* format, ...);
}

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

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

// The enumerator NAMES are not in the image; the values 1 and 2 are.
enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };

MemTracker* GetGlobalHeapTracker(GlobalHeapEnum heap);
void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap,
                      unsigned long align, eMemMgrTag tag, bool physical);

}  // namespace Memory

class GMemoryHeap;
class GHeapAllocLite;
class GHeapAllocEngine;
class GHeapDebugStorage;

class GListNode {
public:
    GListNode* pPrev;
    GListNode* pNext;
};

class HeapDesc {
public:
    unsigned int Flags;
    unsigned long MinAlign;
    unsigned long Granularity;
    unsigned long Reserve;
    unsigned long Threshold;
    unsigned long Limit;
    unsigned long HeapId;
};

class HeapInfo {
public:
    HeapDesc Desc;
    GMemoryHeap* pParent;
    class GSysAlloc* pSysAlloc;
    char* pName;
};

class GList {
public:
    GListNode Root;
};

class GMemoryHeap {
public:
    GListNode _base0;
    unsigned long SelfSize;
    unsigned int RefCount;
    unsigned long OwnerThreadId;
    void* pAutoRelease;
    HeapInfo Info;
    GList ChildHeaps;
    unsigned char HeapLock;
    bool UseLocks;
    bool TrackDebugInfo;
    GHeapAllocEngine* pEngine;
    GHeapDebugStorage* pDebugStorage;
};

// 0x4 bytes: the vtable pointer and nothing else. One virtual ahead of
// the three, so Alloc lands on slot +12 as the bytes have it.
class GSysAlloc {
public:
    virtual void _v0();
    virtual void* Alloc(unsigned long size, unsigned long align,
                        GMemoryHeap* heap);
    virtual bool Free(void* ptr, unsigned long size, unsigned long align,
                      GMemoryHeap* heap);
    virtual bool ReallocInPlace(void* oldPtr, unsigned long oldSize,
                                unsigned long newSize, unsigned long align,
                                GMemoryHeap* heap);
};

class GSysAllocStatic : public GSysAlloc {
public:
    GSysAllocStatic(void* p1 = 0, unsigned long s1 = 0, void* p2 = 0,
                    unsigned long s2 = 0, void* p3 = 0, unsigned long s3 = 0,
                    void* p4 = 0, unsigned long s4 = 0);

    bool AddMemSegment(void* p, unsigned long size);

    virtual void* Alloc(unsigned long size, unsigned long align,
                        GMemoryHeap* heap);
    virtual bool Free(void* ptr, unsigned long size, unsigned long align,
                      GMemoryHeap* heap);
    virtual bool ReallocInPlace(void* oldPtr, unsigned long oldSize,
                                unsigned long newSize, unsigned long align,
                                GMemoryHeap* heap);

    unsigned long MinSize;
    unsigned long NumSegments;
    GHeapAllocLite* pAllocator;
    unsigned long PrivateData[8];
    // +0x30..+0xB0: four segment descriptors of 0x20 bytes each.
    unsigned char Segments[0x80];
    unsigned long TotalSpace;
};

namespace Scaleform {

class HeapAllocatorWii;

extern HeapAllocatorWii* sfAllocator;

bool ShouldBeInFastMem(GMemoryHeap* heap);

class HeapAllocatorWii : public GSysAllocStatic {
public:
    HeapAllocatorWii();

    void Create(Memory::GlobalHeapEnum heap);

    virtual void* Alloc(unsigned long size, unsigned long align,
                        GMemoryHeap* heap);
    virtual bool Free(void* ptr, unsigned long size, unsigned long align,
                      GMemoryHeap* heap);
    virtual bool ReallocInPlace(void* oldPtr, unsigned long oldSize,
                                unsigned long newSize, unsigned long align,
                                GMemoryHeap* heap);

    int sfAllocCount[2];
    int sfAllocUsageCurrent[2];
    int sfAllocUsagePeak[2];
    Memory::MemTracker* memTracker;
    GSysAllocStatic fastMemAlloc;
    bool mSpam;
    Memory::GlobalHeapEnum mHeap;
    System::CriticalSection heapLock;
};

HeapAllocatorWii* sfAllocator;

HeapAllocatorWii::HeapAllocatorWii() {
    mSpam = false;
    mHeap = (Memory::GlobalHeapEnum)0;

    sfAllocCount[0] = sfAllocCount[1] = 0;
    sfAllocUsageCurrent[0] = sfAllocUsageCurrent[1] = 0;
    sfAllocUsagePeak[0] = sfAllocUsagePeak[1] = 0;

    sfAllocator = this;
}

void HeapAllocatorWii::Create(Memory::GlobalHeapEnum heap) {
    mHeap = (Memory::GlobalHeapEnum)2;
    memTracker = Memory::GetGlobalHeapTracker((Memory::GlobalHeapEnum)2);

    AddMemSegment(Memory::AllocGlobalHeap(0x540000, (Memory::GlobalHeapEnum)2,
                                          128, (eMemMgrTag)80, true),
                  0x540000);
    fastMemAlloc.AddMemSegment(
        Memory::AllocGlobalHeap(0x100000, (Memory::GlobalHeapEnum)1, 128,
                                (eMemMgrTag)80, true),
        0x100000);

    heapLock.refcount = 0;
    OSInitMutex(&heapLock.mutex);
}

void* HeapAllocatorWii::Alloc(unsigned long size, unsigned long align,
                              GMemoryHeap* heap) {
    void* mem = 0;
    int seg = 0;

    if (ShouldBeInFastMem(heap)) {
        mem = fastMemAlloc.Alloc(size, align, heap);
    }

    if (mem == 0) {
        mem = GSysAllocStatic::Alloc(size, align, heap);
        seg = 1;
    }

    if (mem == 0) {
        OSReport("Scaleform out of memory!!\n");
        OSReport("Size: %d  Align: %d\n", size, align);
    }

    if (mem != 0) {
        sfAllocCount[seg]++;
        sfAllocUsageCurrent[seg] += size;

        if (sfAllocUsagePeak[seg] < sfAllocUsageCurrent[seg]) {
            sfAllocUsagePeak[seg] = sfAllocUsageCurrent[seg];
        }
    }

    return mem;
}

bool ShouldBeInFastMem(GMemoryHeap* heap) {
    if (heap == 0) {
        return false;
    }

    switch (heap->Info.Desc.HeapId) {
    case 5:
    case 6:
    case 3:
        return true;
    }

    return false;
}

bool HeapAllocatorWii::Free(void* ptr, unsigned long size, unsigned long align,
                            GMemoryHeap* heap) {
    int seg = 0;
    bool freed = false;

    if (ShouldBeInFastMem(heap)) {
        freed = fastMemAlloc.Free(ptr, size, align, heap);
    }

    if (!freed) {
        freed = GSysAllocStatic::Free(ptr, size, align, heap);
        seg = 1;
    }

    if (freed) {
        sfAllocCount[seg]--;
        sfAllocUsageCurrent[seg] -= size;
    }

    return freed;
}

bool HeapAllocatorWii::ReallocInPlace(void* oldPtr, unsigned long oldSize,
                                      unsigned long newSize,
                                      unsigned long align, GMemoryHeap* heap) {
    bool resized;

    if (ShouldBeInFastMem(heap)) {
        resized = fastMemAlloc.ReallocInPlace(oldPtr, oldSize, newSize, align,
                                              heap);
    } else {
        resized = GSysAllocStatic::ReallocInPlace(oldPtr, oldSize, newSize,
                                                  align, heap);
    }

    if (resized) {
        int seg = ShouldBeInFastMem(heap) ? 0 : 1;

        sfAllocUsageCurrent[seg] += newSize - oldSize;

        if (sfAllocUsagePeak[seg] < sfAllocUsageCurrent[seg]) {
            sfAllocUsagePeak[seg] = sfAllocUsageCurrent[seg];
        }
    }

    return resized;
}

}  // namespace Scaleform
