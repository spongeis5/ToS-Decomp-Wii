// The accessor at the top was written by tools/gen_accessors.py and is
// kept unchanged; the destructor at the foot was added by hand, so the
// generator's banner is gone -- gen_units.py overwrites any file that
// still carries it.
//
// World::Basic3DEntity's destructor is the compiler's own: the
// null-this test, the member at +108 destroyed with the don't-delete
// flag, then the BASE on `this` with the flag CLEAR, then operator
// delete when the caller's flag is positive, and `return this`. The
// second call is what says base rather than member: r4 = 0 is a base
// subobject where r4 = -1 is a complete one.
//
// Both calls folded onto __dt__12hkBaseObjectFv, the eight-byte
// survivor every trivial destructor in the image collapsed onto, so
// what the member really is is not in the linked image; it is spelled
// as the object the branch reaches, and the base as the direct base.
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

class Basic3DEntity : public hkBaseObject {
public:
    virtual void __key();
    ~Basic3DEntity();

    void Deactivate();
    void DeferDestroy();

    unsigned char _pad0[0x6C - 0x4];
    hkBaseObject f6C;
};

}  // namespace World

void World::Basic3DEntity::Deactivate() { DeferDestroy(); }

World::Basic3DEntity::~Basic3DEntity() {}
