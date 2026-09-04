// zHintSphere.cpp -- twenty-three functions, 5,056 bytes, read from the
// image with tools/disasm.py. A hint sphere plays a levelled sequence of
// voice hints while a player stands inside it; the sound side is a
// zHintSoundSourceManager embedded in the entity at +0x40.
//
// Layouts from the DWARF (tools/dwarf_types.py): zHintSphere 0x90 on
// World::xOGEntity, with the asset at +0x3C, the manager at +0x40, two
// timers at +0x74 and +0x78, the level and repeat at +0x7C and +0x80,
// four bools from +0x84 and the nearest player at +0x88.
// zHintSoundSourceManager 0x34 -- the asset, the sound array and its
// count, isPlaying at +0xC, a timer at +0x10 and four indices from
// +0x14, then four active-sound pointers from +0x24. zHintSoundAsset
// 0x58 on zSoundAsset 0x48, adding Delay, level, player and element;
// 88 is the stride SceneExit walks. Sext::zHintSphereAsset 0x70 with the
// link array at +0x64, which is the +100 Init stores.
//
// MEASURED: 6 of the 7 functions this object defines are
// byte-identical, 332 bytes. Sixteen of the unit's twenty-three are not
// written yet -- the manager's Init (580), Update (512) and Play, the
// sphere's two UpdateParse functions, inRange (656), Play, Rewind,
// Reset, Setup, HandleEvent, Create and the free zHintSphere_Update.
//
// TWO STATIC-VERSUS-MEMBER CALLS, and the bytes name both. 
// World::EntityManager::FindAsset is STATIC: retail calls
// GetEntityManager, then overwrites r3 -- the manager it just returned
// -- with the uid's high word before the call. That is exactly what a
// static member reached through an object expression compiles to, the
// object evaluated for its side effect and then dropped.
// zHintSoundSourceManager::HintIsPlaying is the opposite: it is a
// MEMBER, because retail reads the sound's event from r4 and not r3.
// Spelled the other way each was three words out.
//
// NEAR MISS -- DebugReset, 21 of the 20 words that compare and two
// instructions short (80 bytes against 88). Retail reads the asset's
// 64-bit uid into two callee-saved registers BEFORE calling
// GetEntityManager and passes them as the pair; ours reaches the same
// call with two instructions fewer. Not swept.

typedef unsigned long long uid;

class LinkAsset;
class TemplateEntity;
class xBase;
class zPlayer;

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

class zHintSphereAsset : public xBaseAsset {
public:
    bool isActive;
    bool canInterrupt;
    bool isProtected;
    unsigned char _pad0[1];
    float timeout;
    unsigned char _pad1[0x64 - 0x18];
    LinkAssetArray EventLinksNew;
    unsigned char _pad2[0x70 - 0x6C];
};

}  // namespace Sext

void xBaseInit(xBase* base, const Sext::xBaseAsset* asset);

// The translation unit's one generic event wrapper: it dispatches
// through the receiver's vtable, so every class in the unit installs the
// same static and dtk names it after the first one it could attribute
// it to. WAD01_15.cpp records the same thing.
void zCamTweakCurveEventWrapper(xBase* from, xBase* to, unsigned int event,
                                Sext::EventAny* any);

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
    xOGModelHandle ogModel;
};

class EntityManager {
public:
    static Sext::zHintSphereAsset* FindAsset(uid id);
};

EntityManager* GetEntityManager();

}  // namespace World

namespace System {
extern bool gSoundReadyToExit;
}

namespace FMOD {

class Event {
public:
    int getState(unsigned int* state);
};

}  // namespace FMOD

class zSoundAsset {
public:
    unsigned char _pad0[0x14];
    FMOD::Event* event;
    unsigned char _pad1[0x44 - 0x18];
};

class zHintSoundAsset : public zSoundAsset {
public:
    bool Equal(int level, int player);

    float Delay;
    int level;
    int player;
    int element;
    unsigned char _pad0[0x58 - 0x54];
};

void zSoundAsset_Unregister(zSoundAsset* asset);

class zHintSoundSourceManager {
public:
    void Init(Sext::zHintSphereAsset* asset);
    void SceneExit();

    bool HintIsPlaying(zHintSoundAsset* sound);

    Sext::zHintSphereAsset* asset;
    zHintSoundAsset* soundAssetList;
    int soundAssetCount;
    bool isPlaying;
    unsigned char _pad0[3];
    float timer;
    int nowLevel;
    int nowPlayer;
    int nowPlaying;
    int nextToPlay;
    zHintSoundAsset* playerActiveSound[4];
};

class zHintSphere : public World::xOGEntity {
public:
    void Init(Sext::zHintSphereAsset* asset);
    void Reset();
    void DebugReset();
    void ResetTimer();
    void SceneExit();

    Sext::zHintSphereAsset* asset;
    zHintSoundSourceManager soundSource;
    float insideTimer;
    float timeoutTimer;
    int currentLevel;
    int currentRepeat;
    bool isFirstPlay;
    bool isFinish;
    bool isActive;
    bool isPlayerIn;
    zPlayer* nearestPlayer;
    unsigned char _pad0[0x90 - 0x8C];
};

bool zHintSoundAsset::Equal(int l, int p) {
    bool equal = false;

    if (l == level && p == player) {
        equal = true;
    }

    return equal;
}

void zHintSoundSourceManager::SceneExit() {
    int i;

    for (i = 0; i < soundAssetCount; i++) {
        zSoundAsset_Unregister(&soundAssetList[i]);
    }
}

bool zHintSoundSourceManager::HintIsPlaying(zHintSoundAsset* sound) {
    unsigned int state;

    if (System::gSoundReadyToExit) {
        return false;
    }

    sound->event->getState(&state);

    return (state & 0x1A) != 0;
}

void zHintSphere::Init(Sext::zHintSphereAsset* a) {
    xBaseInit(this, a);

    asset = a;
    eventFunc = zCamTweakCurveEventWrapper;
    linkArray = (LinkAsset*)&a->EventLinksNew;
    isFirstPlay = true;
}

void zHintSphere::ResetTimer() {
    insideTimer = 0.0f;
    timeoutTimer = 0.0f;
    currentLevel = 0;
    currentRepeat = 0;
    isFinish = true;
}

void zHintSphere::SceneExit() {
    soundSource.SceneExit();
}

void zHintSphere::DebugReset() {
    asset = World::GetEntityManager()->FindAsset(asset->id);
    linkArray = (LinkAsset*)&asset->EventLinksNew;

    Reset();
}
