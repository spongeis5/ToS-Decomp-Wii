// zBase.cpp -- three functions, read from the image with
// tools/disasm.py. zNPC::IsNPCBase walks the RTTI parent table from the
// entity's base type until it reaches the NPC type (0x39) or runs out.
// zBase_GetChildrenBaseCount and zBase_GetChildBase take the type, or
// the NPC type when the entity is an NPC, and dispatch: a group answers
// through xGroup's capacity and items, an NPC through its 29th and 30th
// virtuals, anything else with zero. Layout from the DWARF (xBase's
// base type at +0x20 behind the entity's vptr).

class xGroup;

namespace Util {
extern unsigned int g_rttidParentTable[];
}  // namespace Util

// An entity, with the two virtuals the dispatch calls at their slots;
// the slots before them exist only to put them there.
class xBase {
public:
    virtual void _v0();   virtual void _v1();   virtual void _v2();
    virtual void _v3();   virtual void _v4();   virtual void _v5();
    virtual void _v6();   virtual void _v7();   virtual void _v8();
    virtual void _v9();   virtual void _v10();  virtual void _v11();
    virtual void _v12();  virtual void _v13();  virtual void _v14();
    virtual void _v15();  virtual void _v16();  virtual void _v17();
    virtual void _v18();  virtual void _v19();  virtual void _v20();
    virtual void _v21();  virtual void _v22();  virtual void _v23();
    virtual void _v24();  virtual void _v25();  virtual void _v26();
    virtual void _v27();
    virtual int GetChildrenBaseCount();
    virtual xBase* GetChildBase(int index);

    unsigned char _pad0[0x1C];
    unsigned int baseType;
};

class xGroup : public xBase {
public:
    int GetMaxCapacity();
    xBase* GetItem(unsigned int index);
};

class zNPC {
public:
    static bool IsNPCBase(const xBase* base);
};

int zBase_GetChildrenBaseCount(const xBase* base);
xBase* zBase_GetChildBase(const xBase* base, int index);

int zBase_GetChildrenBaseCount(const xBase* base) {
    unsigned int type = base->baseType;

    if (zNPC::IsNPCBase(base)) {
        type = 0x39;
    }

    switch (type) {
    case 0x50:
        return ((xGroup*)base)->GetMaxCapacity();
    case 0x39:
        return ((xBase*)base)->GetChildrenBaseCount();
    default:
        return 0;
    }
}

bool zNPC::IsNPCBase(const xBase* base) {
    unsigned int type = base->baseType;

    do {
        if (type == 0x39) {
            return true;
        }

        type = Util::g_rttidParentTable[type];
    } while (type != 0);

    return false;
}

xBase* zBase_GetChildBase(const xBase* base, int index) {
    unsigned int type = base->baseType;

    if (zNPC::IsNPCBase(base)) {
        type = 0x39;
    }

    switch (type) {
    case 0x50:
        return ((xGroup*)base)->GetItem(index);
    case 0x39:
        return ((xBase*)base)->GetChildBase(index);
    default:
        return 0;
    }
}
