// zNPCUPGeneric -- read from the image with tools/disasm.py.
//
// WHERE EACH VPTR IS, measured per class rather than assumed:
//
//   f98's own object       lwz r12,0(r3)     vptr at 0
//   f98's object at +188   lwz r12,192(r4)   vptr at 4 of a thing at 188
//   f94, fA8, the extras   lwz r12,4(r3)     vptr at 4
//
// A vtable entry at byte N is the (N-8)/4th virtual, so 48 is slot 10,
// 32 is slot 6, 184 is slot 44. The classes below carry exactly enough
// slots to put the ones that are called where the image has them, and
// four bytes of padding in front where the vptr is at 4.
//
// The two event ids are `addis r0,r6,SIMM ; cmplwi r0,K`, which is a
// 32-bit compare done in two halves: the value is K - (SIMM << 16).
// 0x657B - 0xA1870000 is 0x5E79657B, and 0xF747 - 0xA7E10000 is
// 0x581FF747.

class xBase;
class xEnt;
class xVec3;
class xAnimPlay;
class xQuat;
class xModelInstance;
class zNPCUPGeneric;
class zNPCStatus;

namespace Math { class Matrix43; }
namespace Sext { class EventAny; }
// 64 bytes: retail reaches the second with `addi r4,r6,64`.
namespace World {
class ModelInstanceAsset { public: unsigned char _pad[0x40]; };
}  // namespace World
namespace Sext { class NPCTemplate; }

void xEntGetCenterFromAABB(const xEnt* ent, xVec3& out);
xBase* zSceneFindObject(unsigned long long uid);

namespace World {

class EntityManager {
public:
    static Sext::NPCTemplate* FindAsset(unsigned long long uid);
};

EntityManager* GetEntityManager();

}  // namespace World

class xMat4x3 {
public:
    xMat4x3& operator=(const xMat4x3& rhs);
};

extern xMat4x3 g_I3;

class xVec3Fields {
public:
    float x;
    float y;
    float z;
};
// ---- the pieces zNPCUPGeneric reaches into --------------------------------

class zBoneTracker {
public:
    void Reset();
    void Update(float dt);
    void LookAtLocation(const xVec3* at, bool immediate);
    void InitBoneTracker(xModelInstance* model, int a, int b, int c);
    void SetLimits(float a, float b, float c, float d, float e, bool f);
    void ApplyBoneModificationAfterAnimatrices(xAnimPlay* play,
                                               Math::Matrix43* mtx,
                                               xQuat* q, xVec3* a,
                                               xVec3* b, int i);

    unsigned char _pad0[0x7C];
};

class zNPCExtraModel {
public:
    unsigned char _pad0[0x4];
    virtual void _v0();  virtual void _v1();  virtual void _v2();
    virtual void _v3();  virtual void _v4();  virtual void _v5();
    virtual void Render();

    void SetModel(const World::ModelInstanceAsset* asset, xMat4x3* mtx,
                  xVec3* scale);

    xMat4x3 matrix;
};

class zNPCUPComponent {
public:
    unsigned char _pad0[0x4];
    virtual void _v0();  virtual void _v1();  virtual void _v2();
    virtual void _v3();  virtual void _v4();  virtual void _v5();
    virtual void _v6();  virtual void _v7();  virtual void _v8();
    virtual void _v9();
    virtual void Update(float dt);
};

class zNPCUPSetup {
public:
    unsigned char _pad0[0x4];
    virtual void _v0();  virtual void _v1();  virtual void _v2();
    virtual void _v3();  virtual void _v4();  virtual void _v5();
    virtual void _v6();  virtual void _v7();  virtual void _v8();
    virtual void _v9();
    virtual void Apply(void* params);
};

class zNPCModelRenderer {
public:
    unsigned char _pad0[0x4];
    virtual void _v0();  virtual void _v1();  virtual void _v2();
    virtual void _v3();  virtual void _v4();  virtual void _v5();
    virtual void Render();
};

class zNPCModel {
public:
    virtual void _v0();  virtual void _v1();  virtual void _v2();
    virtual void _v3();  virtual void _v4();  virtual void _v5();
    virtual void _v6();  virtual void _v7();  virtual void _v8();
    virtual void _v9();  virtual void _v10(); virtual void _v11();
    virtual void _v12(); virtual void _v13(); virtual void _v14();
    virtual void _v15(); virtual void _v16(); virtual void _v17();
    virtual void _v18(); virtual void _v19(); virtual void _v20();
    virtual void _v21(); virtual void _v22(); virtual void _v23();
    virtual void _v24(); virtual void _v25(); virtual void _v26();
    virtual void _v27(); virtual void _v28(); virtual void _v29();
    virtual void _v30(); virtual void _v31(); virtual void _v32();
    virtual void _v33(); virtual void _v34(); virtual void _v35();
    virtual void _v36(); virtual void _v37(); virtual void _v38();
    virtual void _v39(); virtual void _v40(); virtual void _v41();
    virtual void _v42(); virtual void _v43();
    virtual void SetParams(void* params);

    unsigned char _pad0[0x30];
    xMat4x3* matrix;
    unsigned char _pad1[0x84];
    zNPCModelRenderer renderer;
};

class zNPCCombatParams {
public:
    void SetDefaults();

    unsigned char _pad0[0x28];
};

class zNPCTemplateFlags {
public:
    unsigned char _pad0[0x38];
    unsigned int flags;
    unsigned char _pad1[0xC];
    unsigned char headTracking;
};

class zNPCTemplate {
public:
    zNPCTemplateFlags* info;
    unsigned char _pad0[0xD0];
    int boneA;
    int boneB;
    int boneC;
    float limitA;
    float limitB;
    float limitC;
    float limitD;
    float limitE;
    bool limitFlag;
};

class zNPCModelInfo {
public:
    unsigned char _pad0[0x88];
    unsigned int modelCount;
    World::ModelInstanceAsset* models;
};

class zNPCAssetIDs {
public:
    unsigned char _pad0[0x138];
    unsigned int uidHigh;
    unsigned int uidLow;
};

class zNPCBase {
public:
    void PreUpdateAllComponents(float dt);
    void PostUpdateAllComponents(float dt);
    void SystemEvent(xBase* from, xBase* to, unsigned int id,
                     Sext::EventAny* event);
};

class zNPCGeneric {
public:
    void HeadTrackingSetFocusVec(xVec3* v, bool immediate);
};

// ---- the unit's own class -------------------------------------------------

class zNPCUPGeneric {
public:
    void InitTypeParameters();
    static void AfterAnimMatrices(zNPCBase* base, xAnimPlay* play,
                                  Math::Matrix43* mtx, xQuat* q, xVec3* a,
                                  xVec3* b, int i);
    bool Activate(const zNPCStatus* status);
    void Initialize(const zNPCStatus* status);
    void Update(float dt);
    void Render();
    void SystemEvent(xBase* from, xBase* to, unsigned int id,
                     Sext::EventAny* event);
    void HeadTrackingUpdate(float dt);
    bool GetPositionFromBase(xBase* base, xVec3& pos);

    unsigned char _pad0[0x60];
    zNPCAssetIDs* assetIDs;
    unsigned char _pad1[0x4];
    zNPCModelInfo* modelInfo;
    unsigned char _pad2[0x5];
    unsigned char stateFlags;
    unsigned char _pad3[0x6];
    zNPCTemplate* tmpl;
    unsigned char _pad4[0x18];
    zNPCUPComponent* component;
    zNPCModel* model;
    unsigned char _pad5[0xC];
    zNPCUPSetup* setup;
    unsigned char _pad6[0x8];
    zNPCExtraModel* extra[2];
    unsigned char _pad7[0x4];
    zBoneTracker boneTracker;
    unsigned char headTracking;
    unsigned char _pad8[0x3];
    int f140;
    xBase* posBase;
    xVec3Fields focus;
    unsigned char hasFocus;
    bool focusImmediate;
    unsigned char _pad9[0x2];
    xBase* focusEnt;
};


// --------------------------------------------------------------------------

void zNPCUPGeneric::AfterAnimMatrices(zNPCBase* base, xAnimPlay* play,
                                      Math::Matrix43* mtx, xQuat* q,
                                      xVec3* a, xVec3* b, int i) {
    ((zNPCUPGeneric*)base)->boneTracker
        .ApplyBoneModificationAfterAnimatrices(play, mtx, q, a, b, i);
}

bool zNPCUPGeneric::GetPositionFromBase(xBase* base, xVec3& pos) {
    // The `else` RETURNS, and the then arm jumps over it to reach the
    // `return true` that follows: [call][b +12][li 0][b +8][li 1].
    // Assigning into a variable and returning it once puts the `li 1`
    // inside the then arm instead, which is one word shorter.
    if (*(unsigned short*)((char*)base + 0x26) & 0x20) {
        xEntGetCenterFromAABB((const xEnt*)base, pos);
    } else {
        return false;
    }

    return true;
}

void zNPCUPGeneric::HeadTrackingUpdate(float dt) {
    boneTracker.Update(dt);

    xVec3Fields pos = focus;

    if (focusEnt) {
        hasFocus = 1;
        xEntGetCenterFromAABB((const xEnt*)focusEnt, *(xVec3*)&pos);
        ((zNPCGeneric*)this)->HeadTrackingSetFocusVec((xVec3*)&pos, false);
    }

    if (posBase) {
        GetPositionFromBase(posBase, *(xVec3*)&pos);
    }

    if (hasFocus) {
        boneTracker.LookAtLocation((const xVec3*)&pos, focusImmediate);
    }

    focusImmediate = 0;
}

void zNPCUPGeneric::Render() {
    if (model) {
        model->renderer.Render();
    }

    for (int i = 0; i < 2; i++) {
        if (extra[i]) {
            extra[i]->Render();
        }
    }
}

void zNPCUPGeneric::Update(float dt) {
    ((zNPCBase*)this)->PreUpdateAllComponents(dt);

    if (component) {
        component->Update(dt);
    }

    if (headTracking) {
        HeadTrackingUpdate(dt);
    }

    for (int i = 0; i < 2; i++) {
        if (extra[i]) {
            extra[i]->matrix = *model->matrix;
        }
    }

    ((zNPCBase*)this)->PostUpdateAllComponents(dt);
}

void zNPCUPGeneric::SystemEvent(xBase* from, xBase* to, unsigned int id,
                                Sext::EventAny* event) {
    switch (id) {
    case 0x5E79657B: {
        focusEnt = 0;

        xBase* found = 0;

        if (event) {
            if (*(unsigned long long*)event != 0) {
                found = zSceneFindObject(*(unsigned long long*)event);

                if (found == 0) {
                    World::GetEntityManager();
                    found = (xBase*)World::EntityManager::FindAsset(
                        *(unsigned long long*)event);
                }
            }
        }

        if (found && (*(unsigned short*)((char*)found + 0x26) & 0x20)) {
            focusEnt = found;
        }

        break;
    }

    case 0x581FF747:
        focusEnt = 0;
        hasFocus = 0;
        break;

    default:
        ((zNPCBase*)this)->SystemEvent(from, to, id, event);
        break;
    }
}

bool zNPCUPGeneric::Activate(const zNPCStatus*) {
    if (tmpl) {
        setup->Apply((char*)tmpl + 0xB0);

        unsigned char f = stateFlags;
        f = (unsigned char)((f & ~0x10) | ((tmpl->info->flags >> 4) & 0x10));
        stateFlags = f;
        f = (unsigned char)((f & ~0x08) | ((tmpl->info->flags << 1) & 0x08));
        stateFlags = f;
    } else {
        zNPCCombatParams params;
        params.SetDefaults();
        params.SetDefaults();
        setup->Apply(&params);
    }

    bool track = false;

    if (tmpl && tmpl->info->headTracking) {
        track = true;
    }

    headTracking = track;
    f140 = 0;
    posBase = 0;
    focusEnt = 0;

    boneTracker.Reset();

    if (headTracking) {
        boneTracker.InitBoneTracker((xModelInstance*)model->matrix,
                                    tmpl->boneA, tmpl->boneB, tmpl->boneC);
        boneTracker.SetLimits(tmpl->limitA, tmpl->limitB, tmpl->limitC,
                              tmpl->limitD, tmpl->limitE,
                              tmpl->limitFlag);
    }

    if (modelInfo->modelCount >= 1) {
        xVec3Fields scale;
        scale.x = 1.0f;
        scale.y = 1.0f;
        scale.z = 1.0f;
        extra[0]->SetModel(modelInfo->models, &g_I3, (xVec3*)&scale);
    }

    if (modelInfo->modelCount >= 2) {
        xVec3Fields scale;
        scale.x = 1.0f;
        scale.y = 1.0f;
        scale.z = 1.0f;
        extra[1]->SetModel(modelInfo->models + 1, &g_I3, (xVec3*)&scale);
    }

    return true;
}
