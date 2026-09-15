#include "SB/GM/Engine/Game/zNPCGenericSwarm.pool.h"

// zNPCGenericSwarm.cpp -- the swarm NPC: up to 32 members, each an entity
// with its own effects component, spawned over time from the swarm's asset
// and killed one at a time. Read from the image with tools/brief.py; the
// layouts are the DWARF's, the virtual slots the image's (tools/vtable.py).
// No GENERATED banner: gen_units.py overwrites any file that carries one.
//
// A member's status is 1 while alive and 0 once dead. The swarm is
// polymorphic from +0 (slot 42 renders it); an entity is too, with its
// component calls in slots 44 to 53 and its component base at +0xBC, whose
// vtable pointer sits at +4 behind the owner, as an effects component's
// does.

class xBase;
class xEntFrame;
class zNPCBase;
class zNPCEntity;
class zNPCStatus;
class zPlayer;
class zNPCSteering;
class zNPCSwarmSteering;
class zNPCBTManager;
class zNPCLogic;

namespace World {
class EntityHandleBase;
class xOGModelUpdater {};
}  // namespace World

inline void* operator new(unsigned long size, void* mem) { return mem; }

extern "C" double floor(double x);

// ---------------------------------------------------------------------------
// Assets

namespace Sext {

class EventAny {};
class EventActionNew : public EventAny {};

class EventActionOneInt : public EventActionNew {
public:
    short param0;
};

class uid {
public:
    operator unsigned long long() const { return internalUid; }

    unsigned long long internalUid;
};

class vec3 {
public:
    float x;
    float y;
    float z;
};

class xBaseAsset {
public:
    uid id;
    unsigned int baseType;
    unsigned short linkCount;
    unsigned short baseFlags;
};

class xEntAsset : public xBaseAsset {
public:
    unsigned char _pad0[0x2C - 0x10];
    vec3 Pos;
    unsigned char _pad1[0x100 - 0x38];
};

class NPCAsset : public xBaseAsset {
public:
    xEntAsset EntAsset;
    unsigned char _pad0[0x138 - 0x110];
    uid NPCTemplate;
    unsigned int EnemyFlags;
    uid WallNet;
    uid MovePoint;
    uid MovePointNetwork;
    unsigned int SpawnType;
    unsigned char EventLinksNew[0xC];
    uid SpawnSwarmEmit;
    unsigned int LODType;
    unsigned char LODData[0xC];
};

class GenericSwarm : public NPCAsset {
public:
    int MemberNumber;
    float SpawnPeriod;
    uid HidePoint;
    uid DeathDecal;
};

class NPCTemplate {
public:
    unsigned char _pad0[0x30];
    uid BehaviorSet;
    unsigned int flags;
};

}  // namespace Sext

enum ForceEvent {
    FE_YES = 0,
    FE_NO = 1
};

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {

enum GlobalHeapEnum { GlobalHeap = 0 };

enum eFactoryMemType { eFactoryMemType_ = 0x7FFFFFFF };

class Factory {
public:
    void* AllocMemStatic(unsigned int size);
    void DeallocMem(void* mem);

    template <class T>
    T* Create(eFactoryMemType type);
};

}  // namespace Memory

void* xMemAlloc(Memory::GlobalHeapEnum heap, unsigned int size, int align,
                eMemMgrTag tag);

class zNPCManager {
public:
    static Memory::Factory factory;
};

// ---------------------------------------------------------------------------
// Vectors

// xVec3's dot product is folded onto hkVector4::dot3 in the image, and the
// calls are named by that symbol.
class hkVector4 {
public:
    float dot3(const hkVector4& other) const;
};

class xVec3 {
public:
    xVec3& operator=(const xVec3& other);
    xVec3& operator+=(const xVec3& other);
    xVec3& operator*=(float s);
    xVec3& safe_normalize(const xVec3& fallback);
    xVec3& normalize();
    float Distance(const xVec3& other) const;

    float dot(const xVec3& other) const {
        return ((const hkVector4*)this)->dot3(*(const hkVector4*)&other);
    }

    static const xVec3 m_UnitAxisX;
    static const xVec3 m_UnitAxisY;
    static const xVec3 m_UnitAxisZ;

    float x;
    float y;
    float z;
};

xVec3 operator-(const xVec3& a, const xVec3& b);

extern const xVec3 g_O3;

// The linker folded xVec3's three-float constructor onto Math::Vector's.
extern "C" void __ct__Q24Math6VectorFfff(void* v, float x, float y, float z);

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

void xMat3x3Euler(xMat3x3* m, float yaw, float pitch, float roll);

float xrand_GenRandFloat();
float xrand_RandomFloatRange(float lo, float hi);
int xrand_RandomRange(int lo, int hi);

// A random value from lo to hi, in line.
inline float xrandf(float lo, float hi) {
    float t = xrand_GenRandFloat();

    return t * hi + (1.0f - t) * lo;
}

float zRandDist_Gamma_Int(unsigned int n);
float zRandDist_Gamma_Frac(float a);

xBase* zSceneFindObject(unsigned long long id);
unsigned int xStrHash(const char* str);
unsigned long long xUIDMgrFindUID(unsigned int hash);
void zEntEvent(xBase* from, unsigned int fromEvent, xBase* to,
               unsigned int toEvent, Sext::EventAny* params, ForceEvent force);

namespace World {

class ModelInstanceAsset {
public:
    unsigned long long modelPrototypeID;
};

class xOGModel {
public:
    void UpdaterSwitch(xOGModelUpdater* updater, void* parent);

    xMat4x3 Mat;
};

extern xOGModelUpdater g_modelUpdateNone;

class EntityManager {
public:
    static void* FindAsset(unsigned long long id);
};

EntityManager* GetEntityManager();

}  // namespace World

// ---------------------------------------------------------------------------
// Creators

class CreatorI {
public:
    virtual void* Create(Memory::Factory* factory);
};

namespace Memory {

template <int N, class T, class B>
class Creator : public CreatorI {
public:
    static Creator* Get() {
        static Creator _inst;
        return &_inst;
    }
};

// Retail calls this instance's Get, emitted with an earlier file of the
// unity build; here it is only declared.
template <>
class Creator<1, zNPCBTManager, zNPCLogic> : public CreatorI {
public:
    static Creator* Get();
};

template <>
inline Creator<7, zNPCSwarmSteering, zNPCSteering>*
Creator<7, zNPCSwarmSteering, zNPCSteering>::Get() {
    static Creator _inst;
    return &_inst;
}

}  // namespace Memory

// ---------------------------------------------------------------------------
// The entity classes. Entity is polymorphic from +0.

class EntityVirtuals {
public:
    virtual void _b0();
    virtual void _b1();
    virtual void _b2();
    virtual void _b3();
    virtual void _b4();
    virtual void _b5();
    virtual void _b6();
    virtual void _b7();
    virtual void _b8();
    virtual void _b9();
    virtual void _b10();
    virtual void _b11();
    virtual void _b12();
    virtual void _b13();
    virtual void _b14();
    virtual void _b15();
    virtual void _b16();
    virtual void _b17();
    virtual void _b18();
    virtual void _b19();
};

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
    void* linkArray;
    void* templateParent;
    void* eventFunc;
};

namespace World {

class xOGEntity : public xBase {
public:
    xOGEntity(EntityHandleBase* handle);

    xOGModel* model;
    void* modelAutoptr;
};

}  // namespace World

#pragma pack(pop)

class zNPCEntityParams {
public:
    unsigned char _pad0[0x14];
};

class xEnt : public World::xOGEntity {
public:
    virtual void _x20();
    virtual void _x21();
    virtual void _x22();
    virtual void _x23();
    virtual void _x24();
    virtual void _x25();
    virtual void _x26();
    virtual void _x27();
    virtual void _x28();
    virtual void _x29();
    virtual void _x30();
    virtual void _x31();
    virtual void _x32();
    virtual void _x33();
    virtual void _x34();
    virtual void _x35();
    virtual void _x36();
    virtual void _x37();
    virtual void _x38();
    virtual void _x39();
    virtual void _x40();
    virtual void _x41();
    virtual void _x42();
    virtual void _x43();
    virtual void SetParameters(zNPCEntityParams* params);
    virtual void Attached(const zNPCStatus* status);
    virtual void AllAttached();
    virtual void Detached(zNPCStatus* status);
    virtual void Reset(const zNPCStatus* status);
    virtual void PreUpdate(float dt);
    virtual void PostUpdate(float dt);
    virtual void Paused();
    virtual void Resumed();
    virtual bool SystemEvent(xBase* from, xBase* to, unsigned int toEvent,
                             Sext::EventAny* params);

    unsigned char _pad0[0x58 - 0x3C];
    xEntFrame* frame;
    unsigned char _pad1[0xBC - 0x5C];
};

// Components keep their owner in front of the vtable pointer at +4.
class zNPCComponent {
public:
    zNPCComponent() { owner = 0; }

    zNPCBase* owner;

    virtual void Attached(const zNPCStatus* status);
    virtual void Detached(zNPCStatus* status);
    virtual void Reset(const zNPCStatus* status);
    virtual void AllAttached();
    virtual void PreUpdate(float dt);
    virtual void PostUpdate(float dt);
    virtual void Render();
    virtual void Paused();
    virtual void Resumed();
    virtual bool SystemEvent(xBase* from, xBase* to, unsigned int toEvent,
                             Sext::EventAny* params);
};

class zNPCEntity : public xEnt, public zNPCComponent {
public:
    zNPCEntity(World::EntityHandleBase* handle);

    void Enable();
    void Disable();
    void RenderModelInstance();
    void Teleport(const xMat4x3& mat);
    void CopyFrame(const xEntFrame* frame);

    virtual void Attached(const zNPCStatus* status);
    virtual void AllAttached();
    virtual void Detached(zNPCStatus* status);
    virtual void Reset(const zNPCStatus* status);
    virtual void PreUpdate(float dt);
    virtual void PostUpdate(float dt);
    virtual void Paused();
    virtual void Resumed();
    virtual bool SystemEvent(xBase* from, xBase* to, unsigned int toEvent,
                             Sext::EventAny* params);

    unsigned char _pad2[0x1D0 - 0xC4];
};

class zNPCFX : public zNPCComponent {
public:
    // Weak in the image: zNPCFX.h's.
    zNPCFX() : numberOfFXs(0), params(0), customEnt(0) {}

    virtual void Attached(const zNPCStatus* status);
    virtual void Detached(zNPCStatus* status);
    virtual void Reset(const zNPCStatus* status);
    virtual void PostUpdate(float dt);
    virtual void Paused();
    virtual void Resumed();

    void StopAllFX();
    bool RunFX(unsigned int nameHash);
    void StopFX(unsigned int nameHash);

    void* FXs[8];
    bool isRunning[8];
    unsigned int numberOfFXs;
    void* params;
    zNPCEntity* customEnt;
};

class zNPCLogic : public zNPCComponent {
public:
    virtual void Update(float dt);
};

class zNPCGetsDamageInfo {
public:
    unsigned int flags;
    xBase* from;
    zNPCEntity* npcEntity;
    float damageHP;
    float remainingHP;
    int hitSource;
    int hitTarget;
    int hitReaction;
    xVec3 knockback;
};

class zNPCHitReactionTable {
public:
    int hitSource;
    int hitReaction;
};

class zNPCCombatParams {
public:
    unsigned char _pad0[0x14];
    zNPCHitReactionTable* hitReactionTable;
    unsigned int hitReactionTableCount;
    unsigned char _pad1[0x24 - 0x1C];
};

extern zNPCHitReactionTable sGenericSwarmCombatReactionTable[];

class zNPCType {
public:
    unsigned char _pad0[0x18];
    CreatorI* logicCreator;
    CreatorI* steeringCreator;
    unsigned char _pad1[0x3C - 0x20];
};

class zNPCTemplate {
public:
    void Setup(Sext::NPCTemplate* asset, zNPCBase* npc);

    Sext::NPCTemplate* templateAsset;
};

class zCharacterAsset {
public:
    World::ModelInstanceAsset ModelInstance;
};

class zWallNet {
public:
    bool IsInsideWallNetXZ(const xVec3& pos) const;
};

class zDirection : public World::xOGEntity {
public:
    virtual void _d20();
    virtual void _d21();
    virtual void _d22();
    virtual void _d23();
    virtual void _d24();
    virtual void _d25();
    virtual void _d26();
    virtual void _d27();
    virtual void _d28();
    virtual void _d29();
    virtual void _d30();
    virtual void _d31();
    virtual void GetLocation(xVec3* location);
};

// ---------------------------------------------------------------------------
// Behaviour-tree clients. The client's vtable pointer follows its data.

class zBTActionBuilder {
public:
    unsigned char _pad0[0x8];
};

class zBTConditionBuilder {
public:
    unsigned char _pad0[0x8];
};

class zBTClient {
public:
    unsigned char _pad0[0x90];
    zBTActionBuilder* actionBuilder;
    zBTConditionBuilder* conditionBuilder;
    unsigned char _pad1[0x19C - 0x98];

    virtual void SetupInterpreter();
    virtual void SetupClient(unsigned long long behaviorSet);
};

class zNPCBTClientBase : public zBTClient {
public:
    zNPCBase* owner;
};

class zNPCBTClient : public zNPCBTClientBase {
public:
    zNPCBTClient();
};

class zNPCBTActionBuilder : public zBTActionBuilder {
public:
    zNPCBTActionBuilder(zBTClient* client);

    zNPCBase* npcBase;
    unsigned int actionCount;
    unsigned int* actions;
};

class zNPCBTConditionBuilder : public zBTConditionBuilder {
public:
    zNPCBTConditionBuilder(zBTClient* client);

    zNPCBase* npcBase;
};

// ---------------------------------------------------------------------------
// Players

class zUpContextAction {
public:
    unsigned char volumeType;
    unsigned char actionType;
    unsigned char uniqueActionId;
    unsigned char pad0;
    void* upInteraction;
    void* userData;
};

class zUpContextActionManager {
public:
    zUpContextAction* GetFreePresence(zPlayer* player);
    zUpContextAction* GetFreeActivity(zPlayer* player, unsigned char type);

    unsigned char _pad0[0x1C];
};

class zPlayerActionManager {
public:
    unsigned char _pad0[0x1C];
    zUpContextActionManager contextManager;
};

class zPlayer : public xEnt {
public:
    virtual void _p54();
    virtual void _p55();
    virtual void _p56();
    virtual void _p57();
    virtual void _p58();
    virtual void _p59();
    virtual void _p60();
    virtual void _p61();
    virtual void _p62();
    virtual void _p63();
    virtual void _p64();
    virtual void _p65();
    virtual void _p66();
    virtual void _p67();
    virtual void _p68();
    virtual void _p69();
    virtual void _p70();
    virtual void _p71();
    virtual void _p72();
    virtual void _p73();
    virtual void _p74();
    virtual void _p75();
    virtual void _p76();
    virtual void _p77();
    virtual void _p78();
    virtual void _p79();
    virtual void _p80();
    virtual void _p81();
    virtual void _p82();
    virtual void _p83();
    virtual void _p84();
    virtual void _p85();
    virtual void _p86();
    virtual void _p87();
    virtual void _p88();
    virtual void _p89();
    virtual void _p90();
    virtual void _p91();
    virtual void _p92();
    virtual void _p93();
    virtual void _p94();
    virtual void _p95();
    virtual void _p96();
    virtual void _p97();
    virtual void _p98();
    virtual void _p99();
    virtual void _p100();
    virtual void _p101();
    virtual void _p102();
    virtual void _p103();
    virtual void _p104();
    virtual bool IsAI() const;

    void* atbl;
    zPlayerActionManager actionManager;
    unsigned char _pad2[0x2EC - 0xF8];
    int eName;
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

class zNPCBTClientBase;

// ---------------------------------------------------------------------------
// The NPC

class zNPCBase : public World::xOGEntity {
public:
    zNPCBase(World::EntityHandleBase* handle);

    virtual void _n20();
    virtual void _n21();
    virtual void _n22();
    virtual void _n23();
    virtual void _n24();
    virtual void _n25();
    virtual void _n26();
    virtual void _n27();
    virtual void _n28();
    virtual void _n29();
    virtual void _n30();
    virtual void _n31();
    virtual void _n32();
    virtual void _n33();
    virtual void _n34();
    virtual void _n35();
    virtual void _n36();
    virtual void _n37();
    virtual void _n38();
    virtual void _n39();
    virtual void _n40();
    virtual void _n41();
    virtual void Render();

    void SystemEvent(xBase* from, xBase* to, unsigned int toEvent,
                     Sext::EventAny* params);
    bool CheckWallNetValidity(bool noWallNet);
    bool CheckIfModelHasCollisionModel(
        World::ModelInstanceAsset* modelInstanceAsset);
    void PreUpdateAllComponents(float dt);
    void PostUpdateAllComponents(float dt);

    // A component joins the NPC, or leaves it if it had joined.
    void AttachComponent(zNPCComponent& component, const zNPCStatus* status) {
        component.owner = this;
        component.Attached(status);
    }

    void DetachComponent(zNPCComponent& component, zNPCStatus* status) {
        if (component.owner != 0) {
            component.Detached(status);
            component.owner = 0;
        }
    }

    int numProjectiles;
    unsigned char npcStatus[0x1C];
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
    bool infoNodesUpdating : 1;
    bool quietKill : 1;
    bool updating : 1;
    bool flying : 1;
    bool collectible : 1;
    unsigned char _pad0[0x78 - 0x72];
    zNPCTemplate* npcTemplate;
    zNPCBTClientBase* btClient;
    void* npcMovePoint;
    void* npcMovePointNetwork;
    unsigned char npcUpdateLOD[0xC];
    zNPCLogic* npcLogic;
    zNPCEntity* npcEntity;
    unsigned char _pad1[0xC0 - 0x9C];
};

class zNPCGenericSwarm : public zNPCBase {
public:
    class Type : public zNPCType {
    public:
        void InitTypeParameters();
    };

    zNPCGenericSwarm(World::EntityHandleBase* handle);

    bool Activate(const zNPCStatus* npcStatus);
    void DeactivateNPC(zNPCStatus* npcStatus);
    void Initialize(const zNPCStatus* npcStatus);
    void Uninitialize(zNPCStatus* npcStatus);
    bool CheckSceneSetupValidity();
    void SceneSetup();
    void Update(float dt);
    void UpdateSpawns(float dt);
    void UpdateContextButtons();
    void Render();
    void Paused();
    void Resumed();
    void NPCGetsDamage(const zNPCGetsDamageInfo& info);
    void SystemEvent(xBase* from, xBase* to, unsigned int toEvent,
                     Sext::EventAny* baseParams);
    bool SpawnNewMember();
    void KillMember(int childNum, xBase* killedBy, bool quiet);
    int GetNumberOfChildren();
    xBase* GetChild(int index) const;
    int FindChildNumFromBase(xBase* base);
    unsigned int GetNPCLODPositions(xVec3* positions);
    int GetTotalAliveChildNum();
    int GetNextDeadChildNum();
    int GetRandomAliveChildNum();
    void CreateRandomPopSpawnMat(xVec3& pos, float radius, xMat4x3& spawnMat,
                                 const zWallNet* wallNet);
    bool IsWithinCone(xVec3& sourcePos, xVec3& sourceDir, float coneAngle,
                      float coneDistance2);
    bool IsWithinDistance(xVec3& sourcePos, float distance);

    int IsMemberAlive(int i) const { return memberStatus[i] > 0; }

    static zNPCEntityParams sEntParams;
    static zNPCCombatParams sCombatParams;

    zNPCEntity* entities[32];
    zNPCFX* entFXs[32];
    int memberStatus[32];
    int numberOfSwarmers;
    xVec3 averagePos;
    float memberSpawnTimeAcc;
    float avgMemberSpawnPeriod;
    float nextMemberSpawnPeriod;
    int remainingSpawns;
    zWallNet* wallNet;
    xVec3 hidePoint;
    int numberOfKilledMembers;
    unsigned int killedMembers;
};

namespace zNPCGenericSwarmNS {
extern xBase* defaultDecalGenerator;
extern xBase* defaultSplatSound;
}  // namespace zNPCGenericSwarmNS

// ---------------------------------------------------------------------------
// zRandDistribution.h's, weak in the image

#define ZRAND_MIN(a, b) ((a) < (b) ? (a) : (b))
#define ZRAND_MAX(a, b) ((a) > (b) ? (a) : (b))

inline float zRandDist_Gamma(float shapeParam, float mean, float clampRatio) {
    float intPart = floor(shapeParam);
    float gammaResult;

    if (shapeParam == intPart) {
        gammaResult = mean * zRandDist_Gamma_Int((unsigned int)intPart) / shapeParam;
    } else if (0.0f == intPart) {
        gammaResult = mean * zRandDist_Gamma_Frac(shapeParam) / shapeParam;
    } else {
        gammaResult = mean *
                      (zRandDist_Gamma_Int((unsigned int)intPart) +
                       zRandDist_Gamma_Frac(shapeParam - intPart)) /
                      shapeParam;
    }

    return ZRAND_MAX(mean / clampRatio, ZRAND_MIN(gammaResult, mean * clampRatio));
}


// ---------------------------------------------------------------------------

#pragma push
#pragma always_inline on

void zNPCGenericSwarm::Type::InitTypeParameters() {
    steeringCreator = Memory::Creator<7, zNPCSwarmSteering, zNPCSteering>::Get();
    logicCreator = Memory::Creator<1, zNPCBTManager, zNPCLogic>::Get();

    sCombatParams.hitReactionTable = sGenericSwarmCombatReactionTable;
    sCombatParams.hitReactionTableCount = 2;
}

#pragma pop

zNPCGenericSwarm::zNPCGenericSwarm(World::EntityHandleBase* handle)
    : zNPCBase(handle) {
    numberOfSwarmers = 0;
}

bool zNPCGenericSwarm::Activate(const zNPCStatus* npcStatus) {
    Sext::GenericSwarm* swarmAsset = (Sext::GenericSwarm*)npcAsset;

    if (npcTemplate != 0) {
        flying = npcTemplate->templateAsset->flags & 1;
        collectible = npcTemplate->templateAsset->flags & 4;
    }

    zDirection* dir = (zDirection*)zSceneFindObject(swarmAsset->HidePoint);

    if (dir != 0) {
        dir->GetLocation(&hidePoint);
    }

    memberSpawnTimeAcc = 0.0f;
    avgMemberSpawnPeriod = swarmAsset->SpawnPeriod;
    nextMemberSpawnPeriod = zRandDist_Gamma(4.0f, avgMemberSpawnPeriod, 5.0f);
    remainingSpawns = numberOfSwarmers;

    numberOfKilledMembers = 0;
    killedMembers = 0;

    return true;
}

void zNPCGenericSwarm::DeactivateNPC(zNPCStatus* npcStatus) {
    for (int i = 0; i < numberOfSwarmers; i++) {
        entities[i]->Disable();
        memberStatus[i] = 0;
        entFXs[i]->StopAllFX();
    }
}

void zNPCGenericSwarm::Initialize(const zNPCStatus* npcStatus) {
    Sext::NPCTemplate* npcTemplateAsset;
    Sext::GenericSwarm* assetSwarm = (Sext::GenericSwarm*)npcAsset;

    npcTemplateAsset =
        (Sext::NPCTemplate*)World::GetEntityManager()->FindAsset(
            assetSwarm->NPCTemplate);
    if (npcTemplateAsset != 0) {
        npcTemplate = zNPCManager::factory.Create<zNPCTemplate>(
            (Memory::eFactoryMemType)15);
        npcTemplate->Setup(npcTemplateAsset, this);
    }

    numberOfSwarmers = assetSwarm->MemberNumber;

    for (int i = 0; i < numberOfSwarmers; i++) {
        World::EntityHandleBase* entityHandle = handle;
        void* mem = zNPCManager::factory.AllocMemStatic(sizeof(zNPCEntity));
        entities[i] = !mem ? 0 : new (mem) zNPCEntity(entityHandle);

        mem = zNPCManager::factory.AllocMemStatic(sizeof(zNPCFX));
        entFXs[i] = !mem ? 0 : new (mem) zNPCFX;

        memberStatus[i] = 0;

        entities[i]->SetParameters(&sEntParams);
        AttachComponent(*entities[i], 0);
        entities[i]->AllAttached();

        entities[i]->Disable();

        AttachComponent(*entFXs[i], 0);
        entFXs[i]->AllAttached();

        entFXs[i]->customEnt = entities[i];

        if (entities[i]->model != 0) {
            entities[i]->model->UpdaterSwitch(&World::g_modelUpdateNone, 0);
        }
    }

    numberOfKilledMembers = 0;
    killedMembers = 0;

    averagePos = g_O3;
}

void zNPCGenericSwarm::Uninitialize(zNPCStatus* npcStatus) {
    if (npcTemplate != 0) {
        zNPCManager::factory.DeallocMem(npcTemplate);
        npcTemplate = 0;
    }

    for (int i = 0; i < numberOfSwarmers; i++) {
        if (entFXs[i] != 0) {
            DetachComponent(*entFXs[i], npcStatus);
            entFXs[i] = 0;
        }

        if (entities[i] != 0) {
            DetachComponent(*entities[i], npcStatus);
            entities[i] = 0;
        }
    }
}

bool zNPCGenericSwarm::CheckSceneSetupValidity() {
    bool wallNetValid = CheckWallNetValidity(true);
    if (!wallNetValid) {
        return false;
    }

    Sext::NPCAsset* npcAsset = this->npcAsset;
    if (npcAsset->WallNet != 0) {
        xBase* b = zSceneFindObject(npcAsset->WallNet);
        if (b == 0) {
            return false;
        }

        if (b->baseType != 0x62) {
            return false;
        }

        wallNet = (zWallNet*)b;
    }

    Sext::GenericSwarm* swarmAsset = (Sext::GenericSwarm*)this->npcAsset;

    (void)(swarmAsset->SpawnType == 0 || swarmAsset->SpawnType == 1 ||
           swarmAsset->SpawnType == 2);

    if (characterAsset->ModelInstance.modelPrototypeID == 0) {
        return false;
    }

    if (!CheckIfModelHasCollisionModel(&characterAsset->ModelInstance)) {
        return false;
    }

    return true;
}

void zNPCGenericSwarm::SceneSetup() {
    if (npcTemplate != 0) {
        zNPCBTClient* client = new (xMemAlloc(Memory::GlobalHeap, sizeof(zNPCBTClient), 0,
                                              (eMemMgrTag)84)) zNPCBTClient();
        zNPCBTActionBuilder* actionBuilder =
            new (xMemAlloc(Memory::GlobalHeap, sizeof(zNPCBTActionBuilder), 0,
                           (eMemMgrTag)85)) zNPCBTActionBuilder(client);
        zNPCBTConditionBuilder* conditionBuilder =
            new (xMemAlloc(Memory::GlobalHeap, sizeof(zNPCBTConditionBuilder), 0,
                           (eMemMgrTag)85)) zNPCBTConditionBuilder(client);

        actionBuilder->npcBase = this;
        conditionBuilder->npcBase = this;

        client->owner = this;
        client->actionBuilder = actionBuilder;
        client->conditionBuilder = conditionBuilder;
        client->SetupClient(npcTemplate->templateAsset->BehaviorSet);
        btClient = client;
    }

    zNPCGenericSwarmNS::defaultDecalGenerator =
        zSceneFindObject(xUIDMgrFindUID(xStrHash("UpBugSplatDecalRef")));
    zNPCGenericSwarmNS::defaultSplatSound =
        zSceneFindObject(xUIDMgrFindUID(xStrHash("UpBugSplatSoundRef")));
}

void zNPCGenericSwarm::Update(float dt) {
    for (int i = 0; i < numberOfSwarmers; i++) {
        if (IsMemberAlive(i)) {
            entities[i]->PreUpdate(dt);
        }
    }

    PreUpdateAllComponents(dt);

    UpdateSpawns(dt);

    zNPCLogic* logicComponent = npcLogic;
    if (logicComponent != 0) {
        logicComponent->Update(dt);
    }

    UpdateContextButtons();

    PostUpdateAllComponents(dt);

    for (int i = 0; i < numberOfSwarmers; i++) {
        if (IsMemberAlive(i)) {
            entities[i]->PostUpdate(dt);
        }
    }

    Render();

    float sumx = 0.0f;
    float sumy = 0.0f;
    float sumz = 0.0f;
    int count = 0;
    for (int i = 0; i < numberOfSwarmers; i++) {
        if (IsMemberAlive(i)) {
            xVec3* pos = &entities[i]->model->Mat.pos;
            sumx += pos->x;
            sumy += pos->y;
            sumz += pos->z;
            count++;
        }
    }

    if (count == 0) {
        __ct__Q24Math6VectorFfff(&averagePos, npcAsset->EntAsset.Pos.x,
                                 npcAsset->EntAsset.Pos.y,
                                 npcAsset->EntAsset.Pos.z);
    } else {
        __ct__Q24Math6VectorFfff(&averagePos, sumx / count, sumy / count,
                                 sumz / count);
    }
}

void zNPCGenericSwarm::UpdateSpawns(float dt) {
    if (remainingSpawns > 0) {
        memberSpawnTimeAcc += dt;

        while (memberSpawnTimeAcc - nextMemberSpawnPeriod >= 0.0f &&
               remainingSpawns > 0) {
            SpawnNewMember();
            remainingSpawns--;
            memberSpawnTimeAcc -= nextMemberSpawnPeriod;

            nextMemberSpawnPeriod =
                zRandDist_Gamma(4.0f, avgMemberSpawnPeriod, 5.0f);
        }
    }
}

void zNPCGenericSwarm::UpdateContextButtons() {
    for (int playerIdx = 0; playerIdx < xglobals->players.numPlayers;
         playerIdx++) {
        zPlayer* player = xglobals->players.playerArray[playerIdx];

        if (player->IsAI()) {
            if (player->eName == 0 &&
                IsWithinCone(player->model->Mat.pos, player->model->Mat.at,
                             0.35f, 2.25f)) {
                zUpContextAction* ca =
                    player->actionManager.contextManager.GetFreePresence(player);
                if (ca != 0) {
                    ca->uniqueActionId = 3;
                    ca->userData = this;
                }
            }
        } else {
            if (IsWithinDistance(player->model->Mat.pos, 3.0f)) {
                if (player->eName == 0) {
                    zUpContextAction* ca =
                        player->actionManager.contextManager.GetFreeActivity(
                            player, 3);
                    if (ca != 0) {
                        ca->uniqueActionId = 3;
                        ca->userData = this;
                    }
                } else if (player->eName == 1) {
                    zUpContextAction* ca =
                        player->actionManager.contextManager.GetFreeActivity(
                            player, 4);
                    if (ca != 0) {
                        ca->userData = this;
                    }
                }
            }
        }
    }
}

void zNPCGenericSwarm::Render() {
    for (int i = 0; i < numberOfSwarmers; i++) {
        if (memberStatus[i] > 0) {
            entities[i]->RenderModelInstance();
        }
    }
}

void zNPCGenericSwarm::Paused() {
    for (int i = 0; i < numberOfSwarmers; i++) {
        entFXs[i]->Paused();
    }
}

void zNPCGenericSwarm::Resumed() {
    for (int i = 0; i < numberOfSwarmers; i++) {
        entFXs[i]->Resumed();
    }
}

void zNPCGenericSwarm::NPCGetsDamage(const zNPCGetsDamageInfo& info) {
    if (info.hitReaction == 4) {
        int childNumToKill = FindChildNumFromBase(info.npcEntity);
        if (childNumToKill >= 0) {
            KillMember(childNumToKill, 0, false);
        }
    }
}

void zNPCGenericSwarm::SystemEvent(xBase* from, xBase* to,
                                   unsigned int toEvent,
                                   Sext::EventAny* baseParams) {
    switch (toEvent) {
    case 0xF5C0FB75:
        SpawnNewMember();
        return;

    case 0xB9690398: {
        int childNumToKill;

        if (to != 0 && to->baseType == 0x38) {
            childNumToKill = FindChildNumFromBase(to);
            if (childNumToKill < 0) {
                return;
            }
        } else {
            childNumToKill = GetRandomAliveChildNum();
            if (childNumToKill < 0) {
                return;
            }
        }

        KillMember(childNumToKill, from, false);
        return;
    }

    case 0xDFC57D03:
    case 0xDFC57D04:
    case 0xDFC57D05:
    case 0xDFC57D06:
    case 0xDFC57D07:
    case 0xDFC57D08:
    case 0xDFC57D09:
    case 0xDFC57D0A:
    case 0xDFC57D0B:
    case 0x820EF8B9:
    case 0x820EF8BA:
    case 0x820EF8BB:
    case 0x820EF8BC:
    case 0x820EF8BD:
    case 0x820EF8BE:
    case 0x820EF8BF: {
        Sext::EventActionOneInt* params = (Sext::EventActionOneInt*)baseParams;
        int memberIndex = 0;

        if (params != 0) {
            memberIndex = params->param0;
        }

        if ((killedMembers & (1 << memberIndex)) == 0) {
            killedMembers |= 1 << memberIndex;
            numberOfKilledMembers++;
        }
        break;
    }
    }

    zNPCBase::SystemEvent(from, to, toEvent, baseParams);
}

void zNPCGenericSwarm::KillMember(int childNum, xBase* killedBy, bool quiet) {
    memberStatus[childNum] = 0;
    entities[childNum]->Disable();

    entFXs[childNum]->StopFX(xStrHash("ActivateFXLoop"));

    entFXs[childNum]->RunFX(xStrHash("SwarmSquishFXOS"));

    zEntEvent(0, 0, this, 0x72B4A55B, 0, FE_NO);

    if (killedBy != 0) {
        int playerIdx = -1;
        for (int i = 0; i < xglobals->players.numPlayers; i++) {
            if (killedBy == xglobals->players.playerArray[i]) {
                playerIdx = i;
                break;
            }
        }

        if (playerIdx != -1) {
            Sext::EventActionOneInt params;
            params.param0 = childNum;

            switch (playerIdx) {
            case 0:
                zEntEvent(0, 0, this, 0xDFC57D03, &params, FE_NO);
                break;
            case 1:
                zEntEvent(0, 0, this, 0xDFC57D04, &params, FE_NO);
                break;
            case 2:
                zEntEvent(0, 0, this, 0xDFC57D05, &params, FE_NO);
                break;
            case 3:
                zEntEvent(0, 0, this, 0xDFC57D06, &params, FE_NO);
                break;
            case 4:
                zEntEvent(0, 0, this, 0xDFC57D07, &params, FE_NO);
                break;
            case 5:
                zEntEvent(0, 0, this, 0xDFC57D08, &params, FE_NO);
                break;
            case 6:
                zEntEvent(0, 0, this, 0xDFC57D09, &params, FE_NO);
                break;
            case 7:
                zEntEvent(0, 0, this, 0xDFC57D0A, &params, FE_NO);
                break;
            case 8:
                zEntEvent(0, 0, this, 0xDFC57D0B, &params, FE_NO);
                break;
            case 9:
                zEntEvent(0, 0, this, 0x820EF8B9, &params, FE_NO);
                break;
            case 10:
                zEntEvent(0, 0, this, 0x820EF8BA, &params, FE_NO);
                break;
            case 11:
                zEntEvent(0, 0, this, 0x820EF8BB, &params, FE_NO);
                break;
            case 12:
                zEntEvent(0, 0, this, 0x820EF8BC, &params, FE_NO);
                break;
            case 13:
                zEntEvent(0, 0, this, 0x820EF8BD, &params, FE_NO);
                break;
            case 14:
                zEntEvent(0, 0, this, 0x820EF8BE, &params, FE_NO);
                break;
            case 15:
                zEntEvent(0, 0, this, 0x820EF8BF, &params, FE_NO);
                break;
            }
        }
    }
}

int zNPCGenericSwarm::GetNumberOfChildren() { return numberOfSwarmers; }

xBase* zNPCGenericSwarm::GetChild(int index) const {
    return entities[index];
}

int zNPCGenericSwarm::FindChildNumFromBase(xBase* base) {
    zNPCEntity* childEnt = (zNPCEntity*)base;

    for (int i = 0; i < numberOfSwarmers; i++) {
        if (entities[i] == childEnt) {
            return i;
        }
    }

    return -1;
}

unsigned int zNPCGenericSwarm::GetNPCLODPositions(xVec3* positions) {
    unsigned int numberOfLODPositions = 0;

    positions[numberOfLODPositions++] = (const xVec3&)npcAsset->EntAsset.Pos;

    return numberOfLODPositions;
}

int zNPCGenericSwarm::GetTotalAliveChildNum() {
    int aliveChildNum = 0;
    for (int i = 0; i < numberOfSwarmers; i++) {
        if (memberStatus[i] > 0) aliveChildNum++;
    }

    return aliveChildNum;
}

int zNPCGenericSwarm::GetNextDeadChildNum() {
    for (int i = 0; i < numberOfSwarmers; i++) {
        if (memberStatus[i] <= 0) {
            return i;
        }
    }

    return -1;
}

int zNPCGenericSwarm::GetRandomAliveChildNum() {
    int aliveChildNum = GetTotalAliveChildNum();
    if (aliveChildNum == 0) {
        return -1;
    }

    int randomChildNum = xrand_RandomRange(0, aliveChildNum - 1);
    for (int i = 0; i < numberOfSwarmers; i++) {
        if (memberStatus[i] > 0) {
            if (randomChildNum == 0) {
                return i;
            }
            randomChildNum--;
        }
    }

    for (int i = 0; i < numberOfSwarmers; i++) {
        if (memberStatus[i] > 0) {
            return i;
        }
    }

    return -1;
}

void zNPCGenericSwarm::CreateRandomPopSpawnMat(xVec3& pos, float radius,
                                               xMat4x3& spawnMat,
                                               const zWallNet* wallNet) {
    xVec3& spawnPos = spawnMat.pos;
    unsigned int tries = 0;

    while (true) {
        spawnPos.x = xrandf(-1.0f, 1.0f);
        if (flying) {
            spawnPos.y = xrandf(-1.0f, 1.0f);
        } else {
            spawnPos.y = 0.0f;
        }
        spawnPos.z = xrandf(-1.0f, 1.0f);
        spawnPos.safe_normalize(xVec3::m_UnitAxisX);

        float distFromCenter = xrand_RandomFloatRange(0.0f, xrandf(0.0f, radius));
        spawnPos *= distFromCenter;

        spawnPos += pos;

        if (wallNet != 0 && wallNet->IsInsideWallNetXZ(spawnPos)) {
            break;
        }

        tries++;
        if (tries > 3) {
            spawnPos = pos;
            break;
        }
    }

    xMat3x3Euler(&spawnMat, xrandf(0.0f, 6.2831855f), 0.0f, 0.0f);
}

bool zNPCGenericSwarm::IsWithinCone(xVec3& sourcePos, xVec3& sourceDir,
                                    float coneAngle, float coneDistance2) {
    for (int i = 0; i < numberOfSwarmers; i++) {
        if (memberStatus[i] > 0) {
            xVec3 toBugVec = entities[i]->model->Mat.pos - sourcePos;

            float dist2 = toBugVec.dot(toBugVec);
            if (dist2 <= coneDistance2) {
                toBugVec.normalize();
                float actualAngle = toBugVec.dot(sourceDir);
                if (coneAngle <= actualAngle) {
                    return true;
                }
            }
        }
    }

    return false;
}

bool zNPCGenericSwarm::IsWithinDistance(xVec3& sourcePos, float distance) {
    for (int i = 0; i < numberOfSwarmers; i++) {
        if (memberStatus[i] > 0) {
            float testDist = sourcePos.Distance(entities[i]->model->Mat.pos);
            if (testDist <= distance) {
                return true;
            }
        }
    }

    return false;
}
