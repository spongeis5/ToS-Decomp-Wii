// zShootingPlayer.cpp -- hand-owned: the idle tables of
// zPlayerIdleShooting, merged by tools/gen_animtables.py.
//
// This file is a fragment of a unity build: the generated header
// puts the whole string pool in front, so the string offsets baked
// into the code come out as retail has them.
#include "SB/GM/Engine/Game/zShootingPlayer.pool.h"

class xAnimTable;
class xAnimState;
class xAnimPlay;
class xQuat;
class xVec3;
class xAnimSingle;
class xAnimTransition;
unsigned int xAnimTableNewState(xAnimTable* table, const char* name, unsigned int a, unsigned int b, float c, float* d, float* e, float f, unsigned short* g, void* h, void (*i)(xAnimPlay*, xAnimState*, void*), void (*j)(xAnimPlay*, xAnimState*, void*), void (*k)(xAnimState*, xAnimSingle*, void*), void (*l)(xAnimPlay*, xQuat*, xVec3*, xVec3*, int), unsigned long long m, unsigned int n);
unsigned int xAnimTableNewTransition(xAnimTable* table, const char* from, const char* to, unsigned int (*a)(xAnimTransition*, xAnimSingle*, void*), unsigned int (*b)(xAnimTransition*, xAnimSingle*, void*), unsigned int (*c)(xAnimTransition*, xAnimSingle*, void*), unsigned int d, unsigned int e, float f, float g, unsigned short h, unsigned short i, float j, unsigned short* k);

// The action base, as zPlayerAction.cpp declares it: three members and
// then the virtuals, so the vptr sits at +12 and slots 1, 2, 3 are the
// three transition-adding virtuals the tables below dispatch through
// (`lwz r12,12(r3); lwz r12,8|12|20(r12); bctrl`). A table class that
// makes such a call derives from this stub, and reaches the manager's
// action array through `manager`, its first word.
class zPlayer;
class zPlayerActionManager;
class zPlayerAction {
public:
    zPlayerActionManager* manager;
    zPlayer* player;
    void (*doneCB)(zPlayerAction*);

    enum SpecialActions { SpecialActions_ = 0x7FFFFFFF };

    virtual void _v0();
    virtual void AddStandardTransitions(xAnimTable* table, const char* name);
    virtual void AddDefaultTransitions(xAnimTable* table, const char* name);
    virtual void AddTransitions(xAnimTable* table, const char* name,
                                unsigned int (*a)(xAnimTransition*, xAnimSingle*, void*),
                                unsigned int (*b)(xAnimTransition*, xAnimSingle*, void*),
                                unsigned short e, float f, unsigned int g,
                                unsigned int h, SpecialActions i);

    unsigned int NewState(xAnimTable* table, const char* name,
                          unsigned int a, unsigned int b, float c,
                          float* d, float* e, float f, unsigned short* g,
                          void (*h)(xAnimPlay*, xAnimState*, void*),
                          void (*i)(xAnimPlay*, xAnimState*, void*),
                          void (*j)(xAnimState*, xAnimSingle*, void*),
                          void (*k)(xAnimPlay*, xQuat*, xVec3*, xVec3*, int),
                          unsigned int l);
    static unsigned int ActionChange(xAnimTransition*, xAnimSingle*, void*);
    static unsigned int AddActionTransition(
        xAnimTable* table, const char* from, const char* to,
        unsigned int (*a)(xAnimTransition*, xAnimSingle*, void*),
        unsigned int (*b)(xAnimTransition*, xAnimSingle*, void*),
        unsigned int (*c)(xAnimTransition*, xAnimSingle*, void*),
        unsigned short g, float h, unsigned int i, unsigned int j);
};

// The manager, and the three helpers zPlayerAction.cpp defines as
// `actions[id]->...(...)`. Retail's unity build held that file in
// the same translation unit as the tables (WAD03) and -O4 inlined
// them, and the inlined form is what the bytes say: a table written
// as the direct `manager->actions[k]->AddTransitions(...)` creates
// the pool-string temp before the load chain, retail after it --
// zPlayerRunSB 5 of 299 words, zSBPlayerPuckAttack 9 of 164, both
// exact once spelled through the helper. Inline here so the
// fragment inlines them the same way and emits no copy.
class zPlayerActionManager {
public:
    zPlayerAction** actions;

    void AddStandardTransitionsTo(unsigned int id, xAnimTable* table, const char* name);
    void AddDefaultTransitionsTo(unsigned int id, xAnimTable* table, const char* name);
    void AddTransitionsTo(unsigned int id, xAnimTable* table, const char* name,
                          unsigned int (*a)(xAnimTransition*, xAnimSingle*, void*), unsigned int (*b)(xAnimTransition*, xAnimSingle*, void*),
                          unsigned short e, float f, unsigned int g,
                          unsigned int h, zPlayerAction::SpecialActions i);
};

inline void zPlayerActionManager::AddStandardTransitionsTo(unsigned int id, xAnimTable* table, const char* name) {
    actions[id]->AddStandardTransitions(table, name);
}
inline void zPlayerActionManager::AddDefaultTransitionsTo(unsigned int id, xAnimTable* table, const char* name) {
    actions[id]->AddDefaultTransitions(table, name);
}
inline void zPlayerActionManager::AddTransitionsTo(unsigned int id, xAnimTable* table, const char* name,
                                                   unsigned int (*a)(xAnimTransition*, xAnimSingle*, void*), unsigned int (*b)(xAnimTransition*, xAnimSingle*, void*),
                                                   unsigned short e, float f, unsigned int g,
                                                   unsigned int h, zPlayerAction::SpecialActions i) {
    actions[id]->AddTransitions(table, name, a, b, e, f, g, h, i);
}

// zPlayerAction.cpp defines this, and retail's unity build had it in the
// same translation unit as the tables (WAD03: 80105550..801894A0 holds
// both), where -O4's auto-inliner took it: an action table's direct
// xAnimTableNewTransition with ActionChange as its third callback and
// the fixed zeros is this call, its `c == 0` test folded away. Inline
// here so the fragment inlines it the same way and emits no copy.
// NewState is the same story as AddActionTransition below: defined in
// zPlayerAction.cpp, inlined by retail's unity build. A state table
// spelled as the direct xAnimTableNewState with `this` as the owner
// hoists its constants in another order (zPlayerHitSB::AddStates 250
// of 385 words); through the helper, 0.
inline unsigned int zPlayerAction::NewState(
    xAnimTable* table, const char* name, unsigned int a, unsigned int b,
    float c, float* d, float* e, float f, unsigned short* g,
    void (*h)(xAnimPlay*, xAnimState*, void*),
    void (*i)(xAnimPlay*, xAnimState*, void*),
    void (*j)(xAnimState*, xAnimSingle*, void*),
    void (*k)(xAnimPlay*, xQuat*, xVec3*, xVec3*, int), unsigned int l) {
    return xAnimTableNewState(table, name, a, b, c, d, e, f, g, this,
                              h, i, j, k, 0, l);
}
inline unsigned int zPlayerAction::AddActionTransition(
    xAnimTable* table, const char* from, const char* to,
    unsigned int (*a)(xAnimTransition*, xAnimSingle*, void*),
    unsigned int (*b)(xAnimTransition*, xAnimSingle*, void*),
    unsigned int (*c)(xAnimTransition*, xAnimSingle*, void*),
    unsigned short g, float h, unsigned int i, unsigned int j) {
    if (c == 0) {
        c = ActionChange;
    }

    return xAnimTableNewTransition(table, from, to, a, b, c, i, j, 0.0f,
                                   0.0f, g, 0, h, 0);
}

class zPlayerIdleShooting : public zPlayerAction {
public:
    void AddTransitionsFrom(xAnimTable* table, const char* name,
                            unsigned int (*a)(xAnimTransition*, xAnimSingle*, void*),
                            unsigned int (*b)(xAnimTransition*, xAnimSingle*, void*),
                            unsigned short e, float f, unsigned int g,
                            unsigned int h, zPlayerAction::SpecialActions i);
    static unsigned int anShakeCheck(xAnimTransition*, xAnimSingle*, void*);
    static unsigned int anZapHurtCheck(xAnimTransition*, xAnimSingle*, void*);
    static unsigned int anZapMissCheck(xAnimTransition*, xAnimSingle*, void*);
    void AddInternalTransitions(xAnimTable* table);
    void AddStates(xAnimTable* table);
};

class zPlayerIdlePlankton {
public:
    static unsigned int anEnterCheck(xAnimTransition*, xAnimSingle*, void*);
    static unsigned int anExitCheck(xAnimTransition*, xAnimSingle*, void*);
    static unsigned int anIdleCheck(xAnimTransition*, xAnimSingle*, void*);
    static unsigned int anShakeCheck(xAnimTransition*, xAnimSingle*, void*);
    static unsigned int anZapHurtCheck(xAnimTransition*, xAnimSingle*, void*);
    static unsigned int anZapStunCheck(xAnimTransition*, xAnimSingle*, void*);
};

// -- the animation tables, read from the image ------------------

// zPlayerIdleShooting::AddStates: 10 call(s)
void zPlayerIdleShooting::AddStates(xAnimTable* table) {
    NewState(table, "Hidden01", 16, 0, 1.0f, 0, 0, 0.0f, 0, 0, 0, 0, 0, 0);
    NewState(table, "Idle01", 16, 0x2000000, 1.0f, 0, 0, 0.0f, 0, 0, 0, 0, 0, 0);
    NewState(table, "Enter01", 32, 0, 1.0f, 0, 0, 0.0f, 0, 0, 0, 0, 0, 0);
    NewState(table, "Exit01", 32, 0, 1.0f, 0, 0, 0.0f, 0, 0, 0, 0, 0, 0);
    NewState(table, "Talk01", 16, 0, 1.0f, 0, 0, 0.0f, 0, 0, 0, 0, 0, 0);
    NewState(table, "AttackShake01", 16, 0x2000000, 1.0f, 0, 0, 0.0f, 0, 0, 0, 0, 0, 0);
    NewState(table, "AttackShakeMiss01", 32, 0x2000000, 1.0f, 0, 0, 0.0f, 0, 0, 0, 0, 0, 0);
    NewState(table, "AttackStun01", 32, 0, 1.0f, 0, 0, 0.0f, 0, 0, 0, 0, 0, 0);
    NewState(table, "AttackShoot01", 32, 0, 1.0f, 0, 0, 0.0f, 0, 0, 0, 0, 0, 0);
    NewState(table, "AttackEmpty01", 32, 0x2000000, 1.0f, 0, 0, 0.0f, 0, 0, 0, 0, 0, 0);
}

// zPlayerIdleShooting::AddInternalTransitions: 21 call(s)
void zPlayerIdleShooting::AddInternalTransitions(xAnimTable* table) {
    float FAST_BLEND_TIME = 0.06666667f;
    xAnimTableNewTransition(table, "Hidden01", "Enter01", 0, zPlayerIdlePlankton::anEnterCheck, 0, 0, 0, 0.0f, 0.0f, 1000, 0, FAST_BLEND_TIME, 0);
    xAnimTableNewTransition(table, "Exit01", "Enter01", 0, zPlayerIdlePlankton::anEnterCheck, 0, 0, 0, 0.0f, 0.0f, 1000, 0, FAST_BLEND_TIME, 0);
    xAnimTableNewTransition(table, "Exit01", "Enter01", 0, zPlayerIdlePlankton::anEnterCheck, 0, 16, 0, 0.0f, 0.0f, 1000, 0, FAST_BLEND_TIME, 0);
    xAnimTableNewTransition(table, "*", "Exit01", 0, zPlayerIdlePlankton::anExitCheck, 0, 0, 0, 0.0f, 0.0f, 1000, 0, FAST_BLEND_TIME, 0);
    xAnimTableNewTransition(table, "*", "Exit01", 0, zPlayerIdlePlankton::anExitCheck, 0, 16, 0, 0.0f, 0.0f, 1000, 0, FAST_BLEND_TIME, 0);
    xAnimTableNewTransition(table, "Exit01", "Hidden01", 0, 0, 0, 16, 0, 0.0f, 0.0f, 1000, 0, FAST_BLEND_TIME, 0);
    xAnimTableNewTransition(table, "Enter01", "Idle01", 0, 0, 0, 16, 0, 0.0f, 0.0f, 999, 0, FAST_BLEND_TIME, 0);
    xAnimTableNewTransition(table, "Attack*", "Idle01", 0, 0, 0, 16, 0, 0.0f, 0.0f, 999, 0, FAST_BLEND_TIME, 0);
    xAnimTableNewTransition(table, "*", "Talk01", 0, zPlayerIdlePlankton::anShakeCheck, 0, 16, 0, 0.0f, 0.0f, 1000, 0, FAST_BLEND_TIME, 0);
    xAnimTableNewTransition(table, "Talk01", "Idle01", 0, zPlayerIdlePlankton::anIdleCheck, 0, 0, 0, 0.0f, 0.0f, 999, 0, FAST_BLEND_TIME, 0);
    xAnimTableNewTransition(table, "AttackShake01", "Idle01", 0, zPlayerIdlePlankton::anIdleCheck, 0, 0, 0, 0.0f, 0.0f, 999, 0, FAST_BLEND_TIME, 0);
    xAnimTableNewTransition(table, "*", "AttackShake01", 0, zPlayerIdleShooting::anShakeCheck, 0, 0, 0, 0.0f, 0.0f, 1000, 0, FAST_BLEND_TIME, 0);
    xAnimTableNewTransition(table, "*", "AttackShakeMiss01", 0, zPlayerIdlePlankton::anZapStunCheck, 0, 0, 0, 0.0f, 0.0f, 1000, 0, FAST_BLEND_TIME, 0);
    xAnimTableNewTransition(table, "*", "AttackStun01", 0, zPlayerIdlePlankton::anZapHurtCheck, 0, 0, 0, 0.0f, 0.0f, 1000, 0, FAST_BLEND_TIME, 0);
    xAnimTableNewTransition(table, "*", "AttackShoot01", 0, zPlayerIdleShooting::anZapHurtCheck, 0, 0, 0, 0.0f, 0.0f, 1000, 0, FAST_BLEND_TIME, 0);
    xAnimTableNewTransition(table, "*", "AttackEmpty01", 0, zPlayerIdleShooting::anZapMissCheck, 0, 0, 0, 0.0f, 0.0f, 1000, 0, FAST_BLEND_TIME, 0);
    xAnimTableNewTransition(table, "*", "AttackShake01", 0, zPlayerIdleShooting::anShakeCheck, 0, 16, 0, 0.0f, 0.0f, 1000, 0, FAST_BLEND_TIME, 0);
    xAnimTableNewTransition(table, "*", "AttackShakeMiss01", 0, zPlayerIdlePlankton::anZapStunCheck, 0, 16, 0, 0.0f, 0.0f, 1000, 0, FAST_BLEND_TIME, 0);
    xAnimTableNewTransition(table, "*", "AttackStun01", 0, zPlayerIdlePlankton::anZapHurtCheck, 0, 16, 0, 0.0f, 0.0f, 1000, 0, FAST_BLEND_TIME, 0);
    xAnimTableNewTransition(table, "*", "AttackShoot01", 0, zPlayerIdleShooting::anZapHurtCheck, 0, 16, 0, 0.0f, 0.0f, 1000, 0, FAST_BLEND_TIME, 0);
    xAnimTableNewTransition(table, "*", "AttackEmpty01", 0, zPlayerIdleShooting::anZapMissCheck, 0, 16, 0, 0.0f, 0.0f, 1000, 0, FAST_BLEND_TIME, 0);
}

// zPlayerIdleShooting::AddTransitionsFrom: every transition INTO the idle, from the
// state `name`: the action helper with the idle's own check as the
// first callback and the caller's as the second and third.
void zPlayerIdleShooting::AddTransitionsFrom(xAnimTable* table, const char* name,
                            unsigned int (*a)(xAnimTransition*, xAnimSingle*, void*),
                            unsigned int (*b)(xAnimTransition*, xAnimSingle*, void*),
                            unsigned short e, float f, unsigned int g,
                            unsigned int h, zPlayerAction::SpecialActions i) {
    zPlayerAction::AddActionTransition(table, name, "Idle01", zPlayerIdlePlankton::anIdleCheck, a, b, e, f, g, h);
}
