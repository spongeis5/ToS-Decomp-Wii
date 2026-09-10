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
// Effect::FindFeature MATCHES. The note that stood here had the
// diagnosis the wrong way round -- it said retail computed the bound
// twice and we computed it once -- and the truth is the reverse: OURS
// respelled `lod->features + lod->count` in both loop conditions, and
// retail forms an `end` cursor FROM `it`.
//
// EACH LOOP DECLARES BOTH CURSORS IN ITS OWN BLOCK, which the debug
// info states outright:
//
//   local line 768  end  Feature*  r8     local line 776  end  Feature*  r4
//   local line 768  it   Feature*  r7     local line 776  it   Feature*  r3
//
// The prologue loads `lod->features` into r3 and `lod->count * 12` into
// r4 once, so the second loop's whole setup is `add r4,r3,r4` -- the
// one instruction we were short. tools/aligndiff.py found it: 20 of 23
// words differed and every one after word 13 was retail's shifted by
// one, so it was never twenty problems.
//
// Measured over twelve spellings: `end` written as
// `lod->features + lod->count` instead of `it + lod->count` costs 14
// words, and `it < end` instead of `it != end` costs 32. `featureMask`
// as `int` rather than the `unsigned int` the DWARF names is INERT,
// and so is a `while` in an explicit block instead of the `for` --
// six of the twelve are byte-identical.

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
                                                               int featureSet) const {
    unsigned int featureMask = 0;

    for (const Feature* it = lod->features, *end = it + lod->count; it != end; it++) {
        featureMask |= it->flags;
    }

    featureSet &= featureMask;

    for (const Feature* it = lod->features, *end = it + lod->count; it != end; it++) {
        if ((featureSet & it->flags) == featureSet) {
            return it;
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
