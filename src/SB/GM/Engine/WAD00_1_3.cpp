// WAD00_1_3 -- eight functions, read from the image with tools/disasm.py:
// the rest of the xVec3 arithmetic that WAD00_1_2 begins, plus xBox's
// assignment. NormalizeSafe measures the vector, leaves it alone and
// returns zero when its squared length is below 1e-10, and otherwise
// scales it by one over the length and returns the length -- the length
// itself is the squared length times its reciprocal square root, which
// is how the Gekko's estimate is used throughout. The two AddScale
// members add a scaled vector, one into a third vector and one in
// place, both as fused multiply-adds. v3cross and the two cross_?pos
// members are cross products: the general one, and the two special
// cases against the unit Y and X axes, where two of the three results
// are a copy and a negation and the third is zero. v3length is the
// squared length by fused multiply-adds, then the same reciprocal-root
// step. xBox::operator= copies its six words.
//
// Layouts from the DWARF (tools/dwarf_types.py): xVec3 is 0xC, three
// floats; xBox is 0x18, an upper and a lower xVec3, and the assignment
// moves it as six words rather than as two vectors. The three literals
// (1e-10, zero and one) are the translation unit's pool, hence the
// generated header first.
//
// Three shapes the bytes fixed. The epsilon test is a less-than taken as
// the branch away from the work, so the zero return is the block the
// compare falls into. The length is spelled as the squared length times
// the reciprocal root, not as a division or a sqrt call, which is what
// puts the multiply before the divide in both functions that compute
// it. And the two special-case cross products are written in the order
// their floating-point registers say, not in the order their stores
// come out: cross_xpos assigns the zero first and cross_ypos assigns
// the negation first, which is what gives each of them f1 and f2 the
// way round retail has -- with cross_ypos written zero-first the two
// registers swap and four of its nine words differ, while the stores
// stay in the same order either way.

#include "SB/GM/Engine/WAD00_1_3.pool.h"

class xVec3 {
public:
    xVec3& operator*=(float s);
    float length2() const;

    void AddScale(const xVec3& a, const xVec3& b, float s);
    void AddScale(const xVec3& b, float s);
    void cross_ypos(const xVec3& other);
    void cross_xpos(const xVec3& other);

    float x;
    float y;
    float z;
};

class xBox {
public:
    xBox& operator=(const xBox& other);

    xVec3 upper;
    xVec3 lower;
};

namespace Math {
float rsqrt(float x);
}

float xVec3NormalizeSafe(xVec3& v) {
    float len2 = v.length2();

    if (len2 < 1e-10f) {
        return 0.0f;
    }

    float len = len2 * Math::rsqrt(len2);

    v *= 1.0f / len;

    return len;
}

void xVec3::AddScale(const xVec3& a, const xVec3& b, float s) {
    x = b.x * s + a.x;
    y = b.y * s + a.y;
    z = b.z * s + a.z;
}

void v3cross(xVec3* out, xVec3* a, xVec3* b) {
    out->x = a->y * b->z - b->y * a->z;
    out->y = a->z * b->x - b->z * a->x;
    out->z = a->x * b->y - b->x * a->y;
}

void xVec3::cross_ypos(const xVec3& other) {
    x = -other.z;
    y = 0.0f;
    z = other.x;
}

void xVec3::cross_xpos(const xVec3& other) {
    x = 0.0f;
    y = other.z;
    z = -other.y;
}

xBox& xBox::operator=(const xBox& other) {
    *(unsigned int*)&upper.x = *(const unsigned int*)&other.upper.x;
    *(unsigned int*)&upper.y = *(const unsigned int*)&other.upper.y;
    *(unsigned int*)&upper.z = *(const unsigned int*)&other.upper.z;
    *(unsigned int*)&lower.x = *(const unsigned int*)&other.lower.x;
    *(unsigned int*)&lower.y = *(const unsigned int*)&other.lower.y;
    *(unsigned int*)&lower.z = *(const unsigned int*)&other.lower.z;

    return *this;
}

void xVec3::AddScale(const xVec3& b, float s) {
    x = b.x * s + x;
    y = b.y * s + y;
    z = b.z * s + z;
}

float v3length(xVec3* v) {
    float len2 = v->z * v->z + (v->x * v->x + v->y * v->y);

    return len2 * Math::rsqrt(len2);
}
