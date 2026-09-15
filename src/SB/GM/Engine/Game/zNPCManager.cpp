#include "SB/GM/Engine/Game/zNPCManager.pool.h"

// zNPCManager.cpp -- the NPC manager module: the type registry's scene
// hooks, creating NPCs and NPC groups from their assets, the per-frame
// update through each NPC's LOD record, the sphere and cone searches, and
// the animation tables kept per animation set. Read from the image with
// tools/brief.py; the layouts are the DWARF's (zNPCManager 0x1C on a 0x18
// zModule, animTables at +0x18).
//
// zNPCType keeps its vtable pointer at +0x38 behind its data, zNPCGroupType
// at +0xC. The registry's accessors and zScene::SubTypeCount are header
// inlines retail emits here.

class xAnimTable;
class xBase {};
class zCharacterAsset;
class zNPCBase;
class zNPCGroupBase;
class CreatorI;

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

enum enModulePriority {
    MODULE_PRIORITY_EARLY = 0,
    MODULE_PRIORITY_NORMAL = 1,
    MODULE_PRIORITY_LATE = 2
};

enum eNPCType {
    eNPCType_Unknown = -1,
    eNPCTypeCount = 28,
    eNPCTypeForceInt = 0x7FFFFFFF
};

enum eNPCGroupType {
    eNPCGroupType_Unknown = -1,
    eNPCGroupTypeCount = 2,
    eNPCGroupTypeForceInt = 0x7FFFFFFF
};

namespace Memory {

enum GlobalHeapEnum { GlobalHeap = 0 };

void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap, eMemMgrTag tag,
                      bool clear);

class Factory {
public:
    void Initialize(void* mem, unsigned int size, unsigned int pageSize,
                    const unsigned int* sizes);
    void Deinitialize();
};

}  // namespace Memory

void* xMemAlloc(Memory::GlobalHeapEnum heap, unsigned int size, int align,
                eMemMgrTag tag);

namespace Util {
extern unsigned int g_rttidParentTable[];
}  // namespace Util

namespace World {

class EntityHandleBase;

class EntityManager {
public:
    static void* FindAsset(unsigned long long id);
};

EntityManager* GetEntityManager();

}  // namespace World

namespace Sext {

class xBaseAsset {
public:
    unsigned long long id;
    unsigned int baseType;
    unsigned short linkCount;
    unsigned short baseFlags;
};

class NPCAsset : public xBaseAsset {};
class NPCGroupAsset : public xBaseAsset {};

class AnimationSet {
public:
    enum eAnimSetType { eAnimSetType_ = 0x7FFFFFFF };
};

}  // namespace Sext

void zNPCAsset_Prepare(Sext::NPCAsset* asset, zCharacterAsset* charAsset);


class xVec3 {
public:
    xVec3& operator=(const xVec3& other);
    float length2() const;
    float NormalizeSafe();

    float x;
    float y;
    float z;
};

xVec3 operator-(const xVec3& a, const xVec3& b);

class hkVector4 {
public:
    float dot3(const hkVector4& other) const;
};

extern "C" double acos(double x);

class xAnimSingle;
class xAnimTransition;
class xAnimPlay;
class xAnimState;
class xQuat;

xAnimTable* xAnimTableNew(const char* name, unsigned int a, void* owner,
                          xAnimTable** list);
unsigned int xAnimTableNewState(xAnimTable* table, const char* name,
                                unsigned int a, unsigned int b, float c,
                                float* d, float* e, float f,
                                unsigned short* g, void* h,
                                void (*i)(xAnimPlay*, xAnimState*, void*),
                                void (*j)(xAnimPlay*, xAnimState*, void*),
                                void (*k)(xAnimState*, xAnimSingle*, void*),
                                void (*l)(xAnimPlay*, xQuat*, xVec3*,
                                          xVec3*, int),
                                unsigned long long m, unsigned int n);
unsigned int xAnimTableNewTransition(
    xAnimTable* table, const char* from, const char* to,
    unsigned int (*a)(xAnimTransition*, xAnimSingle*, void*),
    unsigned int (*b)(xAnimTransition*, xAnimSingle*, void*),
    unsigned int (*c)(xAnimTransition*, xAnimSingle*, void*),
    unsigned int d, unsigned int e, float f, float g, unsigned short h,
    unsigned short i, float j, unsigned short* k);

// The transition's third callback answers 0; the linker folded it into
// this function, which is the name retail's relocation carries.
namespace World {
class ShaderCodeBlobAsset {
public:
    static void* Create(EntityHandleBase* handle, ShaderCodeBlobAsset* asset);
};
}  // namespace World

typedef unsigned int (*xAnimTransitionCB)(xAnimTransition*, xAnimSingle*, void*);

class zCompLogicAnimViewer {
public:
    static unsigned long long FindCharacterAssetID();
};

// ---------------------------------------------------------------------------
// The scene: a list of entities per RTTI type

class EmbeddedListNode {
public:
    EmbeddedListNode* next;
    EmbeddedListNode* prev;
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

#pragma always_inline on

class zScene {
public:
    // The entities of a type and of every type derived from it. Defined at
    // the end of the file, below its caller: retail calls it.
    int SubTypeCount(unsigned int subType);

    // Calls T::Callback on every entity of a type and of its subtypes.
    template <class T, class D>
    void ForAllBasePtr(unsigned int subType, D* data) {
        for (int i = 0; i < 264; i++) {
            unsigned int tid = i;
            do {
                if (tid == subType) {
                    EmbeddedListNode* node;
                    EmbeddedListNode* end = &baseInfo[i].entz.head;
                    for (node = end->next; node != end; node = node->next) {
                        T::Callback((xBase*)((char*)node - 4), data);
                    }
                    break;
                }
                tid = Util::g_rttidParentTable[tid];
            } while (tid != 0);
        }
    }

    template <class T>
    void ForAllBase(unsigned int subType, void* context) {
        for (int i = 0; i < 264; i++) {
            unsigned int tid = i;
            do {
                if (tid == subType) {
                    EmbeddedListNode* node;
                    EmbeddedListNode* end = &baseInfo[i].entz.head;
                    for (node = end->next; node != end; node = node->next) {
                        T::Callback((xBase*)((char*)node - 4), context);
                    }
                    break;
                }
                tid = Util::g_rttidParentTable[tid];
            } while (tid != 0);
        }
    }

    unsigned char _pad0[0x28];
    BaseInfo baseInfo[264];
};

class xGlobals {
public:
    unsigned char _pad0[0x43C];
    zScene* sceneCur;
};

class zGlobals : public xGlobals {};

extern zGlobals globals;

// ---------------------------------------------------------------------------
// NPCs and groups

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
    virtual void _v20();
    virtual void Load(const class zNPCType* npcType, Sext::NPCAsset* npcAsset,
                      unsigned int count);
    virtual void _v22();
    virtual void _v23();
    virtual void Setup();
    virtual void _v25();
    virtual void _v26();
    virtual void _v27();
    virtual void _v28();
    virtual void _v29();
    virtual void _v30();
    virtual void _v31();
    virtual void _v32();
    virtual void _v33();
    virtual void _v34();
    virtual void _v35();
    virtual void _v36();
    virtual void _v37();
    virtual void _v38();
    virtual void _v39();
    virtual void _v40();
    virtual void _v41();
    virtual void _v42();
    virtual void _v43();
    virtual void Paused();
    virtual void Resumed();
};

class zNPCStatus;

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
                             void* params);
};

class zNPCCombat : public zNPCComponent {
public:
    virtual void _k10();
    virtual bool IsDead() const;
};

class xOGModel {
public:
    unsigned char _pad0[0x30];
    xVec3 pos;
};

class zNPCEntity {
public:
    unsigned char _pad0[0x34];
    xOGModel* model;
};

// The result CheckLOD returns by value: four bytes, back in r3.
class zNPCUpdateLODResult {
public:
    unsigned char result;
    bool callDeactivate : 1;
    bool callActivate : 1;
    bool callPause : 1;
    bool callResume : 1;
    bool callUpdate : 1;
    unsigned char pad1 : 3;
    unsigned short pad2;
};

class zNPCUpdateLOD {
public:
    void Reset(zNPCBase* npcBase, const Sext::NPCAsset* npcAsset);
    zNPCUpdateLODResult CheckLOD();

    int lodType;
    void* lodCylinder;
    zNPCBase* npcBase;
};

class zNPCBase : public zNPCBaseVirtuals {
public:
    void BaseInitialize();
    void BaseUninitialize();
    void BaseDeactivate();
    void BaseReset();
    bool BaseActivate();
    void BaseUpdate(float dt);
    void PauseAllComponents();
    void ResumeAllComponents();

    unsigned char _pad0[0x60 - 0x4];
    Sext::NPCAsset* npcAsset;
    unsigned char _pad1[0x6C - 0x64];
    zNPCGroupBase* parentGroup;
    bool puppetMode : 1;
    bool alive : 1;
    bool present : 1;
    bool activated : 1;
    bool spawned : 1;
    bool paused : 1;
    bool updateInCinematicAlways : 1;
    bool updateInCinematicNever : 1;
    unsigned char _pad2[0x88 - 0x71];
    zNPCUpdateLOD npcUpdateLOD;
    unsigned char _pad3[0x98 - 0x94];
    zNPCEntity* npcEntity;
    unsigned char _pad4[0xA8 - 0x9C];
    zNPCCombat* npcCombat;
};

class zNPCGroupVirtuals {
public:
    virtual void _g0();
    virtual void _g1();
    virtual void _g2();
    virtual void _g3();
    virtual void _g4();
    virtual void _g5();
    virtual void _g6();
    virtual void Load(const class zNPCGroupType* npcGroupType,
                      Sext::NPCGroupAsset* npcGroupAsset, unsigned int count);
    virtual void _g8();
    virtual void Reset();
    virtual void Initialize();
    virtual void Uninitialize();
    virtual void Setup();
    virtual void Update(float dt);
};

class zNPCGroupBase : public zNPCGroupVirtuals {
public:
    unsigned char _pad0[0x3C - 0x4];
    zNPCBase** npcs;
    unsigned int npcsNum;
};

// ---------------------------------------------------------------------------
// The registry

class zNPCType {
public:
    eNPCType npcTypeEnum;
    Sext::AnimationSet::eAnimSetType animSetType;
    char* typeName;
    zNPCBase* (*allocateNPCFunction)(World::EntityHandleBase* handle);
    unsigned int npcTID_ID;
    bool hasEntity;
    CreatorI* logicCreator;
    CreatorI* steeringCreator;
    CreatorI* perceptionCreator;
    CreatorI* combatCreator;
    CreatorI* quickTimeCombatCreator;
    CreatorI* fxCreator;
    CreatorI* extraModelCreator[2];

    virtual void _t0();
    virtual void _t1();
    virtual void CreateAnimTable(xAnimTable* table);
    virtual void ModulePrepUse();
    virtual void ScenePrepare();
    virtual void SceneFinish();
    virtual void SceneInit();
    virtual void ScenePostInit();
    virtual void SceneSetup();
    virtual void ScenePostSetup();
    virtual void SceneReset();
    virtual void SceneExit();
};

class zNPCGroupType {
public:
    eNPCGroupType npcGroupTypeEnum;
    zNPCGroupBase* (*allocateNPCGroupFunction)(World::EntityHandleBase* handle);
    unsigned int npcGroupTID_ID;

    virtual void _t0();
    virtual void ModulePrepUse();
    virtual void ScenePrepare();
    virtual void SceneFinish();
    virtual void SceneInit();
    virtual void ScenePostInit();
    virtual void SceneSetup();
    virtual void ScenePostSetup();
    virtual void SceneReset();
    virtual void SceneExit();
};

class zNPCRegistry {
public:
    // Defined at the end of the file: retail calls them, never inlines them.
    static zNPCType* GetNPCType(unsigned int index);
    static zNPCGroupType* GetNPCGroupType(unsigned int index);

    // The type whose TID the asset's base type names, or the default.
    static zNPCType* FindNPCType(const Sext::xBaseAsset* asset) {
        for (unsigned int i = 0; i < eNPCTypeCount; i++) {
            if (asset->baseType == npcTypes[i]->npcTID_ID) {
                return npcTypes[i];
            }
        }
        return &npcDefaultType;
    }

    static zNPCGroupType* FindNPCGroupType(const Sext::xBaseAsset* asset) {
        for (unsigned int i = 0; i < eNPCGroupTypeCount; i++) {
            if (asset->baseType == npcGroupTypes[i]->npcGroupTID_ID) {
                return npcGroupTypes[i];
            }
        }
        return &npcGroupDefaultType;
    }

    static zNPCType* npcTypes[eNPCTypeCount];
    static zNPCType npcDefaultType;
    static zNPCGroupType* npcGroupTypes[eNPCGroupTypeCount];
    static zNPCGroupType npcGroupDefaultType;
};

// ---------------------------------------------------------------------------
// The manager

// Five words, then the vtable pointer at +0x14.
class zModule {
public:
    unsigned int flg_skipUpdates : 1;
    unsigned int flg_skipRenders : 1;
    unsigned int flg_useBucketRender : 1;
    unsigned int flg_useLayerRender : 1;
    unsigned int flg_updateWhenPaused : 1;
    unsigned int flg_updateInCinematic : 1;
    unsigned int flg_notUsed : 26;

    int tag_module;
    char* nam_module;
    enModulePriority updatePriority;
    enModulePriority renderPriority;

    virtual void ModuleSetup();
};

class zAnimTableUID;

class xAnimTable {
public:
    xAnimTable* Next;
    unsigned char _pad0[0x1C - 0x4];
    void* UserContext;
};

template <class T, int N>
class fixed_stack_list {
public:
    class node_type;

    class empty_node_type {
    public:
        node_type* prev;
        node_type* next;
    };

    class node_type : public empty_node_type {
    public:
        T value;
    };

    // Defined at the end of the file: retail calls both.
    void reset();
    void push_back(const T& value);

    unsigned long _size;
    empty_node_type head;
    empty_node_type tail;
    node_type* stack;
    node_type buffer[N];
};

class zNPCManager_WithinSphereCheck_CB {
public:
    fixed_stack_list<zNPCBase*, 32>* npcList;
    xVec3* checkPos;
    float radius;
};

class zNPCManager_WithinConeCheck_CB {
public:
    fixed_stack_list<zNPCBase*, 32>* npcList;
    xVec3* startPos;
    xVec3* dirNormalized;
    float coneLength;
    float coneAngle;
};

class zNPCManager : public zModule {
public:
    static zNPCManager* Manager();

    void ModulePrepUse();
    void ScenePrepare();
    void SceneFinish();
    void SceneInit();
    void ScenePostInit();
    void SceneSetup();
    void ScenePostSetup();
    void SceneReset();
    void SceneExit();
    int _FindNumberOfBases();
    static zNPCBase* CreateNPC(World::EntityHandleBase* handle,
                               Sext::NPCAsset* npcAsset,
                               const unsigned int dynAssetSize);
    static zNPCGroupBase* CreateNPCGroup(World::EntityHandleBase* handle,
                                         Sext::NPCGroupAsset* npcGroupAsset,
                                         const unsigned int dynAssetSize);
    void _LoadNPC(zNPCBase* npc, zNPCType* type,
                  const Sext::xBaseAsset* asset,
                  const unsigned int dynAssetSize);
    xAnimTable* _CreateAnimTable(Sext::AnimationSet::eAnimSetType animSetType,
                                 unsigned long long animSetID);
    void _DestroyAnimTable(Sext::AnimationSet::eAnimSetType animSetType,
                           unsigned long long animSetID);
    void _AddAnimTable(xAnimTable* table, unsigned long long animSetID);
    xAnimTable* _GetAnimTable(unsigned long long animSetID);
    bool _RemoveAnimTable(unsigned long long animSetID);
    void _GetAllNPCsWithinSphereByType(eNPCType type, xVec3* pos, float radius,
                                       fixed_stack_list<zNPCBase*, 32>* list);
    void _GetAllNPCsWithinConeByType(eNPCType type, xVec3* startPos, xVec3* dir,
                                     float angle,
                                     fixed_stack_list<zNPCBase*, 32>* outputList);
    void Timestep(float dt);

    static Memory::Factory factory;

    xAnimTable* animTables;
};

extern zNPCManager gNPCManager;

zNPCManager* zNPCManager::Manager() { return &gNPCManager; }

void zNPCManager::ModulePrepUse() {
    flg_updateWhenPaused = 0;
    flg_skipRenders = 1;
    flg_updateInCinematic = 1;

    for (unsigned int i = 0; i < eNPCTypeCount; i++) {
        zNPCRegistry::GetNPCType(i)->ModulePrepUse();
    }

    for (unsigned int i = 0; i < eNPCGroupTypeCount; i++) {
        zNPCRegistry::GetNPCGroupType(i)->ModulePrepUse();
    }

    animTables = 0;
}

void zNPCManager::ScenePrepare() {
    factory.Initialize(xMemAlloc(Memory::GlobalHeap, 0x80000, 0, (eMemMgrTag)15),
                       0x80000, 0x2000, 0);

    for (unsigned int i = 0; i < eNPCTypeCount; i++) {
        zNPCRegistry::GetNPCType(i)->ScenePrepare();
    }

    for (unsigned int i = 0; i < eNPCGroupTypeCount; i++) {
        zNPCRegistry::GetNPCGroupType(i)->ScenePrepare();
    }
}

void zNPCManager::SceneFinish() {
    for (unsigned int i = 0; i < eNPCTypeCount; i++) {
        zNPCRegistry::GetNPCType(i)->SceneFinish();
    }

    for (unsigned int i = 0; i < eNPCGroupTypeCount; i++) {
        zNPCRegistry::GetNPCGroupType(i)->SceneFinish();
    }
}

void zNPCManager::SceneInit() {
    for (unsigned int i = 0; i < eNPCTypeCount; i++) {
        zNPCRegistry::GetNPCType(i)->SceneInit();
    }

    for (unsigned int i = 0; i < eNPCGroupTypeCount; i++) {
        zNPCRegistry::GetNPCGroupType(i)->SceneInit();
    }
}

void zNPCManager::ScenePostInit() {
    for (unsigned int i = 0; i < eNPCTypeCount; i++) {
        zNPCRegistry::GetNPCType(i)->ScenePostInit();
    }

    for (unsigned int i = 0; i < eNPCGroupTypeCount; i++) {
        zNPCRegistry::GetNPCGroupType(i)->ScenePostInit();
    }
}


struct zNPCManager_SetupGroupCB {
    static void Callback(xBase* base, void* context) {
        zNPCGroupBase* group = (zNPCGroupBase*)base;

        group->Setup();
        for (unsigned int i = 0; i < group->npcsNum; i++) {
            group->npcs[i]->parentGroup = group;
        }
    }
};

struct zNPCManager_SetupNPCCB {
    static void Callback(xBase* base, void* context) {
        ((zNPCBase*)base)->Setup();
    }
};

void zNPCManager::SceneSetup() {
    globals.sceneCur->ForAllBase<zNPCManager_SetupGroupCB>(0x3A, 0);

    for (unsigned int i = 0; i < eNPCTypeCount; i++) {
        zNPCRegistry::GetNPCType(i)->SceneSetup();
    }

    for (unsigned int i = 0; i < eNPCGroupTypeCount; i++) {
        zNPCRegistry::GetNPCGroupType(i)->SceneSetup();
    }

    globals.sceneCur->ForAllBase<zNPCManager_SetupNPCCB>(0x39, 0);
}

void zNPCManager::ScenePostSetup() {
    for (unsigned int i = 0; i < eNPCTypeCount; i++) {
        zNPCRegistry::GetNPCType(i)->ScenePostSetup();
    }

    for (unsigned int i = 0; i < eNPCGroupTypeCount; i++) {
        zNPCRegistry::GetNPCGroupType(i)->ScenePostSetup();
    }
}

struct zNPCManager_ResetNPCCB {
    static void Callback(xBase* base, void* context) {
        zNPCBase* npc = (zNPCBase*)base;

        npc->BaseDeactivate();
        npc->BaseReset();
        npc->npcUpdateLOD.Reset(npc, npc->npcAsset);
    }
};

struct zNPCManager_ResetGroupCB {
    static void Callback(xBase* base, void* context) {
        ((zNPCGroupBase*)base)->Reset();
    }
};

struct zNPCManager_DeactivateNPCCB {
    static void Callback(xBase* base, void* context) {
        ((zNPCBase*)base)->BaseDeactivate();
    }
};

struct zNPCManager_UninitializeNPCCB {
    static void Callback(xBase* base, void* context) {
        ((zNPCBase*)base)->BaseUninitialize();
    }
};

struct zNPCManager_UninitializeGroupCB {
    static void Callback(xBase* base, void* context) {
        ((zNPCGroupBase*)base)->Uninitialize();
    }
};

void zNPCManager::SceneReset() {
    globals.sceneCur->ForAllBase<zNPCManager_ResetNPCCB>(0x39, 0);
    globals.sceneCur->ForAllBase<zNPCManager_ResetGroupCB>(0x3A, 0);

    for (unsigned int i = 0; i < eNPCTypeCount; i++) {
        zNPCRegistry::GetNPCType(i)->SceneReset();
    }

    for (unsigned int i = 0; i < eNPCGroupTypeCount; i++) {
        zNPCRegistry::GetNPCGroupType(i)->SceneReset();
    }
}

void zNPCManager::SceneExit() {
    globals.sceneCur->ForAllBase<zNPCManager_DeactivateNPCCB>(0x39, 0);
    globals.sceneCur->ForAllBase<zNPCManager_UninitializeNPCCB>(0x39, 0);
    globals.sceneCur->ForAllBase<zNPCManager_UninitializeGroupCB>(0x3A, 0);

    for (unsigned int i = 0; i < eNPCTypeCount; i++) {
        zNPCRegistry::GetNPCType(i)->SceneExit();
    }

    for (unsigned int i = 0; i < eNPCGroupTypeCount; i++) {
        zNPCRegistry::GetNPCGroupType(i)->SceneExit();
    }

    factory.Deinitialize();
}

int zNPCManager::_FindNumberOfBases() {
    return globals.sceneCur->SubTypeCount(0x39) +
           globals.sceneCur->SubTypeCount(0x3A);
}

#pragma push
#pragma always_inline on
zNPCBase* zNPCManager::CreateNPC(World::EntityHandleBase* handle,
                                 Sext::NPCAsset* npcAsset,
                                 const unsigned int dynAssetSize) {
    zNPCType* npcType = zNPCRegistry::FindNPCType(npcAsset);

    zNPCBase* npc = npcType->allocateNPCFunction(handle);

    gNPCManager._LoadNPC(npc, npcType, npcAsset, dynAssetSize);

    npc->BaseInitialize();

    return npc;
}

zNPCGroupBase* zNPCManager::CreateNPCGroup(World::EntityHandleBase* handle,
                                           Sext::NPCGroupAsset* npcGroupAsset,
                                           const unsigned int dynAssetSize) {
    zNPCGroupType* npcGroupType = zNPCRegistry::FindNPCGroupType(npcGroupAsset);

    zNPCGroupBase* npcGroup = npcGroupType->allocateNPCGroupFunction(handle);

    npcGroup->Load(npcGroupType, npcGroupAsset, dynAssetSize);

    npcGroup->Initialize();

    return npcGroup;
}
#pragma pop

void zNPCManager::_LoadNPC(zNPCBase* npc, zNPCType* type,
                           const Sext::xBaseAsset* asset,
                           const unsigned int dynAssetSize) {
    Sext::NPCAsset* npcAsset = (Sext::NPCAsset*)asset;

    if (npcAsset->baseType == 0xEE) {
        unsigned long long characterAssetsID =
            zCompLogicAnimViewer::FindCharacterAssetID();
        zNPCAsset_Prepare(npcAsset,
                          (zCharacterAsset*)World::GetEntityManager()->FindAsset(
                              characterAssetsID));
    } else {
        zNPCAsset_Prepare(npcAsset, 0);
    }

    npc->Load(type, npcAsset, dynAssetSize);
}

// Every live NPC of a type within the radius of a point.
struct zNPCManager_WithinSphereCheck {
    static void Callback(xBase* base, zNPCManager_WithinSphereCheck_CB* cb) {
        zNPCBase* npc = (zNPCBase*)base;

        if (npc != 0 && npc->npcCombat != 0 && !npc->npcCombat->IsDead()) {
            xVec3 npcPos;
            npcPos = npc->npcEntity->model->pos;

            xVec3 delta = npcPos - *cb->checkPos;
            if (delta.length2() <= cb->radius * cb->radius) {
                cb->npcList->push_back(npc);
            }
        }
    }
};

void zNPCManager::_GetAllNPCsWithinSphereByType(
    eNPCType type, xVec3* pos, float radius, fixed_stack_list<zNPCBase*, 32>* list) {
    zNPCManager_WithinSphereCheck_CB sphereCB;
    sphereCB.npcList = list;
    sphereCB.checkPos = pos;
    sphereCB.radius = radius;

    sphereCB.npcList->reset();

    zNPCType* npcType = zNPCRegistry::GetNPCType(type);
    globals.sceneCur->ForAllBasePtr<zNPCManager_WithinSphereCheck>(
        npcType->npcTID_ID, &sphereCB);
}

// Every live NPC of a type within the cone: the distance along the axis
// and the angle off it, in degrees.
struct zNPCManager_WithinConeCheck {
    static void Callback(xBase* base, zNPCManager_WithinConeCheck_CB* cb) {
        zNPCBase* npc = (zNPCBase*)base;

        if (npc != 0 && npc->npcCombat != 0 && !npc->npcCombat->IsDead()) {
            xVec3 npcPos;
            npcPos = npc->npcEntity->model->pos;

            xVec3 delta = npcPos - *cb->startPos;
            if (delta.length2() <= cb->coneLength * cb->coneLength) {
                delta.NormalizeSafe();

                float cosAngle =
                    ((hkVector4&)delta).dot3((hkVector4&)*cb->dirNormalized);
                if (cosAngle > 1.0f) {
                    cosAngle = 1.0f;
                }
                if (cosAngle < -1.0f) {
                    cosAngle = -1.0f;
                }

                if (180.0f * (float)acos(cosAngle) / 3.1415927f <= cb->coneAngle) {
                    cb->npcList->push_back(npc);
                }
            }
        }
    }
};

void zNPCManager::_GetAllNPCsWithinConeByType(
    eNPCType type, xVec3* startPos, xVec3* dir, float angle,
    fixed_stack_list<zNPCBase*, 32>* outputList) {
    zNPCManager_WithinConeCheck_CB coneCB;
    coneCB.npcList = outputList;
    coneCB.startPos = startPos;
    xVec3 coneDir;
    coneDir.x = dir->x;
    coneDir.y = dir->y;
    coneDir.z = dir->z;
    coneCB.coneLength = coneDir.NormalizeSafe();
    coneCB.dirNormalized = &coneDir;
    coneCB.coneAngle = angle;

    outputList->reset();

    zNPCType* npcType = zNPCRegistry::GetNPCType(type);
    globals.sceneCur->ForAllBasePtr<zNPCManager_WithinConeCheck>(
        npcType->npcTID_ID, &coneCB);
}

xAnimTable* zNPCManager::_CreateAnimTable(
    Sext::AnimationSet::eAnimSetType animSetType, unsigned long long animSetID) {
    zNPCType* npcType;

    for (unsigned int i = 0; i < eNPCTypeCount; i++) {
        if (animSetType == zNPCRegistry::GetNPCType(i)->animSetType) {
            npcType = zNPCRegistry::GetNPCType(i);
            goto found;
        }
    }
    npcType = 0;
found:

    xAnimTable* table = xAnimTableNew("NPC", 0, 0, 0);

    if (npcType != 0) {
        npcType->CreateAnimTable(table);
    }

    xAnimTableNewState(table, "ERROR", 0, 0, 1.0f, 0, 0, 0.0f, 0, 0, 0, 0, 0,
                       0, 0, 0);
    xAnimTableNewTransition(table, "*", "ERROR", 0, 0,
                            (xAnimTransitionCB)World::ShaderCodeBlobAsset::Create,
                            16, 0, 0.0f, 0.0f, 0, 0, 0.1f, 0);

    _AddAnimTable(table, animSetID);

    return table;
}

void zNPCManager::_DestroyAnimTable(Sext::AnimationSet::eAnimSetType animSetType,
                                    unsigned long long animSetID) {
    _RemoveAnimTable(animSetID);
}

void zNPCManager::_AddAnimTable(xAnimTable* table, unsigned long long animSetID) {
    unsigned long long* id = (unsigned long long*)Memory::AllocGlobalHeap(
        sizeof(unsigned long long), Memory::GlobalHeap, (eMemMgrTag)7, false);
    *id = animSetID;
    table->UserContext = id;

    table->Next = animTables;
    animTables = table;
}

xAnimTable* zNPCManager::_GetAnimTable(unsigned long long animSetID) {
    xAnimTable* curTable = animTables;
    while (curTable != 0) {
        if (curTable->UserContext != 0 &&
            *(unsigned long long*)curTable->UserContext == animSetID) {
            return curTable;
        }
        curTable = curTable->Next;
    }

    return 0;
}

bool zNPCManager::_RemoveAnimTable(unsigned long long animSetID) {
    xAnimTable* prevTable = 0;
    xAnimTable* curTable = animTables;
    while (curTable != 0) {
        if (curTable->UserContext != 0 &&
            *(unsigned long long*)curTable->UserContext == animSetID) {
            delete (unsigned long long*)curTable->UserContext;
            curTable->UserContext = 0;

            if (prevTable != 0) {
                prevTable->Next = curTable->Next;
            } else {
                animTables = curTable->Next;
            }
            return true;
        }

        prevTable = curTable;
        curTable = curTable->Next;
    }

    return false;
}

struct zNPCManager_TimestepGroupCB {
    static void Callback(xBase* base, void* context) {
        ((zNPCGroupBase*)base)->Update(*(float*)context);
    }
};

// Each NPC runs what its LOD record says: an update, or the activation,
// resumption, update, pause and deactivation it asks for.
struct zNPCManager_TimestepNPCCB {
    static void Callback(xBase* base, void* context) {
        zNPCBase* npc = (zNPCBase*)base;
        zNPCUpdateLODResult lodResult = npc->npcUpdateLOD.CheckLOD();

        switch (lodResult.result) {
        case 0:
            npc->BaseUpdate(*(float*)context);
            break;
        case 2:
            if (lodResult.callActivate && !npc->activated) {
                if (!npc->BaseActivate()) {
                    npc->BaseDeactivate();
                    break;
                }
            }

            if (lodResult.callResume) {
                npc->Resumed();
                npc->ResumeAllComponents();
                npc->paused = false;
            }

            if (lodResult.callUpdate) {
                npc->BaseUpdate(*(float*)context);
            }

            if (lodResult.callPause) {
                npc->Paused();
                npc->PauseAllComponents();
                npc->paused = true;
            }

            if (lodResult.callDeactivate) {
                npc->BaseDeactivate();
            }
            break;
        }
    }
};

void zNPCManager::Timestep(float dt) {
    globals.sceneCur->ForAllBase<zNPCManager_TimestepGroupCB>(0x3A, &dt);
    globals.sceneCur->ForAllBase<zNPCManager_TimestepNPCCB>(0x39, &dt);
}

inline zNPCType* zNPCRegistry::GetNPCType(unsigned int index) {
    return npcTypes[index] != 0 ? npcTypes[index] : &npcDefaultType;
}

inline zNPCGroupType* zNPCRegistry::GetNPCGroupType(unsigned int index) {
    return npcGroupTypes[index] != 0 ? npcGroupTypes[index]
                                     : &npcGroupDefaultType;
}

inline int zScene::SubTypeCount(unsigned int subType) {
    int total = 0;
    for (int i = 0; i < 264; i++) {
        unsigned int tid = i;
        do {
            if (tid == subType) {
                total += baseInfo[i].entz.size;
                break;
            }
            tid = Util::g_rttidParentTable[tid];
        } while (tid != 0);
    }
    return total;
}

template <class T, int N>
void fixed_stack_list<T, N>::push_back(const T& value) {
    node_type* node = stack;

    stack = node->next;

    node->prev = tail.prev;
    node->next = (node_type*)&tail;
    tail.prev = node;
    node->prev->next = node;

    _size++;

    T& slot = tail.prev->value;
    slot = value;
}

template <class T, int N>
void fixed_stack_list<T, N>::reset() {
    _size = 0;
    head.next = (node_type*)&tail;
    head.prev = 0;
    tail.prev = (node_type*)&head;
    tail.next = 0;
    stack = buffer;

    for (node_type* it = stack, *end = stack + (N - 1); it != end; it++) {
        it->next = it + 1;
    }

    buffer[N - 1].next = 0;
}
