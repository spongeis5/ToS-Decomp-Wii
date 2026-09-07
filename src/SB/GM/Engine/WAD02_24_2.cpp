// WAD02_24_2.cpp -- the NPC behaviour-tree CONDITIONS, and after them
// the entity and bound geometry: 111 functions, 19,880 bytes.
//
// A condition is a tiny object: the asset it was built from, the client
// that owns it, and the NPC it asks about. Evaluate() reads one thing
// off one of the NPC's components and answers yes or no, which is why
// most of them are under fifty bytes.
//
// Layouts from the Wii build's DWARF (tools/dwarf_types.py), which
// covers this file: zNPCBTCondition 0x14 on a zBTCondition of 0xC, and
// each condition adds its own fields after it. The Sext:: asset structs
// the Setup functions read are NOT in the DWARF -- only the offsets
// each one touches are recovered, and they are named for the condition
// that reads them.

class xEnt;
class zBTClient;
class zNPCEntity;

namespace Sext {

class ConditionBase;

enum eHitSource { eHitSourceEVENT = 0, END_eHitSourceENUM = 62 };

enum eRPSAttackTypes {
    eRPSAttackType_None = 0,
    eRPSAttackType_Hammer = 1,
    eRPSAttackType_Spin = 2,
    eRPSAttackType_Puck = 3,
    eRPSAttackType_Miniboss_Hammer = 4,
    eRPSAttackType_Miniboss_Spin = 5,
    eRPSAttackType_Miniboss_Puck = 6,
    END_eRPSAttackType_ENUM = 7,
};

enum eNPCPerceptionType {
    eNPCPerception_TargetReachable = 0,
    eNPCPerception_TargetChargable = 1,
    eNPCPerception_TargetAttackable = 2,
    eNPCPerception_TargetVisible = 3,
    eNPCPerception_TargetShootable = 4,
    eNPCPerception_TargetEngageable = 5,
    END_eNPCPerception_ENUM = 6,
};

// Only the fields each Setup reads; the structs themselves are not
// described by the debug info.
class DamagedCondition {
public:
    bool nonzero;
};

class InRPSAttackStateCondition {
public:
    unsigned char state;
};

class CheckPerceptionCondition {
public:
    unsigned int targets;
    unsigned char perceptionType;
    unsigned char anded;
};

}  // namespace Sext

// 0x2C in the DWARF; this file reads the knockback flag and the damage.
class zNPCGetsDamageInfo {
public:
    unsigned int flags;
    unsigned char _pad0[0xC - 0x4];
    float damageHP;
    unsigned char _pad1[0x2C - 0x10];
};

class zNPCCombat {
public:
    bool HitByType(Sext::eHitSource source) const;
    bool BlockedType(Sext::eHitSource source) const;


    unsigned char _pad0[0x294];
    Sext::eRPSAttackTypes attackState;
    unsigned char _pad1[0x29C - 0x298];
    zNPCGetsDamageInfo damageList[6];
    unsigned char _pad2[0x3BC - 0x3A4];
    unsigned int damageListSize;
    unsigned char _pad3[0x4E0 - 0x3C0];
    unsigned int blockListSize;
};

class zNPCPerception {
public:
    bool AreTargetsPerceived(unsigned int mask, Sext::eNPCPerceptionType type,
                             bool all);
};

class zPlanktonShakeManager {
public:
    static bool IsBeingShaken(const xEnt* ent);
};

// The word at +0x10 and the flag byte at +0x70 are inside the entity
// base; only their offsets and the value tested against are recovered.
class zNPCBase {
public:
    unsigned char _pad0[0x10];
    unsigned int typeID;
    unsigned char _pad1[0x70 - 0x14];
    bool puppetMode : 1;
    bool _bits0 : 7;
    unsigned char _pad2[0x98 - 0x71];
    zNPCEntity* npcEntity;
    unsigned char _pad3[0xA4 - 0x9C];
    zNPCPerception* npcPerception;
    zNPCCombat* npcCombat;
};

// 0x1D0 in the DWARF; the two poke flags sit at +0x181 and +0x182.
class zNPCEntity {
public:
    unsigned char _pad0[0x181];
    bool pokedA;
    bool pokedB;
};

class zPlayerCommon {
public:
    unsigned char _pad0[0x8B0];
    int spongeBuffState;
};

class xGlobals {
public:
    unsigned char _pad0[0x428];
    zPlayerCommon* player;
};

extern xGlobals* xglobals;

// zBTCondition is 0xC in the DWARF with two members named, so the word
// left over is the vtable pointer; nothing here dispatches through it.

class zNPCBTBlockedCondition {
public:
    bool Evaluate() const;

    Sext::ConditionBase* conditionAsset;
    zBTClient* btClient;
    unsigned char _vtable[4];
    zNPCBase* npcBase;
    Sext::ConditionBase* asset;
};

class zNPCBTBlockedTypeCondition {
public:
    bool Evaluate() const;

    Sext::ConditionBase* conditionAsset;
    zBTClient* btClient;
    unsigned char _vtable[4];
    zNPCBase* npcBase;
    Sext::ConditionBase* asset;
    Sext::eHitSource type;
};

class zNPCBTDamagedByTypeCondition {
public:
    bool Evaluate() const;

    Sext::ConditionBase* conditionAsset;
    zBTClient* btClient;
    unsigned char _vtable[4];
    zNPCBase* npcBase;
    Sext::ConditionBase* asset;
    Sext::eHitSource type;
};

class zNPCBTDamagedCondition {
public:
    void Setup(const Sext::ConditionBase* condition);

    Sext::ConditionBase* conditionAsset;
    zBTClient* btClient;
    unsigned char _vtable[4];
    zNPCBase* npcBase;
    Sext::ConditionBase* asset;
    bool nonzero;
};


class zNPCBTPokedCondition {
public:
    bool Evaluate() const;

    Sext::ConditionBase* conditionAsset;
    zBTClient* btClient;
    unsigned char _vtable[4];
    zNPCBase* npcBase;
    Sext::ConditionBase* asset;
    int pokeType;
};

class zNPCBTInRPSAttackStateCondition {
public:
    void Setup(const Sext::ConditionBase* condition);
    bool Evaluate() const;

    Sext::ConditionBase* conditionAsset;
    zBTClient* btClient;
    unsigned char _vtable[4];
    zNPCBase* npcBase;
    Sext::ConditionBase* asset;
    Sext::eRPSAttackTypes state;
};

class zNPCBTIsPlanktonShakingCondition {
public:
    bool Evaluate() const;

    Sext::ConditionBase* conditionAsset;
    zBTClient* btClient;
    unsigned char _vtable[4];
    zNPCBase* npcBase;
    Sext::ConditionBase* asset;
};

class zNPCBTSBPlayerIsBuff {
public:
    bool Evaluate() const;

    Sext::ConditionBase* conditionAsset;
    zBTClient* btClient;
    unsigned char _vtable[4];
    zNPCBase* npcBase;
    Sext::ConditionBase* asset;
};

class zNPCBTIsInPuppetModeCondition {
public:
    bool Evaluate() const;

    Sext::ConditionBase* conditionAsset;
    zBTClient* btClient;
    unsigned char _vtable[4];
    zNPCBase* npcBase;
    Sext::ConditionBase* asset;
};

class zNPCBTCheckPerceptionCondition {
public:
    void Setup(const Sext::ConditionBase* condition);
    bool Evaluate() const;

    Sext::ConditionBase* conditionAsset;
    zBTClient* btClient;
    unsigned char _vtable[4];
    zNPCBase* npcBase;
    Sext::ConditionBase* asset;
    Sext::eNPCPerceptionType perceptionType;
    unsigned int targets;
    bool anded;
};

bool zNPCBTBlockedCondition::Evaluate() const {
    zNPCCombat* combat = npcBase->npcCombat;

    if (combat != 0) {
        return combat->blockListSize != 0;
    }

    return false;
}

void zNPCBTInRPSAttackStateCondition::Setup(
    const Sext::ConditionBase* condition) {
    unsigned char value =
        ((const Sext::InRPSAttackStateCondition*)condition)->state;

    asset = (Sext::ConditionBase*)condition;
    state = (Sext::eRPSAttackTypes)value;
}

bool zNPCBTInRPSAttackStateCondition::Evaluate() const {
    zNPCCombat* combat = npcBase->npcCombat;

    if (combat != 0) {
        return state == combat->attackState;
    }

    return false;
}

bool zNPCBTBlockedTypeCondition::Evaluate() const {
    zNPCCombat* combat = npcBase->npcCombat;

    if (combat != 0) {
        return combat->BlockedType(type);
    }

    return false;
}

bool zNPCBTDamagedByTypeCondition::Evaluate() const {
    zNPCCombat* combat = npcBase->npcCombat;

    if (combat != 0 && combat->HitByType(type)) {
        return true;
    }

    return false;
}

// THE STORE MOVES ABOVE THE LOAD, and nothing tried has moved it back.
// Retail reads the asset byte, then stores the asset pointer, then
// stores the byte; mwcc stores the pointer first. Reading into a local
// first does not stop it, and neither does a declared constructor on
// the struct being read or on the one being written -- the lever that
// worked in zNPCCombat. Two words of four, at retail's size; the same
// two words are the whole difference in the other two Setups.
void zNPCBTDamagedCondition::Setup(const Sext::ConditionBase* condition) {
    bool value = ((const Sext::DamagedCondition*)condition)->nonzero;

    asset = (Sext::ConditionBase*)condition;
    nonzero = value;
}


bool zNPCBTIsPlanktonShakingCondition::Evaluate() const {
    return zPlanktonShakeManager::IsBeingShaken((const xEnt*)npcBase->npcEntity);
}


bool zNPCBTPokedCondition::Evaluate() const {
    switch (pokeType) {
    case 0:
        return npcBase->npcEntity->pokedA;
    case 1:
        return npcBase->npcEntity->pokedB;
    }

    return false;
}

bool zNPCBTSBPlayerIsBuff::Evaluate() const {
    return xglobals->player->spongeBuffState == 1;
}

void zNPCBTCheckPerceptionCondition::Setup(
    const Sext::ConditionBase* condition) {
    const Sext::CheckPerceptionCondition* c =
        (const Sext::CheckPerceptionCondition*)condition;
    unsigned int t = c->targets;
    unsigned char p = c->perceptionType;
    unsigned char a = c->anded;

    targets = t;
    perceptionType = (Sext::eNPCPerceptionType)p;
    anded = a;
}

bool zNPCBTCheckPerceptionCondition::Evaluate() const {
    zNPCPerception* perception = npcBase->npcPerception;

    if (perception != 0) {
        return perception->AreTargetsPerceived(targets, perceptionType,
                                               anded);
    }

    return false;
}

bool zNPCBTIsInPuppetModeCondition::Evaluate() const {
    zNPCBase* npc = npcBase;

    if (npc->typeID != 0xF0) {
        return false;
    }

    return npc->puppetMode;
}
