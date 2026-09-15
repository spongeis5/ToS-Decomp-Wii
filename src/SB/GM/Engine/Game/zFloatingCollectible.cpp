#include "SB/GM/Engine/Game/zFloatingCollectible.pool.h"

// zFloatingCollectible.cpp -- the floating collectible: happy and unhappy
// nuggets, health, lives, powerups, keys and the rest, turning where they
// were placed or dropped to fall under Havok, until a player collects one.
// Read from the image with tools/brief.py; the layouts are the DWARF's
// (tools/dwarf_types.py), the virtual slots the image's (tools/vtslot.py).
//
// Not written, sorted before reading from counts over the unit's briefs:
// - Init and zFloatingCollectibleSceneExit name symbols in WAD02.cpp's
//   anonymous namespace (the event wrapper, sGenericIdleFX), which a
//   fragment cannot name; the wrapper is one of them.
// - LoadModel (5), Reset (7), StartCollecting (4), Update (6), UpdateIdle
//   (11) and UpdateToBeCollected (4) load that many distinct float
//   literals: the four-literal wall.
// - SetTextureBlendFactor calls RenderableSceneRef::UpdateIsAlpha through a
//   pointer to member, a stack copy of an anonymous data constant: not
//   attempted. UpdateIsAlpha is emitted only with it, xVec3::operator+=(float)
//   only with StartCollecting, and xBase's constructor has no caller in the
//   unit, so nothing here would emit them.

class hkpCollidable;
class hkpEntity;
class hkpShape;
class xBase;
class xEffectAttachIntf;
class xSerial;
class zFloatingCollectible;

namespace World {
class EntityHandleBase;
class xOGEntity;
}

namespace Sext {
class zFloatingCollectibleAsset;
}

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {

enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };

void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap, eMemMgrTag tag,
                      bool clear);

}  // namespace Memory

extern "C" {
void* memset(void* dst, int c, unsigned long n);
}

inline void* operator new(unsigned long, void* p) { return p; }

enum ForceEvent {
    FE_YES = 0,
    FE_NO = 1
};

enum E_HAVOK_COLLIDE_FILTER_LAYER {
    eNoCollisionLayer = 31,
    eCollectiblesLayer = 20,
    eStaticCollectiblesLayer = 21,
    eCollectiblesNoPlanktonLayer = 22
};

enum eSBFloatingItemType {
    eSBFloatingItemType_PlaceHolder = 0,
    eSBFloatingItemType_HappyNugget = 1,
    eSBFloatingItemType_UnhappyNugget = 2,
    eSBFloatingItemType_Health = 3,
    eSBFloatingItemType_Life = 4,
    eSBFloatingItemType_PuckAmmo = 5,
    eSBFloatingItemType_SpongeBuffPowerup = 6,
    eSBFloatingItemType_SpinPowerup = 7,
    eSBFloatingItemType_HammerPowerup = 8,
    eSBFloatingItemType_PuckPowerup = 9,
    eSBFloatingItemType_InvincibilityPowerup = 10,
    eSBFloatingItemType_Key = 11,
    eSBFloatingItemType_MemoryObject = 12,
    eSBFloatingItemType_BonusFeature = 13,
    END_eSBFloatingItemType_ENUM = 14
};

enum eAchievementType {
    eAchievementType_42 = 42
};

enum hkpContactPointAccept {
    HK_CONTACT_POINT_ACCEPT = 0,
    HK_CONTACT_POINT_REJECT = 1
};

enum hkpCollidableQualityType {
    HK_COLLIDABLE_QUALITY_KEYFRAMED_REPORTING = 9
};

enum hkpEntityActivation {
    HK_ENTITY_ACTIVATION_DO_NOT_ACTIVATE = 0,
    HK_ENTITY_ACTIVATION_DO_ACTIVATE = 1
};

extern "C" double cos(double x);
extern "C" double sin(double x);

// ---------------------------------------------------------------------------
// Assets

namespace Sext {

class EventAny {};

class uid {
public:
    // Passed where an id is wanted, a uid is converted by this inline,
    // which mwcc evaluates ahead of the call's object.
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

class vec3 {
public:
    float x;
    float y;
    float z;
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

class EventActionVector : public EventActionNew {
public:
    vec3 param0;
};

class zFloatingCollectibleAsset : public xBaseScene {
public:
    static zFloatingCollectible* Create(World::EntityHandleBase* handle,
                                        zFloatingCollectibleAsset* asset);

    LinkAsset EventLinksNew;
    unsigned char _pad0[0x20 - 0x18];
    unsigned char ModelInstance[0x40];
    vec3 Position;
    float CollectDistance;
    float SB09CollectTime;
    float SB09CollectHeight;
    uid VFXSpawn;
    uid VFXCollectSpawn;
    bool HasBalloon;
    bool UseGeneric;
    bool InitiallyHidden;
    bool RandomInitialRotation;
    unsigned int MotionType;
    unsigned char uGameName[4];
    unsigned int GameName;
    unsigned char DriveType;
    uid DrivenByObject;
    unsigned char _pad1[0xB0 - 0xA8];
};

}  // namespace Sext

// ---------------------------------------------------------------------------
// Vectors and matrices

class xVec3 {
public:
    xVec3& operator=(const xVec3& other);
    xVec3& operator+=(const xVec3& other);

    static const xVec3 m_Null;
    static const xVec3 m_UnitAxisY;

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
    xMat4x3& operator=(const xMat4x3& other);

    xVec3 pos;
    unsigned int pad3;
};

void xMat4x3Invert(xMat4x3* o, const xMat4x3* m);
void xMat4x3Mul(xMat4x3* o, const xMat4x3* a, const xMat4x3* b);
void xMat3x3RMulVec(xVec3* o, const xMat3x3* m, const xVec3* v);
void xMat3x3Tolocal(xVec3* o, const xMat3x3* m, const xVec3* v);

class xQuat {
public:
    xVec3 v;
    float s;
};

void xQuatFromMat(xQuat* q, const xMat3x3* m);
void xQuatFromAxisAngle(xQuat* q, const xVec3* axis, float angle);
void xQuatMul(xQuat* o, const xQuat* a, const xQuat* b);
void xQuatToMat(const xQuat* q, xMat3x3* m);

// Math::Vector's constructor, which the linker folded xVec3's onto.
extern "C" void __ct__Q24Math6VectorFfff(void* v, float x, float y, float z);

// Math::Matrix33's empty constructor: the linker folded the image's empty
// bodies onto it, so a call to one of them carries this name.
extern "C" void __ct__Q24Math8Matrix33Fv(void* m);

// ---------------------------------------------------------------------------
// Havok

class hkVector4 {
public:
    hkVector4& operator=(const hkVector4& v);

    // Weak in the image and called out of line: defined below its caller.
    void setNeg3(const hkVector4& v);
    void setZero4();

    float& operator()(int i) { return m_quad[i]; }

    float m_quad[4] __attribute__((aligned(16)));
};

class hkMatrix3 {
public:
    // Weak in the image and called out of line: defined below its caller.
    void setIdentity();

    // Through it, setIdentity keeps each column's address for the diagonal.
    hkVector4& getColumn(int i) { return (&m_col0)[i]; }

    hkVector4 m_col0;
    hkVector4 m_col1;
    hkVector4 m_col2;
};

class hkRotation : public hkMatrix3 {};

class hkTransform {
public:
    hkRotation& getRotation() { return m_rotation; }
    void setTranslation(const hkVector4& t) { m_translation = t; }

    hkRotation m_rotation;
    hkVector4 m_translation;
};

namespace Math {

// Sixteen-byte aligned, which is the dynamic frame alignment in the
// prologue of a function with one on its stack.
class Vector4 {
public:
    Vector4() {}

    Vector4(const xVec3& v, float w) { Assign(v.x, v.y, v.z, w); }

    Vector4& Assign(float x, float y, float z, float w);

    operator const hkVector4&() const { return *(const hkVector4*)this; }

    float v[4] __attribute__((aligned(16)));
};

}  // namespace Math

template <class ENUM, class STORAGE>
class hkEnum {
public:
    hkEnum(ENUM e) { m_storage = (STORAGE)e; }

    STORAGE m_storage;
};

class hkpMotion {
public:
    enum MotionType { MOTION_KEYFRAMED = 6 };
};

class hkpRigidBodyDeactivator {
public:
    enum DeactivatorType { DEACTIVATOR_SPATIAL = 2 };
};

class hkpRigidBodyCinfo {
public:
    enum SolverDeactivation { SOLVER_DEACTIVATION_MEDIUM = 3 };
};

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

class hkContactPoint {
public:
    const hkVector4& getSeparatingNormal() const { return m_separatingNormal; }

    hkVector4 m_position;
    hkVector4 m_separatingNormal;
};

class hkpProcessCdPoint {
public:
    hkContactPoint m_contact;
    unsigned int m_contactPointId;
};

// Havok's padding wrapper. Read through its conversion, the contact count's
// operands take retail's registers; a plain pointer swaps them (NOTES.md).
template <class T>
class hkPadSpu {
public:
    operator T() const { return m_storage; }

    T m_storage;
};

class hkpProcessCollisionData {
public:
    int getNumContactPoints() const {
        return (int)(m_firstFreeContactPoint - &m_contactPoints[0]);
    }

    hkPadSpu<hkpProcessCdPoint*> m_firstFreeContactPoint;
    hkPadSpu<void*> m_constraintOwner;
    hkpProcessCdPoint m_contactPoints[256];
};

class hkpCdBody {
public:
    const hkpCollidable* getRootCollidable() const;
};

class hkpContactPointAddedEvent {
public:
    hkpCdBody* m_bodyA;
    hkpCdBody* m_bodyB;
    int m_type;
    hkpEntity* m_callbackFiredFrom;
    void* m_contactPoint;
    void* m_gskCache;
    void* m_contactPointProperties;
    float m_projectedVelocity;
    hkpContactPointAccept m_status;
};

class hkpContactProcessEvent {
public:
    hkpCollidable* m_collidableA;
    hkpCollidable* m_collidableB;
    hkpEntity* m_callbackFiredFrom;
    hkpProcessCollisionData* m_collisionData;
};

class hkpCollisionListener {
public:
    virtual void contactPointAddedCallback(hkpContactPointAddedEvent& event);
    virtual void _h1();
    virtual void _h2();
    virtual void contactProcessCallback(hkpContactProcessEvent& event);
    // The destructor's slot, folded onto hkBaseObject's.
    virtual void _h4();
};

class hkpEntityListener {
public:
    virtual void _e0();
    virtual void entityAddedCallback(hkpEntity* entity);
    virtual void entityRemovedCallback(hkpEntity* entity);
    virtual void entityShapeSetCallback(hkpEntity* entity);
    virtual void _e4();
    virtual void entityDeletedCallback(hkpEntity* entity);
};

class hkpWorldObject {
public:
    void setUserData(void* data) { m_userData = data; }

    unsigned char _pad0[0xC];
    void* m_userData;
    unsigned char _pad1[0x2A - 0x10];
    // m_collidable.m_broadPhaseHandle.m_objectQualityType
    signed char m_objectQualityType;
};

class hkpEntity : public hkpWorldObject {
public:
    void addCollisionListener(hkpCollisionListener* cl);
    void addEntityListener(hkpEntityListener* el);

    void setQualityType(hkpCollidableQualityType type) {
        m_objectQualityType = (signed char)type;
    }
    void setProcessContactCallbackDelay(unsigned short delay) {
        m_processContactCallbackDelay = delay;
    }

    unsigned char _pad2[0xA6 - 0x2C];
    unsigned short m_processContactCallbackDelay;
};

class hkpRigidBody : public hkpEntity {
public:
    void setTransform(const hkTransform& transform);
};

hkpRigidBody* hkGetRigidBody(const hkpCollidable* collidable);
void xHavok_SetCollisionFilterInfo(hkpRigidBody* body,
                                   E_HAVOK_COLLIDE_FILTER_LAYER layer);
void xHavok_RemoveFromSimWorld(hkpRigidBody* body);
hkpRigidBody* xHavok_CreateRigidBody(
    const hkpShape* shape, hkEnum<hkpMotion::MotionType, signed char> motionType,
    float f0, float f1, float f2, unsigned int filterInfo, float f3, float f4,
    float f5,
    hkEnum<hkpRigidBodyDeactivator::DeactivatorType, signed char> deactivator,
    hkEnum<hkpRigidBodyCinfo::SolverDeactivation, signed char> solver);
void xHavok_AddToSimWorld(hkpRigidBody* body, hkpEntityActivation activation);

// Two table pointers: the entity listener's at +4.
class zCollectibleCollisionListener : public hkpCollisionListener,
                                      public hkpEntityListener {
public:
    zCollectibleCollisionListener(hkpRigidBody* rigidBody);

    virtual void contactPointAddedCallback(hkpContactPointAddedEvent& event);
    virtual void contactProcessCallback(hkpContactProcessEvent& event);
};

// ---------------------------------------------------------------------------
// The entity classes

namespace World {

class CollisionMeshBlobEntity {
public:
    hkpShape* GetPhysicsShape(int index) const;
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
};

class xOGModel {
public:
    void Show();
    void Hide();
    void UpdateRender();
    void DeferDestroy();
    void SetColorMultiplier(float r, float g, float b, float a);

    xMat4x3 Mat;
    unsigned char _pad0[0xC4 - 0x40];
    ModelInstanceArticle mModelArt;
};

class xOGModelRefPtr;

class xOGModelRef {
public:
    xOGModel* data;
    xOGModelRefPtr* autoptr;
};

class xOGModelHandle : public xOGModelRef {
public:
    // Weak in the image and called out of line: defined below its caller.
    void Destroy();
};

class EntityManager {
public:
    static void* FindAsset(unsigned long long id);
};

EntityManager* GetEntityManager();

}  // namespace World

// The slots of the entity table this unit calls or fills
// (tools/vtslot.py __vt__20zFloatingCollectible).
class EntityVirtuals {
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
    virtual void Init(Sext::zFloatingCollectibleAsset* asset);
    virtual void DebugReset();
    virtual void Render();
    virtual void SceneExit();
    virtual void Update(float dt);
    virtual void HandleEvent(xBase* from, unsigned int toEvent,
                             Sext::EventAny* params);
    virtual void Save(xSerial* s);
    virtual void Load(xSerial* s);
    virtual void Free();
};

class EmbeddedListNode {
public:
    EmbeddedListNode* next;
    EmbeddedListNode* prev;
};

typedef void (*xBaseEventCB)(xBase* from, xBase* to, unsigned int toEvent,
                             Sext::EventAny* params);

// Packed to four: the id's eight-byte alignment would otherwise round xBase
// up past the model handle the DWARF puts at +0x34.
#pragma pack(push, 4)

namespace World {

class Entity : public EntityVirtuals {
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

    xOGModelHandle ogModel;
};

}  // namespace World

#pragma pack(pop)

class xEnt : public World::xOGEntity {};

void xBaseInit(xBase* base, const Sext::xBaseAsset* asset);
void xBaseSave(xBase* base, xSerial* s);
void xBaseLoad(xBase* base, xSerial* s);
void zEntEvent(xBase* from, unsigned int fromEvent, xBase* to,
               unsigned int toEvent, Sext::EventAny* params, ForceEvent force);
xBase* zSceneFindObject(unsigned long long id);
unsigned int xStrHash(const char* str);
unsigned long long xUIDMgrFindUID(unsigned int hash);

class xSerial {
public:
    int Write_b1(int bit);
    int Read_b1(int* bit);
};

// ---------------------------------------------------------------------------
// Players, projectiles, effects

class zPlayer : public xEnt {
public:
    unsigned char _pad0[0x2EC - 0x3C];
    int eName;
};

class zPlanktonPlayer : public zPlayer {
public:
    void CollectedAmmo(unsigned int count);
};

class zSBPlayer : public zPlayer {
public:
    void ForceRunSuccessAnim();
};

class zPlayerContainer {
public:
    zPlayer* playerArray[4];
    int numPlayers;
};

class EmbeddedList {
public:
    EmbeddedListNode head;
    unsigned long size;
};

class BaseInfo {
public:
    EmbeddedList entz;
};

class zScene {
public:
    unsigned char _pad0[0x28];
    BaseInfo baseInfo[264];
};

class xGlobals {
public:
    unsigned char _pad0[0x428];
    zPlayerContainer players;
    zScene* sceneCur;
};

class zGlobals : public xGlobals {};

extern xGlobals* xglobals;
extern zGlobals globals;

class zProjectile {
public:
    unsigned char _pad0[0x58];
    xBase* owner;
};

class zProjectileHavok {
public:
    zProjectile* GetOwnerProjectile();
};

class zHintSphere {
public:
    void Rewind();
};

class zSoundWiimoteSpeakerList {
public:
    static void Play(int sound, zPlayer* player);
};

class zAchievementsMgr {
public:
    static void Award(eAchievementType type, xBase* from);

    static unsigned short ACH42_PTTurnUnhappyCount;
};

namespace FX {

class zFXSpawn {
public:
    void Init(zFXSpawn* script, xEffectAttachIntf* attach,
              World::xOGModelRef* modelRef, int bone, const xVec3* offset,
              const xVec3* orientation, const xVec3* pos, const xMat3x3* mat,
              bool oneShot);
    void SetForcedAgeRate(float rate);
};

}  // namespace FX

class zFXScriptSpawnPtMgr {
public:
    static FX::zFXSpawn* GetNewPoolSpawnPoint(const char* name);
    static void ReturnPoolSpawnPoint(FX::zFXSpawn* spawn);
};

// ---------------------------------------------------------------------------
// The collectible

class zFloatingCollectible : public World::xOGEntity {
public:
    zFloatingCollectible(World::EntityHandleBase* handle);

    static void CollisionCallback(zFloatingCollectible* obj, xBase* other,
                                  hkpContactProcessEvent& event, bool flip);

    void Instance(zFloatingCollectible* toCopy);
    virtual void Init(Sext::zFloatingCollectibleAsset* asset);
    void LoadModel();
    void Setup(bool sceneStart, const xVec3* startPos);
    void Show();
    void Reset(bool sceneStart, const xVec3* startPos);
    virtual void DebugReset();
    virtual void Render();
    virtual void SceneExit();
    virtual void Free();
    void StartPhysics();
    void StopPhysics();
    E_HAVOK_COLLIDE_FILTER_LAYER GetCollisionFilter();
    void SetInactiveFade(bool fade);
    void TurnUnhappy();
    void StartCollecting(xEnt* collector);
    void Collect();
    void StartFX(FX::zFXSpawn* fxSpawnPt_Asset, FX::zFXSpawn** fxSpawnPt);
    virtual void Update(float dt);
    void UpdateIdle(float dt);
    void Rotate(float rotScale, float dt);
    void UpdateToBeCollected(float dt);
    virtual void HandleEvent(xBase* from, unsigned int toEvent,
                             Sext::EventAny* params);
    virtual void Save(xSerial* s);
    virtual void Load(xSerial* s);
    void SetVisible(bool vis, bool fromBSP);
    virtual void DriveCauseMove(xMat4x3* parent, xMat4x3* relMat,
                                xMat4x3* world, xMat4x3* oldMat, bool moved,
                                bool rotated, float speed, float dt);

    static FX::zFXSpawn* nuggetBecomeUnhappyScriptAsset;
    static zHintSphere* UnHappyDialog;
    static int unhappyNuggetsInProgress;
    static float g_idleRotCos;
    static float g_idleRotSin;

    Sext::zFloatingCollectibleAsset* asset;
    FX::zFXSpawn* nuggetBecomeUnhappySpawnPt;
    xEnt* collector;
    bool toBeCollected;
    bool reachedCollector;
    bool collected;
    bool visible;
    bool bypass;
    float collectDist2;
    float collectVel;
    FX::zFXSpawn* fxSpawnPtIdle;
    FX::zFXSpawn* fxSpawnPtIdle_Asset;
    FX::zFXSpawn* fxSpawnPtCollect;
    FX::zFXSpawn* fxSpawnPtCollect_Asset;
    float boundRadius;
    xVec3 prevTargetDirection;
    bool dynamic;
    bool hiddenByBSP;
    bool toFree;
    hkpRigidBody* havokSimObj;
    bool falling;
    float elasticity;
    float timeUntilCollectible;
    float timeoutTimer;
    float timeout;
    float fadeoutTimer;
    float fadeout;
    xVec3 velocity;
    xVec3 depenDir;
    float depenMag;
    World::xOGEntity* depenEntity;
    bool toFade;
    bool inactiveFade;
    bool autocollect;
    bool randomCollectArc;
    zCollectibleCollisionListener* listener;
    float collectTime;
    xVec3 collectStartPos;
    float happinessTimer;
    float happinessBlend;
    eSBFloatingItemType floatingItemType;
};

typedef char _size_zFloatingCollectibleAsset
    [(sizeof(Sext::zFloatingCollectibleAsset) == 0xB0) ? 1 : -1];
typedef char _size_zFloatingCollectible
    [(sizeof(zFloatingCollectible) == 0xE0) ? 1 : -1];

// ---------------------------------------------------------------------------
// Collision

// A projectile a player fired turns the collectible to collect itself; the
// contact is rejected either way.
void zCollectibleCollisionListener::contactPointAddedCallback(
    hkpContactPointAddedEvent& event) {
    zFloatingCollectible* owner =
        (zFloatingCollectible*)event.m_callbackFiredFrom->m_userData;
    hkpRigidBody* otherBody;

    {
        hkpRigidBody* bodyA = hkGetRigidBody(event.m_bodyA->getRootCollidable());
        hkpRigidBody* bodyB = hkGetRigidBody(event.m_bodyB->getRootCollidable());

        otherBody = bodyA != event.m_callbackFiredFrom ? bodyA : bodyB;
    }

    if (otherBody != 0) {
        xBase* otherBase = (xBase*)otherBody->m_userData;

        if (otherBase != 0 && otherBase->baseType == 0x57) {
            event.m_status = HK_CONTACT_POINT_REJECT;

            zProjectile* projectile =
                ((zProjectileHavok*)otherBase)->GetOwnerProjectile();
            if (projectile->owner == xglobals->players.playerArray[0]) {
                owner->autocollect = true;
            }
        }
    }
}

zCollectibleCollisionListener::zCollectibleCollisionListener(
    hkpRigidBody* rigidBody) {
    rigidBody->addCollisionListener(this);
    rigidBody->addEntityListener(this);
}

// The deepest penetration of a falling collectible this step, and what it
// sank into when that is an entity.
void zFloatingCollectible::CollisionCallback(zFloatingCollectible* obj,
                                             xBase* other,
                                             hkpContactProcessEvent& event,
                                             bool flip) {
    if (!obj->falling) {
        return;
    }

    if (other->typeID == 0xA1) {
        return;
    }

    hkpProcessCollisionData* pCollisionData = event.m_collisionData;
    int totalContactPoints = pCollisionData->getNumContactPoints();

    for (int i = 0; i < totalContactPoints; i++) {
        hkContactPoint& contactPoint = pCollisionData->m_contactPoints[i].m_contact;
        hkVector4 separatingNormal;

        if (!flip) {
            separatingNormal = contactPoint.getSeparatingNormal();
        } else {
            separatingNormal.setNeg3(contactPoint.getSeparatingNormal());
        }

        float distFromPos = separatingNormal.m_quad[3];

        if (distFromPos < 0.0f) {
            float depenMag = -distFromPos;
            xVec3 depenDir;
            // Read z, y, x, the order retail loads them in; written in
            // place, the arguments load x first (NOTES.md).
            float nz = separatingNormal.m_quad[2];
            float ny = separatingNormal.m_quad[1];
            float nx = separatingNormal.m_quad[0];
            __ct__Q24Math6VectorFfff(&depenDir, nx, ny, nz);

            if (obj->depenMag < depenMag) {
                obj->depenMag = depenMag;
                obj->depenDir = depenDir;

                if (other->typeID == 0x56) {
                    obj->depenEntity = (World::xOGEntity*)other;
                } else {
                    obj->depenEntity = 0;
                }
            }
        }
    }
}

inline void hkVector4::setNeg3(const hkVector4& v) {
    m_quad[0] = -v.m_quad[0];
    m_quad[1] = -v.m_quad[1];
    m_quad[2] = -v.m_quad[2];
    m_quad[3] = v.m_quad[3];
}

// Hands a contact to the collectible on either side of it, unless both are.
void zCollectibleCollisionListener::contactProcessCallback(
    hkpContactProcessEvent& event) {
    hkpRigidBody* rbA = hkGetRigidBody(event.m_collidableA);
    hkpRigidBody* rbB = hkGetRigidBody(event.m_collidableB);

    xBase* baseA = 0;
    xBase* baseB = 0;

    if (rbA != 0) {
        baseA = (xBase*)rbA->m_userData;
    }

    if (rbB != 0) {
        baseB = (xBase*)rbB->m_userData;
    }

    if (baseA->baseType == 0x6D && baseB->baseType == 0x6D) {
        return;
    }

    if (baseA->baseType == 0x6D) {
        zFloatingCollectible::CollisionCallback((zFloatingCollectible*)baseA,
                                                baseB, event, false);
    } else {
        zFloatingCollectible::CollisionCallback((zFloatingCollectible*)baseB,
                                                baseA, event, true);
    }
}

// ---------------------------------------------------------------------------
// Setup, visibility, freeing

void zFloatingCollectible::Instance(zFloatingCollectible* toCopy) {
    asset = toCopy->asset;
    baseType = 0x6D;

    xBaseInit(this, asset);

    fxSpawnPtIdle_Asset = toCopy->fxSpawnPtIdle_Asset;
    fxSpawnPtCollect_Asset = toCopy->fxSpawnPtCollect_Asset;
    linkArray = toCopy->linkArray;
}

void zFloatingCollectible::Setup(bool sceneStart, const xVec3* startPos) {
    // A base's empty member, folded (see the declaration).
    __ct__Q24Math8Matrix33Fv(this);

    fxSpawnPtIdle_Asset = (FX::zFXSpawn*)zSceneFindObject(asset->VFXSpawn);

    fxSpawnPtCollect_Asset =
        (FX::zFXSpawn*)zSceneFindObject(asset->VFXCollectSpawn);

    if (nuggetBecomeUnhappyScriptAsset == 0) {
        nuggetBecomeUnhappyScriptAsset = (FX::zFXSpawn*)zSceneFindObject(
            xUIDMgrFindUID(xStrHash("SBNuggetBecomeUnhappyScriptRef")));
    }

    if (UnHappyDialog == 0) {
        UnHappyDialog = (zHintSphere*)zSceneFindObject(
            xUIDMgrFindUID(xStrHash("UID_A_BOOT_collect_unhap_nug")));
    }

    LoadModel();
    Reset(sceneStart, startPos);
}

void zFloatingCollectible::Show() {
    if (!collected && (baseFlags & 1) && visible) {
        ogModel.data->Show();
        StartFX(fxSpawnPtIdle_Asset, &fxSpawnPtIdle);
    } else {
        zFXScriptSpawnPtMgr::ReturnPoolSpawnPoint(fxSpawnPtIdle);
        zFXScriptSpawnPtMgr::ReturnPoolSpawnPoint(fxSpawnPtCollect);
        fxSpawnPtIdle = 0;
        fxSpawnPtCollect = 0;

        zFXScriptSpawnPtMgr::ReturnPoolSpawnPoint(nuggetBecomeUnhappySpawnPt);
        nuggetBecomeUnhappySpawnPt = 0;

        StopPhysics();

        ogModel.data->Hide();
    }
}

void zFloatingCollectible::DebugReset() {
    asset = (Sext::zFloatingCollectibleAsset*)World::GetEntityManager()->FindAsset(
        asset->id);
    linkArray = &asset->EventLinksNew;

    Reset(false, 0);
}

void zFloatingCollectible::Render() {
    ogModel.data->UpdateRender();
}

void zFloatingCollectible::SceneExit() {
    Free();
}

void zFloatingCollectible::Free() {
    zFXScriptSpawnPtMgr::ReturnPoolSpawnPoint(fxSpawnPtIdle);
    zFXScriptSpawnPtMgr::ReturnPoolSpawnPoint(fxSpawnPtCollect);
    fxSpawnPtIdle = 0;
    fxSpawnPtCollect = 0;

    zFXScriptSpawnPtMgr::ReturnPoolSpawnPoint(nuggetBecomeUnhappySpawnPt);
    nuggetBecomeUnhappySpawnPt = 0;

    DriveDetach();

    ogModel.Destroy();

    StopPhysics();
}

inline void World::xOGModelHandle::Destroy() {
    if (data != 0) {
        data->DeferDestroy();
        data = 0;
    }
}

// A keyframed body from the model's collision mesh, placed at the model and
// reporting its contacts to a listener.
void zFloatingCollectible::StartPhysics() {
    hkpShape* pNewShape =
        ogModel.data->mModelArt.protoEnt->collmeshBlob->GetPhysicsShape(0);
    // Through a reference the builder is loaded ahead of the scale's Assign,
    // as retail loads it.
    CHavokShapeBuilder& builder = hkSingleton<CHavokShapeBuilder>::getInstance();
    hkpShape* scaled_shape =
        builder.getShape(pNewShape, Math::Vector4().Assign(1.0f, 1.0f, 1.0f, 0.0f));

    E_HAVOK_COLLIDE_FILTER_LAYER collisionLayer = GetCollisionFilter();
    havokSimObj = xHavok_CreateRigidBody(
        scaled_shape, hkpMotion::MOTION_KEYFRAMED, 1.0f, 1.0f, 0.0f,
        collisionLayer, -1.0f, -1.0f, -1.0f,
        hkpRigidBodyDeactivator::DEACTIVATOR_SPATIAL,
        hkpRigidBodyCinfo::SOLVER_DEACTIVATION_MEDIUM);

    listener = new zCollectibleCollisionListener(havokSimObj);

    havokSimObj->setProcessContactCallbackDelay(0);

    const xVec3& position = ogModel.data->Mat.pos;
    hkTransform transform;
    transform.getRotation().setIdentity();
    // Built from the position by a constructor: the components load in
    // order and the temporary itself is passed on (NOTES.md).
    transform.setTranslation(Math::Vector4(position, 0.0f));
    havokSimObj->setTransform(transform);

    havokSimObj->setUserData(this);
    havokSimObj->setQualityType(HK_COLLIDABLE_QUALITY_KEYFRAMED_REPORTING);

    xHavok_AddToSimWorld(havokSimObj, HK_ENTITY_ACTIVATION_DO_NOT_ACTIVATE);
}

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

// ---------------------------------------------------------------------------
// Physics, fading, happiness

void zFloatingCollectible::StopPhysics() {
    if (havokSimObj != 0) {
        xHavok_RemoveFromSimWorld(havokSimObj);
        havokSimObj = 0;
    }
}

E_HAVOK_COLLIDE_FILTER_LAYER zFloatingCollectible::GetCollisionFilter() {
    if (falling) {
        return floatingItemType == eSBFloatingItemType_HappyNugget
                   ? eCollectiblesLayer
                   : eCollectiblesNoPlanktonLayer;
    } else {
        return floatingItemType == eSBFloatingItemType_HappyNugget
                   ? eStaticCollectiblesLayer
                   : eNoCollisionLayer;
    }
}

void zFloatingCollectible::SetInactiveFade(bool fade) {
    if (fade == inactiveFade) {
        return;
    }

    inactiveFade = fade;

    if (toFade) {
        return;
    }

    if (inactiveFade) {
        ogModel.data->SetColorMultiplier(1.0f, 1.0f, 1.0f, 0.5f);
    } else if (floatingItemType == eSBFloatingItemType_Health) {
        ogModel.data->SetColorMultiplier(1.0f, 1.0f, 1.0f, 0.99f);
    } else {
        ogModel.data->SetColorMultiplier(1.0f, 1.0f, 1.0f, 1.0f);
    }
}

// A happy nugget sours: its script plays, the achievement counts it, and
// whatever the type was it is an unhappy nugget from here.
void zFloatingCollectible::TurnUnhappy() {
    if (floatingItemType == eSBFloatingItemType_HappyNugget) {
        if (nuggetBecomeUnhappyScriptAsset != 0) {
            zFXScriptSpawnPtMgr::ReturnPoolSpawnPoint(nuggetBecomeUnhappySpawnPt);
            nuggetBecomeUnhappySpawnPt = zFXScriptSpawnPtMgr::GetNewPoolSpawnPoint(
                "FLOATING_COLLECTIBLE_TURN_UNHAPPY");

            if (nuggetBecomeUnhappySpawnPt != 0) {
                nuggetBecomeUnhappySpawnPt->Init(nuggetBecomeUnhappyScriptAsset,
                                                 0, &ogModel, -1, 0, 0, 0, 0,
                                                 false);
            }
        }

        unhappyNuggetsInProgress++;

        zAchievementsMgr::ACH42_PTTurnUnhappyCount++;
        if (zAchievementsMgr::ACH42_PTTurnUnhappyCount >= 100) {
            zAchievementsMgr::Award(eAchievementType_42, 0);
        }
    }

    floatingItemType = eSBFloatingItemType_UnhappyNugget;
    happinessTimer = 5.0f;
    timeoutTimer = 0.0f;
    timeUntilCollectible = 0.5f;

    if (havokSimObj != 0) {
        xHavok_SetCollisionFilterInfo(havokSimObj, GetCollisionFilter());
    }
}

// ---------------------------------------------------------------------------
// Collecting

void zFloatingCollectible::Collect() {
    if (collector->baseType == 0x55) {
        zPlayer* player = (zPlayer*)collector;

        if (player->eName == 8) {
            ((zPlanktonPlayer*)player)->CollectedAmmo(1);
            unhappyNuggetsInProgress--;

            if (UnHappyDialog != 0) {
                UnHappyDialog->Rewind();
            }
        } else if (player->eName == 6) {
            if (floatingItemType == eSBFloatingItemType_Health ||
                floatingItemType == eSBFloatingItemType_Life) {
                ((zSBPlayer*)player)->ForceRunSuccessAnim();
            }

            zEntEvent(collector, 0, this, 0xE6C20347, 0, FE_NO);
        }

        switch (floatingItemType) {
        case eSBFloatingItemType_Life:
            zSoundWiimoteSpeakerList::Play(8, player);
            break;
        case eSBFloatingItemType_Key:
            zSoundWiimoteSpeakerList::Play(5, player);
            break;
        case eSBFloatingItemType_Health:
            zSoundWiimoteSpeakerList::Play(7, player);
            break;
        case eSBFloatingItemType_SpongeBuffPowerup:
        case eSBFloatingItemType_SpinPowerup:
        case eSBFloatingItemType_HammerPowerup:
        case eSBFloatingItemType_PuckPowerup:
        case eSBFloatingItemType_InvincibilityPowerup:
        case eSBFloatingItemType_MemoryObject:
        case eSBFloatingItemType_BonusFeature:
            zSoundWiimoteSpeakerList::Play(4, player);
            break;
        }
    }

    zEntEvent(collector, 0, this, 0xDB02C485, 0, FE_NO);

    collected = true;

    zFXScriptSpawnPtMgr::ReturnPoolSpawnPoint(fxSpawnPtIdle);
    zFXScriptSpawnPtMgr::ReturnPoolSpawnPoint(fxSpawnPtCollect);
    fxSpawnPtIdle = 0;
    fxSpawnPtCollect = 0;

    zFXScriptSpawnPtMgr::ReturnPoolSpawnPoint(nuggetBecomeUnhappySpawnPt);
    nuggetBecomeUnhappySpawnPt = 0;

    ogModel.data->Hide();
    visible = false;

    if (dynamic) {
        Free();
        toFree = true;
    }
}

void zFloatingCollectible::StartFX(FX::zFXSpawn* fxSpawnPt_Asset,
                                   FX::zFXSpawn** fxSpawnPt) {
    if (fxSpawnPt_Asset != 0 && *fxSpawnPt == 0) {
        *fxSpawnPt = zFXScriptSpawnPtMgr::GetNewPoolSpawnPoint("FloatingCollectible");

        if (*fxSpawnPt != 0) {
            (*fxSpawnPt)->Init(fxSpawnPt_Asset, 0, &ogModel, -1, 0, 0, 0, 0,
                               false);

            float forceAgeRate = 3.0f;
            (*fxSpawnPt)->SetForcedAgeRate(forceAgeRate);
        }
    }
}

// ---------------------------------------------------------------------------
// Construction, turning, saving

zFloatingCollectible::zFloatingCollectible(World::EntityHandleBase* handle)
    : World::xOGEntity(handle), dynamic(true) {
    havokSimObj = 0;
}

void zFloatingCollectible::Rotate(float rotScale, float dt) {
    xMat4x3& modelMat = ogModel.data->Mat;

    xQuat currentRot;
    xQuatFromMat(&currentRot, &modelMat);

    xQuat dRot;
    float dRotAngle = dt * (6.2831855f * rotScale);
    xQuatFromAxisAngle(&dRot, &xVec3::m_UnitAxisY, dRotAngle);

    xQuatMul(&currentRot, &currentRot, &dRot);
    xQuatToMat(&currentRot, &modelMat);
}

void zFloatingCollectible::HandleEvent(xBase* from, unsigned int toEvent,
                                       Sext::EventAny* params) {
    switch (toEvent) {
    case 0x0AA2A302:
        if (visible) {
            StartCollecting(xglobals->players.playerArray[0]);
        }
        break;

    case 0x389E01C0:
        DebugReset();
        break;

    case 0x0B580CF4:
        Show();
        break;

    case 0xA8B93047:
        Reset(false, 0);
        break;

    case 0xD576CA21:
        if (asset->DriveType == 1) {
            World::xOGEntity* drivingObject =
                (World::xOGEntity*)zSceneFindObject(asset->DrivenByObject);

            if (drivingObject != 0) {
                zEntEvent(this, 0, drivingObject, 0x3FE52B13, 0, FE_NO);
            }
        }
        break;

    case 0x27858BA2:
    case 0x34716A29:
        if (baseFlags & 1) {
            SetVisible(true, false);
        }
        break;

    case 0xAE72E9E5:
    case 0xD6094F29:
        SetVisible(false, false);
        break;

    case 0x3FE52B13: {
        Sext::EventActionDrivenBy* paramst = (Sext::EventActionDrivenBy*)params;
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

    case 0x98D2A517: {
        Sext::EventActionVector* paramst = (Sext::EventActionVector*)params;
        xVec3 vel;
        vel.x = paramst->param0.x;
        vel.y = paramst->param0.y;
        vel.z = paramst->param0.z;

        if (!falling) {
            falling = true;

            if (havokSimObj != 0) {
                xHavok_SetCollisionFilterInfo(havokSimObj, GetCollisionFilter());
            }
        }

        velocity = vel;
        elasticity = 0.7f;

        DriveDetach();
        break;
    }
    }
}

void zFloatingCollectible::Save(xSerial* s) {
    xBaseSave(this, s);

    if (collected) {
        s->Write_b1(1);
    } else {
        s->Write_b1(0);
    }
}

void zFloatingCollectible::Load(xSerial* s) {
    xBaseLoad(this, s);

    int b = 0;
    s->Read_b1(&b);
    collected = b;

    SetVisible((baseFlags & 1) && !collected && !asset->InitiallyHidden, false);
}

// Hidden by the BSP, a collectible remembers it, and only the BSP shows it
// again.
void zFloatingCollectible::SetVisible(bool vis, bool fromBSP) {
    if (vis && fromBSP && !hiddenByBSP) {
        return;
    }

    hiddenByBSP = false;

    if (!vis && visible) {
        hiddenByBSP = fromBSP;
    }

    if (visible != vis) {
        visible = vis;
        Show();
    }
}

zFloatingCollectible* Sext::zFloatingCollectibleAsset::Create(
    World::EntityHandleBase* handle, zFloatingCollectibleAsset* asset) {
    zFloatingCollectible* entity =
        new (memset(Memory::AllocGlobalHeap(sizeof(zFloatingCollectible),
                                            (Memory::GlobalHeapEnum)0,
                                            (eMemMgrTag)16, false),
                    0, sizeof(zFloatingCollectible))) zFloatingCollectible(handle);

    entity->Init(asset);

    return entity;
}

// The idle turn shared by every collectible this frame, then each enabled
// one's update.
void zFloatingCollectibleUpdate(float dt) {
    float dRotAngle = 4.712389f * dt;

    zFloatingCollectible::g_idleRotCos = cos(dRotAngle);
    zFloatingCollectible::g_idleRotSin = sin(dRotAngle);

    EmbeddedListNode* end = &globals.sceneCur->baseInfo[0x6D].entz.head;
    for (EmbeddedListNode* node = end->next; node != end; node = node->next) {
        xBase* base = (xBase*)((char*)node - 4);

        if (base->baseFlags & 1) {
            // Through a double, which retail rounds back to single.
            base->Update((double)dt);
        }
    }
}

// Carried by what drives it. The position is not the one worked out: the
// moved matrix's own position is written, as retail does.
void zFloatingCollectible::DriveCauseMove(xMat4x3* parent, xMat4x3* relMat,
                                          xMat4x3* world, xMat4x3* oldMat,
                                          bool moved, bool rotated,
                                          float speed, float dt) {
    xMat4x3 InvRel;
    xMat4x3Invert(&InvRel, relMat);

    xMat4x3 tempMat, baseMat;
    xMat4x3Mul(&baseMat, relMat, parent);
    tempMat = *oldMat;

    xVec3 R = xVec3::m_Null;

    xMat3x3Tolocal(&R, &tempMat, &R);

    xMat4x3Mul(&tempMat, &tempMat, &baseMat);

    xMat3x3RMulVec(&R, &tempMat, &R);

    R += tempMat.pos;
    ogModel.data->Mat.pos = tempMat.pos;
}
