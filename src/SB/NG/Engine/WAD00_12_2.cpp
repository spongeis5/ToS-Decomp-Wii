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

namespace World {

class TextureResourceEntity {
public:
    void Deactivate();
    void DeferDestroy();
    ~TextureResourceEntity();

    unsigned char _pad0[0x20];
    hkBaseObject m20;
};

}  // namespace World

namespace World {

class VertexDeclEntity {
public:
    void Deactivate();
    void DeferDestroy();
    ~VertexDeclEntity();

    unsigned char _pad0[0x18];
    hkBaseObject m18;
};

}  // namespace World

namespace Graphics {

// Its own destructor is a real symbol and lives in another unit;
// declared here, not defined.
class StaticBuilder {
public:
    ~StaticBuilder();
};

}  // namespace Graphics

namespace World {

// The third destructor in this unit and the only one with a BASE: the
// member at +64 is destroyed with the don't-delete flag, then the base
// on `this` with the flag CLEAR. r4 = 0 is a base subobject where
// r4 = -1 is a complete one, and that one bit is the whole of the
// difference from the two above.
//
// The class's vtable lives in the WAD00 blob, so an undefined virtual
// is declared ahead of the destructor: the first non-inline virtual is
// where the compiler emits the vtable, and it must not be this unit.
class StaticGeometryEntity : public hkBaseObject {
public:
    virtual void __key();
    ~StaticGeometryEntity();

    unsigned char _pad0[0x40 - 0x4];
    Graphics::StaticBuilder builder;
};

}  // namespace World

void World::TextureResourceEntity::Deactivate() { DeferDestroy(); }
void World::VertexDeclEntity::Deactivate() { DeferDestroy(); }

World::VertexDeclEntity::~VertexDeclEntity() {}

World::TextureResourceEntity::~TextureResourceEntity() {}

World::StaticGeometryEntity::~StaticGeometryEntity() {}
