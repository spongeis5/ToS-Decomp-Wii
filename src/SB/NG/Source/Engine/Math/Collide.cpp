// C:/branches/SB09/main/NG/Source/Engine/Math/Collide.cpp
//
// Layout from the Wii build's DWARF (tools/dwarf_types.py):
//
//   class CollKDTree  /* 0x10 */
//   {
//       /* +0x0 */ unsigned int branchCount;
//       /* +0x4 */ Branch* branchList;
//       /* +0x8 */ unsigned int triangleCount;
//       /* +0xC */ Triangle* triangleList;
//   };
//
// FixHeader points the two lists at the data that follows the header:
//
//   lwz  r0, 0(r3)        branchCount
//   addi r4, r3, 0x10     this + 1  -- the header is 0x10 bytes
//   stw  r4, 4(r3)        branchList
//   slwi r0, r0, 4        * 0x10    -- so sizeof(Branch) is 0x10
//   add  r0, r4, r0
//   stw  r0, 0xc(r3)      triangleList
//
// The `slwi 4` is what gives sizeof(Branch); the DWARF names the field but
// not the stride, and the stride is what makes `branchList + branchCount`
// the right way to write it.
//
// Fix/UnfixVerts walk the triangles turning a stored OFFSET into a pointer
// and back -- add on the way in, `subf` on the way out. Both reload
// triangleList and triangleCount from `this` on every iteration, which is
// why they are written as member accesses and not hoisted into locals: the
// store into the array could alias the members, and mwcc reloads.

namespace Math {

struct Vector3 {
    float x, y, z;
};

struct Branch {
    unsigned char _bytes[0x10];
};

struct Triangle {
    unsigned int verts;
    unsigned char _rest[0xC];
};

class CollKDTree {
public:
    void FixHeader();
    void FixVerts(const Vector3* base);
    void UnfixVerts(const Vector3* base);

    unsigned int branchCount;
    Branch* branchList;
    unsigned int triangleCount;
    Triangle* triangleList;
};

// THE FIRST UNIT IN THIS PROJECT TO CARRY ITS OWN DATA. Three things were
// needed and only one of them is in this file:
//
//   1. these definitions, in ADDRESS ORDER;
//   2. `.bss start:0x8077AAA8 end:0x8077AB08` on this unit in splits.txt,
//      with the parent's range CUT -- the upper half goes to WAD02_2.cpp,
//      the chunk that follows, because leaving both halves under the
//      parent makes the link order cyclic;
//   3. all six names in `force_active` in config.yml. NOTHING in the image
//      references them, so the linker dead-strips them and everything
//      after moves down by 96 bytes. The object is perfect while that
//      happens -- .bss 96 bytes, align 8, six symbols at the right
//      offsets -- so the failure reads as a layout mistake and is not one.
//
// The unit's own .bss, 96 bytes at 0x8077AAA8, from
// tools/dwarf_data_decl.py and the retail symbol table. Six static data
// members of two classes; the definitions are in address order, which is
// what puts them at the right offsets.
//
// float[4] rather than a vector type: 16 bytes and align 4. Nothing with
// align 16 can be right, because 0x8077AAA8 is not 16-aligned and that is
// where retail put it.
class QuickCull20 {
public:
    static float s_clamp[4];
    static float s_scaleOffs[4];
    static float s_unscaleOffs[4];
};

class QuickCull15 {
public:
    static float s_clamp[4];
    static float s_scaleOffs[4];
    static float s_unscaleOffs[4];
};

float QuickCull20::s_clamp[4];
float QuickCull20::s_scaleOffs[4];
float QuickCull20::s_unscaleOffs[4];
float QuickCull15::s_clamp[4];
float QuickCull15::s_scaleOffs[4];
float QuickCull15::s_unscaleOffs[4];

void CollKDTree::FixHeader() {
    branchList = (Branch*)(this + 1);
    triangleList = (Triangle*)(branchList + branchCount);
}

// POINTER arithmetic, not integer arithmetic. This was one word in
// forty-one: retail has `add r0, r4, r0` and `verts += (unsigned int)base`
// gives `add r0, r0, r4`.
//
// Writing the source the other way round -- `(unsigned int)base + verts` --
// changes NOTHING. mwcc normalises a commutative add before it reaches the
// emitter, so operand order in the text does not survive. What does survive
// is the KIND of expression: `(char*)base + offset` is a pointer being
// displaced, and the pointer becomes the first operand. That is the lever
// here, and it is not the one it looks like.
//
// UnfixVerts never had the choice -- `subf` is not commutative -- and it
// matched first time.
void CollKDTree::FixVerts(const Vector3* base) {
    for (unsigned int i = 0; i < triangleCount; i++) {
        triangleList[i].verts = (unsigned int)((char*)base + triangleList[i].verts);
    }
}

void CollKDTree::UnfixVerts(const Vector3* base) {
    for (unsigned int i = 0; i < triangleCount; i++) {
        triangleList[i].verts -= (unsigned int)base;
    }
}

}  // namespace Math

void CollKDTree_FixHeader(void* tree) {
    ((Math::CollKDTree*)tree)->FixHeader();
}
