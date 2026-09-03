// Texture.cpp -- five functions, read from the image with
// tools/disasm.py. Create wraps an already-built GXTexObj: it takes the
// object, disclaims ownership, clears the size and the alpha scale, and
// reads the width and height back out of the object when there is one.
// SetImageFromFileInMemory builds a texture from a TPL in memory: it
// claims ownership, reads the texture count out of the file header
// (turning six into seven, which is the cube-map case), allocates that
// many GXTexObjs on the global heap, binds the TPL palette that follows
// the 32-byte header, takes the first descriptor's dimensions, and then
// for every descriptor in the palette copies the object out of the
// palette, re-reads its LOD settings, forces the bias clamp and edge
// LOD on, replaces the anisotropy with the header's per-texture value
// for the first six, and writes the settings back. A seven-texture
// image registers itself with the dynamic environment map, and Destroy
// unregisters it. AmendImageFromFileInMemory calls
// SetImageFromFileInMemory through a POINTER TO MEMBER. RenderDetach
// frees the GXTexObj array and nulls it when the texture owns it.
//
// Layouts from the DWARF (tools/dwarf_types.py): Texture is 0x48 -- a
// Graphics::Node base of 0xC (its vptr, then the node type and three
// bools), tex at 0xC, numTextures at 0x10, header at 0x14, a
// WiiTextureFileHeader by value at 0x18, width at 0x38, height at 0x3C,
// ownTex at 0x40, three tile bytes and alphaScale at 0x44.
// WiiTextureFileHeader is 0x20 -- textureType, alphaScale, a SHORT
// numTextures at 8 (the `lha`), max_aniso[6] at 0xA (the `lbz` plus
// `extsb`), and sixteen bytes of padding, which is the 32 the palette
// pointer skips. _GXTexObj is 0x20, which is the shift by five in the
// allocation size.
//
// Six shapes the bytes fixed.
//
// The GXTexObj array comes from the NewArray helper whose heap enum is
// a CONST REFERENCE, so the enumerator is bound to a static temporary
// and loaded back (`lwz r4,-8544(r3)`) instead of being an immediate;
// the matching DeleteArray copies the heap into a local BEFORE its own
// null test, which is why RenderDetach's `beq cr1` on the already-made
// compare sits after the load. Both are the spelling Channel.cpp uses.
//
// AmendImageFromFileInMemory's twelve-byte constant is {0, -1,
// SetImageFromFileInMemory} in the image, which is a pointer to a
// non-virtual member: a local of pointer-to-member type, copied to the
// stack and called through __ptmf_scall.
//
// numTextures is re-read from the member after the six-to-seven bump,
// and the TPL descriptor's header pointer is loaded twice for the width
// and the height, because the store to `this` in between can alias
// either -- both fall out of naming the member and the descriptor
// rather than holding them in locals.
//
// The eight LOD outputs are stack slots at 16, 17, 20, 24, 28, 32, 36
// and 40, and those displacements are NOT relocated, so the bytes fix
// them. mwcc lays a scope's locals out in REVERSE declaration order --
// the four-byte ones downward from the top of the local area, the
// one-byte ones separately -- so the declaration here runs maxAniso,
// lodBias, maxLod, minLod, magFilt, minFilt, doEdgeLod, biasClamp, and
// the GX parameter order gives the whole set the other way round: all
// eight displacements mirrored, twelve words.
//
// The allocated array is held in a LOCAL and the loop indexes that,
// not the member: retail carries it in r25 across the loop where
// reading `tex` back gives an `lwz` at the top of every iteration.
//
// The loop counter is signed against 6 (`cmpwi`) and unsigned against
// the palette's descriptor count (`cmplw`), which is an `int` i against
// an `unsigned long` member.
//
// NEAR MISS -- SetImageFromFileInMemory, 8 words of 95, and every one
// of the eight is a register NUMBER. tools/fndiff.py aligns 87 of
// retail's 95 (91.58%): retail keeps the allocated array in r25 and the
// loop counter in r27, ours the other way round, and the element
// pointer, the palette, the anisotropy slot, the hoisted one, the
// stride and `this` all agree. It is the same symptom BuildMemory.cpp
// records beside it -- retail's loop counter takes the HIGHER
// callee-saved register of the pair and ours the lower -- and nothing
// tried reaches it.
//
// The lever that later closed two OTHER instances of this shape --
// zSoundWiimoteSpeakerList::Init and zSoundSourcesPhysics::InitMemory,
// where taking the allocation into a `void*` and casting it separately,
// TOGETHER with declaring the loop counter above it, moved both to an
// exact match -- does not move this one. Measured all four ways: as
// written 8 of 95, the void* alone 8, the counter first alone 13, and
// both together 13. So the shape has an answer in some functions and
// this is not it. Ruled out, none moving a word except where noted:
// all six declaration orders of the three named locals hoisted to the
// top of the function (the three that declare the counter first are 13
// of 95, the rest 8); the counter declared outside the `for` (13); the
// counter as `long` and as `unsigned long` with an `(int)` cast on the
// six-test; an empty `else` on that test; the element pointer unnamed
// (16) and written as `texObjs + i`; the palette computed before the
// allocation (22) and with the element pointer unnamed too (40); the
// header stored before the texture (10); the array assigned through a
// chained `tex = texObjs = ...`; the descriptor count read into a local
// (53); the element pointer declared after the palette call (16); and
// compiler 1.3, which gives exactly the same eight.

extern "C" {

// Only the size is needed: the allocation shifts the count by five.
class _GXTexObj {
public:
    unsigned char _pad0[0x20];
};

enum GXTexFilter { GX_TEX_FILTER_ = 0x7FFFFFFF };
enum GXAnisotropy { GX_ANISOTROPY_ = 0x7FFFFFFF };

unsigned short GXGetTexObjWidth(const _GXTexObj* obj);
unsigned short GXGetTexObjHeight(const _GXTexObj* obj);

void GXGetTexObjLODAll(const _GXTexObj* obj, GXTexFilter* min_filt,
                       GXTexFilter* mag_filt, float* min_lod, float* max_lod,
                       float* lod_bias, unsigned char* bias_clamp,
                       unsigned char* do_edge_lod, GXAnisotropy* max_aniso);

void GXInitTexObjLOD(_GXTexObj* obj, GXTexFilter min_filt,
                     GXTexFilter mag_filt, float min_lod, float max_lod,
                     float lod_bias, unsigned char bias_clamp,
                     unsigned char do_edge_lod, GXAnisotropy max_aniso);

// The TPL palette that follows the file header, and the two records the
// dimensions are read out of.
class TPLHeader {
public:
    unsigned short height;
    unsigned short width;
    unsigned int format;
};

class TPLDescriptor {
public:
    TPLHeader* textureHeader;
    void* CLUTHeader;
};

class TPLPalette {
public:
    unsigned long versionNumber;
    unsigned long numDescriptors;
    TPLDescriptor* descriptorArray;
};

void TPLBind(TPLPalette* pal);
TPLDescriptor* TPLGet(TPLPalette* pal, unsigned long id);
void TPLGetGXTexObjFromPalette(TPLPalette* pal, _GXTexObj* obj,
                               unsigned long id);
}

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {

enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };

void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap, eMemMgrTag tag,
                      bool clear);
void FreeGlobalHeap(void* block, GlobalHeapEnum heap);

}  // namespace Memory

// The heap comes by reference, which is what binds the enumerator to a
// static temporary and loads it back at the call site.
template <class T, class H>
inline T* NewArray(const H& heap, eMemMgrTag tag, unsigned long count) {
    return (T*)Memory::AllocGlobalHeap(count * sizeof(T), heap, tag, false);
}

template <class H, class T>
inline void DeleteArray(const H& heap, T* array, unsigned long count) {
    H h = heap;

    if (array) {
        Memory::FreeGlobalHeap(array, h);
    }
}

namespace Graphics {

class WiiTextureFileHeader {
public:
    int textureType;
    float alphaScale;
    short numTextures;
    char max_aniso[6];
    char pad[16];
};

class Node {
public:
    enum NodeTypeEnum { NodeTypeEnum_ = 0x7FFFFFFF };

    virtual void _v0();

    NodeTypeEnum type;
    bool attached;
    bool detachPending;
    bool updateThread;
};

class Texture;

class WiiDynamicEnvMap {
public:
    static void Register(Texture* texture);
    static void Unregister(Texture* texture);
};

class Texture : public Node {
public:
    void Create(_GXTexObj* texObj);
    void Destroy();
    void RenderDetach();
    void SetImageFromFileInMemory(const void* data, int size);
    void AmendImageFromFileInMemory(const void* data, int size);

    _GXTexObj* tex;
    int numTextures;
    WiiTextureFileHeader* header;
    WiiTextureFileHeader dummyHeader;
    int width;
    int height;
    bool ownTex;
    signed char tileX;
    signed char tileY;
    signed char tileZ;
    float alphaScale;
};

}  // namespace Graphics

void Graphics::Texture::AmendImageFromFileInMemory(const void* data, int size) {
    void (Texture::*set)(const void*, int) =
        &Texture::SetImageFromFileInMemory;

    (this->*set)(data, size);
}

void Graphics::Texture::Create(_GXTexObj* texObj) {
    tex = texObj;
    ownTex = false;
    width = 0;
    height = 0;
    alphaScale = 0.0f;

    if (texObj) {
        width = GXGetTexObjWidth(texObj);
        height = GXGetTexObjHeight(texObj);
    }
}

void Graphics::Texture::Destroy() {
    if (numTextures == 7) {
        WiiDynamicEnvMap::Unregister(this);
    }
}

void Graphics::Texture::RenderDetach() {
    if (tex && ownTex) {
        DeleteArray((Memory::GlobalHeapEnum)0, tex, numTextures);

        tex = 0;
    }
}

void Graphics::Texture::SetImageFromFileInMemory(const void* data, int size) {
    GXAnisotropy maxAniso;
    float lodBias;
    float maxLod;
    float minLod;
    GXTexFilter magFilt;
    GXTexFilter minFilt;
    unsigned char doEdgeLod;
    unsigned char biasClamp;

    ownTex = true;
    width = 0;
    height = 0;

    numTextures = ((WiiTextureFileHeader*)data)->numTextures;

    if (numTextures == 6) {
        numTextures = numTextures + 1;
    }

    _GXTexObj* texObjs = NewArray<_GXTexObj, Memory::GlobalHeapEnum>(
        (Memory::GlobalHeapEnum)0, (eMemMgrTag)38, numTextures);

    tex = texObjs;
    header = (WiiTextureFileHeader*)data;

    TPLPalette* pal = (TPLPalette*)((char*)data + 32);

    TPLBind(pal);

    TPLDescriptor* desc = TPLGet(pal, 0);

    width = desc->textureHeader->width;
    height = desc->textureHeader->height;

    for (int i = 0; i < pal->numDescriptors; i++) {
        _GXTexObj* obj = &texObjs[i];

        TPLGetGXTexObjFromPalette(pal, obj, i);

        GXGetTexObjLODAll(obj, &minFilt, &magFilt, &minLod, &maxLod, &lodBias,
                          &biasClamp, &doEdgeLod, &maxAniso);

        biasClamp = 1;
        doEdgeLod = 1;

        if (i < 6) {
            maxAniso = (GXAnisotropy)header->max_aniso[i];
        }

        GXInitTexObjLOD(obj, minFilt, magFilt, minLod, maxLod, lodBias,
                        biasClamp, doEdgeLod, maxAniso);
    }

    if (numTextures == 7) {
        WiiDynamicEnvMap::Register(this);
    }
}
