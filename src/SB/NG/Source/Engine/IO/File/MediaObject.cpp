// C:/branches/SB09/main/NG/Source/Engine/IO/File/MediaObject.cpp
//
// Layout from the Wii build's DWARF (tools/dwarf_types.py):
//
//   class MediaObject  /* 0x58 */
//   {
//       /* +0x0  */ MediaError error;
//       /* +0x4  */ MediaData* mediaData;
//       /* +0x8  */ enum enOpStatus opStatus;
//       /* +0xC  */ Notify notice;
//       /* +0x10 */ SyncEvent waitState;
//       /* +0x34 */ CriticalSection critStatus;
//       /* +0x54 */ bool haveWaitState;
//   };
//   class CriticalSection /* 0x20 */
//   { int refcount; OSMutex mutex; OSThread* owner; };
//
// The emitted order is the MEMBER CONSTRUCTION order, and it reads straight
// off the disassembly:
//
//   stw  r3, 0xc(r3)      notice(this)          -- Notify's constructor
//   stw  r0, 0x34(r3)     critStatus.refcount=0 } CriticalSection's,
//   addi r3, r3, 0x38     &critStatus.mutex     } inlined
//   bl   OSInitMutex
//   ... then the body: Enter, opStatus = 0, Exit, haveWaitState = false
//
// error, mediaData and opStatus have no constructor and emit nothing;
// waitState emits nothing either, so SyncEvent has none. critStatus.owner
// at +0x1C is never written, so CriticalSection's constructor sets only
// refcount and the mutex.

struct OSMutex {
    unsigned char _bytes[0x18];
};

class OSThread;

extern "C" void OSInitMutex(OSMutex* mutex);

namespace System {

class CriticalSection {
public:
    CriticalSection() {
        refcount = 0;
        OSInitMutex(&mutex);
    }

    void Enter();
    void Exit();

    int refcount;
    OSMutex mutex;
    OSThread* owner;
};

}  // namespace System

namespace IO {

class MediaObject;

class Notify {
public:
    Notify(MediaObject* owner) : mOwner(owner) {}

    MediaObject* mOwner;
};

struct SyncEvent {
    unsigned char _bytes[0x24];
};

class MediaObject {
public:
    MediaObject();

    int error;
    void* mediaData;
    int opStatus;
    Notify notice;
    SyncEvent waitState;
    System::CriticalSection critStatus;
    bool haveWaitState;
};

MediaObject::MediaObject() : notice(this) {
    critStatus.Enter();
    opStatus = 0;
    critStatus.Exit();

    haveWaitState = false;
}

}  // namespace IO
