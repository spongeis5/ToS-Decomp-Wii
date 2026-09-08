// ImmediateGeometryEntity.cpp -- one function of the unit:
// World::ImmediateGeometry's destructor, which is also the one
// StaticGeometryEntity.cpp's Graphics::StaticBuilder destructor calls on
// its member at +16. The rest of the unit is not written.
//
// The 80-byte base-only destructor, the compiler's own: the null-this
// test, the BASE's destructor on `this` with the flag CLEAR -- r4 = 0 is
// a base subobject where r4 = -1 is a complete one -- then operator
// delete when the CALLER's flag is positive, and `return this`. No
// member is destroyed, so the class needs nothing but its base.
//
// Non-virtual throughout: the call is a direct `bl` either way, and a
// virtual destructor would make this unit the home of a vtable retail
// keeps elsewhere. The base's own folded onto __dt__12hkBaseObjectFv --
// 64 bytes of null test, conditional operator delete and `return this`,
// which is what a destructor with nothing to destroy compiles to -- so
// it is spelled as the symbol that survived, and that is what makes the
// relocation reach retail's own.

void operator delete(void* mem);

class hkBaseObject {
public:
    ~hkBaseObject();
};

namespace World {

class ImmediateGeometry : public hkBaseObject {
public:
    ~ImmediateGeometry();
};

}  // namespace World

World::ImmediateGeometry::~ImmediateGeometry() {}
