// zPlayerTemplate.cpp -- three functions, read from the image with
// tools/disasm.py. A player template turns two assets into the numbers
// the player runs on. Setup keeps the template asset, looks its combat
// asset up through the entity manager, builds the attack table from it,
// takes the hit points from it or one when there is none, and then the
// jump parameters. InitAttackTable allocates one 216-byte entry per
// attack (tag 2), and for each one clears it, copies the state and the
// three times, copies as many hit spheres as the attack names into the
// entry's six hit-bone slots -- a sphere's bone, offset and radius, in
// the order the destination wants them -- fills the bone of every slot
// the attack did not name with 0xFFFF, and then the damage, impact,
// source, flags, chain state and hit filter. With no combat asset it
// clears the table pointer and its count instead. InitJumpParams takes
// the height and air time from the asset or a default pair, and derives
// the apex time, the initial velocity and the deceleration from them.
//
// Layouts from the DWARF (tools/dwarf_types.py): zPlayerTemplate 0x24 --
// the asset at +0, zCombatParams at +4 (hit points, table, count) and
// zJumpParams at +0x10 (height, time, apex, velocity, deceleration);
// Sext::PlayerTemplate 0x50 with its combat asset uid at +0x20 and its
// JumpData at +0x48; Sext::NPC_Combat 0x1C with the hit points at +0 and
// the attack array's count and pointer at +0xC and +0x10;
// Sext::CombatAttack 0x50 with the sphere array at +0x10 and the rest of
// the numbers from +0x18; zCombatAttack 0xD8 with six hitBoneInfo at
// +0x10, each 0x14. The sphere the DWARF leaves untyped is 20 bytes and
// the bytes read it as a bone, an offset and a radius.
//
// Four shapes the bytes fixed. The loop bound is read from the ENTITY's
// own count, not from the asset's, which is why retail reloads
// this->attackTableSize every iteration. The empty case is the ELSE at
// the end of InitAttackTable, not an early return at the top: written
// as a return the whole function shifts and 88 of 105 words differ.
// The table, the source attack and the destination entry are declared
// above the loop counter, in that order, which is the order their
// callee-saved registers run down from r30. And the fill of the
// unnamed slots counts with its OWN variable, initialised from the
// sphere count -- continuing the first loop's counter is the same
// value and costs six words, because retail's fill increments the
// register the count was loaded into.
//
// NEAR MISS, InitJumpParams at 24 of 26 words. Retail loads the
// height, stores it, loads the air time and stores that, using one
// register twice; ours loads both before storing either, which is
// three words in a different order and nothing else. Tried and no
// better: one reused float local, a const float* over the pair, a
// JumpData copied whole (23), two separate ifs (23), two conditional
// expressions (18), the null case first (20), a volatile view on the
// second read, and a volatile view on both stores. What is left to
// find is what stops the compiler hoisting the second load above the
// first store, since both are floats and it has decided they cannot
// alias.

#include "SB/GM/Engine/Game/zPlayerTemplate.pool.h"

typedef unsigned long long uid;

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {
enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };
}

extern "C" void* memset(void* dst, int c, unsigned long n);

void* xMemAlloc(Memory::GlobalHeapEnum heap, unsigned int size, int align,
                eMemMgrTag tag);

class xVec3 {
public:
    float x;
    float y;
    float z;
};

namespace Sext {

class xBaseAsset;

// The sphere the attack names: a bone, an offset and a radius. The DWARF
// gives the array a count and an untyped pointer, and the bytes give the
// twenty bytes this shape.
class CombatSphere {
public:
    unsigned short bone;
    unsigned short pad;
    xVec3 offset;
    float radius;
};

class CombatAttack {
public:
    unsigned int Name;
    float StartTime;
    float EndTime;
    float Radius;
    unsigned int sphereCount;
    CombatSphere* spheres;
    float Damage;
    unsigned int SourceType;
    float Impact;
    unsigned int AttackFlags;
    unsigned int ChainAttackState;
    unsigned int HitFilter;
    unsigned char _pad0[0x50 - 0x30];
};

class NPC_Combat {
public:
    float HitPoints;
    unsigned char _pad0[0x8];
    unsigned int attackCount;
    CombatAttack* attacks;
    unsigned char _pad1[0x1C - 0x14];
};

class JumpData {
public:
    float Height;
    float AirTime;
};

class PlayerTemplate {
public:
    unsigned char _pad0[0x20];
    uid CombatAsset;
    unsigned char _pad1[0x48 - 0x28];
    JumpData JumpParameter;
};

}  // namespace Sext

namespace World {

class EntityManager {
public:
    static Sext::xBaseAsset* FindAsset(uid id);
};

EntityManager* GetEntityManager();

}  // namespace World

class hitBoneInfo {
public:
    float radius;
    unsigned short bone;
    unsigned short pad;
    xVec3 boneOffset;
};

class zCombatAttack {
public:
    unsigned int state;
    float attackStart;
    float attackEnd;
    float attackRadius;
    hitBoneInfo hitBones[6];
    float damage;
    unsigned int hitFilter;
    unsigned short flags;
    unsigned short pad;
    unsigned int source;
    bool hitsBSP;
    unsigned char _pad0[0x3];
    float impact;
    unsigned char _pad1[0xAC - 0xA0];
    unsigned int chainStateID;
    unsigned char _pad2[0xD8 - 0xB0];
};

class zCombatParams {
public:
    float hitPoints;
    zCombatAttack* attackTable;
    unsigned int attackTableSize;
};

class zJumpParams {
public:
    float height;
    float time;
    float timeApex;
    float initialVelocity;
    float deceleration;
};

class zPlayerTemplate {
public:
    void Setup(Sext::PlayerTemplate* asset);
    void InitAttackTable(const Sext::NPC_Combat* combat);
    void InitJumpParams(const Sext::PlayerTemplate* asset);

    Sext::PlayerTemplate* templateAsset;
    zCombatParams combatParams;
    zJumpParams jumpParams;
};

void zPlayerTemplate::Setup(Sext::PlayerTemplate* asset) {
    uid combatAsset;
    Sext::NPC_Combat* combat = 0;

    templateAsset = asset;

    if (asset) {
        combatAsset = asset->CombatAsset;

        combat = (Sext::NPC_Combat*)World::GetEntityManager()->FindAsset(
            combatAsset);
    }

    InitAttackTable(combat);

    if (combat) {
        combatParams.hitPoints = combat->HitPoints;
    } else {
        combatParams.hitPoints = 1.0f;
    }

    InitJumpParams(asset);
}

void zPlayerTemplate::InitAttackTable(const Sext::NPC_Combat* combat) {
    zCombatAttack* table;
    const Sext::CombatAttack* attack;
    zCombatAttack* entry;
    unsigned int i;
    unsigned int j;
    unsigned int k;

    if (combat) {
        combatParams.attackTableSize = combat->attackCount;

        table = (zCombatAttack*)xMemAlloc(
            (Memory::GlobalHeapEnum)0,
            combat->attackCount * sizeof(zCombatAttack), 0, (eMemMgrTag)2);

        for (i = 0; i < combatParams.attackTableSize; i++) {
            entry = &table[i];
            attack = &combat->attacks[i];

            memset(entry, 0, sizeof(zCombatAttack));

            entry->state = attack->Name;
            entry->attackStart = attack->StartTime;
            entry->attackEnd = attack->EndTime;
            entry->attackRadius = attack->Radius;

            for (j = 0; j < attack->sphereCount; j++) {
                entry->hitBones[j].radius = attack->spheres[j].radius;
                entry->hitBones[j].bone = attack->spheres[j].bone;
                entry->hitBones[j].boneOffset.x = attack->spheres[j].offset.x;
                entry->hitBones[j].boneOffset.y = attack->spheres[j].offset.y;
                entry->hitBones[j].boneOffset.z = attack->spheres[j].offset.z;
            }

            for (k = attack->sphereCount; k < 6; k++) {
                entry->hitBones[k].bone = 0xFFFF;
            }

            entry->damage = attack->Damage;
            entry->impact = attack->Impact;
            entry->source = attack->SourceType;
            entry->flags = attack->AttackFlags;
            entry->chainStateID = attack->ChainAttackState;
            entry->hitFilter = attack->HitFilter;
        }

        combatParams.attackTable = table;
    } else {
        combatParams.attackTable = 0;
        combatParams.attackTableSize = 0;
    }
}

void zPlayerTemplate::InitJumpParams(const Sext::PlayerTemplate* asset) {
    if (asset) {
        jumpParams.height = asset->JumpParameter.Height;
        jumpParams.time = asset->JumpParameter.AirTime;
    } else {
        jumpParams.height = 2.1f;
        jumpParams.time = 0.775f;
    }

    jumpParams.timeApex = 0.5f * jumpParams.time;
    jumpParams.initialVelocity =
        jumpParams.height / (jumpParams.timeApex - 0.5f * jumpParams.timeApex);
    jumpParams.deceleration =
        -jumpParams.initialVelocity / jumpParams.timeApex;
}
