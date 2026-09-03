// WAD01_5.cpp (NG) -- three functions, read from the image with
// tools/disasm.py. Graphics::Effect::GetShader picks a feature: it walks the
// technique's render-stage entry for one LOD and returns the first feature
// whose flags cover the wanted mask, or a null feature, then tail-calls the
// GetShader that takes a feature and a pass. Math::Mul multiplies a vector by
// a view matrix -- four dot products, one per row, assigned into the result.
// Graphics::ShaderVec4's constructor sets its four floats to zero.
//
// Layouts from the DWARF (tools/dwarf_types.py): Graphics::Effect 0x74 with
// techs at +0x4C; Effect::Technique 0x1C -- name, lodMax, renderStageFlags,
// then stages[4] at +0xC, one pointer each, which is why the render stage is
// scaled by four and added to twelve; Effect::LOD 0x8, a feature array and its
// count, so the LOD index is scaled by eight; Effect::Feature 0xC with its
// flags at +0x8, so the walk steps by twelve; Graphics::Shader::RenderStage is
// the four-value enum the stage array is sized for. Graphics::ViewMatrix is
// 0x40, a Math::Matrix43 at +0 (three Vector4 rows) and a fourth Vector4 col3
// at +0x30, which is where the four dot products' 0/16/32/48 come from.
// Graphics::ShaderVec4 is 0x10, four floats. The zero is a literal (@96809 at
// 0x80692C18, read from the image), and being the unit's only one it gets its
// own lis whatever is ahead of it.
//
// Four shapes the bytes fixed. The feature walk is a bottom-entered loop --
// retail branches to the test first, and falls THROUGH the loop's exit into
// the null -- so the source is `while (f != end)` with the found case leaving
// by a goto; the alternatives and what each cost are listed under NEAR MISS
// below. The mask test reads the flags into an INT local first: the DWARF's
// featureFlags is `unsigned int`, and `mask & f->featureFlags` compared
// against an int mask is an unsigned compare, `cmplw`, where retail has the
// signed `cmpw`; one local for the loaded word is the whole of that word.
// Math::Mul's four dot products are named locals declared in the order they
// are computed -- w, z, y, x -- because that is what puts the first result in
// f31 and the last in f29; written as four arguments of the Assign call
// instead, mwcc still evaluates them right to left but allocates f29 upwards
// and four words come out wrong. And ShaderVec4's constructor is four stores
// of one loaded literal, so the four fields are assigned separately.
//
// NEAR MISS -- GetShader, 23 words against retail's 24, unitcmp scoring 9 of
// 23. Everything before the loop is word for word identical and so is
// everything after it; the whole of the difference is one branch. Retail
// spends two instructions where we spend one:
//
//     retail                        ours
//       cmpw  r7,r0                   cmpw  r7,r0
//       bne   -> increment            beq   -> tail
//       b     -> tail                 addi  r4,r4,12
//       addi  r4,r4,12
//
// -- that is, retail's conditional branch goes to the increment and an
// unconditional one leaves the loop, where mwcc folds ours into a single
// conditional that leaves and falls through to the increment. The blocks are
// in retail's order either way: the `li r4,0` sits directly above the shared
// `mr r5,r8 ; b`, which is what the found path jumps over.
//
// Ten spellings of the loop, each compiled and read back. Five fold to the
// same 23 words: the goto above; `if (!=) { f++; continue; }` then `goto
// found`; the same as an explicit if/else; a `for (; f != end; f++)` with
// `continue`; and an explicit `goto next` / `goto found` pair with the
// increment labelled. Two RETURNS in the body DO give retail's branch polarity
// -- `bne` to the increment and `b` out -- but emit the tail twice, 25 words,
// and spelling both of them `GetShader(feature, pass)` so they could merge
// does not make mwcc merge them. `break` plus `if (f == end) f = 0;` after the
// loop keeps the compare, 24 words with ten wrong. A `found` pointer assigned
// in the loop takes a register of its own, 19 of 24 wrong. A `for (;;)` with
// the end test written first stops the loop being rotated: 24 words, ten
// wrong, the test at the top. And a `FindFeature` helper is not inlined at
// all, one expression or not -- it comes out as a 56-byte function this unit
// would define and retail does not have.
//
// Fifteen compiler settings were swept over the same source. Twelve leave the
// function exactly as it is: -opt nopeephole, nocse, nodeadcode, nolifetimes,
// noloopinvariants, nostrength, level=3 and space; -inline off; -O3,s; -O4;
// and -func_align 4. `-schedule off` is much worse (18 words wrong). `-O4,p`
// and `-opt speed` reach 24 words
// -- but the extra word is a loop-alignment `nop`, still with `beq`, and they
// cost Math::Mul, which goes from exact to 37 words. So the flag is not the
// lever here: this unit is -O4,s like the rest of the game library, and what
// is left is one branch mwcc folds and the original's compiler did not.

#include "SB/NG/Engine/WAD01_5.pool.h"

namespace Graphics {
class ViewMatrix;
}

namespace Math {

class Vector4 {
public:
    void Assign(float x, float y, float z, float w);

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

float Dot(const Vector4& a, const Vector4& b);
void Mul(Vector4& out, const Vector4& v, const Graphics::ViewMatrix& m);

}  // namespace Math

namespace Graphics {

class ViewMatrix {
public:
    Math::Matrix43 mat;
    Math::Vector4 col3;
};

class Shader {
public:
    enum RenderStage {
        RENDER_STAGE_DEPTH_FIRST = 0,
        RENDER_STAGE_SHADOW_MAP = 1,
        RENDER_STAGE_STENCIL_SHADOW = 2,
        RENDER_STAGE_COLOR = 3,
        MAX_RENDER_STAGE = 4
    };
};

class ShaderVec4 {
public:
    ShaderVec4();

    float x;
    float y;
    float z;
    float w;
};

class ShaderRef;
class EffectLodFeatureLookup;
class EffectPreprocessLookup;
class ParamFormat;

class ParamFormatTable {
public:
    ParamFormat* formats;
    int count;
    int dataSize;
};

class Node {
public:
    virtual void _v0();

    int type;
    bool attached;
    bool detachPending;
    bool updateThread;
};

class NodeHeader {
public:
    NodeHeader* prev;
    NodeHeader* next;
};

class Effect : public Node {
public:
    class Pass;

    class Feature {
    public:
        Pass* passes;
        int passCount;
        unsigned int featureFlags;
    };

    class LOD {
    public:
        Feature* features;
        int featureCount;
    };

    class RenderStageEntry {
    public:
        LOD* lods;
    };

    class Technique {
    public:
        char* name;
        int lodMax;
        int renderStageFlags;
        RenderStageEntry stages[4];
    };

    const Shader* GetShader(int tech, Shader::RenderStage stage, int lod,
                            int featureMask, int pass) const;
    const Shader* GetShader(const Feature* feature, int pass) const;

    NodeHeader listNode;
    EffectLodFeatureLookup* depotLodFeat;
    int depotPreprocessCount;
    EffectPreprocessLookup* depotPreprocess;
    ShaderRef* shaderList;
    int shaderListSize;
    ParamFormatTable params[3];
    Technique* techs;
    int techCount;
    signed char standardTechIndices[6];
    unsigned char standardTechShaders[6];
    unsigned short effectFlags;
    unsigned short colorMulHandle;
    unsigned short alphaMulHandle;
    unsigned short diffuseMapHandle;
    unsigned short lightMapHandle;
    unsigned short textureBlendFactorHandle;
    unsigned short textureBlendTextureHandle;
    unsigned short textureBlendSpecularHandle;
    unsigned char diffuseMapExposure;
    unsigned char lightMapExposure;
};

}  // namespace Graphics

const Graphics::Shader* Graphics::Effect::GetShader(int tech,
                                                    Shader::RenderStage stage,
                                                    int lod, int featureMask,
                                                    int pass) const {
    const LOD& entry = techs[tech].stages[stage].lods[lod];
    const Feature* feature = entry.features;
    const Feature* end = feature + entry.featureCount;

    while (feature != end) {
        int flags = feature->featureFlags;

        if ((featureMask & flags) == featureMask) {
            goto found;
        }

        feature++;
    }

    feature = 0;

found:
    return GetShader(feature, pass);
}

void Math::Mul(Vector4& out, const Vector4& v, const Graphics::ViewMatrix& m) {
    float w = Dot(v, m.col3);
    float z = Dot(v, m.mat.v[2]);
    float y = Dot(v, m.mat.v[1]);
    float x = Dot(v, m.mat.v[0]);

    out.Assign(x, y, z, w);
}

Graphics::ShaderVec4::ShaderVec4() {
    x = 0.0f;
    y = 0.0f;
    z = 0.0f;
    w = 0.0f;
}
