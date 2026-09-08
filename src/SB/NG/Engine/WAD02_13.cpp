// The accessor part below was written by tools/gen_accessors.py and
// is kept unchanged; the TextParams destructor at the foot was
// added by hand, so the generator's banner is gone -- gen_units.py
// overwrites any file that still carries it.
//
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

// Each class below stands in for one a member points at.
// Nothing NAMES that class -- these five words carry no
// relocation -- so it is named after where it was found,
// and holds virtuals only up to the slot that is called.
class hkpMoppBvTreeShape_m34 {
public:
    virtual void _v0() const;
    virtual void _v1() const;
    virtual void _v2() const;
    virtual void _v3() const;
    virtual void _v4() const;
};


class hkpMoppBvTreeShape {
public:
    void getContainer() const;

    unsigned char _pad0[0x34];
    int f34;
};


namespace Scaleform {

class CustomTextRenderer {
public:
    void SetTextChanged();

    unsigned char _pad0[0x4688];
    unsigned char f4688;
};

}  // namespace Scaleform

void hkpMoppBvTreeShape::getContainer() const { ((hkpMoppBvTreeShape_m34*)f34)->_v4(); }
void Scaleform::CustomTextRenderer::SetTextChanged() { f4688 = 1; }

void operator delete(void* mem);

// Scaleform's own string, whose destructor is a real symbol in another
// unit: declared here, not defined.
class GString {
public:
    ~GString();
};

// The compiler's own destructor: the null-this test, the member at the
// offset below destroyed with the don't-delete flag, and operator
// delete when the CALLER's flag is positive, then `return this`. There
// is no second call, so nothing it derives from has a destructor.
class GFxDrawTextManager {
public:
    class TextParams {
    public:
        ~TextParams();

        unsigned char _pad0[0x14];
        GString text;
    };
};

GFxDrawTextManager::TextParams::~TextParams() {}

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

class GFxFSCommandHandler : public GNewOverrideBase {
public:
    ~GFxFSCommandHandler();
};

class GFxFileOpener : public GFxFSCommandHandler {
public:
    ~GFxFileOpener();
};

GFxFileOpener::~GFxFileOpener() {}

template <class T, int N>
class GRefCountBase : public GNewOverrideBase {
public:
    ~GRefCountBase();
};

class GRefCountBaseImpl : public GNewOverrideBase {
public:
    ~GRefCountBaseImpl();
};

template <int N>
class GRefCountBaseStatImpl : public GRefCountBaseImpl {
public:
    ~GRefCountBaseStatImpl();
};

class GFxState : public GRefCountBase<GFxState, 2> {
public:
    ~GFxState();
};

class GFile : public GRefCountBase<GFxState, 2> {
public:
    ~GFile();
};

GFxState::~GFxState() {}

GFile::~GFile() {}

template <> GRefCountBaseStatImpl<2>::~GRefCountBaseStatImpl() {}
