// zBlackboard.cpp -- twenty-five functions, 3,120 bytes, read from the
// image with tools/disasm.py. A blackboard is a small open-addressed
// hash of zVariableBase* keyed by a variable id: Init allocates size*4
// pointers and zeroes them, Find probes from `id & (size - 1)` and
// Write/Read/Cast are one template each, instantiated per stored type.
//
// Layouts from the DWARF (tools/dwarf_types.py): zBlackboard 0x8 --
// size at +0 and the pointer array at +4. zVariableBase 0x30 -- id,
// type and flags from +0, an eight-slot observer array at +0xC, and the
// vtable pointer after it at +0x2C (the virtuals are declared after the
// data members so the vptr lands there, the same shape zSoundAsset and
// EntityMgrModule carry).
//
// MEASURED: 24 of the 24 functions this object defines are
// byte-identical, 2,084 bytes of the unit's 3,120 -- and fourteen of the
// twenty-four are THREE template bodies written once: Write<T> (five
// instantiations, 800 bytes), Read<T> (four, 416) and
// zVariableDynamicCast::Cast<T> (five, 160). What the bytes fixed:
//
//   * Reset and ResetVariable call the variable's FIRST virtual: retail
//     reads vtable+8, and under CodeWarrior's two-word vtable header that
//     is slot 0. Declaring it third (vtable+16) was one word off in each.
//     The virtual is declared and never defined, so no vtable lands here.
//   * FindEmpty and Find are register order: `result` declared BEFORE
//     `i` puts them in r8/r9 (FindEmpty) and r10/r11 (Find) where ours
//     had them the other way round. Same lever as BalanceRight's.
//   * Find also wants NO per-iteration local -- `variables[i]` spelled
//     three times, not `v = variables[i]` once -- because a named v takes
//     r11 and pushes the three long-lived locals down a register. A
//     twelve-way sweep (four orders x three spellings) found exactly one
//     match; the named-v form with the right order is 14 words off.
//   * Cast<T> compares v->type against one constant per T (int 1, float
//     2, xVec3 3, zPlayer* 6, Sext::uid 8): a specialised trait with an
//     enumerator folds to retail's cmpwi. mwcc 1.1 takes `template <>`
//     specialisations, member function templates, and explicit
//     instantiation of a member template (`template bool
//     zBlackboard::Write<int>(...)`) -- all three were needed, because
//     nothing in this unit calls Write or Read.
//   * The pointer Cast fills through its reference and the pointer the
//     rest of the function uses are TWO locals. One address-taken local
//     is reloaded from its stack slot every loop iteration (Write<T> a
//     word long in every instantiation); retail loads it once into r29.
//   * Read<uid> copies the eight-byte value through a stack temporary the
//     scalar Reads never show. A by-value accessor gives that: `out =
//     var->GetValue()` with `T GetValue() const { return value; }` makes
//     the class return a temporary and leaves int, float and the pointer
//     as a register.
//   * Write<T> was then a swapped counter/pointer pair -- retail has var
//     in r29 and the loop counter in r30, ours the reverse, in all five
//     instantiations -- and it fell to NOTES' recipe for exactly that
//     shape and nothing else: a `void*` hop between the cast result and
//     `var`, AND the counter declared above `var`. Thirty other shapes
//     across six sweeps (declaration orders, a named observer local, a
//     pointer walk, an inlined setter, a reference or pointer to the
//     value, the counter above alone, the hop alone) moved nothing.
//     Third confirmation of that lever; see NOTES.
//
// STILL UNWRITTEN: the payload dispatcher Write(const
// Sext::BlackboardWritePayload*), 1,036 bytes, the unit's last function.

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {
enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };
}  // namespace Memory

void* xMemAlloc(Memory::GlobalHeapEnum heap, unsigned int size, int align,
                eMemMgrTag tag);
extern "C" void* memset(void* dst, int c, unsigned long n);

enum eVarType {
    eVarType_Invalid = 0,
    eVarType_S32 = 1,
    eVarType_F32 = 2,
    eVarType_xVec3 = 3,
    eVarType_xBasePtr = 4,
    eVarType_zInteractionUpPtr = 5,
    eVarType_zPlayerPtr = 6,
    eVarType_xMovePointPtr = 7,
    eVarType_UID = 8,
    eVarType_zUpFloatingObjectPtr = 9,
    eVarType_zVariableEventData = 10
};

class zPlayer;

// The assignment is out of line in retail (Write<xVec3> calls it), so
// it is declared and never defined here.
class xVec3 {
public:
    xVec3& operator=(const xVec3& other);

    float x;
    float y;
    float z;
};

namespace Sext {
class uid {
public:
    unsigned long long internalUid;
};
}  // namespace Sext

// An observer is an {object, function} pair and Write<T> invokes it
// inline: lwz +4 for the function, lwz +0 for the object, bctrl.
namespace Util {
template <class T>
class DelegateP0 {
public:
    T Invoke() const { return func(object); }

    void* object;
    T (*func)(void*);
};
}  // namespace Util

class zVariableBase {
public:
    unsigned int id;
    eVarType type;
    unsigned int flags;
    Util::DelegateP0<void>* observers[8];

    virtual void OnReset();

    void RegisterObserver(Util::DelegateP0<void>* obs);
    void UnregisterObserver(Util::DelegateP0<void>* obs);
};

template <class T>
class zVariable : public zVariableBase {
public:
    T GetValue() const { return value; }

    T defaultValue;
    T value;
};

// Cast<T> compares the stored type against one constant per T; these
// are the five the image instantiates.
template <class T>
struct zVariableTypeOf;

template <>
struct zVariableTypeOf<int> {
    enum { kType = eVarType_S32 };
};

template <>
struct zVariableTypeOf<float> {
    enum { kType = eVarType_F32 };
};

template <>
struct zVariableTypeOf<xVec3> {
    enum { kType = eVarType_xVec3 };
};

template <>
struct zVariableTypeOf<zPlayer*> {
    enum { kType = eVarType_zPlayerPtr };
};

template <>
struct zVariableTypeOf<Sext::uid> {
    enum { kType = eVarType_UID };
};

class zVariableDynamicCast {
public:
    template <class T>
    static void Cast(zVariableBase* v, zVariable<T>*& out);
};

template <class T>
void zVariableDynamicCast::Cast(zVariableBase* v, zVariable<T>*& out) {
    if (v->type == zVariableTypeOf<T>::kType) {
        out = (zVariable<T>*)v;
    } else {
        out = 0;
    }
}

class zBlackboard {
public:
    unsigned int size;
    zVariableBase** variables;

    zVariableBase* Find(unsigned int id) const;
    zVariableBase** FindEmpty(unsigned int id) const;
    void Init(unsigned int n);
    void Reset();
    eVarType GetVariableType(unsigned int id) const;
    void RegisterVariableObserver(unsigned int id, Util::DelegateP0<void>* obs);
    void UnregisterVariableObserver(unsigned int id,
                                    Util::DelegateP0<void>* obs);
    bool ResetVariable(unsigned int id);

    template <class T>
    bool Write(unsigned int id, const T& value);
    template <class T>
    bool Read(unsigned int id, T& out) const;
};

void zVariableBase::RegisterObserver(Util::DelegateP0<void>* obs) {
    for (int i = 0; i < 8; i++) {
        if (observers[i] == 0) {
            observers[i] = obs;
            return;
        }
    }
}

void zVariableBase::UnregisterObserver(Util::DelegateP0<void>* obs) {
    for (int i = 0; i < 8; i++) {
        if (observers[i] == obs) {
            observers[i] = 0;
            return;
        }
    }
}

zVariableBase* zBlackboard::Find(unsigned int id) const {
    unsigned int probe = id & (size - 1);
    zVariableBase* result = 0;
    unsigned int i = probe;

    do {
        if (variables[i] != 0 && variables[i]->id == id) {
            result = variables[i];
            break;
        }

        i++;

        if (i >= size) {
            i = 0;
        }
    } while (i != probe);

    return result;
}

zVariableBase** zBlackboard::FindEmpty(unsigned int id) const {
    unsigned int probe = id & (size - 1);
    zVariableBase** result = 0;
    unsigned int i = probe;

    do {
        if (variables[i] == 0) {
            result = &variables[i];
            break;
        }

        i++;

        if (i >= size) {
            i = 0;
        }
    } while (i != probe);

    return result;
}

void zBlackboard::Init(unsigned int n) {
    size = n;
    variables = (zVariableBase**)xMemAlloc((Memory::GlobalHeapEnum)0, n * 4, 0,
                                           (eMemMgrTag)88);
    memset(variables, 0, n * 4);
}

void zBlackboard::Reset() {
    for (unsigned int i = 0; i < size; i++) {
        zVariableBase* v = variables[i];

        if (v != 0) {
            v->OnReset();
        }
    }
}

eVarType zBlackboard::GetVariableType(unsigned int id) const {
    zVariableBase* v = Find(id);

    if (v != 0) {
        return v->type;
    }

    return eVarType_Invalid;
}

void zBlackboard::RegisterVariableObserver(unsigned int id,
                                           Util::DelegateP0<void>* obs) {
    zVariableBase* v = Find(id);

    if (v != 0) {
        v->RegisterObserver(obs);
    }
}

void zBlackboard::UnregisterVariableObserver(unsigned int id,
                                             Util::DelegateP0<void>* obs) {
    zVariableBase* v = Find(id);

    if (v != 0) {
        v->UnregisterObserver(obs);
    }
}

bool zBlackboard::ResetVariable(unsigned int id) {
    zVariableBase* v = Find(id);

    if (v == 0) {
        return false;
    }

    v->OnReset();
    return true;
}

template <class T>
bool zBlackboard::Write(unsigned int id, const T& value) {
    zVariableBase* v = Find(id);

    if (v == 0) {
        return false;
    }

    zVariable<T>* result;

    zVariableDynamicCast::Cast(v, result);

    // The counter above the pointer AND a void* hop between the cast
    // result and the pointer: that pair puts var in r29 and i in r30,
    // and neither half does it alone (NOTES, the counter-and-pointer
    // pair).
    unsigned int i;
    void* p = result;
    zVariable<T>* var = (zVariable<T>*)p;

    if (var != 0) {
        var->value = value;

        for (i = 0; i < 8; i++) {
            if (var->observers[i] != 0) {
                var->observers[i]->Invoke();
            }
        }

        return true;
    }

    return false;
}

template <class T>
bool zBlackboard::Read(unsigned int id, T& out) const {
    zVariableBase* v = Find(id);

    if (v == 0) {
        return false;
    }

    zVariable<T>* result;

    zVariableDynamicCast::Cast(v, result);

    zVariable<T>* var = result;

    if (var != 0) {
        out = var->GetValue();
        return true;
    }

    return false;
}

// The instantiations the image holds, in its order. Cast<T> follows
// from each Write.
template bool zBlackboard::Write<Sext::uid>(unsigned int id,
                                            const Sext::uid& value);
template bool zBlackboard::Read<Sext::uid>(unsigned int id,
                                           Sext::uid& out) const;
template bool zBlackboard::Write<xVec3>(unsigned int id, const xVec3& value);
template bool zBlackboard::Write<zPlayer*>(unsigned int id,
                                           zPlayer* const& value);
template bool zBlackboard::Read<zPlayer*>(unsigned int id,
                                          zPlayer*& out) const;
template bool zBlackboard::Write<float>(unsigned int id, const float& value);
template bool zBlackboard::Read<float>(unsigned int id, float& out) const;
template bool zBlackboard::Write<int>(unsigned int id, const int& value);
template bool zBlackboard::Read<int>(unsigned int id, int& out) const;
