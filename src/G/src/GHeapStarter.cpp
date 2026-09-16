// G/src/GHeapStarter.cpp -- DRAFT, not yet in the tree.
//
// The first GFx unit, chosen because it proves code generation and nothing
// else: three functions, no statics, no float literals, no layout to
// recover. Two of the three are ONE instruction.
//
// Read off tools/disasm.py --unit G/src/GHeapStarter.cpp. The base class
// lives in its own unit (G/src/GHeapGranulator.cpp), so it is DECLARED here
// and never defined -- its constructor, Alloc and Free are external symbols
// the branches name, which is the same arrangement every SB unit in this
// tree already uses for the weak copies it calls.
//
//   __ct__12GHeapStarterFP9GSysAllocUlUl   0x40 B
//       mr   r7,r6          the third argument moves up to the fourth
//       mr   r0,r5
//       li   r5,256         the granulator's second argument is a literal
//       mr   r6,r0          the second moves up to the third
//       bl   __ct__15GHeapGranulatorFP9GSysAllocUlUlUl
//     so: GHeapGranulator(palloc, 256, granularity, reserve).
//
//   Alloc__12GHeapStarterFUlUl  0x4 B   b Alloc__15GHeapGranulatorFUlUl
//   Free__12GHeapStarterFPvUlUl 0x4 B   b Free__15GHeapGranulatorFPvUlUl
//     A four-byte function is a pure forward with the arguments untouched,
//     which is what a non-inline member calling the base with the same
//     arguments compiles to. r3 is unchanged across both branches, so the
//     base sits at offset 0 and no layout is needed.
//
// SETTLED: the class is not polymorphic. G/src/GHeapStarter.cpp's split
// declares .text ONLY (0x804FDBE0..0x804FDC40) -- no .data and no .rodata --
// so retail's object emits no vtable for it, and the plain spelling below is
// the right one. G/src/GFxLog.cpp by contrast carries 32 bytes of .data,
// which is where its multiple-inheritance vtables live; that is the shape to
// expect when a GFx unit does have virtuals.
//
// NOT settled, and deliberately not asserted below: the parameter NAMES. The
// DWARF type dump has no GHeapGranulator and no GHeapStarter -- it covers
// GSysAlloc and GSysAllocStatic and stops before this layer -- so
// minAlign / granularity / reserve are inferences from the call site. What
// is MEASURED is only the literal 256 and the order the arguments move in.

typedef unsigned long UPInt;

class GSysAlloc;

class GHeapGranulator {
public:
    GHeapGranulator(GSysAlloc* palloc, UPInt minAlign, UPInt granularity, UPInt reserve);

    void* Alloc(UPInt size, UPInt align);
    void Free(void* p, UPInt size, UPInt align);
};

class GHeapStarter : public GHeapGranulator {
public:
    GHeapStarter(GSysAlloc* palloc, UPInt granularity, UPInt reserve);

    void* Alloc(UPInt size, UPInt align);
    void Free(void* p, UPInt size, UPInt align);
};

GHeapStarter::GHeapStarter(GSysAlloc* palloc, UPInt granularity, UPInt reserve)
    : GHeapGranulator(palloc, 256, granularity, reserve)
{
}

void* GHeapStarter::Alloc(UPInt size, UPInt align)
{
    return GHeapGranulator::Alloc(size, align);
}

void GHeapStarter::Free(void* p, UPInt size, UPInt align)
{
    GHeapGranulator::Free(p, size, align);
}
