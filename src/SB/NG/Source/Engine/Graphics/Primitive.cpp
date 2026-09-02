// Primitive.cpp -- one function, read from the image with
// tools/disasm.py: the PrimitiveModule constructor on the System::Module
// base (name, four event stages, then the virtuals, vptr at 0x14),
// storing 1920 into the first event stage.

namespace System {

class Module {
public:
    Module();

    char* name;
    int stage[4];
};

}  // namespace System

namespace Graphics {

class PrimitiveModule : public System::Module {
public:
    virtual void __vtable_anchor();
    PrimitiveModule();
};

}  // namespace Graphics

#pragma dont_inline on
Graphics::PrimitiveModule::PrimitiveModule() : System::Module() {
    stage[0] = 1920;
}
#pragma dont_inline off
