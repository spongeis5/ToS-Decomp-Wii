// WAD00_17 -- six functions, 1068 bytes, read from the image with
// tools/disasm.py. The generic shader entity: its constructor, the
// shader's destructor, the asset's Create, Detach, Destroy and the
// entity's destructor.
//
//   __ct__GenericShaderEntity   the ShaderEntity base, the vtable, the
//                               shader member (Node type 32, its own
//                               vtable, its RenderMode), then typeID 8
//   __dt__GenericShader         the RenderMode member, then the base
//   GenericShaderAsset::Create  sizes one block, allocates it, carves
//                               the entity and seven runs out of a
//                               stack allocator, copies the four state
//                               tables in, builds the parameter offset
//                               table, creates and activates the param
//                               cargo, loads each parameter, then
//                               creates and attaches the shader
//   Detach                      the node, then the cargo
//   Destroy                     the block size, an empty call, the
//                               virtual destructor, then DeleteArray
//   __dt__GenericShaderEntity   the shader member, then the base
//
// LAYOUTS, all from tools/dwarf_types.py (bare names -- `--type
// GenericShader`, not `--type Graphics::GenericShader`):
//
//   GenericShaderEntity 0xA0 = ShaderEntity 0x24, memSize 0x24,
//     paramCargo 0x28, shader 0x30. ShaderEntity 0x24 = Entity 0x18 plus
//     three handles. Entity 0x18 is a vptr, an embedded list node,
//     ogUpdateIdx, typeID at 0x10 and the handle.
//   GenericShader 0x70 = Shader 0x18 (itself a Node 0xC), id 0x18, four
//     state-op tables 0x20..0x40, shaderParamOffsets 0x40,
//     shaderParamCount 0x44, shaderParams 0x48, defaultRenderMode 0x4C,
//     flags 0x68. RenderMode 0x1C is a Node plus four words.
//   GenericShaderAsset 0x48 = ShaderAsset 0x18 (three 64-bit ids), then
//     shaderOps 0x18, materialOps 0x20, geomOps 0x28, rendOps 0x30 --
//     each {states, stateSize} -- shaderParamFormats 0x38 {entries,
//     count 0x3C, pad, dataSize 0x3E}, featureFlags 0x40, flags 0x44.
//   MaterialParam is 8 bytes and MaterialParamFormat 0xC with size at
//   +0x8 and offset at +0xA; the two loop strides in Create, 8 and 12,
//   are those sizes and are what identify the two arrays.
//   ParamCargo::CreateInfo is the 8-byte {bufferSize, textureCount}
//   (CreateInfo__30FD2 of the three the DWARF has under that leaf name).
//
// `Node::T_GenericShader = 32` is the enumerator the DWARF gives for the
// constructor's `li r4,32`; the parameter array is `asset + 1`, since
// `addi r28,r4,72` is exactly sizeof(GenericShaderAsset).
//
// NEAR MISS, 3 of the 12 functions the object defines. This paragraph
// used to read EXACT, 7 of 7, and it was wrong when it was committed:
// the session writing this unit was killed mid-edit and the record
// describes a state of the file that was not preserved. What
// `python tools/unitcmp.py SB/NG/Engine/WAD00_17` says now:
//
//   MATCH   Destroy, 24 words; Detach, 14; and the weak
//           DeleteArray<Memory::EntityPoolEnum, unsigned char> the
//           template emits, 6 -- which retail also has and which comes
//           out identical to it, the same thing RenderModeEntity.cpp
//           records.
//   DIFFER  GenericShaderAsset::Create 59 of 155 words; the
//           GenericShader destructor 21 of 24 (ours 112 bytes against
//           retail's 96); the GenericShaderEntity destructor 17 of 24
//           (108 against 96); the entity's constructor 22 of the 19
//           that compare, ours 76 bytes against retail's 104.
//   EXTRA   five functions retail does not have anywhere in the image,
//           checked against the symbol table one at a time:
//           __dt__Q28Graphics4NodeFv, __dt__Q25World6EntityFv,
//           __dt__Q25World12ShaderEntityFv,
//           __dt__Q28Graphics10RenderModeFv and
//           __ct__Q28Graphics13GenericShaderFv.
//
// The five EXTRA symbols are the thing to fix first, and the notes
// below say why each of them should not exist -- the four destructors
// because an intermediate class with no declared destructor does not
// get one, and the constructor because its one-line in-class form is
// taken by -inline auto. Both are still spelled that way in this file,
// so something else changed around them; the constructor is inline
// in-class at line 191 and none of the four classes declares a
// destructor. Start by finding what makes mwcc emit them anyway.
//
// WHAT THE BYTES SETTLED, and each of these was one compile:
//
//   * AN INTERMEDIATE CLASS WITH NO DECLARED DESTRUCTOR DOES NOT GET
//     ONE. NOTES.md records that a class between the Havok object and
//     the one being written "gets a destructor of its own emitted (80
//     bytes, a vtable store) and called", and prescribes spelling
//     hkBaseObject as the DIRECT base. That is true of a class given an
//     EXPLICIT empty inline destructor; it is not true of one given
//     none. Node and RenderMode are declared here with no destructor at
//     all, and both destructors call `__dt__12hkBaseObjectFv` straight
//     through, exactly as retail does -- so the real hierarchy could be
//     kept (GenericShader on Node, RenderMode on Node, Entity on
//     hkBaseObject) instead of flattening it. That matters because the
//     CONSTRUCTOR needs the hierarchy: it calls `__ct__Q28Graphics4Node
//     FQ38Graphics4Node12NodeTypeEnum` on the shader at +0x30, which no
//     flattened spelling reaches.
//   * THE SHADER'S CONSTRUCTOR IS INLINED AT ITS ONE-LINE FORM.
//     `GenericShader() : Node(T_GenericShader) {}` is taken by -inline
//     auto, so the entity's constructor emits Node's call, the vtable
//     store and the RenderMode call in line, which is what retail has.
//     No pragma was needed.
//   * DESTROY'S SECOND CALL IS TO A NAME NO SPELLING PRODUCES. Retail
//     branches to 0x800075C0, which the image names
//     `__ct__Q24Math8Matrix33Fv`: the lone `blr` every empty function in
//     the game folded onto. The source surely called some empty method
//     on the shader, but that method's name is gone from the image, so
//     the call is written as the mangled symbol -- the lever NOTES.md
//     records for `main` and the module registry's Startup. Declaring
//     and defining an empty `GenericShader::Destroy()` instead would
//     reproduce the instruction and RELOCATE AGAINST THE WRONG SYMBOL,
//     which is precisely what unitcmp's relocation check exists to
//     catch.
//
// Not a matching question, for whoever links this: the unit defines the
// first non-inline virtual of both GenericShaderEntity and
// GenericShader, so our object emits `__vt__Q25World19GenericShaderEntity`
// and `__vt__Q28Graphics13GenericShader`. That is right -- the
// constructor's two stores have to relocate against those names -- but
// retail's copies live in the WAD00 blob's .data, which this text-only
// split does not cover, so the unit needs its data before it can be
// flipped to Matching. Note also that the entity's destructor must stay
// at vtable slot 0, since Destroy calls it as `lwz r12,8(r12)`: the
// WAD00_12_1 trick of declaring a `__key()` virtual ahead of the
// destructor to move the vtable's home would push it to slot 1 and must
// not be used here.

void operator delete(void* mem);

inline void* operator new(unsigned long, void* p) { return p; }

extern "C" void* memcpy(void* dst, const void* src, unsigned long n);

// The empty function every empty body in the image folded onto. The name
// is the image's, not a guess about what the source called.
extern "C" void __ct__Q24Math8Matrix33Fv(void* p);

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {

enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };
enum EntityPoolEnum { EntityPool = 0 };

void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap, eMemMgrTag tag,
                      bool clear);
void FreeGlobalHeap(void* block, GlobalHeapEnum heap);

inline void Free(void* block, EntityPoolEnum) {
    FreeGlobalHeap(block, (GlobalHeapEnum)0);
}

class StackAllocator {
public:
    void* PushAlign(unsigned long align);

    void* Push(unsigned long size) {
        void* p = mark;
        mark += size;
        return p;
    }

    unsigned char* mark;
};

class StaticStackAllocator : public StackAllocator {
public:
    void Create(void* buffer, int size);

    unsigned char* buffer;
    unsigned char* bufferEnd;
};

}  // namespace Memory

template <class H, class T>
void DeleteArray(const H& heap, T* array, unsigned long count) {
    if (array) {
        Memory::Free(array, heap);
    }
}

class hkBaseObject {
public:
    virtual ~hkBaseObject();
};

namespace Graphics {

class Node : public hkBaseObject {
public:
    enum NodeTypeEnum { T_GenericShader = 32 };

    Node(NodeTypeEnum type);

    void Attach();
    void Detach();

    NodeTypeEnum type;
    bool attached;
    bool detachPending;
    bool updateThread;
};

class RenderMode : public Node {
public:
    enum SpaceLayer { SpaceLayer_ = 0x7FFFFFFF };

    RenderMode();

    SpaceLayer spaceLayer;
    int sortOrder;
    void* renderStates;
    int renderStateCount;
};

class GenericShader : public Node {
public:
    GenericShader() : Node(T_GenericShader) {}
    ~GenericShader();

    void Create(unsigned long long id, int featureFlags, void* shaderStates,
                int shaderStateSize, void* materialStates, int materialStateSize,
                void* geomStates, int geomStateSize, void* rendStates,
                int rendStateSize, unsigned short* paramOffsets, int paramCount,
                void* paramData, unsigned int flags);

    unsigned char _pad0[0xC];
    unsigned long long id;
    unsigned char _pad1[0x28];
    void* shaderParams;
    RenderMode defaultRenderMode;
    unsigned int flags;
};

}  // namespace Graphics

namespace World {

class EntityHandleBase {
public:
    unsigned long long blobUID;
};

class MaterialParam {
public:
    unsigned char type;
    unsigned char debugIndex;
    unsigned short elementCount;
    void* p;
};

class MaterialParamFormat {
public:
    void* id;
    unsigned char type;
    unsigned char rows;
    unsigned char cols;
    unsigned char count;
    unsigned short size;
    unsigned short offset;
};

class Entity : public hkBaseObject {
public:
    Entity(EntityHandleBase* handle);

    unsigned char _pad0[0x8];
    int ogUpdateIdx;
    unsigned int typeID;
    EntityHandleBase* handle;
};

class ShaderEntity : public Entity {
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
        void LoadParam(void* paramData, int* offset, const MaterialParam& param,
                       int dstOffset, int size);

        int textureCount;
        void** textureEnts;
    };

    ShaderEntity(EntityHandleBase* handle);

    void* vertexCode;
    void* pixelCode;
    void* vertexDecl;
};

class GenericShaderEntity : public ShaderEntity {
public:
    GenericShaderEntity(EntityHandleBase* handle);
    ~GenericShaderEntity();

    void Detach();
    void Destroy();

    int memSize;
    ShaderEntity::ParamCargo paramCargo;
    Graphics::GenericShader shader;
};

class StateOpTableAsset {
public:
    void* states;
    int stateSize;
};

class MaterialParamFormatTableAsset {
public:
    const MaterialParamFormat* entries;
    unsigned char count;
    unsigned char pad;
    unsigned short dataSize;
};

class ShaderAsset {
public:
    unsigned long long vertexCodeID;
    unsigned long long pixelCodeID;
    unsigned long long vertexDeclID;
};

class GenericShaderAsset : public ShaderAsset {
public:
    static GenericShaderEntity* Create(EntityHandleBase* handle,
                                       GenericShaderAsset* asset);

    StateOpTableAsset shaderOps;
    StateOpTableAsset materialOps;
    StateOpTableAsset geomOps;
    StateOpTableAsset rendOps;
    MaterialParamFormatTableAsset shaderParamFormats;
    int featureFlags;
    unsigned int flags;
};

}  // namespace World

World::GenericShaderEntity::GenericShaderEntity(EntityHandleBase* handle)
    : ShaderEntity(handle) {
    typeID = 8;
}

Graphics::GenericShader::~GenericShader() {
}

World::GenericShaderEntity* World::GenericShaderAsset::Create(EntityHandleBase* handle,
                                                              GenericShaderAsset* asset) {
    const MaterialParamFormat* formats;
    Memory::StaticStackAllocator alloc;
    ShaderEntity::ParamCargo::CreateInfo cargoInfo;
    int paramOffset;
    int i;

    const MaterialParam* params = (const MaterialParam*)(asset + 1);
    int paramCount = asset->shaderParamFormats.count;
    int paramDataSize = asset->shaderParamFormats.dataSize;

    ShaderEntity::ParamCargo::GetCreateInfo(cargoInfo, params, paramCount);

    int shaderStateSize = asset->shaderOps.stateSize;
    int materialStateSize = asset->materialOps.stateSize;
    int geomStateSize = asset->geomOps.stateSize;
    int rendStateSize = asset->rendOps.stateSize;

    int memSize = ((((sizeof(GenericShaderEntity) + shaderStateSize + materialStateSize +
                      geomStateSize + rendStateSize + paramCount * 2 + 3) &
                     ~3) +
                    cargoInfo.bufferSize + 15) &
                   ~15) +
                  paramDataSize;

    alloc.Create(Memory::AllocGlobalHeap(memSize, (Memory::GlobalHeapEnum)0,
                                         (eMemMgrTag)54, false),
                 memSize);

    GenericShaderEntity* ent =
        new (alloc.Push(sizeof(GenericShaderEntity))) GenericShaderEntity(handle);

    void* shaderStates = alloc.Push(shaderStateSize);
    void* materialStates = alloc.Push(materialStateSize);
    void* geomStates = alloc.Push(geomStateSize);
    void* rendStates = alloc.Push(rendStateSize);
    unsigned short* paramOffsets = (unsigned short*)alloc.Push(paramCount * 2);

    alloc.PushAlign(4);

    void* cargoBuffer = alloc.Push(cargoInfo.bufferSize);

    alloc.PushAlign(16);

    void* paramData = alloc.Push(paramDataSize);

    ent->memSize = memSize;

    memcpy(shaderStates, asset->shaderOps.states, shaderStateSize);
    memcpy(materialStates, asset->materialOps.states, materialStateSize);
    memcpy(geomStates, asset->geomOps.states, geomStateSize);
    memcpy(rendStates, asset->rendOps.states, rendStateSize);

    formats = asset->shaderParamFormats.entries;

    for (i = 0; i < paramCount; i++) {
        paramOffsets[i] = formats[i].offset;
    }

    ent->paramCargo.Create(cargoBuffer, cargoInfo, params, paramCount);
    ent->paramCargo.Activate(handle, params, paramCount);

    paramOffset = 0;

    for (i = 0; i < paramCount; i++) {
        ent->paramCargo.LoadParam(paramData, &paramOffset, params[i],
                                  formats[i].offset, formats[i].size);
    }

    ent->shader.Create(handle->blobUID, asset->featureFlags, shaderStates,
                       shaderStateSize, materialStates, materialStateSize, geomStates,
                       geomStateSize, rendStates, rendStateSize, paramOffsets,
                       paramCount, paramData, asset->flags);

    ent->shader.Attach();

    return ent;
}

void World::GenericShaderEntity::Detach() {
    shader.Detach();
    paramCargo.Deactivate();
}

void World::GenericShaderEntity::Destroy() {
    unsigned long size = memSize;

    __ct__Q24Math8Matrix33Fv(&shader);

    this->~GenericShaderEntity();

    DeleteArray(Memory::EntityPool, (unsigned char*)this, size);
}

World::GenericShaderEntity::~GenericShaderEntity() {
}
