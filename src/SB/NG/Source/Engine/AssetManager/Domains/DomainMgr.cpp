// DomainMgr.cpp -- one function of the unit: Domains::DomainModule's
// constructor. The rest is not written.

// System::Module, layout from the DWARF and transcribed in
// SystemCache.cpp: two members ahead of the first virtual, so the vptr
// of anything derived lands at +0x14, which is where the constructor's
// vtable store goes. `events` starts at +4, so the store at +4 is
// stage[0].
namespace System {

class EventSet {
public:
    int stage[4];
};

class Module {
public:
    Module();

    char* name;
    EventSet events;

    virtual void _v0();

    int contextFlags;
    short eventBindingIndices[60];
    bool enabled;
};

}  // namespace System

namespace Domains {

class DomainModule : public System::Module {
public:
    DomainModule();
};

}  // namespace Domains

Domains::DomainModule::DomainModule() {
    events.stage[0] = 67;
}
