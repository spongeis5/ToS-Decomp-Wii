// zNPCAnimViewer.cpp -- five functions, read from the image with
// tools/disasm.py. The anim viewer is an NPC that exists to play
// animations: Activate raises the presence flag, takes a logic
// component (24 bytes, factory type 1) and an entity (464 bytes, type
// 2) from the NPC manager's factory, gives up through BaseDeactivate
// when either is missing, and otherwise owns and activates both
// through their component vtables; DeactivateNPC lowers the flag and
// detaches the info nodes and components; Update brackets the logic's
// update with the pre- and post-update of all components; Render
// hands off to the entity's component. The type's CreateAnimTable
// is the anim viewer logic's, called with the table.
//
// Layouts from the DWARF (tools/dwarf_types.py): zNPCBase 0xC0 on
// xOGEntity, the handle at +0x14, the logic at +0x94 and the entity
// at +0x98; a component is its owner then its vptr; zNPCEntity is an
// xEnt (0xBC) with a component as its second base, which the upcast
// reaches with the 188 the bytes add. The presence flag is a class
// static, data of this unit's own, so it matches and does not link.
//
// NEAR MISS, Activate 58 of 67 words. Retail materialises BOTH
// allocation conditionals into r0 (`mr r0,r3` after each constructor)
// and tests the entity's value there; ours keeps the second in r3,
// because that value is compared afterwards, and the null block spells
// `li r3,0`. The second-base upcast is a reference (`*npcEntity`), which
// gives retail's `stwu` without the null adjustment a pointer conversion
// adds. Tried and no better: the test as `&&` with the success block
// first (54), both results in locals tested directly (43), the entity
// conditional typed as the first base (58), the entity in a local with
// the members tested (58), the upcast through a byte offset (58), the
// first conditional through a base-typed local (58), and the test on
// the raw allocations, one or both (50, 47).

class xAnimTable;
class zNPCStatus;

namespace Memory {

enum eFactoryMemType { eFactoryMemType_ = 0x7FFFFFFF };

class Factory {
public:
    void* AllocMem(unsigned int size, eFactoryMemType type);
};

}  // namespace Memory

inline void* operator new(unsigned long, void* p) { return p; }

namespace World {
class EntityHandleBase;
}  // namespace World

class zNPCManager {
public:
    static Memory::Factory factory;
};

class zNPCBase;

// A component: its owner, then the vptr, with the three virtuals this
// unit dispatches through at their slots.
class zNPCComponent {
public:
    zNPCBase* owner;

    virtual void Activate(const zNPCStatus* status);
    virtual void _v1();
    virtual void _v2();
    virtual void _v3();
    virtual void _v4();
    virtual void _v5();
    virtual void Render();
    virtual void _v7();
    virtual void _v8();
    virtual void _v9();
    virtual void Update(float dt);
};

class zNPCLogic : public zNPCComponent {};

class zCompLogicAnimViewer : public zNPCLogic {
public:
    zCompLogicAnimViewer();

    static void CreateAnimTable(xAnimTable* table);

    float mCurAnimCycleDuration;
    float mAutoActionTimer;
    int mAutoActionCount;
    float mTimer;
};

class xEnt {
public:
    unsigned char _pad0[0xBC];
};

class zNPCEntity : public xEnt, public zNPCComponent {
public:
    zNPCEntity(World::EntityHandleBase* handle);

    unsigned char _pad1[0x1D0 - 0xC4];
};

class zNPCBase {
public:
    void BaseDeactivate();
    void DetachAllInfoNodes();
    void DetachAllComponents(zNPCStatus* status);
    void PreUpdateAllComponents(float dt);
    void PostUpdateAllComponents(float dt);

    unsigned char _pad0[0x14];
    World::EntityHandleBase* handle;
    unsigned char _pad1[0x7C];
    zNPCLogic* npcLogic;
    zNPCEntity* npcEntity;
    unsigned char _pad2[0x24];
};

class zNPCAnimViewer : public zNPCBase {
public:
    class Type {
    public:
        void CreateAnimTable(xAnimTable* table);
    };

    bool Activate(const zNPCStatus* status);
    void DeactivateNPC(zNPCStatus* status);
    void Update(float dt);
    void Render();

    static bool mIsPresent;
};

bool zNPCAnimViewer::mIsPresent;

void zNPCAnimViewer::Type::CreateAnimTable(xAnimTable* table) {
    zCompLogicAnimViewer::CreateAnimTable(table);
}

bool zNPCAnimViewer::Activate(const zNPCStatus* status) {
    mIsPresent = true;

    void* logicMem = zNPCManager::factory.AllocMem(sizeof(zCompLogicAnimViewer),
                                                   (Memory::eFactoryMemType)1);
    npcLogic = !logicMem ? 0 : new (logicMem) zCompLogicAnimViewer;

    World::EntityHandleBase* entityHandle = handle;
    void* entityMem = zNPCManager::factory.AllocMem(sizeof(zNPCEntity),
                                                    (Memory::eFactoryMemType)2);
    npcEntity = !entityMem ? 0 : new (entityMem) zNPCEntity(entityHandle);

    if (npcLogic == 0 || npcEntity == 0) {
        BaseDeactivate();
        return false;
    }

    npcLogic->owner = this;
    npcLogic->Activate(status);

    zNPCComponent& entityComponent = *npcEntity;
    entityComponent.owner = this;
    entityComponent.Activate(status);

    return true;
}

void zNPCAnimViewer::DeactivateNPC(zNPCStatus* status) {
    mIsPresent = false;

    DetachAllInfoNodes();
    DetachAllComponents(status);
}

void zNPCAnimViewer::Update(float dt) {
    PreUpdateAllComponents(dt);

    if (npcLogic) {
        npcLogic->Update(dt);
    }

    PostUpdateAllComponents(dt);
}

void zNPCAnimViewer::Render() {
    if (npcEntity) {
        zNPCComponent& entityComponent = *npcEntity;

        entityComponent.Render();
    }
}
