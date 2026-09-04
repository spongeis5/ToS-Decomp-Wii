// zDestructibles.cpp -- twelve functions, 4,420 bytes, read from the
// image with tools/disasm.py. A destructible is an entity with a list of
// states, each carrying its own sounds, an optional model swap and an
// FX spawn; damage walks it forward one state at a time.
//
// Layouts from the DWARF (tools/dwarf_types.py) and from the strides in
// the bytes: zDestructible 0x78 -- three bools, the asset at +4, the
// state array at +8, the root entity at +0xC, the swap model at +0x10,
// the state count at +0x14, hit points at +0x18, three filters from
// +0x1C, the current state at +0x28, the damage total at +0x2C and an
// idle zSoundAsset from +0x30. A state is 240 bytes, which is the stride
// every loop here uses, and holds three zSoundAssets at +0x18, +0x60 and
// +0xA8 -- 0x48 each, which is exactly 240 from +0x18. The asset's own
// per-state record is 80 bytes with three uids at +0x10, +0x18 and +0x20,
// the three RegisterFX resolves.
//
// MEASURED: 3 of the 6 functions this object defines are
// byte-identical, 324 bytes -- zDestructible_SceneExit,
// StopAllIdleSounds and GetHitFilterFromAsset. Six of the unit's twelve
// are not written: Init (580), Reset (420), UpdateState (760),
// TranslateHitSource (a 52-entry jump table, so it needs the table as
// data too) and zDestructible_EventCB (1,472).
//
// Three one-word shapes the bytes fixed, and each is worth knowing:
//
//   * The idle sound's live test is a WORD load, not a byte -- retail
//     has lwz where a bool member gives lbz, so what is tested is the
//     zSoundAsset's first pointer and not a flag.
//   * HitFilters::gameType is SIGNED. Retail compares it with cmpwi;
//     spelled unsigned it is cmplwi, and that was the single differing
//     word in GetHitFilterFromAsset.
//   * The model-swap test ORs the asset's two words a-then-b, which is
//     the order retail reads them in.
//
// NEAR MISS -- StateSwap, 7 of 23 words, all register numbers: retail
// keeps the state pointer in r7 and the swap in r6 where ours has them
// the other way round, and its tail call to FXInit is reached through a
// branch our version places differently. NEAR MISS -- StopStateSounds,
// 17 of 24 and two instructions long, and zDestructible_RegisterFX, 40
// of 41 and four instructions long. Neither has been swept.

typedef unsigned long long uid;

class xVec3;

namespace World {
class xOGEntity;
}

namespace FX {
class zFXSpawn;
}

namespace Sext {

class xBaseAsset {
public:
    uid id;
    unsigned int baseType;
    unsigned short linkCount;
    unsigned short baseFlags;
};

class HitFilters {
public:
    unsigned int CommonFilters;
    int gameType;
    unsigned int gameFilters;
};

}  // namespace Sext

class zSoundAsset {
public:
    void Stop(bool immediate);

    unsigned char _pad0[0x48];
};

void zSoundAsset_Unregister(zSoundAsset* asset);

class ModelInstanceAsset {
public:
    unsigned int a;
    unsigned int b;
};

class zDestructibleStateAsset {
public:
    unsigned char _pad0[0x10];
    uid fxA;
    uid fxB;
    uid fxC;
    unsigned char _pad1[0x50 - 0x28];
};

class DestructibleAsset {
public:
    unsigned char _pad0[0x34];
    zDestructibleStateAsset* states;
    unsigned char _pad1[0x50 - 0x38];
};

class zDestructibleState {
public:
    unsigned char _pad0[4];
    ModelInstanceAsset* modelSwap;
    void* fxA;
    void* fxB;
    FX::zFXSpawn* fxC;
    unsigned char _pad1[0x18 - 0x14];
    zSoundAsset soundA;
    zSoundAsset soundB;
    zSoundAsset soundC;
};

class zDestructible {
public:
    static void StopAllIdleSounds(World::xOGEntity* ent, zDestructible* d);
    static void StopStateSounds(World::xOGEntity* ent, zDestructible* d);
    static void StateSwap(World::xOGEntity* ent, zDestructible* d, xVec3& pos,
                          int index);
    static void FXInit(World::xOGEntity* ent, FX::zFXSpawn* fx);

    bool active;
    bool swapModel;
    bool didSwapModel;
    unsigned char _pad0[1];
    DestructibleAsset* asset;
    zDestructibleState* states;
    World::xOGEntity* root_ent;
    ModelInstanceAsset* modelInstanceSwap;
    unsigned int totalStateNum;
    unsigned int totalHitPoints;
    unsigned int hitTypeFilter;
    unsigned int hitTypeExcludeFilter;
    unsigned int playerFilter;
    int currentStateNum;
    unsigned int currentDamageTotal;
    zSoundAsset soundIdle;
};

class HitFilterNS {
public:
    static unsigned int GetHitFilterFromAsset(Sext::HitFilters* filters,
                                              const Sext::xBaseAsset* asset);
};

void* zSceneFindObject(uid id);

unsigned int HitFilterNS::GetHitFilterFromAsset(
    Sext::HitFilters* filters, const Sext::xBaseAsset* asset) {
    unsigned int mask = filters->CommonFilters;

    if (filters->gameType == 2) {
        mask |= filters->gameFilters;
    }

    return mask;
}

void zDestructible::StopAllIdleSounds(World::xOGEntity* ent,
                                      zDestructible* d) {
    unsigned int i;

    d->soundIdle.Stop(false);

    for (i = 0; i < d->totalStateNum; i++) {
        d->states[i].soundA.Stop(false);
    }
}

void zDestructible::StopStateSounds(World::xOGEntity* ent, zDestructible* d) {
    if (d->currentStateNum >= 0) {
        d->states[d->currentStateNum].soundA.Stop(false);
        d->states[d->currentStateNum].soundB.Stop(false);
    }
}

void zDestructible::StateSwap(World::xOGEntity* ent, zDestructible* d,
                              xVec3& pos, int index) {
    int next = d->currentStateNum + 1;

    if (next >= (int)d->totalStateNum) {
        return;
    }

    zDestructibleState* state = &d->states[next];
    ModelInstanceAsset* swap = state->modelSwap;

    if (swap != 0 && (swap->a | swap->b) != 0) {
        d->modelInstanceSwap = swap;
        d->swapModel = true;
    }

    if (state->fxC != 0) {
        FXInit(ent, state->fxC);
    }
}

void zDestructible_SceneExit(zDestructible*& d, World::xOGEntity* ent) {
    unsigned int i;

    if (d == 0) {
        return;
    }

    if (*(void**)&d->soundIdle != 0) {
        zSoundAsset_Unregister(&d->soundIdle);
    }

    for (i = 0; i < d->totalStateNum; i++) {
        if (*(void**)&d->states[i].soundA != 0) {
            zSoundAsset_Unregister(&d->states[i].soundA);
        }

        if (*(void**)&d->states[i].soundB != 0) {
            zSoundAsset_Unregister(&d->states[i].soundB);
        }

        if (*(void**)&d->states[i].soundC != 0) {
            zSoundAsset_Unregister(&d->states[i].soundC);
        }
    }
}

void zDestructible_RegisterFX(zDestructible* d) {
    unsigned int i;
    DestructibleAsset* asset = d->asset;

    for (i = 0; i < d->totalStateNum; i++) {
        d->states[i].fxA = zSceneFindObject(asset->states[i].fxA);
        d->states[i].fxB = zSceneFindObject(asset->states[i].fxB);
        d->states[i].fxC = (FX::zFXSpawn*)zSceneFindObject(asset->states[i].fxC);
    }
}
