#include "SB/GM/Engine/Game/zFXParticleLocator.pool.h"

// zFXParticleLocator -- where a particle system's new particles are born.
// Each emitter volume (point, sphere, circle, line, model, box, curve) has
// a setup_volume, run when the system is activated, that picks one of its
// get_offset functions and stores what that function needs; make_locations
// then calls it once per particle. Read from the image with
// tools/disasm.py; layouts and names are the DWARF's.

// This unit's .bss is part of WAD02's. Its first object,
// Local::activities, sits 0x634 bytes into that translation unit's .bss
// (0x807340F0, the `...bss.0` label, to 0x80734724), and scene_enter and
// scene_exit reach all four of Local's objects as displacements from that
// start, which compiled alone would begin at 0. Like the pool header's
// array it holds the place of what is in front and is referenced by
// nothing; it takes effect only defined ahead of Local's objects.
static unsigned char kUnityBssAhead[0x634];

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {
enum GlobalHeapEnum { GlobalHeap = 0, GlobalHeapEnum_ = 0x7FFFFFFF };
}  // namespace Memory

void* xMemAlloc(Memory::GlobalHeapEnum heap, unsigned int size, int align,
                eMemMgrTag tag);
unsigned int xrand_GenRandInt32();

extern "C" {
double sin(double x);
double cos(double x);
}

class xVec3 {
public:
    xVec3& operator=(const xVec3& other);
    xVec3& operator=(float value) {
        x = y = z = value;
        return *this;
    }
    xVec3& operator*=(float s);
    xVec3& operator-=(const xVec3& other);

    void assign(float ax, float ay, float az) {
        x = ax;
        y = ay;
        z = az;
    }

    // Weak in the image and called out of line: defined below its caller.
    inline void ScaleComponents(const xVec3& s);
    void Scale(const xVec3& v, float s);

    static const xVec3 m_Ones;

    float x;
    float y;
    float z;
};

class xMat3x3 {
public:
    xVec3 right;
    unsigned int flags;
    xVec3 up;
    unsigned int pad1;
    xVec3 at;
    unsigned int pad2;
};

class xMat4x3 : public xMat3x3 {
public:
    xVec3 pos;
    unsigned int pad3;
};

void v3scale(xVec3* o, const xVec3* v, const xVec3* s);
void xMat3x3RMulVec(xVec3* o, const xMat3x3* m, const xVec3* v);
void v3add(xVec3* o, xVec3* a, xVec3* b);

// A point carried into the world by a transform.
inline void xMat4x3Toworld(xVec3* out, const xMat4x3* m, const xVec3* v) {
    xVec3 rotated;

    xMat3x3RMulVec(&rotated, m, v);
    v3add(out, &rotated, (xVec3*)&m->pos);
}

namespace Math {

float rsqrt(float x);

class CubicSpline {
public:
    xVec3 GetPoint(float t);
};

}  // namespace Math

inline float xsqrt(float x) { return x * Math::rsqrt(x); }
inline float isin(float x) { return (float)sin(x); }
inline float icos(float x) { return (float)cos(x); }

// A random float in [0, 1).
inline float xurand() { return 2.32830644e-10f * xrand_GenRandInt32(); }

// A random float in [-v, v]. Box::GetPoint evaluates each of its
// arguments whole before the next one's call only when it comes back
// from an inline; spelled in place, the calls interleave.
inline float xrandrange(float v) { return 2.0f * (v * xurand()) - v; }

// Math::Vector's constructor, which the linker folded xVec3's onto.
extern "C" void __ct__Q24Math6VectorFfff(void* v, float x, float y, float z);

// NumberPool (xParticleUtilities.h): every use here is a call.
class NumberPool {
public:
    void Clear();
    void SetSize(unsigned int size);
    unsigned int Get();
    void Release(unsigned int num);

    unsigned int* mAvailableNums;
    unsigned int mTop;
    unsigned int mBottom;
    unsigned int mNextKey;
    unsigned int mSize;
};

namespace World {

class ModelInstanceAsset {
public:
    unsigned char _pad0[0x40];
};

class Entity;

class EntityHandleBase {
public:
    unsigned char _pad0[0x38];
    Entity* entity;
};

class EntityManager {
public:
    static EntityHandleBase* FindHandle(unsigned long long id);
};

}  // namespace World

class CurveEntity {
public:
    unsigned char _pad0[0x1C];
    Math::CubicSpline* spline;
};

namespace Sext {

class uid {
public:
    unsigned long long internalUid;
};

// The id Model::setup_volume passes on by value is the asset's
// xBaseAsset's, reached through xBaseScene as the DWARF has it. As a
// plain member at +0 the copy interleaved with the other arguments, and
// with a copy constructor its halves took the other register pair.
class xBaseAsset {
public:
    uid id;
    unsigned int baseType;
    unsigned short linkCount;
    unsigned short baseFlags;
};

class xBaseScene : public xBaseAsset {};

// ParticleEmitterParameters at +0x170: its volume's type at +0x180, the
// volume's own parameters at +0x190, EdgeVolume at +0x1E0.
class FXParticleSystem : public xBaseScene {
public:
    unsigned char _pad0[0x180 - 0x10];
    int volumeType;
    unsigned char _pad1[0x190 - 0x184];
    union {
        struct {
            float radius;
        } sphere;
        struct {
            float radiusX;
            float radiusY;
            float arc;
            float height;
        } circle;
        struct {
            bool cylinder;
            float radiusX;
            float radiusY;
            float length;
        } line;
        struct {
            bool surface;
            unsigned char _pad0[0xF];
            World::ModelInstanceAsset model;
        } model;
        struct {
            float x;
            float y;
            float z;
        } box;
        struct {
            unsigned long long id;
        } curve;
    } volume;
    bool EdgeVolume;
};

}  // namespace Sext

namespace FX {

class activity_data {
public:
    void (*get_offset)(xVec3& offset, const int volumeIdx);
    int volumeIdx;
};

namespace Locator {

class Model {
public:
    static bool setup_volume(FX::activity_data& activity,
                             const World::ModelInstanceAsset& asset,
                             const xVec3& scale, bool a, bool b, Sext::uid id);
    static void release_volume(FX::activity_data& activity);
};

namespace Local {
extern void* modelData;
extern NumberPool sModelIndices;
}  // namespace Local

}  // namespace Locator

namespace Particles {
namespace Locator {

class other_volume {
public:
    union {
        struct {
            float radiusX;
            float radiusY;
            float arcScale;
            float height;
        } circle;
        struct {
            float radiusX;
            float radiusY;
            float length;
            unsigned int capChance;
            float radius;
        } line;
        struct {
            float x;
            float y;
            float z;
        } box;
        struct {
            CurveEntity* entity;
        } curve;
    };
};

namespace Local {
activity_data* activities;
NumberPool sNumberPool;
other_volume* otherData;
NumberPool sOtherIndices;
}  // namespace Local

class Point {
public:
    static void get_offset(xVec3& offset, const int volumeIdx);
    static bool setup_volume(FX::activity_data& activity,
                             const Sext::FXParticleSystem& asset);

    // Declared, not defined: its slot in deactivate's table holds an
    // address the linker folded onto an empty function of another name.
    static void release_volume(FX::activity_data& activity);
};

class Sphere {
public:
    static void get_offset(xVec3& offset, const int volumeIdx);
    static void get_offset_edge(xVec3& offset, const int volumeIdx);
    static bool setup_volume(FX::activity_data& activity,
                             const Sext::FXParticleSystem& asset);
    static void release_volume(FX::activity_data& activity);
};

class Circle {
public:
    static void get_offset(xVec3& offset, const int volumeIdx);
    static void get_offset_edge(xVec3& offset, const int volumeIdx);
    static void get_offset_cylinder(xVec3& offset, const int volumeIdx);
    static bool setup_volume(FX::activity_data& activity,
                             const Sext::FXParticleSystem& asset);
};

class Line {
public:
    static void get_offset(xVec3& offset, const int volumeIdx);
    static void get_offset_capsule(xVec3& offset, const int volumeIdx);
    static void get_offset_capsule_edge(xVec3& offset, const int volumeIdx);
    static void get_offset_cylinder(xVec3& offset, const int volumeIdx);
    static void get_offset_cylinder_edge(xVec3& offset, const int volumeIdx);
    static bool setup_volume(FX::activity_data& activity,
                             const Sext::FXParticleSystem& asset);
};

class Box {
public:
    static void GetVertex(xVec3& outVert, float inX, float inY, float inZ,
                          int index);
    static void GetPoint(xVec3& outVert, float inX, float inY, float inZ,
                         int index);
    static void get_offset(xVec3& offset, const int volumeIdx);
    static void get_offset_edge(xVec3& offset, const int volumeIdx);
    static bool setup_volume(FX::activity_data& activity,
                             const Sext::FXParticleSystem& asset);
};

class Model {
public:
    static bool setup_volume(FX::activity_data& activity,
                             const Sext::FXParticleSystem& asset);
};

class Curve {
public:
    static void get_offset(xVec3& offset, const int volumeIdx);
    static bool setup_volume(FX::activity_data& activity,
                             const Sext::FXParticleSystem& asset);
};

void release_volume(FX::activity_data& activity);

class zFXParticleLocator {
public:
    static void scene_enter();
    static void scene_exit();
    bool activate(const Sext::FXParticleSystem& asset);
    void deactivate(const Sext::FXParticleSystem& asset);
    void make_locations(void* data, int stride, int count, int loc_offset,
                        const xMat4x3* inWorldMat,
                        const xVec3* inScale) const;

    unsigned int activityIdx;
};

}  // namespace Locator
}  // namespace Particles
}  // namespace FX

using namespace FX::Particles::Locator;

void Point::get_offset(xVec3& offset, const int) {
    offset = 0.0f;
}

bool Point::setup_volume(FX::activity_data& activity,
                         const Sext::FXParticleSystem&) {
    activity.get_offset = get_offset;
    return true;
}

// NEAR MISS: 92 of 89 words, 356 B where retail has 372. This reads seven distinct float literals, and ours
// addresses them off one `addis` base where retail gives each its own
// `lis` (the rule in tools/gen_poolprefix.py's docstring), so the body
// cannot be compared word for word while that stands.
void Sphere::get_offset(xVec3& offset, const int volumeIdx) {
    float scale = volumeIdx * 0.00390625f;

    float ang = 1.46291812e-09f * xrand_GenRandInt32();
    float uz = 4.65661287e-10f * xrand_GenRandInt32() - 1.0f;
    float r = xsqrt(1.0f - uz * uz);
    float rr = 2.32830644e-10f * xrand_GenRandInt32();
    rr = rr * (rr * rr);
    rr = 1.0f - rr * rr;
    rr *= scale;

    __ct__Q24Math6VectorFfff(&offset, rr * r * icos(ang), rr * r * isin(ang),
                             rr * uz);
}

// NEAR MISS: 67 of 78 words, 312 B where retail has 324: six float literals, addressed off one base as in
// Sphere::get_offset.
void Sphere::get_offset_edge(xVec3& offset, const int volumeIdx) {
    float scale = volumeIdx * 0.00390625f;

    float ang = 1.46291812e-09f * xrand_GenRandInt32();
    float uz = 4.65661287e-10f * xrand_GenRandInt32() - 1.0f;
    float r = scale * xsqrt(1.0f - uz * uz);

    __ct__Q24Math6VectorFfff(&offset, r * icos(ang), r * isin(ang), uz * scale);
}

bool Sphere::setup_volume(FX::activity_data& activity,
                          const Sext::FXParticleSystem& asset) {
    activity.get_offset = asset.EdgeVolume ? get_offset_edge : get_offset;
    activity.volumeIdx = (int)(256.0f * asset.volume.sphere.radius);
    return true;
}

void Sphere::release_volume(FX::activity_data& activity) {
    activity.volumeIdx = 0;
}

void Circle::get_offset(xVec3& offset, const int volumeIdx) {
    other_volume& volume = Local::otherData[volumeIdx];

    float ang = volume.circle.arcScale * xrand_GenRandInt32();
    float rrx = volume.circle.radiusX *
                xsqrt(2.32830644e-10f * xrand_GenRandInt32());
    float rry = volume.circle.radiusY *
                xsqrt(2.32830644e-10f * xrand_GenRandInt32());

    __ct__Q24Math6VectorFfff(&offset, rrx * icos(ang), rry * isin(ang), 0.0f);
}

void Circle::get_offset_edge(xVec3& offset, const int volumeIdx) {
    other_volume& volume = Local::otherData[volumeIdx];

    float ang = volume.circle.arcScale * xrand_GenRandInt32();
    float rrx = volume.circle.radiusX;
    float rry = volume.circle.radiusY;

    __ct__Q24Math6VectorFfff(&offset, rrx * icos(ang), rry * isin(ang), 0.0f);
}

// NEAR MISS: 7 of 92 words, all in `z`. Retail multiplies the constant by
// the height before converting the random number; ours converts first.
// It is the same tree with its two sides evaluated in the other order,
// and 19 spellings over four rounds (operand orders, a cast, xurand(), a
// division by 2^32, an inline taking the member) left it at 7 or more.
void Circle::get_offset_cylinder(xVec3& offset, const int volumeIdx) {
    other_volume& volume = Local::otherData[volumeIdx];

    float ang = volume.circle.arcScale * xrand_GenRandInt32();
    float rrx = volume.circle.radiusX *
                xsqrt(2.32830644e-10f * xrand_GenRandInt32());
    float rry = volume.circle.radiusY *
                xsqrt(2.32830644e-10f * xrand_GenRandInt32());
    float z = 2.32830644e-10f * volume.circle.height * xrand_GenRandInt32();

    __ct__Q24Math6VectorFfff(&offset, rrx * icos(ang), rry * isin(ang), z);
}

// NEAR MISS: 66 of 66 words, 264 B where retail has 268: four float literals, addressed off one base as in
// Sphere::get_offset.
bool Circle::setup_volume(FX::activity_data& activity,
                          const Sext::FXParticleSystem& asset) {
    activity.volumeIdx = Local::sOtherIndices.Get();

    if (activity.volumeIdx == 0)
        return false;

    other_volume& volume = Local::otherData[activity.volumeIdx];

    volume.circle.radiusX = asset.volume.circle.radiusX;
    volume.circle.radiusY = asset.volume.circle.radiusY;
    volume.circle.height = asset.volume.circle.height;

    if (asset.volume.circle.height != 0.0f)
        activity.get_offset = asset.EdgeVolume ? get_offset : get_offset_cylinder;
    else
        activity.get_offset = asset.EdgeVolume ? get_offset_edge : get_offset;

    if (asset.volume.circle.arc >= 6.28218555f)
        volume.circle.arcScale = 1.46291812e-09f;
    else
        volume.circle.arcScale = 2.32830644e-10f * asset.volume.circle.arc;

    return true;
}

// NEAR MISS: 7 of 34 words: `d`'s product evaluated in the other order,
// as in Circle::get_offset_cylinder.
void Line::get_offset(xVec3& offset, const int volumeIdx) {
    other_volume& volume = Local::otherData[volumeIdx];

    float d = 2.32830644e-10f * volume.line.length * xrand_GenRandInt32();
    offset.assign(0.0f, 0.0f, d);
}

// NEAR MISS: 137 of 154 words, 616 B where retail has 632: six float literals, addressed off one base as in
// Sphere::get_offset.
void Line::get_offset_capsule(xVec3& offset, const int volumeIdx) {
    other_volume& volume = Local::otherData[volumeIdx];

    float ang = 1.46291812e-09f * xrand_GenRandInt32();

    if (xrand_GenRandInt32() <= volume.line.capChance) {
        float uz = 4.65661287e-10f * xrand_GenRandInt32() - 1.0f;
        float r = xsqrt(1.0f - uz * uz);
        float rr = 2.32830644e-10f * xrand_GenRandInt32();
        rr = 1.0f - rr * (rr * rr);

        float rrx = rr * volume.line.radiusX;
        float rry = rr * volume.line.radiusY;
        float rrz = rr * volume.line.radius;

        __ct__Q24Math6VectorFfff(&offset, rrx * r * icos(ang),
                                 rry * r * isin(ang), rrz * uz);

        if (uz > 0.0f)
            offset.z += volume.line.length;
    } else {
        float d = 2.32830644e-10f * xrand_GenRandInt32();
        float rrx = volume.line.radiusX *
                    xsqrt(2.32830644e-10f * xrand_GenRandInt32());
        float rry = volume.line.radiusY *
                    xsqrt(2.32830644e-10f * xrand_GenRandInt32());

        __ct__Q24Math6VectorFfff(&offset, rrx * isin(ang), rry * icos(ang),
                                 d * volume.line.length);
    }
}

// NEAR MISS: 108 of 108 words, 432 B where retail has 444: six float literals, addressed off one base as in
// Sphere::get_offset.
void Line::get_offset_capsule_edge(xVec3& offset, const int volumeIdx) {
    other_volume& volume = Local::otherData[volumeIdx];

    float ang = 1.46291812e-09f * xrand_GenRandInt32();

    if (xrand_GenRandInt32() <= volume.line.capChance) {
        float uz = 4.65661287e-10f * xrand_GenRandInt32() - 1.0f;
        float r = xsqrt(1.0f - uz * uz);
        float rr = volume.line.radius;

        __ct__Q24Math6VectorFfff(&offset, rr * r * icos(ang), rr * r * isin(ang),
                                 rr * uz);

        if (uz > 0.0f)
            offset.z += volume.line.length;
    } else {
        float d = 2.32830644e-10f * xrand_GenRandInt32();
        float rr = volume.line.radius;

        __ct__Q24Math6VectorFfff(&offset, rr * isin(ang), rr * icos(ang),
                                 d * volume.line.length);
    }
}

// NEAR MISS: 7 of 78 words: `d`'s product evaluated in the other order,
// as in Circle::get_offset_cylinder.
void Line::get_offset_cylinder(xVec3& offset, const int volumeIdx) {
    other_volume& volume = Local::otherData[volumeIdx];

    float ang = 1.46291812e-09f * xrand_GenRandInt32();
    float d = 2.32830644e-10f * volume.line.length * xrand_GenRandInt32();
    float rr = volume.line.radius * xsqrt(2.32830644e-10f * xrand_GenRandInt32());

    __ct__Q24Math6VectorFfff(&offset, rr * isin(ang), rr * icos(ang), d);
}

// NEAR MISS: 7 of 66 words: `d`'s product evaluated in the other order,
// as in Circle::get_offset_cylinder.
void Line::get_offset_cylinder_edge(xVec3& offset, const int volumeIdx) {
    other_volume& volume = Local::otherData[volumeIdx];

    float ang = 1.46291812e-09f * xrand_GenRandInt32();
    float d = 2.32830644e-10f * volume.line.length * xrand_GenRandInt32();
    float rr = volume.line.radius;

    __ct__Q24Math6VectorFfff(&offset, rr * isin(ang), rr * icos(ang), d);
}

// NEAR MISS: 82 of 86 words, 344 B where retail has 348: four float literals, addressed off one base as in
// Sphere::get_offset.
bool Line::setup_volume(FX::activity_data& activity,
                        const Sext::FXParticleSystem& asset) {
    activity.volumeIdx = Local::sOtherIndices.Get();

    if (activity.volumeIdx == 0)
        return false;

    other_volume& volume = Local::otherData[activity.volumeIdx];

    volume.line.radius = asset.volume.line.radiusX > asset.volume.line.radiusY
                             ? asset.volume.line.radiusX
                             : asset.volume.line.radiusY;
    volume.line.radiusX = asset.volume.line.radiusX;
    volume.line.radiusY = asset.volume.line.radiusY;
    volume.line.length = asset.volume.line.length;

    if (volume.line.radius <= 0.0f) {
        activity.get_offset = get_offset;
    } else if (asset.volume.line.cylinder) {
        activity.get_offset =
            asset.EdgeVolume ? get_offset_cylinder_edge : get_offset_cylinder;
    } else {
        activity.get_offset =
            asset.EdgeVolume ? get_offset_capsule_edge : get_offset_capsule;

        float r = volume.line.radius, d = asset.volume.line.length;
        float rr = r * (asset.EdgeVolume ? 2.0f : 1.33333337f);

        volume.line.capChance = (unsigned int)(4294967296.0f * (rr / (d + rr)));
    }

    return true;
}

void Box::GetVertex(xVec3& outVert, float inX, float inY, float inZ,
                    int index) {
    switch (index) {
    case 0: outVert.assign(-inX, -inY, -inZ); break;
    case 1: outVert.assign(-inX, -inY, inZ); break;
    case 2: outVert.assign(inX, -inY, inZ); break;
    case 3: outVert.assign(inX, -inY, -inZ); break;
    case 4: outVert.assign(-inX, inY, -inZ); break;
    case 5: outVert.assign(-inX, inY, inZ); break;
    case 6: outVert.assign(inX, inY, inZ); break;
    case 7: outVert.assign(inX, inY, -inZ); break;
    }
}

// A random point on one face of the box: the face's own axis is held at
// its extent, the other two spread over [-extent, extent].
void Box::GetPoint(xVec3& outVert, float inX, float inY, float inZ,
                   int index) {
    switch (index) {
    case 0: __ct__Q24Math6VectorFfff(&outVert, xrandrange(inX), -inY, xrandrange(inZ)); break;
    case 1: __ct__Q24Math6VectorFfff(&outVert, inX, xrandrange(inY), xrandrange(inZ)); break;
    case 2: __ct__Q24Math6VectorFfff(&outVert, xrandrange(inX), inY, xrandrange(inZ)); break;
    case 3: __ct__Q24Math6VectorFfff(&outVert, xrandrange(inX), xrandrange(inY), -inZ); break;
    case 4: __ct__Q24Math6VectorFfff(&outVert, -inX, xrandrange(inY), xrandrange(inZ)); break;
    case 5: __ct__Q24Math6VectorFfff(&outVert, xrandrange(inX), xrandrange(inY), inZ); break;
    }
}

void Box::get_offset(xVec3& offset, const int volumeIdx) {
    other_volume& volume = Local::otherData[volumeIdx];

    __ct__Q24Math6VectorFfff(&offset, xurand(), xurand(), xurand());
    offset.ScaleComponents(*(xVec3*)&volume.box);
    offset *= 2.0f;
    offset -= *(xVec3*)&volume.box;
}

inline void xVec3::ScaleComponents(const xVec3& s) {
    x *= s.x;
    y *= s.y;
    z *= s.z;
}

void Box::get_offset_edge(xVec3& offset, const int volumeIdx) {
    other_volume& volume = Local::otherData[volumeIdx];

    GetPoint(offset, volume.box.x, volume.box.y, volume.box.z,
             (xrand_GenRandInt32() & 0xFFFF) * 6 >> 16);
}

bool Box::setup_volume(FX::activity_data& activity,
                       const Sext::FXParticleSystem& asset) {
    activity.volumeIdx = Local::sOtherIndices.Get();

    if (activity.volumeIdx == 0)
        return false;

    ((xVec3&)Local::otherData[activity.volumeIdx].box)
        .Scale((const xVec3&)asset.volume.box, 0.5f);
    activity.get_offset = asset.EdgeVolume ? get_offset_edge : get_offset;
    return true;
}

bool Model::setup_volume(FX::activity_data& activity,
                         const Sext::FXParticleSystem& asset) {
    return FX::Locator::Model::setup_volume(
        activity, asset.volume.model.model, xVec3::m_Ones,
        asset.volume.model.surface, asset.EdgeVolume, asset.id);
}

void Curve::get_offset(xVec3& offset, const int volumeIdx) {
    other_volume& volume = Local::otherData[volumeIdx];

    offset = volume.curve.entity->spline->GetPoint(xurand());
}

bool Curve::setup_volume(FX::activity_data& activity,
                         const Sext::FXParticleSystem& asset) {
    activity.volumeIdx = Local::sOtherIndices.Get();

    if (activity.volumeIdx == 0)
        return false;

    other_volume& volume = Local::otherData[activity.volumeIdx];

    if (asset.volume.curve.id == 0)
        return false;

    volume.curve.entity = (CurveEntity*)World::EntityManager::FindHandle(
                              asset.volume.curve.id)->entity;

    if (volume.curve.entity == 0)
        return false;

    activity.get_offset = get_offset;
    return true;
}

void FX::Particles::Locator::release_volume(FX::activity_data& activity) {
    if (activity.volumeIdx > 0)
        Local::sOtherIndices.Release(activity.volumeIdx);

    activity.volumeIdx = 0;
}

void zFXParticleLocator::scene_enter() {
    Local::activities = (FX::activity_data*)xMemAlloc(
        Memory::GlobalHeap, 320 * sizeof(FX::activity_data), 0, (eMemMgrTag)27);
    Local::sNumberPool.SetSize(320);

    Local::otherData = (other_volume*)xMemAlloc(
        Memory::GlobalHeap, 288 * sizeof(other_volume), 0, (eMemMgrTag)27);
    Local::sOtherIndices.SetSize(288);

    FX::Locator::Local::modelData =
        xMemAlloc(Memory::GlobalHeap, 32 * 144, 0, (eMemMgrTag)27);
    FX::Locator::Local::sModelIndices.SetSize(32);
}

void zFXParticleLocator::scene_exit() {
    Local::activities = 0;
    Local::otherData = 0;
    Local::sNumberPool.Clear();
    Local::sOtherIndices.Clear();

    FX::Locator::Local::modelData = 0;
    FX::Locator::Local::sModelIndices.Clear();
}

bool zFXParticleLocator::activate(const Sext::FXParticleSystem& asset) {
    static bool (* const setup_volume_table[7])(
        FX::activity_data&, const Sext::FXParticleSystem&) = {
        Point::setup_volume, Sphere::setup_volume, Circle::setup_volume,
        Line::setup_volume,  Model::setup_volume,  Box::setup_volume,
        Curve::setup_volume,
    };

    if (activityIdx != 0)
        return true;

    activityIdx = Local::sNumberPool.Get();

    if (activityIdx == 0)
        return false;

    if (setup_volume_table[asset.volumeType](
            Local::activities[activityIdx - 1], asset))
        return true;

    deactivate(asset);
    return false;
}

void zFXParticleLocator::deactivate(const Sext::FXParticleSystem& asset) {
    if (activityIdx != 0) {
        static void (* const release_volume_table[7])(FX::activity_data&) = {
            Point::release_volume,
            Sphere::release_volume,
            FX::Particles::Locator::release_volume,
            FX::Particles::Locator::release_volume,
            FX::Locator::Model::release_volume,
            FX::Particles::Locator::release_volume,
            FX::Particles::Locator::release_volume,
        };

        release_volume_table[asset.volumeType](
            Local::activities[activityIdx - 1]);
        Local::sNumberPool.Release(activityIdx);
        activityIdx = 0;
    }
}

void zFXParticleLocator::make_locations(void* data, int stride, int count,
                                        int loc_offset,
                                        const xMat4x3* inWorldMat,
                                        const xVec3* inScale) const {
    FX::activity_data& activity = Local::activities[activityIdx - 1];

    void (*get_offset)(xVec3&, const int) = activity.get_offset;

    unsigned char* p = (unsigned char*)data, *endp = p + stride * count;

    for (; p != endp; p += stride) {
        xVec3& loc = *(xVec3*)(p + loc_offset);

        xVec3 offset;

        get_offset(offset, activity.volumeIdx);

        if (inScale)
            v3scale(&offset, &offset, inScale);

        if (inWorldMat)
            xMat4x3Toworld(&loc, inWorldMat, &offset);
        else
            loc = offset;
    }
}
