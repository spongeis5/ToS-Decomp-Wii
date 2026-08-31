// C:/branches/SB09/main/GM/Engine/Game/zNPCBTConditionBuilder.cpp
//
// Layout from the Wii build's DWARF (tools/dwarf_types.py):
//
//   class zBTConditionBuilder    /* 0x8 */ { zBTClient* client; };
//   class zNPCBTConditionBuilder /* 0xC */ : zBTConditionBuilder
//                                          { zNPCBase* npcBase; };  // +0x8
//
// The builder overrides its base's Build, calls the base version through a
// QUALIFIED call (a direct `bl`, not a vtable slot), and then patches one
// field on whatever came back.
//
// The returned object's vtable pointer is at +8 and the field written is at
// +0xC, while the virtual call passes the object's own address in r3 rather
// than r3+8. That combination only happens one way under CodeWarrior: a
// non-polymorphic base of exactly 8 bytes, then a derived class that
// introduces the first virtual. Giving Condition its own vptr at +0 puts
// the field at +8 and cannot produce these bytes.
//
// `cmpwi r3, 3` on the virtual's result is a type tag, and the DWARF does
// not name the enumerator -- 3 is what the instruction holds.

namespace Sext {
class ConditionBase;
}

class zNPCBase;
class zBTClient;

// The 8 bytes that sit before Condition's vtable pointer. Not described by
// the DWARF beyond its size, which is all the offsets here need.
class ConditionData {
public:
    unsigned char _head[0x8];
};

class Condition : public ConditionData {
public:
    virtual int GetConditionType() const;

    zNPCBase* npcBase;
};

class zBTConditionBuilder {
public:
    Condition* Build(int id, Sext::ConditionBase* asset) const;

    zBTClient* client;
    // The DWARF gives this class one 4-byte member and a size of 8, so
    // four bytes at +4 are real and undescribed. npcBase lands at +8
    // because of them: without the padding it lands at +4 and the one
    // instruction that reads it is the one instruction that differs.
    //
    // Most likely a vtable pointer -- the derived class overrides Build.
    // It is spelled as padding rather than as `virtual` because declaring
    // it virtual makes CodeWarrior emit a vtable this translation unit
    // does not have; retail's object for this file defines exactly one
    // symbol, and it is the function.
    unsigned char _tail[0x4];
};

class zNPCBTConditionBuilder : public zBTConditionBuilder {
public:
    Condition* Build(int id, Sext::ConditionBase* asset) const;

    zNPCBase* npcBase;
};

Condition* zNPCBTConditionBuilder::Build(int id,
                                         Sext::ConditionBase* asset) const {
    Condition* condition = zBTConditionBuilder::Build(id, asset);

    if (condition->GetConditionType() == 3) {
        condition->npcBase = npcBase;
    }

    return condition;
}
