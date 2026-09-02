// WAD01_11 -- one function, read from the image with tools/disasm.py:
// zBlackboard::Register<int>, a template member instantiated for int
// (the return type rides in the mangled name). It asks the board for
// an empty slot, refuses without one, allocates a zVariable<int> from
// the global heap through the class's operator new (56 bytes, tag 88,
// the empty constructor storing only the vtable behind new's null
// test), fills it -- id, value and default, flags with the observable
// bit, the type, the first observer cleared with a four-byte memset --
// and hangs it on the slot. Layouts from the DWARF (zVariableBase 0x30
// with the vptr after its eleven words, zVariable<T> 0x38, zBlackboard
// 0x8).

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {
enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };
}  // namespace Memory

void* xMemAlloc(Memory::GlobalHeapEnum heap, unsigned int size, int align,
                eMemMgrTag tag);
extern "C" void* memset(void* dst, int c, unsigned long n);

class DelegateP0;

enum eVarType { eVarType_ = 0x7FFFFFFF };

class zVariableBase {
public:
    static void* operator new(unsigned long size) {
        return xMemAlloc((Memory::GlobalHeapEnum)0, size, 0, (eMemMgrTag)88);
    }

    unsigned int id;
    eVarType type;
    unsigned int flags;
    DelegateP0* observers[8];

    virtual void __key();
};

template <class T>
class zVariable : public zVariableBase {
public:
    T defaultValue;
    T value;
};

class zBlackboard {
public:
    zVariableBase** FindEmpty(unsigned int id) const;

    template <class T>
    bool Register(unsigned int id, T value, bool observable);

    unsigned int size;
    zVariableBase** variables;
};

template <class T>
bool zBlackboard::Register(unsigned int id, T value, bool observable) {
    zVariableBase** slot = FindEmpty(id);

    if (!slot) {
        return false;
    }

    zVariable<T>* var = new zVariable<T>;

    var->id = id;
    var->value = value;
    var->defaultValue = value;
    var->flags = 0;

    if (observable) {
        var->flags |= 1;
    }

    var->type = (eVarType)1;
    memset(var->observers, 0, sizeof(var->observers[0]));

    *slot = var;

    return true;
}

template bool zBlackboard::Register<int>(unsigned int id, int value,
                                         bool observable);
