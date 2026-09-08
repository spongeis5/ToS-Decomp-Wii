// The accessor part below was written by tools/gen_accessors.py and
// is kept unchanged; the destructor at the foot was added by hand,
// so the generator's banner is gone -- gen_units.py overwrites any
// file that still carries it, and this one must not be overwritten.
//

// __dt__12hkBaseObjectFv is the trivial destructor body every
// trivial destructor in the image folded onto, so a member's real
// type is gone with the fold and it is spelled as the surviving
// name -- which is what makes the relocation name retail's symbol.
void operator delete(void* mem);

class hkBaseObject {
public:
    virtual ~hkBaseObject();
};
// Every function here touches nothing but its own members or a
// constant: one load, one store, the address of a member, a
// constant return, or members set to one constant. Each body was
// decoded from the image and re-encoded back to the same bytes,
// and each parameter list re-mangled back to the same symbol,
// before being written. GENERATED, not read: real matched
// functions whose offsets are recovered fact, but a count of them
// is not a count of decompiled code.
//
// Members are non-virtual, and the padding is padding -- only the
// offsets each function touches are known, not the fields between.


class GTextureWiiImpl {
public:
    int GetRenderer() const;

    unsigned char _pad0[0x1C];
    int f1C;
};


namespace System {

class CoreJobProcessor {
public:
    virtual void __vtable_anchor();
    CoreJobProcessor();

    class Slot {
    public:
        ~Slot();

        unsigned char _pad0[0xC];
        hkBaseObject mC;
    };
};

}  // namespace System

int GTextureWiiImpl::GetRenderer() const { return f1C; }
System::CoreJobProcessor::CoreJobProcessor() {}

System::CoreJobProcessor::Slot::~Slot() {}

// The 80-byte base-only destructor, the compiler's own: the
// null-this test, the BASE's destructor on `this` with the flag
// CLEAR, then the delete when the CALLER's flag is positive, and
// `return this`.
//
// A GFx class DELETES THROUGH ITS OWN OPERATOR. Retail's branch here
// is Free__11GMemoryHeapFPv and not the global __dl__FPv, which is
// what reloc_audit caught when these were first written the ordinary
// way: every word was equal, report.json credited them, and the call
// went somewhere else. A one-line `operator delete` is taken by
// -inline auto at the call site, so the heap's Free lands in the
// destructor itself, which is what the bytes have.

class GMemoryHeap {
public:
    static void Free(void* p);
};

class GNewOverrideBase {
public:
    static void operator delete(void* p) { GMemoryHeap::Free(p); }
};

class GTextureImplNode : public GNewOverrideBase {
public:
    ~GTextureImplNode();
};

class GTextureWii : public GTextureImplNode {
public:
    ~GTextureWii();
};

GTextureWii::~GTextureWii() {}

template <class T, int N>
class GRefCountBase : public GNewOverrideBase {
public:
    ~GRefCountBase();
};

class GFxState;

class GRenderer : public GRefCountBase<GFxState, 2> {
public:
    ~GRenderer();
};

GRenderer::~GRenderer() {}
