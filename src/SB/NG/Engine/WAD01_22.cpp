// WAD01_22 -- two functions, read from the image with tools/disasm.py.
// Memory::FreePhysicalMemory2 gives a block back to whichever of the two
// physical expanded heaps contains it, the first heap's handle loaded
// before the lookup. The other function is the copy-assignment operator
// of _GXRenderModeObj, the SDK's render mode, which the compiler emitted
// out of line for a struct assignment somewhere in the unity unit. It
// is not reproduced here: the compiler emits it only where an assignment
// is compiled, and an unused inline function assigning two objects does
// not emit it (measured 2026-09-02). The struct stays as the DWARF has
// it (0x3C bytes), for whoever writes the assignment that made it.

extern "C" {
typedef void* MEMHeapHandle;

MEMHeapHandle MEMFindContainHeap(void* block);
void MEMFreeToExpHeap(MEMHeapHandle heap, void* block);
}

struct _GXRenderModeObj {
    int viTVmode;
    unsigned short fbWidth;
    unsigned short efbHeight;
    unsigned short xfbHeight;
    unsigned short viXOrigin;
    unsigned short viYOrigin;
    unsigned short viWidth;
    unsigned short viHeight;
    int xFBmode;
    unsigned char field_rendering;
    unsigned char aa;
    unsigned char sample_pattern[12][2];
    unsigned char vfilter[7];
};

namespace Memory {

extern MEMHeapHandle physicalMEM1Handle;
extern MEMHeapHandle physicalMEM2Handle;

void FreePhysicalMemory2(void* block);

}  // namespace Memory

void Memory::FreePhysicalMemory2(void* block) {
    if (MEMFindContainHeap(block) == physicalMEM1Handle) {
        MEMFreeToExpHeap(physicalMEM1Handle, block);
    } else {
        MEMFreeToExpHeap(physicalMEM2Handle, block);
    }
}
