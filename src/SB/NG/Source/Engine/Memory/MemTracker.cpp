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
// Not a sixth shape -- none of what follows moves a byte -- but
// recovered and worth keeping. The DWARF names both temporaries and
// both scopes. `tagswapstorage` is declared at line 450 and `deptswapstorage` at 462, each between its
// loop's zeroing store (449, 461) and its `while` (451, 463) -- so they
// are in the FOR body, not the while body, and each takes its whole for
// statement as its range. The counters are i, j and k at 447, 459 and
// 471, three declarations as the register pair already said. Retail's
// line numbers also skeleton the file: 447 for, 448 brace, 449 store,
// 450 declaration, 451 while, 452 brace, 453-455 the swap, 456-457 the
// braces, 458 blank, and the same again at 459 and at 471. That is
// Allman, which is what .clang-format asks for and what this tree does
// not do -- 107 of its 112 game-code files brace K&R -- so it is
// recorded here and not adopted.
//
// NEAR MISS -- _Init, three words of 83, and all three are the same
// thing: the NAME on a relocation, not an instruction. `tools/unitcmp.py
// <unit> -v` prints ours beside retail's and 83 of 83 words are equal
// once the 12 relocated fields are masked; what unitcmp still counts is
// that three of the six masked REL24s carry a symbol retail's resolved
// displacement does not land on. Ours say
// `__as__Q26Memory9TagLookupFRCQ26Memory9TagLookup`; retail's three
// reach 0x80012F90, and the only name the image has there is
// `__as__Q28Graphics7SamplerFRCQ28Graphics7Sampler`. Graphics::Sampler
// is 0xC bytes like TagLookup, so their implicit operator= bodies are
// the same seven words and mwld kept one.
//
// This is reloc_audit.py's SECOND reading -- a folded body, not a wrong
// target -- and it was measured rather than assumed, three ways:
//
//   * `__as__Q26Memory9TagLookupFRCQ26Memory9TagLookup` occurs 0 times
//     among the image's 40,616 named symbols, while DeptLookup's
//     occurs once. A wrong target would have the name somewhere else.
//   * 0x80012F90 is the ONLY function in the image with that body, and
//     48 call sites reach it -- from zScheduler, Domains::ActUnloadAll,
//     zQueue<AnimSetup::AnimSoundEntry,1024>, hkSimplexSolverSolve,
//     GHashSetBase<...> and this function. Those classes share nothing
//     but a twelve-byte memberwise copy.
//   * The folding is confined to WEAK bodies: 95 groups of identical
//     bodies of 64 bytes or less survive under more than one name, and
//     0 of the 95 share an address; no address in the image carries two
//     sized function names.
//
// DeptLookup's operator= was NOT folded -- 0x10 bytes make a nine-word
// body no earlier unit had emitted -- so retail keeps it at 0x801EBD70,
// right after this function, and it comes out byte-identical here. The
// three calls that check out and the three that do not differ by
// exactly one thing, the fold.
//
// Our own TagLookup instance is byte-identical to 0x80012F90 and
// STB_WEAK, so mwld folds ours onto the same copy; that is why unitcmp
// reports it EXTRA, and why the EXTRA is correct rather than a defect.
// No spelling reaches it: the mangled name follows from the type, the
// type follows from the table, and the DWARF names that table's element
// type TagLookup. Naming the survivor instead would be a lie for the
// link, which is the answer NOTES.md already gives for zPlayerRun and
// zPlayerFall.
//
// 27 spellings measured with tools/sweep_src.py, all 27 compiled: 16
// tie at three words -- byte-identical output -- and 11 cost words.
// None of the 16 is a lever here. Four are the temporary's placement,
// inside the `while`, in the `for` body before or after the zeroing
// store, or at the top of the function; three are the condition, as
// `!(== )`, with the load cast to int, or with the counter cast to
// eMemMgrTag; three are constants, the loop bounds as 103 and 5 rather
// than the enumerators, the stack depth as an enumerator rather than 8,
// and the stack's fill as 103 rather than eMemMgrTag_NumTags; two are
// the counters named i/j/k rather than i/i/i, with and without the
// declaration move; three are the type, TagLookup as a struct, with int
// fields instead of enum fields, and with a hand-written operator=
// whose body matches the implicit one; the sixteenth is the spelling on
// disk. The 11 that cost are recorded so they are not retried: `do {}
// while` +70 and `for (;;)` with a `break` +30, each at all four
// placements, the swap index hoisted into a local +65, the entry
// reached through a reference +76, and the department temporary
// declared before the tag temporary +4 -- which is the reverse-order
// allocation two paragraphs up, measured from the other side.

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
        TagLookup tagswapstorage;

        while (TagLookupTable[i].tagID != i) {
            tagswapstorage = TagLookupTable[TagLookupTable[i].tagID];
            TagLookupTable[TagLookupTable[i].tagID] = TagLookupTable[i];
            TagLookupTable[i] = tagswapstorage;
        }
    }

    for (int j = 0; j < DeptTag_NumDepts; j++) {
        sizePerDept[j] = 0;
        DeptLookup deptswapstorage;

        while (TagLookupTable[j].tagID != j) {
            deptswapstorage = DeptLookupTable[DeptLookupTable[j].deptID];
            DeptLookupTable[DeptLookupTable[j].deptID] = DeptLookupTable[j];
            DeptLookupTable[j] = deptswapstorage;
        }
    }

    for (int k = 0; k < 8; k++) {
        tagStack[k] = eMemMgrTag_NumTags;
    }
}
