// The accessor at the top was written by tools/gen_accessors.py and is
// kept unchanged; the two destructors at the foot were added by hand, so
// the generator's banner is gone -- gen_units.py overwrites any file
// that still carries it.
//
// Both are the compiler's own: the null-this test, one member destroyed
// with the don't-delete flag, then the member at +0 with the same flag,
// then operator delete when the CALLER's flag is positive, and `return
// this`. Neither class has a base whose destructor runs -- both calls
// pass r4 = -1, which is a complete subobject, where a base takes r4 = 0
// -- so the two are members and nothing is inherited.
//
// zViewport's pair both folded onto __dt__12hkBaseObjectFv, the
// eight-byte survivor every trivial destructor in the image collapsed
// onto, so what the two members really are is not in the linked image;
// they are spelled as the object the branch reaches. zNGLoadingScreen's
// second is xLightEffectFlicker's own, which is a real symbol.
//
// The padding is padding: only the offsets the destructors touch are
// known -- +0 and +96 in one, +0 and +732 in the other -- and members
// are destroyed in reverse declaration order, which is why the far one
// is written last in each.

void operator delete(void* mem);

class hkBaseObject {
public:
    virtual ~hkBaseObject();
};

// Size not known and not needed: it is the last member, and nothing
// here reads past it.
class xLightEffectFlicker {
public:
    ~xLightEffectFlicker();
};


class GestureData {
public:
    GestureData();

    int f0;
};


class zNGLoadingScreen {
public:
    ~zNGLoadingScreen();

    hkBaseObject f0;
    unsigned char _pad0[0x60 - 0x4];
    xLightEffectFlicker flicker;
};

class zViewport {
public:
    ~zViewport();

    hkBaseObject f0;
    unsigned char _pad0[0x2DC - 0x4];
    hkBaseObject f2DC;
};


GestureData::GestureData() { f0 = 0; }

zNGLoadingScreen::~zNGLoadingScreen() {}

zViewport::~zViewport() {}
