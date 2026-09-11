#include "SB/GM/Engine/Game/zNPCCommonBTActions.pool.h"

// zNPCCommonBTActions.cpp -- the common behaviour-tree actions an NPC runs:
// idle, play an effect or an animation, rotate to face, show, hide, fade,
// collision, collectibles, the spawner actions. Read from the image with
// tools/disasm.py; the layouts are the DWARF's (tools/dwarf_types.py), and
// the strings and float literals come out of WAD02.cpp's pool, which the
// header in front reproduces.
//
// Every action keeps its asset, a resume callback and its client in front
// of the vtable pointer at +0xC, and the NPC it drives at +0x10. The NPC's
// steering is polymorphic from +4: Begin hands it the action's control
// (slot 10), End takes it back (slot 11), Update steps it (slot 14). The
// status an Update returns is 1 while running, 3 when done and 4 when it
// cannot run.
//
// Math::Vector is declared twelve bytes here: PositionEntAtBone keeps one
// at 20(r1) with a matrix at 32(r1), so the local retail constructs there
// is twelve bytes whatever its class was called -- the linker folded its
// constructor onto Math::Vector's, which is the symbol every call names.

extern "C" int snprintf(char* s, unsigned long n, const char* format, ...);
extern "C" void* memcpy(void* dst, const void* src, unsigned long n);

namespace Sext {

class ActionBase {};

struct vec3 {
    float x;
    float y;
    float z;
};

class uid {
public:
    unsigned long long internalUid;
};

class Action_NPC_PlayFX : public ActionBase {
public:
    unsigned long long fx;
    int bone;
    vec3 offset;
    bool kill;
};

class Action_NPC_PlayAnimationType : public ActionBase {
public:
    unsigned int Animation;
    bool ForceRestart;
    unsigned int EventData;
    float Acceleration;
    int Heading;
};

// Rotate-to-face: what to face, whether facing it ends the action, the
// turn rate (radians a second) and the tolerance (degrees).
class Action_NPC_RotateToFace : public ActionBase {
public:
    int type;
    bool stopWhenFacing;
    float speed;
    float tolerance;
};

class Action_NPC_PlayEELFX : public ActionBase {
public:
    unsigned long long ElectricArcUID;
};

class Action_NPC_GenerateCollectibles : public ActionBase {
public:
    unsigned int BoneIndex;
    vec3 Offset;
    int Type;
};

class Action_NPC_StartHeadTracking : public ActionBase {
public:
    union {
        unsigned char Player;
        unsigned int Variable;
    };
    unsigned int TargetType;
};

class Action_NPC_PositionEntAtBone : public ActionBase {
public:
    unsigned long long ent;
    int bone;
    vec3 offset;
    bool done;
};

class Action_NPC_GenericSpawnerInit : public ActionBase {
public:
    unsigned int spawnDelayVar;
    float spawnDelayDefault;
};

class Action_NPC_InstantSpawnNPC : public ActionBase {
public:
    unsigned int MPVariableName;
    bool SetMovePoint;
};

enum eNPCCollectibleType { eNPCCollectibleType_ = 0x7FFFFFFF };

class EventAny;

}  // namespace Sext

class xBase;
class xEnt;
class xEffectAttachIntf;
class zBTClient;
class zCommonPlayer;
class hkpPhysicsSystem;

enum ForceEvent { ForceEvent_ = 0x7FFFFFFF };
enum ePlayerName { ePlayerName_ = 0x7FFFFFFF };
enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {

enum GlobalHeapEnum {
    GlobalHeap,
    GlobalHeapMain,
    GlobalHeapSecondary,
};

void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap, eMemMgrTag tag,
                      bool zeroed);
void FreeGlobalHeap(void* block, GlobalHeapEnum heap);

}  // namespace Memory

inline void* operator new(unsigned long, void* p) { return p; }

// const H&: a constant heap binds to an unnamed four-byte static, and the
// load happens where the reference binds -- ahead of the null test -- as in
// WAD00_1.cpp.
template <class H>
inline void Free(const H& heap, void* p) {
    H h = heap;

    if (p != 0) {
        Memory::FreeGlobalHeap(p, h);
    }
}

void zEntEvent(xBase* from, unsigned int fromEvent, xBase* to,
               unsigned int toEvent, Sext::EventAny* args, ForceEvent force);

class xVec3 {
public:
    xVec3& operator+=(const xVec3& other);
    bool operator==(const xVec3& other) const;
    bool operator!=(const xVec3& other) const { return !(*this == other); }

    static const xVec3 m_Null;
    static const xVec3 m_DoubleVec;

    float x;
    float y;
    float z;
};

xVec3 operator-(const xVec3& a, const xVec3& b);

inline void xVec3Init(xVec3* v, float x, float y, float z) {
    v->x = x;
    v->y = y;
    v->z = z;
}

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

void xMat3x3RMulVec(xVec3* o, const xMat3x3* m, const xVec3* v);
void xMat3x3LMulRotY(xMat3x3* o, const xMat3x3* m, float t);
void v3add(xVec3* o, xVec3* a, xVec3* b);
float xatan2(float y, float x);
float xClampAngle0_2PI(float a);
float xClampAnglePI_PI(float a);

namespace Math {

class Vector4 {
public:
    Vector4& Assign(float x, float y, float z, float w);

    float x;
    float y;
    float z;
    float w;
};

class Vector {
public:
    Vector(float x, float y, float z);

    float x;
    float y;
    float z;
};

}  // namespace Math

// Constructs a Math::Vector in place, at a member: this compiler rejects
// `p->T::T()` and a placement new tests its pointer, so the constructor is
// called by its own symbol, as zNPCStatus.cpp does.
extern "C" void __ct__Q24Math6VectorFfff(void* self, float x, float y,
                                         float z);

namespace Graphics {

class RenderableSceneRef {
public:
    void UpdateIsAlpha(float alpha);
};

class Renderable {
public:
    // In the class body: retail's copy is the weak one the pointer to
    // member in xOGModel::SetColorMulAlpha names.
    void SetColorMulAlpha(float alpha) {
        float a = alpha * ((Math::Vector4*)((char*)paramData +
                                            colorMulHandle))->w;

        localColorMul.w = a;

        if (sceneRef != 0) {
            sceneRef->UpdateIsAlpha(a);
        }
    }

    unsigned char _pad0[0x10];
    RenderableSceneRef* sceneRef;
    void* paramData;
    void* instRenderMode;
    Math::Vector4 localColorMul;
    void* geom;
    float hackAlphaFade;
    unsigned short colorMulHandle;
};

class Renderable3D : public Renderable {};

}  // namespace Graphics

namespace World {

class EntityHandleBase {
public:
    unsigned char _pad0[0x48];
};

class EntityManager {
public:
    static void* FindAsset(unsigned long long id);
};

EntityManager* GetEntityManager();

class xOGEntity;
class xOGModelRef;

class xOGModel {
public:
    // Out of range keeps the current alpha. Every renderable with a colour
    // multiplier gets the new one through a pointer to member, declared
    // inside the loop as retail reloads it every pass.
    void SetColorMulAlpha(float alpha) {
        if (alpha < 0.0f || alpha > 1.0f) {
            alpha = colorMultiplier.w;
        }

        colorMultiplier.Assign(colorMultiplier.x, colorMultiplier.y,
                               colorMultiplier.z, alpha);

        for (int i = 0; i < renderableCount; i++) {
            Graphics::Renderable3D* r = renderables[i];

            if (r->colorMulHandle != 0xFFFF) {
                void (Graphics::Renderable::*fn)(float) =
                    &Graphics::Renderable::SetColorMulAlpha;

                (r->*fn)(alpha);
            }
        }
    }

    xMat4x3 Mat;
    unsigned char _pad0[0xB4 - 0x40];
    Math::Vector4 colorMultiplier;
    unsigned char _pad1[0x11C - 0xC4];
    unsigned short renderableCount;
    unsigned short renderCustomizerCount;
    Graphics::Renderable3D** renderables;
};

}  // namespace World

// The engine's objects as the actions reach them: World::Entity's type id
// at +0x10, xBase's type and flags, and an entity's model at +0x34.
class xBase {
public:
    unsigned char _pad0[0x10];
    unsigned int entityType;
    unsigned char _pad1[0x20 - 0x14];
    unsigned int baseType;
    unsigned char _pad2[0x26 - 0x24];
    unsigned short baseFlags;
    unsigned char _pad3[0x34 - 0x28];
    World::xOGModel* model;
};

class xOGEntityData : public xBase {
public:
    unsigned char _pad4[0x3C - 0x38];
    void* asset;
};

class xMovePoint : public xBase {
public:
    unsigned char _pad4[0x40 - 0x38];
    xVec3* pos;
};

class zTrigger : public xBase {
public:
    void UpdateTriggerEntry();

    unsigned char _pad4[0x3C - 0x38];
    xMat4x3 mat;
};

xBase* zSceneFindObject(unsigned long long id);

void xEntReset(xEnt* ent, const xVec3& pos, const xVec3& rot);

void xModelGetBoneMatNoScale(xMat4x3& mat, const World::xOGModel& model,
                             unsigned long index);
void xModelGetBoneLocationNoScale(xVec3& loc, const World::xOGModel& model,
                                  unsigned long index);

class zPlayer {
public:
    unsigned char _pad0[0x34];
    World::xOGModel* model;
};

class zPlayerContainer {
public:
    zPlayer* playerArray[4];
    int numPlayers;
};

class xGlobals {
public:
    unsigned char _pad0[0x428];
    zPlayerContainer players;
};

extern xGlobals* xglobals;

zPlayer* GetPlayerByName(zCommonPlayer* from, ePlayerName name);

class xEntFrame {
public:
    unsigned char _pad0[0x7C];
    xVec3 dvel;
    xVec3 vel;
};

class xAnimState;
class xAnimTable;

xAnimState* xAnimTableGetStateID(xAnimTable* table, unsigned int id);
void xAnimSetRawData(xAnimState* state, void* data, int size);

namespace FX {

class zFXSpawn {
public:
    void Init(zFXSpawn* src, xEffectAttachIntf* attach,
              World::xOGModelRef* modelRef, int bone, const xVec3* pos,
              const xVec3* a, const xVec3* b, const xMat3x3* mat, bool flag);

    unsigned char _pad0[0xB4];
    unsigned int flags;
};

}  // namespace FX

class zFXScriptSpawnPtMgr {
public:
    static FX::zFXSpawn* GetNewPoolSpawnPoint(const char* name);
    static void ReturnPoolSpawnPoint(FX::zFXSpawn* spawn);
    void _ImmediateReturnPoolSpawnPoint(FX::zFXSpawn* spawn);
};

namespace SpawnPointMgr {
namespace Local {

extern bool sInvalidSceneState;
extern zFXScriptSpawnPtMgr* fxScriptSpawnPtMgr;

}  // namespace Local
}  // namespace SpawnPointMgr

// xEnt is polymorphic from +0; slot 20 hands out the model reference an
// effect attaches to.
class xEntVirtuals {
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
    virtual World::xOGModelRef* GetAttachModel();
};

class zNPCEntity : public xEntVirtuals {
public:
    bool DoesAnimExist(unsigned int animID);
    bool IsAnimationStopped(unsigned int animID);
    unsigned int GetCurAnimID();
    unsigned int GetCurAnimLoopCount();
    void ClearCurAnimLoopCount();
    void SetCurAnimSpeed(float speed);
    void Show();
    void Hide();
    void EnableCollision();
    void DisableCollision();

    unsigned char _pad0[0x34 - 0x4];
    World::xOGModel* model;
    unsigned char _pad1[0x58 - 0x38];
    xEntFrame* frame;
    unsigned char _pad2[0x178 - 0x5C];
    xAnimTable* instanceAnimTable;
    unsigned int idleNumber;
    unsigned char _pad3[0x1C4 - 0x180];
    int collisionType;
};

class zNPCFX {
public:
    bool RunFX(unsigned int nameHash);
    bool IsFXRunning(unsigned int nameHash);
    void StopFX(unsigned int nameHash);
};

class zNPCSteeringControl;

class zNPCSteeringData {
public:
    int f0;
};

class zNPCSteering : public zNPCSteeringData {
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
    virtual void _v10(zNPCSteeringControl* control);
    virtual void _v11(zNPCSteeringControl* control);
    virtual void _v12();
    virtual void _v13();
    virtual void _v14(float dt);
    virtual void _v15();
};

class zCollectibleSpawner {
public:
    void AddRequest(const char* name, const xVec3& pos, const xVec3& scale,
                    World::xOGEntity* owner);
};

class zNPCTemplate {
public:
    zCollectibleSpawner* GetCollectibleSpawner(
        Sext::eNPCCollectibleType type) const;
};

class zNPCBase {
public:
    void Spawn();
    zPlayer* GetClosestPlayerOnWallnet(bool onWallnet);

    unsigned char _pad0[0x18];
    unsigned long long id;
    unsigned char _pad1[0x71 - 0x20];
    bool infoNodesUpdating : 1;
    bool quietKill : 1;
    bool updating : 1;
    bool flying : 1;
    bool collectible : 1;
    unsigned char _pad2[0x78 - 0x72];
    zNPCTemplate* npcTemplate;
    void* btClient;
    xMovePoint* npcMovePoint;
    unsigned char _pad3[0x98 - 0x84];
    zNPCEntity* npcEntity;
    void* npcSteeringOld;
    zNPCSteering* npcSteering;
    void* npcPerception;
    void* npcCombat;
    void* npcQuickTimeCombat;
    zNPCFX* npcFX;
    unsigned char _pad4[0xC0 - 0xB4];
};

class zNPCGeneric : public zNPCBase {};

class zNPCGenericPool {
public:
    zNPCGeneric* GetNextInactiveNPC(unsigned long long id);
    void RemoveFromReserved(zNPCGeneric* npc);
};

class GenericSpawner {
public:
    unsigned char _pad0[0x1B4];
    float spawnDelay;
};

class zNPCGenericSpawner : public zNPCBase {
public:
    void GetSpawnPos(xVec3* pos, const zNPCGeneric* npc);
    void NPCWasSpawned(zNPCGeneric* npc);

    unsigned char _pad5[0x1CC - 0xC0];
    GenericSpawner* assetSpawner;
    zNPCGenericPool* genericPool;
    zNPCGeneric* myReservedNPC;
};

// ---------------------------------------------------------------------------
// The blackboard, as zBlackboard.cpp spells it. Only Read<zVariableEventData>
// is defined here -- retail emits it in this unit -- and every other Read and
// Write is zBlackboard.cpp's.

enum eVarType {
    eVarType_Invalid = 0,
    eVarType_S32 = 1,
    eVarType_F32 = 2,
    eVarType_xVec3 = 3,
    eVarType_xBasePtr = 4,
    eVarType_zInteractionUpPtr = 5,
    eVarType_zPlayerPtr = 6,
    eVarType_xMovePointPtr = 7,
    eVarType_UID = 8,
    eVarType_zUpFloatingObjectPtr = 9,
    eVarType_zVariableEventData = 10
};

// An event copied into a variable carries its payload with it: the data is
// copied into the variable's own buffer and the pointer re-aimed at it.
class zVariableEventData {
public:
    zVariableEventData() : eventID(0), dataPointer(0) {}

    unsigned int eventID;
    Sext::EventAny* dataPointer;
    char eventDataBuffer[64];
};

class zVariableBase {
public:
    unsigned int id;
    eVarType type;
    unsigned int flags;
    void* observers[8];

    virtual void OnReset();
};

template <class T>
class zVariable : public zVariableBase {
public:
    T defaultValue;
    T value;
};

template <class T>
struct zVariableTypeOf;

template <>
struct zVariableTypeOf<zVariableEventData> {
    enum { kType = eVarType_zVariableEventData };
};

class zVariableDynamicCast {
public:
    template <class T>
    static void Cast(zVariableBase* v, zVariable<T>*& out);
};

// Spelled out for the one T this unit reads. mwcc instantiates a template
// at the END of the unit, and Read<zVariableEventData> below is an explicit
// specialization compiled where it stands, so a generic Cast<T> would not
// exist yet to inline there: it would be emitted and called.
template <>
inline void zVariableDynamicCast::Cast<zVariableEventData>(
    zVariableBase* v, zVariable<zVariableEventData>*& out) {
    if (v->type == eVarType_zVariableEventData) {
        out = (zVariable<zVariableEventData>*)v;
    } else {
        out = 0;
    }
}

class zBlackboard {
public:
    unsigned int size;
    zVariableBase** variables;

    zVariableBase* Find(unsigned int id) const;
    eVarType GetVariableType(unsigned int id) const;

    template <class T>
    bool Write(unsigned int id, const T& value);
    template <class T>
    bool Read(unsigned int id, T& out) const;
};

template <>
bool zBlackboard::Read<zVariableEventData>(unsigned int id,
                                           zVariableEventData& out) const;

class zBTClient {
public:
    unsigned char _pad0[0x88];
    zBlackboard blackboard;
};

extern unsigned int NPC_VAR_ROTATE_TO_FACE_POS;
extern unsigned int NPC_VAR_CURRENT_DESTINATION;
extern unsigned int GENERIC_HASHES[2];
extern const char* GENERIC_NAMES[2];

// ---------------------------------------------------------------------------
// Steering controls and the animation an action plays

// The destination is spelled as three floats, not an xVec3: retail's
// SetCustomHeading copies the sixteen bytes as a block of words in line,
// and a struct holding an xVec3 gets a synthesized operator= out of line.
class zNPCSteeringDest {
public:
    float dest[3];
    xVec3* pDest;
};

class zNPCSteeringControl {
public:
    void SetCustomHeading(zNPCSteeringDest& dest);
    void SetMaxAcc(float acc);

    void* steering;
    zNPCBase* npcBase;
    zNPCEntity* npcEntity;
    void* wallNetPosition;
    int headingCalcType;
    zNPCSteeringDest customHeading;
    float speedLimitXZ;
    float speedLimitY;
    unsigned int flags;
    unsigned char movementStyle;
    float maxAcc;
    unsigned char accLimiter[0x10];

    virtual void _v0();
};

class zNPCSteeringStopControl : public zNPCSteeringControl {};

class zNPCBTActionAnim {
public:
    void Init(const char* name, float blend, float start);

    // In the class body: retail's symbol is weak, and its one literal is
    // the @STRING@ object named after it.
    void Init(unsigned int id, float blend, float start) {
        animationName = (char*)"UNKNOWN";
        animStateID = id;
        blendTime = blend;
        animStartTime = start;
    }

    void StartOnNPC(zNPCEntity* npc, bool force);
    void StartInstanceAnimOnNPC(zNPCEntity* npc, bool force);

    char* animationName;
    unsigned int animStateID;
    float blendTime;
    float animStartTime;
    float leanMaxAngle;
    bool enabled;
};

class zBTActionData {
public:
    const Sext::ActionBase* actionAsset;
    void* resumeCB;
    zBTClient* btClient;
};

class zBTAction : public zBTActionData {
public:
    virtual void _v0();
    virtual void _v1();
    virtual void _v2();
    virtual void _v3();
    virtual void _v4();
    virtual void _v5();
};

class zNPCBTAction : public zBTAction {
public:
    zNPCBase* npcBase;
};

enum IdleType {
    Normal = 0,
    FidgetCommon = 1,
    FidgetRare = 2,
};

class zNPCBTIdleAction : public zNPCBTAction {
public:
    void ActionAnimInit(const char* name);
    void Initialize();
    void Setup(const Sext::ActionBase* asset);
    void Begin();
    int Update(float dt);
    void End();

    zNPCSteeringStopControl stopControl;
    zNPCBTActionAnim actionAnim;
    bool fidget;
    unsigned int animationPlays;
    IdleType idleType;
};

class zNPCBTPlayFXAction : public zNPCBTAction {
public:
    void Setup(const Sext::ActionBase* asset);
    void Begin();
    int Update(float dt);
    void End();

    FX::zFXSpawn* fxSpawn;
    xVec3 pos;
};

class zNPCBTPlayAnimationAction : public zNPCBTAction {
public:
    void Setup(const Sext::ActionBase* asset);
    void Begin();
    int Update(float dt);
    void End();

    unsigned char _pad0[0x18 - 0x14];
    unsigned long long animID;
    zNPCBTActionAnim actionAnim;
    bool valid;
    zNPCSteeringStopControl stopControl;
};

class zNPCBTPlayAnimationTypeAction : public zNPCBTAction {
public:
    void Initialize();
    void Setup(const Sext::ActionBase* asset);
    void Begin();
    int Update(float dt);
    void End();

    unsigned int animID;
    unsigned int eventVarID;
    zNPCBTActionAnim actionAnim;
    bool valid;
    zNPCSteeringStopControl stopControl;
    int headingCalcType;
    zNPCSteeringDest steeringDest;
    float acceleration;
    bool forceRestart;
};

class zNPCBTRotateToFaceAction : public zNPCBTAction {
public:
    int Update(float dt);
};

class zNPCBTShowAction : public zNPCBTAction {
public:
    int Update(float dt);
};

class zNPCBTHideAction : public zNPCBTAction {
public:
    int Update(float dt);
};

class zNPCBTFadeOutAction : public zNPCBTAction {
public:
    void Begin();
    int Update(float dt);

    float elapsedTime;
};

class zNPCBTFadeInAction : public zNPCBTAction {
public:
    int Update(float dt);

    float elapsedTime;
};

class zNPCBTSetCollidesAction : public zNPCBTAction {
public:
    void Setup(const Sext::ActionBase* asset);
    int Update(float dt);

    bool collides;
};

class zNPCBTPlayNPCFXAction : public zNPCBTAction {
public:
    void Setup(const Sext::ActionBase* asset);
    void Begin();
    int Update(float dt);
    void End();

    unsigned int FXNameHash;
    bool FXFound;
};

class ElectricArc {
public:
    ElectricArc(World::EntityHandleBase* handle);

    void Setup();
    void Release();

    virtual ~ElectricArc();
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
    virtual void Init(void* asset);
    virtual void Update(float dt);
};

class zNPCBTPlayEELFXAction : public zNPCBTAction {
public:
    void Setup(const Sext::ActionBase* asset);
    void Begin();
    int Update(float dt);
    void End();
    void Deinitialize();

    ElectricArc* arc;
    const Sext::Action_NPC_PlayEELFX* asset;
};

class zNPCBTGenerateCollectiblesAction : public zNPCBTAction {
public:
    void Setup(const Sext::ActionBase* asset);
    int Update(float dt);

    unsigned int boneIndex;
    xVec3 offset;
    Sext::eNPCCollectibleType type;
};

class zNPCBTSetCollectibleAction : public zNPCBTAction {
public:
    int Update(float dt);
};

class zNPCBTStartHeadTrackingAction : public zNPCBTAction {
public:
    int Update(float dt);
};

class xHavokPhysicsObject {
public:
    void SetCollisionFilter(unsigned int filter);
    void Deactivate();
    void Cleanup();

    int physicsObjectType;
    void* packedPhysicsData;
    hkpPhysicsSystem* physicsSystem;
    unsigned char _pad0[0x20 - 0xC];
};

void xHavok_RemoveFromSimWorld(const hkpPhysicsSystem* system);

class zNPCBTExtraCollisionAction : public zNPCBTAction {
public:
    void End();
    void Cleanup();

    unsigned char _pad0[0x20 - 0x14];
    xHavokPhysicsObject physicsObject;
};

class zNPCBTPositionEntAtBoneAction : public zNPCBTAction {
public:
    int Update(float dt);
};

class zNPCBT_GenericSpawnerInit_Action : public zNPCBTAction {
public:
    void Setup(const Sext::ActionBase* value);
    int Update(float dt);

    const Sext::Action_NPC_GenericSpawnerInit* asset;
};

class zNPCBT_SpawnNPC_ThrowToLocation_Action : public zNPCBTAction {
public:
    void End();

    zNPCGeneric* NPCToSpawn;
};

class zNPCBT_Spawner_SetRotateToFaceVariable : public zNPCBTAction {
public:
    int Update(float dt);
};

class zNPCBT_Spawner_UnreserveNPC : public zNPCBTAction {
public:
    int Update(float dt);
};

class zNPCBT_InstantSpawnNPC_Action : public zNPCBTAction {
public:
    void Begin();
    int Update(float dt);
    void End();

    const Sext::Action_NPC_InstantSpawnNPC* asset;
    zNPCGenericSpawner* spawner;
    zNPCGeneric* NPCToSpawn;
    xMovePoint* pMovePoint;
};

// The handle every arc is made on. Retail's is in WAD02.cpp's unnamed
// namespace; only its address is used here.
extern World::EntityHandleBase dummyHandle;

// ---------------------------------------------------------------------------
// zNPCBTIdleAction

// A variant NPC plays "<name>_<variant>" when it has one, and the plain
// name when it has none or the variant's animation does not exist.
void zNPCBTIdleAction::ActionAnimInit(const char* name) {
    unsigned int variant = npcBase->npcEntity->idleNumber;

    if (variant != 0) {
        char buf[32];

        snprintf(buf, 32, "%s_%u", name, variant);
        actionAnim.Init(buf, 0.2f, 0.0f);

        if (!npcBase->npcEntity->DoesAnimExist(actionAnim.animStateID)) {
            actionAnim.Init(name, 0.2f, 0.0f);
        }
    } else {
        actionAnim.Init(name, 0.2f, 0.0f);
    }
}

void zNPCBTIdleAction::Initialize() { stopControl.headingCalcType = 0; }

void zNPCBTIdleAction::Setup(const Sext::ActionBase* asset) {
    fidget = *(const bool*)asset;
}

void zNPCBTIdleAction::Begin() {
    idleType = Normal;
    animationPlays = 0;

    ActionAnimInit("IDLE");
    actionAnim.StartOnNPC(npcBase->npcEntity, false);

    npcBase->npcSteering->_v10(&stopControl);
}

// Every third completed play is a fidget, and every third fidget a rare
// one; a fidget waits for one loop of the animation and an idle for two.
int zNPCBTIdleAction::Update(float dt) {
    npcBase->npcSteering->_v14(dt);

    if (npcBase->npcEntity->IsAnimationStopped(actionAnim.animStateID)) {
        return 3;
    }

    if (fidget) {
        unsigned int loops = npcBase->npcEntity->GetCurAnimLoopCount();
        unsigned int needed = 2;

        if (idleType >= FidgetCommon && idleType <= FidgetRare) {
            needed = 1;
        }

        if (loops >= needed) {
            animationPlays += loops;
            npcBase->npcEntity->ClearCurAnimLoopCount();

            if ((animationPlays + 1) % 3 == 0) {
                if (((animationPlays + 1) / 3) % 3 == 0) {
                    idleType = FidgetRare;
                    ActionAnimInit("FIDGET_RARE");

                    if (!npcBase->npcEntity->DoesAnimExist(
                            actionAnim.animStateID)) {
                        ActionAnimInit("FIDGET_COMMON");
                    }
                } else {
                    idleType = FidgetCommon;
                    ActionAnimInit("FIDGET_COMMON");
                }

                if (!npcBase->npcEntity->DoesAnimExist(
                        actionAnim.animStateID)) {
                    ActionAnimInit("IDLE");
                }
            } else {
                idleType = Normal;
                ActionAnimInit("IDLE");
            }

            actionAnim.StartOnNPC(npcBase->npcEntity, false);
        }
    }

    return 1;
}

void zNPCBTIdleAction::End() { npcBase->npcSteering->_v11(&stopControl); }

// ---------------------------------------------------------------------------
// zNPCBTPlayFXAction

void zNPCBTPlayFXAction::Setup(const Sext::ActionBase* asset) { fxSpawn = 0; }

// A bone of zero or more attaches the effect to the NPC's model; a negative
// one places it once, at the asset's offset carried through the model.
void zNPCBTPlayFXAction::Begin() {
    const Sext::Action_NPC_PlayFX* asset =
        (const Sext::Action_NPC_PlayFX*)actionAsset;
    FX::zFXSpawn* fx = (FX::zFXSpawn*)zSceneFindObject(asset->fx);
    int bone = asset->bone;
    Math::Vector offset(asset->offset.x, asset->offset.y, asset->offset.z);

    if (fx != 0) {
        fxSpawn = zFXScriptSpawnPtMgr::GetNewPoolSpawnPoint("Action_PlayFX");

        if (fxSpawn != 0) {
            if (bone >= 0) {
                fxSpawn->Init(fx, 0, npcBase->npcEntity->GetAttachModel(), bone,
                              0, 0, 0, 0, false);
            } else {
                xMat3x3RMulVec(&pos, &npcBase->npcEntity->model->Mat,
                               (const xVec3*)&offset);
                pos += npcBase->npcEntity->model->Mat.pos;

                fxSpawn->Init(fx, 0, 0, -1, &pos, 0, 0,
                              &npcBase->npcEntity->model->Mat, false);
            }
        }
    }
}

// The effect follows the NPC: its position is the asset's offset carried
// through the model's matrix. The spawn's low flag bit says it is still
// playing.
int zNPCBTPlayFXAction::Update(float dt) {
    if (fxSpawn == 0) {
        return 4;
    }

    const Sext::Action_NPC_PlayFX* asset =
        (const Sext::Action_NPC_PlayFX*)actionAsset;
    Math::Vector offset(asset->offset.x, asset->offset.y, asset->offset.z);

    xMat3x3RMulVec(&pos, &npcBase->npcEntity->model->Mat,
                   (const xVec3*)&offset);
    pos += npcBase->npcEntity->model->Mat.pos;

    return (fxSpawn->flags & 1) ? 1 : 3;
}

void zNPCBTPlayFXAction::End() {
    if (fxSpawn != 0) {
        if (((const Sext::Action_NPC_PlayFX*)actionAsset)->kill) {
            if (!SpawnPointMgr::Local::sInvalidSceneState && fxSpawn != 0) {
                SpawnPointMgr::Local::fxScriptSpawnPtMgr
                    ->_ImmediateReturnPoolSpawnPoint(fxSpawn);
            }
        } else {
            zFXScriptSpawnPtMgr::ReturnPoolSpawnPoint(fxSpawn);
        }

        fxSpawn = 0;
    }
}

// ---------------------------------------------------------------------------
// zNPCBTPlayAnimationAction

void zNPCBTPlayAnimationAction::Setup(const Sext::ActionBase* asset) {
    animID = *(const unsigned long long*)asset;
    valid = false;
}

// The animation is raw data played on the NPC's instance table, under
// whichever of the two generic states the NPC is not already in.
void zNPCBTPlayAnimationAction::Begin() {
    npcBase->npcSteering->_v10(&stopControl);

    int idx = 0;

    if (GENERIC_HASHES[0] == npcBase->npcEntity->GetCurAnimID()) {
        idx = 1;
    }

    actionAnim.Init(GENERIC_NAMES[idx], 0.2f, 0.0f);

    void* data = World::GetEntityManager()->FindAsset(animID);

    if (data != 0 && npcBase->npcEntity->instanceAnimTable != 0) {
        xAnimSetRawData(xAnimTableGetStateID(
                            npcBase->npcEntity->instanceAnimTable,
                            GENERIC_HASHES[idx]),
                        data, 0);
        actionAnim.StartInstanceAnimOnNPC(npcBase->npcEntity, false);
        npcBase->npcEntity->SetCurAnimSpeed(1.0f);
        valid = true;
    }
}

int zNPCBTPlayAnimationAction::Update(float dt) {
    npcBase->npcSteering->_v14(dt);

    if (!valid) {
        return 4;
    }

    return npcBase->npcEntity->IsAnimationStopped(actionAnim.animStateID)
               ? 3
               : 1;
}

void zNPCBTPlayAnimationAction::End() {
    npcBase->npcSteering->_v11(&stopControl);
}

// ---------------------------------------------------------------------------
// zNPCBTPlayAnimationTypeAction

void zNPCBTPlayAnimationTypeAction::Initialize() {
    stopControl.headingCalcType = 0;
}

void zNPCBTPlayAnimationTypeAction::Setup(const Sext::ActionBase* a) {
    const Sext::Action_NPC_PlayAnimationType* asset =
        (const Sext::Action_NPC_PlayAnimationType*)a;

    animID = asset->Animation;
    forceRestart = asset->ForceRestart;
    eventVarID = asset->EventData;
    acceleration = asset->Acceleration;
    headingCalcType = asset->Heading;
}

// The animation to play and what to face while playing it can come from an
// event held in a blackboard variable.
struct PlayAnimationEvent {
    unsigned int animation;
    unsigned int pad;
    unsigned long long target;
};

void zNPCBTPlayAnimationTypeAction::Begin() {
    stopControl.SetMaxAcc(acceleration);
    stopControl.headingCalcType = headingCalcType;
    npcBase->npcSteering->_v10(&stopControl);

    if (eventVarID != 0) {
        zVariableEventData data;

        if (btClient->blackboard.Read(eventVarID, data) &&
            data.eventID == 0x7CF724FB && data.dataPointer != 0) {
            animID = *(unsigned int*)data.dataPointer;

            xBase* target = zSceneFindObject(
                ((PlayAnimationEvent*)data.dataPointer)->target);

            if (target != 0 && (target->baseFlags & 0x20)) {
                stopControl.headingCalcType = 8;
                steeringDest.pDest = &target->model->Mat.pos;
                stopControl.SetCustomHeading(steeringDest);
            }
        }
    }

    valid = npcBase->npcEntity->DoesAnimExist(animID);
    actionAnim.Init(animID, 0.2f, 0.0f);
    actionAnim.StartOnNPC(npcBase->npcEntity, forceRestart);
}

// Weak in retail and CALLED from Begin above, so it is defined below it:
// read earlier, the auto-inliner takes it into Begin.
//
// NEAR MISS: 2 words against retail's 9. Retail copies the sixteen bytes
// as four words in line; here mwcc synthesizes zNPCSteeringDest's
// operator= out of line (an EXTRA function) and tail-calls it, and the
// same happens with the vector spelled as an xVec3 or as three floats.
// The likely cause is that retail's unity build had already generated
// that operator= in an earlier file, which a fragment cannot reproduce;
// that is a guess, and nothing further was tried.
inline void zNPCSteeringControl::SetCustomHeading(zNPCSteeringDest& dest) {
    customHeading = dest;
}

// The copy is written out here, not left to zVariableEventData's
// operator=: a class's member does not inline into a member template.
template <>
bool zBlackboard::Read<zVariableEventData>(unsigned int id,
                                           zVariableEventData& out) const {
    zVariableBase* v = Find(id);

    if (v == 0) {
        return false;
    }

    // The cast written where retail has it inlined: a template helper is
    // not instantiated until the end of the unit, after this is compiled.
    zVariable<zVariableEventData>* result;

    if (v->type == eVarType_zVariableEventData) {
        result = (zVariable<zVariableEventData>*)v;
    } else {
        result = 0;
    }

    zVariable<zVariableEventData>* var = result;

    if (var != 0) {
        Sext::EventAny* data = var->value.dataPointer;

        out.eventID = var->value.eventID;

        if (data != 0) {
            memcpy(out.eventDataBuffer, data, 64);
            out.dataPointer = (Sext::EventAny*)out.eventDataBuffer;
        } else {
            out.dataPointer = 0;
        }

        return true;
    }

    return false;
}

int zNPCBTPlayAnimationTypeAction::Update(float dt) {
    npcBase->npcSteering->_v14(dt);

    if (valid) {
        return npcBase->npcEntity->IsAnimationStopped(actionAnim.animStateID)
                   ? 3
                   : 1;
    }

    return 4;
}

void zNPCBTPlayAnimationTypeAction::End() {
    npcBase->npcSteering->_v11(&stopControl);
}

// ---------------------------------------------------------------------------
// zNPCBTRotateToFaceAction: turn toward the first player, the closest one on
// the wallnet, or a position held in a blackboard variable, at no more than
// the asset's rate; inside the tolerance the action may end.
//
// NEAR MISS: 15 of 165 words, every one a register choice with every
// instruction right. Retail keeps the NPC pointer in r0 and the switch
// value in r4 (ours the other way round), and the tolerance product in f4
// with the zero `rot` in f2 (ours swapped). tools/sweep_src.py measured
// twelve spellings -- the type read into a local or not, three orders of
// the float locals, the constant on either side of the multiply -- and
// none goes below 15. Not worth another sweep.

int zNPCBTRotateToFaceAction::Update(float dt) {
    const Sext::Action_NPC_RotateToFace* asset =
        (const Sext::Action_NPC_RotateToFace*)actionAsset;
    zNPCBase* npc = npcBase;
    xVec3 target;

    switch (asset->type) {
    case 0:
        target = xglobals->players.playerArray[0]->model->Mat.pos;
        break;
    case 1: {
        zPlayer* player = npc->GetClosestPlayerOnWallnet(true);

        if (player == 0) {
            return 4;
        }

        target = player->model->Mat.pos;
        break;
    }
    case 2:
        return 4;
    case 3:
        if (btClient->blackboard.Read(NPC_VAR_ROTATE_TO_FACE_POS, target) &&
            target != xVec3::m_Null) {
            break;
        }

        return 4;
    case 4:
        if (btClient->blackboard.Read(NPC_VAR_CURRENT_DESTINATION, target) &&
            target != xVec3::m_Null) {
            break;
        }

        return 4;
    }

    xVec3 delta = target - npcBase->npcEntity->model->Mat.pos;
    float targetAngle = xClampAngle0_2PI(xatan2(delta.z, delta.x));
    float angle = xClampAngle0_2PI(
        xatan2(npcBase->npcEntity->model->Mat.at.z,
               npcBase->npcEntity->model->Mat.at.x));
    float diff = xClampAnglePI_PI(targetAngle - angle);
    float speed = asset->speed;
    float tolerance = 0.017453292f * asset->tolerance;
    float rot = 0.0f;

    if (diff < -tolerance) {
        float step = -speed * dt;

        rot = -((diff < step) ? step : diff);
    } else if (diff > tolerance) {
        float step = speed * dt;

        rot = -((diff > step) ? step : diff);
    } else if (asset->stopWhenFacing) {
        return 3;
    }

    xMat3x3LMulRotY(&npcBase->npcEntity->model->Mat,
                    &npcBase->npcEntity->model->Mat, rot);

    if (npcBase->npcSteering != 0) {
        npcBase->npcSteering->_v15();
    }

    return 1;
}

// ---------------------------------------------------------------------------
// Show, hide, fade

int zNPCBTShowAction::Update(float dt) {
    npcBase->npcEntity->Show();

    return 3;
}

int zNPCBTHideAction::Update(float dt) {
    npcBase->npcEntity->Hide();

    return 3;
}

void zNPCBTFadeOutAction::Begin() { elapsedTime = 0.0f; }

// The asset's one field is the fade's duration.
int zNPCBTFadeOutAction::Update(float dt) {
    float alpha;
    float duration = *(const float*)actionAsset;

    elapsedTime += dt;

    if (elapsedTime >= duration) {
        alpha = 0.0f;
    } else {
        alpha = (duration - elapsedTime) / duration;
    }

    if (npcBase->npcEntity != 0) {
        npcBase->npcEntity->model->SetColorMulAlpha(alpha);
    }

    if (alpha >= -1e-5f && alpha <= 1e-5f) {
        return 3;
    }

    return 1;
}

int zNPCBTFadeInAction::Update(float dt) {
    float alpha;
    float duration = *(const float*)actionAsset;

    elapsedTime += dt;

    if (elapsedTime >= duration) {
        alpha = 1.0f;
    } else {
        alpha = elapsedTime / duration;
    }

    if (npcBase->npcEntity != 0) {
        npcBase->npcEntity->model->SetColorMulAlpha(alpha);
    }

    if ((float)__fabs(alpha - 1.0f) <= 1e-5f) {
        return 3;
    }

    return 1;
}

// ---------------------------------------------------------------------------
// Collision

void zNPCBTSetCollidesAction::Setup(const Sext::ActionBase* asset) {
    collides = *(const bool*)asset;
}

int zNPCBTSetCollidesAction::Update(float dt) {
    zNPCEntity* entity = npcBase->npcEntity;
    bool enabled = entity->collisionType != 0;

    if (collides) {
        if (!enabled) {
            entity->EnableCollision();
        }
    } else if (enabled) {
        entity->DisableCollision();
    }

    return 3;
}

// ---------------------------------------------------------------------------
// zNPCBTPlayNPCFXAction

void zNPCBTPlayNPCFXAction::Setup(const Sext::ActionBase* asset) {
    FXNameHash = *(const unsigned int*)asset;
    FXFound = false;
}

void zNPCBTPlayNPCFXAction::Begin() {
    if (npcBase->npcFX != 0) {
        FXFound = npcBase->npcFX->RunFX(FXNameHash);
    }
}

int zNPCBTPlayNPCFXAction::Update(float dt) {
    if (FXFound && npcBase->npcFX != 0) {
        return npcBase->npcFX->IsFXRunning(FXNameHash) ? 1 : 3;
    }

    return 4;
}

void zNPCBTPlayNPCFXAction::End() {
    if (npcBase->npcFX != 0) {
        npcBase->npcFX->StopFX(FXNameHash);
    }
}

// ---------------------------------------------------------------------------
// zNPCBTPlayEELFXAction: the arc is made on the global heap, told to start
// and to stop by event, and destroyed with the action.

void zNPCBTPlayEELFXAction::Setup(const Sext::ActionBase* a) {
    asset = (const Sext::Action_NPC_PlayEELFX*)a;

    xOGEntityData* source =
        (xOGEntityData*)zSceneFindObject(asset->ElectricArcUID);

    arc = new (Memory::AllocGlobalHeap(272, Memory::GlobalHeap, (eMemMgrTag)43,
                                       false)) ElectricArc(&dummyHandle);
    arc->Init(source->asset);
    arc->Setup();
}

void zNPCBTPlayEELFXAction::Begin() {
    zEntEvent(0, 0, (xBase*)arc, 10427, 0, (ForceEvent)1);
}

int zNPCBTPlayEELFXAction::Update(float dt) {
    arc->Update(dt);

    return 1;
}

void zNPCBTPlayEELFXAction::End() {
    zEntEvent(0, 0, (xBase*)arc, 1364959, 0, (ForceEvent)1);
}

void zNPCBTPlayEELFXAction::Deinitialize() {
    arc->Release();

    ElectricArc* a = arc;

    if (a != 0) {
        a->~ElectricArc();
    }

    Free(Memory::GlobalHeap, a);
    arc = 0;
}

// ---------------------------------------------------------------------------
// Collectibles

void zNPCBTGenerateCollectiblesAction::Setup(const Sext::ActionBase* a) {
    const Sext::Action_NPC_GenerateCollectibles* asset =
        (const Sext::Action_NPC_GenerateCollectibles*)a;

    boneIndex = asset->BoneIndex;
    __ct__Q24Math6VectorFfff(&offset, asset->Offset.x, asset->Offset.y,
                             asset->Offset.z);
    type = (Sext::eNPCCollectibleType)asset->Type;
}

int zNPCBTGenerateCollectiblesAction::Update(float dt) {
    zNPCTemplate* npcTemplate = npcBase->npcTemplate;

    if (npcTemplate != 0) {
        zCollectibleSpawner* spawner = npcTemplate->GetCollectibleSpawner(type);

        if (spawner != 0) {
            static xVec3 scale = xVec3::m_DoubleVec;
            xVec3 pos;

            xModelGetBoneLocationNoScale(pos, *npcBase->npcEntity->model,
                                         boneIndex);
            pos += offset;
            spawner->AddRequest("NPC Generate Collectibles Action", pos, scale,
                                (World::xOGEntity*)npcBase->npcEntity);

            return 3;
        }
    }

    return 4;
}

int zNPCBTSetCollectibleAction::Update(float dt) {
    npcBase->collectible = *(const bool*)actionAsset;

    return 3;
}

// ---------------------------------------------------------------------------
// zNPCBTStartHeadTrackingAction: the target is looked up -- a player by name,
// or a player or object held in a blackboard variable -- and that is all
// this build does with it.

int zNPCBTStartHeadTrackingAction::Update(float dt) {
    const Sext::Action_NPC_StartHeadTracking* asset =
        (const Sext::Action_NPC_StartHeadTracking*)actionAsset;

    switch (asset->TargetType) {
    case 0: {
        ePlayerName name;

        switch (asset->Player) {
        case 1:
            name = (ePlayerName)0;
            break;
        case 0:
            name = (ePlayerName)1;
            break;
        case 3:
            name = (ePlayerName)2;
            break;
        case 2:
            name = (ePlayerName)3;
            break;
        default:
            name = (ePlayerName)13;
            break;
        }

        GetPlayerByName(0, name);
        break;
    }
    case 1: {
        unsigned int var = asset->Variable;

        switch (btClient->blackboard.GetVariableType(var)) {
        case eVarType_zPlayerPtr: {
            zPlayer* player;

            btClient->blackboard.Read(var, player);
            break;
        }
        case eVarType_zUpFloatingObjectPtr:
            btClient->blackboard.Find(var);
            break;
        }

        break;
    }
    }

    return 3;
}

// ---------------------------------------------------------------------------
// Extra collision

void zNPCBTExtraCollisionAction::End() {
    if (physicsObject.physicsSystem != 0) {
        physicsObject.SetCollisionFilter(31);
        physicsObject.Deactivate();
        xHavok_RemoveFromSimWorld(physicsObject.physicsSystem);
        physicsObject.Cleanup();
    }
}

void zNPCBTExtraCollisionAction::Cleanup() { _v5(); }

// ---------------------------------------------------------------------------
// zNPCBTPositionEntAtBoneAction: put an entity on one of the NPC's bones,
// at an offset in the bone's frame. A trigger keeps its own matrix.

int zNPCBTPositionEntAtBoneAction::Update(float dt) {
    const Sext::Action_NPC_PositionEntAtBone* asset =
        (const Sext::Action_NPC_PositionEntAtBone*)actionAsset;
    xBase* ent = zSceneFindObject(asset->ent);

    if (ent == 0) {
        return 4;
    }

    xMat4x3 mat;

    xModelGetBoneMatNoScale(mat, *npcBase->npcEntity->model, asset->bone);

    Math::Vector offset(asset->offset.x, asset->offset.y, asset->offset.z);
    xVec3 moved;

    xMat3x3RMulVec(&moved, &mat, (const xVec3*)&offset);
    v3add(&mat.pos, &moved, &mat.pos);

    if (ent->entityType == 0x5D) {
        ((zTrigger*)ent)->mat = mat;
        ((zTrigger*)ent)->UpdateTriggerEntry();
    } else {
        ent->model->Mat = mat;
    }

    if (asset->done) {
        return 3;
    }

    return 1;
}

// ---------------------------------------------------------------------------
// The spawner actions

void zNPCBT_GenericSpawnerInit_Action::Setup(const Sext::ActionBase* value) {
    asset = (const Sext::Action_NPC_GenericSpawnerInit*)value;
}

int zNPCBT_GenericSpawnerInit_Action::Update(float dt) {
    float delay = ((zNPCGenericSpawner*)npcBase)->assetSpawner->spawnDelay;

    if (delay == 0.0f) {
        delay = asset->spawnDelayDefault;
    }

    btClient->blackboard.Write(asset->spawnDelayVar, delay);

    return 3;
}

void zNPCBT_SpawnNPC_ThrowToLocation_Action::End() {
    if (NPCToSpawn != 0) {
        xVec3Init(&NPCToSpawn->npcEntity->frame->vel, 0.0f, 0.0f, 0.0f);
        xVec3Init(&NPCToSpawn->npcEntity->frame->dvel, 0.0f, 0.0f, 0.0f);
    }
}

int zNPCBT_Spawner_SetRotateToFaceVariable::Update(float dt) {
    zNPCGenericSpawner* spawner = (zNPCGenericSpawner*)npcBase;
    zNPCGeneric* npc = spawner->genericPool->GetNextInactiveNPC(spawner->id);

    spawner->myReservedNPC = npc;

    if (npc == 0) {
        return 4;
    }

    xVec3 pos;

    spawner->GetSpawnPos(&pos, spawner->myReservedNPC);

    if (btClient->blackboard.Write(NPC_VAR_ROTATE_TO_FACE_POS, pos)) {
        return 3;
    }

    spawner->genericPool->RemoveFromReserved(spawner->myReservedNPC);
    spawner->myReservedNPC = 0;

    return 4;
}

int zNPCBT_Spawner_UnreserveNPC::Update(float dt) {
    zNPCGenericSpawner* spawner = (zNPCGenericSpawner*)npcBase;

    if (spawner->myReservedNPC != 0) {
        spawner->genericPool->RemoveFromReserved(spawner->myReservedNPC);
        spawner->myReservedNPC = 0;
    }

    return 3;
}

// The NPC comes out of the spawner's pool at once, at a move point a
// blackboard variable names when there is one, and where it is otherwise.
//
// NEAR MISS: 10 of 65 words, all the stack frame: retail's is 112 bytes
// with the second vector at 80(r1), ours is 48 with it at 16(r1). Retail
// keeps 52 bytes of locals this body does not declare; what they were is
// not recorded (the function has no locals in the DWARF). Not swept.
void zNPCBT_InstantSpawnNPC_Action::Begin() {
    asset = (const Sext::Action_NPC_InstantSpawnNPC*)actionAsset;

    unsigned int mpVariable = asset->MPVariableName;

    NPCToSpawn = 0;
    pMovePoint = 0;
    spawner = (zNPCGenericSpawner*)npcBase;
    NPCToSpawn = spawner->genericPool->GetNextInactiveNPC(spawner->id);

    if (NPCToSpawn != 0) {
        Sext::uid mp;

        if (btClient->blackboard.Read(mpVariable, mp) && mp.internalUid != 0) {
            xBase* obj = zSceneFindObject(mp.internalUid);

            if (obj->baseType == 0x51) {
                pMovePoint = (xMovePoint*)obj;

                xVec3 pos;

                pos = *pMovePoint->pos;
                NPCToSpawn->npcEntity->model->Mat.pos = pos;
            } else {
                return;
            }
        }

        xVec3 at;

        at = NPCToSpawn->npcEntity->model->Mat.pos;
        xEntReset((xEnt*)NPCToSpawn->npcEntity, at, xVec3::m_Null);
    }
}

int zNPCBT_InstantSpawnNPC_Action::Update(float dt) {
    if (NPCToSpawn == 0) {
        return 4;
    }

    NPCToSpawn->Spawn();

    if (pMovePoint != 0 && asset->SetMovePoint) {
        NPCToSpawn->npcMovePoint = pMovePoint;
    }

    spawner->genericPool->RemoveFromReserved(NPCToSpawn);

    return 3;
}

void zNPCBT_InstantSpawnNPC_Action::End() {
    if (NPCToSpawn != 0) {
        xVec3Init(&NPCToSpawn->npcEntity->frame->vel, 0.0f, 0.0f, 0.0f);
        xVec3Init(&NPCToSpawn->npcEntity->frame->dvel, 0.0f, 0.0f, 0.0f);
        spawner->NPCWasSpawned(NPCToSpawn);
        NPCToSpawn = 0;
    }
}
