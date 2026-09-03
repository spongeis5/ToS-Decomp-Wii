// Channel.cpp -- two functions, read from the image with
// tools/disasm.py. The graphics channel is the display list buffer:
// Create takes 16 KB from the global heap, 16-aligned, tag 48 and not
// cleared, and keeps it in the class's own static; Destroy frees it
// back, tail-calling the deallocator behind a null test.
//
// Layout from the DWARF (tools/dwarf_types.py): Graphics::Channel has
// no described members at all -- one byte -- so everything it owns is
// static, which is what the bytes show: Create ignores both of its
// OSThread* parameters and neither function touches r3 as a `this`.
// `buffer` is the image's own global symbol buffer__Q28Graphics7Channel
// in .bss, and PostRenderChannel.cpp's split owns the data around it,
// so it is DECLARED here and not defined -- this unit's split is .text
// only.
//
// Two shapes the bytes fixed.
//
// The heap enum arrives by CONST REFERENCE at both call sites. Each
// callee takes it by value, and retail still builds an address with a
// `lis` and loads the value back out of an anonymous .data word -- a
// different word for Create than for Destroy, which is the static
// temporary an enumerator bound to a reference creates, one per call
// site. The allocating side is the NewArray template WAD03.cpp carries,
// with an alignment argument added because this allocator's mangled
// name has a second Ul; the freeing side is the DeleteArray
// RenderModeEntity.cpp carries, whose null test is the `beqlr` here.
//
// Both templates have to be spelled `inline`: retail has no such
// symbol, and left un-inlined mwcc emits the instantiation out of line
// and calls it. That was measured on MediaConfig.cpp first.
//
// And DeleteArray COPIES THE REFERENCE BEFORE THE TEST. Retail builds
// both addresses and performs both loads, buffer then heap, and only
// then compares -- the heap load stands above a branch it is not needed
// on. With the dereference left inside the `if`, as RenderModeEntity's
// DeleteArray has it, ours emits the same eight instructions in another
// order: load, test, branch, load. That is 5 of 8 words, all of them
// placement. Reading the reference into a local at the top of the
// helper is what puts the load ahead of the compare; the scheduler does
// not hoist it on its own.
//
// The reference binding gives our object .data words of its own where
// the split has none, so this unit matches and does not link.

class OSThread;

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {
enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };

void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap,
                      unsigned long align, eMemMgrTag tag, bool clear);
void FreeGlobalHeap(void* block, GlobalHeapEnum heap);
}  // namespace Memory

// The heap comes by reference, which is what binds the enumerator to a
// static temporary and loads it back at the call site.
template <class T, class H>
inline T* NewArray(const H& heap, unsigned long align, eMemMgrTag tag,
                   unsigned long count) {
    return (T*)Memory::AllocGlobalHeap(count * sizeof(T), heap, align, tag,
                                       false);
}

template <class H, class T>
inline void DeleteArray(const H& heap, T* array, unsigned long count) {
    H h = heap;

    if (array) {
        Memory::FreeGlobalHeap(array, h);
    }
}

namespace Graphics {

class Channel {
public:
    static void Create(OSThread* mainThread, OSThread* renderThread);
    static void Destroy();

    static void* buffer;
};

}  // namespace Graphics

void Graphics::Channel::Create(OSThread*, OSThread*) {
    buffer = NewArray<char, Memory::GlobalHeapEnum>(
        (Memory::GlobalHeapEnum)0, 16, (eMemMgrTag)48, 16384);
}

void Graphics::Channel::Destroy() {
    DeleteArray((Memory::GlobalHeapEnum)0, buffer, 0);
}
