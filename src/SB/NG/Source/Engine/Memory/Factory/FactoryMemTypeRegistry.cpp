// FactoryMemTypeRegistry.cpp -- two functions, read from the image with
// tools/disasm.py. Inst returns the one registry, a function-local
// static behind the compiler's guard byte (`@GUARD@Inst...@inst`,
// `@LOCAL@Inst...@inst` in the image), constructed by clearing its 132
// bytes. FactoryMemTypeLeakCBRegisterer's constructor files a leak
// callback in the registry's table under its memory type. Layout from
// the DWARF (FactoryMemTypeRegistry 0x84: 33 callbacks).

extern "C" void* memset(void* dst, int c, unsigned long n);

namespace Memory {

enum eFactoryMemType { eFactoryMemType_ = 0x7FFFFFFF };

typedef void (*FactoryMemTypeLeakCB)(void* block, int size);

class FactoryMemTypeRegistry {
public:
    FactoryMemTypeRegistry() { memset(this, 0, sizeof(*this)); }

    static FactoryMemTypeRegistry* Inst();

    FactoryMemTypeLeakCB leakCBTable[33];
};

class FactoryMemTypeLeakCBRegisterer {
public:
    FactoryMemTypeLeakCBRegisterer(eFactoryMemType type,
                                   FactoryMemTypeLeakCB callback);
};

}  // namespace Memory

Memory::FactoryMemTypeRegistry* Memory::FactoryMemTypeRegistry::Inst() {
    static FactoryMemTypeRegistry inst;

    return &inst;
}

Memory::FactoryMemTypeLeakCBRegisterer::FactoryMemTypeLeakCBRegisterer(
    eFactoryMemType type, FactoryMemTypeLeakCB callback) {
    FactoryMemTypeRegistry::Inst()->leakCBTable[type] = callback;
}
