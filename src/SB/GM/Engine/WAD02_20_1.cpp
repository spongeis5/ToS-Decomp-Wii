// WAD02_20_1.cpp -- four functions, read from the image with
// tools/disasm.py. They are the point-in-wallnet test of the NPC
// navigation mesh, plus one vector helper. IsInsideWallNet refuses a net
// with no bounds, requires the position to be inside bound 0, and then
// refuses it if it is inside any of the other bounds -- bound 0 is the
// outline and the rest are holes. IsInsideBound tests the position
// against that bound in XZ, finds the triangle under it, checks the
// position really is in that triangle, interpolates the triangle's
// height at that XZ, and finally accepts only if the position's own Y is
// within the asset's two tolerances of it. FindYOnTriangleFromXZ does
// that interpolation: it takes the triangle's three vertices from two of
// its edges (the third vertex is whichever end of the second edge is not
// already used), flattens all three and the query point onto the XZ
// plane, forms the barycentric coordinates of the point in that
// flattened triangle by the usual five dot products, refuses anything
// outside the triangle by a hundred-thousandth, and then re-reads the
// three vertices unflattened and reads the height off the plane.
// xVec3Dist2 is the squared distance between two points.
//
// Layouts from the DWARF (tools/dwarf_types.py): zWallNet 0x60 on
// World::xOGEntity, with the asset at +0x3C in the base's tail padding;
// zWallNetAsset 0x50 with numBounds at +0x14, upperYTol at +0x28,
// lowerYTol at +0x2C and the bounds, vertices, edges and triangles
// pointers at +0x34, +0x38, +0x3C and +0x40; zWallNetBound 4 bytes;
// zWallNetEdge 8 bytes, srcVertex and dstVertex the first two;
// zWallNetTriangle 8 bytes, three edge indices then flags; xVec3 0xC and
// hkVector4 0x10. Every local's name, type and declaration line and the
// statement structure come from tools/dwarf_locals.py and
// tools/dwarf_lines.py -- which is also what fixed the frame, since mwcc
// lays a scope's locals out in REVERSE declaration order and the seven
// xVec3 slots run v1, v2, v3, pos, e0, e1, e2 downwards from +80 to +8.
//
// The three float literals (0.0f, -1e-5f, 1.00001f) are why the
// generated padding header comes first: retail gives each of them its
// own lis, which mwcc only does past 32 KB of .rodata, and the image
// measures this fragment's first literal at 50872 bytes into its unity
// unit's.
//
// MEASURED: 3 of the unit's 4 functions are byte-identical by
// tools/unitcmp.py, and so is the fifth function the object defines --
// `__as__5xVec3FRC5xVec3`, xVec3's implicit copy assignment, which mwcc
// emits out of line for the six `v = vertices[id]` statements and which
// the image has under that same name. The miss is IsInsideWallNet, and
// the last paragraph says what it is.
//
// Two shapes the bytes fixed, both of them a value given a name.
// IsInsideBound's last line is not `return pos.y >= y - lowerYTol &&
// pos.y <= y + upperYTol;` -- retail forms BOTH tolerance bounds before
// testing either, which is what two float locals give and the
// short-circuit && does not (9 of 66 words, and reversing the
// comparisons is 8). And xVec3Dist2 names its three differences: retail
// keeps dy, dx and dz in f4, f3 and f1, where the repeated-subexpression
// form reuses f0 for the first difference and shifts the other two down,
// 10 of 13 words. Four spellings without the locals tie at 10 -- the
// declaration split from the assignment, no local at all, references to
// the two points, and the sum re-associated (12). Worth writing down,
// because the DWARF disagrees: dwarf_locals.py names exactly one local
// in that function, `d`, and no dx, dy or dz. So the original reached
// retail's registers by some spelling the debug info does not name a
// variable for, and three named differences reproduce the bytes.
//
// NEAR MISS, IsInsideWallNet at 5 of 38 words. Every word is present and
// the difference is one instruction's position: retail finishes the
// prologue and both parameter copies before loading wallNetAsset, ours
// issues that load right after the mflr so the dependent load of
// numBounds does not stall behind it. It is the same shape as
// zNPCUpdateLOD::Reset in the sibling unit -- our compiler moving a load
// above stores that retail's compiler left alone -- but it does not
// answer to the same lever: `#pragma scheduling off` puts the load back
// where retail has it and then costs two other words instead (the asset
// temp takes r4 rather than r5, and the two argument registers of the
// first IsInsideBound call are set in the other order), so it lands at 4
// of 38 rather than at a match. Six more spellings tie at 5: a local for
// the asset, no local for the count, `!numberOfBounds` as the test, the
// loop counter declared above the loop, a volatile read of the count,
// and the function marked inline with a caller to force it out of line;
// the count and the counter unsigned is 6. Testing bound 0 before
// reading the count is a different function altogether (34 of 34) and
// contradicts the line table, which puts the count on line 698 and the
// bound-0 test on 703.

#include "SB/GM/Engine/WAD02_20_1.pool.h"

class zWallNetTriConx;
class zShortestPathTree;

class hkVector4 {
public:
    float dot3(const hkVector4& other) const;

    float x;
    float y;
    float z;
    float w;
};

class xVec3 {
public:
    void Sub(const xVec3& a, const xVec3& b);

    float x;
    float y;
    float z;
};

// The dot products are hkVector4's, called on twelve-byte xVec3 slots:
// the bytes branch to dot3__9hkVector4CFRC9hkVector4 with the xVec3's
// own address in r3. One expression, so it inlines and emits nothing.
inline float xVec3Dot3(const xVec3& a, const xVec3& b) {
    return ((const hkVector4*)&a)->dot3(*(const hkVector4*)&b);
}

class zWallNetBound {
public:
    unsigned char firstVertex;
    unsigned char verticesNum;
    unsigned char minZVertex;
    unsigned char maxZVertex;
};

class zWallNetEdge {
public:
    unsigned char srcVertex;
    unsigned char dstVertex;
    unsigned short flags;
    float lengthXZ;
};

class zWallNetTriangle {
public:
    unsigned short edges[3];
    unsigned short flags;
};

class zWallNetAsset {
public:
    unsigned char _pad0[0x10];
    int assetSize;
    int numBounds;
    int numVertices;
    int numEdges;
    int numBoundEdges;
    int numTriangles;
    float upperYTol;
    float lowerYTol;
    unsigned int flags;
    zWallNetBound* bounds;
    xVec3* vertices;
    zWallNetEdge* edges;
    zWallNetTriangle* triangles;
    zWallNetTriConx* connections;
    unsigned char _pad1[0x50 - 0x48];
};

class zWallNet {
public:
    bool IsInsideWallNet(const xVec3& pos) const;
    bool IsInsideBound(int boundID, const xVec3& pos) const;
    bool IsInsideBoundXZ(zWallNetBound* bound, const xVec3& pos) const;
    int FindTriangleIDXZ(const xVec3& pos) const;
    bool IsInTriangleXZ(const zWallNetTriangle* tri, const xVec3& pos) const;
    bool FindYOnTriangleFromXZ(const zWallNetTriangle* tri, float x, float z,
                               float& outY) const;

    unsigned char _pad0[0x3C];
    zWallNetAsset* wallNetAsset;
    bool isOn;
    bool isNPCOnly;
    xVec3 boundBoxMin;
    xVec3 boundBoxMax;
};

bool zWallNet::IsInsideWallNet(const xVec3& pos) const {
    int numberOfBounds = wallNetAsset->numBounds;

    if (numberOfBounds == 0) {
        return false;
    }

    if (!IsInsideBound(0, pos)) {
        return false;
    }

    for (int i = 1; i < numberOfBounds; i++) {
        if (IsInsideBound(i, pos)) {
            return false;
        }
    }

    return true;
}

bool zWallNet::IsInsideBound(int boundID, const xVec3& pos) const {
    if (!IsInsideBoundXZ(&wallNetAsset->bounds[boundID], pos)) {
        return false;
    }

    int triID = FindTriangleIDXZ(pos);

    if (triID == 255) {
        return false;
    }

    const zWallNetTriangle* tri = &wallNetAsset->triangles[triID];

    if (!IsInTriangleXZ(tri, pos)) {
        return false;
    }

    float y;

    if (!FindYOnTriangleFromXZ(tri, pos.x, pos.z, y)) {
        return false;
    }

    float lowerY = y - wallNetAsset->lowerYTol;
    float upperY = y + wallNetAsset->upperYTol;

    return pos.y >= lowerY && pos.y <= upperY;
}

bool zWallNet::FindYOnTriangleFromXZ(const zWallNetTriangle* tri, float x,
                                     float z, float& outY) const {
    int vID1, vID2, vID3;

    const zWallNetEdge* edge1 = &wallNetAsset->edges[tri->edges[0]];
    const zWallNetEdge* edge2 = &wallNetAsset->edges[tri->edges[1]];

    vID1 = edge1->srcVertex;
    vID2 = edge1->dstVertex;
    vID3 = edge2->srcVertex;

    if (vID3 == vID1 || vID3 == vID2) {
        vID3 = edge2->dstVertex;
    }

    xVec3 v1, v2, v3;

    v1 = wallNetAsset->vertices[vID1];
    v2 = wallNetAsset->vertices[vID2];
    v3 = wallNetAsset->vertices[vID3];

    v1.y = v2.y = v3.y = 0.0f;

    xVec3 pos, e0, e1, e2;

    pos.x = x;
    pos.y = 0.0f;
    pos.z = z;

    e0.Sub(pos, v1);
    e1.Sub(v2, v1);
    e2.Sub(v3, v1);

    float dot01, dot02, dot11, dot12, dot22;

    dot01 = xVec3Dot3(e0, e1);
    dot02 = xVec3Dot3(e0, e2);
    dot11 = xVec3Dot3(e1, e1);
    dot12 = xVec3Dot3(e1, e2);
    dot22 = xVec3Dot3(e2, e2);

    float denom = dot11 * dot22 - dot12 * dot12;
    float s = (dot22 * dot01 - dot12 * dot02) / denom;
    float t = (dot11 * dot02 - dot12 * dot01) / denom;

    if (s < -0.00001f || 1.00001f < s || t < -0.00001f || 1.00001f < t ||
        1.00001f < t + s) {
        return false;
    }

    v1 = wallNetAsset->vertices[vID1];
    v2 = wallNetAsset->vertices[vID2];
    v3 = wallNetAsset->vertices[vID3];

    e1.Sub(v2, v1);
    e2.Sub(v3, v1);

    outY = v1.y + s * e1.y + t * e2.y;

    return true;
}

float xVec3Dist2(const xVec3* a, const xVec3* b) {
    float dx = a->x - b->x;
    float dy = a->y - b->y;
    float dz = a->z - b->z;
    float d = dx * dx + dy * dy + dz * dz;

    return d;
}
