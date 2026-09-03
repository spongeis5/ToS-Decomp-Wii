// zNPCGroupBase.cpp -- four functions, 472 bytes, read from the image with
// tools/disasm.py. An NPC group is an xBase entity that owns an array of
// the NPCs its asset names: the constructor runs xBase's and clears the
// type and the asset, Load keeps both and installs the group's own event
// wrapper before xBaseInit, Setup walks the asset's uid list TWICE -- once
// to count the NPCs the scene can actually find, then again to fill an
// array of exactly that many pointers -- and EventWrapper tail-dispatches
// the event to the receiving entity's handler.
//
//   __ct__13zNPCGroupBaseFPQ25World16EntityHandleBase
//                       calls __ct__5xBaseFPQ25World16EntityHandleBase,
//                       stores __vt__13zNPCGroupBase at 0, zeroes 52 and 56
//   Load                stores the type, the asset and the wrapper, calls
//                       xBaseInit(xBase*, const Sext::xBaseAsset*), sets
//                       the flag halfword to 1 and points the link array
//                       at the asset's event links
//   Setup               two loops over the asset's uid array around
//                       zSceneFindObject(unsigned long long) and one
//                       xMemAlloc of npcsNum words, tag 15, heap 0
//   EventWrapper        a tail call through slot 8 of the receiver's
//                       vtable with four arguments
//
// Layouts from the DWARF (tools/dwarf_types.py): xBase 0x38 on a 0x18
// Entity, with the flag halfword at +0x26, the link array at +0x28 and the
// event function at +0x30 -- Entity's first word is the vtable pointer,
// which the DWARF never describes; zNPCGroupBase 0x48 with the type at
// +0x34, the asset at +0x38, the NPC array at +0x3C and its count at
// +0x40; Sext::NPCGroupAsset 0x20 on xBaseAsset with the NPC list's count
// and data at +0x10 and +0x14 and the event links at +0x18;
// zNPCGroupType 0x10. The DWARF gives the list's data as an untyped
// 32-bit pointer, and the BYTES give the element: two word loads eight
// bytes apart, or-ed together for the test and passed as the register
// pair a `Ux` argument takes, which is an unsigned long long.
//
// The vtable is recovered fact, read out of .data: __vt__13zNPCGroupBase
// at 806BDF00 is fourteen slots, the destructor first, Load at index 7
// and Setup at index 12, so both are virtual in retail. Nothing in this
// unit dispatches through them; what it DOES dispatch is slot 8 of the
// receiver's xBase vtable in EventWrapper, which in the group's own table
// is one of the folded empty bodies -- so xBase is declared here with
// nine virtuals and the handler last, and zNPCGroupBase declares one
// UNDEFINED virtual ahead of its own so the vtable's home stays in WAD02's
// data, where retail has it and where this unit's split has no room.
//
// Four shapes the bytes fixed.
//
// Load's flag store comes BEFORE the link array, and the reload of the
// asset is scheduled above it: retail has `lwz r3,56(r31) ; li r0,1 ;
// sth r0,38(r31) ; addi r0,r3,24 ; stw r0,40(r31)`. The asset is read
// back through the MEMBER after xBaseInit, not from the parameter still
// live in r5.
//
// Setup's first bound is a LOCAL and its second is the member. Retail
// keeps the asset's count in r30 across the whole counting loop, and
// reloads npcsNum from +0x40 in the filling loop's condition every
// iteration -- which is what a store through `npcs`, a heap pointer the
// compiler cannot prove does not alias the count, forces. Both compares
// are `cmplw`, so both indices and both bounds are unsigned.
//
// The uid is read into a local before the test. Retail's `lwzx r3,..` and
// `lwz r4,4(r4)` land the pair straight in the argument registers, the
// `or.` tests it there, and the call needs no move.
//
// The second loop's destination index is the LOOP index, not a separate
// count of the ones it stored: `addi r29,r29,4` sits in the common
// continue block with `addi r28,r28,1`, past both refusals.

typedef unsigned long long uid;

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {
enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };
}  // namespace Memory

void* xMemAlloc(Memory::GlobalHeapEnum heap, unsigned int size, int align,
                eMemMgrTag tag);

class xBase;
class TemplateEntity;
class zNPCBase;
class zNPCGroupType;

namespace World {
class EntityHandleBase;
}

namespace Sext {

class EventAny;

class LinkAsset {
public:
    unsigned int count;
    unsigned int data;
};

class xBaseAsset {
public:
    uid id;
    unsigned int baseType;
    unsigned short linkCount;
    unsigned short baseFlags;
};

class NPCGroupAsset : public xBaseAsset {
public:
    class __NPCs__ {
    public:
        unsigned int count;
        uid* data;
    };

    /* +0x10 */ __NPCs__ NPCs;
    /* +0x18 */ LinkAsset EventLinksNew;
};

}  // namespace Sext

void xBaseInit(xBase* base, const Sext::xBaseAsset* asset);
void* zSceneFindObject(uid id);

class EmbeddedListNode {
public:
    EmbeddedListNode* next;
    EmbeddedListNode* prev;
};

class xBase {
public:
    xBase(World::EntityHandleBase* handle);

    virtual void _v0();   virtual void _v1();   virtual void _v2();
    virtual void _v3();   virtual void _v4();   virtual void _v5();
    virtual void _v6();   virtual void _v7();
    virtual void HandleEvent(xBase* from, xBase* to, unsigned int event,
                             Sext::EventAny* any);

    /* +0x4  */ EmbeddedListNode ogSceneNode;
    /* +0xC  */ int ogUpdateIdx;
    /* +0x10 */ unsigned int typeID;
    /* +0x14 */ World::EntityHandleBase* handle;
    /* +0x18 */ uid id;
    /* +0x20 */ unsigned int baseType;
    /* +0x24 */ unsigned char UNUSED_linkCount;
    /* +0x25 */ unsigned char assertFlags;
    /* +0x26 */ unsigned short baseFlags;
    /* +0x28 */ Sext::LinkAsset* linkArray;
    /* +0x2C */ TemplateEntity* templateParent;
    /* +0x30 */ void (*eventFunc)(xBase* from, xBase* to, unsigned int event,
                                  Sext::EventAny* any);
};

class zNPCGroupBase : public xBase {
public:
    zNPCGroupBase(World::EntityHandleBase* handle);

    // Declared first and left undefined so the vtable's home stays where
    // retail has it, in the unity unit's data.
    virtual void _v0();

    virtual void Load(const zNPCGroupType* groupType,
                      Sext::NPCGroupAsset* groupAsset, unsigned int flags);
    virtual void Setup();

    static void EventWrapper(xBase* from, xBase* to, unsigned int event,
                             Sext::EventAny* any);

    /* +0x34 */ const zNPCGroupType* type;
    /* +0x38 */ Sext::NPCGroupAsset* asset;
    /* +0x3C */ zNPCBase** npcs;
    /* +0x40 */ unsigned int npcsNum;
};

zNPCGroupBase::zNPCGroupBase(World::EntityHandleBase* handle)
    : xBase(handle), type(0), asset(0) {
}

void zNPCGroupBase::Load(const zNPCGroupType* groupType,
                         Sext::NPCGroupAsset* groupAsset, unsigned int flags) {
    type = groupType;
    asset = groupAsset;
    eventFunc = EventWrapper;

    xBaseInit(this, groupAsset);

    baseFlags = 1;
    linkArray = &asset->EventLinksNew;
}

void zNPCGroupBase::Setup() {
    unsigned int i;
    unsigned int count = asset->NPCs.count;

    if (count == 0) {
        npcs = 0;
        return;
    }

    npcsNum = 0;

    for (i = 0; i < count; i++) {
        uid id = asset->NPCs.data[i];

        if (id != 0 && zSceneFindObject(id) != 0) {
            npcsNum++;
        }
    }

    npcs = (zNPCBase**)xMemAlloc((Memory::GlobalHeapEnum)0,
                                 npcsNum * sizeof(zNPCBase*), 0,
                                 (eMemMgrTag)15);

    for (i = 0; i < npcsNum; i++) {
        uid id = asset->NPCs.data[i];

        if (id != 0) {
            zNPCBase* npc = (zNPCBase*)zSceneFindObject(id);

            if (npc != 0) {
                npcs[i] = npc;
            }
        }
    }
}

void zNPCGroupBase::EventWrapper(xBase* from, xBase* to, unsigned int event,
                                 Sext::EventAny* any) {
    to->HandleEvent(from, to, event, any);
}
