// zWallNetGroup.cpp -- five functions, read from the image with
// tools/disasm.py. A wall-net group owns the wall nets its asset names.
// The asset's Create takes one 72-byte block from the global heap (tag
// 16), places the entity on it, runs xBaseInit and keeps the asset.
// Setup copies the asset's wall-net count, allocates that many pointers
// (tag 87) and resolves each of the asset's uids through
// zSceneFindEntity. FindWallNetWithPos walks the nets and returns the
// first one whose asset's bounds contain the position: inside bound 0,
// which is the outer box, and outside every later bound, which are the
// holes. GetWallNet range-checks an index, and GetIndexOfWallNet is the
// reverse lookup, -1 when the net is not in the group.
//
// Layouts from the DWARF (tools/dwarf_types.py): zWallNetGroup 0x48 on
// xOGEntity 0x40, with the asset at +0x3C in the base's tail padding,
// the net array at +0x40 and the count at +0x44; Sext::WallNetGroup 0x18
// with its wall-net array descriptor at +0x10 (count) and +0x14 (data),
// which the DWARF types as a 4-byte Pointer32 and which the bytes index
// eight bytes at a time as the uid array it is; zWallNet 0x60 with its
// asset at +0x3C; zWallNetAsset 0x50 with numBounds at +0x14.
//
// Four shapes the bytes fixed. The group declares a virtual, because the
// constructor stores __vt__13zWallNetGroup where zNavLink's -- a derived
// class that adds none -- stores nothing. The single `cmpwi r3,0 ; mr
// r31,r3 ; beq` after the memset is the placement new's own null test on
// memset's return, so the constructor is skipped and xBaseInit is not.
// FindWallNetWithPos's containment test is written IN the caller with
// gotos to a continuation label: as a helper it is not inlined at all,
// `inline` and `#pragma always_inline on` alike -- an out-of-line copy
// is emitted and called, 152 bytes the image does not have. NOTES puts
// the inliner's floor at four stores; this one has none and still is
// refused, so a LOOP declines it too. And the net is
// held in a local across the two calls, while the assignment after them
// re-reads wallNets[i], because a call can have written it.
//
// The last twelve words of FindWallNetWithPos were register numbers,
// and declaration order is what picks them: the five callee-saved
// values take r30 down to r26 in the order they are DECLARED, so
// retail's j(30), numBounds(29), net(28), wn(27), i(26) is a block of
// declarations at the top with the loop counter left in the for. Eight
// orders were swept (scratch agx_wng_gen.py); the natural one -- the
// result, then i, net, numBounds, j as each comes into scope -- is the
// exact mirror of retail's and aligns 46 of 58.
//
// tools/dwarf_locals.py confirms the two ends of that and not the
// middle: it names `wn` in r27 and `i` in r26, which is what the names
// here are, and gives the containment test's three values no DIE with a
// location at all -- so r28, r29 and r30 are compiler temporaries in
// retail, and the declarations that reproduce them are a statement
// about the register order and not about the original text. The same
// tool gives GetWallNet no local either, which is why its range check
// is a conditional expression: a bool set inside its own `if` gives the
// same fifteen words, and an early return gives eleven.
//
// NEAR MISS: none. All five functions are byte-identical by
// tools/unitcmp.py.

typedef unsigned long long uid;

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {
enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };

void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap, eMemMgrTag tag,
                      bool clear);
}  // namespace Memory

void* xMemAlloc(Memory::GlobalHeapEnum heap, unsigned int size, int align,
                eMemMgrTag tag);

extern "C" void* memset(void* dst, int c, unsigned long n);

inline void* operator new(unsigned long, void* p) { return p; }

class LinkAsset;
class TemplateEntity;
class xBase;
class xVec3;
class zWallNet;
class zWallNetGroup;

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

class WallNetArray {
public:
    unsigned int count;
    uid* data;
};

class WallNetGroup : public xBaseAsset {
public:
    static zWallNetGroup* Create(World::EntityHandleBase* handle,
                                 WallNetGroup* asset);

    WallNetArray WallNets;
};

}  // namespace Sext

void xBaseInit(xBase* base, const Sext::xBaseAsset* asset);

xBase* zSceneFindEntity(uid id);

class xVec3 {
public:
    float x;
    float y;
    float z;
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

class zWallNetAsset : public Sext::xBaseAsset {
public:
    int assetSize;
    int numBounds;
};

class zWallNet : public World::xOGEntity {
public:
    bool IsInsideBound(int bound, const xVec3& pos) const;

    zWallNetAsset* wallNetAsset;
};

class zWallNetGroup : public World::xOGEntity {
public:
    zWallNetGroup(World::EntityHandleBase* handle)
        : World::xOGEntity(handle) {}

    virtual void _v1();

    void Setup();
    zWallNet* FindWallNetWithPos(const xVec3& pos) const;
    zWallNet* GetWallNet(int index) const;
    int GetIndexOfWallNet(const zWallNet* net) const;

    Sext::WallNetGroup* myAsset;
    zWallNet** wallNets;
    int count;
};

zWallNetGroup* Sext::WallNetGroup::Create(World::EntityHandleBase* handle,
                                          WallNetGroup* asset) {
    zWallNetGroup* group = new (memset(
        Memory::AllocGlobalHeap(sizeof(zWallNetGroup),
                                (Memory::GlobalHeapEnum)0, (eMemMgrTag)16,
                                false),
        0, sizeof(zWallNetGroup))) zWallNetGroup(handle);

    xBaseInit(group, asset);

    group->myAsset = asset;

    return group;
}

void zWallNetGroup::Setup() {
    count = myAsset->WallNets.count;
    wallNets = (zWallNet**)xMemAlloc((Memory::GlobalHeapEnum)0,
                                     count * sizeof(zWallNet*), 0,
                                     (eMemMgrTag)87);

    for (int i = 0; i < count; i++) {
        wallNets[i] = (zWallNet*)zSceneFindEntity(myAsset->WallNets.data[i]);
    }
}

zWallNet* zWallNetGroup::FindWallNetWithPos(const xVec3& pos) const {
    int j;
    int numBounds;
    zWallNet* net;
    zWallNet* wn = 0;
    bool inside;

    for (int i = 0; i < count; i++) {
        net = wallNets[i];

        if (net != 0) {
            numBounds = net->wallNetAsset->numBounds;

            if (numBounds == 0) {
                inside = false;
                goto tested;
            }

            if (!net->IsInsideBound(0, pos)) {
                inside = false;
                goto tested;
            }

            for (j = 1; j < numBounds; j++) {
                if (net->IsInsideBound(j, pos)) {
                    inside = false;
                    goto tested;
                }
            }

            inside = true;

        tested:
            if (inside) {
                wn = wallNets[i];
                break;
            }
        }
    }

    return wn;
}

zWallNet* zWallNetGroup::GetWallNet(int index) const {
    return index >= 0 && index < count ? wallNets[index] : 0;
}

int zWallNetGroup::GetIndexOfWallNet(const zWallNet* net) const {
    int index = -1;

    for (int i = 0; i < count; i++) {
        if (wallNets[i] == net) {
            index = i;
            break;
        }
    }

    return index;
}
