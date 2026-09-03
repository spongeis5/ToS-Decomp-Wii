// WAD01_15 -- three functions, read from the image with tools/disasm.py:
// the reset paths of the pad input. IO::PadInput::Reset clears the flags
// and the changed mask, puts each of the three sticks back to a zero
// offset, a unit-X direction and no magnitude or angle, zeroes the
// eighteen pressures, resets both tilts and the IR pointer, and then the
// extended DPD state -- eight floats, two of them one. IO::PadTilt::Reset
// puts the accelerations back to the zero vector, the angles and the
// swing state to zero, the swing recognition's four vectors and two
// speeds to zero, the Wii MotionPlus velocities and angles to zero, its
// orientation to the identity matrix and its calibration to minus one.
//
// Layouts from the DWARF (tools/dwarf_types.py): PadInput 0x440 with the
// three PadStick at +0x14, the eighteen pressures at +0x68, the two
// PadTilt at +0xB0, the IR pointer at +0x3E0 and the 0x24-byte extended
// DPD state at +0x3F0; PadStick 0x1C as two Vector2 and three floats,
// of which normMag is the one this function does not touch; PadTilt
// 0x198 with the swing recognition at +0x114 and the MotionPlus block
// from +0x148; SwingRecognition 0x34. The zero, one and minus-one are
// the translation unit's pool, hence the generated header first, and the
// vector constants are Math's own globals.
//
// MATCHED, all three, and three separate levers were needed. Every one
// of them is about how a vector CONSTANT is copied, so any unit that
// assigns a Math vector will meet the same three.
//
//   1. The constants are NOT const. vec2Zero, vec2OneX, vec3Zero,
//      vec4Zero and _matIdentity all live in .bss in the image, not
//      .rodata -- they are built at runtime. Declared const, mwcc is
//      free to hoist their words out of a loop and past the stores
//      that could alias them: PadInput::Reset then pre-loaded four
//      words into registers, SPILLED them to the stack, and ran a
//      nine-instruction loop body where retail runs thirteen. Declared
//      plain, mwcc keeps only the address live and re-reads inside the
//      loop, which is what retail does. This one lever took
//      PadInput::Reset from 67 of 68 words wrong to exact and
//      PadTilt::Reset from 80 of 80 to 19 of 81.
//
//   2. Each vector's one member is itself a class -- Vector2 holds a
//      DataType of two floats, Vector3 of three, Vector4 of four, and
//      Vector derives from Vector4 (tools/dwarf_types.py --all; they
//      are in the DWARF under bare names, not Math::). That nesting is
//      what makes the copy a block move of WORDS. Spelled flat, as
//      four float members, mwcc synthesises a memberwise lfs/stfs
//      operator= and emits it OUT OF LINE as __as__Q24Math7Vector2 and
//      two siblings retail does not have at all; `#pragma
//      always_inline on` hides the symbols but keeps the floats.
//      Declaring them as plain PODs with no access specifier does not
//      help either -- mwcc still synthesised all three.
//
//   3. Nearly every assignment here is CHAINED. Retail loads
//      vec4Zero's four words once and stores them to two destinations,
//      vec3Zero's three once to three destinations, and each scalar run
//      goes out highest-address-first. That is `a = b = c = k`: the
//      rightmost target is assigned first, so the store order names the
//      chain. PadIRData::Reset is `x = y = 0.0f` and matched first try.
//
// One statement's position was left to fix after all three: retail's
// store of accRecordLength lands after the whole swingRecognition
// block and just before wmp_connected, though the member is declared
// ahead of it. Moving the statement there was the last word.
#include "SB/NG/Engine/WAD01_15.pool.h"

namespace Math {

class Vector2 {
public:
    class DataType {
    public:
        float x;
        float y;
    };

    DataType data;
};

class Vector3 {
public:
    class DataType {
    public:
        float x;
        float y;
        float z;
    };

    DataType data;
};

class Vector4 {
public:
    class DataType {
    public:
        float x;
        float y;
        float z;
        float w;
    };

    DataType data;
};

class Vector : public Vector4 {
};

class Matrix33 {
public:
    Vector4 v[3];
};

class Matrix43 : public Matrix33 {
public:
    Matrix43& operator=(const Matrix43& other);
};

// These five are in .bss in the image, not .rodata: they are set up at
// runtime and are not const. It matters -- see the note at the top.
extern Vector4 vec4Zero;
extern Vector2 vec2Zero;
extern Vector2 vec2OneX;
extern Vector3 vec3Zero;
extern Matrix43 _matIdentity;

}  // namespace Math

namespace IO {

class PadStick {
public:
    Math::Vector2 off;
    Math::Vector2 dir;
    float mag;
    float normMag;
    float ang;
};

class SwingRecognition {
public:
    Math::Vector3 lastAcc;
    Math::Vector3 deltaAcc;
    Math::Vector2 dirVec2;
    Math::Vector3 dirVec3;
    float dirSpeedMax2;
    float dirSpeedMax3;
};

class PadTilt {
public:
    void Reset();

    Math::Vector acc;
    Math::Vector lastAcc;
    float pitchAngle;
    float rollAngle;
    Math::Vector2 swing2D;
    Math::Vector3 swing3D;
    int swing2Stability;
    int swing3Stability;
    Math::Vector3 shakeSpeed;
    Math::Vector3 accRecord[16];
    unsigned int accRecordLength;
    SwingRecognition swingRecognition;
    bool wmp_connected;
    unsigned char _pad0[0x3];
    float wmp_pitchVel;
    float wmp_yawVel;
    float wmp_rollVel;
    float wmp_pitchAngle;
    float wmp_yawAngle;
    float wmp_rollAngle;
    Math::Matrix33 wmp_orientation;
    float wmp_calibration;
};

class PadIRData {
public:
    void Reset();

    float x;
    float y;
    float distanceFromBar;
    unsigned int status;
};

// The DWARF gives the extended state as an anonymous 0x24-byte type; the
// bytes touch eight of its floats.
class PadExtendedDPD {
public:
    float a;
    float b;
    float c;
    float d;
    float e;
    float f;
    float g;
    float h;
    float i;
};

class PadInput {
public:
    void Reset();

    unsigned int padLayout;
    unsigned int padSubLayout;
    bool gamecubeControllerConnected;
    unsigned char _pad0[0x3];
    int flags;
    int changed;
    PadStick stick[3];
    float pressure[18];
    PadTilt tilt[2];
    PadIRData irPointer;
    PadExtendedDPD extendedDPDState;
    Math::Vector WMPCorrectedAcc;
    PadIRData WMPExtendedIR;
    int wbc_err;
    float tgc_weight;
    bool zeroPointSet;
};

void PadInput::Reset() {
    int i;

    flags = 0;
    changed = 0;

    for (i = 0; i < 3; i++) {
        stick[i].off = Math::vec2Zero;
        stick[i].dir = Math::vec2OneX;
        stick[i].mag = 0.0f;
        stick[i].ang = 0.0f;
    }

    for (i = 0; i < 18; i++) {
        pressure[i] = 0.0f;
    }

    for (i = 0; i < 2; i++) {
        tilt[i].Reset();
    }

    irPointer.Reset();

    extendedDPDState.a = extendedDPDState.b = 0.0f;
    extendedDPDState.c = 1.0f;
    extendedDPDState.d = 0.0f;
    extendedDPDState.e = extendedDPDState.f = extendedDPDState.g = 0.0f;
    extendedDPDState.i = 1.0f;
}

void PadTilt::Reset() {
    acc = lastAcc = *(Math::Vector*)&Math::vec4Zero;

    pitchAngle = rollAngle = 0.0f;

    swing2D = Math::vec2Zero;
    swing3D = Math::vec3Zero;

    swing2Stability = swing3Stability = 0;

    shakeSpeed = Math::vec3Zero;

    swingRecognition.lastAcc = swingRecognition.deltaAcc =
        swingRecognition.dirVec3 = Math::vec3Zero;
    swingRecognition.dirVec2 = Math::vec2Zero;
    swingRecognition.dirSpeedMax2 = swingRecognition.dirSpeedMax3 = 0.0f;

    accRecordLength = 0;

    wmp_connected = false;

    wmp_pitchVel = wmp_yawVel = wmp_rollVel = 0.0f;
    wmp_pitchAngle = wmp_yawAngle = wmp_rollAngle = 0.0f;

    *(Math::Matrix43*)&wmp_orientation = Math::_matIdentity;

    wmp_calibration = -1.0f;
}

void PadIRData::Reset() {
    x = y = 0.0f;
    distanceFromBar = -1.0f;
    status = 0;
}
}  // namespace IO
