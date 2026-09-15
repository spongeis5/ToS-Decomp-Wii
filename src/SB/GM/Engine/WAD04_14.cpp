#include "SB/GM/Engine/WAD04_14.pool.h"

// WAD04_14.cpp -- TriggerPhantom.cpp of the retail build: a trigger volume
// made of a Havok phantom (an AABB phantom, or a simple-shape phantom for
// boxes, spheres and capsules) that collects what penetrates it, fires the
// asset's events on enter and exit, deals damage over time and pushes what
// is inside. Read from the image with tools/brief.py; the layouts are the
// DWARF's (alltypes.h), the virtual slots the image's (vtable.py).
//
// The generated accessor banner was taken over: Setup and the simple-shape
// collector's removeOverlappingCollidable came from gen_accessors.py.

class hkQuaternion;
class hkpCdBodyPairCollector;
class hkpCollisionInput;
class hkpShape;
class xBase;
class xOGModel;
class TriggerPhantom;

namespace World {
class Entity;
class EntityHandleBase;
class xOGEntity;
}  // namespace World

namespace Math {
class Matrix43;
}  // namespace Math

namespace Sext {
class EventAny {};
class TriggerPhantomAsset;
}  // namespace Sext

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {

enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };

void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap, eMemMgrTag tag,
                      bool clear);

}  // namespace Memory

void* xMemAlloc(Memory::GlobalHeapEnum heap, unsigned int size, int align,
                eMemMgrTag tag);

extern "C" {
void* memset(void* dst, int c, unsigned long n);
}

inline void* operator new(unsigned long, void* p) { return p; }

enum ForceEvent {
    FE_YES = 0,
    FE_NO = 1
};

// ---------------------------------------------------------------------------
// Vectors and matrices

class xVec3 {
public:
    float normalize();
    xVec3& operator+=(const xVec3& v);

    float x;
    float y;
    float z;
};

class xMat3x3 {
public:
    xVec3 right;
    int flags;
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

extern xMat4x3 g_I3;

void xMat4x3Copy(xMat4x3* o, const xMat4x3* m);
void xMat4x3FromNGMatrix(xMat4x3* o, const Math::Matrix43* m);
void xMat3x3GetScale(const xMat3x3* m, xVec3* scale);

class xQuat {
public:
    xVec3 v;
    float s;
};

void xQuatFromMat(xQuat* q, const xMat3x3* m);
void xQuatNormalize(xQuat* o, const xQuat* q);

namespace Math {
float rsqrt(float x);
}  // namespace Math

inline float xsqrt(float x) { return x * Math::rsqrt(x); }
inline float iabs(float x) { return (float)__fabs(x); }
inline bool xeq(float a, float b, float eps) {
    return (float)__fabs(a - b) <= eps;
}

// Weak in the image and called out of line: defined at the bottom of
// the file, below every caller.
inline float xVec3Normalize(xVec3* o, const xVec3* v);

// Math::Vector's constructor, which the linker folded xVec3's onto.
extern "C" void __ct__Q24Math6VectorFfff(void* v, float x, float y, float z);

// Its arguments load z, y, x, as retail's do.
inline void xVec3Init(xVec3* v, float x, float y, float z) {
    __ct__Q24Math6VectorFfff(v, x, y, z);
}

class hkRotation;

class hkVector4 {
public:
    hkVector4() {}
    // Copies go through the assignment, as retail calls it.
    hkVector4(const hkVector4& v) { *this = v; }

    void operator=(const hkVector4& v);

    float& operator()(int i) { return m_quad[i]; }

    void setZero4();
    void sub4(const hkVector4& a);
    void mul4(float s);
    void normalize3();
    void _setRotatedDir(const hkRotation& r, const hkVector4& v);

    float dot3(const hkVector4& a) const {
        return m_quad[0] * a.m_quad[0] + m_quad[1] * a.m_quad[1] +
               m_quad[2] * a.m_quad[2];
    }

    float m_quad[4] __attribute__((aligned(16)));
};

class hkMatrix3 {
public:
    hkMatrix3() {}

    void setIdentity();
    void setCols(const hkVector4& c0, const hkVector4& c1,
                 const hkVector4& c2) {
        m_col0 = c0;
        m_col1 = c1;
        m_col2 = c2;
    }

    hkVector4& getColumn(int i) { return (&m_col0)[i]; }
    float& operator()(int row, int col) { return getColumn(col)(row); }

    hkVector4 m_col0;
    hkVector4 m_col1;
    hkVector4 m_col2;
};

// Its copy constructor is the weak function this unit emits: defined at
// the bottom of the file, below every caller, so it is called, not inlined.
class hkRotation : public hkMatrix3 {
public:
    hkRotation() {}
    hkRotation(const hkRotation& other);

    void set(const hkQuaternion& q);
};

class hkTransform {
public:
    hkTransform() {}
    hkTransform(const hkTransform& t);

    void setIdentity() {
        m_rotation.setIdentity();
        m_translation.setZero4();
    }
    void setRotation(const hkRotation& r) {
        m_rotation.m_col0 = r.m_col0;
        m_rotation.m_col1 = r.m_col1;
        m_rotation.m_col2 = r.m_col2;
    }
    hkRotation& getRotation() { return m_rotation; }
    hkVector4& getTranslation() { return m_translation; }
    void setTranslation(const hkVector4& t) { m_translation = t; }

    hkRotation m_rotation;
    hkVector4 m_translation;
};

class hkAabb {
public:
    hkVector4 m_min;
    hkVector4 m_max;
};

namespace Math {

class Vector4 {
public:
    Vector4() {}
    Vector4(const xVec3& v, float w) { Assign(v.x, v.y, v.z, w); }
    Vector4(const Vector4& v, float w) { Assign(v.v[0], v.v[1], v.v[2], w); }
    Vector4(const xQuat& q) { Assign(q.v.x, q.v.y, q.v.z, q.s); }

    Vector4& Assign(float x, float y, float z, float w);

    operator const hkVector4&() const { return *(const hkVector4*)this; }
    operator const hkQuaternion&() const {
        return *(const hkQuaternion*)this;
    }

    float v[4] __attribute__((aligned(16)));
};

class Matrix33 {
public:
    Vector4 v[3];
};

class Matrix43POD {
public:
    float m[12];
};

class Matrix43 : public Matrix33 {
public:
    Matrix43(const Matrix43POD& m);
};

}  // namespace Math

// ---------------------------------------------------------------------------
// The asset

namespace Sext {

class uid {
public:
    // Passed where an id is wanted, a uid is converted by this inline.
    operator unsigned long long() const { return internalUid; }

    unsigned long long internalUid;
};

class xBaseAsset {
public:
    uid id;
    unsigned int baseType;
    unsigned short linkCount;
    unsigned short baseFlags;
};

class xBaseScene : public xBaseAsset {};

class LinkAsset {
public:
    unsigned int count;
    void* data;
};

class LinkEvent {
public:
    int type;
    void* v;
};

class LinkAssetBaseNew {
public:
    LinkEvent srcEvent;
    LinkEvent dstEvent;
    uid dstAssetID;
    uid chkAssetID;
    bool chkSourceParams;
    bool disabled;
    unsigned int chkSourceMask;
};

class EntityList {
public:
    unsigned int count;
    unsigned long long* data;
};

class vec3 {
public:
    float x;
    float y;
    float z;
};

class VelocityTriggerStruct {
public:
    float Magnitude;
    vec3 Direction;
};

class VelocityPointStruct {
public:
    bool Continuous;
    float Magnitude;
    vec3 Offset;
    float Yaw;
    float Pitch;
};

class ForceTorqueTriggerStruct {
public:
    float Magnitude;
    vec3 Direction;
    bool FallOff;
    float FallOffDist;
};

class TriggerPhantomAsset : public xBaseScene {
public:
    static World::Entity* Create(World::EntityHandleBase* handle,
                                 TriggerPhantomAsset* asset);

    LinkAsset EventLinksNew;
    unsigned char Type;
    unsigned char _pad0[0x20 - 0x19];
    // The shape union: every arm is one Matrix43POD transform.
    Math::Matrix43POD Transform;
    unsigned int Threshold;
    float DotDamageAmount;
    float DotDamageFreq;
    bool DotDrownThePlayer;
    float VelocityDampeningDelay;
    EntityList EntitiesInclude;
    EntityList EntitiesExclude;
    float WeightThreshold;
    bool Targettable;
    unsigned char Pad00;
    unsigned char Pad01;
    unsigned char Pad02;
    unsigned int Action;
    union {
        VelocityTriggerStruct VelocityTrigger;
        ForceTorqueTriggerStruct ForceTorqueTrigger;
        VelocityPointStruct VelocityPoint;
        unsigned char ActionData[0x20];
    };
};

class EventActionNew : public EventAny {};

class EventActionDrivenBy : public EventActionNew {
public:
    bool param0;
    bool param1;
    bool param2;
    bool cam;
    uid specificPassenger;
    int bone;
};

}  // namespace Sext

typedef char _size_TriggerPhantomAsset
    [(sizeof(Sext::TriggerPhantomAsset) == 0xA0) ? 1 : -1];

// ---------------------------------------------------------------------------
// Havok

class hkBool {
public:
    hkBool() {}
    hkBool(bool b) { m_bool = (char)b; }
    operator bool() const { return m_bool != 0; }
    hkBool operator==(bool e) const { return (m_bool != 0) == e; }

    char m_bool;
};

template <class ENUM, class STORAGE>
class hkEnum {
public:
    operator ENUM() const { return (ENUM)m_storage; }
    hkBool operator==(ENUM e) const { return m_storage == (STORAGE)e; }

    STORAGE m_storage;
};

template <class T>
class hkArray {
public:
    int getSize() const { return m_size; }
    T& operator[](int i) { return m_data[i]; }
    const T& operator[](int i) const { return m_data[i]; }

    T* m_data;
    int m_size;
    int m_capacityAndFlags;
};

class hkBaseObject {
public:
    // The destructor's slot, spelled plain: nothing here calls it.
    virtual void _hb0();
};

class hkReferencedObject : public hkBaseObject {
public:
    void removeReference() const;

    unsigned short m_memSizeAndFlags;
    short m_referenceCount;
};

enum HK_MEMORY_CLASS {
    HK_MEMORY_CLASS_CDINFO = 40,
    HK_MEMORY_CLASS_PHANTOM = 50
};

class hkThreadMemory {
public:
    void* allocateChunk(int nbytes, HK_MEMORY_CLASS cls);
};

extern hkThreadMemory* hkThreadMemory__s_threadMemoryInstance;

class hkpShape : public hkReferencedObject {
public:
    void* operator new(unsigned long nbytes) {
        hkReferencedObject* b = (hkReferencedObject*)
            hkThreadMemory__s_threadMemoryInstance->allocateChunk(
                (int)nbytes, HK_MEMORY_CLASS_CDINFO);
        b->m_memSizeAndFlags = (unsigned short)nbytes;
        return b;
    }
};

class hkpBoxShape : public hkpShape {
public:
    hkpBoxShape(const hkVector4& halfExtents, float radius);

    unsigned char _pad0[0x30 - 0x8];
};

class hkpSphereShape : public hkpShape {
public:
    hkpSphereShape(float radius);

    unsigned char _pad0[0x20 - 0x8];
};

class hkpCapsuleShape : public hkpShape {
public:
    hkpCapsuleShape(const hkVector4& vertexA, const hkVector4& vertexB,
                    float radius);

    unsigned char _pad0[0x40 - 0x8];
};

class hkpPropertyValue {
public:
    hkpPropertyValue() {}
    hkpPropertyValue(const int i) { setInt(i); }

    void setInt(const int i) { m_data = i; }
    int getInt() const { return (int)m_data; }

    unsigned long long m_data;
};

class hkpProperty {
public:
    unsigned int m_key;
    unsigned int m_alignmentPadding;
    hkpPropertyValue m_value;
};

class hkpCdBody {
public:
    const hkpShape* m_shape;
    unsigned int m_shapeKey;
    void* m_motion;
    const hkpCdBody* m_parent;
};

class hkpCollidable : public hkpCdBody {
public:
    void* getOwner() const { return (void*)((char*)this + m_ownerOffset); }

    signed char m_ownerOffset;
};

class hkpWorldObject : public hkReferencedObject {
public:
    enum MtChecks {
        MULTI_THREADING_CHECKS_ENABLE = 0,
        MULTI_THREADING_CHECKS_IGNORE = 1
    };

    bool hasProperty(unsigned int key,
                     MtChecks mtCheck = MULTI_THREADING_CHECKS_ENABLE) const;
    void addProperty(unsigned int key, hkpPropertyValue value);
    hkpPropertyValue getProperty(
        unsigned int key,
        MtChecks mtCheck = MULTI_THREADING_CHECKS_ENABLE) const;
    unsigned long getUserData() const { return m_userData; }

    void* m_world;
    unsigned long m_userData;
    unsigned char _pad0[0x78 - 0x10];
    hkArray<hkpProperty> m_properties;
    void* m_aiData;
};

inline hkpPropertyValue hkpWorldObject::getProperty(unsigned int key,
                                                    MtChecks mtCheck) const {
    for (int i = 0; i < m_properties.getSize(); ++i) {
        if (m_properties[i].m_key == key) {
            return m_properties[i].m_value;
        }
    }

    hkpPropertyValue returnValue;
    returnValue.m_data = 0;
    return returnValue;
}

inline hkpWorldObject* hkGetWorldObject(const hkpCollidable* collidable) {
    return (hkpWorldObject*)collidable->getOwner();
}

class hkpMotion : public hkReferencedObject {
public:
    enum MotionType {
        MOTION_INVALID = 0,
        MOTION_DYNAMIC = 1,
        MOTION_SPHERE_INERTIA = 2,
        MOTION_STABILIZED_SPHERE_INERTIA = 3,
        MOTION_BOX_INERTIA = 4,
        MOTION_STABILIZED_BOX_INERTIA = 5,
        MOTION_KEYFRAMED = 6,
        MOTION_FIXED = 7,
        MOTION_THIN_BOX_INERTIA = 8
    };

    float getMass() const;

    virtual void _mo1();
    virtual void _mo2();
    virtual void _mo3();
    virtual void _mo4();
    virtual void _mo5();
    virtual void _mo6();
    virtual void _mo7();
    virtual void _mo8();
    virtual void _mo9();
    virtual void _mo10();
    virtual void _mo11();
    virtual void _mo12();
    virtual void _mo13();
    virtual void _mo14();
    virtual void _mo15();
    virtual void setLinearVelocity(const hkVector4& newVel);
    virtual void _mo17();
    virtual void _mo18();
    virtual void applyLinearImpulse(const hkVector4& imp);
    virtual void _mo20();
    virtual void _mo21();
    virtual void applyForce(const float deltaTime, const hkVector4& force);

    hkEnum<MotionType, unsigned char> m_type;
    unsigned char _pad0[0x10 - 0x9];
    // m_motionState.m_transform.
    hkTransform m_transform;
};

class hkpEntity : public hkpWorldObject {
public:
    hkBool isFixed() const { return m_motion.m_type == hkpMotion::MOTION_FIXED; }
    void activate();
    hkpMotion* getMotion() { return &m_motion; }

    unsigned char _pad1[0xE0 - 0x88];
    hkpMotion m_motion;
};

class hkpRigidBody : public hkpEntity {
public:
    hkpMotion::MotionType getMotionType() const {
        return (hkpMotion::MotionType)m_motion.m_type;
    }
    float getMass() const { return m_motion.getMass(); }
    const hkVector4& getPosition() const {
        return m_motion.m_transform.m_translation;
    }
    void setLinearVelocity(const hkVector4& newVel) {
        activate();
        getMotion()->setLinearVelocity(newVel);
    }
    void applyLinearImpulse(const hkVector4& imp) {
        activate();
        getMotion()->applyLinearImpulse(imp);
    }
    void applyForce(const float deltaTime, const hkVector4& force) {
        activate();
        getMotion()->applyForce(deltaTime, force);
    }
};

hkpRigidBody* hkGetRigidBody(const hkpCollidable* collidable);

// The slots this unit calls or fills, as the collectors' tables give them
// (vtable.py AabbPhantomCollector, SimpleShapePhantomCollector).
class hkpPhantom : public hkpWorldObject {
public:
    void* operator new(unsigned long nbytes) {
        hkReferencedObject* b = (hkReferencedObject*)
            hkThreadMemory__s_threadMemoryInstance->allocateChunk(
                (int)nbytes, HK_MEMORY_CLASS_PHANTOM);
        b->m_memSizeAndFlags = (unsigned short)nbytes;
        return b;
    }

    virtual void _ph1();
    virtual void _ph2();
    virtual void _ph3();
    virtual void _ph4();
    virtual void _ph5();
    virtual void _ph6();
    virtual void addOverlappingCollidable(hkpCollidable* collidable);
    virtual void _ph8();
    virtual void removeOverlappingCollidable(hkpCollidable* collidable);
    virtual void _ph10();
    virtual void _ph11();
    virtual void _ph12();
    virtual void _ph13();

    unsigned char _pad1[0xA0 - 0x88];
};

hkpPhantom* hkGetPhantom(const hkpCollidable* collidable);

class hkpAabbPhantom : public hkpPhantom {
public:
    hkpAabbPhantom(const hkAabb& aabb, unsigned int collisionFilterInfo);

    void addOverlappingCollidable(hkpCollidable* collidable);
    void removeOverlappingCollidable(hkpCollidable* collidable);
    void setAabb(const hkAabb& aabb);

    const hkArray<hkpCollidable*>& getOverlappingCollidables() const {
        return m_overlappingCollidables;
    }

    hkAabb m_aabb;
    hkArray<hkpCollidable*> m_overlappingCollidables;
    hkBool m_orderDirty;
};

class hkpShapePhantom : public hkpPhantom {
public:
    virtual void _sp14();
    virtual void _sp15();
    virtual void _sp16();
    virtual void getPenetrations(hkpCdBodyPairCollector& collector,
                                 const hkpCollisionInput* input);

    void setTransform(const hkTransform& transform);
    // m_motionState.m_transform.
    hkTransform& getTransform() { return m_transform; }

    hkTransform m_transform;
    unsigned char _pad2[0x150 - 0xE0];
};

class hkpSimpleShapePhantom : public hkpShapePhantom {
public:
    hkpSimpleShapePhantom(const hkpShape* shape, const hkTransform& transform,
                          unsigned int collisionFilterInfo);

    void addOverlappingCollidable(hkpCollidable* collidable);
    void removeOverlappingCollidable(hkpCollidable* collidable);

    unsigned char _pad3[0x160 - 0x150];
};

class hkpCdBodyPairCollector {
public:
    hkpCdBodyPairCollector() { reset(); }

    virtual void _cb0();
    virtual void _cb1();
    virtual void reset();

    hkBool m_earlyOut;
};

class hkpRootCdBodyPair {
public:
    hkpCollidable* m_rootCollidableA;
    unsigned int m_shapeKeyA;
    hkpCollidable* m_rootCollidableB;
    unsigned int m_shapeKeyB;
};

template <class T, unsigned int N>
class hkInplaceArray : public hkArray<T> {
public:
    T m_storage[N];
};

class hkpAllCdBodyPairCollector : public hkpCdBodyPairCollector {
public:
    hkpAllCdBodyPairCollector() {
        m_hits.m_data = m_hits.m_storage;
        m_hits.m_size = 0;
        m_hits.m_capacityAndFlags = (int)(16 | 0x80000000);
        reset();
    }
    ~hkpAllCdBodyPairCollector();

    virtual void reset();

    const hkArray<hkpRootCdBodyPair>& getHits() const { return m_hits; }

    hkInplaceArray<hkpRootCdBodyPair, 16> m_hits;
};

void xHavok_AddToSimWorld(hkpPhantom* phantom);
void xHavok_RemoveFromSimWorld(hkpPhantom* phantom);

// ---------------------------------------------------------------------------
// The entity classes

// The slots of __vt__14TriggerPhantom this unit calls or fills.
class TriggerPhantomVirtuals {
public:
    virtual void _v0();
    virtual void _v1();
    virtual void _v2();
    virtual void _v3();
    virtual void _v4();
    virtual void _v5();
    virtual void _v6();
    virtual xMat4x3* DriveGetCurMat(int bone);
    virtual void _v8();
    virtual void DriveAttach(World::xOGEntity* passenger, unsigned int flags,
                             int bone);
    virtual void DriveDetach();
    virtual void DriveOn();
    virtual void DriveOff();
    virtual void _v13();
    virtual void _v14();
    virtual void _v15();
    virtual void DriveCauseMove(xMat4x3* parent, xMat4x3* relMat,
                                xMat4x3* world, xMat4x3* oldMat, bool moved,
                                bool rotated, float speed, float dt);
    virtual void _v17();
    virtual void _v18();
    virtual void _v19();
    virtual void Init(Sext::TriggerPhantomAsset* asset);
    virtual void DebugReset();
    virtual void Reset();
    virtual void Setup();
    virtual void HandleEvent(xBase* from, xBase* to, unsigned int toEvent,
                             Sext::EventAny* toParam);
};

class EmbeddedListNode {
public:
    EmbeddedListNode* next;
    EmbeddedListNode* prev;
};

typedef void (*xBaseEventCB)(xBase* from, xBase* to, unsigned int toEvent,
                             Sext::EventAny* toParam);

class xOGModelHandle {
public:
    xOGModel* data;
    void* autoptr;
};

// Packed to four: the id's eight-byte alignment would otherwise round xBase
// up past the model handle the DWARF puts at +0x34.
#pragma pack(push, 4)

namespace World {

class Entity : public TriggerPhantomVirtuals {
public:
    EmbeddedListNode ogSceneNode;
    int ogUpdateIdx;
    unsigned int typeID;
    EntityHandleBase* handle;
};

}  // namespace World

class xBase : public World::Entity {
public:
    unsigned long long id;
    unsigned int baseType;
    unsigned char UNUSED_linkCount;
    unsigned char assertFlags;
    unsigned short baseFlags;
    Sext::LinkAsset* linkArray;
    void* templateParent;
    xBaseEventCB eventFunc;
};

namespace World {

class xOGEntity : public xBase {
public:
    xOGEntity(EntityHandleBase* handle);
    ~xOGEntity();

    xOGModelHandle ogModel;
};

class EntityManager {
public:
    static void* FindAsset(unsigned long long id);
};

EntityManager* GetEntityManager();

}  // namespace World

#pragma pack(pop)

void xBaseInit(xBase* base, const Sext::xBaseAsset* asset);
void zEntEvent(xBase* from, unsigned int fromEvent, xBase* to,
               unsigned int toEvent, Sext::EventAny* params, ForceEvent force);
xBase* zSceneFindObject(unsigned long long id);

class xGroup : public World::xOGEntity {
public:
    bool ContainsBase_Recursive(xBase* base);
};

class zPlayer : public World::xOGEntity {
public:
    hkpShapePhantom* GetPhantomObject() const;

    // The slots of __vt__7zPlayer this unit calls.
    virtual void _zp25();
    virtual void _zp26();
    virtual void _zp27();
    virtual void _zp28();
    virtual void _zp29();
    virtual void _zp30();
    virtual void _zp31();
    virtual void _zp32();
    virtual void _zp33();
    virtual void _zp34();
    virtual void _zp35();
    virtual void _zp36();
    virtual void _zp37();
    virtual void _zp38();
    virtual void _zp39();
    virtual void _zp40();
    virtual void _zp41();
    virtual void _zp42();
    virtual void _zp43();
    virtual void _zp44();
    virtual void _zp45();
    virtual void _zp46();
    virtual void _zp47();
    virtual void _zp48();
    virtual void _zp49();
    virtual void _zp50();
    virtual void _zp51();
    virtual void _zp52();
    virtual void _zp53();
    virtual void _zp54();
    virtual void _zp55();
    virtual void _zp56();
    virtual void _zp57();
    virtual void _zp58();
    virtual void _zp59();
    virtual void _zp60();
    virtual void _zp61();
    virtual void _zp62();
    virtual void _zp63();
    virtual void _zp64();
    virtual void _zp65();
    virtual void _zp66();
    virtual void _zp67();
    virtual void _zp68();
    virtual void _zp69();
    virtual void _zp70();
    virtual void _zp71();
    virtual void _zp72();
    virtual void _zp73();
    virtual void _zp74();
    virtual void _zp75();
    virtual void _zp76();
    virtual void _zp77();
    virtual void _zp78();
    virtual void _zp79();
    virtual void _zp80();
    virtual void _zp81();
    virtual void _zp82();
    virtual void _zp83();
    virtual void _zp84();
    virtual void _zp85();
    virtual void _zp86();
    virtual void _zp87();
    virtual void _zp88();
    virtual void _zp89();
    virtual void _zp90();
    virtual void _zp91();
    virtual void _zp92();
    virtual void _zp93();
    virtual void _zp94();
    virtual void _zp95();
    virtual void _zp96();
    virtual void _zp97();
    virtual void _zp98();
    // +404: an empty body in zPlayer's own table.
    virtual void OnTriggerExit(TriggerPhantom* trigger);
    virtual void _zp100();
    virtual void _zp101();
    virtual void _zp102();
    virtual void _zp103();
    virtual void _zp104();
    virtual void _zp105();
    virtual void _zp106();
    virtual void _zp107();
    virtual void _zp108();
    virtual void _zp109();
    virtual void ImpartForce(const xVec3& force);
    virtual void _zp111();
    virtual void ImpartImpulse(const xVec3& impulse);
    virtual void ImpartVelocity(const xVec3& velocity);

    unsigned char _pad0[0xF8 - 0x3C];
    int playerIndex;
    unsigned char _pad1[0x218 - 0xFC];
    float pWeight;
};

class zNPCSteering {
public:
    void ApplyExternalImpulse(const xVec3* impulse);
    void ApplyExternalForce(const xVec3* force);

    unsigned char _pad0[0x1C];
    xVec3 externalVelocityAccum;
};

class zNPCBase {
public:
    unsigned char _pad0[0xA0];
    zNPCSteering* npcSteering;
};

class zNPCEntity : public World::xOGEntity {
public:
    hkpShapePhantom* GetPhantomObject();

    unsigned char _pad0[0xBC - 0x3C];
    // zNPCComponent::owner
    zNPCBase* owner;
};

class zProjectile;

class zProjectileHavok : public xBase {
public:
    zProjectile* GetOwnerProjectile();
    float GetMass();

    unsigned char _pad0[0x98 - 0x34];
};

class zProjectile {
public:
    unsigned char _pad0[0xF8];
    zProjectileHavok physicsEntity;
};

class Globals {
public:
    static float dt;
};

// ---------------------------------------------------------------------------
// The trigger and its collectors

class TriggerPhantom : public World::xOGEntity {
public:
    TriggerPhantom(World::EntityHandleBase* handle) : World::xOGEntity(handle) {}
    ~TriggerPhantom();

    // Declared first and defined nowhere here, so the table stays where
    // retail keeps it.
    virtual void DriveCauseMove(xMat4x3* parent, xMat4x3* relMat,
                                xMat4x3* world, xMat4x3* oldMat, bool moved,
                                bool rotated, float speed, float dt);
    virtual xMat4x3* DriveGetCurMat(int bone);
    virtual void Init(Sext::TriggerPhantomAsset* asset);
    virtual void DebugReset();
    virtual void Reset();
    virtual void Setup();
    virtual void HandleEvent(xBase* from, xBase* to, unsigned int toEvent,
                             Sext::EventAny* toParam);

    void CreateCollector();
    Sext::TriggerPhantomAsset* GetAsset() { return Asset; }
    unsigned int GetThreshold() { return Asset->Threshold; }
    void PopulateIncludeAndExcludeLists();
    void Update(float dt);
    void UpdatePenetrations();
    void AddPenetratingCollidableToList(const hkpCollidable* pColl);
    void RemovePenetratingCollidableFromList(int index);
    void AddToDotTargetList(xBase* target);
    void RemoveFromDotTargetList(xBase* target);
    bool IsDotTarget(xBase* target);
    void DealDotDamage(float dt);
    bool CanPenetrate(const hkpCollidable* pColl);
    hkTransform getTransform();
    void Listen(xBase* to, unsigned int eventToListen);
    bool IsAffectedByTrigger(xBase* base);
    void ApplyTriggerPhysics(const hkpCollidable* pColl, bool entering);
    void ComputeVelocityInTriggerSpace(hkVector4& output);
    void ComputeVelocityToPoint(const hkVector4* subjectPos, hkVector4& output);
    void ComputeImpulseToPoint(const hkVector4* subjectPos, hkVector4& output);
    void ComputeForceTorqueToPoint(const hkVector4* subjectPos, hkVector4& output);
    void ComputeForceTorqueInTriggerSpace(const hkVector4* subjectPos,
                                          hkVector4& output);
    float getCollidableWeight(const hkpCollidable* c);
    bool NotifyWeightThresholdEnter(float weight, float prevWeight);
    bool NotifyWeightThresholdExit(float weight, float prevWeight);
    void CalculateAABB(Math::Vector4& posAABBox, Math::Vector4& scaleAABBox,
                       hkAabb& infoAABB);

    hkpShape* shape;
    hkpPhantom* collector;
    hkpRigidBody* rigidBody;
    Sext::TriggerPhantomAsset* Asset;
    hkpCollidable* penetrationList[100];
    float weightList[100];
    unsigned int baseTypeList[100];
    int penetrationCount;
    float penetrationWeight;
    float prevPenetrationWeight;
    unsigned int includedEntCount;
    unsigned int excludedEntCount;
    xBase** includedEnt;
    xBase** excludedEnt;
    bool enabled;
    xBase* dotTargetList[8];
    float dotTimerList[8];
    int dotTargetCount;
    xMat4x3 driveCurMat;
    bool formSwitch;
    unsigned int playersEnteredFlag;
};

typedef char _size_TriggerPhantom[(sizeof(TriggerPhantom) == 0x5A8) ? 1 : -1];

class AabbPhantomCollector : public hkpAabbPhantom {
public:
    AabbPhantomCollector(const hkAabb& aabb, TriggerPhantom* ptr,
                         unsigned int collisionFilterInfo);

    // Declared first and defined nowhere here (see TriggerPhantom).
    virtual void _ph8();
    virtual void addOverlappingCollidable(hkpCollidable* c);
    virtual void removeOverlappingCollidable(hkpCollidable* c);

    unsigned int PostDecOverlapCount() { return overlapCount--; }

    unsigned int overlapCount;
    TriggerPhantom* owner;
};

typedef char _size_AabbPhantomCollector
    [(sizeof(AabbPhantomCollector) == 0xE0) ? 1 : -1];

class SimpleShapePhantomCollector : public hkpSimpleShapePhantom {
public:
    SimpleShapePhantomCollector(const hkpShape* shape,
                                const hkTransform& transform,
                                TriggerPhantom* ptr,
                                unsigned int collisionFilterInfo);

    // Declared first and defined nowhere here (see TriggerPhantom).
    virtual void _ph8();
    virtual void addOverlappingCollidable(hkpCollidable* c);
    virtual void removeOverlappingCollidable(hkpCollidable* a0);

    unsigned int type;
    unsigned int overlapCount;
    TriggerPhantom* owner;
};

typedef char _size_SimpleShapePhantomCollector
    [(sizeof(SimpleShapePhantomCollector) == 0x170) ? 1 : -1];

// ---------------------------------------------------------------------------
// Functions, in the image's order

// Retail inlines getProperty here; -inline auto emits it out of line.
#pragma push
#pragma always_inline on
bool CheckForActiveBodyPartProperty(hkpWorldObject* wo) {
    if (wo->hasProperty(6666) == false) {
        return true;
    }
    if (wo->getProperty(6666).getInt() == 1) {
        return true;
    }
    return false;
}
#pragma pop

// Retail inlines the constructor here; -inline auto emits it out of line.
#pragma push
#pragma always_inline on
World::Entity* Sext::TriggerPhantomAsset::Create(World::EntityHandleBase* handle,
                                                 TriggerPhantomAsset* asset) {
    TriggerPhantom* entity =
        new (memset(Memory::AllocGlobalHeap(sizeof(TriggerPhantom),
                                            (Memory::GlobalHeapEnum)0,
                                            (eMemMgrTag)16, false),
                    0, sizeof(TriggerPhantom))) TriggerPhantom(handle);

    entity->Init(asset);

    return entity;
}
#pragma pop

static void TriggerPhantomEventWrapper(xBase* from, xBase* to,
                                       unsigned int toEvent,
                                       Sext::EventAny* toParam) {
    ((TriggerPhantom*)to)->HandleEvent(from, to, toEvent, toParam);
}

void TriggerPhantom::Init(Sext::TriggerPhantomAsset* inAsset) {
    xBaseInit(this, inAsset);
    Asset = inAsset;
    eventFunc = TriggerPhantomEventWrapper;
    enabled = inAsset->baseFlags & 1;

    CreateCollector();

    linkArray = &Asset->EventLinksNew;

    excludedEntCount = Asset->EntitiesExclude.count;
    includedEntCount = Asset->EntitiesInclude.count;

    if (excludedEntCount) {
        excludedEnt = (xBase**)xMemAlloc((Memory::GlobalHeapEnum)0,
                                         excludedEntCount * sizeof(xBase*),
                                         0, (eMemMgrTag)24);
    }

    if (includedEntCount) {
        includedEnt = (xBase**)xMemAlloc((Memory::GlobalHeapEnum)0,
                                         includedEntCount * sizeof(xBase*),
                                         0, (eMemMgrTag)24);
    }

    if (Asset->Type <= 3) {
        xHavok_AddToSimWorld(collector);

        hkpPropertyValue val(1);
        collector->addProperty(7777, val);
    }

    penetrationCount = 0;
    for (int i = 0; i < 100; i++) {
        penetrationList[i] = 0;
        weightList[i] = 0.0f;
        baseTypeList[i] = 0;
    }

    dotTargetCount = 0;
    for (int i = 0; i < 8; i++) {
        dotTargetList[i] = 0;
        dotTimerList[i] = 0.0f;
    }

    prevPenetrationWeight = penetrationWeight = 0.0f;

    formSwitch = false;
    playersEnteredFlag = 0;
}

void TriggerPhantom::PopulateIncludeAndExcludeLists() {
    unsigned int i;

    for (i = 0; i < excludedEntCount; i++) {
        xBase* base = zSceneFindObject(Asset->EntitiesExclude.data[i]);
        excludedEnt[i] = base;
    }

    for (i = 0; i < includedEntCount; i++) {
        xBase* base = zSceneFindObject(Asset->EntitiesInclude.data[i]);
        includedEnt[i] = base;
    }
}

void TriggerPhantom::Update(float dt) {
    UpdatePenetrations();

    if (enabled && Asset->DotDamageAmount > 0.0f &&
        Asset->DotDamageFreq > 0.0f) {
        DealDotDamage(dt);
    }
}

// Retail inlines the collector's constructors here; -inline auto emits
// them out of line.
#pragma push
#pragma always_inline on
void TriggerPhantom::UpdatePenetrations() {
    if (Asset->Type == 0) {
        for (int i = 0;
             i < ((hkpAabbPhantom*)collector)->getOverlappingCollidables().getSize();
             i++) {
            hkpCollidable* c =
                ((hkpAabbPhantom*)collector)->getOverlappingCollidables()[i];

            if (CanPenetrate(c)) {
                xBase* to = (xBase*)hkGetWorldObject(c)->getUserData();

                if (to && IsAffectedByTrigger(to)) {
                    if (enabled) {
                        ApplyTriggerPhysics(c, false);
                    }
                }
            }
        }
    } else {
        hkpAllCdBodyPairCollector collPenetrations;
        ((hkpShapePhantom*)collector)->getPenetrations(collPenetrations, 0);

        for (int i = 0; i < penetrationCount; i++) {
            bool hasLeft = true;

            if (enabled) {
                for (int j = 0; j < collPenetrations.getHits().getSize(); j++) {
                    if (penetrationList[i] ==
                        collPenetrations.getHits()[j].m_rootCollidableB) {
                        hasLeft = false;
                    }
                }
            }

            if (hasLeft) {
                if (baseTypeList[i] == 0x57) {
                    RemovePenetratingCollidableFromList(i);
                    zEntEvent(0, 0, this, 0x36599286, 0, FE_NO);

                    if (Asset->Threshold - 1 == penetrationCount) {
                        zEntEvent(0, 0, this, 0x0B28DA21, 0, FE_NO);
                    }

                    if (NotifyWeightThresholdExit(penetrationWeight,
                                                  prevPenetrationWeight)) {
                        zEntEvent(0, 0, this, 0x4DE9E9E9, 0, FE_NO);
                    }
                } else if (baseTypeList[i] != 0x55) {
                    xBase* to =
                        (xBase*)hkGetWorldObject(penetrationList[i])->getUserData();
                    zEntEvent(0, 0, this, 0x36599286, 0, FE_NO);

                    if (baseTypeList[i] != 0x72 && to && IsAffectedByTrigger(to)) {
                        Listen(to, 0x83B16590);
                    }

                    if (Asset->Threshold - 1 == penetrationCount) {
                        zEntEvent(0, 0, this, 0x0B28DA21, 0, FE_NO);
                    }

                    if (NotifyWeightThresholdExit(penetrationWeight,
                                                  prevPenetrationWeight)) {
                        zEntEvent(0, 0, this, 0x4DE9E9E9, 0, FE_NO);
                    }

                    RemovePenetratingCollidableFromList(i);
                } else if (baseTypeList[i] == 0x55) {
                    xBase* to =
                        (xBase*)hkGetWorldObject(penetrationList[i])->getUserData();

                    if (IsDotTarget(to)) {
                        RemoveFromDotTargetList(to);
                    }

                    RemovePenetratingCollidableFromList(i);

                    if (to && IsAffectedByTrigger(to)) {
                        zEntEvent(0, 0, this, 0x36599286, 0, FE_NO);
                        Listen(to, 0x83B16590);

                        if (Asset->Threshold - 1 == penetrationCount) {
                            zEntEvent(0, 0, this, 0x0B28DA21, 0, FE_NO);
                        }

                        if (NotifyWeightThresholdExit(penetrationWeight,
                                                      prevPenetrationWeight)) {
                            zEntEvent(0, 0, this, 0x4DE9E9E9, 0, FE_NO);
                        }

                        ((zPlayer*)to)->OnTriggerExit(this);
                        zEntEvent(0, 0, this,
                                  ((zPlayer*)to)->playerIndex + 0xDD3D4B1F, 0, FE_NO);
                        playersEnteredFlag &= ~(1 << ((zPlayer*)to)->playerIndex);

                        if (playersEnteredFlag == 0) {
                            zEntEvent(0, 0, this, 0x36599173, 0, FE_NO);
                        }
                    }
                }
            }
        }

        for (int i = 0; i < collPenetrations.getHits().getSize(); i++) {
            bool hasEntered = true;

            if (enabled) {
                for (int j = 0; j < penetrationCount; j++) {
                    if (collPenetrations.getHits()[i].m_rootCollidableB ==
                        penetrationList[j]) {
                        hasEntered = false;
                    }
                }
            } else {
                hasEntered = false;
            }

            hkpWorldObject* obj =
                hkGetWorldObject(collPenetrations.getHits()[i].m_rootCollidableB);

            if (hasEntered &&
                CanPenetrate(collPenetrations.getHits()[i].m_rootCollidableB) &&
                CheckForActiveBodyPartProperty(obj)) {
                xBase* to = (xBase*)hkGetWorldObject(
                                collPenetrations.getHits()[i].m_rootCollidableB)
                                ->getUserData();

                if (to && IsAffectedByTrigger(to)) {
                    zEntEvent(0, 0, this, 0x8A812DD8, 0, FE_NO);
                    Listen(to, 0xD6BC5172);

                    if (to->baseType == 0x55) {
                        zEntEvent(0, 0, this,
                                  ((zPlayer*)to)->playerIndex + 0x22475DE5, 0, FE_NO);
                        playersEnteredFlag |= ((zPlayer*)to)->playerIndex + 1;

                        if (playersEnteredFlag == 0xFFFF) {
                            zEntEvent(0, 0, this, 0x8A812CC5, 0, FE_NO);
                        }
                    }

                    ApplyTriggerPhysics(
                        collPenetrations.getHits()[i].m_rootCollidableB, true);

                    if (IsDotTarget(to)) {
                        AddToDotTargetList(to);
                    }

                    AddPenetratingCollidableToList(
                        collPenetrations.getHits()[i].m_rootCollidableB);

                    if (Asset->Threshold <= penetrationCount) {
                        zEntEvent(0, 0, this, 0x02CA28A3, 0, FE_NO);
                    }

                    if (NotifyWeightThresholdEnter(penetrationWeight,
                                                   prevPenetrationWeight)) {
                        zEntEvent(0, 0, this, 0xDE922C1B, 0, FE_NO);
                    }
                }
            } else if (!hasEntered &&
                       CanPenetrate(collPenetrations.getHits()[i].m_rootCollidableB)) {
                xBase* to = (xBase*)hkGetWorldObject(
                                collPenetrations.getHits()[i].m_rootCollidableB)
                                ->getUserData();

                if (to && IsAffectedByTrigger(to)) {
                    if (enabled) {
                        ApplyTriggerPhysics(
                            collPenetrations.getHits()[i].m_rootCollidableB, false);
                    }
                }
            }
        }
    }
}
#pragma pop

void TriggerPhantom::AddPenetratingCollidableToList(const hkpCollidable* pColl) {
    penetrationList[penetrationCount] = (hkpCollidable*)pColl;
    weightList[penetrationCount] = (int)getCollidableWeight(pColl);
    baseTypeList[penetrationCount] =
        ((xBase*)hkGetWorldObject(pColl)->getUserData())->baseType;

    prevPenetrationWeight = penetrationWeight;
    penetrationWeight += weightList[penetrationCount];

    penetrationCount++;
}

void TriggerPhantom::RemovePenetratingCollidableFromList(int index) {
    prevPenetrationWeight = penetrationWeight;
    penetrationWeight -= weightList[index];

    for (int i = index; i < penetrationCount - 1; i++) {
        penetrationList[i] = penetrationList[i + 1];
        weightList[i] = weightList[i + 1];
        baseTypeList[i] = baseTypeList[i + 1];
    }

    penetrationCount--;
}

void TriggerPhantom::AddToDotTargetList(xBase* target) {
    dotTargetList[dotTargetCount] = target;
    dotTimerList[dotTargetCount] = 0.0f;
    dotTargetCount++;
}

void TriggerPhantom::RemoveFromDotTargetList(xBase* target) {
    bool found = false;
    int i = 0;

    for (; i < dotTargetCount && !found; i++) {
        if (dotTargetList[i] == target) {
            found = true;
        }
    }

    for (; i < dotTargetCount - 1; i++) {
        dotTargetList[i] = dotTargetList[i + 1];
        dotTimerList[i] = dotTimerList[i + 1];
    }

    dotTargetCount--;
}

bool TriggerPhantom::IsDotTarget(xBase* target) {
    if (IsAffectedByTrigger(target) == false) {
        return false;
    }

    if (Asset->DotDamageAmount == 0.0f || Asset->DotDamageFreq == 0.0f) {
        return false;
    }

    return target->baseType == 0x55;
}

bool TriggerPhantom::CanPenetrate(const hkpCollidable* pColl) {
    hkpRigidBody* rb = hkGetRigidBody(pColl);

    if (hkGetPhantom(pColl) != 0 &&
        !hkGetWorldObject(pColl)->hasProperty(7777)) {
        return true;
    }

    return rb != 0 && !rb->isFixed();
}

// Retail inlines the quaternion temporary's constructor here; -inline auto
// emits it out of line.
#pragma push
#pragma always_inline on
void TriggerPhantom::Reset() {
    xHavok_RemoveFromSimWorld(collector);

    switch (Asset->Type) {
    case 0: {
        xMat4x3 matrix;
        Math::Matrix43 matNg(Asset->Transform);
        xMat4x3FromNGMatrix(&matrix, &matNg);

        xVec3 scale;
        xMat3x3GetScale(&matrix, &scale);

        Math::Vector4 posAABBox(matrix.pos, 0.0f);
        Math::Vector4 scaleAABBox(scale, 0.0f);

        hkAabb infoAABB;
        CalculateAABB(posAABBox, scaleAABBox, infoAABB);

        shape = 0;
        collector = new AabbPhantomCollector(infoAABB, this, 15);
        break;
    }

    case 1: {
        xMat4x3 matrix;
        Math::Matrix43 matNg(Asset->Transform);
        xMat4x3FromNGMatrix(&matrix, &matNg);

        xVec3 scale;
        xMat3x3GetScale(&matrix, &scale);

        matrix.right.normalize();
        matrix.up.normalize();
        matrix.at.normalize();

        hkRotation rt;
        rt.setIdentity();
        rt.setCols(Math::Vector4(matrix.right, 0.0f), Math::Vector4(matrix.up, 0.0f),
                   Math::Vector4(matrix.at, 0.0f));

        hkTransform transform;
        transform.setRotation(rt);
        transform.setTranslation(Math::Vector4(matrix.pos, 1.0f));

        hkVector4 halfExtent;
        ((Math::Vector4&)halfExtent)
            .Assign(0.5f * scale.x, 0.5f * scale.y, 0.5f * scale.z, 0.0f);

        shape = new hkpBoxShape(halfExtent, 0.0f);
        collector = new SimpleShapePhantomCollector(shape, transform, this, 15);
        shape->removeReference();
        ((SimpleShapePhantomCollector*)collector)->type = Asset->Type;
        break;
    }

    case 2: {
        xMat4x3 matrix;
        Math::Matrix43 matNg(Asset->Transform);
        xMat4x3FromNGMatrix(&matrix, &matNg);

        xVec3 scale;
        xMat3x3GetScale(&matrix, &scale);

        xVec3* pos = &matrix.pos;
        hkVector4 trans;
        ((Math::Vector4&)trans).Assign(pos->x, pos->y, pos->z, 1.0f);

        xQuat quat;
        xMat3x3 tempMat = matrix;
        xVec3Normalize(&tempMat.right, &tempMat.right); xVec3Normalize(&tempMat.up, &tempMat.up); xVec3Normalize(&tempMat.at, &tempMat.at);
        xQuatFromMat(&quat, &tempMat);
        xQuatNormalize(&quat, &quat);

        hkTransform transform;
        transform.getRotation().set(Math::Vector4(quat));
        transform.setTranslation(trans);

        shape = new hkpSphereShape(scale.z);
        collector = new SimpleShapePhantomCollector(shape, transform, this, 15);
        shape->removeReference();
        ((SimpleShapePhantomCollector*)collector)->type = Asset->Type;
        break;
    }

    case 3: {
        xMat4x3 matrix;
        Math::Matrix43 matNg(Asset->Transform);
        xMat4x3FromNGMatrix(&matrix, &matNg);

        xVec3 scale;
        xMat3x3GetScale(&matrix, &scale);

        xQuat quat;
        xMat3x3 tempMat = matrix;
        xVec3Normalize(&tempMat.right, &tempMat.right); xVec3Normalize(&tempMat.up, &tempMat.up); xVec3Normalize(&tempMat.at, &tempMat.at);
        xQuatFromMat(&quat, &tempMat);
        xQuatNormalize(&quat, &quat);

        hkTransform transform;
        transform.getRotation().set(Math::Vector4(quat));
        transform.setTranslation(Math::Vector4(matrix.pos, 1.0f));

        float capRadius = scale.x;
        float height = scale.y;
        hkVector4 capV0;
        ((Math::Vector4&)capV0).Assign(0.0f, 0.0f, 0.0f, 0.0f);
        hkVector4 capV1;
        ((Math::Vector4&)capV1).Assign(0.0f, height, 0.0f, 0.0f);

        shape = new hkpCapsuleShape(capV0, capV1, capRadius);
        collector = new SimpleShapePhantomCollector(shape, transform, this, 15);
        shape->removeReference();
        ((SimpleShapePhantomCollector*)collector)->type = Asset->Type;
        break;
    }
    }

    if (Asset->Type <= 3) {
        xHavok_AddToSimWorld(collector);

        hkpPropertyValue val(1);
        collector->addProperty(7777, val);
    }

    enabled = Asset->baseFlags & 1;

    penetrationCount = 0;
    for (int i = 0; i < 100; i++) {
        penetrationList[i] = 0;
        weightList[i] = 0.0f;
        baseTypeList[i] = 0;
    }

    dotTargetCount = 0;
    for (int i = 0; i < 8; i++) {
        dotTargetList[i] = 0;
        dotTimerList[i] = 0.0f;
    }

    prevPenetrationWeight = penetrationWeight = 0.0f;
}
#pragma pop

// Retail inlines the quaternion temporary's constructor here; -inline auto
// emits it out of line.
#pragma push
#pragma always_inline on
void TriggerPhantom::CreateCollector() {
    switch (Asset->Type) {
    case 0: {
        xMat4x3 matrix;
        Math::Matrix43 matNg(Asset->Transform);
        xMat4x3FromNGMatrix(&matrix, &matNg);

        xVec3 scale;
        xMat3x3GetScale(&matrix, &scale);

        Math::Vector4 posAABBox(matrix.pos, 0.0f);
        Math::Vector4 scaleAABBox(scale, 0.0f);

        hkAabb infoAABB;
        CalculateAABB(posAABBox, scaleAABBox, infoAABB);

        shape = 0;
        collector = new AabbPhantomCollector(infoAABB, this, 15);
        break;
    }

    case 1: {
        xMat4x3 matrix;
        Math::Matrix43 matNg(Asset->Transform);
        xMat4x3FromNGMatrix(&matrix, &matNg);

        xVec3 scale;
        xMat3x3GetScale(&matrix, &scale);

        matrix.right.normalize();
        matrix.up.normalize();
        matrix.at.normalize();

        hkRotation rt;
        rt.setIdentity();
        rt.setCols(Math::Vector4(matrix.right, 0.0f), Math::Vector4(matrix.up, 0.0f),
                   Math::Vector4(matrix.at, 0.0f));

        hkTransform transform;
        transform.setRotation(rt);
        transform.setTranslation(Math::Vector4(matrix.pos, 1.0f));

        hkVector4 halfExtent;
        ((Math::Vector4&)halfExtent)
            .Assign(0.5f * scale.x, 0.5f * scale.y, 0.5f * scale.z, 0.0f);

        shape = new hkpBoxShape(halfExtent, 0.0f);
        collector = new SimpleShapePhantomCollector(shape, transform, this, 15);
        shape->removeReference();
        ((SimpleShapePhantomCollector*)collector)->type = Asset->Type;
        break;
    }

    case 2: {
        xMat4x3 matrix;
        Math::Matrix43 matNg(Asset->Transform);
        xMat4x3FromNGMatrix(&matrix, &matNg);

        xVec3 scale;
        xMat3x3GetScale(&matrix, &scale);

        xVec3* pos = &matrix.pos;
        hkVector4 trans;
        ((Math::Vector4&)trans).Assign(pos->x, pos->y, pos->z, 1.0f);

        xQuat quat;
        xMat3x3 tempMat = matrix;
        xVec3Normalize(&tempMat.right, &tempMat.right); xVec3Normalize(&tempMat.up, &tempMat.up); xVec3Normalize(&tempMat.at, &tempMat.at);
        xQuatFromMat(&quat, &tempMat);
        xQuatNormalize(&quat, &quat);

        hkTransform transform;
        transform.getRotation().set(Math::Vector4(quat));
        transform.setTranslation(trans);

        shape = new hkpSphereShape(scale.z);
        collector = new SimpleShapePhantomCollector(shape, transform, this, 15);
        shape->removeReference();
        ((SimpleShapePhantomCollector*)collector)->type = Asset->Type;
        break;
    }

    case 3: {
        xMat4x3 matrix;
        Math::Matrix43 matNg(Asset->Transform);
        xMat4x3FromNGMatrix(&matrix, &matNg);

        xVec3 scale;
        xMat3x3GetScale(&matrix, &scale);

        xQuat quat;
        xMat3x3 tempMat = matrix;
        xVec3Normalize(&tempMat.right, &tempMat.right); xVec3Normalize(&tempMat.up, &tempMat.up); xVec3Normalize(&tempMat.at, &tempMat.at);
        xQuatFromMat(&quat, &tempMat);
        xQuatNormalize(&quat, &quat);

        hkTransform transform;
        transform.getRotation().set(Math::Vector4(quat));
        transform.setTranslation(Math::Vector4(matrix.pos, 1.0f));

        float capRadius = scale.x;
        float height = scale.y;
        hkVector4 capV0;
        ((Math::Vector4&)capV0).Assign(0.0f, 0.0f, 0.0f, 0.0f);
        hkVector4 capV1;
        ((Math::Vector4&)capV1).Assign(0.0f, height, 0.0f, 0.0f);

        shape = new hkpCapsuleShape(capV0, capV1, capRadius);
        collector = new SimpleShapePhantomCollector(shape, transform, this, 15);
        shape->removeReference();
        ((SimpleShapePhantomCollector*)collector)->type = Asset->Type;
        break;
    }
    }
}
#pragma pop

void TriggerPhantom::Setup() { PopulateIncludeAndExcludeLists(); }

hkTransform TriggerPhantom::getTransform() {
    if (shape == 0 && Asset->Type == 0) {
        xMat4x3 matrix;
        Math::Matrix43 matNg(Asset->Transform);
        xMat4x3FromNGMatrix(&matrix, &matNg);

        xVec3 scale;
        xMat3x3GetScale(&matrix, &scale);

        Math::Vector4 posAABBox(matrix.pos, 0.0f);

        hkTransform trans;
        trans.setIdentity();
        // Built through xVec3: from the Vector4 itself the store's operands
        // load the other way round.
        trans.setTranslation(Math::Vector4(*(const xVec3*)posAABBox.v, 0.0f));
        return trans;
    }

    return ((hkpShapePhantom*)collector)->getTransform();
}

TriggerPhantom::~TriggerPhantom() {
    if (Asset->Type <= 3) {
        xHavok_AddToSimWorld(collector);
    }
}

void TriggerPhantom::DebugReset() {
    Asset = (Sext::TriggerPhantomAsset*)World::GetEntityManager()->FindAsset(id);
    enabled = Asset->baseFlags & 1;
    linkArray = &Asset->EventLinksNew;

    if (collector) {
        xHavok_AddToSimWorld(collector);
    }

    collector = 0;
    shape = 0;

    CreateCollector();

    if (excludedEntCount != Asset->EntitiesExclude.count) {
        if (excludedEnt && excludedEntCount < Asset->EntitiesExclude.count) {
            delete excludedEnt;
            excludedEnt = 0;
        }

        excludedEntCount = Asset->EntitiesExclude.count;
        if (!excludedEnt) {
            excludedEnt = (xBase**)xMemAlloc((Memory::GlobalHeapEnum)0,
                                             excludedEntCount * sizeof(xBase*),
                                             0, (eMemMgrTag)24);
        }
    }

    if (includedEntCount != Asset->EntitiesInclude.count) {
        if (includedEnt && includedEntCount < Asset->EntitiesInclude.count) {
            delete includedEnt;
            includedEnt = 0;
        }

        includedEntCount = Asset->EntitiesInclude.count;
        if (!includedEnt) {
            includedEnt = (xBase**)xMemAlloc((Memory::GlobalHeapEnum)0,
                                             includedEntCount * sizeof(xBase*),
                                             0, (eMemMgrTag)24);
        }
    }

    PopulateIncludeAndExcludeLists();

    if (Asset->Type <= 3) {
        xHavok_AddToSimWorld(collector);

        hkpPropertyValue val(1);
        collector->addProperty(7777, val);
    }

    Reset();
}

void TriggerPhantom::HandleEvent(xBase* from, xBase* to, unsigned int toEvent,
                                 Sext::EventAny* genericParams) {
    switch (toEvent) {
    case 0x5568A8C3:
        enabled = true;
        break;

    case 0x52183048:
        enabled = false;
        break;

    case 0x389E01C0:
        DebugReset();
        break;

    case 0xA8B93047:
        Reset();
        break;

    case 0x3FE52B13: {
        Sext::EventActionDrivenBy* paramst =
            (Sext::EventActionDrivenBy*)genericParams;
        unsigned int flags = 0;

        if (paramst != 0) {
            if (paramst->param0) {
                flags |= 1;
            }
            if (paramst->param1) {
                flags |= 2;
            }
            if (paramst->param2) {
                flags |= 4;
            }
        }

        World::xOGEntity* possibleDriver = (World::xOGEntity*)from;

        if (paramst != 0 && paramst->specificPassenger != 0) {
            possibleDriver =
                (World::xOGEntity*)zSceneFindObject(paramst->specificPassenger);

            if (possibleDriver == 0) {
                possibleDriver = (World::xOGEntity*)World::GetEntityManager()->FindAsset(
                    paramst->specificPassenger);
            }

            DriveAttach(possibleDriver, flags, -1);
        } else {
            DriveAttach(possibleDriver, flags, -1);
        }
        break;
    }

    case 0x3954A566:
        DriveOn();
        break;

    case 0x56509F60:
        DriveOff();
        break;

    case 0xF9090A3B:
        DriveDetach();
        break;
    }
}

xMat4x3* TriggerPhantom::DriveGetCurMat(int bone) {
    hkTransform curHKTrans = getTransform();
    xMat4x3Copy(&driveCurMat, &g_I3);
    hkVector4 curHKPos = curHKTrans.getTranslation();
    __ct__Q24Math6VectorFfff(&driveCurMat.pos, curHKPos(0), curHKPos(1),
                             curHKPos(2));
    hkRotation curHKRot = curHKTrans.getRotation();

    if (Asset->Type != 0) {
        __ct__Q24Math6VectorFfff(&driveCurMat.right, curHKRot(0, 0),
                                 curHKRot(0, 1), curHKRot(0, 2));
        __ct__Q24Math6VectorFfff(&driveCurMat.up, curHKRot(1, 0),
                                 curHKRot(1, 1), curHKRot(1, 2));
        __ct__Q24Math6VectorFfff(&driveCurMat.at, curHKRot(2, 0),
                                 curHKRot(2, 1), curHKRot(2, 2));
    }

    return &driveCurMat;
}

void TriggerPhantom::Listen(xBase* to, unsigned int eventToListen) {
    if (linkArray != 0 && linkArray->count != 0) {
        for (unsigned int i = 0; i < linkArray->count; i++) {
            Sext::LinkAssetBaseNew& link =
                ((Sext::LinkAssetBaseNew*)linkArray->data)[i];

            if (link.srcEvent.type == eventToListen) {
                zEntEvent(this, eventToListen, to, link.dstEvent.type,
                          (Sext::EventAny*)link.srcEvent.v, FE_NO);
            }
        }
    }
}

bool TriggerPhantom::IsAffectedByTrigger(xBase* base) {
    for (unsigned int i = 0; i < excludedEntCount; i++) {
        if (excludedEnt[i] == base) {
            return false;
        }

        if (excludedEnt[i] != 0) {
            if (excludedEnt[i]->typeID == 0x50) {
                if (((xGroup*)excludedEnt[i])->ContainsBase_Recursive(base)) {
                    return false;
                }
            }
        }
    }

    if (includedEntCount == 0) {
        return true;
    }

    for (unsigned int i = 0; i < includedEntCount; i++) {
        if (includedEnt[i] == base) {
            return true;
        }

        if (includedEnt[i] != 0) {
            if (includedEnt[i]->typeID == 0x50) {
                if (((xGroup*)includedEnt[i])->ContainsBase_Recursive(base)) {
                    return true;
                }
            }
        }
    }

    return false;
}

void TriggerPhantom::ApplyTriggerPhysics(const hkpCollidable* c, bool firstFrame) {
    if (CheckForActiveBodyPartProperty(hkGetWorldObject(c))) {
        hkpRigidBody* rb = hkGetRigidBody(c);
        hkpPhantom* phantom = hkGetPhantom(c);
        hkVector4 linVec;

        if (phantom) {
            xBase* base = (xBase*)phantom->getUserData();
            zPlayer* player;

            if (base->baseType == 0x55) {
                player = (zPlayer*)base;
            }
        }

        if (rb) {
            switch (Asset->Action) {
            case 0:
                firstFrame = firstFrame;
                break;

            case 1:
                if (firstFrame) {
                    ComputeVelocityInTriggerSpace(linVec);
                    rb->setLinearVelocity(linVec);
                }
                break;

            case 2:
                if (firstFrame || Asset->VelocityPoint.Continuous) {
                    ComputeVelocityToPoint(&rb->getPosition(), linVec);
                    rb->setLinearVelocity(linVec);
                }
                break;

            case 3:
                if (firstFrame) {
                    ComputeVelocityInTriggerSpace(linVec);
                    rb->applyLinearImpulse(linVec);
                }
                break;

            case 4:
                if (firstFrame) {
                    ComputeImpulseToPoint(&rb->getPosition(), linVec);
                    rb->applyLinearImpulse(linVec);
                }
                break;

            case 5:
                ComputeForceTorqueInTriggerSpace(&rb->getPosition(), linVec);
                rb->applyForce(Globals::dt, linVec);
                break;

            case 6:
                ComputeForceTorqueToPoint(&rb->getPosition(), linVec);
                rb->applyForce(Globals::dt, linVec);
                break;
            }
        } else if (phantom) {
            zPlayer* player;
            zNPCEntity* npc;
            zNPCSteering* npcSteering;
            xBase* base = (xBase*)phantom->getUserData();
            xVec3 vec;

            switch (Asset->Action) {
            case 0:
                firstFrame = firstFrame;
                break;

            case 1:
                if (firstFrame) {
                    ComputeVelocityInTriggerSpace(linVec);
                    xVec3Init(&vec, linVec(0), linVec(1), linVec(2));
                    if (base->baseType == 0x55) {
                        player = (zPlayer*)base;
                        player->ImpartVelocity(vec);
                    } else if (base->baseType == 0x38) {
                        npc = (zNPCEntity*)base;
                        npcSteering = npc->owner->npcSteering;
                        if (npcSteering) {
                            npcSteering->externalVelocityAccum += vec;
                        }
                    }
                }
                break;

            case 2:
                if (firstFrame || Asset->VelocityPoint.Continuous) {
                    if (base->baseType == 0x55) {
                        player = (zPlayer*)base;
                        ComputeVelocityToPoint(
                            &player->GetPhantomObject()->getTransform().getTranslation(),
                            linVec);
                        xVec3Init(&vec, linVec(0), linVec(1), linVec(2));
                        player->ImpartVelocity(vec);
                    } else if (base->baseType == 0x38) {
                        npc = (zNPCEntity*)base;
                        ComputeVelocityToPoint(
                            &npc->GetPhantomObject()->getTransform().getTranslation(),
                            linVec);
                        xVec3Init(&vec, linVec(0), linVec(1), linVec(2));
                        npcSteering = npc->owner->npcSteering;
                        if (npcSteering) {
                            npcSteering->externalVelocityAccum += vec;
                        }
                    }
                }
                break;

            case 3:
                if (firstFrame) {
                    ComputeVelocityInTriggerSpace(linVec);
                    xVec3Init(&vec, linVec(0), linVec(1), linVec(2));
                    if (base->baseType == 0x55) {
                        player = (zPlayer*)base;
                        player->ImpartImpulse(vec);
                    } else if (base->baseType == 0x38) {
                        npc = (zNPCEntity*)base;
                        npcSteering = npc->owner->npcSteering;
                        if (npcSteering) {
                            npcSteering->ApplyExternalImpulse(&vec);
                        }
                    }
                }
                break;

            case 4:
                if (firstFrame) {
                    if (base->baseType == 0x55) {
                        player = (zPlayer*)base;
                        ComputeImpulseToPoint(
                            &player->GetPhantomObject()->getTransform().getTranslation(),
                            linVec);
                        xVec3Init(&vec, linVec(0), linVec(1), linVec(2));
                        player->ImpartImpulse(vec);
                    } else if (base->baseType == 0x38) {
                        npc = (zNPCEntity*)base;
                        ComputeImpulseToPoint(
                            &npc->GetPhantomObject()->getTransform().getTranslation(),
                            linVec);
                        xVec3Init(&vec, linVec(0), linVec(1), linVec(2));
                        npcSteering = npc->owner->npcSteering;
                        if (npcSteering) {
                            npcSteering->ApplyExternalImpulse(&vec);
                        }
                    }
                }
                break;

            case 5:
                if (base->baseType == 0x55) {
                    player = (zPlayer*)base;
                    ComputeForceTorqueInTriggerSpace(
                        &player->GetPhantomObject()->getTransform().getTranslation(),
                        linVec);
                    xVec3Init(&vec, linVec(0), linVec(1), linVec(2));
                    player->ImpartForce(vec);
                } else if (base->baseType == 0x38) {
                    npc = (zNPCEntity*)base;
                    ComputeForceTorqueInTriggerSpace(
                        &npc->GetPhantomObject()->getTransform().getTranslation(),
                        linVec);
                    xVec3Init(&vec, linVec(0), linVec(1), linVec(2));
                    zNPCSteering* npcSteering = npc->owner->npcSteering;
                    if (npcSteering) {
                        npcSteering->ApplyExternalForce(&vec);
                    }
                }
                break;

            case 6:
                if (base->baseType == 0x55) {
                    player = (zPlayer*)base;
                    ComputeForceTorqueToPoint(
                        &player->GetPhantomObject()->getTransform().getTranslation(),
                        linVec);
                    xVec3Init(&vec, linVec(0), linVec(1), linVec(2));
                    player->ImpartForce(vec);
                } else if (base->baseType == 0x38) {
                    npc = (zNPCEntity*)base;
                    ComputeForceTorqueToPoint(
                        &npc->GetPhantomObject()->getTransform().getTranslation(),
                        linVec);
                    xVec3Init(&vec, linVec(0), linVec(1), linVec(2));
                    zNPCSteering* npcSteering = npc->owner->npcSteering;
                    if (npcSteering) {
                        npcSteering->ApplyExternalForce(&vec);
                    }
                }
                break;
            }
        }
    }
}

void TriggerPhantom::ComputeVelocityInTriggerSpace(hkVector4& output) {
    ((Math::Vector4&)output)
        .Assign(Asset->VelocityTrigger.Direction.x,
                Asset->VelocityTrigger.Direction.y,
                Asset->VelocityTrigger.Direction.z, 0.0f);
    hkRotation rotation = getTransform().getRotation();
    output._setRotatedDir(rotation, output);
    output.normalize3();
    output.mul4(Asset->VelocityTrigger.Magnitude);
}

void TriggerPhantom::ComputeForceTorqueInTriggerSpace(const hkVector4* subjectPos,
                                                      hkVector4& output) {
    ((Math::Vector4&)output)
        .Assign(Asset->ForceTorqueTrigger.Direction.x,
                Asset->ForceTorqueTrigger.Direction.y,
                Asset->ForceTorqueTrigger.Direction.z, 0.0f);
    hkRotation rotation = getTransform().getRotation();
    output._setRotatedDir(rotation, output);
    output.normalize3();

    if (Asset->ForceTorqueTrigger.FallOff) {
        hkVector4 subjectDisplacementFromCenter = *subjectPos;
        subjectDisplacementFromCenter.sub4(getTransform().getTranslation());
        hkVector4 fallOffVec = output;
        fallOffVec.mul4(Asset->ForceTorqueTrigger.FallOffDist);
        float fallOffFactor =
            1.0f - (fallOffVec.m_quad[0] * subjectDisplacementFromCenter.m_quad[0] +
                    fallOffVec.m_quad[1] * subjectDisplacementFromCenter.m_quad[1] +
                    fallOffVec.m_quad[2] * subjectDisplacementFromCenter.m_quad[2]) /
                       Asset->ForceTorqueTrigger.FallOffDist;
        if (fallOffFactor < 0.0f) {
            fallOffFactor = 0.0f;
        }
        output.mul4(fallOffFactor);
    }

    output.mul4(Asset->ForceTorqueTrigger.Magnitude);
}

float TriggerPhantom::getCollidableWeight(const hkpCollidable* c) {
    hkpRigidBody* rb = hkGetRigidBody(c);
    hkpPhantom* phantom = hkGetPhantom(c);

    if (rb != 0) {
        if (rb->getMotionType() == hkpMotion::MOTION_KEYFRAMED) {
            xBase* base = (xBase*)rb->getUserData();

            if (base->baseType == 0x57) {
                return ((zProjectileHavok*)base)
                    ->GetOwnerProjectile()
                    ->physicsEntity.GetMass();
            } else if (base->baseType == 0x55) {
                return ((zPlayer*)base)->pWeight;
            }
        }

        return rb->getMass();
    } else if (phantom != 0) {
        xBase* base = (xBase*)phantom->getUserData();

        if (base->baseType == 0x55) {
            return ((zPlayer*)base)->pWeight;
        } else if (base->baseType == 0x38) {
            return 1000.0f;
        }
    }

    return 0.0f;
}

bool TriggerPhantom::NotifyWeightThresholdEnter(float weight, float prevWeight) {
    return Asset->WeightThreshold <= weight &&
           prevWeight < Asset->WeightThreshold;
}

bool TriggerPhantom::NotifyWeightThresholdExit(float weight, float prevWeight) {
    return weight < Asset->WeightThreshold &&
           Asset->WeightThreshold <= prevWeight;
}

void TriggerPhantom::CalculateAABB(Math::Vector4& posAABBox,
                                   Math::Vector4& scaleAABBox,
                                   hkAabb& infoAABB) {
    ((Math::Vector4&)infoAABB.m_min)
        .Assign(-0.5f * scaleAABBox.v[0] + posAABBox.v[0],
                -0.5f * scaleAABBox.v[1] + posAABBox.v[1],
                -0.5f * scaleAABBox.v[2] + posAABBox.v[2], 0.0f);
    ((Math::Vector4&)infoAABB.m_max)
        .Assign(0.5f * scaleAABBox.v[0] + posAABBox.v[0],
                0.5f * scaleAABBox.v[1] + posAABBox.v[1],
                0.5f * scaleAABBox.v[2] + posAABBox.v[2], 0.0f);
}

AabbPhantomCollector::AabbPhantomCollector(const hkAabb& aabb,
                                           TriggerPhantom* ptr,
                                           unsigned int collisionFilterInfo)
    : hkpAabbPhantom(aabb, collisionFilterInfo) {
    owner = ptr;
    overlapCount = 0;
}

void AabbPhantomCollector::addOverlappingCollidable(hkpCollidable* c) {
    hkpRigidBody* rb = hkGetRigidBody(c);
    hkpPhantom* phantom = hkGetPhantom(c);
    hkpWorldObject* wo = hkGetWorldObject(c);

    if (phantom) {
        if (wo->hasProperty(8888)) {
            hkpAabbPhantom::addOverlappingCollidable(c);
            zEntEvent(0, 0, owner, 0xDEFDA6F1, 0, FE_NO);
        } else if (!wo->hasProperty(7777) && CheckForActiveBodyPartProperty(wo)) {
            hkpAabbPhantom::addOverlappingCollidable(c);
            xBase* to = (xBase*)phantom->getUserData();

            if (to && owner->IsAffectedByTrigger(to)) {
                if (owner->enabled) {
                    zEntEvent(0, 0, owner, 0x8A812DD8, 0, FE_NO);
                    owner->Listen(to, 0xD6BC5172);
                    owner->ApplyTriggerPhysics(c, true);
                }

                if (owner->IsDotTarget(to)) {
                    owner->AddToDotTargetList(to);
                }
            }

            ++overlapCount;
            if (owner->Asset->Threshold <= overlapCount && owner->enabled) {
                zEntEvent(0, 0, owner, 0x02CA28A3, 0, FE_NO);
            }
        }
    } else if (rb) {
        bool penetrable = false; if (!rb->isFixed()) { if (CheckForActiveBodyPartProperty(wo)) penetrable = true; } if (penetrable) {
            hkpAabbPhantom::addOverlappingCollidable(c);
            xBase* to = (xBase*)rb->getUserData();

            if (to && owner->IsAffectedByTrigger(to)) {
                if (owner->enabled) {
                    zEntEvent(0, 0, owner, 0x8A812DD8, 0, FE_NO);
                    owner->Listen(to, 0xD6BC5172);
                    owner->ApplyTriggerPhysics(c, true);
                }

                if (owner->IsDotTarget(to)) {
                    owner->AddToDotTargetList(to);
                }
            }

            ++overlapCount;
            if (owner->Asset->Threshold <= overlapCount && owner->enabled) {
                zEntEvent(0, 0, owner, 0x02CA28A3, 0, FE_NO);
            }
        }
    }
}

void AabbPhantomCollector::removeOverlappingCollidable(hkpCollidable* c) {
    hkpRigidBody* rb = hkGetRigidBody(c);
    hkpPhantom* phantom = hkGetPhantom(c);
    hkpWorldObject* wo = hkGetWorldObject(c);

    if (phantom) {
        if (wo->hasProperty(8888)) {
            hkpAabbPhantom::removeOverlappingCollidable(c);
            zEntEvent(0, 0, owner, 0x6066F24B, 0, FE_NO);
        } else if (!wo->hasProperty(7777) && CheckForActiveBodyPartProperty(wo)) {
            hkpAabbPhantom::removeOverlappingCollidable(c);
            xBase* to = (xBase*)phantom->getUserData();

            if (to && owner->IsAffectedByTrigger(to)) {
                if (owner->enabled) {
                    zEntEvent(0, 0, owner, 0x36599286, 0, FE_NO);
                    owner->Listen(to, 0x83B16590);
                }

                if (owner->IsDotTarget(to)) {
                    owner->RemoveFromDotTargetList(to);
                }
            }

            unsigned int count = overlapCount--; if (owner->Asset->Threshold - 1 == count && owner->enabled) {
                zEntEvent(0, 0, owner, 0x0B28DA21, 0, FE_NO);
            }
        }
    } else if (rb) {
        bool penetrable = false; if (!rb->isFixed()) { if (CheckForActiveBodyPartProperty(wo)) penetrable = true; } if (penetrable) {
            hkpAabbPhantom::removeOverlappingCollidable(c);
            xBase* to = (xBase*)rb->getUserData();

            if (to && owner->IsAffectedByTrigger(to)) {
                if (owner->enabled) {
                    zEntEvent(0, 0, owner, 0x36599286, 0, FE_NO);
                    owner->Listen(to, 0x83B16590);
                }

                if (owner->IsDotTarget(to)) {
                    owner->RemoveFromDotTargetList(to);
                }
            }

            unsigned int count = overlapCount--; if (owner->Asset->Threshold - 1 == count && owner->enabled) {
                zEntEvent(0, 0, owner, 0x0B28DA21, 0, FE_NO);
            }
        }
    }
}

SimpleShapePhantomCollector::SimpleShapePhantomCollector(
    const hkpShape* shape, const hkTransform& transform, TriggerPhantom* ptr,
    unsigned int collisionFilterInfo)
    : hkpSimpleShapePhantom(shape, transform, collisionFilterInfo) {
    owner = ptr;
    overlapCount = 0;
    type = 0;
}

void SimpleShapePhantomCollector::addOverlappingCollidable(hkpCollidable* c) {
    if (owner->CanPenetrate(c)) {
        hkpSimpleShapePhantom::addOverlappingCollidable(c);
    }
}

void SimpleShapePhantomCollector::removeOverlappingCollidable(hkpCollidable* a0) { hkpSimpleShapePhantom::removeOverlappingCollidable(a0); }

inline hkRotation::hkRotation(const hkRotation& other) {
    m_col0 = other.m_col0;
    m_col1 = other.m_col1;
    m_col2 = other.m_col2;
}

inline float xVec3Normalize(xVec3* o, const xVec3* v) {
    float len;
    {
        float len2 = v->x * v->x + v->y * v->y + v->z * v->z;
        if (iabs(len2 - 1.0f) <= 1e-5f) {
            len = 1.0f;
            o->x = v->x;
            o->y = v->y;
            o->z = v->z;
        } else if (iabs(len2 - 0.0f) <= 1e-5f) {
            len = 0.0f;
            o->x = 0.0f;
            o->y = 1.0f;
            o->z = 0.0f;
        } else {
            len = xsqrt(len2);
            float len_inv = 1.0f / len;
            o->x = v->x * len_inv;
            o->y = v->y * len_inv;
            o->z = v->z * len_inv;
        }
    }
    return len;
}
