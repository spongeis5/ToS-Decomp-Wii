// zCombatSystem.cpp -- four functions, read from the image with
// tools/disasm.py. The combat system is a per-frame list of entities:
// Init clears the list count and the globals' bound-update tick,
// FrameStart clears the count alone, UpdateEntity adds an entity to the
// list when its model is playing an animation state and a combat is
// attached to it (the list holds 96 and the test is unsigned), and
// Update ticks the bound-update counter once and then walks the list,
// fetching each entity's combat and posting the frame time to it.
//
// Layouts from the DWARF (tools/dwarf_types.py): xEnt 0xC0 on
// xEffectAttachIntf on xOGEntity 0x40, whose model handle is at +0x34;
// xOGModel on xModelInstance, whose Anim is at +0x4C; xAnimPlay 0x30
// with Single at +0xC; xAnimSingle 0x54 with State at +4; zGlobals
// 0x5A0 with boundUpdateTime at +0x56C, which is the field both Init and
// Update touch. Only the parts on the path are declared, since
// everything else here is reached through a pointer.
//
// The two file statics are `combatList` and `currentCombat` in retail's
// unnamed namespace, so their symbols carry the unity unit's basename
// (@unnamed@WAD01_cpp@) and a fragment's carry its own. The mangling
// follows the main file's name and neither #line nor an #include moves
// it, so the definitions here are the honest source; every reference to
// them is a masked data relocation either way, and the unit matches
// without linking. Their addresses say the shape: the list is 96 words
// at 0x80732C70 and the count follows it at 0x80732DF0. No float or
// string literal is loaded, so there is no pool header.
//
// Three shapes the bytes fixed. The loop counter is UNSIGNED -- the
// re-read of the count each iteration is compared with `cmplw` -- and
// the count is re-read because the calls in the body can change it, so
// it stays a plain global rather than a local. The entity is read into a
// local once and passed to both calls, which is the r27 retail keeps
// across zCombatGetFrom. And UpdateEntity's animation test is an `||`
// chain guarding an early RETURN, not an `&&` chain guarding the body:
// retail's second test is `bne` into the body followed by a `b` to the
// exit, one word more than the `beq` every positive spelling gives. Ten
// were measured -- the and-chain with and without `!= 0`, nested ifs,
// two early returns, a local for the state, a ternary for the state, a
// ternary for the combat, a bool flag, and a goto -- and six of them
// emit the same 21-of-31 word answer; only the or-chain return is 32
// words, and it is exact.

class xEnt;
class zCombat;

class xAnimState;

class xAnimSingle {
public:
    unsigned int SingleFlags;
    xAnimState* State;
};

class xAnimPlay {
public:
    unsigned char _pad0[0xC];
    xAnimSingle* Single;
};

class xOGModel {
public:
    unsigned char _pad0[0x4C];
    xAnimPlay* Anim;
};

class xOGModelRef {
public:
    xOGModel* model;
    unsigned int _pad0;
};

class xOGModelHandle : public xOGModelRef {};

class xEnt {
public:
    unsigned char _pad0[0x34];
    xOGModelHandle ogModel;
};

class zCombat {
public:
    void PostUpdate(xEnt* ent, float dt);
};

zCombat* zCombatGetFrom(xEnt* ent);

class zGlobals {
public:
    unsigned char _pad0[0x56C];
    unsigned int boundUpdateTime;
};

extern zGlobals globals;

namespace {

xEnt* combatList[96];
unsigned int currentCombat;

}  // namespace

void zCombatSystemInit() {
    currentCombat = 0;
    globals.boundUpdateTime = 0;
}

void zCombatSystemFrameStart() {
    currentCombat = 0;
}

void zCombatSystemUpdate(float dt) {
    if (currentCombat != 0) {
        globals.boundUpdateTime++;

        for (unsigned int i = 0; i < currentCombat; i++) {
            xEnt* ent = combatList[i];
            zCombat* combat = zCombatGetFrom(ent);

            if (combat) {
                combat->PostUpdate(ent, dt);
            }
        }
    }
}

void zCombatSystemUpdateEntity(xEnt* ent, float dt) {
    xAnimPlay* anim = ent->ogModel.model->Anim;

    if (anim == 0 || anim->Single->State == 0) {
        return;
    }

    if (zCombatGetFrom(ent) != 0) {
        if (currentCombat < 96) {
            combatList[currentCombat++] = ent;
        }
    }
}
