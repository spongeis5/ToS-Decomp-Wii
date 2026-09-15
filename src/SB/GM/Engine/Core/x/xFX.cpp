#include "SB/GM/Engine/Core/x/xFX.pool.h"

// xFX -- the ribbon effect, FX::Ribbon::xFXRibbon: a trail of joints kept
// in a tier_queue whose storage is blocks from one shared allocator, a
// colour and width curve evaluated along the trail, and the pool of 32
// activity records a ribbon borrows while it is live. Read from the image
// with tools/disasm.py; layouts and names are the DWARF's, and the queue
// is containers.h's, emitted here as weak copies.
//
// Not written: update, refresh_joint and the iterator form of render, past
// the four-literal wall; scene_enter and the allocator's alloc_block, which
// call into WAD00.cpp's anonymous namespace (xMemWatermark::PushMemory).

class xVec3 {
public:
    xVec3& operator=(const xVec3& other);
    void Sub(const xVec3& a, const xVec3& b);
    float length2() const;

    float x;
    float y;
    float z;
};

class xVec2 {
public:
    float x;
    float y;
};

class RGBA_U8s {
public:
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
};

class xColor {
public:
    union {
        unsigned int rgbaU32;
        RGBA_U8s rgbaU8s;
    };
};

// Not inline, so this unit emits it as retail does; its one use, in
// load_default_config, is still folded in line.
unsigned int xColorU32FromRGBA(unsigned char r, unsigned char g,
                               unsigned char b, unsigned char a) {
    return (r << 24) | (g << 16) | (b << 8) | a;
}

namespace Math {

template <class T>
inline void Lerp(T& out, const T& a, const T& b, float t) {
    out = a * (1.0f - t) + b * t;
}

}  // namespace Math

class ImmediatePrototypeEntity;

namespace World {

class Entity;

class EntityHandleBase {
public:
    unsigned char _pad0[0x38];
    Entity* entity;
};

class EntityManager {
public:
    static EntityHandleBase* FindHandle(unsigned long long id);
};

}  // namespace World

class zFXScriptSpawnPtMgr {
public:
    static void UpdateAll(float dt);
};

class zCollectibleInstancer {
public:
    static void UpdateAll(float dt);
};

// ---------------------------------------------------------------------------
// containers.h: a queue stored in fixed-size blocks that an allocator hands
// out from a ring of free ones. Every use of these members in the image is
// a call, so they are defined with inlining off; operator+ and size are the
// two that retail writes in place.

class tier_queue_allocator {
public:
    class block_data {
    public:
        unsigned char prev;
        unsigned char next;
        unsigned short flags;
        void* data;
    };

    unsigned char alloc_block(const unsigned int tag);

#pragma dont_inline on
    void free_block(unsigned char index) {
        block_data& block = blocks[index];

        block.next = head;
        block.prev = blocks[head].prev;
        blocks[block.prev].next = index;
        blocks[block.next].prev = index;
        head = index;
    }
#pragma dont_inline off

    unsigned long log2_ceil(unsigned long v) const;

    block_data* blocks;
    unsigned long _unit_size;
    unsigned long _block_size;
    unsigned long _block_size_shift;
    unsigned long _max_blocks;
    unsigned long _max_blocks_shift;
    unsigned char head;
};

template <class T>
class tier_queue {
public:
    class iterator {
    public:
#pragma dont_inline on
        // The sum in its own local, as retail computes it in the
        // argument's register.
        iterator& operator+=(int n) { unsigned long next = it + n; it = next & owner->wrap_mask; return *this; }
        T& operator*() const { return ((T*)owner->alloc->blocks[owner->blocks[it >> owner->alloc->_block_size_shift]].data)[it & (owner->alloc->_block_size - 1)]; }
#pragma dont_inline off

        iterator operator+(int n) const {
            iterator t = *this;
            t += n;
            return t;
        }

        unsigned long it;
        tier_queue* owner;
    };

    unsigned long size() const { return _size; }

#pragma dont_inline on
    // NEAR MISS: 6 of 17 words, all the allocator in r5 and its head in r6
    // where retail has them the other way round. This branch shape took it
    // from 14 (the test as an early return; one return with && was 15), and
    // a ternary or the head in a local give the same 6.
    bool front_full() const {
        if (alloc->head == alloc->blocks[alloc->head].next)
            return (first & (alloc->_block_size - 1)) == 0;

        return false;
    }

    // NEAR MISS: 29 of 33 words. Retail loads alloc twice, once for the
    // block size and once for the shift, and keeps i in r31 and end in
    // r30; ours loads it once and has the pair the other way round.
    // Declaring end first, and either field or both through an accessor:
    // all 29.
    void clear() {
        unsigned long i = first >> alloc->_block_size_shift;
        unsigned long end = (unsigned char)(i + ((_size + alloc->_block_size - 1) >> alloc->_block_size_shift));

        for (; i != end; i = (unsigned char)(i + 1))
            alloc->free_block(blocks[i]);

        first = _size = 0;
    }

    // NEAR MISS: 22 of 31 words. Retail works out the block index before
    // it calls alloc_block, holding it in r31 and `this` in r30, and reads
    // `first` twice; ours reads the index after the call. The index in a
    // local, or either field through an accessor, made it worse (24).
    void push_front(const unsigned int tag) {
        ++_size;
        unsigned long oldfirst = first;
        first = (first - 1) & wrap_mask;

        if ((oldfirst & (alloc->_block_size - 1)) == 0) {
            blocks[first >> alloc->_block_size_shift] =
                alloc->alloc_block(tag);
        }
    }

    // NEAR MISS: 17 of 23 words. Retail reloads alloc for the shift and
    // keeps `this` in r7; ours shares one load and uses r8. Either field
    // through an accessor, the test's operands swapped or turned into an
    // early return, and no `last` local: all 17.
    void pop_back() {
        if (_size <= 1) {
            clear();
            return;
        }

        --_size;

        {
            unsigned long last = (first + _size) & wrap_mask;

            if ((last & (alloc->_block_size - 1)) == 0)
                alloc->free_block(blocks[last >> alloc->_block_size_shift]);
        }
    }

    iterator begin() const { iterator it; it.it = first; it.owner = (tier_queue*)this; return it; }
    // NEAR MISS: 6 of 11 words. Retail opens its frame before any load and
    // ands the sum (r5) with the mask (r0); ours loads two fields first and
    // has the operands the other way round. The mask first, the owner
    // stored first, the sum in a local, and a definition outside the
    // class: all 6.
    iterator end() const { iterator it; it.it = (first + _size) & wrap_mask; it.owner = (tier_queue*)this; return it; }
#pragma dont_inline off

    unsigned long first;
    unsigned long _size;
    unsigned long wrap_mask;
    tier_queue_allocator* alloc;
    unsigned char blocks[128];
};

// ---------------------------------------------------------------------------

namespace FX {
namespace Ribbon {

class xFXRibbon {
public:
    class joint_data {
    public:
        unsigned int born;
        xVec3 loc;
        xVec3 norm;
    };

    class curve_node {
    public:
        float time;
        xColor color;
        float scale;
    };

    class config_type {
    public:
        float width;
        float life_time;
        int flags;
        curve_node* curve;
        int curve_size;
        unsigned long long entity;
        xVec2 mUVanim;
        unsigned char mTexAnim[0xC];
    };

    class activity_data {
    public:
        xFXRibbon* owner;
        activity_data** position;
        tier_queue<joint_data> joints;
        int curve_index;
        float ilife;
        unsigned int mtime;
        unsigned int mlife;
        bool mFadeOut;
        bool mExtendMode;
        float currentFrame;
        float alphaModifier;
        float texture_offset;
        float texture_increment_scale;
        ImmediatePrototypeEntity* entity;
    };

    void create();
    void clear();
    static void load_default_config(config_type& config);
    void set_config(const config_type* config);
    bool add_joint(const xVec3& loc);
    void insert(const xVec3& loc, const xVec3& norm, int);
    void insert(const xVec3& loc, int);
    void activate();
    void deactivate();
    void StartFade();
    void Reset();
    void update(float dt);
    void render();
    void refresh_joint(joint_data& joint,
                       const tier_queue<joint_data>::iterator& it);
    void eval_joint(const joint_data& joint, xColor& color, float& ioWidth);
    void render(tier_queue<joint_data>::iterator first, unsigned long size);
    static void scene_enter();
    static void update_all(float dt);
    float time_remaining();
    int size() const;
    bool empty() const;

    // A joint's age in seconds. time_remaining takes it back from this
    // inline: spelled in place, the product fuses into the subtraction
    // (fnmsubs) where retail multiplies first.
    float joint_age(const joint_data& joint) const {
        return 0.001f * (act->mtime - joint.born);
    }

    activity_data* act;
    config_type* cfg;
};

namespace Local {

extern tier_queue_allocator joint_alloc;
extern xFXRibbon::activity_data* activities[32];
extern int activities_used;

// Weak, and a call from eval_joint.
inline void lerp(xColor& out, float t, xColor c0, xColor c1) {
    float r, g, b, a;

    Math::Lerp(r, (float)c0.rgbaU8s.r, (float)c1.rgbaU8s.r, t);
    Math::Lerp(g, (float)c0.rgbaU8s.g, (float)c1.rgbaU8s.g, t);
    Math::Lerp(b, (float)c0.rgbaU8s.b, (float)c1.rgbaU8s.b, t);
    Math::Lerp(a, (float)c0.rgbaU8s.a, (float)c1.rgbaU8s.a, t);
    out.rgbaU8s.r = r + 0.5f;
    out.rgbaU8s.g = g + 0.5f;
    out.rgbaU8s.b = b + 0.5f;
    out.rgbaU8s.a = a + 0.5f;
}

}  // namespace Local

}  // namespace Ribbon
}  // namespace FX

using namespace FX::Ribbon;

void xFXUpdate(float dt) {
    xFXRibbon::update_all(dt);
    zFXScriptSpawnPtMgr::UpdateAll(dt);
    zCollectibleInstancer::UpdateAll(dt);
}

void xFXRibbon::create() {
    act = 0;
    cfg = 0;
}

void xFXRibbon::clear() {
    if (act) {
        if (act->joints.size() != 0)
            act->joints.clear();

        deactivate();
    }
}

void xFXRibbon::load_default_config(config_type& config) {
    config.width = 1.0f;
    config.life_time = 1.0f;
    config.flags = 0;

    static curve_node default_curve[2] = {
        { 0.0f, xColorU32FromRGBA(255, 255, 255, 255), 1.0f },
        { 1.0f, xColorU32FromRGBA(255, 255, 255, 255), 1.0f },
    };

    config.curve = default_curve;
    config.curve_size = 2;
}

void xFXRibbon::set_config(const config_type* config) {
    cfg = (config_type*)config;

    if (act) {
        World::EntityHandleBase* goodie =
            World::EntityManager::FindHandle(config->entity);

        if (goodie)
            act->entity = (ImmediatePrototypeEntity*)goodie->entity;

        act->ilife = 1.0f / cfg->life_time;
        act->mlife = (unsigned int)(1000.0f * cfg->life_time);
    }
}

bool xFXRibbon::add_joint(const xVec3& loc) {
    activate();

    if (!act)
        return false;

    if (act->joints.size() > 1 && act->mExtendMode) {
        float minDist2 = 0.0f;
        tier_queue<joint_data>::iterator first = act->joints.begin();
        tier_queue<joint_data>::iterator second = first + 1;

        // Sub into a named vector: through an operator- returning one, the
        // result is copied before length2 is called.
        xVec3 diff;
        diff.Sub(loc, (*second).loc);
        float d2second = diff.length2();

        if (d2second > minDist2) {
            act->mExtendMode = false;
            act->joints.push_front(14);
        }
    } else {
        act->mExtendMode = true;

        while (act->joints.front_full() && act->joints.size() != 0)
            act->joints.pop_back();

        if (act->joints.front_full())
            return false;

        act->joints.push_front(14);
    }

    return true;
}

void xFXRibbon::insert(const xVec3& loc, const xVec3& norm, int) {
    if (add_joint(loc)) {
        joint_data& joint = *act->joints.begin();

        joint.loc = loc;
        joint.norm = norm;
        joint.born = act->mtime;
    }
}

void xFXRibbon::insert(const xVec3& loc, int) {
    if (add_joint(loc)) {
        joint_data& joint = *act->joints.begin();

        joint.loc = loc;
        joint.born = (unsigned int)-1;
    }
}

void xFXRibbon::activate() {
    if (act)
        return;

    if (Local::activities_used >= 32)
        return;

    act = Local::activities[Local::activities_used];
    Local::activities_used++;

    act->owner = this;
    act->mtime = 0;
    act->currentFrame = 0.0f;
    act->alphaModifier = 1.0f;
    act->texture_offset = 0.0f;
    act->texture_increment_scale = 1.0f;
    act->mFadeOut = false;
    act->mExtendMode = false;

    set_config(cfg);
}

void xFXRibbon::deactivate() {
    if (act == 0 || Local::activities_used == 0)
        return;

    Local::activities_used--;

    activity_data** move_act_position = act->position;
    activity_data* move_act = *move_act_position;
    activity_data** tail_act_position =
        &Local::activities[Local::activities_used];

    *move_act_position = *tail_act_position;
    (*move_act_position)->position = move_act_position;
    *tail_act_position = move_act;
    move_act->position = tail_act_position;

    act = 0;
}

void xFXRibbon::StartFade() {
    if (!act)
        return;

    act->mFadeOut = true;
}

void xFXRibbon::Reset() {
    if (!act)
        return;

    act->mFadeOut = false;
    act->mExtendMode = false;
    act->joints.clear();
}

void xFXRibbon::render() {
    if (act) {
        act->curve_index = cfg->curve_size - 2;

        int size = act->joints.size();

        if (size >= 2)
            render(act->joints.begin() + size, size);
    }
}

void xFXRibbon::eval_joint(const joint_data& joint, xColor& color,
                           float& ioWidth) {
    if (!act || cfg->curve_size < 1)
        return;

    if (cfg->curve_size == 1) {
        color = cfg->curve[0].color;
        ioWidth *= cfg->curve[0].scale;
        return;
    }

    float frac = act->ilife * (0.001f * (act->mtime - joint.born));

    if (frac > 1.0f)
        frac = 1.0f;

    curve_node* curve = cfg->curve;

    for (; act->curve_index > 0; act->curve_index--) {
        if (frac >= curve[act->curve_index].time &&
            frac <= curve[act->curve_index + 1].time)
            break;
    }

    curve_node& node0 = curve[act->curve_index];
    curve_node& node1 = curve[act->curve_index + 1];
    float ifrac = 1.0f / (node1.time - node0.time);
    float subfrac = ifrac * (frac - node0.time);

    Local::lerp(color, frac, node0.color, node1.color);

    float scale;
    Math::Lerp(scale, node0.scale, node1.scale, subfrac);

    ioWidth *= scale;
}

// Out of line: its only caller in the image, scene_enter, is not written
// here, and an inline member nothing calls is not emitted.
unsigned long tier_queue_allocator::log2_ceil(unsigned long v) const {
    unsigned long power = 0;

    while (v > 1) {
        v >>= 1;
        power++;
    }

    return power;
}

void xFXRibbon::update_all(float dt) {
    activity_data** act = Local::activities,
                  **end_act = Local::activities + Local::activities_used;

    for (; act != end_act; ++act) {
        (*act)->owner->update(dt);
    }
}

float xFXRibbon::time_remaining() {
    if (empty())
        return 0.0f;

    return cfg->life_time - joint_age(*act->joints.begin());
}

int xFXRibbon::size() const {
    if (act == 0)
        return 0;

    return act->joints.size();
}

bool xFXRibbon::empty() const {
    if (act == 0)
        return true;

    return act->joints.size() == 0;
}

// Called in the image only from functions not written here; taking their
// addresses is what has the compiler emit them.
static tier_queue<xFXRibbon::joint_data>::iterator (tier_queue<xFXRibbon::joint_data>::*const kKeepEnd)() const =
    &tier_queue<xFXRibbon::joint_data>::end;
static void (*const kKeepLerp)(float&, const float&, const float&, float) =
    &Math::Lerp<float>;
