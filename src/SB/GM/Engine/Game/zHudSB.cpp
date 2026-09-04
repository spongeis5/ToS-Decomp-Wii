// zHudSB.cpp -- twenty-eight functions, 5,976 bytes, read from the image
// with tools/disasm.py. The SpongeBob HUD is a Scaleform movie and this
// entity is the thin layer that pushes values into it: nearly every
// function stores a member and calls SFWrap_InvokeASFunc with one
// ActionScript name and an array of floats.
//
// Layouts from the DWARF (tools/dwarf_types.py): zHudSB 0x78 on
// World::xOGEntity 0x40, with the asset at +0x3C in the base's tail
// padding, the movie player at +0x40, health +0x44, maxHealth +0x48,
// stunEnergy +0x4C, shakeEnergy +0x50, powerup +0x54, the two powerup
// times +0x58 and +0x5C, the timer +0x60, its two times +0x64 and +0x68,
// bossMeter +0x6C, hudShown +0x70, hudGlobalVisibility +0x71 and the
// sound cue +0x74. Sext::HudSB 0x20 on xBaseScene with the movie's uid
// at +0x10 and the link array at +0x18.
//
// The ActionScript names are the translation unit's pool, hence the
// generated header first.
//
// MEASURED: 15 of the 19 functions this object defines are
// byte-identical. Nine of the twenty-eight the unit holds are not
// written yet -- HandleEvent (1,808 bytes), Setup, Update, SetPowerup,
// SetPowerupTimer, HappinessPointsChanged, ShowTimer, UpdateTimer and
// PauseGame.
//
// THE POOL HEADER HAS TO BE --whole HERE. Nine functions were each
// exactly ONE word wrong, and the word was the string offset every
// time: gen_poolprefix's default builds the prefix up to the first
// string this unit introduces, but the strings between that and
// SetCounter's live in functions not written yet, so every later offset
// came out short. `python tools/gen_poolprefix.py --whole` emits all 360
// and the nine matched at once. A unit written a function at a time
// wants --whole; the tool says so in its own help and it is easy to
// read past.
//
// NEAR MISS -- SetHealth, 19 of 20 words and one instruction short, and
// SetPlanktonShakeEnergy and SetPlanktonStunEnergy, 20 of 23 each and
// also one short. All three are the same missing instruction: retail
// narrows the incoming float parameter with an explicit `frsp` before
// using it -- storing the rounded value to the member and the raw f1 to
// the argument slot, and in the Plankton pair comparing the rounded
// one. Ours uses f1 for both and never narrows. Swapping the order of
// the two stores changes nothing but the length, and routing the value
// through a named float local does not survive either -- mwcc folds it.
// So something else makes the compiler treat the parameter as needing a
// narrowing, and that is what to find.
//
// NEAR MISS -- ShowHud, 25 of 37 words. Retail tests the scene's game
// mode against a constant, then the player, then a byte at +0x120 in
// the player, then hudShown, and the four tests share exits; ours reads
// the same four but does not share them the same way. It has not been
// swept.

#include "SB/GM/Engine/Game/zHudSB.pool.h"

typedef unsigned long long uid;

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };
enum ForceEvent { ForceEvent_ = 0x7FFFFFFF };

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
class xTimer;
class zScaleform;
class zSoundCue;
class zHudSB;

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

class HudSB : public xBaseAsset {
public:
    uid scaleformHud;
    LinkAssetArray EventLinksNew;

    static ::zHudSB* Create(World::EntityHandleBase* handle, HudSB* asset);
};

}  // namespace Sext

void xBaseInit(xBase* base, const Sext::xBaseAsset* asset);

void zEntEvent(xBase* from, unsigned int fromEvent, xBase* to,
               unsigned int event, Sext::EventAny* any, ForceEvent force);

xBase* zSceneFindObject(const char* name);

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
// pointer, which is why a conversion to it is a null-tested offset and
// not a cast.
class ScaleformOGAsset {
public:
    virtual void _v0();
};

}  // namespace World

// zUI and everything under it, as the 0xF8 bytes the second base sits
// past. The byte at +0x120 is the one ShowHud tests.
class zUIStub : public xBase {
public:
    unsigned char _pad0[0xF8 - 0x34];
};

class zScaleform : public zUIStub, public World::ScaleformOGAsset {
public:
    void Clear();
    void Unclear();

    unsigned char _pad1[0x120 - 0xFC];
    bool loaded;
};

namespace Scaleform {

void SFWrap_InvokeASFunc(World::ScaleformOGAsset* player, const char* name,
                         unsigned int argc, float* args);

}  // namespace Scaleform

class zSoundCue {
public:
    bool IsActive();
};

class xScene {
public:
    unsigned char _pad0[0x1C];
    unsigned int gameMode;
};

class zGlobals {
public:
    unsigned char _pad0[0x43C];
    xScene* sceneCur;
};

extern zGlobals globals;

class zHudSB : public World::xOGEntity {
public:
    zHudSB(World::EntityHandleBase* handle, Sext::HudSB* asset);

    void HandleEvent(xBase* from, unsigned int event, Sext::EventAny* any);
    void Reset();
    void ShowHud();
    void HideHud();
    void SetAllHudVisible();
    void SetAllHudInvisible();
    void SetHealth(float health);
    void SetMaxHealth(int maxHealth);
    void SetPuckAmmo(int ammo);
    void SetPlanktonShakeEnergy(float energy);
    void SetPlanktonStunEnergy(float energy);
    void SetPlanktonAmmo(int ammo);
    void ShowCounter(int a, int b);
    void SetCounter(int count);
    void SetBossMeter(float value);
    void ShowBossMeter(int show);
    void LivesChanged();

    Sext::HudSB* asset;
    zScaleform* hudPlayer;
    float health;
    int maxHealth;
    float stunEnergy;
    float shakeEnergy;
    int powerup;
    float powerupMinutes;
    float powerupSeconds;
    xTimer* timer;
    float timerMinutes;
    float timerSeconds;
    float bossMeter;
    bool hudShown;
    bool hudGlobalVisibility;
    unsigned char _pad0[2];
    zSoundCue* scTimer;
};

static void zHudSBEventWrapper(xBase* from, xBase* to, unsigned int event,
                               Sext::EventAny* any) {
    ((zHudSB*)to)->HandleEvent(from, event, any);
}

zHudSB::zHudSB(World::EntityHandleBase* handle, Sext::HudSB* a)
    : World::xOGEntity(handle) {
    asset = a;
    hudPlayer = 0;
    health = 1.0f;
    maxHealth = 0;
    timer = 0;
    bossMeter = 0.0f;
    hudShown = false;
    hudGlobalVisibility = true;
    eventFunc = zHudSBEventWrapper;

    xBaseInit(this, a);
}

zHudSB* Sext::HudSB::Create(World::EntityHandleBase* handle,
                            Sext::HudSB* asset) {
    return new (memset(Memory::AllocGlobalHeap(sizeof(zHudSB),
                                               (Memory::GlobalHeapEnum)0,
                                               (eMemMgrTag)16, false),
                       0, sizeof(zHudSB))) zHudSB(handle, asset);
}

void zHudSB::Reset() {
    if (scTimer != 0 && scTimer->IsActive()) {
        zEntEvent(0, 0, (xBase*)scTimer, 0x5D2F4217, 0, (ForceEvent)1);
    }
}

void zHudSB::ShowHud() {
    if (globals.sceneCur->gameMode == 0x4D045553) {
        return;
    }

    if (hudPlayer == 0) {
        return;
    }

    if (!hudPlayer->loaded) {
        return;
    }

    if (hudShown) {
        return;
    }

    Scaleform::SFWrap_InvokeASFunc(hudPlayer, "ShowHud", 0, 0);

    hudShown = true;
}

void zHudSB::HideHud() {
    if (hudShown) {
        Scaleform::SFWrap_InvokeASFunc(hudPlayer, "HideHud", 0, 0);

        hudShown = false;
    }
}

void zHudSB::SetAllHudVisible() {
    if (hudPlayer != 0) {
        Scaleform::SFWrap_InvokeASFunc(hudPlayer, "HudVisible", 0, 0);

        hudPlayer->Unclear();

        hudGlobalVisibility = true;
    }
}

void zHudSB::SetAllHudInvisible() {
    if (hudPlayer != 0) {
        Scaleform::SFWrap_InvokeASFunc(hudPlayer, "HudInvisible", 0, 0);

        hudPlayer->Clear();

        hudGlobalVisibility = false;
    }
}

void zHudSB::SetHealth(float h) {
    float args[1];

    args[0] = h;
    health = h;

    Scaleform::SFWrap_InvokeASFunc(hudPlayer, "SetHealth", 1, args);
}

void zHudSB::SetMaxHealth(int max) {
    float args[1];

    maxHealth = max;
    args[0] = (float)max;

    Scaleform::SFWrap_InvokeASFunc(hudPlayer, "SetMaxHealth", 1, args);
}

void zHudSB::SetPuckAmmo(int ammo) {
    float args[1];

    args[0] = (float)ammo;

    Scaleform::SFWrap_InvokeASFunc(hudPlayer, "SetPuckAmmo", 1, args);
}

void zHudSB::SetPlanktonShakeEnergy(float energy) {
    float args[1];

    args[0] = energy;

    if (energy != shakeEnergy) {
        shakeEnergy = energy;

        Scaleform::SFWrap_InvokeASFunc(hudPlayer, "H", 1, args);
    }
}

void zHudSB::SetPlanktonStunEnergy(float energy) {
    float args[1];

    args[0] = energy;

    if (energy != stunEnergy) {
        stunEnergy = energy;

        Scaleform::SFWrap_InvokeASFunc(hudPlayer, "T", 1, args);
    }
}

void zHudSB::SetPlanktonAmmo(int ammo) {
    float args[1];

    args[0] = (float)ammo;

    Scaleform::SFWrap_InvokeASFunc(hudPlayer, "SetPlanktonAmmo", 1, args);
}

void zHudSB::ShowCounter(int a, int b) {
    float args[2];

    args[0] = (float)a;
    args[1] = (float)b;

    Scaleform::SFWrap_InvokeASFunc(hudPlayer, "ShowCounter", 2, args);
}

void zHudSB::SetCounter(int count) {
    float args[1];

    args[0] = (float)count;

    Scaleform::SFWrap_InvokeASFunc(hudPlayer, "SetCounter", 1, args);
}

void zHudSB::SetBossMeter(float value) {
    if (bossMeter != value) {
        bossMeter = value;

        Scaleform::SFWrap_InvokeASFunc(hudPlayer, "SetBossMeter", 1,
                                       &bossMeter);
    }
}

void zHudSB::ShowBossMeter(int show) {
    float args[1];

    args[0] = (float)show;

    Scaleform::SFWrap_InvokeASFunc(hudPlayer, "ShowBossMeter", 1, args);
}

void zHudSB::LivesChanged() {
    float args[1];

    args[0] = (float)((short*)zSceneFindObject("COLLECTIBLE_LIFE_reference"))[32] - 1.0f;

    Scaleform::SFWrap_InvokeASFunc(hudPlayer, "SetLives", 1, args);
}
