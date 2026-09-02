// TRCModule.cpp -- five functions, read from the image with
// tools/disasm.py. The constructor builds the System::Module base, the
// pad manager and the save/load object, clears the game interface and
// sets two words of the base's event set. GetPriority hands back a
// three-entry table of relative priorities, filled with the pad, console
// pad and Scaleform module pointers on first call under a guard (the
// constants are static data, the pointers are not). Startup clears the
// DVD-error critical section, initialises its mutex and starts the
// save/load object through its first virtual; Update runs the pad
// manager, the save/load object's second virtual and the message-box
// manager. RegisterGameInterface stores the game's interface.
//
// Layouts from the DWARF (tools/dwarf_types.py): TRCModule 0x28C on
// Module 0x98, the pad manager at +0x98 (0x1E0), SaveLoad at +0x278 with
// its vptr eight bytes in, the interface at +0x288; RelativePriority
// {order, associate, events}; CriticalSection {refcount, OSMutex,
// owner}. The vtable slots are read off __vt__Q23TRC9TRCModule.
//
// The message-box manager is a file static in the unity unit's unnamed
// namespace, so retail's symbol carries WAD02's name and a fragment's
// carries its own: the mangling follows the main file, and a #line or
// an #include does not move it (measured 2026-09-02). The object is
// defined here as the source has it; the load is a masked relocation
// either way, and the unit matches without linking.

namespace TRC {
class GameInterface;
}  // namespace TRC

extern "C" {
struct OSMutex {
    unsigned char _pad0[0x18];
};

void OSInitMutex(OSMutex* mutex);
}

namespace System {

enum PriorityOrder {
    PRIORITY_BEFORE = 0,
    PRIORITY_AFTER = 1
};

class Module;

class RelativePriority {
public:
    PriorityOrder order;
    Module* associate;
    int events;
};

class EventSet {
public:
    int stage[4];
};

// The vptr follows the two members declared ahead of the first virtual
// (+0x14), where the retail constructor stores the vtable. Slot 0 is
// left undefined here so the vtable's home stays in WAD02's data.
class Module {
public:
    Module();

    char* name;
    EventSet events;

    virtual void _v0();
    virtual void _v1();
    virtual const RelativePriority* GetPriority(int& count) const;
    virtual void Startup(int stage);
    virtual void _v4();
    virtual void _v5();
    virtual void _v6();
    virtual void _v7();
    virtual void _v8();
    virtual void Update(int stage);

    int contextFlags;
    short eventBindingIndices[60];
    bool enabled;
};

}  // namespace System

namespace IO {
extern System::Module* padModule;
extern System::Module* consolePadModule;
}  // namespace IO

namespace Scaleform {
extern System::Module* scaleformModule;
}  // namespace Scaleform

namespace TRC {

class CriticalSection {
public:
    int refcount;
    OSMutex mutex;
    void* owner;
};

extern CriticalSection wiiDVDErrorCriticalSection;

// Word-aligned, so that it follows the base's trailing bool at +0x98
// and not at +0x95: the DWARF's first word member is at its +0x8.
class TRCPadManager {
public:
    TRCPadManager();
    void Update();

    unsigned char _pad0[0x8];
    int holdingType;
    unsigned char _pad1[0x1D4];
};

class SaveLoadBase {
public:
    unsigned char _pad0[0x8];

    virtual void Startup();
    virtual void Update();
};

class SaveLoad : public SaveLoadBase {
public:
    int lastSlotIdx;
};

class MsgBoxManager {
public:
    void Update();

    unsigned char _pad0[0x48];
};

namespace {
MsgBoxManager sMsgBoxManager;
}  // namespace

class TRCModule : public System::Module {
public:
    TRCModule();

    // Declared first, undefined here: the vtable's home is not this unit.
    virtual void _v1();
    virtual const System::RelativePriority* GetPriority(int& count) const;
    virtual void Startup(int stage);
    virtual void Update(int stage);

    void RegisterGameInterface(GameInterface* value);

    TRCPadManager trcPadManager;
    SaveLoad saveLoad;
    GameInterface* gameInterface;
};

}  // namespace TRC

TRC::TRCModule::TRCModule() : gameInterface(0) {
    events.stage[0] = 1;
    events.stage[2] = 0x40;
}

const System::RelativePriority* TRC::TRCModule::GetPriority(int& count) const {
    static System::RelativePriority priority[3] = {
        { System::PRIORITY_AFTER, IO::padModule, 0x40 },
        { System::PRIORITY_AFTER, IO::consolePadModule, 0x40 },
        { System::PRIORITY_BEFORE, Scaleform::scaleformModule, 0x40 },
    };

    count = 3;

    return priority;
}

void TRC::TRCModule::Startup(int) {
    wiiDVDErrorCriticalSection.refcount = 0;
    OSInitMutex(&wiiDVDErrorCriticalSection.mutex);

    saveLoad.Startup();
}

void TRC::TRCModule::Update(int) {
    trcPadManager.Update();
    saveLoad.Update();
    sMsgBoxManager.Update();
}

void TRC::TRCModule::RegisterGameInterface(GameInterface* value) {
    gameInterface = value;
}
