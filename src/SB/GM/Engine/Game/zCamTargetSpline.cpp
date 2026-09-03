// zCamTargetSpline.cpp -- six functions, read from the image with
// tools/disasm.py. A camera that follows a spline while keeping a target
// framed. create runs zCam2Player's and raises the camera's priority to
// 129. stop runs xCam's and drops the curve's back-pointer to its camera.
// start runs zCam2Player's, hands the spline mix the viewport index so it
// can find its paths, applies the current bead's values un-lazily, resets
// the basis, and then -- only when the bead asked for a Y offset --
// shifts both the collision and the live spatial height by it and rebuilds
// the matrix. pre_update saves the spatial info and the matrix for the
// frame. ApplyBeadValues pushes one bead's numbers into the camera: the
// secondary target and its radius and margin, the Y offset goal, the rest
// configuration scaled and offset by the bead, the player constrainer's
// two wall scales, and the field of view. update walks the follower to the
// viewport's midpoint, springs the path position toward the next curve
// point, applies the bead lazily, runs zCam2Player's update, springs the
// Y offset while it or its goal is still live, and copies the curve
// asset's depth-of-field block across before rebuilding the matrix.
//
// Layouts from the DWARF (tools/dwarf_types.py): zCamTargetSpline 0x518
// on TWO bases -- zCam2Player at +0 (0x408, whose own data ends at 0x404)
// and zCamSplineCommonMix at +0x404, in that tail padding -- then
// oldSpatial at +0x490, oldMat at +0x4C0 and mYOffset at +0x500. xCam is
// 0x148 with viewportIndex at +0x98, spatial at +0xC0, coll_spatial at
// +0xF0, cfg_common at +0x120 and the six dof fields at +0x128..+0x13C;
// zCam2Player has mFirstFrame at +0x17C and rest_config is nested in it.
// zCamSplineCommonMix is 0x8C: its own vtable pointer at +0, mCurveInfo
// at +4, mCurveFollower (zCameraCurve::Follower, 0x5C) at +8 and
// mCameraPathPosSprung at +0x64, so the bead info sits at +0x414 of the
// camera and the sprung path position at +0x468. zCameraCurve is 0x58
// with mAsset at +0x3C and mCamera at +0x54; zCameraCurveAsset carries
// dofEnabled at +0x60 and its five floats at +0x64..+0x74. The three
// literals are 0.0f, 0.017453292f and 1e-5f, read out of the image at
// 806893B8, 80689618 and 806894F8.
//
// The vtable is the image's, __vt__16zCamTargetSpline at 806B9A30: 0x68
// bytes, so the primary table runs to slot 19 and a SECOND table starts
// at +0x58 with SetCurves and a @1028@ adjustor thunk -- which is what
// says zCamSplineCommonMix is a base and not a member. Slot 18 is
// reset__11zCam2PlayerFv, and that is the one virtual this unit calls;
// xCam's own table is 0x50 bytes, so it owns slots 0..17 and zCam2Player
// adds 18 and 19. The eighteen placeholders are declared and never
// defined, so no vtable lands in our object.
//
// Six shapes the bytes fixed, all six functions byte-identical.
//
// The height block of start and the spring block of update are guarded by
// tests on the Y offset, and update's is an OR of two of them -- the goal
// being non-zero, or the current value still bigger than 1e-5f -- which is
// the short-circuit pair of branches retail has. The absolute value is
// __fabs's double rounded back to float, which is where the frsp between
// the fabs and the compare comes from.
//
// update's dof copy reads mCurveInfo once and mCurveInfo->mAsset six
// times. The compiler can tell two offsets off `this` apart and cannot
// tell a store through `this` from one through mAsset, so the plain
// member spelling gives exactly that, with no local anywhere.
//
// The yaw offset adds two scaled bead angles, and the one written SECOND
// is the one that gets the fmuls: retail multiplies the yaw offset there
// and folds the lead offset into the fmadds, so the source reads
// `K * mLeadOffset + K * mYawOffset`. Written the other way round the two
// loads swap, which is two words.
//
// update's Y-offset block needs THREE named locals and the file says so
// because the DWARF disagrees. `dwarf_locals.py` gives update only this,
// scene and dt -- no local at all -- yet without `goal` the compare's
// register pair comes out f1/f0 where retail has f0/f1, and without `cur`
// and `g` the four loads of the two height adds take f3,f2,f1,f0 where
// retail takes f2,f1,f3,f0: two words and six words respectively, and
// eleven other spellings of the block (compound assignment, both operand
// orders, a reference to each height, a reference to the springy, the two
// statements swapped) all leave six. What WOULD reconcile the two -- an
// inlined one-expression helper, whose parameters this producer records
// nowhere, since it emits no DW_TAG_inlined_subroutine -- was measured
// and does not: `AddHeight(xCamSpatialInfo&, float)` and a two-argument
// predicate are NOT inlined under these flags (unitcmp reports them as
// EXTRA functions retail lacks) and leave 38 of 79 and 49 of 72 words.
// So the locals are what the bytes say, and the DWARF's silence about
// them is unexplained rather than explained away.
//
// ApplyBeadValues' locals DO show up: frame +16, +12 and +8, which is
// declaration order with the rest_config first, and dwarf_locals.py names
// them newRest, nearWall and farWall.

#include "SB/GM/Engine/Game/zCamTargetSpline.pool.h"

class xScene;
class xCamBlend;
class xCamGroup;
class CurveEntity;
class zCamPoolBase;
class zCameraCurveAsset;

typedef unsigned long long uid;

class xVec3 {
public:
    xVec3& operator=(const xVec3& other);

    float x;
    float y;
    float z;
};

class xVec2 {
public:
    float x;
    float y;
};

class xMat4x3 {
public:
    xMat4x3& operator=(const xMat4x3& other);

    unsigned char _pad0[0x40];
};

// The coord union's three variants (cart, cylinder, sphere) all open with
// an xVec3, so +4 of the coord is that vector's y whichever is live.
class xCamCoord {
public:
    xVec3 origin;
    unsigned char _pad0[0x20 - 0xC];
};

class xCamSpatialInfo {
public:
    xCamSpatialInfo& operator=(const xCamSpatialInfo& other);

    xCamCoord coord;
    unsigned char _pad0[0x30 - 0x20];
};

class xCamConfigCommon {
public:
    unsigned char priority;
    unsigned char pad1;
    unsigned char pad2;
    unsigned char pad3;
    float blend_time;
};

class xSpringy {
public:
    float mResponse;
};

class xSpringyVec3 : public xSpringy {
public:
    void Update(float dt);

    xVec3 mVelocity;
    xVec3 mGoal;
    xVec3 mCurrent;
};

class xSpringyF32 : public xSpringy {
public:
    void Reset();
    void Update(float dt);

    float mVelocitySaveMax;
    float mVelocityMax;
    float mVelocity;
    float mGoal;
    float mCurrent;
};

// Eighteen virtuals, slots 0..17, so zCam2Player's reset lands at slot 18
// and the call to it reads +80 of the table.
class xCam {
public:
    virtual void _v0();
    virtual void _v1();
    virtual void _v2();
    virtual void _v3();
    virtual void _v4();
    virtual void _v5();
    virtual void _v6();
    virtual void _v7();
    virtual void _v8();
    virtual void _v9();
    virtual void _v10();
    virtual void _v11();
    virtual void _v12();
    virtual void _v13();
    virtual void _v14();
    virtual void _v15();
    virtual void _v16();
    virtual void _v17();

    void stop();
    void refresh_mat();

    uid transitionID;
    xMat4x3 mat;
    xMat4x3 coll_mat;
    float fov;
    int flags;
    int viewportIndex;
    uid owner;
    xCamGroup* group;
    xCam* next;
    xVec2 analog;
    int coord_type;
    int orient_type;
    xCamSpatialInfo spatial;
    xCamSpatialInfo coll_spatial;
    xCamConfigCommon cfg_common;
    bool dofEnabled;
    float dofBlur;
    float dofNearFocusPoint;
    float dofNearFocusFalloff;
    float dofFarFocusPoint;
    float dofFarFocusFalloff;
    int group_flags;
    xCamBlend* blender;
};

class zCam2Player : public xCam {
public:
    class rest_config {
    public:
        float dist;
        float input_center_phi;
        float yaw_offset;
        float pitch_offset;
        float roll_offset;
    };

    virtual void reset();

    void create();
    void start();
    void update(xScene& scene, float dt);
    void apply_basis();
    void set_secondary_target(const xVec3& target, float radius, float margin);
    void get_rest_default(rest_config& cfg) const;
    void set_rest(const rest_config& cfg, bool lazy, float blend);

    unsigned char _pad0[0x17C - 0x148];
    bool mFirstFrame;
    unsigned char _pad1[0x404 - 0x17D];
};

class zCameraCurve {
public:
    class BeadInfo {
    public:
        int mBeadIndex;
        float mBeadIndexFrac;
        float mCurveU[2];
        xVec3 mCurvePos[2];
        float mDistanceScale;
        float mYawOffset;
        float mPitchOffset;
        float mRollOffset;
        float mTargetRadius;
        float mTargetMarginAngle;
        float mLeadOffset;
        float mYOffset;
        float mNearWallScale;
        float mFarWallScale;
        float mFOV;
    };

    class Follower {
    public:
        void UpdatePosition(const xVec3& pos);

        bool mValidPlayerPathPos;
        zCameraCurve* mCurves;
        BeadInfo mInfo;
    };

    unsigned char _pad0[0x3C];
    zCameraCurveAsset* mAsset;
    unsigned char _pad1[0x54 - 0x40];
    xCam* mCamera;
};

class zCameraCurveAsset {
public:
    unsigned char _pad0[0x60];
    bool dofEnabled;
    unsigned char _pad1[0x64 - 0x61];
    float dofBlur;
    float dofNearFocusPoint;
    float dofNearFocusFalloff;
    float dofFarFocusPoint;
    float dofFarFocusFalloff;
};

// The second base. Its four-byte head is its own vtable pointer: the
// image gives it a table of SetCurves and a destructor.
class zCamSplineCommonMix {
public:
    virtual void SetCurves(int index, zCameraCurve* curves);
    virtual void _v1();

    void InitPaths(int viewportIndex);

    zCameraCurve* mCurveInfo;
    zCameraCurve::Follower mCurveFollower;
    xSpringyVec3 mCameraPathPosSprung;
};

class zPlayerConstrainer {
public:
    static void GetDefaults(float& nearWall, float& farWall);
    static void SetConstraints(float nearWall, float farWall);
};

class zCam;

class zCam {
public:
    unsigned char _pad0[0x234];
    xVec3 cameraPlayersMidpoint;
};

zCam* zViewportGetCamera(int viewportIndex);

class zCamTargetSpline : public zCam2Player, public zCamSplineCommonMix {
public:
    void create();
    void stop();
    void start();
    void pre_update(xScene& scene);
    void ApplyBeadValues(bool lazy);
    void update(xScene& scene, float dt);

    xCamSpatialInfo oldSpatial;
    xMat4x3 oldMat;
    xSpringyF32 mYOffset;
};

void zCamTargetSpline::create() {
    zCam2Player::create();
    cfg_common.priority = 129;
}

void zCamTargetSpline::stop() {
    xCam::stop();
    mCurveInfo->mCamera = 0;
}

void zCamTargetSpline::start() {
    zCam2Player::start();
    InitPaths(viewportIndex);
    ApplyBeadValues(false);
    reset();
    apply_basis();
    refresh_mat();
    mFirstFrame = true;
    mYOffset.Reset();
    if (mYOffset.mGoal != 0.0f) {
        coll_spatial.coord.origin.y = coll_spatial.coord.origin.y + mYOffset.mGoal;
        spatial.coord.origin.y = coll_spatial.coord.origin.y;
        refresh_mat();
    }
}

void zCamTargetSpline::pre_update(xScene& scene) {
    oldSpatial = spatial;
    oldMat = mat;
}

void zCamTargetSpline::ApplyBeadValues(bool lazy) {
    set_secondary_target(mCameraPathPosSprung.mCurrent,
                         mCurveFollower.mInfo.mTargetRadius,
                         0.017453292f * mCurveFollower.mInfo.mTargetMarginAngle);

    mYOffset.mGoal = mCurveFollower.mInfo.mYOffset;

    rest_config newRest;
    get_rest_default(newRest);
    newRest.dist = newRest.dist * mCurveFollower.mInfo.mDistanceScale;
    newRest.yaw_offset = newRest.yaw_offset +
                         (0.017453292f * mCurveFollower.mInfo.mLeadOffset +
                          0.017453292f * mCurveFollower.mInfo.mYawOffset);
    newRest.input_center_phi = newRest.input_center_phi +
                               0.017453292f * mCurveFollower.mInfo.mPitchOffset;
    newRest.roll_offset =
        newRest.roll_offset + 0.017453292f * mCurveFollower.mInfo.mRollOffset;
    set_rest(newRest, lazy, 0.0f);

    float nearWall, farWall;
    zPlayerConstrainer::GetDefaults(nearWall, farWall);
    nearWall = nearWall * mCurveFollower.mInfo.mNearWallScale;
    farWall = farWall * mCurveFollower.mInfo.mFarWallScale;
    zPlayerConstrainer::SetConstraints(nearWall, farWall);
    fov = mCurveFollower.mInfo.mFOV;
}

void zCamTargetSpline::update(xScene& scene, float dt) {
    mCurveFollower.UpdatePosition(
        zViewportGetCamera(viewportIndex)->cameraPlayersMidpoint);
    mCameraPathPosSprung.mGoal = mCurveFollower.mInfo.mCurvePos[1];
    mCameraPathPosSprung.Update(dt);

    ApplyBeadValues(true);
    zCam2Player::update(scene, dt);
    float goal = mYOffset.mGoal;
    if (goal != 0.0f || (float)__fabs(mYOffset.mCurrent) > 1e-5f) {
        mYOffset.Update(dt);
        float cur = mYOffset.mCurrent;
        spatial.coord.origin.y = spatial.coord.origin.y + cur;
        float g = mYOffset.mGoal;
        coll_spatial.coord.origin.y = coll_spatial.coord.origin.y + g;
    }

    dofEnabled = mCurveInfo->mAsset->dofEnabled;
    dofBlur = mCurveInfo->mAsset->dofBlur;
    dofNearFocusPoint = mCurveInfo->mAsset->dofNearFocusPoint;
    dofNearFocusFalloff = mCurveInfo->mAsset->dofNearFocusFalloff;
    dofFarFocusPoint = mCurveInfo->mAsset->dofFarFocusPoint;
    dofFarFocusFalloff = mCurveInfo->mAsset->dofFarFocusFalloff;
    refresh_mat();
}
