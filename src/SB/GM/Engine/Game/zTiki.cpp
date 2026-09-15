#include "SB/GM/Engine/Game/zTiki.pool.h"

// zTiki.cpp -- the tiki: a breakable totem that sits, hovers or falls,
// explodes or fills with fluid, and rides whatever drives it. Read from the
// image with tools/brief.py; the layouts are the DWARF's
// (tools/dwarf_types.py), the virtual slots the image's (tools/vtslot.py).

class xAnimState;
class xAnimTransition;
class xBase;
class hkpCollidable;
class hkpContactPointRemovedEvent;
class zTiki;

namespace World {
class EntityHandleBase;
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
    eKeyFrameObjLayer = 3,
    eTikisLayer = 24
};

enum hkpEntityActivation {
    HK_ENTITY_ACTIVATION_DO_NOT_ACTIVATE = 0,
    HK_ENTITY_ACTIVATION_DO_ACTIVATE = 1
};

enum BoardPowerupState {
    eBoardPowerup_None = 0,
    eBoardPowerup_Spongebuff = 1
};

enum enState {
    Idle = 0,
    IdleActive = 1,
    Hit = 2,
    Death = 3,
    FluidFill = 4,
    Unactive = 5
};

enum TikiType {
    Regular = 0,
    Explosive = 1,
    Hover = 2,
    Hint = 3,
    Bonus = 4,
    Fluid = 5,
    END_TikiTypeENUM = 6
};

// ---------------------------------------------------------------------------
// Assets and events

namespace Sext {

enum eHitSource {
    eHitSourceHAMMER_ATTACK = 29,
    eHitSourceHAMMER_SPONGEBUFF_ATTACK = 33
};

class EventAny {};
class EventActionNew : public EventAny {};

class EventActionDestructableFlag : public EventActionNew {
public:
    bool sentByDestructable;
};

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

class Rotation3 {
public:
    float yaw;
    float pitch;
    float roll;
};

class vec3 {
public:
    float x;
    float y;
    float z;
};

class LinkAsset {
public:
    unsigned int count;
    void* data;
};

}  // namespace Sext

// ---------------------------------------------------------------------------
// Vectors, matrices, animation

class xVec3 {
public:
    xVec3& operator=(const xVec3& other);
    xVec3& operator+=(const xVec3& other);

    static const xVec3 m_Null;

    float x;
    float y;
    float z;
};

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

void xMat3x3Euler(xMat3x3* m, float yaw, float pitch, float roll);
void xMat4x3Invert(xMat4x3* out, const xMat4x3* in);
void xMat3x3GetEuler(const xMat3x3* m, xVec3* euler);
void xMat3x3RMulVec(xVec3* o, const xMat3x3* m, const xVec3* v);
void xMat4x3Mul(xMat4x3* o, const xMat4x3* a, const xMat4x3* b);
xVec3 operator+(const xVec3& a, const xVec3& b);
xVec3 operator*(const xVec3& v, float s);
xVec3 ConstructxVec3(float x, float y, float z);

class xQuat {
public:
    xVec3 v;
    float s;
};

void xQuatFromMat(xQuat* q, const xMat3x3* m);

// Math::Vector's constructor, which the linker folded xVec3's onto.
extern "C" void __ct__Q24Math6VectorFfff(void* v, float x, float y, float z);

class xAnimSingle {
public:
    unsigned int SingleFlags;
    xAnimState* State;
};

class xAnimTable {
public:
    xAnimTable* Next;
    char* Name;
    xAnimTransition* TransitionList;
    xAnimState* StateList;
};

class xAnimPlay {
public:
    xAnimPlay* Next;
    unsigned short NumSingle;
    unsigned short BoneCount;
    unsigned short MorphCount;
    unsigned short padding0;
    xAnimSingle* Single;
    void* Object;
    xAnimTable* Table;
};

void xAnimPlaySetState(xAnimSingle* single, xAnimState* state, float time);

class xModelInstance {
public:
    xMat4x3 Mat;
    xVec3 Scale;
    xAnimPlay* Anim;
};

class xSerial {
public:
    int Write_b1(int bit);
    int Read_b1(int* bit);
};

// ---------------------------------------------------------------------------
// The entity classes

namespace World {

class ModelInstanceAsset {
public:
    unsigned char _pad0[0x40];
};

class xOGModel : public xModelInstance {
public:
    void Show();
    void Hide();
    void SetModelBlendFactor(float blend, unsigned char overrideAnimTexMerge);
};

class xOGModelHandle {
public:
    void SwapRenderModel(const ModelInstanceAsset& asset);

    xOGModel* GetModel() const { return data; }

    xOGModel* data;
    void* autoptr;
};

class EntityManager {
public:
    static void* FindAsset(unsigned long long id);
};

EntityManager* GetEntityManager();

}  // namespace World

// The slots of the entity table this unit calls through: DriveDetach 10,
// Init 25, HandleEvent 27, GetSceneInitCollisionFilter 29
// (tools/vtslot.py __vt__5zTiki).
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
    virtual void _v9();
    virtual void DriveDetach();
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
    virtual void Init(void* asset);
    virtual void _v26();
    virtual void HandleEvent(xBase* from, unsigned int to_event,
                             Sext::EventAny* params);
    virtual void _v28();
    virtual E_HAVOK_COLLIDE_FILTER_LAYER GetSceneInitCollisionFilter();
};

typedef void (*xBaseEventCB)(xBase* from, xBase* to, unsigned int to_event,
                             Sext::EventAny* params);

// Packed to four: the id's eight-byte alignment would otherwise round xBase
// up past the model handle the DWARF puts at +0x34.
#pragma pack(push, 4)

class xBase : public EntityVirtuals {
public:
    unsigned char _pad0[0x14 - 0x4];
    World::EntityHandleBase* handle;
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
    xOGModelHandle ogModel;
};

}  // namespace World

#pragma pack(pop)

class xEffectAttachIntf : public World::xOGEntity {
public:
    xEffectAttachIntf(World::EntityHandleBase* handle);
};

void xBaseReset(xBase* base, Sext::xBaseAsset* asset);
void xBaseSave(xBase* base, xSerial* s);
void xBaseLoad(xBase* base, xSerial* s);
void zEntEvent(xBase* from, unsigned int fromEvent, xBase* to,
               unsigned int toEvent, Sext::EventAny* param, ForceEvent force);

// ---------------------------------------------------------------------------
// Physics

class hkpCollisionListener {
public:
    virtual void _h0();
    virtual void _h1();
    virtual void contactPointRemovedCallback(hkpContactPointRemovedEvent& event);
    virtual void contactProcessCallback(class hkpContactProcessEvent& event);
};

class hkpContactProcessEvent {
public:
    hkpCollidable* m_collidableA;
    hkpCollidable* m_collidableB;
};

class hkpEntity {
public:
    void removeCollisionListener(hkpCollisionListener* cl);

    unsigned char _pad0[0xC];
    void* m_userData;
};

class hkpRigidBody : public hkpEntity {};

class hkVector4 {
public:
    float x;
    float y;
    float z;
    float w;
};

class hkQuaternion {
public:
    hkVector4 m_vec;
};

namespace Math {

// Sixteen-byte aligned, which is the dynamic frame alignment in the
// prologue of a function with one on its stack.
class Vector4 {
public:
    Vector4() {}

    Vector4& Assign(float x, float y, float z, float w);

    operator const hkVector4&() const { return *(const hkVector4*)this; }
    operator const hkQuaternion&() const { return *(const hkQuaternion*)this; }

    float v[4] __attribute__((aligned(16)));
};

}  // namespace Math

void xHavok_UpdateRigidBodyMotion(hkpRigidBody* body, const hkVector4& pos,
                                  const hkQuaternion& rot, float dt);

hkpRigidBody* hkGetRigidBody(const hkpCollidable* collidable);
void xHavok_SetCollisionFilterInfo(hkpRigidBody* body,
                                   E_HAVOK_COLLIDE_FILTER_LAYER layer);
void xHavok_AddToSimWorld(hkpRigidBody* body, hkpEntityActivation activation);
void xHavok_RemoveFromSimWorld(hkpRigidBody* body);

class zTikiCollisionListener : public hkpCollisionListener {
public:
    zTikiCollisionListener();

    virtual void contactPointRemovedCallback(hkpContactPointRemovedEvent& event);
    virtual void contactProcessCallback(hkpContactProcessEvent& event);

    zTiki* owner;
};

// ---------------------------------------------------------------------------
// Others

class zDestructible {
public:
    bool active;
    bool swapModel;
    bool didSwapModel;
    void* asset;
    void* states;
    World::xOGEntity* root_ent;
    void* modelInstanceSwap;
    unsigned int totalStateNum;
    unsigned int totalHitPoints;
    unsigned int hitTypeFilter;
};

void zDestructible_Reset(zDestructible*& destructible, World::xOGEntity* ent);

class zBoardPlayer {
public:
    static zBoardPlayer* GetInstance();

    unsigned char _pad0[0x8B0];
    BoardPowerupState powerupModelState;
};

namespace HitFilterNS {
unsigned int TranslateHitSource(Sext::eHitSource hitSource);
}

class zNeoDrivenLink {
public:
    static void AddChild(World::xOGEntity* parent, World::xOGEntity* child,
                         xMat4x3* mat, unsigned int flags, float yaw, int bone);
};

namespace Sext {

class zTikiAsset : public xBaseAsset {
public:
    static zTiki* Create(World::EntityHandleBase* handle, zTikiAsset* asset);

    Rotation3 Orientation;
    vec3 Pos;
    vec3 Scale;
    unsigned char _pad0[0x40 - 0x34];
    World::ModelInstanceAsset modelInstance;
    TikiType type;
    bool hover;
    unsigned char _pad1[0xD0 - 0x85];
    LinkAsset EventLinksNew;
};

}  // namespace Sext

// ---------------------------------------------------------------------------
// The tiki

class zTiki : public xEffectAttachIntf {
public:
    zTiki(World::EntityHandleBase* handle) : xEffectAttachIntf(handle) {}

    static unsigned int anIdleCheck(xAnimTransition* tran, xAnimSingle* anim,
                                    void* data);
    static unsigned int anHitCheck(xAnimTransition* tran, xAnimSingle* anim,
                                   void* data);
    static unsigned int anIdleActiveCheck(xAnimTransition* tran,
                                          xAnimSingle* anim, void* data);
    static unsigned int anFluidFillCheck(xAnimTransition* tran,
                                         xAnimSingle* anim, void* data);

    unsigned int HitCheck(xAnimTransition* tran, xAnimSingle* anim);

    virtual void Init(void* asset);
    virtual void HandleEvent(xBase* from, unsigned int to_event,
                             Sext::EventAny* params);
    virtual E_HAVOK_COLLIDE_FILTER_LAYER GetSceneInitCollisionFilter();

    xVec3 GetCenter();
    void InitPhysics();
    void StartPhysics();
    void StopPhysics();
    void Show();
    bool IsAnimated();
    bool IsAnimatedThisFrame();
    void Reset();
    void DebugReset();
    void TikiSave(xSerial* s);
    void TikiLoad(xSerial* s);
    void DeactivateTiki(bool silent);
    void CollisionCallback(xBase* other, hkpRigidBody* otherRigidBody,
                           hkpContactProcessEvent& event, bool flip);
    bool CheckHammerRecoil();
    void SetBSPVisible(bool bspVisible);
    bool DrivePrep(World::xOGEntity* driver);
    void DriveCauseMove(xMat4x3* parent, xMat4x3* local, xMat4x3* world,
                        xMat4x3* oldMat, bool moved, bool rotated,
                        float speed, float dt);
    void DriveAttach(World::xOGEntity* passenger, unsigned int flags, int bone);

    enState state;
    float sqrDistActivation;
    float sqrDistExplosionActivation;
    TikiType type;
    zDestructible* destructible;
    float explodeCountdown;
    Sext::zTikiAsset* asset;
    xVec3 accumRelPos;
    float fallWaitTimer;
    float ySpeed;
    float depen;
    xVec3 depenEntitySurfaceVel;
    World::xOGEntity* depenEntity;
    World::xOGEntity* driverEntity;
    bool settled;
    bool isOnDamageSurface;
    bool isHovering;
    bool destroyedByPlankton;
    bool tikiCollisionOn;
    bool bspOn;
    bool visible;
    bool requiresPhysicsFreeze;
    bool isDriving;
    bool deactivatedOnce;
    hkpRigidBody* physicsObject;
    zTikiCollisionListener tikiListener;
    float hitTimer;
};

// ---------------------------------------------------------------------------
// Creation, events, animation callbacks

zTiki* Sext::zTikiAsset::Create(World::EntityHandleBase* handle,
                                zTikiAsset* asset) {
    zTiki* tiki = new (memset(Memory::AllocGlobalHeap(sizeof(zTiki),
                                                      (Memory::GlobalHeapEnum)0,
                                                      (eMemMgrTag)16, false),
                              0, sizeof(zTiki))) zTiki(handle);

    tiki->Init(asset);

    return tiki;
}

void zTikiEventWrapper(xBase* from, xBase* to, unsigned int to_event,
                       Sext::EventAny* params) {
    ((zTiki*)to)->HandleEvent(from, to_event, params);
}

// A hit plays once: the transition clears it back to idle.
unsigned int zTiki::HitCheck(xAnimTransition* tran, xAnimSingle* anim) {
    if (state == Hit) {
        state = Idle;
        return 1;
    }

    return 0;
}

void ExplodeCB(xAnimPlay* play, xAnimState* st, void* data) {
    zTiki* tiki = (zTiki*)data;

    if (tiki->state != Death) {
        Sext::EventActionDestructableFlag params;
        params.sentByDestructable = false;
        zEntEvent(0, 0, tiki, 0x8662F06A, &params, FE_NO);
    }
}

unsigned int zTiki::anIdleActiveCheck(xAnimTransition* tran, xAnimSingle* anim,
                                      void* data) {
    return ((zTiki*)data)->state == IdleActive;
}

unsigned int zTiki::anHitCheck(xAnimTransition* tran, xAnimSingle* anim,
                               void* data) {
    return ((zTiki*)data)->HitCheck(tran, anim);
}

unsigned int zTiki::anFluidFillCheck(xAnimTransition* tran, xAnimSingle* anim,
                                     void* data) {
    return ((zTiki*)data)->state == FluidFill;
}

unsigned int zTiki::anIdleCheck(xAnimTransition* tran, xAnimSingle* anim,
                                void* data) {
    return ((zTiki*)data)->state == Idle || ((zTiki*)data)->state == Unactive;
}

// ---------------------------------------------------------------------------
// Physics and visibility

E_HAVOK_COLLIDE_FILTER_LAYER zTiki::GetSceneInitCollisionFilter() {
    return isHovering ? eKeyFrameObjLayer : eTikisLayer;
}

inline xVec3 operator*(float f, const xVec3& v) {
    return ConstructxVec3(v.x * f, v.y * f, v.z * f);
}

// NEAR MISS: 33 of 40 words, and the whole of it is one instruction:
// retail loads ogModel.data again for the `+=` after copying the position
// into center, ours keeps the pointer from the copy, so every later word is
// one early. The scale as a non-const member or with the float on the left,
// and an inline accessor on either use or both, all measure the same.
xVec3 zTiki::GetCenter() {
    xVec3 center = ogModel.data->Mat.pos;
    center += 0.5f * (ogModel.data->Mat.up * ogModel.data->Scale.y);
    return center;
}

void zTiki::StartPhysics() {
    if (physicsObject == 0) {
        InitPhysics();
        xHavok_SetCollisionFilterInfo(physicsObject,
                                      GetSceneInitCollisionFilter());
        xHavok_AddToSimWorld(physicsObject, HK_ENTITY_ACTIVATION_DO_NOT_ACTIVATE);
    }
}

void zTiki::StopPhysics() {
    if (physicsObject != 0) {
        if (!isHovering) {
            physicsObject->removeCollisionListener(&tikiListener);
        }

        xHavok_RemoveFromSimWorld(physicsObject);
        physicsObject = 0;
    }
}

void zTiki::Show() {
    if (visible && bspOn && state != Unactive) {
        ogModel.data->Show();
    }
}

bool zTiki::IsAnimated() {
    return type == Regular || type == Hover || type == Fluid;
}

bool zTiki::IsAnimatedThisFrame() {
    if (type == Hover || type == Fluid || (type == Regular && hitTimer > 0.0f)) {
        return true;
    }

    return false;
}

// ---------------------------------------------------------------------------
// Reset, save and load

void zTiki::Reset() {
    state = Idle;
    hitTimer = 0.0f;

    if (IsAnimated() && ogModel.data->Anim != 0) {
        xAnimPlaySetState(ogModel.data->Anim->Single,
                          ogModel.data->Anim->Table->StateList, 0.0f);
    }

    if (destructible != 0) {
        if (destructible->didSwapModel) {
            ogModel.SwapRenderModel(asset->modelInstance);
        }

        zDestructible_Reset(destructible, this);
    }

    tikiCollisionOn = true;
    StopPhysics();

    xBaseReset(this, asset);

    xMat3x3Euler(&ogModel.data->Mat, asset->Orientation.yaw,
                 asset->Orientation.pitch, asset->Orientation.roll);
    __ct__Q24Math6VectorFfff(&ogModel.data->Mat.pos, asset->Pos.x, asset->Pos.y,
                             asset->Pos.z);

    ogModel.data->Scale.x = asset->Scale.x;
    ogModel.data->Scale.y = asset->Scale.y;
    ogModel.data->Scale.z = asset->Scale.z;

    accumRelPos = xVec3::m_Null;

    fallWaitTimer = 0.0f;

    explodeCountdown = 0.0f;
    ySpeed = 0.0f;

    depen = 0.0f;
    depenEntitySurfaceVel = xVec3::m_Null;
    depenEntity = 0;
    driverEntity = 0;
    settled = false;
    isOnDamageSurface = false;

    DriveDetach();

    isHovering = type == Hover || asset->hover;

    destroyedByPlankton = false;

    visible = true;
    ogModel.data->Show();
    requiresPhysicsFreeze = false;

    if (type == Regular || type == Explosive) {
        ogModel.data->SetModelBlendFactor(1.0f, 1);
    }

    deactivatedOnce = false;
}

void zTiki::DebugReset() {
    asset = (Sext::zTikiAsset*)World::GetEntityManager()->FindAsset(asset->id);
    linkArray = &asset->EventLinksNew;

    Reset();
}

void zTiki::TikiSave(xSerial* s) {
    xBaseSave(this, s);

    if (state == Unactive) {
        s->Write_b1(1);
    } else {
        s->Write_b1(0);
    }
}

void zTiki::TikiLoad(xSerial* s) {
    xBaseLoad(this, s);

    int b = 0;
    s->Read_b1(&b);

    if (b) {
        DeactivateTiki(true);
    }
}

// ---------------------------------------------------------------------------
// Hits, visibility, driving, collision

// True when a hammer should bounce off: always for a fluid tiki, and for a
// destructible one whose filter does not take this hammer.
bool zTiki::CheckHammerRecoil() {
    if (type == Fluid) {
        return true;
    }

    if (destructible != 0) {
        Sext::eHitSource hitSource = Sext::eHitSourceHAMMER_ATTACK;
        if (zBoardPlayer::GetInstance()->powerupModelState ==
            eBoardPowerup_Spongebuff) {
            hitSource = Sext::eHitSourceHAMMER_SPONGEBUFF_ATTACK;
        }

        if (!(destructible->hitTypeFilter &
              HitFilterNS::TranslateHitSource(hitSource))) {
            return true;
        }
    }

    return false;
}

void zTiki::SetBSPVisible(bool bspVisible) {
    bspOn = bspVisible;

    if (bspVisible) {
        Show();
    } else {
        ogModel.data->Hide();
    }

    if (bspOn && state != Unactive && tikiCollisionOn) {
        StartPhysics();
    } else {
        StopPhysics();
    }
}

bool zTiki::DrivePrep(World::xOGEntity* driver) {
    accumRelPos = xVec3::m_Null;
    return false;
}

// NEAR MISS: 5 of 72 words; retail loads the model for xQuatFromMat's
// argument and again into r30 after the call, ours loads it into r30 before
// the call and passes that. The four-vectors are temporaries with Vector4's
// empty constructor (value-initialised ones zero-fill), and the moved model
// is read through the accessor so its object is loaded before the `+`.
// The position's reference after the call swaps r30 and r31 (10 words), as
// does a pointer declared early and assigned late (10).
void zTiki::DriveCauseMove(xMat4x3* parent, xMat4x3* local, xMat4x3* world,
                           xMat4x3* oldMat, bool moved, bool rotated,
                           float speed, float dt) {
    xMat4x3 baseMat;

    xMat4x3Mul(&baseMat, local, parent);

    ogModel.GetModel()->Mat.pos = accumRelPos + oldMat->pos;
    xMat3x3RMulVec(&ogModel.data->Mat.pos, &baseMat, &ogModel.data->Mat.pos);
    ogModel.data->Mat.pos += baseMat.pos;

    if (physicsObject != 0) {
        const xVec3& pos = ogModel.data->Mat.pos;
        xQuat quat;

        xQuatFromMat(&quat, &ogModel.data->Mat);
        xHavok_UpdateRigidBodyMotion(physicsObject,
                                     Math::Vector4().Assign(pos.x, pos.y, pos.z, 0.0f),
                                     Math::Vector4().Assign(quat.v.x, quat.v.y, quat.v.z, quat.s), dt);

        requiresPhysicsFreeze = true;
    }
}

void zTiki::DriveAttach(World::xOGEntity* passenger, unsigned int flags,
                        int bone) {
    xMat4x3 D;

    xMat4x3Invert(&D, &ogModel.data->Mat);

    xVec3 euler;
    xMat3x3GetEuler(&ogModel.data->Mat, &euler);

    zNeoDrivenLink::AddChild(this, passenger, &D, flags, euler.x, -1);

    isDriving = true;
}

void zTikiCollisionListener::contactPointRemovedCallback(
    hkpContactPointRemovedEvent& event) {
    owner->deactivatedOnce = false;
}

// Hands a contact to the tiki whichever side of it the tiki is on.
void zTikiCollisionListener::contactProcessCallback(
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

    if (baseA == owner) {
        owner->CollisionCallback(baseB, rbB, event, false);
    } else if (baseB == owner) {
        owner->CollisionCallback(baseA, rbA, event, true);
    }
}

zTikiCollisionListener::zTikiCollisionListener() {}
