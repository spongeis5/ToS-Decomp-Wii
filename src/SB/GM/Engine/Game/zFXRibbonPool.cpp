#include "SB/GM/Engine/Game/zFXRibbonPool.pool.h"

// zFXRibbonPool.cpp -- four functions, 504 bytes, read from the image with
// tools/disasm.py. A fixed pool of 128 ribbons with a byte of "in use"
// beside it: Init creates every ribbon and gives it a default config block
// off the heap, New hands out the first ribbon that is both empty and
// free, falling back to the free one with the least time left, Delete
// clears the caller's pointer and the flag its index names, and SceneReset
// memsets the flags.
//
//   zFXRibbonPoolInit         128 iterations of create(), a 56-byte
//                             xMemAlloc (tag 14, heap 0),
//                             load_default_config and set_config
//   zFXRibbonPoolNew          two 128-iteration scans and a fallback
//                             return of 0
//   zFXRibbonPoolDelete       a pointer difference into the flag array
//   zFXRibbonPool_SceneReset  a tail call to memset, 128 bytes
//
// Layouts from the DWARF (tools/dwarf_types.py): FX::Ribbon::xFXRibbon is
// 8 bytes, an activity pointer and a config pointer, and
// xFXRibbon::config_type is 0x38 -- which is the 56 the allocation asks
// for -- with the four-byte hole at +0x14 that its 8-aligned `entity`
// leaves. The symbol table gives the two pool objects:
// `ribbonInUse__19@unnamed@WAD02_cpp@` is 0x80 bytes of `.bss` at
// 0x807341B0 and `ribbonPool__19@unnamed@WAD02_cpp@` is 0x400 at
// 0x80734230 -- 128 ribbons of 8 bytes, which is the loop stride and the
// shift in Delete.
//
// THE ANONYMOUS NAMESPACE CANNOT BE NAMED FROM HERE. CodeWarrior mangles
// an anonymous namespace with the BASENAME of the file being compiled, so
// retail's `@unnamed@WAD02_cpp@` would come out of this fragment as
// `@unnamed@zFXRibbonPool_cpp@`. Both references are data relocations and
// unitcmp masks them, so the instruction words still compare; the names do
// not, and this unit could not link as it stands anyway -- its split owns
// no data section, and defining the pool gives the object 0x480 bytes of
// `.bss`. Defining them here is the honest source for a fragment; naming
// the file after the blob is what would reproduce the symbols, and that is
// a splits question, not a source one.
//
// The float base: this unit loads one literal, FLT_MAX (0x7F7FFFFF at
// 0x8068BA50), and gen_poolprefix.py measured its translation unit's
// .rodata as 50,912 bytes ahead of it. The generated header carries that
// distance so the literal is addressed the way retail addresses it.
//
// Four shapes the bytes fixed.
//
// The index is UNSIGNED and its "none" value is compared as a 32-bit
// constant: retail spells `addis r0,r28,1 ; cmplwi r0,0xFFFF`, which is
// how mwcc compares an unsigned word against 0xFFFFFFFF, where a signed
// int would have given a plain `cmpwi r28,-1`.
//
// The two "no ribbon" tests are separate statements. Retail tests the
// index once to decide whether to run the second scan and again to decide
// what to return, and the second is written the POSITIVE way round --
// `if (index != -1) { ...; return &ribbonPool[index]; } return 0;` -- so
// the `li r3,0` sits after the success block, branched to, rather than in
// front of it.
//
// Both loops count 128 with a constant bound, so they are bottom-tested
// with no jump to the test, and both compares are `cmplwi`, so the index
// is unsigned there too.
//
// load_default_config is a STATIC member: retail calls it with the
// allocation still in r3 and never sets up a second argument, so the
// block is the reference parameter and not a `this`. CodeWarrior mangles
// a static member exactly like a non-static one, so the symbol does not
// say which it is; the registers do.
//
// Two register orders, both measured rather than guessed, and each was
// every remaining word of its function.
//
// Init's three locals go in the order config, ribbon, i -- the LAST
// declared takes the lowest callee-saved register. All six orders were
// compiled: cri is exact, irc and rci are 5 of 30 words, cir is 6, icr
// and ric are 8. Ribbon has to be a named local at all (the address is
// computed once and used by create() and set_config), which is 8 words
// on its own.
//
// New declares its index in EACH loop, not once for the function.
// Retail's index-of-the-chosen-ribbon takes r28, right above the counter
// in r27, and a counter shared by both loops pushes it to r31 and shifts
// the pool base and the two strength-reduced pointers down with it -- 28
// of 77 words. Ten other spellings all tie at 28: a `while` for the first
// loop, a `continue` form, nested tests instead of `&&`, the second loop
// on its own named counter, the found ribbon in a local at the end, the
// sentinel written 0xFFFFFFFF, and the float declared at the top or
// before the counter. Two `for (unsigned int i = ...)` scopes is exact.
// The sentinel's type is settled by the same sweep: a signed int gives
// `cmpwi r28,-1` and a two-word shorter function (54 of 75), where retail
// spells the unsigned 32-bit compare.

extern "C" void* memset(void* dst, int c, unsigned long n);

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {
enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };
}  // namespace Memory

void* xMemAlloc(Memory::GlobalHeapEnum heap, unsigned int size, int align,
                eMemMgrTag tag);

namespace FX {
namespace Ribbon {

class activity_data;
class curve_node;

class xVec2 {
public:
    float x;
    float y;
};

class xFXRibbon {
public:
    class config_type {
    public:
        /* +0x0  */ float width;
        /* +0x4  */ float life_time;
        /* +0x8  */ int flags;
        /* +0xC  */ curve_node* curve;
        /* +0x10 */ int curve_size;
        /* +0x18 */ unsigned long long entity;
        /* +0x20 */ xVec2 mUVanim;
        /* +0x28 */ unsigned char mTexAnim[0xC];
    };

    void create();
    void clear();
    int size() const;
    float time_remaining();
    void set_config(const config_type* config);
    static void load_default_config(config_type& config);

    /* +0x0 */ activity_data* act;
    /* +0x4 */ config_type* cfg;
};

}  // namespace Ribbon
}  // namespace FX

namespace {

bool ribbonInUse[128];
FX::Ribbon::xFXRibbon ribbonPool[128];

}  // namespace

void zFXRibbonPoolInit() {
    FX::Ribbon::xFXRibbon::config_type* config;
    FX::Ribbon::xFXRibbon* ribbon;
    unsigned int i;

    for (i = 0; i < 128; i++) {
        ribbon = &ribbonPool[i];
        ribbon->create();

        config = (FX::Ribbon::xFXRibbon::config_type*)xMemAlloc(
            (Memory::GlobalHeapEnum)0,
            sizeof(FX::Ribbon::xFXRibbon::config_type), 0, (eMemMgrTag)14);

        FX::Ribbon::xFXRibbon::load_default_config(*config);
        ribbon->set_config(config);
    }
}

FX::Ribbon::xFXRibbon* zFXRibbonPoolNew() {
    unsigned int index = -1;

    for (unsigned int i = 0; i < 128; i++) {
        if (ribbonPool[i].size() == 0 && !ribbonInUse[i]) {
            index = i;
            break;
        }
    }

    if (index == -1) {
        float best = 3.4028235e38f;

        for (unsigned int i = 0; i < 128; i++) {
            if (!ribbonInUse[i]) {
                float remaining = ribbonPool[i].time_remaining();

                if (remaining < best) {
                    best = remaining;
                    index = i;
                }
            }
        }
    }

    if (index != -1) {
        ribbonPool[index].clear();
        ribbonInUse[index] = true;

        return &ribbonPool[index];
    }

    return 0;
}

void zFXRibbonPoolDelete(FX::Ribbon::xFXRibbon*& ribbon) {
    if (ribbon == 0) {
        return;
    }

    ribbonInUse[ribbon - ribbonPool] = false;
    ribbon = 0;
}

void zFXRibbonPool_SceneReset() {
    memset(ribbonInUse, 0, sizeof(ribbonInUse));
}
