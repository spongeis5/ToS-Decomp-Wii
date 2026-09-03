// zNPCExtraModel.cpp -- six functions, read from the image with
// tools/disasm.py. An extra model is an NPC component that renders one
// more model beside the NPC's own. Attached creates the render model
// from the instance asset the component was given, switches it onto the
// null updater so nothing animates it, shows it, and marks the component
// enabled and running. AllAttached fills in whichever of the initial
// matrix and the initial scale SetModel was not given -- from the NPC
// entity's own model matrix and model scale -- and then copies both
// initials into the current pair. Detached destroys the render model and
// clears every field. Reset copies the initials back over the current
// pair. SetModel keeps the instance asset and takes the matrix and the
// scale when they are given, recording for each whether it was. Render
// scales the current matrix into the model's, carries the position
// across and asks the model to update.
//
// Layouts from the DWARF (tools/dwarf_types.py): zNPCExtraModel is 0xAC
// on an 8-byte zNPCComponent, with curMatrix at +0x8, initialMatrix at
// +0x48, curScale at +0x88, initialScale at +0x94, modelRenderData at
// +0xA0, modelInstanceAsset at +0xA4 and the four bools matSet, scaleSet,
// enabled and paused at +0xA8..+0xAB. zNPCComponent is 8 bytes with the
// owner at +0 and nothing else described, which is the vtable pointer
// behind it -- so the owner is a plain base here and the virtuals go on
// the class above it, exactly as zNPCQuickTimeCombat.cpp has them.
// zNPCBase is 0xC0 with npcEntity at +0x98; zNPCEntity is 0x1D0 with the
// xOGEntity model handle at +0x34 (its xEnt base sits on xEffectAttachIntf
// on xOGEntity, whose ogModel is at +0x34) and modelScale at +0x124;
// World::xOGModel is 0x178 with its xModelInstance base's xMat4x3 Mat at
// +0, so a bare model pointer IS the address of its matrix, which is why
// the scale call passes the pointer unadjusted and the position copy
// passes it plus 0x30. xMat4x3 is 0x40 on xMat3x3 0x30 with pos at +0x30;
// xVec3 is 0xC.
//
// __vt__14zNPCExtraModel at 806C0168 is 0x30 bytes: two leading zero
// words, then Attached, Detached, Reset, AllAttached, two slots filled
// with the weak empty body every empty function folded into, Render,
// Paused, Resumed and one more empty. Paused and Resumed belong to this
// class and live in another unit (800D93C0 and 800D9520), so they are
// declared FIRST and left undefined and the vtable's home stays in the
// unity unit's data. SetModel is in no slot and is not virtual.
//
// Four shapes the bytes fixed. Every struct assignment here is a CALL:
// __as__7xMat4x3FRC7xMat4x3 and __as__5xVec3FRC5xVec3 are the image's own
// out-of-line implicit operator=, so the classes declare one and never
// define it. Attached uses the value it just stored for UpdaterSwitch and
// RELOADS the member for Show, which is what writing both through the
// member gives -- the store forwards, the call does not.
//
// Render MIXES the two: the model is read into a local for the two calls
// that take its address and re-read through the member for UpdateRender,
// which is where retail's third `lwz r3,160(r30)` comes from. Both
// uniform spellings were measured against it -- the member throughout is
// 18 of 30 words and two short, the local throughout 29 of 30.
//
// And SetModel's two flags are if/else, not an assignment from the test:
// retail branches round a `li 1; stb` to a `li 0; stb`. `matSet = mat !=
// 0` folds each pair into one store, 22 of 32 words and four short, and
// clearing the flag first and overwriting it inside the if gives 26 of
// 32 and two short.
//
// NO NEAR MISS. `python tools/unitcmp.py
// SB/GM/Engine/Game/zNPCExtraModel.cpp` reports 6 of 6 byte-identical,
// 612 bytes, which is every function the split holds. That is unitcmp's
// answer and not the oracle's: the unit has not been through ninja, is
// not marked Matching in configure.py, and nothing here has been linked
// or placed.

class zNPCBase;
class zNPCEntity;
class zNPCStatus;
class xMat3x3;
class xMat4x3;
class xVec3;

namespace World {
class ModelInstanceAsset;
class xOGModel;
class xOGModelUpdater;
}  // namespace World

class xVec3 {
public:
    xVec3& operator=(const xVec3& other);

    float x;
    float y;
    float z;
};

class xMat3x3 {
public:
    unsigned char _pad0[0x30];
};

class xMat4x3 {
public:
    xMat4x3& operator=(const xMat4x3& other);

    xMat3x3 rot;
    xVec3 pos;
    unsigned int pad3;
};

void xMat3x3MulScaleC(xMat3x3* dst, const xMat3x3* src, float x, float y,
                      float z);

namespace World {

// The article a model instance is created for; its shadow enum is nested
// in the class, which is what the mangled name Q35World20ModelInstance-
// Article7eShadow says.
class ModelInstanceArticle {
public:
    enum eShadow { eShadow_ = 0x7FFFFFFF };
};

class xOGModel {
public:
    static xOGModel* Create(const ModelInstanceAsset& asset,
                            ModelInstanceArticle::eShadow shadow);

    void UpdaterSwitch(xOGModelUpdater* updater, void* parent);
    void Show();
    void DeferDestroy();
    void UpdateRender();

    xMat4x3 Mat;
    unsigned char _pad0[0x178 - 0x40];
};

class xOGModelHandle {
public:
    xOGModel* model;
    unsigned int _pad0;
};

// The updater that does nothing: 0x807228F0, 16 bytes.
extern xOGModelUpdater g_modelUpdateNone;

}  // namespace World

class zNPCEntity {
public:
    unsigned char _pad0[0x34];
    World::xOGModelHandle ogModel;
    unsigned char _pad1[0x124 - 0x3C];
    xVec3 modelScale;
    unsigned char _pad2[0x1D0 - 0x130];
};

class zNPCBase {
public:
    unsigned char _pad0[0x98];
    zNPCEntity* npcEntity;
    unsigned char _pad1[0xC0 - 0x9C];
};

// zNPCComponent's owner is at +0 and its vtable pointer follows it, so
// the data is a base of its own and the virtuals go on the class above.
class zNPCComponentData {
public:
    zNPCBase* owner;
};

class zNPCComponent : public zNPCComponentData {
public:
    virtual void Attached(const zNPCStatus* status);
    virtual void Detached(zNPCStatus* status);
    virtual void Reset(const zNPCStatus* status);
    virtual void AllAttached();
    virtual void _v4();
    virtual void _v5();
    virtual void Render();
    virtual void Paused();
    virtual void Resumed();
    virtual void _v9();
};

class zNPCExtraModel : public zNPCComponent {
public:
    // Declared first and left undefined: they are this class's own, in
    // another unit, and they keep the vtable's home there.
    virtual void Paused();
    virtual void Resumed();

    virtual void Attached(const zNPCStatus* status);
    virtual void AllAttached();
    virtual void Detached(zNPCStatus* status);
    virtual void Reset(const zNPCStatus* status);
    virtual void Render();

    void SetModel(const World::ModelInstanceAsset* asset, xMat4x3* mat,
                  xVec3* scale);

    xMat4x3 curMatrix;
    xMat4x3 initialMatrix;
    xVec3 curScale;
    xVec3 initialScale;
    World::xOGModel* modelRenderData;
    World::ModelInstanceAsset* modelInstanceAsset;
    bool matSet;
    bool scaleSet;
    bool enabled;
    bool paused;
};

void zNPCExtraModel::Attached(const zNPCStatus* status) {
    modelRenderData = World::xOGModel::Create(
        *modelInstanceAsset, (World::ModelInstanceArticle::eShadow)0);

    modelRenderData->UpdaterSwitch(&World::g_modelUpdateNone, 0);
    modelRenderData->Show();

    enabled = true;
    paused = false;
}

void zNPCExtraModel::AllAttached() {
    if (!matSet) {
        initialMatrix = owner->npcEntity->ogModel.model->Mat;
    }

    if (!scaleSet) {
        initialScale = owner->npcEntity->modelScale;
    }

    curMatrix = initialMatrix;
    curScale = initialScale;
}

void zNPCExtraModel::Detached(zNPCStatus* status) {
    modelRenderData->DeferDestroy();

    modelRenderData = 0;
    modelInstanceAsset = 0;
    matSet = false;
    scaleSet = false;
    enabled = false;
    paused = false;
}

void zNPCExtraModel::Reset(const zNPCStatus* status) {
    curMatrix = initialMatrix;
    curScale = initialScale;
}

void zNPCExtraModel::SetModel(const World::ModelInstanceAsset* asset,
                              xMat4x3* mat, xVec3* scale) {
    modelInstanceAsset = (World::ModelInstanceAsset*)asset;

    if (mat != 0) {
        initialMatrix = *mat;
        matSet = true;
    } else {
        matSet = false;
    }

    if (scale != 0) {
        initialScale = *scale;
        scaleSet = true;
    } else {
        scaleSet = false;
    }
}

void zNPCExtraModel::Render() {
    if (enabled && !paused) {
        World::xOGModel* model = modelRenderData;

        xMat3x3MulScaleC(&model->Mat.rot, &curMatrix.rot, curScale.x,
                         curScale.y, curScale.z);

        model->Mat.pos = curMatrix.pos;

        modelRenderData->UpdateRender();
    }
}
