// The accessor part below was written by tools/gen_accessors.py and
// is kept unchanged; xDecal::decal_instance's destructor at the foot was
// added by hand, so the generator's banner is gone -- gen_units.py
// overwrites any file that still carries it.
//
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

void operator delete(void* mem);

namespace World {

class RefData;

// Layout from the DWARF, as xOGModelRefPtr.cpp records it: one word,
// the RefData the weak pointer holds. Its default constructor is what
// the decal's own constructor emits -- `li r0,0; stw r0,84(r3)` and
// nothing else -- and its destructor is the one the decal's calls.
class xOGModelRefPtr {
public:
    xOGModelRefPtr() : mData(0) {}
    ~xOGModelRefPtr();

    RefData* mData;
};

}  // namespace World

namespace xDecal {

// The compiler's own destructor: the null-this test, the member at the
// offset below destroyed with the don't-delete flag, and operator
// delete when the CALLER's flag is positive, then `return this`. There
// is no second call, so nothing it derives from has a destructor.
class decal_instance {
public:
    decal_instance();
    ~decal_instance();

    unsigned char _pad0[0x54];
    World::xOGModelRefPtr model;
};

}  // namespace xDecal

namespace Graphics {

class Geometry {
public:
    int GetVertexCount() const;

    unsigned char _pad0[0x60];
    int f60;
};

}  // namespace Graphics

namespace Graphics {

class StaticBuilder {
public:
    int* GetCollTree();
    int GetFixedVertexBuffers() const;
    int* GetGeometry();

    unsigned char _pad0[0x10];
    int f10;
    unsigned char _pad1[0xB8];
    int fCC;
    unsigned char _pad2[0x4];
    int fD4;
};

}  // namespace Graphics

namespace Graphics {

class FixedVertexBuffer {
public:
    int GetReadBuffer() const;

    unsigned char _pad0[0xC];
    int fC;
};

}  // namespace Graphics

xDecal::decal_instance::decal_instance() {}

xDecal::decal_instance::~decal_instance() {}

int Graphics::Geometry::GetVertexCount() const { return f60; }
int Graphics::StaticBuilder::GetFixedVertexBuffers() const { return fCC; }
int Graphics::FixedVertexBuffer::GetReadBuffer() const { return fC; }
int* Graphics::StaticBuilder::GetGeometry() { return &f10; }
int* Graphics::StaticBuilder::GetCollTree() { return &fD4; }
