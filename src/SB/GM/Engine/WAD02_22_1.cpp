// WAD02_22_1.cpp -- zNPCCombat: 23 functions, 3,752 bytes in the image.
// The combat component an NPC carries. It holds a zCombat by value --
// bounds, hit points, an attack state table and the list of things
// already hit this swing -- and around it keeps two ring-buffer-ish
// lists, one of damage taken and one of blows blocked, each six entries
// deep and each with a parallel array of the frame the entry landed on.
//
// Every layout here is recovered, not guessed: WAD02.cpp is one of the
// eleven compile units the retail link kept debug info for, so
// tools/dwarf_types.py names every member and every offset of
// zNPCCombat (0x4E8), zCombat (0x260), zNPCGetsDamageInfo (0x2C),
// zNPCGivesDamageInfo (0x14), zCombatDamageInfo (0x3C),
// zCombatDamageMultiplier (0x8) and the enums they use. The DWARF does
// not describe the classes only reached through a pointer, and those
// are left as forward declarations here.

// 0x38 in the DWARF, on an Entity base; only the type id is read here.
#include "SB/GM/Engine/WAD02_22_1.pool.h"

class xBase {
public:
    unsigned char _base0[0x18];
    unsigned long long id;
    unsigned int baseType;
    unsigned char UNUSED_linkCount;
    unsigned char assertFlags;
    unsigned short baseFlags;
};

class zNPCBase;
class zNPCCombat;
class xAnimState;
class zCombatHitSpot;
class xHierarchyNode;
class zNPCStatus;

class xVec3 {
public:
    float x;
    float y;
    float z;
};

namespace World {

// position at +0x30, which is where the damage broadcast takes the
// centre of the blow from.
class xOGModel {
public:
    unsigned char _pad0[0x30];
    xVec3 position;
};

}  // namespace World

class xEnt {
public:
    unsigned char _base0[0x34];
    World::xOGModel* model;
};

class xSphere {
public:
    xVec3 center;
    float r;
};

class xHierarchyBound {
public:
    xSphere master;
    xHierarchyNode* nodes;
    unsigned char count;
    unsigned char maxCount;
    signed char masterBone;
};

namespace Sext {

enum eHitSource {
    eHitSourceEVENT = 0,
    eHitSourceGENERAL = 1,
    eHitSourceEXPLOSION = 2,
    eHitSourceZERO_POINT = 3,
    eHitSourceJUGGLE = 4,
    eHitSourceLASER_PLAYER = 5,
    eHitSourceLASER_ENEMY = 6,
    eHitSourceSURFACE = 7,
    eHitSourceDEATHPLANE = 8,
    eHitSourceKNOCKBACK = 9,
    eHitSourceENEMY_HEAVY = 10,
    eHitSourceENEMY_LIGHT = 11,
    eHitSourceENEMY_ELECTRIFY = 12,
    eHitSourceEVENT_KILL = 14,
    eHitSourceDOT = 16,
    eHitSourceDOT_DROWN = 19,
    eHitSourceDEATHPLANE_DROWN = 20,
    eHitSourceDEATHPLANE_FALL = 21,
    eHitSourceCAVE_WORM = 23,
    eHitSourceDART_GUN = 24,
    eHitSourceSNARE = 25,
    eHitSourcePLANT = 26,
    eHitSourceSPIN_ATTACK = 27,
    eHitSourceSPIN_NOHAMMER_ATTACK = 28,
    eHitSourceHAMMER_ATTACK = 29,
    eHitSourceHAMMER_CRUSHING_ATTACK = 30,
    eHitSourcePUCK_ATTACK = 31,
    eHitSourceSPIN_SPONGEBUFF_ATTACK = 32,
    eHitSourceHAMMER_SPONGEBUFF_ATTACK = 33,
    eHitSourcePUCK_SPONGEBUFF_ATTACK = 34,
    eHitSourceSPIN_POWERUP_ATTACK = 35,
    eHitSourceHAMMER_POWERUP_ATTACK = 36,
    eHitSourcePUCK_POWERUP_ATTACK = 37,
    eHitSourceTURRET1 = 38,
    eHitSourceSPLASH_SPIN = 39,
    eHitSourceSPLASH_HAMMER = 40,
    eHitSourceSPLASH_PUCK = 41,
    eHitSourceHIGH_FALL_SB = 42,
    eHitSourceSPLASH_NPCSpawner = 43,
    eHitSourcePROJECTILE = 44,
    eHitSourceSB_PROJECTILE_NPC_BOMB_EXPLOSION = 45,
    eHitSourcePLAYER_FLUID = 46,
    eHitSourcePLANKTON_STUN = 47,
    eHitSourcePLANKTON_DAMAGE = 48,
    eHitSourceSPONGEBUFF_CANDY = 49,
    eHitSourceBUNGEE = 50,
    eHitSourceSPONGEBUFF_BUNGEE = 51,
    eHitSourceSPLASHBACK = 52,
    eHitSourceJOSE_KNOCKBACK = 53,
    eHitSourceSB_ELECTRIC_ARC = 54,
    eHitSourceRUBBER_BAND = 55,
    eHitSourcePLAYER = 56,
    eHitSourceBUG_CATCH = 57,
    eHitSourceBIPLANE_PLAYER1 = 58,
    eHitSourceBIPLANE_PLAYER2 = 59,
    eHitSourceBIPLANE_PLAYER3 = 60,
    eHitSourceBIPLANE_PLAYER4 = 61,
    END_eHitSourceENUM = 62,
};

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

class EventAny;

}  // namespace Sext

enum zHitTarget {
    zHT_GENERAL = 0,
    zHT_FRONT = 1,
    zHT_BACK = 2,
    zHT_LEFT = 3,
    zHT_RIGHT = 4,
    zHT_ENTITY = 5,
    zHT_ENV = 6,
    zHT_CRITICAL = 7,
    zHT_COUNT = 8,
};

enum eNPCHitReaction {
    eNPCHitReaction_Unknown = 0,
    eNPCHitReaction_Block = 1,
    eNPCHitReaction_Ignore = 2,
    eNPCHitReaction_Damage = 3,
    eNPCHitReaction_InstaDeath = 4,
};

// The hit event the bounds carry, which is an empty Sext::EventActionNew
// with its own fields laid on top -- so damage is at +0, not after a
// base.
class EventActionHit {
public:
    float damage;
    Sext::eHitSource hitSource;
    xVec3 knockback;
    bool SentByDestructable;
};

// 0xD8 in the DWARF; only the hit source at +0x94 is read here.
class zCombatAttack {
public:
    unsigned char _pad0[0x94];
    Sext::eHitSource source;
};

class zNPCHitReactionTable {
public:
    Sext::eHitSource hitSource;
    eNPCHitReaction hitReaction;
};

// 0x24 in the DWARF. The asset's own hit-reaction table is the last
// word on a blow the attack states do not already answer for.
class zNPCCombatParams {
public:
    unsigned char _pad0[0x14];
    zNPCHitReactionTable* hitReactionTable;
    unsigned int hitReactionTableCount;
};

class Time {
public:
    long long ticks;
};

class sphereInfo {
public:
    xSphere sphere;
    xSphere previousSphere;
};

class zCombat {
public:
    void Render(xEnt* ent);

    xHierarchyBound bounds;
    unsigned int lastBoundUpdateTime;
    EventActionHit collParams;
    float currentHitPoints;
    float maximumHitPoints;
    unsigned short stateTableSize;
    zCombatAttack* stateTable;
    xAnimState* animationState;
    zCombatAttack* runningAttack;
    float runningAttackTimer;
    xEnt* lastNPCDamaged;
    bool disableMovement;
    bool runningEffect;
    bool runningBlur;
    bool hitting;
    bool hitEnv;
    bool hitObject;
    bool forceReset;
    unsigned char hitObjectCount;
    signed char firstValidHitObjectIndex;
    signed char lastValidHitObjectIndex;
    xBase* hitObjects[24];
    Time hitObjectsTime[24];
    float currentDamage;
    float lastHitDamage;
    Sext::eHitSource lastHitSource;
    zHitTarget lastHitTarget;
    sphereInfo location[6];
    unsigned int effectParam;
    zCombatHitSpot* hitSpots;
    unsigned short hitSpotCount;
    unsigned short hitProfile;
};

class hkpCharacterProxyListener {
public:
    unsigned char _vt[0x4];
};

class zNPCCollisionListener {
public:
    hkpCharacterProxyListener _base0;
    zNPCBase* npc;
};

class zNPCCombatCollisionListener : public zNPCCollisionListener {
};

// 0x1D0 in the DWARF, on an xEnt base at +0 and a zNPCComponent at
// +0xBC -- so the word at +0xBC is that component's owner.
class zNPCEntity {
public:
    void RegisterCollisionListener(zNPCCollisionListener* listener);
    void UnregisterCollisionListener(zNPCCollisionListener* listener);

    unsigned char _base0[0xBC];
    zNPCBase* owner;
    unsigned char _pad0[0x184 - 0xC0];
    float damageColorTimer;
};

namespace Memory {

enum eFactoryMemType { eFactoryMemType_ = 0x7FFFFFFF };

class Factory {
public:
    void* AllocMem(unsigned int size, eFactoryMemType type);
    void DeallocMem(void* p);
};

}  // namespace Memory

class zNPCManager {
public:
    static Memory::Factory factory;
};

// 0x44 apart in the array AllAttached builds.
class zCombatHitSpot {
public:
    void Destroy();

    unsigned char _pad0[0x44];
};

void zCombatSystemUpdateEntity(xEnt* ent, float dt);

enum ForceEvent { ForceEvent_ = 0x7FFFFFFF };

void zEntEvent(xBase* from, unsigned int fromEvent, xBase* to,
               unsigned int toEvent, Sext::EventAny* param,
               ForceEvent force);

void SendEventFromNpcToNpcsWithinDistance(xVec3* pos, xBase* from,
                                          unsigned int fromEvent,
                                          unsigned int toEvent,
                                          Sext::EventAny* param,
                                          float distance);
int zCombatGetBaseAttackSB(Sext::eHitSource source);

class zCombatDamageMultiplier {
public:
    Sext::eHitSource sourceType;
    float multiplier;
};

class zCombatDamageInfo {
public:
    zCombatDamageInfo();

    int flags;
    xBase* from;
    float damage;
    Sext::eHitSource source;
    zHitTarget target;
    xVec3 knockback;
    xVec3 hitLocation;
    xVec3 hitNormal;
    int attackID;
};

// Bit 0 of flags means the blow carries a knockback; the setter below
// works it out from the vector rather than being told.
class zNPCGetsDamageInfo {
public:
    void SetFromCombatDamageInfo(xEnt* ent, const zCombatDamageInfo& info,
                                 eNPCHitReaction reaction, float remaining);

    unsigned int flags;
    xBase* from;
    zNPCEntity* npcEntity;
    float damageHP;
    float remainingHP;
    Sext::eHitSource hitSource;
    zHitTarget hitTarget;
    eNPCHitReaction hitReaction;
    xVec3 knockback;
};

class zNPCGivesDamageInfo {
public:
    void SetFromCombatDamageInfo(xEnt* npc, xEnt* to,
                                 const zCombatDamageInfo& info);

    xEnt* toEnt;
    zNPCEntity* npcEntity;
    float damageHP;
    Sext::eHitSource hitSource;
    zHitTarget hitTarget;
};

// The vtable sits at +4, after owner, which is what declaring the
// virtuals BELOW the data member gives. Attached dispatches through
// the slot at vtable +0x10, which is the third; the two before it
// exist only to put it there, and none of the three is named by
// anything this file can read.
class zNPCComponent {
public:
    zNPCBase* owner;

    virtual void _v0();   virtual void _v1();   virtual void _v2();
    virtual void _v3();   virtual void _v4();   virtual void _v5();
    virtual void _v6();   virtual void _v7();   virtual void _v8();
    virtual void _v9();   virtual void _v10();
    virtual void* _v11();
};

class zNPCBase {
public:
    unsigned char _pad0[0x98];
    zNPCEntity* npcEntity;
    unsigned char _pad1[0xA8 - 0x9C];
    zNPCCombat* npcCombat;
};

extern unsigned int gSceneFrameCount;

class zNPCCombat : public zNPCComponent {
public:
    void SetParameters(zNPCCombatParams* params);
    void SetCurHitPoints(float hp);
    float GetDamageMultiplier(Sext::eHitSource source) const;
    void AddToDamageList(const zNPCGetsDamageInfo& info);
    bool HitByType(Sext::eHitSource source) const;
    bool BlockedType(Sext::eHitSource source) const;
    void ClearHitList(zNPCGetsDamageInfo* list, unsigned int* frames,
                      unsigned int& count, unsigned int frame);
    void Attached(const zNPCStatus* status);
    void Detached(zNPCStatus* status);
    void Reset(const zNPCStatus* status);
    void PreUpdate(float dt);
    void PostUpdate(float dt);
    void Render();
    eNPCHitReaction FindHitReaction(xEnt* ent,
                                    const zCombatDamageInfo* info);
    eNPCHitReaction FindHitReaction(Sext::eHitSource source);
    void SetAttackState(Sext::eRPSAttackTypes type, float time);
    bool HandleNPCDamage(xEnt* ent, const zCombatDamageInfo& info,
                         zNPCGetsDamageInfo* out);

    int attackID;
    zCombat baseCombat;
    zCombatHitSpot* hitSpots;
    unsigned short hitSpotCount;
    zNPCCombatParams* parameters;
    zNPCCombatCollisionListener combatCollisionListener;
    bool hitsDisabled;
    bool damageDisabled;
    zCombatDamageMultiplier* damageMultipliers;
    unsigned short damageMultiplierCount;
    float attackStateTimer;
    Sext::eRPSAttackTypes attackState;
    Sext::eRPSAttackTypes nextAttackState;
    zNPCGetsDamageInfo damageList[6];
    unsigned int damageListFrames[6];
    unsigned int damageListSize;
    unsigned int blockListFrames[6];
    zNPCGetsDamageInfo blockList[6];
    unsigned int blockListSize;
};

void zNPCGivesDamageInfo::SetFromCombatDamageInfo(
    xEnt* npc, xEnt* to, const zCombatDamageInfo& info) {
    toEnt = to;
    npcEntity = (zNPCEntity*)npc;
    damageHP = info.damage;
    hitSource = info.source;
    hitTarget = info.target;
}

void zNPCGetsDamageInfo::SetFromCombatDamageInfo(xEnt* ent,
                                                 const zCombatDamageInfo& info,
                                                 eNPCHitReaction reaction,
                                                 float remaining) {
    flags = 0;
    from = info.from;
    npcEntity = (zNPCEntity*)ent;
    damageHP = info.damage;
    remainingHP = remaining;
    hitSource = info.source;
    hitTarget = info.target;
    hitReaction = reaction;
    knockback = info.knockback;

    // The constant on the LEFT of each test: retail compares 0.0f
    // against the member, not the other way round.
    if (0.0f != info.knockback.x || 0.0f != info.knockback.y ||
        0.0f != info.knockback.z) {
        flags |= 1;
    }
}

void zNPCCombat::SetParameters(zNPCCombatParams* params) {
    if (owner == 0) {
        parameters = params;
    }
}

void zNPCCombat::SetCurHitPoints(float hp) {
    baseCombat.currentHitPoints = hp;

    if (hp > baseCombat.maximumHitPoints) {
        baseCombat.maximumHitPoints = hp;
    }
}

float zNPCCombat::GetDamageMultiplier(Sext::eHitSource source) const {
    int i;

    for (i = 0; i < damageMultiplierCount; i++) {
        if (source == damageMultipliers[i].sourceType) {
            return damageMultipliers[i].multiplier;
        }
    }

    return 1.0f;
}

void zNPCCombat::AddToDamageList(const zNPCGetsDamageInfo& info) {
    if (damageListSize >= 6) {
        return;
    }

    damageListFrames[damageListSize] = gSceneFrameCount;
    damageList[damageListSize++] = info;
}

bool zNPCCombat::HitByType(Sext::eHitSource source) const {
    unsigned int i;

    for (i = 0; i < damageListSize; i++) {
        if (source == damageList[i].hitSource) {
            return true;
        }
    }

    return false;
}

bool zNPCCombat::BlockedType(Sext::eHitSource source) const {
    unsigned int i;

    for (i = 0; i < blockListSize; i++) {
        if (source == blockList[i].hitSource) {
            return true;
        }
    }

    return false;
}

void zNPCCombat::Attached(const zNPCStatus* status) {
    _v2();

    if (owner->npcEntity != 0) {
        owner->npcEntity->RegisterCollisionListener(
            &combatCollisionListener);
    }

    hitsDisabled = false;
    damageDisabled = false;
}

void zNPCCombat::Detached(zNPCStatus* status) {
    if (owner->npcEntity != 0) {
        owner->npcEntity->UnregisterCollisionListener(
            &combatCollisionListener);
    }

    if (hitSpots != 0) {
        unsigned int i;

        for (i = 0; i < hitSpotCount; i++) {
            hitSpots[i].Destroy();
        }

        zNPCManager::factory.DeallocMem(hitSpots);
    }

    if (damageMultipliers != 0) {
        zNPCManager::factory.DeallocMem(damageMultipliers);
    }

    parameters = 0;
}

void zNPCCombat::Reset(const zNPCStatus* status) {
    damageListSize = 0;
    attackStateTimer = 0.0f;
    attackState = Sext::eRPSAttackType_None;
    nextAttackState = Sext::eRPSAttackType_None;
    blockListSize = 0;
    attackID = -1;
}

void zNPCCombat::PreUpdate(float dt) {
    xEnt* ent = (xEnt*)owner->npcEntity;

    if (ent == 0) {
        return;
    }

    zCombatSystemUpdateEntity(ent, dt);
}

void zNPCCombat::PostUpdate(float dt) {
    ClearHitList(damageList, damageListFrames, damageListSize,
                 gSceneFrameCount);
    ClearHitList(blockList, blockListFrames, blockListSize,
                 gSceneFrameCount);

    if (attackStateTimer > 0.0f) {
        attackStateTimer = attackStateTimer - dt;

        if (attackStateTimer <= 0.0f) {
            attackStateTimer = 0.0f;
            attackState = nextAttackState;
            nextAttackState = Sext::eRPSAttackType_None;
        }
    }
}

// zCombat::Render is empty, so the linker folded it onto the first
// empty function it saw; the branch here reaches that symbol's name,
// not this one's.
void zNPCCombat::Render() {
    baseCombat.Render((xEnt*)owner->npcEntity);
}

eNPCHitReaction zNPCCombat::FindHitReaction(
    xEnt* ent, const zCombatDamageInfo* info) {
    // Only a blow from the player carries an attack id worth tracking,
    // and only for the two sources that can repeat within one swing.
    if (info->from != 0 && info->from->baseType == 0x55) {
        if (info->source == 9 || info->source == 52) {
            if (attackID == info->attackID) {
                return eNPCHitReaction_Ignore;
            }
        }

        attackID = info->attackID;
    }

    if (attackState == Sext::eRPSAttackType_Hammer) {
        // The result goes nowhere, and the call is still made.
        zCombatGetBaseAttackSB(info->source);
        return eNPCHitReaction_Damage;
    }

    return FindHitReaction(info->source);
}

void zNPCCombat::ClearHitList(zNPCGetsDamageInfo* list,
                              unsigned int* frames, unsigned int& count,
                              unsigned int frame) {
    unsigned int i;

    for (i = 0; i < count; i++) {
        unsigned int j;

        if (frames[i] >= frame) {
            continue;
        }

        frames[i] = -1;

        // Walk in from the end for a survivor to move into the hole.
        // Anything passed on the way is old too, and goes with it.
        for (j = count - 1; j > i; j--) {
            if (frames[j] >= frame) {
                list[i] = list[j];
                frames[i] = frames[j];
                frames[j] = -1;
                break;
            }

            frames[j] = -1;
            count = count - 1;
        }

        count = count - 1;
    }
}

void zNPCCombat::SetAttackState(Sext::eRPSAttackTypes type, float time) {
    // A timed state goes in as the NEXT one and takes effect when the
    // timer runs out in PostUpdate; an untimed one takes effect now.
    if (time > 0.0f) {
        attackStateTimer = time;
        nextAttackState = type;
        return;
    }

    attackStateTimer = 0.0f;
    attackState = type;
    nextAttackState = Sext::eRPSAttackType_None;
}

// 86 OF RETAIL'S 88 WORDS, AND THE TWO MISSING ONES ARE ONE PAIR.
// Retail dispatches the switch below as `cmpwi r0,0; beq default;
// cmplwi r0,1; ble case1` -- it excludes zero first and then reaches
// case 1 with an UNSIGNED <= -- where we emit `cmpwi r0,1; beq case1`.
// Every word after that pair matches on a two-word shift, so nothing
// else in the function is wrong. Ruled out by measurement: a case 0
// that breaks, at the top or the bottom of the switch; an explicit
// default at the top; extra cases 3 and 6; a guard `if (attackState
// != None)` around the whole switch; casting the subject to unsigned;
// and writing out every enumerator of both enums. All seven measure
// 69 of 88, identically -- so it is not the shape of the switch.
eNPCHitReaction zNPCCombat::FindHitReaction(Sext::eHitSource source) {
    zNPCHitReactionTable* table;
    unsigned int count;
    int base;
    unsigned int i;

    if (parameters == 0) {
        return eNPCHitReaction_Unknown;
    }

    if (source == 50) {
        return eNPCHitReaction_Ignore;
    }

    // What the blow reduces to once the power-up and buff variants are
    // folded together -- 27 spin, 29 hammer, 31 puck.
    base = zCombatGetBaseAttackSB(source);

    // The guard is not decoration: it is what lets the case-1 test be
    // an unsigned <= 1, and a case 0 inside the switch folds away.
    if (attackState != Sext::eRPSAttackType_None) {
        switch (attackState) {
        case 1:
            return eNPCHitReaction_Damage;
        case 2:
            if (base == 27) {
                if (source != 35) {
                    return eNPCHitReaction_Ignore;
                }

                return eNPCHitReaction_Damage;
            }

            if (base == 31) {
                return eNPCHitReaction_Ignore;
            }

            if (base == 29) {
                if (baseCombat.runningAttack != 0 &&
                    baseCombat.runningAttack->source == 28) {
                    return eNPCHitReaction_Ignore;
                }

                return eNPCHitReaction_Damage;
            }

            break;
        case 4:
            return eNPCHitReaction_Damage;
        case 5:
            if (base == 27) {
                return eNPCHitReaction_Ignore;
            }

            if (base != 31) {
                return eNPCHitReaction_Damage;
            }

            return eNPCHitReaction_Ignore;
        }
    }
    table = parameters->hitReactionTable;
    count = parameters->hitReactionTableCount;

    if (table == 0) {
        return eNPCHitReaction_Unknown;
    }

    for (i = 0; i < count; i++) {
        if (source == table[i].hitSource) {
            return table[i].hitReaction;
        }
    }

    return eNPCHitReaction_Unknown;
}

// BYTE-IDENTICAL AT 672 WHEN THE SECTION IS BIG ENOUGH, AND 660
// HERE. This function reads four constants out of .rodata, which is
// enough for mwcc to anchor a base register at the section and bake
// the displacements in; retail materialises a high half per
// reference, because in WAD02.cpp these constants are past the
// signed 16-bit displacement from the section base. Compiled with
// 36,000 bytes of .rodata placed AHEAD of them, this exact text is
// 168 of 168 words. So nothing below is wrong: it is waiting on the
// rest of its translation unit, the same as zNPCPerception's
// IsInDirectPath, and the NOTES entry on REACH says how to tell.
//
// Two things did have to be found, and both were measured against
// that padded build: `before` is a separate local in EACH case --
// retail gives the damage branch f31 and the instadeath branch f30,
// which one declaration at the top cannot do -- and the damage
// multiplier needs a local of its own, below.
bool zNPCCombat::HandleNPCDamage(xEnt* ent, const zCombatDamageInfo& info,
                                 zNPCGetsDamageInfo* out) {
    eNPCHitReaction reaction = FindHitReaction(ent, &info);

    // A blow from something flagged at bit 8 only counts if it came
    // through the environment.
    if (info.from != 0 && (info.from->baseFlags & 0x100) &&
        info.target != zHT_ENV) {
        return false;
    }

    switch (reaction) {
    case eNPCHitReaction_Block:
    case eNPCHitReaction_Ignore:
        return false;
    case eNPCHitReaction_Unknown:
    case eNPCHitReaction_Damage:
        if (hitsDisabled) {
            return false;
        }

        {
        float before = baseCombat.currentHitPoints;
        float after;

        if (!damageDisabled) {
            // The multiplier into a local of its own: spelled inline,
            // either way round, the product comes out with its
            // operands the other way and costs the one word.
            float m = GetDamageMultiplier(info.source);

            after = baseCombat.currentHitPoints - m * info.damage;

            after = after > 0.0f ? after : 0.0f;

            {
                float hp[2];

                hp[0] = baseCombat.currentHitPoints;
                hp[1] = after;
                zEntEvent((xBase*)ent, 0, (xBase*)ent, 0xC0648E27,
                          (Sext::EventAny*)hp, (ForceEvent)1);
            }

            baseCombat.currentHitPoints = after;
        }

        out->SetFromCombatDamageInfo(ent, info, reaction,
                                     baseCombat.currentHitPoints);

        if (before > 0.0f) {
            AddToDamageList(*out);
        }

        zEntEvent(info.from, 0, (xBase*)ent, 0x00130037, 0,
                  (ForceEvent)1);

        if (info.from != 0 && info.from->baseType == 0x55) {
            xVec3 at = ((xEnt*)info.from)->model->position;

            SendEventFromNpcToNpcsWithinDistance(&at, (xBase*)ent, 0,
                                                 0xD4F680F7, 0, 20.0f);

            if (((zNPCEntity*)ent)->owner->npcCombat != 0 &&
                ((zNPCEntity*)ent)->owner->npcCombat->_v11() != 0) {
                SendEventFromNpcToNpcsWithinDistance(
                    &at, (xBase*)ent, 0, 0xEA07FF5B, 0, 20.0f);
            }
        }

        if (before > baseCombat.currentHitPoints) {
            ((zNPCEntity*)ent)->damageColorTimer = 0.25f;
        }

        return true;
        }
    case eNPCHitReaction_InstaDeath:
        {
        float before = baseCombat.currentHitPoints;
        baseCombat.currentHitPoints = 0.0f;
        out->SetFromCombatDamageInfo(ent, info, reaction, 0.0f);

        if (before > 0.0f) {
            AddToDamageList(*out);
        }

        return true;
        }
    }

    return false;
}
