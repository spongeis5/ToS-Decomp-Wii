// zDecal.cpp -- six functions, read from the image with tools/disasm.py.
// A decal entity emits decals through xDecal: the asset's Create takes
// one 152-byte block from the global heap (tag 16), places the entity
// on it and runs init; the event wrapper, in namespace Local, answers
// the one event by looking the entity's own asset up again through the
// entity manager and running init with it. init keeps the asset, zeroes
// the next-emit time, runs xBaseInit, installs the wrapper and base
// type 65, clears the emit context and fills it from the asset: flags 3,
// an unbounded life (flag 8, life 1) when the asset's is under a
// thousandth, the range or 5 when it is not positive, unit texture
// coordinates, the size and its variance through xVec2::assign, the
// animation grid, and three flag bits for random start, random rotation
// and locked variance; with curve nodes it copies them (a key and six
// values each) into an xMemAlloc block (tag 11), resets the response
// curve over them and points the context at it. The two emit functions
// resolve the context's entity once from the asset's handle, emit at
// once when forced or when the rate is not positive, and otherwise run
// the timer down by the frame's dt and emit when it crosses zero, adding
// a period back. setHeight is the generated accessor.
//
// Layouts from the DWARF (tools/dwarf_types.py): xBase 0x38 with the id
// at +0x18, base type +0x20 and event function +0x30; xOGEntity 0x40
// with its model reference at +0x34, in xBase's tail padding; zDecal
// 0x98 with the curve at +0x3C (xOGEntity's tail padding again), the
// emit context at +0x4C, the next-emit time at +0x90 and the asset at
// +0x94; Sext::Decal 0x58 and emit_context 0x44 as declared. The first
// virtual is left undefined so the vtables' homes stay in the unity
// units' data. The four literals are the unity unit's pool, hence the
// generated padding header first; the unit has no data of its own.
//
// Two shapes the bytes fixed: once the asset is stored, init reads every
// field through the member and not the parameter (retail reloads
// this->asset after each call); the node copy is seven float assignments
// and not a struct copy, since retail reloads the asset's node pointer
// before every one of them.
//
// NEAR MISS, init 139 of 159 words: the four-literal base, the same wall
// zBouncer::BouncePlayer and zSBPlayerActions stand at. init loads four
// distinct literals seven times; retail spells a lis per load and ours
// forms one addis base in r30, which pays for the register it saves and
// restores and shifts every later word. The float-base section of
// NOTES.md lists what has been ruled out for it -- every -opt
// sub-option, every -O level, -inline, -ipa, -str, the pool_data and
// section pragmas, every mwcc on disk, and the literals emitted first by
// an unreferenced static function. Nothing here is new: the padding
// header is already at the measured 41,320 bytes, and the wall stands at
// 32 KB, 41,320, 70,000 and 140,000.
//
// NEAR MISS, the model emit 58 of 60 words: in the timed branch retail
// loads model->model last, after the store to the next-emit time, and
// ours loads it before the division, the same twelve instructions
// otherwise (the untimed branch, which has no store, loads it first in
// both). Tried and no better: the read with const cast away, through a
// volatile view, and the function restructured to one call site with an
// early return (that one also turns both compares round and costs nine
// words).

#include "SB/GM/Engine/Game/zDecal.pool.h"

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
class ImmediatePrototypeEntity;
class xBase;
class xMat4x3;
class xEnt;
class zDecal;

namespace World {
class EntityHandleBase;
}

namespace Sext {

class EventAny;

class xBaseAsset {
public:
    unsigned long long id;
    unsigned int baseType;
    unsigned short linkCount;
    unsigned short baseFlags;
};

class Decal : public xBaseAsset {
public:
    struct _Variance {
        float widthVar;
        float heightVar;
        bool randRotation;
        bool varianceLocked;
    };

    struct CurveNode {
        float key;
        float value[6];
    };

    struct __curveNodes__ {
        unsigned int count;
        CurveNode* nodes;
    };

    static zDecal* Create(World::EntityHandleBase* handle, Decal* asset);

    float emitRate;
    float life;
    float range;
    unsigned long long entity;
    float width;
    float height;
    _Variance Variance;
    unsigned int animRows;
    unsigned int animCols;
    float fps;
    float animTime;
    bool randomStart;
    __curveNodes__ curveNodes;
};

}  // namespace Sext

void xBaseInit(xBase* base, const Sext::xBaseAsset* asset);

namespace World {

class Entity;
class xOGModel;

class EntityHandle {
public:
    unsigned char _pad0[0x38];
    Entity* entity;
};

class EntityManager {
public:
    static Sext::xBaseAsset* FindAsset(unsigned long long id);
    static EntityHandle* FindHandle(unsigned long long id);
};

EntityManager* GetEntityManager();

class xOGModelRef {
public:
    xOGModel* model;
    unsigned int _pad0;
};

class xOGModelHandle : public xOGModelRef {};

}  // namespace World

class xVec2 {
public:
    void assign(float x, float y);

    float x;
    float y;
};

class xResponseCurve {
public:
    void reset(unsigned int values, const void* nodes, unsigned int count);

    unsigned int _values;
    void* curve;
    unsigned int _nodes;
    unsigned int active_node;
};

class xDecal {
public:
    struct emit_context {
        float life;
        float max_dist;
        ImmediatePrototypeEntity* entity;
        xVec2 size;
        xVec2 sizeVariation;
        xVec2 uv[2];
        unsigned int animRows;
        unsigned int animCols;
        float fps;
        float animTime;
        unsigned short flags;
        xResponseCurve* curve;
    };

    static bool emit(const emit_context& context, const xMat4x3& mat, int flags,
                     const xMat4x3* mat2);
    static void emit_model(const emit_context& context, const xMat4x3& mat,
                           const World::xOGModelRef* model, const xMat4x3* mat2,
                           xEnt* ent);
};

class xGlobals {
public:
    unsigned char _pad0[0x3AC];
    float update_dt;
};

extern xGlobals* xglobals;

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

class zDecal : public World::xOGEntity {
public:
    zDecal(World::EntityHandleBase* handle) : World::xOGEntity(handle) {}

    virtual void _v0();

    void init(Sext::Decal* a);
    void setHeight(float value);
    bool emit(const xMat4x3& mat, const xMat4x3* mat2, bool force);
    void emit(const xMat4x3& mat, const World::xOGModelRef* model);

    xResponseCurve curve;
    xDecal::emit_context emitInfo;
    float nextEmit;
    Sext::Decal* asset;
};

namespace Local {

void DecalEventWrapper(xBase* from, xBase* to, unsigned int event,
                       Sext::EventAny* any) {
    if (event == 0x389E01C0) {
        ((zDecal*)to)->init(
            (Sext::Decal*)World::GetEntityManager()->FindAsset(to->id));
    }
}

}  // namespace Local

zDecal* Sext::Decal::Create(World::EntityHandleBase* handle, Decal* asset) {
    zDecal* decal = new (memset(
        Memory::AllocGlobalHeap(sizeof(zDecal), (Memory::GlobalHeapEnum)0,
                                (eMemMgrTag)16, false),
        0, sizeof(zDecal))) zDecal(handle);

    decal->init(asset);

    return decal;
}

void zDecal::init(Sext::Decal* a) {
    asset = a;
    nextEmit = 0.0f;

    xBaseInit(this, asset);

    eventFunc = Local::DecalEventWrapper;
    baseType = 65;

    memset(&emitInfo, 0, sizeof(emitInfo));

    emitInfo.flags = 3;

    if (asset->life < 0.001f) {
        emitInfo.flags |= 8;
        emitInfo.life = 1.0f;
    } else {
        emitInfo.life = asset->life;
    }

    float dist = asset->range;

    if (dist <= 0.0f) {
        dist = 5.0f;
    }

    emitInfo.max_dist = dist;

    emitInfo.uv[0].x = 0.0f;
    emitInfo.uv[0].y = 0.0f;
    emitInfo.uv[1].x = 1.0f;
    emitInfo.uv[1].y = 1.0f;

    emitInfo.size.assign(asset->width, asset->height);
    emitInfo.sizeVariation.assign(asset->Variance.widthVar,
                                  asset->Variance.heightVar);

    emitInfo.curve = 0;
    emitInfo.animRows = asset->animRows;
    emitInfo.animCols = asset->animCols;
    emitInfo.fps = asset->fps;
    emitInfo.animTime = asset->animTime;

    if (asset->randomStart) {
        emitInfo.flags |= 0x40;
    }

    if (asset->Variance.randRotation) {
        emitInfo.flags |= 0x80;
    }

    if (asset->Variance.varianceLocked) {
        emitInfo.flags |= 0x100;
    }

    if (asset->curveNodes.count) {
        Sext::Decal::CurveNode* nodes = (Sext::Decal::CurveNode*)xMemAlloc(
            (Memory::GlobalHeapEnum)0,
            asset->curveNodes.count * sizeof(Sext::Decal::CurveNode), 0,
            (eMemMgrTag)11);

        for (unsigned int i = 0; i < asset->curveNodes.count; i++) {
            nodes[i].key = asset->curveNodes.nodes[i].key;
            nodes[i].value[0] = asset->curveNodes.nodes[i].value[0];
            nodes[i].value[1] = asset->curveNodes.nodes[i].value[1];
            nodes[i].value[2] = asset->curveNodes.nodes[i].value[2];
            nodes[i].value[3] = asset->curveNodes.nodes[i].value[3];
            nodes[i].value[4] = asset->curveNodes.nodes[i].value[4];
            nodes[i].value[5] = asset->curveNodes.nodes[i].value[5];
        }

        curve.reset(6, nodes, asset->curveNodes.count);

        emitInfo.curve = &curve;
    } else {
        emitInfo.curve = 0;
    }
}

void zDecal::setHeight(float value) {
    emitInfo.size.y = value;
}

bool zDecal::emit(const xMat4x3& mat, const xMat4x3* mat2, bool force) {
    if (emitInfo.entity == 0) {
        World::EntityHandle* handle =
            World::EntityManager::FindHandle(asset->entity);

        if (handle) {
            emitInfo.entity = (ImmediatePrototypeEntity*)handle->entity;
        }
    }

    if (force || asset->emitRate <= 0.0f) {
        return xDecal::emit(emitInfo, mat, 70, mat2);
    }

    nextEmit -= xglobals->update_dt;

    if (nextEmit <= 0.0f) {
        nextEmit += 1.0f / asset->emitRate;

        return xDecal::emit(emitInfo, mat, 70, mat2);
    }

    return false;
}

// The model's instance begins with its matrix, so the model pointer is
// the matrix pointer the emitter takes.
void zDecal::emit(const xMat4x3& mat, const World::xOGModelRef* model) {
    if (emitInfo.entity == 0) {
        World::EntityHandle* handle =
            World::EntityManager::FindHandle(asset->entity);

        if (handle) {
            emitInfo.entity = (ImmediatePrototypeEntity*)handle->entity;
        }
    }

    if (asset->emitRate <= 0.0f) {
        xDecal::emit_model(emitInfo, mat, model, (const xMat4x3*)model->model,
                           0);
        return;
    }

    nextEmit -= xglobals->update_dt;

    if (nextEmit <= 0.0f) {
        nextEmit += 1.0f / asset->emitRate;

        xDecal::emit_model(emitInfo, mat, model, (const xMat4x3*)model->model,
                           0);
    }
}
