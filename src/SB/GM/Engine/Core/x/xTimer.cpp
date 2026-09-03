// xTimer.cpp -- eight functions, read from the image with
// tools/disasm.py. A timer entity counts a number of seconds down and
// sends an event when it reaches zero. GetRandomizedTime turns the
// asset's seconds and random range into a starting time: with no range
// it is the plain seconds, and otherwise a random offset in thousandths
// either side of it. The asset's Create takes one 72-byte block from the
// global heap (tag 16), places the entity on it and runs xTimerInit,
// which runs xBaseInit, installs the event callback, points the link
// array at the asset's links, keeps the asset, clears the state and the
// flags, takes the randomized time and copies the run-in-pause byte.
// Reset re-reads the asset's base flags into the entity's, clears the
// state and takes a fresh randomized time. Save and Load write and read
// the state byte and the remaining seconds, Load reading into the stack
// when it has no entity. The event callback answers seven ids, two of
// them sharing the reset body, and Update counts down while the state is
// running and sends the expiry event when the time crosses zero.
//
// Layouts from the DWARF (tools/dwarf_types.py): xTimer 0x48 on
// xOGEntity 0x40, with the asset at +0x3C in the base's tail padding,
// the state at +0x40, the run-in-pause byte at +0x41, the flags at
// +0x42 and the remaining seconds at +0x44; the timer asset 0x20 with
// its seconds at +0x10, random range at +0x14 and run-in-pause byte at
// +0x18. The asset type is named twice in the image's symbols: the
// entity's Create takes Sext::xTimerAsset and the init and randomizer
// take a global xTimerAsset, so both names are declared here. The two
// literals (a thousand, and zero) are the translation unit's pool,
// hence the generated header first.
//
// Four shapes the bytes fixed. The randomized offset is converted as a
// SIGNED int -- the magic constant the compiler pairs with it is the
// signed one -- while the modulo above it is unsigned, which is what
// makes the divide a divwu. The two ids that share the reset body are
// one case pair of a switch, since retail tests them in sequence and
// branches both to the same tail call. Init stores the asset BEFORE the
// link array, which is the order of the two stores in the bytes and not
// the order the fields sit in. And Update sends its event from inside
// an if on secondsLeft <= 0, not after an early return on the opposite
// test: the compare against zero carries the cror that an ordered <=
// needs, where a > test leaves a bare bgtlr and costs a word; the event
// is also sent FROM the timer, so its first argument is the timer and
// not null.

#include "SB/GM/Engine/Core/x/xTimer.pool.h"

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {
enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };

void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap, eMemMgrTag tag,
                      bool clear);
}  // namespace Memory

extern "C" void* memset(void* dst, int c, unsigned long n);

inline void* operator new(unsigned long, void* p) { return p; }

typedef unsigned long long uid;

class LinkAsset;
class TemplateEntity;
class xBase;
class xScene;
class xTimer;

namespace World {
class EntityHandleBase;
}

namespace Sext {

class EventAny {
public:
    float value;
};

class xBaseAsset {
public:
    uid id;
    unsigned int baseType;
    unsigned short linkCount;
    unsigned short baseFlags;
};

}  // namespace Sext

// The asset, under the name the init and the randomizer use.
class xTimerAsset : public Sext::xBaseAsset {
public:
    float seconds;
    float randomRange;
    unsigned char runsInPause;
    unsigned char pad[3];
    unsigned char EventLinksNew[4];
};

namespace Sext {

// And under the name the entity's Create uses.
class xTimerAsset {
public:
    static xTimer* Create(World::EntityHandleBase* handle, xTimerAsset* asset);
};

}  // namespace Sext

void xBaseInit(xBase* base, const Sext::xBaseAsset* asset);
void xBaseSave(xBase* base, class xSerial* serial);
void xBaseLoad(xBase* base, class xSerial* serial);

unsigned int xrand_GenRandInt32();

class xSerial {
public:
    void Write(char* data, int size, int count);
    void Read(char* data, int size, int count);
};

enum ForceEvent { ForceEvent_ = 0x7FFFFFFF };

void zEntEvent(xBase* from, unsigned int fromEvent, xBase* to,
               unsigned int event, Sext::EventAny* any, ForceEvent force);

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

class xTimer : public World::xOGEntity {
public:
    xTimer(World::EntityHandleBase* handle) : World::xOGEntity(handle) {}

    xTimerAsset* tasset;
    unsigned char state;
    unsigned char runsInPause;
    unsigned short flags;
    float secondsLeft;
};

void xTimerInit(xBase* base, xTimerAsset* asset);
void xTimerReset(xTimer* timer);
void xTimerEventCB(xBase* from, xBase* to, unsigned int event,
                   Sext::EventAny* any);

float GetRandomizedTime(xTimerAsset* asset) {
    unsigned int range = (unsigned int)(1000.0f * asset->randomRange);

    if (range == 0) {
        return asset->seconds;
    }

    int offset = xrand_GenRandInt32() % (range * 2) - range;

    return asset->seconds + offset / 1000.0f;
}

xTimer* Sext::xTimerAsset::Create(World::EntityHandleBase* handle,
                                  xTimerAsset* asset) {
    xTimer* timer = new (memset(
        Memory::AllocGlobalHeap(sizeof(xTimer), (Memory::GlobalHeapEnum)0,
                                (eMemMgrTag)16, false),
        0, sizeof(xTimer))) xTimer(handle);

    xTimerInit(timer, (::xTimerAsset*)asset);

    return timer;
}

void xTimerInit(xBase* base, xTimerAsset* asset) {
    xTimer* timer = (xTimer*)base;

    xBaseInit(base, asset);

    timer->eventFunc = xTimerEventCB;
    timer->tasset = asset;
    timer->linkArray = (LinkAsset*)asset->EventLinksNew;
    timer->state = 0;
    timer->secondsLeft = GetRandomizedTime(asset);
    timer->runsInPause = asset->runsInPause;
    timer->flags = 0;
}

void xTimerReset(xTimer* timer) {
    xTimerAsset* asset = timer->tasset;

    timer->baseFlags = asset->baseFlags;
    timer->state = 0;
    timer->secondsLeft = GetRandomizedTime(asset);
    timer->flags = 0;
}

void xTimerSave(xTimer* timer, xSerial* serial) {
    xBaseSave(timer, serial);

    unsigned char state = timer->state;

    serial->Write((char*)&state, 1, 1);

    float secondsLeft = timer->secondsLeft;

    serial->Write((char*)&secondsLeft, 4, 1);
}

void xTimerLoad(xTimer* timer, xSerial* serial) {
    unsigned char state;
    float secondsLeft;

    xBaseLoad(timer, serial);

    if (timer) {
        serial->Read((char*)&timer->state, 1, 1);
        serial->Read((char*)&timer->secondsLeft, 4, 1);
    } else {
        serial->Read((char*)&state, 1, 1);
        serial->Read((char*)&secondsLeft, 4, 1);
    }
}

void xTimerEventCB(xBase* from, xBase* to, unsigned int event,
                   Sext::EventAny* any) {
    xTimer* timer = (xTimer*)to;

    switch (event) {
    case 0x0015A4AF:
        timer->state = 1;
        break;
    case 0x0B3550F2:
        if (timer->state == 1) {
            timer->state = 0;
        }
        break;
    case 0x389E01C0:
    case 0xA8B93047:
        xTimerReset(timer);
        break;
    case 0xAFC4A7FD:
        timer->state = 0;
        break;
    case 0x2161051D:
        timer->secondsLeft = any->value;
        break;
    case 0x215C4DE8:
        timer->secondsLeft = timer->secondsLeft + any->value;
        break;
    }
}

void xTimerUpdate(xBase* base, xScene* scene, float dt) {
    xTimer* timer = (xTimer*)base;

    if (timer->state != 1) {
        return;
    }

    timer->secondsLeft = timer->secondsLeft - dt;

    if (timer->secondsLeft <= 0.0f) {
        zEntEvent(timer, 0, timer, 0xAFC4A7FD, 0, (ForceEvent)1);
    }
}
