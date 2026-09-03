#include "SB/GM/Engine/Game/zNPCBTAction.pool.h"

// zNPCBTAction.cpp -- five functions, read from the image with
// tools/disasm.py. zNPCBTActionAnim is the animation an NPC behaviour
// node plays. SetAnimation turns a move type into one of six pooled
// animation names -- MOSEY, JOG, RUN, SPRINT, STALK, CHARGE -- and
// initialises the action with it; move type 7 takes the caller's own
// name when it has one that is not empty, and everything else, that
// case included when the name is missing, falls through to WALK. Init
// keeps the name, hashes it into the animation state id and keeps the
// two times. StartOnNPC plays the animation on the NPC when the action
// is enabled and either the caller forces it, the NPC's animation has
// stopped, or the NPC is playing some other animation.
// StartInstanceAnimOnNPC is the same guard around the instance-animation
// call, which takes only the id and the blend time.
// zNPCBTStuckRangeMultiplier::Update ages a multiplier while the NPC is
// slower than its stuck threshold and resets it to one when it is not.
//
// Layouts from the DWARF (tools/dwarf_types.py): zNPCBTActionAnim is
// 0x18 -- animationName at +0, animStateID at +4, blendTime at +8,
// animStartTime at +0xC, leanMaxAngle at +0x10 and enabled at +0x14 --
// and zNPCBTStuckRangeMultiplier is 0x10, stuckVel2, multiplierRate,
// maxMultiplier and curMultiplier in that order. Neither has a vtable
// in the image and neither is polymorphic here. xVec3 is 0xC.
//
// The constants are read out of the image, not guessed: @228890 at
// 0x8068BA7C is 0.2f and @227979 at 0x8068BA28 is 0.0f, the blend time
// and start time every SetAnimation case passes, and @228151 at
// 0x8068BA48 is the 1.0f Update resets to. The seven strings are the
// ones this file is first to reference in WAD02's 5,231-byte pool,
// MOSEY at +2937 through WALK at +2971, so the generated pool header
// goes first; it also carries the 50,872 bytes of .rodata the files in
// front put ahead of the literals, which is what gives each literal its
// own `lis` instead of one shared base.
//
// Four shapes the bytes fixed.
//
// The move-type dispatch is a `switch` whose default is written LAST,
// because case 7 FALLS THROUGH into it: retail's case-7 body branches to
// the default's body on either half of its guard failing, and only a
// fall-through reaches a default from a case. An if/else chain over the
// same seven values is 91 of 105 words; a switch whose case 7 carries
// its own copy of the WALK call aligns all 105 of retail's but is 114
// words, nine longer, so it is not the same function.
//
// Init is defined in the class body -- retail's symbol is weak, which is
// what an in-class definition gives -- and is not inlined into its six
// call sites, since with these flags no user function of four stores or
// more is.
//
// The two start guards are one `||` condition, so each of the first two
// tests branches FORWARD to the call and only the last one branches past
// it.
//
// And Update's clamp is a conditional EXPRESSION whose true side is the
// value already in f1, which is what gives retail's branch-to-the-else-
// then-jump-past pair. An `if` that overwrites the local is 31 of 33 and
// one word short, an if/else storing the member on both sides 31 of 33,
// and the conditional with its operands the other way round 28 of 33.
//
// NO NEAR MISS. `python tools/unitcmp.py
// SB/GM/Engine/Game/zNPCBTAction.cpp` reports 5 of 5 byte-identical,
// 904 bytes, which is every function the split holds. That is unitcmp's
// answer and not the oracle's: the unit has not been through ninja, is
// not marked Matching in configure.py, and it is compared through a
// pool header rather than linked.

extern "C" unsigned long strlen(const char* s);

unsigned int xStrHash(const char* s);

class zNPCEntity;

class xVec3 {
public:
    float length2() const;

    float x;
    float y;
    float z;
};

namespace Sext {

enum eNPCMoveType { eNPCMoveType_ = 0x7FFFFFFF };

}  // namespace Sext

class zNPCEntity {
public:
    unsigned int GetCurAnimID();
    bool IsAnimationStopped(unsigned int which);
    void SetAnimState(unsigned int id, float blend, float start,
                      const char* name);
    void SetInstanceAnimState(unsigned int id, float blend);
};

class zNPCBTActionAnim {
public:
    void SetAnimation(Sext::eNPCMoveType moveType, const char* name);

    // In the class body, which is what makes retail's symbol weak.
    void Init(const char* name, float blend, float start) {
        animationName = (char*)name;
        animStateID = xStrHash(name);
        blendTime = blend;
        animStartTime = start;
    }

    void StartOnNPC(zNPCEntity* npc, bool force);
    void StartInstanceAnimOnNPC(zNPCEntity* npc, bool force);

    char* animationName;
    unsigned int animStateID;
    float blendTime;
    float animStartTime;
    float leanMaxAngle;
    bool enabled;
};

class zNPCBTStuckRangeMultiplier {
public:
    void Update(const xVec3& vel, float dt);

    float stuckVel2;
    float multiplierRate;
    float maxMultiplier;
    float curMultiplier;
};

void zNPCBTActionAnim::SetAnimation(Sext::eNPCMoveType moveType,
                                    const char* name) {
    switch (moveType) {
    case 0:
        Init("MOSEY", 0.2f, 0.0f);
        break;
    case 2:
        Init("JOG", 0.2f, 0.0f);
        break;
    case 3:
        Init("RUN", 0.2f, 0.0f);
        break;
    case 4:
        Init("SPRINT", 0.2f, 0.0f);
        break;
    case 5:
        Init("STALK", 0.2f, 0.0f);
        break;
    case 6:
        Init("CHARGE", 0.2f, 0.0f);
        break;
    case 7:
        if (name != 0 && strlen(name) != 0) {
            Init(name, 0.2f, 0.0f);

            break;
        }

        // falls through to the default
    default:
        Init("WALK", 0.2f, 0.0f);
        break;
    }
}

void zNPCBTActionAnim::StartOnNPC(zNPCEntity* npc, bool force) {
    if (enabled) {
        if (force || npc->IsAnimationStopped(0) ||
            animStateID != npc->GetCurAnimID()) {
            npc->SetAnimState(animStateID, blendTime, animStartTime,
                              animationName);
        }
    }
}

void zNPCBTActionAnim::StartInstanceAnimOnNPC(zNPCEntity* npc, bool force) {
    if (enabled) {
        if (force || npc->IsAnimationStopped(0) ||
            animStateID != npc->GetCurAnimID()) {
            npc->SetInstanceAnimState(animStateID, blendTime);
        }
    }
}

void zNPCBTStuckRangeMultiplier::Update(const xVec3& vel, float dt) {
    if (vel.length2() < stuckVel2) {
        float next = curMultiplier + multiplierRate * dt;

        curMultiplier = next < maxMultiplier ? next : maxMultiplier;
    } else {
        curMultiplier = 1.0f;
    }
}
