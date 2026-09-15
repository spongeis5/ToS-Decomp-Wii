#include "SB/GM/Engine/WAD00.pool.h"

// WAD00.cpp -- the front of the WAD00 unity build: xHavokPhysicsObject, the
// game's wrapper around a Havok physics system (one shape, several shapes,
// or a packed system of jointed bodies), and the Math and Havok inlines it
// instantiates. Read from the image with tools/brief.py; the layouts are the
// DWARF's (tools/dwarf_types.py).
//
// The six accessors at the foot were written by tools/gen_accessors.py as
// out-of-line definitions of what retail has as weak copies with no caller
// in this unit; they are kept as they were.
//
// `#pragma always_inline on` holds in three regions, each measured (NOTES.md,
// "WAD00"): Cleanup and the function after it, SetMotionType and the
// function after it, and the weak copies from deallocateChunkConstSize to
// the generated accessors. On for the whole file it also inlines ordinary
// members into their callers.

class Dummy;
class hkpRigidBody;
class hkpPhysicsSystem;

typedef unsigned long hkUlong;

enum hkResult {
    HK_SUCCESS = 0,
    HK_FAILURE = 1
};

enum HK_MEMORY_CLASS {
    HK_MEMORY_CLASS_MAP = 29
};

enum E_HAVOK_COLLIDE_FILTER_LAYER {
    eNoCollisionLayer = 31
};

void xHavok_SetCollisionFilterInfo(hkpRigidBody* body,
                                   E_HAVOK_COLLIDE_FILTER_LAYER layer);

// ---------------------------------------------------------------------------
// Havok values

class hkBool {
public:
    hkBool(bool b) { m_bool = (char)b; }

    operator bool() const { return m_bool != 0; }

    char m_bool;
};

// Weak copies in the image, called out of line: the inlines are defined at
// the foot of the file, below every caller.
class hkVector4 {
public:
    void operator=(const hkVector4& v);
    void setZero4();
    void setSub4(const hkVector4& a, const hkVector4& b);
    void mul4(float s);
    float length3() const;

    float& operator()(int i) { return (&x)[i]; }
    const float& operator()(int i) const { return (&x)[i]; }

    float x __attribute__((aligned(16)));
    float y;
    float z;
    float w;
};

extern const hkVector4 hkVector4Zero;

class hkMatrix3 {
public:
    // Through it, setIdentity keeps each column's address for the diagonal.
    hkVector4& getColumn(int i) { return (&m_col0)[i]; }
    const hkVector4& getColumn(int i) const { return (&m_col0)[i]; }

    const float& operator()(int row, int col) const { return getColumn(col)(row); }

    void setIdentity();

    void setCols(const hkVector4& c0, const hkVector4& c1, const hkVector4& c2) {
        m_col0 = c0;
        m_col1 = c1;
        m_col2 = c2;
    }

    hkVector4 m_col0;
    hkVector4 m_col1;
    hkVector4 m_col2;
};

class hkQuaternion;

class hkRotation : public hkMatrix3 {
public:
    void set(const hkQuaternion& q);
};

class hkQuaternion {
public:
    hkQuaternion(const hkRotation& r) { set(r); }

    void set(const hkRotation& r);

    hkVector4 m_vec;
};

class hkTransform {
public:
    hkTransform() {}
    hkTransform(const hkQuaternion& q, const hkVector4& t);

    hkTransform& operator=(const hkTransform& t);
    void setIdentity();
    void setTranslation(const hkVector4& t) { m_translation = t; }
    void setMul(const hkTransform& a, const hkTransform& b);
    void setMulInverseMul(const hkTransform& bTa, const hkTransform& bTc);
    void setMulMulInverse(const hkTransform& a, const hkTransform& b);
    void setMulEq(const hkTransform& b);
    void get4x4ColumnMajor(float* d) const;
    void set4x4ColumnMajor(const float* p);

    hkRotation& getRotation() { return m_rotation; }
    const hkRotation& getRotation() const { return m_rotation; }
    const hkVector4& getTranslation() const { return m_translation; }

    const float& operator()(int row, int col) const { return m_rotation(row, col); }

    hkRotation m_rotation;
    hkVector4 m_translation;
};

class hkAabb {
public:
    hkVector4 m_min;
    hkVector4 m_max;
};

template <class T>
class hkArray {
public:
    int getSize() const { return m_size; }
    T* begin() const { return m_data; }
    T* end() const { return m_data + m_size; }
    T& operator[](int i) const { return m_data[i]; }

    T* m_data;
    int m_size;
    int m_capacityAndFlags;
};

class hkClass;
class hkStatisticsCollector;

class hkBaseObject {
public:
    virtual ~hkBaseObject();
};

class hkReferencedObject : public hkBaseObject {
public:
    virtual const hkClass* getClassType() const;
    virtual void calcContentStatistics(hkStatisticsCollector* collector,
                                       const hkClass* cls) const;

    void removeReference() const;
    int getReferenceCount() const { return m_referenceCount; }

    unsigned short m_memSizeAndFlags;
    short m_referenceCount;
};

// ---------------------------------------------------------------------------
// Havok memory and maps

class hkThreadMemory {
public:
    class FreeElem {
    public:
        FreeElem* m_next;
    };

    class FreeList {
    public:
        void put(void* p) {
            m_numElem++;
            FreeElem* n = (FreeElem*)p;
            n->m_next = m_head;
            m_head = n;
        }

        FreeElem* m_head;
        int m_numElem;
    };

    static hkThreadMemory& getInstance();
    static int constSizeToRow(int size);

    void deallocateChunkConstSize(void* p, int nbytes, HK_MEMORY_CLASS cl);
    void onRowFull(int row, void* p, HK_MEMORY_CLASS cl);

    unsigned char _pad0[0x24];
    int m_maxNumElemsOnFreeList;
    FreeList m_free_list[17];
};

extern hkThreadMemory* hkThreadMemory__s_threadMemoryInstance;

inline hkThreadMemory& hkThreadMemory::getInstance() {
    return *hkThreadMemory__s_threadMemoryInstance;
}

template <class K, class V>
class hkPointerMapOperations {};

template <class K, class V, class OPS>
class hkPointerMapBase {
public:
    class Pair {
    public:
        K key;
        V val;
    };

    ~hkPointerMapBase();

    Dummy* findKey(K key) const;
    hkResult get(K key, V* out) const;
    void remove(Dummy* it);
    void clear();

    int getSize() const { return m_numElems & 0x7FFFFFFF; }

    Pair* m_elem;
    int m_numElems;
    int m_hashMod;
};

template <class K, class V>
class hkPointerMap {
public:
    typedef hkPointerMapBase<hkUlong, hkUlong, hkPointerMapOperations<hkUlong, hkUlong> >
        Storage;

    Dummy* findKey(K key) const { return m_map.findKey(hkUlong(key)); }
    hkResult get(K key, V* out) const;
    hkBool isValid(Dummy* it) const;
    V getValue(Dummy* it) const;
    void remove(Dummy* it) { m_map.remove(it); }
    void clear() { m_map.clear(); }
    int getSize() const { return m_map.getSize(); }

    void operator delete(void* p, unsigned long nbytes) {
        if (p) {
            hkThreadMemory::getInstance().deallocateChunkConstSize(p, nbytes,
                                                                   HK_MEMORY_CLASS_MAP);
        }
    }

    Storage m_map;
};

// ---------------------------------------------------------------------------
// Havok bodies

class hkpGroupFilter {
public:
    static int getLayerFromFilterInfo(unsigned int info) { return info & 0x1F; }
    static int getSystemGroupFromFilterInfo(unsigned int info) { return info >> 16; }
    static int getSubSystemIdFromFilterInfo(unsigned int info) {
        return (info >> 5) & 0x1F;
    }
    static int getSubSystemDontCollideWithFromFilterInfo(unsigned int info) {
        return (info >> 10) & 0x1F;
    }
    static unsigned int calcFilterInfo(int layer, int systemGroup, int subSystemId,
                                       int subSystemDontCollideWith) {
        return (subSystemId << 5) | (subSystemDontCollideWith << 10) |
               (systemGroup << 16) | layer;
    }
};

class hkpMaterial {
public:
    float getFriction() const { return m_friction; }

    unsigned char m_responseType;
    float m_friction;
    float m_restitution;
};

class hkpWorld {
public:
    // Multithreading checks, compiled out.
    void markForWrite() {}
    void unmarkForWrite() {}
};

hkpWorld* xHavok_GetWorld();

class hkpShapeContainer;
struct CalcSizeForSpuInput;

enum hkpShapeType {
    HK_SHAPE_CONVEX_TRANSLATE = 11,
    HK_SHAPE_CONVEX_TRANSFORM = 12
};

// getAabb's slot is the image's (tools/vtslot.py __vt__8hkpShape 36: the
// first of three pure virtuals); the four slots before it are uncalled
// here, and their names are Havok's as best known, unchecked.
class hkpShape : public hkReferencedObject {
public:
    virtual float getMaximumProjection(const hkVector4& direction) const;
    virtual const hkpShapeContainer* getContainer() const;
    virtual hkBool isConvex() const;
    virtual int calcSizeForSpu(const CalcSizeForSpuInput& input,
                               int spuBufferSizeLeft) const;
    virtual void getAabb(const hkTransform& localToWorld, float tolerance,
                         hkAabb& out) const = 0;

    hkpShapeType getType() const { return (hkpShapeType)m_type; }

    unsigned long m_userData;
    unsigned int m_type;
};

class hkpSphereRepShape : public hkpShape {};

class hkpConvexShape : public hkpSphereRepShape {
public:
    float m_radius;
};

class hkpShapeContainer {
public:
    virtual ~hkpShapeContainer();
};

class hkpSingleShapeContainer : public hkpShapeContainer {
public:
    const hkpShape* getChild() const { return m_childShape; }

    const hkpShape* m_childShape;
};

class hkpConvexTransformShapeBase : public hkpConvexShape {
public:
    const hkpShape* getChildShape() const { return m_childShape.getChild(); }

    hkpSingleShapeContainer m_childShape;
    int m_childShapeSize;
};

class hkpConvexTranslateShape : public hkpConvexTransformShapeBase {
public:
    const hkVector4& getTranslation() const { return m_translation; }

    hkVector4 m_translation;
};

class hkpConvexTransformShape : public hkpConvexTransformShapeBase {
public:
    const hkTransform& getTransform() const { return m_transform; }

    hkTransform m_transform;
};

class hkpCdBody {
public:
    const hkpShape* getShape() const { return m_shape; }

    const hkpShape* m_shape;
};

class hkpCollidable : public hkpCdBody {
public:
    unsigned char _pad4[0x1C - 0x4];
    // m_broadPhaseHandle's
    unsigned int m_collisionFilterInfo;
};

class hkpWorldObject : public hkReferencedObject {
public:
    hkpWorld* getWorld() const { return m_world; }
    hkUlong getUserData() const { return m_userData; }
    void setUserData(hkUlong data) { m_userData = data; }
    const hkpCollidable* getCollidable() const { return &m_collidable; }

    unsigned int getCollisionFilterInfo() const {
        return m_collidable.m_collisionFilterInfo;
    }

    hkpWorld* m_world;
    hkUlong m_userData;
    hkpCollidable m_collidable;
    unsigned char _pad30[0x88 - 0x30];
};

// The slots are the image's (tools/vtslot.py __vt__16hkpMaxSizeMotion); the
// eight from setMass to getInertiaInvWorld are Havok's names for the slots
// the keyframed motion folds onto four bodies.
class hkpMotion : public hkReferencedObject {
public:
    enum MotionType {
        MOTION_KEYFRAMED = 6,
        MOTION_FIXED = 7
    };

    virtual void setMass(float m);
    virtual void setMassInv(float mInv);
    virtual void getInertiaLocal(hkMatrix3& inertiaOut) const;
    virtual void getInertiaWorld(hkMatrix3& inertiaOut) const;
    virtual void setInertiaLocal(const hkMatrix3& inertia);
    virtual void setInertiaInvLocal(const hkMatrix3& inertiaInv);
    virtual void getInertiaInvLocal(hkMatrix3& inertiaInvOut) const;
    virtual void getInertiaInvWorld(hkMatrix3& inertiaInvOut) const;
    virtual void setCenterOfMassInLocal(const hkVector4& centerOfMass);
    virtual void setPosition(const hkVector4& position);
    virtual void setRotation(const hkQuaternion& rotation);
    virtual void setPositionAndRotation(const hkVector4& position,
                                        const hkQuaternion& rotation);
    virtual void setTransform(const hkTransform& transform);
    virtual void setLinearVelocity(const hkVector4& newVel);
    virtual void setAngularVelocity(const hkVector4& newVel);
    virtual void getProjectedPointVelocity(const hkVector4& p, const hkVector4& normal,
                                           float& velOut, float& invVirtMassOut) const;
    virtual void applyLinearImpulse(const hkVector4& imp);

    float getMass() const;

    unsigned char m_type;
    // m_motionState's
    hkTransform m_transform;
    unsigned char _pad50[0xB4 - 0x50];
    float m_linearDamping;
    float m_angularDamping;
};

enum hkpEntityActivation {
    HK_ENTITY_ACTIVATION_DO_NOT_ACTIVATE = 0,
    HK_ENTITY_ACTIVATION_DO_ACTIVATE = 1
};

enum hkpUpdateCollisionFilterOnEntityMode {
    HK_UPDATE_FILTER_ON_ENTITY_FULL_CHECK = 0,
    HK_UPDATE_FILTER_ON_ENTITY_DISABLE_ENTITY_ENTITY_COLLISIONS_ONLY = 1
};

class hkpEntity : public hkpWorldObject {
public:
    hkBool isFixed() const { return m_motion.m_type == hkpMotion::MOTION_FIXED; }
    hkBool isActive() const;
    void activate();
    void deactivate();

    hkpMotion::MotionType getMotionType() const {
        return (hkpMotion::MotionType)m_motion.m_type;
    }

    const hkTransform& getTransform() const { return m_motion.m_transform; }
    const hkpMaterial& getMaterial() const { return m_material; }

    hkpMotion* getRigidMotion() const { return (hkpMotion*)&m_motion; }

    hkpMaterial m_material;
    unsigned char _pad94[0xE0 - 0x94];
    hkpMotion m_motion;
};

class hkpRigidBody : public hkpEntity {
public:
    void setTransform(const hkTransform& transform);
    void setFriction(float friction, float scale);
    void setMass(float m);
    void setMotionType(hkpMotion::MotionType newState,
                       hkpEntityActivation preferredActivationState,
                       hkpUpdateCollisionFilterOnEntityMode collisionFilterUpdateMode);

    float getMass() const { return getRigidMotion()->getMass(); }

    // Weak copies with no caller here: the pointers to members that
    // SetLinearVelocity and the rest pass need them.
    void applyLinearImpulse(const hkVector4& imp);
    void setLinearVelocity(const hkVector4& newVel);
    void setAngularVelocity(const hkVector4& newVel);

    float getRestitution() const;
    float getLinearDamping() const;
    float getAngularDamping() const;
    void setLinearDamping(float value);
    void setAngularDamping(float value);
};

void xHavok_UpdateRigidBodyMotion(hkpRigidBody* body, const hkVector4& pos,
                                  const hkQuaternion& rot, float dt);

class hkpCharacterProxy {
public:
    const hkVector4& getPosition() const;
};

class hkpCharacterRigidBody {
public:
    const hkVector4& getPosition() const;
    const hkVector4& getLinearVelocity() const;
};

class hkpPhysicsSystem : public hkReferencedObject {
public:
    const hkArray<hkpRigidBody*>& getRigidBodies() const { return m_rigidBodies; }

    hkArray<hkpRigidBody*> m_rigidBodies;
};

// ---------------------------------------------------------------------------
// Math

namespace Math {

class Vector4 {
public:
    class DataType {
    public:
        float x;
        float y;
        float z;
        float w;
    };

    void Assign(float x, float y, float z, float w);

    float& operator[](int i) { return ((float*)&data)[i]; }
    const float& operator[](int i) const { return ((float*)&data)[i]; }

    DataType data;
};

class Vector : public Vector4 {
public:
    Vector() {}
    Vector(float x, float y, float z);
};

class Quaternion {
public:
    Vector4 v;
};

class Matrix33 {
public:
    Matrix33();

    // An inline between the loads and the call binds its parameters right to
    // left, which is the order retail loads these arguments in; the name is
    // ours.
    void SetRow(int row, float x, float y, float z) { SetRowInternal(row, x, y, z); }
    void SetRowInternal(int row, float x, float y, float z);
    Vector GetRowInternal(int row) const;
    Quaternion GetQuaternion() const;

    Vector4 v[3];
};

class Matrix43 : public Matrix33 {
public:
    Matrix43() {}
    Matrix43(const Matrix43& a) { *this = a; }

    Matrix43& operator=(const Matrix43& a);
    void Assign(float x0, float y0, float z0, float x1, float y1, float z1,
                float x2, float y2, float z2, float x3, float y3, float z3);
    void SetPos(const Vector& pos);
    void MakeQuaternion(const Quaternion& q);
};

extern Matrix43 _matIdentity;

void Orthonormalize(Matrix43& o, const Matrix43& a);
void Normalize(Matrix43& o, const Matrix43& a);
void Slerp(Quaternion& o, const Quaternion& a, const Quaternion& b, float t);
void Add(Vector& o, const Vector& a, const Vector& b);

inline void Lerp(Vector4& o, const Vector4& a, const Vector4& b, float t) {
    float s = 1.0f - t;

    o[0] = a[0] * s + b[0] * t;
    o[1] = a[1] * s + b[1] * t;
    o[2] = a[2] * s + b[2] * t;
    o[3] = a[3] * s + b[3] * t;
}

}  // namespace Math

extern "C" void PSMTXConcat(const Math::Matrix43* a, const Math::Matrix43* b,
                            Math::Matrix43* ab);

namespace Math {

inline void Mul(Matrix43& o, const Matrix43& a, const Matrix43& b) {
    PSMTXConcat(&a, &b, &o);
}

}  // namespace Math

class xVec3 {
public:
    float x;
    float y;
    float z;
};

class xSphere {
public:
    xVec3 center;
    float r;
};

namespace Globals {
extern float dt;
}  // namespace Globals

// Retail writes an xVec3 by branching to Math::Vector's constructor with the
// xVec3 as `this` (the linker folded the two); NOTES.md records reaching that
// name through a C declaration.
extern "C" void __ct__Q24Math6VectorFfff(void* v, float x, float y, float z);

// For the same argument order as Matrix33::SetRow; the name is ours.
inline void ConstructVector(void* v, float x, float y, float z) {
    __ct__Q24Math6VectorFfff(v, x, y, z);
}

class xMat3x3 {
public:
    xVec3 left;
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

void xMat4x3ToNGMatrix(Math::Matrix43* out, const xMat4x3* in);

// ---------------------------------------------------------------------------
// The engine's side

namespace World {

class ModelPrototypeEntity;

class CollisionMeshBlobEntity {
public:
    hkpPhysicsSystem* GetPhysicsSystem(int index, int flags) const;
};

class xOGModel {
public:
    ModelPrototypeEntity* GetPrototype() const;

    unsigned char _pad0[0xDC];
    ModelPrototypeEntity* protoEnt;
};

}  // namespace World

class xHavokPhysicsObject {
public:
    enum PhysicsObjectType {
        NONE = 0,
        SHAPE = 1,
        MULTI_SHAPE = 2,
        SYSTEM = 3
    };

    enum GraphicsAssociationType {
        INVALID = 0,
        JOINT_NUMBER = 1,
        RENDERABLE_INDEX = 2
    };

    // The values are the listing's (0 is what it tests); the names are ours.
    enum SystemApplicationType {
        APPLY_ALL_BODIES = 0,
        APPLY_ANCHOR_BODIES = 1
    };

    class RelativeJointEntry {
    public:
        Math::Matrix43 mat;
        int rigidBodyIndex;
        int transformSpace;
    };

    class PhysicsJointRelativeTransforms : public hkReferencedObject {
    public:
        hkArray<RelativeJointEntry>* bodyRelativeJointTransforms;
        Math::Matrix43 rootJointInvBindMat;
        int rootRigidBodyJointIndex;
    };

    enum PhysicsRenderableMatchType {
        TELEPORT = 0,
        KEYFRAMED = 1
    };

    static void ConvertHKTransformToGraphicsTransform(const hkTransform& hkTrans,
                                                      Math::Matrix43& trans);
    static void ConvertGraphicsTransformToHKTransform(const Math::Matrix43& graphicsTrans,
                                                      hkTransform& trans);
    static void GetFlattenedTransform(const hkpShape* shape, hkTransform& flatTrans);
    static void SetFlattenedHKTransform(hkpRigidBody* body,
                                        const hkTransform& finalFlatBodyTrans,
                                        PhysicsRenderableMatchType setType);

    void Cleanup();
    bool IsAnchorBody(const hkpRigidBody* body);
    GraphicsAssociationType GetGraphicsAssociationDataFromRigidBody(
        const hkpRigidBody* body, unsigned int& renderableIndex);
    unsigned int GetNumRigidBodies();
    void SetOwner(const void* owner);
    hkpRigidBody* GetRigidBody(unsigned int index);
    void GetTransform(hkTransform& worldTransform);
    void GetTransform(Math::Matrix43& worldTransform);
    void SetTransform(const xMat4x3& matrix);
    void SetTransform(const hkTransform& worldTransform);
    void SetPosition(const hkVector4& pos);
    void SetFriction(float friction);
    void Activate();
    void Deactivate();
    void SetCollisionFilter(unsigned int filterInfo);
    void SystemSetHavokFloatScalar(void (hkpRigidBody::*setRoutine)(float),
                                   float (hkpRigidBody::*getRoutine)() const, float val);
    void SetMass(float mass);
    void ApplyLinearImpulse(const hkVector4& impulse, SystemApplicationType applyType);
    void SetMotionType(hkpMotion::MotionType motionType, SystemApplicationType applyType);
    void SetLinearVelocity(const hkVector4& velocity, SystemApplicationType applyType);
    void SetAngularVelocity(const hkVector4& velocity, SystemApplicationType applyType);

    template <class T>
    void SystemApplyHavok1Param(void (hkpRigidBody::*routine)(T), T param,
                                SystemApplicationType applyType);
    template <class A, class B, class C>
    void SystemApplyHavok3Param(void (hkpRigidBody::*routine)(A, B, C), A a, B b, C c,
                                SystemApplicationType applyType);
    void UpdateKeyframedMotion(float dt, const hkVector4& pos, const hkQuaternion& rot);
    void ZeroKeyframedMotion();
    void InterpolateTransformsNoScale(Math::Matrix43& out, const Math::Matrix43& transformA,
                                      const Math::Matrix43& transformB, float blend);
    void GetBoundingSphere(xSphere* pSphere) const;
    void GetBoundingBoxSize(xVec3* pBoundSize) const;

    static hkPointerMap<const World::CollisionMeshBlobEntity*,
                        PhysicsJointRelativeTransforms*>* jointRelativeCreationMap;

    PhysicsObjectType physicsObjectType;
    World::CollisionMeshBlobEntity* packedPhysicsData;
    hkpPhysicsSystem* physicsSystem;
    hkVector4 creationScale;
};

// ---------------------------------------------------------------------------
// xHavokPhysicsObject

// A float property set on every body; a packed system's bodies scaled by the
// same property of the bodies they were packed as.
void xHavokPhysicsObject::SystemSetHavokFloatScalar(void (hkpRigidBody::*setRoutine)(float),
                                                    float (hkpRigidBody::*getRoutine)() const,
                                                    float val) {
    if (physicsObjectType == SHAPE || physicsObjectType == MULTI_SHAPE) {
        hkpRigidBody* body = GetRigidBody(0);

        if (body != 0) {
            (body->*setRoutine)(val);
        }
    } else if (physicsSystem != 0 && packedPhysicsData != 0) {
        hkpPhysicsSystem* packedPhysicsSystem = packedPhysicsData->GetPhysicsSystem(0, 0);

        if (packedPhysicsSystem != 0) {
            const hkArray<hkpRigidBody*>& packedBodies = packedPhysicsSystem->getRigidBodies();
            const hkArray<hkpRigidBody*>& bodies = physicsSystem->getRigidBodies();

            unsigned int packedBodyIndex = 0;

            for (hkpRigidBody** it = bodies.begin(); it != bodies.end();
                 it++, packedBodyIndex++) {
                // A local: retail keeps it in a register across getRoutine's
                // call, where `(*it)` written twice is read again after it.
                hkpRigidBody* body = *it;

                if (body != 0) {
                    (body->*setRoutine)(val * (packedBodies[packedBodyIndex]->*getRoutine)());
                }
            }
        }
    }
}

// Drops the system, and this object's hold on the joint transforms its
// packed data was created with; the map goes when it empties.
//
// always_inline puts the map's destructor and its allocator in line in the
// delete. The region closes one function later than Cleanup: a pop straight
// after its brace is already in force when Cleanup is generated.
#pragma push
#pragma always_inline on
void xHavokPhysicsObject::Cleanup() {
    if (physicsSystem != 0) {
        physicsSystem->removeReference();
    }

    if (jointRelativeCreationMap != 0) {
        Dummy* it = jointRelativeCreationMap->findKey(packedPhysicsData);

        if (jointRelativeCreationMap->isValid(it)) {
            PhysicsJointRelativeTransforms* physicsCreationTransforms =
                jointRelativeCreationMap->getValue(it);

            if (physicsCreationTransforms->getReferenceCount() == 1) {
                jointRelativeCreationMap->remove(it);
            }
            physicsCreationTransforms->removeReference();

            if (jointRelativeCreationMap->getSize() == 0) {
                jointRelativeCreationMap->clear();
                delete jointRelativeCreationMap;
                jointRelativeCreationMap = 0;
            }
        }
    }

    packedPhysicsData = 0;
    physicsSystem = 0;
    physicsObjectType = NONE;
}


// A body's user data: a tag byte ('J' a joint, 'R' a renderable) over a
// 16-bit index.
xHavokPhysicsObject::GraphicsAssociationType
xHavokPhysicsObject::GetGraphicsAssociationDataFromRigidBody(const hkpRigidBody* body,
                                                             unsigned int& renderableIndex) {
    renderableIndex = 0;

    if (body != 0) {
        unsigned int userData = body->getUserData();

        if (userData != 0) {
            bool renderableIndexData = (userData >> 24) == 'R';

            renderableIndex = userData & 0xFFFF;

            if ((userData >> 24) == 'J') {
                return JOINT_NUMBER;
            } else if (renderableIndexData) {
                return RENDERABLE_INDEX;
            }
        }
    }

    return INVALID;
}
#pragma pop

unsigned int xHavokPhysicsObject::GetNumRigidBodies() {
    return physicsSystem != 0 ? physicsSystem->getRigidBodies().getSize() : 0;
}

void xHavokPhysicsObject::SetOwner(const void* owner) {
    hkpPhysicsSystem* system = physicsSystem;

    if (system != 0) {
        for (hkpRigidBody** it = system->getRigidBodies().begin();
             it != system->getRigidBodies().end(); it++) {
            (*it)->setUserData(hkUlong(owner));
        }
    }
}

hkpRigidBody* xHavokPhysicsObject::GetRigidBody(unsigned int index) {
    if (physicsSystem != 0) {
        if (index < physicsSystem->getRigidBodies().getSize()) {
            return physicsSystem->getRigidBodies()[index];
        }
    }

    return 0;
}

// The system's world transform: a shape's body's own, or for a packed system
// the model root worked back from the root joint's body.
void xHavokPhysicsObject::GetTransform(hkTransform& worldTransform) {
    hkpPhysicsSystem* system = physicsSystem;

    if (system != 0) {
        if (system->getRigidBodies().getSize() > 0) {
            if (physicsObjectType == SYSTEM) {
                PhysicsJointRelativeTransforms* physicsCreationTransforms = 0;

                if (jointRelativeCreationMap != 0 &&
                    jointRelativeCreationMap->get(packedPhysicsData,
                                                  &physicsCreationTransforms) == HK_SUCCESS) {
                    hkArray<RelativeJointEntry>* bodyRelativeJointTransforms =
                        physicsCreationTransforms->bodyRelativeJointTransforms;

                    int rootJointRigidBodyIndex =
                        (*bodyRelativeJointTransforms)
                            [physicsCreationTransforms->rootRigidBodyJointIndex]
                            .rigidBodyIndex;

                    if (rootJointRigidBodyIndex >= 0) {
                        Math::Matrix43 rootBodyMat, rootBodyWorldspaceJointMat;
                        ConvertHKTransformToGraphicsTransform(
                            system->getRigidBodies()[rootJointRigidBodyIndex]
                                ->getTransform(),
                            rootBodyMat);

                        Math::Mul(rootBodyWorldspaceJointMat, rootBodyMat,
                                  (*bodyRelativeJointTransforms)
                                      [physicsCreationTransforms->rootRigidBodyJointIndex]
                                      .mat);

                        Math::Matrix43 joint0WorldMat;

                        if ((*bodyRelativeJointTransforms)[0].transformSpace == 2) {
                            Math::Mul(joint0WorldMat, rootBodyWorldspaceJointMat,
                                      (*bodyRelativeJointTransforms)[0].mat);
                        } else {
                            joint0WorldMat = rootBodyWorldspaceJointMat;
                        }

                        Math::Matrix43 modelRootMat;

                        Math::Mul(modelRootMat, joint0WorldMat,
                                  physicsCreationTransforms->rootJointInvBindMat);
                        ConvertGraphicsTransformToHKTransform(modelRootMat, worldTransform);
                    } else {
                        worldTransform.setIdentity();
                    }
                } else {
                    worldTransform = system->getRigidBodies()[0]->getTransform();
                }
            } else {
                worldTransform = system->getRigidBodies()[0]->getTransform();
            }
        }
    } else {
        worldTransform.setIdentity();
    }
}

void xHavokPhysicsObject::GetTransform(Math::Matrix43& worldTransform) {
    hkpPhysicsSystem* system = physicsSystem;

    if (system != 0) {
        if (system->getRigidBodies().getSize() > 0) {
            if (physicsObjectType == SYSTEM) {
                PhysicsJointRelativeTransforms* physicsCreationTransforms = 0;

                if (jointRelativeCreationMap != 0 &&
                    jointRelativeCreationMap->get(packedPhysicsData,
                                                  &physicsCreationTransforms) == HK_SUCCESS) {
                    hkArray<RelativeJointEntry>* bodyRelativeJointTransforms =
                        physicsCreationTransforms->bodyRelativeJointTransforms;

                    int rootJointRigidBodyIndex =
                        (*bodyRelativeJointTransforms)
                            [physicsCreationTransforms->rootRigidBodyJointIndex]
                            .rigidBodyIndex;

                    if (rootJointRigidBodyIndex >= 0) {
                        Math::Matrix43 rootBodyMat, rootBodyWorldspaceJointMat;
                        ConvertHKTransformToGraphicsTransform(
                            system->getRigidBodies()[rootJointRigidBodyIndex]
                                ->getTransform(),
                            rootBodyMat);

                        Math::Mul(rootBodyWorldspaceJointMat, rootBodyMat,
                                  (*bodyRelativeJointTransforms)
                                      [physicsCreationTransforms->rootRigidBodyJointIndex]
                                      .mat);

                        Math::Matrix43 joint0WorldMat;

                        if ((*bodyRelativeJointTransforms)[0].transformSpace == 2) {
                            Math::Mul(joint0WorldMat, rootBodyWorldspaceJointMat,
                                      (*bodyRelativeJointTransforms)[0].mat);
                        } else {
                            joint0WorldMat = rootBodyWorldspaceJointMat;
                        }

                        Math::Mul(worldTransform, joint0WorldMat,
                                  physicsCreationTransforms->rootJointInvBindMat);
                    } else {
                        worldTransform = Math::_matIdentity;
                    }
                } else {
                    ConvertHKTransformToGraphicsTransform(
                        system->getRigidBodies()[0]->getTransform(), worldTransform);
                }
            } else {
                ConvertHKTransformToGraphicsTransform(
                    system->getRigidBodies()[0]->getTransform(), worldTransform);
            }
        }
    } else {
        worldTransform = Math::_matIdentity;
    }
}

void xHavokPhysicsObject::SetTransform(const xMat4x3& matrix) {
    Math::Matrix43 mathmat;
    xMat4x3ToNGMatrix(&mathmat, &matrix);

    hkTransform transform;
    ConvertGraphicsTransformToHKTransform(mathmat, transform);

    SetTransform(transform);
}

// A single shape's body takes the transform; otherwise every body keeps its
// place relative to the system's current transform.
void xHavokPhysicsObject::SetTransform(const hkTransform& worldTransform) {
    if (physicsSystem != 0) {
        const hkArray<hkpRigidBody*>& bodies = physicsSystem->getRigidBodies();

        if (physicsObjectType == SHAPE && bodies.getSize() > 0) {
            bodies[0]->setTransform(worldTransform);
        } else {
            hkTransform systemWorldTransform;

            GetTransform(systemWorldTransform);
            for (hkpRigidBody** it = bodies.begin(); it != bodies.end(); it++) {
                hkpRigidBody* body = *it;
                hkTransform newBodyTransform;

                newBodyTransform.setMulInverseMul(systemWorldTransform, body->getTransform());
                newBodyTransform.setMulEq(worldTransform);

                body->setTransform(newBodyTransform);
            }
        }
    }
}

void xHavokPhysicsObject::SetPosition(const hkVector4& pos) {
    hkTransform systemTransform;

    GetTransform(systemTransform);
    systemTransform.setTranslation(pos);
    SetTransform(systemTransform);
}

// A packed system's bodies take the mass in the proportion they were packed
// with; a packed body of no mass counts as one.
//
// NEAR MISS, 99 of 109 words: the four-literal wall (NOTES.md). It loads
// 0.0f, -1e-5f, 1e-5f and 1.0f; retail spells a `lis` for each, ours forms
// one `addis` base for all four, and everything after it moves.
void xHavokPhysicsObject::SetMass(float mass) {
    if (physicsObjectType == SHAPE || physicsObjectType == MULTI_SHAPE) {
        hkpRigidBody* body = GetRigidBody(0);

        if (body != 0) {
            body->setMass(mass);
        }
    } else if (physicsSystem != 0 && packedPhysicsData != 0) {
        hkpPhysicsSystem* packedPhysicsSystem = packedPhysicsData->GetPhysicsSystem(0, 0);

        if (packedPhysicsSystem != 0) {
            const hkArray<hkpRigidBody*>& bodies = physicsSystem->getRigidBodies();

            float totalSystemMass = 0.0f;

            for (hkpRigidBody** it = packedPhysicsSystem->getRigidBodies().begin();
                 it != packedPhysicsSystem->getRigidBodies().end(); it++) {
                if (*it != 0) {
                    totalSystemMass += (*it)->getMass();
                }

                // Retail reads it again for every body, null or not, and
                // discards it.
                (*it)->getMass();
            }

            float totalSystemMassScalar;

            if (totalSystemMass != 0.0f && mass > 0.0f) {
                totalSystemMassScalar = mass / totalSystemMass;
            } else {
                return;
            }

            unsigned int packedBodyIndex = 0;

            for (hkpRigidBody** it = bodies.begin(); it != bodies.end();
                 it++, packedBodyIndex++) {
                if (*it != 0) {
                    float packedMass =
                        packedPhysicsSystem->getRigidBodies()[packedBodyIndex]->getMass();

                    if (packedMass >= -1e-5f && packedMass <= 1e-5f) {
                        packedMass = 1.0f;
                    }

                    (*it)->setMass(packedMass * totalSystemMassScalar);
                }
            }
        }
    }
}

// A packed system scales each body's friction by the one it was packed with.
void xHavokPhysicsObject::SetFriction(float friction) {
    if (physicsObjectType == SHAPE || physicsObjectType == MULTI_SHAPE) {
        hkpRigidBody* body = GetRigidBody(0);

        if (body != 0) {
            body->setFriction(friction, -1.0f);
        }
    } else if (physicsSystem != 0 && packedPhysicsData != 0) {
        hkpPhysicsSystem* packedPhysicsSystem = packedPhysicsData->GetPhysicsSystem(0, 0);

        if (packedPhysicsSystem != 0) {
            const hkArray<hkpRigidBody*>& packedBodies = packedPhysicsSystem->getRigidBodies();
            const hkArray<hkpRigidBody*>& bodies = physicsSystem->getRigidBodies();

            unsigned int packedBodyIndex = 0;

            for (hkpRigidBody** it = bodies.begin(); it != bodies.end();
                 it++, packedBodyIndex++) {
                if (*it != 0) {
                    (*it)->setFriction(
                        friction * packedBodies[packedBodyIndex]->getMaterial().getFriction(),
                        -1.0f);
                }
            }
        }
    }
}

void xHavokPhysicsObject::Activate() {
    const hkArray<hkpRigidBody*>& bodies = physicsSystem->getRigidBodies();

    for (hkpRigidBody** it = bodies.begin(); it != bodies.end(); it++) {
        hkpRigidBody* body = *it;

        if (!(body->isFixed() || body->isActive())) {
            body->activate();
        }
    }
}

void xHavokPhysicsObject::Deactivate() {
    const hkArray<hkpRigidBody*>& bodies = physicsSystem->getRigidBodies();

    for (hkpRigidBody** it = bodies.begin(); it != bodies.end(); it++) {
        hkpRigidBody* body = *it;

        if (!body->isFixed()) {
            body->deactivate();
        }
    }
}

// The layer changes and each body keeps its group; a packed system's bodies
// take their subsystem ids from the bodies they were packed as, none in the
// no-collision layer.
void xHavokPhysicsObject::SetCollisionFilter(unsigned int filterInfo) {
    unsigned int layer = hkpGroupFilter::getLayerFromFilterInfo(filterInfo);

    if (physicsSystem != 0) {
        const hkArray<hkpRigidBody*>& bodies = physicsSystem->getRigidBodies();

        if (physicsObjectType != SYSTEM) {
            for (hkpRigidBody** it = bodies.begin(); it != bodies.end(); it++) {
                hkpRigidBody* body = *it;
                unsigned int info = body->getCollisionFilterInfo();

                unsigned int newInfo = hkpGroupFilter::calcFilterInfo(
                    layer, hkpGroupFilter::getSystemGroupFromFilterInfo(info),
                    hkpGroupFilter::getSubSystemIdFromFilterInfo(info),
                    hkpGroupFilter::getSubSystemDontCollideWithFromFilterInfo(info));
                if (body != 0) {
                    xHavok_SetCollisionFilterInfo(body, (E_HAVOK_COLLIDE_FILTER_LAYER)newInfo);
                }
            }
        } else {
            hkpPhysicsSystem* packedPhysicsSystem = packedPhysicsData->GetPhysicsSystem(0, 0);
            const hkArray<hkpRigidBody*>& packedBodies = packedPhysicsSystem->getRigidBodies();

            unsigned int bodyIndex = 0;
            for (hkpRigidBody** it = bodies.begin(); it != bodies.end(); it++, bodyIndex++) {
                hkpRigidBody* packedBody = packedBodies[bodyIndex];
                hkpRigidBody* body = *it;

                int systemGroup = body->getCollisionFilterInfo() >> 16;

                int oldSubsystemID = packedBody->getCollisionFilterInfo() >> 16;
                int oldSubSystemNoCollideID = packedBody->getCollisionFilterInfo() & 0xFFFF;

                if (layer == eNoCollisionLayer) {
                    oldSubsystemID = 0;
                    oldSubSystemNoCollideID = 0;
                }

                unsigned int newInfo = hkpGroupFilter::calcFilterInfo(
                    layer, systemGroup, oldSubsystemID, oldSubSystemNoCollideID);
                if (body != 0) {
                    xHavok_SetCollisionFilterInfo(body, (E_HAVOK_COLLIDE_FILTER_LAYER)newInfo);
                }
            }
        }
    }
}

void xHavokPhysicsObject::ApplyLinearImpulse(const hkVector4& impulse,
                                             SystemApplicationType applyType) {
    SystemApplyHavok1Param<const hkVector4&>(&hkpRigidBody::applyLinearImpulse, impulse,
                                             applyType);
}

// SystemApplyHavok1Param's shape with three arguments, which retail has in
// line in SetMotionType; the name is ours.
template <class A, class B, class C>
inline void xHavokPhysicsObject::SystemApplyHavok3Param(void (hkpRigidBody::*routine)(A, B, C),
                                                        A a, B b, C c,
                                                        SystemApplicationType applyType) {
    if (physicsSystem != 0) {
        const hkArray<hkpRigidBody*>& bodies = physicsSystem->getRigidBodies();

        if (applyType == APPLY_ALL_BODIES || physicsObjectType != SYSTEM) {
            for (hkpRigidBody** it = bodies.begin(); it != bodies.end(); it++) {
                hkpRigidBody* body = *it;

                if (body != 0) {
                    (body->*routine)(a, b, c);
                }
            }
        } else {
            hkpPhysicsSystem* packedPhysicsSystem = packedPhysicsData->GetPhysicsSystem(0, 0);
            unsigned int index = 0;

            for (hkpRigidBody** it = bodies.begin(); it != bodies.end(); it++, index++) {
                hkpRigidBody* body = *it;
                hkpRigidBody* packedBody = packedPhysicsSystem->getRigidBodies()[index];

                if (packedBody != 0 && body != 0) {
                    if (IsAnchorBody(packedBody)) {
                        (body->*routine)(a, b, c);
                    }
                }
            }
        }
    }
}

// always_inline takes SystemApplyHavok3Param in line, as retail has it;
// under -inline auto it is emitted on its own. The region closes one function
// later, as Cleanup's does, and IsAnchorBody is defined below it.
#pragma push
#pragma always_inline on
void xHavokPhysicsObject::SetMotionType(hkpMotion::MotionType motionType,
                                        SystemApplicationType applyType) {
    SystemApplyHavok3Param<hkpMotion::MotionType, hkpEntityActivation,
                           hkpUpdateCollisionFilterOnEntityMode>(
        &hkpRigidBody::setMotionType, motionType, HK_ENTITY_ACTIVATION_DO_ACTIVATE,
        HK_UPDATE_FILTER_ON_ENTITY_FULL_CHECK, applyType);
}

void xHavokPhysicsObject::SetLinearVelocity(const hkVector4& velocity,
                                            SystemApplicationType applyType) {
    SystemApplyHavok1Param<const hkVector4&>(&hkpRigidBody::setLinearVelocity, velocity,
                                             applyType);
}
#pragma pop

void xHavokPhysicsObject::SetAngularVelocity(const hkVector4& velocity,
                                             SystemApplicationType applyType) {
    SystemApplyHavok1Param<const hkVector4&>(&hkpRigidBody::setAngularVelocity, velocity,
                                             applyType);
}

// Defined below SetMotionType: always_inline is on around that one, and would
// take this in line where retail calls it.
bool xHavokPhysicsObject::IsAnchorBody(const hkpRigidBody* body) {
    unsigned int userData = body->getUserData();

    if (userData != 0) {
        return (userData & 0x10000) != 0;
    }

    return false;
}

// Each keyframed body is driven to where the new system transform puts it,
// keeping its offset from the system's current transform.
void xHavokPhysicsObject::UpdateKeyframedMotion(float dt, const hkVector4& pos,
                                                const hkQuaternion& rot) {
    if (physicsSystem != 0) {
        hkTransform systemWorldTransform;

        GetTransform(systemWorldTransform);

        hkTransform newSystemTransform(rot, pos);
        const hkArray<hkpRigidBody*>& bodies = physicsSystem->getRigidBodies();

        for (hkpRigidBody** it = bodies.begin(); it != bodies.end(); it++) {
            hkpRigidBody* body = *it;

            if (body->getMotionType() == hkpMotion::MOTION_KEYFRAMED) {
                hkTransform newBodyTransform;

                {
                    hkTransform differenceTransform;

                    differenceTransform.setMulInverseMul(systemWorldTransform,
                                                         body->getTransform());
                    newBodyTransform.setMul(newSystemTransform, differenceTransform);
                }

                hkQuaternion newBodyRotation(newBodyTransform.getRotation());

                xHavok_UpdateRigidBodyMotion(body, newBodyTransform.getTranslation(),
                                             newBodyRotation, dt);
            }
        }
    }
}

// A keyframed body in the world stops where it is.
void xHavokPhysicsObject::ZeroKeyframedMotion() {
    if (physicsSystem != 0) {
        const hkArray<hkpRigidBody*>& bodies = physicsSystem->getRigidBodies();

        for (hkpRigidBody** it = bodies.begin(); it != bodies.end(); it++) {
            hkpRigidBody* body = *it;

            if (body->getMotionType() == hkpMotion::MOTION_KEYFRAMED &&
                body->getWorld() == xHavok_GetWorld()) {
                xHavok_GetWorld()->markForWrite();
                body->getRigidMotion()->setAngularVelocity(hkVector4Zero);
                body->getRigidMotion()->setLinearVelocity(hkVector4Zero);
                xHavok_GetWorld()->unmarkForWrite();
            }
        }
    }
}

// Havok's rotation is three columns; the engine's matrix takes them as rows.
void xHavokPhysicsObject::ConvertHKTransformToGraphicsTransform(const hkTransform& hkTrans,
                                                                Math::Matrix43& trans) {
    trans.SetRow(0, hkTrans.m_rotation.m_col0.x, hkTrans.m_rotation.m_col0.y,
                 hkTrans.m_rotation.m_col0.z);
    trans.SetRow(1, hkTrans.m_rotation.m_col1.x, hkTrans.m_rotation.m_col1.y,
                 hkTrans.m_rotation.m_col1.z);
    trans.SetRow(2, hkTrans.m_rotation.m_col2.x, hkTrans.m_rotation.m_col2.y,
                 hkTrans.m_rotation.m_col2.z);
    trans.SetRow(3, hkTrans.m_translation.x, hkTrans.m_translation.y, hkTrans.m_translation.z);
}

// hkVector4's (x, y, z, w = 0) constructor: the linker folded it onto
// Math::Vector4::Assign, which is the name retail branches to.
class hkVector4Init : public hkVector4 {
public:
    hkVector4Init(float x, float y, float z) {
        ((Math::Vector4*)this)->Assign(x, y, z, 0.0f);
    }
};

// Orthonormalized first; the engine's rows become Havok's columns.
void xHavokPhysicsObject::ConvertGraphicsTransformToHKTransform(
    const Math::Matrix43& graphicsTrans, hkTransform& trans) {
    Math::Matrix43 normalizeGraphicsTrans(graphicsTrans);

    Math::Orthonormalize(normalizeGraphicsTrans, normalizeGraphicsTrans);

    Math::Vector posRef = normalizeGraphicsTrans.GetRowInternal(3);
    Math::Vector forwardRef = normalizeGraphicsTrans.GetRowInternal(2);
    Math::Vector leftRef = normalizeGraphicsTrans.GetRowInternal(0);
    Math::Vector upRef = normalizeGraphicsTrans.GetRowInternal(1);

    trans.getRotation().setCols(
        hkVector4Init(leftRef.data.x, leftRef.data.y, leftRef.data.z),
        hkVector4Init(upRef.data.x, upRef.data.y, upRef.data.z),
        hkVector4Init(forwardRef.data.x, forwardRef.data.y, forwardRef.data.z));

    trans.setTranslation(hkVector4Init(posRef.data.x, posRef.data.y, posRef.data.z));
}

// NEAR MISS, 21 of 83 words: only the stack slots of the two GetQuaternion
// temporaries differ -- ours sit below every named local (sp+8 and sp+24),
// retail's straight after interpolatedQuat (sp+72 and sp+56) -- and the
// three vectors declared after them move by that much. Tried: the
// quaternions as named locals (82 of 83); interpolatedQuat initialized from
// a value-returning Slerp, which mwcc builds at sp+8 and copies (71 of 83);
// always_inline over Math::Lerp instead of the blend written out (29 of 83).
void xHavokPhysicsObject::InterpolateTransformsNoScale(Math::Matrix43& out,
                                                       const Math::Matrix43& transformA,
                                                       const Math::Matrix43& transformB,
                                                       float blend) {
    Math::Matrix43 normTransA(transformA), normTransB(transformB);

    Math::Normalize(normTransA, normTransA);
    Math::Normalize(normTransB, normTransB);

    Math::Quaternion interpolatedQuat;

    Math::Slerp(interpolatedQuat,
                normTransA.GetQuaternion(),
                normTransB.GetQuaternion(),
                blend);
    out.MakeQuaternion(interpolatedQuat);

    Math::Vector posA = transformA.GetRowInternal(3);
    Math::Vector posB = transformB.GetRowInternal(3);
    Math::Vector interpolatedPos;

    interpolatedPos.data.x = posA.data.x * (1.0f - blend) + posB.data.x * blend;
    interpolatedPos.data.y = posA.data.y * (1.0f - blend) + posB.data.y * blend;
    interpolatedPos.data.z = posA.data.z * (1.0f - blend) + posB.data.z * blend;
    interpolatedPos.data.w = posA.data.w * (1.0f - blend) + posB.data.w * blend;
    out.SetPos(interpolatedPos);
}

// The first body's box.
//
// NEAR MISS, 34 of 46 words, and GetBoundingBoxSize below 23 of 35 for the
// same reason: retail reads bodies' m_data once and bodies[0] TWICE -- once
// for the shape, once for the transform -- where ours reads the element once,
// so everything after it is a word early. Tried: *begin() for either use
// (m_data is then read twice), m_data[0] for either (folded into one read
// again), and an inline taking the body for the shape or for the transform
// (folded again).
void xHavokPhysicsObject::GetBoundingSphere(xSphere* pSphere) const {
    hkAabb aabb;

    const hkArray<hkpRigidBody*>& bodies = physicsSystem->getRigidBodies();

    bodies[0]->getCollidable()->getShape()->getAabb(bodies[0]->getTransform(), 0.0f, aabb);

    hkVector4 halfInternalDiagonal, center;

    halfInternalDiagonal.setSub4(aabb.m_max, aabb.m_min);
    halfInternalDiagonal.mul4(0.5f);
    Math::Add((Math::Vector&)center, (const Math::Vector&)halfInternalDiagonal,
              (const Math::Vector&)aabb.m_min);

    ConstructVector(&pSphere->center, center.x, center.y, center.z);
    pSphere->r = halfInternalDiagonal.length3();
}

// NEAR MISS, 23 of 35 words: GetBoundingSphere's.
void xHavokPhysicsObject::GetBoundingBoxSize(xVec3* pBoundSize) const {
    hkAabb aabb;

    const hkArray<hkpRigidBody*>& bodies = physicsSystem->getRigidBodies();

    bodies[0]->getCollidable()->getShape()->getAabb(bodies[0]->getTransform(), 0.0f, aabb);

    hkVector4 dimensions;

    dimensions.setSub4(aabb.m_max, aabb.m_min);

    ConstructVector(pBoundSize, dimensions.x, dimensions.y, dimensions.z);
}

// A shape under convex translate and transform wrappers: the product of
// their transforms, outermost first.
void xHavokPhysicsObject::GetFlattenedTransform(const hkpShape* shape, hkTransform& flatTrans) {
    flatTrans.setIdentity();

    hkpShapeType shapeType = shape->getType();

    while (shapeType == HK_SHAPE_CONVEX_TRANSLATE || shapeType == HK_SHAPE_CONVEX_TRANSFORM) {
        hkTransform childTrans;

        if (shapeType == HK_SHAPE_CONVEX_TRANSLATE) {
            const hkpConvexTranslateShape* translateShape =
                (const hkpConvexTranslateShape*)shape;

            childTrans.setIdentity();
            childTrans.setTranslation(translateShape->getTranslation());
            shape = translateShape->getChildShape();
            shapeType = shape->getType();
        } else if (shapeType == HK_SHAPE_CONVEX_TRANSFORM) {
            const hkpConvexTransformShape* transformShape =
                (const hkpConvexTransformShape*)shape;
            float hkMat[16];

            transformShape->getTransform().get4x4ColumnMajor(hkMat);
            childTrans.set4x4ColumnMajor(hkMat);
            shape = transformShape->getChildShape();
            shapeType = shape->getType();
        }

        flatTrans.setMulEq(childTrans);
    }
}

// The shape's own flattened transform is taken back off before the body is
// placed; a keyframed body asked to match is driven there over one frame.
void xHavokPhysicsObject::SetFlattenedHKTransform(hkpRigidBody* body,
                                                  const hkTransform& finalFlatBodyTrans,
                                                  PhysicsRenderableMatchType setType) {
    hkTransform flattenedShapeTransform, finalBodyTransform;

    GetFlattenedTransform(body->getCollidable()->getShape(), flattenedShapeTransform);

    finalBodyTransform.setMulMulInverse(finalFlatBodyTrans, flattenedShapeTransform);

    if (setType == KEYFRAMED && body->getMotionType() == hkpMotion::MOTION_KEYFRAMED) {
        hkQuaternion rotation(finalBodyTransform.getRotation());

        xHavok_UpdateRigidBodyMotion(body, finalBodyTransform.getTranslation(), rotation,
                                     Globals::dt);
    } else {
        body->setTransform(finalBodyTransform);
    }

    body->setTransform(finalBodyTransform);
}

// ---------------------------------------------------------------------------
// xHavokInterface

void xHavok_SetFrameFromCharacterProxy(xMat4x3* mat, hkpCharacterProxy* pCharacterProxy) {
    const hkVector4& proxy_pos = pCharacterProxy->getPosition();

    ConstructVector(&mat->pos, proxy_pos.x, proxy_pos.y, proxy_pos.z);
}

void xHavok_SetFrameFromCharacterProxy(xMat4x3* mat, xVec3* vel,
                                       hkpCharacterRigidBody* pCharacterProxy) {
    const hkVector4& proxy_pos = pCharacterProxy->getPosition();

    ConstructVector(&mat->pos, proxy_pos.x, proxy_pos.y, proxy_pos.z);

    const hkVector4& proxy_vel = pCharacterProxy->getLinearVelocity();

    ConstructVector(vel, proxy_vel.x, proxy_vel.y, proxy_vel.z);
}

// ---------------------------------------------------------------------------
// The weak copies, below every caller so each is called out of line

template <class K, class V>
inline hkResult hkPointerMap<K, V>::get(K key, V* out) const {
    hkUlong tmp;

    if (m_map.get(hkUlong(key), &tmp) == HK_SUCCESS) {
        *out = V(tmp);
        return HK_SUCCESS;
    }

    return HK_FAILURE;
}

template <class K, class V>
inline hkBool hkPointerMap<K, V>::isValid(Dummy* it) const {
    return (int)hkUlong(it) <= m_map.m_hashMod;
}

template <class K, class V>
inline V hkPointerMap<K, V>::getValue(Dummy* it) const {
    return V(m_map.m_elem[int(hkUlong(it))].val);
}

// A body member applied to every body -- or, for a packed system asked for its
// anchors, to the bodies whose packed counterparts are anchors. Defined below
// its three callers, so they call it out of line as retail does.
template <class T>
inline void xHavokPhysicsObject::SystemApplyHavok1Param(void (hkpRigidBody::*routine)(T),
                                                        T param,
                                                        SystemApplicationType applyType) {
    if (physicsSystem != 0) {
        const hkArray<hkpRigidBody*>& bodies = physicsSystem->getRigidBodies();

        if (applyType == APPLY_ALL_BODIES || physicsObjectType != SYSTEM) {
            for (hkpRigidBody** it = bodies.begin(); it != bodies.end(); it++) {
                hkpRigidBody* body = *it;

                if (body != 0) {
                    (body->*routine)(param);
                }
            }
        } else {
            hkpPhysicsSystem* packedPhysicsSystem = packedPhysicsData->GetPhysicsSystem(0, 0);
            unsigned int index = 0;

            for (hkpRigidBody** it = bodies.begin(); it != bodies.end(); it++, index++) {
                hkpRigidBody* body = *it;
                hkpRigidBody* packedBody = packedPhysicsSystem->getRigidBodies()[index];

                if (packedBody != 0 && body != 0) {
                    if (IsAnchorBody(packedBody)) {
                        (body->*routine)(param);
                    }
                }
            }
        }
    }
}

// On through the last weak copy: with it deallocateChunkConstSize has
// FreeList::put in line, and hkTransform::setIdentity hkMatrix3::setIdentity,
// as retail has them. Wrapping put's or hkMatrix3::setIdentity's own
// definition instead changes nothing. Inlines defined further down are still
// called out of line. It is popped before the generated accessors so that
// the file ENDS with it off: SystemApplyHavok1Param is generated there, and
// with it on takes IsAnchorBody in line where retail calls it.
#pragma push
#pragma always_inline on

inline void hkThreadMemory::deallocateChunkConstSize(void* p, int nbytes,
                                                     HK_MEMORY_CLASS cl) {
    int row = constSizeToRow(nbytes);

    if (m_free_list[row].m_numElem >= m_maxNumElemsOnFreeList) {
        onRowFull(row, p, cl);
    } else {
        m_free_list[row].put(p);
    }
}

// Inlined into hkTransform::setIdentity, so defined above it.
inline void hkMatrix3::setIdentity() {
    hkVector4 zero;
    zero.setZero4();
    getColumn(0) = zero;
    getColumn(1) = zero;
    getColumn(2) = zero;

    float one = 1.0f;
    getColumn(0)(0) = one;
    getColumn(1)(1) = one;
    getColumn(2)(2) = one;
}

inline void hkTransform::setIdentity() {
    m_rotation.setIdentity();
    m_translation.setZero4();
}

inline void hkpRigidBody::applyLinearImpulse(const hkVector4& imp) {
    activate();
    getRigidMotion()->applyLinearImpulse(imp);
}

inline void hkpRigidBody::setLinearVelocity(const hkVector4& newVel) {
    activate();
    getRigidMotion()->setLinearVelocity(newVel);
}

inline void hkpRigidBody::setAngularVelocity(const hkVector4& newVel) {
    activate();
    getRigidMotion()->setAngularVelocity(newVel);
}

inline hkTransform::hkTransform(const hkQuaternion& q, const hkVector4& t) {
    m_translation = t;
    m_rotation.set(q);
}

inline void hkVector4::setSub4(const hkVector4& a, const hkVector4& b) {
    x = a.x - b.x;
    y = a.y - b.y;
    z = a.z - b.z;
    w = a.w - b.w;
}

inline void hkVector4::mul4(float s) {
    x *= s;
    y *= s;
    z *= s;
    w *= s;
}

inline hkTransform& hkTransform::operator=(const hkTransform& t) {
    m_rotation.m_col0 = t.m_rotation.m_col0;
    m_rotation.m_col1 = t.m_rotation.m_col1;
    m_rotation.m_col2 = t.m_rotation.m_col2;
    m_translation = t.m_translation;
    return *this;
}

inline void hkVector4::setZero4() {
    x = y = z = w = 0.0f;
}

inline void hkVector4::operator=(const hkVector4& v) {
    x = v.x;
    y = v.y;
    z = v.z;
    w = v.w;
}

inline Math::Matrix43& Math::Matrix43::operator=(const Matrix43& a) {
    for (int i = 0; i < 3; i++) {
        v[i] = a.v[i];
    }

    return *this;
}

inline void Math::Matrix33::SetRowInternal(int row, float x, float y, float z) {
    v[0][row] = x;
    v[1][row] = y;
    v[2][row] = z;
}

inline Math::Vector Math::Matrix33::GetRowInternal(int row) const {
    // Declared z, y, x: the order retail loads them in.
    float z = ((float*)&v[2].data)[row];
    float y = ((float*)&v[1].data)[row];
    float x = ((float*)&v[0].data)[row];

    return Vector(x, y, z);
}

inline void Math::Matrix43::SetPos(const Vector& pos) {
    // Declared x, y, z: retail loads them in that order into f0, f1, f2.
    float x = pos.data.x;
    float y = pos.data.y;
    float z = pos.data.z;

    SetRowInternal(3, x, y, z);
}

inline void Math::Add(Vector& o, const Vector& a, const Vector& b) {
    o[0] = a[0] + b[0];
    o[1] = a[1] + b[1];
    o[2] = a[2] + b[2];
    o[3] = a[3] + b[3];
}

inline Math::Vector::Vector(float x, float y, float z) {
    data.x = x;
    data.y = y;
    data.z = z;
}

inline Math::Matrix33::Matrix33() {}

inline void xMat4x3ToNGMatrix(Math::Matrix43* out, const xMat4x3* in) {
    out->Assign(in->left.x, in->left.y, in->left.z,
                in->up.x, in->up.y, in->up.z,
                in->at.x, in->at.y, in->at.z,
                in->pos.x, in->pos.y, in->pos.z);
}

inline void Math::Matrix43::Assign(float x0, float y0, float z0, float x1, float y1,
                                   float z1, float x2, float y2, float z2, float x3,
                                   float y3, float z3) {
    v[0].Assign(x0, x1, x2, x3);
    v[1].Assign(y0, y1, y2, y3);
    v[2].Assign(z0, z1, z2, z3);
}

inline void Math::Vector4::Assign(float x, float y, float z, float w) {
    data.x = x;
    data.y = y;
    data.z = z;
    data.w = w;
}

#pragma pop

// ---------------------------------------------------------------------------
// The generated accessors

float hkpRigidBody::getRestitution() const { return m_material.m_restitution; }
float hkpRigidBody::getLinearDamping() const { return m_motion.m_linearDamping; }
float hkpRigidBody::getAngularDamping() const { return m_motion.m_angularDamping; }
void hkpRigidBody::setLinearDamping(float value) { m_motion.m_linearDamping = value; }
void hkpRigidBody::setAngularDamping(float value) { m_motion.m_angularDamping = value; }
World::ModelPrototypeEntity* World::xOGModel::GetPrototype() const { return protoEnt; }
