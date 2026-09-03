// zReference.cpp -- five functions, read from the image with
// tools/disasm.py. A reference entity holds one other entity and
// forwards events to it. The asset's Create takes one 64-byte block from
// the global heap (tag 16), places the entity on it -- the constructor
// stores the class's own vtable pointer, which is why the null check and
// the base call are in the bytes -- runs xBaseInit, clears the held
// entity and installs the event callback. Setup reaches the entity's own
// asset through its handle, which IS a blobloid, and looks the asset's
// initial uid up in the scene when it is not zero. The event callback
// first resolves the event's uid, by scene lookup and then by asset
// lookup, and then answers two ids: one stores what it resolved, the
// other stores that reference's own held entity; any other id is passed
// on to the held entity when it has an event function. Save writes the
// held entity's id, or a zero when there is none. Load reads a uid and,
// when it has an entity, stores the scene lookup or a null.
//
// Layouts from the DWARF (tools/dwarf_types.py): zReference 0x40 on
// xOGEntity 0x40, with the held entity at +0x3C in the base's tail
// padding; xBase 0x38 with the handle at +0x14, the id at +0x18 and the
// event function at +0x30; World::EntityHandleBase 0x48 whose FIRST BASE
// is Domains::Blobloid, which is what lets Setup call BlobData on the
// handle with no adjustment; Sext::Reference 0x20 with its initial uid
// at +0x10. Sext::EventAny is one byte in the DWARF -- a placeholder for
// a variant -- and the bytes here read a uid out of its first eight, so
// that is how it is declared.
//
// Five shapes the bytes fixed. The two event ids are an if / else-if
// chain and not a switch: each is one `addis`/`cmplwi` pair against a
// 32-bit constant, tested in source order, and the fall-through block
// RE-READS the held entity rather than using the resolved one still in
// r5. The resolved entity is computed before the dispatch and starts
// null, so the two guards are one `if` with an `&&`: the pointer test
// and the uid test share the load that feeds the first lookup, while the
// second lookup reads the uid AGAIN -- retail has two loads there, and a
// local kept across the first call would have none.
//
// The second lookup's uid IS a local, declared inside the branch that
// makes it. Written as `FindAsset(any->id)` the whole callback was two
// words short and 43 of its 52 words differed: the read folded into the
// argument registers after the manager call, and the function then
// needed only four callee-saved registers where retail saves six. A
// named uid ahead of the call loads it into r31 and r30 and keeps it
// across `World::GetEntityManager()`, which is retail's shape and moves
// the other four locals down to r26..r29 with it. That is the same lever
// zNPCAsset.cpp records for its own FindAsset call.
//
// Load's stored value is an if/else and not a conditional expression,
// because retail's null block is a separate `li r0,0 ; stw` and not a
// folded operand. And Save's serial call takes a `long long` (the `x` in
// its mangled name), so the zero arm passes a plain zero pair.

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
class xSerial;
class zReference;

namespace Domains {

class Blobloid {
public:
    void* BlobData() const;
};

}  // namespace Domains

namespace World {

class EntityHandleBase : public Domains::Blobloid {};

}  // namespace World

namespace Sext {

class EventAny {
public:
    uid id;
};

class xBaseAsset {
public:
    uid id;
    unsigned int baseType;
    unsigned short linkCount;
    unsigned short baseFlags;
};

class Reference : public xBaseAsset {
public:
    static zReference* Create(World::EntityHandleBase* handle,
                              Reference* asset);

    uid initial;
    unsigned char _pad0[0x20 - 0x18];
};

}  // namespace Sext

void xBaseInit(xBase* base, const Sext::xBaseAsset* asset);
void xBaseSave(xBase* base, xSerial* serial);
void xBaseLoad(xBase* base, xSerial* serial);

xBase* zSceneFindEntity(uid id);

namespace World {

class EntityManager {
public:
    static Sext::xBaseAsset* FindAsset(uid id);
};

EntityManager* GetEntityManager();

}  // namespace World

class xSerial {
public:
    void Write(long long value);
    void Read(long long* value);
};

enum ForceEvent { ForceEvent_ = 0x7FFFFFFF };

void zEntEvent(xBase* from, unsigned int fromEvent, xBase* to,
               unsigned int event, Sext::EventAny* any, ForceEvent force);

class xBase {
public:
    virtual void _v0();

    unsigned char _pad0[0x14 - 0x4];
    World::EntityHandleBase* handle;
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

class zReference : public World::xOGEntity {
public:
    zReference(World::EntityHandleBase* handle) : World::xOGEntity(handle) {}

    virtual void _v1();

    void Setup();

    xBase* current;
};

void zReferenceEventCB(xBase* from, xBase* to, unsigned int event,
                       Sext::EventAny* any);

zReference* Sext::Reference::Create(World::EntityHandleBase* handle,
                                    Reference* asset) {
    zReference* ref = new (memset(
        Memory::AllocGlobalHeap(sizeof(zReference), (Memory::GlobalHeapEnum)0,
                                (eMemMgrTag)16, false),
        0, sizeof(zReference))) zReference(handle);

    xBaseInit(ref, asset);

    ref->current = 0;
    ref->eventFunc = zReferenceEventCB;

    return ref;
}

void zReference::Setup() {
    Sext::Reference* asset = (Sext::Reference*)handle->BlobData();

    if (asset->initial != 0) {
        current = zSceneFindEntity(asset->initial);
    }
}

void zReferenceEventCB(xBase* from, xBase* to, unsigned int event,
                       Sext::EventAny* any) {
    xBase* found = 0;

    if (any != 0 && any->id != 0) {
        found = zSceneFindEntity(any->id);

        if (found == 0) {
            uid id = any->id;

            found = (xBase*)World::GetEntityManager()->FindAsset(id);
        }
    }

    if (event == 0xC6EDA5CD) {
        ((zReference*)to)->current = found;
    } else if (event == 0xC52BDB26) {
        ((zReference*)to)->current = ((zReference*)found)->current;
    } else {
        xBase* current = ((zReference*)to)->current;

        if (current != 0 && current->eventFunc != 0) {
            zEntEvent(from, 0, current, event, any, (ForceEvent)1);
        }
    }
}

void zReferenceSave(zReference* ref, xSerial* serial) {
    xBaseSave(ref, serial);

    xBase* current = ref->current;

    if (current != 0) {
        serial->Write(current->id);
    } else {
        serial->Write(0);
    }
}

void zReferenceLoad(zReference* ref, xSerial* serial) {
    long long id;

    xBaseLoad(ref, serial);

    serial->Read(&id);

    if (ref != 0) {
        if (id != 0) {
            ref->current = zSceneFindEntity(id);
        } else {
            ref->current = 0;
        }
    }
}
