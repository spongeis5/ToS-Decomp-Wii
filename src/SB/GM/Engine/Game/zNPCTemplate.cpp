// zNPCTemplate.cpp -- five functions, read from the image with
// tools/disasm.py. An NPC's runtime template: the numbers a spawned NPC
// reads out of its asset once, unpacked into a form the game uses every
// frame. Setup keeps the asset, fills the movement table, takes the hover
// height, finds the combat asset by uid and builds the attack table from
// it -- copying the hit spots and damage multipliers across, or falling
// back to one hit point when the asset is missing -- then finds the
// counter-pattern asset the same way and copies its rank and its counter
// moves, converts the four head-tracking limits from degrees to radians
// (negating the two minima), and takes the jaw-flapping bones.
// InitAttackTable allocates one zCombatAttack per asset attack from the
// global heap, zeroes each, copies the timing, the hit spheres (marking
// the unused ones with bone 0xFFFF), the damage, the flags and the rumble
// block; with no combat asset it leaves the table null and empty.
// InitMoveTable is seven calls to SetupMoveEntry, one per movement style,
// and SetupMoveEntry copies a MovementType's four floats into a MoveData.
// GetCollectibleSpawner walks the asset's collectible list for a matching
// type and tail-calls zSceneFindObject on that entry's uid.
//
// Layouts from the DWARF (tools/dwarf_types.py): zNPCTemplate 0x104 --
// templateAsset at +0, moveData[8] at +4, hoverHeight +0x84, rank +0x88,
// counterPattern +0x8C, combatParams +0xB0, headTrackParams +0xD4,
// jawFlappingParams +0xF8. Sext::NPCTemplate is 0x128 with the head-track
// block at +0x44..+0x68, the jaw bones at +0x68, the two uids at +0x70
// and +0x78, HoverHeight at +0x8C, the seven MovementTypes at +0x9C and
// the collectible list at +0x10C. Sext::NPC_Combat is 0x1C,
// Sext::NPCCounterPattern 0xC, Sext::CombatAttack 0x50 and zCombatAttack
// 0xD8 with hitBones[6] at +0x10 and the rumble block at +0xB0. The three
// literals are 1.0f, 3.1415927f and 180.0f, read out of the image at
// 8068BA48, 8068BAF4 and 8068BAF8.
//
// Three element types are NOT in the DWARF and were read off the bytes,
// because the asset arrays are all `Pointer32`, a bare unsigned int with
// no element type behind it: the attack's hit sphere (20 bytes -- a bone
// index at +0, an offset vec3 at +4 and a radius at +0x10), the counter
// move (8 bytes -- a pattern id and a reaction time) and the collectible
// spawner (16 bytes -- a type at +0 and an 8-aligned uid at +8). They are
// named here for what the code does with them and the SIZES are the
// recovered fact: 20, 8 and 16 come from the strides the loops walk.
//
// Five shapes the bytes fixed, and dwarf_locals.py and dwarf_lines.py
// settled three of them before anything was compiled.
//
// World::EntityManager::FindAsset is STATIC and is still called through
// the manager. Both call sites run GetEntityManager, throw its result
// away and put the uid in r3:r4, which is exactly what a static member
// called through an object expression compiles to -- the object is
// evaluated for its side effects and discarded.
//
// InitAttackTable's inner loop has TWO variables named j (dwarf_locals
// gives them lines 113 and 121, both unsigned), so the fill loop that
// stamps 0xFFFF into the unused hit spheres re-reads the sphere count
// rather than carrying the first loop's counter -- which is why retail
// loads it again into r5 and computes the byte offset from it.
//
// The asset side of every copy is re-read after each store: the sphere
// array pointer is loaded five times in five statements and the attack
// array pointer once per outer iteration. A store through the table can
// alias a read through the asset, and the plain member spelling is what
// gives that; hoisting either into a local is smaller code that does not
// match.
//
// The head-tracking conversion is a real division. `PI * -x / 180.0f`
// keeps the fneg, the fmuls and the fdivs retail has, where folding the
// constant would lose the divide.
//
// SetupMoveEntry is declared in zNPCTemplate.h, not the .cpp -- that is
// dwarf_lines.py's answer, and it is a header function emitted out of
// line into this translation unit. It has four stores, which is the
// inliner's floor, so InitMoveTable calls it seven times rather than
// expanding it.
//
// NEAR MISS, two of the five, and neither was carried further: the
// session that wrote this unit was cut off. InitAttackTable is 71 of 119
// words and Setup 104 of 112. zPlayerTemplate.cpp beside it is the same
// pair of functions for the player and is now exact, so its header is
// where to start: the empty case is the ELSE at the end and not an early
// return, the table, the source attack and the destination entry are
// declared above the loop counter in that order, and the fill of the
// unnamed slots counts with its own variable initialised from the sphere
// count. No spelling has been tried and rejected here.
//

#include "SB/GM/Engine/Game/zNPCTemplate.pool.h"

typedef unsigned long long uid;

class xBase;
class zNPCBase;
class zNPCHitReactionTable;

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {
enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };
}

void* xMemAlloc(Memory::GlobalHeapEnum heap, unsigned int size, int align,
                eMemMgrTag tag);

extern "C" void* memset(void* dst, int c, unsigned long n);

xBase* zSceneFindObject(uid id);

namespace World {

class EntityManager {
public:
    static void* FindAsset(uid id);
};

EntityManager& GetEntityManager();

}  // namespace World

class xVec3 {
public:
    float x;
    float y;
    float z;
};

namespace Sext {

enum eHitSource { eHitSource_ = 0x7FFFFFFF };

enum eNPCCollectibleType {
    OnDeath = 0,
    ShakeVertical = 1,
    ShakeHorizontal = 2,
    OnPlanktonDeath = 3,
    OnSpinAttackDeath = 4,
    OnHammerAttackDeath = 5,
    OnPuckAttackDeath = 6,
    END_eNPCCollectibleTypeENUM = 7
};

class vec3 {
public:
    float x;
    float y;
    float z;
};

class Pointer32 {
public:
    unsigned int _p;
};

// Read off the bytes: the attack's hit sphere array walks 20 at a time
// and touches +0 as a halfword, +4/+8/+0xC as floats and +0x10 as one.
class AttackSphere {
public:
    unsigned short Bone;
    unsigned short pad;
    vec3 Offset;
    float Radius;
};

class __Spheres__ {
public:
    unsigned int count;
    Pointer32 data;
};

class SBScreenShakeData {
public:
    float MaxScreenShake;
    float MaxScreenShakeDist;
    float MinScreenShake;
    float MinScreenShakeDist;
};

class RumbleEmitterData {
public:
    uid RumbleEmitterID;
    float RumbleStartTime;
    SBScreenShakeData SBScreenShake;
};

class CombatAttack {
public:
    unsigned int Name;
    float StartTime;
    float EndTime;
    float Radius;
    __Spheres__ Spheres;
    float Damage;
    eHitSource SourceType;
    float Impact;
    unsigned int AttackFlags;
    unsigned int ChainAttackState;
    unsigned int HitFilter;
    RumbleEmitterData RumbleEmitter;
};

class __Attacks__ {
public:
    unsigned int count;
    Pointer32 data;
};

class __HitSpots__ {
public:
    unsigned int count;
    Pointer32 data;
};

class __DamageMultipliers__ {
public:
    unsigned int count;
    Pointer32 data;
};

class NPC_Combat {
public:
    float HitPoints;
    __DamageMultipliers__ DamageMultipliers;
    __Attacks__ Attacks;
    __HitSpots__ HitSpots;
};

// Read off the bytes: the counter list walks 8 at a time, a word then a
// float.
class Counter {
public:
    int Pattern;
    float TimeToReact;
};

class __Counters__ {
public:
    unsigned int count;
    Pointer32 data;
};

class NPCCounterPattern {
public:
    unsigned int Rank;
    __Counters__ Counters;
};

// Read off the bytes: the collectible list walks 16 at a time, the type
// at +0 and an 8-aligned uid at +8.
class Collectible {
public:
    eNPCCollectibleType Type;
    unsigned int pad;
    uid SpawnerID;
};

class __Collectibles__ {
public:
    unsigned int count;
    Pointer32 data;
};

class NPCTemplate {
public:
    class MovementType {
    public:
        float MaxSpeed;
        float MaxAcceleration;
        float TurningRadius;
        float TurnSpring;
    };

    uid ChrAssetID;
    float ModelScaleMult;
    unsigned char CollisionType;
    unsigned char CollisionShape;
    bool CollisionRotWithNPC;
    unsigned char CollisionMainAxis;
    bool CollideWithPlayer;
    unsigned char _pad0[0x14 - 0x11];
    vec3 BoundOffset;
    vec3 BoundScaleV;
    unsigned int UserData;
    uid BehaviorSet;
    unsigned int flags;
    bool PlanktonTargetable;
    unsigned char _pad1[0x40 - 0x3D];
    int PlanktonTargetBone;
    bool EnableHeadTracking;
    unsigned char _pad2[0x48 - 0x45];
    int FirstHeadBone;
    int LastHeadBone;
    int FacingDirBone;
    float MinPitch;
    float MaxPitch;
    float MinYaw;
    float MaxYaw;
    float StopTrackDistance;
    bool EnableJawFlapping;
    unsigned char _pad3[0x6A - 0x69];
    short FirstBone;
    short LastBone;
    unsigned char _pad4[0x70 - 0x6E];
    uid CombatAsset;
    uid NPCCounterPattern;
    bool IsBoss;
    unsigned char _pad5[0x84 - 0x81];
    float VisibleRangeDEPRECATED;
    float AttackRangeDEPRECATED;
    float HoverHeight;
    unsigned char MovementStyle;
    unsigned char _pad6[0x94 - 0x91];
    float LeanSpringTension;
    float LeanRange;
    MovementType MoseyData;
    MovementType WalkData;
    MovementType JogData;
    MovementType RunData;
    MovementType SprintData;
    MovementType StalkData;
    MovementType ChargeData;
    __Collectibles__ Collectibles;
    unsigned char _pad7[0x118 - 0x114];
    uid CarlAnims;
    uid RussellAnims;
};

}  // namespace Sext

class HitSpot {
public:
    unsigned char _pad0[0x30];
};

class DamageMultiplier;

class hitBoneInfo {
public:
    float radius;
    unsigned short bone;
    unsigned short pad;
    xVec3 boneOffset;
};

class RumbleEffectParams {
public:
    float startTime;
    unsigned int pad;
    uid emitterID;
    float SB_NMEHammerRumbleMax;
    float SB_NMEHammerRumbleMin;
    float SB_NMEHammerRumbleMaxDist;
    float SB_NMEHammerRumbleMinDist;
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
    unsigned short pad0;
    Sext::eHitSource source;
    bool hitsBSP;
    unsigned char pad1[3];
    float impact;
    unsigned short effect;
    unsigned short hitEffect;
    float effectStart;
    float effectEnd;
    unsigned int chainStateID;
    RumbleEffectParams rumbleEffect;
    void (*hitCB)();
};

class zNPCCounterPatternEntry {
public:
    int pattern;
    float timeToReact;
};

class zNPCCounterPattern {
public:
    int numCounterMoves;
    zNPCCounterPatternEntry counterMoves[4];
};

class zNPCCombatParams {
public:
    float hitPoints;
    zCombatAttack* attackTable;
    unsigned int attackTableSize;
    HitSpot* hitSpots;
    unsigned int hitSpotCount;
    zNPCHitReactionTable* hitReactionTable;
    unsigned int hitReactionTableCount;
    DamageMultiplier* damageMultipliers;
    unsigned int damageMultiplierCount;
};

class zNPCHeadTrackParams {
public:
    int firstBone;
    int lastBone;
    int facingDirBone;
    float minPitch;
    float maxPitch;
    float minYaw;
    float maxYaw;
    float stopTrackingDistance;
    bool stopTrackingWhenClipped;
    unsigned char pad[3];
};

class zNPCJawFlappingParams {
public:
    bool enableJawFlapping;
    unsigned char pad[3];
    int firstBone;
    int lastBone;
};

class zNPCTemplate {
public:
    class MoveData {
    public:
        float maxSpeed;
        float maxAcceleration;
        float turningRadius;
        float turnSpring;
    };

    void Setup(Sext::NPCTemplate* asset, zNPCBase* npc);
    void InitAttackTable(const Sext::NPC_Combat* combatAsset);
    void InitMoveTable(const Sext::NPCTemplate* asset);
    void SetupMoveEntry(const Sext::NPCTemplate::MovementType& src,
                        MoveData& dst);
    xBase* GetCollectibleSpawner(Sext::eNPCCollectibleType type) const;

    Sext::NPCTemplate* templateAsset;
    MoveData moveData[8];
    float hoverHeight;
    int rank;
    zNPCCounterPattern counterPattern;
    zNPCCombatParams combatParams;
    zNPCHeadTrackParams headTrackParams;
    zNPCJawFlappingParams jawFlappingParams;
};

void zNPCTemplate::Setup(Sext::NPCTemplate* asset, zNPCBase* npc) {
    templateAsset = asset;

    InitMoveTable(asset);

    hoverHeight = asset->HoverHeight;

    // <<SWEEPA
    uid combatID = asset->CombatAsset;
    Sext::NPC_Combat* combatAsset =
        (Sext::NPC_Combat*)World::GetEntityManager().FindAsset(combatID);
    InitAttackTable(combatAsset);
    if (combatAsset) {
        combatParams.hitSpots = (HitSpot*)combatAsset->HitSpots.data._p;
        combatParams.hitSpotCount = combatAsset->HitSpots.count;
        combatParams.damageMultipliers =
            (DamageMultiplier*)combatAsset->DamageMultipliers.data._p;
        combatParams.damageMultiplierCount =
            combatAsset->DamageMultipliers.count;

        combatParams.hitPoints = combatAsset->HitPoints;
    } else {
        combatParams.hitPoints = 1.0f;
    }
    // SWEEPA>>

    uid counterID = asset->NPCCounterPattern;

    rank = 0;
    counterPattern.numCounterMoves = 0;
    if (counterID != 0) {
        Sext::NPCCounterPattern* counterAsset =
            (Sext::NPCCounterPattern*)World::GetEntityManager().FindAsset(
                counterID);
        rank = counterAsset->Rank;
        counterPattern.numCounterMoves = counterAsset->Counters.count;

        for (int i = 0; i < counterPattern.numCounterMoves; i++) {
            counterPattern.counterMoves[i].pattern =
                ((Sext::Counter*)counterAsset->Counters.data._p)[i].Pattern;
            counterPattern.counterMoves[i].timeToReact =
                ((Sext::Counter*)counterAsset->Counters.data._p)[i].TimeToReact;
        }
    }

    if (asset->EnableHeadTracking) {
        headTrackParams.firstBone = asset->FirstHeadBone;
        headTrackParams.lastBone = asset->LastHeadBone;
        headTrackParams.facingDirBone = asset->FacingDirBone;
        headTrackParams.minPitch = 3.1415927f * -asset->MinPitch / 180.0f;
        headTrackParams.maxPitch = 3.1415927f * asset->MaxPitch / 180.0f;
        headTrackParams.minYaw = 3.1415927f * -asset->MinYaw / 180.0f;
        headTrackParams.maxYaw = 3.1415927f * asset->MaxYaw / 180.0f;
        headTrackParams.stopTrackingDistance = asset->StopTrackDistance;
    }

    jawFlappingParams.enableJawFlapping = asset->EnableJawFlapping;
    jawFlappingParams.firstBone = asset->FirstBone;
    jawFlappingParams.lastBone = asset->LastBone;
}

void zNPCTemplate::InitAttackTable(const Sext::NPC_Combat* combatAsset) {
    if (combatAsset) {
        combatParams.attackTableSize = combatAsset->Attacks.count;

        zCombatAttack* attackTable = (zCombatAttack*)xMemAlloc(
            (Memory::GlobalHeapEnum)0,
            combatParams.attackTableSize * sizeof(zCombatAttack), 0,
            (eMemMgrTag)2);

        for (unsigned int i = 0; i < combatParams.attackTableSize; i++) {
            const Sext::CombatAttack& attack =
                ((const Sext::CombatAttack*)combatAsset->Attacks.data._p)[i];

            memset(&attackTable[i], 0, sizeof(zCombatAttack));

            attackTable[i].state = attack.Name;
            attackTable[i].attackStart = attack.StartTime;
            attackTable[i].attackEnd = attack.EndTime;
            attackTable[i].attackRadius = attack.Radius;
            for (unsigned int j = 0; j < attack.Spheres.count; j++) {
                attackTable[i].hitBones[j].radius =
                    ((Sext::AttackSphere*)attack.Spheres.data._p)[j].Radius;
                attackTable[i].hitBones[j].bone =
                    ((Sext::AttackSphere*)attack.Spheres.data._p)[j].Bone;
                attackTable[i].hitBones[j].boneOffset.x =
                    ((Sext::AttackSphere*)attack.Spheres.data._p)[j].Offset.x;
                attackTable[i].hitBones[j].boneOffset.y =
                    ((Sext::AttackSphere*)attack.Spheres.data._p)[j].Offset.y;
                attackTable[i].hitBones[j].boneOffset.z =
                    ((Sext::AttackSphere*)attack.Spheres.data._p)[j].Offset.z;
            }
            for (unsigned int j = attack.Spheres.count; j < 6; j++) {
                attackTable[i].hitBones[j].bone = 0xFFFF;
            }

            attackTable[i].damage = attack.Damage;
            attackTable[i].impact = attack.Impact;
            attackTable[i].source = attack.SourceType;
            attackTable[i].flags = attack.AttackFlags;
            attackTable[i].chainStateID = attack.ChainAttackState;
            attackTable[i].hitFilter = attack.HitFilter;
            attackTable[i].rumbleEffect.emitterID =
                attack.RumbleEmitter.RumbleEmitterID;
            attackTable[i].rumbleEffect.startTime =
                attack.RumbleEmitter.RumbleStartTime;

            attackTable[i].rumbleEffect.SB_NMEHammerRumbleMax =
                attack.RumbleEmitter.SBScreenShake.MaxScreenShake;
            attackTable[i].rumbleEffect.SB_NMEHammerRumbleMin =
                attack.RumbleEmitter.SBScreenShake.MinScreenShake;
            attackTable[i].rumbleEffect.SB_NMEHammerRumbleMaxDist =
                attack.RumbleEmitter.SBScreenShake.MaxScreenShakeDist;
            attackTable[i].rumbleEffect.SB_NMEHammerRumbleMinDist =
                attack.RumbleEmitter.SBScreenShake.MinScreenShakeDist;
        }

        combatParams.attackTable = attackTable;
    } else {
        combatParams.attackTable = 0;
        combatParams.attackTableSize = 0;
    }
}

void zNPCTemplate::InitMoveTable(const Sext::NPCTemplate* asset) {
    SetupMoveEntry(asset->MoseyData, moveData[0]);
    SetupMoveEntry(asset->WalkData, moveData[1]);
    SetupMoveEntry(asset->JogData, moveData[2]);
    SetupMoveEntry(asset->RunData, moveData[3]);
    SetupMoveEntry(asset->SprintData, moveData[4]);
    SetupMoveEntry(asset->StalkData, moveData[5]);
    SetupMoveEntry(asset->ChargeData, moveData[6]);
}

void zNPCTemplate::SetupMoveEntry(const Sext::NPCTemplate::MovementType& src,
                                  MoveData& dst) {
    dst.maxSpeed = src.MaxSpeed;
    dst.maxAcceleration = src.MaxAcceleration;
    dst.turningRadius = src.TurningRadius;
    dst.turnSpring = src.TurnSpring;
}

xBase* zNPCTemplate::GetCollectibleSpawner(Sext::eNPCCollectibleType type)
    const {
    for (unsigned int i = 0; i < templateAsset->Collectibles.count; i++) {
        if (type ==
            ((Sext::Collectible*)templateAsset->Collectibles.data._p)[i].Type) {
            return zSceneFindObject(
                ((Sext::Collectible*)templateAsset->Collectibles.data._p)[i]
                    .SpawnerID);
        }
    }

    return 0;
}
