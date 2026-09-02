// Renderable.cpp -- two functions, read from the image with
// tools/disasm.py. Create files the geometry and parameter block,
// clears the scene links and the technique, sets the visibility mask,
// the alpha fade and the extra-data handle, assigns the local colour
// multiplier to white (the 1.0f is loaded once and copied to the four
// arguments), copies the four effect handles, and initialises the
// multiplier; the effect is read once for the four. InitLocalColorMultiplier reads the colour at the handle's
// byte offset into the parameter block and assigns it, as a tail call.
// Layouts from the DWARF (Renderable 0x48 on Node 0xC, Geometry 0xB4
// with the effect at +0x4C); the handles' offsets in the effect are the
// loads, named after the renderable's own.

namespace Math {

class Vector4 {
public:
    void Assign(float x, float y, float z, float w);

    float x;
    float y;
    float z;
    float w;
};

}  // namespace Math

namespace Graphics {

class Scene;
class RenderableSceneRef;
class RenderMode;
class Builder;
class Material;
class Geometry;

class Effect {
public:
    class ParamData;

    unsigned char _pad0[0x62];
    unsigned short colorMulHandle;
    unsigned char _pad1[0x6];
    unsigned short textureBlendFactorHandle;
    unsigned short textureBlendTextureHandle;
    unsigned short textureBlendSpecularHandle;
};

class Node {
public:
    virtual void __key();

    int type;
    bool attached;
    bool detachPending;
    bool updateThread;
};

class Geometry : public Node {
public:
    unsigned char _pad0[0x40];
    Effect* effect;
    Material* material;
    Effect::ParamData* paramData;
};

class Renderable : public Node {
public:
    void Create(Geometry* geometry, Effect::ParamData* params);
    void InitLocalColorMultiplier();

    Scene* scene;
    RenderableSceneRef* sceneRef;
    Effect::ParamData* paramData;
    RenderMode* instRenderMode;
    Math::Vector4 localColorMul;
    Geometry* geom;
    float hackAlphaFade;
    unsigned short colorMulHandle;
    unsigned short textureBlendFactorHandle;
    unsigned short textureBlendTextureHandle;
    unsigned short textureBlendSpecularHandle;
    float textureBlendFactor;
    unsigned short viewportVisibleMask;
    unsigned short extraData;
    unsigned char hackVertLOD;
    unsigned char activeTechIndex;
    unsigned char viewAttrib;
};

}  // namespace Graphics

void Graphics::Renderable::Create(Geometry* geometry,
                                  Effect::ParamData* params) {
    geom = geometry;
    paramData = params;
    activeTechIndex = 0;
    viewportVisibleMask = 1;
    scene = 0;
    sceneRef = 0;
    instRenderMode = 0;
    hackAlphaFade = 1.0f;
    hackVertLOD = 0;
    extraData = 0xFFFF;
    localColorMul.Assign(1.0f, 1.0f, 1.0f, 1.0f);

    Effect* effect = geometry->effect;

    colorMulHandle = effect->colorMulHandle;
    textureBlendFactorHandle = effect->textureBlendFactorHandle;
    textureBlendTextureHandle = effect->textureBlendTextureHandle;
    textureBlendSpecularHandle = effect->textureBlendSpecularHandle;

    InitLocalColorMultiplier();
}

void Graphics::Renderable::InitLocalColorMultiplier() {
    unsigned short handle = colorMulHandle;

    if (handle == 0xFFFF) {
        return;
    }

    const float* color = (const float*)((const unsigned char*)paramData + handle);

    localColorMul.Assign(color[0], color[1], color[2], color[3]);
}
