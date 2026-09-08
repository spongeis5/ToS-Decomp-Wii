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
    class EventHandler;

    ~GRenderer();
};

GRenderer::~GRenderer() {}

// The two GArray destructors, read out of their mangled names:
// GArray<T, N, Policy> derives from
// GArrayBase<GArrayData<T, GAllocatorGH<T, N>, Policy> >, whose
// destructor is declared and never defined -- which is what makes
// these the 80-byte shape that destroys a base rather than the
// 64-byte one that destroys nothing.
class GTexture {
public:
    class ChangeHandler;
};

class GArrayDefaultPolicy;

template <class T, int N>
class GAllocatorGH;

template <class T, class A, class P>
class GArrayData;

// Through GNewOverrideBase, so the delete reaches GMemoryHeap::Free
// and not the global operator: the call is Free__11GMemoryHeapFPv,
// which -inline auto takes at the call site.
template <class D>
class GArrayBase : public GNewOverrideBase {
public:
    ~GArrayBase();
};

template <class T, int N, class P>
class GArray : public GArrayBase<GArrayData<T, GAllocatorGH<T, N>, P> > {
public:
    ~GArray();
};

template <class T, int N, class P>
GArray<T, N, P>::~GArray() {}

template class GArray<GTexture::ChangeHandler*, 2, GArrayDefaultPolicy>;
template class GArray<GRenderer::EventHandler*, 2, GArrayDefaultPolicy>;
