#include "SB/GM/Engine/Game/zUI.pool.h"

// zUI.cpp -- the UI element base, read from the image with tools/disasm.py
// and tools/brief.py. Layouts are the DWARF's (zUI 0x100 with the asset at
// +0x40, State current at +0x50 and startMovement at +0x90, the motion at
// +0xEC; Sext::zUI with its flags at +0x50 and its motion and auto-menu
// uids from +0x60), the virtual slots the image's (__vt__3zUI). The first
// virtual of each class is declared and never defined, so no vtable is
// emitted here.
//
// The unit was a tools/gen_accessors.py stub with two functions; they are
// kept at the foot of the file.
//
// Shapes the bytes fixed, each measured with tools/unitcmp.py:
// - Math::Vector4 holds one DataType member, as the DWARF has it; a row copy
//   is then retail's paired-word block move, where four float members copy
//   float by float.
// - GetTransform: the Mul temporaries and the rotation matrix are locals of
//   inlines (MakeScaleZYX, RotateEuler), which gives them retail's stack
//   slots below parentTransform; as function locals they swapped slots.
// - MakeEuler: an inline defined in a dont_inline region, so
//   GetNoScaleTransform calls it as retail does. The region around the
//   caller instead left Matrix43() out of line.
// - CalcAcceleratedDistance: the result is left uninitialised and a final
//   else assigns the distance, which keeps it in f2 with the parameter.
// - DoInitMotion: State's implicit operator= is emitted out of line and no
//   pragma inlines it; the member-wise copy gives retail's words.
// - AutoMenu's one-pass loop keeps retail's test only as
//   `while (autoMenuLimit++ < 32)`; a break, a for loop, an if/else, a goto
//   and a redundant test each let the loop fold away.
//
// Not written, sorted from the unit's listing:
// - UIEventHandler, zUI_Init, Signal, DoUpdate, zUISetCustomSignalHandler,
//   ResetAllowPadActivationThisFrame and SetAllowPadActivationThisFrame
//   name symbols in WAD03.cpp's anonymous namespace (the event wrapper, the
//   signal handler and its user, gAllowPortActivationThisFrame), which a
//   fragment cannot name.
// - DoResetMotion and ApplyMotion load six distinct float literals each:
//   the four-literal wall.

class LinkAsset;
class TemplateEntity;
class xBase;
class zUI;

extern bool uiAnyActive;

namespace Math {

class Vector4 {
public:
    class DataType {
    public:
        float x;
        float y;
        float z;
        float w;
    };

    Vector4& Assign(float x, float y, float z, float w);

    DataType data;
};

class Vector {
public:
    Vector4::DataType data;
};

void Mul(Vector4& result, const Vector4& v, float s);

extern Vector vec4Zero;
extern Vector4 vec4OneX;
extern Vector4 vec4OneY;
extern Vector4 vec4OneZ;

class Matrix43POD {
public:
    float m[3][4];
};

class Matrix33 {
public:
    Matrix33();

    void MakeEulerInternal(float x, float y, float z);

    void MakeScaleZYX(float z, float y, float x) {
        Vector4 tz;
        Vector4 ty;
        Vector4 tx;

        Mul(tx, vec4OneX, x);
        v[0] = tx;
        Mul(ty, vec4OneY, y);
        v[1] = ty;
        Mul(tz, vec4OneZ, z);
        v[2] = tz;
    }

    Vector4 v[3];
};

class Matrix43 : public Matrix33 {
public:
    Matrix43() {}
    Matrix43(const Matrix43POD& c);

    void SetPos(const Vector& pos);
    void RotateEuler(const Vector& euler);
    inline void MakeEuler(const Vector& euler);
};

}  // namespace Math

extern "C" {
void PSMTXConcat(const Math::Matrix43* a, const Math::Matrix43* b,
                 Math::Matrix43* ab);
}

inline void Math::Matrix43::RotateEuler(const Vector& euler) {
    Matrix43 rotation;
    float z = euler.data.z;
    float y = euler.data.y;
    float x = euler.data.x;
    rotation.MakeEulerInternal(x, y, z);
    rotation.SetPos(vec4Zero);
    PSMTXConcat(&rotation, this, this);
}

namespace Graphics {

class Viewport {
public:
    enum VisibilityMask { VisibilityMask_ = 0x7FFFFFFF };
};

}  // namespace Graphics

class RGBA_U8s {
public:
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
};

class xColor {
public:
    union {
        unsigned int rgbaU32;
        RGBA_U8s rgbaU8s;
    };
};

namespace Sext {

class EventAny {};

enum ePauseType { ePauseType_ = 0x7FFFFFFF };

class uid {
public:
    operator unsigned long long() const { return internalUid; }

    unsigned long long internalUid;
};

class xBaseAsset {
public:
    uid id;
    unsigned int baseType;
    unsigned short linkCount;
    unsigned short baseFlags;
};

class LinkAsset {
public:
    unsigned int count;
    void* data;
};

class EventActionNew : public EventAny {};

class EventActionLegacy : public EventActionNew {
public:
    float param0;
    float param1;
    float param2;
    float param3;
    uid paramWidgetAssetID;
};

class zUI : public xBaseAsset {
public:
    Math::Matrix43POD Transform;
    unsigned int anchor;
    uid anchorWidget;
    unsigned int flags;
    unsigned int focusType;
    unsigned int color;
    uid selectedMotion;
    uid unselectedMotion;
    unsigned char brightness;
    unsigned char pad1;
    unsigned char pad2;
    unsigned char pad3;
    uid autoMenuUp;
    uid autoMenuDown;
    uid autoMenuLeft;
    uid autoMenuRight;
    unsigned int customFunction;
    uid customFunctionWidget;
};

class UI_Motion : public xBaseAsset {
public:
    LinkAsset EventLinksNew;
    unsigned char cmdCount;
    unsigned char in;
    unsigned short parameterType;
    uid parameterWidget;
    unsigned int cmdSize;
    float totalTime;
    float loopTime;
    bool lockInput;
};

}  // namespace Sext

class zUIMotionFrame {
public:
    float offsetX;
    float offsetY;
    float scaleX;
    float scaleY;
    float textScaleX;
    float textScaleY;
    float rotation;
    float offsetU;
    float offsetV;
    xColor color;
    unsigned char brightness;
    bool visible;
};

namespace World {

class EntityManager {
public:
    static void* FindAsset(unsigned long long id);
};

EntityManager* GetEntityManager();

class xOGModel;
class xOGModelRefPtr;

class xOGModelRef {
public:
    xOGModel* data;
    xOGModelRefPtr* autoptr;
};

class xOGModelHandle : public xOGModelRef {};

}  // namespace World

class xBase {
public:
    virtual void _v0();

    unsigned char _pad0[0x14];
    unsigned long long id;
    unsigned int baseType;
    unsigned char UNUSED_linkCount;
    unsigned char assertFlags;
    unsigned short baseFlags;
    LinkAsset* linkArray;
    TemplateEntity* templateParent;
    void (*eventFunc)(xBase* from, xBase* to, unsigned int event,
                      Sext::EventAny* any);
};

namespace World {

class xOGEntity : public xBase {
public:
    xOGModelHandle ogModel;
};

}  // namespace World

xBase* zSceneFindEntity(unsigned long long id);

class xTimerAsset {
public:
    unsigned char _pad0[0x10];
    float seconds;
    float randomRange;
};

class xTimer {
public:
    unsigned char _pad0[0x3C];
    xTimerAsset* tasset;
    unsigned char state;
    unsigned char runsInPause;
    unsigned short flags;
    float secondsLeft;
};

class xCounter {
public:
    unsigned char _pad0[0x40];
    short count;
};

class zPlayerAction {
public:
    unsigned char _pad0[0x10];
    float _unk10;
};

// The slot this unit calls is 82 (GetCurrentHitPoints, +336).
class zPlayer {
public:
    virtual void _p0();
    virtual void _p1();
    virtual void _p2();
    virtual void _p3();
    virtual void _p4();
    virtual void _p5();
    virtual void _p6();
    virtual void _p7();
    virtual void _p8();
    virtual void _p9();
    virtual void _p10();
    virtual void _p11();
    virtual void _p12();
    virtual void _p13();
    virtual void _p14();
    virtual void _p15();
    virtual void _p16();
    virtual void _p17();
    virtual void _p18();
    virtual void _p19();
    virtual void _p20();
    virtual void _p21();
    virtual void _p22();
    virtual void _p23();
    virtual void _p24();
    virtual void _p25();
    virtual void _p26();
    virtual void _p27();
    virtual void _p28();
    virtual void _p29();
    virtual void _p30();
    virtual void _p31();
    virtual void _p32();
    virtual void _p33();
    virtual void _p34();
    virtual void _p35();
    virtual void _p36();
    virtual void _p37();
    virtual void _p38();
    virtual void _p39();
    virtual void _p40();
    virtual void _p41();
    virtual void _p42();
    virtual void _p43();
    virtual void _p44();
    virtual void _p45();
    virtual void _p46();
    virtual void _p47();
    virtual void _p48();
    virtual void _p49();
    virtual void _p50();
    virtual void _p51();
    virtual void _p52();
    virtual void _p53();
    virtual void _p54();
    virtual void _p55();
    virtual void _p56();
    virtual void _p57();
    virtual void _p58();
    virtual void _p59();
    virtual void _p60();
    virtual void _p61();
    virtual void _p62();
    virtual void _p63();
    virtual void _p64();
    virtual void _p65();
    virtual void _p66();
    virtual void _p67();
    virtual void _p68();
    virtual void _p69();
    virtual void _p70();
    virtual void _p71();
    virtual void _p72();
    virtual void _p73();
    virtual void _p74();
    virtual void _p75();
    virtual void _p76();
    virtual void _p77();
    virtual void _p78();
    virtual void _p79();
    virtual void _p80();
    virtual void _p81();
    virtual float GetCurrentHitPoints() const;

    unsigned char _pad0[0xC0 - 0x4];
    zPlayerAction** actionList;
};

class zCommonPlayer {
public:
    static const float HP_GAME_MAX_MAXHEALTH;
};

class zPlayerContainer {
public:
    zPlayer* playerArray[4];
    int numPlayers;
};

class xGlobals {
public:
    unsigned char _pad0[0x428];
    zPlayerContainer players;
};

extern xGlobals* xglobals;

// The DWARF's State is 0x40: these members, then the eight bytes the
// vectors' alignment leaves, which a copy does not touch. The three pad
// bytes are one struct here so that a copy block-moves them (lhz, lbz).
class Pad3 {
public:
    unsigned char b[3];
};

class State {
public:
    Math::Vector position;
    Math::Vector scale;
    Math::Vector rotation;
    xColor color;
    unsigned char brightness;
    Pad3 pad;
};

class zUI : public World::xOGEntity {
public:
    virtual void _v1();
    virtual void _v2();
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
    virtual void _v20();
    virtual void _v21();
    virtual void _v22();
    virtual void _v23();
    virtual void _v24();
    virtual void _v25();
    virtual void _v26();
    virtual void _v27();
    virtual void _v28();
    virtual void _v29();
    virtual void _v30();
    virtual void _v31();
    virtual void _v32();
    virtual void _v33();
    virtual void _v34();
    virtual void _v35();
    virtual void _v36();
    virtual void _v37();
    virtual void _v38();
    virtual void DoInit();                                       // slot 39
    virtual void _v40();
    virtual void DoReset();                                      // slot 41
    virtual void DoResetMotion();                                // slot 42
    virtual void DoHandleEvent(xBase* from, unsigned int event,  // slot 43
                               Sext::EventAny* any);
    virtual void _v44();
    virtual void DoUpdate(float dt);                             // slot 45
    virtual void _v46();
    virtual void DoInitMotion();                                 // slot 47
    virtual void DoApplyMotionFrame(const zUIMotionFrame* frame);  // slot 48
    virtual void FocusOn();                                      // slot 49
    virtual void FocusOff();                                     // slot 50
    virtual void Select();                                       // slot 51
    virtual void Unselect();                                     // slot 52
    virtual void Visible();                                      // slot 53
    virtual void Invisible();                                    // slot 54

    void GetTransform(Math::Matrix43& modTransform) const;
    void GetNoScaleTransform(Math::Matrix43& transform) const;
    void Signal(xBase* from, unsigned int event);
    void StopMotion();
    void RunMotion(Sext::UI_Motion* asset, bool loop, bool reset);
    float CalcAcceleratedDistance(const float time, const float distance,
                                  const float startTime, const float endTime,
                                  const float accelTime,
                                  const float decelTime);
    void ApplyMotion();
    float GetParameterizedMotionTime(float motionTime,
                                     unsigned short parameterType,
                                     Sext::uid parameterWidget);
    void AutoMenu(Sext::uid Sext::zUI::* dir, unsigned int event);

    Graphics::Viewport::VisibilityMask UIViewportMask;
    Sext::zUI* asset;
    unsigned char _pad0[0x50 - 0x44];
    State current;
    unsigned char _tail0[8];
    State startMovement;
    unsigned char _tail1[8];
    Sext::UI_Motion* selectedMotion;
    Sext::UI_Motion* unselectedMotion;
    int port;
    zUI* parent;
    bool visible;
    bool focus;
    bool lastFocus;
    bool selected;
    bool brighten;
    bool hdrPass;
    bool locked;
    bool forcehdr;
    bool restoreFocus;
    Sext::UI_Motion* motion;
    float motionTime;
    bool motionFiredEvent;
    bool motionLoop;
};

static inline Sext::UI_Motion* FindMotion(unsigned long long id) {
    return id == 0 ? 0
                   : (Sext::UI_Motion*)World::GetEntityManager()->FindAsset(id);
}

// NEAR MISS: 2 of 92 words; the scale's x and z loads are swapped (retail
// loads z into f31 first and x into f1 last); registers and the rest match.
#pragma push
#pragma always_inline on
void zUI::GetTransform(Math::Matrix43& modTransform) const {
    modTransform.MakeScaleZYX(current.scale.data.z, current.scale.data.y,
                              current.scale.data.x);
    modTransform.RotateEuler(current.rotation);
    modTransform.SetPos(current.position);

    if (parent != 0) {
        Math::Matrix43 parentTransform;
        parent->GetNoScaleTransform(parentTransform);
        PSMTXConcat(&parentTransform, &modTransform, &modTransform);
    }
}
#pragma pop

#pragma push
#pragma dont_inline on
inline void Math::Matrix43::MakeEuler(const Vector& euler) {
    float z = euler.data.z;
    float y = euler.data.y;
    float x = euler.data.x;
    MakeEulerInternal(x, y, z);
    SetPos(vec4Zero);
}
#pragma pop

void zUI::GetNoScaleTransform(Math::Matrix43& transform) const {
    transform.MakeEuler(current.rotation);
    transform.SetPos(current.position);

    if (parent != 0) {
        Math::Matrix43 parentTransform;
        parent->GetNoScaleTransform(parentTransform);
        PSMTXConcat(&parentTransform, &transform, &transform);
    }
}

void zUI::DoInit() {
    selectedMotion = FindMotion(asset->selectedMotion);
    unselectedMotion = FindMotion(asset->unselectedMotion);
    hdrPass = false;
    locked = false;
    port = -1;
    restoreFocus = true;
    UIViewportMask = (Graphics::Viewport::VisibilityMask)2;
}

void zUI::DoReset() {
    DoResetMotion();
    visible = (asset->flags & 0x1) != 0;
    focus = (asset->flags & 0x2) != 0;
    selected = (asset->flags & 0x4) != 0;
    brighten = (asset->flags & 0x200) != 0;
}

Math::Matrix43::Matrix43(const Matrix43POD& c) {
    for (int i = 0; i < 3; i++) {
        v[i].Assign(c.m[i][0], c.m[i][1], c.m[i][2], c.m[i][3]);
    }
}

void zUI::DoHandleEvent(xBase* from, unsigned int event,
                        Sext::EventAny* genericParams) {
    switch (event) {
    case 0x2C9D0683:
        baseFlags |= 0x1;
        break;
    case 0x71E42988:
        baseFlags &= (unsigned short)~0x1;
        break;
    case 0x08FA27BB:
        FocusOn();
        break;
    case 0xF9F2A2BC:
        FocusOn();
        Select();
        Signal(this, 0x08FA27BB);
        Signal(this, 0x7D0F0CB4);
        break;
    case 0x980250DF:
        FocusOff();
        break;
    case 0xF72073ED:
        FocusOff();
        Unselect();
        Signal(this, 0x980250DF);
        Signal(this, 0x80BC88B9);
        break;
    case 0xBE78B459:
        Visible();
        FocusOn();
        Select();
        Signal(this, 0x27858BA2);
        Signal(this, 0x08FA27BB);
        Signal(this, 0x7D0F0CB4);
        break;
    case 0xC1424617:
        FocusOff();
        Unselect();
        Invisible();
        Signal(this, 0x980250DF);
        Signal(this, 0x80BC88B9);
        Signal(this, 0xAE72E9E5);
        break;
    case 0x27858BA2:
        Visible();
        break;
    case 0xAE72E9E5:
        Invisible();
        break;
    case 0x33C046EF:
        if (visible) {
            Invisible();
            Signal(this, 0xAE72E9E5);
        } else {
            Visible();
            Signal(this, 0x27858BA2);
        }
        break;
    case 0xA8B93047:
    case 0x389E01C0:
    case 0x60C0731F:
        DoReset();
        break;
    case 0x7D3E0C05:
        DoResetMotion();
        break;
    case 0x7D0F0CB4:
        Select();
        break;
    case 0x80BC88B9:
        Unselect();
        break;
    case 0x4345E174: {
        Sext::EventActionLegacy* params =
            (Sext::EventActionLegacy*)genericParams;
        Sext::UI_Motion* toParamWidgetPtr = 0;

        if (genericParams != 0) {
            if (params != 0 && params->paramWidgetAssetID != 0) {
                toParamWidgetPtr = (Sext::UI_Motion*)zSceneFindEntity(
                    params->paramWidgetAssetID);

                if (toParamWidgetPtr == 0) {
                    toParamWidgetPtr =
                        (Sext::UI_Motion*)World::GetEntityManager()->FindAsset(
                            params->paramWidgetAssetID);
                }
            }

            RunMotion(toParamWidgetPtr, params->param0 != 0.0f,
                      params->param1 != 0.0f);
        } else {
            StopMotion();
        }
        break;
    }
    case 0xC664E98B:
        brighten = true;
        break;
    case 0x4EF394B8:
        brighten = false;
        break;
    }
}

void zUI::StopMotion() {
    if (motion != 0) {
        motionTime = motion->totalTime;
        ApplyMotion();
    }

    motionTime = 0.0f;
    motion = 0;
}

void zUI::RunMotion(Sext::UI_Motion* asset, bool loop, bool reset) {
    if (reset) {
        DoResetMotion();
    } else if (motion != 0) {
        motionLoop = false;
        motionTime = motion->totalTime;
        ApplyMotion();
    }

    DoInitMotion();

    motionTime = 0.0f;
    motion = asset;
    motionFiredEvent = false;
    motionLoop = loop;

    ApplyMotion();
}

float zUI::CalcAcceleratedDistance(const float time, const float distance,
                                   const float startTime, const float endTime,
                                   const float accelTime,
                                   const float decelTime) {
    float timePassed = motionTime - startTime;
    float timeLength = endTime - startTime;
    float topVelocity =
        distance / (timeLength - 0.5f * (accelTime + decelTime));
    float curDistance;

    if (motionTime < startTime + accelTime) {
        curDistance =
            0.5f * topVelocity * timePassed * timePassed / accelTime;
    } else if (motionTime <= endTime - decelTime) {
        curDistance = topVelocity * (timePassed - 0.5f * accelTime);
    } else if (motionTime <= endTime) {
        float timeToEnd = timeLength - timePassed;
        curDistance =
            distance - 0.5f * topVelocity * timeToEnd * timeToEnd / decelTime;
    } else {
        curDistance = distance;
    }

    return curDistance;
}

float zUI::GetParameterizedMotionTime(float motionTime,
                                     unsigned short parameterType,
                                     Sext::uid parameterWidget) {
    switch (parameterType) {
    case 1: {
        float totalTime;
        xTimer* timer = (xTimer*)zSceneFindEntity(parameterWidget);

        if (timer == 0) {
            return 0.0f;
        }

        totalTime = timer->tasset->seconds;
        return motionTime * ((totalTime - timer->secondsLeft) / totalTime);
    }
    case 2: {
        zPlayer* player = (zPlayer*)zSceneFindEntity(parameterWidget);
        return player != 0 ? motionTime * (player->GetCurrentHitPoints() /
                                           zCommonPlayer::HP_GAME_MAX_MAXHEALTH)
                           : 0.0f;
    }
    case 3: {
        zPlayer* player = xglobals->players.playerArray[0];
        return player != 0 ? motionTime * (player->GetCurrentHitPoints() /
                                           zCommonPlayer::HP_GAME_MAX_MAXHEALTH)
                           : 0.0f;
    }
    case 4: {
        zPlayer* player = xglobals->players.playerArray[1];
        return player != 0 ? motionTime * (player->GetCurrentHitPoints() /
                                           zCommonPlayer::HP_GAME_MAX_MAXHEALTH)
                           : 0.0f;
    }
    case 5:
        return 0.0f;
    case 6: {
        xCounter* counter = (xCounter*)zSceneFindEntity(parameterWidget);
        return counter != 0 ? (float)counter->count : 0.0f;
    }
    case 7: {
        zPlayer* player = xglobals->players.playerArray[0];
        if (player != 0) {
            return player->actionList[15]->_unk10;
        }
        return 0.0f;
    }
    case 8: {
        zPlayer* player = xglobals->players.playerArray[1];
        if (player != 0) {
            return player->actionList[15]->_unk10;
        }
        return 0.0f;
    }
    case 9: {
        zPlayer* player = xglobals->players.playerArray[2];
        if (player != 0) {
            return player->actionList[15]->_unk10;
        }
        return 0.0f;
    }
    case 10: {
        zPlayer* player = xglobals->players.playerArray[3];
        if (player != 0) {
            return player->actionList[15]->_unk10;
        }
        return 0.0f;
    }
    }

    return 0.0f;
}

void zUI::DoInitMotion() {
    startMovement.position = current.position;
    startMovement.scale = current.scale;
    startMovement.rotation = current.rotation;
    startMovement.color = current.color;
    startMovement.brightness = current.brightness;
    startMovement.pad = current.pad;
}

void zUI::DoApplyMotionFrame(const zUIMotionFrame* frame) {
    current.position.data.x = frame->offsetX + startMovement.position.data.x;
    current.position.data.y = frame->offsetY + startMovement.position.data.y;
    current.scale.data.x = frame->scaleX * startMovement.scale.data.x;
    current.scale.data.y = frame->scaleY * startMovement.scale.data.y;
    current.rotation.data.z =
        0.017453292f * frame->rotation + startMovement.rotation.data.z;

    current.color = frame->color;

    current.brightness = frame->brightness;

    if (frame->visible) {
        Visible();
    } else {
        Invisible();
    }
}

// NEAR MISS: 1 of 109 words; the pointer-to-member add has its operands the
// other way round (ours add r4,r4,r0, retail add r4,r0,r4).
void zUI::AutoMenu(Sext::uid Sext::zUI::* dir, unsigned int event) {
    zUI* ui;
    unsigned long long id = asset->*dir;
    unsigned int autoMenuLimit = 0;

    while (autoMenuLimit++ < 32) {
        if (id == 0) {
            return;
        }

        ui = (zUI*)zSceneFindEntity(id);

        if (ui == 0) {
            return;
        }

        if (ui == this) {
            return;
        }

        break;
    }

    if (ui != 0 && ui->locked) {
        switch (event) {
        case 0x6E1BC788:
            ui->AutoMenu(&Sext::zUI::autoMenuUp, 0x6E1BC788);
            break;
        case 0x21EB5E47:
            ui->AutoMenu(&Sext::zUI::autoMenuDown, 0x21EB5E47);
            break;
        }

        Signal(this, 0xCB274F31);
        Signal(this, event);
        DoHandleEvent(this, 0xF72073ED, 0);
        Signal(this, 0xF72073ED);
    } else {
        Signal(this, 0xCB274F31);
        Signal(this, event);
        DoHandleEvent(this, 0xF72073ED, 0);
        Signal(this, 0xF72073ED);
        ui->DoHandleEvent(this, 0xF9F2A2BC, 0);
        ui->Signal(this, 0xF9F2A2BC);
    }
}

void zUI::FocusOn() {
    focus = true;

    if (asset->flags & 0x100) {
        Visible();
        Signal(this, 0x27858BA2);
    }
}

void zUI::FocusOff() {
    focus = false;

    if (asset->flags & 0x100) {
        Invisible();
        Signal(this, 0xAE72E9E5);
    }
}

void zUI::Visible() {
    uiAnyActive = true;
    visible = true;

    if (asset->flags & 0x8) {
        focus = true;
    }
}

void zUI::Invisible() {
    visible = false;

    if (asset->flags & 0x10) {
        focus = false;
    }
}

void zUI::Select() {
    selected = true;

    if (asset->flags & 0x40) {
        if (selectedMotion != 0) {
            RunMotion(selectedMotion, (asset->flags & 0x2000) != 0,
                      (asset->flags & 0x800) != 0);
        }
    } else if (asset->flags & 0x80) {
        StopMotion();
    }
}

void zUI::Unselect() {
    selected = false;

    if (asset->flags & 0x80) {
        if (unselectedMotion != 0) {
            RunMotion(unselectedMotion, (asset->flags & 0x4000) != 0,
                      (asset->flags & 0x1000) != 0);
        }
    } else if (asset->flags & 0x40) {
        StopMotion();
    }
}

// ---------------------------------------------------------------------------
// From the tools/gen_accessors.py stub, unchanged.

extern int pause_type;

int zUIGetPauseType();
void zUISetPauseType(Sext::ePauseType value);

int zUIGetPauseType() { return pause_type; }
void zUISetPauseType(Sext::ePauseType value) { pause_type = (int)value; }
