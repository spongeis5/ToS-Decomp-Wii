// zNPCBTActionBuilder.cpp -- one function, read from the image with
// tools/disasm.py: Build asks the base builder for the action, and if
// its type is 3 gives it the NPC and returns it; an action of type 3 or
// 0 is returned as it is, anything else becomes the always-fail action
// (assigned, then the one return: a direct return of it is a word long).
// The type is the action's first virtual, behind a vptr at +12 (three
// members precede it). Layouts from the DWARF: zBTActionBuilder 0x8,
// zNPCBTActionBuilder 0x14, zBTAction 0x10; the NPC pointer at +16 is
// the derived action's.

namespace Sext {
class ActionBase;
}

class zBTClient;
class zNPCBase;

class zBTAction {
public:
    Sext::ActionBase* actionAsset;
    void* resumeCB;
    zBTClient* btClient;

    virtual int GetType();

    static zBTAction gActionAlwaysFail;
};

class zNPCBTAction : public zBTAction {
public:
    zNPCBase* npc;
};

class zBTActionBuilder {
public:
    zBTAction* Build(int index, Sext::ActionBase* base) const;

    zBTClient* client;
    unsigned char _pad0[0x4];
};

class zNPCBTActionBuilder : public zBTActionBuilder {
public:
    zBTAction* Build(int index, Sext::ActionBase* base) const;

    zNPCBase* npcBase;
    unsigned int actionCount;
    unsigned int* actions;
};

zBTAction* zNPCBTActionBuilder::Build(int index,
                                      Sext::ActionBase* base) const {
    zBTAction* action = zBTActionBuilder::Build(index, base);

    if (action->GetType() == 3) {
        ((zNPCBTAction*)action)->npc = npcBase;
        return action;
    }

    if (action->GetType() != 3 && action->GetType() != 0) {
        action = &zBTAction::gActionAlwaysFail;
    }

    return action;
}
