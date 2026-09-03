// zSoundWiimoteSpeaker.cpp -- nine functions, read from the image with
// tools/disasm.py. A zSoundWiimoteSpeakerList is the scene's one owner
// of the Wii remote's speaker sounds. Init runs xBaseInit, keeps the
// asset and installs the event wrapper, and then either clears the
// array (when a list already claimed the static slot) or claims it,
// takes one block of the asset's count times 72 bytes from the global
// heap under tag 3, placement-constructs a zSoundAsset in each, and
// runs each one's Init with the name, the index tree and the streamed
// flag from the asset's twelve-byte records. SceneExit unregisters
// every sound. _Play and _Stop bounds-check an index against the count
// and hand the slot to the zSoundAsset. The two static Plays reach the
// list through the static slot: one takes an index, the other a zPlayer
// whose controller it asks for the speaker channel. HandleEvent answers
// four ids -- two reset the base, one plays and one stops. Create takes
// 72 bytes from the global heap under tag 16, clears them and places
// the entity. The free SceneExit walks the scene's list of these
// entities and calls each one's.
//
// Layouts from the DWARF (tools/dwarf_types.py):
// zSoundWiimoteSpeakerList 0x48 on World::xOGEntity 0x40, with the asset
// at +0x3C in the base's tail padding, the array at +0x40 and the count
// at +0x44; Sext::zSoundWiimoteSpeakerListAsset 0x18 on xBaseScene, with
// the sound list's count at +0x10 and its data pointer at +0x14;
// zSoundAsset 0x48, whose vtable pointer lands at +0x3C because the
// virtual is declared there -- the same spelling zSoundPhysics.cpp
// already carries. The scene's list of these entities is at
// xScene+0x31C and the scene itself at globals+0x43C, the two loads the
// free SceneExit makes, and its nodes sit four bytes into each entity.
//
// MEASURED: 9 of 9 functions byte-identical by tools/unitcmp.py, 1,028
// bytes. There is no near miss.
//
// Seven shapes the bytes fixed, and one thing the DWARF does not say.
//
// Sext::EventAny is a byte in the DWARF with nothing described inside
// it, so each unit declares the member it reads. HandleEvent reads a
// SIGNED halfword at +0 (`lha`), so it is spelled as a short here;
// zBuyScreen reads a float from the same type and spells it as one.
//
// The event wrapper is NOT this class's. It is 28 bytes of generic
// virtual dispatch -- load the receiver's vtable, take slot 0x58, swap
// the first two arguments and tail-call -- so one static copy serves
// every class in the unity translation unit and dtk names it after the
// first class it could attribute the static to. Defining one here
// leaves an EXTRA function retail does not have, which is how this was
// found; the fragment references the one the unit already carries.
// zEventSpy, zNavLink and zDecal each really do own theirs -- all three
// names are in the symbol table -- which is why they declare their own.
//
// Create calls Init AFTER the branch the placement null-test takes, so
// Init is not part of the constructor: the constructor takes the handle
// alone and Init is a separate statement on the placed pointer.
//
// Both static Plays put the failure LAST -- test, do the work, return
// true, jump over a trailing `li r3,0`. An early `return false` puts the
// zero first and inverts the branch.
//
// The two speaker tests in Play(int, zPlayer*) branch to ONE `li r3,0`
// between them, which two separate `return false` statements do not
// give: that is an `||`. And the three controller slots are named by
// the bytes, not guessed -- with a two-word vtable header the index is
// (offset - 8) / 4, so 0xBC is 45, 0xD8 is 52 and 0x14C is 81, and the
// OFF test sits before the BUSY one in the table.
//
// Init's two branches are retail's in the opposite order. The beq the
// static-slot test takes lands on the pair of clearing stores, so the
// ALREADY-CLAIMED case is the then branch and the claim is the else.
// That alone took it from 49 differing words of 68 to 9.
//
// The last nine words were the register triple, and this is the shape
// NOTES.md records with four other instances and no answer -- a count
// and a pointer in each other's registers. Sixteen spellings were swept
// over four rounds and the first eleven all tied: the order of the
// three locals, the count taken from the member instead of a local
// (much worse, 51), the count as int, as const, the loop as a pointer
// walk (50) and as a while. What moved it was a FOURTH local. Taking
// the allocation into a `void*` and casting that to the array separately
// shifted the allocation enough to put the pointer in retail's register
// -- 7 words to 6 -- and with the pointer settled, declaring the count
// BEFORE the loop counter decided the pair that was left and the
// function matched. Declaration order had not moved those two at all
// while the pointer was still in the way. So the lever for this shape
// is not only the order of the two registers that differ: a third value
// that changes the allocation can be what lets the order matter.

typedef unsigned long long uid;

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {
enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };

void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap, eMemMgrTag tag,
                      bool clear);
}  // namespace Memory

extern "C" void* memset(void* dst, int c, unsigned long n);

void* xMemAlloc(Memory::GlobalHeapEnum heap, unsigned int size, int align,
                eMemMgrTag tag);

inline void* operator new(unsigned long, void* p) { return p; }

class LinkAsset;
class TemplateEntity;
class xBase;
class Event;
class Channel;
class FMOD_VECTOR;

namespace World {
class EntityHandleBase;
}

namespace Sext {

class EventAny {
public:
    short value;
};

class xBaseAsset {
public:
    uid id;
    unsigned int baseType;
    unsigned short linkCount;
    unsigned short baseFlags;
};

}  // namespace Sext

void xBaseInit(xBase* base, const Sext::xBaseAsset* asset);
void xBaseReset(xBase* base, Sext::xBaseAsset* asset);

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

}  // namespace World

// The vtable pointer lands at +0x3C, where the virtual is declared --
// the spelling zSoundPhysics.cpp already carries for this class.
class zSoundAsset {
public:
    zSoundAsset();

    void PlayWiimote(int channel);
    void Stop(bool immediate);

    char* soundSourceName;
    int soundSourceIdx;
    int indicesCount;
    int* indicesTree;
    int level;
    Event* event;
    Channel* channel;
    unsigned int _pad0;
    uid idLoopingSoundOwner;
    FMOD_VECTOR* pos;
    void* userCallback;
    void* userCallbackData;
    char rootGroupName[6];
    bool streamed;
    bool stopOnAnimEnd;

    virtual void Init(const char* name, int* indices, bool streamed);

    float max3Ddistance;
};

void zSoundAsset_Unregister(zSoundAsset* asset);

// The controller the player holds, reached through the player and asked
// for the speaker channel. Only the three slots the bytes call are
// named; the ones before them exist to put those three where they are.
class zPlayerController {
public:
    virtual void _v0();  virtual void _v1();  virtual void _v2();
    virtual void _v3();  virtual void _v4();  virtual void _v5();
    virtual void _v6();  virtual void _v7();  virtual void _v8();
    virtual void _v9();  virtual void _v10();  virtual void _v11();
    virtual void _v12();  virtual void _v13();  virtual void _v14();
    virtual void _v15();  virtual void _v16();  virtual void _v17();
    virtual void _v18();  virtual void _v19();  virtual void _v20();
    virtual void _v21();  virtual void _v22();  virtual void _v23();
    virtual void _v24();  virtual void _v25();  virtual void _v26();
    virtual void _v27();  virtual void _v28();  virtual void _v29();
    virtual void _v30();  virtual void _v31();  virtual void _v32();
    virtual void _v33();  virtual void _v34();  virtual void _v35();
    virtual void _v36();  virtual void _v37();  virtual void _v38();
    virtual void _v39();  virtual void _v40();  virtual void _v41();
    virtual void _v42();  virtual void _v43();  virtual void _v44();
    virtual int IsSpeakerOff();
    virtual void _v46();  virtual void _v47();  virtual void _v48();
    virtual void _v49();  virtual void _v50();  virtual void _v51();
    virtual int IsSpeakerBusy();
    virtual void _v53();  virtual void _v54();  virtual void _v55();
    virtual void _v56();  virtual void _v57();  virtual void _v58();
    virtual void _v59();  virtual void _v60();  virtual void _v61();
    virtual void _v62();  virtual void _v63();  virtual void _v64();
    virtual void _v65();  virtual void _v66();  virtual void _v67();
    virtual void _v68();  virtual void _v69();  virtual void _v70();
    virtual void _v71();  virtual void _v72();  virtual void _v73();
    virtual void _v74();  virtual void _v75();  virtual void _v76();
    virtual void _v77();  virtual void _v78();  virtual void _v79();
    virtual void _v80();
    virtual int SpeakerChannel();
};

class zPlayer {
public:
    unsigned char _pad0[0x1E8];
    zPlayerController* controller;
    unsigned char _pad1[0x2EC - 0x1EC];
    int state;
};

class EmbeddedListNode {
public:
    EmbeddedListNode* next;
    EmbeddedListNode* prev;
};

class xScene {
public:
    unsigned char _pad0[0x31C];
    EmbeddedListNode speakerList;
};

class zGlobals {
public:
    unsigned char _pad0[0x43C];
    xScene* sceneCur;
};

extern zGlobals globals;

class zSoundWiimoteSpeakerList;

namespace Sext {

// The asset's sound list: a count and a pointer to records whose three
// fields are the three arguments of zSoundAsset::Init.
class SoundListEntry {
public:
    bool streamed;
    char* soundSourceName;
    int* indicesTree;
};

class SoundList {
public:
    unsigned int count;
    SoundListEntry* data;
};

class zSoundWiimoteSpeakerListAsset : public xBaseAsset {
public:
    SoundList soundList;

    static zSoundWiimoteSpeakerList* Create(
        World::EntityHandleBase* handle,
        zSoundWiimoteSpeakerListAsset* asset);
};

}  // namespace Sext

class zSoundWiimoteSpeakerList : public World::xOGEntity {
public:
    zSoundWiimoteSpeakerList(World::EntityHandleBase* handle)
        : World::xOGEntity(handle) {}

    void Init(Sext::zSoundWiimoteSpeakerListAsset* asset);
    void SceneExit();
    bool _Play(int index, int channel);
    void _Stop(int index);
    void HandleEvent(xBase* from, unsigned int event, Sext::EventAny* any);

    static bool Play(int index, int channel);
    static bool Play(int index, zPlayer* player);

    static zSoundWiimoteSpeakerList* SoundList;

    Sext::zSoundWiimoteSpeakerListAsset* asset;
    zSoundAsset* soundAssetList;
    int soundCount;
};

zSoundWiimoteSpeakerList* zSoundWiimoteSpeakerList::SoundList;

// The translation unit's one generic event wrapper, which every class
// in it installs: it dispatches through slot 0x58 of the receiver's
// vtable, so it needs no knowledge of the receiver's type. dtk names
// it after zCamTweakCurve because that is the first class it could
// attribute the static to, not because that class owns it.
void zCamTweakCurveEventWrapper(xBase* from, xBase* to,
                                unsigned int event, Sext::EventAny* any);

void zSoundWiimoteSpeakerList::Init(
    Sext::zSoundWiimoteSpeakerListAsset* a) {
    xBaseInit(this, a);

    asset = a;
    eventFunc = zCamTweakCurveEventWrapper;

    if (SoundList != 0) {
        soundCount = 0;
        soundAssetList = 0;
    } else {
        SoundList = this;

        unsigned int count = asset->soundList.count;
        unsigned int i;

        soundCount = count;

        void* block = xMemAlloc((Memory::GlobalHeapEnum)0,
                                count * sizeof(zSoundAsset), 0,
                                (eMemMgrTag)3);
        zSoundAsset* list = (zSoundAsset*)block;

        if (list != 0) {
            for (i = 0; i < count; i++) {
                new (&list[i]) zSoundAsset();
            }
        }

        soundAssetList = list;

        for (int i = 0; i < soundCount; i++) {
            Sext::SoundListEntry* entry = &asset->soundList.data[i];

            soundAssetList[i].Init(entry->soundSourceName, entry->indicesTree,
                                   entry->streamed);
        }
    }
}

void zSoundWiimoteSpeakerList::SceneExit() {
    for (int i = 0; i < soundCount; i++) {
        zSoundAsset_Unregister(&soundAssetList[i]);
    }
}

bool zSoundWiimoteSpeakerList::_Play(int index, int channel) {
    if (index >= 0 && index <= soundCount && (unsigned int)channel <= 3) {
        soundAssetList[index].PlayWiimote(channel);
        return true;
    }

    return false;
}

void zSoundWiimoteSpeakerList::_Stop(int index) {
    if (index < 0) {
        return;
    }

    if (index > soundCount) {
        return;
    }

    soundAssetList[index].Stop(false);
}

bool zSoundWiimoteSpeakerList::Play(int index, int channel) {
    if (SoundList != 0) {
        SoundList->_Play(index, channel);
        return true;
    }

    return false;
}

bool zSoundWiimoteSpeakerList::Play(int index, zPlayer* player) {
    if (player != 0 && player->controller != 0) {
        if (player->state == 6) {
            if (player->controller->IsSpeakerBusy() ||
                player->controller->IsSpeakerOff()) {
                return false;
            }
        }

        return Play(index, player->controller->SpeakerChannel());
    }

    return false;
}

void zSoundWiimoteSpeakerList::HandleEvent(xBase* from, unsigned int event,
                                           Sext::EventAny* any) {
    switch (event) {
    case 0x389E01C0:
        xBaseReset(this, asset);
        break;
    case 0xA8B93047:
        xBaseReset(this, asset);
        break;
    case 0x09FF6C82:
        _Play(any->value, 0);
        break;
    case 0x1A7F1C90:
        _Stop(any->value);
        break;
    }
}

zSoundWiimoteSpeakerList* Sext::zSoundWiimoteSpeakerListAsset::Create(
    World::EntityHandleBase* handle,
    Sext::zSoundWiimoteSpeakerListAsset* asset) {
    zSoundWiimoteSpeakerList* entity =
        new (memset(Memory::AllocGlobalHeap(
                        sizeof(zSoundWiimoteSpeakerList),
                        (Memory::GlobalHeapEnum)0, (eMemMgrTag)16, false),
                    0, sizeof(zSoundWiimoteSpeakerList)))
            zSoundWiimoteSpeakerList(handle);

    entity->Init(asset);
    return entity;
}

void zSoundWiimoteSpeakerList_SceneExit() {
    EmbeddedListNode* head = &globals.sceneCur->speakerList;

    for (EmbeddedListNode* node = head->next; node != head; node = node->next) {
        ((zSoundWiimoteSpeakerList*)((char*)node - 4))->SceneExit();
    }
}
