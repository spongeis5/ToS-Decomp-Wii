// zNPCHelper.cpp -- three functions, read from the image with
// tools/disasm.py. Two of them answer "how far off its facing is this
// point": GetTanTheta2 gives the squared tangent of the angle between the
// NPC's facing and the target, signed by which side of the NPC the target
// is on, and GetTanThetaXZ the tangent in the horizontal plane alone, also
// signed. Both take the target minus the NPC's position, rotate that
// difference into the NPC's frame with the third function, and divide the
// sideways components by the forward one.
//
// Layout from the DWARF (tools/dwarf_types.py): xVec3 0xC, three floats.
// The structure of both bodies is the DWARF's too: tools/dwarf_locals.py
// names diffVector and diffLocal (the frame slots the two calls take,
// declared in that order), oneOverZ (f2, declared ABOVE the branch, since
// both arms assign it), and in each arm tanThetaXZ then tanThetaYZ then
// tanTheta2; tools/dwarf_lines.py puts one line number on each ternary and
// gives RotateWorldToLocal's three statements in the order x, y, z.
//
// The float constants are read from the image, not guessed: 0.0f, 1e-5f
// and -1e-5f, 100000.0f and -100000.0f, 1.0f and -1.0f, at 8068BA28,
// 8068BA78, 8068BB24, 8068BE08, 8068BE0C, 8068BA48 and 8068BA4C.
//
// Four shapes the bytes fixed.
//
// The forward test is `>= 0.0f`: `fcmpo` then `cror 2,1,2` folds greater
// into equal and the `bne` leaves on the negative side, which is the
// greater-or-equal form and not the less-or-equal one.
//
// Each clamp is a conditional EXPRESSION on one line, not an if with two
// assignments: the guarded value comes FIRST and the divide is branched to
// over it, which is the ternary's block order.
//
// The two products are read back out of the local, so `tanThetaXZ` and
// `tanThetaYZ` are named locals and the sum is written over them --
// mwcc evaluates the right operand first, so the YZ load, multiply and
// square come out ahead of the XZ ones though XZ is declared first. The
// negated arm is one `fnmadds`, which is the negation written around the
// whole sum rather than on either term.
//
// RotateWorldToLocal is NOT inlined into GetTanThetaXZ, which is defined
// after it: retail branches to it from both callers. It is small enough
// that -inline auto would take it, so the definition here is guarded with
// `#pragma dont_inline`, which leaves its own thirteen instructions
// untouched.
//
// NEAR MISS: both GetTanTheta2 and GetTanThetaXZ. See the bottom of this
// comment block -- the paragraph is written after the measurement, below
// the functions, so it states the aligned counts rather than predicting
// them.

#include "SB/GM/Engine/Game/zNPCHelper.pool.h"

class xVec3 {
public:
    void Sub(const xVec3& a, const xVec3& b);

    float x;
    float y;
    float z;
};

class zNPCHelper {
public:
    static float GetTanTheta2(const xVec3* npcPos, const xVec3* npcAt,
                              const xVec3* targetPos);
    static void RotateWorldToLocal(xVec3& out, const xVec3& v,
                                   const xVec3& dir);
    static float GetTanThetaXZ(const xVec3* npcPos, const xVec3* npcAt,
                               const xVec3* targetPos);
};

float zNPCHelper::GetTanTheta2(const xVec3* npcPos, const xVec3* npcAt,
                               const xVec3* targetPos) {
    xVec3 diffVector;

    diffVector.Sub(*targetPos, *npcPos);

    xVec3 diffLocal;

    RotateWorldToLocal(diffLocal, diffVector, *npcAt);

    float oneOverZ;

    if (diffLocal.z >= 0.0f) {
        oneOverZ = diffLocal.z < 0.00001f ? 100000.0f : 1.0f / diffLocal.z;

        float tanThetaXZ = diffLocal.x * oneOverZ;
        float tanThetaYZ = diffLocal.y * oneOverZ;

        float tanTheta2 = tanThetaXZ * tanThetaXZ + tanThetaYZ * tanThetaYZ;

        return tanTheta2;
    } else {
        oneOverZ = diffLocal.z > -0.00001f ? -100000.0f : 1.0f / diffLocal.z;

        float tanThetaXZ = diffLocal.x * oneOverZ;
        float tanThetaYZ = diffLocal.y * oneOverZ;

        float tanTheta2 = -(tanThetaXZ * tanThetaXZ + tanThetaYZ * tanThetaYZ);

        return tanTheta2;
    }
}

#pragma dont_inline on
void zNPCHelper::RotateWorldToLocal(xVec3& out, const xVec3& v,
                                    const xVec3& dir) {
    out.x = dir.z * v.x - dir.x * v.z;
    out.y = v.y;
    out.z = dir.x * v.x + dir.z * v.z;
}
#pragma dont_inline off

float zNPCHelper::GetTanThetaXZ(const xVec3* npcPos, const xVec3* npcAt,
                                const xVec3* targetPos) {
    xVec3 diffVector;

    diffVector.Sub(*targetPos, *npcPos);

    xVec3 diffLocal;

    RotateWorldToLocal(diffLocal, diffVector, *npcAt);

    float oneOverZ;

    if (diffLocal.z >= 0.0f) {
        oneOverZ = diffLocal.z < 0.00001f ? 100000.0f : 1.0f / diffLocal.z;

        float tanThetaXZ = diffLocal.x * oneOverZ;

        if (tanThetaXZ < 0.0f) {
            tanThetaXZ = tanThetaXZ * -1.0f;
        }

        return tanThetaXZ;
    } else {
        oneOverZ = diffLocal.z > -0.00001f ? -100000.0f : 1.0f / diffLocal.z;

        float tanThetaXZ = diffLocal.x * oneOverZ;

        if (tanThetaXZ > 0.0f) {
            tanThetaXZ = tanThetaXZ * -1.0f;
        }

        return tanThetaXZ;
    }
}
