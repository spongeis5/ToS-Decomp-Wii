#include "SB/GM/Engine/Game/zPlayerInputPadMgr.pool.h"

// zPlayerInputPadMgr.cpp -- the four human and four AI pad slots, which
// player owns which, which game port each answers to, and the drop-in /
// drop-out of a second player. Read from the image with tools/disasm.py;
// the layouts are the DWARF's (alltypes.h), the virtual slots are the
// retail vtables' (vtable.py), and the strings come out of WAD03.cpp's
// pool, which the header in front reproduces.
//
// A game port of 0x7FC00000 means none. Slot 7 of a pad answers 1 for an
// AI pad (zPlayerInputAI's slot is the folded return-true, the human's the
// folded return-false), which is why every test of it compares with 1.
//
// A loop the compiler does not turn into a count-down compares its bound
// unsigned (cmplwi) although the DWARF's index is an int: the bound is an
// unsigned 4, spelled 4u below.

class xBase;
class zScript;
class zUIFlashOnScreenText;

namespace Sext {

class EventAny {};

}  // namespace Sext

enum ForceEvent { ForceEvent_ = 0x7FFFFFFF };

namespace Memory {
enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };
}

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

void* xMemAlloc(Memory::GlobalHeapEnum heap, unsigned int size, int align,
                eMemMgrTag tag);

inline void* operator new(unsigned long, void* p) { return p; }

unsigned int xStrHash(const char* str);
unsigned long long xUIDMgrFindUID(unsigned int hash);
xBase* zSceneFindEntity(unsigned long long id);
bool zSceneInMainMenu();
void zEntEvent(xBase* from, unsigned int fromEvent, xBase* to,
               unsigned int toEvent, Sext::EventAny* any, ForceEvent force);
void zEntEventAllOfType(xBase* from, unsigned int fromEvent,
                        unsigned int toEvent, Sext::EventAny* any,
                        unsigned int type, ForceEvent force);

namespace zPlayerInputNS {

enum PadType {
    PADTYPE_DEBUG = 0,
    PADTYPE_MENU = 1,
    PADTYPE_GAME = 2,
    PADTYPE_MISC = 3,
    PADTYPE_PAUSE = 4,
    PADTYPE_LAST = 5,
};

enum Action { Action_ = 0x7FFFFFFF };

}  // namespace zPlayerInputNS

namespace IO {

class Pad {
public:
    bool acquired;
};

class PadNative {
public:
    static void DPDForceOff();
};

}  // namespace IO

namespace TRC {

class TRCMsgBox {
public:
    TRCMsgBox();

    char* message;
    char* yesText;
    char* noText;
    char* header;
    int image;
    int promptType;
    int msgType;
    int responseType;
    int inputType;
    bool defaultNo;
    void* userData;
    int state;
    int _pad30;
};

class MsgBoxManager {
public:
    static MsgBoxManager* Instance();
    void Add(TRCMsgBox* box);
};

class TRCPadManager {
public:
    enum AccessoryType {
        ACCESSORY_NONE = 0,
        ACCESSORY_1 = 1
    };

    void EnforcePadActivity(int port);
    void EnableAccessoryActivity(int port, AccessoryType type);
    void DisablePadTRC(int port);
    void DisablePadActivity(int port);
};

class TRCModule {
public:
    unsigned char _pad0[0x98];
    TRCPadManager trcPadManager;
};

extern TRCModule* trcModule;

bool IsSaveSystemBusy();

}  // namespace TRC

class D3DRECT {
public:
    int x;
    int y;
    int w;
    int h;
};

namespace Graphics {

class MovieData;

enum movieEndCode { movieEndCode_ = 0x7FFFFFFF };
enum movieSkippable { movieSkippable_ = 0x7FFFFFFF };

void RenderMovie(movieEndCode (*play)(MovieData*), const char* name,
                 unsigned long long subtitles, bool flag,
                 movieSkippable skippable, D3DRECT* rect, D3DRECT* dstRect,
                 int arg, void* data, bool (*busy)());

}  // namespace Graphics

Graphics::movieEndCode PlayMovieTemp(Graphics::MovieData* data);

// The pad interface: only the slots this file calls are named.
class zPlayerInput {
public:
    virtual void _v0();
    virtual void _v1();
    virtual int GetPadSubLayout();
    virtual void _v3();
    virtual void _v4();
    virtual void _v5();
    virtual void _v6();
    virtual int GetInputType();
    virtual zPlayerInputNS::PadType GetPadType();
    virtual void _v9();
    virtual void Update(unsigned int port);
    virtual void Reset();
    virtual void _v12();
    virtual void _v13();
    virtual bool IsPadDisabled();
    virtual void _v15();
    virtual void EnablePadType(zPlayerInputNS::PadType type);
    virtual void _v17();
    virtual void _v18();
    virtual bool Pressed(zPlayerInputNS::Action action, bool exact,
                         bool useMap);
    virtual void _v20(); virtual void _v21(); virtual void _v22();
    virtual void _v23(); virtual void _v24(); virtual void _v25();
    virtual void _v26(); virtual void _v27(); virtual void _v28();
    virtual void _v29(); virtual void _v30(); virtual void _v31();
    virtual void _v32(); virtual void _v33(); virtual void _v34();
    virtual void _v35(); virtual void _v36(); virtual void _v37();
    virtual void _v38(); virtual void _v39(); virtual void _v40();
    virtual void _v41(); virtual void _v42(); virtual void _v43();
    virtual void _v44(); virtual void _v45(); virtual void _v46();
    virtual void _v47(); virtual void _v48(); virtual void _v49();
    virtual void _v50(); virtual void _v51(); virtual void _v52();
    virtual void _v53(); virtual void _v54(); virtual void _v55();
    virtual void _v56(); virtual void _v57(); virtual void _v58();
    virtual void _v59(); virtual void _v60(); virtual void _v61();
    virtual void _v62(); virtual void _v63(); virtual void _v64();
    virtual void _v65(); virtual void _v66(); virtual void _v67();
    virtual void _v68(); virtual void _v69(); virtual void _v70();
    virtual void _v71(); virtual void _v72(); virtual void _v73();
    virtual void _v74(); virtual void _v75(); virtual void _v76();
    virtual void _v77(); virtual void _v78(); virtual void _v79();
    virtual bool IsResetPending();
    virtual unsigned int GetPlayerInputPort();
};

class zPlayerInputHuman : public zPlayerInput {
public:
    zPlayerInputHuman();
};

class zPlayerInputAI : public zPlayerInput {
public:
    zPlayerInputAI();
};

// The vtable pointer is at +0, so the virtuals live in a data-less base.
class zPlayerVirtuals {
public:
    virtual void _v0(); virtual void _v1(); virtual void _v2();
    virtual void _v3(); virtual void _v4(); virtual void _v5();
    virtual void _v6(); virtual void _v7(); virtual void _v8();
    virtual void _v9(); virtual void _v10(); virtual void _v11();
    virtual void _v12(); virtual void _v13(); virtual void _v14();
    virtual void _v15(); virtual void _v16(); virtual void _v17();
    virtual void _v18(); virtual void _v19(); virtual void _v20();
    virtual void _v21(); virtual void _v22(); virtual void _v23();
    virtual void _v24(); virtual void _v25(); virtual void _v26();
    virtual void _v27(); virtual void _v28(); virtual void _v29();
    virtual void _v30(); virtual void _v31(); virtual void _v32();
    virtual void _v33(); virtual void _v34(); virtual void _v35();
    virtual void _v36(); virtual void _v37(); virtual void _v38();
    virtual void _v39(); virtual void _v40(); virtual void _v41();
    virtual void _v42(); virtual void _v43(); virtual void _v44();
    virtual void _v45(); virtual void _v46(); virtual void _v47();
    virtual void _v48(); virtual void _v49(); virtual void _v50();
    virtual void _v51(); virtual void _v52(); virtual void _v53();
    virtual void _v54(); virtual void _v55(); virtual void _v56();
    virtual void _v57(); virtual void _v58(); virtual void _v59();
    virtual void _v60(); virtual void _v61(); virtual void _v62();
    virtual void _v63(); virtual void _v64(); virtual void _v65();
    virtual void _v66(); virtual void _v67(); virtual void _v68();
    virtual void _v69(); virtual void _v70(); virtual void _v71();
    virtual void _v72(); virtual void _v73(); virtual void _v74();
    virtual void _v75(); virtual void _v76(); virtual void _v77();
    virtual void _v78(); virtual void _v79(); virtual void _v80();
    virtual void _v81(); virtual void _v82(); virtual void _v83();
    virtual void _v84(); virtual void _v85(); virtual void _v86();
    virtual void _v87(); virtual void _v88(); virtual void _v89();
    virtual void _v90(); virtual void _v91(); virtual void _v92();
    virtual void _v93(); virtual void _v94(); virtual void _v95();
    virtual void _v96(); virtual void _v97(); virtual void _v98();
    virtual void _v99(); virtual void _v100(); virtual void _v101();
    virtual void _v102(); virtual void _v103(); virtual void _v104();
    virtual bool IsAI() const;
    virtual void _v106();
    virtual bool CanJoinOrDrop();
    virtual void SwitchedToHuman(bool notify);
    virtual void SwitchedToAI(bool notify);
};

class zPlayer : public zPlayerVirtuals {
public:
    unsigned char _pad0[0xF8 - 0x4];
    int playerIndex;
    unsigned char _pad1[0x1E8 - 0xFC];
    zPlayerInput* playerInput;
    unsigned char _pad2[0x2EC - 0x1EC];
    int eName;
};

class zPlayerContainer {
public:
    zPlayer* playerArray[4];
    int numPlayers;
};

class xGlobals {
public:
    unsigned char _pad0[0x3AC];
    float update_dt;
    unsigned char _pad1[0x428 - 0x3B0];
    zPlayerContainer players;
    unsigned char _pad2[0x4D8 - 0x43C];
    int playerPort;
    int player2Port;
    unsigned char _pad3[0x4E8 - 0x4E0];
};

class zGlobals : public xGlobals {
public:
    unsigned char _pad4[0x588 - 0x4E8];
    int demoType;
    unsigned char _pad5[0x5A0 - 0x58C];
};

extern xGlobals* xglobals;
extern zGlobals globals;

class EventActionHudSetPlayer : public Sext::EventAny {
public:
    int playerIdx;
    int characterID;
};

class EventActionOneInt : public Sext::EventAny {
public:
    short param0;
};

class PadData {
public:
    zPlayerInput* pad;
    zPlayer* owner;
    unsigned int gamePort;
    unsigned int originalPort;
};

class zPlayerInputPadMgr {
public:
    zPlayerInputPadMgr();
    void StartUp();
    int FindPadWithGamePort(int port);
    void zPlayerInputSceneSetup();
    void zPlayerInputUpdate();
    void UpdateDebugPads();
    void zPlayerInputReset();
    void zPlayerInputSceneExit();
    zPlayerInput* SetNewPlayer(zPlayer* const newPlayer, bool ai);
    void HandleDominantPlayerPort();
    void HandleSecondaryPlayerPort();
    zPlayerInput* CheckForJoinDrop(zPlayer* player);
    bool ClearPlayer(zPlayer* const player, bool clear);
    zPlayerInput* ClaimAIPad(zPlayer* player);
    zPlayerInput* RestoreHumanPad(zPlayer* player);
    void reSort();
    bool AcquirePad(IO::Pad* pad);
    bool ReleasePad(IO::Pad* pad);
    PadData* FindPadData(zPlayer* const player);
    unsigned int GetGamePort(zPlayerInput* key);
    unsigned int GetGamePort(zPlayer* player);
    zPlayerInput* GetPadAtGamePort(int port);
    zPlayerInput* GetDebugPad();
    zPlayer* GetPadOwner(zPlayerInput* pad);
    bool UserSceneReset();
    zPlayerInput* CheckForJoin(zPlayer* player, bool forceJoin);
    zPlayerInput* CheckForDrop(zPlayer* player, bool forceDrop);
    int GetPlayerIndex(zPlayer* player) const;
    int GetAIPlayer(zPlayer* player) const;
    bool IsAI(unsigned int index);
    zPlayer* GetPlayerFromPlayerInput(zPlayerInput* input);
    zPlayerInput* GetBalancePad();

    unsigned int joinDropWait;
    float dropDelay;
    PadData humanPads[4];
    PadData aiPads[4];
    zPlayerInput* curDebugPad[4];
    IO::Pad* acquiredPads[4];
    bool updatePadStatus;
    bool resortPlayersToo;
    TRC::TRCMsgBox joinDropMsgBox;
    int joinDropBoxInUse;
    zScript* joinTextBoxScript;
    zUIFlashOnScreenText* joinTextBox;
    zUIFlashOnScreenText* noAchTextBox;
    bool firstJoin;
    TRC::TRCMsgBox planktonTutorialMsgBox;
    int planktonTutorialBoxInUse;
};

namespace zPlayerInputNS {

extern zPlayerInputPadMgr padManager;

zPlayerInputPadMgr* GetPadManager();
unsigned int GetGamePort(zPlayerInput* key);
zPlayerInput* GetPadAtGamePort(int port);
zPlayerInput* GetDebugPad();
bool UserSceneReset();

}  // namespace zPlayerInputNS

zPlayerInputPadMgr::zPlayerInputPadMgr() {
    for (int i = 0; i < 4; i++) {
        acquiredPads[i] = 0;
        humanPads[i].owner = 0;
        humanPads[i].gamePort = i;
        aiPads[i].owner = 0;
        aiPads[i].gamePort = i;
        curDebugPad[i] = 0;
        aiPads[i].originalPort = i;
        humanPads[i].originalPort = i;
    }

    resortPlayersToo = true;
    joinDropBoxInUse = -1;
    joinDropWait = 0;
    dropDelay = 0.0f;
    firstJoin = true;
    planktonTutorialBoxInUse = -1;
}

void zPlayerInputPadMgr::StartUp() {
    for (int i = 0; i < 4u; i++) {
        humanPads[i].pad =
            new (xMemAlloc((Memory::GlobalHeapEnum)0, 536, 0, (eMemMgrTag)55))
                zPlayerInputHuman;
        aiPads[i].pad =
            new (xMemAlloc((Memory::GlobalHeapEnum)0, 28, 0, (eMemMgrTag)55))
                zPlayerInputAI;
    }
}

int zPlayerInputPadMgr::FindPadWithGamePort(int port) {
    for (int i = 0; i < 4; i++) {
        if (port == humanPads[i].gamePort) {
            return i;
        }
    }

    return -1;
}

void zPlayerInputPadMgr::zPlayerInputSceneSetup() {
    joinTextBoxScript = (zScript*)zSceneFindEntity(
        xUIDMgrFindUID(xStrHash("JoinDrop_P2JoinScriptOtherPlatformRef")));
    joinTextBox = (zUIFlashOnScreenText*)zSceneFindEntity(
        xUIDMgrFindUID(xStrHash("JoinDrop_P2_Join_TextBox_ref")));
    noAchTextBox = (zUIFlashOnScreenText*)zSceneFindEntity(
        xUIDMgrFindUID(xStrHash("JoinDrop_P2NoAchievementTextBoxRef")));

    IO::PadNative::DPDForceOff();

    if (globals.demoType != 0) {
        firstJoin = true;
    }
}

void zPlayerInputPadMgr::zPlayerInputUpdate() {
    for (int i = 0; i < 4u; i++) {
        humanPads[i].pad->Update(humanPads[i].gamePort);
        aiPads[i].pad->Update(i);
    }

    for (int i = 0; i < 4u; i++) {
        if (humanPads[i].pad->GetPlayerInputPort() != 0x7FC00000) {
            if (humanPads[i].gamePort !=
                humanPads[i].pad->GetPlayerInputPort()) {
                int padToSwap = FindPadWithGamePort(
                    humanPads[i].pad->GetPlayerInputPort());

                if (padToSwap >= 0) {
                    int temp = humanPads[padToSwap].gamePort;
                    humanPads[padToSwap].gamePort = humanPads[i].gamePort;
                    humanPads[i].gamePort = temp;
                }
            }
        }
    }

    zPlayerInputNS::padManager.UpdateDebugPads();
}

void zPlayerInputPadMgr::UpdateDebugPads() {
    bool gotDebugPad = false;

    for (int i = 0; i < 4u; i++) {
        if (!gotDebugPad &&
            humanPads[i].pad->GetPadType() == zPlayerInputNS::PADTYPE_DEBUG) {
            gotDebugPad = true;
            curDebugPad[0] = humanPads[i].pad;
        }
    }

    if (!gotDebugPad) {
        curDebugPad[0] = 0;
    }
}

void zPlayerInputPadMgr::zPlayerInputReset() {
    joinDropWait = 0;
    dropDelay = 0.0f;

    for (int i = 0; i < 4u; i++) {
        if (humanPads[i].owner != 0 && aiPads[i].owner != 0) {
            aiPads[i].owner = 0;
        }

        humanPads[i].pad->Reset();
        aiPads[i].pad->Reset();
        humanPads[i].pad->EnablePadType(zPlayerInputNS::PADTYPE_GAME);
    }

    zPlayerInputNS::padManager.UpdateDebugPads();
}

void zPlayerInputPadMgr::zPlayerInputSceneExit() {
    joinDropWait = 0;
    dropDelay = 0.0f;

    for (int i = 0; i < 4u; i++) {
        humanPads[i].pad->Reset();
        humanPads[i].owner = 0;
        aiPads[i].pad->Reset();
        aiPads[i].owner = 0;
    }

    zPlayerInputNS::padManager.UpdateDebugPads();
    reSort();
    IO::PadNative::DPDForceOff();
}

zPlayerInput* zPlayerInputPadMgr::SetNewPlayer(zPlayer* const newPlayer,
                                               bool ai) {
    if (newPlayer == 0) {
        return 0;
    }

    if (ai) {
        for (int i = 0; i < 4; i++) {
            if (aiPads[i].owner == 0 && humanPads[i].owner == 0) {
                aiPads[i].owner = newPlayer;
                aiPads[i].gamePort = 0x7FC00000;
                return aiPads[i].pad;
            }
        }
    } else {
        for (int i = 0; i < 4; i++) {
            if (humanPads[i].owner == 0 && aiPads[i].owner == 0) {
                humanPads[i].owner = newPlayer;

                EventActionHudSetPlayer params;
                params.playerIdx =
                    zPlayerInputNS::padManager.GetPlayerIndex(newPlayer);
                params.characterID = newPlayer->eName;
                zEntEventAllOfType(0, 0, 0x71D85377, &params, 218,
                                   (ForceEvent)1);

                if (i == 0 && humanPads[0].gamePort != globals.playerPort) {
                    HandleDominantPlayerPort();
                } else if (i == 1 && globals.player2Port != -1) {
                    HandleSecondaryPlayerPort();
                }

                return humanPads[i].pad;
            }
        }
    }

    return 0;
}

void zPlayerInputPadMgr::HandleDominantPlayerPort() {
    for (int i = 0; i < 4u; i++) {
        humanPads[i].pad->EnablePadType(zPlayerInputNS::PADTYPE_GAME);
    }

    if (!zSceneInMainMenu()) {
        if (globals.playerPort >= 0) {
            zPlayerInput* oldPad = humanPads[0].pad;

            humanPads[0].pad = humanPads[globals.playerPort].pad;
            humanPads[0].owner->playerInput =
                humanPads[globals.playerPort].pad;
            humanPads[globals.playerPort].pad = oldPad;

            if (humanPads[globals.playerPort].owner != 0) {
                humanPads[globals.playerPort].owner->playerInput = oldPad;
            }

            int temp = humanPads[0].gamePort;
            humanPads[0].gamePort = humanPads[globals.playerPort].gamePort;
            humanPads[globals.playerPort].gamePort = temp;
        }
    }
}

void zPlayerInputPadMgr::HandleSecondaryPlayerPort() {
    if (!zSceneInMainMenu()) {
        if (globals.player2Port > -1 && globals.player2Port != 1) {
            zPlayerInput* oldPad = humanPads[1].pad;

            humanPads[1].pad = humanPads[globals.player2Port].pad;
            humanPads[1].owner->playerInput =
                humanPads[globals.player2Port].pad;
            humanPads[globals.player2Port].pad = oldPad;

            if (humanPads[globals.player2Port].owner != 0) {
                humanPads[globals.player2Port].owner->playerInput = oldPad;
            }

            int temp = humanPads[1].gamePort;
            humanPads[1].gamePort = humanPads[globals.player2Port].gamePort;
            humanPads[globals.player2Port].gamePort = temp;
        }
    }
}

zPlayerInput* zPlayerInputPadMgr::CheckForJoinDrop(zPlayer* player) {
    if (planktonTutorialBoxInUse >= 0 &&
        humanPads[planktonTutorialBoxInUse].owner == player) {
        if (planktonTutorialMsgBox.responseType == 1) {
            D3DRECT rect = {0, 0, 640, 480};
            D3DRECT dstRect = {0, 0, 640, 480};

            Graphics::RenderMovie(
                PlayMovieTemp, "V800",
                xUIDMgrFindUID(xStrHash("Movie800_Subtitles_reference")), true,
                (Graphics::movieSkippable)2, &rect, &dstRect, 0, 0,
                TRC::IsSaveSystemBusy);
            planktonTutorialBoxInUse = -1;
        } else if (planktonTutorialMsgBox.responseType == 2) {
            planktonTutorialBoxInUse = -1;
        }
    }

    if (joinDropWait != 0) {
        joinDropWait--;
        return 0;
    }

    return IsAI(player->playerIndex) ? CheckForJoin(player, false)
                                     : CheckForDrop(player, false);
}

bool zPlayerInputPadMgr::ClearPlayer(zPlayer* const player, bool clear) {
    resortPlayersToo = false;

    if (player == 0) {
        return true;
    }

    PadData* p = FindPadData(player);

    if (p != 0) {
        p->owner = 0;
        return true;
    }

    return false;
}

zPlayerInput* zPlayerInputPadMgr::ClaimAIPad(zPlayer* player) {
    zPlayerInput* input = player->playerInput;

    if (input == 0 || input->GetInputType() == 1) {
        return 0;
    }

    for (int i = 0; i < 4; i++) {
        if (humanPads[i].owner == player) {
            if (aiPads[i].owner == 0) {
                aiPads[i].owner = player;
                return aiPads[i].pad;
            }
        }
    }

    return 0;
}

zPlayerInput* zPlayerInputPadMgr::RestoreHumanPad(zPlayer* player) {
    zPlayerInput* input = player->playerInput;

    if (input == 0 || input->GetInputType() != 1) {
        return 0;
    }

    for (int i = 0; i < 4; i++) {
        if (aiPads[i].owner == player && humanPads[i].owner == player) {
            aiPads[i].owner = 0;
            return humanPads[i].pad;
        }
    }

    return 0;
}

// NEAR MISS: 33 of 173 words, all in the first bubble loop: the volatile
// registers one lower than retail's (x, low and high in r6-r8 where retail
// has r7-r9), and high loaded before low.
void zPlayerInputPadMgr::reSort() {
    int numHumans = 0;

    for (int i = 0; i < xglobals->players.numPlayers; i++) {
        if (!IsAI(i)) {
            numHumans++;
        }
    }

    if (numHumans <= 2) {
        for (int x = 0; x < 4u; x++) {
            unsigned int low = x;
            unsigned int high = x + 1;

            if (high == 4) {
                low = 0;
                high = 3;
            }

            if (humanPads[low].gamePort > humanPads[high].gamePort) {
                unsigned int tempPort = humanPads[low].gamePort;
                zPlayerInput* tempPad = humanPads[low].pad;

                humanPads[low].gamePort = humanPads[high].gamePort;
                humanPads[low].pad = humanPads[high].pad;
                humanPads[high].gamePort = tempPort;
                humanPads[high].pad = tempPad;
                humanPads[high].originalPort = high;
                humanPads[low].originalPort = low;

                if (resortPlayersToo) {
                    if (humanPads[low].owner == 0 &&
                        humanPads[high].owner != 0) {
                        zPlayer* tempOwner = humanPads[low].owner;
                        humanPads[low].owner = aiPads[low].owner;
                        aiPads[low].owner = tempOwner;

                        tempOwner = humanPads[high].owner;
                        humanPads[high].owner = aiPads[high].owner;
                        aiPads[high].owner = tempOwner;
                    }
                }

                x = -1;
            }
        }

        if (resortPlayersToo) {
            bool switched = true;

            while (switched) {
                switched = false;

                for (unsigned int counter = 0; counter < 3; counter++) {
                    unsigned int y = counter + 1;

                    if (humanPads[counter].owner == 0 &&
                        humanPads[y].owner != 0) {
                        aiPads[y].owner = humanPads[y].owner;
                        humanPads[y].owner = 0;
                        humanPads[counter].owner = aiPads[counter].owner;
                        aiPads[counter].owner = 0;

                        zPlayerInput* pad = humanPads[counter].pad;
                        humanPads[counter].pad = humanPads[y].pad;
                        humanPads[y].pad = pad;

                        unsigned int port = humanPads[counter].gamePort;
                        humanPads[counter].gamePort = humanPads[y].gamePort;
                        humanPads[y].gamePort = port;

                        port = humanPads[counter].originalPort;
                        humanPads[counter].originalPort =
                            humanPads[y].originalPort;
                        humanPads[y].originalPort = port;

                        switched = true;
                    }
                }
            }
        }

        resortPlayersToo = true;

        for (int x = 0; x < 4u; x++) {
            if (humanPads[x].owner != 0) {
                bool switchToHuman = humanPads[x].owner->IsAI();

                xglobals->players.playerArray[x]->playerInput =
                    humanPads[x].pad;

                if (switchToHuman) {
                    xglobals->players.playerArray[x]->SwitchedToHuman(true);
                }
            } else if (aiPads[x].owner != 0) {
                bool switchToAI = !aiPads[x].owner->IsAI();

                xglobals->players.playerArray[x]->playerInput = aiPads[x].pad;

                if (switchToAI) {
                    xglobals->players.playerArray[x]->SwitchedToAI(true);
                }
            }
        }
    }
}

bool zPlayerInputPadMgr::AcquirePad(IO::Pad* pad) {
    if (pad == 0) {
        return false;
    }

    int i = 0;
    int firstNull = -1;

    for (; i < 4; i++) {
        if (acquiredPads[i] == pad) {
            return false;
        }

        if (firstNull == -1 && acquiredPads[i] == 0) {
            firstNull = i;
        }
    }

    if (!pad->acquired) {
        pad->acquired = true;
    }

    acquiredPads[firstNull] = pad;
    return true;
}

bool zPlayerInputPadMgr::ReleasePad(IO::Pad* pad) {
    if (pad == 0) {
        return true;
    }

    for (int i = 0; i < 4; i++) {
        if (acquiredPads[i] == pad) {
            acquiredPads[i] = 0;
            return true;
        }
    }

    return false;
}

PadData* zPlayerInputPadMgr::FindPadData(zPlayer* const player) {
    for (int i = 0; i < 4; i++) {
        if (humanPads[i].owner == player) {
            return &humanPads[i];
        }

        if (aiPads[i].owner == player) {
            return &aiPads[i];
        }
    }

    return 0;
}

unsigned int zPlayerInputPadMgr::GetGamePort(zPlayerInput* key) {
    if (key == 0) {
        return 0x7FC00000;
    }

    if (key->GetInputType() == 1) {
        return 0x7FC00000;
    }

    for (int x = 0; x < 4; x++) {
        if (humanPads[x].pad == key) {
            return humanPads[x].gamePort;
        }
    }

    return 0x7FC00000;
}

unsigned int zPlayerInputPadMgr::GetGamePort(zPlayer* player) {
    if (player == 0) {
        return 0x7FC00000;
    }

    for (int x = 0; x < 4; x++) {
        if (humanPads[x].owner == player || aiPads[x].owner == player) {
            return humanPads[x].gamePort;
        }
    }

    return 0x7FC00000;
}

zPlayerInput* zPlayerInputPadMgr::GetPadAtGamePort(int port) {
    for (int x = 0; x < 4; x++) {
        if (port == humanPads[x].gamePort) {
            return humanPads[x].pad;
        }
    }

    return 0;
}

// The first pad that is set: each test's true arm is the pad itself, so
// the found value falls to the single store at the foot.
zPlayerInput* zPlayerInputPadMgr::GetDebugPad() {
    return curDebugPad[0] == 0
               ? (curDebugPad[1] == 0
                      ? (curDebugPad[2] == 0
                             ? (curDebugPad[3] == 0 ? 0 : curDebugPad[3])
                             : curDebugPad[2])
                      : curDebugPad[1])
               : curDebugPad[0];
}

zPlayer* zPlayerInputPadMgr::GetPadOwner(zPlayerInput* pad) {
    for (int i = 0; i < 4; i++) {
        if (humanPads[i].pad == pad) {
            return humanPads[i].owner;
        }

        if (aiPads[i].pad == pad) {
            return aiPads[i].owner;
        }
    }

    return 0;
}

bool zPlayerInputPadMgr::UserSceneReset() {
    if (dropDelay > 0.0f) {
        dropDelay -= globals.update_dt;

        if (dropDelay < 0.0f) {
            dropDelay = 0.0f;
        }
    }

    joinDropWait = 0;

    for (int i = 0; i < 4u; i++) {
        if (humanPads[i].owner != 0 && humanPads[i].pad->IsResetPending()) {
            return true;
        }
    }

    return false;
}

zPlayerInput* zPlayerInputPadMgr::CheckForJoin(zPlayer* player,
                                               bool forceJoin) {
    int j;

    if (!forceJoin) {
        if (!player->CanJoinOrDrop()) {
            return 0;
        }

        int sum = 0;

        for (int i = 0; i < xglobals->players.numPlayers; i++) {
            if (!IsAI(i)) {
                sum++;
            }
        }

        if (sum > 1) {
            return 0;
        }
    }

    for (unsigned int x = 0; x < 4; x++) {
        if (humanPads[x].pad->Pressed((zPlayerInputNS::Action)21, false,
                                      true) ||
            forceJoin) {
            if (humanPads[x].owner == 0) {
                if (!humanPads[x].pad->IsPadDisabled() &&
                    humanPads[x].owner == 0) {
                    j = GetAIPlayer(player);

                    if (j >= 0) {
                        if (j != x) {
                            zPlayerInput* pad = humanPads[x].pad;
                            humanPads[x].pad = humanPads[j].pad;
                            humanPads[j].pad = pad;

                            unsigned int port = humanPads[x].gamePort;
                            humanPads[x].gamePort = humanPads[j].gamePort;
                            humanPads[j].gamePort = port;

                            port = humanPads[x].originalPort;
                            humanPads[x].originalPort =
                                humanPads[j].originalPort;
                            humanPads[j].originalPort = port;
                        }

                        humanPads[j].owner = aiPads[j].owner;
                        aiPads[j].owner = 0;
                        humanPads[j].owner->playerInput = humanPads[j].pad;
                        humanPads[j].owner->SwitchedToHuman(true);

                        globals.player2Port = humanPads[j].originalPort;

                        TRC::trcModule->trcPadManager.EnforcePadActivity(
                            humanPads[j].gamePort);

                        if (globals.playerPort == humanPads[j].gamePort) {
                            TRC::trcModule->trcPadManager
                                .EnableAccessoryActivity(
                                    humanPads[j].gamePort,
                                    TRC::TRCPadManager::ACCESSORY_1);
                        }

                        EventActionHudSetPlayer params;
                        params.playerIdx =
                            zPlayerInputNS::padManager.GetPlayerIndex(
                                humanPads[j].owner);
                        params.characterID = humanPads[j].owner->eName;
                        zEntEventAllOfType(0, 0, 0x71D85377, &params, 218,
                                           (ForceEvent)1);

                        EventActionOneInt params2;
                        params2.param0 = 1;
                        zEntEventAllOfType(0, 0, 0x4F469486, &params2, 218,
                                           (ForceEvent)1);

                        if (joinTextBoxScript != 0) {
                            zEntEvent(0, 0, (xBase*)joinTextBoxScript,
                                      0x15A4AF, 0, (ForceEvent)1);
                        }

                        if (firstJoin) {
                            firstJoin = false;
                            planktonTutorialMsgBox.message =
                                "=MNU_PLANKTONTUTORIAL_CONFIRM";
                            planktonTutorialMsgBox.yesText =
                                "=MNU_NAVHELP_YES";
                            planktonTutorialMsgBox.noText = "=MNU_NAVHELP_NO";
                            planktonTutorialMsgBox.promptType = 5;
                            planktonTutorialMsgBox.msgType = 6;
                            planktonTutorialMsgBox.inputType =
                                1 << zPlayerInputNS::padManager.GetGamePort(
                                         player);
                            planktonTutorialMsgBox.image = 3;
                            TRC::MsgBoxManager::Instance()->Add(
                                &planktonTutorialMsgBox);
                            planktonTutorialBoxInUse = j;
                        }

                        joinDropWait = 4;
                        dropDelay = 2.0f;

                        TRC::trcModule->trcPadManager.DisablePadTRC(
                            humanPads[j].gamePort);

                        return humanPads[j].pad;
                    }
                }
            }
        }
    }

    return 0;
}

// NEAR MISS: 3 of 194 words; retail shifts the box index in place (slwi
// r25,r25,4), ours into r31.
zPlayerInput* zPlayerInputPadMgr::CheckForDrop(zPlayer* player,
                                               bool forceDrop) {
    int box;

    if (dropDelay > 0.0f) {
        return 0;
    }

    if (!forceDrop) {
        if (!player->CanJoinOrDrop()) {
            return 0;
        }

        int sum = 0;

        for (int i = 0; i < xglobals->players.numPlayers; i++) {
            if (!IsAI(i)) {
                sum++;
            }
        }

        if (sum <= 1) {
            return 0;
        }
    }

    if (player->playerInput->Pressed((zPlayerInputNS::Action)21, false,
                                     true)) {
        forceDrop = true;
    }

    if (forceDrop) {
        for (int i = 0; i < 4; i++) {
            if (humanPads[i].owner == player &&
                globals.playerPort == humanPads[i].gamePort) {
                return 0;
            }

            if (humanPads[i].owner == player && aiPads[i].owner == 0) {
                joinDropBoxInUse = i;
            }
        }
    }

    box = joinDropBoxInUse;

    if ((box >= 0 && humanPads[box].owner == player) || forceDrop) {
        if (joinDropMsgBox.responseType == 1 || forceDrop) {
            joinDropBoxInUse = -1;

            PadData* pd = FindPadData(player);

            EventActionOneInt params2;
            params2.param0 = 0;
            zEntEventAllOfType(0, 0, 0x4F469486, &params2, 218,
                               (ForceEvent)1);

            TRC::trcModule->trcPadManager.DisablePadActivity(
                humanPads[box].gamePort);

            aiPads[box].owner = pd->owner;
            pd->owner = 0;
            player->playerInput = aiPads[box].pad;
            player->SwitchedToAI(true);

            globals.player2Port = -1;

            zPlayerInput* returnVal = aiPads[box].pad;

            if (joinTextBoxScript != 0) {
                zEntEvent(0, 0, (xBase*)joinTextBoxScript, 0x8397793C, 0,
                          (ForceEvent)1);
            }

            if (joinTextBox != 0) {
                zEntEvent(0, 0, (xBase*)joinTextBox, 0xE86017A9, 0,
                          (ForceEvent)1);
            }

            if (noAchTextBox != 0) {
                zEntEvent(0, 0, (xBase*)noAchTextBox, 0xE86017A9, 0,
                          (ForceEvent)1);
            }

            EventActionHudSetPlayer params;
            params.playerIdx = 1;
            params.characterID = -1;
            zEntEventAllOfType(0, 0, 0x71D85377, &params, 218,
                               (ForceEvent)1);

            joinDropWait = 4;
            return returnVal;
        } else if (joinDropMsgBox.responseType == 2) {
            joinDropBoxInUse = -1;
        }
    }

    return 0;
}

zPlayerInputPadMgr* zPlayerInputNS::GetPadManager() {
    return &padManager;
}

unsigned int zPlayerInputNS::GetGamePort(zPlayerInput* key) {
    return padManager.GetGamePort(key);
}

zPlayerInput* zPlayerInputNS::GetPadAtGamePort(int port) {
    return padManager.GetPadAtGamePort(port);
}

zPlayerInput* zPlayerInputNS::GetDebugPad() {
    return padManager.GetDebugPad();
}

bool zPlayerInputNS::UserSceneReset() {
    return padManager.UserSceneReset();
}

int zPlayerInputPadMgr::GetPlayerIndex(zPlayer* player) const {
    if (player == 0) {
        return -1;
    }

    for (int i = 0; i < 4; i++) {
        if (humanPads[i].owner == player || aiPads[i].owner == player) {
            if (globals.playerPort == humanPads[i].gamePort) {
                return 0;
            }

            return 1;
        }
    }

    return -1;
}

int zPlayerInputPadMgr::GetAIPlayer(zPlayer* player) const {
    for (int i = 0; i < 4; i++) {
        if (aiPads[i].owner == player) {
            return i;
        }
    }

    return -1;
}

bool zPlayerInputPadMgr::IsAI(unsigned int index) {
    return humanPads[index].owner == 0;
}

zPlayer* zPlayerInputPadMgr::GetPlayerFromPlayerInput(zPlayerInput* input) {
    for (int i = 0; i < 4; i++) {
        if (aiPads[i].pad == input) {
            return aiPads[i].owner;
        }

        if (humanPads[i].pad == input) {
            return humanPads[i].owner;
        }
    }

    return 0;
}

zPlayerInput* zPlayerInputPadMgr::GetBalancePad() {
    for (int i = 0; i < 4u; i++) {
        zPlayerInput* curPad = humanPads[i].pad;

        if (curPad != 0 && !curPad->IsPadDisabled() &&
            curPad->GetPadSubLayout() == 3) {
            return curPad;
        }
    }

    return 0;
}
