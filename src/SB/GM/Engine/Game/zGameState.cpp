// zGameState.cpp -- nine functions, read from the image with
// tools/disasm.py. The game state is one of eleven combined states,
// grouped into three modes: GameModeFromGameState maps the game states
// (6..10) to the game mode, the pause states (2..5) to pause, the two
// title states to title, and anything else to the count. Four accessors
// read and write the mode, state and ostrich globals. zGameStateSwitch
// sets the mode, running the game-to-pause or pause-to-game transition
// when it changes, and on a state change stores it and sends the
// state-switch event to every entity; zGameStateSwitchEvent does that
// and then keeps the game timer: leaving the game for pause adds the
// time since the last tick, coming back records the tick, and exiting
// the game wipes the serial buffer and queues the menu scene. The
// transitions pause and unpause the sound categories, pause or unpause
// the running cinematic through its vtable, and coming back also set
// the sound fade-in and mute the master category when a portal is
// pending.
//
// Layout from the DWARF (tools/dwarf_types.py): zGlobals' scene at
// +0x43C, running cinematic at +0x52C, game timer and last tick at
// +0x538 and +0x540, timing flag at +0x57C; the scene's pending portal
// at +0xCD8 (a portal pointer then a name). The string and the two
// literals are the unity unit's pool, hence the generated header
// first; the unit has no data of its own.
//
// Three shapes the bytes fixed: the mode mapping is a switch over all
// eleven states (the clusters are tested from the top and the title
// pair last, which no if-chain gives); each timer branch reads the time
// through zGameTickNow, an INLINED helper returning the one-member
// struct BY VALUE; and the portal test is an inlined predicate whose
// result is a value rather than a branch.
//
// The by-value return is what places the four stack slots, and it took
// the layout to settle rather than a score. mwcc hands out ascending
// frame slots in REVERSE declaration order out of ONE pool, so two
// named locals per branch give now1@0x20 cur1@0x18 now2@0x10 cur2@0x08
// -- grouped by branch. Retail groups by ROLE: now1@0x10 now2@0x08 and
// cur1@0x20 cur2@0x18, which one pool cannot produce in any declaration
// order, so retail has two. An inlined function returning the aggregate
// splits them: its own body local lands in the low pool and the unnamed
// RETURN-VALUE temporary in the high one. Of 26 spellings swept, TEN
// match: the helper writing its member or aggregate-initialised, and
// its result taken into a named local, a const reference, an
// assignment, a further copy, a by-value or by-reference inline
// argument, or straight into the expression. The one written is also a
// single statement per branch, which is what retail's line table says:
// line 116 owns the whole of the first branch and 120 the whole of the
// second.
//
// The sixteen that miss, and how -- the counts matter, because two of
// these groups fail in opposite directions. FOUR leave the same 8 of 64
// with all four objects still named locals in the one pool: the two
// branches written the other way round, the copy in an inner block,
// aggregate initialisation from the call, and a by-value inline
// helper's ARGUMENT taking the named local rather than the return
// (that one does move two of the four slots, just not into retail's
// grouping). TEN are WORSE
// at 41 of 56 -- the whole function is eight words shorter, because a
// user-declared constructor of any kind (converting, direct-init,
// copy-init, a default one calling OSGetTime, an explicit inline copy
// constructor) or an assignment to a copy declared first stops the
// aggregate being memory-resident and the four stack objects vanish
// entirely; a by-reference argument on a named local does the same. A
// function-style cast was only ever measured alongside that copy
// constructor, so it is not isolated. The last two land the LAYOUT
// exactly right and miss elsewhere: calling the helper twice in a
// branch is 40 of 64 on scheduling alone, and folding the subtraction
// into a second helper is 13 of 64 on the callee-saved rotation.
//
// Ten compiler flags were recorded by an earlier pass and NOT
// re-measured here: -opt nolifetimes, nodeadstore, noprop, nostrength
// and noloop, -common on and -align mac68k left the eight words exactly
// where they were, and -opt nocse and -pool off were worse.
//
// The portal test is the same lever in miniature, and its old note here
// was stale twice over: 8 of 42 words, not 34, and a predicate DOES
// inline. Retail puts the materialised boolean in r3 and the scene
// pointer in r4; a plain `bool portalPending = false;` local gets them
// the other way round, and that alone was all eight words. An inlined
// predicate returns its result in r3 because r3 is the return register,
// and the object it was called on stays in r4 -- which is also why
// retail's line table gives every one of those instructions to the
// single line 199 while the call sits on 203, and why its DWARF names
// no local in this function at all. Of 20 spellings swept, THREE
// match: zPortalPending::IsPending (written), a free predicate over
// zPortalPending*, and no helper at all with the scene in a named local
// and the || as the flag's initialiser. What they share is that the
// scene pointer is already a value in a register before the test --
// a named local, or the address the inlined predicate was handed.
// THIRTEEN come out SHORTER, short-circuited straight to branches with
// no value ever materialised: the bare `if (A || B)` with no helper at
// all, and every predicate handed `globals.sceneCur` read inline or no
// argument at all. THREE leave the same 8 of 42 -- the flag typed
// unsigned char or int with the scene read inline, and the scene in a
// local declared after the flag -- and a ternary is 10. Note for the
// next reader: a static inline predicate DOES inline here, whether or
// not it is marked inline or static; the old note's "none inlines; a
// call appears" is not what -inline auto does with any of these.
//
// Retail's DWARF names, now carried by the source so regdiff pairs all
// seven: zGameStateSwitchEvent's parameter is newState and its locals
// prevMode, prevState and newMode, all in
// registers, with no stack local named. zGameStateSwitch's are newState
// and newMode plus a block-scoped `params` at frame +8 whose type is
// EventNotifyDispatcher_Combined..., not the Sext::EventAny spelled
// below -- that name feeds zEntEventAllOfType's mangling and is still
// unrecovered.

#include "SB/GM/Engine/Game/zGameState.pool.h"

extern "C" {
typedef long long OSTime;

OSTime OSGetTime(void);
}

struct zGameTick {
    OSTime ticks;
};

enum eGameMode {
    eGameMode_Title = 0,
    eGameMode_Pause = 1,
    eGameMode_Game = 2,
    eGameMode_Count = 3
};

enum _GameOstrich { _GameOstrich_ = 0x7FFFFFFF };

enum ForceEvent { ForceEvent_ = 0x7FFFFFFF };

namespace Sext {

enum eGameStateCombined {
    eState_Title_Start = 0,
    eState_Title_Attract = 1,
    eState_Pause_Pause = 2,
    eState_Pause_TRC = 3,
    eState_Pause_Join = 4,
    eState_Pause_Misc = 5,
    eState_Game_FirstTime = 6,
    eState_Game_Playing = 7,
    eState_Game_SceneSwitch = 8,
    eState_Game_Dead = 9,
    eState_Game_Exit = 10,
    END_eState_ENUM = 11
};

class EventAny {
public:
    unsigned int state;
    unsigned char _pad0[0xC];
};

}  // namespace Sext

class xBase;

void zEntEventAllOfType(xBase* from, unsigned int type, unsigned int event,
                        Sext::EventAny* any, unsigned int, ForceEvent force);
void xSerialWipeMainBuffer();
void zPlayerResourcesSetNextScene(unsigned int scene);

class zSoundModule {
public:
    static void SoundCategoryPauseUnpauseAllExceptUI(bool pause);
    static void SoundCategorySetMute(const char* category, bool mute);

    void SetUnpauseFadeIn(float a, float b);
};

zSoundModule* GlobalGetSoundModule();

// The cinematic, with the two virtuals the transitions call at their
// slots; the slots before them exist only to put them there.
class zCinematic {
public:
    virtual void _v0();   virtual void _v1();   virtual void _v2();
    virtual void _v3();   virtual void _v4();   virtual void _v5();
    virtual void _v6();   virtual void _v7();   virtual void _v8();
    virtual void _v9();   virtual void _v10();  virtual void _v11();
    virtual void _v12();  virtual void _v13();  virtual void _v14();
    virtual void _v15();  virtual void _v16();  virtual void _v17();
    virtual void _v18();  virtual void _v19();  virtual void _v20();
    virtual void _v21();  virtual void _v22();  virtual void _v23();
    virtual void _v24();  virtual void _v25();
    virtual void Pause();
    virtual void Unpause();
};

class zPortal;

class zPortalPending {
public:
    zPortal* assetPortal;
    char manualPortal[64];

    bool IsPending() const {
        return assetPortal != 0 || manualPortal[0] != 0;
    }
};

class zScene {
public:
    unsigned char _pad0[0xCD8];
    zPortalPending pendingPortal;
};

class zGlobals {
public:
    unsigned char _pad0[0x43C];
    zScene* sceneCur;
    unsigned char _pad1[0x52C - 0x440];
    zCinematic* runningCinematic;
    unsigned char _pad2[0x538 - 0x530];
    OSTime gameTimer;
    OSTime lastGameTick;
    unsigned char _pad3[0x57C - 0x548];
    bool timingGame;
};

extern zGlobals globals;

extern eGameMode gGameMode;
extern Sext::eGameStateCombined gGameState;
extern _GameOstrich gGameOstrich;

static inline zGameTick zGameTickNow() {
    zGameTick tick;

    tick.ticks = OSGetTime();

    return tick;
}

eGameMode GameModeFromGameState(Sext::eGameStateCombined state);
Sext::eGameStateCombined zGameStateGet();
eGameMode zGameModeGet();
_GameOstrich zGameGetOstrich();
void zGameSetOstrich(_GameOstrich ostrich);
void zGameStateSwitchEvent(Sext::eGameStateCombined newState);
void zGameStateSwitch(Sext::eGameStateCombined newState);
void TransitioningMode_GameToPause();
void TransitioningMode_PauseToGame();

eGameMode GameModeFromGameState(Sext::eGameStateCombined state) {
    switch (state) {
    case Sext::eState_Title_Start:
    case Sext::eState_Title_Attract:
        return eGameMode_Title;
    case Sext::eState_Pause_Pause:
    case Sext::eState_Pause_TRC:
    case Sext::eState_Pause_Join:
    case Sext::eState_Pause_Misc:
        return eGameMode_Pause;
    case Sext::eState_Game_FirstTime:
    case Sext::eState_Game_Playing:
    case Sext::eState_Game_SceneSwitch:
    case Sext::eState_Game_Dead:
    case Sext::eState_Game_Exit:
        return eGameMode_Game;
    default:
        return eGameMode_Count;
    }
}

Sext::eGameStateCombined zGameStateGet() {
    return gGameState;
}

eGameMode zGameModeGet() {
    return gGameMode;
}

_GameOstrich zGameGetOstrich() {
    return gGameOstrich;
}

void zGameSetOstrich(_GameOstrich ostrich) {
    gGameOstrich = ostrich;
}

void zGameStateSwitchEvent(Sext::eGameStateCombined newState) {
    eGameMode prevMode = gGameMode;
    Sext::eGameStateCombined prevState = gGameState;

    zGameStateSwitch(newState);

    eGameMode newMode = GameModeFromGameState(newState);

    if (newState == prevState) {
        return;
    }

    if (newState == Sext::eState_Game_Exit) {
        xSerialWipeMainBuffer();
        zPlayerResourcesSetNextScene(0x4D4E5553);
        return;
    }

    if (!globals.timingGame) {
        return;
    }

    if (newMode == eGameMode_Pause && prevMode == eGameMode_Game) {
        globals.gameTimer += zGameTickNow().ticks - globals.lastGameTick;
    } else if (newMode == eGameMode_Game && prevMode == eGameMode_Pause) {
        globals.lastGameTick = zGameTickNow().ticks;
    }
}

void zGameStateSwitch(Sext::eGameStateCombined newState) {
    eGameMode newMode = GameModeFromGameState(newState);

    if (newMode != gGameMode) {
        if (gGameMode == eGameMode_Game && newMode == eGameMode_Pause) {
            TransitioningMode_GameToPause();
        } else if (gGameMode == eGameMode_Pause && newMode == eGameMode_Game) {
            TransitioningMode_PauseToGame();
        }

        gGameMode = newMode;
    }

    if (newState != gGameState) {
        gGameState = newState;

        Sext::EventAny params;

        params.state = newState;

        zEntEventAllOfType(0, 0, 0xB32A134C, &params, 78, (ForceEvent)1);
    }
}

void TransitioningMode_GameToPause() {
    zSoundModule::SoundCategoryPauseUnpauseAllExceptUI(true);

    if (globals.runningCinematic) {
        globals.runningCinematic->Pause();
    }
}

void TransitioningMode_PauseToGame() {
    zSoundModule::SoundCategoryPauseUnpauseAllExceptUI(false);

    GlobalGetSoundModule()->SetUnpauseFadeIn(2.0f, 1.0f);

    if (globals.runningCinematic) {
        globals.runningCinematic->Unpause();
    }

    if (globals.sceneCur->pendingPortal.IsPending()) {
        zSoundModule::SoundCategorySetMute("master", true);
    }
}
