// Exception.cpp -- three functions, read from the image with
// tools/disasm.py. The debug exception handler is a System::Module that
// keeps the map file it symbolicates a crash with. MapFileData's
// destructor frees the two blocks it owns and clears both pointers; the
// handler's destructor clears its active flag and destroys the map file
// data; GetPriority hands back a one-entry relative-priority table,
// filled with the media module's pointer on first call under a guard,
// and says so through the count parameter.
//
// Layouts from the DWARF (tools/dwarf_types.py): Module 0x98 with the
// name at +0, the event set at +4, the vptr at +0x14 where the DWARF
// leaves a hole, the context flags at +0x18, sixty binding indices at
// +0x1C and the enabled flag at +0x94; MapFileData 0x10 {mapFilename,
// mapFuncCount, mapFuncStrings, mapFuncList}; ExceptionHandler 0xA8,
// its active flag at +0x95 -- inside the base's trailing padding, since
// Module's members end at 0x95 and only its SIZE rounds to 0x98 -- and
// the map file data at +0x98. RelativePriority {order, associate,
// events} as TRCModule.cpp already has it. The first virtual is
// declared and left undefined so the vtable's home stays in the unity
// unit that holds it. No float or string literal is loaded, so there is
// no pool header; the priority table and its guard are the unit's own
// data.
//
// Four shapes the bytes fixed. The heap enumerator is bound to a const
// reference at each call site, which is what puts the compiler's static
// zero temporary in front of every one of them and makes the free load
// the enum back through it. The second block's helper is inlined and the
// null test that becomes its second `beq` belongs to Memory::Free, which
// takes the enum BY VALUE -- so the reference is loaded before the test,
// as retail has it, where a by-reference Free loads it after and costs
// three words. The first block's helper is a real call, so the two are
// different helpers and not one written twice. And a destructor keeps
// the compiler's vtable-pointer store only when its body makes a member
// call: written as the flag test alone the handler's destructor is 26
// words with retail's three-word store missing, and an empty inline
// member -- of this class, of the base, or of the member's own class --
// brings it back at retail's 29. The full exclusion list is beside the
// function.
//
// NEAR MISS -- __dt__Q25Debug11MapFileDataFv, 35 of 36 words aligned and
// every instruction identical; the word counted is a relocation's NAME.
// See the paragraph beside the destructor.

namespace Memory {

enum GlobalHeapEnum { GlobalHeap = 0 };

void FreeGlobalHeap(void* block, GlobalHeapEnum heap);

// The enum arrives BY VALUE, so the reference behind it is loaded before
// this function's own null test -- which is the order retail has, the
// `lis`/`lwz` off the call site's static temporary ahead of the second
// `beq`. Taking it by reference here instead loads it after the test and
// costs three words.
inline void Free(void* block, GlobalHeapEnum heap) {
    if (block) {
        FreeGlobalHeap(block, heap);
    }
}

}  // namespace Memory

// Out of line in retail: declared here and not defined, so the unit
// emits no copy of it.
template <class H, class T>
void DeleteArray(const H& heap, T* array, unsigned long count);

// Inlined in retail: the null test that becomes the second `beq` is
// Memory::Free's, not one of its own.
template <class H, class T>
inline void Delete(const H& heap, T* p) {
    Memory::Free(p, heap);
}

namespace System {

enum PriorityOrder {
    PRIORITY_BEFORE = 0,
    PRIORITY_AFTER = 1
};

class Module;

class RelativePriority {
public:
    PriorityOrder order;
    Module* associate;
    int events;
};

class EventSet {
public:
    int stage[4];
};

// The vptr follows the two members declared ahead of the first virtual
// (+0x14), which is the hole the DWARF leaves.
class Module {
public:
    Module();

    char* name;
    EventSet events;

    virtual void _v0();
    virtual void _v1();
    virtual const RelativePriority* GetPriority(int& count) const;
    virtual void Startup(int stage);
    virtual void _v4();
    virtual void _v5();
    virtual void _v6();
    virtual void _v7();
    virtual void _v8();
    virtual void Update(int stage);

    int contextFlags;
    short eventBindingIndices[60];
    bool enabled;
};

}  // namespace System

namespace IO {
extern System::Module* mediaModule;
}  // namespace IO

namespace Debug {

class MapFuncEntry;

class MapFileData {
public:
    ~MapFileData();

    char* mapFilename;
    unsigned int mapFuncCount;
    char* mapFuncStrings;
    MapFuncEntry* mapFuncList;
};

class ExceptionHandler : public System::Module {
public:
    // Declared first and undefined: the vtable's home is not this unit.
    // With the destructor ahead of it the destructor is the class's key
    // function, and the object grows a 52-byte __vt__ of undefined
    // entries -- measured, and it changes no instruction.
    virtual void _v1();

    // Non-virtual or virtual makes no difference to the bytes here; it
    // is virtual because retail's vtable holds a destructor in slot 0.
    virtual ~ExceptionHandler();

    virtual const System::RelativePriority* GetPriority(int& count) const;

    // Emits nothing, and is what keeps the vtable-pointer store alive.
    void Uninstall() {}

    bool active;
    MapFileData mapFileData;
};

}  // namespace Debug

// NEAR MISS -- 35 of 36 words, and the one that differs is a NAME, not an
// instruction: all 36 instruction words are identical, and unitcmp counts
// the first `bl` because our relocation names a different symbol from the
// one retail's displacement lands on.
//
// That call reaches 0x800387E0, which the image names
// `Delete<Q26Memory14GlobalHeapEnum,Q34Util23BlockAllocatorArray<Pv>5Block>
// __FRQ26Memory14GlobalHeapEnumPQ34Util23BlockAllocatorArray<Pv>5Block_v`
// -- a TWO-parameter function taking a NON-const reference. Two things
// say that is a folded name rather than the function this call means.
// The body there is seven instructions, `if (p) Free(p, heap)`, which a
// three-parameter array form compiles to identically because it never
// reads its count. And of the 35 `bl` sites in the image that reach that
// address, 23 set r5 in the six instructions before the call -- one
// loading a member, one building the constant 0x90F04 -- while others
// set none. So at least two functions compiled to those 28 bytes and the
// linker kept one name.
//
// Measured, not assumed, before this was left: a defaulted third
// parameter IS mangled (`DeleteB(heap, p)` with `unsigned long count =
// 0` emits `...Ul_v`), so no three-argument call can carry a
// two-parameter name; a two-parameter call emits no `li r5,0` at all
// (16 bytes against 20 in the same probe), so writing it that way drops
// a word and misaligns the rest; and a non-const `H&` refuses the
// enumerator outright (error 10248), so retail's surviving symbol cannot
// even have been called from here. `char` is the DWARF's type for
// mapFuncStrings and is what this file instantiates; retail's `Block`
// comes from the other function that folded onto the address.
Debug::MapFileData::~MapFileData() {
    if (mapFuncStrings) {
        DeleteArray(Memory::GlobalHeap, mapFuncStrings, 0);
        mapFuncStrings = 0;
    }

    if (mapFuncList) {
        Delete(Memory::GlobalHeap, mapFuncList);
        mapFuncList = 0;
    }
}

// The vtable-pointer store at the top is the compiler's, and it survives
// only when the body makes a MEMBER CALL. Written as the flag test alone
// the destructor is 26 words with retail's `lis`/`addi`/`stw` at +0x24
// missing, and the store comes back for an explicit `mapFileData.
// ~MapFileData()` (which then emits the implicit one as well, 32 words),
// for an empty inline member of this class, of the base, or of the
// member's own class -- all three give retail's 29 words. So the bytes
// say a call was made here and that it emitted nothing; which call it
// was is not in them, and the empty inline member below is the shortest
// spelling of that fact. Not the levers: `virtual` on the destructor, a
// volatile flag, a constructor in the unit, an extra declared virtual, a
// base destructor (which adds its own call), a reference to the member,
// or clearing the flag through a pointer -- each measured, none moved a
// word.
Debug::ExceptionHandler::~ExceptionHandler() {
    if (active) {
        Uninstall();
        active = false;
    }
}

const System::RelativePriority* Debug::ExceptionHandler::GetPriority(
    int& count) const {
    static System::RelativePriority priority[1] = {
        { System::PRIORITY_AFTER, IO::mediaModule, 1 },
    };

    count = 1;

    return priority;
}
