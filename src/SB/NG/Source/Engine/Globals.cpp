// Globals.cpp -- one function, read from the image with tools/disasm.py:
// the GlobalsModule constructor on the System::Module base (name, four
// event stages, then the virtuals, vptr at 0x14), storing 6147 into the
// first event stage. The module's instance is the unit's .ctors entry,
// which this file does not supply.

namespace System {

class Module {
public:
    Module();

    char* name;
    int stage[4];
};

}  // namespace System

namespace GlobalsPrivate {

class GlobalsModule : public System::Module {
public:
    virtual void __vtable_anchor();
    GlobalsModule();
};

}  // namespace GlobalsPrivate

#pragma dont_inline on
GlobalsPrivate::GlobalsModule::GlobalsModule() : System::Module() {
    stage[0] = 6147;
}
#pragma dont_inline off
