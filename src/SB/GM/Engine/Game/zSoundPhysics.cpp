// zSoundPhysics.cpp -- five functions, read from the image with
// tools/disasm.py. A physics sound source owns up to three sound assets
// -- hit, scrape and roll -- taken from one asset block. Init keeps the
// asset's physics data and the owning uid and then, three times over,
// takes a named sound when the asset names one that is not the empty
// string and stores a null when it does not. _Init is the shared half:
// it refuses outright when sound effects are off, walks the slots
// already in use and returns the one that already carries the same name
// and index tree, refuses when all 32 slots are taken, and otherwise
// initialises the next slot through its first virtual and returns it.
// StopAll stops each of the three that exists, for the owning uid.
// InitMemory takes one 3,328-byte block for 32 slots, constructs each in
// place and publishes the array with an empty count. DeInitInstances
// stops and unregisters every slot in use and empties the count.
//
// Layouts from the DWARF (tools/dwarf_types.py): zSoundSourcesPhysics
// 0x18, the three assets at +0, +4 and +8, the physics data at +0xC and
// the owner uid at +0x10; Sext::SoundSourcesPhysics 0x38, three
// SoundBankSource of 0xC (streamed at +0, the name at +4, the indices at
// +8) then the 0x14-byte physics data at +0x24; zSoundAsset 0x48 and
// zSoundAssetMultiple 0x68 on it, which is the 104-byte stride the loops
// walk. zSoundAsset's VPTR is at +0x3C -- the DWARF leaves exactly that
// word undescribed between stopOnAnimEnd at +0x3B and max3Ddistance at
// +0x40, and _Init calls through it -- so the virtual is declared
// between those two members, and the slot it calls is vptr+8, the first.
// The flag _Init refuses on is xGlobals::NoSoundFX at globals+0x3DF.
//
// Four shapes the bytes fixed. The three blocks of Init read the asset's
// name and indices through the reference each time rather than through
// locals: the DWARF gives Init no locals at all, only `this` and
// `asset`, and the two loads that survive strlen are the compiler's
// doing. _Init's refusal is a `return` written between the tests and the
// work, so it stays where it is instead of moving to the end. Its loop
// re-reads both statics every iteration -- the array base twice inside
// one body -- so neither is held in a local, and the count the loop exit
// leaves in a register is the same read the 32-slot test uses. And the
// slot handed back at the end is addressed from the count AFTER the
// increment, less one, which is what `ms_slotsLast++` followed by an
// index of `ms_slotsLast - 1` gives; the array base is read a third time
// for it.
//
// NEAR MISS, two of the five.
//
// InitMemory differs in 7 of its 31 words and every one of them is the
// same pair of registers: retail holds the allocated block in r29 and
// the loop's counter in r30, ours the other way round, with the byte
// offset in r31 either way. That is the shape NOTES.md records as
// having four other instances and no answer -- BuildMemory, Texture,
// SkeletonBlobEntity and zNPCType::Setup. Declaring the counter above
// the block, which is the lever that works elsewhere, changes nothing
// here, and neither does defining the counter with a value above the
// allocation: that one is worse, ten differing words against seven,
// re-measured on the state this file is committed in.
//
// Init differs in 64 of the 61 words that can be compared -- ours is
// 244 bytes against retail's 264, five instructions short -- and has
// not been read: the session that wrote this unit was cut off, so no
// spelling has been tried and rejected for it. Start from the aligned
// diff:
//
//   python tools/unitcmp.py SB/GM/Engine/Game/zSoundPhysics
//
typedef unsigned long long uid;

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {
enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };
}  // namespace Memory

void* xMemAlloc(Memory::GlobalHeapEnum heap, unsigned long size, int align,
                eMemMgrTag tag);

extern "C" unsigned long strlen(const char* s);

inline void* operator new(unsigned long, void* p) { return p; }

class Event;
class Channel;
class FMOD_VECTOR;
class zSoundAsset;

namespace Sext {

class SoundPhysicsData {
public:
    float MinVelocity;
    float MaxVelocity;
    float MinRun;
    float MaxRun;
    float MaxRise;
};

class SoundBankSource {
public:
    bool streamed;
    unsigned char pad1;
    unsigned char pad2;
    unsigned char pad3;
    char* sourceString;
    int* indices;
};

class SoundSourcesPhysics {
public:
    SoundBankSource SoundHit;
    SoundBankSource SoundScrape;
    SoundBankSource SoundRoll;
    SoundPhysicsData PhysicsData;
};

}  // namespace Sext

class xGlobals {
public:
    unsigned char _pad0[0x3DF];
    bool NoSoundFX;
};

extern xGlobals globals;

class zSoundAssetMultiple;

// The vtable pointer lands at +0x3C, where the virtual is declared.
class zSoundAsset {
public:
    bool IsEqual(const char* name, int* indices);

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

class zSoundAssetMultiple : public zSoundAsset {
public:
    zSoundAssetMultiple();

    void Stop(uid owner);
    void StopAll();

    int InstanceListSize;
    unsigned char InstanceList[0x1C];
};

class zSoundSourcesPhysics {
public:
    void Init(const Sext::SoundSourcesPhysics& asset, uid owner);
    zSoundAssetMultiple* _Init(const char* name, int* indices, bool streamed);
    void StopAll();

    static void InitMemory();
    static void DeInitInstances();

    static zSoundAssetMultiple* ms_slotsPhysStorage;
    static int ms_slotsLast;

    zSoundAssetMultiple* m_soundHit;
    zSoundAssetMultiple* m_soundScrape;
    zSoundAssetMultiple* m_soundRoll;
    Sext::SoundPhysicsData* m_soundPhysicsData;
    uid m_owner;
};

void zSoundSourcesPhysics::Init(const Sext::SoundSourcesPhysics& asset,
                                uid owner) {
    m_soundPhysicsData = (Sext::SoundPhysicsData*)&asset.PhysicsData;
    m_owner = owner;

    if (asset.SoundHit.sourceString != 0 &&
        strlen(asset.SoundHit.sourceString) != 0) {
        m_soundHit = _Init(asset.SoundHit.sourceString, asset.SoundHit.indices,
                           asset.SoundHit.streamed);
    } else {
        m_soundHit = 0;
    }

    if (asset.SoundScrape.sourceString != 0 &&
        strlen(asset.SoundScrape.sourceString) != 0) {
        m_soundScrape = _Init(asset.SoundScrape.sourceString,
                              asset.SoundScrape.indices,
                              asset.SoundScrape.streamed);
    } else {
        m_soundScrape = 0;
    }

    if (asset.SoundRoll.sourceString != 0 &&
        strlen(asset.SoundRoll.sourceString) != 0) {
        m_soundRoll = _Init(asset.SoundRoll.sourceString,
                            asset.SoundRoll.indices, asset.SoundRoll.streamed);
    } else {
        m_soundRoll = 0;
    }
}

zSoundAssetMultiple* zSoundSourcesPhysics::_Init(const char* name,
                                                 int* indices, bool streamed) {
    if (globals.NoSoundFX) {
        return 0;
    }

    for (int i = 0; i < ms_slotsLast; i++) {
        if (ms_slotsPhysStorage[i].IsEqual(name, indices)) {
            return &ms_slotsPhysStorage[i];
        }
    }

    if (ms_slotsLast >= 32) {
        return 0;
    }

    ms_slotsPhysStorage[ms_slotsLast].Init(name, indices, streamed);
    ms_slotsLast++;

    return &ms_slotsPhysStorage[ms_slotsLast - 1];
}

void zSoundSourcesPhysics::StopAll() {
    if (m_soundHit != 0) {
        m_soundHit->Stop(m_owner);
    }

    if (m_soundScrape != 0) {
        m_soundScrape->Stop(m_owner);
    }

    if (m_soundRoll != 0) {
        m_soundRoll->Stop(m_owner);
    }
}

void zSoundSourcesPhysics::InitMemory() {
    zSoundAssetMultiple* storage = (zSoundAssetMultiple*)xMemAlloc(
        (Memory::GlobalHeapEnum)0, 32 * sizeof(zSoundAssetMultiple), 0,
        (eMemMgrTag)3);

    if (storage != 0) {
        for (unsigned int i = 0; i < 32; i++) {
            new (&storage[i]) zSoundAssetMultiple();
        }
    }

    ms_slotsPhysStorage = storage;
    ms_slotsLast = 0;
}

void zSoundSourcesPhysics::DeInitInstances() {
    for (int i = 0; i < ms_slotsLast; i++) {
        ms_slotsPhysStorage[i].StopAll();
        zSoundAsset_Unregister(&ms_slotsPhysStorage[i]);
    }

    ms_slotsLast = 0;
}
