// BuildMemory.cpp -- six functions, read from the image with
// tools/disasm.py. Graphics::BuildMemoryModule is a System::Module that
// owns the render-side geometry build buffers. Its constructor sets the
// first event stage and clears the swap-suppression flag; GetPriority
// hands back a four-entry relative-priority table held as a
// function-local static, one pair of entries against the display module
// and one against the scene-graph module. RenderStartup records the
// buffer sizes, allocates two vertex buffers of 0x160000 bytes and two
// index buffers of 0x40000 from the secondary physical pool, clears the
// current-buffer block and swaps once; RenderShutdown marks the block
// inactive, frees all four and nulls the pointers. RenderFrameEnd swaps
// unless the flag suppresses it. Swap is the double-buffer step of the
// build buffer itself: advance the index, wrap it at two, point the
// current block at that buffer pair, zero the four counters and mark it
// active.
//
// Layouts from the DWARF (tools/dwarf_types.py): System::Module is 0x98
// -- name at 0, an EventSet of four ints at 4, the vptr at 0x14 where
// the constructor stores it, contextFlags at 0x18, sixty event-binding
// shorts at 0x1C and `enabled` at 0x94 -- and BuildMemoryModule is 0x98
// as well, its one bool sitting at 0x95 INSIDE the base's tail padding,
// which is the offset RenderFrameEnd reads. System::RelativePriority is
// 0xC (order, associate, events) and PriorityOrder is BEFORE 0 / AFTER
// 1. BuildBuffer is 0x3C -- two vertex pointers, two index pointers,
// the two maxima, the init flags, and a BufferStruct of 0x20 at 0x1C
// whose active/buf/vertexMem/indexMem/... offsets are every store Swap
// makes; the memset of 32 bytes at +0x1C is that whole sub-object.
//
// Four shapes the bytes fixed.
//
// The priority table's constant fields are already in the image
// (1/128, 0/256, 0/4, 1/8) and only the two module pointers are stored
// at run time, under a guard byte: that is a function-local static
// array whose initialisers are not all constants, so mwcc initialises
// what it can statically and emits stores for the rest. The four stores
// are two loads and four displacements off one base, 4/16/28/40, which
// is the 12-byte stride of RelativePriority.
//
// The buffer sizes are LITERALS at both use sites. Retail materialises
// 0x160000 once for the member store and again inside the loop rather
// than reading the member back, and the index buffer is 0x40000 where
// the member holds 0x20000 -- two bytes an index -- so the allocation
// sizes are written out and not derived from the fields.
//
// The frees pass 1 as the size and 0 as the pool where the allocations
// passed 32 as the alignment and 1 as the pool. That is what the bytes
// say and it is reproduced rather than corrected.
//
// RenderFrameEnd's flag test is `bnelr` and the swap is a tail call `b`
// with a trailing unreachable `blr` behind it: the branch over an if
// whose body ends the function is folded into a conditional return, and
// the epilogue stays where it was emitted.
//
// NEAR MISS -- RenderStartup 6 words of 41 and RenderShutdown 7 of 30,
// and in both it is ONE register pair. tools/fndiff.py aligns 35 of 41
// and 23 of 30; every differing word is the loop counter against the
// element pointer, which retail allocates r29/r28 in RenderStartup and
// r28/r27 in RenderShutdown where ours has them the other way round.
// The buffer base, the byte offset, the hoisted zero, the schedule and
// every displacement agree. Texture.cpp beside this one records the
// same symptom in the same direction -- retail's loop counter takes the
// HIGHER callee-saved register of the pair and ours the lower -- so it
// is one mechanism, not two, and neither file has reached it.
//
// What was tried here, none of it moving a word: the counter declared
// outside the loop, initialised there, pre-incremented, `!= 2` instead
// of `< 2`, a `while`, a `do/while` (which costs 2 words in
// RenderStartup), `register int`, `long`, a `const int` bound, and
// `short` (which is much worse); the buffer taken through a local
// reference (11 and 15) or a pointer to each array (10 and 13); the
// allocation results through named locals (12 and 23); the two nulls
// interleaved with the frees (9); the index buffer stored before the
// vertex buffer (8); and pointer arithmetic instead of subscripts.
//
// One thing measured and NOT used. tools/dwarf_lines.py puts every
// instruction of RenderStartup on line 155 and every instruction of
// RenderShutdown on line 219, with one lexical block each: this
// producer keeps the CALLER's line for inlined code, so retail's
// bodies read as one call apiece into the buffer's own Init and Term.
// Written that way -- template members of BuildBuffer, with
// `#pragma always_inline on` -- mwcc declines to inline either and
// emits them as two extra functions with a six-word RenderStartup, so
// the flags this project builds with cannot reproduce that reading and
// the bodies stay written out. A macro would give the same line table.

extern "C" void* memset(void* dest, int value, unsigned long count);

namespace System {

class EventSet {
public:
    int stage[4];
};

class Module;

enum PriorityOrder { PRIORITY_BEFORE = 0, PRIORITY_AFTER = 1 };

class RelativePriority {
public:
    PriorityOrder order;
    Module* associate;
    int events;
};

// The vptr follows the name and the event set (+0x14), where the
// constructor stores it. Slot 0 is left undefined here.
class Module {
public:
    Module();

    char* name;
    EventSet events;

    virtual void _v0();
    virtual void _v1();
    virtual const RelativePriority* GetPriority(int& count) const;

    int contextFlags;
    short eventBindingIndices[60];
    bool enabled;
};

}  // namespace System

namespace Memory {

enum PhysicalMemoryPool { PHYS_MEM_MAIN = 0, PHYS_MEM_SECONDARY = 1 };

int AllocPhysicalMemory(int size, int align, PhysicalMemoryPool pool);
void FreePhysicalMemory(void* block, int size, PhysicalMemoryPool pool);

}  // namespace Memory

namespace Graphics {

extern System::Module* displayModule;
extern System::Module* sceneGraphModule;

class BufferStruct {
public:
    int active;
    int buf;
    unsigned char* vertexMem;
    unsigned short* indexMem;
    int vertexSize;
    int indexCount;
    int vertexSizeUpload;
    int indexCountUpload;
};

template <int NUM_BUFFERS, bool DOUBLE_BUFFERED>
class BuildBuffer {
public:
    void Swap();

    unsigned char* vertexMems[NUM_BUFFERS];
    unsigned short* indexMems[NUM_BUFFERS];
    int vertexMaxSize;
    int indexMaxCount;
    int initFlags;
    BufferStruct current;
};

template <int NUM_BUFFERS, bool DOUBLE_BUFFERED>
void BuildBuffer<NUM_BUFFERS, DOUBLE_BUFFERED>::Swap() {
    current.buf++;

    if (current.buf >= NUM_BUFFERS) {
        current.buf = 0;
    }

    current.vertexMem = vertexMems[current.buf];
    current.indexMem = indexMems[current.buf];
    current.vertexSize = 0;
    current.indexCount = 0;
    current.vertexSizeUpload = 0;
    current.indexCountUpload = 0;
    current.active = 1;
}

extern BuildBuffer<2, true> g_renderBuildBuffer;

class BuildMemoryModule : public System::Module {
public:
    BuildMemoryModule();

    // Declared first, undefined here: the vtable's home is not this unit.
    virtual void _v1();
    virtual const System::RelativePriority* GetPriority(int& count) const;
    virtual void RenderStartup(int stage);
    virtual void RenderShutdown(int stage);
    virtual void RenderFrameEnd(int stage);

    bool hackSuppressRenderSwap;
};

}  // namespace Graphics

Graphics::BuildMemoryModule::BuildMemoryModule() {
    events.stage[0] = 1408;
    hackSuppressRenderSwap = false;
}

const System::RelativePriority* Graphics::BuildMemoryModule::GetPriority(
    int& count) const {
    static System::RelativePriority priority[4] = {
        { System::PRIORITY_AFTER, displayModule, 128 },
        { System::PRIORITY_BEFORE, displayModule, 256 },
        { System::PRIORITY_BEFORE, sceneGraphModule, 4 },
        { System::PRIORITY_AFTER, sceneGraphModule, 8 },
    };

    count = 4;

    return priority;
}

void Graphics::BuildMemoryModule::RenderStartup(int) {
    g_renderBuildBuffer.vertexMaxSize = 0x160000;
    g_renderBuildBuffer.indexMaxCount = 0x20000;
    g_renderBuildBuffer.initFlags = 0;

    for (int i = 0; i < 2; i++) {
        g_renderBuildBuffer.vertexMems[i] =
            (unsigned char*)Memory::AllocPhysicalMemory(
                0x160000, 32, Memory::PHYS_MEM_SECONDARY);
        g_renderBuildBuffer.indexMems[i] =
            (unsigned short*)Memory::AllocPhysicalMemory(
                0x40000, 32, Memory::PHYS_MEM_SECONDARY);
    }

    memset(&g_renderBuildBuffer.current, 0, sizeof(BufferStruct));

    g_renderBuildBuffer.Swap();
}

void Graphics::BuildMemoryModule::RenderShutdown(int) {
    g_renderBuildBuffer.current.active = 0;

    for (int i = 0; i < 2; i++) {
        Memory::FreePhysicalMemory(g_renderBuildBuffer.vertexMems[i], 1,
                                   Memory::PHYS_MEM_MAIN);
        Memory::FreePhysicalMemory(g_renderBuildBuffer.indexMems[i], 1,
                                   Memory::PHYS_MEM_MAIN);

        g_renderBuildBuffer.vertexMems[i] = 0;
        g_renderBuildBuffer.indexMems[i] = 0;
    }
}

void Graphics::BuildMemoryModule::RenderFrameEnd(int) {
    if (!hackSuppressRenderSwap) {
        g_renderBuildBuffer.Swap();
    }
}
