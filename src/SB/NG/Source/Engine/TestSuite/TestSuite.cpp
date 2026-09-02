// TestSuite.cpp -- one function, read from the image with
// tools/disasm.py: the TestSuite constructor on the System::Module base
// (name, four event stages, then the virtuals, vptr at 0x14), storing 3,
// 1 and 67 into the first three event stages.

namespace System {

class Module {
public:
    Module();

    char* name;
    int stage[4];
};

}  // namespace System

namespace Test {

class TestSuite : public System::Module {
public:
    virtual void __vtable_anchor();
    TestSuite();
};

}  // namespace Test

#pragma dont_inline on
Test::TestSuite::TestSuite() : System::Module() {
    stage[0] = 3;
    stage[1] = 1;
    stage[2] = 67;
}
#pragma dont_inline off
