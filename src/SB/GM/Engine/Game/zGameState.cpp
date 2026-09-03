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
// pair last, which no if-chain gives); each timer branch keeps its time
// in a one-member struct, copied once, because retail stores the time
// to two stack slots per branch, one lo-first as a call result and one
// hi-first as a variable, the memory-resident shape of an aggregate.
//
// NEAR MISS, zGameStateSwitchEvent 8 of 64 words, all four stack slots:
// retail keeps each branch's two eight-byte locals with the copy above
// the time (0x20 over 0x10, 0x18 over 0x8) and ours the time above the
// copy, the stores and everything else identical. Tried and no better:
// the copy declared first and assigned (the copy becomes loads, 38),
// all four at the function's top (38), the copies at the function's
// top assigned member-wise or whole (38), a converting constructor
// with copy- or direct-initialisation (38), and the copy as a
// function-style cast temporary or a by-value inline helper's
// argument (the same 8, the temporary below the time).
//
// NEAR MISS, TransitioningMode_PauseToGame 34 of 42 words: the portal
// flag and the scene pointer are r3 and r4 in retail and r4 and r3 here,
// the same twelve instructions otherwise. Tried and no better: the flag
// as the `||` expression's value, set in an `if`, in both branches of
// an if/else, by two ifs, typed int, declared at the function's top
// with and without its initialiser, the scene in a local before or
// after it, and the test as a static inline or member predicate with
// and without the always-inline pragma (none inlines; a call appears).

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

eGameMode GameModeFromGameState(Sext::eGameStateCombined state);
Sext::eGameStateCombined zGameStateGet();
eGameMode zGameModeGet();
_GameOstrich zGameGetOstrich();
void zGameSetOstrich(_GameOstrich ostrich);
void zGameStateSwitchEvent(Sext::eGameStateCombined state);
void zGameStateSwitch(Sext::eGameStateCombined state);
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

void zGameStateSwitchEvent(Sext::eGameStateCombined state) {
    eGameMode oldMode = gGameMode;
    Sext::eGameStateCombined oldState = gGameState;

    zGameStateSwitch(state);

    eGameMode mode = GameModeFromGameState(state);

    if (state == oldState) {
        return;
    }

    if (state == Sext::eState_Game_Exit) {
        xSerialWipeMainBuffer();
        zPlayerResourcesSetNextScene(0x4D4E5553);
        return;
    }

    if (!globals.timingGame) {
        return;
    }

    if (mode == eGameMode_Pause && oldMode == eGameMode_Game) {
        zGameTick now;

        now.ticks = OSGetTime();

        zGameTick current = now;

        globals.gameTimer += current.ticks - globals.lastGameTick;
    } else if (mode == eGameMode_Game && oldMode == eGameMode_Pause) {
        zGameTick now;

        now.ticks = OSGetTime();

        zGameTick current = now;

        globals.lastGameTick = current.ticks;
    }
}

void zGameStateSwitch(Sext::eGameStateCombined state) {
    eGameMode mode = GameModeFromGameState(state);

    if (mode != gGameMode) {
        if (gGameMode == eGameMode_Game && mode == eGameMode_Pause) {
            TransitioningMode_GameToPause();
        } else if (gGameMode == eGameMode_Pause && mode == eGameMode_Game) {
            TransitioningMode_PauseToGame();
        }

        gGameMode = mode;
    }

    if (state != gGameState) {
        gGameState = state;

        Sext::EventAny any;

        any.state = state;

        zEntEventAllOfType(0, 0, 0xB32A134C, &any, 78, (ForceEvent)1);
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

    bool portalPending = false;

    if (globals.sceneCur->pendingPortal.assetPortal != 0 ||
        globals.sceneCur->pendingPortal.manualPortal[0] != 0) {
        portalPending = true;
    }

    if (portalPending) {
        zSoundModule::SoundCategorySetMute("master", true);
    }
}
