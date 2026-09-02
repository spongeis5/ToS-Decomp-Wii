// zPlayerAction and zPlayerActionManager -- 25 functions, read from the
// image with tools/disasm.py.
//
// THE TWO VTABLE POSITIONS ARE MEASURED. zPlayerAction's constructor
// stores its __vt__ at offset 12 and zero at offset 8, so its vptr sits
// after three words of members; zPlayer is reached through `lwz r12,0(r3)`
// so its vptr is at zero and nothing precedes it. mwcc puts the vptr after
// whatever data members are declared before the first virtual, which is
// why the members come first in one class and not the other.
//
// A vtable entry at byte offset N is the (N-8)/4th virtual -- the table
// has an eight-byte header. zPlayerAction is called at 12,16,20,24,36,40,
// 44,48,52,56,60,68,72,92,96,100,104,112, so it needs 27 slots; zPlayer at
// 160,176,184,220, so it needs 54. The slots nothing calls exist only to
// put the ones that ARE called where the image has them.
//
// WHICH FUNCTIONS ARE STATIC is read off the register shuffle, not
// guessed. NewState moves r3 into r10 and NewStateMany spills it to the
// first stack slot, so in both `this` is being passed as the callee's
// `void* owner` -- they are members. AddActionTransition moves nothing
// into the callee's argument registers and tests r8 as its third callback,
// so r3 is already the table: it is static.

class xAnimTable;
class xAnimPlay;
class xAnimState;
class xAnimSingle;
class xAnimTransition;
class xQuat;
class xVec3;
class xScene;
class xEntFrame;
class zPlayer;
class zPlayerAction;
class zPlayerActionManager;
class zInteractionUP;

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {
enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };
}

typedef void (*xAnimStateCB)(xAnimPlay*, xAnimState*, void*);
typedef void (*xAnimSingleCB)(xAnimState*, xAnimSingle*, void*);
typedef void (*xAnimMatrixCB)(xAnimPlay*, xQuat*, xVec3*, xVec3*, int);
typedef unsigned int (*xAnimTranCB)(xAnimTransition*, xAnimSingle*, void*);

unsigned int xAnimTableNewState(
    xAnimTable* table, const char* name, unsigned int a, unsigned int b,
    float c, float* d, float* e, float f, unsigned short* g, void* owner,
    xAnimStateCB h, xAnimStateCB i, xAnimSingleCB j, xAnimMatrixCB k,
    unsigned long long l, unsigned int m);

unsigned int xAnimTableNewStateMany(
    xAnimTable* table, const char* name, int n, unsigned int a,
    unsigned int b, float c, float* d, float* e, float f,
    unsigned short* g, void* owner, xAnimStateCB h, xAnimStateCB i,
    xAnimSingleCB j, xAnimMatrixCB k, unsigned int m);

unsigned int xAnimTableNewTransition(
    xAnimTable* table, const char* from, const char* to, xAnimTranCB a,
    xAnimTranCB b, xAnimTranCB c, unsigned int d, unsigned int e, float f,
    float g, unsigned short h, unsigned short i, float j,
    unsigned short* k);

void* xMemAlloc(Memory::GlobalHeapEnum heap, unsigned int size, int align,
                eMemMgrTag tag);
extern "C" void* memset(void* dst, int c, unsigned long n);

inline void* operator new(unsigned long, void* p) { return p; }

class zPlayerInventory {
public:
    zPlayerInventory();
};

// The constructor the image names zPlayerInventory's, by its symbol.
// Its body zeroes one bool, which is also zUpContextActionManager's
// constructor, and the two folded; BeginUpdate calls it on the
// manager's context through the player with no null test, which
// every placement-new spelling adds (the check is on operator new's
// result, and CodeWarrior rejects the explicit constructor-call syntax).
extern "C" void __ct__16zPlayerInventoryFv(void* inventory);

class zUpContextActionManager {
public:
    void SceneSetup();
    void SceneReset();
    void SceneExit();
    void CollectionBegin();
    void CollectionEnd(zPlayer* player);
    zInteractionUP* IsInteractionNavigation();
    zInteractionUP* IsInteractionActivity();
    zInteractionUP* IsInteractionPresence();

    unsigned char _pad[0x4];
};

class zInteractionManager {
public:
    void ContextCollect();
    void SetCurrent(zInteractionUP* up);
};

// --------------------------------------------------------------------------

class zPlayerAction {
public:
    zPlayerActionManager* manager;
    zPlayer* player;
    void (*doneCB)(zPlayerAction*);

    enum SpecialActions { SpecialActions_ = 0x7FFFFFFF };

    zPlayerAction();

    virtual void _v0();
    virtual void AddStandardTransitions(xAnimTable* table, const char* name);
    virtual void AddDefaultTransitions(xAnimTable* table, const char* name);
    virtual void AddTransitions(xAnimTable* table, const char* name,
                                xAnimTranCB a, xAnimTranCB b,
                                unsigned short e, float f, unsigned int g,
                                unsigned int h, SpecialActions i);
    virtual unsigned int GetID();
    virtual void _v5();
    virtual void _v6();
    virtual void CollectA();
    virtual void CollectB();
    virtual void CollectC();
    virtual void AddStates(xAnimTable* table);
    virtual void AddTableTransitions(xAnimTable* table);
    virtual void AddExtraTransitions(xAnimTable* table);
    virtual void Reset();
    virtual void _v14();
    virtual void BeginUpdate(float dt);
    virtual void Update(float dt);
    virtual void Move(xScene* scene, float dt, xEntFrame* frame);
    virtual void UpdateFall(float dt);
    virtual void EndUpdate(float dt);
    virtual void _v20();
    virtual void Setup();
    virtual void Enter();
    virtual void Leave();
    virtual void Exit();
    virtual void _v25();
    virtual void PreUpdate(float dt);

    unsigned int NewState(xAnimTable* table, const char* name,
                          unsigned int a, unsigned int b, float c, float* d,
                          float* e, float f, unsigned short* g,
                          xAnimStateCB h, xAnimStateCB i, xAnimSingleCB j,
                          xAnimMatrixCB k, unsigned int l);
    unsigned int NewStateMany(xAnimTable* table, const char* name, int n,
                              unsigned int a, unsigned int b, float c,
                              float* d, float* e, float f,
                              unsigned short* g, xAnimStateCB h,
                              xAnimStateCB i, xAnimSingleCB j,
                              xAnimMatrixCB k, unsigned int l);
    static unsigned int AddActionTransition(
        xAnimTable* table, const char* from, const char* to, xAnimTranCB a,
        xAnimTranCB b, xAnimTranCB c, unsigned short g, float h,
        unsigned int i, unsigned int j);
    static unsigned int ActionChange(xAnimTransition* tran,
                                     xAnimSingle* single, void* data);
    unsigned int IsEnabled();
};

class zPlayerActionManager {
public:
    zPlayerAction** actions;
    zPlayerAction* current;
    unsigned int count;
    unsigned int currentID;
    unsigned int lastID;
    unsigned int* enabled;
    unsigned int enabledWords;
    zUpContextActionManager context;
    // The context is the manager's last member and the manager sits
    // at zPlayer+192, so `this+28` and `player+220` are one address;
    // BeginUpdate reaches it through the player.

    void SetCurrentAction(zPlayerAction* action);
    unsigned int GetCurrentActionID() const;
    void Init(unsigned int n);
    void Add(zPlayer* player, zPlayerAction* action);
    void Setup();
    void Reset();
    void Exit();
    void BeginUpdate(float dt);
    void Update(float dt);
    void AddStates(xAnimTable* table);
    void AddTransitions(xAnimTable* table);
    void AddStandardTransitionsTo(unsigned int id, xAnimTable* table,
                                  const char* name);
    void AddDefaultTransitionsTo(unsigned int id, xAnimTable* table,
                                 const char* name);
    void AddTransitionsTo(unsigned int id, xAnimTable* table,
                          const char* name, xAnimTranCB a, xAnimTranCB b,
                          unsigned short e, float f, unsigned int g,
                          unsigned int h,
                          zPlayerAction::SpecialActions i);
    unsigned int IsEnabled(unsigned int id) const;
};

class zPlayer {
public:
    virtual void _v0();  virtual void _v1();  virtual void _v2();
    virtual void _v3();  virtual void _v4();  virtual void _v5();
    virtual void _v6();  virtual void _v7();  virtual void _v8();
    virtual void _v9();  virtual void _v10(); virtual void _v11();
    virtual void _v12(); virtual void _v13(); virtual void _v14();
    virtual void _v15(); virtual void _v16(); virtual void _v17();
    virtual void _v18(); virtual void _v19(); virtual void _v20();
    virtual void _v21(); virtual void _v22(); virtual void _v23();
    virtual void _v24(); virtual void _v25(); virtual void _v26();
    virtual void _v27(); virtual void _v28(); virtual void _v29();
    virtual void _v30(); virtual void _v31(); virtual void _v32();
    virtual void _v33(); virtual void _v34(); virtual void _v35();
    virtual void _v36(); virtual void _v37();
    virtual void BeginUpdate(float dt);
    virtual void _v39(); virtual void _v40(); virtual void _v41();
    virtual void EndUpdate(float dt);
    virtual void _v43();
    virtual void Move(xScene* scene, float dt, xEntFrame* frame);
    virtual void _v45(); virtual void _v46(); virtual void _v47();
    virtual void _v48(); virtual void _v49(); virtual void _v50();
    virtual void _v51(); virtual void _v52();
    virtual void UpdateFall(float dt);

    unsigned char _pad0[0xBC];
    zPlayerActionManager actionManager;
    unsigned char _pad1[0x3FC];
    zInteractionManager interactionManager;
};

// The object a transition belongs to, reached as `tran->f4->f90`. Neither
// type is named by anything in the image, so both are their offsets.
class zAnimTranOwner {
public:
    unsigned char _pad[0x90];
    zPlayerAction* action;
};

class zAnimTranHolder {
public:
    unsigned char _pad[0x4];
    zAnimTranOwner* owner;
};

// --------------------------------------------------------------------------

zPlayerAction::zPlayerAction() { doneCB = 0; }

unsigned int zPlayerAction::NewState(
    xAnimTable* table, const char* name, unsigned int a, unsigned int b,
    float c, float* d, float* e, float f, unsigned short* g,
    xAnimStateCB h, xAnimStateCB i, xAnimSingleCB j, xAnimMatrixCB k,
    unsigned int l) {
    return xAnimTableNewState(table, name, a, b, c, d, e, f, g, this,
                              h, i, j, k, 0, l);
}

unsigned int zPlayerAction::NewStateMany(
    xAnimTable* table, const char* name, int n, unsigned int a,
    unsigned int b, float c, float* d, float* e, float f,
    unsigned short* g, xAnimStateCB h, xAnimStateCB i, xAnimSingleCB j,
    xAnimMatrixCB k, unsigned int l) {
    return xAnimTableNewStateMany(table, name, n, a, b, c, d, e, f, g,
                                  this, h, i, j, k, l);
}

unsigned int zPlayerAction::AddActionTransition(
    xAnimTable* table, const char* from, const char* to, xAnimTranCB a,
    xAnimTranCB b, xAnimTranCB c, unsigned short g, float h,
    unsigned int i, unsigned int j) {
    if (c == 0) {
        c = ActionChange;
    }

    return xAnimTableNewTransition(table, from, to, a, b, c, i, j, 0.0f,
                                   0.0f, g, 0, h, 0);
}

unsigned int zPlayerAction::ActionChange(xAnimTransition* tran,
                                         xAnimSingle*, void*) {
    zPlayerAction* action = ((zAnimTranHolder*)tran)->owner->action;
    action->manager->SetCurrentAction(action);
    return 0;
}

unsigned int zPlayerAction::IsEnabled() {
    return player->actionManager.IsEnabled(GetID());
}

// --------------------------------------------------------------------------

void zPlayerActionManager::SetCurrentAction(zPlayerAction* action) {
    if (current) {
        currentID = current->GetID();
    }

    if (current) {
        lastID = action->GetID();
        current->Leave();
        lastID = 0;
    }

    current = action;
    action->Enter();
}

unsigned int zPlayerActionManager::GetCurrentActionID() const {
    // The virtual call is the FALL-THROUGH and -1 is the branch, which
    // is the opposite of testing for null first.
    if (current) {
        return current->GetID();
    }

    return -1;
}

void zPlayerActionManager::Init(unsigned int n) {
    count = n;
    actions = (zPlayerAction**)xMemAlloc((Memory::GlobalHeapEnum)0, n * 4,
                                         0, (eMemMgrTag)29);
    memset(actions, 0, n * 4);

    enabledWords = (n >> 5) + 1;
    enabled = (unsigned int*)xMemAlloc((Memory::GlobalHeapEnum)0,
                                       enabledWords * 4, 0,
                                       (eMemMgrTag)29);
    current = 0;
}

void zPlayerActionManager::Add(zPlayer* player, zPlayerAction* action) {
    action->manager = this;
    action->player = player;
    actions[action->GetID()] = action;
}

void zPlayerActionManager::Setup() {
    current = 0;

    for (unsigned int i = 0; i < count; i++) {
        actions[i]->Setup();
    }

    context.SceneSetup();
}

void zPlayerActionManager::Reset() {
    if (current) {
        current->Leave();
        current = 0;
    }

    for (unsigned int i = 0; i < count; i++) {
        actions[i]->Reset();
    }

    for (unsigned int i = 0; i < enabledWords; i++) {
        enabled[i] = -1;
    }

    context.SceneReset();
}

void zPlayerActionManager::Exit() {
    context.SceneExit();
    current = 0;

    for (unsigned int i = 0; i < count; i++) {
        actions[i]->Exit();
    }
}

void zPlayerActionManager::BeginUpdate(float dt) {
    // Retail:   lwz r3,4(r4) ; addi r3,r3,220 ; bl __ct__16zPlayer...
    // and placement new gives `addic. ; beq` around the call -- the
    // one instruction this function was short of for weeks. Calling
    // the constructor by its symbol is the spelling with no test. The
    // address is both `manager+28` (the context, by Setup/Reset/Exit)
    // and `player+220`, and the source goes through the player.
    __ct__16zPlayerInventoryFv(&current->player->actionManager.context);

    for (unsigned int i = 0; i < count; i++) {
        actions[i]->PreUpdate(dt);
        actions[i]->CollectA();
        actions[i]->CollectB();
        actions[i]->CollectC();
    }

    current->player->interactionManager.ContextCollect();
    context.CollectionEnd(current->player);

    zPlayer* p = current->player;

    zInteractionUP* up = context.IsInteractionNavigation();
    if (up) {
        p->interactionManager.SetCurrent(up);
    }

    up = context.IsInteractionActivity();
    if (up) {
        p->interactionManager.SetCurrent(up);
    }

    up = context.IsInteractionPresence();
    if (up) {
        p->interactionManager.SetCurrent(up);
    }

    current->BeginUpdate(dt);
    context.CollectionBegin();
}

void zPlayerActionManager::Update(float dt) {
    current->Update(dt);

    if (current->doneCB) {
        current->doneCB(current);
    }
}

void zPlayerActionManager::AddStates(xAnimTable* table) {
    for (unsigned int i = 0; i < count; i++) {
        actions[i]->AddStates(table);
    }
}

void zPlayerActionManager::AddTransitions(xAnimTable* table) {
    for (unsigned int i = 0; i < count; i++) {
        actions[i]->AddTableTransitions(table);
        actions[i]->AddExtraTransitions(table);
    }
}

void zPlayerActionManager::AddStandardTransitionsTo(unsigned int id,
                                                    xAnimTable* table,
                                                    const char* name) {
    actions[id]->AddStandardTransitions(table, name);
}

void zPlayerActionManager::AddDefaultTransitionsTo(unsigned int id,
                                                   xAnimTable* table,
                                                   const char* name) {
    actions[id]->AddDefaultTransitions(table, name);
}

void zPlayerActionManager::AddTransitionsTo(
    unsigned int id, xAnimTable* table, const char* name, xAnimTranCB a,
    xAnimTranCB b, unsigned short e, float f, unsigned int g,
    unsigned int h, zPlayerAction::SpecialActions i) {
    actions[id]->AddTransitions(table, name, a, b, e, f, g, h, i);
}

unsigned int zPlayerActionManager::IsEnabled(unsigned int id) const {
    unsigned int word = id / 32;
    unsigned int bit = id - word * 32;

    return (enabled[word] & (1 << bit)) != 0;
}

// The four that forward to the player's own virtuals. They are overrides,
// not helpers: the manager dispatches to BeginUpdate through slot 15.
void zPlayerAction::Move(xScene* scene, float dt, xEntFrame* frame) {
    player->Move(scene, dt, frame);
}

void zPlayerAction::BeginUpdate(float dt) { player->BeginUpdate(dt); }
void zPlayerAction::UpdateFall(float dt) { player->UpdateFall(dt); }
void zPlayerAction::EndUpdate(float dt) { player->EndUpdate(dt); }
