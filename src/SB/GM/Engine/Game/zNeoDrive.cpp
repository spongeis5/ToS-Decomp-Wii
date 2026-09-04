// zNeoDrive.cpp -- eighteen functions, 5,168 bytes, read from the image
// with tools/disasm.py. A zNeoDrivenLink attaches one entity to another
// so that it rides it: the links live in a static block-allocated array
// and form a parent/sibling/child tree, and ProcessMoves walks the array
// every frame moving the ones whose flags say to.
//
// Layouts from the DWARF (tools/dwarf_types.py): zNeoDrivenLink 0xF8 --
// three xMat4x3 at +0, +0x40 and +0x80, the driver and passenger at
// +0xC0 and +0xC4, four quick links from +0xC8, the count at +0xD8,
// flags +0xDC, bone +0xE0, yaw +0xE4 and the four tree pointers from
// +0xE8. Util::BlockAllocatorArray is 0x20 with blockSize at +4, size at
// +8, the block pool at +0xC, the back block at +0x10 and the count at
// +0x14; a block is a pointer to its elements and a next pointer, which
// is what operator[] walks. The array's element stride, 248, is
// sizeof(zNeoDrivenLink) and confirms the type.
//
// MEASURED: 8 of the 9 functions this object defines are
// byte-identical, 676 bytes -- and two of the eight are the xMat3x3 and
// xMat4x3 assignment operators, which come free from spelling xMat4x3
// as deriving from xMat3x3, the shape iCameraNG.cpp recovered. Nine of
// the unit's eighteen are not written: AddChild (1,920 bytes),
// RemoveChild (656), Moved, MOVE, DriveStatus, DriveReset,
// SetParentDriverLink, openSpot, Clear, DeleteBlocks and PushBlock.
//
// Three shapes the bytes fixed, each one word or two:
//
//   * findPassenger compares `entity == list[i].passenger`, not the
//     other way round -- retail's cmplw takes the parameter first.
//   * OldestSibling returns the PARAMETER on the null path, not a
//     literal zero: retail's early exit is a bare `blr` with r3 still
//     holding the argument.
//   * ProcessMoves tests `flags & 8`, which is the rlwinm on bit 28.
//
// NEAR MISS -- BlockAllocatorArray<zNeoDrivenLink>::operator[], 6 of 13
// words, and every one of the six is a register NUMBER. Retail
// overwrites r3 -- `this` -- with blockPool immediately and keeps the
// quotient in r5; ours puts the block in r5 and the quotient in r4.
// Every displacement, the divw., the mullw, the mtctr, the counted loop
// and the 248-byte stride agree. Four spellings tie at 6: the three
// locals in both orders, the within-block index written inline at the
// return, and the block walk as a counted for rather than a while on
// the quotient. It is the same allocator question as the count-and-
// pointer pair NOTES.md records, in a function too small to hold the
// third local that answered it twice.

typedef unsigned long long uid;

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {
enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };
}

class xVec3 {
public:
    float x;
    float y;
    float z;
};

class xMat3x3 {
public:
    xVec3 left;
    int flags;
    xVec3 up;
    unsigned int pad1;
    xVec3 at;
    unsigned int pad2;
};

class xMat4x3 : public xMat3x3 {
public:
    xVec3 pos;
    unsigned int pad3;
};

extern xMat4x3 g_I3;

namespace World {
class xOGEntity;
}

namespace Util {

template <class T>
class BlockAllocatorArray {
public:
    class Block {
    public:
        T* data;
        Block* next;
    };

    void DeleteBlocks();
    void PushBlock();

    T& operator[](int index);

    void* _vptr;
    int blockSize;
    int size;
    Block* blockPool;
    Block* backBlock;
    int blockCount;
    Memory::GlobalHeapEnum heap;
    eMemMgrTag memTag;
};

template <class T>
T& BlockAllocatorArray<T>::operator[](int index) {
    Block* b = blockPool;
    int block = index / blockSize;
    int within = index - block * blockSize;

    while (block--) {
        b = b->next;
    }

    return b->data[within];
}

}  // namespace Util

class zNeoDrivenLink {
public:
    zNeoDrivenLink();

    static void Init();
    static void Clear();
    static void Reset();
    static void ProcessMoves(float dt);
    static zNeoDrivenLink* OldestSibling(zNeoDrivenLink* link);
    static void MOVE(zNeoDrivenLink* link, float dt);
    static int findPassenger(World::xOGEntity* entity);

    static Util::BlockAllocatorArray<zNeoDrivenLink> neoDriveList;

    xMat4x3 relativeMat;
    xMat4x3 passengerOriginalMat;
    xMat4x3 lastMat;
    World::xOGEntity* driver;
    World::xOGEntity* passenger;
    zNeoDrivenLink* quickLinks[4];
    int totalQuickLinks;
    unsigned int flags;
    unsigned int bone;
    float yaw;
    zNeoDrivenLink* parentLink;
    zNeoDrivenLink* olderSiblingLink;
    zNeoDrivenLink* siblingLink;
    zNeoDrivenLink* childLink;
};

zNeoDrivenLink::zNeoDrivenLink() {
    totalQuickLinks = 0;

    relativeMat = g_I3;
    passengerOriginalMat = g_I3;
    lastMat = g_I3;
}

void zNeoDrivenLink::Init() {
    if (neoDriveList.blockSize == 0) {
        neoDriveList.DeleteBlocks();

        neoDriveList.blockSize = 128;

        neoDriveList.PushBlock();
    }
}

void zNeoDrivenLink::Reset() {
    Clear();
    Init();
}

zNeoDrivenLink* zNeoDrivenLink::OldestSibling(zNeoDrivenLink* link) {
    if (link == 0) {
        return link;
    }

    while (link->olderSiblingLink != 0) {
        link = link->olderSiblingLink;
    }

    return link;
}

void zNeoDrivenLink::ProcessMoves(float dt) {
    int i;

    for (i = 0; i < neoDriveList.size; i++) {
        if (neoDriveList[i].flags & 8) {
            MOVE(OldestSibling(&neoDriveList[i]), dt);
        }
    }
}

int zNeoDrivenLink::findPassenger(World::xOGEntity* entity) {
    int i;

    for (i = 0; i < neoDriveList.size; i++) {
        if (entity == neoDriveList[i].passenger) {
            return i;
        }
    }

    return -1;
}
