// MediaConfig.cpp -- one function, read from the image with
// tools/disasm.py. IO::MediaConfig::CreateDatabase gives the media
// config its entry pool: one block of 32 entries from the global heap
// (tag 47, not cleared), then the pool's used count, its buffer and its
// capacity.
//
// Layouts from the DWARF (tools/dwarf_types.py): MediaConfig is 0xC
// bytes and holds a single member, a PoolArray of {int size; Entry*
// pool; int poolSize;} at +0, +4 and +8; Entry is 0x104 bytes, an
// enMediaPath followed by char rootpath[256]. So the 8320 the
// allocation asks for is 32 * 0x104 and the 32 stored at +8 is the same
// count -- the element size is what recovers it, and neither number is
// guessed.
//
// Two shapes the bytes fixed.
//
// The heap enum arrives by CONST REFERENCE, not by value. The callee
// takes it by value (the mangled name says Q26Memory14GlobalHeapEnum),
// yet retail does not write `li r4,0`: it builds an address with a
// `lis` and loads the value back out of an anonymous .data word right
// before the call. That is the static temporary an enumerator bound to
// a reference creates, one per call site, and it is the NewArray
// template WAD03.cpp already carries.
//
// And the ALLOCATION IS SEQUENCED BEFORE ALL THREE STORES, which are in
// offset order 0, 4, 8 after the call. Written as three plain
// assignments with `size = 0` first, that store cannot sink past the
// call and lands ahead of it; a local holding the block puts the call
// first and leaves the three stores in source order. The DWARF
// describes no local here -- only `this` in r31 (tools/dwarf_locals.py)
// -- so the store order is the only thing that says one was written,
// and it says so.
//
// The template has to be spelled `inline`. WAD03.cpp carries the same
// NewArray as an explicit instantiation because retail has that
// function; here retail has no such symbol, and left un-inlined mwcc
// emits the instantiation out of line and calls it -- 17 of 20 words,
// plus an EXTRA function the image does not have. One keyword.
//
// The split is .text only, while the reference binding gives our object
// a .data word of its own, so this unit matches and does not link.

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {
enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };

void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap, eMemMgrTag tag,
                      bool clear);
}  // namespace Memory

// The heap comes by reference, which is what binds the enumerator to a
// static temporary and loads it back at the call site.
template <class T, class H>
inline T* NewArray(const H& heap, eMemMgrTag tag, unsigned long count) {
    return (T*)Memory::AllocGlobalHeap(count * sizeof(T), heap, tag, false);
}

namespace IO {

enum enMediaPath {
    PATH_BOOT,
    PATH_INI,
    PATH_LOG,
    PATH_UNCFILE,
    PATH_NOMORE,
};

class MediaConfig {
public:
    class Entry {
    public:
        enMediaPath tgtPathID;
        char rootpath[256];
    };

    class PoolArray {
    public:
        int size;
        Entry* pool;
        int poolSize;
    };

    void CreateDatabase();

    PoolArray configArray;
};

}  // namespace IO

void IO::MediaConfig::CreateDatabase() {
    Entry* pool = NewArray<Entry, Memory::GlobalHeapEnum>(
        (Memory::GlobalHeapEnum)0, (eMemMgrTag)47, 32);

    configArray.size = 0;
    configArray.pool = pool;
    configArray.poolSize = 32;
}
