// WAD01_12_1 -- six functions, 808 bytes, read from the image with
// tools/disasm.py: four free functions of Math over the four-component
// vector, and the two setters of Graphics::Geometry.
//
//   MulProject     builds a Vector4 from the vector and a w of 1, runs it
//                  through the view matrix, divides by the resulting w and
//                  copies the four words out
//   Min / Max      componentwise, four calls each to Math::min / Math::max
//   Lerp           a * (1 - t) + b * t, four fmadds, no frame at all
//   Geometry::Create        eighteen member stores and one call
//   Geometry::SetBroadBound the bound by its out-of-line operator=, the
//                           sphere as four words, then degenerateBound
//
// LAYOUTS, from tools/dwarf_types.py -- the DWARF holds them under BARE
// names, so `--type Vector4` finds what `--type Math::Vector4` does not:
//
//   Vector4 0x10 is one member, a DataType of four floats; Vector 0x10
//   derives from Vector4 and adds nothing. That nesting is what makes a
//   copy a block move of WORDS rather than a memberwise lfs/stfs -- see
//   WAD01_15.cpp, where spelling them flat emitted three __as__ symbols
//   retail does not have.
//
//   BroadBound 0x20 is two Vectors, lower then upper.
//
//   Geometry 0xB4 (Geometry__B37C in the DWARF; Geometry__140FB5 is a
//   different, one-byte type) derives from Graphics::Node 0xC and lays
//   out bbound 0xC, boundSphere 0x2C, degenerateBound 0x3C, offDist 0x40,
//   vertLodDist 0x44, builder 0x48, effect 0x4C, material 0x50,
//   paramData 0x54, shaderContextList 0x58, primType 0x5C, vertexCount
//   0x60, indexLods[2] 0x64, indexLodCount 0x9C, vertexBuffers 0xA0,
//   vertexBufferCount 0xA4, sectorLight 0xA8, flags 0xAC, wiiDirect 0xAD,
//   wiiIndexStride 0xAE, wiiIndexSkin 0xAF, wiiGeomStateOp 0xB0.
//   IndexLOD 0x1C is an IndexBuffer 0xC then primCount, triCount,
//   indexCount, lodVertCount.
//
// THE THREE LITERALS ARE READ OUT OF THE IMAGE, not guessed: @96809 at
// 0x80692C18 is 0.0f, @96810 at 0x80692C1C is 1.0f, and @99291 at
// 0x80692D88 is 0x5F0AC723, which is 1.0e19f exactly.
//
// No unit-level padding header is needed here and none is used. The
// float-base rule NOTES.md records is that mwcc shares one `lis` for
// THREE or more literals under 32 KB of .rodata; no function here loads
// more than two, and retail spells a `lis` per literal, which is what the
// fragment gives on its own.
//
// EXACT: 7 of the 7 functions the object defines are byte-identical
// (`python tools/unitcmp.py SB/NG/Engine/WAD01_12_1`). Six are the unit's
// own 808 bytes; the seventh is
// `__as__Q28Graphics10BroadBoundFRCQ28Graphics10BroadBound`, the implicit
// operator= that `bbound = bound` emits OUT OF LINE and that retail also
// has -- it lives elsewhere in the WAD01 blob, and our copy is identical
// to it, 17 of 17 words.
//
// A LEVER, and it is new: A COMPONENT READ THROUGH AN INLINE ACCESSOR IS
// NOT THE SAME AS THE MEMBER READ, even though both emit one `lfs`.
// Written `min(a.data.x, b.data.x)`, mwcc emits the first argument's load
// first and then the second's; written `min(a.X(), b.X())` it emits the
// SECOND first, and -- from the second statement on -- it puts the next
// `a` load AHEAD of the previous statement's store, into f0, and follows
// it with `fmr f1,f0`. That is retail's shape exactly, three `fmr` and
// all. Min and Max went from 24 of 30 words wrong (ours 120 bytes against
// retail's 132) to exact, in one edit; MulProject's four `Assign`
// arguments came out z, y, x, constant instead of x, y, z, constant for
// the same reason and matched with the same change.
//
// AND THE BYTES SAY WHERE IT DOES NOT APPLY, which is why it is worth
// recording rather than applying everywhere. Lerp reads its components
// as plain members. Through the accessors it goes to 21 of 24 words
// wrong; through `a.data.x` it is exact. Lerp's operands are consumed by
// arithmetic operators, where mwcc already evaluates the right operand
// first, and the accessor's call node reorders that in the wrong
// direction. So the rule is: an accessor changes the order where the
// component is a CALL ARGUMENT, and changes it the other way where it is
// an operand. Both halves were measured, one compile each.

namespace Math {

class Vector4 {
public:
    class DataType {
    public:
        float x;
        float y;
        float z;
        float w;
    };

    void Assign(float x, float y, float z, float w);

    float X() const { return data.x; }
    float Y() const { return data.y; }
    float Z() const { return data.z; }
    float W() const { return data.w; }

    DataType data;
};

class Vector : public Vector4 {
};

float min(float a, float b);
float max(float a, float b);

}  // namespace Math

namespace Graphics {

class ViewMatrix;
class Builder;
class Material;
class VertexBuffer;
class Wii_GeometryStateOp;

class Shader {
public:
    class GeometryContext;
};

class Effect {
public:
    class ParamData;

    Wii_GeometryStateOp* GetWiiGeomStateOp() const;
};

enum PrimitiveType {
    PrimitiveType_0
};

class BroadBound {
public:
    Math::Vector lower;
    Math::Vector upper;
};

class Node {
public:
    enum NodeTypeEnum {
        NodeTypeEnum_0
    };

    void* __vptr;
    NodeTypeEnum type;
    bool attached;
    bool detachPending;
    bool updateThread;
};

class IndexBuffer {
public:
    unsigned char handle[0x8];
    int firstIndex;
};

class IndexLOD {
public:
    IndexBuffer indexBuffer;
    int primCount;
    int triCount;
    int indexCount;
    int lodVertCount;
};

class Geometry : public Node {
public:
    void Create(Builder* builder, const Effect* effect, const Material* material,
                Effect::ParamData* paramData, Shader::GeometryContext** shaderContextList,
                PrimitiveType primType, int vertexCount, int primCount, int triCount,
                int indexCount, VertexBuffer* vertexBuffers, int vertexBufferCount,
                unsigned int flags, unsigned int sectorLight);
    void SetBroadBound(const BroadBound& bound, const Math::Vector4& sphere);

    BroadBound bbound;
    Math::Vector4 boundSphere;
    bool degenerateBound;
    unsigned char _pad0[0x3];
    float offDist;
    float vertLodDist;
    Builder* builder;
    Effect* effect;
    Material* material;
    Effect::ParamData* paramData;
    Shader::GeometryContext** shaderContextList;
    PrimitiveType primType;
    int vertexCount;
    IndexLOD indexLods[2];
    int indexLodCount;
    VertexBuffer* vertexBuffers;
    int vertexBufferCount;
    unsigned int sectorLight;
    unsigned char flags;
    bool wiiDirect;
    unsigned char wiiIndexStride;
    unsigned char wiiIndexSkin;
    Wii_GeometryStateOp* wiiGeomStateOp;
};

}  // namespace Graphics

namespace Math {

void Mul(Vector4& out, const Vector4& v, const Graphics::ViewMatrix& m);
void Mul(Vector4& out, const Vector4& v, float s);

void MulProject(Vector& out, const Vector& v, const Graphics::ViewMatrix& m) {
    Vector t;

    t.Assign(v.X(), v.Y(), v.Z(), 1.0f);
    Mul(t, t, m);
    Mul(t, t, 1.0f / t.W());

    out = t;
}

void Min(Vector& out, const Vector& a, const Vector& b) {
    out.data.x = min(a.X(), b.X());
    out.data.y = min(a.Y(), b.Y());
    out.data.z = min(a.Z(), b.Z());
    out.data.w = min(a.W(), b.W());
}

void Max(Vector& out, const Vector& a, const Vector& b) {
    out.data.x = max(a.X(), b.X());
    out.data.y = max(a.Y(), b.Y());
    out.data.z = max(a.Z(), b.Z());
    out.data.w = max(a.W(), b.W());
}

void Lerp(Vector& out, const Vector& a, const Vector& b, float t) {
    out.data.x = a.data.x * (1.0f - t) + b.data.x * t;
    out.data.y = a.data.y * (1.0f - t) + b.data.y * t;
    out.data.z = a.data.z * (1.0f - t) + b.data.z * t;
    out.data.w = a.data.w * (1.0f - t) + b.data.w * t;
}

}  // namespace Math

namespace Graphics {

void Geometry::Create(Builder* builder_, const Effect* effect_, const Material* material_,
                      Effect::ParamData* paramData_,
                      Shader::GeometryContext** shaderContextList_, PrimitiveType primType_,
                      int vertexCount_, int primCount, int triCount, int indexCount,
                      VertexBuffer* vertexBuffers_, int vertexBufferCount_,
                      unsigned int flags_, unsigned int sectorLight_) {
    builder = builder_;
    effect = (Effect*)effect_;
    material = (Material*)material_;
    paramData = paramData_;
    shaderContextList = shaderContextList_;
    primType = primType_;
    vertexCount = vertexCount_;

    vertexBuffers = vertexBuffers_;
    vertexBufferCount = vertexBufferCount_;

    degenerateBound = true;
    offDist = 0.0f;
    vertLodDist = 1.0e19f;

    sectorLight = sectorLight_;
    flags = flags_;

    wiiDirect = false;
    wiiIndexStride = 0;
    wiiIndexSkin = 0;
    wiiGeomStateOp = effect_->GetWiiGeomStateOp();

    indexLodCount = 1;
    indexLods[0].primCount = primCount;
    indexLods[0].triCount = triCount;
    indexLods[0].indexCount = indexCount;
    indexLods[0].lodVertCount = vertexCount_;
}

void Geometry::SetBroadBound(const BroadBound& bound, const Math::Vector4& sphere) {
    bbound = bound;
    boundSphere = sphere;
    degenerateBound = false;
}

}  // namespace Graphics
