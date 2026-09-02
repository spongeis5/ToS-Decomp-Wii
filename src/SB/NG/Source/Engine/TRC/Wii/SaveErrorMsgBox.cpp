// SaveErrorMsgBox.cpp -- two functions, read from the image with
// tools/disasm.py. Init stores the type and fills the message box's
// strings, image, prompt, message and input types for each of the
// seven kinds; the strings are the unity pool's, hence the generated
// header first. Close runs the base close, and unless the response is
// none dispatches on the type: the two NAND02/NAND03 boxes call the
// data manager back on a second-button response, the two NAND05/NAND06
// boxes call the menu back, and the corrupt-data box hands the NAND07
// handler the response as a result. Layouts from the DWARF (TRCMsgBox
// 0x34 with its vptr after the members, SaveErrorMsgBox 0x38 with the
// type at +0x34).

#include "SB/NG/Source/Engine/TRC/Wii/SaveErrorMsgBox.pool.h"

namespace TRC {

enum eSaveErrorMsgBoxType {
    eSaveErrorMsgBox_NAND02 = 0,
    eSaveErrorMsgBox_NAND03 = 1,
    eSaveErrorMsgBox_NAND05 = 2,
    eSaveErrorMsgBox_NAND06 = 3,
    eSaveErrorMsgBox_Corrupt = 4,
    eSaveErrorMsgBox_NAND08 = 5,
    eSaveErrorMsgBox_NAND11 = 6
};

enum eNAND07Result { eNAND07Result_ = 0x7FFFFFFF };

class TRCMsgBox {
public:
    enum ResponseType {
        eResponseNone = -1,
        eResponseFirst = 1,
        eResponseSecond = 2
    };

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

    // The vptr follows the members (0x34 bytes, the DWARF's size); the
    // undefined virtual ahead of Close keeps the vtable's home elsewhere.
    virtual void __key();
    virtual void Close(ResponseType response);
};

class SaveErrorMsgBox : public TRCMsgBox {
public:
    void Init(eSaveErrorMsgBoxType type);
    void Close(TRCMsgBox::ResponseType response);

    eSaveErrorMsgBoxType type;
};

class SaveLoad {
public:
    void NAND07Handler(eNAND07Result result);
};

class TRCModule {
public:
    unsigned char _pad0[0x278];
    SaveLoad saveLoad;
};

extern TRCModule* trcModule;

void WiiDataMgrCallback();
void WiiMenuCallback();

}  // namespace TRC

void TRC::SaveErrorMsgBox::Init(eSaveErrorMsgBoxType t) {
    type = t;

    switch (t) {
    case eSaveErrorMsgBox_NAND02:
        message = "=MNUI_SAVELOAD_WII_NAND02";
        yesText = "=MNUS_SAVELOAD_WII_CONTINUE";
        noText = "=MNUS_SAVELOAD_WII_MENU";
        image = 2;
        promptType = 4;
        msgType = 4;
        inputType = 128;
        break;
    case eSaveErrorMsgBox_NAND03:
        message = "=MNUI_SAVELOAD_WII_NAND03";
        yesText = "=MNUS_SAVELOAD_WII_CONTINUE";
        noText = "=MNUS_SAVELOAD_WII_MENU";
        image = 2;
        promptType = 4;
        msgType = 4;
        inputType = 128;
        break;
    case eSaveErrorMsgBox_NAND05:
        message = "=MNUI_SAVELOAD_WII_NAND05";
        yesText = "=MNUS_NAVHELP_ACCEPT";
        image = 2;
        promptType = 1;
        msgType = 4;
        inputType = 128;
        break;
    case eSaveErrorMsgBox_NAND06:
        message = "=MNUI_SAVELOAD_WII_NAND06";
        yesText = "=MNUS_NAVHELP_ACCEPT";
        image = 2;
        promptType = 1;
        msgType = 4;
        inputType = 128;
        break;
    case eSaveErrorMsgBox_Corrupt:
        message = "=UI_TXT_DATA_CORRUPT";
        yesText = "=UI_TXT_DAMAGE_MC";
        noText = "=UI_TXT_CORRUPT_DATA";
        image = 2;
        promptType = 3;
        msgType = 4;
        inputType = 128;
        break;
    case eSaveErrorMsgBox_NAND08:
        message = "=MNUI_SAVELOAD_WII_NAND08";
        image = 2;
        promptType = 0;
        msgType = 1;
        inputType = 0;
        break;
    case eSaveErrorMsgBox_NAND11:
        message = "=MNUI_SAVELOAD_WII_NAND11";
        image = 2;
        promptType = 0;
        msgType = 1;
        inputType = 0;
        break;
    }
}

void TRC::SaveErrorMsgBox::Close(TRCMsgBox::ResponseType response) {
    TRCMsgBox::Close(response);

    if (response == eResponseNone) {
        return;
    }

    switch (type) {
    case eSaveErrorMsgBox_NAND02:
        if (response == eResponseSecond) {
            WiiDataMgrCallback();
        }
        break;
    case eSaveErrorMsgBox_NAND03:
        if (response == eResponseSecond) {
            WiiDataMgrCallback();
        }
        break;
    case eSaveErrorMsgBox_NAND05:
        WiiMenuCallback();
        break;
    case eSaveErrorMsgBox_NAND06:
        WiiMenuCallback();
        break;
    case eSaveErrorMsgBox_Corrupt:
        if (response == eResponseFirst) {
            trcModule->saveLoad.NAND07Handler((eNAND07Result)0);
        } else if (response == eResponseSecond) {
            trcModule->saveLoad.NAND07Handler((eNAND07Result)1);
        }
        break;
    }
}
