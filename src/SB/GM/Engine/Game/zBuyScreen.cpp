// zBuyScreen.cpp -- eight functions, read from the image with
// tools/disasm.py. The buy screen is a Scaleform movie the game shows
// when the player can spend happiness points. The asset's Create takes
// one 72-byte block from the global heap (tag 16), clears it and places
// the entity on it. The constructor runs the xOGEntity one, keeps the
// asset, clears the player pointer and the three flags, installs the
// event wrapper, runs xBaseInit and points the link array at the asset's
// links. Setup finds the movie player by hashing a name into a UID and
// asking the scene for it, but only once, and clears the flags either
// way. HandleEvent answers eight ids: two hide the screen, one sets it
// up, two forward an event of their own id to the movie player, one asks
// for the screen when there is a player, and two -- with the screen
// active -- send an event to the entity itself and clean up. Hide clears
// the pending flag always, and when the screen is shown clears that too
// and calls the movie's Hide. Update waits for the movie to stop, then
// clears the pending flag, plays it, marks it active, and calls Show
// with the asset's text and its price as ActionScript arguments, marks
// it shown and turns the player's controls off if the asset says to.
// Cleanup clears both flags and turns them back on.
//
// Layouts from the DWARF (tools/dwarf_types.py): zBuyScreen 0x48 on
// World::xOGEntity 0x40, with the asset at +0x3C in the base's tail
// padding, the movie player at +0x40 and the three bools at +0x44,
// +0x45 and +0x46; Sext::BuyScreen 0x20 on xBaseScene, with the text
// pointer at +0x10 (the DWARF types it Pointer32), the price as a
// SIGNED short at +0x14, the turn-off-control byte at +0x16 and the
// link array at +0x18; GFxValue 0x10 with its type at +0 and its value
// union at +8, VT_String being 4; zScaleform 0x140 with World::
// ScaleformOGAsset as its SECOND base at +0xF8. The three strings
// ("BuyScreenPlayer_reference", "Hide", "Show") are the translation
// unit's pool, hence the generated header first.
//
// MEASURED: 8 of 8 functions byte-identical by tools/unitcmp.py. There
// is no near miss.
//
// Six shapes the bytes fixed. The movie player is converted to its
// second base by a POINTER conversion, which is what emits the
// null-tested `addi r5,r5,248` retail has before each InvokeASFunc; the
// same pointer passed as an xBase reaches its first base at +0 and gets
// no test. Hide clears needToShow BEFORE the early return on shown, so
// that store is above the `beqlr` and not folded into the body. The
// eight event ids are one switch, which mwcc compiles to a binary
// SEARCH over the case values in SIGNED order even though the parameter
// is unsigned -- and the two ids that both call Hide are two separate
// cases with the same body, not a case pair, since the search branches
// to two distinct labels. The constructor stores the asset before the
// vtable and the link array after xBaseInit, which is the order of the
// stores in the bytes and not the order the fields sit in.
//
// The last two are Update's two ActionScript arguments, and each cost a
// round. The price is a SIGNED short, so its conversion is the signed
// magic-constant pair (lis 0x4330 / xoris 0x8000) rather than the
// unsigned one. And the text argument is set through SetString rather
// than by assigning the two fields: retail keeps the array
// constructor's `Type = VT_Undefined` store into args[0] and then
// stores 4 over it, where two plain assignments with the type written
// first let mwcc drop the first store as dead -- 58 words against
// retail's 59, every other word identical. Nine spellings were swept.
// Writing the VALUE first and the type second keeps it (mwcc will not
// remove a store to an object something else has since been written
// into), and so does any form that puts the pair behind a call
// boundary: a pointer local, a reference local, or the inline
// SetString, whose own two stores then match in EITHER order. The DWARF
// line table decides between them -- both stores lie on source line 141
// with the declaration alone on 140, so the original wrote one
// statement, which is the SetString here. Two that do not: assigning
// args[1] before args[0] moves the call and costs 41 of 59 words, and a
// constructor initialiser list changes nothing at all.

#include "SB/GM/Engine/Game/zBuyScreen.pool.h"

typedef unsigned long long uid;

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };
enum ForceEvent { ForceEvent_ = 0x7FFFFFFF };
enum zControlOwner { zControlOwner_ = 0x7FFFFFFF };

namespace Memory {
enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };

void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap, eMemMgrTag tag,
                      bool clear);
}  // namespace Memory

extern "C" void* memset(void* dst, int c, unsigned long n);

inline void* operator new(unsigned long, void* p) { return p; }

class LinkAsset;
class TemplateEntity;
class xBase;
class zScaleform;

namespace World {
class EntityHandleBase;
}

namespace Sext {

class EventAny {
public:
    float value;
};

class xBaseAsset {
public:
    uid id;
    unsigned int baseType;
    unsigned short linkCount;
    unsigned short baseFlags;
};

class LinkAssetArray {
public:
    unsigned int count;
    unsigned int data;
};

class BuyScreen : public xBaseAsset {
public:
    char* text;
    short price;
    bool turnOffControl;
    LinkAssetArray EventLinksNew;

    static zScaleform* Create(World::EntityHandleBase* handle,
                              BuyScreen* asset);
};

}  // namespace Sext

void xBaseInit(xBase* base, const Sext::xBaseAsset* asset);

unsigned int xStrHash(const char* str);
uid xUIDMgrFindUID(unsigned int hash);
xBase* zSceneFindObject(uid id);

void zEntEvent(xBase* from, unsigned int fromEvent, xBase* to,
               unsigned int event, Sext::EventAny* any, ForceEvent force);

class zPlayer {
public:
    static void TurnAllControlsOn(zControlOwner owner);
    static void TurnAllControlsOff(zControlOwner owner);
};

class xBase {
public:
    virtual void _v0();

    unsigned char _pad0[0x14];
    uid id;
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

class xOGModelHandle {
public:
    void* model;
    unsigned int _pad0;
};

class xOGEntity : public xBase {
public:
    xOGEntity(EntityHandleBase* handle);

    xOGModelHandle ogModel;
};

// The second base of zScaleform, at +0xF8: four bytes and a vtable
// pointer, which is why a conversion to it is an offset and not a cast.
class ScaleformOGAsset {
public:
    virtual void _v0();
};

}  // namespace World

// zUI and everything under it, as the 0xF8 bytes the second base sits
// past. It reaches xBase at offset zero, so an event sent to the movie
// player needs no adjustment.
class zUIStub : public xBase {
public:
    unsigned char _pad0[0xF8 - 0x34];
};

class zScaleform : public zUIStub, public World::ScaleformOGAsset {
public:
    bool IsStopped();
    void Play();
};

class GFxValue {
public:
    enum ValueType {
        VT_Undefined = 0,
        VT_Null = 1,
        VT_Boolean = 2,
        VT_Number = 3,
        VT_String = 4
    };

    union ValueUnion {
        double NValue;
        bool BValue;
        char* pString;
        unsigned short* pStringW;
    };

    GFxValue() { Type = VT_Undefined; }

    void SetString(char* str) {
        Type = VT_String;
        Value.pString = str;
    }

    void SetNumber(double value);

    ValueType Type;
    ValueUnion Value;
};

namespace Scaleform {

void SFWrap_InvokeASFunc(World::ScaleformOGAsset* asset, const char* name,
                         unsigned int count, float* args);
void SFWrap_InvokeASFunc(World::ScaleformOGAsset* asset, const char* name,
                         unsigned int count, const GFxValue* args);

}  // namespace Scaleform

class zBuyScreen : public World::xOGEntity {
public:
    zBuyScreen(World::EntityHandleBase* handle, Sext::BuyScreen* asset);

    void HandleEvent(xBase* from, unsigned int event, Sext::EventAny* any);
    void Setup();
    void Hide();
    void Update(float dt);
    void Cleanup();

    Sext::BuyScreen* asset;
    zScaleform* scaleformPlayer;
    bool shown;
    bool isActive;
    bool needToShow;
};

// scope:local in the image's symbol table, so a file static here.
static void zBuyScreenEventWrapper(xBase* from, xBase* to, unsigned int event,
                                   Sext::EventAny* any);

zScaleform* Sext::BuyScreen::Create(World::EntityHandleBase* handle,
                                    BuyScreen* asset) {
    return (zScaleform*)new (memset(
        Memory::AllocGlobalHeap(sizeof(zBuyScreen), (Memory::GlobalHeapEnum)0,
                                (eMemMgrTag)16, false),
        0, sizeof(zBuyScreen))) zBuyScreen(handle, asset);
}

static void zBuyScreenEventWrapper(xBase* from, xBase* to, unsigned int event,
                                   Sext::EventAny* any) {
    ((zBuyScreen*)to)->HandleEvent(from, event, any);
}

zBuyScreen::zBuyScreen(World::EntityHandleBase* handle, Sext::BuyScreen* a)
    : World::xOGEntity(handle) {
    asset = a;
    scaleformPlayer = 0;
    shown = false;
    isActive = false;
    needToShow = false;
    eventFunc = zBuyScreenEventWrapper;

    xBaseInit(this, a);

    linkArray = (LinkAsset*)&a->EventLinksNew;
}

void zBuyScreen::HandleEvent(xBase* from, unsigned int event,
                             Sext::EventAny* any) {
    switch (event) {
    case 0xD576CA21:
        Setup();
        break;
    case 0x2DE3146B:
        Hide();
        break;
    case 0xAE72E9E5:
        zEntEvent(0, 0, scaleformPlayer, 0xAE72E9E5, 0, (ForceEvent)1);
        break;
    case 0x27858BA2:
        zEntEvent(0, 0, scaleformPlayer, 0x27858BA2, 0, (ForceEvent)1);
        break;
    case 0x73A90B4D:
        if (scaleformPlayer) {
            needToShow = true;
        }
        break;
    case 0xFB267034:
        Hide();
        break;
    case 0x42B85591:
        if (isActive) {
            zEntEvent(0, 0, this, 0xD6BBF310, 0, (ForceEvent)1);
            Cleanup();
        }
        break;
    case 0x7AF953B7:
        if (isActive) {
            zEntEvent(0, 0, this, 0x0EFCF136, 0, (ForceEvent)1);
            Cleanup();
        }
        break;
    }
}

void zBuyScreen::Setup() {
    if (scaleformPlayer == 0) {
        scaleformPlayer = (zScaleform*)zSceneFindObject(
            xUIDMgrFindUID(xStrHash("BuyScreenPlayer_reference")));
        shown = false;
    }

    isActive = false;
    needToShow = false;
}

void zBuyScreen::Hide() {
    needToShow = false;

    if (!shown) {
        return;
    }

    shown = false;

    Scaleform::SFWrap_InvokeASFunc((World::ScaleformOGAsset*)scaleformPlayer,
                                   "Hide", 0, (float*)0);
}

void zBuyScreen::Update(float dt) {
    if (needToShow && scaleformPlayer->IsStopped()) {
        needToShow = false;

        scaleformPlayer->Play();

        isActive = true;

        GFxValue args[2];

        args[0].SetString(asset->text);

        args[1].SetNumber(asset->price);

        Scaleform::SFWrap_InvokeASFunc(
            (World::ScaleformOGAsset*)scaleformPlayer, "Show", 2, args);

        shown = true;

        if (asset->turnOffControl) {
            zPlayer::TurnAllControlsOff((zControlOwner)8192);
        }
    }
}

void zBuyScreen::Cleanup() {
    isActive = false;
    shown = false;

    if (asset->turnOffControl) {
        zPlayer::TurnAllControlsOn((zControlOwner)8192);
    }
}
