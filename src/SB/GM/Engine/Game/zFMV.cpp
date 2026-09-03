// zFMV.cpp -- four functions, read from the image with tools/disasm.py.
// An FMV is named on disc by a letter and a number: zFMVFileGetName
// writes 'V' and then the identifier as decimal. The asset's Create
// takes one 72-byte block from the global heap (tag 16), places the
// entity on it, runs xBaseInit, clears the event callback, copies the
// asset's identifier and builds the file name into the entity.
// PlayMovieTemp is the whole of playing one movie: pause the sound
// master, mark the movie playing, drop any rumble, render the
// subtitles asset the movie names if it names one, play it, broadcast
// one Scaleform event for a clean finish and another for a skip,
// unmark and unpause, and map the player's return code to 0, 1 or 2.
// zFMVPlay is the file-name half: unless movies are switched off, it
// tries "FMV<lang><name>_<locale>" first, falls back to
// "FMV<lang><name>" with rumble disabled across the retry, then walks
// the four game ports handing each pad a mask, unpauses the menu and
// records that an FMV just finished.
//
// Layouts from the DWARF (tools/dwarf_types.py): Sext::FMVReference
// 0x20 with its identifier at +0x10; zFMVReference is 72 bytes on
// xOGEntity 0x40, the identifier at +0x3C in the base's tail padding
// and the eight-byte name at +0x40; Graphics::MovieData 0x50 with the
// name at +0, the subtitles uid at +0x40 and the skippable flag at
// +0x49; zGlobals 0x5A0 with noMovies at +0x568, fmvJustFinished at
// +0x577 and fmvPlaying at +0x578 -- those three offsets are plain
// immediates off the one relocated base, so they are measured, not
// masked. The parameter and local NAMES are the DWARF's
// (tools/dwarf_locals.py): handle/asset, data with toParamWidgetPtr
// and retval, and name/buttons/time/skippable with fullFileName, i and
// pad. tools/dwarf_lines.py gives the statement order. The strings and
// the one float literal are the unity unit's, hence the generated
// header first.
//
// Seven shapes the bytes fixed. zFMVFileGetName is emitted out of line
// AND inlined into Create -- it has external linkage, so -O4 does both,
// and Create's copy shares the identifier it has just stored. The
// subtitles uid is tested as a 64-bit value with `or.` of its two
// halves. World::EntityManager::FindAsset is STATIC: the bytes call
// World::GetEntityManager() and then throw r3 away, which is what an
// object expression in front of a static member does. The skippable
// mask is a conditional EXPRESSION on a bool, which the compiler
// lowers branchlessly through subfic/subfe rather than a branch. And
// the pad call goes through vtable slot +80, so zPlayerInput is
// declared with nineteen virtuals and nothing defines them -- retail's
// entry there is the folded empty weak function, so the image cannot
// name it either.
//
// The last two were one word each, and both were swept (scratch
// agx_fmv_gen.py). PlayMovieTemp's result is a ternary whose arms are
// ENUM constants: retail materialises one value, tests, and overwrites,
// falling into the epilogue with no jump, where `? 2 : 1` on plain
// integers is folded to `1 + (retval == 4)` -- cntlzw and a shift, four
// words and no branch. Eight forms were compiled: five if/else and
// early-return shapes all sit at 56 of 60, a switch at 57 in 62 words,
// and only the enum ternary and a named result variable reach 60 of
// 60. The ternary is the one kept, because dwarf_lines makes the whole
// tail one statement. And zFMVPlay's port loop compares
// UNSIGNED (cmplwi) while dwarf_locals types the counter `int`, so the
// bound is what carries it: `i < 4U`, a cast on the counter and an
// unsigned named bound all give the same bytes, and a plain `i < 4`
// gives cmpwi.
//
// NEAR MISS: none. All four functions are byte-identical by
// tools/unitcmp.py.

#include "SB/GM/Engine/Game/zFMV.pool.h"

typedef unsigned long long uid;

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {
enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };

void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap, eMemMgrTag tag,
                      bool clear);
}  // namespace Memory

extern "C" {
void* memset(void* dst, int c, unsigned long n);
int sprintf(char* buf, const char* fmt, ...);
}

inline void* operator new(unsigned long, void* p) { return p; }

class LinkAsset;
class TemplateEntity;
class xBase;
class zFMVReference;
class zPlayerInput;

namespace World {
class EntityHandleBase;
}

namespace Sext {

class EventAny;
class xSubtitlesAsset;

class xBaseAsset {
public:
    uid id;
    unsigned int baseType;
    unsigned short linkCount;
    unsigned short baseFlags;
};

enum FMVIdentifier { FMVIdentifier_ = 0x7FFFFFFF };

class FMVReference : public xBaseAsset {
public:
    static zFMVReference* Create(World::EntityHandleBase* handle,
                                 FMVReference* asset);

    FMVIdentifier fmv;
    unsigned char EventLinksNew[8];
};

}  // namespace Sext

void xBaseInit(xBase* base, const Sext::xBaseAsset* asset);

void xSubTitlesRender(Sext::xSubtitlesAsset* asset);

const char* xSTGetLocalizationCode();

unsigned int iFMVPlay(const char* name, unsigned int buttons, float time,
                      bool skippable);

void zMenuPause(bool pause);

class xBase {
public:
    virtual void _v0();

    unsigned char _pad0[0x14];
    uid id;
    unsigned int baseType;
    unsigned char _pad1[0xC];
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

class EntityManager {
public:
    static void* FindAsset(uid id);
};

EntityManager& GetEntityManager();

}  // namespace World

class zFMVReference : public World::xOGEntity {
public:
    zFMVReference(World::EntityHandleBase* handle)
        : World::xOGEntity(handle) {}

    virtual void _v1();

    unsigned int fmv;
    char name[8];
};

namespace Graphics {

enum movieEndCode { eMovieDone = 0, eMoviePlayed = 1, eMovieSkipped = 2 };

class MovieData {
public:
    char movie[12];
    unsigned char _pad0[0x40 - 0xC];
    uid subtitlesID;
    bool fullscreen;
    bool skippable;
    bool last;
    unsigned char _pad1[1];
    bool (*delaycbFn)();
};

}  // namespace Graphics

namespace Scaleform {

enum eSFEventType { eSFEventType_ = 0x7FFFFFFF };

void SFWrap_BroadcastEvent(eSFEventType event);

}  // namespace Scaleform

namespace xRumble {

class Manager {
public:
    static Manager& Get();

    void ClearRumbling();
    void EnableRumbling(bool enable);
};

}  // namespace xRumble

class zSoundModule {
public:
    static void PauseMaster(bool pause);
    static void UnPauseMaster(bool unpause);
};

// Nineteen virtuals: the pad call reads slot +80, and mwcc puts the Nth
// virtual at 8 + 4N. None is defined, so no vtable lands in the object.
class zPlayerInput {
public:
    virtual void _v0();
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
    virtual void ClearButtons(unsigned int mask);
};

class zPlayerInputNS {
public:
    static zPlayerInput* GetPadAtGamePort(int port);
};

// The three fields the code reaches, at the offsets the DWARF gives.
class zGlobals {
public:
    unsigned char _pad0[0x568];
    unsigned int noMovies;
    unsigned char _pad1[0x577 - 0x56C];
    bool fmvJustFinished;
    bool fmvPlaying;
    unsigned char _pad2[0x5A0 - 0x579];
};

extern zGlobals globals;

// System::config is a POINTER: the bytes load it and then read a signed
// char out of what it points at.
namespace System {

class Config {
public:
    unsigned char _pad0[36];
    char language;
};

extern Config* config;

}  // namespace System

void zFMVFileGetName(char* name, unsigned int fmv);
unsigned int zFMVPlay(const char* name, unsigned int buttons, float time,
                      bool skippable);

void zFMVFileGetName(char* name, unsigned int fmv) {
    name[0] = 'V';

    sprintf(name + 1, "%d", fmv);
}

zFMVReference* Sext::FMVReference::Create(World::EntityHandleBase* handle,
                                          FMVReference* asset) {
    zFMVReference* ref = new (memset(
        Memory::AllocGlobalHeap(sizeof(zFMVReference),
                                (Memory::GlobalHeapEnum)0, (eMemMgrTag)16,
                                false),
        0, sizeof(zFMVReference))) zFMVReference(handle);

    xBaseInit(ref, asset);

    ref->eventFunc = 0;

    ref->fmv = asset->fmv;

    zFMVFileGetName(ref->name, ref->fmv);

    return ref;
}

Graphics::movieEndCode PlayMovieTemp(Graphics::MovieData* data) {
    zSoundModule::PauseMaster(true);

    globals.fmvPlaying = true;

    xRumble::Manager::Get().ClearRumbling();

    Sext::xSubtitlesAsset* toParamWidgetPtr = 0;

    if (data->subtitlesID != 0) {
        toParamWidgetPtr = (Sext::xSubtitlesAsset*)World::GetEntityManager()
                               .FindAsset(data->subtitlesID);
    }

    xSubTitlesRender(toParamWidgetPtr);

    unsigned int retval = zFMVPlay(data->movie,
                                   data->skippable ? 0x0002FFFF : 0, 0.1f,
                                   data->skippable);

    if (retval == 0) {
        Scaleform::SFWrap_BroadcastEvent((Scaleform::eSFEventType)34);
    } else {
        Scaleform::SFWrap_BroadcastEvent((Scaleform::eSFEventType)35);
    }

    globals.fmvPlaying = false;

    zSoundModule::UnPauseMaster(true);

    if (retval == 0) {
        return Graphics::eMovieDone;
    }

    return retval == 4 ? Graphics::eMovieSkipped : Graphics::eMoviePlayed;
}

unsigned int zFMVPlay(const char* name, unsigned int buttons, float time,
                      bool skippable) {
    unsigned int retval = 0;

    if (globals.noMovies == 0) {
        char fullFileName[256];

        sprintf(fullFileName, "FMV%c%s_%s", System::config->language, name,
                xSTGetLocalizationCode());

        retval = iFMVPlay(fullFileName, buttons, time, skippable);

        if (retval == 1) {
            sprintf(fullFileName, "FMV%c%s", System::config->language, name);

            xRumble::Manager::Get().ClearRumbling();
            xRumble::Manager::Get().EnableRumbling(false);

            retval = iFMVPlay(fullFileName, buttons, time, skippable);

            xRumble::Manager::Get().EnableRumbling(true);
        }

        for (int i = 0; i < 4U; i++) {
            zPlayerInput* pad = zPlayerInputNS::GetPadAtGamePort(i);

            if (pad != 0) {
                pad->ClearButtons(0x0002FFFF);
            }
        }

        zMenuPause(false);

        globals.fmvJustFinished = true;
    }

    return retval;
}
