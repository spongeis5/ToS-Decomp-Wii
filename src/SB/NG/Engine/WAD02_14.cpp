// WAD02_14 -- one of the 68 functions in this unit:
// Scaleform::HeapAllocatorWii's destructor, the compiler's own. The
// null-this test, the member at +208 destroyed with the don't-delete
// flag, then the BASE on `this` with the flag CLEAR, then operator
// delete when the caller's flag is positive, and `return this`. r4 = 0
// on the second call is what says base rather than member; r4 = -1 is a
// complete subobject.
//
// Both calls folded onto __dt__12hkBaseObjectFv, the eight-byte survivor
// every trivial destructor in the image collapsed onto, so neither the
// member's real type nor the base's is in the linked image; each is
// spelled as the object the branch reaches.
//
// The class's vtable lives elsewhere, so an undefined virtual is
// declared ahead of the destructor: the first non-inline virtual is
// where the compiler emits the vtable, and it must not be this unit.
//
// The other 67 functions of the unit are not written.

void operator delete(void* mem);

class hkBaseObject {
public:
    virtual ~hkBaseObject();
};

namespace Scaleform {

class HeapAllocatorWii : public hkBaseObject {
public:
    virtual void __key();
    ~HeapAllocatorWii();

    unsigned char _pad0[0xD0 - 0x4];
    hkBaseObject fD0;
};

}  // namespace Scaleform

Scaleform::HeapAllocatorWii::~HeapAllocatorWii() {}

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
class MovieLog : public GFxFSCommandHandler {
public:
    ~MovieLog();
};
}  // namespace Scaleform

Scaleform::MovieLog::~MovieLog() {}
