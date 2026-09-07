// zPlayerInputHuman.cpp -- the human player's pad: 77 functions, 13,580
// bytes. Most of the file is accessors that reach through the Pad the
// player is currently bound to, and answer a default when there is none.
//
// zPlayerInputHuman's own layout is from the Wii build's DWARF
// (tools/dwarf_types.py): 0x218, on a zPlayerInput of 4 -- so the word
// at +0 is the vtable pointer -- with curPad at +0x30 and the
// controller flags at +0x78. Pad is NOT in the DWARF, and only the
// offsets these functions touch are recovered.

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

enum PadType {
    PADTYPE_DEBUG = 0,
    PADTYPE_MENU = 1,
    PADTYPE_GAME = 2,
    PADTYPE_MISC = 3,
    PADTYPE_PAUSE = 4,
    PADTYPE_LAST = 5,
};

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
    unsigned char _pad1[0x18 - 0x11];
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
    float GetWMPRollAngle();
    float GetWMPRollSpeed();
    float GetWMPPitchAngle();
    float GetWMPPitchSpeed();
    float GetWMPYawAngle();
    float GetWMPYawSpeed();
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
    float GetStickAng(unsigned int index, zPlayerInputNS::PadType type);
    float GetStickMag(unsigned int index, zPlayerInputNS::PadType type);
    float GetStickNormMag(unsigned int index,
                          zPlayerInputNS::PadType type);
    bool FlyCheatBoost();

    virtual void _v0();
    virtual void _v1();
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

    unsigned char _pad0[0x2C - 0x4];
    zPlayerInputNS::PadType curPadType;
    IO::Pad* curPad;
    unsigned int curPort;
    bool isDisabled;
    bool disabledTypes[5];
    unsigned char _pad3[0x6C - 0x3E];
    int shouldEnableWiiClassicController;
    int shouldEnableGamecubeController;
    int shouldEnableWiiMotionPlus;
    bool wiiClassicControllerEnabled;
    bool gamecubeControllerEnabled;
    bool wiiMotionPlusExtension;
    unsigned char _pad2[0x218 - 0x7B];
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
