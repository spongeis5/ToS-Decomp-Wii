// WAD02_4_1 -- three functions of Math's quaternion code, read from the
// image with tools/disasm.py.
//
//   Math::Quaternion::MakeEuler builds a Matrix33 from the three
//   components of a Vector, clears its fourth row and takes the
//   matrix's quaternion.
//   Math::Slerp is the ordinary spherical interpolation: above a
//   cosine of 0.9995 it lerps and renormalises, otherwise it flips the
//   near quaternion when the cosine is negative, takes the angle with
//   acos, and blends the source with the perpendicular component by
//   the sine and cosine of t * angle.
//   Math::Vector4::Normalize scales the vector by the reciprocal square
//   root of its own dot product and returns itself.
//
// Layouts from the DWARF (tools/dwarf_types.py): Vector4 is 0x10 and its
// one member is a nested DataType of four floats, Vector derives from
// Vector4, Quaternion HOLDS a Vector4 (it does not derive from one),
// Matrix33 is three Vector4.  The two constants are read out of the
// image: @149918 at 0x80693CC4 is 0x3F7FDF3B, 0.9995f, and @145708 at
// 0x80693C0C is 0.0f.
//
// The mangled names settle two things that were open:
//
//   * A HIDDEN STRUCT-RETURN BUFFER IS NOT MANGLED.
//     GetQuaternion__Q24Math8Matrix33CFv takes no parameters and is
//     const, yet retail passes the destination in r3 and `this` in r4.
//     So Math's `Mul(dst, src, s)` family really does take its result
//     as an explicit first parameter -- the `R` in the name is a
//     parameter, not a return slot.
//   * vec4Zero__4Math is in .bss (0x8077B3F8), not .rodata, so it is
//     built at runtime and is NOT const -- the Math-constants lever.
//
// 2 of the 3 functions the object defines are byte-identical:
// MakeEuler (36 of 36 words) and Vector4::Normalize (17 of 17).
//
// NEAR MISS -- Slerp, 113 of 121 words, 8 differing, retail's length
// exactly (484 bytes).  The eight are ONE 16-byte struct copy, `q = a`
// in the else of the negation test, and the difference is which of
// mwcc's two block-move strategies it uses:
//
//     retail   lwz r3,0(r29) ; lwz r0,4(r29)  ; stw r0,164(r1)
//              stw r3,160(r1); lwz r3,8(r29)  ; lwz r0,12(r29)
//              stw r0,172(r1); stw r3,168(r1)          -- 8-byte chunks
//     ours     lwz r5,0(r29) ; lwz r4,4(r29)  ; lwz r3,8(r29)
//              lwz r0,12(r29); stw r5,160(r1) ; stw r4,164(r1)
//              stw r3,168(r1); stw r0,172(r1)          -- one 16-byte move
//
// The function has FOUR such copies and the other three match.  Reading
// all four gives each compiler's rule, and they differ by one clause:
//
//     `out = sum` and `out = blended`   local -> reference   chunked in
//                                       both
//     `q = negated`                     local -> local       one move in
//                                       both
//     `q = a`                           reference -> local   chunked in
//                                       retail, one move in ours
//
// so retail chunks when EITHER side is reached through a pointer and
// ours chunks only when the DESTINATION is.  Same compiler, same flags,
// so something about the source of the read has to differ, and none of
// the following moves it -- every one compiled and read back, all
// eight words unchanged in each:
//
//   * ten spellings of the copy itself: `q = a`, `q.v = a.v`,
//     `q = *(Quaternion*)&a` (const cast away), `q = *(const
//     Quaternion*)&a`, `(Vector&)q = (const Vector&)a`,
//     `*(Quaternion*)&q = *(const Quaternion*)&a`, `q.v = a.V()` and
//     `q = (const Quaternion&)a.V()` (a call as the source, which is
//     what fixed MakeEuler's argument order), `q.v = *(const
//     Vector4*)&a`, and four word-by-word `*(int*)&q.v.data.x = ...`
//     assignments, which mwcc coalesces back into the same 16-byte move;
//   * a pointer on either side -- `Quaternion* qp = &q; *qp = a;` and
//     `const Quaternion* pa = &a; q = *pa;` -- both folded away;
//   * `q.v.data = a.v.data`, which is a DataType and therefore
//     synthesises an out-of-line `__as__Q34Math7Vector48DataType`
//     retail does not have (73 of 116 words, and an EXTRA function);
//   * four float assignments, member by member (lfs/stfs, 28 of 121);
//   * `memcpy(&q, &a, sizeof(Quaternion))`, which mwcc expands into
//     something else again (468 bytes, 72 of 117);
//   * `__attribute__((aligned(8)))` on Vector4's member and on the
//     class, in case the copy width followed the alignment;
//   * ALIASING RULED OUT BY MEASUREMENT: making q's address escape
//     NON-const before the copy (an added `Mul(q.v, q.v, cosValue)`)
//     leaves all eight words exactly as they are, so the difference is
//     not mwcc deciding that a const reference parameter cannot alias
//     a local.
//
// Not tried, and where the next attempt should go: another compiler
// version, and a census of retail's own reference-to-local 16-byte
// copies to see whether the chunked form is universal there or whether
// some other Math function has the one-move form -- if retail never has
// it, the lever is a flag or a type property and not this statement.
//
// THE FRAME SLOTS ARE THE WHOLE OF SLERP, and they come out of one
// rule already measured in RVLFaceLibEntity.cpp: mwcc lays a scope's
// locals out in REVERSE DECLARATION ORDER, so the first declared gets
// the HIGHEST address.  Retail's ten sixteen-byte objects sit at
// +160, +144, +128, +112, +96, +80, +64, +48, +32 and +16, and the two
// floats SinCos writes at +12 and +8.  Reading the `addi rX,r1,N` of
// every call in order gives which object is which, and the
// declarations below are in exactly that order -- q, perp, then the
// three of the lerp branch (its Add result highest and its subtraction
// lowest), then the negation, the scaled source, and the three of the
// blend.  Written in any other order the calls still come out in the
// same sequence and every `addi` immediate is wrong.
//
// Two further orderings the bytes fix:
//
//   * SinCos's two out parameters are at +12 and +8, so `sinValue` is
//     declared before `cosValue`; the sine multiplies the perpendicular
//     and the cosine multiplies q, which is what says which is which.
//   * The blend evaluates its RIGHT operand first -- perp * sinValue at
//     +16 is computed before q * cosValue at +32 -- so the two Mul
//     statements are written in that order.
//
// SLERP IS 8 OF ITS 121 WORDS, and all eight are one 16-byte struct
// copy -- the assignment of a Quaternion into a stack local, at +160
// off r1 from a source in r29. Retail copies it in two 8-byte halves,
// two registers at a time, storing the HIGH word of each half before
// the low one:
//
//   lwz r3,0(r29) ; lwz r0,4(r29) ; stw r0,164(r1) ; stw r3,160(r1)
//   lwz r3,8(r29) ; lwz r0,12(r29) ; stw r0,172(r1) ; stw r3,168(r1)
//
// Ours loads all four words into four registers (r5, r4, r3, r0) and
// then stores all four in ascending order. Every other word of the
// function, the twenty masked relocations included, is identical.
//
// The TYPE is not the cause and was checked: Quaternion is one Vector4
// and Vector4 is one nested DataType of four floats, which is what the
// DWARF gives and what makes the copy a block move of words at all --
// spelled flat it would be a memberwise lfs/stfs, as WAD01_15.cpp
// records. So the difference is how many registers the copy is allowed,
// which is a question about what else is live at that point rather than
// about the copy.
//
// AND RETAIL DOES THE OTHER COPY THE WAY WE DO. The if branch copies
// the same type from the stack local `negated` at +80, and retail
// spells that one with FOUR registers ascending -- lwz r5,r4,r3,r0
// then four stores in order -- which is exactly what ours emits and it
// matches. So the type is not the cause and neither is the number of
// registers as such: the same 16-byte class is copied twice in one
// function, four registers one way and two the other, and only the
// second disagrees. Both read through the SAME pointer-free path in
// ours and through r29 in retail, so it is not the source kind either.
//
// Six spellings swept, all tied at 8 of 121 except where noted:
// as written; the else copy as `q.v = a.v`; both copies as `.v`; the
// else copy through a named const reference. Two are much worse and
// are not the answer: `q.v.data = a.v.data` is 73 of 116 (it shortens
// the function by five instructions), and swapping the two branches so
// the else becomes the then is 72 of 121.
//
// That leaves the instruction SCHEDULER, which is the same family
// zNPCUpdateLOD::Reset and WAD02_20_1::IsInsideWallNet record in this
// repository -- ours interleaving loads and stores differently from
// retail with every word otherwise identical. Neither of those has
// been reached either, and `#pragma scheduling off` is measured there
// as costing more than it buys.
//// MAKEEULER'S ARGUMENTS ARE ACCESSOR CALLS, and that is the whole of
// it.  Retail loads the vector's z first, then y, then x -- right to
// left.  Written `MakeEulerInternal(euler.data.x, euler.data.y,
// euler.data.z)` mwcc emits three plain loads LEFT to right and the
// function is 2 of 36 words out, the first and third lfs swapped.  An
// argument that is a CALL forces right-to-left evaluation, so
// `euler.X(), euler.Y(), euler.Z()` with one-expression inline
// accessors is exact; so is `euler[0], euler[1], euler[2]` through an
// inline operator[], and so is reading the three into locals declared
// z, y, x and passing those.  A reference to the DataType
// (`const Vector4::DataType& d = euler.data; ... d.x, d.y, d.z`) and a
// cast on each argument both leave it at 2 of 36, so it is the CALL
// that does it, not the extra name.
//
// `float angle = acos(cosAngle);` is a double call narrowed to float:
// retail has `fmr f1,f30 ; bl acos ; frsp f0,f1 ; fmuls f1,f29,f0`, the
// frsp BEFORE the multiply, which is the narrowing of the result rather
// than of the product.
//
// Vector4::Normalize is `scope:weak` in the image, so it is defined
// in-class and emitted weak at its first use -- which is inside Slerp,
// and that is what puts it third, after Slerp, as retail has it.
// Math::Matrix33's default constructor is declared and never defined:
// retail calls it (the weak lone `blr` at 0x800075C0), and an empty
// inline definition would be inlined away.

class hkVector4 {
public:
    void setSub4(const hkVector4& a, const hkVector4& b);

    float x;
    float y;
    float z;
    float w;
};

extern "C" double acos(double x);

namespace Math {

class Vector4;

float Dot(const Vector4& a, const Vector4& b);
Vector4& Mul(Vector4& dst, const Vector4& src, float scale);
Vector4& Negate(Vector4& dst, const Vector4& src);
float rsqrt(float x);
void SinCos(float& sinValue, float& cosValue, float angle);

class Vector4 {
public:
    class DataType {
    public:
        float x;
        float y;
        float z;
        float w;
    };

    Vector4& Normalize();

    float X() const { return data.x; }
    float Y() const { return data.y; }
    float Z() const { return data.z; }
    float operator[](int i) const { return (&data.x)[i]; }

    DataType data;
};

class Vector : public Vector4 {
};

Vector& Add(Vector& dst, const Vector& a, const Vector& b);

// In .bss in the image, not .rodata: built at runtime, so not const.
extern Vector4 vec4Zero;

class Quaternion {
public:
    void MakeEuler(const Vector& euler);

    const Vector4& V() const { return v; }

    Vector4 v;
};

class Matrix33 {
public:
    // Declared, never defined: retail calls the image's weak lone blr.
    Matrix33();

    void MakeEulerInternal(float x, float y, float z);
    void SetRowInternal(int row, const Vector& v);
    Quaternion GetQuaternion() const;

    Vector4 v[3];
};

void Slerp(Quaternion& out, const Quaternion& a, const Quaternion& b, float t);

}  // namespace Math

void Math::Quaternion::MakeEuler(const Math::Vector& euler) {
    Matrix33 m;

    m.MakeEulerInternal(euler.X(), euler.Y(), euler.Z());
    m.SetRowInternal(3, (const Vector&)vec4Zero);

    *this = m.GetQuaternion();
}

void Math::Slerp(Math::Quaternion& out, const Math::Quaternion& a,
                 const Math::Quaternion& b, float t) {
    Quaternion q;
    Quaternion perp;
    Quaternion sum;
    Quaternion scaled;
    Quaternion diff;
    Quaternion negated;
    Quaternion aScaled;
    Quaternion blended;
    Quaternion qPart;
    Quaternion perpPart;
    float sinValue;
    float cosValue;
    float dot;
    float cosAngle;
    float angle;

    dot = Dot(a.v, b.v);
    cosAngle = dot;

    if (dot > 0.9995f) {
        ((hkVector4&)diff).setSub4((const hkVector4&)b, (const hkVector4&)a);
        Mul(scaled.v, diff.v, t);
        Add((Vector&)sum, (const Vector&)a, (const Vector&)scaled);

        out = sum;
        out.v.Normalize();
        return;
    }

    if (dot < 0.0f) {
        cosAngle = -dot;
        Negate(negated.v, a.v);
        q = negated;
    } else {
        q = a;
    }

    angle = acos(cosAngle);
    SinCos(sinValue, cosValue, t * angle);

    Mul(aScaled.v, a.v, dot);
    ((hkVector4&)perp).setSub4((const hkVector4&)b, (const hkVector4&)aScaled);
    perp.v.Normalize();

    Mul(perpPart.v, perp.v, sinValue);
    Mul(qPart.v, q.v, cosValue);
    Add((Vector&)blended, (const Vector&)qPart, (const Vector&)perpPart);

    out = blended;
}

// DEFINED AFTER ITS CALLER, and that is the only thing that keeps it out
// of line.  Three calls and no store of its own, so -inline auto takes
// it: defined above Slerp it is inlined into both of Slerp's uses, Slerp
// comes out 524 bytes against retail's 484, and the object defines two
// functions instead of three.  `#pragma dont_inline on` around the
// definition, in the class body and out of it, changes nothing -- both
// were measured.  mwcc cannot inline a body it has not read yet, so the
// definition goes here; it stays `inline`, which is what makes it weak
// as retail has it, and being emitted at the end is retail's order too.
inline Math::Vector4& Math::Vector4::Normalize() {
    Mul(*this, *this, rsqrt(Dot(*this, *this)));
    return *this;
}
