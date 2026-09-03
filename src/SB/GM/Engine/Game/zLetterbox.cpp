// zLetterbox.cpp -- six functions, read from the image with
// tools/disasm.py. The letterbox is a Scaleform movie found once at
// scene setup and driven by name afterwards. SceneSetup hashes
// "LetterBox_reference", turns the hash into a uid and looks the object
// up, keeping it in the file's one static. AnimateIn plays the movie,
// AnimateInPop plays it and calls the movie's own _root.AnimateInPop,
// AnimateOut calls _root.AnimateOut, AnimateOutPop stops the movie, and
// ShowEventSkippable calls _root.ShowEventSkippable. Each of the four
// animate calls broadcasts an event to every entity of type 219
// afterwards -- one hash going in, another going out -- and the two
// "in" entries only act in game state 7 with a single active viewport.
//
// Layouts from the DWARF (tools/dwarf_types.py): zScaleform 0x140, a
// zUI entity for its first 0xF8 bytes (the DWARF's zUI__39C93, 0x100
// whose last member is a bool at +0xF5, so the SECOND base starts at
// 0xF8), World::ScaleformOGAsset 4 bytes as that second base at +0xF8,
// and moviePlaying at +0x120. letterbox_ref is
// letterbox_ref__10zLetterbox, .bss 0x80734C3C, four bytes, so a
// zLetterbox static member. The strings are the unity unit's pool,
// hence the generated header first.
//
// Four shapes the bytes fixed. The second base is what the `beq ; addi
// r3,r3,248` pairs are: passing the letterbox to SFWrap_InvokeASFunc is
// a derived-to-base conversion of a pointer that can be null, so the
// compiler adjusts it under a test -- and in AnimateOut and
// ShowEventSkippable that test is the SAME compare as the null check
// above it, kept in cr1. The state test is an INLINED predicate whose
// bool result is materialised in r31 and tested after: tools/
// dwarf_lines.py makes the whole guard one statement (line 34 of
// AnimateIn, 49 of AnimateInPop), and tools/dwarf_locals.py gives the
// unit no named local at all, so the value in r31 is a return and not a
// variable. A bool local set inside its own `if` gives the same 34 and
// 44 words -- both were compiled and compared (scratch agx_lb_gen.py)
// -- and the debug info is what separates them; the one-expression
// helper is inlined with no out-of-line copy emitted. AnimateIn tests
// moviePlaying for zero and AnimateOut for non-zero, which is the whole
// difference between their two shapes. And ShowEventSkippable's last
// call is a tail branch with the epilogue's blr still behind it.
//
// NEAR MISS: none. All six functions are byte-identical by
// tools/unitcmp.py.

#include "SB/GM/Engine/Game/zLetterbox.pool.h"

typedef unsigned long long uid;

namespace World {

// Four bytes and a vtable pointer in the DWARF: zScaleform's second
// base, which is why a conversion to it costs a test and an addi.
class ScaleformOGAsset {
public:
    virtual void _v0();
};

}  // namespace World

namespace Scaleform {

void SFWrap_InvokeASFunc(World::ScaleformOGAsset* asset, const char* func,
                         unsigned int argc, float* argv);

}  // namespace Scaleform

class xBase;

unsigned int xStrHash(const char* str);
uid xUIDMgrFindUID(unsigned int hash);
xBase* zSceneFindObject(uid id);
int zViewportGetActiveCount();
void zEntEventAllOfType(unsigned int event, unsigned int baseType);

extern int gGameState;

// The zUI entity zScaleform derives from, padded to the offset its
// second base sits at.
class zUIEntity {
public:
    virtual void _v0();

    unsigned char _pad0[0xF2];
};

class zScaleform : public zUIEntity, public World::ScaleformOGAsset {
public:
    void Play();
    void Stop();

    unsigned char _pad1[0x120 - 0xFC];
    bool moviePlaying;
};

class zLetterbox {
public:
    static void SceneSetup();
    static void AnimateIn();
    static void AnimateInPop();
    static void AnimateOut();
    static void AnimateOutPop();
    static void ShowEventSkippable();

    static zScaleform* letterbox_ref;
};

zScaleform* zLetterbox::letterbox_ref;

static inline bool zLetterboxFullScreenGameplay() {
    return gGameState == 7 && zViewportGetActiveCount() == 1;
}

void zLetterbox::SceneSetup() {
    letterbox_ref = (zScaleform*)zSceneFindObject(
        xUIDMgrFindUID(xStrHash("LetterBox_reference")));
}

void zLetterbox::AnimateIn() {
    if (letterbox_ref != 0 && zLetterboxFullScreenGameplay() &&
        !letterbox_ref->moviePlaying) {
        letterbox_ref->Play();

        zEntEventAllOfType(0xAE72E9E5, 219);
    }
}

void zLetterbox::AnimateInPop() {
    if (letterbox_ref != 0 && zLetterboxFullScreenGameplay() &&
        !letterbox_ref->moviePlaying) {
        letterbox_ref->Play();

        Scaleform::SFWrap_InvokeASFunc(letterbox_ref, "_root.AnimateInPop", 0,
                                       0);

        zEntEventAllOfType(0xAE72E9E5, 219);
    }
}

void zLetterbox::AnimateOut() {
    if (letterbox_ref != 0 && letterbox_ref->moviePlaying) {
        Scaleform::SFWrap_InvokeASFunc(letterbox_ref, "_root.AnimateOut", 0, 0);
    }

    zEntEventAllOfType(0x27858BA2, 219);
}

void zLetterbox::AnimateOutPop() {
    if (letterbox_ref != 0 && letterbox_ref->moviePlaying) {
        letterbox_ref->Stop();
    }

    zEntEventAllOfType(0x27858BA2, 219);
}

void zLetterbox::ShowEventSkippable() {
    if (letterbox_ref != 0 && letterbox_ref->moviePlaying) {
        Scaleform::SFWrap_InvokeASFunc(letterbox_ref,
                                       "_root.ShowEventSkippable", 0, 0);
    }
}
