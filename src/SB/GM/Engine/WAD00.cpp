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
// `#pragma always_inline on` holds in seven regions, each measured (NOTES.md,
// "WAD00"): AddToHavokSimWorld with its two helpers, above every member
// definition; ScaleConstraintBodyAttachSpace; Cleanup and the function after
// it; SetMotionType and the function after it;
// ConvertGraphicsTransformToHKTransform and the function after it;
// xHavok_SetNPCFromCharacterProxyMotion and the function after it; and the
// weak copies between hkLocalArray's two explicit instantiations. On for the
// whole file it also inlines ordinary members into their callers.
// `#pragma dont_inline on` holds over the weak copies after the last of those
// regions, which retail calls rather than inlines, and `#pragma
// optimize_for_size off` over xMat3x3Normalize among them.

class Dummy;
class hkpRigidBody;
class hkpRigidBodyCinfo;
class hkpPhysicsSystem;

typedef unsigned long hkUlong;

enum hkResult {
    HK_SUCCESS = 0,
    HK_FAILURE = 1
};

enum HK_MEMORY_CLASS {
    HK_MEMORY_CLASS_BASE_CLASS = 22,
    HK_MEMORY_CLASS_ARRAY = 24,
    HK_MEMORY_CLASS_MAP = 29,
    HK_MEMORY_CLASS_AGENT = 32,
    HK_MEMORY_CLASS_CONSTRAINT = 45,
    HK_MEMORY_CLASS_ENTITY = 46,
    HK_MEMORY_CLASS_WORLD = 48
};

enum E_HAVOK_COLLIDE_FILTER_LAYER {
    eNoCollisionLayer = 31,
    eFixedLayer = 1,
    eFixedNoCamLayer = 2,
    eKeyFrameObjLayer = 3,
    eDynamicSimpleObjLayer = 4
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
// Havok's FPU comparison: one bit per component.
class hkVector4Comparison {
public:
    enum Mask {
        MASK_NONE = 0,
        MASK_W = 1,
        MASK_Z = 2,
        MASK_Y = 4,
        MASK_X = 8,
        MASK_XYZ = 14
    };

    bool allAreSet(Mask m) const { return (m_mask & m) == m; }

    int m_mask;
};

class hkVector4 {
public:
    // Havok's. Through the copy constructor, a class holding a vector is
    // copied with operator= for it, as HavokRayCastStopFilterCollide's hit is.
    hkVector4() {}
    hkVector4(const hkVector4& v) { *this = v; }

    void operator=(const hkVector4& v);
    void setZero4();
    void setSub4(const hkVector4& a, const hkVector4& b);
    void mul4(float s);
    void setAll3(float v);
    float length3() const;
    void sub4(const hkVector4& a);
    void mul4(const hkVector4& a);
    void normalize3();
    bool equals3(const hkVector4& v, float epsilon) const;
    float dot3(const hkVector4& v) const;
    void normalize4();

    void setAbs4(const hkVector4& v) {
        x = (float)__fabs(v.x);
        y = (float)__fabs(v.y);
        z = (float)__fabs(v.z);
        w = (float)__fabs(v.w);
    }

    hkVector4Comparison compareLessThanEqual4(const hkVector4& a) const {
        hkVector4Comparison ret;
        ret.m_mask = ((x <= a.x) ? hkVector4Comparison::MASK_X : hkVector4Comparison::MASK_NONE) |
                     ((y <= a.y) ? hkVector4Comparison::MASK_Y : hkVector4Comparison::MASK_NONE) |
                     ((z <= a.z) ? hkVector4Comparison::MASK_Z : hkVector4Comparison::MASK_NONE) |
                     ((w <= a.w) ? hkVector4Comparison::MASK_W : hkVector4Comparison::MASK_NONE);
        return ret;
    }

    void setInterpolate4(const hkVector4& a, const hkVector4& b, float t) {
        float s = 1.0f - t;
        x = s * a.x + t * b.x;
        y = s * a.y + t * b.y;
        z = s * a.z + t * b.z;
        w = s * a.w + t * b.w;
    }

    float& operator()(int i) { return (&x)[i]; }
    const float& operator()(int i) const { return (&x)[i]; }

    float x __attribute__((aligned(16)));
    float y;
    float z;
    float w;
};

extern const hkVector4 hkVector4Zero;

class hkMath {
public:
    static float sqrtInverse(float r);
};

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
    hkQuaternion() {}
    hkQuaternion(const hkRotation& r) { set(r); }

    void normalize();

    void set(const hkRotation& r);

    hkVector4 m_vec;
};

class hkTransform {
public:
    hkTransform() {}
    hkTransform(const hkQuaternion& q, const hkVector4& t);
    hkTransform(const hkTransform& t);

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

// Here, above operator='s definition at the tail: hkVector4's constructors
// are in line and its operator= is called, as retail has them.
hkTransform::hkTransform(const hkTransform& t) {
    hkVector4 col0(t.m_rotation.m_col0);
    hkVector4 col1(t.m_rotation.m_col1);
    hkVector4 col2(t.m_rotation.m_col2);
    hkVector4 translation(t.m_translation);

    m_rotation.m_col0 = col0;
    m_rotation.m_col1 = col1;
    m_rotation.m_col2 = col2;
    m_translation = translation;
}

class hkAabb {
public:
    hkVector4 m_min;
    hkVector4 m_max;
};

class hkMemory;

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

        // The head tested is the one returned; the count's store comes
        // between, so the head is read again for its next.
        void* get() {
            if (m_head) {
                FreeElem* n = m_head;
                m_numElem--;
                m_head = m_head->m_next;
                return n;
            }

            return 0;
        }

        FreeElem* m_head;
        int m_numElem;
    };

    class Stack {
    public:
        char* m_current;
        Stack* m_prev;
        char* m_base;
        char* m_end;
    };

    virtual void* alignedAllocate(int alignment, int nbytes, HK_MEMORY_CLASS cl);
    virtual void alignedDeallocate(void* p);
    virtual void setStackArea(void* buf, int nbytes);
    virtual void releaseCachedMemory();
    virtual ~hkThreadMemory();
    virtual void* onStackOverflow(int nbytes);
    virtual void onStackUnderflow(void* p);

    // Sixteen bytes past the request, rounded down to sixteen: retail's
    // addi 16 and rlwinm 0,0,27.
    void* allocateStack(int nbytesin) {
        int nbytes = (nbytesin + 16) & ~15;
        char* current = m_stack.m_current;
        char* next = current + nbytes;

        if (next <= m_stack.m_end) {
            m_stack.m_current = next;
            return current;
        }

        return onStackOverflow(nbytes);
    }

    void deallocateStack(void* p) {
        m_stack.m_current = (char*)p;

        if (m_stack.m_current == m_stack.m_base) {
            onStackUnderflow(p);
        }
    }

    static hkThreadMemory& getInstance();
    static int constSizeToRow(int size);

    void* allocateChunk(int nbytes, HK_MEMORY_CLASS cl);
    void* allocateChunkConstSize(int nbytes, HK_MEMORY_CLASS cl);
    void* onRowEmpty(int row, HK_MEMORY_CLASS cl);
    void deallocateChunk(void* p, int nbytes, HK_MEMORY_CLASS cl);
    void deallocateChunkConstSize(void* p, int nbytes, HK_MEMORY_CLASS cl);
    void onRowFull(int row, void* p, HK_MEMORY_CLASS cl);

    hkMemory* m_memory;
    int m_referenceCount;
    unsigned char _padC[0x10 - 0xC];
    Stack m_stack;
    int m_stackSize;
    int m_maxNumElemsOnFreeList;
    FreeList m_free_list[17];
};

extern hkThreadMemory* hkThreadMemory__s_threadMemoryInstance;

inline hkThreadMemory& hkThreadMemory::getInstance() {
    return *hkThreadMemory__s_threadMemoryInstance;
}

class hkArrayUtil {
public:
    static hkResult _reserve(void* array, int reqElems, int sizeElem);
};

template <class T>
class hkArray {
public:
    enum {
        CAPACITY_MASK = int(0x3FFFFFFF),
        DONT_DEALLOCATE_FLAG = int(0x80000000)
    };

    hkArray() : m_data(0), m_size(0), m_capacityAndFlags(DONT_DEALLOCATE_FLAG) {}
    hkArray(T* buffer, int size, int capacity)
        : m_data(buffer), m_size(size), m_capacityAndFlags(capacity | DONT_DEALLOCATE_FLAG) {}

    // Defined at the foot, after the collector destructors that call it.
    ~hkArray();

    void operator delete(void* p) {
        if (p) {
            hkThreadMemory::getInstance().deallocateChunkConstSize(p, sizeof(hkArray<T>),
                                                                   HK_MEMORY_CLASS_ARRAY);
        }
    }

    int getSize() const { return m_size; }
    void clear() { m_size = 0; }
    int indexOf(const T& t, int start = 0, int end = -1) const;
    void setSize(int n);
    T* expandBy(int n);
    int getCapacity() const { return m_capacityAndFlags & CAPACITY_MASK; }
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
    // Havok's: the size given back is the one the object was allocated with.
    void operator delete(void* p) {
        hkThreadMemory::getInstance().deallocateChunk(
            p, ((hkReferencedObject*)p)->m_memSizeAndFlags, HK_MEMORY_CLASS_BASE_CLASS);
    }

    virtual ~hkReferencedObject();
    virtual const hkClass* getClassType() const;
    virtual void calcContentStatistics(hkStatisticsCollector* collector,
                                       const hkClass* cls) const;

    void addReference() const;
    void removeReference() const;
    int getReferenceCount() const { return m_referenceCount; }

    unsigned short m_memSizeAndFlags;
    short m_referenceCount;
};

// ---------------------------------------------------------------------------
// Havok memory and maps

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

class hkpCollisionFilter : public hkReferencedObject {};

class hkpGroupFilter : public hkpCollisionFilter {
public:
    hkBool isCollisionEnabled(unsigned int infoA, unsigned int infoB) const;

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

    const hkpCollisionFilter* getCollisionFilter() const { return m_collisionFilter; }

    unsigned char _pad0[0x78];
    hkpCollisionFilter* m_collisionFilter;
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
    void* getOwner() const { return (void*)((char*)this + m_ownerOffset); }
    unsigned int getCollisionFilterInfo() const { return m_collisionFilterInfo; }

    unsigned char _pad4[0x10 - 0x4];
    signed char m_ownerOffset;
    unsigned char _pad11[0x18 - 0x11];
    // m_broadPhaseHandle's
    signed char m_type;
    unsigned char _pad19[0x1C - 0x19];
    unsigned int m_collisionFilterInfo;
};

class hkpPropertyValue {
public:
    unsigned long long m_data;
};

class hkpProperty {
public:
    unsigned int m_key;
    unsigned int m_alignmentPadding;
    hkpPropertyValue m_value;
};

class hkpWorldObject : public hkReferencedObject {
public:
    enum MtChecks {
        MULTI_THREADING_CHECKS_ENABLE = 0,
        MULTI_THREADING_CHECKS_IGNORE = 1
    };

    bool hasProperty(unsigned int key,
                     MtChecks mtCheck = MULTI_THREADING_CHECKS_ENABLE) const;

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
    unsigned char _pad30[0x78 - 0x30];
    hkArray<hkpProperty> m_properties;
    hkReferencedObject* m_aiData;
};

// The slots are the image's (tools/vtslot.py __vt__16hkpMaxSizeMotion); the
// eight from setMass to getInertiaInvWorld are Havok's names for the slots
// the keyframed motion folds onto four bodies.
class hkpMotion : public hkReferencedObject {
public:
    enum MotionType {
        MOTION_DYNAMIC = 1,
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
    // The rest of the entity's motion, and its members after it: the body
    // AddRigidBodyToSystem allocates is 0x220 bytes.
    unsigned char _pad1A0[0x220 - 0x1A0];
};

class hkpRigidBody : public hkpEntity {
public:
    // Havok's class allocator, as hkpPhysicsSystem's below.
    void* operator new(unsigned long nbytes) {
        hkReferencedObject* b = (hkReferencedObject*)hkThreadMemory::getInstance().allocateChunk(
            (int)nbytes, HK_MEMORY_CLASS_ENTITY);
        b->m_memSizeAndFlags = (unsigned short)nbytes;
        return b;
    }

    hkpRigidBody(const hkpRigidBodyCinfo& info);

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

typedef char _size_hkpRigidBody[(sizeof(hkpRigidBody) == 0x220) ? 1 : -1];

void xHavok_UpdateRigidBodyMotion(hkpRigidBody* body, const hkVector4& pos,
                                  const hkQuaternion& rot, float dt);

class hkpCdPoint;

class hkpCdPointCollector {
public:
    void operator delete(void* p) {
        if (p) {
            hkThreadMemory::getInstance().deallocateChunkConstSize(
                p, sizeof(hkpCdPointCollector), HK_MEMORY_CLASS_AGENT);
        }
    }

    hkpCdPointCollector() { reset(); }
    virtual ~hkpCdPointCollector() {}
    virtual void addCdPoint(const hkpCdPoint& point) = 0;
    virtual void reset();

    float m_earlyOutDistance;
};

class hkContactPoint {
public:
    hkVector4 m_position;
    hkVector4 m_separatingNormal;
};

class hkpRootCdPoint {
public:
    hkContactPoint m_contact;
    const hkpCollidable* m_rootCollidableA;
    unsigned int m_shapeKeyA;
    const hkpCollidable* m_rootCollidableB;
    unsigned int m_shapeKeyB;
};

template <class T, int N>
class hkInplaceArray : public hkArray<T> {
public:
    hkInplaceArray(int size = 0) : hkArray<T>(m_storage, size, N) {}
    ~hkInplaceArray() {}

    T m_storage[N];
};

template <class T>
class hkLocalArray : public hkArray<T> {
public:
    void operator delete(void* p) {
        if (p) {
            hkThreadMemory::getInstance().deallocateChunkConstSize(p, sizeof(hkLocalArray<T>),
                                                                   HK_MEMORY_CLASS_ARRAY);
        }
    }

    // Defined at the foot: retail calls both.
    hkLocalArray(int capacity);
    ~hkLocalArray();

    T* m_localMemory;
};

class hkpAllCdPointCollector : public hkpCdPointCollector {
public:
    // Its size, not the size of the class deleted: the destructors of the
    // collectors derived from it pass 0x1A0 too.
    void operator delete(void* p) {
        if (p) {
            hkThreadMemory::getInstance().deallocateChunkConstSize(
                p, sizeof(hkpAllCdPointCollector), HK_MEMORY_CLASS_AGENT);
        }
    }

    hkpAllCdPointCollector();
    virtual ~hkpAllCdPointCollector();
    virtual void addCdPoint(const hkpCdPoint& point);
    virtual void reset();

    hkInplaceArray<hkpRootCdPoint, 8> m_hits;
};

typedef char _size_hkpAllCdPointCollector[(sizeof(hkpAllCdPointCollector) == 0x1A0) ? 1 : -1];

class hkpWorldObject;

class TriggerIdentifyingPointCollector : public hkpAllCdPointCollector {
public:
    TriggerIdentifyingPointCollector();
    virtual ~TriggerIdentifyingPointCollector();

    hkArray<hkpWorldObject*> m_objectsWeHit;
    hkArray<hkContactPoint> m_contacts;
};

typedef char _size_TriggerIdentifyingPointCollector[
    (sizeof(TriggerIdentifyingPointCollector) == 0x1C0) ? 1 : -1];

class hkpShapeRayCastCollectorOutput {
public:
    hkpShapeRayCastCollectorOutput() { reset(); }

    void reset() {
        m_hitFraction = 1.0f;
        m_extraInfo = -1;
    }

    hkVector4 m_normal;
    float m_hitFraction;
    int m_extraInfo;
    int m_pad[2];
};

class hkpShapeRayCastOutput : public hkpShapeRayCastCollectorOutput {
public:
    hkpShapeRayCastOutput() { _reset(); }

    void reset() {
        hkpShapeRayCastCollectorOutput::reset();
        _reset();
    }

    void _reset() {
        m_shapeKeyIndex = 0;
        m_shapeKeys[0] = (unsigned int)-1;  // HK_INVALID_SHAPE_KEY
    }

    unsigned int m_shapeKeys[8];
    int m_shapeKeyIndex;
};

class hkpWorldRayCastOutput : public hkpShapeRayCastOutput {
public:
    hkpWorldRayCastOutput() { reset(); }

    void reset() {
        hkpShapeRayCastOutput::reset();
        m_rootCollidable = 0;
    }

    const hkpCollidable* m_rootCollidable;
};

typedef char _size_hkpWorldRayCastOutput[(sizeof(hkpWorldRayCastOutput) == 0x50) ? 1 : -1];

class hkpRayHitCollector {
public:
    hkpRayHitCollector() { reset(); }

    virtual void addRayHit(const hkpCdBody& cdBody,
                           const hkpShapeRayCastCollectorOutput& hitInfo) = 0;
    virtual ~hkpRayHitCollector() {}

    void reset() { m_earlyOutHitFraction = 1.0f; }

    float m_earlyOutHitFraction;
};

class hkpAllRayHitCollector : public hkpRayHitCollector {
public:
    hkpAllRayHitCollector() { reset(); }
    virtual ~hkpAllRayHitCollector();

    const hkArray<hkpWorldRayCastOutput>& getHits() const { return m_hits; }
    void sortHits();

    void reset() {
        m_hits.clear();
        hkpRayHitCollector::reset();
    }

    virtual void addRayHit(const hkpCdBody& cdBody,
                           const hkpShapeRayCastCollectorOutput& hitInfo);

    hkInplaceArray<hkpWorldRayCastOutput, 8> m_hits;
};

typedef char _size_hkpAllRayHitCollector[(sizeof(hkpAllRayHitCollector) == 0x2A0) ? 1 : -1];

class hkpSurfaceInfo {
public:
    enum SupportedState {
        UNSUPPORTED = 0,
        SLIDING = 1,
        SUPPORTED = 2
    };

    hkpSurfaceInfo();

    SupportedState m_supportedState;
    hkVector4 m_surfaceNormal;
    hkVector4 m_surfaceVelocity;
    float m_surfaceDistance;
    hkpMotion::MotionType m_surfaceMotionType;
};

typedef char _size_hkpSurfaceInfo[(sizeof(hkpSurfaceInfo) == 0x40) ? 1 : -1];

template <class T>
class hkPadSpu {
public:
    void operator=(T x) { m_storage = x; }
    operator T() const { return m_storage; }

    T m_storage;
};

class hkStepInfo {
public:
    void set(float startTime, float endTime);

    hkPadSpu<float> m_startTime;
    hkPadSpu<float> m_endTime;
    hkPadSpu<float> m_deltaTime;
    hkPadSpu<float> m_invDeltaTime;
};

class hkpCharacterProxy;

// hkpCharacterProxy::getLinearVelocity, which hands back the address of its
// velocity (addi r3, r3, 16): the linker folded it onto
// Graphics::StaticBuilder::GetGeometry, the name retail branches to.
extern "C" const hkVector4* GetGeometry__Q28Graphics13StaticBuilderFv(
    const hkpCharacterProxy* proxy);

class hkpCharacterProxy {
public:
    const hkVector4& getPosition() const;
    const hkVector4& getLinearVelocity() const {
        return *GetGeometry__Q28Graphics13StaticBuilderFv(this);
    }
    void checkSupport(const hkVector4& direction, hkpSurfaceInfo& ground);
    void checkSupportWithCollector(const hkVector4& direction, hkpSurfaceInfo& ground,
                                   hkpAllCdPointCollector& startPointCollector);
};

class hkpCharacterRigidBody {
public:
    const hkVector4& getPosition() const;
    const hkVector4& getLinearVelocity() const;
    void checkSupport(const hkStepInfo& stepInfo, hkpSurfaceInfo& ground) const;
    void checkSupport(const hkStepInfo& stepInfo, hkpSurfaceInfo& ground,
                      hkpCdPointCollector* startPointCollector) const;
};

enum ControllerType {
    NONE = 0,
    PROXY = 1,
    RIGID_BODY = 2
};

// GetCharacterProxy is called, GetCharacterRigidBody taken in line: the
// first is defined at the foot.
class xHavokCharacterController {
public:
    hkpCharacterProxy* GetCharacterProxy() const;
    hkpCharacterRigidBody* GetCharacterRigidBody() const {
        return controllerType == RIGID_BODY ? characterRigidBody : 0;
    }

    union {
        hkpCharacterProxy* characterProxy;
        hkpCharacterRigidBody* characterRigidBody;
    };
    ControllerType controllerType;
};

class hkpPhysicsSystem : public hkReferencedObject {
public:
    // Havok's class allocator: the chunk's size goes into m_memSizeAndFlags
    // before the object is constructed.
    void* operator new(unsigned long nbytes) {
        hkReferencedObject* b = (hkReferencedObject*)hkThreadMemory::getInstance().allocateChunk(
            (int)nbytes, HK_MEMORY_CLASS_WORLD);
        b->m_memSizeAndFlags = (unsigned short)nbytes;
        return b;
    }

    hkpPhysicsSystem();

    void addRigidBody(hkpRigidBody* body);

    const hkArray<hkpRigidBody*>& getRigidBodies() const { return m_rigidBodies; }

    hkArray<hkpRigidBody*> m_rigidBodies;
    // m_constraints, m_actions, m_phantoms, m_name, m_userData, m_active
    unsigned char _pad14[0x44 - 0x14];
};

typedef char _size_hkpPhysicsSystem[(sizeof(hkpPhysicsSystem) == 0x44) ? 1 : -1];

// ---------------------------------------------------------------------------
// Math

namespace Math {

// Not a named type in the DWARF; retail passes 0.
enum HintOrthonormalEnum {};

class Vector4 {
public:
    Vector4() {}

    operator const hkVector4&() const { return *(const hkVector4*)this; }

    class DataType {
    public:
        float x;
        float y;
        float z;
        float w;
    };

    Vector4& Assign(float x, float y, float z, float w);

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
    Vector GetRecipScaleSqr() const;
    int CheckHint(HintOrthonormalEnum hint) const;
    void MakeScale(const Vector& scale);
    void Assign(float x0, float y0, float z0, float x1, float y1, float z1, float x2,
                float y2, float z2);

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
    void Invert(HintOrthonormalEnum hint);
};

extern Matrix43 _matIdentity;

void Orthonormalize(Matrix43& o, const Matrix43& a);
void Normalize(Matrix43& o, const Matrix43& a);
void Slerp(Quaternion& o, const Quaternion& a, const Quaternion& b, float t);
void Add(Vector& o, const Vector& a, const Vector& b);

// Not a named type in the DWARF; retail passes 0, and no enumerator name is
// known.
enum HintOrthogonalEnum {};
void Invert(Matrix43& o, const Matrix43& a, HintOrthogonalEnum hint);
void Transpose(Matrix33& o, const Matrix33& a);
void Mul(Matrix33& o, const Matrix33& a, const Matrix33& b);
void Negate(Vector4& o, const Vector4& a);
void Mul(Vector4& o, const Vector4& a, float s);
float rsqrt(float x);
float sqrt(float x);
bool feq(float a, float b, float epsilon);

// Not a named type in the DWARF; the name is the symbol's.
enum MatrixOpScaleEnum {};
void Mul(Matrix43& o, MatrixOpScaleEnum op, const Matrix43& a, const Vector& s);

extern Vector vec4Zero;
bool Equal4(const Vector4& a, const Vector4& b, float epsilon);

extern Matrix43 _matZero;

inline Vector operator*(const Vector4& a, float s) {
    Vector r;
    Mul(r, a, s);
    return r;
}

inline Vector operator+(const Vector& a, const Vector& b) {
    Vector r;
    Add(r, a, b);
    return r;
}

inline Vector operator-(const Vector& a) {
    Vector r;
    Negate(r, a);
    return r;
}

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
extern "C" void PSMTXMultVecSR(const Math::Matrix43* m, const Math::Vector* src,
                               Math::Vector* dst);

namespace Math {

inline void Mul(Matrix43& o, const Matrix43& a, const Matrix43& b) {
    PSMTXConcat(&a, &b, &o);
}

}  // namespace Math

class xVec3 {
public:
    float length2() const;
    float length() const {
        float len2 = length2();
        return len2 * Math::rsqrt(len2);
    }
    float normalize();
    xVec3& operator*=(float s);
    void cross(const xVec3& a, const xVec3& b);
    void Sub(const xVec3& a, const xVec3& b);

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
extern unsigned int updateFrameNumber;
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
void xMat3x3Normalize(xMat3x3* o, const xMat3x3* m);
void v3normalize(float& len, xVec3* out, xVec3* in);
void v3add(xVec3* o, xVec3* a, xVec3* b);

class xQuat {
public:
    xVec3 v;
    float s;
};

void xQuatFromMat(xQuat* q, const xMat3x3* m);

// ---------------------------------------------------------------------------
// The engine's side

namespace Graphics {

// The members this file reads, at the DWARF's offsets.
class SkinCluster {
public:
    unsigned char _pad0[0x8];
    Math::Matrix43* invBindMat;
};

class Skeleton {
public:
    unsigned char _pad0[0x24];
    SkinCluster* skins;
    int skinCount;
    int skinJointTotal;
};

class ModelPrototype {
public:
    unsigned char _pad0[0x8];
    Math::Matrix43* childTransforms;
    unsigned char _padC[0x2C - 0xC];
    unsigned short nonRefTransformCount;
    unsigned char _pad2E[0x38 - 0x2E];
    Skeleton* skeleton;
    unsigned char _pad3C[0x44 - 0x3C];
    bool segmentedModel;
};

class ModelJointBuffer {
public:
    Math::Matrix43* data;
    ModelJointBuffer* next;
    unsigned int updateFrame;
};

class Model {
public:
    void CalcWorldChildTransforms(Math::Matrix43* childTransformsOut) const;
    void SetRootTransform(const Math::Matrix43& transform);
    void SetChildTransform(int index, const Math::Matrix43& transform);
    void XformSetDirty(unsigned int index);
    Math::Matrix43* NextJointMatrices(bool& advanced);

    Math::Matrix43 rootTransform;
    unsigned short visibleCount;
    unsigned short childTransformCount;
    unsigned short renderableCount;
    unsigned short renderCustomizerCount;
    void* renderables;
    void* xforms;
    ModelJointBuffer* joints;
    void* morphWeights;
    ModelPrototype* modelProto;
    void* geoms;
    Math::Matrix43* childTransforms;
    unsigned short* childTransformParents;
    unsigned short* renderableTransformMap;
    void* refModelInstanceChildXforms;
    unsigned short dirtyRefChildTransformCount;
    unsigned short dirtyNonRefChildTransformCount;
    unsigned int* xformDirtyBits;
};

}  // namespace Graphics

namespace World {

class CollisionMeshBlobEntity {
public:
    hkpPhysicsSystem* GetPhysicsSystem(int index, int flags) const;

    unsigned char _pad0[0x38];
    int collFilter;
};

class ModelPrototypeEntity {
public:
    unsigned char _pad0[0x6C];
    CollisionMeshBlobEntity* collmeshBlob;
};

class ModelInstanceArticle {
public:
    unsigned char _pad0[0x18];
    ModelPrototypeEntity* protoEnt;
    unsigned char _pad1C[0x24 - 0x1C];
    Graphics::Model model;
    unsigned char _pad8C[0x98 - 0x8C];
};

class xOGModel;
class xOGModelUpdater;

}  // namespace World

class EmbeddedListNode {
public:
    EmbeddedListNode* next;
    EmbeddedListNode* prev;
};

// A list threaded through T by the node at nodeOffset in it.
template <class T, int nodeOffset>
class EmbeddedList {
public:
    void Remove(T* item);
    void PushBack(T* item);

    EmbeddedListNode head;
    unsigned long size;
};

namespace World {

class xOGModel {
public:
    ModelPrototypeEntity* GetPrototype() const;
    void UpdaterSwitch(xOGModelUpdater* newUpdater, void* newParent);

    // xModelInstance's, first in it
    xMat4x3 Mat;
    unsigned char _pad40[0xC4 - 0x40];
    ModelInstanceArticle mModelArt;
    xOGModelUpdater* updater;
    void* updateParent;
    EmbeddedListNode updateNode;
};

class xOGModelUpdater {
public:
    virtual void _u0();

    EmbeddedList<xOGModel, 356> updList;
};

extern xOGModelUpdater g_modelUpdateDefault;

class xOGModelRefPtr;

class xOGModelRef {
public:
    bool IsValid() const { return data != 0; }

    xOGModel* data;
    xOGModelRefPtr* autoptr;
};

class xOGModelHandle : public xOGModelRef {};

}  // namespace World

// The members of an entity this file reads, at the DWARF's offsets.
class xEntFrame {
public:
    xMat4x3 oldmat;
    unsigned char _pad40[0x88 - 0x40];
    xVec3 vel;
};


enum hkpCollidableQualityType {
    HK_COLLIDABLE_QUALITY_INVALID = -1,
    HK_COLLIDABLE_QUALITY_FIXED = 0,
    HK_COLLIDABLE_QUALITY_KEYFRAMED = 1,
    HK_COLLIDABLE_QUALITY_DEBRIS = 2,
    HK_COLLIDABLE_QUALITY_DEBRIS_SIMPLE_TOI = 3,
    HK_COLLIDABLE_QUALITY_MOVING = 4,
    HK_COLLIDABLE_QUALITY_CRITICAL = 5,
    HK_COLLIDABLE_QUALITY_BULLET = 6,
    HK_COLLIDABLE_QUALITY_USER = 7,
    HK_COLLIDABLE_QUALITY_CHARACTER = 8,
    HK_COLLIDABLE_QUALITY_KEYFRAMED_REPORTING = 9,
    HK_COLLIDABLE_QUALITY_MAX = 10
};

template <class ENUM, class STORAGE>
class hkEnum {
public:
    void operator=(ENUM e) { m_storage = (STORAGE)e; }

    STORAGE m_storage;
};

class hkLocalFrame;

class hkpRigidBodyCinfo {
public:
    hkpRigidBodyCinfo();

    unsigned int m_collisionFilterInfo;
    hkpShape* m_shape;
    hkLocalFrame* m_localFrame;
    signed char m_collisionResponse;
    unsigned short m_processContactCallbackDelay;
    hkVector4 m_position;
    hkQuaternion m_rotation;
    hkVector4 m_linearVelocity;
    hkVector4 m_angularVelocity;
    hkMatrix3 m_inertiaTensor;
    hkVector4 m_centerOfMass;
    float m_mass;
    float m_linearDamping;
    float m_angularDamping;
    float m_gravityFactor;
    float m_friction;
    float m_restitution;
    float m_maxLinearVelocity;
    float m_maxAngularVelocity;
    float m_allowedPenetrationDepth;
    hkEnum<hkpMotion::MotionType, signed char> m_motionType;
    signed char m_rigidBodyDeactivatorType;
    signed char m_solverDeactivation;
    hkEnum<hkpCollidableQualityType, signed char> m_qualityType;
    signed char m_autoRemoveLevel;
    signed char m_numUserDatasInContactPointProperties;
    hkBool m_forceCollideOntoPpu;
};

typedef char _size_hkpRigidBodyCinfo[(sizeof(hkpRigidBodyCinfo) == 0xC0) ? 1 : -1];

class CHavokShapeBuilder {
public:
    hkpShape* getShape(const hkpShape* shape, const hkVector4& scale);
};

template <class T>
class hkSingleton {
public:
    static T& getInstance() { return *s_instance; }

    static T* s_instance;
};

class hkpInertiaTensorComputer {
public:
    static void setShapeVolumeMassProperties(const hkpShape* shape, float mass,
                                             hkpRigidBodyCinfo& bodyInfo);
};

// ---------------------------------------------------------------------------
// Havok constraints: the members ScaleConstraintBodyAttachSpace reads, at the
// DWARF's offsets.

// A debug check compiled to nothing: its test stays in the listing, its
// branch does not.
inline void DebugCheck() {}

class hkpConstraintInstance;
class hkpConstraintRuntime;
class hkpSolverResults;
class hkpConstraintQueryIn;
class hkpConstraintQueryOut;
class hkpConstraintMotor;

class hkpConstraintAtom {
public:
    unsigned short m_type;
};

class hkpSetLocalTransformsConstraintAtom : public hkpConstraintAtom {
public:
    hkTransform m_transformA;
    hkTransform m_transformB;
};

class hkpSetLocalTranslationsConstraintAtom : public hkpConstraintAtom {
public:
    hkVector4 m_translationA;
    hkVector4 m_translationB;
};

class hkpConstraintInfo {
public:
    int m_maxSizeOfSchema;
    int m_sizeOfSchemas;
    int m_numSolverResults;
    int m_numSolverElemTemps;
};

// The slots are the image's (tools/vtslot.py __vt__24hkpRagdollConstraintData).
class hkpConstraintData : public hkReferencedObject {
public:
    // Not a named type in the DWARF: the values are the jump table's.
    enum ConstraintType {
        CONSTRAINT_TYPE_HINGE = 1,
        CONSTRAINT_TYPE_RAGDOLL = 7,
        CONSTRAINT_TYPE_STIFFSPRING = 8
    };

    class ConstraintInfo : public hkpConstraintInfo {
    public:
        hkpConstraintAtom* m_atoms;
        unsigned int m_sizeOfAllAtoms;
    };

    class RuntimeInfo;

    virtual void setMaxLinearImpulse(float maxImpulse);
    virtual float getMaxLinearImpulse() const;
    virtual void setBodyToNotify(int bodyIdx);
    virtual unsigned char getNotifiedBodyIndex() const;
    virtual hkBool isValid() const;
    virtual int getType() const;
    virtual void getRuntimeInfo(hkBool wantRuntime, RuntimeInfo& infoOut) const;
    virtual hkpSolverResults* getSolverResults(hkpConstraintRuntime* runtime);
    virtual void addInstance(hkpConstraintInstance* constraint, hkpConstraintRuntime* runtime,
                             int sizeOfRuntime) const;
    virtual void buildJacobian(const hkpConstraintQueryIn& in, hkpConstraintQueryOut& out);
    virtual hkBool isBuildJacobianCallbackRequired() const;
    virtual void buildJacobianCallback(const hkpConstraintQueryIn& in);
    virtual void getConstraintInfo(ConstraintInfo& infoOut) const;

    unsigned long m_userData;
};

class hkpRagdollMotorConstraintAtom : public hkpConstraintAtom {
public:
    hkBool m_isEnabled;
    short m_initializedOffset;
    short m_previousTargetAnglesOffset;
    hkMatrix3 m_target_bRca;
    hkpConstraintMotor* m_motors[3];
};

class hkpAngFrictionConstraintAtom : public hkpConstraintAtom {
public:
    unsigned char m_isEnabled;
    unsigned char m_firstFrictionAxis;
    unsigned char m_numFrictionAxes;
    float m_maxFrictionTorque;
};

class hkpTwistLimitConstraintAtom : public hkpConstraintAtom {
public:
    unsigned char m_isEnabled;
    unsigned char m_twistAxis;
    unsigned char m_refAxis;
    float m_minAngle;
    float m_maxAngle;
    float m_angularLimitsTauFactor;
};

class hkpConeLimitConstraintAtom : public hkpConstraintAtom {
public:
    unsigned char m_isEnabled;
    unsigned char m_twistAxisInA;
    unsigned char m_refAxisInB;
    unsigned char m_angleMeasurementMode;
    unsigned char m_memOffsetToAngleOffset;
    float m_minAngle;
    float m_maxAngle;
    float m_angularLimitsTauFactor;
};

class hkpBallSocketConstraintAtom : public hkpConstraintAtom {
public:
    unsigned char m_bodiesToNotify;
    unsigned char m_stabilizationFactor;
    float m_maxImpulse;
};

class hkpRagdollConstraintData : public hkpConstraintData {
public:
    void* operator new(unsigned long nbytes) {
        hkReferencedObject* b = (hkReferencedObject*)hkThreadMemory::getInstance().allocateChunk(
            (int)nbytes, HK_MEMORY_CLASS_CONSTRAINT);
        b->m_memSizeAndFlags = (unsigned short)nbytes;
        return b;
    }

    hkpRagdollConstraintData();

    void setInBodySpace(const hkVector4& pivotA, const hkVector4& pivotB,
                        const hkVector4& planeAxisA, const hkVector4& planeAxisB,
                        const hkVector4& twistAxisA, const hkVector4& twistAxisB);
    void setMaxFrictionTorque(float tmag);
    void setConeLimitStabilization(hkBool enable);
    hkpConstraintMotor* getTwistMotor() const;
    void setTwistMotor(hkpConstraintMotor* motor);
    hkpConstraintMotor* getConeMotor() const;
    void setConeMotor(hkpConstraintMotor* motor);
    hkpConstraintMotor* getPlaneMotor() const;
    void setPlaneMotor(hkpConstraintMotor* motor);
    void getTarget(hkMatrix3& target_out);
    void setTarget(const hkMatrix3& target_cbRca);

    float getMaxFrictionTorque() const { return m_atoms.m_angFriction.m_maxFrictionTorque; }

    // Retail compares the twist limit's factor with the cone limit's and
    // keeps nothing of it.
    float getAngularLimitsTauFactor() const {
        if (m_atoms.m_twistLimit.m_angularLimitsTauFactor !=
            m_atoms.m_coneLimit.m_angularLimitsTauFactor) {
            DebugCheck();
        }

        return m_atoms.m_twistLimit.m_angularLimitsTauFactor;
    }

    void setAngularLimitsTauFactor(float mag) {
        m_atoms.m_twistLimit.m_angularLimitsTauFactor = mag;
        m_atoms.m_coneLimit.m_angularLimitsTauFactor = mag;
        m_atoms.m_planesLimit.m_angularLimitsTauFactor = mag;
    }

    float getTwistMinAngularLimit() const { return m_atoms.m_twistLimit.m_minAngle; }
    float getTwistMaxAngularLimit() const { return m_atoms.m_twistLimit.m_maxAngle; }
    float getPlaneMinAngularLimit() const { return m_atoms.m_planesLimit.m_minAngle; }
    float getPlaneMaxAngularLimit() const { return m_atoms.m_planesLimit.m_maxAngle; }
    float getConeAngularLimit() const { return m_atoms.m_coneLimit.m_maxAngle; }
    void setTwistMinAngularLimit(float rad) { m_atoms.m_twistLimit.m_minAngle = rad; }
    void setTwistMaxAngularLimit(float rad) { m_atoms.m_twistLimit.m_maxAngle = rad; }
    void setPlaneMinAngularLimit(float rad) { m_atoms.m_planesLimit.m_minAngle = rad; }
    void setPlaneMaxAngularLimit(float rad) { m_atoms.m_planesLimit.m_maxAngle = rad; }
    void setConeAngularLimit(float rad) { m_atoms.m_coneLimit.m_maxAngle = rad; }

    hkBool getConeLimitStabilization() const {
        return m_atoms.m_coneLimit.m_memOffsetToAngleOffset != 0;
    }

    class Atoms {
    public:
        hkpSetLocalTransformsConstraintAtom m_transforms;
        hkpRagdollMotorConstraintAtom m_ragdollMotors;
        hkpAngFrictionConstraintAtom m_angFriction;
        hkpTwistLimitConstraintAtom m_twistLimit;
        hkpConeLimitConstraintAtom m_coneLimit;
        hkpConeLimitConstraintAtom m_planesLimit;
        hkpBallSocketConstraintAtom m_ballSocket;
    };

    Atoms m_atoms;
};

typedef char _size_hkpRagdollConstraintData[
    (sizeof(hkpRagdollConstraintData) == 0x140) ? 1 : -1];

class hkpStiffSpringConstraintAtom : public hkpConstraintAtom {
public:
    float m_length;
};

class hkpStiffSpringConstraintData : public hkpConstraintData {
public:
    void* operator new(unsigned long nbytes) {
        hkReferencedObject* b = (hkReferencedObject*)hkThreadMemory::getInstance().allocateChunk(
            (int)nbytes, HK_MEMORY_CLASS_CONSTRAINT);
        b->m_memSizeAndFlags = (unsigned short)nbytes;
        return b;
    }

    hkpStiffSpringConstraintData();

    void setInBodySpace(const hkVector4& pivotA, const hkVector4& pivotB, float restLength) {
        m_atoms.m_pivots.m_translationA = pivotA;
        m_atoms.m_pivots.m_translationB = pivotB;
        m_atoms.m_spring.m_length = restLength;
    }

    float getSpringLength() const { return m_atoms.m_spring.m_length; }

    class Atoms {
    public:
        hkpSetLocalTranslationsConstraintAtom m_pivots;
        hkpStiffSpringConstraintAtom m_spring;
    };

    Atoms m_atoms;
};

typedef char _size_hkpStiffSpringConstraintData[
    (sizeof(hkpStiffSpringConstraintData) == 0x50) ? 1 : -1];

class hkpHingeConstraintData : public hkpConstraintData {
public:
    void* operator new(unsigned long nbytes) {
        hkReferencedObject* b = (hkReferencedObject*)hkThreadMemory::getInstance().allocateChunk(
            (int)nbytes, HK_MEMORY_CLASS_CONSTRAINT);
        b->m_memSizeAndFlags = (unsigned short)nbytes;
        return b;
    }

    hkpHingeConstraintData();

    void setInBodySpace(const hkVector4& pivotA, const hkVector4& pivotB,
                        const hkVector4& axisA, const hkVector4& axisB);

    unsigned char _pad0C[0xB0 - 0xC];
};

typedef char _size_hkpHingeConstraintData[(sizeof(hkpHingeConstraintData) == 0xB0) ? 1 : -1];

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

    enum JointTransformSpace {
        MODEL_SPACE = 0,
        MODEL_INV_BIND_SPACE = 1
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

    bool AddRigidBodyToSystem(const hkpShape* shape, const hkVector4& shapeScale,
                              const hkTransform& worldTransform, unsigned int collisionFilter,
                              hkpMotion::MotionType motionType, float mass, float friction,
                              float elasticity, float linearDamping, float angularDamping,
                              hkpCollidableQualityType qualityType, unsigned long long* id);
    bool CreateFromCollisionShape(const hkpShape* shape, const hkVector4& shapeScale,
                                  const xMat4x3& worldTransformMat, unsigned int collisionFilter,
                                  hkpMotion::MotionType motionType, float mass, float friction,
                                  float elasticity, float linearDamping, float angularDamping,
                                  hkpCollidableQualityType qualityType, unsigned long long* id);
    bool CreateFromCollisionShape(const hkpShape* shape, const hkVector4& shapeScale,
                                  const hkTransform& worldTransform, unsigned int collisionFilter,
                                  hkpMotion::MotionType motionType, float mass, float friction,
                                  float elasticity, float linearDamping, float angularDamping,
                                  hkpCollidableQualityType qualityType, unsigned long long* id);
    bool CreateFromCollisionShapes(const hkArray<const hkpShape*>& shapes,
                                   const hkArray<hkVector4>& shapeScales,
                                   const hkArray<hkTransform>& worldTransforms,
                                   const hkArray<unsigned int>& collisionFilters,
                                   const hkArray<hkpMotion::MotionType>& motionTypes,
                                   const hkArray<float>& masses, const hkArray<float>& frictions,
                                   const hkArray<float>& elasticities,
                                   const hkArray<hkpCollidableQualityType>& qualityTypes,
                                   unsigned long long* id);
    bool CreateFromPackedData(const Graphics::Model& model,
                              const World::CollisionMeshBlobEntity* packedData,
                              const hkTransform& worldTransform, float mass, float friction,
                              float elasticity, float linearDamping, float angularDamping,
                              hkpCollidableQualityType qualityType,
                              hkpMotion::MotionType motionType, unsigned int collisionFilter,
                              const hkVector4& scale, unsigned long long* id);
    static hkpConstraintData* ScaleConstraintBodyAttachSpace(hkpConstraintData* constraintData,
                                                             const hkVector4& scaleA,
                                                             const hkVector4& scaleB,
                                                             unsigned long long* id,
                                                             bool forceCopy);
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
    void MatchPhysicsToRenderedModel(Graphics::Model& model,
                                     PhysicsRenderableMatchType matchType,
                                     JointTransformSpace jointSpace);

    static hkPointerMap<const World::CollisionMeshBlobEntity*,
                        PhysicsJointRelativeTransforms*>* jointRelativeCreationMap;

    PhysicsObjectType physicsObjectType;
    World::CollisionMeshBlobEntity* packedPhysicsData;
    hkpPhysicsSystem* physicsSystem;
    hkVector4 creationScale;
};

class PhysicsDataStruct {
public:
    float mass;
    float friction;
    float elasticity;
    float linearDamping;
    float angularDamping;
};

class xEntAsset {
public:
    unsigned char _pad0[0x98];
    PhysicsDataStruct physicsData;
};

// The members of an entity this file reads, at the DWARF's offsets (0xC0).
// The virtuals are there for their slots; none is defined here, so no vtable
// is emitted.
class xEnt {
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
    virtual void _v18();
    virtual void _v19();
    virtual void _v20();
    virtual void _v21();
    virtual void _v22();
    virtual void _v23();
    virtual void _v24();
    virtual E_HAVOK_COLLIDE_FILTER_LAYER GetSceneInitCollisionFilter();

    unsigned char _pad4[0x18 - 0x4];
    unsigned long long id;
    unsigned int baseType;
    unsigned char _pad24[0x34 - 0x24];
    World::xOGModelHandle ogModel;
    xEntAsset* asset;
    E_HAVOK_COLLIDE_FILTER_LAYER storedCollisionLayer;
    unsigned char _pad44[0x4E - 0x44];
    unsigned char collType;
    unsigned char chkby;
    unsigned char penby;
    unsigned char collisionOn : 2;
    unsigned char _pad52[0x58 - 0x52];
    xEntFrame* frame;
    unsigned char _pad5C[0x70 - 0x5C];
    xVec3 pRigidBodyPostScale;
    unsigned char _pad7C[0x80 - 0x7C];
    xHavokPhysicsObject physicsObject;
    unsigned char _padA0[0xC0 - 0xA0];
};

class zNPCEntity : public xEnt {};

class zEnt : public xEnt {};

// Base type 0x5A.
class zEntSimpleObj : public zEnt {
public:
    unsigned int sflags;
};

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {

enum GlobalHeapEnum { GlobalHeap };

void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap, eMemMgrTag tag, bool zeroed);

}  // namespace Memory

inline void* operator new(unsigned long, void* p) { return p; }

class EntCollisionListener {
public:
    EntCollisionListener(hkpRigidBody* body, float hitSoundInterval, float rippleInterval);

    unsigned char _pad0[0x50];
};

void xHavok_AddToSimWorld(const hkpPhysicsSystem* system);
void xHavok_RemoveFromSimWorld(const hkpPhysicsSystem* system);

// hkVector4's (x, y, z, w = 0) constructor: the linker folded it onto
// Math::Vector4::Assign, which is the name retail branches to.
class hkVector4Init : public hkVector4 {
public:
    hkVector4Init(float x, float y, float z) {
        ((Math::Vector4*)this)->Assign(x, y, z, 0.0f);
    }
};

// always_inline takes the two helpers, hkVector4Init's constructor and
// normalize4 in line, as retail has them. Above every member definition, it
// cannot take the members AddToHavokSimWorld calls.
#pragma push
#pragma always_inline on

// Assign's result, not the temporary's address, is what retail passes on to
// operator=; the three floats are bound right to left, as an inline's are.
inline const hkVector4& hkVector4Set(hkVector4& v, float x, float y, float z) {
    return ((Math::Vector4&)v).Assign(x, y, z, 0.0f);
}

inline void hkVector4::normalize4() {
    float lengthSquared = x * x + y * y + z * z + w * w;
    float lengthInverse = (0.0f != lengthSquared) ? hkMath::sqrtInverse(lengthSquared) : 0.0f;
    mul4(lengthInverse);
}

inline void hkQuaternion::normalize() { m_vec.normalize4(); }

// An entity that already has a system: out of the world when its collision
// is off, otherwise matched to its model's transforms again. The scale is
// computed and not used. The name is ours.
inline void xHavok_ResyncSystem(xEnt* pEnt) {
    if (!pEnt->collisionOn) {
        xHavok_RemoveFromSimWorld(pEnt->physicsObject.physicsSystem);
        pEnt->physicsObject.Cleanup();
        return;
    }

    Graphics::Model& model = pEnt->ogModel.data->mModelArt.model;
    Math::Matrix43 rootTransform;
    xMat4x3ToNGMatrix(&rootTransform, &pEnt->ogModel.data->Mat);
    model.SetRootTransform(rootTransform);

    if (model.modelProto->segmentedModel) {
        for (unsigned short i = 0; i < model.childTransformCount; i++) {
            model.SetChildTransform(i, model.modelProto->childTransforms[i]);
        }
    }

    hkVector4Init scale(1.0f, 1.0f, 1.0f);
    E_HAVOK_COLLIDE_FILTER_LAYER filter;
    World::xOGModel* data = pEnt->ogModel.data;
    hkVector4 scaleTmp;
    (hkVector4&)scale = hkVector4Set(scaleTmp, data->Mat.left.length(),
                                     data->Mat.up.length(), data->Mat.at.length());

    filter = pEnt->GetSceneInitCollisionFilter();
    pEnt->physicsObject.MatchPhysicsToRenderedModel(model, xHavokPhysicsObject::TELEPORT,
                                                    xHavokPhysicsObject::MODEL_INV_BIND_SPACE);
    pEnt->physicsObject.SetCollisionFilter(filter);
}

// The system built from the model's packed collision data: the layer, the
// quality and the motion from the asset's mass and the entity's type. A
// model with no prototype counts as created. The name is ours.
inline bool xHavok_CreateSystem(xEnt* pEnt) {
    unsigned char motionType;
    unsigned char qualityType;
    unsigned int layer;
    World::CollisionMeshBlobEntity* collMesh;
    bool created;
    float mass = pEnt->asset->physicsData.mass;
    hkVector4Init creationScale(1.0f, 1.0f, 1.0f);
    layer = eNoCollisionLayer;
    bool dynamic = 0.0f != mass;

    if (pEnt->chkby != 0) {
        layer = dynamic ? eDynamicSimpleObjLayer : eFixedLayer;
    }


    if (pEnt->baseType == 0x56) {
        qualityType = HK_COLLIDABLE_QUALITY_KEYFRAMED;
        motionType = hkpMotion::MOTION_KEYFRAMED;
        layer = eKeyFrameObjLayer;
    } else {
        qualityType = dynamic ? HK_COLLIDABLE_QUALITY_MOVING : HK_COLLIDABLE_QUALITY_FIXED;
        motionType = dynamic ? hkpMotion::MOTION_DYNAMIC : hkpMotion::MOTION_FIXED;
    }

    pEnt->storedCollisionLayer = (E_HAVOK_COLLIDE_FILTER_LAYER)layer;

    World::ModelPrototypeEntity* protoEnt = pEnt->ogModel.data->mModelArt.protoEnt;

    if (protoEnt != 0) {
        collMesh = protoEnt->collmeshBlob;

        if (collMesh->collFilter == eFixedNoCamLayer && layer == eFixedLayer) {
            pEnt->storedCollisionLayer = eFixedNoCamLayer;
            layer = eFixedNoCamLayer;
        }

        if (collMesh != 0) {
            World::xOGModel* data = pEnt->ogModel.data;
            hkVector4 translation;
            hkQuaternion rotation;

            hkVector4 scaleTmp;
            (hkVector4&)creationScale = hkVector4Set(scaleTmp, data->Mat.left.length(),
                                                     data->Mat.up.length(), data->Mat.at.length());

            if (creationScale.dot3(creationScale) > 0.0f) {
                xMat3x3 tempMat = data->Mat;
                xMat3x3Normalize(&tempMat, &tempMat);
                xQuat quat;
                xQuatFromMat(&quat, &tempMat);
                ((Math::Vector4*)&rotation)->Assign(quat.v.x, quat.v.y, quat.v.z, quat.s);
                ((Math::Vector4*)&translation)
                    ->Assign(data->Mat.pos.x, data->Mat.pos.y, data->Mat.pos.z, 0.0f);
            }

            rotation.normalize();
            hkTransform transform(rotation, translation);

            created = pEnt->physicsObject.CreateFromPackedData(
                pEnt->ogModel.data->mModelArt.model, collMesh, transform, mass,
                pEnt->asset->physicsData.friction, pEnt->asset->physicsData.elasticity,
                pEnt->asset->physicsData.linearDamping, pEnt->asset->physicsData.angularDamping,
                (hkpCollidableQualityType)(signed char)qualityType,
                (hkpMotion::MotionType)(signed char)motionType, layer,
                creationScale, &pEnt->id);

            if (pEnt->baseType == 0x5A && created && dynamic) {
                ((zEntSimpleObj*)pEnt)->sflags |= 0x2000;
                pEnt->ogModel.data->UpdaterSwitch(&World::g_modelUpdateDefault, 0);
            }
        } else {
            created = false;
        }
    } else {
        created = true;
    }

    return created;
}

// NEAR MISS, 178 of 390 words (retail 392). The structure is retail's; what
// differs: the registers (pEnt r26 where retail has r30, the helpers' locals
// coloured otherwise); the scale temporary and the quaternion in each other's
// stack slots; the quaternion's length compare with the literal first; and
// CreateFromPackedData's arguments loaded in another order, retail
// zero-extending the quality and motion bytes before sign-extending them (two
// words that unsigned char locals did not give). Tried: the helpers out of
// line under -inline auto (381 of 132); in line under always_inline (189 of
// 390); Assign's result to operator=, the layer unsigned, the bytes unsigned
// char, rotation and translation swapped (182 of 390); the helpers' locals
// declared in retail's register order (178 of 390).
// Players and NPCs are left to their own controllers. Collision listeners
// only for bodies with mass.
void AddToHavokSimWorld(xEnt* pEnt) {
    if (pEnt->baseType == 0x38 || pEnt->baseType == 0x55) {
        return;
    }

    if (pEnt->physicsObject.physicsSystem != 0) {
        xHavok_ResyncSystem(pEnt);
        return;
    }

    if (pEnt != 0 && pEnt->collType != 0 && pEnt->ogModel.IsValid() &&
        pEnt->ogModel.data->mModelArt.protoEnt != 0 && pEnt->collisionOn) {
        World::ModelPrototypeEntity* modelProto = pEnt->ogModel.data->mModelArt.protoEnt;

        if (modelProto != 0) {
            World::CollisionMeshBlobEntity* collMesh = modelProto->collmeshBlob;

            if (collMesh != 0) {
                xMat4x3* pMat = &pEnt->ogModel.data->Mat;
                hkVector4Init scale(pMat->left.length(), pMat->up.length(), pMat->at.length());

                if (scale.dot3(scale) > 0.0f) {
                    if (!xHavok_CreateSystem(pEnt)) {
                        pEnt->collisionOn = 0;
                        return;
                    }

                    __ct__Q24Math6VectorFfff(&pEnt->pRigidBodyPostScale, scale.x, scale.y,
                                             scale.z);
                    pEnt->physicsObject.SetOwner(pEnt);
                    xHavok_AddToSimWorld(pEnt->physicsObject.physicsSystem);

                    if (pEnt->asset->physicsData.mass != 0.0f) {
                        for (unsigned int c = 0; c < pEnt->physicsObject.GetNumRigidBodies(); c++) {
                            new (Memory::AllocGlobalHeap(sizeof(EntCollisionListener),
                                                         Memory::GlobalHeap, (eMemMgrTag)42,
                                                         false))
                                EntCollisionListener(pEnt->physicsObject.GetRigidBody(c), 0.25f,
                                                     0.25f);
                        }
                    }
                }
            }
        }
    }
}

#pragma pop

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

// One shape's body, placed by an engine matrix.
bool xHavokPhysicsObject::CreateFromCollisionShape(const hkpShape* shape,
                                                   const hkVector4& shapeScale,
                                                   const xMat4x3& worldTransformMat,
                                                   unsigned int collisionFilter,
                                                   hkpMotion::MotionType motionType,
                                                   float mass, float friction, float elasticity,
                                                   float linearDamping, float angularDamping,
                                                   hkpCollidableQualityType qualityType,
                                                   unsigned long long* id) {
    Math::Matrix43 mathmat;
    xMat4x3ToNGMatrix(&mathmat, &worldTransformMat);

    hkTransform transform;
    ConvertGraphicsTransformToHKTransform(mathmat, transform);

    return CreateFromCollisionShape(shape, shapeScale, transform, collisionFilter, motionType,
                                    mass, friction, elasticity, linearDamping, angularDamping,
                                    qualityType, id);
}

bool xHavokPhysicsObject::CreateFromCollisionShape(const hkpShape* shape,
                                                   const hkVector4& shapeScale,
                                                   const hkTransform& worldTransform,
                                                   unsigned int collisionFilter,
                                                   hkpMotion::MotionType motionType,
                                                   float mass, float friction, float elasticity,
                                                   float linearDamping, float angularDamping,
                                                   hkpCollidableQualityType qualityType,
                                                   unsigned long long* id) {
    creationScale = shapeScale;

    physicsSystem = new hkpPhysicsSystem();

    bool success = AddRigidBodyToSystem(shape, shapeScale, worldTransform, collisionFilter,
                                        motionType, mass, friction, elasticity, linearDamping,
                                        angularDamping, qualityType, id);

    if (success) {
        physicsObjectType = SHAPE;
    }

    return success;
}

// One body per shape, each with the default dampings; the system is a
// multi-shape one when there is more than one.
bool xHavokPhysicsObject::CreateFromCollisionShapes(
    const hkArray<const hkpShape*>& shapes, const hkArray<hkVector4>& shapeScales,
    const hkArray<hkTransform>& worldTransforms, const hkArray<unsigned int>& collisionFilters,
    const hkArray<hkpMotion::MotionType>& motionTypes, const hkArray<float>& masses,
    const hkArray<float>& frictions, const hkArray<float>& elasticities,
    const hkArray<hkpCollidableQualityType>& qualityTypes, unsigned long long* id) {
    creationScale.setAll3(1.0f);

    physicsSystem = new hkpPhysicsSystem();

    unsigned int numShapes = shapes.getSize();
    bool success = true;

    for (unsigned int c = 0; c < numShapes; c++) {
        float mass = masses[c];
        float friction = frictions[c];
        float elasticity = elasticities[c];

        success &= AddRigidBodyToSystem(shapes[c], shapeScales[c], worldTransforms[c],
                                        collisionFilters[c], motionTypes[c], mass, friction,
                                        elasticity, 0.0f, 0.05f, qualityTypes[c], id);
    }

    if (shapes.getSize() > 1) {
        physicsObjectType = MULTI_SHAPE;
    } else {
        physicsObjectType = SHAPE;
    }

    return success;
}

// One body for a shape, into the system. A mass within 1e-5 of zero becomes
// one unless the body is fixed or keyframed. Retail tests the id first and
// holds the same test on both sides of it, so the source does too. Retail
// has it first in the file; defined above CreateFromCollisionShapes it is
// taken in line there, so it is defined below its callers.
bool xHavokPhysicsObject::AddRigidBodyToSystem(const hkpShape* shape,
                                               const hkVector4& shapeScale,
                                               const hkTransform& worldTransform,
                                               unsigned int collisionFilter,
                                               hkpMotion::MotionType motionType, float mass,
                                               float friction, float elasticity,
                                               float linearDamping, float angularDamping,
                                               hkpCollidableQualityType qualityType,
                                               unsigned long long* id) {
    if (id != 0) {
        if (mass >= -1e-5f && mass <= 1e-5f && motionType != hkpMotion::MOTION_FIXED &&
            motionType != hkpMotion::MOTION_KEYFRAMED) {
            mass = 1.0f;
        }
    } else {
        if (mass >= -1e-5f && mass <= 1e-5f && motionType != hkpMotion::MOTION_FIXED &&
            motionType != hkpMotion::MOTION_KEYFRAMED) {
            mass = 1.0f;
        }
    }

    hkQuaternion rotation(worldTransform.getRotation());
    hkpRigidBodyCinfo info;

    info.m_shape = hkSingleton<CHavokShapeBuilder>::getInstance().getShape(shape, shapeScale);
    hkpInertiaTensorComputer::setShapeVolumeMassProperties(info.m_shape, mass, info);

    info.m_mass = mass;
    info.m_linearDamping = linearDamping;
    info.m_angularDamping = angularDamping;
    info.m_friction = friction;
    info.m_restitution = elasticity;
    info.m_collisionFilterInfo = collisionFilter;
    info.m_position = worldTransform.getTranslation();
    info.m_rotation.m_vec = rotation.m_vec;
    info.m_motionType = motionType;
    info.m_qualityType = qualityType;
    info.m_numUserDatasInContactPointProperties = 1;

    hkpRigidBody* body = new hkpRigidBody(info);
    physicsSystem->addRigidBody(body);
    body->removeReference();

    return true;
}

// With always_inline off, the ragdoll data's in-class setters are called out
// of line, where retail has them in line.
#pragma push
#pragma always_inline on

// NEAR MISS, 161 of 311 words. Retail tests the id in the default case
// (cmpwi r26,0) and the twist limit's tau factor against the cone limit's
// (fcmpu), keeping neither result; ours drops both tests, so the id is dead
// and every saved register after it is one lower. Retail dispatches the type
// through a 20-entry jump table; ours stays a compare tree with every value
// from 0 to 19 listed. Tried: always_inline over the function (243 of 307 to
// 236 of 308); the vectors of ones as temporaries, every type listed, and an
// empty inline call inside each unused test (to 161 of 311).
// A constraint's data rebuilt for bodies scaled by scaleA and scaleB: the
// pivots scaled, and under a non-uniform scale the axes scaled and
// renormalized. Unscaled bodies, unless a copy is forced, and a type it does
// not rebuild, get the data back with a reference added. Retail's hinge case
// builds new data and returns the old; the source does the same.
hkpConstraintData* xHavokPhysicsObject::ScaleConstraintBodyAttachSpace(
    hkpConstraintData* constraintData, const hkVector4& scaleA, const hkVector4& scaleB,
    unsigned long long* id, bool forceCopy) {
    if (scaleA.equals3(Math::Vector4().Assign(1.0f, 1.0f, 1.0f, 0.0f), 0.001f) &&
        scaleB.equals3(Math::Vector4().Assign(1.0f, 1.0f, 1.0f, 0.0f), 0.001f) && !forceCopy) {
        constraintData->addReference();
        return constraintData;
    }

    hkpConstraintData::ConstraintInfo info;
    constraintData->getConstraintInfo(info);

    switch (constraintData->getType()) {
    case hkpConstraintData::CONSTRAINT_TYPE_RAGDOLL: {
        hkpRagdollConstraintData* oldData = static_cast<hkpRagdollConstraintData*>(constraintData);
        hkpSetLocalTransformsConstraintAtom* transforms =
            static_cast<hkpSetLocalTransformsConstraintAtom*>(info.m_atoms);
        const hkTransform& transformA = transforms->m_transformA;
        const hkTransform& transformB = transforms->m_transformB;

        hkVector4 pivotA = transformA.getTranslation();
        hkVector4 pivotB = transformB.getTranslation();
        pivotA.mul4(scaleA);
        pivotB.mul4(scaleB);

        hkVector4 planeAxisA = transformA.getRotation().getColumn(1);
        hkVector4 planeAxisB = transformB.getRotation().getColumn(1);
        hkVector4 twistAxisA = transformA.getRotation().getColumn(0);
        hkVector4 twistAxisB = transformB.getRotation().getColumn(0);

        if (scaleA.x != scaleA.y || scaleA.y != scaleA.z) {
            planeAxisA.mul4(scaleA);
            planeAxisA.normalize3();
            twistAxisA.mul4(scaleA);
            twistAxisA.normalize3();
        }

        if (scaleB.x != scaleB.y || scaleB.y != scaleB.z) {
            planeAxisB.mul4(scaleB);
            planeAxisB.normalize3();
            twistAxisB.mul4(scaleB);
            twistAxisB.normalize3();
        }

        hkpRagdollConstraintData* newData = new hkpRagdollConstraintData();
        newData->setInBodySpace(pivotA, pivotB, planeAxisA, planeAxisB, twistAxisA, twistAxisB);
        newData->setMaxFrictionTorque(oldData->getMaxFrictionTorque());
        newData->setAngularLimitsTauFactor(oldData->getAngularLimitsTauFactor());
        newData->setTwistMinAngularLimit(oldData->getTwistMinAngularLimit());
        newData->setTwistMaxAngularLimit(oldData->getTwistMaxAngularLimit());
        newData->setPlaneMinAngularLimit(oldData->getPlaneMinAngularLimit());
        newData->setPlaneMaxAngularLimit(oldData->getPlaneMaxAngularLimit());
        newData->setConeAngularLimit(oldData->getConeAngularLimit());
        newData->setConeLimitStabilization(oldData->getConeLimitStabilization());
        newData->setMaxFrictionTorque(oldData->getMaxFrictionTorque());
        newData->setTwistMotor(oldData->getTwistMotor());
        newData->setConeMotor(oldData->getConeMotor());
        newData->setPlaneMotor(oldData->getPlaneMotor());

        hkMatrix3 target;
        oldData->getTarget(target);
        newData->setTarget(target);

        return newData;
    }

    case hkpConstraintData::CONSTRAINT_TYPE_STIFFSPRING: {
        hkpStiffSpringConstraintData* oldData =
            static_cast<hkpStiffSpringConstraintData*>(constraintData);
        hkpSetLocalTranslationsConstraintAtom* translations =
            static_cast<hkpSetLocalTranslationsConstraintAtom*>(info.m_atoms);

        hkVector4 pivotA = translations->m_translationA;
        hkVector4 pivotB = translations->m_translationB;
        pivotA.mul4(scaleA);
        pivotB.mul4(scaleB);

        hkpStiffSpringConstraintData* newData = new hkpStiffSpringConstraintData();
        newData->setInBodySpace(pivotA, pivotB, oldData->getSpringLength());

        return newData;
    }

    case hkpConstraintData::CONSTRAINT_TYPE_HINGE: {
        hkpSetLocalTransformsConstraintAtom* transforms =
            static_cast<hkpSetLocalTransformsConstraintAtom*>(info.m_atoms);
        const hkTransform& transformA = transforms->m_transformA;
        const hkTransform& transformB = transforms->m_transformB;

        hkVector4 pivotA = transformA.getTranslation();
        hkVector4 pivotB = transformB.getTranslation();
        pivotA.mul4(scaleA);
        pivotB.mul4(scaleB);

        hkVector4 axisA = transformA.getRotation().getColumn(0);
        hkVector4 axisB = transformB.getRotation().getColumn(0);

        if (scaleA.x != scaleA.y || scaleA.y != scaleA.z) {
            axisA.mul4(scaleA);
        }
        axisA.normalize3();

        if (scaleB.x != scaleB.y || scaleB.y != scaleB.z) {
            axisB.mul4(scaleB);
        }
        axisB.normalize3();

        hkpHingeConstraintData* newData = new hkpHingeConstraintData();
        newData->setInBodySpace(pivotA, pivotB, axisA, axisB);
        break;
    }

    // Retail's table covers every type from 0 to 19; the ones it does not
    // rebuild share the default.
    case 0:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
    default:
        // Retail tests the id here and keeps nothing of it.
        if (id != 0) {
            DebugCheck();
        }

        constraintData->addReference();
        return constraintData;
    }

    return constraintData;
}

#pragma pop

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

// NEAR MISS, 185 of 236 words: words 0-51 match. From the first load of the
// system case the nonvolatile registers differ (retail: the system r25, the
// joint count r24, the body count r21, the skins r20, i r19, bodies placed
// r18; ours holds one more counter and places the bodies in r20), the stream
// runs a word long, and the renderable loop opens with a zero-trip guard
// retail lacks. Tried: the type as an if/else-if (219 of 235); i declared
// before the placed-bodies counter with the index advanced at the foot of
// the renderable loop (186); i declared with the relative transforms and a
// while loop (195).
// A shape's body goes where the model's root is; a packed system's bodies
// follow the skinned joints they were created against, or, for a model
// without skin joints, the renderables they are associated with. Keyframed
// bodies in a keyframed match are driven there instead of placed.
void xHavokPhysicsObject::MatchPhysicsToRenderedModel(Graphics::Model& model,
                                                      PhysicsRenderableMatchType matchType,
                                                      JointTransformSpace jointSpace) {
    switch (physicsObjectType) {
    case SHAPE:
    case MULTI_SHAPE: {
        hkpRigidBody* body = GetRigidBody(0);

        if (body != 0) {
            hkTransform trans;
            ConvertGraphicsTransformToHKTransform(model.rootTransform, trans);

            if (matchType == KEYFRAMED && body->getMotionType() == hkpMotion::MOTION_KEYFRAMED) {
                hkQuaternion rotation(trans.getRotation());
                xHavok_UpdateRigidBodyMotion(body, trans.getTranslation(), rotation, Globals::dt);
            } else {
                body->setTransform(trans);
            }
        }
        break;
    }

    case SYSTEM: {
        hkpPhysicsSystem* packedPhysicsSystem = packedPhysicsData->GetPhysicsSystem(0, 0);
        Graphics::Skeleton* skeleton = model.modelProto->skeleton;
        const hkArray<hkpRigidBody*>& bodies = physicsSystem->getRigidBodies();
        int numBodies = bodies.getSize();

        if (skeleton != 0 && skeleton->skinJointTotal != 0) {
            int numJoints = skeleton->skinJointTotal;
            Graphics::SkinCluster* skins = skeleton->skins;
            Math::Matrix43* jointMatrices = model.joints->data;
            bool jointsUpdated = model.joints->updateFrame == Globals::updateFrameNumber;
            PhysicsJointRelativeTransforms* relativeTransforms = 0;

            jointRelativeCreationMap->get(packedPhysicsData, &relativeTransforms);

            int bodyCount = 0;

            for (int i = 0; i < numJoints && bodyCount < numBodies; i++) {
                RelativeJointEntry& entry = (*relativeTransforms->bodyRelativeJointTransforms)[i];
                int bodyIndex = entry.rigidBodyIndex;

                if (bodyIndex >= 0) {
                    Math::Matrix43 jointMat;

                    if (jointsUpdated) {
                        if (jointSpace == MODEL_INV_BIND_SPACE) {
                            Math::Matrix43 bindMat(skins->invBindMat[i]);
                            Math::Invert(bindMat, bindMat, (Math::HintOrthogonalEnum)0);
                            Math::Mul(jointMat, jointMatrices[i], bindMat);
                        } else {
                            jointMat = jointMatrices[i];
                        }
                    } else {
                        Math::Matrix43 bindMat(skins->invBindMat[i]);
                        Math::Invert(bindMat, bindMat, (Math::HintOrthogonalEnum)0);
                        jointMat = bindMat;
                    }

                    Math::Matrix43 worldMat;
                    Math::Mul(worldMat, model.rootTransform, jointMat);

                    Math::Matrix43 invRelative(entry.mat);
                    Math::Invert(invRelative, invRelative, (Math::HintOrthogonalEnum)0);

                    Math::Matrix43 bodyMat;
                    Math::Mul(bodyMat, worldMat, invRelative);

                    hkTransform bodyTrans;
                    ConvertGraphicsTransformToHKTransform(bodyMat, bodyTrans);

                    if (matchType == KEYFRAMED &&
                        bodies[bodyIndex]->getMotionType() == hkpMotion::MOTION_KEYFRAMED) {
                        hkQuaternion rotation(bodyTrans.getRotation());
                        xHavok_UpdateRigidBodyMotion(bodies[bodyIndex], bodyTrans.getTranslation(),
                                                     rotation, Globals::dt);
                    } else {
                        bodies[bodyIndex]->setTransform(bodyTrans);
                    }

                    bodyCount++;
                }
            }
        } else {
            hkLocalArray<Math::Matrix43> childTransforms(model.childTransformCount + 1);

            model.CalcWorldChildTransforms(childTransforms.expandBy(model.childTransformCount + 1));

            unsigned int index = 0;

            for (hkpRigidBody** it = bodies.begin(); it != bodies.end(); it++, index++) {
                hkpRigidBody* body = *it;
                unsigned int renderableIndex;

                GetGraphicsAssociationDataFromRigidBody(packedPhysicsSystem->getRigidBodies()[index],
                                                        renderableIndex);

                hkTransform childTrans;
                ConvertGraphicsTransformToHKTransform(
                    childTransforms[model.renderableTransformMap[renderableIndex]], childTrans);

                SetFlattenedHKTransform(body, childTrans, matchType);
            }
        }
        break;
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

// With hkVector4's own constructors declared, hkVector4Init's is taken in
// line below only with always_inline on (mwcc takes the pragma state at the
// start of the function after the one it compiles).
#pragma push
#pragma always_inline on

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

#pragma pop

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

bool xHavok_TestRayAndCollectInSimWorld(const hkVector4& from, const hkVector4& to,
                                        hkpAllRayHitCollector& collector);

// NEAR MISS, 92 of 172 words: retail computes `!enabled` as a value (cntlzw
// and srwi on the hkBool's byte) and merges the || into r3; ours holds a flag
// in r29, which also swaps the registers of i and `ignored`. Tried: `== false`
// through hkBool::operator==(bool) (95 of 172); the hit default-constructed
// and assigned (182); the condition held in a bool first, with that
// assignment (182). Without hkVector4's copy constructor the hit's normal is
// copied as four words (132).
// The nearest hit along the ray whose body the world's filter lets collide
// with filterInfo and that lacks property 7777: where it is, how far from the
// start, and its owner. A hit on an ignored object clears the owner and the
// search goes on.
bool HavokRayCastStopFilterCollide(const hkVector4& from, const hkVector4& to,
                                   hkVector4& hitPoint, float& hitDistance, void** hitObject,
                                   const hkBaseObject** ignoreList, int numIgnore,
                                   unsigned int filterInfo) {
    hkpAllRayHitCollector collector;

    if (xHavok_TestRayAndCollectInSimWorld(from, to, collector)) {
        collector.sortHits();

        for (int i = 0; i < collector.getHits().getSize(); i++) {
            bool ignored = false;
            hkpWorldRayCastOutput hit = collector.getHits()[i];
            *hitObject = hit.m_rootCollidable->getOwner();

            const hkpGroupFilter* filter =
                static_cast<const hkpGroupFilter*>(xHavok_GetWorld()->getCollisionFilter());

            if (!filter->isCollisionEnabled(hit.m_rootCollidable->getCollisionFilterInfo(),
                                            filterInfo) ||
                static_cast<hkpWorldObject*>(hit.m_rootCollidable->getOwner())
                    ->hasProperty(7777)) {
                continue;
            }
            for (int j = 0; j < numIgnore; j++) {
                if (*hitObject == ignoreList[j]) {
                    ignored = true;
                }
            }

            if (!ignored) {
                hitPoint.setInterpolate4(from, to, hit.m_hitFraction);

                hkVector4 diff;
                diff = hitPoint;
                diff.sub4(from);
                hitDistance = diff.length3();

                return true;
            } else {
                *hitObject = 0;
            }
        }
    }

    return false;
}

// ConstructVector is taken in line at the model's position only with
// always_inline on; the region closes one function later, as Cleanup's does.
#pragma push
#pragma always_inline on

// The NPC's model goes where its controller's body is and its frame takes the
// body's velocity; then whether it stands on anything, a proxy probing along
// the model's down with the model lowered 0.05, a rigid body stepping 0.05.
bool xHavok_SetNPCFromCharacterProxyMotion(zNPCEntity* npc,
                                           xHavokCharacterController* controller) {
    xEntFrame* frame = npc->frame;
    hkVector4 position;
    hkVector4 velocity;

    switch (controller->controllerType) {
    case PROXY:
        position = controller->characterProxy->getPosition();
        break;
    case RIGID_BODY:
        position = controller->characterRigidBody->getPosition();
        break;
    }

    switch (controller->controllerType) {
    case PROXY:
        velocity = controller->characterProxy->getLinearVelocity();
        break;
    case RIGID_BODY:
        velocity = controller->characterRigidBody->getLinearVelocity();
        break;
    }

    hkpSurfaceInfo ground;
    hkVector4 up;
    hkVector4 down;

    // Called, not built through hkVector4Init: retail loads the components
    // x, y, z, a direct call's order (an inline's arguments load z, y, x).
    ((Math::Vector4*)&up)->Assign(npc->ogModel.data->Mat.up.x, npc->ogModel.data->Mat.up.y,
                                  npc->ogModel.data->Mat.up.z, 0.0f);
    ((Math::Vector4*)&down)->Assign(-npc->ogModel.data->Mat.up.x,
                                    -npc->ogModel.data->Mat.up.y,
                                    -npc->ogModel.data->Mat.up.z, 0.0f);

    ConstructVector(&npc->ogModel.data->Mat.pos, position.x, position.y, position.z);
    ConstructVector(&frame->vel, velocity.x, velocity.y, velocity.z);

    hkpCharacterProxy* proxy = controller->GetCharacterProxy();

    if (proxy != 0) {
        npc->ogModel.data->Mat.pos.y -= 0.05f;
        proxy->checkSupport(down, ground);
    } else {
        hkStepInfo stepInfo;
        stepInfo.set(0.0f, 0.05f);
        controller->GetCharacterRigidBody()->checkSupport(stepInfo, ground);
    }

    if (ground.m_supportedState == hkpSurfaceInfo::SUPPORTED) {
        return true;
    } else if (ground.m_supportedState == hkpSurfaceInfo::SLIDING) {
        return true;
    } else if (ground.m_supportedState == hkpSurfaceInfo::UNSUPPORTED) {
        return false;
    }

    return true;
}

void xHavok_SetFrameFromCharacterProxy(xMat4x3* mat, hkpCharacterProxy* pCharacterProxy) {
    const hkVector4& proxy_pos = pCharacterProxy->getPosition();

    ConstructVector(&mat->pos, proxy_pos.x, proxy_pos.y, proxy_pos.z);
}

#pragma pop

void xHavok_SetFrameFromCharacterProxy(xMat4x3* mat, xVec3* vel,
                                       hkpCharacterRigidBody* pCharacterProxy) {
    const hkVector4& proxy_pos = pCharacterProxy->getPosition();

    ConstructVector(&mat->pos, proxy_pos.x, proxy_pos.y, proxy_pos.z);

    const hkVector4& proxy_vel = pCharacterProxy->getLinearVelocity();

    ConstructVector(vel, proxy_vel.x, proxy_vel.y, proxy_vel.z);
}

// Whether the proxy stands on anything below it, and the surface's normal.
// Without a collector of the caller's, a trigger-identifying one of its own.
bool xHavok_CheckGroundSupportFromCharacterProxy(hkpCharacterProxy* pCharacterProxy,
                                                 hkpAllCdPointCollector* pCollector,
                                                 xVec3* pNormal) {
    hkpSurfaceInfo ground;
    hkVector4Init down(0.0f, -1.0f, 0.0f);

    if (pCollector != 0) {
        pCharacterProxy->checkSupportWithCollector(down, ground, *pCollector);
    } else {
        TriggerIdentifyingPointCollector collector;
        pCharacterProxy->checkSupportWithCollector(down, ground, collector);
    }

    if (pNormal != 0) {
        ConstructVector(pNormal, ground.m_surfaceNormal.x, ground.m_surfaceNormal.y,
                        ground.m_surfaceNormal.z);
    }

    return ground.m_supportedState != hkpSurfaceInfo::UNSUPPORTED;
}

// The same for a character rigid body, over one frame's step. The down
// vector is built and not passed.
bool xHavok_CheckGroundSupportFromCharacterRigidBody(hkpCharacterRigidBody* pCharacterRigidBody,
                                                     hkpAllCdPointCollector* pCollector,
                                                     xVec3* pNormal) {
    hkpSurfaceInfo ground;
    hkVector4Init down(0.0f, -1.0f, 0.0f);

    if (pCollector != 0) {
        hkStepInfo stepInfo;
        stepInfo.set(0.0f, Globals::dt);
        pCharacterRigidBody->checkSupport(stepInfo, ground, pCollector);
    } else {
        TriggerIdentifyingPointCollector collector;
        hkStepInfo stepInfo;
        stepInfo.set(0.0f, Globals::dt);
        pCharacterRigidBody->checkSupport(stepInfo, ground, &collector);
    }

    if (pNormal != 0) {
        ConstructVector(pNormal, ground.m_surfaceNormal.x, ground.m_surfaceNormal.y,
                        ground.m_surfaceNormal.z);
    }

    return ground.m_supportedState != hkpSurfaceInfo::UNSUPPORTED;
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

// ---------------------------------------------------------------------------
// Weak copies above the always_inline region: each calls
// deallocateChunkConstSize, or a destructor defined after it, out of line.

inline hkpSurfaceInfo::hkpSurfaceInfo() : m_supportedState(SUPPORTED) {
    ((Math::Vector4*)&m_surfaceNormal)->Assign(0.0f, 0.0f, 1.0f, 0.0f);
    ((Math::Vector4*)&m_surfaceVelocity)->Assign(0.0f, 0.0f, 0.0f, 0.0f);
    m_surfaceDistance = 0.0f;
    m_surfaceMotionType = hkpMotion::MOTION_FIXED;
}

inline hkpCharacterProxy* xHavokCharacterController::GetCharacterProxy() const {
    return controllerType == PROXY ? characterProxy : 0;
}

inline void hkStepInfo::set(float startTime, float endTime) {
    m_startTime = startTime;
    m_endTime = endTime;
    m_deltaTime = endTime - startTime;
    m_invDeltaTime = m_deltaTime == 0.0f ? 0.0f : 1.0f / m_deltaTime;
}

inline TriggerIdentifyingPointCollector::~TriggerIdentifyingPointCollector() {}

// One word from retail's: its branch to the hits array's destructor names
// hkArray<Math::Matrix43>'s, onto which the linker folded
// hkArray<hkpRootCdPoint>'s (both elements 48 bytes).
inline hkpAllCdPointCollector::~hkpAllCdPointCollector() {}

inline hkpAllRayHitCollector::~hkpAllRayHitCollector() {}

inline bool hkpWorldObject::hasProperty(unsigned int key, MtChecks mtCheck) const {
    for (int i = 0; i < m_properties.getSize(); ++i) {
        if (m_properties[i].m_key == key) {
            return true;
        }
    }

    return false;
}

inline void hkVector4::sub4(const hkVector4& a) {
    x -= a.x;
    y -= a.y;
    z -= a.z;
    w -= a.w;
}

template <class T>
inline void hkDeallocateChunk(T* ptr, int nelem, HK_MEMORY_CLASS mclass) {
    hkThreadMemory::getInstance().deallocateChunk(ptr, nelem * sizeof(T), mclass);
}

template <class T>
inline hkArray<T>::~hkArray() {
    if ((m_capacityAndFlags & DONT_DEALLOCATE_FLAG) == 0) {
        hkDeallocateChunk<T>(m_data, getCapacity(), HK_MEMORY_CLASS_ARRAY);
    }
}

template <class T>
inline T* hkAllocateStack(int n) {
    return (T*)hkThreadMemory::getInstance().allocateStack(n * sizeof(T));
}

template <class T>
inline void hkDeallocateStack(T* p) {
    hkThreadMemory::getInstance().deallocateStack(p);
}

// NEAR MISS for Math::Matrix43, 8 of 39 words: retail rounds the size into
// the register that held capacity * 48 (r5) and loads the stack's current
// pointer into r6; ours rounds into r7 and loads into r5. Tried: the size
// rounded in allocateStack's own parameter (13 of 39); allocateStack called
// here without hkAllocateStack (8 of 39).
template <class T>
inline hkLocalArray<T>::hkLocalArray(int capacity) {
    this->m_data = hkAllocateStack<T>(capacity);
    this->m_capacityAndFlags = capacity | hkArray<T>::DONT_DEALLOCATE_FLAG;
    m_localMemory = this->m_data;
}

template <class T>
inline hkLocalArray<T>::~hkLocalArray() {
    hkDeallocateStack<T>(m_localMemory);
}

template <class T>
inline void hkArray<T>::setSize(int n) {
    int cap = getCapacity();

    if (cap < n) {
        int cap2 = 2 * cap;
        int newSize = (n < cap2) ? cap2 : n;
        hkArrayUtil::_reserve(this, newSize, sizeof(T));
    }

    m_size = n;
}

template <class T>
inline T* hkArray<T>::expandBy(int n) {
    int oldsize = m_size;
    setSize(oldsize + n);
    return m_data + oldsize;
}

// ---------------------------------------------------------------------------
// Math's weak copies: Invert calls each of the others out of line, so it
// comes first.

inline void Math::Invert(Matrix43& o, const Matrix43& a, HintOrthogonalEnum hint) {
    Vector recipScaleSqr = a.GetRecipScaleSqr();
    Vector pos = a.GetRowInternal(3);

    Transpose(o, a);

    Matrix33 scale;
    scale.MakeScale(recipScaleSqr);
    Mul(o, o, scale);

    PSMTXMultVecSR(&o, &pos, &pos);

    Vector negPos;
    Negate(negPos, pos);
    o.SetPos(negPos);
}

// Matrix33's assignment is Matrix43's 48 bytes, and retail branches to
// Matrix43's name: the copy goes through Matrix43 references.
inline void Math::Transpose(Matrix33& o, const Matrix33& a) {
    (Matrix43&)o = (const Matrix43&)a;

    // Each pair's two values named in the order they are loaded.
    float f10 = a.v[1][0];
    float f01 = a.v[0][1];
    o.v[1][0] = f01;
    o.v[0][1] = f10;

    float f20 = a.v[2][0];
    float f02 = a.v[0][2];
    o.v[2][0] = f02;
    o.v[0][2] = f20;

    float f21 = a.v[2][1];
    float f12 = a.v[1][2];
    o.v[2][1] = f12;
    o.v[1][2] = f21;
}

inline void Math::Matrix33::MakeScale(const Vector& scale) {
    float z = scale.data.z;
    float y = scale.data.y;
    float x = scale.data.x;
    (Matrix43&)*this = _matZero;

    v[0][0] = x;
    v[1][1] = y;
    v[2][2] = z;
}

// NEAR MISS, 89 differing words (retail 90): retail spills nine inputs into
// f23-f31 and computes the middle products p1, p2, p4, p3 first; ours spills
// fewer and starts from p9's. Tried: the products spelled b[i][k] * a[k][j]
// (89 of 86); the nine entries named, then Assign (89 of 86); both (89 of 86).
inline void Math::Mul(Matrix33& o, const Matrix33& a, const Matrix33& b) {
    o.Assign(a.v[0][0] * b.v[0][0] + a.v[1][0] * b.v[0][1] + a.v[2][0] * b.v[0][2],
             a.v[0][0] * b.v[1][0] + a.v[1][0] * b.v[1][1] + a.v[2][0] * b.v[1][2],
             a.v[0][0] * b.v[2][0] + a.v[1][0] * b.v[2][1] + a.v[2][0] * b.v[2][2],
             a.v[0][1] * b.v[0][0] + a.v[1][1] * b.v[0][1] + a.v[2][1] * b.v[0][2],
             a.v[0][1] * b.v[1][0] + a.v[1][1] * b.v[1][1] + a.v[2][1] * b.v[1][2],
             a.v[0][1] * b.v[2][0] + a.v[1][1] * b.v[2][1] + a.v[2][1] * b.v[2][2],
             a.v[0][2] * b.v[0][0] + a.v[1][2] * b.v[0][1] + a.v[2][2] * b.v[0][2],
             a.v[0][2] * b.v[1][0] + a.v[1][2] * b.v[1][1] + a.v[2][2] * b.v[1][2],
             a.v[0][2] * b.v[2][0] + a.v[1][2] * b.v[2][1] + a.v[2][2] * b.v[2][2]);
}

// Stored row by row; Mul passes columns.
inline void Math::Matrix33::Assign(float x0, float y0, float z0, float x1, float y1,
                                   float z1, float x2, float y2, float z2) {
    v[0][0] = x0;
    v[0][1] = x1;
    v[0][2] = x2;
    v[1][0] = y0;
    v[1][1] = y1;
    v[1][2] = y2;
    v[2][0] = z0;
    v[2][1] = z1;
    v[2][2] = z2;
}

void Math::Negate(Vector4& o, const Vector4& a) {
    o[0] = -a[0];
    o[1] = -a[1];
    o[2] = -a[2];
    o[3] = -a[3];
}

void Math::Mul(Vector4& o, const Vector4& a, float s) {
    o.data.x = a.data.x * s;
    o.data.y = a.data.y * s;
    o.data.z = a.data.z * s;
    o.data.w = a.data.w * s;
}

// Retail's copies with no caller here: defined out of line.

// NEAR MISS, 23 of 70 words, stack offsets only: every instruction and the
// frame size are retail's. Retail keeps the first position at sp+8, the
// rotated vector at +24, the first negation at +40, the second position at +56
// and the second negation at +72, with the sums and products above them (+88
// to +152). Ours puts the second negation, the products and the sums lowest
// (+8 to +88) and the first negation, second position, rotated vector and
// first position above them (+104 to +152). Tried: the rotated vector
// initialised from the sum (43 of 62, without retail's copy of the sum);
// declared then assigned, before or after the position (23 of 70 both);
// declared at function scope (23 of 70).
// CheckHint is asked with 0, not the hint. A matrix it answers zero for is
// inverted as a translation; otherwise the rotation is transposed and the
// position taken back through it.
void Math::Matrix43::Invert(HintOrthonormalEnum hint) {
    if (!CheckHint((HintOrthonormalEnum)0)) {
        Vector position = GetRowInternal(3);
        *this = _matIdentity;
        SetPos(-position);
    } else {
        // Assigned, not initialised: retail copies the sum's temporary in.
        Vector rotated;
        Vector position = GetRowInternal(3);
        rotated = v[0] * position.data.x + v[1] * position.data.y + v[2] * position.data.z;
        Transpose(*this, *this);
        SetPos(-rotated);
    }
}

template <class T>
int hkArray<T>::indexOf(const T& t, int start, int end) const {
    if (end < 0) {
        end = m_size;
    }

    for (int i = start; i < end; ++i) {
        if (m_data[i] == t) {
            return i;
        }
    }

    return -1;
}

template int hkArray<hkpRigidBody*>::indexOf(hkpRigidBody* const& t, int start, int end) const;

// ---------------------------------------------------------------------------
// Havok vector copies with no caller here: out of line.

void hkVector4::mul4(const hkVector4& a) {
    x *= a.x;
    y *= a.y;
    z *= a.z;
    w *= a.w;
}

// NEAR MISS, 1 of 25 words: fcmpu compares the literal with the length where
// retail compares the length with the literal. Tried: the literal on the
// right (1 of 25) and on the left (1 of 25); the length through an inline
// lengthSquared3(), which was called out of line (23 of 20).
void hkVector4::normalize3() {
    float lengthSquared = x * x + y * y + z * z;
    float lengthInverse = (lengthSquared != 0.0f) ? hkMath::sqrtInverse(lengthSquared) : 0.0f;
    mul4(lengthInverse);
}

// One Newton step from frsqrte's estimate; the (float) of the intrinsic's
// double is the frsp retail has.
// NEAR MISS, 5 of 11 words: retail loads 1.0 first (f2) and multiplies the
// difference by 0.5 * e; ours loads 0.5 first (f3). Tried: r * e * e (6 of
// 11); r * (e * e) (5); the difference multiplied first (5).
inline float hkMath::sqrtInverse(float r) {
    float e = (float)__frsqrte(r);
    return e + 0.5f * e * (1.0f - r * (e * e));
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

// Here, before deallocateChunkConstSize is defined: always_inline takes
// deallocateStack in line, and cannot take the deallocation retail calls.
template hkLocalArray<Math::Matrix43>::~hkLocalArray();

// No caller here, so out of line; in the region, so FreeList::get is in line
// as retail has it.
void* hkThreadMemory::allocateChunkConstSize(int nbytes, HK_MEMORY_CLASS cl) {
    int row = constSizeToRow(nbytes);
    void* o = m_free_list[row].get();

    if (o) {
        return o;
    }

    return onRowEmpty(row, cl);
}

// In the region, so setAbs4 and the comparison are in line; above setSub4's
// and setAll3's definitions, so those two are called, as retail calls them.
inline bool hkVector4::equals3(const hkVector4& v, float epsilon) const {
    hkVector4 t;
    t.setSub4(*this, v);
    t.setAbs4(t);

    hkVector4 epsilonV;
    epsilonV.setAll3(epsilon);

    return t.compareLessThanEqual4(epsilonV).allAreSet(hkVector4Comparison::MASK_XYZ);
}

inline void hkThreadMemory::deallocateChunkConstSize(void* p, int nbytes,
                                                     HK_MEMORY_CLASS cl) {
    int row = constSizeToRow(nbytes);

    if (m_free_list[row].m_numElem >= m_maxNumElemsOnFreeList) {
        onRowFull(row, p, cl);
    } else {
        m_free_list[row].put(p);
    }
}

// Below both chunk functions, which call it. A size past every row breaks
// into the debugger: HK_BREAKPOINT sets the MSR's single-step bit and clears
// it again.
inline int hkThreadMemory::constSizeToRow(int size) {
    if (size <= 16) {
        return 1;
    }
    if (size <= 32) {
        return 2;
    }
    if (size <= 48) {
        return 3;
    }
    if (size <= 64) {
        return 4;
    }
    if (size <= 96) {
        return 5;
    }
    if (size <= 128) {
        return 6;
    }
    if (size <= 160) {
        return 7;
    }
    if (size <= 192) {
        return 8;
    }
    if (size <= 256) {
        return 9;
    }
    if (size <= 320) {
        return 10;
    }
    if (size <= 512) {
        return 11;
    }
    if (size <= 544) {
        return 12;
    }
    if (size <= 1024) {
        return 13;
    }
    if (size <= 2048) {
        return 14;
    }
    if (size <= 4096) {
        return 15;
    }
    if (size <= 8192) {
        return 16;
    }

    asm { mfmsr r0 }
    asm { ori r3, r0, 0x400 }
    asm { mtmsr r3 }
    asm { mtmsr r0 }

    return -1;
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

inline void hkVector4::setAll3(float v) {
    x = v;
    y = v;
    z = v;
    w = v;
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

inline Math::Vector4& Math::Vector4::Assign(float x, float y, float z, float w) {
    data.x = x;
    data.y = y;
    data.z = z;
    data.w = w;
    return *this;
}

// Instantiated here, under always_inline: generated at the end of the file,
// where it is off, it calls allocateStack and hkArray's constructor out of
// line, which retail has in line. The destructor is instantiated at the top
// of the region.
template hkLocalArray<Math::Matrix43>::hkLocalArray(int capacity);

#pragma pop

// Weak copies retail calls rather than inlines: those AddToHavokSimWorld
// reaches and the rest of the unit's. dont_inline keeps Matrix43's operator=,
// defined above, a call as retail has it.
#pragma push
#pragma dont_inline on

inline float xVec3::length2() const { return x * x + y * y + z * z; }

// Zero, a negative and anything past the largest float come back as they
// are; otherwise one Newton step from frsqrte's estimate, in LinkFastSqrt's
// form.
inline float Math::rsqrt(register float x) {
    if (x <= 0.0f || x > 3.40282347e+38f) {
        return x;
    }

    register float h;
    register float e;
    register float t;
    register float half = 0.5f;

    asm { frsqrte e, x }
    h = half * x;
    t = e * e;
    asm { fnmsubs t, t, h, half }
    e = e * t + e;

    return e;
}

inline float hkVector4::dot3(const hkVector4& v) const {
    return x * v.x + y * v.y + z * v.z;
}

// Stored, and marked dirty, only when it differs.
inline void Graphics::Model::SetRootTransform(const Math::Matrix43& transform) {
    register float epsilon = 1e-5f;
    bool equal = true;

    equal &= Math::Equal4(rootTransform.v[0], transform.v[0], epsilon);
    equal &= Math::Equal4(rootTransform.v[1], transform.v[1], epsilon);
    equal &= Math::Equal4(rootTransform.v[2], transform.v[2], epsilon);

    if (!equal) {
        rootTransform = transform;
        XformSetDirty(0);
    }
}

inline void Graphics::Model::SetChildTransform(int index, const Math::Matrix43& transform) {
    childTransforms[index] = transform;
    XformSetDirty(index + 1);
}

// Transform 0 is the root; the prototype's own transforms come before the
// reference models'.
inline void Graphics::Model::XformSetDirty(unsigned int index) {
    unsigned int bit = 1 << (index & 31);

    if ((bit & xformDirtyBits[index >> 5]) != 0) {
        return;
    }

    xformDirtyBits[index >> 5] |= bit;

    if (index == 0) {
        return;
    }

    if (index < modelProto->nonRefTransformCount + 1U) {
        dirtyNonRefChildTransformCount++;
    } else {
        dirtyRefChildTransformCount++;
    }
}

inline bool Math::Equal4(const Vector4& a, const Vector4& b, float epsilon) {
    return feq(a.data.x, b.data.x, epsilon) && feq(a.data.y, b.data.y, epsilon) &&
           feq(a.data.z, b.data.z, epsilon) && feq(a.data.w, b.data.w, epsilon);
}

inline bool Math::feq(float a, float b, float epsilon) {
    return (float)__fabs(a - b) <= epsilon;
}

inline void World::xOGModel::UpdaterSwitch(xOGModelUpdater* newUpdater, void* newParent) {
    if (updater != newUpdater) {
        if (updateNode.prev != 0) {
            updater->updList.Remove(this);
            updateNode.prev = 0;
        }

        updater = newUpdater;
        updateParent = newParent;

        if (mModelArt.model.visibleCount != 0) {
            newUpdater->updList.PushBack(this);
        }
    }
}

template <class T, int nodeOffset>
inline void EmbeddedList<T, nodeOffset>::Remove(T* item) {
    EmbeddedListNode* node = (EmbeddedListNode*)((char*)item + nodeOffset);

    node->prev->next = node->next;
    node->next->prev = node->prev;
    size--;
}

template <class T, int nodeOffset>
inline void EmbeddedList<T, nodeOffset>::PushBack(T* item) {
    EmbeddedListNode* node = (EmbeddedListNode*)((char*)item + nodeOffset);

    node->prev = head.prev;
    node->next = &head;
    head.prev->next = node;
    head.prev = node;
    size++;
}

// NEAR MISS, 12 of 68 words: retail keeps the epsilon in f0 and each
// absolute value in f1, ours the other way round. Tried: the epsilon a local
// before len2 or after it (18 of 68, the epsilon in f4); the absolute values
// through an inline xabs ahead of this region, as xCam.cpp's v3normalizexz
// matched (59 of 68, xabs emitted on its own).
// A length of one or of zero is not divided by: the vector is copied, or
// made the unit y, and the length given as 1 or 0.
inline void v3normalize(float& len, xVec3* out, xVec3* in) {
    float len2 = in->x * in->x + in->y * in->y + in->z * in->z;

    if ((float)__fabs(len2 - 1.0f) <= 1e-5f) {
        out->x = in->x;
        out->y = in->y;
        out->z = in->z;
        len = 1.0f;
    } else if ((float)__fabs(len2) <= 1e-5f) {
        out->y = 1.0f;
        out->x = 0.0f;
        out->z = 0.0f;
        len = 0.0f;
    } else {
        float l = len2 * Math::rsqrt(len2);
        len = l;
        float inv = 1.0f / l;
        out->x = in->x * inv;
        out->y = in->y * inv;
        out->z = in->z * inv;
    }
}

// optimize_for_size off: retail saves r30 and r31 with two stw.
#pragma push
#pragma optimize_for_size off
inline void xMat3x3Normalize(xMat3x3* o, const xMat3x3* m) {
    float leftLength;
    float upLength;
    float atLength;

    v3normalize(leftLength, &o->left, (xVec3*)&m->left);
    v3normalize(upLength, &o->up, (xVec3*)&m->up);
    v3normalize(atLength, &o->at, (xVec3*)&m->at);
}
#pragma pop

// NEAR MISS, 47 of 48 words (ours 52): retail computes the doubled
// components, the squares, the three diagonal terms and then the other
// products, all in f0 to f11; twelve named products need f31 as well. Tried:
// tx, ty and tz named with the products in Assign's arguments (43 of 42, the
// products fused into fnmsubs); nothing named (44 of 42).
inline void Math::Matrix43::MakeQuaternion(const Quaternion& q) {
    float tx = 2.0f * q.v.data.x;
    float ty = 2.0f * q.v.data.y;
    float tz = 2.0f * q.v.data.z;
    float twx = tx * q.v.data.w;
    float twy = ty * q.v.data.w;
    float twz = tz * q.v.data.w;
    float txx = tx * q.v.data.x;
    float txy = ty * q.v.data.x;
    float txz = tz * q.v.data.x;
    float tyy = ty * q.v.data.y;
    float tyz = tz * q.v.data.y;
    float tzz = tz * q.v.data.z;

    Matrix33::Assign(1.0f - tyy - tzz, txy + twz, txz - twy,
                     txy - twz, 1.0f - tzz - txx, tyz + twx,
                     txz + twy, tyz - twx, 1.0f - txx - tyy);
    SetPos(vec4Zero);
}

inline xVec3& xVec3::operator*=(float s) {
    x *= s;
    y *= s;
    z *= s;
    return *this;
}

float xVec3::normalize() {
    float len2 = length2();
    float len = len2 * Math::rsqrt(len2);
    *this *= 1.0f / len;
    return len;
}


// The next frame's joint buffer, once per frame.
Math::Matrix43* Graphics::Model::NextJointMatrices(bool& advanced) {
    unsigned int frame = Globals::updateFrameNumber;

    if (joints->updateFrame != frame) {
        joints = joints->next;
        joints->updateFrame = frame;
        advanced = true;
    } else {
        advanced = false;
    }

    return joints->data;
}

void xVec3::cross(const xVec3& a, const xVec3& b) {
    x = a.y * b.z - b.y * a.z;
    y = a.z * b.x - b.z * a.x;
    z = a.x * b.y - b.x * a.y;
}

void xVec3::Sub(const xVec3& a, const xVec3& b) {
    x = a.x - b.x;
    y = a.y - b.y;
    z = a.z - b.z;
}

void v3add(xVec3* o, xVec3* a, xVec3* b) {
    o->x = a->x + b->x;
    o->y = a->y + b->y;
    o->z = a->z + b->z;
}

float Math::sqrt(float x) { return x * rsqrt(x); }

// Each row scaled by the vector: paired-single code in retail.
asm void Math::Mul(Matrix43& o, MatrixOpScaleEnum op, const Matrix43& a, const Vector& s) {
    nofralloc
    psq_l f0, 0(r6), 0, 0
    psq_l f2, 0(r5), 0, 0
    psq_l f4, 16(r5), 0, 0
    psq_l f5, 32(r5), 0, 0
    ps_mul f2, f2, f0
    ps_mul f4, f4, f0
    psq_l f1, 8(r6), 1, 0
    ps_mul f5, f5, f0
    psq_l f3, 8(r5), 0, 0
    psq_l f0, 24(r5), 0, 0
    psq_l f6, 40(r5), 0, 0
    ps_mul f3, f3, f1
    psq_st f2, 0(r3), 0, 0
    ps_mul f0, f0, f1
    ps_mul f6, f6, f1
    psq_st f3, 8(r3), 0, 0
    psq_st f4, 16(r3), 0, 0
    psq_st f0, 24(r3), 0, 0
    psq_st f5, 32(r3), 0, 0
    psq_st f6, 40(r3), 0, 0
    blr
}

// A file static of the unity build (symbols.txt: scope:local).
static void xMat3x3RMulVec(xVec3* o, const xMat3x3* m, const xVec3* v) {
    float x = m->left.x * v->x + m->up.x * v->y + m->at.x * v->z;
    float y = m->left.y * v->x + m->up.y * v->y + m->at.y * v->z;
    float z = m->left.z * v->x + m->up.z * v->y + m->at.z * v->z;

    o->x = x;
    o->y = y;
    o->z = z;
}

#pragma pop

// With dont_inline off, so that the base's reset is in line.
void hkpCdPointCollector::reset() { m_earlyOutDistance = 3.40282e+38f; }

void hkpAllCdPointCollector::reset() {
    m_hits.clear();
    hkpCdPointCollector::reset();
}

// NEAR MISS, 20 of 28 words: the in-place array's constructor, a class
// template's member, is called out of line (and emitted, an EXTRA row) where
// retail has it in line. Tried: -inline auto; always_inline pushed straight
// before this constructor, and two functions before it; the array's
// constructor instantiated explicitly ahead of it. All four gave these 20.
hkpAllCdPointCollector::hkpAllCdPointCollector() { reset(); }

// The owner of an entity's collidable.
hkpRigidBody* hkGetRigidBody(const hkpCollidable* collidable) {
    if (collidable->m_type == 1) {
        return (hkpRigidBody*)collidable->getOwner();
    }

    return 0;
}

hkBaseObject::~hkBaseObject() {}

hkReferencedObject::~hkReferencedObject() {}

// ---------------------------------------------------------------------------
// The generated accessors

float hkpRigidBody::getRestitution() const { return m_material.m_restitution; }
float hkpRigidBody::getLinearDamping() const { return m_motion.m_linearDamping; }
float hkpRigidBody::getAngularDamping() const { return m_motion.m_angularDamping; }
void hkpRigidBody::setLinearDamping(float value) { m_motion.m_linearDamping = value; }
void hkpRigidBody::setAngularDamping(float value) { m_motion.m_angularDamping = value; }
World::ModelPrototypeEntity* World::xOGModel::GetPrototype() const { return mModelArt.protoEnt; }
