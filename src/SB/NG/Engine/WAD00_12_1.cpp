// WAD00_12_1 -- one function, read from the image with tools/disasm.py:
// World::SkinGeometryEntity's destructor, the compiler's own -- the
// null-this test, the skin builder member's destructor at +0x40 with
// the don't-delete flag, then the Havok base's destructor on `this`
// with the flag clear, and operator delete when the caller's flag says
// so. The Havok object is spelled as the direct base: a class between
// them gets a destructor of its own emitted and called, and retail
// calls the Havok one. The class's vtable lives in the WAD00 blob, so
// an undefined virtual is declared ahead of the destructor: the first
// non-inline virtual is where the compiler emits the vtable, and it
// must not be this unit.

void operator delete(void* mem);

class hkBaseObject {
public:
    virtual ~hkBaseObject();
};

namespace Graphics {

class SkinBuilder {
public:
    ~SkinBuilder();

    unsigned char _pad0[0x100];
};

}  // namespace Graphics

namespace World {

class SkinGeometryEntity : public hkBaseObject {
public:
    virtual void __key();
    ~SkinGeometryEntity();

    unsigned char _pad0[0x3C];
    Graphics::SkinBuilder builder;
};

}  // namespace World

World::SkinGeometryEntity::~SkinGeometryEntity() {}
