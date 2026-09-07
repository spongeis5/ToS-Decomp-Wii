// zPlayerInputHuman.cpp -- the human player's pad: 77 functions, 13,580
// bytes. Most of the file is accessors that reach through the Pad the
// player is currently bound to, and answer a default when there is none.
//
// zPlayerInputHuman's own layout is from the Wii build's DWARF
// (tools/dwarf_types.py): 0x218, on a zPlayerInput of 4 -- so the word
// at +0 is the vtable pointer -- with curPad at +0x30 and the
// controller flags at +0x78. Pad is NOT in the DWARF, and only the
// offsets these functions touch are recovered.

// 0x28 in the DWARF; Reset touches the flag and the counter.
class ShakeRecognition {
public:
    bool shakeInProgress;
    unsigned char _pad0[0x20 - 0x1];
    unsigned int shakeCounter;
    unsigned char _pad1[0x28 - 0x24];
};
class xVec3 {
public:
    float x;
    float y;
    float z;
};
class xVec2 {
public:
    float x;
    float y;
};

namespace Math {

class Vector4 {
public:
    float x;
    float y;
    float z;
    float w;
};

extern const Vector4 vec4Zero;

}  // namespace Math

namespace Sext {

enum ePad { ePad_ = 0x7FFFFFFF };

}  // namespace Sext

namespace zPlayerInputNS {

enum PadButton { PadButton_ = 0x7FFFFFFF };

enum Action { Action_ = 0x7FFFFFFF };

enum Direction { Direction_ = 0x7FFFFFFF };

enum PadType {
    PADTYPE_DEBUG = 0,
    PADTYPE_MENU = 1,
    PADTYPE_GAME = 2,
    PADTYPE_MISC = 3,
    PADTYPE_PAUSE = 4,
    PADTYPE_LAST = 5,
};

// An action maps to a pad type and a button mask, and which table it
// is looked up in depends on the layout the pad reports.
class ButtonMap {
public:
    PadType type;
    unsigned int mask;
};

extern const ButtonMap wiiButtonMap[];
extern const ButtonMap defaultButtonMap[];

}  // namespace zPlayerInputNS

namespace IO {

// Two button words, at +0xC and +0x10; a button counts as released when
// it is in the second and not in the first.
class PadInput {
public:
    // Returns the raw masked word, not a normalised flag.
    int Released(int mask) const;

    unsigned char _pad0[0xC];
    unsigned int a;
    unsigned int b;
};

}  // namespace IO

namespace IO {

class PadNative {
public:
    static void SetGameCanUseWiiMotionPlus(bool on, int port, bool ext);
    static void SetGameCanUseGamecubeController(bool on, int port);
    static void SetGameCanUseWiiClassicController(bool on, int port);
};


class Pad {
public:
    int GetDevicePort();

    unsigned char _pad0[0x8];
    int layout;
    int subLayout;
    bool gamecubeConnected;
    unsigned char _pad1[0x14 - 0x11];
    unsigned int buttons;
    unsigned int pressed;
    unsigned char _pad2[0x2C - 0x1C];
    float stickMag;
    float stickNormMag;
    float stickAng;
    unsigned char _pad7[0x200 - 0x38];
    bool wiiMotionPlusConnected;
    unsigned char _pad3[0x204 - 0x201];
    float wmpPitchSpeed;
    float wmpYawSpeed;
    float wmpRollSpeed;
    float wmpPitchAngle;
    float wmpYawAngle;
    float wmpRollAngle;
    unsigned char _pad4[0x24C - 0x21C];
    float wmpCalibration;
    unsigned char _pad5[0x41C - 0x250];
    Math::Vector4 wmpCorrectedAcc;
};

}  // namespace IO

namespace TRC {

class TRCPadManager {
public:
    enum AccessoryType {
        AccessoryType_Nunchuk = 1,
        AccessoryType_ClassicController = 2,
        AccessoryType_Gamecube = 4,
    };

    bool CheckAccessoryActivity(int port, AccessoryType type);
    bool CheckPreviousAccessoryActivity(int port, AccessoryType type);
    bool CheckAccessoryConnected(int port);
    bool CheckPadConnected(int port);
    void DisableAccessoryActivity(int port, AccessoryType type);
    void EnableAccessoryActivity(int port, AccessoryType type);
};

class TRCModule {
public:
    unsigned char _pad0[0x98];
    TRCPadManager padManager;
};

extern TRCModule* trcModule;

}  // namespace TRC

class zGlobals {
public:
    unsigned char _pad0[0x4D8];
    int activePad;
    unsigned char _pad1[0x4E0 - 0x4DC];
};

extern zGlobals globals;

// 0x218 in the DWARF; the members below are the ones this page reads.
class zPlayerInputHuman {
public:
    int GetPadLayout();
    int GetPadSubLayout();
    void ClearPressed(zPlayerInputNS::PadButton button);
    bool IsWiiMotionPlusConnected();
    float GetWMPCalibration();
    const Math::Vector4& GetWMPCorrectedAcc();
    bool IsGamecubeControllerEnabled();
    bool IsGamecubeControllerPrevious();
    bool IsGamecubeControllerConnected();
    bool IsWiiClassicControllerEnabled();
    bool IsWiiClassicControllerPrevious();
    bool NunchukPrevious();
    void DisableWiiNunchuk();
    void EnableWiiNunchuk();
    void DisableWiiMotionPlus();
    void EnableWiiMotionPlus(bool extension);
    void DisableGamecubeController();
    void EnableGamecubeController();
    void DisableWiiClassicController();
    void EnableWiiClassicController();
    bool IsWiiClassicControllerConnected();
    bool FlyCheatBoost();
    void GetWMPAngles(float* pitch, float* yaw, float* roll);
    void GetWMPAngularSpeeds(float* pitch, float* yaw, float* roll);
    void GetWMPGlobalAngularSpeeds(float* pitch, float* yaw, float* roll);
    xVec2 GetWMPGlobalSwing2D();
    xVec2 GetWMPLocalSwing2D();
    void Reset();
    bool IsControllerConnected();
    zPlayerInputNS::Direction StickOn(unsigned int index,
                                      zPlayerInputNS::PadType type);
    zPlayerInputNS::Direction AngleToDirection(float angle);
    int On(zPlayerInputNS::Action action, bool exact, bool useMap);

    virtual void _v0();
    virtual int GetLayoutKind();
    virtual int GetControllerType();
    virtual void _v3();
    virtual void _v4();
    virtual void _v5();
    virtual void _v6();
    virtual void _v7();
    virtual void _v8();
    virtual void _v9();
    virtual void _v10();
    virtual void _v11();
    virtual void _v12();
    virtual void _v13();
    virtual void _v14();
    virtual void _v15();
    virtual void _v16();
    virtual void _v17();
    virtual void _v18();
    virtual void _v19();
    virtual bool SetRumble(int a, int b, int c);
    virtual void _v21();
    virtual void _v22();
    virtual void _v23();
    virtual bool ValidateStick(unsigned int index,
                               zPlayerInputNS::PadType type);
    virtual void _v25();
    virtual void _v26();
    virtual float GetStickMag(unsigned int index,
                              zPlayerInputNS::PadType type);
    virtual float GetStickNormMag(unsigned int index,
                                  zPlayerInputNS::PadType type);
    virtual float GetStickAng(unsigned int index,
                              zPlayerInputNS::PadType type);
    virtual void _v30();
    virtual void _v31();
    virtual void _v32();
    virtual void _v33();
    virtual void _v34();
    virtual void _v35();
    virtual void _v36();
    virtual void _v37();
    virtual void _v38();
    virtual void _v39();
    virtual void _v40();
    virtual void _v41();
    virtual void _v42();
    virtual void _v43();
    virtual void _v44();
    virtual void _v45();
    virtual void _v46();
    virtual void _v47();
    virtual void _v48();
    virtual void _v49();
    virtual void _v50();
    virtual void _v51();
    virtual void _v52();
    virtual void _v53();
    virtual void _v54();
    virtual void _v55();
    virtual void _v56();
    virtual void _v57();
    virtual void _v58();
    virtual void _v59();
    virtual void _v60();
    virtual float GetWMPPitchSpeed();
    virtual float GetWMPYawSpeed();
    virtual float GetWMPRollSpeed();
    virtual void _v64();
    virtual float GetWMPGlobalPitchSpeed();
    virtual float GetWMPGlobalYawSpeed();
    virtual void _v67();
    virtual float GetWMPPitchAngle();
    virtual float GetWMPYawAngle();
    virtual float GetWMPRollAngle();

    unsigned char _pad0[0x2C - 0x4];
    zPlayerInputNS::PadType curPadType;
    IO::Pad* curPad;
    unsigned int curPort;
    bool isDisabled;
    bool disabledTypes[5];
    unsigned char _pad3[0x40 - 0x3E];
    xVec3 lastAcc;
    xVec3 swingVec;
    float swingSpeedMax;
    xVec3 swingDir;
    bool wasSwinging;
    unsigned char _pad5[0x6C - 0x69];
    int shouldEnableWiiClassicController;
    int shouldEnableGamecubeController;
    int shouldEnableWiiMotionPlus;
    bool wiiClassicControllerEnabled;
    bool gamecubeControllerEnabled;
    bool wiiMotionPlusExtension;
    unsigned char _pad2[0x80 - 0x7B];

    // Five PAIRS, not five records: Reset walks it with an outer
    // stride of 80 and an inner of 40, and five times two times
    // forty lands exactly on the connected flags below.
    ShakeRecognition shakeRecognition[5][2];
    bool wiiRemoteConnected;
    bool nunchukConnected;
    bool wiiClassicControllerConnected;
    unsigned char _pad4[0x218 - 0x213];
};

int IO::PadInput::Released(int mask) const {
    return (mask & b) & ~a;
}

int zPlayerInputHuman::GetPadLayout() {
    return curPad->layout;
}

int zPlayerInputHuman::GetPadSubLayout() {
    return curPad->subLayout;
}

void zPlayerInputHuman::ClearPressed(zPlayerInputNS::PadButton button) {
    IO::Pad* pad = curPad;

    if (pad != 0) {
        pad->pressed = pad->pressed & ~button;
    }
}

bool zPlayerInputHuman::IsWiiMotionPlusConnected() {
    IO::Pad* pad = curPad;

    if (pad != 0) {
        return pad->wiiMotionPlusConnected;
    }

    return false;
}

float zPlayerInputHuman::GetWMPCalibration() {
    IO::Pad* pad = curPad;

    if (pad != 0) {
        return pad->wmpCalibration;
    }

    return -1.0f;
}

float zPlayerInputHuman::GetWMPRollAngle() {
    IO::Pad* pad = curPad;

    if (pad != 0) {
        return pad->wmpRollAngle;
    }

    return 0.0f;
}

float zPlayerInputHuman::GetWMPRollSpeed() {
    IO::Pad* pad = curPad;

    if (pad != 0) {
        return pad->wmpRollSpeed;
    }

    return 0.0f;
}

// The pitch and yaw readings come back with the opposite sign to the
// roll ones, which is the only difference between the four and the two.
float zPlayerInputHuman::GetWMPPitchAngle() {
    IO::Pad* pad = curPad;

    if (pad != 0) {
        return -pad->wmpPitchAngle;
    }

    return 0.0f;
}

float zPlayerInputHuman::GetWMPPitchSpeed() {
    IO::Pad* pad = curPad;

    if (pad != 0) {
        return -pad->wmpPitchSpeed;
    }

    return 0.0f;
}

float zPlayerInputHuman::GetWMPYawAngle() {
    IO::Pad* pad = curPad;

    if (pad != 0) {
        return -pad->wmpYawAngle;
    }

    return 0.0f;
}

float zPlayerInputHuman::GetWMPYawSpeed() {
    IO::Pad* pad = curPad;

    if (pad != 0) {
        return -pad->wmpYawSpeed;
    }

    return 0.0f;
}

const Math::Vector4& zPlayerInputHuman::GetWMPCorrectedAcc() {
    if (isDisabled) {
        return Math::vec4Zero;
    }

    return curPad->wmpCorrectedAcc;
}

bool zPlayerInputHuman::IsGamecubeControllerEnabled() {
    return TRC::trcModule->padManager.CheckAccessoryActivity(
        globals.activePad, TRC::TRCPadManager::AccessoryType_Gamecube);
}

bool zPlayerInputHuman::IsGamecubeControllerPrevious() {
    return TRC::trcModule->padManager.CheckPreviousAccessoryActivity(
        globals.activePad, TRC::TRCPadManager::AccessoryType_Gamecube);
}

bool zPlayerInputHuman::IsWiiClassicControllerEnabled() {
    return TRC::trcModule->padManager.CheckAccessoryActivity(
        globals.activePad,
        TRC::TRCPadManager::AccessoryType_ClassicController);
}

bool zPlayerInputHuman::IsWiiClassicControllerPrevious() {
    return TRC::trcModule->padManager.CheckPreviousAccessoryActivity(
        globals.activePad,
        TRC::TRCPadManager::AccessoryType_ClassicController);
}

bool zPlayerInputHuman::NunchukPrevious() {
    return TRC::trcModule->padManager.CheckPreviousAccessoryActivity(
        globals.activePad, TRC::TRCPadManager::AccessoryType_Nunchuk);
}

bool zPlayerInputHuman::IsGamecubeControllerConnected() {
    if (gamecubeControllerEnabled) {
        IO::Pad* pad = curPad;

        if (pad != 0) {
            return pad->gamecubeConnected;
        }

        return false;
    }

    return false;
}

void zPlayerInputHuman::DisableWiiNunchuk() {
    if (curPad == 0) {
        return;
    }

    TRC::trcModule->padManager.DisableAccessoryActivity(
        globals.activePad, TRC::TRCPadManager::AccessoryType_Nunchuk);
}

void zPlayerInputHuman::EnableWiiNunchuk() {
    if (curPad == 0) {
        return;
    }

    TRC::trcModule->padManager.EnableAccessoryActivity(
        globals.activePad, TRC::TRCPadManager::AccessoryType_Nunchuk);
}

// The pad driver is told directly when there is a pad; with none, the
// wish is remembered in the should-enable word and acted on later.
void zPlayerInputHuman::DisableWiiMotionPlus() {
    IO::Pad* pad = curPad;

    if (pad != 0) {
        IO::PadNative::SetGameCanUseWiiMotionPlus(false,
                                                  pad->GetDevicePort(),
                                                  false);
        return;
    }

    shouldEnableWiiMotionPlus = 2;
}

void zPlayerInputHuman::EnableWiiMotionPlus(bool extension) {
    IO::Pad* pad = curPad;

    if (pad != 0) {
        IO::PadNative::SetGameCanUseWiiMotionPlus(true,
                                                  pad->GetDevicePort(),
                                                  extension);
        return;
    }

    shouldEnableWiiMotionPlus = 1;
    wiiMotionPlusExtension = extension;
}

void zPlayerInputHuman::EnableGamecubeController() {
    IO::Pad* pad = curPad;

    if (pad != 0) {
        IO::PadNative::SetGameCanUseGamecubeController(
            true, pad->GetDevicePort());
        TRC::trcModule->padManager.EnableAccessoryActivity(
            globals.activePad, TRC::TRCPadManager::AccessoryType_Gamecube);
        gamecubeControllerEnabled = true;
        return;
    }

    shouldEnableGamecubeController = 1;
}

bool zPlayerInputHuman::IsWiiClassicControllerConnected() {
    if (wiiClassicControllerEnabled) {
        if (curPad != 0) {
            return GetControllerType() == 2;
        }

        return false;
    }

    return false;
}

bool zPlayerInputHuman::ValidateStick(unsigned int index,
                                      zPlayerInputNS::PadType type) {
    if (isDisabled) {
        return false;
    }

    if (index >= 3) {
        return false;
    }

    if (disabledTypes[type]) {
        return false;
    }

    if (type != 0 && curPadType == 0) {
        return false;
    }

    return true;
}

float zPlayerInputHuman::GetStickAng(unsigned int index,
                                     zPlayerInputNS::PadType type) {
    if (ValidateStick(index, type)) {
        return curPad->stickAng;
    }

    return 0.0f;
}

float zPlayerInputHuman::GetStickMag(unsigned int index,
                                     zPlayerInputNS::PadType type) {
    if (ValidateStick(index, type)) {
        return curPad->stickMag;
    }

    return 0.0f;
}

bool zPlayerInputHuman::FlyCheatBoost() {
    if (isDisabled) {
        return false;
    }

    return SetRumble(64, 0, 1);
}

void zPlayerInputHuman::DisableGamecubeController() {
    IO::Pad* pad = curPad;

    if (pad != 0) {
        IO::PadNative::SetGameCanUseGamecubeController(
            false, pad->GetDevicePort());
        TRC::trcModule->padManager.DisableAccessoryActivity(
            globals.activePad, TRC::TRCPadManager::AccessoryType_Gamecube);
        gamecubeControllerEnabled = false;
        return;
    }

    shouldEnableGamecubeController = 2;
}

void zPlayerInputHuman::EnableWiiClassicController() {
    IO::Pad* pad = curPad;

    if (pad != 0) {
        IO::PadNative::SetGameCanUseWiiClassicController(
            true, pad->GetDevicePort());
        TRC::trcModule->padManager.EnableAccessoryActivity(
            globals.activePad,
            TRC::TRCPadManager::AccessoryType_ClassicController);
        wiiClassicControllerEnabled = true;
        return;
    }

    shouldEnableWiiClassicController = 1;
}

void zPlayerInputHuman::DisableWiiClassicController() {
    IO::Pad* pad = curPad;

    if (pad != 0) {
        IO::PadNative::SetGameCanUseWiiClassicController(
            false, pad->GetDevicePort());
        TRC::trcModule->padManager.DisableAccessoryActivity(
            globals.activePad,
            TRC::TRCPadManager::AccessoryType_ClassicController);
        wiiClassicControllerEnabled = false;
        return;
    }

    shouldEnableWiiClassicController = 2;
}

float zPlayerInputHuman::GetStickNormMag(unsigned int index,
                                         zPlayerInputNS::PadType type) {
    if (ValidateStick(index, type)) {
        return curPad->stickNormMag;
    }

    return 0.0f;
}

void zPlayerInputHuman::GetWMPAngles(float* pitch, float* yaw,
                                     float* roll) {
    *pitch = GetWMPPitchAngle();
    *yaw = GetWMPYawAngle();
    *roll = GetWMPRollAngle();
}

void zPlayerInputHuman::GetWMPAngularSpeeds(float* pitch, float* yaw,
                                            float* roll) {
    *pitch = GetWMPPitchSpeed();
    *yaw = GetWMPYawSpeed();
    *roll = GetWMPRollSpeed();
}

void zPlayerInputHuman::GetWMPGlobalAngularSpeeds(float* pitch,
                                                  float* yaw,
                                                  float* roll) {
    *pitch = GetWMPGlobalPitchSpeed();
    *yaw = GetWMPGlobalYawSpeed();
    *roll = GetWMPRollSpeed();
}

// Two floats out in r3 and r4, which is how a pair this size comes
// back by value.
xVec2 zPlayerInputHuman::GetWMPGlobalSwing2D() {
    xVec2 swing;

    swing.x = GetWMPGlobalPitchSpeed();
    swing.y = GetWMPGlobalYawSpeed();

    return swing;
}

xVec2 zPlayerInputHuman::GetWMPLocalSwing2D() {
    xVec2 swing;

    swing.x = GetWMPPitchSpeed();
    swing.y = GetWMPYawSpeed();

    return swing;
}

void zPlayerInputHuman::Reset() {
    int i;
    int j;

    lastAcc.x = 0.0f;
    lastAcc.y = 0.0f;
    lastAcc.z = 0.0f;
    swingVec.x = 0.0f;
    swingVec.y = 0.0f;
    swingVec.z = 0.0f;
    swingDir.x = 1.0f;
    swingDir.y = 0.0f;
    swingDir.z = 0.0f;
    swingSpeedMax = 0.0f;
    wasSwinging = false;

    for (i = 0; i < 5; i++) {
        for (j = 0; j < 2; j++) {
            shakeRecognition[i][j].shakeInProgress = false;
            shakeRecognition[i][j].shakeCounter = 0;
        }
    }
}

bool zPlayerInputHuman::IsControllerConnected() {
    if (globals.activePad == curPort &&
        TRC::trcModule->padManager.CheckAccessoryActivity(
            globals.activePad,
            TRC::TRCPadManager::AccessoryType_Gamecube)) {
        return true;
    }

    if (globals.activePad == curPort &&
        TRC::trcModule->padManager.CheckAccessoryActivity(
            globals.activePad,
            TRC::TRCPadManager::AccessoryType_ClassicController)) {
        bool connected = false;

        if (TRC::trcModule->padManager.CheckAccessoryConnected(
                globals.activePad) &&
            TRC::trcModule->padManager.CheckPadConnected(curPort)) {
            connected = true;
        }

        return connected;
    }

    return TRC::trcModule->padManager.CheckPadConnected(curPort);
}

zPlayerInputNS::Direction zPlayerInputHuman::StickOn(
    unsigned int index, zPlayerInputNS::PadType type) {
    // One shared exit: retail's two failing tests branch to the same
    // li r3,4, which an early return per test cannot produce.
    if (!ValidateStick(index, type) || GetStickMag(index, type) < 0.5f) {
        return (zPlayerInputNS::Direction)4;
    }

    return AngleToDirection(GetStickAng(index, type));
}

// Every failing path but one falls through to a single return at the
// foot; an early return for the disabled case puts it in the wrong
// place and costs the whole function.
int zPlayerInputHuman::On(zPlayerInputNS::Action action, bool exact,
                          bool useMap) {
    if (!isDisabled) {
        if (useMap) {
            unsigned int mask;
            zPlayerInputNS::PadType type;

            // Both halves read the pair themselves; hoisting a pointer
            // to the entry out of the branch costs four words.
            if (GetLayoutKind() == 4) {
                type = zPlayerInputNS::wiiButtonMap[action].type;
                mask = zPlayerInputNS::wiiButtonMap[action].mask;
            } else {
                type = zPlayerInputNS::defaultButtonMap[action].type;
                mask = zPlayerInputNS::defaultButtonMap[action].mask;
            }

            if (disabledTypes[type]) {
                return 0;
            }

            if (curPadType == type) {
                if (exact) {
                    return curPad->buttons & mask;
                }

                return (mask & curPad->buttons) == mask;
            }

            if (type == zPlayerInputNS::PADTYPE_MISC) {
                if (exact) {
                    return curPad->buttons & mask;
                }

                return (mask & curPad->buttons) == mask;
            }
        } else {
            return (action & curPad->buttons) == action;
        }
    }

    return 0;
}
