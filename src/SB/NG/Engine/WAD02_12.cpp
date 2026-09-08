// The accessor part below was written by tools/gen_accessors.py and
// is kept unchanged; the constructor at the foot was added by hand,
// so the generator's banner is gone -- gen_units.py overwrites any
// file that still carries it, and this one must not be overwritten.
//
// Every function here touches nothing but its own members or a
// constant: one load, one store, the address of a member, a
// constant return, or members set to one constant. Each body was
// decoded from the image and re-encoded back to the same bytes,
// and each parameter list re-mangled back to the same symbol,
// before being written. GENERATED, not read: real matched
// functions whose offsets are recovered fact, but a count of them
// is not a count of decompiled code.
//
// Members are non-virtual, and the padding is padding -- only the
// offsets each function touches are known, not the fields between.

extern int ghkSimWorld;

int xHavok_GetWorld();

int xHavok_GetWorld() { return ghkSimWorld; }

// System::Module, layout from the DWARF and transcribed in
// SystemCache.cpp: two members ahead of the first virtual, so the
// vptr of anything derived lands at +0x14, which is where the
// constructor's vtable store goes.
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

class HavokModule : public System::Module {
public:
    HavokModule();
};

HavokModule::HavokModule() {
    events.stage[1] = 3;
}
