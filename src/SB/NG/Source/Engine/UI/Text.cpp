// Text.cpp -- one function, read from the image with tools/disasm.py:
// UI::RenderText counts the visible characters of the string through
// the font asset's blob entity, and when there are any takes a copy of
// the string from the update-frame allocator rounded up to sixteen
// bytes, then a 32-byte item pointing at it with the bound rectangle's
// corner and the scales, spacing and slant, builds the render request
// on the stack from the clip rectangle, the colour, the brightness, the
// depth, the count and the one item, and hands it to the font's
// eighth virtual with the scene and the visibility mask. The call
// between the two allocations, made with (0, copy, length), lands on
// the lone `blr` the image names Math::Matrix33's constructor -- the
// weak empty function every empty function folded into -- so it is
// called by that symbol. Layouts from the DWARF (RenderTextArgs 0x64,
// Font 0x2C on Entity 0x18, EntityHandleBase with the entity at
// +0x38); the item and the request are not named there and are the
// stores. Struct copies of the clip rectangle and the colour are
// member-wise: assigning the structs calls an out-of-line implicit
// operator= the unit would then define.
//
// NEAR MISS, 94 of 95 words. Retail computes the first allocation's
// size (`rlwinm r4`) before the allocator's address (`addi r3`); ours
// the other way round. Six spellings leave it: the size inline, in a
// local, through an inline rounding helper, the call through an inline
// allocation wrapper for one call and for both, and the copy declared
// ahead of the length (which fixed the registers and nothing else).

extern "C" {
unsigned long strlen(const char* s);
void* memcpy(void* dst, const void* src, unsigned long n);
void __ct__Q24Math8Matrix33Fv(void* a, void* b, unsigned long c);
}

namespace Memory {

class SingleBufferAllocator {
public:
    void* Alloc(unsigned long size);
};

extern SingleBufferAllocator updateFrameAllocator;

}  // namespace Memory

namespace Graphics {

class Scene;

class Viewport {
public:
    enum VisibilityMask { VisibilityMask_ = 0x7FFFFFFF };
};

}  // namespace Graphics

namespace World {

class EntityHandleBase {
public:
    unsigned char _pad0[0x38];
    class Entity* entity;
};

}  // namespace World

namespace UI {

class Rect {
public:
    float x0;
    float y0;
    float x1;
    float y1;
};

class ColorF {
public:
    float r;
    float g;
    float b;
    float a;
};

class TextItem {
public:
    const char* begin;
    const char* end;
    float x;
    float y;
    float xScale;
    float yScale;
    float xSpace;
    float slant;
};

class TextRequest {
public:
    Rect clip;
    ColorF color;
    unsigned char brightness;
    float z;
    int count;
    TextItem* items;
    int itemCount;
};

class FontAssetBlobEntity {
public:
    int CountVisible(const char* begin, const char* end) const;
};

class TextState;
class TextBackground;

class Font {
public:
    virtual void _v0();
    virtual void _v1();
    virtual void _v2();
    virtual void _v3();
    virtual void _v4();
    virtual void _v5();
    virtual void _v6();
    virtual void Render(Graphics::Scene* scene, const TextRequest& request,
                        Graphics::Viewport::VisibilityMask mask);

    unsigned char _pad0[0x14];
    World::EntityHandleBase* assetHandle;
    World::EntityHandleBase* textureHandle;
    float padTop;
    float padBottom;
    float yAdvance;
};

class RenderTextArgs {
public:
    int wrap;
    int xjustify;
    int yjustify;
    Font* font;
    TextState* state;
    TextBackground* background;
    Rect boundRect;
    Rect clipRect;
    float xScale;
    float yScale;
    float xSpace;
    float ySpace;
    float slant;
    ColorF color;
    unsigned char brightness;
    unsigned char pad[3];
    float z;
};

void RenderText(Graphics::Scene* scene, const RenderTextArgs& args,
                const char* text, Graphics::Viewport::VisibilityMask mask);

}  // namespace UI

void UI::RenderText(Graphics::Scene* scene, const RenderTextArgs& args,
                    const char* text, Graphics::Viewport::VisibilityMask mask) {
    char* copy;
    unsigned long len = strlen(text);
    int count = ((FontAssetBlobEntity*)args.font->assetHandle->entity)
                    ->CountVisible(text, text + len);

    if (count <= 0) {
        return;
    }

    unsigned long size = (len + 15) & ~15;

    copy = (char*)Memory::updateFrameAllocator.Alloc(size);

    __ct__Q24Math8Matrix33Fv(0, copy, len);

    if (!copy) {
        return;
    }

    memcpy(copy, text, len);

    TextItem* item = (TextItem*)Memory::updateFrameAllocator.Alloc(sizeof(TextItem));

    if (!item) {
        return;
    }

    item->begin = copy;
    item->end = copy + len;
    item->x = args.boundRect.x0;
    item->y = args.boundRect.y0;
    item->xScale = args.xScale;
    item->yScale = args.yScale;
    item->xSpace = args.xSpace;
    item->slant = args.slant;

    TextRequest request;

    request.clip.x0 = args.clipRect.x0;
    request.clip.y0 = args.clipRect.y0;
    request.clip.x1 = args.clipRect.x1;
    request.clip.y1 = args.clipRect.y1;
    request.color.r = args.color.r;
    request.color.g = args.color.g;
    request.color.b = args.color.b;
    request.color.a = args.color.a;
    request.brightness = args.brightness;
    request.z = args.z;
    request.count = count;
    request.items = item;
    request.itemCount = 1;

    args.font->Render(scene, request, mask);
}
