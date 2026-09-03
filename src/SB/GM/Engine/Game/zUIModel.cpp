// zUIModel.cpp -- six functions, read from the image with
// tools/disasm.py. A UI model: the asset's Create takes one 256-byte
// block from the global heap (tag 16), places the entity on it (the zUI
// constructor and the vtable) and runs zUI_Init. DoInit creates the
// model from the asset's model instance with no shadow, sets viewport
// mask 2 on each of the model's renderables through a pointer to member
// (the 12-byte constant copied to the stack and __ptmf_scall, once per
// renderable), runs the base DoInit, points the link array at the
// asset's event links and runs InitModel. InitModel clears the animation
// table and, when the asset names a model prototype and an animation
// list whose handle resolves to an entity with a table and animations,
// keeps the table, allocates the model's animation play from the current
// scene's pool and starts the "stop00" state at time zero. DoUpdate runs
// the base update and, with a model, updates and evaluates it, builds
// the transform and shows it. DoHandleEvent is a switch: eight animation
// events go to zEntAnimEventCore, one runs the base handler and restarts
// the current state, and the rest the base handler. DoReset is the
// generated tail call.
//
// Layouts from the DWARF (tools/dwarf_types.py): xBase 0x38, xOGEntity
// 0x40 with its model reference at +0x34; zUI 0x100 with the asset at
// +0x40 and its last members at +0xF0; zUIModel adds the table at +0xF8,
// in zUI's tail padding, and stays 0x100; UI_Model with the model
// instance at +0xB0, the animation list at +0xF0 and the event links at
// +0xF8; xOGModel with the play at +0x4C and the article's renderable
// count and array at +0x11C and +0x120; xAnimPlay's single at +0xC, the
// single's state at +4. The Matrix43 constructor is empty and folded in
// retail into the shared blr the image names __ct__Q24Math8Matrix33Fv,
// so that is the name called here, as the other units do. The first
// virtual is left undefined so the vtables' homes stay in the unity
// units' data. The string and the literal are the unity unit's pool,
// hence the generated header first; the pointer-to-member constant is
// the unit's own data.
//
// Four shapes the bytes fixed. The renderable count is read once into a
// local before the loop and the model kept in a local through it. The
// pointer to member is declared INSIDE the loop, after the renderable
// is read into its own local: retail reloads the three constant words
// every iteration, so hoisting the declaration out of the loop (which
// loads them once and keeps them) costs 22 words, and reading the
// renderable first is what leaves r3 occupied and sends the constant to
// r6, r5 and r0 as retail has it. The animation list is a nested
// conditional with the null cases first, which is the block order
// retail has. And DoUpdate reads the model through a volatile view for
// its test only: retail loads it for the test and again for each of the
// four calls, and a plain member read is folded into one load.

#include "SB/GM/Engine/Game/zUIModel.pool.h"

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {
enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };

void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap, eMemMgrTag tag,
                      bool clear);
}  // namespace Memory

extern "C" {
void* memset(void* dst, int c, unsigned long n);

// The shared empty function every folded empty body branches to.
void __ct__Q24Math8Matrix33Fv(void* matrix);
}

inline void* operator new(unsigned long, void* p) { return p; }

class LinkAsset {
public:
    unsigned int count;
    unsigned int data;
};
class TemplateEntity;
class xAnimTable;
class xAnimState;
class xMat4x3;
class xEnt;
class xBase;
class zUIModel;

namespace Math {

class Vector4 {
public:
    float x;
    float y;
    float z;
    float w;
};

class Matrix33 {
public:
    Vector4 v[3];
};

class Matrix43 : public Matrix33 {};

}  // namespace Math

namespace Graphics {

class Viewport {
public:
    enum VisibilityMask { VisibilityMask_ = 0x7FFFFFFF };
};

class Renderable {
public:
    void SetViewportVisibleMask(Viewport::VisibilityMask mask);
};

}  // namespace Graphics

class xAnimSingle {
public:
    unsigned int SingleFlags;
    xAnimState* State;
};

class xAnimPlay {
public:
    unsigned char _pad0[0xC];
    xAnimSingle* Single;
};

class xMemPool {
public:
    void* FreeList;
    unsigned short NextOffset;
    unsigned short Flags;
    void* UsedList;
    void (*InitCB)();
    void* Buffer;
    unsigned short Size;
    unsigned short NumRealloc;
    unsigned int Total;
};

namespace Sext {

class EventAny;

class xBaseAsset {
public:
    unsigned long long id;
    unsigned int baseType;
    unsigned short linkCount;
    unsigned short baseFlags;
};

class zUI : public xBaseAsset {
public:
    unsigned char _pad0[0xB0 - 0x10];
};

}  // namespace Sext

namespace World {

class EntityHandleBase;
class Entity;
class xOGModel;

class EntityHandle {
public:
    unsigned char _pad0[0x38];
    Entity* entity;
};

class EntityManager {
public:
    static EntityHandle* FindHandle(unsigned long long id);
};

class ModelInstanceAsset {
public:
    unsigned long long modelPrototypeID;
    unsigned long long lightKitID;
    unsigned int instanceParamCount;
    unsigned int renderCustomizerCount;
    unsigned int instanceParams;
    unsigned int renderCustomizers;
    unsigned short shadowType;
    unsigned short shadowFlags;
    unsigned int shadowColorOverride;
    float shadowMaxDepthOverride;
    float shadowStartDepthOverride;
    unsigned int shadowMinBlurOverride;
    unsigned int shadowMaxBlurOverride;
    unsigned long long parentID;
};

// Only the renderable count and array are known inside the article.
class ModelInstanceArticle {
public:
    enum eShadow { eShadow_ = 0x7FFFFFFF };

    unsigned char _pad0[0x58];
    unsigned short renderableCount;
    unsigned short _pad1;
    Graphics::Renderable** renderables;
    unsigned char _pad2[0x98 - 0x60];
};

class xOGModel {
public:
    static xOGModel* Create(const ModelInstanceAsset& asset,
                            ModelInstanceArticle::eShadow shadow);

    void Show();

    unsigned char _pad0[0x4C];
    xAnimPlay* Anim;
    unsigned char _pad1[0xC4 - 0x50];
    ModelInstanceArticle mModelArt;
};

class xOGModelRef {
public:
    operator xOGModel*() const { return model; }

    xOGModel* model;
    unsigned int _pad0;
};

class xOGModelHandle : public xOGModelRef {};

}  // namespace World

namespace Sext {

class UI_Model : public zUI {
public:
    static zUIModel* Create(World::EntityHandleBase* handle, UI_Model* asset);

    World::ModelInstanceAsset ModelInstance;
    unsigned long long animationList;
    LinkAsset EventLinksNew;
};

}  // namespace Sext

// The entity an animation list resolves to: only its table and its
// animations are touched here.
class zAnimListEntity {
public:
    unsigned char _pad0[0x3C];
    xAnimTable* table;
    void* animations;
};

class zScene {
public:
    unsigned char _pad0[0xC];
    xMemPool animPool;
};

class zGlobals {
public:
    unsigned char _pad0[0x43C];
    zScene* sceneCur;
};

extern zGlobals globals;

void xAnimPoolAlloc(xMemPool* pool, void* object, xAnimTable* table,
                    World::xOGModel* model);
xAnimState* xAnimTableGetState(xAnimTable* table, const char* name);
void xAnimPlaySetState(xAnimSingle* single, xAnimState* state, float time);
void xModelUpdate(World::xOGModel* model, float dt);
void xModelEval(World::xOGModel* model);
void xMat4x3FromNGMatrix(xMat4x3* mat, const Math::Matrix43* matrix);
void zEntAnimEventCore(World::xOGModel* model, xAnimTable* table,
                       unsigned int event, Sext::EventAny* any, xEnt* ent);

class xBase {
public:
    virtual void _v0();

    unsigned char _pad0[0x14];
    unsigned long long id;
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

class xOGEntity : public xBase {
public:
    xOGEntity(EntityHandleBase* handle);

    xOGModelHandle ogModel;
};

}  // namespace World

class zUI : public World::xOGEntity {
public:
    zUI(World::EntityHandleBase* handle);

    virtual void _v0();

    void DoInit();
    void DoReset();
    void DoUpdate(float dt);
    void DoHandleEvent(xBase* from, unsigned int event, Sext::EventAny* any);
    void GetTransform(Math::Matrix43& matrix) const;

    Graphics::Viewport::VisibilityMask UIViewportMask;
    Sext::zUI* asset;
    unsigned char _pad0[0xF0 - 0x44];
    float motionTime;
    bool motionFiredEvent;
    bool motionLoop;
};

void zUI_Init(zUI* ui, Sext::zUI* asset);

class zUIModel : public zUI {
public:
    zUIModel(World::EntityHandleBase* handle) : zUI(handle) {}

    virtual void _v0();

    void DoInit();
    void DoReset();
    void DoUpdate(float dt);
    void DoHandleEvent(xBase* from, unsigned int event, Sext::EventAny* any);

    void InitModel();

    Sext::UI_Model* modelAsset() { return (Sext::UI_Model*)asset; }

    xAnimTable* atbl;
};

void zUIModel::DoInit() {
    World::xOGModel* model = World::xOGModel::Create(
        modelAsset()->ModelInstance, (World::ModelInstanceArticle::eShadow)0);

    ogModel.model = model;

    unsigned int count = model->mModelArt.renderableCount;

    for (unsigned int i = 0; i < count; i++) {
        Graphics::Renderable* renderable = model->mModelArt.renderables[i];

        void (Graphics::Renderable::*setMask)(Graphics::Viewport::VisibilityMask) =
            &Graphics::Renderable::SetViewportVisibleMask;

        (renderable->*setMask)((Graphics::Viewport::VisibilityMask)2);
    }

    zUI::DoInit();

    linkArray = &modelAsset()->EventLinksNew;

    InitModel();
}

void zUIModel::InitModel() {
    atbl = 0;

    if (modelAsset()->ModelInstance.modelPrototypeID != 0) {
        unsigned long long animList = modelAsset()->animationList;
        World::EntityHandle* handle;

        zAnimListEntity* list =
            animList == 0
                ? 0
                : (handle = World::EntityManager::FindHandle(animList)) == 0
                      ? 0
                      : (zAnimListEntity*)handle->entity;

        if (list == 0) {
            return;
        }

        if (list->animations == 0) {
            return;
        }

        atbl = list->table;

        xAnimPoolAlloc(&globals.sceneCur->animPool, this, atbl, ogModel.model);

        xAnimState* state = xAnimTableGetState(atbl, "stop00");

        if (state) {
            xAnimPlaySetState(ogModel.model->Anim->Single, state, 0.0f);
        }
    }
}

void zUIModel::DoReset() {
    zUI::DoReset();
}

// The model's instance begins with its matrix, so the model pointer is
// the matrix pointer the conversion takes.
void zUIModel::DoUpdate(float dt) {
    zUI::DoUpdate(dt);

    // Read for the test and again for every call, as retail has it: a
    // plain member read is folded into a single load.
    if (*(World::xOGModel* volatile*)&ogModel.model) {
        xModelUpdate(ogModel.model, dt);
        xModelEval(ogModel.model);

        Math::Matrix43 matrix;

        __ct__Q24Math8Matrix33Fv(&matrix);  // Math::Matrix43::Matrix43(), empty

        GetTransform(matrix);

        xMat4x3FromNGMatrix((xMat4x3*)ogModel.model, &matrix);

        ogModel.model->Show();
    }
}

void zUIModel::DoHandleEvent(xBase* from, unsigned int event,
                             Sext::EventAny* any) {
    switch (event) {
    case 0x6A66E1C9:
    case 0xDD8D2820:
    case 0xBF6E4B2F:
    case 0x8E0B3921:
    case 0x2946B62E:
    case 0x7BE98441:
    case 0x71316E19:
    case 0x6ACFEA03:
        zEntAnimEventCore(ogModel.model, atbl, event, any, 0);
        break;
    case 0x7D3E0C05:
        zUI::DoHandleEvent(from, event, any);

        if (ogModel.model->Anim) {
            xAnimSingle* single = ogModel.model->Anim->Single;

            if (single->State) {
                xAnimPlaySetState(single, single->State, 0.0f);
            }
        }
        break;
    default:
        zUI::DoHandleEvent(from, event, any);
        break;
    }
}

zUIModel* Sext::UI_Model::Create(World::EntityHandleBase* handle,
                                 UI_Model* asset) {
    zUIModel* ui = new (memset(
        Memory::AllocGlobalHeap(sizeof(zUIModel), (Memory::GlobalHeapEnum)0,
                                (eMemMgrTag)16, false),
        0, sizeof(zUIModel))) zUIModel(handle);

    zUI_Init(ui, asset);

    return ui;
}
