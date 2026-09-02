// WAD02_1_1 -- one function, read from the image with tools/disasm.py:
// IO::LFDeviceModule's destructor, the compiler's own -- the null-this
// test, the device factory member's destructor (hkBaseObject's, with
// the don't-delete flag) at +0x98, and operator delete when the flag
// says so. The layout is the DWARF's: the System::Module base is 0x98
// bytes and the factory follows it.

void operator delete(void* mem);

class hkBaseObject {
public:
    virtual ~hkBaseObject();
};

namespace System {

class Module {
public:
    unsigned char _pad0[0x98];
};

}  // namespace System

namespace IO {

class LFDeviceModule : public System::Module {
public:
    ~LFDeviceModule();

    // The bytes call hkBaseObject's destructor on this member directly,
    // with the don't-delete flag: whatever the DWARF names it, a class
    // of its own here gets a destructor of its own emitted and called.
    hkBaseObject factory;
    unsigned char _pad1[0xC];
};

}  // namespace IO

IO::LFDeviceModule::~LFDeviceModule() {}
