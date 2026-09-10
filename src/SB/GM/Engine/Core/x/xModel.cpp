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

class xAnimFile {
public:
    xAnimFile* Next;
    char* Name;
    unsigned int ID;
    unsigned int FileFlags;
    float Duration;
    float TimeOffset;
    float TexMergeStartTime;
    float TexMergeEndTime;
    unsigned long long TexMergeTexture;
    unsigned long long TexMergeSpecularTexture;
    unsigned short BoneCount;
    unsigned char MorphCount;
    unsigned char pad1[1];
    unsigned char NumAnims[3];
    unsigned char pad2[1];
    void** RawData;
};

namespace Math {

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
    Matrix33();

    Vector4 v[3];
};

class Matrix43 : public Matrix33 {
public:
    Matrix43& operator=(const Matrix43& other);
};

class Vector {
public:
    Vector(float x, float y, float z);

    float x;
    float y;
    float z;
};

enum HintInvertibleEnum { HintInvertibleEnum_ = 0x7FFFFFFF };

void Invert(Matrix43& out, const Matrix43& in, HintInvertibleEnum hint);

}  // namespace Math


namespace Graphics {
class TextureResourceEntity;
class ShadowSimpleCache;
class ModelPrototypeEntity;
class LightKitEntity;
class RenderCustomizerInfo;
class ModelXformBuffer;
class ModelJointBuffer {
public:
    Math::Matrix43* joints;
};

class SkinCluster {
public:
    unsigned char _pad0[0x8];
    Math::Matrix43* skinToBone;
};
class ModelMorphWeightBuffer;
class ModelPrototype;
class Geometry;
class RefModelInstanceChildXform;
class Renderable3D;
class LightKit;
class LightKitData {
public:
    unsigned char _pad0[0x80];
};
class CollisionMeshBlobEntity;
class PerInstanceData;
class Builder;
class ModelInstanceParamData;
class ModelPartDefinition;
class SkeletonBlobEntity;
class Scene;
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
    void HidePart(Scene* scene, unsigned int part);
    void ShowPart(Scene* scene, unsigned int part);
    void Hide(Scene* scene);
    void SetChildTransform(int index, const Math::Matrix43& xform);
    void CommitWorldTransformAttached();

    Math::Matrix43 rootTransform;
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
    Math::Matrix43* childTransforms;
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
    Math::Matrix43* childTransforms;
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
    SkinCluster* skins;
    int skinCount;
    int skinJointTotal;
    int morphWeightTotal;
    signed char* transformToGroupMap;
    bool userScale;
    unsigned char _pad0[0x3C - 0x39];
    Math::Matrix43* hackEvalResults;
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

class LightKit : public Node {
public:
    LightKitData data;
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
        xAnimFile animFile;
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

    static float referenceAnimationLODCurrentFPS;
    static float referenceAnimationLODMinFPS;
    static unsigned int referenceAnimationLimit;
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

class xRefModelAnimationData {
public:
    unsigned long long animationID;
    unsigned long long referenceID;
    unsigned int firstBoundInstance;
    int numBoundInstances;
    bool looping;
    bool randomizeStartTime;
};

class EmbeddedListNode {
public:
    EmbeddedListNode* next;
    EmbeddedListNode* prev;
};

template <class T, int OFFSET>
class EmbeddedList {
public:
    void Remove(T* item);

    EmbeddedListNode head;
    unsigned long size;
};

namespace World {

class xOGModel;

class WorldPrivate {
public:
    static Graphics::Scene* primaryScene;
};

class EntityManager {
public:
    // STATIC: retail calls GetEntityManager(), then overwrites r3 with
    // the id before the call, so the object expression is evaluated and
    // thrown away -- which is what calling a static member through one
    // does.
    static void* FindAsset(unsigned long long id);
};

EntityManager* GetEntityManager();

class ModelInstanceArticle {
public:
    void Detach();
    void Deactivate();

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

    EmbeddedList<xOGModel, 356> updList;
};

class xOGModel : public xModelInstance {
public:
    void UpdateRender();
    void SwapXModel(xOGModel& src);
    unsigned short BindRefModelAnimation(
        const xRefModelAnimationData& animBindData);
    unsigned short UnbindRefModelAnimation(unsigned long long refId,
                                           unsigned short firstInstanceIndex,
                                           int numInstances);
    void DeferDestroy();
    void Destroy();
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
    Math::Vector4 colorMultiplier;
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
void xAnimPoolFree(xAnimPlay* play);
void xMat4x3FromNGMatrix(xMat4x3* out, const Math::Matrix43* in);
void xMat3x3Mul(xMat3x3* o, const xMat3x3* a, const xMat3x3* b);
void v3add(xVec3* o, xVec3* a, xVec3* b);

extern "C" void PSMTXConcat(const Math::Matrix43* a,
                            const Math::Matrix43* b,
                            Math::Matrix43* ab);

extern "C" void* memset(void* dst, int val, unsigned long len);
extern "C" double ceil(double x);

unsigned int xStrHash(const char* s);
unsigned int xrand_GenRandInt32();
void xAnimFileNewBilinearPrealloc(xAnimFile* file, void** buf,
                                  const char* name, unsigned int hash,
                                  unsigned int a, xAnimFile** b,
                                  unsigned int c, unsigned int d,
                                  unsigned int e, const char* f);

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

// The three class statics get real definitions so each is its own
// symbol: the image has the two initialised ones in .data at 806B4024
// and 806B4028 and the running average in .bss, and addresses them
// with a `lis` apiece. Declared only, mwcc reaches all three off one
// base with offsets 0, 4 and 8.
float xModelInstance::referenceAnimationLODCurrentFPS;
float xModelInstance::referenceAnimationLODMinFPS = 30.0f;
unsigned int xModelInstance::referenceAnimationLimit = 0xFFFFFFFF;

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

// NEAR MISS, exact size, 9 of 44 words: refInstanceOffset, modelProto
// and refModelIndexCount are a three-way rotation of retail's r31, r30
// and r29. Naming the entry local as the debug info does changes
// nothing, and neither does the walk's shape -- that part is right now.
//
// Every reference model before this one contributes its instances to
// the offset, and the answer is that many links down the chain.
xModelInstance::RefInstanceAnimation* World::xOGModel::GetRefAnimation(
    unsigned long long refId, unsigned short refInstanceOffset) {
    unsigned short refModelIndex = 0;

    // The entry is a NAMED local in the debug info, between
    // refModelIndex and modelProto; inlined into the test the three
    // callee-saved registers come out rotated.
    Graphics::ReferenceModelEntry* refModelEntry =
        mModelArt.model.GetReferenceModel(refId, refModelIndex);

    if (refModelEntry == 0) {
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

    // A `while` with both conditions, not a `for` with a `break`: the
    // break form lets mwcc count the loop with `mtctr`/`bdnz`, where
    // retail keeps a real counter and tests it with `cmpw`.
    int c = 0;

    while (c < refInstanceOffset && animInst != 0) {
        animInst = animInst->next;
        c++;
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


// NEAR MISS, 332 against 336: ONE instruction short, and it is the
// second guard. Retail spells it `beq +8` into the body followed by an
// unconditional branch out; every spelling here collapses to a single
// inverted `bne`. Four are already excluded, do not redo them:
//     if (disable) return;
//     if (disable) return; else { loop }
//     if (disable == 0) { loop }
//     if (visCount != 0 && !disable) { loop }
// Everything after that one word is identical, so the 74 differing
// words are the shift it causes and not 74 differences.
//
// AND TWO THINGS THAT WERE WRONG AND ARE NOW RIGHT. Assigning the
// struct -- visParts[j] = visParts[n] -- makes mwcc call an
// out-of-line copy helper for a two-byte POD, and spelling the
// subscript twice makes it recompute the index twice; retail reads one
// address and spells two lbz/stb pairs off it.
//
// Four states, and the two masks say which: 0x24 with 0x10 clear means
// hide, 0x11 means show, and the bare 0x08 / 0x02 cases only settle the
// flags. A part whose flags come out zero is finished with, so the last
// one is swapped into its slot and the count drops.
//
// NO `part` OR `state` LOCAL. The debug info names only visModel and j,
// and writing the subscript out each time is what puts the address in
// r30 and the byte in r3 the way retail has them; hoisting either into
// a variable of its own shifts every callee-saved register one slot.
void xModelUpdatePartsVis(World::xOGModel* modelInst) {
    xModelInstance::ModelVisibility* visModel = &modelInst->visModel;

    if (visModel->visCount != 0 && !visModel->disableVisibilityAnim) {
        for (unsigned char j = 0; j < visModel->visCount; j++) {
            if ((visModel->visParts[j].stateMask & 0x24) && !(visModel->visParts[j].stateMask & 0x10)) {
                if (!(visModel->visParts[j].stateMask & 0x08)) {
                modelInst->mModelArt.model.HidePart(
                    World::WorldPrivate::primaryScene, visModel->visParts[j].partIndex);
                }

                visModel->visParts[j].stateMask = (visModel->visParts[j].stateMask & 0xF8) | 0x08;
            } else if (visModel->visParts[j].stateMask & 0x11) {
                if (!(visModel->visParts[j].stateMask & 0x02)) {
                modelInst->mModelArt.model.ShowPart(
                    World::WorldPrivate::primaryScene, visModel->visParts[j].partIndex);
                }

                visModel->visParts[j].stateMask = (visModel->visParts[j].stateMask & 0xF2) | 0x02;
            } else if (visModel->visParts[j].stateMask & 0x08) {
                modelInst->mModelArt.model.ShowPart(
                    World::WorldPrivate::primaryScene, visModel->visParts[j].partIndex);
                visModel->visParts[j].stateMask = 0;
            } else if (visModel->visParts[j].stateMask & 0x02) {
                visModel->visParts[j].stateMask = 0;
            }

            if (visModel->visParts[j].stateMask == 0) {
                // Field by field, off ONE address: assigning the struct
            // makes mwcc call a copy helper, and spelling the subscript
            // twice makes it recompute the index twice.
            xModelInstance::PartsVisibility* last =
                &visModel->visParts[visModel->visCount - 1];

            visModel->visParts[j].partIndex = last->partIndex;
            visModel->visParts[j].stateMask = last->stateMask;
                visModel->visCount--;
                j--;
            }
        }
    }
}



// NEAR MISS, exact size, 8 of 35 words. What is left is the SCHEDULE
// inside the loop: retail loads `renderables` first and interleaves the
// three constant words with it, taking r6/r5/r0; ours completes the
// twelve-byte copy first and so has r3 still free, taking r5/r3/r0.
// Two spellings are excluded -- the declaration inside the loop and the
// declaration outside with the assignment inside -- and both give the
// same eight words.
//
// The pointer-to-member call: mwcc puts a twelve-byte constant in
// .rodata, copies it to the stack and branches to __ptmf_scall. The
// constant in the image reads delta 0, vtable offset -1 and
// Renderable3D::SetLightKit, so that is what is taken here. NOTES.md
// records that the declaration has to be its OWN statement.
void xModelSetLightKit(World::xOGModel* minst,
                       const Graphics::LightKit* lightKit) {
    if (&minst->mModelArt != 0) {
        void (Graphics::Renderable3D::*fn)(const Graphics::LightKitData*);

        // Retail re-copies the twelve bytes at every call and hoists
        // only the ADDRESS of the constant, so the ASSIGNMENT is
        // per-iteration even though the declaration is not.
        for (int i = 0; i < minst->mModelArt.model.renderableCount; i++) {
            fn = &Graphics::Renderable3D::SetLightKit;

            (minst->mModelArt.model.renderables[i]->*fn)(&lightKit->data);
        }
    }
}

// Everything the model holds is given back and then the object destroys
// itself through a pointer to its own member -- the constant in the
// image names xOGModel::Destroy, delta 0 and not virtual.
void World::xOGModel::DeferDestroy() {
    mModelArt.model.Hide(World::WorldPrivate::primaryScene);
    mModelArt.Detach();
    mModelArt.Deactivate();

    if (updateNode.prev != 0) {
        updater->updList.Remove(this);
        updateNode.prev = 0;
    }

    if (Anim != 0) {
        xAnimPoolFree(Anim);
        Anim = 0;
    }

    RefInstanceAnimation* animInst = referenceAnimations;

    while (animInst != 0) {
        RefUniqueAnimation* uniqueAnimEntry =
            GetRefAnimationEntry(animInst->animFile);

        if (uniqueAnimEntry != 0) {
            RemoveRefAnimation(uniqueAnimEntry, 1);
        }

        xMemPoolFree(&refAnimPool, animInst);
        animInst = animInst->next;
    }

    numAnimationsEnabled = 0;
    enabledReferenceAnimations = 0;
    disabledReferenceAnimations = 0;
    nextReferenceAnimationLODUpdate = 0;
    referenceAnimations = 0;

    void (World::xOGModel::*fn)();

    fn = &World::xOGModel::Destroy;

    (this->*fn)();
}


// A negative count means `all the instances from here on`. Each
// instance that still holds an animation gives its reference back, has
// its child transforms reset from the prototype, and is counted; when
// the model is holding none at all the instances themselves go too.
// The count comes back UNMASKED in retail -- `mr r3,r28`, not a
// `rlwinm` -- so the return type is the counter's own unsigned short
// and not an int it would have to be widened into.
unsigned short World::xOGModel::UnbindRefModelAnimation(
    unsigned long long refId, unsigned short firstInstanceIndex,
    int numInstances) {
    unsigned short refModelIndex = 0;
    Graphics::ReferenceModelEntry* refModelEntry =
        mModelArt.model.GetReferenceModel(refId, refModelIndex);

    // Read BEFORE the null check: retail's `lhz` sits above the branch,
    // so the assignment is above it in source too.
    int numRefChildTransforms = refModelEntry->numChildTransforms;

    if (refModelEntry == 0) {
        return 0;
    }

    if (numInstances < 0) {
        numInstances = refModelEntry->numInstances - firstInstanceIndex;
    }

    RefInstanceAnimation* refInstanceAnimation =
        GetRefAnimation(refId, firstInstanceIndex);
    unsigned short numSuccessfullyUnbound = 0;

    for (unsigned short instance = firstInstanceIndex;
         instance < firstInstanceIndex + (unsigned short)numInstances;
         instance++) {
        xAnimFile* animFile = refInstanceAnimation->animFile;

        if (animFile != 0) {
            RemoveRefAnimation(GetRefAnimationEntry(animFile), 1);
            boundRefModelInstanceAnimationCount--;
            numSuccessfullyUnbound++;
            refInstanceAnimation->animFile = 0;

            Math::Matrix43* modelProtoChildTransforms =
                mModelArt.model.modelProto->childTransforms;
            int refInstanceChildTransformOffset =
                refModelEntry->childTransformOffset
                + refModelEntry->numChildTransforms * instance;

            // The explicit guard is retail's: it spells `cmpwi`/`ble`
            // AND the loop's own bottom test, which a bare `for` does
            // not.
            int i = 0;

            if (numRefChildTransforms > 0) {
                for (; i < numRefChildTransforms; i++) {
                    mModelArt.model.SetChildTransform(
                        refInstanceChildTransformOffset + i,
                        modelProtoChildTransforms[
                            refInstanceChildTransformOffset + i]);
                }
            }
        }

        refInstanceAnimation = refInstanceAnimation->next;
    }

    mModelArt.model.CommitWorldTransformAttached();

    if (boundRefModelInstanceAnimationCount == 0) {
        DeallocAnimationInstances();
    }

    return numSuccessfullyUnbound;
}


// The bone matrix is the joint transform composed with the inverse of
// the skin's bind pose, brought back into xMat4x3 and then moved into
// the root's frame. An index past the bone count just hands back the
// root.
void xModelGetBoneMatNoScale(xMat4x3& mat, const World::xOGModel& model,
                             unsigned long index) {
    const xMat4x3& root_mat = model.Mat;

    if (index >= (unsigned long)xModelGetBoneCount(&model)) {
        mat = root_mat;
    } else {
        // NEAR MISS, exact size, 4 of 56 words: r29 and r31 are the
        // wrong way round. Retail puts the byte offset in r29 --
        // overwriting the index, which is dead -- and the element
        // address in r31; ours keeps the index and uses r31 for the
        // product. Two orderings are excluded: declaring skinToBone
        // first costs 5 more words, and dropping both locals costs 44.
        //
        // The ELEMENT address is a local and the joint base is not:
        // retail computes the element address before the matrix is
        // constructed and keeps it across that call.
        Math::Matrix43* jointMatrices =
            model.mModelArt.model.joints->joints;
        Math::Matrix43* skinToBone =
            &model.mModelArt.protoEnt->skelBlob->skel.skins
                 ->skinToBone[index];
        Math::Matrix43 m;

        m = *skinToBone;
        Math::Invert(m, m, (Math::HintInvertibleEnum)0);
        PSMTXConcat(&jointMatrices[index], &m, &m);
        xMat4x3FromNGMatrix(&mat, &m);

        xVec3 offset;

        xMat3x3RMulVec(&offset, &root_mat, &mat.pos);
        v3add(&mat.pos, &offset, (xVec3*)&root_mat.pos);
        xMat3x3Mul(&mat, &mat, &root_mat);
    }
}

// The same body with the root supplied rather than taken from the
// model's own matrix.
void xModelGetBoneMatNoScale(xMat4x3& mat, const World::xOGModel& model,
                             unsigned long index, const xMat4x3& root) {
    if (index >= (unsigned long)xModelGetBoneCount(&model)) {
        mat = root;
    } else {
        // NEAR MISS, exact size, 4 of 56 words: r29 and r31 are the
        // wrong way round. Retail puts the byte offset in r29 --
        // overwriting the index, which is dead -- and the element
        // address in r31; ours keeps the index and uses r31 for the
        // product. Two orderings are excluded: declaring skinToBone
        // first costs 5 more words, and dropping both locals costs 44.
        //
        // The ELEMENT address is a local and the joint base is not:
        // retail computes the element address before the matrix is
        // constructed and keeps it across that call.
        Math::Matrix43* jointMatrices =
            model.mModelArt.model.joints->joints;
        Math::Matrix43* skinToBone =
            &model.mModelArt.protoEnt->skelBlob->skel.skins
                 ->skinToBone[index];
        Math::Matrix43 m;

        m = *skinToBone;
        Math::Invert(m, m, (Math::HintInvertibleEnum)0);
        PSMTXConcat(&jointMatrices[index], &m, &m);
        xMat4x3FromNGMatrix(&mat, &m);

        xVec3 offset;

        xMat3x3RMulVec(&offset, &root, &mat.pos);
        v3add(&mat.pos, &offset, (xVec3*)&root.pos);
        xMat3x3Mul(&mat, &mat, &root);
    }
}

// NOT MATCHING, 228 against 188. Retail calls Math::Vector's
// three-float constructor ON THE DESTINATION three times; every
// spelling here builds a temporary on the stack and copies it, which
// is ten instructions and a 96-byte frame instead of 48. Tried: the
// rows typed as Math::Vector so the assignment is Vector-to-Vector,
// and the same through a cast on the address. What is missing is
// whatever makes mwcc elide the temporary into the member.
//
// Each row of the result is the matching row of `m` scaled by one
// component, constructed in place -- the image calls Math::Vector's
// three-float constructor on the destination three times.
void xMat3x3MulScaleC(xMat3x3* o, const xMat3x3* m, float x, float y,
                      float z) {
    *(Math::Vector*)&o->left =
        Math::Vector(m->left.x * x, m->left.y * x, m->left.z * x);
    *(Math::Vector*)&o->up =
        Math::Vector(m->up.x * y, m->up.y * y, m->up.z * y);
    *(Math::Vector*)&o->at =
        Math::Vector(m->at.x * z, m->at.y * z, m->at.z * z);
}


// NOT MATCHING, 260 against 288 -- 28 bytes, seven instructions.
// Retail spells a `lis` for EVERY access to these four statics, even
// for the two that sit four bytes apart at 806B4024 and 806B4028;
// ours reaches three of them off one base with offsets 0, 4 and 8.
// Giving them real definitions, with the initialised pair in .data
// where the image has them, changes nothing. This is the same shared-
// base blocker WAD01_1_1.cpp records against its constructors.
//
// The FPS estimate is a one-pole filter -- fps = fps * (1 - dt) + 1 --
// which settles at the real frame rate. Every quarter second it either
// raises the reference-animation budget by one or cuts it by the
// fraction the frame rate is short by.
//
// `referenceAnimationLimit` is UNSIGNED: retail tests it against
// 0xFFFFFFFF with `addis r0,r3,1` / `cmplwi r0,0xFFFF`, which is what
// mwcc spells when a 32-bit constant will not fit `cmplwi`, and the
// image holds -1 there.
void xModelInstance::UpdateReferenceAnimationLODFPS(float timeDelta) {
    static float refAnimLimitRefreshTimeElapsed;

    referenceAnimationLODCurrentFPS =
        referenceAnimationLODCurrentFPS * (1.0f - timeDelta) + 1.0f;
    refAnimLimitRefreshTimeElapsed += timeDelta;

    if (refAnimLimitRefreshTimeElapsed > 0.25f) {
        if (referenceAnimationLODCurrentFPS >= referenceAnimationLODMinFPS) {
            if (referenceAnimationLimit != 0xFFFFFFFF) {
                referenceAnimationLimit++;
            }
        } else if (referenceAnimationLimit != 0) {
            float percentOverFPSLimit =
                referenceAnimationLODMinFPS - referenceAnimationLODCurrentFPS;

            if (referenceAnimationLODMinFPS > 0.0f) {
                percentOverFPSLimit /= referenceAnimationLODMinFPS;
            }

            unsigned int animationDecrementAmnt = (unsigned int)ceil(
                referenceAnimationLimit * percentOverFPSLimit);

            if (animationDecrementAmnt < referenceAnimationLimit) {
                referenceAnimationLimit -= animationDecrementAmnt;
            } else {
                referenceAnimationLimit = 0;
            }
        }

        refAnimLimitRefreshTimeElapsed = 0.0f;
    }
}


// NEAR MISS, 744 against 748, 69 of 186 words. Every differing word is
// the same one-register shift: retail runs on r25..r31 and ours on
// r26..r31, so retail keeps one more value live and pays one extra
// instruction for it. Three findings got it from 118 differing words
// to 69 and are worth keeping:
//
//   * EntityManager::FindAsset is STATIC. Retail calls
//     GetEntityManager() and then overwrites r3 with the id before the
//     call, which is what calling a static member through an object
//     expression does -- the expression is evaluated and discarded.
//   * The animation name is a LOCAL. Retail keeps the pooled string in
//     a callee-saved register across xStrHash and uses it on both
//     sides; spelled twice, mwcc materialises it twice.
//   * That local is declared AFTER the asset lookup, not before it.
//
// Bind one animation across a run of reference-model instances. The
// count that comes back is how many instances were actually reached,
// which is short of the run only if the chain ran out.
//
// The random start time is a uint32 scaled by 1/2^32 -- the 0x4330
// store and the 2^52 subtract are mwcc's unsigned-to-double, not data.
unsigned short World::xOGModel::BindRefModelAnimation(
    const xRefModelAnimationData& animBindData) {
    unsigned int totalNumInstancesBoundToUniqueAnim = 0;
    unsigned short modelProtoRefModelIndex = 0;
    Graphics::ReferenceModelEntry* refModelEntry =
        mModelArt.model.GetReferenceModel(animBindData.referenceID,
                                          modelProtoRefModelIndex);

    if (refModelEntry == 0) {
        return 0;
    }

    if (boundRefModelInstanceAnimationCount == 0) {
        AllocAnimationInstances();
    }

    RefInstanceAnimation* firstRefInstAnim =
        GetRefAnimation(animBindData.referenceID,
                        animBindData.firstBoundInstance);
    int numToBind = animBindData.numBoundInstances;

    if (numToBind < 0) {
        numToBind = refModelEntry->numInstances
                    - (unsigned short)animBindData.firstBoundInstance;
    }

    unsigned short numInstancesToBind = numToBind;
    RefUniqueAnimation* uniqueAnimEntry =
        GetRefAnimationEntry(animBindData.animationID);
    xAnimFile* uniqueRefAnimation =
        uniqueAnimEntry != 0 ? &uniqueAnimEntry->animFile : 0;

    if (uniqueRefAnimation != 0) {
        unsigned short numInstancesWithoutThisAnimation = 0;
        RefInstanceAnimation* refInstAnim = firstRefInstAnim;
        unsigned short refInstCount = animBindData.firstBoundInstance;

        while (refInstCount < numInstancesToBind && refInstAnim != 0) {
            if (refInstAnim->animFile != uniqueRefAnimation) {
                numInstancesWithoutThisAnimation++;
            }

            refInstAnim = refInstAnim->next;
            refInstCount++;
        }

        AddRefAnimation(animBindData.animationID,
                        numInstancesWithoutThisAnimation,
                        totalNumInstancesBoundToUniqueAnim);
    } else {
        uniqueRefAnimation = (xAnimFile*)AddRefAnimation(
            animBindData.animationID, numInstancesToBind,
            totalNumInstancesBoundToUniqueAnim);
    }

    if (uniqueRefAnimation == 0) {
        return 0;
    }

    if (totalNumInstancesBoundToUniqueAnim == numInstancesToBind) {
        // The name is a LOCAL: retail keeps the pooled string address
        // in a callee-saved register across the xStrHash call and uses
        // it on both sides. Spelled twice, mwcc materialises it twice
        // and needs one register fewer.
        void* buf =
            World::GetEntityManager()->FindAsset(animBindData.animationID);
        const char* animName = "";

        xAnimFileNewBilinearPrealloc(uniqueRefAnimation, &buf, animName,
                                     xStrHash(animName), 0, 0, 1, 1, 1, 0);
    }

    unsigned short instance = animBindData.firstBoundInstance;
    RefInstanceAnimation* refInstAnimation = firstRefInstAnim;

    while (instance
           < animBindData.firstBoundInstance + numInstancesToBind) {
        if (refInstAnimation == 0) {
            return instance - animBindData.firstBoundInstance;
        }

        if (refInstAnimation->animFile != 0
            && refInstAnimation->animFile != uniqueRefAnimation) {
            RefUniqueAnimation* bound =
                GetRefAnimationEntry(refInstAnimation->animFile);

            if (bound != 0) {
                RemoveRefAnimation(bound, 1);
            }
        } else if (refInstAnimation->animFile == 0) {
            boundRefModelInstanceAnimationCount++;
        }

        refInstAnimation->animFile = uniqueRefAnimation;
        refInstAnimation->looping = animBindData.looping;

        if (animBindData.randomizeStartTime) {
            refInstAnimation->time =
                uniqueRefAnimation->Duration
                * (xrand_GenRandInt32() * 2.3283064e-10f);
        } else {
            refInstAnimation->time = 0.0f;
        }

        refInstAnimation = refInstAnimation->next;
        instance++;
    }

    return instance - animBindData.firstBoundInstance;
}

// -- generated accessor part (gen_accessors.py) --------------------

void Graphics::Renderable3D::SetLightKit(const Graphics::LightKitData* value) { f78 = (int)value; }
