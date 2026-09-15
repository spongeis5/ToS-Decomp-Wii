#include "SB/GM/Engine/WAD02_4.pool.h"

// WAD02_4 -- the effect spawners (FX::zFXSpawn and the script spawn-point
// pool that hands them out) and, after them, the particle systems. Read
// from the image with tools/disasm.py; layouts are the DWARF's.
//
// The asset Create(s) below are one shape 33 Sext assets in this tree
// share: take sizeof(T) from the global heap (heap 0, tag 16, no clear),
// memset it, and place the entity on it with the handle and the asset.
// Each entity's constructor is a CALL, so it is declared and not defined,
// and each class is padded to the size its allocation asks for.
//
// tools/twin_census.py is what paired them with the written ones.

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {
enum GlobalHeapEnum { GlobalHeap = 0, GlobalHeapEnum_ = 0x7FFFFFFF };

void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap, eMemMgrTag tag,
                      bool clear);
void FreeGlobalHeap(void* block, GlobalHeapEnum heap);
}  // namespace Memory

extern "C" {
void* memset(void* dst, int c, unsigned long n);
unsigned long strlen(const char* s);
}

inline void* operator new(unsigned long, void* p) { return p; }
namespace World { class EntityHandleBase; }

void operator delete(void* mem);

void* xMemAlloc(Memory::GlobalHeapEnum heap, unsigned int size, int align,
                eMemMgrTag tag);
unsigned int xrand_GenRandInt32();

// A heap handed over as const H&: the constant binds to an unnamed
// four-byte temporary, and the allocator's argument is loaded from it.
template <class T, class H>
inline T* NewArray(const H& heap, unsigned long count, eMemMgrTag tag) {
    return (T*)Memory::AllocGlobalHeap(count * sizeof(T), heap, tag, false);
}

template <class H>
inline void Free(const H& heap, void* p) {
    H h = heap;

    if (p != 0) {
        Memory::FreeGlobalHeap(p, h);
    }
}

// The spawn slot's head is a World::EntityHandleBase. Its destructor is
// the one the linker folded onto hkBaseObject's (the only name at that
// address, and so the one every call names), while its constructor kept
// its own name; one class cannot carry both, so the slot holds a
// four-byte hkBaseObject whose inline constructor calls the handle's
// constructor by symbol.
extern "C" void __ct__Q25World16EntityHandleBaseFv(void* self);

class hkBaseObject {
public:
    hkBaseObject() { __ct__Q25World16EntityHandleBaseFv(this); }
    ~hkBaseObject();

    unsigned char __head[4];
};

class xVec3 {
public:
    xVec3& operator=(const xVec3& other);
    xVec3& operator=(float value);
    xVec3& operator+=(const xVec3& other);

    float x;
    float y;
    float z;
};

class xMat3x3 {
public:
    xVec3 right;
    unsigned int flags;
    xVec3 up;
    unsigned int pad1;
    xVec3 at;
    unsigned int pad2;
};

class xMat4x3 : public xMat3x3 {
public:
    xMat4x3& operator=(const xMat4x3& other);

    xVec3 pos;
    unsigned int pad3;
};

class xLightData {
public:
    unsigned char _pad0[0x10];
    xVec3 offset;
    unsigned char _pad1[0x68 - 0x1C];
    xVec3 position;
};

class zDecalAsset {
public:
    unsigned char _pad0[0x10];
    float lifetime;
};

class zDecal {
public:
    void emit(const xMat4x3& mat, const xMat4x3* parent, bool force);

    unsigned char _pad0[0x94];
    zDecalAsset* asset;
};

class xEffectAttachIntf;

xVec3 operator-(const xVec3& a, const xVec3& b);

extern xMat4x3 g_I3;

class xBaseTransform {
public:
    xMat4x3 transform;
    xVec3* dPos;
    float dPosScale;
};

void xLightOn(xLightData* light, bool on, float fade);
class xLightAsset;
xLightData* xLightCreateTempInternal(xLightAsset* asset, xVec3* pos);
void xLightSetPos(xLightData* light, const xVec3& pos);
void xLightDestroyTempInternal(xLightData* light);

float xrand_GenRandFloat();

xVec3 operator+(const xVec3& a, const xVec3& b);
void xMat3x3Euler(xMat3x3* m, const xVec3* ypr);
void xMat3x3Copy(xMat3x3* dst, const xMat3x3* src);
void xMat3x3Mul(xMat3x3* out, const xMat3x3* a, const xMat3x3* b);
void xMat4x3Mul(xMat4x3* out, const xMat4x3* a, const xMat4x3* b);
void xMat3x3RMulVec(xVec3* out, const xMat3x3* m, const xVec3* v);
void v3add(xVec3* out, xVec3* a, xVec3* b);

// A point carried into the world by a transform.
inline void xMat4x3Toworld(xVec3* out, const xMat4x3* m, const xVec3* v) {
    xVec3 rotated;

    xMat3x3RMulVec(&rotated, m, v);
    v3add(out, &rotated, (xVec3*)&m->pos);
}

class FMOD_VECTOR;
void* MEM_RTU_REALLOC_STATIC(void* p, unsigned long size);

class FXScreenWarp {
public:
    void Kill();
    void Update(xVec3& pos, bool force);

    unsigned char _pad0[0x60];
};

namespace Sext {

class uid {
public:
    unsigned long long internalUid;
};

struct vec3 {
    float x;
    float y;
    float z;
};

struct Rotation3 {
    float yaw;
    float pitch;
    float roll;
};

// The asset structs are not plain data: each has a constructor, so an
// assignment of one is the compiler's out-of-line operator=.
class CullingData {
public:
    CullingData() {}

    float cullDistance;
    int cullPriority;
};

class FXTiming {
public:
    FXTiming() {}

    float playTime;
    float cycleTime;
    bool loop;
    float chance;
};

class FXAttachEntity {
public:
    FXAttachEntity() {}

    uid entity;
    float velScale;
    short bone;
    vec3 Offset;
    Rotation3 Rotation;
    bool volumeScale;
};

class xBaseAsset {
public:
    uid id;
    unsigned int baseType;
    unsigned short linkCount;
    unsigned short baseFlags;
};

class xBaseScene : public xBaseAsset {};

// One entry of an effect's script, 72 bytes: what to run, where on the
// spawn, and when -- start and stop times with their random variances.
class ScriptEvent {
public:
    uid resource;
    vec3 offset;
    Rotation3 rotation;
    bool noRotation;
    float startTime;
    float stopTime;
    float startVariance;
    float stopVariance;
    float chance;
    bool soundFlag;
    const char* soundName;
    int* soundParam;
};

class ScriptEventList {
public:
    unsigned int count;
    ScriptEvent* data;
};

class EventAny;
class FXSpawn;

}  // namespace Sext

// The engine's base object as this unit reaches it: World::Entity's vtable
// pointer and fields fill the first 0x18 bytes.
class xBase {
public:
    unsigned char _entity[0x18];
    unsigned long long id;
    unsigned int baseType;
    unsigned char UNUSED_linkCount;
    unsigned char assertFlags;
    unsigned short baseFlags;
    void* linkArray;
    void* templateParent;
    void (*eventFunc)(xBase* from, xBase* to, unsigned int toEvent,
                      Sext::EventAny* args);
};

void xBaseInit(xBase* base, const Sext::xBaseAsset* asset);

extern "C" void __ct__Q24Math8Matrix33Fv(void* self);

// The sound asset's fields up to +0x3C; the vtable pointer its virtuals
// add lands there, as the image stores it, and max3Ddistance follows it.
class zSoundAssetData {
public:
    char* soundSourceName;
    int soundSourceIdx;
    int indicesCount;
    int* indicesTree;
    int level;
    void* event;
    void* channel;
    unsigned int _pad1C;
    // An unsigned long long in the game, spelled as two words so the class
    // stays four-aligned and the vtable pointer lands at +0x3C.
    unsigned int idLoopingSoundOwnerHi;
    unsigned int idLoopingSoundOwnerLo;
    void* pos;
    void* userCallback;
    void* userCallbackData;
    char rootGroupName[6];
    bool streamed;
    bool stopOnAnimEnd;
};

class zSoundAsset : public zSoundAssetData {
public:
    zSoundAsset();

    virtual void Init(const char* name, int* param, bool flag);

    bool IsLoopingEvent();

    float max3Ddistance;
};

void zSoundAsset_Unregister(zSoundAsset* asset);

class PoolListData {
public:
    unsigned char _pad0[0x20];
};

// The instance list's constructor is an empty one the linker folded onto
// Math::Matrix33's, so it is called by that name.
class zSoundAssetMultiple : public zSoundAsset {
public:
    zSoundAssetMultiple() { __ct__Q24Math8Matrix33Fv(&InstanceList); }

    virtual void Init(const char* name, int* param, bool flag);

    void GetEvent(unsigned long long owner);
    void Stop(unsigned long long owner);
    void StopAll();
    void Play(unsigned long long owner, const FMOD_VECTOR* pos,
              unsigned int flags);

    int InstanceListSize;
    PoolListData InstanceList;
};

namespace World {

class xOGModel {
public:
    void Show();
    void Hide();
    void UpdateRender();

    xMat4x3 Mat;
};

// The base carries nothing but its constructor: the vptr zFXSpawn's
// own virtual creates sits at +0 either way, so how sizeof splits
// between the two changes no code.
class xOGEntity {
public:
    xOGEntity(EntityHandleBase* handle);
    virtual ~xOGEntity();
};

class EntityHandleBase {
public:
    unsigned long long blobUID;
    unsigned char _pad0[0x34 - 0x8];
    unsigned int typeID;
    void* entity;
    int rc;
    short updateFlags;
    signed char sceneActivateRef;
    signed char pad;
};

class EntityManager {
public:
    static void* FindAsset(unsigned long long id);
    static EntityHandleBase* FindHandle(unsigned long long id);
};

EntityManager* GetEntityManager();

class xOGModelRefPtr;

class xOGModelRef {
public:
    xOGModel* data;
    xOGModelRefPtr* autoptr;
};

class RefData {
public:
    xOGModelRef* mParent;
    int mRefCount;
};

class xOGModelRefPtr {
public:
    // A safe bool: the pointer to member it yields names an empty member
    // the linker folded away, so that member is declared, never defined.
    typedef void (xOGModelRefPtr::*BoolType)() const;

    xOGModelRefPtr() : mData(0) {}
    ~xOGModelRefPtr();

    void decRef();
    bool IsSet() const;
    void BoolTrue() const;

    operator BoolType() const {
        return IsSet() == true ? &xOGModelRefPtr::BoolTrue : 0;
    }

    xOGModel* GetModel() const { return mData->mParent->data; }

    // Drops the reference and forgets it, in line.
    void Release() {
        decRef();
        mData = 0;
    }

    RefData* mData;
};

}  // namespace World

void xModelGetBoneMatScaled(xMat4x3& mat, const World::xOGModel& model,
                            unsigned long index);
void xModelGetBoneLocationNoScale(xVec3& loc, const World::xOGModel& model,
                                  unsigned long index);

// What an effect can be attached to: the model it follows (slot 20), the
// matrix (21) and the velocity (22).
class xEffectAttachIntf : public World::xOGEntity {
public:
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
    virtual void _v18();
    virtual void _v19();
    virtual World::xOGModelRef* GetAttachModel();
    virtual const xMat4x3* GetAttachMatrix();
    virtual xVec3* GetAttachDPos();
};

namespace FX {

class zFXSpawn;

namespace Particles {

class zFXParticleSystem;

namespace MotionSystems {
class Attach;
class Attractor;
class Vortex;
class Collision;
}  // namespace MotionSystems

class SystemInstance {
public:
    void SetRunning(bool running);
    void KillAll();
    void PreloadUpdate(float dt);
    void SetCulling(const Sext::CullingData& culling);

    zFXParticleSystem* mSystem;
    MotionSystems::Attach* mAttachSystem;
    int mNumAttractors;
    MotionSystems::Attractor* mAttractorSystems;
    int mNumVortices;
    MotionSystems::Vortex* mVortexSystems;
    int mNumCollision;
    MotionSystems::Collision* mCollisionSystems;
    xVec3 mLocalAccelDir;
    float mLocalAccelMagnitude;
    Sext::CullingData mUseCulling;
    xBaseTransform mBaseTransform;
    bool mIsRunning;
    bool mIsEmitting;
    bool mIsParentCulled;
    bool mIsRegistered;
    int mPoolIdx;
    xVec3 mVolumeScale;
    float mRateLifeTime;
    float mLifeTimeScale;
    float mEmitRateScale;
    float mEmitAccumulator;
};

}  // namespace Particles

namespace Objects {

class ObjectSystemAsset {
public:
    unsigned char _pad0[0x1D];
    bool burst;
};

class zFXObjectSystem {
public:
    unsigned char _pad0[0x40];
    ObjectSystemAsset* mAsset;
};

class SystemInstance {
public:
    void SetCulling(const Sext::CullingData& culling);
    void SetForcedAgeRate(float rate);
    void SetRunning(bool running);
    void KillAll();
    void PreloadUpdate(float dt);

    zFXObjectSystem* mSystem;
    Sext::CullingData mUseCulling;
    xBaseTransform mBaseTransform;
    xVec3 mVolumeScale;
    unsigned int mPoolIdx;
    float mEmitAccumulator;
    float mEmitRateScale;
    bool mIsRunning;
    bool mIsEmitting;
    bool mIsParentCulled;
    bool mIsRegistered;
};

}  // namespace Objects

namespace Ribbon {

class zRibbon;

class SystemInstance {
public:
    void SetRunning(bool running);
    bool IsRunning() const;
    void emit();

    zRibbon* mSystem;
    void* mRibbon;
    xVec3 mVelocity;
    xVec3 mOffset;
    xBaseTransform mBaseTransform;
};

}  // namespace Ribbon

namespace Shrapnel {

float zShrapnel_GetLifetime(unsigned long long id);

class SystemInstance {
public:
    void Launch(zFXSpawn* spawn);

    unsigned long long mAssetID;
    void* mAsset;
    xBaseTransform mBaseTransform;
};

}  // namespace Shrapnel

// The entity types a script event can run, as the asset names them.
enum {
    eFXDecal = 65,
    eFXModel = 96,
    eFXShrapnel = 228,
    eFXLight = 229,
    eFXRibbon = 251,
    eFXParticleSystem = 257,
    eFXObjectSystem = 258,
    eFXScreenWarp = 263,
};

class FXSoundMultipleList {
public:
    unsigned int index;
    unsigned int _pad4;
    zSoundAssetMultiple zsam;
};

enum eFXState {
    eNotStarted = 0,
    eRunning = 1,
    eFinishing = 2,
    eStopped = 3,
};

class VarianceValues {
public:
    int mStart;
    int mStop;
};

class ScriptResource {
public:
    int mType;
    zSoundAssetMultiple* mSoundAssetMultiple;
};

// The destructor destroys the model reference at +360 with the
// don't-delete flag and then the base at +0 with the flag clear;
// that second flag is the whole of what says base rather than
// member. The World::Entity, xBase and xOGEntity fields between the
// vptr and toFree are reached through those classes, not from here.
class zFXSpawn : public World::xOGEntity {
public:
    class _AssetData {
    public:
        _AssetData() {}

        xVec3 position;
        xVec3 orientation;
        Sext::CullingData Culling;
        Sext::FXTiming Timing;
        Sext::FXAttachEntity Entity;
        bool autoStart;
        bool volumeScale;
    };

    zFXSpawn(World::EntityHandleBase* a0);
    virtual void __vtable_anchor();
    ~zFXSpawn();

    void Construct(Sext::FXSpawn* asset);
    void ConstructPooled();
    void AllocateFXMem(unsigned int count);
    void FreeFXMem();
    void SetForcedAgeRate(float inAgeRate);
    void SetMat(const xMat4x3& mat);
    void Parse();
    void ResetMat();
    void UpdateLocalDPos();
    void TransformSetup(xBaseTransform* xform);
    void SetupLocal();
    void SetVolumeScale(const xVec3& scale);
    void MatrixUpdate(xMat4x3* result, const xMat4x3& local, bool noRotation);
    void HandleEvent(xBase* from, unsigned int toEvent, Sext::EventAny* args);
    void Run();
    void Preload(float inSeconds);
    void SetVisible(bool inActuallyVisible);
    void SetBSPVisible(bool inBSPVisible);
    void SetPaused(bool inPaused);
    void Kill();
    void ReleasePtrs();
    void Reset();
    void StopAllFX(bool inImmediate);
    void ResetStoppedFX();
    void StopFX(unsigned int inEventIdx, bool inImmediate);
    void Cleanup();
    void Update(float dt);
    bool IsSpawnReturnable() const;
    bool IsPaused() const { return (mFlags & 0x80) != 0; }
    bool IsBSPHidden() const { return (mFlags & 0x100) != 0; }
    void UpdateAttachedFX(unsigned int inEventIdx, bool inMove, bool inEmit);
    void CheckStartSound(unsigned int inEventIdx);
    void StartFX(unsigned int inEventIdx);
    bool IsFinished(unsigned int inEventIdx) const;
    bool IsFinished() const;

    unsigned char _xOGEntityData[0x3C - 0x4];
    bool toFree;
    bool isFree;
    int FXPoolIndex;
    _AssetData AssetData;
    unsigned int memAllocedSize;
    unsigned int mFlags;
    unsigned int mNumEvents;
    eFXState* mFxState;
    VarianceValues* mVarianceData;
    float* mStopTimes;
    int* mLookupList;
    ScriptResource* mResourceList;
    Particles::SystemInstance* mParticleSystems;
    Objects::SystemInstance* mObjectSystems;
    xLightData** mDynamicLights;
    zDecal** mDecals;
    Ribbon::SystemInstance* mRibbonSystems;
    Shrapnel::SystemInstance* mShrapnelSystems;
    World::xOGModel** mModels;
    FXScreenWarp* mScreenWarps;
    unsigned int mNumEmittersThatStop;
    Sext::FXSpawn* mAsset;
    xMat4x3 mBaseTransform;
    xVec3 mScaleBounds;
    xVec3 mLocalDPos;
    xVec3 mLocalLastPos;
    xVec3* mDriverPos;
    xVec3* mDriverDPos;
    xEffectAttachIntf* mDriver;
    World::xOGModelRefPtr mModel;
    int mAttachBone;
    float mTime;
    unsigned int mIterations;
};

}  // namespace FX

namespace FX {

class zFXSpawnWithSoundAssetMultiple : public FX::zFXSpawn {
public:
    virtual void __vtable_anchor();
    zFXSpawnWithSoundAssetMultiple(World::EntityHandleBase* a0);

    ~zFXSpawnWithSoundAssetMultiple();

    void ClearOnSceneExit();
    void Setup();
    void AllocSoundAssetMultiple();
    void Construct(Sext::FXSpawn* asset);

    FXSoundMultipleList* mSoundMultipleList;
    unsigned int mSoundMultipleCount;
};

}  // namespace FX

#pragma dont_inline on
FX::zFXSpawnWithSoundAssetMultiple::zFXSpawnWithSoundAssetMultiple(World::EntityHandleBase* a0) : FX::zFXSpawn(a0) {}
#pragma dont_inline off

namespace Sext {
class FXInstance;
class FXParticleSystem;
class FXSpawn;
}  // namespace Sext

namespace FX {

class zFXInstance : public zFXSpawnWithSoundAssetMultiple {
public:
    zFXInstance(World::EntityHandleBase* handle,
                Sext::FXInstance* asset);
    virtual void __vtable_anchor();

    Sext::FXInstance* mAsset;
};

class zFXSpawnObject : public zFXSpawnWithSoundAssetMultiple {
public:
    zFXSpawnObject(World::EntityHandleBase* handle,
                   Sext::FXSpawn* asset);
    virtual void __vtable_anchor();
};

namespace Particles {

class zFXParticleSystem {
public:
    zFXParticleSystem(World::EntityHandleBase* handle,
                      Sext::FXParticleSystem* asset);

    unsigned char _pad0[0x44];
    Sext::FXParticleSystem* mAsset;
    unsigned char _pad1[0xC0 - 0x48];
};

}  // namespace Particles

}  // namespace FX

namespace Sext {

class FXInstance : public xBaseScene {
public:
    static FX::zFXInstance* Create(World::EntityHandleBase* handle,
                                     FXInstance* asset);

    uid SpawnID;
    bool autoStart;
    vec3 position;
    Rotation3 orientation;
    CullingData Culling;
    FXTiming Timing;
    FXAttachEntity Entity;
};

class FXParticleSystem {
public:
    static FX::Particles::zFXParticleSystem* Create(World::EntityHandleBase* handle,
                                                      FXParticleSystem* asset);

    unsigned char _pad0[0x10];
    bool burst;
};

class FXSpawn : public xBaseScene {
public:
    static FX::zFXSpawnObject* Create(World::EntityHandleBase* handle,
                                        FXSpawn* asset);

    bool autoStart;
    vec3 position;
    Rotation3 orientation;
    CullingData Culling;
    FXTiming Timing;
    FXAttachEntity Entity;
    ScriptEventList ScriptEvents;
};

}  // namespace Sext

FX::zFXInstance* Sext::FXInstance::Create(World::EntityHandleBase* handle,
                                        FXInstance* asset) {
    return new (memset(Memory::AllocGlobalHeap(
                           sizeof(FX::zFXInstance), (Memory::GlobalHeapEnum)0,
                           (eMemMgrTag)16, false),
                       0, sizeof(FX::zFXInstance)))
        FX::zFXInstance(handle, asset);
}

FX::Particles::zFXParticleSystem* Sext::FXParticleSystem::Create(World::EntityHandleBase* handle,
                                                               FXParticleSystem* asset) {
    return new (memset(Memory::AllocGlobalHeap(
                           sizeof(FX::Particles::zFXParticleSystem), (Memory::GlobalHeapEnum)0,
                           (eMemMgrTag)16, false),
                       0, sizeof(FX::Particles::zFXParticleSystem)))
        FX::Particles::zFXParticleSystem(handle, asset);
}

FX::zFXSpawnObject* Sext::FXSpawn::Create(World::EntityHandleBase* handle,
                                        FXSpawn* asset) {
    return new (memset(Memory::AllocGlobalHeap(
                           sizeof(FX::zFXSpawnObject), (Memory::GlobalHeapEnum)0,
                           (eMemMgrTag)16, false),
                       0, sizeof(FX::zFXSpawnObject)))
        FX::zFXSpawnObject(handle, asset);
}

// ---------------------------------------------------------------------------
// NumberPool (xParticleUtilities.h): hands out the numbers 1..size, first
// fresh, then from a ring of released ones. Every call in the image is a
// call, never an inline copy, so the class is read with inlining off.

#pragma dont_inline on
class NumberPool {
public:
    NumberPool() : mTop(0), mBottom(0), mNextKey(0), mSize(0) {}

    void Clear() {
        Free(Memory::GlobalHeap, mAvailableNums);

        mNextKey = 0;
        mTop = 0;
        mBottom = 0;
        mSize = 0;
        mAvailableNums = 0;
    }

    // Defined below the manager's constructor, which calls it: read
    // before that, the auto-inliner takes it in despite the pragma.
    void SetSize(unsigned int size);

    unsigned int Get() {
        unsigned int ret;

        if (mNextKey < mSize) {
            ret = ++mNextKey;
        } else if (mBottom == mTop) {
            mNextKey++;

            return 0;
        } else {
            ret = mAvailableNums[mBottom++];

            if (mBottom >= mSize) {
                mBottom = 0;
            }
        }

        return ret;
    }

    void Release(unsigned int num) {
        if (mAvailableNums == 0) {
            return;
        }

        if (num == 0) {
            return;
        }

        mAvailableNums[mTop++] = num;

        if (mTop >= mSize) {
            mTop = 0;
        }

        if (mTop == mBottom) {
            mTop = mBottom = mNextKey = 0;
        }
    }

    unsigned int* mAvailableNums;
    unsigned int mTop;
    unsigned int mBottom;
    unsigned int mNextKey;
    unsigned int mSize;
};
#pragma dont_inline off

// ---------------------------------------------------------------------------
// zFXScriptSpawnPtMgr: 256 pooled spawns. A slot's number in the pool is
// its index plus one, so zero can mean "none left".

class zFXScriptSpawnPtMgr {
public:
    // The slot's trivial head at +0 and its spawn at +72 are destroyed in
    // reverse declaration order, as C++ specifies.
    class SpawnSlot {
    public:
        SpawnSlot() : spawnPt((World::EntityHandleBase*)&dummyHandle) {
            spawnPt.ConstructPooled();
        }

        ~SpawnSlot();

        hkBaseObject dummyHandle;
        unsigned char _pad0[0x48 - 0x4];
        FX::zFXSpawn spawnPt;
    };

    zFXScriptSpawnPtMgr();

    static void Setup();
    static FX::zFXSpawn* GetNewPoolSpawnPoint(const char* name);
    FX::zFXSpawn* _GetNewPoolSpawnPoint(const char* name);
    static void ReturnPoolSpawnPoint(FX::zFXSpawn* usedSpawnPt);
    void _ImmediateReturnPoolSpawnPoint(FX::zFXSpawn* usedSpawnPt);
    void _ImmediateReturnPoolSpawnPoint(FX::zFXSpawn* usedSpawnPt, int idx);
    void _ReturnPoolSpawnPoint(FX::zFXSpawn* usedSpawnPt, int idx);
    static void SceneExit();
    static void Reset();
    void _Reset();
    static void UpdateAll(float dt);
    void _UpdateAll(float dt);

    SpawnSlot slotsStorage[256];
    NumberPool mNumPool;
};

namespace SpawnPointMgr {
namespace Local {

extern bool sInvalidSceneState;
extern zFXScriptSpawnPtMgr* fxScriptSpawnPtMgr;

}  // namespace Local
}  // namespace SpawnPointMgr

FX::zFXSpawn::~zFXSpawn() {}
zFXScriptSpawnPtMgr::SpawnSlot::~SpawnSlot() {}

// The 80-byte base-only destructor, the compiler's own: the
// null-this test, the BASE's destructor on `this` with the flag
// CLEAR -- r4 = 0 is a base subobject where r4 = -1 is a complete
// one -- then operator delete when the CALLER's flag is positive,
// and `return this`. No member is destroyed.
FX::zFXSpawnWithSoundAssetMultiple::~zFXSpawnWithSoundAssetMultiple() {}

// The pointer is stored before the null test and the constructor: the
// allocation is assigned first and the manager placed on it after.
void zFXScriptSpawnPtMgr::Setup() {
    SpawnPointMgr::Local::fxScriptSpawnPtMgr = (zFXScriptSpawnPtMgr*)xMemAlloc(
        Memory::GlobalHeap, sizeof(zFXScriptSpawnPtMgr), 0, (eMemMgrTag)75);
    new (SpawnPointMgr::Local::fxScriptSpawnPtMgr) zFXScriptSpawnPtMgr;
}

zFXScriptSpawnPtMgr::zFXScriptSpawnPtMgr() {
    mNumPool.SetSize(256);

    for (int idx = 0; idx < 256; idx++) {
        SpawnSlot& spawnSlot = slotsStorage[idx];

        spawnSlot.spawnPt.toFree = false;
        spawnSlot.spawnPt.isFree = true;
    }

    SpawnPointMgr::Local::sInvalidSceneState = false;
}

inline void NumberPool::SetSize(unsigned int size) {
    mSize = size;
    mAvailableNums =
        NewArray<unsigned int>(Memory::GlobalHeap, size, (eMemMgrTag)27);
}

FX::zFXSpawn* zFXScriptSpawnPtMgr::GetNewPoolSpawnPoint(const char* name) {
    if (SpawnPointMgr::Local::sInvalidSceneState) {
        return 0;
    }

    return SpawnPointMgr::Local::fxScriptSpawnPtMgr->_GetNewPoolSpawnPoint(name);
}

// A number from the pool is a free slot. With none left, a slot whose
// spawn has been told to return (flag 8) is taken back at once, searching
// from a random start.
FX::zFXSpawn* zFXScriptSpawnPtMgr::_GetNewPoolSpawnPoint(const char* name) {
    int idx = mNumPool.Get();

    if (idx == 0) {
        idx = (int)(((xrand_GenRandInt32() & 0xFFFF) * 255) >> 16) + 1;
        int startidx = idx;

        do {
            if (idx == 256) {
                idx = 0;
            }

            SpawnSlot& spawnSlot = slotsStorage[idx];

            if (spawnSlot.spawnPt.mFlags & 8) {
                _ImmediateReturnPoolSpawnPoint(&spawnSlot.spawnPt, idx);
                goto found;
            }

            idx++;
        } while (idx != startidx);

        return 0;
    } else {
        idx--;
    }

found:
    SpawnSlot& spawnSlot = slotsStorage[idx];

    spawnSlot.spawnPt.isFree = false;
    spawnSlot.spawnPt.FXPoolIndex = idx;

    return &spawnSlot.spawnPt;
}

void zFXScriptSpawnPtMgr::ReturnPoolSpawnPoint(FX::zFXSpawn* usedSpawnPt) {
    if (!SpawnPointMgr::Local::sInvalidSceneState) {
        if (usedSpawnPt != 0) {
            usedSpawnPt->mFlags |= 8;
            usedSpawnPt->StopAllFX(false);
            usedSpawnPt->ReleasePtrs();
        }
    }
}

void zFXScriptSpawnPtMgr::_ImmediateReturnPoolSpawnPoint(
    FX::zFXSpawn* usedSpawnPt) {
    for (int idx = 0; idx < 256; idx++) {
        SpawnSlot& spawnSlot = slotsStorage[idx];

        if (&spawnSlot.spawnPt == usedSpawnPt) {
            if (!spawnSlot.spawnPt.isFree) {
                _ImmediateReturnPoolSpawnPoint(usedSpawnPt, idx);
            }

            return;
        }
    }
}

void zFXScriptSpawnPtMgr::_ImmediateReturnPoolSpawnPoint(
    FX::zFXSpawn* usedSpawnPt, int idx) {
    if (!usedSpawnPt->isFree) {
        usedSpawnPt->StopAllFX(true);
        usedSpawnPt->Cleanup();
        usedSpawnPt->isFree = true;
    }

    usedSpawnPt->toFree = false;
    mNumPool.Release(idx + 1);
}

void zFXScriptSpawnPtMgr::_ReturnPoolSpawnPoint(FX::zFXSpawn* usedSpawnPt,
                                                int idx) {
    usedSpawnPt->Cleanup();
    usedSpawnPt->isFree = true;
    usedSpawnPt->toFree = false;
    mNumPool.Release(idx + 1);
}

void zFXScriptSpawnPtMgr::SceneExit() {
    if (!SpawnPointMgr::Local::sInvalidSceneState) {
        SpawnPointMgr::Local::fxScriptSpawnPtMgr->_Reset();
    }

    zFXScriptSpawnPtMgr* mgr = SpawnPointMgr::Local::fxScriptSpawnPtMgr;

    if (!SpawnPointMgr::Local::sInvalidSceneState) {
        mgr->mNumPool.Clear();
    }

    SpawnPointMgr::Local::sInvalidSceneState = true;
}

void zFXScriptSpawnPtMgr::Reset() {
    if (!SpawnPointMgr::Local::sInvalidSceneState) {
        SpawnPointMgr::Local::fxScriptSpawnPtMgr->_Reset();
    }
}

void zFXScriptSpawnPtMgr::_Reset() {
    for (int idx = 0; idx < 256; idx++) {
        SpawnSlot& spawnSlot = slotsStorage[idx];

        if (!spawnSlot.spawnPt.isFree) {
            spawnSlot.spawnPt.StopAllFX(true);
            spawnSlot.spawnPt.Cleanup();
            spawnSlot.spawnPt.isFree = true;
            spawnSlot.spawnPt.toFree = false;
            mNumPool.Release(idx + 1);
        }

        spawnSlot.spawnPt.FreeFXMem();
    }
}

void zFXScriptSpawnPtMgr::UpdateAll(float dt) {
    if (!SpawnPointMgr::Local::sInvalidSceneState) {
        SpawnPointMgr::Local::fxScriptSpawnPtMgr->_UpdateAll(dt);
    }
}

// A spawn told to return goes back to the pool here, through the manager
// pointer rather than `this`.
void zFXScriptSpawnPtMgr::_UpdateAll(float dt) {
    for (int idx = 0; idx < 256; idx++) {
        SpawnSlot& spawnSlot = slotsStorage[idx];

        if (!spawnSlot.spawnPt.isFree) {
            if (spawnSlot.spawnPt.toFree) {
                SpawnPointMgr::Local::fxScriptSpawnPtMgr->_ReturnPoolSpawnPoint(
                    &spawnSlot.spawnPt, idx);
            } else {
                spawnSlot.spawnPt.Update(dt);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// FX::zFXSpawn
//
// mFlags, as the code reads it: 1 playing, 2 set up, 4 run, 8 return to
// the pool, 0x10 transform given, 0x20 pooled numbers held, 0x40 parsed,
// 0x80 paused, 0x100 hidden by the BSP, 0x200 cleaned up, 0x400 local
// velocity tracked, 0x800 stopping.

namespace FX {
namespace Local {

void FXSpawnEventWrapper(xBase* from, xBase* to, unsigned int toEvent,
                         Sext::EventAny* args) {
    ((zFXSpawn*)to)->HandleEvent(from, toEvent, args);
}

}  // namespace Local
}  // namespace FX

// The asset data's empty constructor is taken inline only when forced.
#pragma always_inline on
FX::zFXSpawn::zFXSpawn(World::EntityHandleBase* handle)
    : World::xOGEntity(handle),
      toFree(false),
      isFree(false),
      mFlags(0x200),
      mParticleSystems(0),
      mObjectSystems(0),
      mDynamicLights(0),
      mDecals(0),
      mRibbonSystems(0),
      mShrapnelSystems(0),
      mModels(0),
      mScreenWarps(0),
      mDriverPos(0),
      mDriverDPos(0),
      mDriver(0),
      mAttachBone(-1) {}
#pragma always_inline off

void FX::zFXSpawn::Construct(Sext::FXSpawn* asset) {
    mAsset = asset;
    xBaseInit((xBase*)this, asset);
    ((xBase*)this)->eventFunc = Local::FXSpawnEventWrapper;

    mBaseTransform = g_I3;

    mNumEvents = mAsset->ScriptEvents.count;
    AllocateFXMem(mNumEvents);
}

void FX::zFXSpawn::SetForcedAgeRate(float inAgeRate) {
    for (unsigned int idx = 0; idx < mNumEvents; idx++) {
        if (mResourceList[idx].mType == eFXObjectSystem) {
            mObjectSystems[mLookupList[idx]].SetForcedAgeRate(inAgeRate);
        }
    }
}

void FX::zFXSpawn::SetMat(const xMat4x3& mat) {
    mBaseTransform = mat;
    mFlags |= 0x10;

    for (unsigned int idx = 0; idx < mNumEvents; idx++) {
        UpdateAttachedFX(idx, true, true);
    }
}

namespace FX {
namespace Local {

// Grows a pool to count entries -- a fresh block when it has none -- or
// empties it.
template <class T>
void AllocMem(T*& ioMemPtr, unsigned int inCount) {
    if (inCount != 0) {
        ioMemPtr = (ioMemPtr == 0)
                       ? (T*)xMemAlloc(Memory::GlobalHeap, inCount * sizeof(T),
                                       0, (eMemMgrTag)75)
                       : (T*)MEM_RTU_REALLOC_STATIC(ioMemPtr,
                                                    inCount * sizeof(T));
    } else {
        ioMemPtr = 0;
    }
}

}  // namespace Local
}  // namespace FX

// Resolves each script event's resource to a type and an index among the
// resources of that type, then sizes each kind's pool to its count. Lights
// and decals share the pointer-array instance, the only one left after the
// linker folded the identical ones together.
void FX::zFXSpawn::Parse() {
    if (mFlags & 0x40) {
        return;
    }

    unsigned long long id;
    int numPS = 0;
    int numOS = 0;
    int numDL = 0;
    int numDC = 0;
    int numRS = 0;
    int numSS = 0;
    int numModels = 0;
    int numSW = 0;

    for (unsigned int idx = 0; idx < mNumEvents; idx++) {
        ScriptResource& res = mResourceList[idx];

        id = mAsset->ScriptEvents.data[idx].resource.internalUid;
        void* voidAsset = World::GetEntityManager()->FindAsset(id);
        World::EntityHandleBase* entHandle =
            World::EntityManager::FindHandle(id);

        if (id == 0) {
            res.mType = -1;
        } else if (voidAsset == 0) {
            res.mType = -2;
        } else if (entHandle == 0) {
            res.mType = -3;
        } else {
            res.mType = entHandle->typeID;
        }

        switch (res.mType) {
        case eFXParticleSystem:
            mLookupList[idx] = numPS++;
            break;
        case eFXObjectSystem:
            mLookupList[idx] = numOS++;
            break;
        case eFXLight:
            mLookupList[idx] = numDL++;
            break;
        case eFXDecal:
            mLookupList[idx] = numDC++;
            break;
        case eFXRibbon:
            mLookupList[idx] = numRS++;
            break;
        case eFXShrapnel: {
            mLookupList[idx] = numSS++;

            Sext::ScriptEvent& event = mAsset->ScriptEvents.data[idx];

            if (event.stopTime <= 0.0f) {
                event.stopTime = Shrapnel::zShrapnel_GetLifetime(
                    event.resource.internalUid);
            }
            break;
        }
        case eFXModel:
            mLookupList[idx] = numModels++;
            break;
        case eFXScreenWarp:
            mLookupList[idx] = numSW++;
            break;
        case -3:
            mLookupList[idx] = -1;
            break;
        case -2:
            mLookupList[idx] = -1;
            break;
        case -1:
            mLookupList[idx] = -1;
            break;
        default:
            mLookupList[idx] = -1;
            break;
        }
    }

    Local::AllocMem(mParticleSystems, numPS);
    Local::AllocMem(mObjectSystems, numOS);
    Local::AllocMem((World::xOGModel**&)mDynamicLights, numDL);
    Local::AllocMem((World::xOGModel**&)mDecals, numDC);
    Local::AllocMem(mRibbonSystems, numRS);
    Local::AllocMem(mShrapnelSystems, numSS);
    Local::AllocMem(mModels, numModels);
    Local::AllocMem(mScreenWarps, numSW);

    mNumEmittersThatStop = 0;

    for (unsigned int idx = 0; idx < mNumEvents; idx++) {
        if (mAsset->ScriptEvents.data[idx].stopTime > 0.0f) {
            mNumEmittersThatStop++;
        }
    }

    mFlags |= 0x40;
}

// Attached to anything -- a driver, a position, a model or an entity the
// asset names -- the base transform is the attachment's offset; otherwise
// it is the spawn's own place in the world.
void FX::zFXSpawn::ResetMat() {
    if (mDriver != 0 || mDriverPos != 0 || mModel ||
        AssetData.Entity.entity.internalUid != 0) {
        xMat3x3Euler(&mBaseTransform,
                     (const xVec3*)&AssetData.Entity.Rotation);
        mBaseTransform.pos = (const xVec3&)AssetData.Entity.Offset;
    } else {
        xMat3x3Euler(&mBaseTransform, &AssetData.orientation);
        mBaseTransform.pos = AssetData.position;
    }
}

void FX::zFXSpawn::UpdateLocalDPos() {
    if (mFlags & 0x400) {
        xMat4x3 result;

        MatrixUpdate(&result, g_I3, false);
        mLocalDPos = result.pos - mLocalLastPos;
        mLocalLastPos = result.pos;
    }
}

// A system follows the driver's velocity only when the asset scales it
// and there is a driver velocity to follow.
void FX::zFXSpawn::TransformSetup(xBaseTransform* xform) {
    if (AssetData.Entity.velScale > 0.001f && mDriverDPos != 0) {
        xform->dPosScale = AssetData.Entity.velScale;
        xform->dPos = mDriverDPos;
    } else {
        xform->dPosScale = 0.0f;
        xform->dPos = 0;
    }
}

void FX::zFXSpawn::SetVolumeScale(const xVec3& scale) { mScaleBounds = scale; }

// Carries a local transform into the world through whatever the spawn is
// attached to, the base offset turned with it, then either keeps the local
// orientation or applies the base's.
void FX::zFXSpawn::MatrixUpdate(xMat4x3* outMat, const xMat4x3& inMatTrans,
                                bool inWorldOrient) {
    if (mDriverPos != 0) {
        xMat3x3Copy(outMat, &g_I3);
        outMat->pos = *mDriverPos + inMatTrans.pos;
        outMat->pos += mBaseTransform.pos;
    } else if (mModel && mAttachBone > -1) {
        xMat4x3 bone;

        xModelGetBoneMatScaled(bone, *mModel.GetModel(), mAttachBone);
        xMat4x3Mul(outMat, &inMatTrans, &bone);

        xVec3 rotatedBasePos;

        xMat3x3RMulVec(&rotatedBasePos, &bone, &mBaseTransform.pos);
        outMat->pos += rotatedBasePos;
    } else if (mDriver != 0) {
        xMat4x3Mul(outMat, &inMatTrans, mDriver->GetAttachMatrix());

        xVec3 rotatedBasePos;

        xMat3x3RMulVec(&rotatedBasePos, mDriver->GetAttachMatrix(),
                       &mBaseTransform.pos);
        outMat->pos += rotatedBasePos;
    } else {
        *outMat = inMatTrans;
        outMat->pos += mBaseTransform.pos;
    }

    if (inWorldOrient) {
        xMat3x3Copy(outMat, &inMatTrans);
    } else {
        xMat3x3Mul(outMat, outMat, &mBaseTransform);
    }
}

void FX::zFXSpawn::HandleEvent(xBase* from, unsigned int toEvent,
                               Sext::EventAny* params) {
    switch (toEvent) {
    case 0x2DE3146B:
        if (mFlags & 1) {
            StopAllFX(true);
        }
        break;
    case 0xA8B93047: {
        xMat4x3 result;

        MatrixUpdate(&result, g_I3, false);
        mLocalLastPos = result.pos;
        mLocalDPos = 0.0f;

        if ((mFlags & 1) || AssetData.autoStart) {
            StopAllFX(true);
            Run();
        }
        break;
    }
    case 0x85151A66:
        SetPaused(true);
        break;
    case 0x26B1D67D:
        SetPaused(false);
        break;
    case 10427:
        if (!(mFlags & 1) || (mFlags & 0x800) == 0x800) {
            Run();
        }
        break;
    case 0x14D3DF:
        if (mFlags & 1) {
            StopAllFX(false);
        }
        break;
    case 0x0A20012A:
        if (mFlags & 1) {
            StopAllFX(true);
        }
    case 0xE6EA975F:
        if (mFlags & 1) {
            Preload(*(float*)params);
        }
        break;
    }
}

void FX::zFXSpawn::Run() {
    if (!(mFlags & 2)) {
        ResetMat();
        SetupLocal();
    }

    Reset();
    mFlags |= 4;
    mFlags &= ~0x800;

    if (xrand_GenRandFloat() <= AssetData.Timing.chance) {
        mFlags |= 1;
    }
}

// Runs the effect forward in thirtieths of a second, letting the particle
// and object systems that are playing catch up at each step.
void FX::zFXSpawn::Preload(float inSeconds) {
    for (float t = 0.0f; t < inSeconds; t += (1.0f / 30.0f)) {
        Update(1.0f / 30.0f);

        for (unsigned int idx = 0; idx < mNumEvents; idx++) {
            if (mFxState[idx] == eRunning || mFxState[idx] == eFinishing) {
                int typeIdx = mLookupList[idx];

                switch (mResourceList[idx].mType) {
                case eFXParticleSystem:
                    mParticleSystems[typeIdx].PreloadUpdate(1.0f / 30.0f);
                    break;
                case eFXObjectSystem:
                    mObjectSystems[typeIdx].PreloadUpdate(1.0f / 30.0f);
                    break;
                }
            }
        }
    }
}

void FX::zFXSpawn::SetVisible(bool inActuallyVisible) {
    if ((mFlags & 3) == 3) {
        for (unsigned int idx = 0; idx < mNumEvents; idx++) {
            int typeIdx = mLookupList[idx];

            switch (mResourceList[idx].mType) {
            case eFXParticleSystem:
                mParticleSystems[typeIdx].SetRunning(inActuallyVisible);
                break;
            case eFXObjectSystem:
                mObjectSystems[typeIdx].SetRunning(inActuallyVisible);
                break;
            case eFXLight:
                if (mDynamicLights[typeIdx] != 0) {
                    xLightOn(mDynamicLights[typeIdx], inActuallyVisible, 0.0f);
                }
                break;
            case eFXRibbon:
                mRibbonSystems[typeIdx].SetRunning(inActuallyVisible);
                break;
            case eFXModel:
                if (inActuallyVisible) {
                    mModels[typeIdx]->Show();
                } else {
                    mModels[typeIdx]->Hide();
                }
                break;
            }
        }
    }
}

// Hiding stops the sounds; showing starts again the ones that are due.
// Either way the systems follow unless the spawn is paused.
void FX::zFXSpawn::SetBSPVisible(bool inBSPVisible) {
    if (inBSPVisible == IsBSPHidden()) {
        if (!inBSPVisible) {
            mFlags |= 0x100;

            for (unsigned int idx = 0; idx < mNumEvents; idx++) {
                if (mResourceList[idx].mSoundAssetMultiple != 0) {
                    mResourceList[idx].mSoundAssetMultiple->Stop(
                        ((xBase*)this)->id);
                }
            }
        } else {
            mFlags &= ~0x100;

            for (unsigned int idx = 0; idx < mNumEvents; idx++) {
                CheckStartSound(idx);
            }
        }

        if (!(mFlags & 0x80)) {
            SetVisible(inBSPVisible);
        }
    }
}

void FX::zFXSpawn::SetPaused(bool inPaused) {
    if (inPaused == IsPaused()) {
        return;
    }

    if (inPaused) {
        mFlags |= 0x80;
    } else {
        mFlags &= ~0x80;
    }

    if (!(mFlags & 0x100)) {
        SetVisible(!inPaused);
    }
}

void FX::zFXSpawn::Kill() { StopAllFX(true); }

void FX::zFXSpawn::ReleasePtrs() {
    mDriver = 0;
    mDriverPos = 0;
    mDriverDPos = 0;
    mModel.Release();

    mFlags &= ~0x400;
    mLocalDPos = 0.0f;
}

void FX::zFXSpawn::Reset() {
    mIterations = 0;
    mTime = 0.0f;
    memset(mFxState, 0, mNumEvents * sizeof(eFXState));
}

void FX::zFXSpawn::StopAllFX(bool inImmediate) {
    if (!(mFlags & 0x200)) {
        for (unsigned int idx = 0; idx < mNumEvents; idx++) {
            switch (mFxState[idx]) {
            case eRunning:
            case eFinishing:
                StopFX(idx, inImmediate);
                break;
            case eNotStarted:
                mFxState[idx] = eStopped;
                break;
            case eStopped:
                break;
            default:
                mFxState[idx] = eStopped;
                break;
            }

            ScriptResource& res = mResourceList[idx];

            if (res.mSoundAssetMultiple != 0) {
                res.mSoundAssetMultiple->GetEvent(((xBase*)this)->id);

                if (res.mSoundAssetMultiple->IsLoopingEvent()) {
                    res.mSoundAssetMultiple->Stop(((xBase*)this)->id);
                }
            }
        }

        if (inImmediate) {
            mFlags &= ~1;
        } else {
            mFlags |= 0x800;
        }
    }
}

void FX::zFXSpawn::ResetStoppedFX() {
    for (unsigned int idx = 0; idx < mNumEvents; idx++) {
        switch (mFxState[idx]) {
        case eNotStarted:
            break;
        case eRunning:
        case eFinishing:
            if (IsFinished(idx)) {
                mFxState[idx] = eNotStarted;
            }
            break;
        case eStopped:
            mFxState[idx] = eNotStarted;
            break;
        default:
            mFxState[idx] = eStopped;
            break;
        }
    }
}

void FX::zFXSpawn::StopFX(unsigned int inEventIdx, bool inImmediate) {
    if (mFxState[inEventIdx] != eRunning &&
        mFxState[inEventIdx] != eFinishing) {
        return;
    }

    memset(&mVarianceData[inEventIdx], -1, sizeof(VarianceValues));

    int typeIdx = mLookupList[inEventIdx];

    switch (mResourceList[inEventIdx].mType) {
    case eFXParticleSystem:
        if (inImmediate) {
            mParticleSystems[typeIdx].KillAll();
        }
        mParticleSystems[typeIdx].SetRunning(false);
        break;
    case eFXObjectSystem:
        if (inImmediate) {
            mObjectSystems[typeIdx].KillAll();
        }
        mObjectSystems[typeIdx].SetRunning(false);
        break;
    case eFXLight: {
        xLightData* lObj = mDynamicLights[typeIdx];

        if (lObj != 0) {
            xLightDestroyTempInternal(lObj);
        }

        mDynamicLights[typeIdx] = 0;
        break;
    }
    case eFXRibbon:
        mRibbonSystems[typeIdx].SetRunning(false);
        break;
    case eFXModel:
        if (mModels[typeIdx] != 0) {
            mModels[typeIdx]->Hide();
        }
        break;
    case eFXScreenWarp:
        mScreenWarps[typeIdx].Kill();
        break;
    }

    mFxState[inEventIdx] = IsFinished(inEventIdx) ? eStopped : eFinishing;
}

void FX::zFXSpawnWithSoundAssetMultiple::ClearOnSceneExit() {
    StopAllFX(true);
    Cleanup();
    FreeFXMem();
    mFlags &= ~2;
    mFlags &= ~0x40;

    for (unsigned int i = 0; i < mSoundMultipleCount; i++) {
        if (mSoundMultipleList[i].zsam.soundSourceName != 0) {
            mSoundMultipleList[i].zsam.StopAll();
            zSoundAsset_Unregister(&mSoundMultipleList[i].zsam);
        }
    }

    Free(Memory::GlobalHeap, mSoundMultipleList);
    mSoundMultipleList = 0;

    mParticleSystems = 0;
    mObjectSystems = 0;
    mDynamicLights = 0;
    mDecals = 0;
    mRibbonSystems = 0;
    mShrapnelSystems = 0;
    mModels = 0;
}

// Steps each script event through its states -- waiting to start (with a
// chance of being skipped and random start and stop offsets), playing until
// its stop time or the cycle's end, finishing -- then decides whether the
// whole effect stops, or loops round with what is left of the frame.
// Everything it calls is called in the image, so nothing is taken inline.
//
// NEAR MISS: 340 of 347 words; the float-literal wall. Its six distinct
// literals (0, 1, 2, -1 and both conversion doubles) are loaded from one
// shared addis base in r31 where retail spells a lis for each, and every
// register after that is one off. The pool header does not move it.
#pragma dont_inline on
void FX::zFXSpawn::Update(float dt) {
    if ((mFlags & 3) != 3) {
        if (IsSpawnReturnable()) {
            toFree = true;
        }

        return;
    }

    if ((mFlags & 0x800) == 0x800 && IsFinished()) {
        mFlags &= ~0x800;
        mFlags &= ~1;

        if (IsSpawnReturnable()) {
            toFree = true;
        }

        return;
    }

    if ((mFlags & 0x80) || (mFlags & 0x100)) {
        return;
    }

    mTime += dt;
    UpdateLocalDPos();

    for (unsigned int idx = 0; idx < mNumEvents; idx++) {
        Sext::ScriptEvent& event = mAsset->ScriptEvents.data[idx];

        switch (mFxState[idx]) {
        case eNotStarted:
            if (mVarianceData[idx].mStart == -1) {
                if (xrand_GenRandFloat() > event.chance) {
                    mFxState[idx] = eStopped;
                    break;
                }

                mVarianceData[idx].mStart = (int)(
                    event.startVariance * (2.0f * xrand_GenRandFloat() - 1.0f));
                mVarianceData[idx].mStop = (int)(
                    event.stopVariance * (2.0f * xrand_GenRandFloat() - 1.0f));
            }

            if (mTime >= event.startTime + (float)mVarianceData[idx].mStart) {
                if (event.stopTime > 0.0f) {
                    mStopTimes[idx] =
                        event.stopTime +
                        (event.startTime + (float)mVarianceData[idx].mStart) +
                        (float)mVarianceData[idx].mStop;
                } else {
                    mStopTimes[idx] = -1.0f;
                }

                UpdateAttachedFX(idx, true, true);
                StartFX(idx);
            }
            break;
        case eRunning:
            UpdateAttachedFX(idx, false, true);

            if (mStopTimes[idx] > 0.0f
                    ? mStopTimes[idx] < mTime
                    : (AssetData.Timing.playTime >= 0.0f &&
                       AssetData.Timing.cycleTime > 0.0f &&
                       AssetData.Timing.cycleTime < mTime)) {
                StopFX(idx, false);
            }
            break;
        case eFinishing:
            UpdateAttachedFX(idx, false, true);

            if (IsFinished(idx)) {
                mFxState[idx] = eStopped;
            }
            break;
        case eStopped:
            break;
        default:
            mFxState[idx] = eRunning;
            StopFX(idx, true);
            break;
        }
    }

    if ((mFlags & 0x800) == 0x800) {
        return;
    }

    bool doStop = false;
    unsigned int stopped = 0;

    if (mNumEmittersThatStop != 0) {
        for (unsigned int idx = 0; idx < mNumEvents; idx++) {
            if (mFxState[idx] == eStopped) {
                stopped++;
            }
        }

        if (mNumEmittersThatStop == stopped && !AssetData.Timing.loop &&
            -1.0f == AssetData.Timing.playTime) {
            doStop = true;
        }
    }

    if (AssetData.Timing.playTime > 0.0f) {
        if (mTime + AssetData.Timing.cycleTime * mIterations >
            AssetData.Timing.playTime) {
            doStop = true;
        }
    } else if (0.0f == AssetData.Timing.playTime && !AssetData.Timing.loop) {
        if (AssetData.Timing.cycleTime <= 0.0f) {
            if (stopped != 0 && stopped == mNumEmittersThatStop) {
                doStop = true;
            }
        } else if (AssetData.Timing.cycleTime < mTime) {
            doStop = true;
        }
    }

    if (doStop) {
        StopAllFX(false);
        return;
    }

    if (AssetData.Timing.loop) {
        bool needsReset = false;
        float remaining = 0.0f;

        if (AssetData.Timing.cycleTime <= 0.0f && stopped != 0 &&
            stopped == mNumEmittersThatStop) {
            needsReset = true;
        } else if (AssetData.Timing.cycleTime > 0.0f &&
                   AssetData.Timing.cycleTime < mTime) {
            needsReset = true;
            remaining = mTime - AssetData.Timing.cycleTime;
        }

        if (needsReset) {
            mTime = 0.0f;
            mIterations++;
            ResetStoppedFX();
            memset(mVarianceData, -1, mNumEvents * sizeof(VarianceValues));

            if (remaining > 0.0f) {
                Update(remaining);
            }
        }
    }
}
#pragma dont_inline off

// Inline in zFXSpawn.h in the game, and called from Update there.
bool FX::zFXSpawn::IsSpawnReturnable() const {
    return (mFlags & 8) == 8 && (mFlags & 6) != 0;
}

// Places one event's system on the spawn: the event's offset and rotation
// carried through the base transform, then through the attachment. Nothing
// moves unless the spawn is attached, the update is forced, or the event is
// a screen warp.
void FX::zFXSpawn::UpdateAttachedFX(unsigned int inEventIdx,
                                    bool inForceUpdate, bool doEmit) {
    ScriptResource& res = mResourceList[inEventIdx];

    if (mDriver == 0 && !mModel && mDriverPos == 0 && !inForceUpdate &&
        res.mType != 263u) {
        return;
    }

    xMat4x3 matTrans;

    xMat3x3Euler(&matTrans,
                 (const xVec3*)&mAsset->ScriptEvents.data[inEventIdx].rotation);
    xMat3x3RMulVec(&matTrans.pos, &mBaseTransform,
                   (const xVec3*)&mAsset->ScriptEvents.data[inEventIdx].offset);

    bool worldOrient = mAsset->ScriptEvents.data[inEventIdx].noRotation;
    int spawnIdx = mLookupList[inEventIdx];

    switch (res.mType) {
    case eFXParticleSystem: {
        Particles::SystemInstance& spawnPt = mParticleSystems[spawnIdx];

        MatrixUpdate(&spawnPt.mBaseTransform.transform, matTrans, worldOrient);
        spawnPt.mIsParentCulled =
            mDriver != 0 && (((xBase*)mDriver)->baseFlags & 0x40) != 0;
        break;
    }
    case eFXObjectSystem: {
        Objects::SystemInstance& spawnPt = mObjectSystems[spawnIdx];

        MatrixUpdate(&spawnPt.mBaseTransform.transform, matTrans, worldOrient);
        spawnPt.mIsParentCulled =
            mDriver != 0 && (((xBase*)mDriver)->baseFlags & 0x40) != 0;
        break;
    }
    case eFXLight: {
        xLightData* lObj = mDynamicLights[spawnIdx];

        if (lObj != 0) {
            xMat4x3 transform = g_I3;

            MatrixUpdate(&transform, matTrans, worldOrient);

            xVec3 pos;

            xMat4x3Toworld(&pos, &transform, &lObj->offset);
            xLightSetPos(lObj, pos);
        }
        break;
    }
    case eFXDecal: {
        xMat4x3 emitLoc;

        MatrixUpdate(&emitLoc, matTrans, worldOrient);

        if (doEmit) {
            mDecals[spawnIdx]->emit(emitLoc, 0, false);
        }
        break;
    }
    case eFXRibbon:
        if (mFxState[inEventIdx] == eRunning) {
            Ribbon::SystemInstance& spawnPt = mRibbonSystems[spawnIdx];

            MatrixUpdate(&spawnPt.mBaseTransform.transform, matTrans,
                         worldOrient);

            if (doEmit) {
                spawnPt.emit();
            }
        }
        break;
    case eFXShrapnel:
        MatrixUpdate(&mShrapnelSystems[spawnIdx].mBaseTransform.transform,
                     matTrans, worldOrient);
        break;
    case eFXModel:
        if (mModels[spawnIdx] != 0) {
            MatrixUpdate(&mModels[spawnIdx]->Mat, matTrans, worldOrient);
            mModels[spawnIdx]->UpdateRender();
        }
        break;
    case eFXScreenWarp: {
        xMat4x3 emitLoc;

        MatrixUpdate(&emitLoc, matTrans, worldOrient);
        mScreenWarps[spawnIdx].Update(emitLoc.pos, false);
        break;
    }
    }
}

// The start test survives with nothing behind it: whatever it guarded is
// not in this build.
void FX::zFXSpawn::CheckStartSound(unsigned int inEventIdx) {
    if (mResourceList[inEventIdx].mSoundAssetMultiple != 0) {
        if (mTime < mAsset->ScriptEvents.data[inEventIdx].startTime +
                        (float)mVarianceData[inEventIdx].mStart) {
            return;
        }
    }
}

// Starts one event's system and, when its resource has a sound, plays the
// sound where the system is.
//
// NEAR MISS: 60 of 241 words; every instruction right, the callee-saved
// registers assigned differently (spawnIdx in r26 where retail has r28, this
// and inEventIdx one higher). Declaring spawnIdx before the first statement
// changed nothing.
void FX::zFXSpawn::StartFX(unsigned int inEventIdx) {
    ScriptResource& res = mResourceList[inEventIdx];
    FMOD_VECTOR* spawnPos = 0;
    xVec3 tmpPos;

    mFxState[inEventIdx] = eRunning;

    int spawnIdx = mLookupList[inEventIdx];

    switch (res.mType) {
    case eFXParticleSystem:
        mParticleSystems[spawnIdx].SetCulling(AssetData.Culling);
        mParticleSystems[spawnIdx].SetRunning(true);
        mParticleSystems[spawnIdx].mVolumeScale = mScaleBounds;

        if (mParticleSystems[spawnIdx].mSystem->mAsset->burst) {
            mFxState[inEventIdx] = eFinishing;
        }

        spawnPos = (FMOD_VECTOR*)&mParticleSystems[spawnIdx]
                       .mBaseTransform.transform.pos;
        break;
    case eFXObjectSystem:
        mObjectSystems[spawnIdx].SetCulling(AssetData.Culling);
        mObjectSystems[spawnIdx].SetRunning(true);
        mObjectSystems[spawnIdx].mVolumeScale = mScaleBounds;

        if (mObjectSystems[spawnIdx].mSystem->mAsset->burst) {
            mFxState[inEventIdx] = eFinishing;
        }

        spawnPos =
            (FMOD_VECTOR*)&mObjectSystems[spawnIdx].mBaseTransform.transform.pos;
        break;
    case eFXLight: {
        xVec3 lightPos;

        v3add(&lightPos, &mBaseTransform.pos,
              (xVec3*)&mAsset->ScriptEvents.data[inEventIdx].offset);

        unsigned long long lightID =
            mAsset->ScriptEvents.data[inEventIdx].resource.internalUid;
        void* voidAsset = World::GetEntityManager()->FindAsset(lightID);
        xLightData* lObj =
            xLightCreateTempInternal((xLightAsset*)voidAsset, &lightPos);

        mDynamicLights[spawnIdx] = lObj;

        if (lObj != 0) {
            UpdateAttachedFX(inEventIdx, true, true);
            xLightOn(lObj, true, 0.0f);
            v3add(&tmpPos, &lObj->position, &lObj->offset);
            spawnPos = (FMOD_VECTOR*)&tmpPos;
        }
        break;
    }
    case eFXDecal:
        if (mDecals[spawnIdx]->asset->lifetime <= 0.0f) {
            mFxState[inEventIdx] = eFinishing;
        }

        if (mAttachBone > -1 && mModel) {
            xModelGetBoneLocationNoScale(tmpPos, *mModel.GetModel(),
                                         mAttachBone);
            spawnPos = (FMOD_VECTOR*)&tmpPos;
        }
        break;
    case eFXRibbon:
        mRibbonSystems[spawnIdx].SetRunning(true);
        spawnPos =
            (FMOD_VECTOR*)&mRibbonSystems[spawnIdx].mBaseTransform.transform.pos;
        break;
    case eFXShrapnel:
        mShrapnelSystems[spawnIdx].Launch(this);
        spawnPos = (FMOD_VECTOR*)&mShrapnelSystems[spawnIdx]
                       .mBaseTransform.transform.pos;
        break;
    case eFXModel:
        mModels[spawnIdx]->Show();
        spawnPos = (FMOD_VECTOR*)&mModels[spawnIdx]->Mat.pos;
        break;
    case eFXScreenWarp:
        mScreenWarps[spawnIdx].Update(mBaseTransform.pos, false);
        break;
    }

    if (spawnPos != 0 && res.mSoundAssetMultiple != 0) {
        res.mSoundAssetMultiple->Play(((xBase*)this)->id, spawnPos,
                                      (unsigned int)-1);
    }
}

bool FX::zFXSpawn::IsFinished(unsigned int inEventIdx) const {
    unsigned int typeIdx = mLookupList[inEventIdx];

    switch (mResourceList[inEventIdx].mType) {
    case eFXParticleSystem:
        if (mParticleSystems[typeIdx].mIsRunning) {
            return false;
        }
        break;
    case eFXObjectSystem:
        if (mObjectSystems[typeIdx].mIsRunning) {
            return false;
        }
        break;
    case eFXLight:
        if (mDynamicLights[typeIdx] != 0) {
            return false;
        }
        break;
    case eFXRibbon:
        if (mRibbonSystems[typeIdx].IsRunning()) {
            return false;
        }
        break;
    }

    return true;
}

bool FX::zFXSpawn::IsFinished() const {
    for (unsigned int idx = 0; idx < mNumEvents; idx++) {
        if (!IsFinished(idx)) {
            return false;
        }
    }

    return true;
}

void FX::zFXSpawnWithSoundAssetMultiple::Setup() {
    if (mLookupList == 0) {
        AllocateFXMem(mNumEvents);
    }

    if (mSoundMultipleList == 0) {
        AllocSoundAssetMultiple();
    }

    if (AssetData.autoStart) {
        Run();
    }
}

// An array of sound lists, each element placed on the block in turn with
// its address tested as it goes. Not a template: a fragment instantiates
// templates only after everything above them is compiled, too late to be
// taken inline.
inline FX::FXSoundMultipleList* NewSoundMultipleLists(
    const Memory::GlobalHeapEnum& heap, unsigned long count, eMemMgrTag tag) {
    FX::FXSoundMultipleList* array =
        (FX::FXSoundMultipleList*)Memory::AllocGlobalHeap(
            count * sizeof(FX::FXSoundMultipleList), heap, tag, false);

    if (array != 0) {
        for (unsigned long i = 0; i < count; i++) {
            new (&array[i]) FX::FXSoundMultipleList;
        }
    }

    return array;
}

// One sound asset per script event that names a sound, in event order;
// every event's resource points at its sound, or at nothing. The array
// helper is taken inline only when forced.
//
// NEAR MISS: 42 of 97 words; every instruction right, the callee-saved
// registers one apart (this in r29 against retail's r30, the second loop's
// index in r30 against r31). Declaring j before the allocation made it 51.
#pragma always_inline on
void FX::zFXSpawnWithSoundAssetMultiple::AllocSoundAssetMultiple() {
    if (mSoundMultipleCount != 0) {
        mSoundMultipleList = NewSoundMultipleLists(
            Memory::GlobalHeap, mSoundMultipleCount, (eMemMgrTag)75);

        unsigned int j = 0;

        for (unsigned int i = 0; i < mNumEvents; i++) {
            mResourceList[i].mSoundAssetMultiple = 0;

            Sext::ScriptEvent& event = mAsset->ScriptEvents.data[i];
            const char* soundName = event.soundName;

            if (soundName != 0 && strlen(soundName) != 0) {
                mSoundMultipleList[j].zsam.Init(soundName, event.soundParam,
                                                event.soundFlag);
                mSoundMultipleList[j].index = i;
                mResourceList[i].mSoundAssetMultiple =
                    &mSoundMultipleList[j].zsam;
                j++;
            }
        }
    } else {
        mSoundMultipleList = 0;

        for (unsigned int i = 0; i < mNumEvents; i++) {
            mResourceList[i].mSoundAssetMultiple = 0;
        }
    }
}
#pragma always_inline off

void FX::zFXSpawnWithSoundAssetMultiple::Construct(Sext::FXSpawn* asset) {
    zFXSpawn::Construct(asset);
    mSoundMultipleCount = 0;

    for (unsigned int i = 0; i < mNumEvents; i++) {
        const char* soundName = mAsset->ScriptEvents.data[i].soundName;

        if (soundName != 0 && strlen(soundName) != 0) {
            mSoundMultipleCount++;
        }
    }

    AllocSoundAssetMultiple();
}

FX::zFXSpawnObject::zFXSpawnObject(World::EntityHandleBase* handle,
                                   Sext::FXSpawn* inAsset)
    : zFXSpawnWithSoundAssetMultiple(handle) {
    Construct(inAsset);

    AssetData.position = (const xVec3&)inAsset->position;
    AssetData.orientation = (const xVec3&)inAsset->orientation;
    AssetData.Culling = inAsset->Culling;
    AssetData.Timing = inAsset->Timing;
    AssetData.Entity = inAsset->Entity;
    AssetData.autoStart = inAsset->autoStart;
    AssetData.volumeScale = inAsset->Entity.volumeScale;
}

// An instance runs the spawn asset it names, placed and timed by its own.
FX::zFXInstance::zFXInstance(World::EntityHandleBase* handle,
                             Sext::FXInstance* inAsset)
    : zFXSpawnWithSoundAssetMultiple(handle), mAsset(inAsset) {
    unsigned long long spawnID = inAsset->SpawnID.internalUid;
    Sext::FXSpawn* spawnAsset =
        (Sext::FXSpawn*)World::GetEntityManager()->FindAsset(spawnID);

    Construct(spawnAsset);

    ((xBase*)this)->baseType = mAsset->baseType;
    AssetData.position = (const xVec3&)mAsset->position;
    AssetData.orientation = (const xVec3&)mAsset->orientation;
    AssetData.Culling = mAsset->Culling;
    AssetData.Timing = mAsset->Timing;
    AssetData.Entity = mAsset->Entity;
    AssetData.autoStart = mAsset->autoStart;
    AssetData.volumeScale = mAsset->Entity.volumeScale;
}
