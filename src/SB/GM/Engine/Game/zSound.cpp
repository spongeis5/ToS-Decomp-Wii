#include "SB/GM/Engine/Game/zSound.pool.h"

// zSound.cpp -- the sound module: FMOD's event system behind the game's
// System::Module interface. Read from the image with tools/disasm.py;
// the layouts are the DWARF's (alltypes.h), and the strings and float
// literals come out of WAD03.cpp's pool, which the header in front
// reproduces.
//
// zSoundModule is a System::Module (0x98, vptr at +0x14) followed by
// the master volumes at +0x98, the fade state at +0x3B0 and, at +0x3C8,
// a 100-slot circular queue of streamed events still loading. The FMOD
// objects are reached through their interfaces' vtables at +0 (slots as
// __vt__Q24FMOD14EventCategoryI, 11EventGroupI and 12MusicSystemI give
// them): a category stops its events at slot 6, sets its volume at 7,
// pauses at 11 and mutes at 13; a group fetches a subgroup at 4 and an
// event at 9; the music system pauses at 3.
//
// Not written here: every function that reaches the FMOD system objects
// (fmodSys, fmodEventSys) or is one of the file-system callbacks. All of
// those are in WAD03's anonymous namespace, whose names a fragment
// compiled under its own file name cannot spell.

#define FMOD_EVENT_STATE_LOADING 0x00000002

enum FMOD_RESULT {
    FMOD_OK = 0,
    FMOD_ERR_FILE_BAD = 19,
    FMOD_ERR_FILE_COULDNOTSEEK = 20,
    FMOD_ERR_FILE_EOF = 22,
    FMOD_ERR_FILE_NOTFOUND = 23,
    FMOD_ERR_FILE_UNWANTED = 24,
    FMOD_ERR_UNINITIALIZED = 79,
    FMOD_RESULT_FORCEINT = 65536
};

extern "C" FMOD_RESULT FMOD_Wii_Controller_Command(int controller, int command);

namespace FMOD {

class Event {
public:
    FMOD_RESULT stop(bool immediate);
    FMOD_RESULT setPaused(bool paused);
    FMOD_RESULT getState(unsigned int* state);
};

// Interfaces: only their vtable slots matter here.
class EventCategory {
public:
    virtual FMOD_RESULT getInfo(int* index, char** name);
    virtual FMOD_RESULT getCategory(const char* name, EventCategory** category);
    virtual FMOD_RESULT getCategoryByIndex(int index, EventCategory** category);
    virtual FMOD_RESULT getNumCategories(int* numcategories);
    virtual FMOD_RESULT getEventByIndex(int index, unsigned int mode, Event** event);
    virtual FMOD_RESULT getNumEvents(int* numevents);
    virtual FMOD_RESULT stopAllEvents();
    virtual FMOD_RESULT setVolume(float volume);
    virtual FMOD_RESULT getVolume(float* volume);
    virtual FMOD_RESULT setPitch(float pitch, int units);
    virtual FMOD_RESULT getPitch(float* pitch, int units);
    virtual FMOD_RESULT setPaused(bool paused);
    virtual FMOD_RESULT getPaused(bool* paused);
    virtual FMOD_RESULT setMute(bool mute);
};

class EventGroup {
public:
    virtual FMOD_RESULT getInfo(int* index, char** name);
    virtual FMOD_RESULT loadEventData(int resource, unsigned int mode);
    virtual FMOD_RESULT freeEventData(Event* event, bool waituntilready);
    virtual FMOD_RESULT getGroup(const char* name, bool cacheevents, EventGroup** group);
    virtual FMOD_RESULT getGroupByIndex(int index, bool cacheevents, EventGroup** group);
    virtual FMOD_RESULT getParentGroup(EventGroup** group);
    virtual FMOD_RESULT getParentProject(void** project);
    virtual FMOD_RESULT getNumGroups(int* numgroups);
    virtual FMOD_RESULT getEvent(const char* name, unsigned int mode, Event** event);
    virtual FMOD_RESULT getEventByIndex(int index, unsigned int mode, Event** event);
};

class MusicSystem {
public:
    virtual FMOD_RESULT reset();
    virtual FMOD_RESULT setVolume(float volume);
    virtual FMOD_RESULT getVolume(float* volume);
    virtual FMOD_RESULT setPaused(bool paused);
};

}  // namespace FMOD

FMOD::MusicSystem* zGetFMODMusicSystem();

namespace System {

extern bool gSysForceExit;

class EventSet {
public:
    int stage[4];
};

class RelativePriority;

// The vptr follows the two members declared ahead of the first virtual
// (+0x14). Slots as __vt__Q26System6Module and __vt__12zSoundModule
// give them: 2 GetPriority, 3 Startup, 4 Shutdown, 9 Update.
class Module {
public:
    Module();

    char* name;
    EventSet events;

    virtual void _v0();
    virtual void _v1();
    virtual const RelativePriority* GetPriority(int& count) const;
    virtual void Startup(int stage);
    virtual void Shutdown(int stage);
    virtual void _v5();
    virtual void _v6();
    virtual void _v7();
    virtual void _v8();
    virtual void Update(int stage);

    int contextFlags;
    short eventBindingIndices[60];
    bool enabled;
};

}  // namespace System

namespace TRC {

enum DiscAccessType {
    DiscAccessType_Read = 0
};

void ShowDiscError(DiscAccessType type, const char* message);

}  // namespace TRC

class zGlobals {
public:
    unsigned char _pad0[0x57F];
    bool unpauseFromSystem;
    unsigned char _pad1[0x5A0 - 0x580];
};

extern zGlobals globals;

// The DWARF's zQueue: a circular buffer, `first` the head slot and
// `size` the count. Removing from the middle swaps the entry with the
// last one and drops the last.
class zQueue {
public:
    typedef FMOD::Event* T;
    enum { N = 100 };

    int first;
    int size;
    T data[N];

    int Size() const {
        return size;
    }

    void Clear() {
        size = 0;
        first = 0;
    }

    void Push(T item) {
        data[(first + size) % N] = item;
        if (size >= N) {
            first = (first + 1) % N;
        } else {
            size++;
        }
    }

    void Get(int idx, T& item) {
        if (size > 0) {
            item = data[(first + idx) % N];
        }
    }

    static void Swap(T& a, T& b) {
        T tmp = b;
        b = a;
        a = tmp;
    }

    void PopFront(T& item) {
        item = data[first];
        size--;
        first = (first + 1) % N;
        if (size == 0) {
            first = 0;
        }
    }

    void PopBack(T& item) {
        if (size > 0) {
            item = data[(size + first - 1) % N];
            size--;
            if (size == 0) {
                first = 0;
            }
        }
    }

    void RemoveAt(int idx, T& item) {
        if (size < 2) {
            if (size == 1) {
                PopFront(item);
            }
        } else {
            Swap(data[(first + size - 1) % N], data[(first + idx) % N]);
            PopBack(item);
        }
    }
};

class zSoundModule : public System::Module {
public:
    zSoundModule();

    // Declared only (all three are walls, not written here). With no
    // virtual of its own the class had no key function, and the object
    // emitted a weak __vt__12zSoundModule that retail keeps elsewhere.
    virtual void Startup(int stage);
    virtual void Shutdown(int stage);
    virtual void Update(int stage);

    void SceneExit();
    FMOD::Event* Stop(FMOD::Event* event);
    FMOD::Event* Pause(FMOD::Event* event);
    FMOD::Event* Unpause(FMOD::Event* event);
    void SetMasterFade(float time, bool fadeIn, bool force);
    void SetUnpauseFadeIn(float fadeTime, float delay);
    void AddLoadingStreamingEvent(FMOD::Event* event);
    void UpdateLoadingEvent();
    void SetMasterVolumeMusic(float value);
    void SetMasterVolumeSFX(float value);
    void SetMasterVolumeDialog(float value);
    void SetMasterVolumeCine(float value);
    void EndOfLevelMuteSound();
    void StartOfLevelUnmuteSound();

    static FMOD::EventCategory* SoundCategoryGetCategory(const char* name);
    static void SoundCategorySetMute(const char* name, bool muted);
    static void SoundCategorySetVolume(const char* name, float volume);
    static void SoundCategoryStopAllEvents(const char* name);
    static void SoundCategoryPauseUnpauseAllExceptUI(bool pause);
    static void CheckDirtyDisc(FMOD_RESULT result);
    FMOD_RESULT GetEventByIndex(FMOD::EventGroup* group, int index, unsigned int mode,
                                FMOD::Event** event);
    FMOD_RESULT GetGroupByIndex(FMOD::EventGroup* group, int index, bool cacheevents,
                                FMOD::EventGroup** subgroup);
    static void TRCUnPauseMaster();
    static void TRCPauseMaster();
    static void PauseMaster(bool callByFMV);
    static void UnPauseMaster(bool callByFMV);

    // Category operations retail spells in place in the functions ahead
    // of SoundCategorySetMute: the out-of-line ones are defined after
    // those callers, so -inline auto cannot have folded them there.
    static void CategoryStopAllEvents(const char* name) {
        FMOD::EventCategory* category = SoundCategoryGetCategory(name);
        if (category) {
            category->stopAllEvents();
        }
    }

    static void CategorySetVolume(const char* name, float volume) {
        FMOD::EventCategory* category = SoundCategoryGetCategory(name);
        if (category) {
            category->setVolume(volume);
        }
    }

    static void CategorySetMute(const char* name, bool mute) {
        FMOD::EventCategory* category = SoundCategoryGetCategory(name);
        if (category) {
            category->setMute(mute);
        }
    }

    static void CategorySetPaused(const char* name, bool paused) {
        FMOD::EventCategory* category = SoundCategoryGetCategory(name);
        if (category) {
            category->setPaused(paused);
        }
    }

    static bool muteByFMV;

    float masterVolumeMusic;
    float masterVolumeSFX;
    float masterVolumeCine;
    float masterVolumeDialog;
    bool masterVolumeChanged;
    char spuProgram[256];
    char spuProgramMpeg[256];
    char mediaPath[256];
    void* soundMemory;
    float fadeFraction;
    float fadeCurrentTime;
    float fadeTotalTime;
    bool isFadeIn;
    float beforeFadeInTime;
    int unPauseFromSystemCount;
    zQueue loadingEventList;
};

extern zSoundModule soundMod;

zSoundModule* GlobalGetSoundModule();

zSoundModule* GlobalGetSoundModule() {
    return &soundMod;
}

zSoundModule::zSoundModule() {
    events.stage[2] = 67;
    fadeFraction = 0.0f;
    fadeCurrentTime = 0.0f;
    fadeTotalTime = 1.0f;
    isFadeIn = false;
    beforeFadeInTime = 0.0f;
}

void zSoundModule::SceneExit() {
    if (!System::gSysForceExit) {
        loadingEventList.Clear();
    }
}

FMOD::Event* zSoundModule::Stop(FMOD::Event* event) {
    if (!System::gSysForceExit) {
        if (event) {
            event->stop(false);
        }
    }

    return event;
}

FMOD::Event* zSoundModule::Pause(FMOD::Event* event) {
    if (!System::gSysForceExit) {
        event->setPaused(true);
    }

    return event;
}

FMOD::Event* zSoundModule::Unpause(FMOD::Event* event) {
    if (!System::gSysForceExit) {
        event->setPaused(false);
    }

    return event;
}

void zSoundModule::SetMasterFade(float time, bool fadeIn, bool force) {
    if (!force && isFadeIn && fadeIn) {
        return;
    }

    beforeFadeInTime = 0.0f;

    if (time > 0.0f) {
        fadeCurrentTime = time;
        fadeTotalTime = time;
        isFadeIn = fadeIn;
        fadeFraction = isFadeIn ? 0.0f : 1.0f;
    } else {
        fadeCurrentTime = 0.0f;
        fadeTotalTime = 1.0f;
        isFadeIn = fadeIn;
        fadeFraction = 0.0f;
        CategoryStopAllEvents("dialog");
        CategorySetVolume("music", 0.0f);
        CategorySetVolume("SFX", 0.0f);
    }
}

void zSoundModule::SetUnpauseFadeIn(float fadeTime, float delay) {
    if (globals.unpauseFromSystem && fadeTime > 0.0f && (isFadeIn || !(fadeCurrentTime > 0.0f))) {
        beforeFadeInTime = delay;
        fadeCurrentTime = fadeTime;
        fadeTotalTime = fadeTime;
        isFadeIn = true;
        fadeFraction = 0.0f;
        globals.unpauseFromSystem = false;
    } else {
        beforeFadeInTime = 0.0f;
        fadeCurrentTime = 0.1f;
        fadeTotalTime = 0.1f;
        isFadeIn = true;
        fadeFraction = 0.0f;
    }
}

#pragma push
#pragma always_inline on

void zSoundModule::AddLoadingStreamingEvent(FMOD::Event* event) {
    loadingEventList.Push(event);
}

// The queue is a plain class, not a template: as zQueue<T, N> none of
// Get / RemoveAt / PopFront / Swap / PopBack was inlined here, even with
// always_inline around the template or this caller. The loop reads the
// size through Size() so the fetch reloads it (through both, or neither,
// the load is shared), and Swap takes the last slot first with its body
// reversed, which puts the (first + i) index first as retail does.
// NEAR MISS: 8 of 92 words differ; the size-1 branch (PopFront) colours
// its volatile registers differently -- retail first/size-1/first+1/
// quotient in r3/r0/r4/r3, here r5/r4/r3/r0 -- same instructions, same
// order. Guard, local head copy, Front() accessor, --size and
// size = size - 1 each left it at 8.
void zSoundModule::UpdateLoadingEvent() {
    FMOD::Event* fevent;
    unsigned int st;

    for (int i = 0; i < loadingEventList.Size(); i++) {
        loadingEventList.Get(i, fevent);
        fevent->getState(&st);

        if (!(st & FMOD_EVENT_STATE_LOADING)) {
            fevent->setPaused(false);
            loadingEventList.RemoveAt(i, fevent);
            i--;
        }
    }
}

#pragma pop

void zSoundModule::SetMasterVolumeMusic(float value) {
    masterVolumeMusic = value;
    masterVolumeChanged = true;
    SoundCategorySetVolume("music", 0.5f * (value * fadeFraction));

    CategorySetVolume("UImusic", 0.5f * value);
}

void zSoundModule::SetMasterVolumeSFX(float value) {
    masterVolumeSFX = value;
    masterVolumeChanged = true;
    CategorySetVolume("UI", 0.4f * value);

    SoundCategorySetVolume("SFX", 0.4f * (value * fadeFraction));
}

void zSoundModule::SetMasterVolumeDialog(float value) {
    masterVolumeDialog = value;
    masterVolumeChanged = true;
    SoundCategorySetVolume("dialog", 0.5f * (value * fadeFraction));
    CategorySetVolume("UIdialog", 0.5f * value);
}

void zSoundModule::SetMasterVolumeCine(float value) {
    masterVolumeCine = value;
    CategorySetVolume("UIcine", 0.5f * value);
}

void zSoundModule::EndOfLevelMuteSound() {
    CategorySetMute("music", true);
    CategorySetMute("dialog", true);
    CategorySetMute("SFX", true);
}

void zSoundModule::StartOfLevelUnmuteSound() {
    CategorySetMute("music", false);
    CategorySetMute("dialog", false);
    CategorySetMute("SFX", false);
}

void zSoundModule::SoundCategorySetMute(const char* name, bool muted) {
    FMOD::EventCategory* category;

    category = SoundCategoryGetCategory(name);
    if (category) {
        category->setMute(muted);
    }
}

void zSoundModule::SoundCategorySetVolume(const char* name, float volume) {
    FMOD::EventCategory* category;

    category = SoundCategoryGetCategory(name);
    if (category) {
        category->setVolume(volume);
    }
}

void zSoundModule::SoundCategoryStopAllEvents(const char* name) {
    FMOD::EventCategory* category;

    category = SoundCategoryGetCategory(name);
    if (category) {
        category->stopAllEvents();
    }
}

void zSoundModule::SoundCategoryPauseUnpauseAllExceptUI(bool pause) {
    if (!System::gSysForceExit) {
        zGetFMODMusicSystem()->setPaused(pause);

        CategorySetVolume("music", 0.0f);
        CategorySetVolume("dialog", 0.0f);
        CategorySetVolume("SFX", 0.0f);

        CategorySetPaused("music", pause);
        CategorySetPaused("dialog", pause);
        CategorySetPaused("SFX", pause);
    }
}

void zSoundModule::CheckDirtyDisc(FMOD_RESULT result) {
    if (result == FMOD_ERR_FILE_BAD || result == FMOD_ERR_FILE_EOF ||
        result == FMOD_ERR_FILE_COULDNOTSEEK || result == FMOD_ERR_FILE_NOTFOUND ||
        result == FMOD_ERR_FILE_UNWANTED) {
        TRC::ShowDiscError(TRC::DiscAccessType_Read, 0);
    }
}

FMOD_RESULT zSoundModule::GetEventByIndex(FMOD::EventGroup* group, int index, unsigned int mode,
                                          FMOD::Event** event) {
    if (System::gSysForceExit) {
        return FMOD_ERR_UNINITIALIZED;
    }

    FMOD_RESULT result;
    result = group->getEventByIndex(index, mode, event);
    CheckDirtyDisc(result);
    return result;
}

FMOD_RESULT zSoundModule::GetGroupByIndex(FMOD::EventGroup* group, int index, bool cacheevents,
                                          FMOD::EventGroup** subgroup) {
    if (System::gSysForceExit) {
        return FMOD_ERR_UNINITIALIZED;
    }

    FMOD_RESULT result;
    result = group->getGroupByIndex(index, cacheevents, subgroup);
    CheckDirtyDisc(result);
    return result;
}

void zSoundModule::TRCUnPauseMaster() {
    globals.unpauseFromSystem = true;
    soundMod.SetUnpauseFadeIn(2.0f, 1.0f);

    CategorySetVolume("music", 0.0f);
    CategorySetVolume("dialog", 0.0f);
    CategorySetVolume("SFX", 0.0f);

    UnPauseMaster(false);
    zGetFMODMusicSystem()->setPaused(false);
}

void zSoundModule::TRCPauseMaster() {
    PauseMaster(false);
    zGetFMODMusicSystem()->setPaused(true);
}

void zSoundModule::PauseMaster(bool callByFMV) {
    if (!muteByFMV) {
        CategorySetPaused("master", true);
        muteByFMV = callByFMV;
    }
}

void zSoundModule::UnPauseMaster(bool callByFMV) {
    if (!muteByFMV || callByFMV) {
        CategorySetPaused("master", false);
        muteByFMV = false;
    }
}

void TurnOnWiimoteSpeaker(int max_pad, bool isTurnON) {
    for (int i = 0; i < max_pad; i++) {
        if (isTurnON) {
            FMOD_Wii_Controller_Command(i, 0);
        } else {
            FMOD_Wii_Controller_Command(i, 1);
        }
    }
}
