#include "SB/GM/Engine/Core/x/xCam.pool.h"

// xCam.cpp -- the camera base, the camera group, the shake, the time bias
// and the blend camera. Taken over from tools/gen_accessors.py (its two
// functions were xCamTimeBias::GetBias and xCamBlend::GetTargets); the
// layouts are the Wii DWARF's, the virtual slots retail's tables
// (vtable.py __vt__4xCam, __vt__9xCamBlend, __vt__12xCamTimeBias,
// __vt__11zCam2Player for slot 11's name).
//
// No vtable is emitted: each class's first declared virtual is its
// destructor, which this unit does not define. The weak virtuals retail
// keeps here (xCam::get_next, xCam::find_camera) are therefore defined out
// of line. The weak inlines retail calls (xQuatToLookVec and the rest) are
// defined at the foot of the file, below every caller.
//
// Not written, walls (brief): xCamShake::Update (nine float literals),
// xrmod (four), CalculateAABB (@unnamed@WAD00_cpp@), CreatePhantom and
// UpdatePhantom (call it), xCamGroup::create_blend (PushMemory on
// @unnamed@WAD00_cpp@'s watermark).

class xScene;
class xCam;
class xCamBlend;
class xCamGroup;
class xCamTransition;
class xCamBias;
class xCamTimeBias;
class hkpAabbPhantom;

extern "C" {
double sin(double x);
double cos(double x);
double atan2(double y, double x);
}

void operator delete(void* p);

enum eMemMgrTag {
    eMemMgrTag_ = 0x7FFFFFFF
};

namespace Memory {

enum GlobalHeapEnum {
    GlobalHeapCached = 0,
    GlobalHeapUncached = 1,
    GlobalHeapDefault = 2
};

void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap, eMemMgrTag tag,
                      bool zeroed);
void FreeGlobalHeap(void* block, GlobalHeapEnum heap);

}  // namespace Memory

inline void* operator new(unsigned long size, Memory::GlobalHeapEnum heap,
                          eMemMgrTag tag)
{
    return Memory::AllocGlobalHeap(size, heap, tag, false);
}

// const H& -- the heap constant binds as an unnamed 4-byte static, loaded
// before the null test (WAD00_1.cpp's measurement; @114508 here).
template <class H>
inline void Free(const H& heap, void* p)
{
    H h = heap;

    if (p != 0) {
        Memory::FreeGlobalHeap(p, h);
    }
}

// ---------------------------------------------------------------------------
// Math

class xVec2 {
public:
    // The assignment is left implicit: mwcc synthesises it and emits it out
    // of line as __as__5xVec2FRC5xVec2, which retail has in this unit.
    void assign(float ax, float ay);

    float x;
    float y;
};

class xVec3 {
public:
    xVec3& operator*=(float s);
    float length2() const;

    float x;
    float y;
    float z;
};

class xMat3x3 {
public:
    xMat3x3& operator=(const xMat3x3& other);

    xVec3 right;
    int flags;
    xVec3 up;
    unsigned int pad1;
    xVec3 at;
    unsigned int pad2;
};

class xMat4x3 : public xMat3x3 {
public:
    xMat4x3& operator=(const xMat4x3& other);

    xVec3 pos;
    unsigned int pad3;
};

class xQuat {
public:
    xVec3 v;
    float s;
};

extern const xMat4x3 g_I3;

void xQuatToMat(const xQuat* q, xMat3x3* m);
void xQuatFromMat(xQuat* q, const xMat3x3* m);
void xQuatSlerp(xQuat* o, const xQuat* a, const xQuat* b, float t);
void xMat3x3Euler(xMat3x3* m, float yaw, float pitch, float roll);
float xMat3x3LookVec(xMat3x3* m, const xVec3* at);
float xAngleClampFast(float angle);
float xrmod(float x);

// Weak in the image: called, never inlined, so defined below their callers.
inline void xQuatToLookVec(const xQuat* q, xVec3* o);
inline void v3normalizexz(float& len, xVec3* o, const xVec3* v);
void v3sub(xVec3* o, xVec3* a, xVec3* b);
inline float xatan2(float y, float x);

namespace Math {
float rsqrt(float x);
}

// xVec3's assignment is mwcc's synthesised one, out of line in an earlier
// file of retail's unity build: reached by its symbol so this unit
// emits no copy of it.
extern "C" xVec3* __as__5xVec3FRC5xVec3(xVec3* self, const xVec3* other);

// A vector built in place from three floats: the image names Math::Vector's
// constructor for it, the one the linker kept.
extern "C" void __ct__Q24Math6VectorFfff(void* self, float x, float y,
                                         float z);

// ---------------------------------------------------------------------------
// Engine types this unit reaches

namespace Graphics {

class Viewport;

class DoFEffectSettings {
public:
    bool enabled;
    float blurAmt;
    float nearPlaneStart;
    float nearPlaneEnd;
    float farPlaneStart;
    float farPlaneEnd;
};

extern DoFEffectSettings GlobalDoFSettings;

}  // namespace Graphics

namespace Sext {

// 0x50 in the DWARF; the transition's blend time opens its type data.
class transition_time {
public:
    unsigned char _pad0[0x38];
    float blendTime;
};

}  // namespace Sext

class zPlayerConstrainer {
public:
    static xVec3 GetPlayerCenter();
    static void AdjustCameraPosition(xMat4x3& mat);
    static void AdjustCameraMatrix(xMat4x3& mat);
};

// 0x18.
class zGlobalSettings {
public:
    unsigned short AnalogMin;
    unsigned short AnalogMax;
    unsigned int TakeDamage;
    float DamageInvincibility;
    float Gravity;
    unsigned char AttractModeDuringGameplay;
    unsigned char DisableScaleformRenderingInMasterDuringCinematics;
    unsigned char ShowTimecodeDuringCinematics;
    unsigned char pad;
    float CameraFOV;
};

// 0x5A0; only the settings this unit reads.
class zGlobals {
public:
    unsigned char _pad0[0x4E8];
    zGlobalSettings settings;
    unsigned char _pad1[0x5A0 - 0x500];
};

extern zGlobals globals;
extern float SECS_PER_VBLANK;

// 0x48.
class iCamera {
public:
    void SetViewport(Graphics::Viewport* viewport);
    iCamera()
    {
        camViewport = 0;
        frame = g_I3;
    }

    void SetFOV(float fov);

    Graphics::Viewport* camViewport;
    float fov;
    xMat4x3 frame;
};

// ---------------------------------------------------------------------------
// Camera coordinates

enum xCamCoordType {
    XCAM_COORD_INVALID = -1,
    XCAM_COORD_CART = 0,
    XCAM_COORD_CYLINDER = 1,
    XCAM_COORD_SPHERE = 2,
    XCAM_COORD_MAX = 3
};

enum xCamOrientType {
    XCAM_ORIENT_INVALID = -1,
    XCAM_ORIENT_QUAT = 0,
    XCAM_ORIENT_EULER = 1,
    XCAM_ORIENT_MAX = 2
};

class xCamCoordCylinder {
public:
    xVec3 origin;
    float dist;
    float height;
    float theta;
};

class xCamCoordSphere {
public:
    xVec3 origin;
    float dist;
    xQuat dir;
};

union xCamCoord {
    xVec3 cart;
    xCamCoordCylinder cylinder;
    xCamCoordSphere sphere;
};

class xCamOrientEuler {
public:
    float yaw;
    float pitch;
    float roll;
};

union xCamOrient {
    xQuat quat;
    xCamOrientEuler euler;
};

class xCamSpatialInfo {
public:
    xCamCoord coord;
    xCamOrient orient;
};

class xCamConfigCommon {
public:
    unsigned char priority;
    unsigned char pad1;
    unsigned char pad2;
    unsigned char pad3;
    float blend_time;
};

class xCamTransitionParams : public xCamConfigCommon {
public:
    xCamTransition* mTransitionObject;
};

// ---------------------------------------------------------------------------
// The camera

// 0x148, vtable pointer at +0 (__vt__4xCam, 18 slots).
class xCam {
public:
    virtual ~xCam();
    virtual const xCam& get_final_dest() const;
    virtual void _v2();
    virtual void _v3();
    virtual void create();
    virtual void destroy();
    virtual void start();
    virtual void stop();
    virtual void update(xScene& scene, float dt);
    virtual void pre_update(xScene& scene);
    virtual void post_update(xScene& scene);
    virtual void FinalAdjust(float dt);
    virtual xCam* get_next();
    virtual xCam* find_camera(unsigned long long ownerID);
    virtual void* get_zCam2Player() const;
    virtual void GetTargets(xVec3* Pos0, xVec3* Pos1) const;
    virtual bool ConstrainMatrix() const;
    virtual bool ConstrainPosition() const;

    void refresh_mat(xMat4x3& mat, const xCamSpatialInfo& spatial);
    void refresh_mat();
    void refresh_dof_effect();
    xCam* do_transient_replacement();

    // xCam.h's blends and conversions: weak, called out of line.
    static inline void blend(float& o, float a, float b, float s);
    static inline void blend_radian(float& o, float a, float b, float s);
    static inline void blend(xVec3& o, const xVec3& a, const xVec3& b,
                             float s);
    static inline void blend(xCamCoordCylinder& o, const xCamCoordCylinder& a,
                             const xCamCoordCylinder& b, float s);
    static inline void blend(xCamCoordSphere& o, const xCamCoordSphere& a,
                             const xCamCoordSphere& b, float s);
    static void blend(xCamOrientEuler& o, const xCamOrientEuler& a,
                      const xCamOrientEuler& b, float s);
    static inline xCamCoordCylinder& convert(xCamCoordCylinder& o,
                                             const xCamCoordSphere& i);
    static inline xCamCoordSphere& convert(xCamCoordSphere& o,
                                           const xCamCoordCylinder& i);

    // One source line each in xCamBlend::stop, with the camera held once.
    void detach_blender()
    {
        group_flags &= ~0x10;
        blender = 0;
    }

    // xCamGroup::update's one line of three table calls on the camera.
    void update_all(xScene& scene, float dt)
    {
        pre_update(scene);
        update(scene, dt);
        post_update(scene);
    }

    unsigned long long transitionID;
    xMat4x3 mat;
    xMat4x3 coll_mat;
    float fov;
    int flags;
    int viewportIndex;
    unsigned long long owner;
    xCamGroup* group;
    xCam* next;
    xVec2 analog;
    xCamCoordType coord_type;
    xCamOrientType orient_type;
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

// 0x88.
class xCamShake {
public:
    void Start(float maxTime_, float magnitude_, float cycleMax_,
               float rotate_magnitude_, float radius_,
               const xVec3* epicenter_, const xVec3* player_, bool vertical);
    void Update(float dt, xMat4x3& m);

    bool isActive;
    float elapsedTime;
    float maxTime;
    float magnitude;
    xVec3 dir;
    float cycleTime;
    float cycleMax;
    float dampen;
    float dampenRate;
    float rotate_magnitude;
    xVec3* epicenterP;
    xVec3 epicenter;
    xVec3* player;
    float radius;
    xMat4x3 mat;
};

// 0x158.
class xCamGroup {
public:
    void create(xCamGroup* parent);
    void destroy();
    void reset();
    void update(xScene& scene, float dt);
    bool transition_to(xCam& cam, const xCamTransitionParams* params,
                       bool cut, bool force);
    void stop(xCam& cam);
    xCamBlend* grab_blend_cam();
    void set_primary(xCam* toCamera, bool force);
    xCam* get_blend(xCamTransition* transition, xCam& to);
    void CreatePhantom();
    void UpdatePhantom();
    xCamBlend* create_blend();

    iCamera* iCam;
    bool isExternalICam;
    int viewportIndex;
    Graphics::Viewport* viewport;
    xCamShake shake;
    bool isCreated;
    float fov;
    float fov_default;
    xMat4x3 mat;
    xMat4x3 coll_mat;
    xVec3 coll_atXZ;
    hkpAabbPhantom* phantomCamera;
    xCam* primary;
    xVec2 analog;
    int child_flags;
    int child_flags_mask;
    xCamBlend* blend_cam[4];
};

// 0x8, vtable pointer at +0 (__vt__12xCamTimeBias's first seven slots).
class xCamBias {
public:
    virtual ~xCamBias();
    virtual void Start(int viewport);
    virtual void Update(float dt);
    virtual float GetBias();
    virtual void Reverse();
    virtual bool IsActive();
    virtual bool DidComplete();

    static xCamBias* GetBiasObject(xCamTransition* transition,
                                   xCamTimeBias& timeBias);

    bool mIsBiDirectional;
};

// 0x14.
class xCamTimeBias : public xCamBias {
public:
    virtual ~xCamTimeBias();
    virtual void Start(int viewport);
    virtual void Update(float dt);
    virtual float GetBias();
    virtual void Reverse();
    virtual bool IsActive();
    virtual bool DidComplete();

    void CalcBias();

    float bias;
    float time;
    float blendTime;
};

// 0x170.
class xCamBlend : public xCam {
public:
    virtual ~xCamBlend();
    virtual void create();
    virtual void start();
    virtual void stop();
    virtual void update(xScene& scene, float dt);
    virtual void pre_update(xScene& scene);
    virtual void post_update(xScene& scene);
    virtual xCam* get_next();
    virtual xCam* find_camera(unsigned long long ownerID);
    virtual void GetTargets(xVec3* Pos0, xVec3* Pos1) const;

    void attach(xCam& s, xCam& d);
    void reverse();
    void blend_coord(float s);
    void blend_orient(float s);

    inline const xCamCoordCylinder& convert(const xCam& c,
                                            xCamCoordCylinder& o) const;
    inline const xCamCoordSphere& convert(const xCam& c,
                                          xCamCoordSphere& o) const;
    const xQuat& convert(const xCam& c, xQuat& o) const;
    const xCamOrientEuler& convert(const xCam& c, xCamOrientEuler& o) const;
    inline const xCamCoordCylinder& coll_convert(const xCam& c,
                                                 xCamCoordCylinder& o) const;
    inline const xCamCoordSphere& coll_convert(const xCam& c,
                                               xCamCoordSphere& o) const;
    const xQuat& coll_convert(const xCam& c, xQuat& o) const;
    const xCamOrientEuler& coll_convert(const xCam& c,
                                        xCamOrientEuler& o) const;

    xCam* src;
    xCam* dst;
    xCamBias* bias;
    xCamTimeBias timeBias;
    xCamTransition* transition;
};

// 0x58; xOGEntity's id at +0x18.
class xCamTransition {
public:
    bool IsCut();
    void Completed(bool success);

    unsigned char _pad0[0x18];
    unsigned long long id;
    unsigned char _pad1[0x3C - 0x20];
    Sext::transition_time* mAsset;
    void* mTransitionVolume;
    int mType;
    bool mActive;
    xCamBias* mCamBlendBias;
    xCamGroup* mCamGroup;
};

// ---------------------------------------------------------------------------
// xCam

void xCam::start()
{
    flags |= 0x1;
}

void xCam::stop()
{
    flags &= ~0x1;
}

void xCam::refresh_mat()
{
    refresh_mat(mat, spatial);

    if (flags & 0x80) {
        refresh_mat(coll_mat, coll_spatial);
    } else {
        coll_mat = mat;
        coll_spatial.coord = spatial.coord;
        coll_spatial.orient = spatial.orient;
    }
}

inline float isin(float x)
{
    return (float)sin(x);
}

inline float icos(float x)
{
    return (float)cos(x);
}

void xCam::refresh_mat(xMat4x3& mat, const xCamSpatialInfo& spatial)
{
    switch (coord_type) {
    case XCAM_COORD_CART:
        __as__5xVec3FRC5xVec3(&mat.pos, &spatial.coord.cart);
        break;
    case XCAM_COORD_CYLINDER:
        mat.pos.x = spatial.coord.cylinder.origin.x +
                    spatial.coord.cylinder.dist *
                        isin(spatial.coord.cylinder.theta);
        mat.pos.y =
            spatial.coord.cylinder.origin.y + spatial.coord.cylinder.height;
        mat.pos.z = spatial.coord.cylinder.origin.z +
                    spatial.coord.cylinder.dist *
                        icos(spatial.coord.cylinder.theta);
        break;
    case XCAM_COORD_SPHERE: {
        xVec3 dir;
        xQuatToLookVec(&spatial.coord.sphere.dir, &dir);
        mat.pos.x = spatial.coord.sphere.origin.x +
                    spatial.coord.sphere.dist * dir.x;
        mat.pos.y = spatial.coord.sphere.origin.y +
                    spatial.coord.sphere.dist * dir.y;
        mat.pos.z = spatial.coord.sphere.origin.z +
                    spatial.coord.sphere.dist * dir.z;
        break;
    }
    }

    switch (orient_type) {
    case XCAM_ORIENT_QUAT:
        xQuatToMat(&spatial.orient.quat, &mat);
        break;
    case XCAM_ORIENT_EULER:
        xMat3x3Euler(&mat, spatial.orient.euler.yaw, spatial.orient.euler.pitch,
                     spatial.orient.euler.roll);
        break;
    }
}

void xCam::refresh_dof_effect()
{
    Graphics::GlobalDoFSettings.enabled = dofEnabled;
    Graphics::GlobalDoFSettings.blurAmt = dofBlur;
    Graphics::GlobalDoFSettings.nearPlaneStart =
        dofNearFocusPoint - dofNearFocusFalloff;
    Graphics::GlobalDoFSettings.nearPlaneEnd = dofNearFocusPoint;
    Graphics::GlobalDoFSettings.farPlaneStart = dofFarFocusPoint;
    Graphics::GlobalDoFSettings.farPlaneEnd =
        dofFarFocusPoint + dofFarFocusFalloff;
}

void xCam::GetTargets(xVec3* Pos0, xVec3* Pos1) const
{
    const xVec3& center = zPlayerConstrainer::GetPlayerCenter();
    __as__5xVec3FRC5xVec3(Pos0, &center);
    __ct__Q24Math6VectorFfff(Pos1, Pos0->x, Pos0->y, Pos0->z);
}

void xCam::create()
{
    flags = 0;
    coord_type = XCAM_COORD_INVALID;
    orient_type = XCAM_ORIENT_INVALID;
    group = 0;
    viewportIndex = 0;

    dofEnabled = false;
    dofBlur = 1.0f;
    dofNearFocusPoint = 0.0f;
    dofNearFocusFalloff = 0.0f;
    dofFarFocusPoint = 10000.0f;
    dofFarFocusFalloff = 0.0f;
    transitionID = 0;
}

// ---------------------------------------------------------------------------
// xCamShake

void xCamShake::Start(float maxTime_, float magnitude_, float cycleMax_,
                      float rotate_magnitude_, float radius_,
                      const xVec3* epicenter_, const xVec3* player_,
                      bool vertical)
{
    maxTime = maxTime_;
    magnitude = magnitude_;

    if (vertical) {
        dir.x = 0.0f;
        dir.y = 1.0f;
    } else {
        dir.x = 1.0f;
        dir.y = 0.0f;
    }

    cycleMax = cycleMax_;
    cycleTime = 0.0f;
    dampen = 0.0f;
    elapsedTime = 0.0f;
    dampenRate = 1.0f / maxTime_;
    rotate_magnitude = rotate_magnitude_;

    radius = radius_;
    epicenterP = (xVec3*)epicenter_;
    if (epicenter_) {
        __as__5xVec3FRC5xVec3(&epicenter, epicenter_);
    }
    player = (xVec3*)player_;

    isActive = true;
}

// ---------------------------------------------------------------------------
// xCamGroup

void xCamGroup::create(xCamGroup* parent)
{
    if (parent) {
        isExternalICam = true;
        iCam = parent->iCam;
        viewport = parent->viewport;
        viewportIndex = parent->viewportIndex;
        fov_default = iCam->fov;
    } else {
        isExternalICam = false;

        if (iCam) {
            Free(Memory::GlobalHeapCached, iCam);
        }

        iCam = new (Memory::GlobalHeapCached, (eMemMgrTag)4) iCamera;
        iCam->SetViewport(viewport);
        fov_default = globals.settings.CameraFOV;
        iCam->SetFOV(fov_default);
    }

    for (xCamBlend** it = blend_cam, **end = it + 4; it != end; ++it) {
        *it = create_blend();
    }

    isCreated = true;
}

void xCamGroup::destroy()
{
    if (primary) {
        stop(*primary);
        primary = 0;
    }

    for (xCamBlend** it = blend_cam, **end = it + 4; it != end; ++it) {
        (*it)->destroy();
    }

    if (!isExternalICam) {
        delete iCam;
        iCam = 0;
    }

    isCreated = false;
}

void xCamGroup::reset()
{
    if (primary) {
        stop(*primary);
        primary = 0;
    }

    fov = fov_default;
    child_flags_mask = -1;
    child_flags = 0;

    coll_mat = g_I3;
    __as__5xVec3FRC5xVec3(&coll_atXZ, &coll_mat.at);
    mat = g_I3;
}

void xCamGroup::update(xScene& scene, float dt)
{
    primary = primary->do_transient_replacement();

    if (primary) {
        primary->analog = analog;
        primary->update_all(scene, dt);
        primary->FinalAdjust(dt);

        mat = primary->mat;
        primary->refresh_dof_effect();
        coll_mat = primary->coll_mat;

        float len;
        v3normalizexz(len, &coll_atXZ, &coll_mat.at);
        coll_atXZ.y = 0.0f;

        fov = primary->fov;

        if (dt < SECS_PER_VBLANK) {
            dt = SECS_PER_VBLANK;
        }
    }

    if (shake.isActive) {
        shake.Update(dt, mat);
    }
}

bool xCamGroup::transition_to(xCam& cam, const xCamTransitionParams* params,
                              bool cut, bool force)
{
    if (isCreated == false) {
        return false;
    }

    if (viewportIndex != cam.viewportIndex) {
        cam.viewportIndex = viewportIndex;
    }

    if (params) {
        cam.transitionID = params->mTransitionObject->id;
    } else {
        cam.transitionID = 0;
    }

    if (!cut && primary && primary->owner == cam.owner) {
        cam.stop();
        cam.transitionID = 0;
        return false;
    }

    if (cut || !params) {
        set_primary(&cam, force);
    } else {
        xCam* next = get_blend(params ? params->mTransitionObject : 0, cam);
        set_primary(next, false);
    }

    return true;
}

void xCamGroup::stop(xCam& cam)
{
    if (cam.flags & 0x1) {
        cam.stop();
    }

    cam.group = 0;
}

xCamBlend* xCamGroup::grab_blend_cam()
{
    for (xCamBlend** it = blend_cam, **end = it + 4; it != end; ++it) {
        if ((*it)->group == 0) {
            return *it;
        }
    }

    return 0;
}

void xCamGroup::set_primary(xCam* toCamera, bool force)
{
    if (!force && primary != 0 && primary == toCamera) {
        return;
    }

    if (primary != 0 && !(primary->group_flags & 0x10)) {
        stop(*primary);
        mat = primary->mat;
        coll_mat = primary->coll_mat;

        float len;
        v3normalizexz(len, &coll_atXZ, &coll_mat.at);
        coll_atXZ.y = 0.0f;
    }

    if (toCamera != 0) {
        primary = toCamera;
        toCamera->group = this;
        primary->mat = mat;
        primary->coll_mat = coll_mat;
        primary->fov = fov_default;
        primary->start();
    }
}

xCam* xCamGroup::get_blend(xCamTransition* transition, xCam& to)
{
    float blend_time = to.cfg_common.blend_time;

    if (primary != 0) {
        if (transition != 0) {
            if (transition->IsCut()) {
                goto cut;
            }
        } else if (blend_time <= 0.0f) {
            goto cut;
        }

        if (to.group_flags & 0x10) {
            xCamBlend* parent = to.blender;
            xCam* pTo = &to;

            while (parent != 0) {
                if (parent->dst != pTo) {
                    parent->reverse();
                }

                pTo = parent;
                parent = parent->blender;
            }

            return primary;
        }

        {
            xCamBlend* blend = grab_blend_cam();
            if (blend == 0) {
                return &to;
            }

            blend->blender = 0;
            blend->attach(*primary, to);
            to.group = this;

            if (transition != 0) {
                blend->bias =
                    xCamBias::GetBiasObject(transition, blend->timeBias);
            } else {
                blend->timeBias.blendTime = blend_time;
                blend->bias = &blend->timeBias;
            }

            blend->transition = transition;
            return blend;
        }
    }

cut:
    if (transition != 0) {
        transition->mActive = false;
    }

    return &to;
}

xCam* xCam::do_transient_replacement()
{
    xCam* curCam = this;
    xCam* next;

    while (curCam != 0 && (curCam->flags & 0x4) &&
           (next = curCam->get_next()) != curCam) {
        if (curCam->flags & 0x1) {
            curCam->stop();
        }

        curCam->group = 0;
        curCam = next;
    }

    return curCam;
}

// ---------------------------------------------------------------------------
// xCamTimeBias

void xCamTimeBias::Start(int)
{
    time = 0.0f;
    bias = 0.0f;
}

void xCamTimeBias::Update(float dt)
{
    time += dt;
    CalcBias();
}

void xCamTimeBias::Reverse()
{
    time = blendTime - time;
    if (time < 0.0f) {
        time = 0.0f;
    }
    CalcBias();
}

float xCamTimeBias::GetBias()
{
    return bias;
}

bool xCamTimeBias::IsActive()
{
    return time < blendTime;
}

bool xCamTimeBias::DidComplete()
{
    return time >= blendTime;
}

// NEAR MISS: 10 of 27 words; t is in f1 where retail has f2 (split
// declaration, a ternary clamp, the literal on the left and an inline
// ease helper under always_inline tie or are worse).
void xCamTimeBias::CalcBias()
{
    float t = time / blendTime;
    if (t > 1.0f) {
        t = 1.0f;
    }
    bias = (t <= 0.5f) ? 2.0f * t * t
                       : 1.0f - 2.0f * (1.0f - t) * (1.0f - t);
}

xCamBias* xCamBias::GetBiasObject(xCamTransition* transition,
                                  xCamTimeBias& timeBias)
{
    if (transition->mCamBlendBias != 0) {
        return transition->mCamBlendBias;
    }

    timeBias.blendTime = transition->mAsset->blendTime;
    return &timeBias;
}

// ---------------------------------------------------------------------------
// xCamBlend

void xCamBlend::create()
{
    xCam::create();
    flags |= 0x4;
    coord_type = XCAM_COORD_INVALID;
    orient_type = XCAM_ORIENT_INVALID;
}

void xCamBlend::start()
{
    static const xCamCoordType coord_table[XCAM_COORD_MAX][XCAM_COORD_MAX] = {
        { XCAM_COORD_CART, XCAM_COORD_CART, XCAM_COORD_CART },
        { XCAM_COORD_CART, XCAM_COORD_CYLINDER, XCAM_COORD_SPHERE },
        { XCAM_COORD_CART, XCAM_COORD_SPHERE, XCAM_COORD_SPHERE },
    };
    static const xCamOrientType
        orient_table[XCAM_ORIENT_MAX][XCAM_ORIENT_MAX] = {
            { XCAM_ORIENT_QUAT, XCAM_ORIENT_QUAT },
            { XCAM_ORIENT_QUAT, XCAM_ORIENT_EULER },
        };

    if (!(src->flags & 0x1)) {
        src->group = group;
        src->mat = mat;
        src->coll_mat = coll_mat;
        src->fov = fov;
        src->start();
    }

    if (!(dst->flags & 0x1)) {
        dst->group = group;
        dst->mat = mat;
        dst->coll_mat = coll_mat;
        dst->fov = fov;
        dst->start();
    }

    cfg_common.priority =
        (src->cfg_common.priority > dst->cfg_common.priority)
            ? src->cfg_common.priority
            : dst->cfg_common.priority;

    coord_type = coord_table[src->coord_type][dst->coord_type];
    orient_type = orient_table[src->orient_type][dst->orient_type];

    if ((src->flags | dst->flags) & 0x80) {
        flags |= 0x80;
    } else {
        flags &= ~0x80;
    }

    bias->Start(viewportIndex);
    xCam::start();
}

void xCamBlend::stop()
{
    src->detach_blender();
    dst->detach_blender();
    blender = 0;

    if (bias->DidComplete()) {
        if (bias->GetBias() < 0.5f) {
            group->stop(*dst);
        } else {
            group->stop(*src);
        }
    } else {
        group->stop(*src);
        group->stop(*dst);
    }

    if (transition) {
        transition->Completed(bias->DidComplete());
        transition = 0;
        transitionID = 0;
    }

    xCam::stop();
}

void xCamBlend::pre_update(xScene& scene)
{
    src->pre_update(scene);
    dst->pre_update(scene);
}

void xCamBlend::post_update(xScene& scene)
{
    src->post_update(scene);
    dst->post_update(scene);
}

void xCamBlend::update(xScene& scene, float dt)
{
    bias->Update(dt);
    float s = bias->GetBias();

    src = src->do_transient_replacement();
    dst = dst->do_transient_replacement();

    src->analog = analog;
    src->update(scene, dt);
    src->FinalAdjust(dt);

    dst->analog = analog;
    dst->update(scene, dt);
    dst->FinalAdjust(dt);

    blend_coord(s);
    blend_orient(s);
    blend(fov, src->fov, dst->fov, s);
    blend(dofBlur, src->dofBlur, dst->dofBlur, s);
    blend(dofNearFocusPoint, src->dofNearFocusPoint, dst->dofNearFocusPoint,
          s);
    blend(dofNearFocusFalloff, src->dofNearFocusFalloff,
          dst->dofNearFocusFalloff, s);

    if (dst->dofEnabled && src->dofEnabled) {
        blend(dofFarFocusPoint, src->dofFarFocusPoint, dst->dofFarFocusPoint,
              s);
        blend(dofFarFocusFalloff, src->dofFarFocusFalloff,
              dst->dofFarFocusFalloff, s);
        dofEnabled = true;
    } else if (dst->dofEnabled) {
        dofFarFocusPoint = dst->dofFarFocusPoint;
        dofFarFocusFalloff = dst->dofFarFocusFalloff;
        dofEnabled = true;
    } else {
        dofFarFocusPoint = dst->dofFarFocusPoint;
        dofFarFocusFalloff = dst->dofFarFocusFalloff;
        dofEnabled = false;
    }

    refresh_mat();

    if (src->ConstrainMatrix() || src->ConstrainPosition()) {
        if (dst->ConstrainPosition()) {
            zPlayerConstrainer::AdjustCameraPosition(mat);
        }

        if (dst->ConstrainPosition() || dst->ConstrainMatrix()) {
            zPlayerConstrainer::AdjustCameraMatrix(mat);
        }
    }
}

void xCamBlend::GetTargets(xVec3* Pos0, xVec3* Pos1) const
{
    dst->GetTargets(Pos0, Pos1);
}

xCam* xCamBlend::get_next()
{
    return bias->IsActive() ? this : (bias->GetBias() < 0.5f ? src : dst);
}

xCam* xCamBlend::find_camera(unsigned long long ownerID)
{
    xCam* found = src->find_camera(ownerID);
    return found != 0 ? found : dst->find_camera(ownerID);
}

void xCamBlend::attach(xCam& s, xCam& d)
{
    s.blender = d.blender = this;
    s.group_flags |= 0x10;
    d.group_flags |= 0x10;
    group_flags = 0x1;
    src = &s;
    dst = &d;
}

void xCamBlend::reverse()
{
    xCam* temp = src;
    src = dst;
    dst = temp;
    bias->Reverse();
}

void xCamBlend::blend_coord(float s)
{
    switch (coord_type) {
    case XCAM_COORD_CART:
        blend(spatial.coord.cart, src->mat.pos, dst->mat.pos, s);
        blend(coll_spatial.coord.cart, src->coll_mat.pos, dst->coll_mat.pos, s);
        break;
    case XCAM_COORD_CYLINDER: {
        xCamCoordCylinder c1;
        xCamCoordCylinder c2;
        blend(spatial.coord.cylinder, convert(*src, c1), convert(*dst, c2), s);
        blend(coll_spatial.coord.cylinder, coll_convert(*src, c1),
              coll_convert(*dst, c2), s);
        break;
    }
    case XCAM_COORD_SPHERE: {
        xCamCoordSphere c1;
        xCamCoordSphere c2;
        blend(spatial.coord.sphere, convert(*src, c1), convert(*dst, c2), s);
        blend(coll_spatial.coord.sphere, coll_convert(*src, c1),
              coll_convert(*dst, c2), s);
        break;
    }
    }
}

void xCamBlend::blend_orient(float s)
{
    switch (orient_type) {
    case XCAM_ORIENT_QUAT: {
        xQuat c1;
        xQuat c2;
        xQuatSlerp(&spatial.orient.quat, &convert(*src, c1), &convert(*dst, c2),
                   s);
        xQuatSlerp(&coll_spatial.orient.quat, &coll_convert(*src, c1),
                   &coll_convert(*dst, c2), s);
        break;
    }
    case XCAM_ORIENT_EULER: {
        xCamOrientEuler c1;
        xCamOrientEuler c2;
        blend(spatial.orient.euler, convert(*src, c1), convert(*dst, c2), s);
        blend(coll_spatial.orient.euler, coll_convert(*src, c1),
              coll_convert(*dst, c2), s);
        break;
    }
    }
}

// ---------------------------------------------------------------------------
// Weak in the image (xCam.h's virtuals, reached only through the table):
// defined out of line, since this unit emits no table to name them.

xCam* xCam::get_next()
{
    return (flags & 0x1) ? this : 0;
}

xCam* xCam::find_camera(unsigned long long ownerID)
{
    return (owner == ownerID) ? this : 0;
}

// ---------------------------------------------------------------------------
// Weak in the image and called: defined below every caller.

inline float xsqrt(float x)
{
    return x * Math::rsqrt(x);
}

inline void xCam::blend(xCamCoordCylinder& o, const xCamCoordCylinder& a,
                        const xCamCoordCylinder& b, float s)
{
    blend(o.origin, a.origin, b.origin, s);
    blend(o.dist, a.dist, b.dist, s);
    blend(o.height, a.height, b.height, s);
    blend_radian(o.theta, a.theta, b.theta, s);
}

inline void xCam::blend(xCamCoordSphere& o, const xCamCoordSphere& a,
                        const xCamCoordSphere& b, float s)
{
    blend(o.origin, a.origin, b.origin, s);
    blend(o.dist, a.dist, b.dist, s);
    xQuatSlerp(&o.dir, &a.dir, &b.dir, s);
}

inline void xCam::blend(xVec3& o, const xVec3& a, const xVec3& b, float s)
{
    blend(o.x, a.x, b.x, s);
    blend(o.y, a.y, b.y, s);
    blend(o.z, a.z, b.z, s);
}

inline void xCam::blend(float& o, float a, float b, float s)
{
    o = a * (1.0f - s) + b * s;
}

inline void xCam::blend_radian(float& o, float a, float b, float s)
{
    float diff = xrmod(b - a + 3.1415927f) - 3.1415927f;
    o = xrmod(a + diff * s);
}

inline const xCamCoordCylinder& xCamBlend::convert(const xCam& c,
                                                   xCamCoordCylinder& o) const
{
    switch (c.coord_type) {
    case XCAM_COORD_CYLINDER:
        return c.spatial.coord.cylinder;
    case XCAM_COORD_SPHERE:
        return xCam::convert(o, c.spatial.coord.sphere);
    }

    return o;
}

inline const xCamCoordCylinder&
xCamBlend::coll_convert(const xCam& c, xCamCoordCylinder& o) const
{
    switch (c.coord_type) {
    case XCAM_COORD_CYLINDER:
        return c.coll_spatial.coord.cylinder;
    case XCAM_COORD_SPHERE:
        return xCam::convert(o, c.coll_spatial.coord.sphere);
    }

    return o;
}

inline const xCamCoordSphere& xCamBlend::convert(const xCam& c,
                                                 xCamCoordSphere& o) const
{
    switch (c.coord_type) {
    case XCAM_COORD_CYLINDER:
        return xCam::convert(o, c.spatial.coord.cylinder);
    case XCAM_COORD_SPHERE:
        return c.spatial.coord.sphere;
    }

    return o;
}

inline const xCamCoordSphere& xCamBlend::coll_convert(const xCam& c,
                                                      xCamCoordSphere& o) const
{
    switch (c.coord_type) {
    case XCAM_COORD_CYLINDER:
        return xCam::convert(o, c.coll_spatial.coord.cylinder);
    case XCAM_COORD_SPHERE:
        return c.coll_spatial.coord.sphere;
    }

    return o;
}

inline xCamCoordCylinder& xCam::convert(xCamCoordCylinder& o,
                                        const xCamCoordSphere& i)
{
    __as__5xVec3FRC5xVec3(&o.origin, &i.origin);

    xVec3 offset;
    xQuatToLookVec(&i.dir, &offset);
    offset *= i.dist;

    o.dist = xsqrt(offset.x * offset.x + offset.z * offset.z);
    o.height = offset.y;
    o.theta = xatan2(offset.x, offset.z);
    return o;
}

inline xCamCoordSphere& xCam::convert(xCamCoordSphere& o,
                                      const xCamCoordCylinder& i)
{
    __as__5xVec3FRC5xVec3(&o.origin, &i.origin);

    xMat3x3 mat;
    xVec3 offset;
    __ct__Q24Math6VectorFfff(&offset, i.dist * -isin(i.theta), -i.height,
                             i.dist * -icos(i.theta));

    o.dist = xMat3x3LookVec(&mat, &offset);
    xQuatFromMat(&o.dir, &mat);
    return o;
}

inline float xatan2(float y, float x)
{
    return xAngleClampFast((float)atan2(y, x));
}

inline void xQuatToLookVec(const xQuat* q, xVec3* o)
{
    float tx = 2.0f * q->v.x;
    float ty = 2.0f * q->v.y;
    float tz = 2.0f * q->v.z;
    float tsx = tx * q->s;
    float tsy = ty * q->s;
    float txx = tx * q->v.x;
    float txz = tz * q->v.x;
    float tyy = ty * q->v.y;
    float tyz = tz * q->v.y;

    o->x = txz + tsy;
    o->y = tyz - tsx;
    o->z = 1.0f - txx - tyy;
}

inline float xabs(float x)
{
    return (float)__fabs(x);
}

inline void v3normalizexz(float& len, xVec3* o, const xVec3* v)
{
    float len2 = v->x * v->x + v->z * v->z;

    if (xabs(len2 - 1.0f) <= 1e-5f) {
        o->x = v->x;
        o->z = v->z;
        len = 1.0f;
    } else if (xabs(len2) <= 1e-5f) {
        o->x = 0.0f;
        o->z = 0.0f;
        len = 0.0f;
    } else {
        len = len2 * Math::rsqrt(len2);
        float len_inv = 1.0f / len;
        o->x = v->x * len_inv;
        o->z = v->z * len_inv;
    }
}

// Weak in the image; its one caller, xCamShake::Update, is not written, so
// it is defined out of line here.
void v3sub(xVec3* o, xVec3* a, xVec3* b)
{
    o->x = a->x - b->x;
    o->y = a->y - b->y;
    o->z = a->z - b->z;
}
