// PowerControl.cpp -- seven functions, read from the image with
// tools/disasm.py. Four callbacks (reset, power, Wii menu, data manager)
// each refuse when a shutdown is already in progress and otherwise raise
// the shutdown flag and exactly one of the three destination flags.
// SetShutdownCallback swaps the game's veto callback and returns the old
// one; IsShutdownInProgress reads the flag. ProcessShutdown does the
// work once the flag is up: blanks the screen, reconfigures video for
// the TV format, blanks again, gives the callback its veto, stops the
// rumble, recalibrates the pads, and restarts, returns to the menu,
// returns to the data manager or shuts the system down, saying which on
// the console.
//
// The flags are reached relative to the unity unit's .bss section base:
// retail's ...bss.0 is Loader's memCB_domDir at 0x80779F70 and
// `shutdown` sits 0x8A70 past it, one `addis` up, so this fragment
// carries that distance as an unreferenced array ahead of the flags and
// the compiler forms the same base for four or more accesses. That
// array is data read from the image so the fragment can be compared;
// a fragment to be LINKED needs the unity unit. The strings are the
// unity unit's pool, hence the generated header first.

#include "SB/NG/Source/Engine/TRC/PowerControl.pool.h"

struct _GXRenderModeObj;

extern "C" {
extern _GXRenderModeObj GXNtsc480IntDf;
extern _GXRenderModeObj GXPal528IntDf;
extern _GXRenderModeObj GXEurgb60Hz480IntDf;

void VISetBlack(int black);
void VIFlush(void);
void VIWaitForRetrace(void);
unsigned int VIGetTvFormat(void);
void VIConfigure(const _GXRenderModeObj* rm);
void OSReport(const char* fmt, ...);
void OSRestart(unsigned int resetCode);
void OSReturnToMenu(void);
void OSReturnToDataManager(void);
void OSShutdownSystem(void);
void PADRecalibrate(unsigned int mask);
}

namespace IO {

class ConsolePadDevice {
public:
    static void StopAllRumble();
};

}  // namespace IO

// The unity unit's .bss ahead of the flags, measured (memCB_domDir to
// shutdown). Referenced by nothing and holds nothing.
static unsigned char kUnityBssAhead[0x8A70];

namespace TRC {

typedef bool (*ShutdownCallback)();

// Volatile: the callbacks run from the system's reset, power and menu
// hooks and the main loop polls the flags. Non-volatile, two of the
// four callbacks materialise their zero one slot early (2026-09-02).
volatile bool shutdown;
volatile bool restartOnShutdown;
volatile bool wiiMenuOnShutdown;
volatile bool dataMgrOnShutdown;
static ShutdownCallback shutdownCallback;

void ResetCallback();
void PowerCallback();
void WiiMenuCallback();
void WiiDataMgrCallback();
ShutdownCallback SetShutdownCallback(ShutdownCallback callback);
bool IsShutdownInProgress();
void ProcessShutdown();

}  // namespace TRC

void TRC::ResetCallback() {
    if (shutdown) {
        return;
    }

    shutdown = true;
    restartOnShutdown = true;
    wiiMenuOnShutdown = false;
    dataMgrOnShutdown = false;
}

void TRC::PowerCallback() {
    if (shutdown) {
        return;
    }

    shutdown = true;
    restartOnShutdown = false;
    wiiMenuOnShutdown = false;
    dataMgrOnShutdown = false;
}

void TRC::WiiMenuCallback() {
    if (shutdown) {
        return;
    }

    shutdown = true;
    restartOnShutdown = false;
    wiiMenuOnShutdown = true;
    dataMgrOnShutdown = false;
}

void TRC::WiiDataMgrCallback() {
    if (shutdown) {
        return;
    }

    shutdown = true;
    restartOnShutdown = false;
    wiiMenuOnShutdown = false;
    dataMgrOnShutdown = true;
}

TRC::ShutdownCallback TRC::SetShutdownCallback(ShutdownCallback callback) {
    ShutdownCallback old = shutdownCallback;

    shutdownCallback = callback;

    return old;
}

bool TRC::IsShutdownInProgress() {
    return shutdown;
}

void TRC::ProcessShutdown() {
    if (!shutdown) {
        return;
    }

    VISetBlack(1);
    VIFlush();
    VIWaitForRetrace();

    const _GXRenderModeObj* renderMode;

    switch (VIGetTvFormat()) {
    case 0:
        renderMode = &GXNtsc480IntDf;
        break;
    case 1:
        renderMode = &GXPal528IntDf;
        break;
    case 5:
        renderMode = &GXEurgb60Hz480IntDf;
        break;
    default:
        renderMode = &GXNtsc480IntDf;
        break;
    }

    VIConfigure(renderMode);

    VISetBlack(1);
    VIFlush();
    VIWaitForRetrace();

    bool proceed = true;

    if (shutdownCallback) {
        proceed = shutdownCallback();
    }

    if (!proceed) {
        return;
    }

    IO::ConsolePadDevice::StopAllRumble();
    PADRecalibrate(0xF0000000);

    if (restartOnShutdown) {
        OSReport("OSRestart called!\n");
        OSRestart(0);
    } else if (wiiMenuOnShutdown) {
        OSReport("OSReturnToMenu called!\n");
        OSReturnToMenu();
    } else if (dataMgrOnShutdown) {
        OSReport("OSReturnToDataManager called!\n");
        OSReturnToDataManager();
    } else {
        OSReport("OSShutdownSystem called!\n");
        OSShutdownSystem();
    }
}
