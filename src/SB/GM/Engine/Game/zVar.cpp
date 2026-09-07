// zVar.cpp -- the text-substitution variables the UI puts inside a
// string: 79 functions, 5,532 bytes. Two families live here. The
// var_text_* ones format a string into a static buffer of their own and
// hand it back; the zVarEntryCB_* ones answer a question about an object
// the caller passes as a void*, and are the callbacks a zVar table
// entry points at.
//
// The callbacks are what this file starts with, because most are one
// expression and the shapes repeat: seventeen ask whether the current
// localisation is a subset of one language id, and the rest read a
// field off the context or off the game's globals.

class zCounter {
public:
    unsigned char _pad0[0x40];
    short count;
};

class zReference {
public:
    unsigned char _pad0[0x3C];
    void* target;
};

// The type id word is at +0x26 and the flag word at +0x48. The bit
// IsVisible reads is the same one -- +0x48, eight from the top -- that
// zNPCPerception's node check reads before testing any shape.
class xEnt {
public:
    unsigned char _pad0[0x26];
    unsigned short baseFlags;
    unsigned char _pad1[0x48 - 0x28];
    unsigned int _bits0 : 7;
    unsigned int visible : 1;
    unsigned int _bits1 : 24;
};

class zPlayerActionManager {
public:
    unsigned int GetCurrentActionID() const;

    unsigned char _pad0[0x4];
};

class zScene {
public:
    unsigned int sceneID;
};

class zTimer {
public:
    unsigned char _pad0[0x40];
    unsigned char state;
    unsigned char _pad1[0x44 - 0x41];
    float secondsLeft;
};

// The health question goes through the vtable at +0x150, which is
// slot 82 counting the first virtual as slot 0; the slots before it
// exist only to put it there and none of them is named by anything
// this file can read.
class zPlayerCommon {
public:
    virtual void _v0();  virtual void _v1();  virtual void _v2();  virtual void _v3();
    virtual void _v4();  virtual void _v5();  virtual void _v6();  virtual void _v7();
    virtual void _v8();  virtual void _v9();  virtual void _v10();  virtual void _v11();
    virtual void _v12();  virtual void _v13();  virtual void _v14();  virtual void _v15();
    virtual void _v16();  virtual void _v17();  virtual void _v18();  virtual void _v19();
    virtual void _v20();  virtual void _v21();  virtual void _v22();  virtual void _v23();
    virtual void _v24();  virtual void _v25();  virtual void _v26();  virtual void _v27();
    virtual void _v28();  virtual void _v29();  virtual void _v30();  virtual void _v31();
    virtual void _v32();  virtual void _v33();  virtual void _v34();  virtual void _v35();
    virtual void _v36();  virtual void _v37();  virtual void _v38();  virtual void _v39();
    virtual void _v40();  virtual void _v41();  virtual void _v42();  virtual void _v43();
    virtual void _v44();  virtual void _v45();  virtual void _v46();  virtual void _v47();
    virtual void _v48();  virtual void _v49();  virtual void _v50();  virtual void _v51();
    virtual void _v52();  virtual void _v53();  virtual void _v54();  virtual void _v55();
    virtual void _v56();  virtual void _v57();  virtual void _v58();  virtual void _v59();
    virtual void _v60();  virtual void _v61();  virtual void _v62();  virtual void _v63();
    virtual void _v64();  virtual void _v65();  virtual void _v66();  virtual void _v67();
    virtual void _v68();  virtual void _v69();  virtual void _v70();  virtual void _v71();
    virtual void _v72();  virtual void _v73();  virtual void _v74();  virtual void _v75();
    virtual void _v76();  virtual void _v77();  virtual void _v78();  virtual void _v79();
    virtual void _v80();  virtual void _v81();
    virtual float GetHealthFraction();

    unsigned char _pad0[0x1C];
    unsigned int baseType;
    unsigned char _pad1[0xC0 - 0x24];
    zPlayerActionManager actionManager;
    unsigned char _pad2[0x8B0 - 0xC4];
    int spongeBuffState;
};

class zGlobals {
public:
    unsigned char _pad0[0x308];
    void* straightToMainMenu;
    unsigned char _pad5[0x390 - 0x30C];
    unsigned int vibrationOnP1;
    unsigned char _pad1[0x3A4 - 0x394];
    unsigned int subtitlesOn;
    unsigned char _pad2[0x43C - 0x3A8];
    zScene* sceneCur;
    unsigned char _pad6[0x4C5 - 0x440];
    bool showHints;
    unsigned char _pad7[0x4D3 - 0x4C6];
    bool padFlipped;
    unsigned char _pad3[0x588 - 0x4D4];
    int demoType;
    int subGameType;
    unsigned char _pad4[0x5A0 - 0x590];
};

extern zGlobals globals;

// The four UI glyph strings, each a pointer this file only reads.
extern const char* ACCECTP_PAD_DEFAULT;
extern const char* ACCECTP_PAD_FLIPPED;
extern const char* BACK_PAD_DEFAULT;
extern const char* BACK_PAD_FLIPPED;

// A scene id, not a constant this file can spell.
extern unsigned int SCENE_ID_MNU_START;

// float -> unsigned, which is what a cast of one to the other calls.
extern "C" unsigned int __cvt_fp2unsigned(float f);

// The localisation the title is running in, and whether it falls inside
// a language group. The ids are the image's; their names are not.
int xSTGetLocalizationEnum();
bool IsSubSetOfLanguage(unsigned short current, unsigned short language);

int zVarEntryCB_CounterValue(void* context) {
    return ((zCounter*)context)->count;
}

bool zVarEntryCB_IsEnabled(void* context) {
    return ((xEnt*)context)->baseFlags & 1;
}

bool zVarEntryCB_IsVisible(void* context) {
    return ((xEnt*)context)->visible;
}

bool zVarEntryCB_IsReferenceNULL(void* context) {
    return ((zReference*)context)->target == 0;
}

int zVarEntryCB_SubGameType(void* context) {
    return globals.subGameType;
}

int zVarEntryCB_DemoType(void* context) {
    return globals.demoType;
}

bool zVarEntryCB_ShowHints(void* context) {
    return globals.showHints;
}

bool zVarEntryCB_VibrationOnP1(void* context) {
    return globals.vibrationOnP1 & 1;
}

bool zVarEntryCB_SubtitlesOn(void* context) {
    return globals.subtitlesOn & 1;
}

bool zVarEntryCB_IsPlayerSpongeBuff(void* context) {
    return ((zPlayerCommon*)context)->spongeBuffState == 1;
}

bool zVarEntryCB_CheatMinigamesUnlocked(void* context) {
    return ((zCounter*)context)->count > 0;
}

bool zVarEntryCB_ShowEnglishVideos(void* context) {
    return IsSubSetOfLanguage(xSTGetLocalizationEnum(), 9);
}

bool zVarEntryCB_IsChinese(void* context) {
    return IsSubSetOfLanguage(xSTGetLocalizationEnum(), 4);
}

bool zVarEntryCB_IsDanish(void* context) {
    return IsSubSetOfLanguage(xSTGetLocalizationEnum(), 6);
}

bool zVarEntryCB_IsGerman(void* context) {
    return IsSubSetOfLanguage(xSTGetLocalizationEnum(), 7);
}

bool zVarEntryCB_IsGreek(void* context) {
    return IsSubSetOfLanguage(xSTGetLocalizationEnum(), 8);
}

bool zVarEntryCB_IsSpanish(void* context) {
    return IsSubSetOfLanguage(xSTGetLocalizationEnum(), 10);
}

bool zVarEntryCB_IsFinnish(void* context) {
    return IsSubSetOfLanguage(xSTGetLocalizationEnum(), 11);
}

bool zVarEntryCB_IsFrench(void* context) {
    return IsSubSetOfLanguage(xSTGetLocalizationEnum(), 12);
}

bool zVarEntryCB_IsItalian(void* context) {
    return IsSubSetOfLanguage(xSTGetLocalizationEnum(), 16);
}

bool zVarEntryCB_IsJapanese(void* context) {
    return IsSubSetOfLanguage(xSTGetLocalizationEnum(), 17);
}

bool zVarEntryCB_IsKorean(void* context) {
    return IsSubSetOfLanguage(xSTGetLocalizationEnum(), 18);
}

bool zVarEntryCB_IsDutch(void* context) {
    return IsSubSetOfLanguage(xSTGetLocalizationEnum(), 19);
}

bool zVarEntryCB_IsNorwegian(void* context) {
    return IsSubSetOfLanguage(xSTGetLocalizationEnum(), 20);
}

bool zVarEntryCB_IsPolish(void* context) {
    return IsSubSetOfLanguage(xSTGetLocalizationEnum(), 21);
}

bool zVarEntryCB_IsPortuguese(void* context) {
    return IsSubSetOfLanguage(xSTGetLocalizationEnum(), 22);
}

bool zVarEntryCB_IsRussian(void* context) {
    return IsSubSetOfLanguage(xSTGetLocalizationEnum(), 25);
}

bool zVarEntryCB_IsSwedish(void* context) {
    return IsSubSetOfLanguage(xSTGetLocalizationEnum(), 29);
}

bool zVarEntryCB_IsUkranian(void* context) {
    return IsSubSetOfLanguage(xSTGetLocalizationEnum(), 34);
}

bool zVarEntryCB_IsMNUS(void* context) {
    unsigned int scene = globals.sceneCur->sceneID;
    bool result = true;

    if (scene != SCENE_ID_MNU_START && scene != 0x574D4E53) {
        result = false;
    }

    return result;
}

bool zVarEntryCB_StriaghtToMainMenuOnXbox(void* context) {
    if (globals.demoType != 0) {
        return false;
    }

    return globals.straightToMainMenu == 0;
}

unsigned int zVarEntryCB_TimerSecondsLeftValue(void* context) {
    zTimer* timer = (zTimer*)context;

    if (timer->state == 1) {
        return (unsigned int)timer->secondsLeft;
    }

    return 0;
}

unsigned int zVarEntryCB_TimerMilliSecondsLeftValue(void* context) {
    zTimer* timer = (zTimer*)context;

    if (timer->state == 1) {
        return (unsigned int)(1000.0f * timer->secondsLeft);
    }

    return 0;
}

bool zVarEntryCB_IsPlayersHealthLow(void* context) {
    return ((zPlayerCommon*)context)->GetHealthFraction() <= 2.0f;
}

// The id list arrives as a pointer and a count rather than a
// terminator, and the callers below build theirs on the stack.
bool _PlayerInActionTest(void* context, unsigned int* ids, int count) {
    zPlayerCommon* player = (zPlayerCommon*)context;
    unsigned int action;
    int i;

    if (player == 0 || player->baseType != 0x55) {
        return false;
    }

    action = player->actionManager.GetCurrentActionID();

    for (i = 0; i < count; i++) {
        if (action == *ids) {
            return true;
        }

        ids++;
    }

    return false;
}

bool zVarEntryCB_IsPlayerFalling(void* context) {
    unsigned int ids[2] = {6, 14};

    return _PlayerInActionTest(context, ids, 2);
}

bool zVarEntryCB_IsPlayerDashing(void* context) {
    unsigned int ids[1] = {15};

    return _PlayerInActionTest(context, ids, 1);
}

bool zVarEntryCB_IsPlayerIdling(void* context) {
    unsigned int ids[1] = {0};

    return _PlayerInActionTest(context, ids, 1);
}

bool zVarEntryCB_IsPlayerRunning(void* context) {
    unsigned int ids[3] = {1, 2, 3};

    return _PlayerInActionTest(context, ids, 3);
}

bool zVarEntryCB_IsPlayerSlipping(void* context) {
    unsigned int ids[1] = {4};

    return _PlayerInActionTest(context, ids, 1);
}

const char* var_text_UIAcceptImage() {
    if (globals.padFlipped) {
        return ACCECTP_PAD_FLIPPED;
    }

    return ACCECTP_PAD_DEFAULT;
}

const char* var_text_UICancelImage() {
    if (globals.padFlipped) {
        return BACK_PAD_FLIPPED;
    }

    return BACK_PAD_DEFAULT;
}
