// WAD00_1_2.cpp -- six xVec3 members, read from the image with
// tools/disasm.py. NormalizeSafe takes the squared length, refuses to
// divide when it is under 1e-10 (returning zero and leaving the vector
// alone), and otherwise forms the length as len2 * rsqrt(len2), scales
// the vector by its reciprocal through operator*= and returns the
// length. negate flips the three components in place. operator-=
// subtracts componentwise. Lerp writes a + t * (b - a) into this, one
// fused multiply-add per component. Add and Scale are the plain
// componentwise sum and scalar product.
//
// Layouts from the DWARF (tools/dwarf_types.py): xVec3 is 0xC bytes,
// three floats at +0, +4 and +8, and nothing else is touched here. The
// three literals are read out of the image at the addresses the loads
// build: @216698 (0x8067D440) is 1.0f, @217794 (0x8067D444) is 0.0f and
// @227851 (0x8067F240) is 1e-10f. The unit loads float literals and
// carries none of its own .rodata in the split, so the generated
// padding header goes first -- the measured distance is 24 bytes and
// gen_poolprefix.py floors it at 32 KB, which is what makes mwcc spell
// a lis per literal instead of sharing one base.
//
// Three shapes the bytes fixed. The refusal is the EARLY RETURN, not
// the else: retail falls through the bge into the zero and branches to
// the shared epilogue, which is what `if (len2 < 1e-10f) return 0.0f;`
// gives and what an `if (len2 >= ...) { ... }` with a trailing return
// does not. length2's result is kept in f31 across the rsqrt call, so
// it is a named local. And the reciprocal is a real division of the
// 1.0f literal by the length (fdivs), not a multiply by a folded
// constant.

#include "SB/GM/Engine/WAD00_1_2.pool.h"

class xVec3 {
public:
    float length2() const;

    xVec3& operator*=(float s);
    xVec3& operator-=(const xVec3& v);

    float NormalizeSafe();
    void negate();
    void Lerp(const xVec3& a, const xVec3& b, float t);
    void Add(const xVec3& a, const xVec3& b);
    void Scale(const xVec3& v, float s);

    float x;
    float y;
    float z;
};

namespace Math {
float rsqrt(float v);
}

float xVec3::NormalizeSafe() {
    float len2 = length2();

    if (len2 < 1e-10f) {
        return 0.0f;
    }

    float len = len2 * Math::rsqrt(len2);

    *this *= 1.0f / len;

    return len;
}

void xVec3::negate() {
    x = -x;
    y = -y;
    z = -z;
}

xVec3& xVec3::operator-=(const xVec3& v) {
    x -= v.x;
    y -= v.y;
    z -= v.z;

    return *this;
}

void xVec3::Lerp(const xVec3& a, const xVec3& b, float t) {
    x = a.x + t * (b.x - a.x);
    y = a.y + t * (b.y - a.y);
    z = a.z + t * (b.z - a.z);
}

void xVec3::Add(const xVec3& a, const xVec3& b) {
    x = a.x + b.x;
    y = a.y + b.y;
    z = a.z + b.z;
}

void xVec3::Scale(const xVec3& v, float s) {
    x = v.x * s;
    y = v.y * s;
    z = v.z * s;
}
