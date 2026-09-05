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
// MEASURED: 25 of the 25 functions this object defines are
// byte-identical -- all 3,120 bytes of the unit -- and fourteen of the
// twenty-five are THREE template bodies written once: Write<T> (five
// instantiations, 800 bytes), Read<T> (four, 416) and
// zVariableDynamicCast::Cast<T> (five, 160). The object also emits a
// Read<xVec3> the unit does not hold; the image has it as a weak
// instantiation elsewhere and it matches there. What the bytes fixed:
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
//   * The 1,036-byte payload dispatcher took four more, each the whole
//     remaining difference at the time (NOTES, "what -O4's auto-inliner
//     takes"): GetVariableType spelled as a ternary is the same 52
//     bytes standalone and is auto-inlined three times where the if/else
//     is never taken; the xVec3 case's cast pointer must be an inlined
//     callee's local to take slot +8, so it goes through a tiny in-class
//     CastTo<T>; `if (Read(...) == false) return false;` lays the false
//     return inline where `!Read` and every other spelling put the Write
//     first; and a named `targetType` for one side of the type compare
//     turns `cmpw r29,r0` into retail's `cmpw r0,r29`, with `eVarType
//     sourceType` declared before `source` for r28/r29.

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

class zPlayerContainer {
public:
    zPlayer* playerArray[4];
    int numPlayers;
};

class xGlobals {
public:
    unsigned char _pad0[0x428];
    zPlayerContainer players;
};

extern xGlobals* xglobals;

namespace Sext {

enum eSourceType {
    eSourceType_Variable = 0,
    eSourceType_Value = 1,
    eSourceType_Reset = 2
};

enum eBlackboardVariableType {
    eBlackboardVariableType_Integer = 0,
    eBlackboardVariableType_Float = 1,
    eBlackboardVariableType_Boolean = 2,
    eBlackboardVariableType_Player = 3,
    eBlackboardVariableType_Vector3 = 4,
    eBlackboardVariableType_UID = 5,
    eBlackboardVariableType_EventData = 6
};

// 0x20: the target id, the source kind, and a union of the three
// sources. The value's own union sits at +8 of ValueStruct because the
// uid in it is eight-aligned.
class BlackboardWritePayload {
public:
    class VariableStruct {
    public:
        unsigned int VariableName;
    };

    class ValueStruct {
    public:
        eBlackboardVariableType type;
        union {
            struct {
                int Accumulate;
                int Value;
            } Integer;
            struct {
                int Accumulate;
                float Value;
            } Float;
            unsigned char Boolean;
            int Player;
            // mwcc warns (10402) that a union cannot hold xVec3 because
            // of its declared operator=, then lays it out anyway; the
            // bytes match, so the warning is expected on every build.
            xVec3 Vector3;
            uid UID;
        };
    };

    class ResetStruct {
    public:
        unsigned char _pad0[0x1];
    };

    unsigned int WriteTo;
    eSourceType SourceType;
    union {
        VariableStruct Variable;
        ValueStruct Value;
        ResetStruct Reset;
    };
};

}  // namespace Sext

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
    bool Write(const Sext::BlackboardWritePayload* payload);

    template <class T>
    bool Write(unsigned int id, const T& value);
    template <class T>
    bool Read(unsigned int id, T& out) const;

    // Small enough for -O4 to inline, and that is its job: the payload
    // dispatcher's xVec3 case needs the cast pointer allocated AFTER all
    // of its own locals (slot +8), which only an inlined callee's local
    // gets. Read<T> itself is never auto-inlined in any spelling.
    template <class T>
    zVariable<T>* CastTo(zVariableBase* v) const {
        zVariable<T>* var;

        zVariableDynamicCast::Cast(v, var);
        return var;
    }
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

// A ternary, not an if: the same 52 bytes standalone, but small enough
// that -O4's auto-inliner takes it into the payload dispatcher three
// times, as retail's did. The if/else spelling is not taken.
eVarType zBlackboard::GetVariableType(unsigned int id) const {
    zVariableBase* v = Find(id);

    return v != 0 ? v->type : eVarType_Invalid;
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

bool zBlackboard::Write(const Sext::BlackboardWritePayload* payload) {
    unsigned int writeTo = payload->WriteTo;

    switch (payload->SourceType) {
    case Sext::eSourceType_Variable: {
        eVarType sourceType;
        unsigned int source = payload->Variable.VariableName;

        sourceType = GetVariableType(source);

        eVarType targetType = GetVariableType(writeTo);

        if (targetType != sourceType) {
            return false;
        }

        switch (GetVariableType(writeTo)) {
        case eVarType_S32: {
            int value;

            if (Read(source, value) == false) {
                return false;
            }

            return Write(writeTo, value);
        }
        case eVarType_F32: {
            float value;

            if (Read(source, value) == false) {
                return false;
            }

            return Write(writeTo, value);
        }
        case eVarType_zPlayerPtr: {
            zPlayer* value;

            if (Read(source, value) == false) {
                return false;
            }

            return Write(writeTo, value);
        }
        case eVarType_xVec3: {
            xVec3 value;
            bool ok;
            zVariableBase* var = Find(source);

            if (var == 0) {
                ok = false;
            } else {
                zVariable<xVec3>* result = CastTo<xVec3>(var);

                if (result != 0) {
                    value = result->GetValue();
                    ok = true;
                } else {
                    ok = false;
                }
            }

            if (ok == false) {
                return false;
            }

            return Write(writeTo, value);
        }
        case eVarType_UID: {
            Sext::uid value;

            if (Read(source, value) == false) {
                return false;
            }

            return Write(writeTo, value);
        }
        default:
            return false;
        }
    }
    case Sext::eSourceType_Value:
        switch (payload->Value.type) {
        case Sext::eBlackboardVariableType_Integer: {
            int value = payload->Value.Integer.Value;

            if (payload->Value.Integer.Accumulate == 1) {
                int current;

                if (Read(writeTo, current)) {
                    current += value;
                    return Write(writeTo, current);
                }

                return false;
            }

            return Write(writeTo, value);
        }
        case Sext::eBlackboardVariableType_Float: {
            float value = payload->Value.Float.Value;

            if (payload->Value.Float.Accumulate == 1) {
                float current;

                if (Read(writeTo, current)) {
                    current += value;
                    return Write(writeTo, current);
                }

                return false;
            }

            return Write(writeTo, value);
        }
        case Sext::eBlackboardVariableType_Boolean: {
            int value = payload->Value.Boolean;

            return Write(writeTo, value);
        }
        case Sext::eBlackboardVariableType_Player: {
            int index = payload->Value.Player;
            zPlayer* player = 0;

            if (index >= 0 && index < xglobals->players.numPlayers) {
                player = xglobals->players.playerArray[index];
            }

            return Write(writeTo, player);
        }
        case Sext::eBlackboardVariableType_Vector3: {
            xVec3 value = payload->Value.Vector3;

            return Write(writeTo, value);
        }
        case Sext::eBlackboardVariableType_UID: {
            Sext::uid value = payload->Value.UID;

            return Write(writeTo, value);
        }
        }

        // no break: an unknown value kind resets the variable
    case Sext::eSourceType_Reset:
        return ResetVariable(writeTo);
    }

    return false;
}
