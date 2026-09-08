// The accessor at the top was written by tools/gen_accessors.py and is
// kept unchanged; the destructor at the foot was added by hand, so the
// generator's banner is gone -- gen_units.py overwrites any file that
// still carries it.
//
// World::xOGEntity's destructor is the compiler's own: the null-this
// test, the model handle at +52 destroyed with the don't-delete flag,
// then the BASE on `this` with the flag CLEAR, then operator delete
// when the caller's flag is positive, and `return this`. The second
// call is what says base rather than member: r4 = 0 is a base
// subobject where r4 = -1 is a complete one.
//
// The member's destructor is xOGModelHandle's own and is a real symbol;
// the base's folded onto __dt__12hkBaseObjectFv, the eight-byte
// survivor every trivial destructor in the image collapsed onto, so the
// base is spelled as the object the branch reaches.
//
// The class's vtable lives in the WAD00 blob, so an undefined virtual
// is declared ahead of the destructor: the first non-inline virtual is
// where the compiler emits the vtable, and it must not be this unit.

void operator delete(void* mem);

class hkBaseObject {
public:
    virtual ~hkBaseObject();
};

namespace World {

// Size not known and not needed: it is the last member the destructor
// touches, and nothing here reads past it.
class xOGModelHandle {
public:
    ~xOGModelHandle();
};

class ModelPrototypeEntity {
public:
    int* GetModelPrototype();

    unsigned char _pad0[0x20];
    int f20;
};

class xOGEntity : public hkBaseObject {
public:
    virtual void __key();
    ~xOGEntity();

    unsigned char _pad0[0x34 - 0x4];
    xOGModelHandle model;
};

}  // namespace World

int* World::ModelPrototypeEntity::GetModelPrototype() { return &f20; }

World::xOGEntity::~xOGEntity() {}

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

class xEffectAttachIntf : public World::xOGEntity {
public:
    ~xEffectAttachIntf();
};

class xEnt {
public:
    ~xEnt();
};

class zEnt : public xEnt {
public:
    ~zEnt();
};

xEffectAttachIntf::~xEffectAttachIntf() {}

zEnt::~zEnt() {}
