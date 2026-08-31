// C:/branches/SB09/main/GM/Engine/Game/zCamSplineCommonMix.cpp
//
// Layout from the Wii build's DWARF (tools/dwarf_types.py):
//
//   class zCamSplineCommonMix  /* 0x8C */
//   {
//       /* +0x0  */ (vtable pointer -- not a DWARF member)
//       /* +0x4  */ zCameraCurve* mCurveInfo;
//       /* +0x8  */ Follower mCurveFollower;          // 0x5C
//       /* +0x64 */ xSpringyVec3 mCameraPathPosSprung; // 0x28
//   };
//   class xSpringyVec3 /* 0x28 */ { xSpringy _base0; xVec3 mVelocity;
//                                   xVec3 mGoal; xVec3 mCurrent; };
//
// which resolves every address the code forms:
//
//   addi r3, r31, 8      -> mCurveFollower
//   addi r3, r31, 0x74   -> 0x64 + 0x10 = mCameraPathPosSprung.mGoal
//   addi r4, r31, 0x2c   -> 0x08 + 0x24 = a vector inside the Follower
//   addi r3, r31, 0x64   -> mCameraPathPosSprung
//
// SetCurves stores its SECOND argument and tail-calls with the first still
// in r4 untouched, which is what says InitPaths takes the int rather than
// anything derived from the curve.

struct xVec3 {
    float x, y, z;
    xVec3& operator=(const xVec3& o);
};

struct xSpringy {
    unsigned char _bytes[0x4];
};

struct xSpringyVec3 {
    xSpringy _base0;
    xVec3 mVelocity;
    xVec3 mGoal;
    xVec3 mCurrent;

    void Reset();
};

// Follower is NESTED in zCameraCurve. The DWARF gives leaf names only, so
// it reads as a bare `Follower`, and declaring it that way produced an
// object objdiff scored at 100% -- every instruction identical -- that the
// linker then refused:
//
//   undefined: 'Follower::Init(const zCameraCurve&,const xVec3&)'
//
// against the real Init__Q212zCameraCurve8FollowerFRC12zCameraCurveRC5xVec3.
// Matching instructions is not the same as naming the right symbol, and
// objdiff scores the first.
class zCameraCurve {
public:
    struct Follower {
        unsigned char _head[0x24];
        xVec3 mPos;
        unsigned char _tail[0x5C - 0x24 - 0xC];

        void Init(const zCameraCurve& curve, const xVec3& at);
    };
};

struct xCamera {
    unsigned char _head[0x234];
    xVec3 mPos;
};

xCamera* zViewportGetCamera(int viewport);

class zCamSplineCommonMix {
public:
    void SetCurves(int count, zCameraCurve* curves);
    void InitPaths(int viewport);

    unsigned char _vt[0x4];
    zCameraCurve* mCurveInfo;
    zCameraCurve::Follower mCurveFollower;
    xSpringyVec3 mCameraPathPosSprung;
};

void zCamSplineCommonMix::SetCurves(int count, zCameraCurve* curves) {
    mCurveInfo = curves;
    InitPaths(count);
}

void zCamSplineCommonMix::InitPaths(int viewport) {
    xCamera* cam = zViewportGetCamera(viewport);

    mCurveFollower.Init(*mCurveInfo, cam->mPos);
    mCameraPathPosSprung.mGoal = mCurveFollower.mPos;
    mCameraPathPosSprung.Reset();
}
