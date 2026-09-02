// zBTDepot.cpp -- five functions, read from the image with
// tools/disasm.py. The depot holds every behaviour tree of the scene,
// built at scene init by a tree creator: the creator answers the asset
// type it builds from (0xB3), how many such assets the scene has, and
// builds one by index, handing back the asset's id. SceneInit resets
// the depot, registers the file-static creator (once, under a guard,
// and only when no registered creator already builds from that asset
// type), counts the items of every creator into a thread-stack array,
// allocates the item table from the global heap (tag 89), fills it
// creator by creator, and frees the counts. FindTree walks the table
// for an id.
//
// Layouts from the DWARF (tools/dwarf_types.py): zDepot {items,
// itemCount, creators, creatorCount}; ItemEntry {item, sourceID} at 16
// bytes; the creator's vptr then its item. The thread-stack enum comes
// to NewArray and DeleteArray by reference, so each use binds the
// enumerator to a static temporary and loads it back (the anonymous
// data words retail has). The depot and the local static are data of
// this unit's own, so it matches and does not link.
//
// NEAR MISS, SceneInit 130 of 145 words, every word a register number.
// The registration is written out in the function, not as a member:
// with these flags the compiler inlines no user function of four
// stores or more, pragma or not (measured on seven shapes), and retail
// tests the static's address (`addic.`) and re-forms it at every use,
// which a base-typed cast of the address gives and a pointer variable
// (kept in a register) does not. The two exits are gotos to the
// continuation, the shape an inlined early return would leave. The
// counters declared before the second loop in the order j, i, k, with k
// reused as that loop's counter, put j and i where retail has them
// (r25, r24). What stays: the first loop's counter takes r26 where
// retail's shares r31 with the counts pointer, and k and the creator
// pointer of the last loop are swapped (r26 and r23 for r23 and r26).
// Tried and no better: the three counters at the top in either order,
// one counter shared by all loops, the counts pointer and the first
// counter declared at the top, the creator pointer declared with the
// counters before or after them, k declared before the last loop only,
// and the registration as inline members (out of line, 99 of 145).

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {
enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };
enum ThreadStackEnum { ThreadStack = 0 };
}  // namespace Memory

void* xMemAlloc(Memory::GlobalHeapEnum heap, unsigned int size, int align,
                eMemMgrTag tag);
void* xSTFindAssetByType(unsigned int type, int index, unsigned int* size);
int xSTAssetCountByType(unsigned int type);

void* operator new(unsigned long size, Memory::ThreadStackEnum stack,
                   eMemMgrTag tag);
void operator delete[](void* block, Memory::ThreadStackEnum stack,
                       unsigned long size);

// Inlined in retail: no out-of-line instance for int exists in the image.
#pragma always_inline on
template <class T, class H>
inline T* NewArray(const H& heap, eMemMgrTag tag, unsigned long count) {
    return (T*)operator new(count * sizeof(T), heap, tag);
}

template <class H, class T>
inline void DeleteArray(const H& heap, T* array, unsigned long count) {
    operator delete[](array, heap, count * sizeof(T));
}
#pragma always_inline off

namespace Sext {
class BehaviorTree {
public:
    unsigned long long id;
};
}  // namespace Sext

class zBT;

class zBTBuilder {
public:
    static zBT* Build(Sext::BehaviorTree* asset);
};

class zDepotItemCreatorBase {
public:
    virtual void Create(int index, unsigned long long& id) = 0;
    virtual unsigned int GetSourceAssetType() const = 0;
    virtual int GetItemCount() const = 0;
};

template <class T>
class zDepotItemCreator : public zDepotItemCreatorBase {
public:
    T* item;
};

template <class T>
class zDepot {
public:
    class ItemEntry {
    public:
        T* item;
        unsigned long long sourceID;
    };

    ItemEntry* items;
    int itemCount;
    zDepotItemCreator<T>** creators;
    int creatorCount;
};

class zBTDepotItemCreator : public zDepotItemCreator<zBT> {
public:
    virtual void Create(int index, unsigned long long& id);
    virtual unsigned int GetSourceAssetType() const;
    virtual int GetItemCount() const;
};

class zBTDepot {
public:
    static void SceneInit();
    static zBT* FindTree(unsigned long long id);

    static zDepot<zBT> depot;
};

zDepot<zBT> zBTDepot::depot;

void zBTDepotItemCreator::Create(int index, unsigned long long& id) {
    Sext::BehaviorTree* asset =
        (Sext::BehaviorTree*)xSTFindAssetByType(GetSourceAssetType(), index, 0);

    if (asset) {
        item = zBTBuilder::Build(asset);
        id = asset->id;
    } else {
        id = 0;
    }
}

unsigned int zBTDepotItemCreator::GetSourceAssetType() const {
    return 0xB3;
}

int zBTDepotItemCreator::GetItemCount() const {
    return xSTAssetCountByType(GetSourceAssetType());
}

void zBTDepot::SceneInit() {
    static zBTDepotItemCreator treeCreator;

    depot.items = 0;
    depot.itemCount = 0;
    depot.creators = 0;
    depot.creatorCount = 0;

    if (!&treeCreator) {
        goto Registered;
    }

    if (!depot.creators) {
        depot.creators = (zDepotItemCreator<zBT>**)xMemAlloc(
            (Memory::GlobalHeapEnum)0, sizeof(zDepotItemCreator<zBT>*), 0,
            (eMemMgrTag)89);
    }

    for (int c = 0; c < depot.creatorCount; c++) {
        if (depot.creators[c]->GetSourceAssetType() ==
            ((zDepotItemCreatorBase*)&treeCreator)->GetSourceAssetType()) {
            goto Registered;
        }
    }

    depot.creators[depot.creatorCount] = &treeCreator;
    depot.creatorCount++;

Registered:
    int* counts = NewArray<int, Memory::ThreadStackEnum>(
        Memory::ThreadStack, (eMemMgrTag)89, depot.creatorCount);

    int j;
    int i;
    int k;

    depot.itemCount = 0;

    for (k = 0; k < depot.creatorCount; k++) {
        counts[k] = depot.creators[k]->GetItemCount();
        depot.itemCount += counts[k];
    }

    depot.items = (zDepot<zBT>::ItemEntry*)xMemAlloc(
        (Memory::GlobalHeapEnum)0,
        depot.itemCount * sizeof(zDepot<zBT>::ItemEntry), 0, (eMemMgrTag)89);

    k = 0;

    for (i = 0; i < depot.creatorCount; i++) {
        for (j = 0; j < counts[i]; j++) {
            zDepotItemCreator<zBT>* itemCreator = depot.creators[i];

            itemCreator->Create(j, depot.items[k].sourceID);
            depot.items[k].item = itemCreator->item;
            k++;
        }
    }

    DeleteArray(Memory::ThreadStack, counts, depot.creatorCount);
}

zBT* zBTDepot::FindTree(unsigned long long id) {
    for (int i = 0; i < depot.itemCount; i++) {
        if (depot.items[i].sourceID == id) {
            return depot.items[i].item;
        }
    }

    return 0;
}
