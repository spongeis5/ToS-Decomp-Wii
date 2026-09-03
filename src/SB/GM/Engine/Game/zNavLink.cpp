// zNavLink.cpp -- seven functions, read from the image with
// tools/disasm.py. A nav link joins two nav markers and can be locked by
// one entity at a time. The asset's Create takes one 88-byte block from
// the global heap (tag 16), places the entity on it, runs xBaseInit,
// keeps the asset and installs the event wrapper, which swaps its first
// two arguments and tail-branches to HandleEvent. Reset takes the
// enabled flag from bit 0 of the asset's flags, drops the lock and marks
// it loose. IsEnabled is that flag and both markers being on. TryLock
// takes a free lock, renews its own, and otherwise -- only for a loose
// lock between two entities of base type 0x55 -- takes it from the
// holder when the caller is nearer to either marker than the holder is.
// ReleaseLock drops the lock for the entity that holds it. HandleEvent
// answers three ids: two set the enabled flag, and the third
// tail-branches to Reset.
//
// Layouts from the DWARF (tools/dwarf_types.py): zNavLink 0x58 on
// xOGEntity 0x40, with the asset at +0x3C in the base's tail padding,
// the two markers at +0x40 and +0x44, the enabled flag at +0x48, the
// holder at +0x4C and the loose flag at +0x50; Sext::NavLink 0x30 with
// its flags at +0x20; zNavMarker 0x58 with its position at +0x40;
// xVec3 0xC. The entity a lock is compared through is reached as an
// xOGEntity: the bytes read +0x34, its model handle, and then +0x30 of
// the model, which is the position at the end of the model's matrix.
//
// Four shapes the bytes fixed. Both nearest-distance choices are
// conditional EXPRESSIONS whose true side keeps the value already in
// hand, so the compiler branches to the else block and jumps past
// it. ReleaseLock's mismatch leaves through a conditional return
// (bnelr), which is what an if around the whole body gives. The
// eligibility test is an EARLY RETURN, not a condition wrapped round
// the distance work: retail places that `return false` between the
// tests and the work, and written the other way the compiler commons
// it with the final one, moves it to the end and inverts the last
// branch, which costs three words. And xBase carries its 64-bit id
// as a real member rather than padding, because that is what makes
// the object 8-aligned: padded to bytes it is 84, and retail
// allocates 88.

typedef unsigned long long uid;

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {
enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };

void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap, eMemMgrTag tag,
                      bool clear);
}  // namespace Memory

extern "C" void* memset(void* dst, int c, unsigned long n);

inline void* operator new(unsigned long, void* p) { return p; }

class LinkAsset;
class TemplateEntity;
class xBase;
class zNavLink;
class zNavMarker;
class zWallNet;

namespace World {
class EntityHandleBase;
}

namespace Sext {

class EventAny;

class xBaseAsset {
public:
    uid id;
    unsigned int baseType;
    unsigned short linkCount;
    unsigned short baseFlags;
};

class NavLink : public xBaseAsset {
public:
    static zNavLink* Create(World::EntityHandleBase* handle, NavLink* asset);

    uid Start;
    uid End;
    unsigned int Flags;
    unsigned char _pad0[0x30 - 0x24];
};

}  // namespace Sext

void xBaseInit(xBase* base, const Sext::xBaseAsset* asset);

class xVec3 {
public:
    xVec3& operator=(const xVec3& other);
    xVec3& Sub(const xVec3& a, const xVec3& b);
    float length2() const;

    float x;
    float y;
    float z;
};

class xMat4x3 {
public:
    unsigned char _pad0[0x30];
    xVec3 pos;
};

class xBase {
public:
    virtual void _v0();

    unsigned char _pad0[0x14];
    uid id;
    unsigned int baseType;
    unsigned char _pad1[0xC];
    void (*eventFunc)(xBase* from, xBase* to, unsigned int event,
                      Sext::EventAny* any);
};

namespace World {

class xOGModel {
public:
    xMat4x3 Mat;
};

class xOGModelHandle {
public:
    xOGModel* model;
    unsigned int _pad0;
};

class xOGEntity : public xBase {
public:
    xOGEntity(EntityHandleBase* handle);

    xOGModelHandle ogModel;
};

}  // namespace World

class zNavMarker : public World::xOGEntity {
public:
    bool IsOn() const;

    Sext::xBaseAsset* asset;
    xVec3 pos;
    zWallNet* wallNet;
    int triID;
};

class zNavLink : public World::xOGEntity {
public:
    zNavLink(World::EntityHandleBase* handle) : World::xOGEntity(handle) {}

    void Reset();
    bool IsEnabled() const;
    bool TryLock(const xBase* who, bool loose);
    void ReleaseLock(const xBase* who);
    void HandleEvent(xBase* from, unsigned int event, Sext::EventAny* any);

    Sext::NavLink* asset;
    zNavMarker* start;
    zNavMarker* end;
    bool enabled;
    xBase* lockedBy;
    bool isLooseLock;
};

void NavLinkEventWrapper(xBase* from, xBase* to, unsigned int event,
                         Sext::EventAny* any) {
    ((zNavLink*)to)->HandleEvent(from, event, any);
}

zNavLink* Sext::NavLink::Create(World::EntityHandleBase* handle,
                                NavLink* asset) {
    zNavLink* link = new (memset(
        Memory::AllocGlobalHeap(sizeof(zNavLink), (Memory::GlobalHeapEnum)0,
                                (eMemMgrTag)16, false),
        0, sizeof(zNavLink))) zNavLink(handle);

    xBaseInit(link, asset);

    link->asset = asset;
    link->eventFunc = NavLinkEventWrapper;

    return link;
}

void zNavLink::Reset() {
    enabled = (asset->Flags & 1) != 0;
    lockedBy = 0;
    isLooseLock = true;
}

bool zNavLink::IsEnabled() const {
    bool on = false;

    if (enabled && start->IsOn() && end->IsOn()) {
        on = true;
    }

    return on;
}

bool zNavLink::TryLock(const xBase* who, bool loose) {
    xBase* holder = lockedBy;

    if (holder == 0) {
        lockedBy = (xBase*)who;
        isLooseLock = loose;

        return true;
    }

    if (holder == who) {
        isLooseLock = loose;

        return true;
    }

    if (!isLooseLock || who->baseType != 0x55 || holder->baseType != 0x55) {
        return false;
    }

    xVec3 startPos;
    xVec3 endPos;
    xVec3 whoToStart;
    xVec3 whoToEnd;
    xVec3 holderToStart;
    xVec3 holderToEnd;

    startPos = start->pos;
    endPos = end->pos;

    whoToStart.Sub(((World::xOGEntity*)who)->ogModel.model->Mat.pos,
                   startPos);

    float whoNearest = whoToStart.length2();

    whoToEnd.Sub(((World::xOGEntity*)who)->ogModel.model->Mat.pos, endPos);

    float whoOther = whoToEnd.length2();

    holderToStart.Sub(((World::xOGEntity*)holder)->ogModel.model->Mat.pos,
                      startPos);

    float holderNearest = holderToStart.length2();

    holderToEnd.Sub(((World::xOGEntity*)holder)->ogModel.model->Mat.pos,
                    endPos);

    float holderOther = holderToEnd.length2();

    whoNearest = whoNearest < whoOther ? whoNearest : whoOther;
    holderNearest = holderNearest < holderOther ? holderNearest : holderOther;

    if (whoNearest < holderNearest) {
        lockedBy = (xBase*)who;
        isLooseLock = loose;

        return true;
    }

    return false;
}

void zNavLink::ReleaseLock(const xBase* who) {
    if (lockedBy == who) {
        lockedBy = 0;
        isLooseLock = true;
    }
}

void zNavLink::HandleEvent(xBase* from, unsigned int event,
                           Sext::EventAny* any) {
    switch (event) {
    case 0x28BB:
        enabled = true;
        break;
    case 0x0014D3DF:
        enabled = false;
        break;
    case 0xA8B93047:
        Reset();
        break;
    }
}
