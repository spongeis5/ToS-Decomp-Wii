// WAD03 -- the remainder of the third unity blob: one function, read
// from the image with tools/disasm.py.
//
// NewArray<float, Memory::GlobalHeapEnum> is a template instantiation:
// the heap enum comes by reference (lwz r4,0(r3)), the count is scaled
// by sizeof(float) (slwi r3,r5,2), and the last argument is a false
// (li r6,0), all in a tail call to Memory::AllocGlobalHeap. The return
// type is in the mangled name (_Pf), as it is for every template.

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {
enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };

void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap, eMemMgrTag tag,
                      bool clear);
}  // namespace Memory

template <class T, class H>
T* NewArray(const H& heap, eMemMgrTag tag, unsigned long count) {
    return (T*)Memory::AllocGlobalHeap(count * sizeof(T), heap, tag, false);
}

template float* NewArray<float, Memory::GlobalHeapEnum>(
    const Memory::GlobalHeapEnum& heap, eMemMgrTag tag, unsigned long count);
