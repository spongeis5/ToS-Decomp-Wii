// ImmediateInstanceArticle.cpp -- six functions, read from the image with
// tools/disasm.py. An immediate-instance article is the per-instance
// renderable parameter state of an immediate prototype: one buffer holding a
// ParamCargo, a parameter-data pointer and a parameter-data size for each of
// the asset's renderables, followed by the cargo and data buffers themselves.
//
// GetCreateInfo sizes that buffer without building it. When the asset names
// no prototype it reports nothing to allocate; otherwise it finds the
// prototype's handle, activates it, keeps its entity, and then walks the
// asset's renderable settings, asking ParamCargo::GetCreateInfo for each
// one's cargo size and taking each geometry's effect renderable-parameter
// data size, both accumulated 16-aligned. The total is the data size plus
// the 16-aligned sum of the three arrays and all of the cargo.
//
// Create builds it on a block the caller supplies: it keeps the count and
// the size, cuts the three arrays and the two buffers off one static stack
// allocator, opens a second and a third over the cargo and data buffers, and
// then for each renderable creates its ParamCargo, activates it against the
// entity handle and loads its parameter data, recording the data pointer and
// its 16-aligned size.
//
// Deactivate deactivates every cargo and then releases the prototype's own
// handle; BufferDestroy does that and frees the block to the entity pool
// through DeleteArray, whose pool enum comes by reference (the static
// temporary retail loads at @50055). BufferCreate is the whole sequence:
// a scratch array of ParamCargo::CreateInfo on the thread stack, the sizing
// pass, one global-heap block big enough for the article and everything
// GetCreateInfo asked for, the article placed on it, Create over the rest,
// and the scratch popped again. ApplyToRenderable copies one renderable's
// recorded parameter data into the renderable and re-initialises its local
// colour multiplier.
//
// Layouts from the DWARF (tools/dwarf_types.py): ImmediateInstanceArticle
// 0x18 -- rendCount, memSize, protoEnt, paramCargos, paramDatas, paramSizes;
// its CreateInfo 0x14 -- memSize, memAlign, protoEnt,
// allParamCargoBufferSize, allParamDataBufferSize; ImmediateInstanceAsset
// 0x10 with the prototype uid at +0, rendCount at +8 and rendSettings at
// +0xC; RenderableSettings 0x8 -- one byte of count, three of padding, then
// the parameter pointer; ShaderEntity::ParamCargo 0x8 and its CreateInfo 0x8
// -- bufferSize, textureCount; MaterialParam 0x8; Memory::StaticStackAllocator
// 0xC {mark, buffer, bufferEnd}; Entity 0x18 with the handle at +0x14 and
// EntityHandleBase 0x48 with its entity at +0x38; Graphics::Renderable 0x48
// with paramData at +0x14. The effect chain is three DWARF layouts end to
// end: ImmediateGeometryEntity 0x64 has builderData at +0x44 whose first
// member is the MaterialEntity, MaterialEntity 0x58 has its Material at
// +0x2C, Material 0x2C has its Effect at +0x1C -- which is retail's
// `lwz +0x44 ; lwz +0x48` -- and Effect's three ParamFormatTables sit at
// +0x28, so the renderable scope's table is params[2]: its formats at +0x40
// and its dataSize at +0x48, the two +0x40/+0x48 reads in the bytes.
//
// The DWARF types the two file-format pointers as its `Pointer32` wrapper;
// they are spelled as real pointers here, which is the same word and the
// same load.
//
// Six shapes the bytes fixed.
//
// Both zero tests are `if (id == 0) { nothing } else { the work }`: retail
// falls through into the empty case and branches over it, which is the `bne`
// to the far block, and the `!= 0` spelling with the arms swapped puts the
// work in the fall-through instead.
//
// GetCreateInfo's loop bound is a LOCAL read before the test -- retail keeps
// asset.rendCount in r30 across every call -- while Deactivate's is the
// member itself, reloaded on every iteration (`lwz r0,0(r29)` inside the
// loop). Create hoists it too, but out of `this->rendCount` after the store,
// not out of the asset: once the count is stored, every other read in that
// function goes back through the member.
//
// The zero the two accumulator stores use is the loop's own byte offset:
// retail materialises `li r31,0` once, stores it into both fields and then
// walks it by eight. Writing the two zeros as a separate statement pair
// after the loop variable is declared is what lets the allocator share it.
//
// The three arrays are cut with `n * sizeof(ParamCargo) + n * sizeof(void*)
// + n * sizeof(int)` and NOT with `n * 16`: retail shifts by three, shifts
// by two, and adds the second shift twice, which is the three terms with the
// common subexpression folded, where a single multiply is one `slwi`.
//
// `params` -- the renderable's MaterialParam pointer -- is a named local
// read once before the geometry call and kept in r29 across all three cargo
// calls, while `rendParamCount` is re-read with an `lbz` at each of the
// three: a char-typed read does not survive the stores between them.
//
// And BufferCreate's `cmpwi r30,0` with nothing branching on it is the
// placement new's own null test, whose branch folds away because the class
// has an EMPTY INLINE constructor -- the same shape zStoryMoment recorded.
// The scratch array is freed by popping the thread stack directly, with no
// null test, so it is not the DeleteArray shape; the size is recomputed from
// the asset, as retail's second `lhz` says.
//
// NEAR MISS, two of the seven, neither carried further -- the session
// that wrote this unit was cut off. Create is 91 of 118 words and
// GetCreateInfo 55 of 79. Nothing here records a spelling that was tried
// and rejected, because none was.
//

extern "C" void* memcpy(void* dst, const void* src, unsigned long n);

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {

enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };
enum EntityPoolEnum { EntityPool = 0 };
enum ThreadStackEnum { ThreadStack = 0 };

void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap, eMemMgrTag tag,
                      bool clear);

void FreeGlobalHeap(void* block, GlobalHeapEnum heap);

// The entity pool frees to the global heap; the image's instance of
// DeleteArray for it is a null test and this tail call.
inline void Free(void* block, EntityPoolEnum) {
    FreeGlobalHeap(block, (GlobalHeapEnum)0);
}

// The DWARF gives StackAllocator and StaticStackAllocator the same three
// words and puts one inside the other; the bytes only say that a
// StaticStackAllocator is what PushAlign is called on, so the derivation is
// written that way round. Either way the base is at offset zero.
class StackAllocator {
public:
    void PushAlign(unsigned long align);

    void Pop(unsigned long size) { mark -= (size + 15) & ~15; }

    unsigned char* mark;
    unsigned char* buffer;
    unsigned char* bufferEnd;
};

class StaticStackAllocator : public StackAllocator {
public:
    void Create(void* buffer, int size);

    void* Alloc(int size) {
        void* p = mark;
        mark += size;
        return p;
    }
};

StackAllocator* GetThreadStackAllocator();

}  // namespace Memory

inline void* operator new(unsigned long, void* p) { return p; }

// The thread-stack allocating operator new. Its heap argument reaches the
// call as a LOAD, not an immediate, which is the enumerator bound to a static
// temporary by NewArray's const reference.
void* operator new(unsigned long size, Memory::ThreadStackEnum heap,
                   eMemMgrTag tag);

template <class T, class H>
inline T* NewArray(const H& heap, eMemMgrTag tag, unsigned long count) {
    return (T*)operator new(count * sizeof(T), heap, tag);
}

template <class H, class T>
void DeleteArray(const H& heap, T* array, unsigned long count) {
    if (array) {
        Memory::Free(array, heap);
    }
}

namespace Util {

// Empty, and passed as a named default-initialised local.
class Referrer {};

}  // namespace Util

typedef unsigned long long uid;

namespace Graphics {

class Renderable {
public:
    void InitLocalColorMultiplier();

    unsigned char _pad0[0x14];
    void* paramData;
    unsigned char _pad1[0x48 - 0x18];
};

class Effect {
public:
    class ParamData;
    class ParamFormat;

    class ParamFormatTable {
    public:
        ParamFormat* formats;
        int count;
        int dataSize;
    };

    unsigned char _pad0[0x28];
    ParamFormatTable params[3];
    unsigned char _pad1[0x74 - 0x4C];
};

}  // namespace Graphics

namespace World {

class EntityHandleBase;
class TextureResourceEntity;

class Entity {
public:
    unsigned char _pad0[0x14];
    EntityHandleBase* handle;
};

class EntityHandleBase {
public:
    void Activate(const Util::Referrer& referrer);
    void Deactivate(const Util::Referrer& referrer);

    unsigned char _pad0[0x38];
    Entity* entity;
};

class EntityManager {
public:
    static EntityHandleBase* FindHandle(uid id);
};

// The file format's parameter descriptor, 8 bytes.
class MaterialParam {
public:
    unsigned char type;
    unsigned char debugIndex;
    unsigned short elementCount;
    void* value;
};

class Material {
public:
    unsigned char _pad0[0x1C];
    Graphics::Effect* effect;
    unsigned char _pad1[0x2C - 0x20];
};

class MaterialEntity {
public:
    unsigned char _pad0[0x2C];
    Material material;
};

class ImmediateGeometryEntity {
public:
    class BuilderData {
    public:
        MaterialEntity* materialEnt;
        unsigned char _pad0[0x1C - 0x4];
    };

    unsigned char _pad0[0x44];
    BuilderData builderData;
    unsigned int vertexDeclHash;
};

class ImmediatePrototypeEntity : public Entity {
public:
    ImmediateGeometryEntity* GetImmediateGeometry(int index);

    int memSize;
    int geomCount;
};

class ShaderEntity {
public:
    class ParamCargo {
    public:
        class CreateInfo {
        public:
            int bufferSize;
            int textureCount;
        };

        static void GetCreateInfo(CreateInfo& info, const MaterialParam* params,
                                  int count);

        void Create(void* mem, const CreateInfo& info,
                    const MaterialParam* params, int count);
        void Activate(EntityHandleBase* handle, const MaterialParam* params,
                      int count);
        void Deactivate();

        template <class F>
        void LoadParamData(Graphics::Effect::ParamData* data,
                           const MaterialParam* params, int count,
                           const F* formats);

        int textureCount;
        TextureResourceEntity** textureEnts;
    };
};

// One renderable's parameters in the asset: the DWARF's RenderableSettings,
// whose pointer it types as its Pointer32 wrapper.
class RenderableSettings {
public:
    unsigned char rendParamCount;
    unsigned char pad[3];
    MaterialParam* rendParams;
};

class ImmediateInstanceAsset {
public:
    uid immediatePrototypeID;
    unsigned short rendCount;
    unsigned short pad;
    RenderableSettings* rendSettings;
};

class ImmediateInstanceArticle {
public:
    class CreateInfo {
    public:
        int memSize;
        int memAlign;
        ImmediatePrototypeEntity* protoEnt;
        int allParamCargoBufferSize;
        int allParamDataBufferSize;
    };

    // Empty and inline: BufferCreate's placement new keeps the compare and
    // folds the branch away.
    ImmediateInstanceArticle() {}

    static void GetCreateInfo(CreateInfo& info,
                              ShaderEntity::ParamCargo::CreateInfo* cargoInfos,
                              EntityHandleBase* handle,
                              const ImmediateInstanceAsset& asset);

    void Create(void* mem, const CreateInfo& info,
                const ShaderEntity::ParamCargo::CreateInfo* cargoInfos,
                EntityHandleBase* handle, const ImmediateInstanceAsset& asset);

    void Deactivate();

    static ImmediateInstanceArticle* BufferCreate(
        EntityHandleBase* handle, const ImmediateInstanceAsset& asset);
    static void BufferDestroy(ImmediateInstanceArticle* article);

    void ApplyToRenderable(Graphics::Renderable* rend, int index);

    int rendCount;
    int memSize;
    ImmediatePrototypeEntity* protoEnt;
    ShaderEntity::ParamCargo* paramCargos;
    void** paramDatas;
    int* paramSizes;
};

}  // namespace World

void World::ImmediateInstanceArticle::GetCreateInfo(
    CreateInfo& info, ShaderEntity::ParamCargo::CreateInfo* cargoInfos,
    EntityHandleBase* handle, const ImmediateInstanceAsset& asset) {
    int rendCount = asset.rendCount;

    if (asset.immediatePrototypeID == 0) {
        info.memSize = 0;
        info.memAlign = 1;
        info.protoEnt = 0;
    } else {
        EntityHandleBase* found =
            EntityManager::FindHandle(asset.immediatePrototypeID);

        Util::Referrer referrer;

        found->Activate(referrer);

        info.protoEnt = (ImmediatePrototypeEntity*)found->entity;

        RenderableSettings* rendSettings = asset.rendSettings;

        info.allParamCargoBufferSize = 0;
        info.allParamDataBufferSize = 0;

        for (int i = 0; i < rendCount; i++) {
            ShaderEntity::ParamCargo::GetCreateInfo(
                cargoInfos[i], rendSettings[i].rendParams,
                rendSettings[i].rendParamCount);

            info.allParamCargoBufferSize += cargoInfos[i].bufferSize;

            info.allParamDataBufferSize =
                ((info.allParamDataBufferSize + 15) & ~15) +
                ((info.protoEnt->GetImmediateGeometry(i)
                      ->builderData.materialEnt->material.effect->params[2]
                      .dataSize +
                  15) &
                 ~15);
        }

        int arraySize = rendCount * sizeof(ShaderEntity::ParamCargo);

        arraySize += rendCount * sizeof(void*);
        arraySize += rendCount * sizeof(int);

        info.memAlign = 16;
        info.memSize = info.allParamDataBufferSize +
                       ((((arraySize + 3) & ~3) +
                         info.allParamCargoBufferSize + 15) &
                        ~15);
    }
}

void World::ImmediateInstanceArticle::Create(
    void* mem, const CreateInfo& info,
    const ShaderEntity::ParamCargo::CreateInfo* cargoInfos,
    EntityHandleBase* handle, const ImmediateInstanceAsset& asset) {
    if (asset.immediatePrototypeID == 0) {
        protoEnt = 0;
    } else {
        Memory::StaticStackAllocator alloc;
        Memory::StaticStackAllocator cargoAlloc;
        Memory::StaticStackAllocator dataAlloc;

        rendCount = asset.rendCount;
        memSize = info.memSize;

        alloc.Create(mem, info.memSize);

        paramCargos = (ShaderEntity::ParamCargo*)alloc.Alloc(
            rendCount * sizeof(ShaderEntity::ParamCargo));
        paramDatas = (void**)alloc.Alloc(rendCount * sizeof(void*));
        paramSizes = (int*)alloc.Alloc(rendCount * sizeof(int));

        void* cargoMem = alloc.Alloc(info.allParamCargoBufferSize);

        alloc.PushAlign(16);

        void* dataMem = alloc.Alloc(info.allParamDataBufferSize);

        protoEnt = info.protoEnt;

        cargoAlloc.Create(cargoMem, info.allParamCargoBufferSize);
        dataAlloc.Create(dataMem, info.allParamDataBufferSize);

        RenderableSettings* rendSettings = asset.rendSettings;
        int count = rendCount;

        for (int i = 0; i < count; i++) {
            const MaterialParam* params = rendSettings[i].rendParams;
            Graphics::Effect* effect = protoEnt->GetImmediateGeometry(i)
                                           ->builderData.materialEnt->material
                                           .effect;
            int paramDataSize = (effect->params[2].dataSize + 15) & ~15;
            ShaderEntity::ParamCargo* cargo = &paramCargos[i];
            const ShaderEntity::ParamCargo::CreateInfo& cargoInfo =
                cargoInfos[i];

            void* paramCargoMem = cargoAlloc.Alloc(cargoInfo.bufferSize);

            dataAlloc.PushAlign(16);

            Graphics::Effect::ParamData* paramDataMem =
                (Graphics::Effect::ParamData*)dataAlloc.Alloc(paramDataSize);

            cargo->Create(paramCargoMem, cargoInfo, params,
                          rendSettings[i].rendParamCount);
            cargo->Activate(handle, params, rendSettings[i].rendParamCount);
            cargo->LoadParamData(paramDataMem, params,
                                 rendSettings[i].rendParamCount,
                                 effect->params[2].formats);

            paramDatas[i] = paramDataMem;
            paramSizes[i] = paramDataSize;
        }
    }
}

void World::ImmediateInstanceArticle::Deactivate() {
    if (protoEnt) {
        for (int i = 0; i < rendCount; i++) {
            paramCargos[i].Deactivate();
        }

        Util::Referrer referrer;

        protoEnt->handle->Deactivate(referrer);
    }
}

World::ImmediateInstanceArticle* World::ImmediateInstanceArticle::BufferCreate(
    EntityHandleBase* handle, const ImmediateInstanceAsset& asset) {
    ShaderEntity::ParamCargo::CreateInfo* cargoInfos =
        NewArray<ShaderEntity::ParamCargo::CreateInfo, Memory::ThreadStackEnum>(
            Memory::ThreadStack, (eMemMgrTag)60, asset.rendCount);

    CreateInfo info;
    Memory::StaticStackAllocator alloc;

    GetCreateInfo(info, cargoInfos, handle, asset);

    int size = info.memSize +
               ((sizeof(ImmediateInstanceArticle) + info.memAlign - 1) &
                ~(info.memAlign - 1));

    alloc.Create(Memory::AllocGlobalHeap(size, (Memory::GlobalHeapEnum)0,
                                         (eMemMgrTag)60, false),
                 size);

    ImmediateInstanceArticle* article =
        new (alloc.Alloc(sizeof(ImmediateInstanceArticle)))
            ImmediateInstanceArticle;

    alloc.PushAlign(info.memAlign);

    article->Create(alloc.Alloc(info.memSize), info, cargoInfos, handle, asset);

    Memory::GetThreadStackAllocator()->Pop(
        asset.rendCount * sizeof(ShaderEntity::ParamCargo::CreateInfo));

    return article;
}

void World::ImmediateInstanceArticle::BufferDestroy(
    ImmediateInstanceArticle* article) {
    article->Deactivate();

    DeleteArray(Memory::EntityPool, (unsigned char*)article, article->memSize);
}

void World::ImmediateInstanceArticle::ApplyToRenderable(
    Graphics::Renderable* rend, int index) {
    memcpy(rend->paramData, paramDatas[index], paramSizes[index]);

    rend->InitLocalColorMultiplier();
}
