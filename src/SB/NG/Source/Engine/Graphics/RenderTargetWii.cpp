// RenderTargetWii.cpp -- one of the 29 functions in this unit:
// Graphics::TextureRenderTargetCommon's destructor, the compiler's own.
// The null-this test, the member at +20 destroyed with the don't-delete
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
// The other 28 functions of the unit are not written.

void operator delete(void* mem);

class hkBaseObject {
public:
    virtual ~hkBaseObject();
};

namespace Graphics {

class TextureRenderTargetCommon : public hkBaseObject {
public:
    virtual void __key();
    ~TextureRenderTargetCommon();

    unsigned char _pad0[0x14 - 0x4];
    hkBaseObject f14;
};

}  // namespace Graphics

Graphics::TextureRenderTargetCommon::~TextureRenderTargetCommon() {}

// The 80-byte base-only destructor, the compiler's own: the
// null-this test, the BASE's destructor on `this` with the flag
// CLEAR -- r4 = 0 is a base subobject where r4 = -1 is a complete
// one -- then operator delete when the CALLER's flag is positive,
// and `return this`. No member is destroyed, so the class needs
// nothing but its base.
//
// Non-virtual throughout: the call is a direct `bl` either way, and
// a virtual destructor would make this unit the home of a vtable
// retail keeps elsewhere. Where the base's destructor folded onto
// __dt__12hkBaseObjectFv -- 64 bytes of null test, conditional
// operator delete and `return this` -- the base is spelled as the
// symbol that survived, which is what makes the relocation reach
// retail's own.
void operator delete(void* mem);

namespace Graphics {
class ScreenRenderTargetCommon : public hkBaseObject {
public:
    ~ScreenRenderTargetCommon();
};
}  // namespace Graphics

Graphics::ScreenRenderTargetCommon::~ScreenRenderTargetCommon() {}
