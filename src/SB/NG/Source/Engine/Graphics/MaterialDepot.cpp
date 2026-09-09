// MaterialDepot.cpp -- seventeen functions, 6,472 bytes, read from the
// image with tools/disasm.py. The depot owns two intrusive lists, one of
// effects and one of materials, and rebuilds a sorted shader-reference
// table out of them.
//
// Five of the seventeen carry 5,700 of the bytes: Refresh (2,816),
// std::sort over ShaderRefEntry (916), DefaultSortFunction (804),
// ShaderRefEntry::operator< (720) and DefaultOrderComparator::operator()
// (444). The twelve written here are the other 772.
//
// Layouts from the DWARF (tools/dwarf_types.py, bare names):
// MaterialDepot 0x4A034 -- customSortFunction +0, the effects list +4,
// the materials list +0xC, dirty +0x14, seven bucket checkpoints +0x16,
// four usage counters +0x24..+0x30 and a 40 KB buffer from +0x34.
// Util::NodeHeader is prev then next, so Erase returns the node's +4.
// Graphics::Effect 0x74 with its list node at +0xC, shaderList at +0x20
// and shaderListSize at +0x24; ShaderRef is 40 bytes, which is the
// stride every index into shaderList uses. ShaderRefEntry is 8 -- an
// Effect* and a key.
//
// MEASURED: 6 of the 9 functions this object defines are
// byte-identical, 344 bytes -- the constructor, AddMaterial, AddEffect,
// VoidList::Erase, Effect::GetShader and Effect::GetRenderMode. The
// five large functions are not written at all yet, and neither is
// std::swap<ShaderRefEntry>, which cannot be: a template that is never
// called is never instantiated, and its only caller is std::sort.
//
// TWO MANGLED NAMES SETTLED THE LIST TYPES. Retail's PushBack is
// PushBack__Q24Util12NodeListBaseFPQ34Util12NodeListBase10NodeHeader --
// three qualifiers on the parameter, so NodeHeader is nested INSIDE
// NodeListBase and not at Util scope, where it would be
// Q24Util10NodeHeader. And Unlink is STATIC: retail puts the node in
// r3, which is where a member call would put `this`. Both were one
// differing word each until they were spelled that way, and Erase went
// from 3 of 13 to exact with them.
//
// RemoveMaterial and RemoveEffect: FOUND, and it was not the argument.
// VoidList::Erase was being INLINED. Ours passed r3 = material+12 and
// no `this` at all, with the bl going to the static Unlink -- which is
// Erase's body, not a call to Erase. Retail passes this = &materials
// and r4 = the address of an iterator temporary, because retail CALLS
// Erase. A call with no `this` to a non-static member is the tell.
//
// Erase is three statements and no loop, so -inline auto takes it.
// `#pragma dont_inline` around the definition restores the call and
// both removers land: RemoveMaterial 68 and RemoveEffect 68, 136
// bytes. Erase itself was ALREADY emitted and already matching at 52
// -- mwcc emits the out-of-line body of a non-inline member whether
// or not it also inlines it at every use, so the unit's function
// count is 9 either way. The pragma changed the CALL SITES only.
//
// The chase before that was for something that would make the ARGUMENT
// a value mwcc must materialise. The materialised temporary is a
// consequence of the call existing, not its cause.
//
// NEAR MISS -- Effect::FindFeature, 20 of 23 words and ONE instruction
// short. Retail computes `lod->features + lod->count` TWICE, once into
// r8 for the first loop's bound and again into r4 for the second;
// ours computes it once and reuses it. Every other word, both loops and
// the mask fold between them, is the same. So the two loops do not
// share the expression in the original, and the question is what makes
// mwcc keep them apart -- the same common-subexpression question that
// zSoundWiimoteSpeaker's Init turned out to hinge on, though the lever
// there was a third local and there is no allocation here to hold one.

typedef unsigned long long uid;

namespace Util {

// NodeHeader is nested inside NodeListBase: retail's PushBack is
// PushBack__Q24Util12NodeListBaseFPQ34Util12NodeListBase10NodeHeader,
// three qualifiers, not Q24Util10NodeHeader. And Unlink is STATIC --
// retail puts the node in r3, where a member call would put this.
class NodeListBase {
public:
    class NodeHeader {
    public:
        NodeHeader* prev;
        NodeHeader* next;
    };

    void PushBack(NodeHeader* node);
    static void Unlink(NodeHeader* node);

    NodeHeader tail;
};

class VoidList : public NodeListBase {
public:
    class Iterator {
    public:
        Iterator(NodeListBase::NodeHeader* n) : node(n) {}

        NodeListBase::NodeHeader* node;
    };

    VoidList();

    NodeListBase::NodeHeader* Erase(Iterator it);
};

}  // namespace Util

namespace Graphics {

class RenderMode;

// 40 bytes, which is the stride every index into shaderList uses.
class ShaderRef {
public:
    void* shader;
    RenderMode* renderMode;
    unsigned char _pad0[0x28 - 0x8];
};

class Effect {
public:
    class Feature {
    public:
        ShaderRef** shaders;
        int _pad0;
        int flags;
    };

    class LOD {
    public:
        Feature* features;
        int count;
    };

    const Feature* FindFeature(const LOD* lod, int mask) const;
    RenderMode* GetRenderMode(unsigned char index) const;
    unsigned char GetShader(const Feature* feature, int index) const;

    unsigned char _pad0[0xC];
    Util::NodeListBase::NodeHeader listNode;
    unsigned char _pad1[0x20 - 0x14];
    ShaderRef* shaderList;
    int shaderListSize;
    unsigned char _pad2[0x74 - 0x28];
};

class Material {
public:
    unsigned char _pad0[0xC];
    Util::NodeListBase::NodeHeader listNode;
};

class MaterialDepot {
public:
    MaterialDepot();

    void AddMaterial(Material* material);
    void RemoveMaterial(Material* material);
    void AddEffect(Effect* effect);
    void RemoveEffect(Effect* effect);

    void (*customSortFunction)();
    Util::VoidList effects;
    Util::VoidList materials;
    bool dirty;
    unsigned char _pad0[1];
    unsigned short bucketCheckpoints[7];
    unsigned int bufferUsage;
    unsigned int threadStackUsage;
    unsigned int maxBufferUsage;
    unsigned int maxThreadStackUsage;
    unsigned int buffer[10240];
};

}  // namespace Graphics

#pragma dont_inline on
Util::NodeListBase::NodeHeader* Util::VoidList::Erase(
    Util::VoidList::Iterator it) {
    NodeHeader* next = it.node->next;

    Unlink(it.node);

    return next;
}
#pragma dont_inline off

Graphics::MaterialDepot::MaterialDepot() {
    dirty = true;

    bufferUsage = 0;
    threadStackUsage = 0;
    maxBufferUsage = 0;
    maxThreadStackUsage = 0;

    customSortFunction = 0;
}

void Graphics::MaterialDepot::AddMaterial(Material* material) {
    if (material->listNode.prev == 0) {
        materials.PushBack(&material->listNode);

        dirty = true;
    }
}

void Graphics::MaterialDepot::RemoveMaterial(Material* material) {
    materials.Erase(Util::VoidList::Iterator(&material->listNode));

    dirty = true;
}

void Graphics::MaterialDepot::AddEffect(Effect* effect) {
    if (effect->listNode.prev == 0) {
        effects.PushBack(&effect->listNode);

        dirty = true;
    }
}

void Graphics::MaterialDepot::RemoveEffect(Effect* effect) {
    effects.Erase(Util::VoidList::Iterator(&effect->listNode));

    dirty = true;
}

const Graphics::Effect::Feature* Graphics::Effect::FindFeature(const LOD* lod,
                                                               int mask) const {
    const Feature* f;
    int all = 0;

    for (f = lod->features; f != lod->features + lod->count; f++) {
        all |= f->flags;
    }

    mask &= all;

    for (f = lod->features; f != lod->features + lod->count; f++) {
        if ((mask & f->flags) == mask) {
            return f;
        }
    }

    return 0;
}

Graphics::RenderMode* Graphics::Effect::GetRenderMode(
    unsigned char index) const {
    return shaderList[index].renderMode;
}

unsigned char Graphics::Effect::GetShader(const Feature* feature,
                                          int index) const {
    return (unsigned char)(feature->shaders[index] - shaderList);
}
