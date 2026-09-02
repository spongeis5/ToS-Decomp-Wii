// WAD02_7_1 -- one function, read from the image with tools/disasm.py:
// Memory::FreePhysicalMemory1 returns a block to whichever of the two
// physical expanded heaps contains it: the MEM2 handle when the SDK
// says the block is in that heap, the MEM1 handle otherwise.

extern "C" {
void* MEMFindContainHeap(void* block);
void MEMFreeToExpHeap(void* heap, void* block);
}

namespace Memory {

extern void* physicalMEM1Handle;
extern void* physicalMEM2Handle;

void FreePhysicalMemory1(void* block);

}  // namespace Memory

void Memory::FreePhysicalMemory1(void* block) {
    if (MEMFindContainHeap(block) == physicalMEM2Handle) {
        MEMFreeToExpHeap(physicalMEM2Handle, block);
    } else {
        MEMFreeToExpHeap(physicalMEM1Handle, block);
    }
}
