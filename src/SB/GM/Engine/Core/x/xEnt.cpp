#include "SB/GM/Engine/Core/x/xEnt.pool.h"

// xEnt.cpp -- the game layer's entity: init, setup, save and load, the
// reset, the update, the default translate and motion, the drive
// (attach/detach) hooks, the surface lookup and the ragdoll state.
//
// The five accessors at the drive hooks were written by
// tools/gen_accessors.py; the file is hand-owned now and they are spelled
// on the real layout.
//
// Layouts from the Wii build's DWARF: xBase 0x38 (0x34 of data), xOGEntity
// puts its model handle at +0x34 and xEnt its asset at +0x3C, so xBase and
// xOGEntity are packed to four. The vtable pointer is World::Entity's, at
// +0. Retail's tables (vtable.py): xOGEntity has 20 slots, xEffectAttachIntf
// 25, xEnt 32; the slots are declared in that order, by name where this
// unit calls or defines them.
//
// xEnt's vtable lives elsewhere in the image, so the first virtual xEnt
// declares is an override this unit does not define: mwcc emits a class's
// vtable where its first declared virtual is defined (NOTES.md, the
// zNPCBTAction tables).
//
// The object also emits xEffectAttachIntf's destructor (weak): xEnt's
// destructor inlines it and mwcc keeps the out-of-line copy as well, which
// retail's unity build has in an earlier file.

class xEnt;
class xScene;
class xFFX;
class xAnimPlay;
class SurfaceGamePlay;
class RagdollCallbackListener {
public:
    virtual void OnRagdollOn(xEnt* ent);
    virtual void OnRagdollOff(xEnt* ent);
    virtual void OnRagdollTransitionOnStart(xEnt* ent);
    virtual void OnRagdollTransitionOffStart(xEnt* ent);
    virtual void OnRagdollUpdate(xEnt* ent, float& blendAmount);
    virtual void OnRagdollPostUpdate(xEnt* ent);
};
class hkpPhysicsSystem;
class zNeoDrivenLink;

void operator delete(void* mem);

// ---------------------------------------------------------------------------
// Math

class xVec3 {
public:
    xVec3& operator=(const xVec3& other);
    xVec3& operator+=(const xVec3& other);
    xVec3& operator*=(float s);
    xVec3& operator-=(const xVec3& other);
    void Scale(const xVec3& v, float s);
    void NormalizeSafe();
    void normalizeFast();

    static const xVec3 m_Null;

    bool operator==(const xVec3& v) const {
        return x == v.x && y == v.y && z == v.z;
    }

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

class xSphere {
public:
    xVec3 center;
    float r;
};

extern xMat4x3 g_I3;
extern const xVec3 g_O3;

void xMat4x3Invert(xMat4x3* o, const xMat4x3* m);
void xMat3x3GetEuler(const xMat3x3* m, xVec3* euler);
void xMat3x3RMulVec(xVec3* o, const xMat3x3* m, const xVec3* v);
void xMat3x3Mul(xMat3x3* o, const xMat3x3* a, const xMat3x3* b);
void xMat3x3Euler(xMat3x3* m, float yaw, float pitch, float roll);
void xMat3x3Rot(xMat3x3* m, const xVec3* axis, float angle);
void xMat3x3RotY(xMat3x3* m, float angle);
void xMat3x3GetScale(const xMat3x3* m, xVec3* scale);
void xMat3x3Normalize(xMat3x3* o, const xMat3x3* m);
void xMat4x3Orthonormalize(xMat4x3* o, const xMat4x3* m);

class xQuat {
public:
    xVec3 v;
    float s;
};

void xQuatFromMat(xQuat* q, const xMat3x3* m);
void xQuatNormalize(xQuat* o, const xQuat* q);
void v3normalize(float& len, xVec3* o, xVec3* v);
void v3add(xVec3* o, xVec3* a, xVec3* b);
float xAngleClamp(float angle);
float xacos(float x);
void v3cross(xVec3* o, xVec3* a, xVec3* b);
xVec3 operator*(const xVec3& v, float s);

// Weak in the image: called, never inlined, so defined below its callers.
inline void v3addto(xVec3* a, xVec3* b);

// A vector built in place from three floats: the image names Math::Vector's
// constructor for it, the one the linker kept.
extern "C" void __ct__Q24Math6VectorFfff(void* self, float x, float y,
                                         float z);

// ---------------------------------------------------------------------------
// Assets

// An asset id: a class whose conversion reads the 64-bit value, which is
// what loads it ahead of the entity manager a call through it needs.
class uid {
public:
    operator unsigned long long() const { return internalUid; }

    unsigned long long internalUid;
};

namespace Sext {

class xBaseAsset {
public:
    uid id;
    unsigned int baseType;
    unsigned short linkCount;
    unsigned short baseFlags;
};

class PhysicsDataStruct {
public:
    float mass;
};

// 0x100 in the DWARF; only the fields this unit reads.
class xEntAsset : public xBaseAsset {
public:
    unsigned char flags;
    unsigned char subtype;
    unsigned char pad;
    bool Targettable;
    unsigned char Pad0;
    unsigned char moreFlags;
    uid surfaceID;
    xVec3 Orientation;
    xVec3 Pos;
    xVec3 Scale;
    unsigned char _pad0[0x98 - 0x44];
    PhysicsDataStruct physicsData;
};

}  // namespace Sext

// ---------------------------------------------------------------------------
// Models

class xAnimState {
public:
    unsigned char _pad0[0x18];
    unsigned int UserFlags;
};

class xAnimSingle {
public:
    unsigned int SingleFlags;
    xAnimState* State;
};

class xAnimPlay {
public:
    unsigned char _pad0[0xC];
    xAnimSingle* Single;
};

namespace Math {
class Matrix43;
}

namespace Graphics {

class ModelPrototype {
public:
    unsigned char _pad0[0x38];
    void* skeleton;
};

// 0x68 in the DWARF.
class Model {
public:
    void SetRootTransform(const Math::Matrix43& mat);

    unsigned char _pad0[0x48];
    ModelPrototype* modelProto;
    unsigned char _pad1[0x68 - 0x4C];
};

}  // namespace Graphics

// A collision triangle: its surface index, then its packed direction.
class FixedPoint_2_14 {
public:
    float ToFloat() const { return 6.10370189e-05f * (float)data; }

    short data;
};

class TriangleInfo {
public:
    short surfaceID;
    FixedPoint_2_14 vDirection[3];
};

class VelocityStruct {
public:
    float magnitude;
    xVec3 direction;
};

// 0xA8 in the DWARF.
class SurfaceGamePlay {
public:
    unsigned char _pad0[0x40];
    VelocityStruct SurfaceVelocity;
};

namespace World {

class EntityHandleBase;

// 0x4C in the DWARF: the triangle table sits in the tri-mesh at +0x18.
class CollisionMeshBlobEntity {
public:
    unsigned char _pad0[0x30];
    TriangleInfo* triInfo;
    void* physicsDataContainer;
    int collFilter;
    unsigned char* memBlock;
    int numSurfaces;
    EntityHandleBase** surfaceTable;
    void* owners;
};

class ModelPrototypeEntity {
public:
    unsigned char _pad0[0x6C];
    CollisionMeshBlobEntity* collmeshBlob;
};

// 0x98 in the DWARF.
class ModelInstanceArticle {
public:
    unsigned char _pad0[0x18];
    ModelPrototypeEntity* protoEnt;
    unsigned char _pad1[0x24 - 0x1C];
    Graphics::Model model;
    unsigned char _pad2[0x98 - 0x8C];
};

// 0x178 in the DWARF: xModelInstance's matrix, scale and animation, then
// the article at +0xC4.
class xOGModel {
public:
    xMat4x3 Mat;
    xVec3 Scale;
    xAnimPlay* Anim;
    unsigned short Flags;
    unsigned short pad;
    unsigned int renderCustomizerMask;
    unsigned char _pad0[0xC4 - 0x58];
    ModelInstanceArticle mModelArt;
};

}  // namespace World

void xModelUpdate(World::xOGModel* model, float dt);
void xModelEval(World::xOGModel* model);
void xModelEvalSingle(World::xOGModel* model);
void xMat4x3FromNGMatrix(xMat4x3* o, const Math::Matrix43* m);
void xFFXApplyOne(xFFX* ffx, xEnt* ent, xScene* sc, float dt);

// ---------------------------------------------------------------------------
// The entity manager, as the weak FindAsset reaches it

namespace Domains {

#pragma pack(push, 4)
class Blobloid {
public:
    void* BlobData() const;

    unsigned long long blobUID;
    int wmlTypeID;
    int blobSize;
    unsigned char subType;
    unsigned char blobFlags;
    unsigned short langID;
    void* memHandle;
    unsigned short domRefMaskList;
};
#pragma pack(pop)

}  // namespace Domains

namespace World {

class EmbeddedTreeNode {
public:
    void* left;
    long right_color_bal;
};

class EntityHandleBase : public Domains::Blobloid {
public:
    EmbeddedTreeNode byUidNode;
};

// The handles sorted by uid: an AVL tree whose right link carries two
// balance bits.
class TypeTree {
public:
    unsigned int count;
    EntityHandleBase* root;
};

class EntityManager {
public:
    // NEAR MISS: 25 of 35 retail words differ, 132 B against 140. Retail's
    // inlined tree Find walks the node in r5 (the id in r6:r7) and leaves a
    // dead `b` after the right-link arm; ours walks in r7.
    static void* FindAsset(unsigned long long id) {
        EntityHandleBase* handle = g_handleSortedTree.root;
        while (handle != 0) {
            int c;
            if (id < handle->blobUID) {
                c = -1;
            } else {
                c = handle->blobUID < id;
            }
            if (c < 0) {
                handle = (EntityHandleBase*)handle->byUidNode.left;
            } else if (c > 0) {
                handle = (EntityHandleBase*)(handle->byUidNode.right_color_bal & ~3);
            } else {
                goto found;
            }
        }
        handle = 0;
    found:
        if (handle != 0) {
            return handle->BlobData();
        }
        return 0;
    }

    static TypeTree g_handleSortedTree;
};

EntityManager* GetEntityManager();

}  // namespace World

// ---------------------------------------------------------------------------
// Serialisation

class xSerial {
public:
    int Write(char* data, int elesize, int n);
    int Read(char* data, int elesize, int n);

    int Write(int data) { return Write((char*)&data, 4, -1); }
    int Read(int* data) { return Read((char*)data, 4, -1); }
};

// ---------------------------------------------------------------------------
// Havok, as the entity reaches it

class hkBool {
public:
    operator bool() const { return m_bool != 0; }

    char m_bool;
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
    virtual void _hb0();
};

class hkReferencedObject : public hkBaseObject {
public:
    unsigned short m_memSizeAndFlags;
    short m_referenceCount;
};

class hkVector4 {
public:
    void operator=(const hkVector4& v);
    void mul4(float s);
    float dot3(const hkVector4& a) const;

    float m_quad[4] __attribute__((aligned(16)));
};

class hkMatrix3 {
public:
    hkVector4 m_col0;
    hkVector4 m_col1;
    hkVector4 m_col2;
};

class hkRotation : public hkMatrix3 {};

class hkTransform {
public:
    hkTransform& operator=(const hkTransform& t);

    hkRotation m_rotation;
    hkVector4 m_translation;
};

class hkAabb {
public:
    hkVector4 m_min;
    hkVector4 m_max;
};

class hkQuaternion;

namespace Math {

class Vector4 {
public:
    // Value-initialising Vector4() would add four zero stores (zTiki.cpp).
    Vector4() {}

    Vector4& Assign(float x, float y, float z, float w);

    operator const hkVector4&() const { return *(const hkVector4*)this; }
    operator const hkQuaternion&() const {
        return *(const hkQuaternion*)this;
    }

    float v[4] __attribute__((aligned(16)));
};

class Vector : public Vector4 {};

void Add(Vector& o, const Vector& a, const Vector& b);

}  // namespace Math

extern "C" void __ct__Q24Math8Matrix33Fv(void* self);

namespace Math {

// 0x30 in the DWARF; its constructor is the one the linker folded onto
// Matrix33's.
class Matrix43 {
public:
    Matrix43() { __ct__Q24Math8Matrix33Fv(this); }

    float m[12];
};

}  // namespace Math

class hkQuaternion {
public:
    hkVector4 m_vec;
};

// hkVector4's (x, y, z, w = 0) constructor: the linker folded it onto
// Math::Vector4::Assign (WAD00.cpp).
class hkVector4Init : public hkVector4 {
public:
    hkVector4Init(float x, float y, float z) {
        ((Math::Vector4*)this)->Assign(x, y, z, 0.0f);
    }
};

class hkpPropertyValue {
public:
    void setInt(const int i) { m_data = i; }
    int getInt() const { return (int)m_data; }
    void* getPtr() const { return (void*)(unsigned int)m_data; }

    unsigned long long m_data;
};

class hkpProperty {
public:
    unsigned int m_key;
    unsigned int m_alignmentPadding;
    hkpPropertyValue m_value;
};

class hkpShape : public hkReferencedObject {
public:
    virtual void _s1();
    virtual void _s2();
    virtual void _s3();
    virtual void _s4();
    virtual void _s5();
    virtual void _s6();
    virtual void getAabb(const hkTransform& localToWorld, float tolerance,
                         hkAabb& out) const;

    unsigned long m_userData;
    int m_type;
};

class hkpCdBody {
public:
    hkpShape* m_shape;
    unsigned int m_shapeKey;
    void* m_motion;
    hkpCdBody* m_parent;
};

// 0x88 in the DWARF.
class hkpWorldObject : public hkReferencedObject {
public:
    enum MtChecks {
        MULTI_THREADING_CHECKS_ENABLE = 0,
        MULTI_THREADING_CHECKS_IGNORE = 1
    };

    bool hasProperty(unsigned int key,
                     MtChecks mtCheck = MULTI_THREADING_CHECKS_ENABLE) const;
    hkpPropertyValue removeProperty(unsigned int key);
    void addProperty(unsigned int key, hkpPropertyValue value);

    // Weak in the image, after xGetSurface: called there, not inlined.
    // NEAR MISS: 2 of 23 words. Retail allocates the frame (stwu) before the
    // loop's first two loads, ours after them. Tried: the result declared
    // first, the index declared first, an empty constructor, the definition
    // outside the class.
    hkpPropertyValue getProperty(
        unsigned int key,
        MtChecks mtCheck = MULTI_THREADING_CHECKS_ENABLE) const {
        for (int i = 0; i < m_properties.getSize(); ++i) {
            if (m_properties[i].m_key == key) {
                return m_properties[i].m_value;
            }
        }

        hkpPropertyValue returnValue;
        returnValue.m_data = 0;
        return returnValue;
    }

    void* m_world;
    unsigned long m_userData;
    hkpCdBody m_collidable;
    unsigned char _pad0[0x78 - 0x20];
    hkArray<hkpProperty> m_properties;
    void* m_aiData;
};

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

    unsigned char m_type;
    unsigned char _pad0[0x10 - 0x9];
    hkTransform m_transform;
    unsigned char _pad1[0x120 - 0x50];
};

class hkpEntity : public hkpWorldObject {
public:
    hkBool isActive() const;

    unsigned char _pad1[0xE0 - 0x88];
    hkpMotion m_motion;
};

class hkpRigidBody : public hkpEntity {
public:
    hkpMotion* getStoredDynamicMotion();
    void setLinearVelocity(const hkVector4& newVel);
    void setAngularVelocity(const hkVector4& newVel);

    const hkTransform& getTransform() const { return m_motion.m_transform; }
    float getMass() const { return m_motion.getMass(); }
    hkpMotion::MotionType getMotionType() const {
        return (hkpMotion::MotionType)m_motion.m_type;
    }
};

class hkpPhantom : public hkpWorldObject {
public:
    unsigned char _pad2[0xA0 - 0x88];
};

// 0x150 in the DWARF: the motion state's transform at +0xA0.
class hkpShapePhantom : public hkpPhantom {
public:
    const hkTransform& getTransform() const { return m_transform; }

    hkTransform m_transform;
};

class hkpCharacterProxy {
public:
    hkpShapePhantom* getShapePhantom();
};

// 0x20 in the DWARF; the vector at +0x10 is four plain floats here so that
// the entity around it stays four-aligned.
class xHavokPhysicsObject {
public:
    enum JointTransformSpace { JointTransformSpace_0 = 0 };
    enum PhysicsRenderableMatchType { PhysicsRenderableMatchType_0 = 0 };
    enum SystemApplicationType { SystemApplicationType_0 = 0 };

    void Cleanup();
    hkpRigidBody* GetRigidBody(unsigned int index);
    void GetBoundingSphere(xSphere* sphere) const;
    xVec3 GetBoundingSphereCenter() const {
        xSphere sphere;
        GetBoundingSphere(&sphere);
        return sphere.center;
    }
    void SetMotionType(hkpMotion::MotionType type, SystemApplicationType app);
    void SetCollisionFilter(unsigned int filterInfo);
    void GetTransform(Math::Matrix43& worldTransform);
    void MatchRenderedModelToPhysics(Graphics::Model& model, float blend,
                                     JointTransformSpace space,
                                     bool applyInvBindMat);
    void UpdateKeyframedMotion(float dt, const hkVector4& pos,
                               const hkQuaternion& rot);

    template <class T>
    void SystemApplyHavok1Param(void (hkpRigidBody::*routine)(T), T param,
                                SystemApplicationType applyType);
    void MatchPhysicsToRenderedModel(Graphics::Model& model,
                                     PhysicsRenderableMatchType match,
                                     JointTransformSpace space);

    int physicsObjectType;
    void* packedPhysicsData;
    hkpPhysicsSystem* physicsSystem;
    unsigned int _pad0;
    float creationScale[4];
};

void xHavok_RemoveFromSimWorld(const hkpPhysicsSystem* system);

// ---------------------------------------------------------------------------
// Entities

enum E_HAVOK_COLLIDE_FILTER_LAYER {
    eNoCollisionLayer = 31,
    eCollisionALL = 0,
    eFixedLayer = 1,
    eFixedNoCamLayer = 2,
    eKeyFrameObjLayer = 3,
    eDynamicSimpleObjLayer = 4,
};

enum RagdollState {
    RAGDOLL_OFF = 0,
    RAGDOLL_ON = 1,
    RAGDOLL_TRANSITION_ON = 2,
    RAGDOLL_TRANSITION_OFF = 3,
};

class xEntFrame;

namespace World {

class xOGEntity;

class EmbeddedListNode {
public:
    EmbeddedListNode* next;
    EmbeddedListNode* prev;
};

class Entity {
public:
    virtual ~Entity();
    virtual void Deactivate();
    virtual void _e2();
    virtual void _e3();
    virtual void _e4();
    virtual void _e5();
    virtual void _e6();
    virtual void _e7();

    EmbeddedListNode ogSceneNode;
    int ogUpdateIdx;
    unsigned int typeID;
    EntityHandleBase* handle;
};

}  // namespace World

#pragma pack(push, 4)

class xBase : public World::Entity {
public:
    unsigned long long id;
    unsigned int baseType;
    unsigned char UNUSED_linkCount;
    unsigned char assertFlags;
    unsigned short baseFlags;
    void* linkArray;
    void* templateParent;
    void* eventFunc;
};

namespace World {

class xOGModelHandle {
public:
    xOGModel* GetModel() const { return data; }

    xOGModel* data;
    void* autoptr;
};

class xOGEntity : public xBase {
public:
    ~xOGEntity();

    virtual bool DrivePrep(xOGEntity* driver);
    virtual void DriveAttach(xOGEntity* passenger, unsigned int flags, int bone);
    virtual void DriveDetach();
    virtual void DriveOn();
    virtual void DriveOff();
    virtual void DriveReset();
    virtual void DriveMoved();
    virtual void DriveMovedNoPass();
    virtual void DriveCauseMove(xMat4x3* a, xMat4x3* b, xMat4x3* c, xMat4x3* d,
                                bool e, bool f, float g, float h);
    virtual void _o17();
    virtual zNeoDrivenLink* DriveGetDriver();
    virtual void DriveSetDriver(zNeoDrivenLink* driver);

    xOGModelHandle ogModel;
};

}  // namespace World

#pragma pack(pop)

class xEffectAttachIntf : public World::xOGEntity {
public:
    virtual World::xOGModelHandle* GetAttachModel();
    virtual xMat4x3* GetAttachMat();
    virtual void _a22();
    virtual xVec3* GetSoundPosition() const;
    virtual void _a24();
};

class xRot {
public:
    xVec3 axis;
    float angle;
};

class xEntFrame {
public:
    xMat4x3 oldmat;
    xVec3 oldvel;
    xRot oldrot;
    xRot drot;
    xRot rot;
    xVec3 dvel;
    xVec3 vel;
    unsigned int mode;
    xVec3 dpos;
    xMat4x3 accumRelMat;
};

typedef void (*xEntMoveCallback)(xEnt* ent, xScene* sc, float dt,
                                 xEntFrame* frame, World::xOGModel* model);
typedef void (*xEntTranslateCallback)(xEnt* ent, xVec3* dpos, xMat4x3* dmat);

// 0xC0 in the DWARF, of which 0xBC is data.
class xEnt : public xEffectAttachIntf {
public:
    // Defined nowhere in this unit: it keeps the vtable out of the object.
    virtual void _e2();

    ~xEnt();
    bool DrivePrep(World::xOGEntity* driver);
    void DriveAttach(World::xOGEntity* passenger, unsigned int flags, int bone);
    void DriveDetach();
    void DriveCauseMove(xMat4x3* parent, xMat4x3* relMat, xMat4x3* lastMat,
                        xMat4x3* oldMat, bool rotate, bool yaw, float givenYaw,
                        float dt);
    zNeoDrivenLink* DriveGetDriver();
    void DriveSetDriver(zNeoDrivenLink* driver);
    World::xOGModelHandle* GetAttachModel();
    xMat4x3* GetAttachMat();
    xVec3* GetSoundPosition() const;

    virtual E_HAVOK_COLLIDE_FILTER_LAYER GetSceneInitCollisionFilter();
    virtual void _x26();
    virtual void _x27();
    virtual void SetInRagdoll(float blendInTime,
                              RagdollCallbackListener* listener);
    virtual void SetOutOfRagdoll(float blendOutTime);
    virtual void SetInRagdollForDuration(float blendInTime, float blendOutTime,
                                         float timeInRagdoll,
                                         RagdollCallbackListener* listener);
    virtual void UpdateRagdoll(float dt,
                               xHavokPhysicsObject::JointTransformSpace space,
                               bool applyInvBindMat);

    void DriveDetach(World::xOGEntity* passenger);

    Sext::xEntAsset* asset;
    E_HAVOK_COLLIDE_FILTER_LAYER storedCollisionLayer;
    unsigned int idx : 16;
    unsigned int moreFlags : 16;
    unsigned int flags : 8;
    unsigned int miscflags : 8;
    unsigned int subType : 8;
    unsigned int pflags : 8;
    unsigned char isCulled;
    unsigned char num_ffx;
    unsigned char collType;
    unsigned char chkby;
    unsigned char penby;
    unsigned char collisionOn : 2;
    xEntMoveCallback move;
    xEntFrame* frame;
    xEntTranslateCallback transl;
    xFFX* ffx;
    bool isDriving;
    zNeoDrivenLink* neoDriver;
    SurfaceGamePlay* pSurface;
    xVec3 pRigidBodyPostScale;
    unsigned int _pad0;
    xHavokPhysicsObject physicsObject;
    void* user_data;
    RagdollState ragdollState;
    float ragdollTotalBlendInTime;
    float ragdollTotalBlendOutTime;
    float ragdollCurrentBlendTime;
    float ragdollStateOnTimeRemaining;
    RagdollCallbackListener* ragdollListener;
};

class zNeoDrivenLink {
public:
    static void AddChild(World::xOGEntity* driver, World::xOGEntity* passenger,
                         xMat4x3* mat, unsigned int flags, float yaw, int bone);
    static void RemoveChild(World::xOGEntity* passenger);
};

class xHavokCharacterController {
public:
    hkpCharacterProxy* GetCharacterProxy() const;

    void* characterProxy;
    int controllerType;
};

// 0x1D0 in the DWARF: the character controller at +0x16C.
class zNPCEntity : public xEnt {
public:
    hkpShapePhantom* GetPhantomObject();

    unsigned char _pad0[0x16C - 0xBC];
    xHavokCharacterController characterController;
};

void xBaseInit(xBase* base, const Sext::xBaseAsset* asset);
void xBaseSave(xBase* base, xSerial* s);
void xBaseLoad(xBase* base, xSerial* s);
void xEntShow(xEnt* ent);
void xEntHide(xEnt* ent);
void AddToHavokSimWorld(xEnt* ent);
void xEntDefaultTranslate(xEnt* ent, xVec3* dpos, xMat4x3* dmat);
void xEntReset(xEnt* ent, const xVec3& pos, const xVec3& ang);
void xEntRotationToMatrix(xEntFrame* frame, xMat4x3* mat);
void xEntBeginUpdate(xEnt* ent, xScene* sc, float dt);
void xEntEndUpdate(xEnt* ent, xScene* sc, float dt);
void xEntApplyPhysics(xEnt* ent, xScene* sc, float dt);
void xEntMove(xEnt* ent, xScene* sc, float dt);
void xEntRedoTranslate(xEnt* ent, xVec3* dpos, xMat4x3* originalmat,
                       xMat4x3* relmat);
unsigned int xVec3Equals(const xVec3* a, const xVec3* b);

// Weak in the image: called, never inlined, so defined below its caller.
inline void xEntUpdateVisibility(xEnt* ent);

// xMath3.h's matrix product with translation, which retail inlines.
inline void xMat4x3Mul(xMat4x3* o, xMat4x3* a, xMat4x3* b) {
    xVec3 t;
    xMat3x3RMulVec(&t, b, &a->pos);
    v3add(&o->pos, &t, &b->pos);
    xMat3x3Mul(o, a, b);
}

inline void xVec3Init(xVec3* v, float x, float y, float z) {
    v->x = x;
    v->y = y;
    v->z = z;
}

// Its arguments load z, y, x, as retail's do (WAD04_14.cpp).
inline void xVec3Set(xVec3* v, float x, float y, float z) {
    __ct__Q24Math6VectorFfff(v, x, y, z);
}

// xVec3's dot product, which the linker folded onto hkVector4::dot3.
inline float xVec3Dot(const xVec3* a, const xVec3* b) {
    return ((const hkVector4*)a)->dot3(*(const hkVector4*)b);
}

// ---------------------------------------------------------------------------
// Functions, in the image's order

void xEntInit(xEnt* ent, Sext::xEntAsset* asset) {
    xBaseInit(ent, asset);

    ent->neoDriver = 0;
    ent->asset = asset;
    ent->move = 0;
    ent->transl = xEntDefaultTranslate;
    ent->flags = asset->flags;
    ent->miscflags = 0;
    ent->moreFlags = asset->moreFlags;
    ent->subType = asset->subtype;
    ent->pflags = 0;
    ent->ffx = 0;
    ent->num_ffx = 0;
    ent->frame = 0;
    ent->baseFlags |= 0x20;
    ent->pRigidBodyPostScale.x = 1.0f;
    ent->pRigidBodyPostScale.y = 1.0f;
    ent->pRigidBodyPostScale.z = 1.0f;
    ent->collisionOn = 1;
    ent->isDriving = false;
}

// The centre of an entity with no AABB to measure: its body's bounding
// sphere, or the model's position. Retail inlines it, copying either value
// into the one result slot.
inline xVec3 xEntGetBoundCenter(const xEnt* ent) {
    xSphere sphere;
    if (((xEnt*)ent)->physicsObject.GetRigidBody(0) != 0) {
        ((xEnt*)ent)->physicsObject.GetBoundingSphere(&sphere);
        return sphere.center;
    }
    return ent->ogModel.data->Mat.pos;
}

#pragma push
#pragma always_inline on
void xEntGetCenterFromAABB(const xEnt* ent, xVec3& center) {
    if (((xEnt*)ent)->physicsObject.GetRigidBody(0) != 0) {
        hkTransform xform;
        hkAabb aabb;
        hkpRigidBody* body = ((xEnt*)ent)->physicsObject.GetRigidBody(0);
        xform = body->getTransform();
        body->m_collidable.m_shape->getAabb(xform, 0.0f, aabb);

        hkVector4 aabbCenter;
        Math::Add((Math::Vector&)aabbCenter, (const Math::Vector&)aabb.m_min,
                  (const Math::Vector&)aabb.m_max);
        aabbCenter.mul4(0.5f);
        xVec3Set(&center, aabbCenter.m_quad[0], aabbCenter.m_quad[1],
                 aabbCenter.m_quad[2]);
    } else if (ent->baseType == 0x38 &&
               ((zNPCEntity*)ent)->GetPhantomObject() != 0) {
        zNPCEntity* npcEnt = (zNPCEntity*)ent;
        hkTransform xform;
        hkAabb aabb;
        xform = npcEnt->GetPhantomObject()->getTransform();
        npcEnt->GetPhantomObject()->m_collidable.m_shape->getAabb(xform, 0.0f,
                                                                  aabb);

        hkVector4 aabbCenter;
        Math::Add((Math::Vector&)aabbCenter, (const Math::Vector&)aabb.m_min,
                  (const Math::Vector&)aabb.m_max);
        aabbCenter.mul4(0.5f);
        xVec3Set(&center, aabbCenter.m_quad[0], aabbCenter.m_quad[1],
                 aabbCenter.m_quad[2]);
    } else {
        const xVec3& boundCenter = xEntGetBoundCenter(ent);
        center = boundCenter;
    }
}

#pragma pop

// Weak in the image, after xEntGetCenterFromAABB: called there, not inlined,
// so defined below it.
inline hkpShapePhantom* zNPCEntity::GetPhantomObject() {
    hkpCharacterProxy* proxy = characterController.GetCharacterProxy();
    return proxy != 0 ? proxy->getShapePhantom() : 0;
}

xEnt::~xEnt() {
    if (physicsObject.physicsSystem != 0) {
        xHavok_RemoveFromSimWorld(physicsObject.physicsSystem);
    }
    physicsObject.Cleanup();
}

void xEntSetup(xEnt* ent) {
    ent->baseFlags |= 0x20;

    if (ent->asset->surfaceID != 0) {
        ent->pSurface = (SurfaceGamePlay*)World::GetEntityManager()->FindAsset(
            ent->asset->surfaceID);
    } else {
        ent->pSurface = 0;
    }

    if (ent->collType != 0 && (ent->flags & 1)) {
        AddToHavokSimWorld(ent);
    }
}

void xEntSave(xEnt* ent, xSerial* s) {
    xBaseSave(ent, s);

    if (ent->flags & 1) {
        s->Write(1);
    } else {
        s->Write(0);
    }
}

void xEntLoad(xEnt* ent, xSerial* s) {
    xBaseLoad(ent, s);

    int b = 0;
    s->Read(&b);
    if (ent != 0) {
        if (b) {
            xEntShow(ent);
        } else {
            xEntHide(ent);
        }
    }
}

void xEntReset(xEnt* ent) {
    xEntReset(ent, ent->asset->Pos, ent->asset->Orientation);
}

void xEntReset(xEnt* ent, const xVec3& pos, const xVec3& ang) {
    xMat4x3 newFrame;

    ent->DriveSetDriver(0);

    ent->baseFlags = ent->asset->baseFlags;
    ent->baseFlags |= 0x20;
    ent->flags = ent->asset->flags;
    ent->miscflags = 0;
    ent->moreFlags = (ent->moreFlags & 0xFF00) | ent->asset->moreFlags;

    xMat3x3Euler(&newFrame, ang.x, ang.y, ang.z);
    newFrame.right *= ent->asset->Scale.x;
    newFrame.up *= ent->asset->Scale.y;
    newFrame.at *= ent->asset->Scale.z;
    __ct__Q24Math6VectorFfff(&newFrame.pos, pos.x, pos.y, pos.z);

    if (ent->ogModel.data != 0) {
        ent->ogModel.data->renderCustomizerMask = 0xFFFFFFFF;
        ent->ogModel.data->Mat = newFrame;

        World::xOGModel* model = ent->ogModel.data;
        unsigned short old_flags = model->Flags;
        model->Flags |= 1;
        xModelEvalSingle(model);
        model->Flags = old_flags;
    }

    xEntUpdateVisibility(ent);

    if (ent->frame != 0) {
        ent->ogModel.data->Mat = newFrame;
        ent->frame->oldmat = ent->ogModel.data->Mat;

        __ct__Q24Math6VectorFfff(&ent->frame->dpos, g_O3.x, g_O3.y, g_O3.z);
        __ct__Q24Math6VectorFfff(&ent->frame->dvel, g_O3.x, g_O3.y, g_O3.z);
        __ct__Q24Math6VectorFfff(&ent->frame->vel, g_O3.x, g_O3.y, g_O3.z);
        __ct__Q24Math6VectorFfff(&ent->frame->oldvel, g_O3.x, g_O3.y, g_O3.z);

        __ct__Q24Math6VectorFfff(&ent->frame->rot.axis,
                                 ent->asset->Orientation.x,
                                 ent->asset->Orientation.y,
                                 ent->asset->Orientation.z);
        ent->frame->rot.angle = 0.0f;
        *(hkVector4*)&ent->frame->oldrot = *(const hkVector4*)&ent->frame->rot;
    }

    if (ent->asset->surfaceID != 0) {
        ent->pSurface = (SurfaceGamePlay*)World::GetEntityManager()->FindAsset(
            ent->asset->surfaceID);
    } else {
        ent->pSurface = 0;
    }

    ent->ragdollState = RAGDOLL_OFF;
    ent->ragdollListener = 0;
    ent->ragdollTotalBlendInTime = ent->ragdollTotalBlendOutTime = -1.0f;

    AddToHavokSimWorld(ent);
}

inline void xEntUpdateVisibility(xEnt* ent) {
    if (ent->flags & 1) {
        xEntShow(ent);
    } else {
        xEntHide(ent);
    }
}

unsigned int xEntGetAnimFlags(const xEnt* ent) {
    return ent->ogModel.data->Anim->Single->State->UserFlags;
}

// The render-follows-physics test is one line of nested inline tests,
// each result materialised as retail keeps it.
inline bool xEntIsMovingMotionType(hkpMotion::MotionType motionType) {
    return motionType != hkpMotion::MOTION_FIXED &&
           motionType != hkpMotion::MOTION_KEYFRAMED;
}

inline bool xEntIsActiveMovingBody(hkpRigidBody* body,
                                   hkpMotion::MotionType motionType) {
    return xEntIsMovingMotionType(motionType) && body->isActive();
}

inline bool xEntIsRagdollOrActive(xEnt* ent, hkpRigidBody* body,
                                  hkpMotion::MotionType motionType) {
    return ent->physicsObject.physicsObjectType == 3 ||
           xEntIsActiveMovingBody(body, motionType);
}

inline bool xEntHasStoredDynamicMotion(xEnt* ent, hkpRigidBody* body,
                                       hkpMotion::MotionType motionType) {
    return ent->baseType == 0x5A && motionType == hkpMotion::MOTION_FIXED &&
           body != 0 && body->getStoredDynamicMotion() != 0;
}

inline bool xEntRenderFollowsPhysics(xEnt* ent, hkpRigidBody* body,
                                     hkpMotion::MotionType motionType) {
    return xEntIsRagdollOrActive(ent, body, motionType) ||
           xEntHasStoredDynamicMotion(ent, body, motionType);
}

#pragma push
#pragma always_inline on
void xEntUpdate(xEnt* ent, xScene* sc, float dt) {
    Graphics::Model* model;

    xEntBeginUpdate(ent, sc, dt);

    hkpMotion::MotionType motionType = hkpMotion::MOTION_FIXED;
    hkpRigidBody* body = ent->physicsObject.GetRigidBody(0);
    if (body != 0 && ent->collisionOn) {
        motionType = body->getMotionType();
    }

    if (xEntRenderFollowsPhysics(ent, body, motionType)) {
        model = &ent->GetAttachModel()->data->mModelArt.model;
        if (ent->physicsObject.physicsObjectType == 3 &&
            model->modelProto->skeleton != 0 &&
            motionType != hkpMotion::MOTION_KEYFRAMED) {
            Math::Matrix43 rootRenderableTrans;
            ent->physicsObject.GetTransform(rootRenderableTrans);
            model->SetRootTransform(rootRenderableTrans);
        }

        ent->physicsObject.MatchRenderedModelToPhysics(
            *model, 1.0f, (xHavokPhysicsObject::JointTransformSpace)1, true);
        xMat4x3FromNGMatrix(&ent->ogModel.data->Mat, (const Math::Matrix43*)model);
        ent->ogModel.data->Mat.right.normalizeFast();
        ent->ogModel.data->Mat.up.normalizeFast();
        ent->ogModel.data->Mat.at.normalizeFast();
        ent->ogModel.data->Mat.right *= ent->pRigidBodyPostScale.x;
        ent->ogModel.data->Mat.up *= ent->pRigidBodyPostScale.y;
        ent->ogModel.data->Mat.at *= ent->pRigidBodyPostScale.z;
        ent->pflags |= 1;
        ent->frame->mode = 0x40;
    }

    if (ent->pflags & 2) {
        xEntApplyPhysics(ent, sc, dt);
    }

    if (ent->pflags & 1) {
        xEntMove(ent, sc, dt);
    }

    if (ent->ffx != 0) {
        xFFXApplyOne(ent->ffx, ent, sc, dt);
    }

    xEntEndUpdate(ent, sc, dt);
}
#pragma pop

#pragma push
#pragma always_inline on
void xEntBeginUpdate(xEnt* ent, xScene*, float dt) {
    // Read for the test and again for the call, as retail has it: a plain
    // member read is folded into a single load (zUIModel.cpp, NOTES.md).
    if (*(World::xOGModel* volatile*)&ent->ogModel.data != 0) {
        xModelUpdate(ent->ogModel.data, dt);

        if (ent->frame != 0) {
            if (!(ent->frame->oldmat.pos == ent->ogModel.data->Mat.pos) &&
                ent->isDriving && ent->baseType != 0x56) {
                hkpRigidBody* body = ent->physicsObject.GetRigidBody(0);
                if (body != 0 && 0.0f != body->getMass() &&
                    body->getMotionType() != hkpMotion::MOTION_KEYFRAMED) {
                    ent->DriveMovedNoPass();
                } else {
                    ent->DriveMoved();
                }
            }

            __ct__Q24Math6VectorFfff(&ent->frame->oldvel, ent->frame->vel.x,
                                     ent->frame->vel.y, ent->frame->vel.z);
            ent->frame->oldmat = ent->ogModel.data->Mat;
            *(hkVector4*)&ent->frame->oldrot =
                *(const hkVector4*)&ent->frame->rot;
            ent->frame->mode = 0;
        }
    }
}
#pragma pop

void xEntEndUpdate(xEnt* ent, xScene*, float dt) {
    if (*(World::xOGModel* volatile*)&ent->ogModel.data != 0) {
        xModelEval(ent->ogModel.data);

        if (ent->baseType != 0x55) {
            ent->UpdateRagdoll(dt, (xHavokPhysicsObject::JointTransformSpace)1,
                               true);
        }

        if (ent->baseType == 0x56 && ent->physicsObject.physicsObjectType == 3) {
            Graphics::Model& model = ent->GetAttachModel()->data->mModelArt.model;
            if (model.modelProto->skeleton == 0) {
                ent->physicsObject.MatchPhysicsToRenderedModel(
                    model, (xHavokPhysicsObject::PhysicsRenderableMatchType)1,
                    (xHavokPhysicsObject::JointTransformSpace)1);
            }
        }
    }
}

// The frame arms move the model's matrix again, as the model arms do: retail's.
void xEntDefaultTranslate(xEnt* ent, xVec3* dpos, xMat4x3* dmat) {
    if (dmat != 0) {
        if (ent->ogModel.data != 0) {
            xMat4x3Mul(&ent->ogModel.data->Mat, &ent->ogModel.data->Mat, dmat);
        }
        if (ent->frame != 0) {
            xMat4x3Mul(&ent->ogModel.data->Mat, &ent->ogModel.data->Mat, dmat);
        }
    } else {
        if (ent->ogModel.data != 0) {
            v3addto(&ent->ogModel.data->Mat.pos, dpos);
        }
        if (ent->frame != 0) {
            v3addto(&ent->ogModel.data->Mat.pos, dpos);
        }
    }
}

void xEntRotationToMatrix(xEntFrame* frame, xMat4x3* mat) {
    if (frame->mode & 0x20) {
        if (frame->mode & 0x400) {
            v3addto(&frame->rot.axis, &frame->drot.axis);
            xMat3x3Euler(mat, frame->rot.axis.x, frame->rot.axis.y,
                         frame->rot.axis.z);
        } else {
            frame->rot.angle = xAngleClamp(frame->rot.angle + frame->drot.angle);
            xMat3x3Rot(mat, &frame->rot.axis, frame->rot.angle);
        }
        frame->drot.angle = 0.0f;
    }
}

void xEntMotionToMatrix(xEnt* ent, xEntFrame* frame, xMat4x3* mat) {
    if (frame->mode & 0x1000) {
        xEntRotationToMatrix(frame, mat);
    }

    if (frame->mode & 0x2) {
        if (frame->mode & 0x800) {
            xMat3x3RMulVec(&frame->dpos, mat, &frame->dpos);
        }
        v3addto(&ent->ogModel.data->Mat.pos, &frame->dpos);
        xVec3Init(&ent->frame->dpos, 0.0f, 0.0f, 0.0f);
    }

    if (frame->mode & 0x8) {
        if (frame->mode & 0x800) {
            xMat3x3RMulVec(&frame->dvel, mat, &frame->dvel);
        }
        v3addto(&frame->vel, &frame->dvel);
        xVec3Init(&ent->frame->dvel, 0.0f, 0.0f, 0.0f);
    }

    if (!(frame->mode & 0x1000)) {
        xEntRotationToMatrix(frame, mat);
    }
}

inline void v3addto(xVec3* a, xVec3* b) {
    a->x += b->x;
    a->y += b->y;
    a->z += b->z;
}

void xEntMove(xEnt* ent, xScene* sc, float dt) {
    hkpRigidBody* entHKBody = ent->physicsObject.GetRigidBody(0);

    if (ent->move != 0) {
        ent->move(ent, sc, dt, ent->frame, ent->ogModel.data);
    }

    xEntMotionToMatrix(ent, ent->frame, &ent->ogModel.data->Mat);

    if (entHKBody != 0 && entHKBody->hasProperty(5555)) {
        ent->physicsObject.SetMotionType(
            (hkpMotion::MotionType)entHKBody->removeProperty(5555).getInt(),
            (xHavokPhysicsObject::SystemApplicationType)1);
    }
}

World::xOGModelHandle* xEnt::GetAttachModel() { return &ogModel; }

xMat4x3* xEnt::GetAttachMat() { return &ogModel.data->Mat; }

xVec3* xEnt::GetSoundPosition() const { return &ogModel.data->Mat.pos; }

void xEnt::DriveAttach(World::xOGEntity* passenger, unsigned int flags, int) {
    xMat4x3 D;
    xMat4x3Invert(&D, &ogModel.data->Mat);

    xVec3 euler;
    xMat3x3 a_descaled;
    float dummy;
    v3normalize(dummy, &a_descaled.right, &ogModel.data->Mat.right);
    v3normalize(dummy, &a_descaled.up, &ogModel.data->Mat.up);
    v3normalize(dummy, &a_descaled.at, &ogModel.data->Mat.at);
    xMat3x3GetEuler(&a_descaled, &euler);

    zNeoDrivenLink::AddChild(this, passenger, &D, flags, euler.x, -1);
    isDriving = true;
}

void xEnt::DriveDetach() { World::xOGEntity::DriveDetach(); }

void xEnt::DriveDetach(World::xOGEntity* passenger) {
    zNeoDrivenLink::RemoveChild(passenger);
}

zNeoDrivenLink* xEnt::DriveGetDriver() { return neoDriver; }

void xEnt::DriveSetDriver(zNeoDrivenLink* driver) { neoDriver = driver; }

E_HAVOK_COLLIDE_FILTER_LAYER xEnt::GetSceneInitCollisionFilter() {
    if (chkby != 0) {
        E_HAVOK_COLLIDE_FILTER_LAYER filter = eFixedLayer;
        if (0.0f != asset->physicsData.mass) {
            filter = eDynamicSimpleObjLayer;
        }

        World::ModelPrototypeEntity* protoEnt = ogModel.data->mModelArt.protoEnt;
        if (protoEnt != 0 && protoEnt->collmeshBlob != 0) {
            E_HAVOK_COLLIDE_FILTER_LAYER packedFilterLayer =
                (E_HAVOK_COLLIDE_FILTER_LAYER)protoEnt->collmeshBlob->collFilter;
            if (packedFilterLayer == eFixedNoCamLayer) {
                if (filter == eFixedLayer) {
                    filter = eFixedNoCamLayer;
                }
            }
        }

        return filter;
    }

    return (baseType == 0x56) ? eKeyFrameObjLayer : eNoCollisionLayer;
}

// The matrix compares retail inlines around the weak xVec3Equals: each
// result materialised, the outer flag set before the inner.
inline unsigned int xMat3x3Equals(const xMat3x3* a, const xMat3x3* b) {
    return xVec3Equals(&a->right, &b->right) && xVec3Equals(&a->up, &b->up) &&
           xVec3Equals(&a->at, &b->at);
}

inline unsigned int xMat4x3Equals(const xMat4x3* a, const xMat4x3* b) {
    return xMat3x3Equals(a, b) && xVec3Equals(&a->pos, &b->pos);
}

// Ahead of xEntRedoTranslate, which it calls and retail does not inline:
// the always_inline region would take any ordinary function above it.
#pragma push
#pragma always_inline on
// NEAR MISS: 8 of 442 words, two register ties. The model and accumRelMat
// pointers of the inlined xMat4x3Mul in the last yaw branch are r26/r27 where
// retail has r27/r26, and the quaternion vector's reference is kept in r26
// where retail keeps r31. The model matrix assigned to itself in three arms
// is retail's copy call, kept.
void xEnt::DriveCauseMove(xMat4x3* parent, xMat4x3* relMat, xMat4x3* lastMat,
                          xMat4x3* oldMat, bool rotate, bool yaw,
                          float givenYaw, float dt) {
    xVec3 scale;
    xVec3 newEuler, lastEuler;
    xMat4x3 InvRel;

    xMat4x3Invert(&InvRel, lastMat);

    if (yaw) {
        xMat3x3 a_descaled;
        float dummy;

        v3normalize(dummy, &a_descaled.right, &parent->right);
        v3normalize(dummy, &a_descaled.up, &parent->up);
        v3normalize(dummy, &a_descaled.at, &parent->at);
        xMat3x3GetEuler(&a_descaled, &newEuler);

        v3normalize(dummy, &a_descaled.right, &InvRel.right);
        v3normalize(dummy, &a_descaled.up, &InvRel.up);
        v3normalize(dummy, &a_descaled.at, &InvRel.at);
        xMat3x3GetEuler(&a_descaled, &lastEuler);
    }

    xMat4x3 baseMat, nullMat;

    nullMat.pos = nullMat.right = nullMat.up = nullMat.at = xVec3::m_Null;

    {
        hkpRigidBody* body = physicsObject.GetRigidBody(0);

        if (xMat4x3Equals(&nullMat, &frame->accumRelMat)) {
            if (body != 0 && 0.0f != body->getMass() &&
                body->getMotionType() != hkpMotion::MOTION_KEYFRAMED) {
                xMat4x3Mul(&baseMat, lastMat, parent);

                if (!rotate || yaw) {
                    xMat3x3 rot;
                    if (yaw) {
                        float yawDiff = newEuler.x - lastEuler.x;
                        xMat3x3RotY(&rot, yawDiff);
                    }

                    xMat3x3RMulVec(&ogModel.data->Mat.pos, &baseMat,
                                   &ogModel.data->Mat.pos);
                    ogModel.data->Mat.pos += baseMat.pos;
                    if (yaw) {
                        xMat3x3Mul(&ogModel.data->Mat, &ogModel.data->Mat, &rot);
                    }
                    if (ogModel.data != 0) {
                        ogModel.data->Mat = ogModel.data->Mat;
                    }
                } else {
                    transl(this, 0, &baseMat);
                }
            } else {
                xMat4x3Mul(&baseMat, relMat, parent);

                if (!rotate || yaw) {
                    xMat3x3 rot;
                    if (yaw) {
                        float yawDiff = newEuler.x - givenYaw;
                        xMat3x3RotY(&rot, yawDiff);
                    }

                    ogModel.data->Mat = *oldMat;
                    xMat3x3RMulVec(&ogModel.data->Mat.pos, &baseMat,
                                   &ogModel.data->Mat.pos);
                    ogModel.data->Mat.pos += baseMat.pos;
                    if (yaw) {
                        xMat3x3Mul(&ogModel.data->Mat, &ogModel.data->Mat, &rot);
                    }
                    if (ogModel.data != 0) {
                        ogModel.data->Mat = ogModel.data->Mat;
                    }
                } else {
                    xEntRedoTranslate(this, 0, oldMat, &baseMat);
                }
            }
        } else {
            if (body != 0 && 0.0f != body->getMass()) {
                xMat4x3Mul(&baseMat, lastMat, parent);

                if (!rotate || yaw) {
                    xMat3x3 rot;
                    if (yaw) {
                        float yawDiff = newEuler.x - lastEuler.x;
                        xMat3x3RotY(&rot, yawDiff);
                    }

                    xMat3x3RMulVec(&ogModel.data->Mat.pos, &baseMat,
                                   &ogModel.data->Mat.pos);
                    ogModel.data->Mat.pos += baseMat.pos;
                    if (yaw) {
                        xMat3x3Mul(&ogModel.data->Mat, &ogModel.data->Mat, &rot);
                    }
                    if (ogModel.data != 0) {
                        ogModel.data->Mat = ogModel.data->Mat;
                    }
                } else {
                    transl(this, 0, &baseMat);
                }
            } else {
                xMat4x3Mul(&baseMat, relMat, parent);

                if (!rotate || yaw) {
                    xMat3x3 rot;
                    if (yaw) {
                        float yawDiff = newEuler.x - givenYaw;
                        xMat3x3RotY(&rot, yawDiff);
                    }

                    xMat4x3Mul(&ogModel.data->Mat, &frame->accumRelMat, oldMat);
                    xMat3x3RMulVec(&ogModel.data->Mat.pos, &baseMat,
                                   &ogModel.data->Mat.pos);
                    ogModel.data->Mat.pos += baseMat.pos;
                    if (yaw) {
                        xMat3x3Mul(&ogModel.data->Mat, &ogModel.data->Mat, &rot);
                    }
                } else {
                    xMat4x3 tempOldMat;
                    xMat4x3Mul(&tempOldMat, &frame->accumRelMat, oldMat);
                    xEntRedoTranslate(this, 0, &tempOldMat, &baseMat);
                }
            }
        }
    }

    xMat3x3GetScale(&ogModel.data->Mat, &scale);
    xMat4x3& mat = ogModel.data->Mat;
    xMat4x3Orthonormalize(&mat, &mat);
    mat.right *= scale.x;
    mat.up *= scale.y;
    mat.at *= scale.z;

    hkpRigidBody* body = physicsObject.GetRigidBody(0);
    if (body != 0) {
        xMat4x3* pMat = &ogModel.data->Mat;
        xMat3x3 tempMat = *pMat;
        xMat3x3Normalize(&tempMat, &tempMat);
        xQuat quat;
        xQuatFromMat(&quat, &tempMat);
        xQuatNormalize(&quat, &quat);

        if (body->getMotionType() != hkpMotion::MOTION_KEYFRAMED) {
            if (!body->hasProperty(5555)) {
                hkpPropertyValue val;
                val.setInt(body->getMotionType());
                body->addProperty(5555, val);
            }
            physicsObject.SetMotionType(
                hkpMotion::MOTION_KEYFRAMED,
                (xHavokPhysicsObject::SystemApplicationType)1);
        }

        physicsObject.UpdateKeyframedMotion(
            dt, Math::Vector4().Assign(pMat->pos.x, pMat->pos.y, pMat->pos.z, 0.0f),
            Math::Vector4().Assign(quat.v.x, quat.v.y, quat.v.z, quat.s));
    }
}
#pragma pop

// Defined ahead of xEntRedoTranslate, which inlines it; the image keeps
// this copy too.
void xMat3x3Copy(xMat3x3* o, const xMat3x3* m) { *o = *m; }

// As in xEntDefaultTranslate, the frame arms write the model's matrix: retail's.
void xEntRedoTranslate(xEnt* ent, xVec3* dpos, xMat4x3* originalmat,
                       xMat4x3* relmat) {
    if (dpos != 0) {
        if (ent->ogModel.data != 0) {
            ent->ogModel.data->Mat.pos = *dpos;
        }
        if (ent->frame != 0) {
            ent->ogModel.data->Mat.pos = *dpos;
        }
    } else if (relmat != 0) {
        if (ent->ogModel.data != 0) {
            xMat4x3 tempModelMat;
            if (ent->baseType == 0x56 && ent->subType == 4) {
                tempModelMat = ent->ogModel.data->Mat;
            }

            xMat4x3Mul(&ent->ogModel.data->Mat, originalmat, relmat);

            if (ent->baseType == 0x56 && ent->subType == 4) {
                xMat3x3Copy(&ent->ogModel.data->Mat, &tempModelMat);
            }
        }

        if (ent->frame != 0) {
            xMat4x3Mul(&ent->ogModel.data->Mat, originalmat, relmat);
        }
    }
}

bool xEnt::DrivePrep(World::xOGEntity*) {
    if (frame != 0) {
        frame->accumRelMat = g_I3;
    }

    return true;
}

// The x-and-y compare, an inline returning unsigned int: its result is
// materialised inside the outer flag, as DriveCauseMove's matrix compares are.
inline unsigned int xVec3EqualsXY(const xVec3* a, const xVec3* b) {
    return (float)__fabs(a->x - b->x) <= 0.0f &&
           (float)__fabs(a->y - b->y) <= 0.0f;
}

// Weak in the image, after DriveCauseMove, its only caller.
// NEAR MISS: 6 of 36 words, a register tie. The outer flag is in r6 and the
// x-and-y flag starts as a copy of it (mr r0,r6), where retail loads both
// with li into r0 and r5. Tried: one && chain, a ternary, a bool, an int and
// an unsigned int x-and-y helper, three inlined compares.
#pragma push
#pragma always_inline on
unsigned int xVec3Equals(const xVec3* a, const xVec3* b) {
    return xVec3EqualsXY(a, b) && (float)__fabs(a->z - b->z) <= 0.0f;
}
#pragma pop

void xEntApplyPhysics(xEnt* ent, xScene*, float dt) {
    xVec3 dposvel;

    dposvel.Scale(ent->frame->vel, dt);
    ent->frame->dpos += dposvel;
    ent->frame->mode |= 2;
}

bool xEntTurnToFace(xEnt* ent, const xVec3* target, float speedLimit,
                    float dt) {
    xVec3 currentFacing = ent->ogModel.data->Mat.at;
    float len;
    v3normalize(len, &currentFacing, &currentFacing);

    float bias = 1e-5f;
    float dot = xVec3Dot(&currentFacing, target);
    if (dot < 1.0f - bias) {
        xVec3 axis;
        v3cross(&axis, &currentFacing, (xVec3*)target);
        float axisLen;
        v3normalize(axisLen, &axis, &axis);

        bool done = false;
        float angle = xacos(dot);
        float maxAngle = speedLimit * dt;
        bool clamped = angle > maxAngle;
        if (clamped) {
            angle = maxAngle;
        }
        if (!clamped) {
            done = true;
        }

        if (xVec3Dot(&ent->frame->rot.axis, &axis) < 0.0f) {
            angle = -angle;
        }

        xVec3Init(&ent->frame->drot.axis, 0.0f, 0.0f, 0.0f);
        ent->frame->drot.angle = angle;
        ent->frame->mode |= 0x20;
        return done;
    }

    return true;
}

SurfaceGamePlay* xGetSurface(hkpRigidBody* pRigidBody, unsigned int shapeKey,
                             bool* usingSurfaceFromTriangles) {
    void* userData = (void*)pRigidBody->m_userData;
    xEnt* pEnt;
    if (userData != 0 && (((xBase*)userData)->baseFlags & 0x20)) {
        pEnt = (xEnt*)userData;
    } else {
        return 0;
    }

    World::CollisionMeshBlobEntity* refModelCollMesh = 0;
    if (pRigidBody->hasProperty(777)) {
        refModelCollMesh =
            (World::CollisionMeshBlobEntity*)pRigidBody->getProperty(777).getPtr();
    }

    SurfaceGamePlay* surface = pEnt->pSurface;
    if (usingSurfaceFromTriangles != 0) {
        *usingSurfaceFromTriangles = false;
    }

    if (surface == 0 && shapeKey != 0xFFFFFFFF &&
        pRigidBody->m_collidable.m_shape->m_type != 9) {
        TriangleInfo* pTriInfo = 0;
        World::EntityHandleBase** pSurfaceTable = 0;
        World::CollisionMeshBlobEntity* collMesh = refModelCollMesh;
        if (collMesh == 0) {
            World::ModelPrototypeEntity* protoEnt =
                pEnt->ogModel.data->mModelArt.protoEnt;
            if (protoEnt != 0) {
                collMesh = protoEnt->collmeshBlob;
            }
        }

        if (collMesh != 0) {
            pTriInfo = collMesh->triInfo;
            pSurfaceTable = collMesh->surfaceTable;
        }

        if (pSurfaceTable != 0 && pTriInfo[shapeKey].surfaceID >= 0 &&
            pSurfaceTable[pTriInfo[shapeKey].surfaceID] != 0) {
            surface = (SurfaceGamePlay*)
                pSurfaceTable[pTriInfo[shapeKey].surfaceID]->BlobData();
            if (usingSurfaceFromTriangles != 0) {
                *usingSurfaceFromTriangles = true;
            }
        }
    }

    return surface;
}

SurfaceGamePlay* xGetSurface(hkpRigidBody* pRigidBody, unsigned int shapeKey,
                             const xVec3& normal, xVec3& velocity) {
    SurfaceGamePlay* surface;
    bool usingSurfaceFromTriangles;
    surface = xGetSurface(pRigidBody, shapeKey, &usingSurfaceFromTriangles);

    __ct__Q24Math6VectorFfff(&velocity, 0.0f, 0.0f, 0.0f);

    if (surface != 0 && surface->SurfaceVelocity.magnitude) {
        xEnt* pEnt = (xEnt*)pRigidBody->m_userData;
        if (usingSurfaceFromTriangles) {
            World::CollisionMeshBlobEntity* refModelCollMesh = 0;
            if (pRigidBody->hasProperty(777)) {
                refModelCollMesh = (World::CollisionMeshBlobEntity*)pRigidBody
                                       ->getProperty(777)
                                       .getPtr();
            }

            TriangleInfo* pTriInfo;
            if (refModelCollMesh != 0) {
                pTriInfo = refModelCollMesh->triInfo;
            } else {
                pTriInfo = pEnt->ogModel.data->mModelArt.protoEnt->collmeshBlob->triInfo;
            }

            xVec3 forward, binormal, materialModifier;
            xVec3Set(&forward, pTriInfo[shapeKey].vDirection[0].ToFloat(),
                     pTriInfo[shapeKey].vDirection[1].ToFloat(),
                     pTriInfo[shapeKey].vDirection[2].ToFloat());
            v3cross(&binormal, &forward, (xVec3*)&normal);
            __ct__Q24Math6VectorFfff(&materialModifier,
                                     surface->SurfaceVelocity.direction.x,
                                     surface->SurfaceVelocity.direction.y,
                                     surface->SurfaceVelocity.direction.z);
            velocity = forward;
            velocity *= materialModifier.x;
            float scale = materialModifier.z;
            velocity.x += binormal.x * scale;
            velocity.y += binormal.y * scale;
            velocity.z += binormal.z * scale;
        } else {
            __ct__Q24Math6VectorFfff(&velocity, surface->SurfaceVelocity.direction.x,
                                     surface->SurfaceVelocity.direction.y,
                                     surface->SurfaceVelocity.direction.z);
        }

        xMat3x3RMulVec(&velocity, &pEnt->ogModel.data->Mat, &velocity);
        velocity.NormalizeSafe();
        velocity *= surface->SurfaceVelocity.magnitude;
        velocity -= normal * xVec3Dot(&velocity, &normal);
    }

    return surface;
}

void xEnt::SetInRagdoll(float blendInTime, RagdollCallbackListener* listener) {
    if (physicsObject.physicsObjectType == 3) {
        ragdollState = RAGDOLL_TRANSITION_ON;
        ragdollListener = listener;
        ragdollTotalBlendOutTime = -1.0f;
        ragdollStateOnTimeRemaining = -1.0f;
        ragdollTotalBlendInTime = blendInTime;
        ragdollCurrentBlendTime = 0.0f;
    }
}

void xEnt::SetOutOfRagdoll(float blendOutTime) {
    if (physicsObject.physicsObjectType != 0) {
        ragdollState = RAGDOLL_TRANSITION_OFF;
        ragdollTotalBlendInTime = -1.0f;
        ragdollTotalBlendOutTime = blendOutTime;
        ragdollCurrentBlendTime = 0.0f;
    }
}

void xEnt::SetInRagdollForDuration(float blendInTime, float blendOutTime,
                                   float timeInRagdoll,
                                   RagdollCallbackListener* listener) {
    SetInRagdoll(blendInTime, listener);
    ragdollStateOnTimeRemaining = timeInRagdoll;
    ragdollTotalBlendOutTime = blendOutTime;
}

void xEnt::UpdateRagdoll(float dt,
                         xHavokPhysicsObject::JointTransformSpace jointSpace,
                         bool applyInvBindMat) {
    RagdollState currentRagdollState = ragdollState;

    if (currentRagdollState != RAGDOLL_OFF) {
        float animRagdollBlendAmount = 1.0f;

        if (currentRagdollState == RAGDOLL_TRANSITION_ON ||
            currentRagdollState == RAGDOLL_TRANSITION_OFF) {
            float* totalBlendTime;

            if (currentRagdollState == RAGDOLL_TRANSITION_ON) {
                if (0.0f == ragdollCurrentBlendTime) {
                    Graphics::Model& model = GetAttachModel()->data->mModelArt.model;
                    physicsObject.SetCollisionFilter(4);
                    physicsObject.SetMotionType(
                        hkpMotion::MOTION_DYNAMIC,
                        (xHavokPhysicsObject::SystemApplicationType)0);
                    physicsObject.MatchPhysicsToRenderedModel(
                        model, (xHavokPhysicsObject::PhysicsRenderableMatchType)0,
                        jointSpace);
                }
                totalBlendTime = &ragdollTotalBlendInTime;
            } else {
                totalBlendTime = &ragdollTotalBlendOutTime;
            }

            if (0.0f == ragdollCurrentBlendTime) {
                if (currentRagdollState == RAGDOLL_TRANSITION_ON) {
                    if (ragdollListener != 0) {
                        ragdollListener->OnRagdollTransitionOnStart(this);
                    }
                } else {
                    if (ragdollListener != 0) {
                        ragdollListener->OnRagdollTransitionOffStart(this);
                    }
                }
            }

            ragdollCurrentBlendTime += dt;

            if (ragdollCurrentBlendTime < *totalBlendTime) {
                animRagdollBlendAmount = ragdollCurrentBlendTime / *totalBlendTime;
            } else {
                *totalBlendTime = -1.0f;

                if (currentRagdollState == RAGDOLL_TRANSITION_OFF) {
                    hkVector4Init zeroVel(0.0f, 0.0f, 0.0f);
                    physicsObject.SetMotionType(
                        hkpMotion::MOTION_FIXED,
                        (xHavokPhysicsObject::SystemApplicationType)0);
                    physicsObject.SystemApplyHavok1Param<const hkVector4&>(
                        &hkpRigidBody::setLinearVelocity, zeroVel,
                        (xHavokPhysicsObject::SystemApplicationType)0);
                    physicsObject.SystemApplyHavok1Param<const hkVector4&>(
                        &hkpRigidBody::setAngularVelocity, zeroVel,
                        (xHavokPhysicsObject::SystemApplicationType)0);
                    physicsObject.SetCollisionFilter(31);
                    ragdollState = RAGDOLL_OFF;
                    if (ragdollListener != 0) {
                        ragdollListener->OnRagdollOff(this);
                    }
                } else {
                    ragdollState = RAGDOLL_ON;
                    if (ragdollListener != 0) {
                        ragdollListener->OnRagdollOn(this);
                    }
                }
            }

            if (currentRagdollState == RAGDOLL_TRANSITION_OFF) {
                animRagdollBlendAmount = 1.0f - animRagdollBlendAmount;
            }
        } else if (currentRagdollState == RAGDOLL_ON) {
            if (ragdollStateOnTimeRemaining > 0.0f) {
                ragdollStateOnTimeRemaining -= dt;
                if (ragdollStateOnTimeRemaining <= 0.0f) {
                    if (ragdollTotalBlendOutTime >= 0.0f) {
                        SetOutOfRagdoll(ragdollTotalBlendOutTime);
                    } else {
                        SetOutOfRagdoll(0.0f);
                    }
                }
            }
        }

        if (currentRagdollState != RAGDOLL_OFF) {
            if (ragdollListener != 0) {
                ragdollListener->OnRagdollUpdate(this, animRagdollBlendAmount);
            }

            physicsObject.MatchRenderedModelToPhysics(
                GetAttachModel()->data->mModelArt.model, animRagdollBlendAmount,
                jointSpace, applyInvBindMat);

            if (ragdollListener != 0) {
                ragdollListener->OnRagdollPostUpdate(this);
            }

            if (ragdollState == RAGDOLL_OFF) {
                ragdollListener = 0;
            }
        }
    }
}
