// C:/branches/SB09/main/GM/Engine/Game/zNPCStatus.cpp
//
// Layout from the Wii build's DWARF (tools/dwarf_types.py):
//
//   class zNPCStatus /* 0x1C */ {
//       /* +0x0  */ xVec3 lastPos;
//       /* +0xC  */ xVec3 lastOrientation;
//       /* +0x18 */ enum eNPCStatus lastStatus;
//   };
//   class NPCAsset  { /* +0x10 */ xEntAsset EntAsset; ... };
//   class xEntAsset { /* +0x20 */ Rotation3 Orientation;
//                     /* +0x2C */ vec3 Pos; ... };
//
// which is what the loaded offsets are:
//
//   0x10 + 0x2C = 0x3C   ->  lfs f1,0x3c / f2,0x40 / f3,0x44   = Pos
//   0x10 + 0x20 = 0x30   ->  lfs f1,0x30 / f2,0x34 / f3,0x38   = Orientation
//
// and the constructor is called at `this+0` then `this+0xC`, so Pos goes to
// lastPos and Orientation to lastOrientation -- the source order is the
// store order, not the field order of the asset.
//
// The callee is Math::Vector::Vector(float, float, float), so xVec3 is
// Math::Vector here rather than a struct of its own.
//
// NEAR-MISS at 67.55%, 39 words against retail's 22, and the mechanism is
// CONSTRUCTION IN PLACE:
//
//   retail                          ours
//   lfs f1,0x3c(r4) ...             lfs f1,0x3c(r4) ...
//   bl Vector::Vector   r3 = this   addi r3, r1, 0x14      temporary
//                                   bl Vector::Vector      into the stack
//                                   mr r4, r3
//                                   mr r3, r30
//                                   bl operator=           then copied out
//
// Retail calls the constructor with r3 pointing AT THE MEMBER; ours builds
// a temporary on the stack and copies it. The stack frame is 0x30 rather
// than 0x10 for exactly that reason, and our object also EMITS a copy
// assignment operator that retail's does not contain -- seven trailing
// words of lfs/stfs.
//
// So `member = Vector(a, b, c)` is not what the original says, or not with
// this declaration of Vector. What would construct straight into the member
// is the open question; it is not the field offsets, which are confirmed
// from two directions, and not the argument order, which the retail loads
// give directly.
//
// TWO SEARCHES ARE ALREADY DONE, so nobody should repeat them.
//
// SEVEN SPELLINGS, all 6 of 22 words except where noted: plain assignment
// with only a constructor declared; with an inline operator=; with a
// declared-but-undefined operator= (29 words instead of 39, still wrong);
// placement new (3 of 22); members declared before the constructor; the
// constructor taking const float& (1 of 22); and a default constructor
// added alongside. The ties are the finding -- these are one function
// written seven ways.
//
// EIGHT FLAG SETS, all 6 of 22: -inline auto/on/off/all/deferred, -O4,p,
// -O3,s, and -Cpp_exceptions on. That matters because the last two
// near-misses of this shape WERE flags -- -O4,s fixed zPlayerContainer and
// -sdata 0 fixed iTime -- so it was the first thing to try and it is not
// the answer here.
//
// What is left is the declaration of Math::Vector itself, which this file
// guesses at and the DWARF does not pin down: `xVec3` is a leaf name, and
// whatever it really is has a constructor that can be called straight onto
// a member.

namespace Math {
struct Vector {
    Vector(float x, float y, float z);
    float x, y, z;
};
}

typedef Math::Vector xVec3;

struct Rotation3 {
    float x, y, z;
};

struct vec3 {
    float x, y, z;
};

namespace Sext {

struct xEntAsset {
    unsigned char _head[0x20];
    Rotation3 Orientation;
    vec3 Pos;
    unsigned char _tail[0x100 - 0x38];
};

struct NPCAsset {
    unsigned char _head[0x10];
    xEntAsset EntAsset;
};

}  // namespace Sext

class zNPCStatus {
public:
    void ResetToNPCAsset(const Sext::NPCAsset* asset);

    xVec3 lastPos;
    xVec3 lastOrientation;
    int lastStatus;
};

void zNPCStatus::ResetToNPCAsset(const Sext::NPCAsset* asset) {
    lastPos = xVec3(asset->EntAsset.Pos.x, asset->EntAsset.Pos.y,
                    asset->EntAsset.Pos.z);
    lastOrientation = xVec3(asset->EntAsset.Orientation.x,
                            asset->EntAsset.Orientation.y,
                            asset->EntAsset.Orientation.z);
}
