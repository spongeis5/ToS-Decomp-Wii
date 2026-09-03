// GeometryEntity.cpp -- six functions, read from the image with
// tools/disasm.py. A geometry entity is built in three steps against a
// caller-owned stack allocator, and these are the three plus the three
// out-of-line helpers the unit's first use of them emitted.
//
// ReserveBuilderData finds the asset's material handle, activates it,
// keeps its entity in the BuilderInfo and then MEASURES the block the
// builder will need: one BlobRef per vertex stream and per index LOD, the
// two parameter-cargo buffers ParamCargo::GetCreateInfo sizes, and the
// effect's two parameter-data blocks, each 16-aligned with the running
// total 16-aligned ahead of it -- the same sequence CreateBuilderData
// then performs against the real allocator.
//
// CreateBuilderData takes the two BlobRef arrays, copies the material
// entity and the two counts, and walks the asset's index LODs and vertex
// streams: each blob uid is looked up, the handle activated, the owner's
// back-pointer stored, the blob entity's defer flag set and the ref's
// owner linked into that entity's owner list. A stream whose uid is zero
// leaves the ref cleared instead; an index LOD is never optional, which
// is why only the second loop tests. Then the primitive type comes out of
// a table indexed by the asset's prim, the four buffers come off the
// allocator, and each of the two ParamCargos is created, activated and
// filled from the effect's format table.
//
// DeactivateBuilderData is the mirror: both cargos deactivated, every
// stream ref with a blob and every index-LOD ref unlinked and released,
// then the material handle released.
//
// AddOwner, LoadParamData<Effect::ParamFormat> and NewArray<BlobRef,
// StackAllocator> are not this file's code in retail -- the line table
// puts them in RawBlobEntity.h, ShaderEntity.h and Global.h -- but they
// are emitted here, between CreateBuilderData and DeactivateBuilderData,
// because CreateBuilderData is where they were first used. They are
// written the same way: defined in the class or as a template so the
// compiler emits them at that point rather than inlining them.
//
// Layouts from the DWARF (tools/dwarf_types.py), every offset recovered
// and none guessed: GeometryAsset 0x60 with materialID at +0x28,
// viewAttrib at +0x30, prim at +0x36, geomParamCount/rendParamCount/
// streamCount at +0x37/+0x38/+0x39, geomParams/rendParams/streams at
// +0x3C/+0x40/+0x44, lodCount at +0x54 and lods at +0x58; VertexStream
// 0x10 and IndexLOD 0x18, which are the 16 and 24 byte strides the two
// loops walk. GeometryEntity::BuilderInfo 0x28 {size, materialEnt,
// viewAttrib, geomParamData, rendParamData, primType, two ParamCargo::
// CreateInfo at +0x18 and +0x20}; BuilderData 0x24 {streamCount,
// indexLODCount, materialEnt, streamBlobs, indexLODBlobs, two ParamCargo
// at +0x14 and +0x1C}; BlobRef 0x14 {blob, BlobOwner owner at +4}, and
// BlobOwner 0x10 is RawBlobEntity::Owner (0xC: vptr, prev, next) with
// `res` at +0xC -- which is why the loops store the handle at +0x10 of a
// ref and hand +4 to AddOwner. ParamCargo 0x8, its CreateInfo 0x8
// {bufferSize, textureCount}; MaterialParam 0x8; Effect 0x74 with
// ParamFormatTable params[3] at +0x28, so params[1] is the geometry set
// (+0x34 formats, +0x3C dataSize) and params[2] the render set (+0x40,
// +0x48); ParamFormat 0xC with size at +4 and offset at +8; MaterialEntity
// 0x58 on Entity, its Material at +0x2C and that material's effect at
// +0x1C, which is the single +0x48 load both builders make;
// EntityHandleBase 0x48 with the entity at +0x38; Entity 0x18 with the
// handle at +0x14; StackAllocator 0xC on StaticStackAllocator {mark,
// buffer, bufferEnd}. RawBlobEntity is reached only through a pointer, so
// it is declared with its owner list at +0x20 and its defer flag at +0x24
// the way WAD00_5_2's RemoveOwner already declares it, and the blob
// handle is spelled EntityHandleBase* where the DWARF has the template
// EntityHandle<T> -- nothing but the base's members is touched.
//
// The primitive table is an ANONYMOUS-NAMESPACE object of ANOTHER
// translation unit: retail's symbol is `primTable__19@unnamed@WAD00_cpp@`,
// and CodeWarrior mangles an unnamed namespace with the basename of the
// file being compiled, so a fragment named GeometryEntity.cpp cannot spell
// it and `#line` does not move it (measured, NOTES "Anonymous namespaces
// pin a unit to its blob"). The reference is a masked data relocation
// either way, so this file DEFINES its own unnamed-namespace table rather
// than declaring an extern it cannot name; the six values are read out of
// the image at 0x80692AF8 (184, 168, 176, 144, 152, 160 -- point list,
// line list, line strip, triangle list, triangle strip, triangle fan) and
// are recovered fact, not invention. The consequence is that our object
// carries a .rodata section the split has none of, so this unit can be
// COMPARED and cannot be linked.
//
// Six shapes the bytes and the debug info fixed.
//
// All three builders are STATIC members: r3 is the first declared
// parameter in each, not a `this`, and CreateBuilderData reads
// `allocator.mark` straight out of r3.
//
// The effect pointer is a LOCAL that lives across a call in both
// builders. ReserveBuilderData reads params[1].dataSize before the first
// GetCreateInfo and params[2].dataSize after it, so the effect is in a
// callee-saved register over that call and cannot be a re-read of the
// member chain; CreateBuilderData does the same across two PushAligns.
// The two aligned sizes are locals for the same reason -- the loads sit
// ahead of calls that would clobber them -- and `dwarf_lines.py` gives
// each its own statement line (57 and 62, 138 and 142).
//
// The referrer is a NAMED default-initialised local PER CALL SITE, not
// one local reused and not a temporary. Retail gives each Activate and
// Deactivate its own stack slot -- 8 in ReserveBuilderData, 12 then 8 in
// CreateBuilderData, 16 then 12 then 8 in DeactivateBuilderData -- and
// the first use takes the highest slot, which is mwcc's reverse
// declaration order. A temporary of an empty class would zero its byte
// on the stack and retail writes nothing to any of those slots.
//
// The blob handle is re-read from the ref for each of the two statements
// that follow the activation: retail loads +0 and then +0x38 twice, with
// no call in between, because the byte store to the entity's defer flag
// can alias the ref. Written as two statements through the member chain
// that is exactly what comes out; a local would keep the entity.
//
// The stream loop's guard is the 64-bit uid tested whole (`or.` of the
// two halves), and it is a named local because the loop needs the value
// again for the lookup; the index-LOD loop passes its uid straight into
// the lookup and the DWARF names no local for it.
//
// AddOwner clears BOTH links of a first owner in one chained assignment
// -- prev stored before next, which is the inner assignment first -- and
// re-reads the list head before writing the old head's back-link, since
// the two stores through the owner could alias it.
//
// NEAR MISS, three of the six, none carried further -- the session that
// wrote this unit was cut off. CreateBuilderData is 158 of 166 words,
// ReserveBuilderData 33 of 49 and the ParamFormat instantiation of
// LoadParamData 22 of 32. Nothing here records a spelling that was tried
// and rejected, because none was.
//

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

typedef unsigned long long uid;

inline void* operator new(unsigned long, void* p) { return p; }

// (value + align - 1) & ~(align - 1); with 16 constant-folded this is the
// `addi 15` / `rlwinm 0,0,27` pair both builders spell six times.
inline int Align(int value, int align) {
    return (value + align - 1) & ~(align - 1);
}

namespace Memory {

class StaticStackAllocator {
public:
    void* Alloc(unsigned long size) {
        void* p = mark;

        mark += size;

        return p;
    }

    unsigned char* mark;
    unsigned char* buffer;
    unsigned char* bufferEnd;
};

class StackAllocator : public StaticStackAllocator {
public:
    void PushAlign(unsigned long align);
};

}  // namespace Memory

// Emitted out of line and called, which is what retail has: the tag is
// never read, the count is scaled by sizeof(T) into the allocator's bump,
// and the constructed run is a counted loop -- no call in the body, so it
// goes on ctr.
template <class T, class H>
T* NewArray(H& heap, eMemMgrTag tag, unsigned long count) {
    T* mem = (T*)heap.Alloc(count * sizeof(T));

    if (mem) {
        for (unsigned long i = 0; i < count; i++) {
            new (&mem[i]) T;
        }
    }

    return mem;
}

namespace Util {

// Empty, and passed as a named default-initialised local.
class Referrer {};

}  // namespace Util

namespace World {
class MaterialParam;
class TextureResourceEntity;
class EntityHandleBase;
}  // namespace World

namespace Graphics {

enum PrimitiveType {
    PRIM_QUADLIST = 128,
    PRIM_TRILIST = 144,
    PRIM_TRISTRIP = 152,
    PRIM_TRIFAN = 160,
    PRIM_LINELIST = 168,
    PRIM_LINESTRIP = 176,
    PRIM_POINTLIST = 184
};

class Effect {
public:
    // Only ever a pointer here; the DWARF gives it no members.
    class ParamData;

    class ParamFormat {
    public:
        char* name;
        unsigned short size;
        unsigned char count;
        unsigned char type;
        unsigned short offset;
        unsigned short pad2;
    };

    class ParamFormatTable {
    public:
        ParamFormat* formats;
        int count;
        int dataSize;
    };

    enum ParamSetEnum { PARAM_MATERIAL, PARAM_GEOM, PARAM_REND };

    unsigned char _pad0[0x28];
    ParamFormatTable params[3];
    unsigned char _pad1[0x74 - 0x4C];
};

class Material {
public:
    unsigned char _pad0[0x1C];
    Effect* effect;
    unsigned char _pad1[0x2C - 0x20];
};

}  // namespace Graphics

namespace World {

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

class MaterialEntity {
public:
    unsigned char _pad0[0x14];
    EntityHandleBase* handle;
    unsigned char _pad1[0x2C - 0x18];
    Graphics::Material material;
};

class MaterialParam {
public:
    unsigned char type;
    unsigned char debugIndex;
    unsigned short elementCount;
    unsigned int p;
};

class RawBlobEntity {
public:
    class Owner {
    public:
        // Declared and not defined: the vtable's home stays where retail
        // has it, and constructing a BlobOwner only needs the symbol.
        virtual void _v0();

        Owner* prev;
        Owner* next;
    };

    // RawBlobEntity.h's own definition, emitted out of line here because
    // this unit is the first to use it.
    void AddOwner(Owner* owner) {
        if (owners == 0) {
            owner->next = owner->prev = 0;
        } else {
            owner->next = owners;
            owner->prev = 0;
            owners->prev = owner;
        }

        owners = owner;
    }

    void RemoveOwner(Owner* owner);

    unsigned char _pad0[0x20];
    Owner* owners;
    bool deferDestroy;
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
                                  int paramCount);

        void Create(void* buffer, const CreateInfo& info,
                    const MaterialParam* params, int paramCount);
        void Activate(EntityHandleBase* handle, const MaterialParam* params,
                      int paramCount);
        void Deactivate();

        void LoadParam(void* data, int* dataOffset, const MaterialParam& param,
                       int offset, int size);

        template <class F>
        void LoadParamData(Graphics::Effect::ParamData* data,
                           const MaterialParam* iparams, int paramCount,
                           const F* formats) {
            int dataOffset = 0;

            for (int i = 0; i < paramCount; i++) {
                LoadParam(data, &dataOffset, iparams[i], formats[i].offset,
                          formats[i].size);
            }
        }

        int textureCount;
        TextureResourceEntity** textureEnts;
    };
};

class ViewAttrib {
public:
    unsigned char spaceFilter;
};

class RenderAttrib {
public:
    unsigned char spaceFilter;
};

class GeometryAsset {
public:
    class VertexStream {
    public:
        uid vertexBufferID;
        unsigned char stride;
        unsigned char pad[7];
    };

    class IndexLOD {
    public:
        uid indexBufferID;
        unsigned short indexCount;
        unsigned short primCount;
        unsigned short triCount;
        unsigned short vertCount;
        float lodDist;
        unsigned char skinSectStart;
        unsigned char skinSectCount;
        unsigned short pad;
    };

    unsigned char _pad0[0x28];
    uid materialID;
    ViewAttrib viewAttrib;
    unsigned char flags;
    unsigned char wiiIndexStride;
    unsigned char wiiIndexSkin;
    unsigned short vertexCount;
    unsigned char prim;
    unsigned char geomParamCount;
    unsigned char rendParamCount;
    unsigned char streamCount;
    unsigned short streamMapBufferCount;
    const MaterialParam* geomParams;
    const MaterialParam* rendParams;
    VertexStream* streams;
    unsigned char _pad1[0x4];
    unsigned int sectorLight;
    unsigned char _pad2[0x4];
    unsigned short lodCount;
    unsigned short batchCount;
    IndexLOD* lods;
    unsigned char _pad3[0x4];
};

class GeometryEntity {
public:
    class BlobOwner : public RawBlobEntity::Owner {
    public:
        EntityHandleBase* res;
    };

    class BlobRef {
    public:
        EntityHandleBase* blob;
        BlobOwner owner;
    };

    class BuilderInfo {
    public:
        int size;
        MaterialEntity* materialEnt;
        RenderAttrib viewAttrib;
        Graphics::Effect::ParamData* geomParamData;
        Graphics::Effect::ParamData* rendParamData;
        Graphics::PrimitiveType primType;
        ShaderEntity::ParamCargo::CreateInfo geomParamCreateInfo;
        ShaderEntity::ParamCargo::CreateInfo rendParamCreateInfo;
    };

    class BuilderData {
    public:
        int streamCount;
        int indexLODCount;
        MaterialEntity* materialEnt;
        BlobRef* streamBlobs;
        BlobRef* indexLODBlobs;
        ShaderEntity::ParamCargo geomParamCargo;
        ShaderEntity::ParamCargo rendParamCargo;
    };

    static void ReserveBuilderData(BuilderInfo& info, const GeometryAsset& asset,
                                   EntityHandleBase* handle);
    static void CreateBuilderData(Memory::StackAllocator& allocator,
                                  BuilderData& data, BuilderInfo& info,
                                  const GeometryAsset& asset,
                                  EntityHandleBase* handle);
    static void DeactivateBuilderData(BuilderData& data,
                                      EntityHandleBase* handle);
};

}  // namespace World

namespace {

// Retail's is `primTable__19@unnamed@WAD00_cpp@`, another translation
// unit's file static; the values are read out of the image at 0x80692AF8.
// See the head of this file for why this unit defines its own.
const Graphics::PrimitiveType primTable[] = {
    Graphics::PRIM_POINTLIST, Graphics::PRIM_LINELIST,
    Graphics::PRIM_LINESTRIP, Graphics::PRIM_TRILIST,
    Graphics::PRIM_TRISTRIP,  Graphics::PRIM_TRIFAN
};

}  // namespace

void World::GeometryEntity::ReserveBuilderData(BuilderInfo& info,
                                               const GeometryAsset& asset,
                                               EntityHandleBase*) {
    Util::Referrer referrer;

    EntityHandleBase* materialHandle =
        EntityManager::FindHandle(asset.materialID);

    materialHandle->Activate(referrer);
    info.materialEnt = (MaterialEntity*)materialHandle->entity;

    const Graphics::Effect* effect = info.materialEnt->material.effect;

    int geomParamDataSize =
        Align(effect->params[Graphics::Effect::PARAM_GEOM].dataSize, 16);
    ShaderEntity::ParamCargo::GetCreateInfo(info.geomParamCreateInfo,
                                            asset.geomParams,
                                            asset.geomParamCount);

    int rendParamDataSize =
        Align(effect->params[Graphics::Effect::PARAM_REND].dataSize, 16);
    ShaderEntity::ParamCargo::GetCreateInfo(info.rendParamCreateInfo,
                                            asset.rendParams,
                                            asset.rendParamCount);

    int size = (asset.streamCount + asset.lodCount) * sizeof(BlobRef) +
               info.geomParamCreateInfo.bufferSize +
               info.rendParamCreateInfo.bufferSize;

    size = Align(size, 16) + geomParamDataSize;
    info.size = Align(size, 16) + rendParamDataSize;
}

void World::GeometryEntity::CreateBuilderData(Memory::StackAllocator& allocator,
                                              BuilderData& data,
                                              BuilderInfo& info,
                                              const GeometryAsset& asset,
                                              EntityHandleBase* handle) {
    const Graphics::Effect* effect = info.materialEnt->material.effect;

    data.streamBlobs =
        NewArray<BlobRef>(allocator, (eMemMgrTag)65, asset.streamCount);
    data.indexLODBlobs =
        NewArray<BlobRef>(allocator, (eMemMgrTag)65, asset.lodCount);

    data.materialEnt = info.materialEnt;
    data.streamCount = asset.streamCount;
    data.indexLODCount = asset.lodCount;

    const GeometryAsset::IndexLOD* ilods = asset.lods;

    for (int i = 0; i < asset.lodCount; i++) {
        BlobRef& blober = data.indexLODBlobs[i];
        Util::Referrer referrer;

        blober.blob = EntityManager::FindHandle(ilods[i].indexBufferID);

        blober.owner.res = handle;
        blober.blob->Activate(referrer);

        ((RawBlobEntity*)blober.blob->entity)->deferDestroy = true;
        ((RawBlobEntity*)blober.blob->entity)->AddOwner(&blober.owner);
    }

    const GeometryAsset::VertexStream* istreams = asset.streams;

    for (int i = 0; i < asset.streamCount; i++) {
        BlobRef& blober = data.streamBlobs[i];
        uid id = istreams[i].vertexBufferID;

        if (id) {
            Util::Referrer referrer;

            blober.blob = EntityManager::FindHandle(id);

            blober.owner.res = handle;
            blober.blob->Activate(referrer);

            ((RawBlobEntity*)blober.blob->entity)->deferDestroy = true;
            ((RawBlobEntity*)blober.blob->entity)->AddOwner(&blober.owner);
        } else {
            blober.blob = 0;
            blober.owner.res = 0;
        }
    }

    info.primType = primTable[asset.prim];

    int geomParamDataSize =
        Align(effect->params[Graphics::Effect::PARAM_GEOM].dataSize, 16);

    int rendParamDataSize =
        Align(effect->params[Graphics::Effect::PARAM_REND].dataSize, 16);

    void* geomParamCargoBuffer =
        allocator.Alloc(info.geomParamCreateInfo.bufferSize);
    void* rendParamCargoBuffer =
        allocator.Alloc(info.rendParamCreateInfo.bufferSize);

    allocator.PushAlign(16);
    void* geomParamDataBuffer = allocator.Alloc(geomParamDataSize);
    allocator.PushAlign(16);
    void* rendParamDataBuffer = allocator.Alloc(rendParamDataSize);

    const MaterialParam* geomParams = asset.geomParams;
    info.geomParamData = (Graphics::Effect::ParamData*)geomParamDataBuffer;
    data.geomParamCargo.Create(geomParamCargoBuffer, info.geomParamCreateInfo,
                               geomParams, asset.geomParamCount);
    data.geomParamCargo.Activate(handle, geomParams, asset.geomParamCount);
    data.geomParamCargo.LoadParamData(
        info.geomParamData, geomParams, asset.geomParamCount,
        effect->params[Graphics::Effect::PARAM_GEOM].formats);

    const MaterialParam* rendParams = asset.rendParams;
    info.rendParamData = (Graphics::Effect::ParamData*)rendParamDataBuffer;
    data.rendParamCargo.Create(rendParamCargoBuffer, info.rendParamCreateInfo,
                               rendParams, asset.rendParamCount);
    data.rendParamCargo.Activate(handle, rendParams, asset.rendParamCount);
    data.rendParamCargo.LoadParamData(
        info.rendParamData, rendParams, asset.rendParamCount,
        effect->params[Graphics::Effect::PARAM_REND].formats);

    info.viewAttrib.spaceFilter = asset.viewAttrib.spaceFilter;
}

void World::GeometryEntity::DeactivateBuilderData(BuilderData& data,
                                                  EntityHandleBase*) {
    data.geomParamCargo.Deactivate();
    data.rendParamCargo.Deactivate();

    for (int i = 0; i < data.streamCount; i++) {
        if (data.streamBlobs[i].blob) {
            Util::Referrer referrer;

            ((RawBlobEntity*)data.streamBlobs[i].blob->entity)
                ->RemoveOwner(&data.streamBlobs[i].owner);
            data.streamBlobs[i].blob->Deactivate(referrer);
        }
    }

    for (int i = 0; i < data.indexLODCount; i++) {
        Util::Referrer referrer;

        ((RawBlobEntity*)data.indexLODBlobs[i].blob->entity)
            ->RemoveOwner(&data.indexLODBlobs[i].owner);
        data.indexLODBlobs[i].blob->Deactivate(referrer);
    }

    Util::Referrer referrer;

    data.materialEnt->handle->Deactivate(referrer);
}
