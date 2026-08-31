// C:/branches/SB09/main/GM/Engine/Core/x/xBase.cpp
//
// Layout from the Wii build's DWARF (tools/dwarf_types.py):
//
//   class xBase /* 0x38 */ {
//       /* +0x0  */ Entity _base0;              // 0x18
//       /* +0x18 */ unsigned long long id;
//       /* +0x20 */ unsigned int baseType;
//       /* +0x24 */ unsigned char UNUSED_linkCount;
//       /* +0x25 */ unsigned char assertFlags;
//       /* +0x26 */ unsigned short baseFlags;
//       /* +0x28 */ LinkAsset* linkArray;
//       ...
//   };
//   class xBaseAsset /* 0x10 */ {
//       /* +0x0 */ uid id;  /* +0x8 */ unsigned int baseType;
//       /* +0xC */ unsigned short linkCount;
//       /* +0xE */ unsigned short baseFlags;
//   };
//
// The id is a 64-bit field and moves as two words, which is why the loads
// come in pairs and the stores land at 0x18/0x1C.
//
// xBaseSave writes its flag through TWO DIFFERENT STACK SLOTS -- 0xC in one
// arm and 8 in the other. That is not scheduling: it is two separate
// locals, one declared inside each branch, and writing it with a single
// local hoisted above the `if` would use one slot.
//
// xBaseLoad's clear is `rlwinm r0, r0, 0, 16, 30`, which keeps bits 16..30
// of the register -- for a value that arrived through `lhz`, that is
// exactly `&= ~1`.

typedef unsigned long long uid;

class Entity;
class LinkAsset;
class TemplateEntity;

namespace World {
class EntityHandleBase;
}

namespace Sext {
struct xBaseAsset {
    uid id;
    unsigned int baseType;
    unsigned short linkCount;
    unsigned short baseFlags;
};
}

struct xBase {
    unsigned char _base0[0x18];
    uid id;
    unsigned int baseType;
    unsigned char UNUSED_linkCount;
    unsigned char assertFlags;
    unsigned short baseFlags;
    LinkAsset* linkArray;
    TemplateEntity* templateParent;
    void (*eventFunc)();
};

namespace World {
struct EntityHandleBase {
    uid id;
    unsigned char _mid[0x34 - 0x8];
    unsigned int type;
};
}

class xSerial {
public:
    void Write(char* data, int size, int bits);
    void Read(char* data, int size, int bits);
};

void xBaseInit(xBase* b, const Sext::xBaseAsset* asset) {
    b->id = asset->id;
    b->baseType = asset->baseType;
    b->baseFlags = asset->baseFlags;
    b->linkArray = 0;
}

void xBaseInit(xBase* b) {
    b->id = 0;
    b->baseType = 0;
    b->baseFlags = 0;
    b->linkArray = 0;
}

void xBaseInit(xBase* b, World::EntityHandleBase* handle) {
    b->id = handle->id;
    b->baseType = handle->type;
}

void xBaseSave(xBase* b, xSerial* s) {
    if (b->baseFlags & 1) {
        int enabled = 1;
        s->Write((char*)&enabled, 4, -1);
    } else {
        int enabled = 0;
        s->Write((char*)&enabled, 4, -1);
    }
}

void xBaseLoad(xBase* b, xSerial* s) {
    int enabled = 0;

    s->Read((char*)&enabled, 4, -1);

    if (b) {
        if (enabled) {
            b->baseFlags |= 1;
        } else {
            b->baseFlags &= ~1;
        }
    }
}

void xBaseReset(xBase* b, Sext::xBaseAsset* asset) {
    b->baseFlags = asset->baseFlags;
}
