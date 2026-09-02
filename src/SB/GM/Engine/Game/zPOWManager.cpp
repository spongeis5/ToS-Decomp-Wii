// zPOWManager.cpp -- three functions, read from the image with
// tools/disasm.py. zPOWManager_Startup registers the manager module
// under 'POWM' (a tail call; the name is the unity pool's, hence the
// generated header first). ScenePrepare hands the manager's factory a
// 128 KB block from the global heap (tag 28) in 8 KB pieces and calls
// the module's eighth virtual. SceneExit walks the current scene's POW
// list, an embedded list whose nodes sit four bytes into each entity,
// calling each entity's 27th virtual, and shuts the factory. Layouts
// from the DWARF (zModule 0x18 with its vptr after five words,
// zPOWManager the same); the list at xScene+0x58C and the scene at
// globals+0x43C are the loads. The manager object is a file static in
// retail, so this unit matches and does not link.

#include "SB/GM/Engine/Game/zPOWManager.pool.h"

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {
enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };

class Factory {
public:
    void Initialize(void* buffer, unsigned int size, unsigned int blockSize,
                    const unsigned int* sizes);
    void Deinitialize();

    unsigned char _pad0[0x3C];
};

}  // namespace Memory

void* xMemAlloc(Memory::GlobalHeapEnum heap, unsigned int size, int align,
                eMemMgrTag tag);

enum enModulePriority { enModulePriority_ = 0x7FFFFFFF };

class zModule {
public:
    int flags;
    int tag_module;
    char* nam_module;
    enModulePriority updatePriority;
    enModulePriority renderPriority;

    virtual void _v0();
    virtual void _v1();
    virtual void _v2();
    virtual void _v3();
    virtual void _v4();
    virtual void _v5();
    virtual void _v6();
    virtual void OnScenePrepared();
};

void zModuleMgr_RegisterModule(zModule* module, int id, const char* name,
                               enModulePriority update,
                               enModulePriority render);

class zPOWManager : public zModule {
public:
    void ScenePrepare();
    void SceneExit();

    static Memory::Factory factory;
};

extern zPOWManager gPOWManager;

class EmbeddedListNode {
public:
    EmbeddedListNode* next;
    EmbeddedListNode* prev;
};

// An entity, with the one virtual SceneExit calls at its slot; the
// slots before it exist only to put it there. Its list node is at +4.
class xBase {
public:
    virtual void _v0();   virtual void _v1();   virtual void _v2();
    virtual void _v3();   virtual void _v4();   virtual void _v5();
    virtual void _v6();   virtual void _v7();   virtual void _v8();
    virtual void _v9();   virtual void _v10();  virtual void _v11();
    virtual void _v12();  virtual void _v13();  virtual void _v14();
    virtual void _v15();  virtual void _v16();  virtual void _v17();
    virtual void _v18();  virtual void _v19();  virtual void _v20();
    virtual void _v21();  virtual void _v22();  virtual void _v23();
    virtual void _v24();  virtual void _v25();
    virtual void POWSceneExit();
};

class xScene {
public:
    unsigned char _pad0[0x58C];
    EmbeddedListNode powList;
};

class zGlobals {
public:
    unsigned char _pad0[0x43C];
    xScene* sceneCur;
};

extern zGlobals globals;

void zPOWManager_Startup() {
    zModuleMgr_RegisterModule(&gPOWManager, 'POWM', "POW Manager",
                              (enModulePriority)1, (enModulePriority)1);
}

void zPOWManager::ScenePrepare() {
    factory.Initialize(xMemAlloc((Memory::GlobalHeapEnum)0, 0x20000, 0,
                                 (eMemMgrTag)28),
                       0x20000, 8192, 0);
    OnScenePrepared();
}

void zPOWManager::SceneExit() {
    EmbeddedListNode* head = &globals.sceneCur->powList;

    for (EmbeddedListNode* node = head->next; node != head; node = node->next) {
        ((xBase*)((char*)node - 4))->POWSceneExit();
    }

    factory.Deinitialize();
}
