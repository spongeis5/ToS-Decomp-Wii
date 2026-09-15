#include "SB/GM/Engine/Game/zNPCBase.pool.h"

// zNPCBase.cpp -- the NPC base: construction and loading from its asset, the
// component lifecycle (create, attach, update, pause, forward events,
// detach), the info-node list, the system-event switch, spawning and
// killing, wall-net tests and damage routing. Read from the image with
// tools/brief.py; the layouts are the DWARF's (tools/dwarf_types.py), the
// virtual slots the image's (tools/vtslot.py).
//
// Components keep their owner in front of the vtable pointer at +4: slot 0
// Attached, 1 Detached, 2 Reset, 3 AllAttached, 4 PreUpdate, 5 PostUpdate,
// 6 Render, 7 Paused, 8 Resumed, 9 SystemEvent (__vt__6zNPCFX,
// __vt__14zNPCExtraModel). The entity is itself a component, at +0xBC;
// through its own class the component calls take slots 45 to 53 of its
// primary vtable.

class xBase;
class xEnt;
class zNPCBase;
class zNPCEntity;
class zNPCStatus;
class zPlayer;

namespace World {
class EntityHandleBase;
class ModelInstanceAsset;
}  // namespace World

inline void* operator new(unsigned long size, void* mem) { return mem; }

extern "C" void* memset(void* dst, int c, unsigned long n);

namespace Sext {

class EventAny {};
class EventActionNew : public EventAny {};

class uid {
public:
    // Passed where an id is wanted, a uid is converted by this inline,
    // which mwcc evaluates ahead of the call's object.
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
    unsigned char flags;
    unsigned char _pad0[0x2C - 0x11];
    vec3 Pos;
    unsigned char _pad1[0x100 - 0x38];
};

class LinkAsset {
public:
    unsigned char _pad0[0xC];
};

class NPCAsset : public xBaseAsset {
public:
    xEntAsset EntAsset;
    unsigned char _pad0[0x140 - 0x110];
    unsigned int EnemyFlags;
    uid WallNet;
    uid MovePoint;
    uid MovePointNetwork;
    unsigned int SpawnType;
    LinkAsset EventLinksNew;
    uid SpawnJumpToMP;
};

class EventActionOneFloat : public EventActionNew {
public:
    float param0;
};

class EventActionDrivenBy : public EventActionNew {
public:
    bool param0;
    bool param1;
    bool param2;
    bool cam;
    uid specificPassenger;
    int bone;
};

class EventActionAdjustHitpoint : public EventActionNew {
public:
    int Method;
    float HP;
};

}  // namespace Sext

enum ForceEvent {
    FE_YES = 0,
    FE_NO = 1
};

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

enum eNPCStatus {
    eNPCStatus_Unknown = 0,
    eNPCStatus_SpawnedAndAlive = 1,
    eNPCStatus_Dead = 2
};

enum eNPCInfoNodeType {
    eNPCInfoNodeType_First = 0,
    eNPCInfoNodeTypeForceInt = 0x7FFFFFFF
};

enum eNPCHitReaction { eNPCHitReaction_ = 0x7FFFFFFF };

namespace Memory {

enum GlobalHeapEnum { GlobalHeap = 0 };

class Factory {
public:
    void DeallocMem(void* mem);
};

}  // namespace Memory

void* xMemAlloc(Memory::GlobalHeapEnum heap, unsigned int size, int align,
                eMemMgrTag tag);
void zEntEvent(xBase* from, unsigned int fromEvent, xBase* to,
               unsigned int toEvent, Sext::EventAny* param, ForceEvent force);
xBase* zSceneFindObject(unsigned long long id);
unsigned int xStrHash(const char* str);
void xBaseInit(xBase* base, const Sext::xBaseAsset* asset);

class xModelAssetInfo;

xModelAssetInfo* zNPCAsset_GetModelAssetInfo(Sext::NPCAsset* asset);
class zCharacterAsset;
zCharacterAsset* zNPCAsset_GetCharacterAsset(const Sext::NPCAsset* asset);

class zNPCManager {
public:
    static Memory::Factory factory;
};

// ---------------------------------------------------------------------------
// Vectors

class xVec3 {
public:
    xVec3& operator=(const xVec3& other);

    float x;
    float y;
    float z;
};

float xVec3Dist2(const xVec3* a, const xVec3* b);

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

class xOGModel {
public:
    xMat4x3 Mat;
};

// ---------------------------------------------------------------------------
// The entity classes. Entity is polymorphic from +0; the twenty slots below
// are the ones zNPCBase's and zNPCEntity's vtables carry ahead of their own.

class EntityVirtuals {
public:
    virtual void _b0();
    virtual void _b1();
    virtual void _b2();
    virtual void _b3();
    virtual void UpdateRender();
    virtual void _b5();
    virtual void _b6();
    virtual void _b7();
    virtual void _b8();
    virtual void DriveAttach(xBase* driver, unsigned int flags, int bone);
    virtual void DriveDetach();
    virtual void DriveOn();
    virtual void DriveOff();
    virtual void DriveReset();
    virtual void DriveMoved();
    virtual void DriveMovedNoPass();
    virtual void _b16();
    virtual void _b17();
    virtual void _b18();
    virtual void _b19();
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
    xOGEntity(EntityHandleBase* handle);

    xOGModel* model;
    void* modelAutoptr;
};

class EntityManager {
public:
    static void* FindAsset(unsigned long long id);
};

EntityManager* GetEntityManager();

}  // namespace World

#pragma pack(pop)

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
    virtual void _x44();
    // Slots 45 to 53 are the component calls: zNPCEntity's overrides take
    // them here, not after its component base's slots.
    virtual void Attached(const zNPCStatus* status);
    virtual void AllAttached();
    virtual void Detached(zNPCStatus* status);
    virtual void Reset(const zNPCStatus* status);
    virtual void PreUpdate(float dt);
    virtual void PostUpdate(float dt);
    virtual void Paused();
    virtual void Resumed();
    virtual bool SystemEvent(xBase* from, xBase* to, unsigned int to_event,
                             Sext::EventAny* params);

    unsigned char _pad0[0xBC - 0x3C];
};

// ---------------------------------------------------------------------------
// Components

class zNPCComponent {
public:
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
    virtual bool SystemEvent(xBase* from, xBase* to, unsigned int to_event,
                             Sext::EventAny* params);
};

class zNPCGetsDamageInfo {
public:
    void SetFromCombatDamageInfo(xEnt* npcEnt, const class zCombatDamageInfo& info,
                                 eNPCHitReaction reaction, float remaining);

    unsigned int flags;
    xBase* from;
    unsigned char _pad0[0x2C - 0x8];
};

class zNPCGivesDamageInfo {
public:
    void SetFromCombatDamageInfo(xEnt* toEnt, xEnt* npcEnt,
                                 const class zCombatDamageInfo& info);

    unsigned char _pad0[0x14];
};

class zCombatDamageInfo {
public:
    int flags;
    xBase* from;
};

class zNPCLogic : public zNPCComponent {
public:
    virtual void _l10();
    virtual void NPCGetsDamage(const zNPCGetsDamageInfo& info);
    virtual void NPCGivesDamage(const zNPCGivesDamageInfo& info);
};

class zCombat {
public:
    // Read through this inline, the hit points load after the adjustment.
    float GetCurHitPoints() const { return currentHitPoints; }

    unsigned char _pad0[0x34];
    float currentHitPoints;
};

class zNPCCombat : public zNPCComponent {
public:
    virtual void _k10();
    virtual bool IsDead() const;

    void SetCurHitPoints(float hp);
    bool HandleNPCDamage(xEnt* npcEnt, const zCombatDamageInfo& damageInfo,
                         zNPCGetsDamageInfo* npcDamageInfo);

    int attackID;
    unsigned char _pad0[0x10 - 0xC];
    zCombat baseCombat;
};

class zWallNetBound {
public:
    unsigned char firstVertex;
    unsigned char verticesNum;
    unsigned char minZVertex;
    unsigned char maxZVertex;
};

class zWallNetAsset {
public:
    unsigned char _pad0[0x14];
    int numBounds;
    unsigned char _pad1[0x34 - 0x18];
    zWallNetBound* bounds;
    xVec3* vertices;
};

class zWallNet {
public:
    bool IsInsideWallNet(const xVec3& pos) const;

    // Is the point inside the net's outer bound and outside every hole?
    // NEAR MISS: 5 of 42 words; ours loads wallNetAsset between the `mflr`
    // and the save of the return address, retail after the parameter
    // copies (its line table gives that load the function's own line).
    // Out of the class, not inline, the count's declaration split,
    // `&bounds[0]`, a local asset pointer and three accessors measure the
    // same.
    bool IsInsideWallNetXZ(const xVec3& pos) const {
        int numberOfBounds = wallNetAsset->numBounds;
        if (numberOfBounds == 0) {
            return false;
        }

        if (!IsInsideBoundXZ(wallNetAsset->bounds, pos)) {
            return false;
        }

        for (int i = 1; i < numberOfBounds; i++) {
            if (IsInsideBoundXZ(&wallNetAsset->bounds[i], pos)) {
                return false;
            }
        }

        return true;
    }

    // Even-odd crossing test of the bound's polygon in XZ. Only the j vertex
    // is read through a reference: that gives retail's r6, r7, r8 for j,
    // inside and i.
    bool IsInsideBoundXZ(zWallNetBound* bound, const xVec3& pos) const {
        int firstVertexID = bound->firstVertex;
        int lastVertexID = firstVertexID + bound->verticesNum - 1;

        float posX = pos.x;
        float posZ = pos.z;

        bool inside = false;
        int i, j;
        for (i = firstVertexID, j = lastVertexID; i <= lastVertexID; j = i++) {
            const xVec3& vj = wallNetAsset->vertices[j];

            if (wallNetAsset->vertices[i].z <= posZ) {
                if (posZ < vj.z) {
                    float m = (vj.x - wallNetAsset->vertices[i].x) * (posZ - wallNetAsset->vertices[i].z);
                    m = m / (vj.z - wallNetAsset->vertices[i].z);
                    m = m + wallNetAsset->vertices[i].x;

                    if (posX <= m) {
                        inside = !inside;
                    }
                }
            } else {
                if (vj.z <= posZ) {
                    float m = (vj.x - wallNetAsset->vertices[i].x) * (posZ - wallNetAsset->vertices[i].z);
                    m = m / (vj.z - wallNetAsset->vertices[i].z);
                    m = m + wallNetAsset->vertices[i].x;

                    if (posX <= m) {
                        inside = !inside;
                    }
                }
            }
        }

        return inside;
    }

    unsigned char _pad0[0x3C];
    zWallNetAsset* wallNetAsset;
};

class zNPCSteering : public zNPCComponent {
public:
    unsigned char _pad0[0x34 - 0x8];
    zWallNet* wallNet;
};

class zNPCSteeringOld : public zNPCComponent {};
class zNPCPerception : public zNPCComponent {};
class zNPCQuickTimeCombat : public zNPCComponent {};
class zNPCFX : public zNPCComponent {};

class zNPCEntity : public xEnt, public zNPCComponent {
public:
    zNPCEntity(World::EntityHandleBase* handle);

    void Enable();
    void Disable();
    void SetAnimState(unsigned int animID, float blend, const char* name);
    void ResetDamageColor();

    virtual void Attached(const zNPCStatus* status);
    virtual void AllAttached();
    virtual void Detached(zNPCStatus* status);
    virtual void Reset(const zNPCStatus* status);
    virtual void PreUpdate(float dt);
    virtual void PostUpdate(float dt);
    virtual void Paused();
    virtual void Resumed();
    virtual bool SystemEvent(xBase* from, xBase* to, unsigned int to_event,
                             Sext::EventAny* params);

    unsigned char _pad1[0x184 - 0xC4];
    float damageColorTimer;
    unsigned char _pad2[0x1D0 - 0x188];
};

// The info nodes: a list threaded through `next`, the vtable pointer at
// +0xC.
class zNPCInfoNode {
public:
    zNPCInfoNode* next;
    zNPCBase* owner;
    eNPCInfoNodeType infoNodeType;

    virtual void Attached();
    virtual void Detached();
    virtual void Update(float dt);
};

class ModelPrototypeEntity {
public:
    unsigned char _pad0[0x6C];
    void* collmeshBlob;
};

class xOGRenderHelper {
public:
    static ModelPrototypeEntity* GetModelPrototypeEntity(
        const World::ModelInstanceAsset* asset);
};

class zCharacterAsset {
public:
    unsigned char _pad0[0x88];
    unsigned int numExtraModels;
};

class CreatorI {
public:
    virtual void* Create(Memory::Factory* factory);
};

class zNPCType {
public:
    unsigned char _pad0[0x14];
    bool hasEntity;
    CreatorI* logicCreator;
    CreatorI* steeringCreator;
    CreatorI* perceptionCreator;
    CreatorI* combatCreator;
    CreatorI* quickTimeCombatCreator;
    CreatorI* fxCreator;
    CreatorI* extraModelCreator[2];
};

class zNPCStatus {
public:
    void ResetToNPCAsset(const Sext::NPCAsset* asset);

    xVec3 lastPos;
    xVec3 lastOrientation;
    eNPCStatus lastStatus;
};

// The LOD record's constructor is out of line and the linker folded it into
// bit_array_alloc's.
class bit_array_alloc {
public:
    bit_array_alloc();
};

class zNPCUpdateLOD : public bit_array_alloc {
public:
    void Reset(zNPCBase* npc, const Sext::NPCAsset* asset);

    int lodType;
    void* lodCylinder;
    zNPCBase* npcBase;
};

class zPlayer {
public:
    unsigned char _pad0[0x34];
    xOGModel* model;
    unsigned char _pad1[0x2EC - 0x38];
    int eName;
};

class zSBPlayer {
public:
    void IncreaseBuffKillCounter();
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

class zNPCTemplate;
class zNPCBTClientBase;
class zNPCGroupBase;

class xMovePoint {
public:
    unsigned char _pad0[0x40];
    xVec3* pos;
};

// ---------------------------------------------------------------------------
// The NPC

class zNPCBase : public World::xOGEntity {
public:
    zNPCBase(World::EntityHandleBase* handle);

    static void EventWrapper(xBase* from, xBase* to, unsigned int to_event,
                             Sext::EventAny* params);

    virtual void UpdateRender();
    virtual void SystemEvent(xBase* from, xBase* to, unsigned int to_event,
                             Sext::EventAny* params);
    virtual void Load(const zNPCType* npcType, Sext::NPCAsset* npcAsset,
                      unsigned int count);
    virtual unsigned int GetNPCLODPositions(xVec3* positions);
    virtual void _n23();
    virtual void _n24();
    virtual bool IsAutoTargetable();
    virtual void SetAnimState(char* playAnim);
    virtual xVec3* GetPositionSound() const;
    virtual int GetNumberOfChildren();
    virtual zNPCEntity* GetChild(int index) const;
    virtual void _n30();
    virtual void _n31();
    virtual void NPCGetsDamage(const zNPCGetsDamageInfo& info);
    virtual void NPCGivesDamage(const zNPCGivesDamageInfo& info);
    virtual void _n34();
    virtual void _n35();
    virtual void _n36();
    virtual void Reset();
    virtual void Initialize(zNPCStatus* status);
    virtual bool Activate(zNPCStatus* status);
    virtual void Deactivate(zNPCStatus* status);
    virtual void Uninitialize(zNPCStatus* status);
    virtual void _n42();
    virtual void Update(float dt);

    // Weak in the image: zNPCBase.h's.
    void Remove() { alive = present = false; }

    // Read through this inline, the asset is loaded again for the call.
    const Sext::NPCAsset* GetNPCAsset() const { return npcAsset; }

    void BaseInitialize();
    void BaseUninitialize();
    bool BaseActivate();
    void BaseDeactivate();
    void BaseReset();
    void BaseUpdate(float dt);
    void AllAttached();
    void PreUpdateAllComponents(float dt);
    void PostUpdateAllComponents(float dt);
    void PauseAllComponents();
    void ResumeAllComponents();
    bool ForwardEventToComponents(xBase* from, xBase* to,
                                  unsigned int to_event,
                                  Sext::EventAny* params);
    void DetachAllComponents(zNPCStatus* npcStatus);
    void ResetAllComponents(const zNPCStatus* npcStatus);
    zNPCInfoNode* GetInfoNode(eNPCInfoNodeType infoNodeType);
    void UpdateInfoNodes(float dt);
    void AttachInfoNode(zNPCInfoNode* infoNode,
                        eNPCInfoNodeType infoNodeType);
    void DetachInfoNode(eNPCInfoNodeType infoNodeType);
    void DetachAllInfoNodes();
    void GetPosition(xVec3& pos);
    void Kill(bool quiet);
    void Spawn();
    bool CheckWallNetValidity(bool noWallNet);
    bool CheckIfModelHasCollisionModel(
        World::ModelInstanceAsset* modelInstanceAsset);
    void GiveNPCDamage(xEnt* npcEnt, const zCombatDamageInfo& damageInfo);
    bool HandleNPCDamage(xEnt* npcEnt, const zCombatDamageInfo& damageInfo,
                         zNPCGetsDamageInfo* npcDamageInfo);
    void NPCGivesDamage(xEnt* toEnt, xEnt* npcEnt,
                        const zCombatDamageInfo& damageInfo);
    zPlayer* GetClosestPlayerOnWallnet(bool ignoreHeight);

    // A component joins the NPC, or leaves it if it had joined.
    void AttachComponent(zNPCComponent& component) {
        component.owner = this;
        component.Attached(&npcStatus);
    }

    void DetachComponent(zNPCComponent& component, zNPCStatus* status) {
        if (component.owner != 0) {
            component.Detached(status);
            component.owner = 0;
        }
    }

    int numProjectiles;
    zNPCStatus npcStatus;
    const zNPCType* type;
    Sext::NPCAsset* npcAsset;
    xModelAssetInfo* modelAsset;
    zCharacterAsset* characterAsset;
    zNPCGroupBase* parentGroup;
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
    xMovePoint* npcMovePoint;
    xMovePoint* npcMovePointNetwork;
    zNPCUpdateLOD npcUpdateLOD;
    zNPCLogic* npcLogic;
    zNPCEntity* npcEntity;
    zNPCSteeringOld* npcSteeringOld;
    zNPCSteering* npcSteering;
    zNPCPerception* npcPerception;
    zNPCCombat* npcCombat;
    zNPCQuickTimeCombat* npcQuickTimeCombat;
    zNPCFX* npcFX;
    // zNPCExtraModel* in the DWARF; typed as the component every call here
    // makes through it.
    zNPCComponent* npcExtraModels[2];
    zNPCInfoNode* npcInfoNodesFront;
};

// ---------------------------------------------------------------------------
// Construction, events, loading

zNPCBase::zNPCBase(World::EntityHandleBase* handle)
    : World::xOGEntity(handle) {
    npcLogic = 0;
    npcEntity = 0;
    npcSteeringOld = 0;
    npcSteering = 0;
    npcPerception = 0;
    npcCombat = 0;
    npcQuickTimeCombat = 0;
    npcFX = 0;
    for (unsigned int i = 0; i < 2; i++) {
        npcExtraModels[i] = 0;
    }
    npcInfoNodesFront = 0;

    npcTemplate = 0;
    btClient = 0;

    parentGroup = 0;
    alive = false;
    present = false;
    activated = false;
    spawned = false;
    paused = false;
    updateInCinematicAlways = false;
    updateInCinematicNever = false;
    infoNodesUpdating = false;
    updating = false;
    flying = false;
    collectible = false;

    memset(&npcUpdateLOD, 0, sizeof(zNPCUpdateLOD));
}

void zNPCBase::EventWrapper(xBase* from, xBase* to, unsigned int to_event,
                            Sext::EventAny* params) {
    ((zNPCBase*)to)->SystemEvent(from, to, to_event, params);
}

void zNPCBase::SystemEvent(xBase* from, xBase* to, unsigned int to_event,
                           Sext::EventAny* params) {
    switch (to_event) {
    case 0xCAE0039D:
        puppetMode = true;
        break;
    case 0x38463CDD:
        puppetMode = false;
        break;
    case 0xBBBB8ECF:
        Spawn();
        return;
    case 0x080601AB:
        BaseReset();
        alive = present = true;
        return;
    case 0x389E01C0:
        npcAsset = (Sext::NPCAsset*)World::GetEntityManager()->FindAsset(id);
        BaseReset();
        return;
    case 0xA8B93047:
        BaseReset();
        return;
    case 0x0A20012A:
        if (npcLogic == 0 ||
            !npcLogic->SystemEvent(from, to, to_event, params)) {
            Kill(false);
            Remove();
        }
        return;
    case 0x25FFF7B7:
        Remove();
        return;
    case 0x2CEA83B4:
        updateInCinematicAlways = true;
        updateInCinematicNever = false;
        return;
    case 0xFEF58127:
        updateInCinematicAlways = false;
        updateInCinematicNever = true;
        return;
    case 0x26A30F94:
        updateInCinematicAlways = false;
        updateInCinematicNever = false;
        return;
    case 0x3FE52B13: {
        Sext::EventActionDrivenBy* drivenParams =
            (Sext::EventActionDrivenBy*)params;
        unsigned int flags = 0;

        if (drivenParams != 0) {
            if (drivenParams->param0) {
                flags |= 1;
            }
            if (drivenParams->param1) {
                flags |= 2;
            }
            if (drivenParams->param2) {
                flags |= 4;
            }
        }

        xBase* possibleDriver;

        if (drivenParams != 0 &&
            drivenParams->specificPassenger.internalUid != 0) {
            possibleDriver = zSceneFindObject(drivenParams->specificPassenger);
            if (possibleDriver == 0) {
                possibleDriver = (xBase*)World::GetEntityManager()->FindAsset(
                    drivenParams->specificPassenger);
            }
            npcEntity->DriveAttach(possibleDriver, flags, -1);
            break;
        }

        npcEntity->DriveAttach(from, flags, -1);
        break;
    }
    case 0x3954A566:
        npcEntity->DriveOn();
        break;
    case 0x56509F60:
        npcEntity->DriveOff();
        break;
    case 0xF9090A3B:
        npcEntity->DriveDetach();
        break;
    case 0x67B3F558:
        if (npcCombat != 0) {
            Sext::EventActionAdjustHitpoint* paramsSetHP =
                (Sext::EventActionAdjustHitpoint*)params;
            float newHP;

            switch (paramsSetHP->Method) {
            case 0:
                newHP = paramsSetHP->HP + npcCombat->baseCombat.GetCurHitPoints();
                break;
            case 1:
                newHP = npcCombat->baseCombat.currentHitPoints - paramsSetHP->HP;
                break;
            case 2:
                newHP = paramsSetHP->HP;
                break;
            }

            npcCombat->SetCurHitPoints(newHP);
        }
        break;
    case 0x2D44415D:
        numProjectiles++;
        break;
    case 0x00C8A50E:
        numProjectiles--;
        break;
    case 0xD68B7CAD:
        npcEntity->damageColorTimer =
            ((Sext::EventActionOneFloat*)params)->param0;
    }

    ForwardEventToComponents(from, to, to_event, params);
}

// NEAR MISS: 43 of 54 words, and the whole of it is one instruction:
// retail branches `bne +8; b end` over the early return where ours folds
// the pair into one `beq end`, so every later word is one early. The
// DWARF gives `entass` r0 over the compare, the store and both branches.
// Twenty-seven spellings keep the fold: no local, and the local split,
// voided, tested, assigned to itself, passed to an empty inline or used
// by a dead `if (0)` call; `!modelAsset`, the assignment in the test, a
// goto, a do-while, the rest of the body in an else, the test inverted;
// and four arms holding a no-op removed only late, the mechanism that
// keeps zNPCFX's extra `b` (NOTES.md).
void zNPCBase::Load(const zNPCType* npcType, Sext::NPCAsset* npcAsset,
                    unsigned int count) {
    type = npcType;
    this->npcAsset = npcAsset;

    modelAsset = zNPCAsset_GetModelAssetInfo(npcAsset);
    if (modelAsset == 0) {
        Sext::xEntAsset* entass = &npcAsset->EntAsset;

        return;
    }

    characterAsset = zNPCAsset_GetCharacterAsset(npcAsset);
    eventFunc = EventWrapper;

    this->npcAsset->EntAsset.flags |= npcAsset->baseFlags & 3;
    this->npcAsset->EntAsset.flags &= ~1;

    activated = false;
    present = alive = npcAsset->EnemyFlags & 1;
    spawned = false;
    paused = false;
    updateInCinematicAlways = false;
    updateInCinematicNever = false;
    flying = false;
    collectible = false;

    xBaseInit(this, &npcAsset->EntAsset);

    BaseReset();

    baseFlags = 265;

    linkArray = &npcAsset->EventLinksNew;
}

// ---------------------------------------------------------------------------
// The component lifecycle

void zNPCBase::BaseInitialize() {
    if (type->hasEntity) {
        World::EntityHandleBase* handle = this->handle;

        npcEntity = new (xMemAlloc(Memory::GlobalHeap, sizeof(zNPCEntity), 0,
                                   (eMemMgrTag)16)) zNPCEntity(handle);
    }

    Initialize(&npcStatus);

    if (type->hasEntity) {
        AttachComponent(*npcEntity);
    }

    npcUpdateLOD.Reset(this, npcAsset);
}

void zNPCBase::BaseUninitialize() {
    Uninitialize(&npcStatus);

    if (npcEntity != 0) {
        DetachComponent(*npcEntity, &npcStatus);
        npcEntity = 0;
    }
}

// NEAR MISS: 4 of 245 words; the extra-model count and the this + i*4
// base of the store sit in r27 and r28 the other way round from retail.
// With no local the count is reloaded every iteration (16 words);
// declared in the for, at the top, const, int, in a block, behind a
// `continue`, in an `&&` or with a reference to the slot, the swap stays.
bool zNPCBase::BaseActivate() {
    if (type->steeringCreator != 0) {
        npcSteering =
            (zNPCSteering*)type->steeringCreator->Create(&zNPCManager::factory);
        if (npcSteering == 0) {
            return false;
        }
    }

    if (type->perceptionCreator != 0) {
        npcPerception = (zNPCPerception*)type->perceptionCreator->Create(
            &zNPCManager::factory);
        if (npcPerception == 0) {
            return false;
        }
    }

    if (type->combatCreator != 0) {
        npcCombat =
            (zNPCCombat*)type->combatCreator->Create(&zNPCManager::factory);
        if (npcCombat == 0) {
            return false;
        }
    }

    if (type->quickTimeCombatCreator != 0) {
        npcQuickTimeCombat =
            (zNPCQuickTimeCombat*)type->quickTimeCombatCreator->Create(
                &zNPCManager::factory);
        if (npcQuickTimeCombat == 0) {
            return false;
        }
    }

    if (type->logicCreator != 0) {
        npcLogic = (zNPCLogic*)type->logicCreator->Create(&zNPCManager::factory);
        if (npcLogic == 0) {
            return false;
        }
    }

    if (type->fxCreator != 0) {
        npcFX = (zNPCFX*)type->fxCreator->Create(&zNPCManager::factory);
        if (npcFX == 0) {
            return false;
        }
    }

    unsigned int numExtraModels = characterAsset->numExtraModels;

    for (unsigned int i = 0; i < 2; i++) {
        if (i < numExtraModels) {
            if (type->extraModelCreator[i] != 0) {
                npcExtraModels[i] =
                    (zNPCComponent*)type->extraModelCreator[i]->Create(
                        &zNPCManager::factory);
                if (npcExtraModels[i] == 0) {
                    return false;
                }
            }
        }
    }

    if (!Activate(&npcStatus)) {
        return false;
    }

    if (type->steeringCreator != 0) {
        AttachComponent(*npcSteering);
    }
    if (type->perceptionCreator != 0) {
        AttachComponent(*npcPerception);
    }
    if (type->combatCreator != 0) {
        AttachComponent(*npcCombat);
    }
    if (type->quickTimeCombatCreator != 0) {
        AttachComponent(*npcQuickTimeCombat);
    }
    if (type->logicCreator != 0) {
        AttachComponent(*npcLogic);
    }
    if (type->fxCreator != 0) {
        AttachComponent(*npcFX);
    }

    for (unsigned int i = 0; i < 2; i++) {
        if (npcExtraModels[i] != 0) {
            AttachComponent(*npcExtraModels[i]);
        }
    }

    if (npcEntity != 0) {
        npcEntity->Enable();
    }

    AllAttached();

    activated = true;
    quietKill = false;
    if (!spawned) {
        spawned = true;
        zEntEvent(0, 0, this, 0xAFF855DA, 0, FE_NO);
    }

    return true;
}

void zNPCBase::BaseDeactivate() {
    Deactivate(&npcStatus);

    activated = false;
    spawned = false;

    if (npcEntity != 0) {
        npcEntity->Disable();
    }

    for (unsigned int i = 0; i < 2; i++) {
        if (type->extraModelCreator[i] != 0 && npcExtraModels[i] != 0) {
            DetachComponent(*npcExtraModels[i], &npcStatus);
            zNPCManager::factory.DeallocMem(npcExtraModels[i]);
            npcExtraModels[i] = 0;
        }
    }

    if (type->fxCreator != 0 && npcFX != 0) {
        DetachComponent(*npcFX, &npcStatus);
        zNPCManager::factory.DeallocMem(npcFX);
        npcFX = 0;
    }

    if (type->logicCreator != 0 && npcLogic != 0) {
        DetachComponent(*npcLogic, &npcStatus);
        zNPCManager::factory.DeallocMem(npcLogic);
        npcLogic = 0;
    }

    if (type->combatCreator != 0 && npcCombat != 0) {
        DetachComponent(*npcCombat, &npcStatus);
        zNPCManager::factory.DeallocMem(npcCombat);
        npcCombat = 0;
    }

    if (type->quickTimeCombatCreator != 0 && npcQuickTimeCombat != 0) {
        DetachComponent(*npcQuickTimeCombat, &npcStatus);
        zNPCManager::factory.DeallocMem(npcQuickTimeCombat);
        npcQuickTimeCombat = 0;
    }

    if (type->perceptionCreator != 0 && npcPerception != 0) {
        DetachComponent(*npcPerception, &npcStatus);
        zNPCManager::factory.DeallocMem(npcPerception);
        npcPerception = 0;
    }

    if (type->steeringCreator != 0 && npcSteering != 0) {
        DetachComponent(*npcSteering, &npcStatus);
        zNPCManager::factory.DeallocMem(npcSteering);
        npcSteering = 0;
    }

    DetachAllInfoNodes();
}

void zNPCBase::BaseReset() {
    puppetMode = false;
    numProjectiles = 0;

    alive = present = npcAsset->EnemyFlags & 1;
    quietKill = false;
    spawned = false;

    npcStatus.ResetToNPCAsset(GetNPCAsset());
    npcStatus.lastStatus = eNPCStatus_Unknown;

    ResetAllComponents(&npcStatus);

    if (activated) {
        BaseDeactivate();
    }

    Reset();
}

void zNPCBase::BaseUpdate(float dt) {
    updating = true;

    bool wasAlive = alive;

    Update(dt);

    UpdateInfoNodes(dt);

    if (wasAlive && !alive) {
        if (!quietKill) {
            zEntEvent(0, 0, npcEntity, 0xBB6E2C0D, 0, FE_NO);
        }
    }

    updating = false;
}

unsigned int zNPCBase::GetNPCLODPositions(xVec3* positions) {
    if (npcEntity != 0) {
        *positions = npcEntity->model->Mat.pos;
    } else {
        *positions = (const xVec3&)npcAsset->EntAsset.Pos;
    }

    return 1;
}

void zNPCBase::AllAttached() {
    if (npcEntity != 0) {
        npcEntity->AllAttached();
    }
    if (npcSteeringOld != 0) {
        npcSteeringOld->AllAttached();
    }
    if (npcSteering != 0) {
        npcSteering->AllAttached();
    }
    if (npcPerception != 0) {
        npcPerception->AllAttached();
    }
    if (npcCombat != 0) {
        npcCombat->AllAttached();
    }
    if (npcQuickTimeCombat != 0) {
        npcQuickTimeCombat->AllAttached();
    }
    if (npcFX != 0) {
        npcFX->AllAttached();
    }

    for (unsigned int i = 0; i < 2; i++) {
        if (npcExtraModels[i] != 0) {
            npcExtraModels[i]->AllAttached();
        }
    }

    if (npcLogic != 0) {
        npcLogic->AllAttached();
    }

    npcMovePoint =
        (xMovePoint*)zSceneFindObject(npcAsset->MovePoint.internalUid);
    npcMovePointNetwork =
        (xMovePoint*)zSceneFindObject(npcAsset->MovePointNetwork.internalUid);
}

void zNPCBase::PreUpdateAllComponents(float dt) {
    if (npcLogic != 0) {
        npcLogic->PreUpdate(dt);
    }
    if (npcEntity != 0) {
        npcEntity->PreUpdate(dt);
    }
    if (npcSteeringOld != 0) {
        npcSteeringOld->PreUpdate(dt);
    }
    if (npcSteering != 0) {
        npcSteering->PreUpdate(dt);
    }
    if (npcPerception != 0) {
        npcPerception->PreUpdate(dt);
    }
    if (npcCombat != 0) {
        npcCombat->PreUpdate(dt);
    }
    if (npcQuickTimeCombat != 0) {
        npcQuickTimeCombat->PreUpdate(dt);
    }
    if (npcFX != 0) {
        npcFX->PreUpdate(dt);
    }
}

void zNPCBase::PostUpdateAllComponents(float dt) {
    if (npcSteeringOld != 0) {
        npcSteeringOld->PostUpdate(dt);
    }

    if (npcSteering != 0) {
        npcSteering->PostUpdate(dt);
    }

    if (npcLogic != 0) {
        npcLogic->PostUpdate(dt);
    }

    if (npcEntity != 0) {
        npcEntity->PostUpdate(dt);
    }

    if (npcPerception != 0) {
        npcPerception->PostUpdate(dt);
    }

    if (npcCombat != 0) {
        npcCombat->PostUpdate(dt);
    }

    if (npcQuickTimeCombat != 0) {
        npcQuickTimeCombat->PostUpdate(dt);
    }

    if (npcFX != 0) {
        npcFX->PostUpdate(dt);
    }
}

void zNPCBase::PauseAllComponents() {
    if (npcLogic != 0) {
        npcLogic->Paused();
    }
    if (npcEntity != 0) {
        npcEntity->Paused();
    }
    if (npcSteeringOld != 0) {
        npcSteeringOld->Paused();
    }
    if (npcSteering != 0) {
        npcSteering->Paused();
    }
    if (npcPerception != 0) {
        npcPerception->Paused();
    }
    if (npcCombat != 0) {
        npcCombat->Paused();
    }
    if (npcQuickTimeCombat != 0) {
        npcQuickTimeCombat->Paused();
    }
    if (npcFX != 0) {
        npcFX->Paused();
    }

    for (unsigned int i = 0; i < 2; i++) {
        if (npcExtraModels[i] != 0) {
            npcExtraModels[i]->Paused();
        }
    }
}

void zNPCBase::ResumeAllComponents() {
    if (npcLogic != 0) {
        npcLogic->Resumed();
    }
    if (npcEntity != 0) {
        npcEntity->Resumed();
    }
    if (npcSteeringOld != 0) {
        npcSteeringOld->Resumed();
    }
    if (npcSteering != 0) {
        npcSteering->Resumed();
    }
    if (npcPerception != 0) {
        npcPerception->Resumed();
    }
    if (npcCombat != 0) {
        npcCombat->Resumed();
    }
    if (npcQuickTimeCombat != 0) {
        npcQuickTimeCombat->Resumed();
    }
    if (npcFX != 0) {
        npcFX->Resumed();
    }

    for (unsigned int i = 0; i < 2; i++) {
        if (npcExtraModels[i] != 0) {
            npcExtraModels[i]->Resumed();
        }
    }
}

// The first component that handles the event stops it.
bool zNPCBase::ForwardEventToComponents(xBase* from, xBase* to,
                                        unsigned int to_event,
                                        Sext::EventAny* params) {
    if (npcLogic != 0 && npcLogic->SystemEvent(from, to, to_event, params)) {
        return true;
    }

    if (npcEntity != 0 && npcEntity->SystemEvent(from, to, to_event, params)) {
        return true;
    }

    if (npcSteeringOld != 0 &&
        npcSteeringOld->SystemEvent(from, to, to_event, params)) {
        return true;
    }

    if (npcSteering != 0 &&
        npcSteering->SystemEvent(from, to, to_event, params)) {
        return true;
    }

    if (npcPerception != 0 &&
        npcPerception->SystemEvent(from, to, to_event, params)) {
        return true;
    }

    if (npcCombat != 0 && npcCombat->SystemEvent(from, to, to_event, params)) {
        return true;
    }

    if (npcQuickTimeCombat != 0 &&
        npcQuickTimeCombat->SystemEvent(from, to, to_event, params)) {
        return true;
    }

    if (npcFX != 0 && npcFX->SystemEvent(from, to, to_event, params)) {
        return true;
    }

    return false;
}

void zNPCBase::DetachAllComponents(zNPCStatus* npcStatus) {
    if (npcCombat != 0) {
        DetachComponent(*npcCombat, npcStatus);
        zNPCManager::factory.DeallocMem(npcCombat);
        npcCombat = 0;
    }

    if (npcQuickTimeCombat != 0) {
        DetachComponent(*npcQuickTimeCombat, npcStatus);
        zNPCManager::factory.DeallocMem(npcQuickTimeCombat);
        npcQuickTimeCombat = 0;
    }

    if (npcPerception != 0) {
        DetachComponent(*npcPerception, npcStatus);
        zNPCManager::factory.DeallocMem(npcPerception);
        npcPerception = 0;
    }

    if (npcSteeringOld != 0) {
        DetachComponent(*npcSteeringOld, npcStatus);
        zNPCManager::factory.DeallocMem(npcSteeringOld);
        npcSteeringOld = 0;
    }

    if (npcSteering != 0) {
        DetachComponent(*npcSteering, npcStatus);
        zNPCManager::factory.DeallocMem(npcSteering);
        npcSteering = 0;
    }

    if (npcEntity != 0) {
        DetachComponent(*npcEntity, npcStatus);
        zNPCManager::factory.DeallocMem(npcEntity);
        npcEntity = 0;
    }

    if (npcLogic != 0) {
        DetachComponent(*npcLogic, npcStatus);
        zNPCManager::factory.DeallocMem(npcLogic);
        npcLogic = 0;
    }

    if (npcFX != 0) {
        DetachComponent(*npcFX, npcStatus);
        zNPCManager::factory.DeallocMem(npcFX);
        npcFX = 0;
    }

    for (unsigned int i = 0; i < 2; i++) {
        if (npcExtraModels[i] != 0) {
            DetachComponent(*npcExtraModels[i], npcStatus);
            zNPCManager::factory.DeallocMem(npcExtraModels[i]);
            npcExtraModels[i] = 0;
        }
    }
}

void zNPCBase::ResetAllComponents(const zNPCStatus* npcStatus) {
    if (npcLogic != 0) {
        npcLogic->Reset(npcStatus);
    }
    if (npcEntity != 0) {
        npcEntity->Reset(npcStatus);
    }
    if (npcSteeringOld != 0) {
        npcSteeringOld->Reset(npcStatus);
    }
    if (npcSteering != 0) {
        npcSteering->Reset(npcStatus);
    }
    if (npcPerception != 0) {
        npcPerception->Reset(npcStatus);
    }
    if (npcCombat != 0) {
        npcCombat->Reset(npcStatus);
    }
    if (npcQuickTimeCombat != 0) {
        npcQuickTimeCombat->Reset(npcStatus);
    }
    if (npcFX != 0) {
        npcFX->Reset(npcStatus);
    }

    for (unsigned int i = 0; i < 2; i++) {
        if (npcExtraModels[i] != 0) {
            npcExtraModels[i]->Reset(npcStatus);
        }
    }
}

// ---------------------------------------------------------------------------
// Info nodes

zNPCInfoNode* zNPCBase::GetInfoNode(eNPCInfoNodeType infoNodeType) {
    zNPCInfoNode* curInfoNode = npcInfoNodesFront;
    while (curInfoNode != 0) {
        if (infoNodeType == curInfoNode->infoNodeType) {
            return curInfoNode;
        }
        curInfoNode = curInfoNode->next;
    }

    return 0;
}

void zNPCBase::UpdateInfoNodes(float dt) {
    infoNodesUpdating = true;

    zNPCInfoNode* curInfoNode = npcInfoNodesFront;
    while (curInfoNode != 0) {
        curInfoNode->Update(dt);
        curInfoNode = curInfoNode->next;
    }

    infoNodesUpdating = false;
}

void zNPCBase::AttachInfoNode(zNPCInfoNode* infoNode,
                              eNPCInfoNodeType infoNodeType) {
    infoNode->next = npcInfoNodesFront;
    npcInfoNodesFront = infoNode;

    infoNode->owner = this;
    infoNode->infoNodeType = infoNodeType;
    infoNode->Attached();
}

void zNPCBase::DetachInfoNode(eNPCInfoNodeType infoNodeType) {
    zNPCInfoNode* prevInfoNode = 0;
    zNPCInfoNode* curInfoNode = npcInfoNodesFront;
    while (curInfoNode != 0) {
        if (infoNodeType == curInfoNode->infoNodeType) {
            if (prevInfoNode == 0) {
                npcInfoNodesFront = curInfoNode->next;
            } else {
                prevInfoNode->next = curInfoNode->next;
            }

            curInfoNode->Detached();
            zNPCManager::factory.DeallocMem(curInfoNode);
            return;
        }

        prevInfoNode = curInfoNode;
        curInfoNode = curInfoNode->next;
    }
}

void zNPCBase::DetachAllInfoNodes() {
    zNPCInfoNode* curInfoNode = npcInfoNodesFront;
    while (curInfoNode != 0) {
        zNPCInfoNode* takeThisNodeOut = curInfoNode;
        curInfoNode = curInfoNode->next;

        takeThisNodeOut->Detached();
        zNPCManager::factory.DeallocMem(takeThisNodeOut);
    }

    npcInfoNodesFront = 0;
}

// ---------------------------------------------------------------------------
// Animation, position, life and death

void zNPCBase::SetAnimState(char* playAnim) {
    zNPCEntity* ent = npcEntity;

    ent->SetAnimState(xStrHash(playAnim), 0.3f, playAnim);
}

void zNPCBase::GetPosition(xVec3& pos) {
    if (npcEntity != 0) {
        pos = npcEntity->model->Mat.pos;
    } else {
        pos = npcStatus.lastPos;
    }
}

void zNPCBase::Kill(bool quiet) {
    if (alive) {
        alive = false;
        quietKill |= quiet;
        if (!updating && !quietKill) {
            zEntEvent(0, 0, npcEntity, 0xBB6E2C0D, 0, FE_NO);
        }
    } else {
        quietKill |= quiet;
    }
}

void zNPCBase::Spawn() {
    alive = present = true;
    if (!activated) {
        if (!BaseActivate()) {
            BaseDeactivate();
        }
    }

    if (npcEntity != 0) {
        npcEntity->ResetDamageColor();
    }
}

// Is the NPC (or the move point it jumps in from) inside its wall net?
bool zNPCBase::CheckWallNetValidity(bool noWallNet) {
    if (npcAsset == 0) {
        return false;
    }

    if (npcAsset->WallNet.internalUid == 0) {
        return !noWallNet;
    }

    xBase* b = zSceneFindObject(npcAsset->WallNet.internalUid);
    if (b == 0) {
        return false;
    }

    if (b->baseType != 0x62) {
        return false;
    }

    zWallNet* npcWallNet = (zWallNet*)b;

    xMovePoint* jumpSpawnMP = 0;
    if (npcAsset->SpawnType == 2) {
        if (npcAsset->SpawnJumpToMP.internalUid != 0) {
            jumpSpawnMP =
                (xMovePoint*)zSceneFindObject(npcAsset->SpawnJumpToMP.internalUid);
        }
    }

    if (jumpSpawnMP != 0) {
        if (!npcWallNet->IsInsideWallNetXZ(*jumpSpawnMP->pos)) {
            return false;
        }
    } else {
        xVec3 npcPos;

        GetPosition(npcPos);

        if (npcWallNet->wallNetAsset->numBounds == 0) {
            return false;
        }

        if (!npcWallNet->IsInsideWallNetXZ(npcPos)) {
            return false;
        }
    }

    return true;
}

bool zNPCBase::CheckIfModelHasCollisionModel(
    World::ModelInstanceAsset* modelInstanceAsset) {
    ModelPrototypeEntity* proto =
        xOGRenderHelper::GetModelPrototypeEntity(modelInstanceAsset);
    if (proto == 0) {
        return false;
    }

    return proto->collmeshBlob != 0;
}

// ---------------------------------------------------------------------------
// Damage

void zNPCBase::GiveNPCDamage(xEnt* npcEnt, const zCombatDamageInfo& damageInfo) {
    zNPCGetsDamageInfo npcDamageInfo;

    if (HandleNPCDamage(npcEnt, damageInfo, &npcDamageInfo)) {
        NPCGetsDamage(npcDamageInfo);

        zNPCCombat* combat = npcCombat;
        if (combat != 0 && combat->IsDead()) {
            if (npcDamageInfo.from != 0 && npcDamageInfo.from->baseType == 0x55) {
                zEntEvent(npcDamageInfo.from, 0, npcEnt, 0x3690C08D, 0, FE_NO);
            }

            if (damageInfo.from != 0 && damageInfo.from->baseType == 0x55) {
                zPlayer* player = (zPlayer*)damageInfo.from;

                if (player->eName == 6) {
                    ((zSBPlayer*)player)->IncreaseBuffKillCounter();
                }
            }

            Kill(false);
        }
    }
}

bool zNPCBase::HandleNPCDamage(xEnt* npcEnt, const zCombatDamageInfo& damageInfo,
                               zNPCGetsDamageInfo* npcDamageInfo) {
    zNPCCombat* combat = npcCombat;
    if (combat != 0) {
        return combat->HandleNPCDamage(npcEnt, damageInfo, npcDamageInfo);
    }

    npcDamageInfo->SetFromCombatDamageInfo(npcEnt, damageInfo,
                                           (eNPCHitReaction)0, 0.0f);
    return true;
}

void zNPCBase::NPCGetsDamage(const zNPCGetsDamageInfo& info) {
    zNPCLogic* logic = npcLogic;
    if (logic != 0) {
        logic->NPCGetsDamage(info);
    }
}

void zNPCBase::NPCGivesDamage(xEnt* toEnt, xEnt* npcEnt,
                              const zCombatDamageInfo& damageInfo) {
    zNPCGivesDamageInfo npcDamageInfo;

    npcDamageInfo.SetFromCombatDamageInfo(toEnt, npcEnt, damageInfo);
    NPCGivesDamage(npcDamageInfo);
}

void zNPCBase::NPCGivesDamage(const zNPCGivesDamageInfo& info) {
    zNPCLogic* logic = npcLogic;
    if (logic != 0) {
        logic->NPCGivesDamage(info);
    }
}

xVec3* zNPCBase::GetPositionSound() const {
    return npcEntity != 0 ? &npcEntity->model->Mat.pos
                          : (xVec3*)&npcStatus.lastPos;
}

void zNPCBase::UpdateRender() {
    if (npcEntity != 0) {
        npcEntity->UpdateRender();
    }
}

// The closest player inside the NPC's wall net, in XZ or in full.
zPlayer* zNPCBase::GetClosestPlayerOnWallnet(bool ignoreHeight) {
    zWallNet* wallnet = npcSteering->wallNet;
    if (wallnet == 0) {
        return 0;
    }

    float minDist2 = 3.4028235e+38f;
    zPlayer* player = 0;
    for (int i = 0; i < xglobals->players.numPlayers; i++) {
        if ((ignoreHeight && wallnet->IsInsideWallNetXZ(
                                 xglobals->players.playerArray[i]->model->Mat.pos)) ||
            (!ignoreHeight && wallnet->IsInsideWallNet(
                                  xglobals->players.playerArray[i]->model->Mat.pos))) {
            float dist2 = xVec3Dist2(&npcEntity->model->Mat.pos,
                                     &xglobals->players.playerArray[i]->model->Mat.pos);
            if (dist2 < minDist2) {
                minDist2 = dist2;
                player = xglobals->players.playerArray[i];
            }
        }
    }

    return player;
}

// ---------------------------------------------------------------------------
// The extra model's pause flag, which retail emits here

class zNPCExtraModel {
public:
    void Paused();
    void Resumed();

    unsigned char _pad0[0xAB];
    bool paused;
};

void zNPCExtraModel::Paused() { paused = true; }
void zNPCExtraModel::Resumed() { paused = false; }
