// iMath3.cpp -- four functions, read from the image with tools/disasm.py:
// the point tests for the three bounding volumes, and the sphere that
// bounds a box. iSphereIsectVec puts the vector from the sphere's centre
// to the point in the intersection's normal, keeps its length as the
// distance, and writes the distance less the radius as the penetration.
// iSphereInitBoundBox centres a sphere on the midpoint of a box's two
// corners -- their sum halved -- and takes its radius from the distance
// to the upper corner. iCylinderIsectVec answers -1 for a point inside
// the cylinder and 1 for one outside: the height test first, against the
// centre's y less and plus the half-height, then the distance in the xz
// plane against the radius. iBoxIsectVec is the same answer for a box,
// six ordered compares against the two corners.
//
// Layouts from the DWARF (tools/dwarf_types.py): xVec3 0xC; xSphere 0x10
// as a centre and a radius; xCylinder 0x14 with a half-height after
// those; xBox 0x18 as an upper and a lower corner; xIsect 0x2C with the
// penetration at +4, the normal at +0x1C and the distance at +0x28. The
// three literals (a half, one and minus one) are the translation unit's
// pool, hence the generated header first.
//
// Three shapes the bytes fixed. Every compare is written the way the
// ordered form needs -- retail's cror before each branch is what a <=
// or >= against a float gives. The box test's six compares are in
// retail's order, each axis against the LOWER corner first and the
// upper second; the other way round costs six words. And the three
// vector helpers take non-const pointers: declared const they mangle
// to different symbols, which unitcmp catches as a branch to the wrong
// name even though the instructions are identical. v3add's arguments
// are the lower corner then the upper, which is the order the bytes
// pass them in.
//
// NEAR MISS, iCylinderIsectVec at 42 of 44 words, and the two that
// differ are the two subtractions. Retail loads the POINT's coordinate
// first and then subtracts it FROM the centre's -- fsubs centre,point
// off loads in the other order -- so whichever way the source is
// written one half comes out wrong: the point minus the centre (kept
// here) gets both loads right and the subtract backwards, and the
// centre minus the point gets the subtract right and both loads
// backwards, six words. Also tried and worse: the negation of the
// first form, which emits the negations (26 words), and each point
// coordinate read into its own local first (6). The two squares are
// symmetric, so the sign never shows; what is left to find is the
// spelling that loads the point first and still subtracts from the
// centre.

#include "SB/GM/Engine/Core/Wii/iMath3.pool.h"

class xVec3 {
public:
    xVec3& operator*=(float s);

    float x;
    float y;
    float z;
};

class xSphere {
public:
    xVec3 center;
    float r;
};

class xCylinder {
public:
    xVec3 center;
    float r;
    float h;
};

class xBox {
public:
    xVec3 upper;
    xVec3 lower;
};

class xIsect {
public:
    unsigned int flags;
    float penned;
    float contained;
    float lapped;
    xVec3 point;
    xVec3 norm;
    float dist;
};

void v3sub(xVec3* out, xVec3* a, xVec3* b);
void v3add(xVec3* out, xVec3* a, xVec3* b);
float v3length(xVec3* v);

namespace Math {
float sqrt(float x);
}

void iSphereIsectVec(const xSphere* sphere, const xVec3* point,
                     xIsect* isect) {
    v3sub(&isect->norm, (xVec3*)point, (xVec3*)&sphere->center);

    isect->dist = v3length(&isect->norm);
    isect->penned = isect->dist - sphere->r;
}

void iSphereInitBoundBox(xSphere* sphere, const xBox* box) {
    v3add(&sphere->center, (xVec3*)&box->lower, (xVec3*)&box->upper);

    sphere->center *= 0.5f;

    xVec3 half;

    v3sub(&half, (xVec3*)&box->upper, &sphere->center);

    sphere->r = v3length(&half);
}

void iCylinderIsectVec(const xCylinder* cylinder, const xVec3* point,
                       xIsect* isect) {
    float low = cylinder->center.y - cylinder->h;
    float high = cylinder->center.y + cylinder->h;

    if (point->y >= low && point->y <= high) {
        float dz = point->z - cylinder->center.z;
        float dx = point->x - cylinder->center.x;

        if (Math::sqrt(dx * dx + dz * dz) <= cylinder->r) {
            isect->penned = -1.0f;

            return;
        }
    }

    isect->penned = 1.0f;
}

void iBoxIsectVec(const xBox* box, const xVec3* point, xIsect* isect) {
    if (point->x >= box->lower.x && point->x <= box->upper.x &&
        point->y >= box->lower.y && point->y <= box->upper.y &&
        point->z >= box->lower.z && point->z <= box->upper.z) {
        isect->penned = -1.0f;

        return;
    }

    isect->penned = 1.0f;
}
