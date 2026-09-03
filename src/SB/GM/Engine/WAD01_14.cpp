// WAD01_14 -- five functions, 624 bytes, read from the image with
// tools/disasm.py. The minigame-two-player camera widget and the pool
// its cameras come from.
//
//   zCamMG2PWidget::Init      xBaseInit, then the asset, the event
//                             callback and the link array
//   zMG2PCamAsset::Create     64 bytes from the global heap (tag 16),
//                             cleared, the entity placed on them and
//                             Init run; then, only while the pool is
//                             still empty, three cameras added to it
//   zCamPool<zCamMG2P>::AddCamera  440 bytes from the global heap (tag
//                             4), a zCamMG2P placed on them, and the
//                             base's AddCamera
//   zCamMG2PWidget::Activate  takes the pool's next free camera, hands
//                             it the asset, the widget's uid and two
//                             flag edits, and transitions the viewport's
//                             group onto it
//   zCamMG2PWidget::EventCB   one event id, then Activate
//
// Layouts from the DWARF (tools/dwarf_types.py, bare names): zCamMG2P
// 0x1B8 on xCam 0x148, with mAsset at +0x148, two xVec3 at +0x14C and
// +0x158, xSpringyVec3 goalPos at +0x164 and poleEnd at +0x18C -- the
// two the constructor runs -- and firstFrame at +0x1B4. zCamMG2PWidget
// 0x40 on World::xOGEntity with mAsset at +0x3C in the base's tail
// padding. zCamPoolBase 0x20 and xCam's flags at +0x94 and 64-bit owner
// at +0xA0 are the ones zCamPool.cpp already recovered; this unit adds
// the byte at +0x120 that Activate sets to 129. mg2pCamPool is the
// retail symbol at 0x80732A08.
//
// MEASURED: 4 of the 5 functions the object defines are byte-identical,
// 436 of the unit's 624 bytes, 0 extra.
//
// Four shapes the bytes fixed.
//
// THERE ARE TWO zMG2PCamAsset TYPES and the mangled names say so.
// Init is Init__14zCamMG2PWidgetFPC13zMG2PCamAsset -- thirteen
// characters, no namespace -- while Create is
// Create__Q24Sext13zMG2PCamAssetF... So the runtime asset is at global
// scope and the on-disk one is Sext's, and the DWARF has both under the
// one leaf name: __94B3E at 0x28 on xBaseAsset, and __2132D at 0x30
// with the link array. Sext's DERIVES from the global one. That is not
// a guess to make the names come out: the global type's data ends at
// 0x24 and its size is 0x28 only because the uid in xBaseAsset aligns
// it to eight, so the derived class's first member lands in that tail
// padding at 0x24 -- which is exactly the offset Init computes for the
// link array -- and 0x24 plus eight rounds back to the 0x30 the DWARF
// gives. Spelled as one class it was 17 of 46 words in Create and an
// EXTRA Init under the wrong name; spelled as two it matched.
//
// THE PRAGMA GOES AT THE END OF THE FILE. zCamMG2P's constructor is
// empty and its whole body is the implicit vtable store and the two
// xSpringyVec3 constructors on +356 and +396, which retail has inline
// inside AddCamera; -inline auto does not take it, in any of four
// spellings (implicit, empty in-class, declared and defined inline
// outside, and the class given its own constructor). always_inline
// does, but WHERE it is written decides what else it catches, and the
// three placements are three different answers:
//
//   * before the class, left on: AddCamera matches, and EventCB is
//     ruined -- 49 words against retail's 20, because Activate gets
//     inlined into it where retail tail-calls it.
//   * before the class and off after it, or around the template with
//     an off after: no effect at all, AddCamera stays 23 of 24.
//   * at the END of the file, after every function: AddCamera matches
//     and nothing else moves.
//
// The reason is that mwcc instantiates the template at the end of the
// translation unit, so the pragma's state THERE is what the
// instantiation sees, and an ordinary function compiled earlier never
// sees it. That makes the end of the file a way to aim always_inline
// at a template alone. It is still a pragma and still the closest
// measured state rather than what the original said.
//
// ACTIVATE PUTS ITS FAILURE LAST. Retail tests the camera, does the
// work, returns true and jumps over a trailing `li r3,0`; an early
// `return false` inverts the branch and puts the zero at the top --
// same instruction count, 23 of 41 words wrong. The same lever as the
// two static Plays in zSoundWiimoteSpeaker.cpp.
//
// NEAR MISS -- Create, 16 of 46 words, and it is ONE instruction, the
// string. Retail forms the pool base into r31 with a lis and an addi
// and then spells each of the three AddCamera arguments as
// `addi r4,r31,6514`; ours has the empty string at offset 0 of its own
// pool, so it needs no base and reaches it in one addi. The fix is the
// generated pool prefix every other unit with strings carries, and
// tools/gen_poolprefix.py cannot produce it here: it builds the prefix
// from the strings a unit is the FIRST to reference, and this unit is
// the first referrer of nothing -- the empty string at +6514 is the
// tail of a string some earlier file in WAD01.cpp introduced. Both
// modes refuse, --whole included, and refusing is right rather than
// wrong. What is missing is a mode that takes the prefix from a pool
// OFFSET the unit references rather than from one it introduces.

typedef unsigned long long uid;

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {
enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };

void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap, eMemMgrTag tag,
                      bool clear);
}  // namespace Memory

extern "C" void* memset(void* dst, int c, unsigned long n);

void* xMemAlloc(Memory::GlobalHeapEnum heap, unsigned int size, int align,
                eMemMgrTag tag);

inline void* operator new(unsigned long, void* p) { return p; }

class LinkAsset;
class TemplateEntity;
class xBase;
class xCam;
class xCamTransitionParams;

namespace World {
class EntityHandleBase;
}

class zCamMG2PWidget;

namespace Sext {

// Read unconditionally at +0 and then tested for null: the widget's
// callback takes the count out of it before it checks that it is there.
class EventAny {
public:
    int value;
    bool flag;
};

class xBaseAsset {
public:
    uid id;
    unsigned int baseType;
    unsigned short linkCount;
    unsigned short baseFlags;
};

class LinkAssetArray {
public:
    unsigned int count;
    unsigned int data;
};

class xVec3 {
public:
    float x;
    float y;
    float z;
};

}  // namespace Sext

// The runtime asset, at global scope: Init's mangled name says
// PC13zMG2PCamAsset, thirteen characters and no namespace. Its data
// ends at 0x24 and its size is 0x28 only because the uid in the base
// aligns it to eight.
class zMG2PCamAsset : public Sext::xBaseAsset {
public:
    Sext::xVec3 pos;
    float FOV;
    float Radius;
};

namespace Sext {

// The on-disk asset, which is the runtime one plus its links. The link
// array lands at 0x24, in the base's tail padding, which is the offset
// Init computes -- and 0x24 plus eight rounds back to the 0x30 the
// DWARF gives.
class zMG2PCamAsset : public ::zMG2PCamAsset {
public:
    LinkAssetArray EventLinksNew;

    static zCamMG2PWidget* Create(World::EntityHandleBase* handle,
                                  zMG2PCamAsset* asset);
};

}  // namespace Sext

void xBaseInit(xBase* base, const Sext::xBaseAsset* asset);

class xBase {
public:
    virtual void _v0();

    unsigned char _pad0[0x14];
    uid id;
    unsigned int baseType;
    unsigned char UNUSED_linkCount;
    unsigned char assertFlags;
    unsigned short baseFlags;
    LinkAsset* linkArray;
    TemplateEntity* templateParent;
    void (*eventFunc)(xBase* from, xBase* to, unsigned int event,
                      Sext::EventAny* any);
};

namespace World {

class xOGModelHandle {
public:
    void* model;
    unsigned int _pad0;
};

class xOGEntity : public xBase {
public:
    xOGEntity(EntityHandleBase* handle);

    xOGModelHandle ogModel;
};

}  // namespace World

// The camera and the pool, as zCamPool.cpp already recovered them; the
// byte at +0x120 is this unit's addition.
class xCam {
public:
    virtual void _v0();
    virtual void _v1();
    virtual void _v2();
    virtual void _v3();
    virtual void create();
    virtual void destroy();
    virtual void start();
    virtual void stop();

    unsigned char _pad0[0x94 - 0x4];
    int flags;
    unsigned char _pad1[0xA0 - 0x98];
    uid owner;
    unsigned char _pad2[0x120 - 0xA8];
    unsigned char activeFlags;
    unsigned char _pad3[0x148 - 0x121];
};

class xCamGroup {
public:
    void transition_to(xCam& cam, const xCamTransitionParams* params,
                       bool immediate, bool force);
};

xCamGroup* zViewportGetCamera(int viewport);

class zCamPoolBase {
public:
    void AddCamera(xCam* cam, const char* name);
    xCam* GetNextCamera();

    xCam* mCameras[6];
    int mNumCameras;
    int mNextCamera;
};

class xSpringyVec3 {
public:
    xSpringyVec3();

    unsigned char _pad0[0x28];
};

class zCamMG2P : public xCam {
public:
    zCamMG2P() {}

    ::zMG2PCamAsset* mAsset;
    Sext::xVec3 target;
    Sext::xVec3 thePole;
    xSpringyVec3 goalPos;
    xSpringyVec3 poleEnd;
    bool firstFrame;
};

template <class T>
class zCamPool : public zCamPoolBase {
public:
    void AddCamera(const char* name);
};

template <class T>
void zCamPool<T>::AddCamera(const char* name) {
    T* cam = new (xMemAlloc((Memory::GlobalHeapEnum)0, sizeof(T), 0,
                            (eMemMgrTag)4)) T();

    zCamPoolBase::AddCamera(cam, name);
}

extern zCamPool<zCamMG2P> mg2pCamPool;

class zCamMG2PWidget : public World::xOGEntity {
public:
    zCamMG2PWidget(World::EntityHandleBase* handle)
        : World::xOGEntity(handle) {}

    void Init(const ::zMG2PCamAsset* asset);
    bool Activate(int viewport, const xCamTransitionParams* params,
                  bool immediate);

    static void EventCB(xBase* from, xBase* to, unsigned int event,
                        Sext::EventAny* any);

    ::zMG2PCamAsset* mAsset;
};

void zCamMG2PWidget::Init(const ::zMG2PCamAsset* asset) {
    xBaseInit(this, asset);

    mAsset = (::zMG2PCamAsset*)asset;
    eventFunc = EventCB;
    linkArray =
        (LinkAsset*)&((const Sext::zMG2PCamAsset*)asset)->EventLinksNew;
}

bool zCamMG2PWidget::Activate(int viewport, const xCamTransitionParams* params,
                              bool immediate) {
    xCam* cam = mg2pCamPool.GetNextCamera();

    if (cam != 0) {
        ((zCamMG2P*)cam)->mAsset = mAsset;
        cam->owner = id;
        cam->activeFlags = 129;
        cam->flags = (cam->flags | 2) & ~0x40;

        zViewportGetCamera(viewport)->transition_to(*cam, params, immediate,
                                                   false);

        return true;
    }

    return false;
}

void zCamMG2PWidget::EventCB(xBase* from, xBase* to, unsigned int event,
                             Sext::EventAny* any) {
    if (event != 0x8CEBAE88) {
        return;
    }

    int viewport = any->value - 1;

    if (viewport < 0) {
        viewport = 0;
    }

    bool immediate = false;

    if (any != 0 && any->flag != 0) {
        immediate = true;
    }

    ((zCamMG2PWidget*)to)->Activate(viewport, 0, immediate);
}

zCamMG2PWidget* Sext::zMG2PCamAsset::Create(World::EntityHandleBase* handle,
                                            Sext::zMG2PCamAsset* asset) {
    zCamMG2PWidget* widget =
        new (memset(Memory::AllocGlobalHeap(sizeof(zCamMG2PWidget),
                                            (Memory::GlobalHeapEnum)0,
                                            (eMemMgrTag)16, false),
                    0, sizeof(zCamMG2PWidget))) zCamMG2PWidget(handle);

    widget->Init(asset);

    if (mg2pCamPool.mNumCameras <= 0) {
        mg2pCamPool.AddCamera("");
        mg2pCamPool.AddCamera("");
        mg2pCamPool.AddCamera("");
    }

    return widget;
}

// -inline auto does not take zCamMG2P constructor here, though it is
// empty and its whole body is the implicit vtable store and the two
// member constructors. Retail has all seven instructions inline. The
// pragma is the closest measured state, not what the original said.
#pragma always_inline on
