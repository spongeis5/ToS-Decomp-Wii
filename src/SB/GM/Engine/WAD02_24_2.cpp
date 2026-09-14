#include "SB/GM/Engine/WAD02_24_2.pool.h"

// WAD02_24_2.cpp -- the NPC behaviour-tree CONDITIONS, and after them
// the entity and bound geometry: 111 functions, 19,880 bytes.
//
// A condition is a tiny object: the asset it was built from, the client
// that owns it, and the NPC it asks about. Evaluate() reads one thing
// off one of the NPC's components and answers yes or no, which is why
// most of them are under fifty bytes.
//
// Layouts from the Wii build's DWARF (tools/dwarf_types.py), which
// covers this file: zNPCBTCondition 0x14 on a zBTCondition of 0xC, and
// each condition adds its own fields after it. The Sext:: asset structs
// the Setup functions read are NOT in the DWARF -- only the offsets
// each one touches are recovered, and they are named for the condition
// that reads them.
//
// The pool header in front reproduces WAD02.cpp's string pool ahead of
// this file's own strings, and the .rodata ahead of its float literals.

extern "C" double atan2(double y, double x);
extern "C" double sin(double x);
extern "C" double cos(double x);

class xBase;
class zBTClient;
class zNPCEntity;
class zNPCBase;
class zNPCSteering;
class xScene;

namespace Sext {

class ConditionBase;
class EventAny;
class NPCAsset;

enum eHitSource { eHitSourceEVENT = 0, END_eHitSourceENUM = 62 };

enum eRPSAttackTypes {
    eRPSAttackType_None = 0,
    eRPSAttackType_Hammer = 1,
    eRPSAttackType_Spin = 2,
    eRPSAttackType_Puck = 3,
    eRPSAttackType_Miniboss_Hammer = 4,
    eRPSAttackType_Miniboss_Spin = 5,
    eRPSAttackType_Miniboss_Puck = 6,
    END_eRPSAttackType_ENUM = 7,
};

enum eNPCPerceptionType {
    eNPCPerception_TargetReachable = 0,
    eNPCPerception_TargetChargable = 1,
    eNPCPerception_TargetAttackable = 2,
    eNPCPerception_TargetVisible = 3,
    eNPCPerception_TargetShootable = 4,
    eNPCPerception_TargetEngageable = 5,
    END_eNPCPerception_ENUM = 6,
};

enum eCollisionLayer { eCollisionLayer_Player = 13 };

// Only the fields each Setup reads; the structs themselves are not
// described by the debug info.
class DamagedCondition {
public:
    bool nonzero;
};

class InRPSAttackStateCondition {
public:
    unsigned char state;
};

// The flag is copied raw into the condition's bool: it is a bool here
// too, or the copy normalises it (addic/subfe), which retail does not.
class CheckPerceptionCondition {
public:
    unsigned int targets;
    unsigned char perceptionType;
    bool anded;
};

// The target indices are one-based in the assets.
class CheckPerceptionTargetStatusCondition {
public:
    unsigned char target;
    unsigned char status;
};

class IsInWallnetCondition {
public:
    unsigned int check;
    unsigned char target;
};

class FacingPerceptionTargetCondition {
public:
    unsigned char target;
    float tolerance;
};

}  // namespace Sext

// ---------------------------------------------------------------------------
// Math

class xVec3 {
public:
    xVec3& operator=(const xVec3& other);
    xVec3& operator+=(const xVec3& other);
    xVec3& operator*=(float s);
    xVec3& operator-=(const xVec3& other);
    bool operator==(const xVec3& other) const;
    bool operator!=(const xVec3& other) const { return !(*this == other); }
    void Sub(const xVec3& a, const xVec3& b);
    void Add(const xVec3& a, const xVec3& b);
    void ScaleComponents(const xVec3& s);
    void Scale(const xVec3& v, float s);
    void rotateY(const float& angle);
    float length2() const;

    static const xVec3 m_Null;
    static const xVec3 m_Ones;
    static const xVec3 m_UnitAxisY;

    float x;
    float y;
    float z;
};

class xMat3x3 {
public:
    xMat3x3& operator=(const xMat3x3& other);

    xVec3 right;
    unsigned int flags;
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

float xClampAngle0_2PI(float angle);
unsigned int xStrHash(const char* s);

class xQuat {
public:
    xVec3 v;
    float s;
};

namespace Math {

class Vector4 {
public:
    float x;
    float y;
    float z;
    float w;
};

class Matrix43 {
public:
    float m[12];
};

}  // namespace Math

// Constructs a vector in place from three floats: the image names
// Math::Vector's constructor for every twelve-byte vector built that way,
// as zNPCCommonBTActions.cpp found.
extern "C" void __ct__Q24Math6VectorFfff(void* self, float x, float y,
                                         float z);

void xMat3x3RMulVec(xVec3* o, const xMat3x3* m, const xVec3* v);
void v3add(xVec3* o, xVec3* a, xVec3* b);
void xMat3x3GetEuler(const xMat3x3* m, xVec3* euler);
void xMat3x3GetScale(const xMat3x3* m, xVec3* scale);
void xMat3x3Normalize(xMat3x3* o, const xMat3x3* m);
void xMat3x3Mul(xMat3x3* o, const xMat3x3* a, const xMat3x3* b);
void xMat3x3MulScaleC(xMat3x3* o, const xMat3x3* m, float x, float y,
                      float z);
void xMat4x3ToNGMatrix(Math::Matrix43* o, const xMat4x3* m);

// Math::Matrix43's constructor, which the linker folded onto Matrix33's:
// the image names Matrix33's.
extern "C" void __ct__Q24Math8Matrix33Fv(void* self);

namespace Sext {

// 0x128 in the DWARF; the bound reads its collision fields.
class NPCTemplate {
public:
    unsigned long long ChrAssetID;
    float ModelScaleMult;
    unsigned char CollisionType;
    unsigned char CollisionShape;
    bool CollisionRotWithNPC;
    unsigned char CollisionMainAxis;
    bool CollideWithPlayer;
    xVec3 BoundOffset;
    xVec3 BoundScaleV;
};

}  // namespace Sext

namespace std {

inline float fabs(float x) { return (float)__fabs(x); }

// Retail calls this one: it is emitted weak after its only caller, and
// defined there so that nothing can inline it.
inline float atan2(float y, float x);

}  // namespace std

// ---------------------------------------------------------------------------
// The model: an xOGModel starts with the instance's matrix.

class xAnimTable;

class xAnimFile {
public:
    xAnimFile* Next;
    char* Name;
    unsigned int ID;
    unsigned int FileFlags;
    float Duration;
    unsigned char _pad0[0x34 - 0x14];
    unsigned char NumAnims[3];
    unsigned char pad2;
    void** RawData;
};

class xAnimState {
public:
    unsigned char _pad0[0x10];
    unsigned int ID;
    unsigned int Flags;
    unsigned int UserFlags;
    float Speed;
    xAnimFile* Data;
};

class xAnimSingle {
public:
    unsigned int SingleFlags;
    xAnimState* State;
    float Time;
    float CurrentSpeed;
    float BilinearLerp[3];
    unsigned char _pad0[0x4C - 0x1C];
    unsigned int LoopCount;
};

class xAnimPlay {
public:
    unsigned char _pad0[0xC];
    xAnimSingle* Single;
    void* Object;
    xAnimTable* Table;
};

xAnimState* xAnimTableGetStateID(xAnimTable* table, unsigned int id);

// A renderable's box: two four-float vectors, copied as eight words.
class xBoxV4 {
public:
    Math::Vector4 lower;
    Math::Vector4 upper;
};

namespace Graphics {

// What a renderable's geometry points at when it is skinned; type 11
// carries its own box.
class GeomSkin {
public:
    unsigned char _pad0[0x4];
    int type;
    unsigned char _pad1[0x7C - 0x8];
    xBoxV4 bound;
};

class Geom {
public:
    unsigned char _pad0[0xC];
    xBoxV4 bound;
    unsigned char _pad1[0x48 - 0x2C];
    GeomSkin* skin;
};

class Renderable3D {
public:
    unsigned char _pad0[0x2C];
    Geom* geom;
};

// 0x68 in the DWARF; its renderables at +0x34.
class Model {
public:
    void SetRootTransform(const Math::Matrix43& mat);
    void CommitWorldTransformAttached();

    unsigned char _pad0[0x34];
    unsigned short renderableCount;
    unsigned short renderCustomizerCount;
    Renderable3D** renderables;
    unsigned char _pad1[0x68 - 0x3C];
};

}  // namespace Graphics

namespace World {

// The collision mesh's box: its centre and half-extent.
class CollisionMeshBlobEntity {
public:
    unsigned char _pad0[0x18];
    xVec3 center;
    xVec3 extent;
};

class ModelPrototypeEntity {
public:
    unsigned char _pad0[0x6C];
    CollisionMeshBlobEntity* collisionMesh;
};

// 0x98 in the DWARF: the prototype at +0x18, the graphics model at +0x24.
class ModelInstanceArticle {
public:
    void ApplyRenderCustomizers(unsigned int mask);

    unsigned char _pad0[0x18];
    ModelPrototypeEntity* protoEnt;
    unsigned char _pad1[0x24 - 0x1C];
    Graphics::Model model;
    unsigned char _pad2[0x98 - 0x8C];
};

// 0x178 in the DWARF: xModelInstance's matrix, scale, animation and
// render mask, then the article at +0xC4.
class xOGModel {
public:
    void SetColorMultiplier(float r, float g, float b, float a);

    xMat4x3 Mat;
    xVec3 Scale;
    xAnimPlay* Anim;
    unsigned short Flags;
    unsigned short pad;
    unsigned int renderCustomizerMask;
    unsigned char _pad0[0xC4 - 0x58];
    ModelInstanceArticle mModelArt;
};

class xOGModelHandle {
public:
    void Destroy();

    xOGModel* data;
    void* autoptr;
};

}  // namespace World

void xModelGetBoneMatNoScale(xMat4x3& mat, const World::xOGModel& model,
                             unsigned long bone);

// ---------------------------------------------------------------------------
// Entities. Every one of them has its vtable pointer at +0, in front of
// the xBase words; the slots are declared where a call needs one.

class xEntVirtuals {
public:
    virtual void _v0();
};

class xEffectAttachIntf : public xEntVirtuals {};

class hkpPhysicsSystem;

// 0x20 in the DWARF; the vector at +0x10 is four plain floats here so
// that the entity around it stays four-aligned.
class xHavokPhysicsObject {
public:
    void Cleanup();

    int physicsObjectType;
    void* packedPhysicsData;
    hkpPhysicsSystem* physicsSystem;
    unsigned int _pad0;
    float creationScale[4];
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

// 0xC0 in the DWARF, of which 0xBC is data: the NPC entity puts its
// component right after it. The 64-bit id at +0x18 is two words here so
// that the class stays four-aligned.
class xEnt : public xEffectAttachIntf {
public:
    unsigned char _pad0[0x20 - 0x4];
    unsigned int baseType;
    unsigned char _pad1[0x26 - 0x24];
    unsigned short baseFlags;
    unsigned char _pad2[0x30 - 0x28];
    void* eventFunc;
    World::xOGModel* model;
    void* modelAutoptr;
    void* asset;
    int storedCollisionLayer;
    unsigned int moreFlags;
    unsigned int flags;
    unsigned char isCulled;
    unsigned char num_ffx;
    unsigned char collType;
    unsigned char chkby;
    unsigned char penby;
    unsigned char collisionOn;
    void* move;
    xEntFrame* frame;
    void* transl;
    void* ffx;
    bool isDriving;
    void* neoDriver;
    void* pSurface;
    xVec3 pRigidBodyPostScale;
    unsigned int _pad3;
    xHavokPhysicsObject physicsObject;
    void* user_data;
    int ragdollState;
    float ragdollTotalBlendInTime;
    float ragdollTotalBlendOutTime;
    float ragdollCurrentBlendTime;
    float ragdollStateOnTimeRemaining;
    void* ragdollListener;
};

xVec3 xEntGetCenter(const xEnt* ent);
void xEntHide(xEnt* ent);
void xEntShow(xEnt* ent);
void xAnimVFX_RemovePairs(xEffectAttachIntf* ent);
void xAnimSFX_RemovePairs(xEffectAttachIntf* ent);
void xModelGetBoneLocationNoScale(xVec3& loc, const World::xOGModel& model,
                                  unsigned long index);

enum eNPCEntityCollMainAxis {
    eNPCEntityCollMainAxis_Auto = 0,
    eNPCEntityCollMainAxis_X = 1,
    eNPCEntityCollMainAxis_Y = 2,
    eNPCEntityCollMainAxis_Z = 3,
    END_eNPCEntityCollMainAxis_ENUM = 4,
};

enum eNPCEntityCollType {
    eNPCEntityCollType_None = 0,
    eNPCEntityCollType_CharacterProxy = 1,
    eNPCEntityCollType_Fixed = 2,
    eNPCEntityCollType_CharacterRigidBody = 3,
    END_eNPCEntityCollType_ENUM = 4,
};

// 0x30 in the DWARF: a box in the model's space, and the scale it was
// built with.
class zNPCBound {
public:
    zNPCBound();
    float GetBoundRadiusXZ() const;
    float GetBoundMinRadiusXZ() const;
    void GetCapsuleLocal(xVec3& a, xVec3& b, float& radius);
    void GetCylinderLocal(xVec3& a, xVec3& b, float& radius);
    bool GetModelBoundFromArticle(const World::xOGModel* model, xVec3& min,
                                  xVec3& max);
    bool GetModelBoundFromCollMesh(const World::xOGModel* model, xVec3& min,
                                   xVec3& max);
    void Init(const World::xOGModel* model, const Sext::NPCTemplate* tmpl,
              const xVec3* scale);

    xVec3 center;
    xVec3 extent;
    eNPCEntityCollMainAxis mainAxis;
    const World::xOGModel* modelInst;
    xVec3 finalBoundScale;
    bool isInit;
};

typedef void (*zNPCBeforeAnimMatricesCB)(zNPCBase* npc, xAnimPlay* play,
                                         xQuat* q, xVec3* t, xVec3* s,
                                         int n);
typedef void (*zNPCAfterAnimMatricesCB)(zNPCBase* npc, xAnimPlay* play,
                                        Math::Matrix43* mat, xQuat* q,
                                        xVec3* t, xVec3* s, int n);

class zNPCEntityParams {
public:
    zNPCBeforeAnimMatricesCB beforeAnimMatricesFn;
    zNPCAfterAnimMatricesCB afterAnimMatricesFn;
    void* modelInstAsset;
    xAnimTable* animTable;
    unsigned char collisionChkBy;
    unsigned char collisionPenBy;
};

// ---------------------------------------------------------------------------
// Havok, as the entity reaches it.

class hkVector4;

class hkReferencedObject {
public:
    void addReference() const;
    void removeReference() const;

    void* _vtable;
    unsigned short m_memSizeAndFlags;
    short m_referenceCount;
};

// Emitted elsewhere. The instance the character proxy's listener list
// uses is named for hkpRigidBody*, the one the linker folded it onto.
template <class T>
class hkArray {
public:
    int indexOf(const T& t, int startIdx = 0, int endIdx = -1) const;

    T* m_data;
    int m_size;
    int m_capacityAndFlags;
};

// Instantiated at the end of the unit and emitted weak there; retail
// calls it too.
template <class T>
class hkSmallArray {
public:
    int indexOf(const T& t) const {
        for (int i = 0; i < m_size; i++) {
            if (m_data[i] == t) {
                return i;
            }
        }

        return -1;
    }

    T* m_data;
    unsigned short m_size;
    unsigned short m_capacityAndFlags;
};

class hkpCharacterProxyListener {
public:
    void* _vtable;
};

class hkpCollisionListener {
public:
    void* _vtable;
};

class hkpPhantom : public hkReferencedObject {};
class hkpShapePhantom : public hkpPhantom {};

class hkpRigidBody;

// 0xC0 in the DWARF; its listener list at +0x80.
class hkpCharacterProxy : public hkReferencedObject {
public:
    hkpShapePhantom* getShapePhantom();
    void addCharacterProxyListener(hkpCharacterProxyListener* listener);
    void removeCharacterProxyListener(hkpCharacterProxyListener* listener);

    unsigned char _pad0[0x80 - 0x8];
    hkArray<hkpRigidBody*> m_listeners;
};

class hkpEntity : public hkReferencedObject {
public:
    void addCollisionListener(hkpCollisionListener* listener);
    void removeCollisionListener(hkpCollisionListener* listener);

    unsigned char _pad0[0x200 - 0x8];
    hkSmallArray<hkpCollisionListener*> m_collisionListeners;
};

class hkpRigidBody : public hkpEntity {};

class hkpCharacterRigidBody : public hkReferencedObject {
public:
    unsigned char _pad0[0x10 - 0x8];
    hkpRigidBody* m_character;
};

// hkpCharacterRigidBody::getRigidBody, which hands back m_character: the
// linker folded it onto zBTActionHandleEvent::Update, the name every call
// in the image uses.
extern "C" hkpRigidBody* Update__20zBTActionHandleEventFf(
    hkpCharacterRigidBody* body);

void xHavok_RemoveFromSimWorld(hkpPhantom* phantom);
void xHavok_RemoveFromSimWorld(hkpRigidBody* body);
void xHavok_RemoveFromSimWorld(const hkpPhysicsSystem* system);

enum ControllerType {
    NONE = 0,
    PROXY = 1,
    RIGID_BODY = 2,
};

// Cleanup and GetCharacterRigidBody are inline, and retail calls them:
// they are defined at the end of the file.
class xHavokCharacterController {
public:
    hkpCharacterProxy* GetCharacterProxy() const;
    hkpCharacterRigidBody* GetCharacterRigidBody() const;
    void Cleanup();
    void SetPosition(const hkVector4& pos);

    union {
        hkpCharacterProxy* characterProxy;
        hkpCharacterRigidBody* characterRigidBody;
    };
    ControllerType controllerType;
};

class zNPCCollisionListener : public hkpCharacterProxyListener {
public:
    zNPCBase* npc;
};

class zNPCRigidBodyCollisionListener : public hkpCollisionListener {
public:
    zNPCBase* npc;
    hkpRigidBody* body;
};

class xEntDrive;
class zNPCType;

// 0x1C in the DWARF: where the NPC was, and facing which way.
class zNPCStatus {
public:
    void ResetToNPCAsset(const Sext::NPCAsset* asset);

    xVec3 lastPos;
    xVec3 lastOrientation;
    int lastStatus;
};

// 0x1D0 in the DWARF. The component's owner and its vtable pointer follow
// the entity at +0xBC; the virtual functions defined here are declared
// plain, so that no vtable is emitted for them.
class zNPCEntity : public xEnt {
public:
    static void NPCBeforeAnimMatrices(xAnimPlay* play, xQuat* q, xVec3* t,
                                      xVec3* s, int n);
    static void NPCAfterAnimMatrices(xAnimPlay* play, Math::Matrix43* mat,
                                     xQuat* q, xVec3* t, xVec3* s, int n);
    static int EventWrapper(xBase* from, xBase* to, unsigned int event,
                            Sext::EventAny* args);

    void SetParameters(zNPCEntityParams* params);
    xAnimTable* GetAnimTable();
    void Detached(zNPCStatus* status);
    void RegisterCollisionListener(zNPCRigidBodyCollisionListener* listener);
    void RegisterCollisionListener(zNPCCollisionListener* listener);
    void UnregisterCollisionListener(zNPCCollisionListener* listener);
    void UnregisterCollisionListener(
        zNPCRigidBodyCollisionListener* listener);
    void _AddCollisionListener(zNPCCollisionListener* listener);
    void _AddCollisionListener(zNPCRigidBodyCollisionListener* listener);
    void _RemoveCollisionListener(zNPCCollisionListener* listener);
    void _RemoveCollisionListener(zNPCRigidBodyCollisionListener* listener);
    void Reset(const zNPCStatus* status);
    float GetYaw() const;
    void RenderModelInstance();
    void SetAnimState(unsigned int animID, float blend, const char* name);
    void SetInstanceAnimState(unsigned int animID, float blend);
    void SetAnimState(unsigned int animID, float blend, float time,
                      const char* name);
    void CopyFrame(const xEntFrame* src);
    void UpdateRender();
    void SetupInstanceAnimTable();
    void _EvalAnimPhysics(xVec3& trans, float& yaw, xQuat& quat);
    void _ApplyAnimPhysicsRotation(float yaw, xQuat quat);
    xVec3 _ApplyAnimPhysicsTranslationToVel(const xVec3& trans, float dt,
                                            xVec3& vel);
    void _UpdateCharacterProxy(xVec3 vel, float dt);
    void PreUpdate(float dt);
    void UpdateWithVelocity(float dt);
    void Enable();
    void Disable();
    void EnableCollision();
    void DisableCollision();
    void _InitializeBounds(xVec3& scale);
    void GetBoneWorldPosition(int bone, xVec3& pos);
    bool DoesAnimExist(const char* name);
    bool DoesAnimExist(unsigned int animID);
    void SetCurAnimLerp0(float lerp);
    void SetCurAnimLerp1(float lerp);
    unsigned int GetCurAnimID();
    void SetCurAnimSpeed(float speed);
    void ClearCurAnimLoopCount();
    unsigned int GetCurAnimLoopCount();
    bool IsAnimationStopped(unsigned int animID);
    void Paused();
    void Resumed();
    int SystemEvent(xBase* from, xBase* to, unsigned int event,
                    Sext::EventAny* args);
    void Hide();
    void Show();
    void KillVelocity();
    void SetFrameFromHeading(xVec3& heading);
    void SetFrameFromYaw(float yaw);
    void Teleport(const xMat4x3& mat);
    void GetBoundCenter(xVec3& center) const;
    void ResetDamageColor();

    zNPCBase* owner;
    void* _componentVtable;
    xMat3x3 modelOnlyMat;
    zNPCBound npcBound;
    xVec3 modelScale;
    float shadowRadius;
    float scalePhysDisp;
    bool entityPosUpdated;
    zNPCType* type;
    zNPCEntityParams* parameters;
    void* modelInstanceAsset;
    zNPCCollisionListener* collisionListeners[4];
    zNPCRigidBodyCollisionListener* rigidBodyCollisionListeners[4];
    xEntDrive* npcDrive;
    xHavokCharacterController characterController;
    void* currVFX;
    xAnimTable* instanceAnimTable;
    unsigned int idleNumber;
    bool queryAnimEndSet;
    bool pokedA;
    bool pokedB;
    float damageColorTimer;
    float currentColorMultiplier;
    xVec3 lastFrameAnimPhysTrans;
    float lastFrameAnimPhysYaw;
    xQuat lastFrameAnimPhysQuat;
    bool enabled;
    bool visible;
    bool modelOnlyMatSet;
    bool floorCollision;
    bool floorDoesDamage;
    xVec3 floorVelocity;
    float floorNormalY;
    eNPCEntityCollType collisionType;
};

class zNPCBaseVirtuals {
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
    virtual int SystemEvent(xBase* from, xBase* to, unsigned int event,
                            Sext::EventAny* args);
};

// A pickup, destructible or the like: type 0x55, asked for its health
// through its vtable.
class zHealthEnt : public xEnt {
public:
    virtual void _h1();
    virtual void _h2();
    virtual void _h3();
    virtual void _h4();
    virtual void _h5();
    virtual void _h6();
    virtual void _h7();
    virtual void _h8();
    virtual void _h9();
    virtual void _h10();
    virtual void _h11();
    virtual void _h12();
    virtual void _h13();
    virtual void _h14();
    virtual void _h15();
    virtual void _h16();
    virtual void _h17();
    virtual void _h18();
    virtual void _h19();
    virtual void _h20();
    virtual void _h21();
    virtual void _h22();
    virtual void _h23();
    virtual void _h24();
    virtual void _h25();
    virtual void _h26();
    virtual void _h27();
    virtual void _h28();
    virtual void _h29();
    virtual void _h30();
    virtual void _h31();
    virtual void _h32();
    virtual void _h33();
    virtual void _h34();
    virtual void _h35();
    virtual void _h36();
    virtual void _h37();
    virtual void _h38();
    virtual void _h39();
    virtual void _h40();
    virtual void _h41();
    virtual void _h42();
    virtual void _h43();
    virtual void _h44();
    virtual void _h45();
    virtual void _h46();
    virtual void _h47();
    virtual void _h48();
    virtual void _h49();
    virtual void _h50();
    virtual void _h51();
    virtual void _h52();
    virtual void _h53();
    virtual void _h54();
    virtual void _h55();
    virtual void _h56();
    virtual void _h57();
    virtual void _h58();
    virtual void _h59();
    virtual void _h60();
    virtual void _h61();
    virtual void _h62();
    virtual void _h63();
    virtual void _h64();
    virtual void _h65();
    virtual void _h66();
    virtual void _h67();
    virtual void _h68();
    virtual void _h69();
    virtual void _h70();
    virtual void _h71();
    virtual void _h72();
    virtual void _h73();
    virtual void _h74();
    virtual void _h75();
    virtual void _h76();
    virtual void _h77();
    virtual void _h78();
    virtual void _h79();
    virtual void _h80();
    virtual void _h81();
    virtual float GetHealth();
};

// The hit points the other two target types carry: what is left is the
// total less what has been taken.
class zHitPoints {
public:
    unsigned char _pad0[0x18];
    unsigned int total;
    unsigned char _pad1[0x2C - 0x1C];
    unsigned int taken;
};

class zHitPointsEnt56 : public xEnt {
public:
    unsigned char _pad3[0x178 - 0xBC];
    zHitPoints* hitPoints;
};

class zHitPointsEnt5A : public xEnt {
public:
    unsigned char _pad3[0xCC - 0xBC];
    zHitPoints* hitPoints;
};

// 0x2C in the DWARF; this file reads the knockback flag and the damage.
class zNPCGetsDamageInfo {
public:
    unsigned int flags;
    unsigned char _pad0[0xC - 0x4];
    float damageHP;
    unsigned char _pad1[0x2C - 0x10];
};

class zNPCCombat {
public:
    bool HitByType(Sext::eHitSource source) const;
    bool BlockedType(Sext::eHitSource source) const;

    // Through the getter the bound is re-read every pass, as retail has
    // it; reading damageListSize directly lets mwcc reuse the count.
    unsigned int GetDamageCount() const { return damageListSize; }
    zNPCGetsDamageInfo* GetDamageInfo(unsigned int i) {
        return (i < GetDamageCount()) ? &damageList[i] : 0;
    }

    unsigned char _pad0[0x294];
    Sext::eRPSAttackTypes attackState;
    unsigned char _pad1[0x29C - 0x298];
    zNPCGetsDamageInfo damageList[6];
    unsigned char _pad2[0x3BC - 0x3A4];
    unsigned int damageListSize;
    unsigned char _pad3[0x4E0 - 0x3C0];
    unsigned int blockListSize;
};

class zNPCPerceptionTarget {
public:
    xEnt* targetEnt;
    unsigned char _pad0[0x74 - 0x4];
};

class zNPCPerception {
public:
    bool AreTargetsPerceived(unsigned int mask, Sext::eNPCPerceptionType type,
                             bool all);
    static bool CheckLineOfSight(const xVec3* from, const xVec3* to,
                                 const zNPCEntity* self, const xEnt* ignore,
                                 Sext::eCollisionLayer layer);

    unsigned char _pad0[0x10];
    zNPCPerceptionTarget targets[4];
};

class zWallNet {
public:
    bool IsInsideWallNetXZ(const xVec3& pos) const;
    bool IsInsideWallNet(const xVec3& pos) const;
};

// The component's owner at +0 and its vptr after it at +4, so the
// steering is polymorphic from +4, as zNPCCommonBTActions.cpp found; the
// wall net stays at +0x34.
class zNPCSteeringData {
public:
    zNPCBase* owner;
};

class zNPCSteeringVirtuals : public zNPCSteeringData {
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
};

class zNPCSteering : public zNPCSteeringVirtuals {
public:
    unsigned char _pad0[0x34 - 0x8];
    zWallNet* wallNet;
};

class zPlanktonShakeManager {
public:
    static bool IsBeingShaken(const xEnt* ent);
};

// The word at +0x10 is inside the entity base; only its offset and the
// value tested against are recovered. The flag byte at +0x70 is a run
// of one-bit flags, puppetMode first.
// CharacterAssets in the DWARF; the anim table's id is the 64-bit word
// at +0x40.
class zCharacterAsset {
public:
    unsigned char _pad0[0x40];
    unsigned long long animTableID;
};

class zNPCTemplate {
public:
    const Sext::NPCTemplate* templateAsset;
};

class zNPCBase : public zNPCBaseVirtuals {
public:
    unsigned char _pad0[0x10 - 0x4];
    unsigned int typeID;
    unsigned char _pad1[0x5C - 0x14];
    zNPCType* type;
    Sext::NPCAsset* npcAsset;
    void* modelAsset;
    zCharacterAsset* characterAsset;
    void* parentGroup;
    bool puppetMode : 1;
    bool alive : 1;
    bool present : 1;
    bool activated : 1;
    bool spawned : 1;
    bool paused : 1;
    bool updateInCinematicAlways : 1;
    bool updateInCinematicNever : 1;
    unsigned char _pad2[0x78 - 0x71];
    zNPCTemplate* npcTemplate;
    unsigned char _pad3[0x98 - 0x7C];
    zNPCEntity* npcEntity;
    void* npcSteeringOld;
    zNPCSteering* npcSteering;
    zNPCPerception* npcPerception;
    zNPCCombat* npcCombat;
};

class zPlayerActionManager {
public:
    unsigned int GetCurrentActionID() const;

    unsigned char _pad0[0x38];
};

class zPlayerCommon {
public:
    unsigned char _pad0[0x34];
    World::xOGModel* model;
    unsigned char _pad1[0xC0 - 0x38];
    zPlayerActionManager actionManager;
    unsigned char _pad2[0x8B0 - 0xF8];
    int spongeBuffState;
};

class xGlobals {
public:
    unsigned char _pad0[0x428];
    zPlayerCommon* player;
    unsigned char _pad1[0x43C - 0x42C];
    xScene* sceneCur;
};

extern xGlobals* xglobals;

class zVariableBase;

class zBlackboard {
public:
    template <class T>
    bool Read(unsigned int id, T& out) const;

    unsigned int size;
    zVariableBase** variables;
};

class zBTClient {
public:
    unsigned char _pad0[0x88];
    zBlackboard blackboard;
};

// zBTCondition is 0xC in the DWARF with two members named, so the word
// left over is the vtable pointer; nothing here dispatches through it.

class zNPCBTBlockedCondition {
public:
    bool Evaluate() const;

    Sext::ConditionBase* conditionAsset;
    zBTClient* btClient;
    unsigned char _vtable[4];
    zNPCBase* npcBase;
    Sext::ConditionBase* asset;
};

class zNPCBTBlockedTypeCondition {
public:
    bool Evaluate() const;

    Sext::ConditionBase* conditionAsset;
    zBTClient* btClient;
    unsigned char _vtable[4];
    zNPCBase* npcBase;
    Sext::ConditionBase* asset;
    Sext::eHitSource type;
};

class zNPCBTDamagedByTypeCondition {
public:
    bool Evaluate() const;

    Sext::ConditionBase* conditionAsset;
    zBTClient* btClient;
    unsigned char _vtable[4];
    zNPCBase* npcBase;
    Sext::ConditionBase* asset;
    Sext::eHitSource type;
};

class zNPCBTDamagedCondition {
public:
    void Setup(const Sext::ConditionBase* condition);
    bool Evaluate() const;

    Sext::ConditionBase* conditionAsset;
    zBTClient* btClient;
    unsigned char _vtable[4];
    zNPCBase* npcBase;
    Sext::ConditionBase* asset;
    bool nonzero;
};

class zNPCBTDamagedByKnockbackCondition {
public:
    bool Evaluate() const;

    Sext::ConditionBase* conditionAsset;
    zBTClient* btClient;
    unsigned char _vtable[4];
    zNPCBase* npcBase;
    Sext::ConditionBase* asset;
};

class zNPCBTPokedCondition {
public:
    bool Evaluate() const;

    Sext::ConditionBase* conditionAsset;
    zBTClient* btClient;
    unsigned char _vtable[4];
    zNPCBase* npcBase;
    Sext::ConditionBase* asset;
    int pokeType;
};

class zNPCBTInRPSAttackStateCondition {
public:
    void Setup(const Sext::ConditionBase* condition);
    bool Evaluate() const;

    Sext::ConditionBase* conditionAsset;
    zBTClient* btClient;
    unsigned char _vtable[4];
    zNPCBase* npcBase;
    Sext::ConditionBase* asset;
    Sext::eRPSAttackTypes state;
};

class zNPCBTIsPlanktonShakingCondition {
public:
    bool Evaluate() const;

    Sext::ConditionBase* conditionAsset;
    zBTClient* btClient;
    unsigned char _vtable[4];
    zNPCBase* npcBase;
    Sext::ConditionBase* asset;
};

class zNPCBTSBPlayerIsBuff {
public:
    bool Evaluate() const;

    Sext::ConditionBase* conditionAsset;
    zBTClient* btClient;
    unsigned char _vtable[4];
    zNPCBase* npcBase;
    Sext::ConditionBase* asset;
};

class zNPCBTIsInPuppetModeCondition {
public:
    bool Evaluate() const;

    Sext::ConditionBase* conditionAsset;
    zBTClient* btClient;
    unsigned char _vtable[4];
    zNPCBase* npcBase;
    Sext::ConditionBase* asset;
};

class zNPCBTCheckPerceptionCondition {
public:
    void Setup(const Sext::ConditionBase* condition);
    bool Evaluate() const;

    Sext::ConditionBase* conditionAsset;
    zBTClient* btClient;
    unsigned char _vtable[4];
    zNPCBase* npcBase;
    Sext::ConditionBase* asset;
    Sext::eNPCPerceptionType perceptionType;
    unsigned int targets;
    bool anded;
};

enum eNPCPerceptionTargetStatusType {
    eNPCPerceptionTargetStatus_TargetAlive = 0,
    eNPCPerceptionTargetStatus_TargetDead = 1,
    eNPCPerceptionTargetStatus_TargetFriendly = 2,
    eNPCPerceptionTargetStatus_TargetHostile = 3,
    END_eNPCPerceptionTargetStatus_ENUM = 4,
};

class zNPCBTCheckPerceptionTargetStatusCondition {
public:
    void Setup(const Sext::ConditionBase* condition);
    bool Evaluate() const;
    bool IsTargetAlive() const;

    Sext::ConditionBase* conditionAsset;
    zBTClient* btClient;
    unsigned char _vtable[4];
    zNPCBase* npcBase;
    Sext::ConditionBase* asset;
    eNPCPerceptionTargetStatusType perceptionTargetStatusType;
    unsigned int target;
};

class zNPCBTCheckPerceptionTargetChangedCondition {
public:
    void Setup(const Sext::ConditionBase* condition);
    bool Evaluate() const;

    Sext::ConditionBase* conditionAsset;
    zBTClient* btClient;
    unsigned char _vtable[4];
    zNPCBase* npcBase;
    Sext::ConditionBase* asset;
    xEnt* originalTarget;
};

class zNPCBTCheckPerceptionTargetInWallnetCondition {
public:
    bool Evaluate() const;

    Sext::ConditionBase* conditionAsset;
    zBTClient* btClient;
    unsigned char _vtable[4];
    zNPCBase* npcBase;
    Sext::ConditionBase* asset;
    unsigned int target;
};

enum CheckType {
    PerceptionTarget = 0,
    Self = 1,
    END_CheckTypeENUM = 2,
};

class zNPCBTIsInWallnetCondition {
public:
    void Setup(const Sext::ConditionBase* condition);
    bool Evaluate() const;

    Sext::ConditionBase* conditionAsset;
    zBTClient* btClient;
    unsigned char _vtable[4];
    zNPCBase* npcBase;
    Sext::ConditionBase* asset;
    CheckType check;
    unsigned int target;
};

class zNPCBTFacingPerceptionTargetCondition {
public:
    void Setup(const Sext::ConditionBase* condition);
    bool Evaluate() const;

    Sext::ConditionBase* conditionAsset;
    zBTClient* btClient;
    unsigned char _vtable[4];
    zNPCBase* npcBase;
    Sext::ConditionBase* asset;
    unsigned char target;
    float tolerance;
};

class zNPCBTBossSquidwardBlockPlayer {
public:
    bool Evaluate() const;

    Sext::ConditionBase* conditionAsset;
    zBTClient* btClient;
    unsigned char _vtable[4];
    zNPCBase* npcBase;
    Sext::ConditionBase* asset;
};

class zNPCBTBossSquidwardDestroyCover {
public:
    bool Evaluate() const;

    Sext::ConditionBase* conditionAsset;
    zBTClient* btClient;
    unsigned char _vtable[4];
    zNPCBase* npcBase;
    Sext::ConditionBase* asset;
};

bool zNPCBTBlockedCondition::Evaluate() const {
    zNPCCombat* combat = npcBase->npcCombat;

    if (combat != 0) {
        return combat->blockListSize != 0;
    }

    return false;
}

// NEAR MISS: 2 of 4 words; retail loads the asset byte ahead of storing
// the pointer, ours stores first. Reading the byte into a local first
// hoists the load but swaps the two stores; seven spellings measured
// (tools/sweep_src.py), the same two words every time. Likewise below.
void zNPCBTInRPSAttackStateCondition::Setup(
    const Sext::ConditionBase* condition) {
    asset = (Sext::ConditionBase*)condition;
    state = (Sext::eRPSAttackTypes)(
        (const Sext::InRPSAttackStateCondition*)condition)
                ->state;
}

bool zNPCBTInRPSAttackStateCondition::Evaluate() const {
    zNPCCombat* combat = npcBase->npcCombat;

    if (combat != 0) {
        return state == combat->attackState;
    }

    return false;
}

bool zNPCBTBlockedTypeCondition::Evaluate() const {
    zNPCCombat* combat = npcBase->npcCombat;

    if (combat != 0) {
        return combat->BlockedType(type);
    }

    return false;
}

bool zNPCBTDamagedByTypeCondition::Evaluate() const {
    zNPCCombat* combat = npcBase->npcCombat;

    if (combat != 0 && combat->HitByType(type)) {
        return true;
    }

    return false;
}

// NEAR MISS: 2 of 4 words; the load/store order, as in the Setup above.
void zNPCBTDamagedCondition::Setup(const Sext::ConditionBase* condition) {
    asset = (Sext::ConditionBase*)condition;
    nonzero = ((const Sext::DamagedCondition*)condition)->nonzero;
}

// Without the flag any damage this frame counts; with it only damage
// that actually took hit points.
bool zNPCBTDamagedCondition::Evaluate() const {
    zNPCCombat* combat = npcBase->npcCombat;

    if (combat != 0) {
        if (!nonzero) {
            return combat->damageListSize != 0;
        }

        for (unsigned int i = 0; i < combat->damageListSize; i++) {
            if (combat->GetDamageInfo(i)->damageHP > 0.0f) {
                return true;
            }
        }
    }

    return false;
}

bool zNPCBTIsPlanktonShakingCondition::Evaluate() const {
    return zPlanktonShakeManager::IsBeingShaken((const xEnt*)npcBase->npcEntity);
}

bool zNPCBTDamagedByKnockbackCondition::Evaluate() const {
    zNPCCombat* combat = npcBase->npcCombat;

    if (combat != 0) {
        for (unsigned int i = 0; i < combat->damageListSize; i++) {
            if (combat->GetDamageInfo(i)->flags & 1) {
                return true;
            }
        }
    }

    return false;
}

bool zNPCBTPokedCondition::Evaluate() const {
    switch (pokeType) {
    case 0:
        return npcBase->npcEntity->pokedA;
    case 1:
        return npcBase->npcEntity->pokedB;
    }

    return false;
}

bool zNPCBTSBPlayerIsBuff::Evaluate() const {
    return xglobals->player->spongeBuffState == 1;
}

void zNPCBTCheckPerceptionCondition::Setup(
    const Sext::ConditionBase* condition) {
    const Sext::CheckPerceptionCondition* c =
        (const Sext::CheckPerceptionCondition*)condition;

    targets = c->targets;
    perceptionType = (Sext::eNPCPerceptionType)c->perceptionType;
    anded = c->anded;
}

bool zNPCBTCheckPerceptionCondition::Evaluate() const {
    zNPCPerception* perception = npcBase->npcPerception;

    if (perception != 0) {
        return perception->AreTargetsPerceived(targets, perceptionType,
                                               anded);
    }

    return false;
}

void zNPCBTCheckPerceptionTargetStatusCondition::Setup(
    const Sext::ConditionBase* condition) {
    const Sext::CheckPerceptionTargetStatusCondition* c =
        (const Sext::CheckPerceptionTargetStatusCondition*)condition;

    target = c->target - 1;
    perceptionTargetStatusType = (eNPCPerceptionTargetStatusType)c->status;
}

bool zNPCBTCheckPerceptionTargetStatusCondition::Evaluate() const {
    switch (perceptionTargetStatusType) {
    case eNPCPerceptionTargetStatus_TargetAlive:
        return IsTargetAlive();
    case eNPCPerceptionTargetStatus_TargetDead:
        return !IsTargetAlive();
    case eNPCPerceptionTargetStatus_TargetFriendly:
        return false;
    case eNPCPerceptionTargetStatus_TargetHostile:
        return true;
    }

    return false;
}

// An NPC is alive by its flag; the three other kinds of target are
// alive while they have health left.
bool zNPCBTCheckPerceptionTargetStatusCondition::IsTargetAlive() const {
    zNPCPerception* perception = npcBase->npcPerception;

    if (perception != 0) {
        xEnt* ent = perception->targets[target].targetEnt;

        if (ent == 0) {
            return false;
        }

        switch (ent->baseType) {
        case 0x38:
            return ((zNPCEntity*)ent)->owner->alive;
        case 0x55:
            return !(((zHealthEnt*)ent)->GetHealth() <= 0.0f);
        case 0x56: {
            zHitPoints* hp = ((zHitPointsEnt56*)ent)->hitPoints;

            if (hp != 0) {
                return !((float)(hp->total - hp->taken) <= 0.0f);
            }

            return false;
        }
        case 0x5A: {
            zHitPoints* hp = ((zHitPointsEnt5A*)ent)->hitPoints;

            if (hp != 0) {
                return !((float)(hp->total - hp->taken) <= 0.0f);
            }

            return false;
        }
        }

        return false;
    }

    return false;
}

void zNPCBTCheckPerceptionTargetChangedCondition::Setup(
    const Sext::ConditionBase* condition) {
    originalTarget = 0;

    zNPCPerception* perception = npcBase->npcPerception;

    if (perception != 0) {
        originalTarget = perception->targets[0].targetEnt;
    }
}

bool zNPCBTCheckPerceptionTargetChangedCondition::Evaluate() const {
    zNPCPerception* perception = npcBase->npcPerception;

    if (perception != 0 &&
        originalTarget != perception->targets[0].targetEnt) {
        return true;
    }

    return false;
}

// Only the player-sized kinds of entity -- NPCs and the three with
// health -- can be in a wall net.
bool zNPCBTCheckPerceptionTargetInWallnetCondition::Evaluate() const {
    zNPCBase* npc = npcBase;

    if (npc->typeID != 0xF0) {
        return false;
    }

    zNPCPerception* perception = npc->npcPerception;

    if (perception == 0) {
        return false;
    }

    xEnt* ent = perception->targets[target].targetEnt;

    if (ent == 0) {
        return false;
    }

    zWallNet* wallNet = npc->npcSteering->wallNet;

    if (wallNet == 0) {
        return false;
    }

    // A switch: the same test as an if-chain is not folded into the
    // range check retail has for 0x55..0x56.
    switch (ent->baseType) {
    case 0x55:
    case 0x56:
    case 0x38:
    case 0x5A:
        return wallNet->IsInsideWallNetXZ(xEntGetCenter(ent));
    }

    return false;
}

bool zNPCBTIsInPuppetModeCondition::Evaluate() const {
    zNPCBase* npc = npcBase;

    if (npc->typeID != 0xF0) {
        return false;
    }

    return npc->puppetMode;
}

void zNPCBTIsInWallnetCondition::Setup(const Sext::ConditionBase* condition) {
    const Sext::IsInWallnetCondition* c =
        (const Sext::IsInWallnetCondition*)condition;

    check = (CheckType)c->check;
    target = c->target - 1;
}

bool zNPCBTIsInWallnetCondition::Evaluate() const {
    zNPCBase* npc = npcBase;

    if (npc->typeID != 0xF0) {
        return false;
    }

    zWallNet* wallNet = npc->npcSteering->wallNet;

    if (wallNet == 0) {
        return false;
    }

    xEnt* ent = 0;

    switch (check) {
    case PerceptionTarget: {
        zNPCPerception* perception = npc->npcPerception;

        if (perception != 0) {
            ent = perception->targets[target].targetEnt;
        }

        break;
    }
    case Self:
        ent = npc->npcEntity;
        break;
    }

    if (ent == 0) {
        return false;
    }

    switch (ent->baseType) {
    case 0x55:
    case 0x56:
    case 0x38:
    case 0x5A:
        return wallNet->IsInsideWallNet(xEntGetCenter(ent));
    }

    return false;
}

// The asset gives the tolerance in degrees.
void zNPCBTFacingPerceptionTargetCondition::Setup(
    const Sext::ConditionBase* condition) {
    const Sext::FacingPerceptionTargetCondition* c =
        (const Sext::FacingPerceptionTargetCondition*)condition;

    target = c->target - 1;
    tolerance = 0.017453292f * c->tolerance;
}

// Headings are compared on the ground plane, both clamped to [0, 2pi).
bool zNPCBTFacingPerceptionTargetCondition::Evaluate() const {
    zNPCPerception* perception = npcBase->npcPerception;

    if (perception != 0) {
        xVec3 toTarget;

        toTarget.Sub(perception->targets[target].targetEnt->model->Mat.pos,
                     npcBase->npcEntity->model->Mat.pos);

        float facing = xClampAngle0_2PI(
            std::atan2(npcBase->npcEntity->model->Mat.at.z,
                       npcBase->npcEntity->model->Mat.at.x));
        float angle = xClampAngle0_2PI(std::atan2(toTarget.z, toTarget.x));

        if (std::fabs(angle - facing) < tolerance) {
            return true;
        }
    }

    return false;
}

inline float std::atan2(float y, float x) { return ::atan2(y, x); }

// Action 0x23 is the player's; the blackboard can veto the block.
bool zNPCBTBossSquidwardBlockPlayer::Evaluate() const {
    if (xglobals->player->actionManager.GetCurrentActionID() == 0x23) {
        unsigned int id = xStrHash("DO_NOT_DISTURB");
        int value = -1;

        btClient->blackboard.Read(id, value);

        return value == 0;
    }

    return false;
}

// Cover is destroyed when bone 19 cannot see the player.
bool zNPCBTBossSquidwardDestroyCover::Evaluate() const {
    xVec3 playerPos = xglobals->player->model->Mat.pos;
    xMat4x3 boneMat;

    xModelGetBoneMatNoScale(boneMat, *npcBase->npcEntity->model, 19);

    xVec3 bonePos = boneMat.pos;

    return !zNPCPerception::CheckLineOfSight(&bonePos, &playerPos,
                                             npcBase->npcEntity, 0,
                                             Sext::eCollisionLayer_Player);
}

// FIVE ASSET CREATES THAT ARE ONE TAIL CALL EACH: `li r5,<id>; b`
// into the NPC manager, with the id in the third argument. A
// non-template symbol does not carry its return type, so what these
// hand back is only known to be whatever the manager returns.

namespace World { class EntityHandleBase; }

namespace Sext {
class NPCAsset;
class NPCGroupAsset;

class AnimViewer {
public:
    static xBase* Create(World::EntityHandleBase* handle,
                     AnimViewer* asset);
};

class NPCGeneric {
public:
    static xBase* Create(World::EntityHandleBase* handle,
                     NPCGeneric* asset);
};

class GenericSpawner {
public:
    static xBase* Create(World::EntityHandleBase* handle,
                     GenericSpawner* asset);
};

class GenericSwarm {
public:
    static xBase* Create(World::EntityHandleBase* handle,
                     GenericSwarm* asset);
};

class NPCGroupCircle {
public:
    static xBase* Create(World::EntityHandleBase* handle,
                     NPCGroupCircle* asset);
};

}  // namespace Sext

namespace Memory {

class Factory {
public:
    void DeallocMem(void* p);
};

}  // namespace Memory

class zNPCManager {
public:
    static Memory::Factory factory;

    static xBase* CreateNPC(World::EntityHandleBase* handle,
                            Sext::NPCAsset* asset, unsigned int type);
    static xBase* CreateNPCGroup(World::EntityHandleBase* handle,
                                 Sext::NPCGroupAsset* asset,
                                 unsigned int type);

    xAnimTable* _GetAnimTable(unsigned long long id);
};

extern zNPCManager gNPCManager;

xBase* Sext::AnimViewer::Create(World::EntityHandleBase* handle,
                                   AnimViewer* asset) {
    return zNPCManager::CreateNPC(handle, (Sext::NPCAsset*)asset,
                                  400);
}

xBase* Sext::NPCGeneric::Create(World::EntityHandleBase* handle,
                                   NPCGeneric* asset) {
    return zNPCManager::CreateNPC(handle, (Sext::NPCAsset*)asset,
                                  432);
}

xBase* Sext::GenericSpawner::Create(World::EntityHandleBase* handle,
                                       GenericSpawner* asset) {
    return zNPCManager::CreateNPC(handle, (Sext::NPCAsset*)asset,
                                  448);
}

xBase* Sext::GenericSwarm::Create(World::EntityHandleBase* handle,
                                     GenericSwarm* asset) {
    return zNPCManager::CreateNPC(handle, (Sext::NPCAsset*)asset,
                                  416);
}

xBase* Sext::NPCGroupCircle::Create(World::EntityHandleBase* handle,
                                    NPCGroupCircle* asset) {
    return zNPCManager::CreateNPCGroup(
        handle, (Sext::NPCGroupAsset*)asset, 32);
}

// ---------------------------------------------------------------------------
// zNPCBound: the NPC's collision bound, a box in its model's space.

zNPCBound::zNPCBound() {
    center = xVec3::m_Null;
    extent = xVec3::m_Ones;
    mainAxis = eNPCEntityCollMainAxis_Auto;
    modelInst = 0;
    isInit = false;
}

float zNPCBound::GetBoundMinRadiusXZ() const {
    float radius = GetBoundRadiusXZ();

    return (extent.y > radius) ? radius
                               : ((extent.x < extent.z) ? extent.x : extent.z);
}

// A capsule along the longest axis: its radius is the larger of the other
// two extents, and its end points are pulled in by that much but never
// onto one another.
//
// The longest axis is worked out in place: as an inline member the same
// test is emitted and called, where retail has it in line.
void zNPCBound::GetCapsuleLocal(xVec3& a, xVec3& b, float& radius) {
    eNPCEntityCollMainAxis axis;

    if (extent.x < extent.z) {
        if (extent.z < extent.y) {
            axis = eNPCEntityCollMainAxis_Y;
        } else {
            axis = eNPCEntityCollMainAxis_Z;
        }
    } else if (extent.x < extent.y) {
        axis = eNPCEntityCollMainAxis_Y;
    } else {
        axis = eNPCEntityCollMainAxis_X;
    }

    switch (axis) {
    case eNPCEntityCollMainAxis_X:
        radius = (extent.y > extent.z) ? extent.y : extent.z;
        a = center;
        b = center;
        a.x += radius - extent.x;
        b.x += extent.x - radius;

        if (b.x == a.x) {
            b.x += 1e-5f;
        }

        break;
    case eNPCEntityCollMainAxis_Y:
        radius = (extent.x > extent.z) ? extent.x : extent.z;
        a = center;
        b = center;
        a.y += radius - extent.y;
        b.y += extent.y - radius;

        if (b.y == a.y) {
            b.y += 1e-5f;
        }

        break;
    case eNPCEntityCollMainAxis_Z:
        radius = (extent.x > extent.y) ? extent.x : extent.y;
        a = center;
        b = center;
        a.z += radius - extent.z;
        b.z += extent.z - radius;

        if (b.z == a.z) {
            b.z += 1e-5f;
        }

        break;
    }
}

// A cylinder along the bound's main axis -- the template's, or else the
// longest -- spans the whole extent.
void zNPCBound::GetCylinderLocal(xVec3& a, xVec3& b, float& radius) {
    eNPCEntityCollMainAxis axis = mainAxis;

    if (axis == eNPCEntityCollMainAxis_Auto) {
        if (extent.x < extent.z) {
            if (extent.z < extent.y) {
                axis = eNPCEntityCollMainAxis_Y;
            } else {
                axis = eNPCEntityCollMainAxis_Z;
            }
        } else if (extent.x < extent.y) {
            axis = eNPCEntityCollMainAxis_Y;
        } else {
            axis = eNPCEntityCollMainAxis_X;
        }
    }

    switch (axis) {
    case eNPCEntityCollMainAxis_X:
        radius = (extent.y > extent.z) ? extent.y : extent.z;
        a = center;
        b = center;
        a.x -= extent.x;
        b.x += extent.x;

        if (b.x == a.x) {
            b.x += 1e-5f;
        }

        break;
    case eNPCEntityCollMainAxis_Y:
        radius = (extent.x > extent.z) ? extent.x : extent.z;
        a = center;
        b = center;
        a.y -= extent.y;
        b.y += extent.y;

        if (b.y == a.y) {
            b.y += 1e-5f;
        }

        break;
    case eNPCEntityCollMainAxis_Z:
        radius = (extent.x > extent.y) ? extent.x : extent.y;
        a = center;
        b = center;
        a.z -= extent.z;
        b.z += extent.z;

        if (b.z == a.z) {
            b.z += 1e-5f;
        }

        break;
    }
}

// The union of every renderable's box; an animated model's skinned
// geometry of type 11 has a box of its own.
bool zNPCBound::GetModelBoundFromArticle(const World::xOGModel* model,
                                         xVec3& min, xVec3& max) {
    if (model->mModelArt.protoEnt == 0) {
        return false;
    }

    if (model->mModelArt.model.renderableCount == 0) {
        return false;
    }

    bool found = false;
    xBoxV4 box;

    for (unsigned int i = 0; i < model->mModelArt.model.renderableCount;
         i++) {
        Graphics::Geom* geom = model->mModelArt.model.renderables[i]->geom;
        const xBoxV4* bound;

        if (model->Anim != 0) {
            Graphics::GeomSkin* skin = geom->skin;

            if (skin->type == 11) {
                bound = &skin->bound;
            } else {
                bound = &geom->bound;
            }
        } else {
            bound = &geom->bound;
        }

        if (found) {
            if (bound->lower.x < box.lower.x) {
                box.lower.x = bound->lower.x;
            }

            if (bound->lower.y < box.lower.y) {
                box.lower.y = bound->lower.y;
            }

            if (bound->lower.z < box.lower.z) {
                box.lower.z = bound->lower.z;
            }

            if (bound->upper.x > box.upper.x) {
                box.upper.x = bound->upper.x;
            }

            if (bound->upper.y > box.upper.y) {
                box.upper.y = bound->upper.y;
            }

            if (bound->upper.z > box.upper.z) {
                box.upper.z = bound->upper.z;
            }
        } else {
            box = *bound;
            found = true;
        }
    }

    __ct__Q24Math6VectorFfff(&min, box.lower.x, box.lower.y, box.lower.z);
    __ct__Q24Math6VectorFfff(&max, box.upper.x, box.upper.y, box.upper.z);

    return true;
}

bool zNPCBound::GetModelBoundFromCollMesh(const World::xOGModel* model,
                                          xVec3& min, xVec3& max) {
    if (model->mModelArt.protoEnt == 0) {
        return false;
    }

    World::CollisionMeshBlobEntity* mesh =
        model->mModelArt.protoEnt->collisionMesh;

    if (mesh == 0) {
        return false;
    }

    min.x = mesh->center.x - mesh->extent.x;
    min.y = mesh->center.y - mesh->extent.y;
    min.z = mesh->center.z - mesh->extent.z;
    max.x = mesh->center.x + mesh->extent.x;
    max.y = mesh->center.y + mesh->extent.y;
    max.z = mesh->center.z + mesh->extent.z;

    return true;
}

// The template's collision shape says where the box comes from; a unit
// box standing on the ground stands in when nothing gives one. The box
// is scaled, kept from starting above the feet, and offset.
// NEAR MISS: 138 words vs retail 153; four distinct float literals (the
// known wall) -- ours loads them off one lis/addi/addis base, retail
// spells a lis per literal, and the extra base register shifts the rest.
void zNPCBound::Init(const World::xOGModel* model,
                     const Sext::NPCTemplate* tmpl, const xVec3* scale) {
    xVec3 min;
    xVec3 max;

    modelInst = model;
    mainAxis = (eNPCEntityCollMainAxis)tmpl->CollisionMainAxis;

    switch (tmpl->CollisionShape) {
    case 0:
    case 2:
    case 4:
        if (!GetModelBoundFromArticle(model, min, max) &&
            !GetModelBoundFromCollMesh(model, min, max)) {
            __ct__Q24Math6VectorFfff(&min, -0.5f, 0.0f, -0.5f);
            __ct__Q24Math6VectorFfff(&max, 0.5f, 1.0f, 0.5f);
        }

        break;
    case 1:
    case 3:
    case 5:
        __ct__Q24Math6VectorFfff(&min, -0.5f, 0.0f, -0.5f);
        __ct__Q24Math6VectorFfff(&max, 0.5f, 1.0f, 0.5f);
        break;
    case 6:
        if (!GetModelBoundFromCollMesh(model, min, max)) {
            __ct__Q24Math6VectorFfff(&min, -0.5f, 0.0f, -0.5f);
            __ct__Q24Math6VectorFfff(&max, 0.5f, 1.0f, 0.5f);
        }

        break;
    default:
        __ct__Q24Math6VectorFfff(&min, -0.5f, 0.0f, -0.5f);
        __ct__Q24Math6VectorFfff(&max, 0.5f, 1.0f, 0.5f);
        break;
    }

    finalBoundScale.x = scale->x * tmpl->BoundScaleV.x;
    finalBoundScale.y = scale->y * tmpl->BoundScaleV.y;
    finalBoundScale.z = scale->z * tmpl->BoundScaleV.z;

    min.ScaleComponents(finalBoundScale);
    max.ScaleComponents(finalBoundScale);

    if (min.y > 0.0f) {
        min.y = 0.0f;
    }

    center.Add(min, max);
    center *= 0.5f;
    extent.Sub(max, center);
    center += tmpl->BoundOffset;
    isInit = true;
}

// ---------------------------------------------------------------------------
// zNPCEntity

class zCinematic;
class xScene;

class zGlobals {
public:
    unsigned char _pad0[0x43C];
    xScene* sceneCur;
    unsigned char _pad1[0x52C - 0x440];
    zCinematic* runningCinematic;
};

extern zGlobals globals;

class xEntDrive {
public:
    unsigned int flags;
};

void xEntReset(xEnt* ent, const xVec3& pos, const xVec3& rot);
void xEntDriveInit(xEntDrive* drive, xEnt* ent);

class xAnimTransition {
public:
    xAnimTransition* Next;
    xAnimState* Dest;
    unsigned char _pad0[0x1C - 0x8];
    float SrcTime;
    float DestTime;
    unsigned short Priority;
    unsigned short QueuePriority;
    float BlendRecip;
};

xAnimTransition* xAnimTempTransitionAlloc(const xAnimTransition* src);
void xAnimPlayStartTransition(xAnimPlay* play, xAnimTransition* tran);
void xModelUpdate(World::xOGModel* model, float dt);

// xRot's assignment: the linker folded it onto xSphere's, the name every
// call in the image uses.
extern "C" void __as__7xSphereFRC7xSphere(void* dst, const void* src);

void xEntApplyPhysics(xEnt* ent, xScene* sc, float dt);
void xEntMove(xEnt* ent, xScene* sc, float dt);
void xEntDriveUpdate(xEntDrive* drive, xScene* sc, float dt);
void xHavok_SetFrameFromCharacterProxy(xMat4x3* mat, xVec3* vel,
                                       hkpCharacterRigidBody* body);
void xQuatFromMat(xQuat* q, const xMat3x3* m);
void xQuatMul(xQuat* o, const xQuat* a, const xQuat* b);
void xQuatToMat(const xQuat* q, xMat3x3* m);

// A three-component dot product: the linker folded it onto
// hkVector4::dot3, the name the image uses.
extern "C" float dot3__9hkVector4CFRC9hkVector4(const void* a,
                                                const void* b);

typedef void (*xAnimStateCB)(xAnimPlay* play, xAnimState* state,
                             void* data);
typedef void (*xAnimSingleCB)(xAnimState* state, xAnimSingle* single,
                              void* data);
typedef void (*xAnimMatricesCB)(xAnimPlay* play, xQuat* q, xVec3* t,
                                xVec3* s, int n);

xAnimTable* xAnimTableNew(const char* name, unsigned int flags, void* mem,
                          xAnimTable** table);
xAnimState* xAnimTableNewState(xAnimTable* table, const char* name,
                               unsigned int flags, unsigned int userFlags,
                               float speed, float* boneBlend, float* timeSnap,
                               float fadeRecip, unsigned short* fadeOffset,
                               void* callbackData, xAnimStateCB beforeEnter,
                               xAnimStateCB beforeEnd,
                               xAnimSingleCB stateCallback,
                               xAnimMatricesCB beforeAnimMatrices,
                               unsigned long long uid, unsigned int extra);
xAnimFile* xAnimFileNewBilinear(void** rawData, const char* name,
                                unsigned int id, unsigned int flags,
                                xAnimFile** file, unsigned int numX,
                                unsigned int numY, unsigned int numZ,
                                const char* suffix);

inline void xVec3Init(xVec3* v, float x, float y, float z) {
    v->x = x;
    v->y = y;
    v->z = z;
}

inline void xRotCopy(xRot* o, const xRot* r) {
    o->axis.x = r->axis.x;
    o->axis.y = r->axis.y;
    o->axis.z = r->axis.z;
    o->angle = r->angle;
}

// The point collector the character proxy sweeps with; reset clears the
// hits and the two lists this class adds.
class TriggerIdentifyingPointCollectorNoShrapnelOrPuck {
public:
    void reset();

    void* _vtable;
    float m_earlyOutDistance;
    unsigned char _pad0[0x10 - 0x8];
    hkArray<void*> m_hits;
    unsigned char _pad1[0x1B8 - 0x1C];
    hkArray<hkpRigidBody*> m_shrapnelObjectsWeHit;
    hkArray<hkpRigidBody*> m_floatingObjectsWeHit;
};

class zPlayerInventory {
public:
    void ApplyCostume(World::xOGModel& model);
};

class zPlayer {
public:
    static zPlayerInventory inventory;
};

// The animation callbacks the entity installs hand over to the NPC's own,
// if its parameters name one.
void zNPCEntity::NPCBeforeAnimMatrices(xAnimPlay* play, xQuat* q, xVec3* t,
                                       xVec3* s, int n) {
    zNPCEntity* ent = (zNPCEntity*)play->Object;

    if (ent->parameters->beforeAnimMatricesFn != 0) {
        ent->parameters->beforeAnimMatricesFn(ent->owner, play, q, t, s, n);
    }
}

void zNPCEntity::NPCAfterAnimMatrices(xAnimPlay* play, Math::Matrix43* mat,
                                      xQuat* q, xVec3* t, xVec3* s, int n) {
    zNPCEntity* ent = (zNPCEntity*)play->Object;

    if (ent->parameters->afterAnimMatricesFn != 0) {
        ent->parameters->afterAnimMatricesFn(ent->owner, play, mat, q, t, s,
                                             n);
    }
}

int zNPCEntity::EventWrapper(xBase* from, xBase* to, unsigned int event,
                             Sext::EventAny* args) {
    return ((zNPCEntity*)to)->owner->SystemEvent(from, to, event, args);
}

void zNPCEntity::SetParameters(zNPCEntityParams* params) {
    if (owner == 0) {
        parameters = params;
    }
}

// An NPC whose table has both generic states gets a table of its own
// holding the two, each played bilinearly from the original's file.
void zNPCEntity::SetupInstanceAnimTable() {
    xAnimState* generic1 =
        xAnimTableGetStateID(GetAnimTable(), xStrHash("GENERIC1"));
    xAnimState* generic2 =
        xAnimTableGetStateID(GetAnimTable(), xStrHash("GENERIC2"));

    if (generic1 == 0 || generic2 == 0) {
        instanceAnimTable = 0;
    } else {
        instanceAnimTable = xAnimTableNew("NPC Instance", 0, 0, 0);

        xAnimState* state =
            xAnimTableNewState(instanceAnimTable, "GENERIC1", 0, 0, 1.0f, 0,
                               0, 0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
        xAnimFile* file = generic1->Data;

        state->Data = xAnimFileNewBilinear(
            file->RawData, file->Name, file->ID, 0, 0, file->NumAnims[0],
            file->NumAnims[1], file->NumAnims[2], 0);

        state = xAnimTableNewState(instanceAnimTable, "GENERIC2", 0, 0, 1.0f,
                                   0, 0, 0.0f, 0, 0, 0, 0, 0, 0, 0, 0);
        file = generic2->Data;
        state->Data = xAnimFileNewBilinear(
            file->RawData, file->Name, file->ID, 0, 0, file->NumAnims[0],
            file->NumAnims[1], file->NumAnims[2], 0);
    }
}

// The parameters' table when they bring a model, else the character's.
xAnimTable* zNPCEntity::GetAnimTable() {
    xAnimTable* table = 0;

    if (parameters->modelInstAsset != 0 && parameters->animTable != 0) {
        table = parameters->animTable;
    } else if (owner->characterAsset->animTableID != 0) {
        table = gNPCManager._GetAnimTable(owner->characterAsset->animTableID);
    }

    return table;
}

// Leaving the world: the NPC remembers where it was (or starts over from
// its asset when it is not present), lets its listeners go, and gives its
// frame and drive back to the manager.
void zNPCEntity::Detached(zNPCStatus* status) {
    if (owner->present) {
        status->lastPos = model->Mat.pos;

        xVec3 euler;

        xMat3x3GetEuler(&model->Mat, &euler);
        status->lastOrientation = euler;
    } else {
        status->ResetToNPCAsset(owner->npcAsset);
    }

    parameters = 0;
    chkby = 0;
    penby = 0;

    if (characterController.controllerType == PROXY) {
        for (unsigned int i = 0; i < 4; i++) {
            if (collisionListeners[i] != 0) {
                _RemoveCollisionListener(collisionListeners[i]);
            }
        }
    } else if (characterController.controllerType == RIGID_BODY) {
        for (unsigned int i = 0; i < 4; i++) {
            if (rigidBodyCollisionListeners[i] != 0) {
                _RemoveCollisionListener(rigidBodyCollisionListeners[i]);
            }
        }
    }

    Disable();

    if (frame != 0) {
        zNPCManager::factory.DeallocMem(frame);
        frame = 0;
    }

    if (npcDrive != 0) {
        zNPCManager::factory.DeallocMem(npcDrive);
        npcDrive = 0;
    }

    // The model pointer is the first word of its handle.
    ((World::xOGModelHandle*)&model)->Destroy();
}

void zNPCEntity::Enable() {
    if (!enabled) {
        Show();
        baseFlags |= 1;
        EnableCollision();
        enabled = true;
    }
}

void zNPCEntity::Disable() {
    if (enabled) {
        DisableCollision();
        Hide();
        baseFlags &= ~1;
        xAnimVFX_RemovePairs(this);
        xAnimSFX_RemovePairs(this);
        enabled = false;
    }
}

void zNPCEntity::DisableCollision() {
    if (collisionType != eNPCEntityCollType_None) {
        hkpCharacterProxy* proxy = characterController.GetCharacterProxy();
        hkpCharacterRigidBody* crb =
            characterController.GetCharacterRigidBody();

        if (proxy != 0) {
            xHavok_RemoveFromSimWorld(proxy->getShapePhantom());
        } else if (crb != 0) {
            hkpRigidBody* body = Update__20zBTActionHandleEventFf(crb);

            body->addReference();
            xHavok_RemoveFromSimWorld(body);
        }

        characterController.Cleanup();

        if (physicsObject.physicsSystem != 0) {
            xHavok_RemoveFromSimWorld(physicsObject.physicsSystem);
            physicsObject.Cleanup();
        }

        collisionType = eNPCEntityCollType_None;
    }
}

// Four slots each. A listener is only added to the controller once it
// has one, and gets the NPC as it takes a slot.
void zNPCEntity::RegisterCollisionListener(
    zNPCRigidBodyCollisionListener* listener) {
    bool found = false;

    for (int i = 0; i < 4; i++) {
        if (rigidBodyCollisionListeners[i] == listener) {
            found = true;
            break;
        }
    }

    if (!found) {
        for (int i = 0; i < 4; i++) {
            if (rigidBodyCollisionListeners[i] == 0) {
                rigidBodyCollisionListeners[i] = listener;
                listener->npc = owner;
                break;
            }
        }
    }

    _AddCollisionListener(listener);
}

void zNPCEntity::RegisterCollisionListener(zNPCCollisionListener* listener) {
    bool found = false;

    for (int i = 0; i < 4; i++) {
        if (collisionListeners[i] == listener) {
            found = true;
            break;
        }
    }

    if (!found) {
        for (int i = 0; i < 4; i++) {
            if (collisionListeners[i] == 0) {
                collisionListeners[i] = listener;
                listener->npc = owner;
                break;
            }
        }
    }

    _AddCollisionListener(listener);
}

void zNPCEntity::UnregisterCollisionListener(
    zNPCCollisionListener* listener) {
    for (int i = 0; i < 4; i++) {
        if (collisionListeners[i] == listener) {
            collisionListeners[i] = 0;
            listener->npc = 0;
        }
    }

    _RemoveCollisionListener(listener);
}

void zNPCEntity::UnregisterCollisionListener(
    zNPCRigidBodyCollisionListener* listener) {
    for (int i = 0; i < 4; i++) {
        if (rigidBodyCollisionListeners[i] == listener) {
            rigidBodyCollisionListeners[i] = 0;
            listener->npc = 0;
        }
    }

    _RemoveCollisionListener(listener);
}

void zNPCEntity::_AddCollisionListener(zNPCCollisionListener* listener) {
    hkpCharacterProxy* proxy = characterController.GetCharacterProxy();

    if (proxy != 0 &&
        proxy->m_listeners.indexOf((hkpRigidBody* const&)listener) < 0) {
        proxy->addCharacterProxyListener(listener);
    }
}

void zNPCEntity::_AddCollisionListener(
    zNPCRigidBodyCollisionListener* listener) {
    hkpCharacterRigidBody* crb = characterController.GetCharacterRigidBody();

    if (crb != 0) {
        hkpRigidBody* body = Update__20zBTActionHandleEventFf(crb);

        if (body->m_collisionListeners.indexOf(
                (hkpCollisionListener* const&)listener) < 0) {
            listener->body = body;
            body->addCollisionListener(listener);
        }
    }
}

void zNPCEntity::_RemoveCollisionListener(zNPCCollisionListener* listener) {
    hkpCharacterProxy* proxy = characterController.GetCharacterProxy();

    if (proxy != 0 &&
        proxy->m_listeners.indexOf((hkpRigidBody* const&)listener) >= 0) {
        proxy->removeCharacterProxyListener(listener);
    }
}

// As the image has it, the body's listener is removed only when the body
// does NOT list it.
void zNPCEntity::_RemoveCollisionListener(
    zNPCRigidBodyCollisionListener* listener) {
    hkpCharacterRigidBody* crb = characterController.GetCharacterRigidBody();

    if (crb != 0) {
        hkpRigidBody* body = Update__20zBTActionHandleEventFf(crb);

        if (body->m_collisionListeners.indexOf(
                (hkpCollisionListener* const&)listener) < 0) {
            body->removeCollisionListener(listener);
        }
    }
}

// Back to where the status says (or the origin), at the template's scale,
// upright, with fresh bounds and collision flags.
void zNPCEntity::Reset(const zNPCStatus* status) {
    if (status != 0) {
        xEntReset(this, status->lastPos, status->lastOrientation);
    } else {
        xEntReset(this, xVec3::m_Null, xVec3::m_Null);
    }

    baseFlags |= 0x109;
    xMat3x3GetScale(&model->Mat, &modelScale);
    modelScale *= owner->npcTemplate->templateAsset->ModelScaleMult;

    World::xOGModel* m = model;

    if (modelScale != xVec3::m_Ones) {
        m->Scale = modelScale;
    }

    xMat3x3Normalize(&model->Mat, &model->Mat);
    xMat3x3Normalize(&model->Mat, &model->Mat);
    frame->drot.axis = xVec3::m_UnitAxisY;
    frame->rot.axis = xVec3::m_UnitAxisY;
    frame->rot.angle = GetYaw();
    _InitializeBounds(modelScale);
    floorCollision = false;

    if (model != 0) {
        model->SetColorMultiplier(1.0f, 1.0f, 1.0f, 1.0f);
    }

    chkby = parameters->collisionChkBy;
    penby = parameters->collisionPenBy;
    idleNumber = 0;
    queryAnimEndSet = false;
    pokedB = false;
    pokedA = false;
    ResetDamageColor();
    xEntDriveInit(npcDrive, this);
    npcDrive->flags = 2;
    floorVelocity = xVec3::m_Null;
    floorDoesDamage = false;
    floorNormalY = 0.0f;
}

void zNPCEntity::_InitializeBounds(xVec3& scale) {
    npcBound.Init(model, owner->npcTemplate->templateAsset, &scale);
}

void zNPCEntity::GetBoneWorldPosition(int bone, xVec3& pos) {
    xModelGetBoneLocationNoScale(pos, *model, bone);
}

// The render matrix: the model's, scaled when it has a scale, and put
// through the model-only matrix when one is set.
void zNPCEntity::RenderModelInstance() {
    if (modelInstanceAsset != 0) {
        xMat4x3* mat = &model->Mat;
        xMat4x3 scaled;

        if (model->Scale.x > 0.0f) {
            xMat3x3MulScaleC(&scaled, mat, model->Scale.x, model->Scale.y,
                             model->Scale.z);
            scaled.pos = mat->pos;
            mat = &scaled;
        }

        const xMat3x3* extra = modelOnlyMatSet ? &modelOnlyMat : 0;
        xMat4x3 combined;

        if (extra != 0) {
            xMat3x3Mul(&combined, extra, mat);
            combined.pos = mat->pos;
            mat = &combined;
        }

        Math::Matrix43 ngMat;

        __ct__Q24Math8Matrix33Fv(&ngMat);
        xMat4x3ToNGMatrix(&ngMat, mat);

        World::xOGModel* m = model;

        m->mModelArt.model.SetRootTransform(ngMat);
        m->mModelArt.model.CommitWorldTransformAttached();
    }
}

// A blend time of zero, or an NPC nobody can see, switches at once.
void zNPCEntity::SetAnimState(unsigned int animID, float blend,
                              const char* name) {
    if (model == 0 || model->Anim == 0 || model->Anim->Table == 0) {
        return;
    }

    xAnimTransition* tran = xAnimTempTransitionAlloc(0);

    if (0.0f == blend || !visible) {
        tran->BlendRecip = 0.0f;
    } else {
        tran->BlendRecip = 1.0f / blend;
    }

    tran->Dest = xAnimTableGetStateID(model->Anim->Table, animID);

    if (tran->Dest != 0) {
        xAnimPlayStartTransition(model->Anim, tran);
    }

    xModelUpdate(model, 0.0f);
}

void zNPCEntity::SetInstanceAnimState(unsigned int animID, float blend) {
    if (model == 0 || model->Anim == 0 || model->Anim->Table == 0) {
        return;
    }

    xAnimTransition* tran = xAnimTempTransitionAlloc(0);

    if (0.0f == blend) {
        tran->BlendRecip = 0.0f;
    } else {
        tran->BlendRecip = 1.0f / blend;
    }

    tran->Dest = xAnimTableGetStateID(instanceAnimTable, animID);

    if (tran->Dest != 0) {
        xAnimPlayStartTransition(model->Anim, tran);
    }
}

// A state flagged 0x40000000 is a placeholder, not an animation.
bool zNPCEntity::DoesAnimExist(const char* name) {
    if (model == 0 || model->Anim == 0 || model->Anim->Table == 0) {
        return false;
    }

    xAnimState* state =
        xAnimTableGetStateID(model->Anim->Table, xStrHash(name));

    return state != 0 && !(state->UserFlags & 0x40000000);
}

bool zNPCEntity::DoesAnimExist(unsigned int animID) {
    if (model == 0 || model->Anim == 0 || model->Anim->Table == 0) {
        return false;
    }

    xAnimState* state = xAnimTableGetStateID(model->Anim->Table, animID);

    return state != 0 && !(state->UserFlags & 0x40000000);
}

void zNPCEntity::SetAnimState(unsigned int animID, float blend, float time,
                              const char* name) {
    if (model == 0 || model->Anim == 0 || model->Anim->Table == 0) {
        return;
    }

    xAnimTransition* tran = xAnimTempTransitionAlloc(0);

    if (0.0f == blend || !visible) {
        tran->BlendRecip = 0.0f;
    } else {
        tran->BlendRecip = 1.0f / blend;
    }

    tran->Dest = xAnimTableGetStateID(model->Anim->Table, animID);
    tran->DestTime = time;

    if (tran->Dest != 0) {
        xAnimPlayStartTransition(model->Anim, tran);
        xModelUpdate(model, 0.0f);
    }
}

// The current animation's accessors each give up first: retail lays the
// early return ahead of the work, which `if (a && b) work;` does not.
void zNPCEntity::SetCurAnimLerp0(float lerp) {
    if (model == 0 || model->Anim == 0 || model->Anim->Single == 0) {
        return;
    }

    model->Anim->Single->BilinearLerp[0] = lerp;
}

void zNPCEntity::SetCurAnimLerp1(float lerp) {
    if (model == 0 || model->Anim == 0 || model->Anim->Single == 0) {
        return;
    }

    model->Anim->Single->BilinearLerp[1] = lerp;
}

unsigned int zNPCEntity::GetCurAnimID() {
    if (model == 0 || model->Anim == 0 || model->Anim->Single == 0 ||
        model->Anim->Single->State == 0) {
        return 0;
    }

    return model->Anim->Single->State->ID;
}

void zNPCEntity::SetCurAnimSpeed(float speed) {
    if (model == 0 || model->Anim == 0 || model->Anim->Single == 0) {
        return;
    }

    model->Anim->Single->CurrentSpeed = speed;
}

void zNPCEntity::ClearCurAnimLoopCount() {
    if (model == 0 || model->Anim == 0 || model->Anim->Single == 0 ||
        model->Anim->Single->State == 0) {
        return;
    }

    model->Anim->Single->LoopCount = 0;
}

unsigned int zNPCEntity::GetCurAnimLoopCount() {
    if (model == 0 || model->Anim == 0 || model->Anim->Single == 0 ||
        model->Anim->Single->State == 0) {
        return 0;
    }

    return model->Anim->Single->LoopCount;
}

// Stopped when there is nothing playing, and for the asked-for animation
// when a non-looping one has run its length and come to rest.
bool zNPCEntity::IsAnimationStopped(unsigned int animID) {
    xAnimSingle* single;
    xAnimState* state;
    xAnimFile* data;

    if (model == 0 || model->Anim == 0 ||
        (single = model->Anim->Single) == 0 || (state = single->State) == 0 ||
        (data = state->Data) == 0) {
        return true;
    }

    if (animID != 0 && animID != state->ID) {
        return false;
    }

    if (state->Flags & 0x70) {
        return false;
    }

    return single->Time >= data->Duration && 0.0f == single->CurrentSpeed;
}

void TriggerIdentifyingPointCollectorNoShrapnelOrPuck::reset() {
    m_hits.m_size = 0;
    m_earlyOutDistance = 3.40282e+38f;
    m_shrapnelObjectsWeHit.m_size = 0;
    m_floatingObjectsWeHit.m_size = 0;
}

// The animation's turn, when there is one, turns the heading about Y;
// its rotation, when it is not the identity, is put in front of the
// model's. Either change tells the steering.
void zNPCEntity::_ApplyAnimPhysicsRotation(float yaw, xQuat quat) {
    if (!(yaw >= -1e-5f && yaw <= 1e-5f)) {
        xVec3 heading = model->Mat.at;

        heading.rotateY(yaw);
        SetFrameFromHeading(heading);
        owner->npcSteering->_v15();
    }

    if (!(std::fabs(dot3__9hkVector4CFRC9hkVector4(&quat, &quat) +
                    quat.s * quat.s - 1.0f) <= 1e-5f)) {
        xQuat current;

        xQuatFromMat(&current, &model->Mat);
        xQuatMul(&quat, &current, &quat);
        xQuatToMat(&quat, &model->Mat);
        owner->npcSteering->_v15();
    }
}

// The animation's translation this frame, as a velocity added to vel;
// the velocity is also what comes back.
xVec3 zNPCEntity::_ApplyAnimPhysicsTranslationToVel(const xVec3& trans,
                                                    float dt, xVec3& vel) {
    xVec3 v = xVec3::m_Null;

    if (trans.length2() > 1e-5f) {
        v.Scale(trans, 1.0f / dt);
        vel += v;
    }

    return v;
}

// The frame starts from where the model is, and from the rigid body when
// the NPC has one.
void zNPCEntity::PreUpdate(float dt) {
    if (model != 0 && frame != 0) {
        hkpCharacterRigidBody* crb =
            characterController.GetCharacterRigidBody();

        if (crb != 0) {
            xHavok_SetFrameFromCharacterProxy(&model->Mat, &frame->vel, crb);
            frame->vel -= floorVelocity;
        }

        __ct__Q24Math6VectorFfff(&frame->oldvel, frame->vel.x, frame->vel.y,
                                 frame->vel.z);
        frame->oldmat = model->Mat;
        xRotCopy(&frame->oldrot, &frame->rot);
        xVec3Init(&frame->dpos, 0.0f, 0.0f, 0.0f);
        frame->mode = 0;
    }

    entityPosUpdated = false;
    xEntDriveUpdate(npcDrive, xglobals->sceneCur, dt);
}

// Without collision the entity moves itself; with a character controller
// the controller does.
void zNPCEntity::UpdateWithVelocity(float dt) {
    entityPosUpdated = true;
    floorCollision = false;

    switch (collisionType) {
    case eNPCEntityCollType_None: {
        if (flags & 2) {
            xEntApplyPhysics(this, globals.sceneCur, dt);
        }

        xVec3 trans;
        float yaw;
        xQuat quat;

        _EvalAnimPhysics(trans, yaw, quat);
        _ApplyAnimPhysicsRotation(yaw, quat);
        frame->dpos += trans;

        if (flags & 1) {
            xEntMove(this, globals.sceneCur, dt);
        }

        break;
    }
    case eNPCEntityCollType_CharacterProxy:
    case eNPCEntityCollType_CharacterRigidBody:
        _UpdateCharacterProxy(frame->vel, dt);
        break;
    }
}

void zNPCEntity::Paused() {
    if (visible && globals.runningCinematic != 0 && ((flags >> 24) & 1)) {
        xEntHide(this);
    }
}

void zNPCEntity::Resumed() {
    if (visible && !((flags >> 24) & 1)) {
        xEntShow(this);
    }
}

int zNPCEntity::SystemEvent(xBase* from, xBase* to, unsigned int event,
                            Sext::EventAny* args) {
    switch (event) {
    case 0x01E94E6C:
        if (model != 0) {
            zPlayer::inventory.ApplyCostume(*model);
        }

        return 1;
    case 0x27858BA2:
        Show();
        return 1;
    case 0xAE72E9E5:
        Hide();
        return 1;
    case 0xD73685DF:
        idleNumber = *(const unsigned int*)args;
        return 1;
    case 0x9443AB92:
        queryAnimEndSet = true;
        return 1;
    case 0x96DB56DB:
        pokedA = true;
        return 1;
    case 0x96DB56DC:
        pokedB = true;
        return 1;
    }

    return 0;
}

void zNPCEntity::Hide() {
    if (visible) {
        if ((flags >> 24) & 1) {
            xEntHide(this);
        }

        visible = false;
    }
}

// Showing an NPC puts the player's costume back on its model.
void zNPCEntity::Show() {
    if (!visible) {
        if (!((flags >> 24) & 1)) {
            xEntShow(this);
        }

        visible = true;

        if (model != 0) {
            zPlayer::inventory.ApplyCostume(*model);
        }
    }
}

void zNPCEntity::KillVelocity() {
    frame->oldvel = xVec3::m_Null;
    frame->dvel = xVec3::m_Null;
    frame->vel = xVec3::m_Null;
}

void zNPCEntity::SetFrameFromHeading(xVec3& heading) {
    xMat4x3* mat = &model->Mat;

    mat->at = heading;
    mat->up = xVec3::m_UnitAxisY;
    mat->right.x = heading.z;
    mat->right.y = 0.0f;
    mat->right.z = -heading.x;
}

void zNPCEntity::SetFrameFromYaw(float yaw) {
    xVec3 heading;

    heading.x = sin(yaw);
    heading.y = 0.0f;
    heading.z = cos(yaw);

    SetFrameFromHeading(heading);
}

void zNPCEntity::Teleport(const xMat4x3& mat) {
    model->Mat = mat;
    frame->oldmat = mat;
    frame->dpos = xVec3::m_Null;
    frame->dvel = xVec3::m_Null;
    frame->vel = xVec3::m_Null;
    frame->oldvel = xVec3::m_Null;
}

// Member by member, as xEntFrame's own assignment would; the model's
// matrix is then assigned to itself, as the image has it.
void zNPCEntity::CopyFrame(const xEntFrame* src) {
    xEntFrame* f = frame;

    f->oldmat = src->oldmat;
    f->oldvel = src->oldvel;
    __as__7xSphereFRC7xSphere(&f->oldrot, &src->oldrot);
    __as__7xSphereFRC7xSphere(&f->drot, &src->drot);
    __as__7xSphereFRC7xSphere(&f->rot, &src->rot);
    f->dvel = src->dvel;
    f->vel = src->vel;
    f->mode = src->mode;
    f->dpos = src->dpos;
    f->accumRelMat = src->accumRelMat;
    model->Mat = model->Mat;
}

void zNPCEntity::GetBoundCenter(xVec3& center) const {
    World::xOGModel* m = model;
    xVec3 local;

    xMat3x3RMulVec(&local, &m->Mat, &npcBound.center);
    v3add(&center, &local, &m->Mat.pos);
}

void zNPCEntity::UpdateRender() {
    if (model != 0) {
        xMat4x3* mat = &model->Mat;
        xMat4x3 scaled;

        if (model->Scale.x > 0.0f) {
            xMat3x3MulScaleC(&scaled, mat, model->Scale.x, model->Scale.y,
                             model->Scale.z);
            scaled.pos = mat->pos;
            mat = &scaled;
        }

        const xMat3x3* extra = modelOnlyMatSet ? &modelOnlyMat : 0;
        xMat4x3 combined;

        if (extra != 0) {
            xMat3x3Mul(&combined, extra, mat);
            combined.pos = mat->pos;
            mat = &combined;
        }

        Math::Matrix43 ngMat;

        __ct__Q24Math8Matrix33Fv(&ngMat);
        xMat4x3ToNGMatrix(&ngMat, mat);
        model->mModelArt.ApplyRenderCustomizers(model->renderCustomizerMask);

        World::xOGModel* m = model;

        m->mModelArt.model.SetRootTransform(ngMat);
        m->mModelArt.model.CommitWorldTransformAttached();
    }
}

void zNPCEntity::ResetDamageColor() {
    damageColorTimer = 0.0f;
    currentColorMultiplier = 1.0f;
    model->SetColorMultiplier(1.0f, 1.0f, 1.0f, -1.0f);
}

// ---------------------------------------------------------------------------
// Inline, and called rather than inlined in retail, which emits them weak
// here: defined last so that nothing above can inline them.

inline void xHavokCharacterController::Cleanup() {
    if (characterProxy != 0) {
        characterProxy->removeReference();
    }

    controllerType = NONE;
}

inline hkpCharacterRigidBody*
xHavokCharacterController::GetCharacterRigidBody() const {
    if (controllerType == RIGID_BODY) {
        return characterRigidBody;
    }

    return 0;
}
