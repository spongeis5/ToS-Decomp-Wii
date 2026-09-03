// MemTracker.cpp -- five functions, read from the image with
// tools/disasm.py. Memory::GetGlobalHeapTracker picks one of the two
// file-scope trackers by heap. The constructor clears the log handle,
// the top line and the running total, builds the tag and department
// tables with _Init, gives the first four departments a one-megabyte
// ceiling and then initialises its own mutex; the destructor is the
// compiler's, destroying the IO::File member with the don't-delete flag
// and freeing `this` when the flag says to. _SetDeptMaxMem walks the
// five-entry department table for a matching id and writes the ceiling
// into it. _Init is three loops: the first zeroes the 103 per-tag
// counters and cycle-sorts Memory::TagLookupTable so entry i holds tag
// i, the second zeroes the five per-department counters and cycle-sorts
// Memory::DeptLookupTable the same way, and the third fills the
// eight-deep tag stack with the out-of-range tag 103.
//
// Layouts from the DWARF (tools/dwarf_types.py): MemTracker is 0x9FC --
// totalSize at 0, sizePerTag[103] at 4, sizePerDept[5] at 0x1A0, the
// IO::File at 0x1B4, tagStack[8] at 0x1B8, logFileName[2048] at 0x1D8,
// topLine at 0x9D8 and the CriticalSection at 0x9DC, whose OSMutex
// begins four bytes into it at 0x9E0 -- which is the address the
// constructor hands OSInitMutex. TagLookup is 0xC (tagID, deptID,
// tagNameString) and DeptLookup is 0x10 (deptID, deptNameString,
// maxMem, throwAssert); the strides in the bytes, 12 and 16, are the
// same two numbers. eMemMgrTag_NumTags is 103 and DeptTag_NumDepts is
// 5, both from the DWARF's own enumerations.
//
// Five shapes the bytes fixed.
//
// GetGlobalHeapTracker forms the address of the MAIN tracker before it
// branches, because `bnelr` returns that value. mwcc hoists the value of
// the LAST return and inverts the test to reach it, so the fall-through
// return has to be the main tracker and the tested early return the
// secondary; written the other way round the same seven words come out
// with `beqlr` and the two addresses swapped, one word.
//
// The three loops of _Init each declare their OWN counter. With one
// `int i` shared by all three the first loop's counter lands in r23 and
// the second's in r23 as well, where retail has r27 and r23 -- five
// words, every one of them that register. Three separate declarations
// give retail's pair.
//
// The mutex is initialised at the END of the constructor, after _Init
// and the four ceilings, which a member's constructor could never do:
// a member with a constructor runs before the body. So the two words
// are body statements, and the IO::File's `stream = 0` -- which retail
// stores FIRST, before the top line and the total -- is a body
// statement too rather than a File constructor.
//
// The second cycle-sort tests Memory::TagLookupTable while it swaps
// Memory::DeptLookupTable. That is what the bytes say: the condition's
// base register is the tag table with a stride of 12, the swapped
// entries are the department table with a stride of 16. Written the
// obvious way -- testing the table it sorts -- the loop is the same
// length and every base register is wrong, so the copy-and-paste in the
// original is reproduced rather than corrected.
//
// The swap is three assignments and not a copy-initialisation. Retail
// calls the implicit operator= three times per iteration; `T temp =
// table[k];` would be a copy CONSTRUCTOR, so `temp` is declared
// uninitialised and assigned. And the department temporary sits at
// 8(r1) with the tag temporary above it at 24(r1) although the tag loop
// comes first, which is mwcc allocating locals in reverse declaration
// order -- declaring them in source order is what puts them there.
//
// NEAR MISS -- _Init, three words of 83, and all three are the same
// thing: a branch to the 12-byte operator=. tools/fndiff.py aligns 83
// of retail's 83 instruction words, 100%, so no instruction differs;
// what unitcmp counts is the NAME the relocation carries. mwld folded
// that weak body onto the identical one for Graphics::Sampler, and the
// only name the image has at 0x80012F90 is
// `__as__Q28Graphics7SamplerFRCQ28Graphics7Sampler` where ours is
// `__as__Q26Memory9TagLookupFRCQ26Memory9TagLookup`. The 16-byte
// operator= for DeptLookup was NOT folded -- retail keeps it at
// 0x801EBD70, in the chunk right after this split, and it comes out
// byte-identical here -- so the three calls that check out and the
// three that do not differ by exactly one thing, the fold. No spelling
// renames a folded symbol; the same object also carries our own weak
// TagLookup instance, which unitcmp reports as EXTRA for the same
// reason.

namespace IO {

// Only the destructor is reached from here: the compiler calls it on
// the member with the don't-delete flag.
class File {
public:
    ~File();

    void* stream;
};

}  // namespace IO

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

extern "C" void OSInitMutex(OSMutex* mutex);

namespace System {

class CriticalSection {
public:
    int refcount;
    OSMutex mutex;
    OSThread* owner;
};

}  // namespace System

enum eMemMgrTag { eMemMgrTag_NumTags = 103 };

enum DeptTag { DeptTag_NumDepts = 5 };

namespace Memory {

class TagLookup {
public:
    eMemMgrTag tagID;
    DeptTag deptID;
    char* tagNameString;
};

class DeptLookup {
public:
    DeptTag deptID;
    char* deptNameString;
    int maxMem;
    unsigned int throwAssert;
};

extern TagLookup TagLookupTable[103];
extern DeptLookup DeptLookupTable[5];

enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };

class MemTracker {
public:
    MemTracker(int size);
    ~MemTracker();

    void _Init();
    void _SetDeptMaxMem(int deptID, int maxMem);

    int totalSize;
    int sizePerTag[103];
    int sizePerDept[5];
    IO::File trckLog;
    unsigned int tagStack[8];
    char logFileName[2048];
    int topLine;
    System::CriticalSection trackerMutex;
};

extern MemTracker globalHeapTrackerMain;
extern MemTracker globalHeapTrackerSecondary;

MemTracker* GetGlobalHeapTracker(GlobalHeapEnum heap);

}  // namespace Memory

Memory::MemTracker* Memory::GetGlobalHeapTracker(GlobalHeapEnum heap) {
    if (heap == 2) {
        return &globalHeapTrackerSecondary;
    }

    return &globalHeapTrackerMain;
}

Memory::MemTracker::MemTracker(int size) {
    trckLog.stream = 0;
    topLine = 0;
    totalSize = size;

    _Init();

    _SetDeptMaxMem(0, 1000000);
    _SetDeptMaxMem(1, 1000000);
    _SetDeptMaxMem(2, 1000000);
    _SetDeptMaxMem(3, 1000000);

    trackerMutex.refcount = 0;
    OSInitMutex(&trackerMutex.mutex);
}

Memory::MemTracker::~MemTracker() {}

void Memory::MemTracker::_SetDeptMaxMem(int deptID, int maxMem) {
    for (int i = 0; i < DeptTag_NumDepts; i++) {
        if (DeptLookupTable[i].deptID == deptID) {
            DeptLookupTable[i].maxMem = maxMem;

            return;
        }
    }
}

void Memory::MemTracker::_Init() {
    for (int i = 0; i < eMemMgrTag_NumTags; i++) {
        sizePerTag[i] = 0;

        while (TagLookupTable[i].tagID != i) {
            TagLookup tagTemp;

            tagTemp = TagLookupTable[TagLookupTable[i].tagID];
            TagLookupTable[TagLookupTable[i].tagID] = TagLookupTable[i];
            TagLookupTable[i] = tagTemp;
        }
    }

    for (int i = 0; i < DeptTag_NumDepts; i++) {
        sizePerDept[i] = 0;

        while (TagLookupTable[i].tagID != i) {
            DeptLookup deptTemp;

            deptTemp = DeptLookupTable[DeptLookupTable[i].deptID];
            DeptLookupTable[DeptLookupTable[i].deptID] = DeptLookupTable[i];
            DeptLookupTable[i] = deptTemp;
        }
    }

    for (int i = 0; i < 8; i++) {
        tagStack[i] = eMemMgrTag_NumTags;
    }
}
