// StaticGeometryEntity.cpp -- one of the six functions in this unit,
// and it is not an entity: Graphics::StaticBuilder's destructor sits at
// 801BD870 between the entity's constructor and the asset's Create,
// because that is where the compiler emitted the builder's implicit
// destructor -- the unit that first needed one. The builder itself is
// declared in WAD00_12_2.cpp, where StaticGeometryEntity holds one at
// +64 and destroys it.
//
// The body is the compiler's own: the null-this test, the member at +16
// destroyed with the don't-delete flag, then the BASE on `this` with the
// flag CLEAR, then operator delete when the caller's flag is positive,
// and `return this`. r4 = 0 on the second call is what says base rather
// than member; r4 = -1 is a complete subobject.
//
// The member's destructor is World::ImmediateGeometry's own and is a
// real symbol. The base's folded onto __dt__12hkBaseObjectFv, the
// eight-byte survivor every trivial destructor in the image collapsed
// onto, so the base is spelled as the object the branch reaches.
//
// The class's vtable lives in another unit, so an undefined virtual is
// declared ahead of the destructor: the first non-inline virtual is
// where the compiler emits the vtable, and it must not be this one.
//
// The other five functions of the unit are not written: the entity's
// constructor, the asset's Create, a NewArray instantiation, Deactivate
// and Destroy, 1,560 bytes between them.

void operator delete(void* mem);

class hkBaseObject {
public:
    virtual ~hkBaseObject();
};

namespace World {

// Size not known and not needed: it is the last member the destructor
// touches, and nothing here reads past it.
class ImmediateGeometry {
public:
    ~ImmediateGeometry();
};

}  // namespace World

namespace Graphics {

class StaticBuilder : public hkBaseObject {
public:
    virtual void __key();
    ~StaticBuilder();

    unsigned char _pad0[0x10 - 0x4];
    World::ImmediateGeometry geom;
};

}  // namespace Graphics

Graphics::StaticBuilder::~StaticBuilder() {}
