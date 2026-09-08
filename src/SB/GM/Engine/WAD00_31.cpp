// The accessor part below was written by tools/gen_accessors.py and
// is kept unchanged; the destructor at the foot was added by hand,
// so the generator's banner is gone -- gen_units.py overwrites any
// file that still carries it, and this one must not be overwritten.
//

// The compiler's own destructor: the null-this test, the member at
// the offset below destroyed with the don't-delete flag, and
// operator delete when the caller's flag is positive.
//
// __dt__12hkBaseObjectFv is the trivial destructor body every
// trivial destructor in the image folded onto, so the member's real
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

// Each class below stands in for one a member points at.
// Nothing NAMES that class -- these five words carry no
// relocation -- so it is named after where it was found,
// and holds virtuals only up to the slot that is called.
class xCamBlend_m148 {
public:
    virtual void _v0() const;
    virtual void _v1() const;
    virtual void _v2() const;
    virtual void _v3() const;
    virtual void _v4() const;
    virtual void _v5() const;
    virtual void _v6() const;
    virtual void _v7() const;
    virtual void _v8() const;
    virtual void _v9() const;
    virtual void _v10() const;
    virtual void _v11() const;
    virtual void _v12() const;
    virtual void _v13() const;
    virtual void _v14() const;
};
class xCamBlend_m14C {
public:
    virtual void _v0() const;
    virtual void _v1() const;
};


class xCamBlend {
public:
    void get_final_dest() const;
    void get_zCam2Player() const;

    ~xCamBlend();

    unsigned char _pad0[0x148];
    int f148;
    int f14C;
    unsigned char _pad1[0x4];
    hkBaseObject m154;
};


namespace Debug {

class QuaternionFormatter {
public:
    virtual void __vtable_anchor();
    QuaternionFormatter();

};

}  // namespace Debug


class bit_array_alloc {
public:
    bit_array_alloc();

    int f0;
    int f4;
    int f8;
};


void xCamBlend::get_final_dest() const { ((xCamBlend_m14C*)f14C)->_v1(); }
void xCamBlend::get_zCam2Player() const { ((xCamBlend_m148*)f148)->_v14(); }
Debug::QuaternionFormatter::QuaternionFormatter() {}
bit_array_alloc::bit_array_alloc() { f0 = 0; f4 = 0; f8 = 0; }

xCamBlend::~xCamBlend() {}
