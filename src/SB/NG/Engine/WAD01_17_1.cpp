// WAD01_17_1 -- two of the 34 functions in this unit, and they are the
// two smallest shapes it holds: IO::MediaHandler's destructor and
// IO::MediaModuleLFS's constructor. The other 32 are not written.

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
// System::Module, layout from the DWARF and transcribed in
// SystemCache.cpp: two members ahead of the first virtual, so the vptr
// of anything derived lands at +0x14, which is where the constructor's
// vtable store goes. `events` starts at +4, so the store at +4 is
// stage[0].
namespace System {

class EventSet {
public:
    int stage[4];
};

class Module {
public:
    Module();

    char* name;
    EventSet events;

    virtual void _v0();

    int contextFlags;
    short eventBindingIndices[60];
    bool enabled;
};

}  // namespace System

namespace IO {

class MediaHandler {
public:
    ~MediaHandler();

    unsigned char _pad0[0x8];
    hkBaseObject f8;
};

// The one module in this batch whose constant is the same as the donor
// HavokModule's, which is why the fill sheet shows no immediate hole
// for it at all -- only the vtable and the stage.
class MediaModuleLFS : public System::Module {
public:
    MediaModuleLFS();
};

}  // namespace IO

IO::MediaHandler::~MediaHandler() {}

IO::MediaModuleLFS::MediaModuleLFS() {
    events.stage[0] = 3;
}
