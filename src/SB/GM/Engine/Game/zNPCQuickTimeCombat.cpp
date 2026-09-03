
//
// NEAR MISS, two of the four.
//
// RenderCounter is 21 of 24 words, and its one remaining difference
// is a branch form. Two of the four it was short came from the
// hit-point test: retail materialises that comparison into a
// register (fcmpo, cror, mfcr, rlwinm., bnelr) rather than branching
// on the condition register, which is what an if on the comparison
// gives -- the result has to be assigned to a bool LOCAL first, the
// same lever zBTNodeCondition's SelfDone needed. A third came from
// nesting the last two guards inside the attacking test instead of
// leaving them as sequential early returns. What is left: retail
// branches FORWARD to those two guards and falls through to a bare
// blr (bne, blr), where ours returns conditionally (beqlr) and saves
// the word. A goto to a label after an explicit return -- which is
// that layout written out -- is folded back to the same beqlr.
//
// AllAttached is 72 of 83 words and has not been read: the session
// that wrote this unit was cut off, and no spelling has been tried
// and rejected for it. Start from the aligned diff:
//
//   python tools/unitcmp.py SB/GM/Engine/Game/zNPCQuickTimeCombat.cpp
//
#include "SB/GM/Engine/Game/zNPCQuickTimeCombat.pool.h"

// zNPCQuickTimeCombat.cpp -- four functions, 524 bytes, read from the image
// with tools/disasm.py. The quick-time combat component of an NPC: Reset
// clears the fight, AllAttached clears it and then translates the NPC
// template's counter pattern into the component's own counter set,
// PreUpdate ages the reaction timer while an attack is up and hands off to
// RenderCounter, and RenderCounter is a chain of guards with the body it
// guarded compiled out of this build.
//
// Layouts from the DWARF (tools/dwarf_types.py): zNPCQuickTimeCombat 0xC0
// on an 8-byte zNPCComponent, with enabled at +0x8, currentHP at +0xC,
// numCountered and numCounterMove at +0x10 and +0x14, the 0x94-byte
// counter set at +0x18, elapsedTime at +0xAC, attacking at +0xB0,
// attackStatus at +0xB4, currentTimeToReact at +0xB8 and currentCounter at
// +0xBC; zNPCCombatCounterSet is a count and four 0x24-byte patterns, and
// a pattern is a count and four 8-byte entries; zNPCComponent 0x8 with the
// owner at +0 -- the second word is the vtable pointer, which the DWARF
// never describes, so the owner is declared here as a plain base and the
// virtuals in the class on top of it; zNPCBase 0xC0 with npcTemplate at
// +0x78; zNPCTemplate 0x104 with counterPattern, a zNPCCounterPattern, at
// +0x8C, whose entries hold a PLAIN INT pattern where the component's hold
// the enum.
//
// The two float literals are read out of the image: @227979 at 0x8068BA28
// is 0.0f and @228151 at 0x8068BA48 is 1.0f, so the fight opens with one
// hit point. gen_poolprefix.py measured the translation unit's .rodata as
// 50,872 bytes ahead of the first of them, and the generated header
// carries that distance so each literal gets its own `lis` as retail has.
//
// __vt__19zNPCQuickTimeCombat at 806C0198 is ten slots with Reset at 2,
// AllAttached at 3 and PreUpdate at 4 and RenderCounter in none of them --
// which is why PreUpdate reaches it with a direct branch. The class
// declares one UNDEFINED virtual ahead of the three so the vtable's home
// stays in the unity unit's data.
//
// Five shapes the bytes fixed.
//
// AllAttached does NOT call Reset. Its first seven stores are Reset's, in
// Reset's order, followed by four more; with these flags the compiler
// inlines no user function of four stores or more, so retail's bytes say
// the clearing was written out in both.
//
// The template's pattern is copied whole onto the stack before it is read.
// Retail unrolls a 36-byte word copy from npcTemplate + 0x8C into a frame
// slot -- the size of one zNPCCounterPattern -- which is a copy
// INITIALISATION; an assignment to an already-declared local would have
// called the implicit operator= out of line.
//
// The count is read from the copy ONCE and kept. Retail loads
// counterPattern.numCounterMoves into r28, stores it into the counter set
// and then uses that same register as the loop's bound across the calls,
// which is what a frame local whose address is never taken allows.
//
// The pattern translation is a SWITCH over the template's int, and the
// case order in the bytes is 2, 3, 0, 1, 4 -- so the template's enum is
// not the component's. Its values map Up, Down, Left, Right, Random onto
// the component's Up, Down, Left, Right and a random one of the four; the
// template's own enum has no DWARF type, so the case labels are written
// as the numbers the compares hold and named only in this comment. The
// default is materialised before the switch: retail's `li r3,0` sits ahead
// of the compares and every case that does not run leaves it.
//
// RenderCounter's first float guard is MATERIALISED and its last is not:
// retail spells `fcmpo ; cror 2,0,2 ; mfcr ; rlwinm. ; bnelr` for the hit
// points and `fcmpo ; cror 2,1,2 ; bnelr` for the timer.

extern int xrand_RandomRange(int lo, int hi);

class zNPCBase;
class zNPCStatus;

enum eNPCCombatPlayerCounter {
    eNPCCombatPlayerCounter_None = 0,
    eNPCCombatPlayerCounter_Up = 1,
    eNPCCombatPlayerCounter_Down = 2,
    eNPCCombatPlayerCounter_Left = 3,
    eNPCCombatPlayerCounter_Right = 4
};

enum eNPCCombatAttackStatus {
    eNPCCombatAttackStatus_None = 0,
    eNPCCombatAttackStatus_Success = 1,
    eNPCCombatAttackStatus_Fail = 2
};

// The template's side of the same idea. The pattern is a plain int in the
// DWARF and its enum is not in the debug info; the numbers below are the
// values the switch compares against.
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

class zNPCTemplate {
public:
    unsigned char _pad0[0x8C];
    zNPCCounterPattern counterPattern;
    unsigned char _pad1[0x104 - 0xB0];
};

class zNPCBase {
public:
    unsigned char _pad0[0x78];
    zNPCTemplate* npcTemplate;
    unsigned char _pad1[0xC0 - 0x7C];
};

// The component's side.
class zNPCCombatCounterPatternEntry {
public:
    eNPCCombatPlayerCounter pattern;
    float timeToReact;
};

class zNPCCombatCounterPattern {
public:
    int numCounterMoves;
    zNPCCombatCounterPatternEntry counterMoves[4];
};

class zNPCCombatCounterSet {
public:
    int numCounters;
    zNPCCombatCounterPattern counters[4];
};

// zNPCComponent's owner is at +0 and its vtable pointer follows it, so the
// data is a base of its own and the virtuals go on the class above it.
class zNPCComponentData {
public:
    zNPCBase* owner;
};

class zNPCComponent : public zNPCComponentData {
public:
    virtual void _v0();
    virtual void _v1();
    virtual void Reset(const zNPCStatus* status);
    virtual void AllAttached();
    virtual void PreUpdate(float dt);
};

class zNPCQuickTimeCombat : public zNPCComponent {
public:
    // Declared first and left undefined so the vtable's home stays where
    // retail has it, in the unity unit's data.
    virtual void _v0();

    virtual void Reset(const zNPCStatus* status);
    virtual void AllAttached();
    virtual void PreUpdate(float dt);

    void RenderCounter();

    /* +0x8  */ bool enabled;
    /* +0xC  */ float currentHP;
    /* +0x10 */ int numCountered;
    /* +0x14 */ int numCounterMove;
    /* +0x18 */ zNPCCombatCounterSet counterSet;
    /* +0xAC */ float elapsedTime;
    /* +0xB0 */ bool attacking;
    /* +0xB4 */ eNPCCombatAttackStatus attackStatus;
    /* +0xB8 */ float currentTimeToReact;
    /* +0xBC */ eNPCCombatPlayerCounter currentCounter;
};

void zNPCQuickTimeCombat::Reset(const zNPCStatus* status) {
    enabled = false;
    currentHP = 1.0f;
    numCountered = 0;
    numCounterMove = 0;
    elapsedTime = 0.0f;
    attacking = false;
    attackStatus = eNPCCombatAttackStatus_None;
}

void zNPCQuickTimeCombat::AllAttached() {
    enabled = false;
    currentHP = 1.0f;
    numCountered = 0;
    numCounterMove = 0;
    elapsedTime = 0.0f;
    attacking = false;
    attackStatus = eNPCCombatAttackStatus_None;
    currentTimeToReact = 0.0f;
    currentCounter = eNPCCombatPlayerCounter_None;
    counterSet.numCounters = 0;

    if (owner->npcTemplate != 0) {
        zNPCCounterPattern counterPattern = owner->npcTemplate->counterPattern;
        int numCounterMoves;
        int i;

        counterSet.numCounters = 1;
        numCounterMoves = counterPattern.numCounterMoves;
        counterSet.counters[0].numCounterMoves = numCounterMoves;

        for (i = 0; i < numCounterMoves; i++) {
            eNPCCombatPlayerCounter counter = eNPCCombatPlayerCounter_None;

            switch (counterPattern.counterMoves[i].pattern) {
            case 2:
                counter = eNPCCombatPlayerCounter_Up;
                break;
            case 3:
                counter = eNPCCombatPlayerCounter_Down;
                break;
            case 0:
                counter = eNPCCombatPlayerCounter_Left;
                break;
            case 1:
                counter = eNPCCombatPlayerCounter_Right;
                break;
            case 4:
                counter = (eNPCCombatPlayerCounter)xrand_RandomRange(1, 4);
                break;
            }

            counterSet.counters[0].counterMoves[i].pattern = counter;
            counterSet.counters[0].counterMoves[i].timeToReact =
                counterPattern.counterMoves[i].timeToReact;
        }
    }
}

void zNPCQuickTimeCombat::PreUpdate(float dt) {
    if (!enabled) {
        return;
    }

    if (!attacking) {
        return;
    }

    elapsedTime += dt;

    RenderCounter();
}

void zNPCQuickTimeCombat::RenderCounter() {
    if (!enabled) {
        return;
    }

    bool dead = currentHP <= 0.0f;

    if (dead) {
        return;
    }

    if (attacking) {
        if (attackStatus != eNPCCombatAttackStatus_None) {
            return;
        }

        if (elapsedTime >= currentTimeToReact) {
            return;
        }
    }
}
