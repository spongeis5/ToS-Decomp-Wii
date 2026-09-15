#include "SB/GM/Engine/Game/zBreakawayPlatform.pool.h"

// zBreakawayPlatform.cpp -- the breakaway platform: a model that warns,
// breaks away, is gone for a while and reappears, optionally riding a
// move-point path and carrying what is driven by it. Read from the image
// with tools/brief.py; the layouts are the DWARF's, the virtual slots the
// image's (__vt__18zBreakawayPlatform).
//
// The unit was a tools/gen_accessors.py stub with three functions; they are
// kept, moved below their callers.
//
// Not written, sorted from the unit's listing:
// - Init, Setup, DebugReset, CreateAnimTable and RemoveAnimTable name symbols
//   in WAD01.cpp's anonymous namespace (the event wrapper, animTables), which
//   a fragment cannot name; the wrapper itself is one of them.
// - Move (8), UpdateWarn (6), UpdateDisappear (6), Mounted (5), SetupReappear
//   (5), UpdateReappear (5), SetupDisappear (4) and Update (4) load that many
//   distinct float literals: the four-literal wall.

class xAnimSingle;
class xAnimState;
class xAnimTable;
class xAnimTransition;
class xBase;
class xEnt;
class xGroup;
class xMovePoint;
class xSpline3;
class xUpdateCullMgr;
class zBreakawayPlatform;
class Event;
class Channel;
class FMOD_VECTOR;

namespace World {
class EntityHandleBase;
class xOGEntity;
}

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {

enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };

void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap, eMemMgrTag tag,
                      bool clear);

}  // namespace Memory

extern "C" {
void* memset(void* dst, int c, unsigned long n);
double cos(double x);
}

inline void* operator new(unsigned long, void* p) { return p; }
void operator delete(void* p);

enum ForceEvent {
    FE_YES = 0,
    FE_NO = 1
};

// ---------------------------------------------------------------------------
// Assets

namespace Sext {

class EventAny {};

class uid {
public:
    operator unsigned long long() const { return internalUid; }

    unsigned long long internalUid;
};

class xBaseAsset {
public:
    uid id;
    unsigned int baseType;
    unsigned short linkCount;
    unsigned short baseFlags;
};

class xBaseScene : public xBaseAsset {};

class LinkAsset {
public:
    unsigned int count;
    void* data;
};

class vec3 {
public:
    float x;
    float y;
    float z;
};

class EventActionNew : public EventAny {};

class EventActionDrivenBy : public EventActionNew {
public:
    bool param0;
    bool param1;
    bool param2;
    bool cam;
    uid specificPassenger;
    int bone;
};

class EventActionOneFloat : public EventActionNew {
public:
    float param0;
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

class MovePointMotion_Type {
public:
    uid FirstMovePoint;
    float Speed;
    unsigned char Flags;
};

class Shake_Type {
public:
    unsigned char Axis;
    float Duration;
};

class Flash_Type {
public:
    vec3 colorMult;
    float Frequency;
    float Duration;
};

class AxisDistanceDuration_Type {
public:
    unsigned char Axis;
    float Distance;
    float Duration;
};

class AxisDurationAngle_Type {
public:
    unsigned char Axis;
    float Duration;
    float Angle;
};

class Duration_Type {
public:
    float Duration;
};

class Programmatic_Type {
public:
    unsigned char Flags;
    unsigned char WarningType;
    union {
        Shake_Type Shake;
        Flash_Type Flash;
    };
    unsigned char BreakType;
    union {
        AxisDistanceDuration_Type Translate;
        Duration_Type Fall;
        Duration_Type Shrink;
        Duration_Type FadeOut;
        AxisDurationAngle_Type Rotate;
    };
    unsigned char AppearType;
    union {
        AxisDistanceDuration_Type TranslateBack;
        Duration_Type Expand;
        Duration_Type FadeIn;
        AxisDurationAngle_Type RotateBack;
    };
};

class zBreakawayPlatformAsset;

}  // namespace Sext

namespace World {
class Entity;
}

namespace Sext {

class zBreakawayPlatformAsset : public xBaseScene {
public:
    static World::Entity* Create(World::EntityHandleBase* handle,
                                 zBreakawayPlatformAsset* asset);

    LinkAsset EventLinksNew;
    vec3 Pos;
    vec3 Orientation;
    vec3 Scale;
    unsigned char _pad0[0x40 - 0x3C];
    unsigned char ModelInstance[0x40];
    uid Surface;
    uid BreakawayGroupID;
    unsigned char MotionType;
    MovePointMotion_Type MovePointMotion;
    unsigned int Pad1;
    unsigned char AnimationType;
    Programmatic_Type Programmatic;
    float BreakawayDelay;
    float ReappearDelay;
    SoundBankSource SoundWarning;
    SoundBankSource SoundBreak;
    SoundBankSource SoundAppear;
    bool FakeBuoyancy;
    bool ForceUpdateDistance;
    float ForceUpdateDistanceValue;
};

}  // namespace Sext

typedef char _size_zBreakawayPlatformAsset
    [(sizeof(Sext::zBreakawayPlatformAsset) == 0x120) ? 1 : -1];

// ---------------------------------------------------------------------------
// Vectors and matrices

class xVec3 {
public:
    xVec3& operator+=(const xVec3& other);
    xVec3& operator*=(float f);

    float x;
    float y;
    float z;
};

class xMat3x3 {
public:
    xVec3 right;
    int flags;
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

class xQuat {
public:
    xVec3 v;
    float s;
};

void xMat3x3Euler(xMat3x3* m, float yaw, float pitch, float roll);
void xMat3x3GetEuler(const xMat3x3* m, xVec3* a);
void xMat3x3LookVec(xMat3x3* m, const xVec3* at);
void xMat3x3Mul(xMat3x3* o, const xMat3x3* a, const xMat3x3* b);
void xMat3x3RMulVec(xVec3* o, const xMat3x3* m, const xVec3* v);
void xMat3x3RotY(xMat3x3* m, float angle);
void xMat4x3Invert(xMat4x3* o, const xMat4x3* m);
void xMat4x3Mul(xMat4x3* o, const xMat4x3* a, const xMat4x3* b);
void xQuatFromMat(xQuat* q, const xMat3x3* m);
void xVec3Inv(xVec3* o, const xVec3* v);
void v3normalize(float& len, xVec3* o, xVec3* v);

namespace Math {

class Vector4 {
public:
    Vector4& Assign(float x, float y, float z, float w);

    float v[4];
};

}  // namespace Math

// Math::Vector's constructor, which the linker folded xVec3's onto.
extern "C" void __ct__Q24Math6VectorFfff(void* v, float x, float y, float z);

// ---------------------------------------------------------------------------
// Models and animation

namespace Graphics {

class Renderable {
public:
    void AmendColorMultiplier(Math::Vector4 color);

    unsigned char _pad0[0x34];
    unsigned short colorMulHandle;
};

class Model {
public:
    unsigned char _pad0[0x34];
    unsigned short renderableCount;
    unsigned short renderCustomizerCount;
    Renderable** renderables;
};

}  // namespace Graphics

class xAnimPlay {
public:
    xAnimPlay* Next;
    unsigned short NumSingle;
    unsigned short BoneCount;
    unsigned short MorphCount;
    unsigned short padding0;
    xAnimSingle* Single;
    void* Object;
    xAnimTable* Table;
};

xAnimState* xAnimTableGetState(xAnimTable* table, const char* name);
void xAnimPlaySetState(xAnimSingle* single, xAnimState* state, float startTime);

namespace World {

class ModelInstanceArticle {
public:
    unsigned char _pad0[0x24];
    Graphics::Model model;
};

class xOGModel {
public:
    void Show();
    void Hide();

    // Weak in the image, emitted after its first caller (Reset).
    void SetColorMultiplier(float r, float g, float b, float a) {
        if (r < 0.0f || r > 1.0f) {
            r = colorMultiplier.v[0];
        }
        if (g < 0.0f || g > 1.0f) {
            g = colorMultiplier.v[1];
        }
        if (b < 0.0f || b > 1.0f) {
            b = colorMultiplier.v[2];
        }
        if (a < 0.0f || a > 1.0f) {
            a = colorMultiplier.v[3];
        }
        colorMultiplier.Assign(r, g, b, a);

        int i;
        Graphics::Model& model = mModelArt.model;

        for (i = 0; i < model.renderableCount; i++) {
            Graphics::Renderable* renderable = model.renderables[i];
            if (renderable->colorMulHandle != 0xFFFF) {
                renderable->AmendColorMultiplier(colorMultiplier);
            }
        }
    }

    xMat4x3 Mat;
    unsigned char _pad0[0x4C - 0x40];
    xAnimPlay* Anim;
    unsigned short Flags;
    unsigned short pad;
    unsigned int renderCustomizerMask;
    unsigned char _pad1[0xB4 - 0x58];
    Math::Vector4 colorMultiplier;
    ModelInstanceArticle mModelArt;
};

}  // namespace World

// ---------------------------------------------------------------------------
// The entity classes

namespace World {
class EntityManager {
public:
    static void* FindAsset(unsigned long long id);
};

EntityManager* GetEntityManager();
}  // namespace World

// The slots of the entity table this unit calls or fills
// (__vt__18zBreakawayPlatform).
class EntityVirtuals {
public:
    virtual ~EntityVirtuals();
    virtual void _v1();
    virtual void _v2();
    virtual void _v3();
    virtual void _v4();
    virtual void _v5();
    virtual void _v6();
    virtual void _v7();
    virtual void _v8();
    virtual void DriveAttach(World::xOGEntity* passenger, unsigned int flags,
                             int bone);
    virtual void DriveDetach();
    virtual void DriveOn();
    virtual void DriveOff();
    virtual void _v13();
    virtual void _v14();
    virtual void _v15();
    virtual void DriveCauseMove(xMat4x3* parent, xMat4x3* local,
                                xMat4x3* world, xMat4x3* oldMat, bool rotate,
                                bool yaw, float givenYaw, float dt);
    virtual void _v17();
    virtual void _v18();
    virtual void _v19();
    virtual void _v20();
    virtual void _v21();
    virtual void _v22();
    virtual const FMOD_VECTOR* GetSoundPosition() const;
    virtual void _v24();
    virtual void Init(Sext::zBreakawayPlatformAsset* asset);
    virtual void SceneExit();
    virtual void Setup();
    virtual void DebugReset();
    virtual void Reset(bool inPlace);
    virtual void Update(float dt);
    virtual void HandleEvent(xBase* from, unsigned int toEvent,
                             Sext::EventAny* params);
    virtual bool IsAnimated();
    virtual bool IsStatic();
    virtual bool ShouldMove();
};

class EmbeddedListNode {
public:
    EmbeddedListNode* next;
    EmbeddedListNode* prev;
};

typedef void (*xBaseEventCB)(xBase* from, xBase* to, unsigned int toEvent,
                             Sext::EventAny* params);

// Packed to four: the id's eight-byte alignment would otherwise round xBase
// up past the model handle the DWARF puts at +0x34.
#pragma pack(push, 4)

namespace World {

class Entity : public EntityVirtuals {
public:
    EmbeddedListNode ogSceneNode;
    int ogUpdateIdx;
    unsigned int typeID;
    EntityHandleBase* handle;
};

}  // namespace World

class xBase : public World::Entity {
public:
    unsigned long long id;
    unsigned int baseType;
    unsigned char UNUSED_linkCount;
    unsigned char assertFlags;
    unsigned short baseFlags;
    Sext::LinkAsset* linkArray;
    void* templateParent;
    xBaseEventCB eventFunc;
};

namespace World {

class xOGModelRef {
public:
    void SetModel(xOGModel* m) { data = m; }

    xOGModel* data;
    void* autoptr;
};

class xOGModelHandle : public xOGModelRef {};

class xOGEntity : public xBase {
public:
    xOGEntity(EntityHandleBase* handle);
    ~xOGEntity();

    xOGModelHandle ogModel;
};

}  // namespace World

#pragma pack(pop)

class xEffectAttachIntf : public World::xOGEntity {
public:
    xEffectAttachIntf(World::EntityHandleBase* handle);
    ~xEffectAttachIntf();
};

class xEnt : public xEffectAttachIntf {
public:
    ~xEnt();

    unsigned char _pad0[0x4E - 0x3C];
    unsigned char collType;
    unsigned char chkby;
    unsigned char penby;
    unsigned char _pad1[0xBC - 0x51];
};

class zCollidable : public xEnt {
public:
    zCollidable(World::EntityHandleBase* handle);
    ~zCollidable();

    void ResetMat(const xMat4x3& mat);

    xBase* owner;
};

typedef char _size_zCollidable[(sizeof(zCollidable) == 0xC0) ? 1 : -1];

void xBaseReset(xBase* base, Sext::xBaseAsset* asset);
void zEntEvent(xBase* from, unsigned int fromEvent, xBase* to,
               unsigned int toEvent, Sext::EventAny* params, ForceEvent force);
xBase* zSceneFindObject(unsigned long long id);

class xMovePoint : public World::xOGEntity {
public:
    float GetRandomChildren(xMovePoint** dest, const xMovePoint* prev,
                            xVec3* hdng) const;

    void* asset;
    xVec3* pos;
    unsigned int numChildren;
    xMovePoint** children;
    xMovePoint** parents;
    unsigned int numParents;
    unsigned int oldParents;
    unsigned int nodeWeightSum;
    xSpline3* spl;
    unsigned char on;
    unsigned char bezIndex;
};

class xSpline3 {
public:
    unsigned short type;
    unsigned short flags;
    unsigned int N;
    unsigned int allocN;
    xVec3* points;
    float* time;
    xVec3* p12;
    xVec3* bctrl;
    float* knot;
    void* coef;
    unsigned int arcSample;
    float* arcLength;
};

class MovePointData {
public:
    xMovePoint* dest;
    xMovePoint* src;
    float speed;
    float dest_speed;
    float dist;
    float curdist;
    float acceleration;
    float t;
    float speedStart;
    float speedLimit;
    float speedEnd;
    float deceleration;
    float curdistBack;
    float curdistStopAccelerate;
    float curdistStartDecelerate;
    float threshold;
    bool startedDecelerate;
    bool startedAccelerate;
    void* asset;
    World::xOGEntity* owner;
};

class xEntMPData {
public:
    MovePointData speed_data;
    xSpline3* spl;
    xQuat aquat;
    xQuat bquat;
};

class zNeoDrivenLink {
public:
    static void AddChild(World::xOGEntity* driver, World::xOGEntity* passenger,
                         xMat4x3* mat, unsigned int flags, float yaw,
                         int bone);
};

// ---------------------------------------------------------------------------
// Scene, globals, update culling

class EmbeddedList {
public:
    EmbeddedListNode head;
    unsigned long size;
};

class BaseInfo {
public:
    EmbeddedList entz;
};

class zScene {
public:
    unsigned char _pad0[0x28];
    BaseInfo baseInfo[264];
};

class xGlobals {
public:
    unsigned char _pad0[0x304];
    xUpdateCullMgr* updateMgr;
    unsigned char _pad1[0x43C - 0x308];
    zScene* sceneCur;
};

class zGlobals : public xGlobals {};

extern zGlobals globals;

class FloatAndVoid {
public:
    union {
        float f;
        void* v;
    };
};

void xUpdateCull_SetCB(xUpdateCullMgr* m, void* entity,
                       unsigned int (*cb)(void*, void*), void* cbdata);
unsigned int xUpdateCull_AlwaysTrueCB(void* ent, void* cbdata);
unsigned int xUpdateCull_DistanceSquaredCB(void* ent, void* cbdata);

// ---------------------------------------------------------------------------
// Sound

// The vtable pointer lands at +0x3C, where the virtual is declared.
class zSoundAsset {
public:
    zSoundAsset();

    void Play(int a, int b, const char* c);
    void SetPosition(const FMOD_VECTOR* pos);

    char* soundSourceName;
    int soundSourceIdx;
    int indicesCount;
    int* indicesTree;
    int level;
    Event* event;
    Channel* channel;
    unsigned int _pad0;
    unsigned long long idLoopingSoundOwner;
    FMOD_VECTOR* pos;
    void* userCallback;
    void* userCallbackData;
    char rootGroupName[6];
    bool streamed;
    bool stopOnAnimEnd;

    virtual void Init(const char* name, int* indices, bool streamed);

    float max3Ddistance;
};

typedef char _size_zSoundAsset[(sizeof(zSoundAsset) == 0x48) ? 1 : -1];

void zSoundAsset_Unregister(zSoundAsset* asset);

// ---------------------------------------------------------------------------
// The platform

class zBreakawayPlatform : public xEffectAttachIntf {
public:
    zBreakawayPlatform(World::EntityHandleBase* handle)
        : xEffectAttachIntf(handle), collEnt(handle) {}
    ~zBreakawayPlatform();

    static unsigned int anWarnDoneCB(xAnimTransition* tran, xAnimSingle* anim,
                                     void* object);
    static unsigned int anDisappearDoneCB(xAnimTransition* tran,
                                          xAnimSingle* anim, void* object);
    static unsigned int anAppearDoneCB(xAnimTransition* tran,
                                       xAnimSingle* anim, void* object);

    // First among the virtuals, so the table (and the inline virtuals it
    // names) is emitted with it.
    virtual void SceneExit();
    void SetMovePoint(xMovePoint* mp);
    virtual void Init(Sext::zBreakawayPlatformAsset* asset);
    virtual void Setup();
    virtual bool IsAnimated() { return asset->AnimationType == 0; }
    virtual bool IsStatic() { return asset->MotionType == 0; }
    virtual void DebugReset();
    virtual void Reset(bool inPlace);
    virtual bool ShouldMove();
    bool CanMove() {
        bool ret = false;
        if (!IsStatic() && mpData.speed_data.dest != 0 &&
            mpData.speed_data.src != mpData.speed_data.dest && !stopped) {
            ret = true;
        }
        return ret;
    }
    bool IsIntact() {
        return breakawayState == 0 || breakawayState == 1;
    }
    void Move(float dt);
    void Mounted(xEnt* ent);
    void ResetCommonVars();
    void SetVisible(bool vis);
    void SetCollision(bool on);
    void SetAxis(unsigned char axis, xVec3& dir, bool negative);
    void SetupNormal();
    void SetupWarn();
    void SetupDisappear();
    void SetupGone();
    void SetupReappear();
    void UpdateNormal(float dt);
    void UpdateWarn(float dt);
    void UpdateDisappear(float dt);
    void UpdateGone(float dt);
    void UpdateReappear(float dt);
    void ChangeState(unsigned int newState);
    virtual void Update(float dt);
    virtual void HandleEvent(xBase* from, unsigned int toEvent,
                             Sext::EventAny* params);
    virtual void DriveAttach(World::xOGEntity* passenger, unsigned int flags,
                             int bone);
    xMat4x3* DriveGetCurMat(int bone);
    virtual void DriveCauseMove(xMat4x3* parent, xMat4x3* local,
                                xMat4x3* world, xMat4x3* oldMat, bool rotate,
                                bool yaw, float givenYaw, float dt);
    virtual const FMOD_VECTOR* GetSoundPosition() const;

    Sext::zBreakawayPlatformAsset* asset;
    zCollidable collEnt;
    xGroup* breakawayGroup;
    xMovePoint* firstMP;
    float moveSpeed;
    xEntMPData mpData;
    xEnt* driver;
    xVec3 refPointLocal;
    xVec3 prevRefPointWorld;
    xQuat prevDrvRot;
    bool drivenbyOn;
    bool mounted;
    float delay;
    bool stopped;
    xMat4x3 mat;
    bool visible;
    int visDelay;
    unsigned int breakawayState;
    xEnt* mounter;
    bool breakable;
    float timer;
    float speed;
    float accel;
    float dispDist;
    xVec3 dispDir;
    float angle;
    float angularVel;
    xVec3 rotateAxis;
    float angularParam;
    float angularParamSpeed;
    bool flip;
    bool buoyancy;
    float scale;
    zSoundAsset soundWarning;
    zSoundAsset soundBreak;
    zSoundAsset soundAppear;
    bool hasSoundWarning;
    bool hasSoundBreak;
    bool hasSoundAppear;
    unsigned char _pad0[0x330 - 0x323];
};

typedef char _size_zBreakawayPlatform
    [(sizeof(zBreakawayPlatform) == 0x330) ? 1 : -1];

// ---------------------------------------------------------------------------

void zBreakawayPlatform::SetMovePoint(xMovePoint* mp) {
    if (mp != 0) {
        xVec3 heading;

        mpData.speed_data.src = mp;
        mpData.speed_data.dist =
            mp->GetRandomChildren(&mpData.speed_data.dest, 0, &heading);
        mpData.speed_data.curdist = 0.0f;

        __ct__Q24Math6VectorFfff(&mat.pos, mp->pos->x, mp->pos->y,
                                 mp->pos->z);

        if (asset->MovePointMotion.Flags & 1) {
            xQuatFromMat(&mpData.aquat, &mat);

            xVec3 tempdir;
            xMat3x3 tempmat;

            xVec3Inv(&tempdir, &heading);
            xMat3x3LookVec(&tempmat, &tempdir);
            xQuatFromMat(&mpData.bquat, &tempmat);
        }

        if (mpData.speed_data.dest != 0 && mpData.speed_data.dest->spl != 0) {
            mpData.spl = mpData.speed_data.dest->spl;

            while (mpData.speed_data.dest->bezIndex != 0) {
                mpData.speed_data.dest = mpData.speed_data.dest->children[0];
            }

            mpData.speed_data.dist =
                mpData.spl->arcLength[mpData.spl->N * mpData.spl->arcSample - 1];
        }
    }
}

void zBreakawayPlatform::SceneExit() {
    if (hasSoundWarning) {
        zSoundAsset_Unregister(&soundWarning);
    }
    if (hasSoundBreak) {
        zSoundAsset_Unregister(&soundBreak);
    }
    if (hasSoundAppear) {
        zSoundAsset_Unregister(&soundAppear);
    }
}

void zBreakawayPlatform::Reset(bool inPlace) {
    xBaseReset(this, asset);

    if (!inPlace) {
        xMat3x3Euler(&mat, asset->Orientation.x, asset->Orientation.y,
                     asset->Orientation.z);
        __ct__Q24Math6VectorFfff(&mat.pos, asset->Pos.x, asset->Pos.y,
                                 asset->Pos.z);
        mat.right *= asset->Scale.x;
        mat.up *= asset->Scale.y;
        mat.at *= asset->Scale.z;

        collEnt.ResetMat(mat);

        if (ogModel.data != 0) {
            ogModel.data->Mat = mat;
        }

        if (firstMP != 0) {
            SetMovePoint(firstMP);
        }

        stopped = (asset->MovePointMotion.Flags & 4) != 0;
    }

    moveSpeed = asset->MovePointMotion.Speed;

    if (IsAnimated()) {
        World::xOGModel* model = ogModel.data;
        if (model != 0 && model->Anim != 0) {
            xAnimPlaySetState(model->Anim->Single,
                              xAnimTableGetState(model->Anim->Table,
                                                 "BreakawayIdle"),
                              0.0f);
        }
    }

    SetCollision(true);
    SetVisible(true);
    ResetCommonVars();
    breakawayState = 0;
    ogModel.data->SetColorMultiplier(1.0f, 1.0f, 1.0f, 1.0f);
    mounted = false;
    breakable = true;

    if (globals.updateMgr != 0 && asset->ForceUpdateDistance) {
        if (asset->ForceUpdateDistanceValue <= 0.0f) {
            xUpdateCull_SetCB(globals.updateMgr, this,
                              xUpdateCull_AlwaysTrueCB, 0);
        } else {
            FloatAndVoid dist;
            dist.f = asset->ForceUpdateDistanceValue *
                     asset->ForceUpdateDistanceValue;
            xUpdateCull_SetCB(globals.updateMgr, this,
                              xUpdateCull_DistanceSquaredCB, dist.v);
        }
    }
}

// Two inlined tests, each kept as a flag: CanMove's result assigned in an
// if (r31), IsIntact's `||` built as a value (r3), the answer in r30.
#pragma push
#pragma always_inline on
bool zBreakawayPlatform::ShouldMove() {
    bool ret = false;
    if (CanMove()) {
        if (IsIntact()) {
            ret = true;
        }
    }
    return ret;
}
#pragma pop

// Weak in the image with no caller here but Move: defined out of line.
void xVec3Lerp(xVec3* o, const xVec3* a, const xVec3* b, float t) {
    o->x = a->x * (1.0f - t) + b->x * t;
    o->y = a->y * (1.0f - t) + b->y * t;
    o->z = a->z * (1.0f - t) + b->z * t;
}

void zBreakawayPlatform::ResetCommonVars() {
    timer = 0.0f;
    speed = 0.0f;
    accel = 0.0f;
    dispDist = 0.0f;
    dispDir.x = 0.0f;
    dispDir.y = 0.0f;
    dispDir.z = 0.0f;
    angle = 0.0f;
    angularVel = 0.0f;
    rotateAxis.x = 0.0f;
    rotateAxis.y = 0.0f;
    rotateAxis.z = 0.0f;
    angularParam = 0.0f;
    angularParamSpeed = 0.0f;
    flip = false;
    buoyancy = false;
    scale = 1.0f;
}

void zBreakawayPlatform::SetVisible(bool vis) {
    if (visible != vis) {
        if (vis) {
            visDelay = 2;
        } else {
            visDelay = 0;
            ogModel.data->Hide();
        }
        visible = vis;
    }
}

void zBreakawayPlatform::SetAxis(unsigned char axis, xVec3& dir,
                                 bool negative) {
    float a = negative ? -1.0f : 1.0f;
    if (axis == 0 || axis == 3) {
        dir.x = a;
        dir.y = 0.0f;
        dir.z = 0.0f;
    } else if (axis == 1 || axis == 4) {
        dir.y = a;
        dir.x = 0.0f;
        dir.z = 0.0f;
    } else {
        dir.z = a;
        dir.x = 0.0f;
        dir.y = 0.0f;
    }
    if (axis <= 2) {
        xMat3x3RMulVec(&dir, &mat, &dir);
    }
}

void zBreakawayPlatform::SetupNormal() {
    Reset((asset->MovePointMotion.Flags & 8) != 0);
}

void zBreakawayPlatform::SetupWarn() {
    if (IsAnimated()) {
        World::xOGModel* model = ogModel.data;
        if (model->Anim != 0) {
            xAnimPlaySetState(model->Anim->Single,
                              xAnimTableGetState(model->Anim->Table,
                                                 "BreakawayWarn"),
                              0.0f);
        }
    } else {
        switch (asset->Programmatic.WarningType) {
        case 1:
            timer = asset->Programmatic.Shake.Duration;
            speed = 4.8f;
            dispDist = 0.0f;
            SetAxis(asset->Programmatic.Shake.Axis, dispDir, flip);
            break;
        case 2:
            timer = asset->Programmatic.Flash.Duration;
            angularParam = 0.0f;
            angularParamSpeed = 6.2831855f * asset->Programmatic.Flash.Frequency;
            break;
        }
    }
}

void zBreakawayPlatform::SetupGone() {
    timer = asset->ReappearDelay;
    if (!(asset->Programmatic.Flags & 1)) {
        SetVisible(false);
    }
}

void zBreakawayPlatform::UpdateNormal(float dt) {
    if (buoyancy) {
        speed -= accel * dt;
        dispDist += speed * dt;
        if (speed < 0.0f && dispDist <= 0.0f) {
            ResetCommonVars();
            timer = asset->BreakawayDelay;
        }
    } else if (mounted && breakable) {
        timer -= dt;
        if (timer <= 0.0f) {
            ChangeState(1);
        }
    }
}

// Weak in the image with no caller here: defined out of line.
float xcos(float x) { return cos(x); }

void zBreakawayPlatform::UpdateGone(float dt) {
    if (asset->Programmatic.AppearType != 4) {
        timer -= dt;
        if (timer <= 0.0f) {
            ChangeState(4);
        }
    }
}

void zBreakawayPlatform::ChangeState(unsigned int newState) {
    switch (newState) {
    case 0:
        ResetCommonVars();
        SetupNormal();
        break;
    case 1:
        ResetCommonVars();
        SetupWarn();
        if (hasSoundWarning) {
            soundWarning.Play(0, -1, 0);
            soundWarning.SetPosition(GetSoundPosition());
        }
        break;
    case 2:
        ResetCommonVars();
        SetupDisappear();
        if (hasSoundBreak) {
            soundBreak.Play(0, -1, 0);
            soundBreak.SetPosition(GetSoundPosition());
        }
        break;
    case 3:
        SetupGone();
        break;
    case 4:
        ResetCommonVars();
        SetupReappear();
        if (hasSoundAppear) {
            soundAppear.Play(0, -1, 0);
            soundAppear.SetPosition(GetSoundPosition());
        }
        break;
    }
    breakawayState = newState;
}

void zBreakawayPlatform::HandleEvent(xBase* from, unsigned int toEvent,
                                     Sext::EventAny* genericParams) {
    switch (toEvent) {
    case 0x3FE52B13: {
        Sext::EventActionDrivenBy* params =
            (Sext::EventActionDrivenBy*)genericParams;
        unsigned int flags = 0;

        if (params != 0) {
            if (params->param0) {
                flags |= 1;
            }
            if (params->param1) {
                flags |= 2;
            }
            if (params->param2) {
                flags |= 4;
            }
        }

        World::xOGEntity* possibleDriver = (World::xOGEntity*)from;

        if (params != 0 && params->specificPassenger != 0) {
            possibleDriver =
                (World::xOGEntity*)zSceneFindObject(params->specificPassenger);

            if (possibleDriver == 0) {
                possibleDriver =
                    (World::xOGEntity*)World::GetEntityManager()->FindAsset(
                        params->specificPassenger);
            }

            DriveAttach(possibleDriver, flags, -1);
        } else {
            DriveAttach(possibleDriver, flags, -1);
        }
        break;
    }

    case 0x3954A566:
        DriveOn();
        break;

    case 0x56509F60:
        DriveOff();
        break;

    case 0xF9090A3B:
        DriveDetach();
        break;

    case 0x524C336D:
        Mounted((xEnt*)from);
        break;

    case 0x389E01C0:
        DebugReset();
        break;

    case 0xA8B93047:
        Reset(false);
        break;

    case 0x0015A4AF:
        stopped = false;
        break;

    case 0x0B3550F2:
        stopped = true;
        break;

    case 0x34716A29:
        SetVisible(true);
    case 0x4B1B1469:
        SetCollision(true);
        break;

    case 0xD6094F29:
        SetVisible(false);
    case 0x6EDB6DE9:
        SetCollision(false);
        break;

    case 0x6A2568C4: {
        Sext::EventActionOneFloat* params =
            (Sext::EventActionOneFloat*)genericParams;

        if (globals.updateMgr != 0) {
            if (params->param0 <= 0.0f) {
                xUpdateCull_SetCB(globals.updateMgr, this,
                                  xUpdateCull_AlwaysTrueCB, 0);
            } else {
                FloatAndVoid dist;
                dist.f = params->param0 * params->param0;
                xUpdateCull_SetCB(globals.updateMgr, this,
                                  xUpdateCull_DistanceSquaredCB, dist.v);
            }
        }
        break;
    }

    case 0xAD46987D:
        moveSpeed = ((Sext::EventActionOneFloat*)genericParams)->param0;
        break;

    case 0xC3334861:
        moveSpeed += ((Sext::EventActionOneFloat*)genericParams)->param0;
        break;

    case 0xEAEF9159:
        moveSpeed -= ((Sext::EventActionOneFloat*)genericParams)->param0;
        break;

    case 0x2CE0E3F8:
        ogModel.data->renderCustomizerMask &= ~0x18;
        break;

    case 0x911FB962:
        ogModel.data->renderCustomizerMask |= 0x19;
        break;

    case 0x14A66082:
        ogModel.data->renderCustomizerMask &= ~0x19;
        break;

    case 0x5CA18840:
        breakable = true;
        if (mounter != 0) {
            mounted = true;
        }
        break;

    case 0xDDA2DA3F:
        breakable = false;
        break;
    }
}

void zBreakawayPlatform::SetCollision(bool on) {
    if (on) {
        collEnt.chkby = 0x18;
        collEnt.penby = 0x18;
    } else {
        collEnt.chkby = 0;
        collEnt.penby = 0;
        if (mounter != 0) {
            zEntEvent(0, 0, (xBase*)mounter, 0x1C261D83, 0, FE_NO);
            mounter = 0;
        }
    }
}

// Weak in the image, referenced only by CreateAnimTable: defined out of line.
unsigned int zBreakawayPlatform::anWarnDoneCB(xAnimTransition* tran,
                                              xAnimSingle* anim,
                                              void* object) {
    ((zBreakawayPlatform*)object)->ChangeState(2);
    return 0;
}

unsigned int zBreakawayPlatform::anDisappearDoneCB(xAnimTransition* tran,
                                                   xAnimSingle* anim,
                                                   void* object) {
    ((zBreakawayPlatform*)object)->SetupGone();
    ((zBreakawayPlatform*)object)->breakawayState = 3;
    return 0;
}

unsigned int zBreakawayPlatform::anAppearDoneCB(xAnimTransition* tran,
                                                xAnimSingle* anim,
                                                void* object) {
    ((zBreakawayPlatform*)object)->ResetCommonVars();
    ((zBreakawayPlatform*)object)->SetupNormal();
    ((zBreakawayPlatform*)object)->breakawayState = 0;
    return 0;
}

#pragma push
#pragma always_inline on
World::Entity* Sext::zBreakawayPlatformAsset::Create(
    World::EntityHandleBase* handle, zBreakawayPlatformAsset* asset) {
    zBreakawayPlatform* entity =
        new (memset(Memory::AllocGlobalHeap(sizeof(zBreakawayPlatform),
                                            (Memory::GlobalHeapEnum)0,
                                            (eMemMgrTag)16, false),
                    0, sizeof(zBreakawayPlatform))) zBreakawayPlatform(handle);

    entity->Init(asset);

    return entity;
}
#pragma pop

// Weak in the image with no caller here: defined out of line.
zCollidable::~zCollidable() { ogModel.SetModel(0); }

// Weak in the image, called out of line by Create: defined below it.
inline zSoundAsset::zSoundAsset()
    : soundSourceName(0), soundSourceIdx(0), indicesCount(0), indicesTree(0),
      level(0), event(0), channel(0), idLoopingSoundOwner(0), pos(0),
      userCallback(0), userCallbackData(0), streamed(false),
      stopOnAnimEnd(true) {
    rootGroupName[0] = 0;
}

void zBreakawayPlatform::DriveAttach(World::xOGEntity* passenger,
                                     unsigned int flags, int bone) {
    xMat4x3 D;
    xMat4x3Invert(&D, &mat);

    xVec3 euler;
    xMat3x3 a_descaled;
    float dummy;

    v3normalize(dummy, &a_descaled.right, &mat.right);
    v3normalize(dummy, &a_descaled.up, &mat.up);
    v3normalize(dummy, &a_descaled.at, &mat.at);
    xMat3x3GetEuler(&a_descaled, &euler);

    zNeoDrivenLink::AddChild(this, passenger, &D, flags, euler.x, -1);
}

void zBreakawayPlatform::DriveCauseMove(xMat4x3* parent, xMat4x3* local,
                                        xMat4x3* world, xMat4x3* oldMat,
                                        bool rotate, bool yaw, float givenYaw,
                                        float dt) {
    xMat4x3 baseMat;
    xMat4x3Mul(&baseMat, local, parent);

    xVec3 newEuler;
    if (yaw) {
        xMat3x3 a_descaled;
        float dummy;

        v3normalize(dummy, &a_descaled.right, &parent->right);
        v3normalize(dummy, &a_descaled.up, &parent->up);
        v3normalize(dummy, &a_descaled.at, &parent->at);
        xMat3x3GetEuler(&a_descaled, &newEuler);
    }

    if (!rotate || yaw) {
        xMat3x3 rot;
        if (yaw) {
            float yawDiff = newEuler.x - givenYaw;
            xMat3x3RotY(&rot, yawDiff);
        }
        mat = *oldMat;
        xMat3x3RMulVec(&mat.pos, &baseMat, &mat.pos);
        mat.pos += baseMat.pos;
        if (yaw) {
            xMat3x3Mul(&mat, &mat, &rot);
        }
    } else {
        // Twice, as retail calls it.
        xMat4x3Mul(&mat, oldMat, &baseMat);
        xMat4x3Mul(&mat, oldMat, &baseMat);
    }
}

void zBreakawayPlatformSceneExit() {
    EmbeddedListNode* end = &globals.sceneCur->baseInfo[0x6E].entz.head;

    for (EmbeddedListNode* node = end->next; node != end; node = node->next) {
        ((zBreakawayPlatform*)((char*)node - 4))->SceneExit();
    }
}

// The three functions tools/gen_accessors.py wrote, below their callers.
#pragma dont_inline on
xEffectAttachIntf::xEffectAttachIntf(World::EntityHandleBase* handle)
    : World::xOGEntity(handle) {}
xMat4x3* zBreakawayPlatform::DriveGetCurMat(int bone) { return &mat; }
const FMOD_VECTOR* zBreakawayPlatform::GetSoundPosition() const {
    return (const FMOD_VECTOR*)&mat.pos;
}
#pragma dont_inline off
