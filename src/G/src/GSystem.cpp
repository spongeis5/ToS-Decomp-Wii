// G/src/GSystem.cpp -- DRAFT, not yet in the tree.
//
// The second bootstrap unit, and the one that actually tests the small-data
// flag: its single function reaches a file static through the SDA base
// register, which is the thing cflags_gfx leaves enabled and cflags_game
// switches off.
//
// Read off tools/disasm.py --unit G/src/GSystem.cpp:
//
//   Init__7GSystemFRCQ211GMemoryHeap8HeapDescP9GSysAlloc   0x54 B
//       mr   r31,r4                 the allocator (second parameter)
//       mr   r30,r3                 the heap description (first)
//       lwz  r0,-23696(r13)         the static, read through r13
//       cmpwi r0,0 ; bne L1         done already? then nothing
//       mr   r3,r31 ; bl Init__11GMemoryHeapFP9GSysAlloc
//       mr   r3,r30 ; bl CreateRootHeap__11GMemoryHeapFRCQ211GMemoryHeap8HeapDesc
//       stw  r31,-23696(r13)        the allocator IS the flag
//   L1: epilogue
//
// Both callees take r3 and no `this`, so they are static members of
// GMemoryHeap, declared here and defined nowhere.
//
// The static is this unit's own -- the split gives G/src/GSystem.cpp a
// `.sbss` of 8 bytes at 0x8081BC90 -- so it is DEFINED here, per the rule
// that a unit's own statics must be defined in the unit or every function
// touching them reaches them through a separate relocation each. Only one
// word of that 8 is referenced by this function; the second may be another
// static this unit owns and nothing here reads, so the section size may
// come out at 4 rather than 8. That is data, not text, and unitcmp
// compares text.
//
// GMemoryHeap's layout IS in the DWARF dump (0x54 bytes, every member),
// but nothing here needs it: the type is only named through a reference
// and a nested class. HeapDesc is not in the dump, and WAD02_14 recovered
// it as seven unsigned longs with a constructor -- that shape is not
// needed here either, since the description is only passed along.

class GSysAlloc;

class GMemoryHeap {
public:
    class HeapDesc;

    static void Init(GSysAlloc* palloc);
    static void CreateRootHeap(const HeapDesc& desc);
};

class GSystem {
public:
    static void Init(const GMemoryHeap::HeapDesc& desc, GSysAlloc* palloc);
};

static GSysAlloc* pSysAlloc;

void GSystem::Init(const GMemoryHeap::HeapDesc& desc, GSysAlloc* palloc)
{
    if (pSysAlloc == 0) {
        GMemoryHeap::Init(palloc);
        GMemoryHeap::CreateRootHeap(desc);

        pSysAlloc = palloc;
    }
}
