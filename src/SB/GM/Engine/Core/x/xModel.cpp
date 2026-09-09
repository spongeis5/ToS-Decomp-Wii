// C:/branches/SB09/main/GM/Engine/Core/x/xModel.cpp
//
// Hand-owned. The accessor part at the foot of this file was written by
// tools/gen_accessors.py and is kept exactly as it emitted it; the rest
// is read from the image, and the LAYOUTS are not guesses -- this unit
// is one of the 415 that tools/dwarf_splits.py recovered from the Wii
// build's DWARF, so every class below came out of
// `tools/dwarf_types.py --type <name>` with its size and every member
// offset asserted, and every function's real name, parameter names and
// local names out of `tools/dwarf_locals.py --unit`.
//
// THE `addic.` IS `&obj->member`, NOT A MASK. Two of these functions
// open with `addic. r0,r3,196` / `beq`, and 196 is 0xC4, which the DWARF
// says is `xOGModel::mModelArt` -- a member held BY VALUE. So the source
// takes its address and null-checks the result, and mwcc does the add
// and the test in one instruction. The same idiom appears at +0x18 into
// SkeletonBlobEntity, which is `&skelBlob->skel`.
//
// THE REFERENCE-ANIMATION POOL IS TWO STATICS OF xModelInstance: a
// 50-entry array and the head of the in-use list, 0x12C0 apart in the
// image, which is exactly 50 * sizeof(RefUniqueAnimation). Both are
// class statics, so their mangled names carry the class and the
// relocations line up with retail's.

class xAnimPlay;
class xAnimFile;

namespace Graphics {
class TextureResourceEntity;
class ShadowSimpleCache;
class ModelPrototypeEntity;
class LightKitEntity;
class RenderCustomizerInfo;
class ModelXformBuffer;
class ModelJointBuffer;
class ModelMorphWeightBuffer;
class ModelPrototype;
class Geometry;
class RefModelInstanceChildXform;
class Renderable3D;
class LightKit;
class LightKitData;
class CollisionMeshBlobEntity;
class PerInstanceData;
class Builder;
class ModelInstanceParamData;
class ModelPartDefinition;
class SkeletonBlobEntity;
}  // namespace Graphics

class xVec3 {
public:
    xVec3& operator=(const xVec3& other);
    xVec3& operator+=(const xVec3& other);

    float x;
    float y;
    float z;
};

class xMat3x3 {
public:
    xVec3 left;
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

namespace Graphics {

class DataType {
public:
    float x;
    float y;
    float z;
    float w;
};

class Vector4 {
public:
    DataType data;
};

class Matrix33 {
public:
    Vector4 v[3];
};

class Matrix43 : public Matrix33 {
};

class Renderable3DLink {
public:
    Renderable3D* rend;
    Renderable3DLink* rendPrev;
    Renderable3DLink* rendNext;
};

class ParamCargo {
public:
    int textureCount;
    TextureResourceEntity** textureEnts;
};

class ReferenceModelEntry {
public:
    unsigned long long id;
    CollisionMeshBlobEntity* collMesh;
    PerInstanceData* instances;
    unsigned short numChildTransforms;
    unsigned short numInstances;
    unsigned short childTransformOffset;
    unsigned short numRenderables;
    unsigned short firstRenderableOffset;
};

class Model {
public:
    ReferenceModelEntry* GetReferenceModel(unsigned long long uid,
                                           unsigned short& refIndex);

    Matrix43 rootTransform;
    unsigned short visibleCount;
    unsigned short childTransformCount;
    unsigned short renderableCount;
    unsigned short renderCustomizerCount;
    Renderable3D** renderables;
    ModelXformBuffer* xforms;
    ModelJointBuffer* joints;
    ModelMorphWeightBuffer* morphWeights;
    ModelPrototype* modelProto;
    Geometry** geoms;
    Matrix43* childTransforms;
    unsigned short* childTransformParents;
    unsigned short* renderableTransformMap;
    RefModelInstanceChildXform* refModelInstanceChildXforms;
    unsigned short dirtyRefChildTransformCount;
    unsigned short dirtyNonRefChildTransformCount;
    unsigned int* xformDirtyBits;
};

class ModelPrototype {
public:
    ReferenceModelEntry* GetReferenceModelEntry(unsigned short index) const;

    Builder** builders;
    unsigned short* renderableGeomMap;
    Matrix43* childTransforms;
    unsigned short* childTransformParents;
    unsigned short* renderableTransformMap;
    unsigned int instanceParamCount;
    unsigned long long* instanceParamKey;
    ModelInstanceParamData* instanceParamData;
    ModelPartDefinition* modelPartDefinitions;
    ReferenceModelEntry* refModelEntries;
    unsigned short geomCount;
    unsigned short renderableCount;
    unsigned short nonRefTransformCount;
    unsigned short modelPartCount;
    unsigned short refInstanceCount;
    unsigned short refModelCount;
    unsigned short refTransformCount;
    unsigned char _pad0[0x38 - 0x36];
    void* skeleton;
    int createSize;
    int createAlign;
    bool segmentedModel;
};

// Node is the engine's scene-graph base: a vtable pointer and four
// bytes of type, which is all these functions reach through.
class Node {
public:
    unsigned char _vptr[4];
    int type;
    bool attached;
    bool detachPending;
    bool updateThread;
};

class Skeleton : public Node {
public:
    void* evals;
    int evalCount;
    void* evalCBs;
    int totalTransformCount;
    void* groups;
    int groupCount;
    void* skins;
    int skinCount;
    int skinJointTotal;
    int morphWeightTotal;
    signed char* transformToGroupMap;
    bool userScale;
    unsigned char _pad0[0x3C - 0x39];
    Matrix43* hackEvalResults;
};

class BlobEntity {
public:
    unsigned char _pad0[0x18];
};

class SkeletonBlobEntity : public BlobEntity {
public:
    Skeleton skel;
};

class Entity {
public:
    unsigned char _pad0[0x18];
};

class ModelPrototypeEntity : public Entity {
public:
    int memSize;
    int refModelActivated;
    ModelPrototype modelProto;
    SkeletonBlobEntity* skelBlob;
    CollisionMeshBlobEntity* collmeshBlob;
};

class Renderable3D {
public:
    void SetLightKit(const Graphics::LightKitData* value);

    unsigned char _pad0[0x78];
    int f78;
};

}  // namespace Graphics

class xModelInstance {
public:
    class PartsVisibility {
    public:
        unsigned char partIndex;
        unsigned char stateMask;
    };

    class ModelVisibility {
    public:
        PartsVisibility visParts[16];
        unsigned char visCount;
        bool disableVisibilityAnim;
    };

    // The two names the DWARF gives the same fourteen bits are the two
    // things the field means: a distance while the animation is
    // enabled, the instance it belongs to while it is not.
    class RefInstanceAnimation {
    public:
        void Clear();

        RefInstanceAnimation* forUseByMemPool;
        RefInstanceAnimation* next;
        RefInstanceAnimation* nextAnim;
        RefInstanceAnimation* lastAnim;
        unsigned int currentLODDistance : 14;
        unsigned int refModelProtoInstanceNum : 16;
        bool enabled : 1;
        bool looping : 1;
        float time;
        xAnimFile* animFile;
    };

    class RefUniqueAnimation {
    public:
        unsigned char animFile[0x40];
        void* animFileRawDataPtr;
        unsigned char _pad0[0x48 - 0x44];
        unsigned long long animID;
        unsigned int refCount;
        RefUniqueAnimation* next;
        RefUniqueAnimation* previous;
    };

    void UpdateReferenceAnimationLODFPS(float fps);
    static RefUniqueAnimation* GetRefAnimationEntry(const xAnimFile* animFile);
    static RefUniqueAnimation* GetRefAnimationEntry(unsigned long long animationID);
    RefInstanceAnimation* GetNextAnimationLODUpdateNode();

    static RefUniqueAnimation* AddRefAnimation(
        unsigned long long animationID, unsigned int numRefsToAdd,
        unsigned int& numInstancesCurrentlyBound);
    static void RemoveRefAnimation(RefUniqueAnimation* uniqueAnimEntry,
                                   unsigned int numRefsToRemove);
    void RefAnimationEnable(RefInstanceAnimation* animInst);
    void RefAnimationDisable(RefInstanceAnimation* animInst);

    static RefUniqueAnimation uniqueRefAnimations[50];
    static RefUniqueAnimation* uniqueRefAnimationsUsed;
    static RefUniqueAnimation* uniqueRefAnimationsAvailable;

    xMat4x3 Mat;
    xVec3 Scale;
    xAnimPlay* Anim;
    unsigned short Flags;
    unsigned short pad;
    unsigned int renderCustomizerMask;
    ModelVisibility visModel;
    unsigned char _pad0[0x7C - 0x7A];
    int boundRefModelInstanceAnimationCount;
    RefInstanceAnimation* referenceAnimations;
    RefInstanceAnimation* enabledReferenceAnimations;
    RefInstanceAnimation* disabledReferenceAnimations;
    RefInstanceAnimation* nextReferenceAnimationLODUpdate;
    unsigned int numAnimationsEnabled;
};

class EmbeddedListNode {
public:
    EmbeddedListNode* next;
    EmbeddedListNode* prev;
};

class EmbeddedList {
public:
    EmbeddedListNode head;
    unsigned long size;
};

namespace World {

class xOGModel;

class ModelInstanceArticle {
public:
    unsigned int projectShadowCache;
    Graphics::ShadowSimpleCache* simpleShadowCachePtr;
    Graphics::Renderable3DLink simpleShadowCastOn;
    int sceneRefCount;
    Graphics::ModelPrototypeEntity* protoEnt;
    Graphics::ParamCargo paramCargo;
    Graphics::Model model;
    Graphics::LightKitEntity* lightKitEnt;
    Graphics::RenderCustomizerInfo* renderCustomizers;
    void* memoryPtr;
};



class xOGModelUpdater {
public:
    virtual void DoUpdate();

    EmbeddedList updList;
};

class xOGModel : public xModelInstance {
public:
    void UpdateRender();
    void SwapXModel(xOGModel& src);
    int AllocAnimationInstances();
    void DeallocAnimationInstances();
    RefInstanceAnimation* GetRefAnimation(unsigned long long refId,
                                          unsigned short refInstanceOffset);

    float blendTime;
    float blendTimeCurrent;
    float blendStart;
    float blendEnd;
    int overrideAnimTexMerge;
    void* morphOverride;
    float morphTime;
    float currentMorphTime;
    Graphics::Vector4 colorMultiplier;
    ModelInstanceArticle mModelArt;
    xOGModelUpdater* updater;
    void* updateParent;
    EmbeddedListNode updateNode;
    float lodScale;
    int castShadow;
    int mMemSize;
};

}  // namespace World

class xAnimPlay {
public:
    xAnimPlay* Next;
    unsigned short NumSingle;
    unsigned short BoneCount;
    unsigned short MorphCount;
    unsigned short padding0;
    void* Single;
    void* Object;
    void* Table;
    void* Pool;
    World::xOGModel* ModelInst;
    void (*BeforeAnimMatrices)();
    void (*AfterAnimMatrices)();
    void (*AnimMorphWeights)();
    unsigned int padding1[1];
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

void* xMemPoolAlloc(xMemPool* pool, unsigned int count);
void xMemPoolFree(xMemPool* pool, void* data);

extern "C" void* memset(void* dst, int val, unsigned long len);

void xAnimPlayEval(xAnimPlay* play, xModelInstance::ModelVisibility* vis);
void xAnimPlayEvalRefModels(World::xOGModel* modelInst);
void xAnimPlayUpdateRefModels(World::xOGModel* modelInst, float timeDelta);
void xAnimPlayChooseTransition(xAnimPlay* play);
void xAnimPlayUpdate(xAnimPlay* play, float timeDelta, bool chooseTransition);
void xModelUpdatePartsVis(World::xOGModel* modelInst);
void xModelGetBoneMatNoScale(xMat4x3& mat, const World::xOGModel& model,
                             unsigned long index);
void xModelGetBoneMatNoScale(xMat4x3& mat, const World::xOGModel& model,
                             unsigned long index, const xMat4x3& root);
void xMat3x3RMulVec(xVec3* o, const xMat3x3* m, const xVec3* v);
void xMat3x3MulScaleC(xMat3x3* o, const xMat3x3* m, float x, float y,
                      float z);

// A file static: CodeWarrior leaves its name unmangled, which is how
// it reads in the image.
static xMemPool refAnimPool;

xModelInstance::RefUniqueAnimation*
xModelInstance::GetRefAnimationEntry(const xAnimFile* animFile) {
    RefUniqueAnimation* curUniqueAnimation = uniqueRefAnimations;
    int c;

    for (c = 0; c < 50; c++) {
        if ((unsigned int)((const char*)animFile
                           - (const char*)curUniqueAnimation)
            < sizeof(RefUniqueAnimation)) {
            break;
        }

        curUniqueAnimation++;
    }

    if (c < 50) {
        return curUniqueAnimation;
    }

    return 0;
}

xModelInstance::RefUniqueAnimation*
xModelInstance::GetRefAnimationEntry(unsigned long long animationID) {
    RefUniqueAnimation* curUniqueAnimEntry = uniqueRefAnimationsUsed;

    while (curUniqueAnimEntry != 0) {
        if (curUniqueAnimEntry->animID == animationID) {
            return curUniqueAnimEntry;
        }

        curUniqueAnimEntry = curUniqueAnimEntry->next;
    }

    return 0;
}

void xModelInstance::RefInstanceAnimation::Clear() {
    next = 0;
    enabled = true;
    looping = false;
    animFile = 0;
    time = 0.0f;
    lastAnim = 0;
    nextAnim = 0;
    currentLODDistance = 0x3FFF;
}

void xModelEvalSingle(World::xOGModel* modelInst) {
    if ((modelInst->Flags & 2) && modelInst->Anim != 0) {
        xAnimPlayEval(modelInst->Anim, &modelInst->visModel);
        xModelUpdatePartsVis(modelInst);
    }
}

void xModelEval(World::xOGModel* modelInst) {
    xModelEvalSingle(modelInst);

    if (modelInst->referenceAnimations != 0) {
        xAnimPlayEvalRefModels(modelInst);
    }
}

int xModelGetBoneCount(const World::xOGModel* model) {
    if (&model->mModelArt != 0) {
        Graphics::SkeletonBlobEntity* skelBlob =
            model->mModelArt.protoEnt->skelBlob;

        if (skelBlob != 0) {
            Graphics::Skeleton* skel = &skelBlob->skel;

            if (skel != 0) {
                return skel->groupCount + skel->skinJointTotal;
            }
        }
    }

    return 0;
}

int xModelGetMorphCount(const World::xOGModel* model) {
    if (&model->mModelArt != 0) {
        Graphics::SkeletonBlobEntity* skelBlob =
            model->mModelArt.protoEnt->skelBlob;

        if (skelBlob != 0) {
            return skelBlob->skel.morphWeightTotal;
        }
    }

    return 0;
}

void World::xOGModelUpdater::DoUpdate() {
    EmbeddedListNode* node = updList.head.next;

    while (node != &updList.head) {
        xOGModel* model = (xOGModel*)((char*)node - 0x164);

        model->UpdateRender();
        node = node->next;
    }
}


void World::xOGModel::DeallocAnimationInstances() {
    RefInstanceAnimation* animInst = referenceAnimations;

    while (animInst != 0) {
        xMemPoolFree(&refAnimPool, animInst);
        animInst = animInst->next;
    }

    nextReferenceAnimationLODUpdate = 0;
    disabledReferenceAnimations = 0;
    enabledReferenceAnimations = 0;
    referenceAnimations = 0;
    numAnimationsEnabled = 0;
}

Graphics::ReferenceModelEntry* Graphics::Model::GetReferenceModel(
    unsigned long long uid, unsigned short& refIndex) {
    refIndex = 0;

    ReferenceModelEntry* entry = modelProto->GetReferenceModelEntry(0);

    while (entry != 0) {
        if (entry->id == uid) {
            return entry;
        }

        entry = modelProto->GetReferenceModelEntry(++refIndex);
    }

    return 0;
}

// Flags bit 4 gates the update and bit 8 asks for a transition to be
// chosen; the same bit is then handed to xAnimPlayUpdate, which is
// why retail re-reads Flags after the call.
void xModelUpdate(World::xOGModel* modelInst, float timeDelta) {
    xAnimPlayUpdateRefModels(modelInst, timeDelta);

    if (modelInst->Anim != 0 && (modelInst->Flags & 4)) {
        if (modelInst->Flags & 8) {
            xAnimPlayChooseTransition(modelInst->Anim);
        }

        xAnimPlayUpdate(modelInst->Anim, timeDelta,
                        (modelInst->Flags & 8) != 0);
    }
}

void xModelGetBoneLocationNoScale(xVec3& loc, const World::xOGModel& model,
                                  unsigned long index) {
    xMat4x3 mat;

    xModelGetBoneMatNoScale(mat, model, index);
    loc = mat.pos;
}


// The walk stops on either list head, and which one it landed on
// decides where it picks up again.
xModelInstance::RefInstanceAnimation*
xModelInstance::GetNextAnimationLODUpdateNode() {
    RefInstanceAnimation* next =
        nextReferenceAnimationLODUpdate->nextAnim;

    if (next != enabledReferenceAnimations
        && next != disabledReferenceAnimations) {
        return next;
    }

    if (next == enabledReferenceAnimations) {
        if (disabledReferenceAnimations != 0) {
            return disabledReferenceAnimations;
        }

        return enabledReferenceAnimations;
    }

    if (enabledReferenceAnimations != 0) {
        return enabledReferenceAnimations;
    }

    return disabledReferenceAnimations;
}

// The last reference away and the entry goes back on the free list,
// unlinked from both neighbours first.
void xModelInstance::RemoveRefAnimation(RefUniqueAnimation* uniqueAnimEntry,
                                        unsigned int numRefsToRemove) {
    if (uniqueAnimEntry == 0) {
        return;
    }

    uniqueAnimEntry->refCount -= numRefsToRemove;

    if (uniqueAnimEntry->refCount != 0) {
        return;
    }

    uniqueAnimEntry->animID = 0;

    RefUniqueAnimation* firstAvail = uniqueRefAnimationsAvailable;

    if (uniqueAnimEntry->next != 0) {
        uniqueAnimEntry->next->previous = uniqueAnimEntry->previous;
    }

    if (uniqueAnimEntry->previous != 0) {
        uniqueAnimEntry->previous->next = uniqueAnimEntry->next;
    }

    if (firstAvail != 0) {
        uniqueAnimEntry->next = firstAvail;
    }

    uniqueRefAnimationsAvailable = uniqueAnimEntry;
}


void xModelGetBoneMatNoScale(xMat4x3& mat, const World::xOGModel& model,
                             unsigned long index, const xVec3& offset) {
    const xMat4x3& root_mat = model.Mat;

    if (index >= (unsigned long)xModelGetBoneCount(&model)) {
        mat = root_mat;
    } else {
        xModelGetBoneMatNoScale(mat, model, index);
    }

    xVec3 newOffset;

    xMat3x3RMulVec(&newOffset, &mat, &offset);
    mat.pos += newOffset;
}

// A zero Scale.x means no scale was ever set, so the root matrix is
// used as it stands; otherwise a scaled copy is built on the stack and
// the root pointer is aimed at that instead.
void xModelGetBoneMatScaled(xMat4x3& mat, const World::xOGModel& model,
                            unsigned long index) {
    xMat4x3 temp_mat;
    xMat4x3* root_mat = (xMat4x3*)&model.Mat;

    if (model.Scale.x != 0.0f) {
        xMat3x3MulScaleC(&temp_mat, &model.Mat, model.Scale.x,
                         model.Scale.y, model.Scale.z);
        temp_mat.pos = model.Mat.pos;
        root_mat = &temp_mat;
    }

    if (index >= (unsigned long)xModelGetBoneCount(&model)) {
        mat = *root_mat;
    } else {
        xModelGetBoneMatNoScale(mat, model, index, *root_mat);
    }
}


// No `this` in the debug info and the id arrives in r3:r4, so this is
// static like both GetRefAnimationEntry overloads.
xModelInstance::RefUniqueAnimation* xModelInstance::AddRefAnimation(
    unsigned long long animationID, unsigned int numRefsToAdd,
    unsigned int& numInstancesCurrentlyBound) {
    RefUniqueAnimation* uniqueAnimEntry = GetRefAnimationEntry(animationID);

    if (uniqueAnimEntry == 0 && numRefsToAdd != 0) {
        uniqueAnimEntry = uniqueRefAnimationsAvailable;

        // An `else` and not an early return: retail lays the `li r3,0`
        // BETWEEN the assignment block and the refcount block, and pays
        // for an extra branch to do it.
        if (uniqueAnimEntry != 0) {
            uniqueAnimEntry->animID = animationID;
            uniqueRefAnimationsAvailable = uniqueAnimEntry->next;
            uniqueAnimEntry->next = uniqueRefAnimationsUsed;
            uniqueRefAnimationsUsed = uniqueAnimEntry;
        } else {
            return 0;
        }
    }

    uniqueAnimEntry->refCount += numRefsToAdd;
    numInstancesCurrentlyBound = uniqueAnimEntry->refCount;

    return uniqueAnimEntry;
}

// Unlink from the disabled ring, splice onto the tail of the enabled
// one. Retail re-reads enabledReferenceAnimations at every step rather
// than holding it, so the source does too.
void xModelInstance::RefAnimationEnable(RefInstanceAnimation* animInst) {
    if (animInst->enabled) {
        return;
    }

    if (animInst->nextAnim == animInst) {
        disabledReferenceAnimations = 0;
    } else {
        animInst->lastAnim->nextAnim = animInst->nextAnim;
        animInst->nextAnim->lastAnim = animInst->lastAnim;

        if (animInst == disabledReferenceAnimations) {
            disabledReferenceAnimations = animInst->nextAnim;
        }
    }

    if (enabledReferenceAnimations != 0) {
        animInst->lastAnim = enabledReferenceAnimations->lastAnim;
        enabledReferenceAnimations->lastAnim->nextAnim = animInst;
        animInst->nextAnim = enabledReferenceAnimations;
        enabledReferenceAnimations->lastAnim = animInst;
    } else {
        animInst->nextAnim = animInst;
        animInst->lastAnim = animInst;
    }

    enabledReferenceAnimations = animInst;
    animInst->enabled = true;
    numAnimationsEnabled++;
}


// The mirror of RefAnimationEnable: off the enabled ring, onto the
// disabled one, flag cleared, counter down.
void xModelInstance::RefAnimationDisable(RefInstanceAnimation* animInst) {
    if (!animInst->enabled) {
        return;
    }

    if (animInst->nextAnim == animInst) {
        enabledReferenceAnimations = 0;
    } else {
        animInst->lastAnim->nextAnim = animInst->nextAnim;
        animInst->nextAnim->lastAnim = animInst->lastAnim;

        if (animInst == enabledReferenceAnimations) {
            enabledReferenceAnimations = animInst->nextAnim;
        }
    }

    if (disabledReferenceAnimations != 0) {
        animInst->lastAnim = disabledReferenceAnimations->lastAnim;
        disabledReferenceAnimations->lastAnim->nextAnim = animInst;
        animInst->nextAnim = disabledReferenceAnimations;
        disabledReferenceAnimations->lastAnim = animInst;
    } else {
        animInst->nextAnim = animInst;
        animInst->lastAnim = animInst;
    }

    disabledReferenceAnimations = animInst;
    animInst->enabled = false;
    numAnimationsEnabled--;
}


// The length handed to memset is `count`, not count * sizeof: that is
// what the image does and it is reproduced, not corrected.
void xModelUniqueRefAnimationPoolInit(unsigned int count) {
    memset(xModelInstance::uniqueRefAnimations, 0, count);

    xModelInstance::uniqueRefAnimationsAvailable =
        xModelInstance::uniqueRefAnimations;
    xModelInstance::uniqueRefAnimationsUsed = 0;

    xModelInstance::RefUniqueAnimation* previous =
        xModelInstance::uniqueRefAnimations;

    previous->previous = 0;

    for (unsigned int c = 1; c < count; c++) {
        xModelInstance::RefUniqueAnimation* cur =
            &xModelInstance::uniqueRefAnimations[c];

        cur->previous = previous;
        previous->next = cur;
        previous = cur;
    }

    xModelInstance::uniqueRefAnimations[count - 1].next = 0;
}

// One instance per reference the prototype declares, chained into a
// ring; any allocation failing throws the whole chain away.
int World::xOGModel::AllocAnimationInstances() {
    RefInstanceAnimation* recent;
    RefInstanceAnimation* newInst;
    unsigned int cmax;

    numAnimationsEnabled = 0;

    RefInstanceAnimation* first =
        (RefInstanceAnimation*)xMemPoolAlloc(&refAnimPool, 1);

    nextReferenceAnimationLODUpdate = first;
    enabledReferenceAnimations = first;
    referenceAnimations = first;
    disabledReferenceAnimations = 0;

    if (first == 0) {
        return 0;
    }

    referenceAnimations->Clear();
    referenceAnimations->refModelProtoInstanceNum = 0;
    numAnimationsEnabled++;
    recent = referenceAnimations;
    cmax = mModelArt.model.modelProto->refInstanceCount - 1;

    for (unsigned int c = 0; c < cmax; c++) {
        newInst = (RefInstanceAnimation*)xMemPoolAlloc(&refAnimPool, 1);

        if (newInst == 0) {
            DeallocAnimationInstances();
            return 0;
        }

        newInst->Clear();
        newInst->refModelProtoInstanceNum = c + 1;
        numAnimationsEnabled++;
        recent->nextAnim = newInst;
        newInst->lastAnim = recent;
        recent->next = newInst;
        recent = newInst;
    }

    newInst->nextAnim = referenceAnimations;
    referenceAnimations->lastAnim = newInst;

    return 1;
}

// Every reference model before this one contributes its instances to
// the offset, and the answer is that many links down the chain.
xModelInstance::RefInstanceAnimation* World::xOGModel::GetRefAnimation(
    unsigned long long refId, unsigned short refInstanceOffset) {
    unsigned short refModelIndex = 0;

    if (mModelArt.model.GetReferenceModel(refId, refModelIndex) == 0) {
        return 0;
    }

    Graphics::ModelPrototype* modelProto = &mModelArt.protoEnt->modelProto;
    unsigned short refModelIndexCount;

    for (refModelIndexCount = 0; refModelIndexCount < refModelIndex;
         refModelIndexCount++) {
        Graphics::ReferenceModelEntry* refEntry =
            modelProto->GetReferenceModelEntry(refModelIndexCount);

        refInstanceOffset += refEntry->numInstances;
    }

    RefInstanceAnimation* animInst = referenceAnimations;

    for (int c = 0; c < refInstanceOffset; c++) {
        if (animInst == 0) {
            break;
        }

        animInst = animInst->next;
    }

    return animInst;
}


// Everything the instance owns moves across and the source is left
// holding nothing; the play, if there is one, is re-pointed at the new
// owner and re-counted. The member order below is the store order in
// the image, which is what decides the schedule.
// NOT MATCHING: 220 against retail's 260 and all 55 words differ, so
// the shape is wrong rather than the order. The member list below is
// read off the store addresses and is not in doubt; what is missing is
// whatever makes retail spend another 40 bytes.
void World::xOGModel::SwapXModel(World::xOGModel& src) {
    Mat = src.Mat;
    Scale = src.Scale;
    Anim = src.Anim;
    Flags = src.Flags;
    renderCustomizerMask = src.renderCustomizerMask;
    visModel = src.visModel;
    boundRefModelInstanceAnimationCount =
        src.boundRefModelInstanceAnimationCount;
    referenceAnimations = src.referenceAnimations;
    numAnimationsEnabled = src.numAnimationsEnabled;
    nextReferenceAnimationLODUpdate = src.nextReferenceAnimationLODUpdate;
    disabledReferenceAnimations = src.disabledReferenceAnimations;
    enabledReferenceAnimations = src.enabledReferenceAnimations;

    if (Anim != 0) {
        Anim->ModelInst = this;
        Anim->BoneCount = xModelGetBoneCount(this);
        Anim->MorphCount = xModelGetMorphCount(this);
    }

    src.Anim = 0;
    src.numAnimationsEnabled = 0;
    src.enabledReferenceAnimations = 0;
    src.disabledReferenceAnimations = 0;
    src.nextReferenceAnimationLODUpdate = 0;
    src.referenceAnimations = 0;
}

// -- generated accessor part (gen_accessors.py) --------------------

void Graphics::Renderable3D::SetLightKit(const Graphics::LightKitData* value) { f78 = (int)value; }
