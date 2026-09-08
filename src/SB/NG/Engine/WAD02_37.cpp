// The accessor part below was written by tools/gen_accessors.py and
// is kept unchanged; UI::FontModule's destructor at the foot was
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

enum HK_MEMORY_CLASS { HK_MEMORY_CLASS_ = 0x7FFFFFFF };


// A base only in the sense that r3 reaches it unchanged:
// the branch is four bytes and names nothing else.
class hkPoolMemory {
public:
    void allocateChunkBatch(void** a0, int a1, int a2, HK_MEMORY_CLASS a3);
    void allocateRuntimeBlock(int a0, HK_MEMORY_CLASS a1);
    void deallocateChunkBatch(void** a0, int a1, int a2, HK_MEMORY_CLASS a3);
    void deallocateRuntimeBlock(void* a0, int a1, HK_MEMORY_CLASS a2);
    void freeRuntimeBlocks();
    void getAllocatedSize(int a0);
    void preAllocateRuntimeBlock(int a0, HK_MEMORY_CLASS a1);
    void provideRuntimeBlock(void* a0, int a1, HK_MEMORY_CLASS a2);
};



class GRendererWiiImpl {
public:
    void FillStyleDisable();
    void LineStyleDisable();
    void ReleaseResources();
    void Clear();

    unsigned char _pad0[0x148];
    int f148;
    unsigned char _pad1[0x8];
    int f154;
    unsigned char _pad2[0x7C];
    int f1D4;
    unsigned char _pad3[0x8];
    int f1E0;
};



class hkPoolMemoryHI : public hkPoolMemory {
public:
    void allocateChunkBatch(void** a0, int a1, int a2, HK_MEMORY_CLASS a3);
    void allocateRuntimeBlock(int a0, HK_MEMORY_CLASS a1);
    void deallocateChunkBatch(void** a0, int a1, int a2, HK_MEMORY_CLASS a3);
    void deallocateRuntimeBlock(void* a0, int a1, HK_MEMORY_CLASS a2);
    void freeRuntimeBlocks();
    void getAllocatedSize(int a0);
    void preAllocateRuntimeBlock(int a0, HK_MEMORY_CLASS a1);
    void provideRuntimeBlock(void* a0, int a1, HK_MEMORY_CLASS a2);

};


namespace Loader {

class DomainDirMemCB {
public:
    virtual void __vtable_anchor();
    DomainDirMemCB();

};

}  // namespace Loader

namespace TRC {

class WiiDVDErrorRunnable {
public:
    virtual void __vtable_anchor();
    WiiDVDErrorRunnable();

};

}  // namespace TRC

void GRendererWiiImpl::LineStyleDisable() { f1D4 = 0; f1E0 = 0; }
void GRendererWiiImpl::FillStyleDisable() { f148 = 0; f154 = 0; }
void GRendererWiiImpl::ReleaseResources() { Clear(); }
void hkPoolMemoryHI::freeRuntimeBlocks() { hkPoolMemory::freeRuntimeBlocks(); }
void hkPoolMemoryHI::provideRuntimeBlock(void* a0, int a1, HK_MEMORY_CLASS a2) { hkPoolMemory::provideRuntimeBlock(a0, a1, a2); }
void hkPoolMemoryHI::deallocateRuntimeBlock(void* a0, int a1, HK_MEMORY_CLASS a2) { hkPoolMemory::deallocateRuntimeBlock(a0, a1, a2); }
void hkPoolMemoryHI::allocateRuntimeBlock(int a0, HK_MEMORY_CLASS a1) { hkPoolMemory::allocateRuntimeBlock(a0, a1); }
void hkPoolMemoryHI::preAllocateRuntimeBlock(int a0, HK_MEMORY_CLASS a1) { hkPoolMemory::preAllocateRuntimeBlock(a0, a1); }
void hkPoolMemoryHI::getAllocatedSize(int a0) { hkPoolMemory::getAllocatedSize(a0); }
void hkPoolMemoryHI::deallocateChunkBatch(void** a0, int a1, int a2, HK_MEMORY_CLASS a3) { hkPoolMemory::deallocateChunkBatch(a0, a1, a2, a3); }
void hkPoolMemoryHI::allocateChunkBatch(void** a0, int a1, int a2, HK_MEMORY_CLASS a3) { hkPoolMemory::allocateChunkBatch(a0, a1, a2, a3); }
Loader::DomainDirMemCB::DomainDirMemCB() {}
TRC::WiiDVDErrorRunnable::WiiDVDErrorRunnable() {}


void operator delete(void* mem);

class hkBaseObject {
public:
    virtual ~hkBaseObject();
};

// The compiler's own destructor: the null-this test, the member at the
// offset below destroyed with the don't-delete flag, and operator
// delete when the CALLER's flag is positive, then `return this`. There
// is no second call, so nothing it derives from has a destructor.
//
// __dt__12hkBaseObjectFv is 64 bytes of null test, conditional operator
// delete and `return this` -- what a destructor with nothing to destroy
// compiles to -- so every trivial destructor in the image folded onto
// it and the member's real type is gone with the fold. Spelling the
// member as hkBaseObject is not a claim about what it was; it is what
// makes the relocation name the symbol that survived.
namespace UI {

class FontModule {
public:
    ~FontModule();

    unsigned char _pad0[0x9C];
    hkBaseObject f9C;
};

}  // namespace UI

UI::FontModule::~FontModule() {}

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

namespace Scaleform {
class CustomTranslator : public GFxFSCommandHandler {
public:
    ~CustomTranslator();
};
}  // namespace Scaleform

namespace Scaleform {
class CustomImageCreator : public GFxFSCommandHandler {
public:
    ~CustomImageCreator();
};
}  // namespace Scaleform

namespace Scaleform {
class CustomCommandHandler : public GFxFSCommandHandler {
public:
    ~CustomCommandHandler();
};
}  // namespace Scaleform

namespace Scaleform {
class ExternalInterfaceHandler : public GFxFSCommandHandler {
public:
    ~ExternalInterfaceHandler();
};
}  // namespace Scaleform

namespace Scaleform {
class GSFMemoryFile : public GNewOverrideBase {
public:
    ~GSFMemoryFile();
};
}  // namespace Scaleform

namespace Scaleform {
class GTextureMemoryFile : public Scaleform::GSFMemoryFile {
public:
    ~GTextureMemoryFile();
};
}  // namespace Scaleform

Scaleform::CustomTranslator::~CustomTranslator() {}

Scaleform::CustomImageCreator::~CustomImageCreator() {}

Scaleform::CustomCommandHandler::~CustomCommandHandler() {}

Scaleform::ExternalInterfaceHandler::~ExternalInterfaceHandler() {}

Scaleform::GTextureMemoryFile::~GTextureMemoryFile() {}
