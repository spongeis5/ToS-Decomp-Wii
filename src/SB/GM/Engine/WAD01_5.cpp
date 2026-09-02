// WAD01_5 -- one function, read from the image with tools/disasm.py:
// zBTFactory::Create<zBTNode::RandomChildIterator>, a template
// instantiation that takes a block from the factory (24 bytes, type 14)
// and constructs in it. The bytes hold TWO null tests on one compare:
// `bne; li r3,0; b` from the conditional, and `beq` from the placement
// new's own check on its pointer -- so the source is the conditional
// around a placement new, not a plain `new`.

namespace Memory {
enum eFactoryMemType { eFactoryMemType_ = 0x7FFFFFFF };

class Factory {
public:
    void* AllocMem(unsigned int size, eFactoryMemType type);
};
}  // namespace Memory

inline void* operator new(unsigned long, void* p) { return p; }

class zBTFactory {
public:
    static Memory::Factory factory;

    template <class T>
    static T* Create();
};

template <class T>
T* zBTFactory::Create() {
    void* mem = factory.AllocMem(sizeof(T), (Memory::eFactoryMemType)14);

    if (!mem) {
        return 0;
    }

    return new (mem) T;
}

class zBTNode {
public:
    class RandomChildIterator {
    public:
        RandomChildIterator();

        unsigned char _pad0[0x18];
    };
};

template zBTNode::RandomChildIterator*
zBTFactory::Create<zBTNode::RandomChildIterator>();
