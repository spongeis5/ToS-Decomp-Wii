// GenericShader.cpp -- eight functions, read from the image with
// tools/disasm.py. The generic shader is the Wii's ordinary geometry
// shader: Create fills it in from fourteen parameters (the id, the
// feature flags, four state-op tables as pointer/size pairs, the shader
// parameter block and the flag word) and gives its default render mode
// the material space layer; the stream and channel counts come out of
// the shader op's own header bytes. GetGeometrySize reports the
// per-renderable context size (the stream count rounded up to four) and
// its alignment through a reference; CreateGeometryContext clears the
// first byte of the block it is handed and returns it. BeginRender
// invalidates the light-array state op's three cached slots and applies
// the shader ops; EndRender applies the end-shader ops; both do nothing
// unless the shader op's flag word has its top bit set. Render walks an
// array of renderables under one technique, applying a material's
// render mode, its material ops and the geometry's material overrides
// only when they change from the last renderable, then the per-
// renderable ops, and finally either the GPU skinning path or a plain
// display-list call at the renderable's LOD. RenderMode::Create and
// RenderMode::ApplyRenderStates are in this translation unit too --
// header inlines the unity build emitted here.
//
// Layouts from the DWARF (tools/dwarf_types.py). GenericShader is 0x70
// on Shader 0x18: featureFlags +0xC, vertexStreamCount +0x10,
// vertexChannelCount +0x14, id +0x18, then shaderOps +0x20, geomOps
// +0x28, materialOps +0x30 and rendOps +0x38 as StateOpTable
// {void* states; int stateSize;}, shaderParamOffsets +0x40,
// shaderParamCount +0x44, shaderParams +0x48, defaultRenderMode +0x4C
// and flags +0x68. RenderMode is 0x1C with spaceLayer +0xC, sortOrder
// +0x10, renderStates +0x14 and renderStateCount +0x18; StateDelta is
// 8 bytes, id and priority halfwords then a four-byte value.
// Wii_ShaderStateOp is 0xC -- the StateOp header, `flags` at +4 whose
// bit 31 is the enable, matSource, channelCount and streamCount at +8,
// +9 and +0xA -- and Wii_GeometryStateOp is 0x48 with a
// World::Wii_MaterialSettings at +4 and posMtx at +0x44. ShaderRef is
// 0x28: shader, renderMode, then paramIndices[3], paramOffsets[3] and
// paramCount[3], which is where the three offset/count pairs Render
// hoists come from. Renderable gives paramData +0x14, instRenderMode
// +0x18, geom +0x2C and hackVertLOD +0x44; Geometry gives material
// +0x50, paramData +0x54, indexLods[2] +0x64 (0x1C each),
// indexLodCount +0x9C and wiiGeomStateOp +0xB0; Material gives
// renderMode +0x20 and paramData +0x24; Effect gives shaderList +0x20.
//
// Six shapes the bytes fixed.
//
// The three cached light slots are `Graphics::LightArrayStateOp`'s
// statics, and retail reaches all three from ONE base register --
// 0x80774A60, which is the unity unit's .bss anchor, with lastOddLight
// at displacement 11200 off it. So the unit carries that distance as an
// unreferenced .bss array and DEFINES the three, which is the .bss
// padding lever MemoryUtil.cpp records; declared extern each would take
// a lis/addi of its own.
//
// ApplyMaterialOverrides is STATIC and ApplyMaterial is not, and the
// registers are what say so: the overrides call sets r3 through r6 and
// leaves r7 alone, so its four arguments start in r3 and there is no
// `this`. CodeWarrior mangles the two the same way, so nothing but the
// register use distinguishes them. Its first argument is the shader's
// own geometry state op's material settings, `states + 4`.
//
// Render's registers came from two levers, and the second is the one
// worth remembering. `tools/dwarf_locals.py` names its locals and the
// register each got -- lastGeom r27, then the six technique values
// r26..r21, lastMaterial r20, lastRenderMode r19, end r18, rend r17,
// vertLOD r16 -- so mwcc hands the callee-saved registers out in
// DECLARATION order, descending. But retail's `geom` is r29 and its
// instRenderMode r28, ABOVE lastGeom, while both are assigned inside
// the loop. Declared UNINITIALISED at the top of the block and merely
// ASSIGNED in the loop, they take r29 and r28 and every register below
// them falls into place: 70 of 121 words became 121 of 121. Declaring
// them where they are first used costs thirteen registers' worth of
// renumbering. `dwarf_lines.py` supplied the statement order that goes
// with it (line 269 lastGeom, 272-277 the six, 279 and 280 the other
// two, 284 MutableInit, 287 the for).
//
// The other is `int idx = shaderHandle;`. Retail computes
// `shaderHandle * 40` TWICE -- once for the first ApplyRenderStates and
// again for the six hoists -- and keeps `shaderHandle` itself in r17
// across the two calls between them. Written with `shaderHandle` in
// both subscripts the compiler common-subexpressions the multiply
// instead and keeps the PRODUCT, which is two words shorter than
// retail and shifts every register after it. Seven spellings were tried
// against that (a reference and a pointer to the entry, pointer
// arithmetic on either side, and a copy of the index in four widths);
// only a separate index variable stops the CSE, and `int`, `unsigned
// int` and `long` all give the identical bytes, so the width is not
// recoverable. The DWARF lists no such local, so this is a spelling
// that reproduces the bytes and not necessarily the original text.
//
// The LOD clamp is written `if (vertLOD >= geom->indexLodCount)`, which
// is the branch retail has (`blt` past the fix-up); the `<` form
// inverts it. The display-list size is `headerAndSize >> 12`, an
// unsigned shift -- the whole word is one rlwinm.
//
// NEAR MISS -- BeginRender, 26 of its 31 words, and the five are one
// shape. Retail MATERIALISES the shader-op flag test into a register
// before branching on it: `rlwinm. r0,r0,0,0,0 ; beq ; li r0,1 ; b ;
// li r0,0 ; cmpwi r0,0 ; beq`, where ours folds the test straight into
// the branch. Every other word of the function, the .bss displacements
// included, is identical. What was tried and did not move it, each
// compiled and read back: the test in a local of seven types (bool,
// char, unsigned char, short, int, unsigned int, long, and a
// `register bool`); `? 1 : 0` and an explicit if/else assigning the two
// constants; `!!`, `(bool)`, `== true`, `!= 0` and `& 1` on the result;
// `>> 31` and `(int)flags < 0` for the test itself; an inline predicate
// in four places (on Wii_ShaderStateOp, on StateOpTable, on
// GenericShader, and a free inline taking the op or the flags word),
// returning bool implicitly, explicitly and through a ternary; an enum
// with FALSE/TRUE members as the materialised type, as a local, as a
// ternary and as an inline's return; and eight ways of leaving an
// `&&`/`||` chain with one surviving operand (`&& true`, `&& 1==1`,
// `&& sizeof(int)`, `&& kConst`, `&& an inline returning true`,
// `|| false`, `|| 0`, and the test AND-ed and OR-ed with itself).
// A scan of the whole image for this shape -- b?? +12, li rX,1, b +8,
// li rX,0, cmpwi rX,0 -- finds 42 functions, and in every other one the
// materialised value is an `&&` or `||` chain of TWO OR MORE tests
// (zUIText_Init's two enum compares, IsWalkable's four float ones,
// nandGetType's pair). BeginRender is the only game function in the
// image that materialises a SINGLE test, so the second operand of
// whatever chain it was written as is gone from the bytes. A predicate
// whose body is an `&&` is not inlined at all by this compiler here (it
// emits a `bl`, measured twice), which is what shuts that door.

namespace World {

// 16 bytes of parameter indices; only its address crosses the call.
class Wii_MaterialSettings {
public:
    unsigned char _pad0[0x10];
};

class RenderModeAsset {
public:
    enum SpaceLayer { SPACE_LAYER_FORCE_INT = 0x7FFFFFFF };
};

}  // namespace World

namespace Graphics {

class Viewport;
class Renderable3D;
class Shader;
class ParamData;

enum RenderStateID { RENDER_STATE_ID_FORCE_INT = 0x7FFFFFFF };

class RenderState {
public:
    // The state list a render mode replays: an id, a priority and the
    // four value bytes the setter takes as one word.
    class StateDelta {
    public:
        unsigned short id;
        unsigned short priority;
        unsigned int value;
    };

    static void Flush();
    static void BeginRenderMode();
    static void EndRenderMode();
    static void SetStateInt(RenderStateID id, unsigned short priority,
                            unsigned int value, unsigned int a, unsigned int b);
};

class RenderMode {
public:
    void Create(World::RenderModeAsset::SpaceLayer layer, int sortOrder,
                const RenderState::StateDelta* states, int count);
    void ApplyRenderStates() const;

    unsigned char _pad0[0xC];
    World::RenderModeAsset::SpaceLayer spaceLayer;
    int sortOrder;
    RenderState::StateDelta* renderStates;
    int renderStateCount;
};

class Renderable;

class StateOpTable {
public:
    void ApplyShader(const void* params, const unsigned short* offsets,
                     int count);
    void ApplyEndShader();
    void MutableInit();
    void ApplyMaterial(const void* params, const unsigned short* offsets,
                       int count, const Viewport* viewport);
    static void ApplyMaterialOverrides(const World::Wii_MaterialSettings& mat,
                                       const void* params,
                                       const unsigned short* offsets,
                                       int count);
    void ApplyRenderable(const void* params, const unsigned short* offsets,
                         int count, const Renderable* rend,
                         const Viewport* viewport, const StateOpTable& shader);

    void* states;
    int stateSize;
};

// The three cached slots BeginRender invalidates.
class LightArrayStateOp {
public:
    static unsigned char lastOddLight;
    static int lastKit;
    static int lastDynLight[2];
};

class Wii_ShaderStateOp {
public:
    unsigned char _pad0[0x4];
    unsigned int flags;
    unsigned char matSource;
    unsigned char channelCount;
    unsigned char streamCount;
    unsigned char uvMapping;
};

class Wii_GeometryStateOp {
public:
    unsigned char _pad0[0x4];
    World::Wii_MaterialSettings mat;
    unsigned char _pad1[0x30];
    unsigned char posMtx;
};

class IndexBufferHandle {
public:
    unsigned char* displayList;
    unsigned int headerAndSize;
};

class IndexBuffer {
public:
    IndexBufferHandle handle;
    int firstIndex;
};

class IndexLOD {
public:
    IndexBuffer indexBuffer;
    int primCount;
    int triCount;
    int indexCount;
    int lodVertCount;
};

class Material {
public:
    unsigned char _pad0[0x20];
    RenderMode* renderMode;
    ParamData* paramData;
};

class Geometry {
public:
    unsigned char _pad0[0x50];
    Material* material;
    ParamData* paramData;
    unsigned char _pad1[0xC];
    IndexLOD indexLods[2];
    int indexLodCount;
    unsigned char _pad2[0x10];
    Wii_GeometryStateOp* wiiGeomStateOp;
};

// Reached only by a reinterpreting cast off the geometry: single
// inheritance would add nothing to the bytes and this keeps the cast
// honest about what the image shows.
class SkinGeometry {
public:
    void WiiSkinRenderGPU(Renderable3D* rend, Viewport* viewport,
                          const StateOpTable& shaderOps);
};

class Renderable {
public:
    unsigned char _pad0[0x14];
    ParamData* paramData;
    RenderMode* instRenderMode;
    unsigned char _pad1[0x10];
    Geometry* geom;
    unsigned char _pad2[0x14];
    unsigned char hackVertLOD;
};

class ShaderRef {
public:
    Shader* shader;
    RenderMode* renderMode;
    unsigned char* paramIndices[3];
    unsigned short* paramOffsets[3];
    unsigned char paramCount[3];
    unsigned char pad;
    unsigned int shaderRefKey;
};

class Effect {
public:
    unsigned char _pad0[0x20];
    ShaderRef* shaderList;
};

class GenericShader {
public:
    void Create(unsigned long long id, int featureFlags, void* shaderStates,
                int shaderSize, void* materialStates, int materialSize,
                void* geomStates, int geomSize, void* rendStates,
                int rendSize, unsigned short* paramOffsets, int paramCount,
                void* params, unsigned int flags);
    int GetGeometrySize(int& align);
    void* CreateGeometryContext(const unsigned char* decl, void* block,
                                int size);
    void BeginRender();
    void Render(const Effect* effect, unsigned char tech, Renderable** rends,
                int count, Viewport* viewport);
    void EndRender();

    unsigned char _pad0[0xC];
    int featureFlags;
    int vertexStreamCount;
    int vertexChannelCount;
    unsigned long long id;
    StateOpTable shaderOps;
    StateOpTable geomOps;
    StateOpTable materialOps;
    StateOpTable rendOps;
    unsigned short* shaderParamOffsets;
    int shaderParamCount;
    void* shaderParams;
    RenderMode defaultRenderMode;
    unsigned int flags;
};

}  // namespace Graphics

extern "C" void GXCallDisplayList(void* list, unsigned long size);

// The unity unit's .bss ahead of the three light slots, measured from
// the image: its anchor is 0x80774A60 and lastOddLight is at 0x80777620,
// so 11200 bytes. Referenced by nothing and holds nothing; without it
// the three stores come out at displacement 0, 4 and 8.
static unsigned char kUnityBssAhead[11200];

unsigned char Graphics::LightArrayStateOp::lastOddLight;
int Graphics::LightArrayStateOp::lastKit;
int Graphics::LightArrayStateOp::lastDynLight[2];

void Graphics::GenericShader::Create(unsigned long long shaderID,
                                     int shaderFeatureFlags,
                                     void* shaderStates, int shaderSize,
                                     void* materialStates, int materialSize,
                                     void* geomStates, int geomSize,
                                     void* rendStates, int rendSize,
                                     unsigned short* paramOffsets,
                                     int paramCount, void* params,
                                     unsigned int shaderFlags) {
    id = shaderID;
    featureFlags = shaderFeatureFlags;
    shaderParamOffsets = paramOffsets;
    shaderParamCount = paramCount;
    shaderParams = params;
    flags = shaderFlags;

    defaultRenderMode.Create((World::RenderModeAsset::SpaceLayer)2, 0, 0, 0);

    shaderOps.states = shaderStates;
    shaderOps.stateSize = shaderSize;

    materialOps.states = materialStates;
    materialOps.stateSize = materialSize;

    geomOps.states = geomStates;
    geomOps.stateSize = geomSize;

    rendOps.states = rendStates;
    rendOps.stateSize = rendSize;

    vertexStreamCount = ((Wii_ShaderStateOp*)shaderStates)->streamCount;
    vertexChannelCount = ((Wii_ShaderStateOp*)shaderStates)->channelCount;
}

void Graphics::RenderMode::Create(World::RenderModeAsset::SpaceLayer layer,
                                  int order,
                                  const RenderState::StateDelta* states,
                                  int count) {
    spaceLayer = layer;
    sortOrder = order;
    renderStates = (RenderState::StateDelta*)states;
    renderStateCount = count;
}

int Graphics::GenericShader::GetGeometrySize(int& align) {
    align = 4;

    return (vertexStreamCount + 3) & ~3;
}

void* Graphics::GenericShader::CreateGeometryContext(const unsigned char* decl,
                                                     void* block, int size) {
    *(unsigned char*)block = 0;

    return block;
}

void Graphics::GenericShader::BeginRender() {
    if (((Wii_ShaderStateOp*)shaderOps.states)->flags & 0x80000000) {
        LightArrayStateOp::lastOddLight = 255;
        LightArrayStateOp::lastKit = -1;
        LightArrayStateOp::lastDynLight[0] = -1;
        LightArrayStateOp::lastDynLight[1] = -1;

        shaderOps.ApplyShader(shaderParams, shaderParamOffsets,
                              shaderParamCount);

        RenderState::Flush();
    }
}

void Graphics::GenericShader::Render(const Effect* effect,
                                     unsigned char shaderHandle,
                                     Renderable** renderList,
                                     int renderListSize, Viewport* viewp) {
    if (((Wii_ShaderStateOp*)shaderOps.states)->flags & 0x80000000) {
        effect->shaderList[shaderHandle].renderMode->ApplyRenderStates();

        RenderState::Flush();

        Geometry* geom;
        RenderMode* instRenderMode;
        Geometry* lastGeom = 0;

        int idx = shaderHandle;

        int materialParamCount = effect->shaderList[idx].paramCount[0];
        unsigned short* materialParamOffsets =
            effect->shaderList[idx].paramOffsets[0];
        int geomParamCount = effect->shaderList[idx].paramCount[1];
        unsigned short* geomParamOffsets =
            effect->shaderList[idx].paramOffsets[1];
        int rendParamCount = effect->shaderList[idx].paramCount[2];
        unsigned short* rendParamOffsets =
            effect->shaderList[idx].paramOffsets[2];

        Material* lastMaterial = 0;
        RenderMode* lastRenderMode = 0;

        rendOps.MutableInit();

        for (Renderable **end = renderList + renderListSize, **it = renderList;
             it != end; it++) {
            Renderable* rend = *it;
            geom = rend->geom;
            int vertLOD = rend->hackVertLOD;

            if (vertLOD >= geom->indexLodCount) { vertLOD = geom->indexLodCount - 1; }

            if (geom != lastGeom) {
                Material* material = geom->material;

                if (material != lastMaterial) {
                    RenderMode* renderMode = material->renderMode;

                    if (renderMode != lastRenderMode) {
                        if (lastRenderMode) {
                            RenderState::EndRenderMode();
                        }

                        if (renderMode) {
                            renderMode->ApplyRenderStates();
                        }

                        RenderState::Flush();

                        lastRenderMode = renderMode;
                    }

                    materialOps.ApplyMaterial(material->paramData,
                                              materialParamOffsets,
                                              materialParamCount, viewp);

                    lastMaterial = material;
                }

                StateOpTable::ApplyMaterialOverrides(
                    ((Wii_GeometryStateOp*)geomOps.states)->mat,
                    geom->paramData, geomParamOffsets, geomParamCount);

                lastGeom = geom;
            }

            rendOps.ApplyRenderable(rend->paramData, rendParamOffsets,
                                    rendParamCount, rend, viewp, shaderOps);

            instRenderMode = rend->instRenderMode;

            if (instRenderMode) {
                instRenderMode->ApplyRenderStates();

                RenderState::Flush();
            }

            if (geom->wiiGeomStateOp->posMtx) {
                ((SkinGeometry*)geom)
                    ->WiiSkinRenderGPU((Renderable3D*)rend, viewp, shaderOps);
            } else {
                IndexLOD* lod = &geom->indexLods[vertLOD];

                GXCallDisplayList(lod->indexBuffer.handle.displayList,
                                  lod->indexBuffer.handle.headerAndSize >> 12);
            }

            if (instRenderMode) {
                RenderState::EndRenderMode();

                RenderState::Flush();
            }
        }

        if (lastRenderMode) {
            RenderState::EndRenderMode();
        }

        RenderState::EndRenderMode();
    }
}

void Graphics::RenderMode::ApplyRenderStates() const {
    RenderState::BeginRenderMode();

    RenderState::StateDelta* delta = renderStates;
    RenderState::StateDelta* end = renderStates + renderStateCount;

    while (delta != end) {
        RenderState::SetStateInt((RenderStateID)delta->id, delta->priority,
                                 delta->value, 0, 0xFFFFFFFF);
        delta++;
    }
}

void Graphics::GenericShader::EndRender() {
    if (((Wii_ShaderStateOp*)shaderOps.states)->flags & 0x80000000) {
        shaderOps.ApplyEndShader();
    }
}
