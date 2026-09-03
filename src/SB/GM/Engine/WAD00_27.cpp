// WAD00_27 -- one function, read from the image with tools/disasm.py:
// IO::PadTilt's copy assignment, 408 bytes of Wii accelerometer state
// copied member by member. Like WAD03_28's, it is the COMPILER'S and not
// the author's -- STB_WEAK in the retail image, which is what an
// implicitly declared copy assignment is -- so this file declares the
// layout and lets the compiler generate the operator.
//
// Layouts from the DWARF (tools/dwarf_types.py): PadTilt 0x198 with the
// two acceleration vectors at +0 and +0x10, the two tilt angles at +0x20
// and +0x24, the 2D and 3D swings at +0x28 and +0x30, the two stability
// counters at +0x3C and +0x40, shakeSpeed at +0x44, a sixteen-entry
// Vector3 history at +0x50 with its length at +0x110, SwingRecognition
// (0x34) at +0x114, the MotionPlus block from +0x148, its orientation
// matrix at +0x164 and the calibration float at +0x194; SwingRecognition
// itself is two Vector3s, a Vector2, a Vector3 and two floats; Math's
// Vector is 0x10, Vector2 0x8, Vector3 0xC and Matrix33 0x30, a
// Vector4[3].
//
// The bytes say how each member is copied, and the four shapes agree:
//
//   * A scalar float or bool moves through its own register (`lfs`/`stfs`
//     at +0x20, +0x24 and the six MotionPlus floats, `lbz`/`stb` for the
//     connected flag). An int moves through a GPR.
//   * A small class member is a BLOCK copy in GPRs, eight bytes at a time
//     with the high word stored first: two of those for each 16-byte
//     Vector, one plus a word for each 12-byte Vector3, one for the
//     Vector2.
//   * A block too big to unroll becomes a loop with `lwzu`/`stwu`, based
//     four bytes before the member: 24 iterations for the 192-byte
//     history, 6 and a tail word for the 52-byte SwingRecognition. That
//     the 16-entry Vector3 array copies 8 bytes an iteration rather than
//     12 is what says it is copied as memory and not element by element.
//   * A member whose own copy assignment is out of line is a CALL, and
//     retail's is `__as__Q24Math8Matrix43FRCQ24Math8Matrix43` -- the
//     linker folded the identical Matrix33 and Matrix43 bodies onto one
//     symbol, so that is the name spelled here, as zUIModel and
//     zRandomModelList spell the folded empty constructor. Retail's body
//     is a three-iteration loop copying one Vector4 an iteration, which
//     is NOT what a plain `Vector4 v[3]` gives: left implicit, the member
//     is copied inline as a 48-byte block, the function needs no frame at
//     all and comes out 352 bytes of 88 words against retail's 380 of 95.
//     Declaring that operator and leaving it undefined is what makes the
//     copy a call; the definition belongs to whichever unit the folded
//     body came from, not this one.
//
// The one thing this file has to arrange: an implicit operator is only
// emitted where something USES it, and this fragment holds no statement
// that assigns a PadTilt -- retail's file did, elsewhere. A function added
// to carry one would be a function retail does not have, so the use is a
// file-scope pointer to the member instead. That costs twelve bytes of
// `.data` the split does not own, which is why this unit matches and could
// not be linked as it stands.

namespace Math {

class Vector4 {
public:
    float x;
    float y;
    float z;
    float w;
};

class Vector : public Vector4 {};

class Vector2 {
public:
    float x;
    float y;
};

class Vector3 {
public:
    float x;
    float y;
    float z;
};

// Retail's Matrix33 and Matrix43 have the same body and the linker folded
// them; the surviving symbol is Matrix43's.
class Matrix43 {
public:
    Matrix43& operator=(const Matrix43& o);

    Vector4 v[3];
};

}  // namespace Math

namespace IO {

class SwingRecognition {
public:
    /* +0x00 */ Math::Vector3 lastAcc;
    /* +0x0C */ Math::Vector3 deltaAcc;
    /* +0x18 */ Math::Vector2 dirVec2;
    /* +0x20 */ Math::Vector3 dirVec3;
    /* +0x2C */ float dirSpeedMax2;
    /* +0x30 */ float dirSpeedMax3;
};

class PadTilt {
public:
    /* +0x000 */ Math::Vector acc;
    /* +0x010 */ Math::Vector lastAcc;
    /* +0x020 */ float pitchAngle;
    /* +0x024 */ float rollAngle;
    /* +0x028 */ Math::Vector2 swing2D;
    /* +0x030 */ Math::Vector3 swing3D;
    /* +0x03C */ int swing2Stability;
    /* +0x040 */ int swing3Stability;
    /* +0x044 */ Math::Vector3 shakeSpeed;
    /* +0x050 */ Math::Vector3 accRecord[16];
    /* +0x110 */ unsigned int accRecordLength;
    /* +0x114 */ SwingRecognition swingRecognition;
    /* +0x148 */ bool wmp_connected;
    /* +0x14C */ float wmp_pitchVel;
    /* +0x150 */ float wmp_yawVel;
    /* +0x154 */ float wmp_rollVel;
    /* +0x158 */ float wmp_pitchAngle;
    /* +0x15C */ float wmp_yawAngle;
    /* +0x160 */ float wmp_rollAngle;
    /* +0x164 */ Math::Matrix43 wmp_orientation;
    /* +0x194 */ float wmp_calibration;
};

}  // namespace IO

// The use that makes the compiler emit the implicit operator.
typedef IO::PadTilt& (IO::PadTilt::*PadTiltAsFn)(const IO::PadTilt&);

static PadTiltAsFn sPadTiltAssign = &IO::PadTilt::operator=;
