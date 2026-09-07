// WAD00_1.cpp -- the Domains subsystem, 51 functions and 8,632 bytes in
// the image. THIS FILE COVERS 40 OF THEM: 30 byte-identical and ten
// recorded near-misses, eight of them at retail's exact size -- StartLoad
// by one word, AbortActivity by six, push_back by five, SubtreeMin by
// nine, Insert by 269 of 320 at 1,280 against 1,284, and the tree
// iterator's increment by 35 of 44 at 176 against 184. Plus two
// functions the image does not hold under the names we give them.
//
// unitcmp reads that as 18 of 31 and report.json as 26 of 52 (2,948
// bytes), and both are right. Eight of the twenty-six reach a symbol the
// linker FOLDED:
// unitcmp checks the branch-target NAME and calls those a difference,
// report.json resolves the branch by ADDRESS in the linked image, where
// the folded symbol is the same bytes. Quote whichever question you are
// asking, and say which. Read from the image with tools/disasm.py.
// A Domain is a unit of streamed
// content; DomainPriv holds its name, its load stream, a queue of
// Activity objects and the entity handles it registered. The Activity
// subclasses do the work in stages -- ActLoadParcel slices a parcel out
// of the stream, ActRegisterEnts walks the handle tree, ActUnloadAll
// walks the UID array back out.
//
// EIGHT FUNCTIONS HERE REACH A SYMBOL THE IMAGE FOLDED AWAY, over nine
// branches -- PushBlock has two. PoolList's constructor is empty, and
// every empty constructor in the image folded onto Math::Matrix33's --
// four bytes, one blr, at 0x800075C0. BlockAllocatorArray<unsigned long
// long>::Block's constructor, the Delete that frees a Block and the
// DeleteArray that frees the TOC array all folded onto the <void*>
// instantiation's Delete; PoolList<Activity*>::SetPool folded onto
// PoolList<zBTTask*>'s; and NewArray<MemHandle*> onto NewArray<float>.
// A fragment cannot reproduce a fold: it can only name its own symbol
// and say so. The eight are DomainPriv's constructor (1 differing word
// of 34), PushBlock (2 of 66), DeleteBlocks (1 of 28), KillUIDArray
// (1 of 35), MakeActivityQue (1 of 20), KillActivityQue (1 of 21),
// MakePackTOCArray (1 of 20) and KillPackTOCArray (1 of 38) -- every
// other word of each agrees, and report.json counts all eight as
// matched because it resolves the branch by address.
//
// Layouts from the DWARF (tools/dwarf_types.py), which covers this file
// completely -- WAD00.cpp is one of the eleven compile units the retail
// link kept debug info for. Domain 0xC, DomainPriv 0xD8, Activity 0x8,
// ActLoadParcel 0x24, ActRegisterEnts 0xF4, ActUnloadAll 0x18,
// MemHandle 0x8, ParcelTOCRef 0x40, ParcelSliceMeta 0x20.
//
// Util::BlockAllocatorArray<T> is a chain of fixed-size blocks: a vtable
// pointer, the element count per block, the live count, the head and
// back Block, the block count, and the heap and tag it allocates from.
// Block is {T* pool; Block* next; Block* prev; int idx;}.

extern "C" char* strcpy(char* dst, const char* src);

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };  // 68 is eMemMgrTag_AssetManager

namespace Memory {

enum GlobalHeapEnum {
    GlobalHeap,
    GlobalHeapMain,
    GlobalHeapSecondary,
    GlobalHeapDefault = 2,
};

void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap, eMemMgrTag tag,
                      bool zeroed);
void FreeGlobalHeap(void* block, GlobalHeapEnum heap);

}  // namespace Memory

inline void* operator new(unsigned long size, Memory::GlobalHeapEnum heap,
                          eMemMgrTag tag) {
    return Memory::AllocGlobalHeap(size, heap, tag, false);
}

// const H& -- the heap can be a constant, and mwcc materialises one as an
// unnamed 4-byte static it then loads through. Retail's Domain::~Domain
// reads @21996, a real STT_OBJECT of that shape in the image's symbol
// table. The load happens where the reference BINDS, which is before the
// body's null test: that ordering is the whole of the difference in both
// this function's callers.
template <class H>
inline void Free(const H& heap, void* p) {
    H h = heap;

    if (p != 0) {
        Memory::FreeGlobalHeap(p, h);
    }
}

// Inlined at its one call site, which is what retail's MakeActivityQue
// has -- the count there is a constant, so count * sizeof(T) folds to a
// single li and the inline is cheaper than the call.
template <class H>
inline void* Alloc(const H& heap, eMemMgrTag tag, unsigned long size) {
    return Memory::AllocGlobalHeap(size, heap, tag, false);
}

// DECLARED here, DEFINED at the foot: retail CALLS this from
// MakePackTOCArray, where the count is a runtime value and the inline
// costs an instruction rather than saving one. With the body up here the
// auto-inliner takes it and MakePackTOCArray is 84 bytes against 80.
template <class T, class H>
T* NewArray(const H& heap, eMemMgrTag tag, unsigned long count);

// Out of line in retail, and its branch folds onto Delete<..Block> --
// the same symbol and the same fold Exception.cpp records. Declared and
// not defined, so this unit emits no copy.
template <class H, class T>
void DeleteArray(const H& heap, T* array, unsigned long count);

// Non-const H& here, which is what retail's mangled name says (R, not
// RC), and DeleteBlocks does pass the array's own heap member. The image
// holds only the BlockAllocatorArray<void*> instantiation of this: the
// <unsigned long long> one is identical code and the linker folded them,
// so a fragment naming <Ux> names a symbol the image does not have.
// reloc_audit reports that as folded, which it is.
// DECLARED here, DEFINED at the foot of the file. Retail calls this from
// both DeleteBlocks and KillUIDArray; with the body up here the
// auto-inliner takes it into DeleteBlocks and costs two instructions.
// The inliner can only take a body it has already read (NOTES, where the
// body sits in the file).
template <class H, class T>
void Delete(H& heap, T* p);

template <class H, class T>
inline void Delete(const H& heap, T* p) {
    if (p != 0) {
        p->~T();
    }

    Free(heap, p);
}

namespace Util {

// Nothing of this class inlines into a caller: not the constructor, not
// a member function, not one with no calls of its own, and not under
// #pragma always_inline placed inside the class or around it. Eleven
// spellings were measured. So DomainPriv's constructor writes the six
// fields out where retail's inlined code has them, and everything the
// image reaches through a call is left as a call.
template <class T>
class BlockAllocatorArray {
public:
    class Block {
    public:
        Block();

        T* pool;
        Block* next;
        Block* prev;
        int idx;
    };

    class Iterator {
    public:
        Iterator() {
            it = 0;
            owner = 0;
        }

        void operator++();

        T* it;
        Block* block;
        const BlockAllocatorArray<T>* owner;
    };

    Iterator CreateIterator(T* p) const;
    Iterator End();

    // Virtual FIRST, so the vtable pointer takes +0 and blockSize +4.
    virtual ~BlockAllocatorArray();

    void DeleteBlocks();
    void PushBlock();

    // No user constructor: the implicit one writes the vtable pointer and
    // nothing else, which is exactly the first instruction retail's
    // DomainPriv emits for this member. Everything else is here, in an
    // ordinary member function -- mwcc will not inline a constructor of a
    // class with a vtable at any spelling, and inlines this one.

    int blockSize;
    int size;
    Block* blockPool;
    Block* backBlock;
    int blockCount;
    Memory::GlobalHeapEnum heap;
    eMemMgrTag memTag;
};

template <class T>
BlockAllocatorArray<T>::~BlockAllocatorArray() {
    DeleteBlocks();
}

template <class T>
typename BlockAllocatorArray<T>::Iterator
BlockAllocatorArray<T>::CreateIterator(T* p) const {
    Iterator ci;

    ci.it = p;
    ci.owner = this;
    ci.block = blockPool;

    while (ci.block != 0) {
        // The flag LAST: retail gives it the lowest scratch register and
        // the block and its pool the two above, which is declaration
        // order. Declared first or in the middle it takes the highest
        // and every use of all three shifts -- 7 and 9 words against 0.
        T* pool = ci.block->pool;
        int n = blockSize;
        bool inBlock = false;

        if (p >= pool && p <= pool + n) {
            inBlock = true;
        }

        if (inBlock) {
            return ci;
        }

        ci.block = ci.block->next;
    }

    return ci;
}

template <class T>
typename BlockAllocatorArray<T>::Iterator BlockAllocatorArray<T>::End() {
    return CreateIterator(backBlock->pool + ((size - 1) % blockSize + 1));
}

template <class T>
void BlockAllocatorArray<T>::DeleteBlocks() {
    while (blockPool != 0) {
        Block* block = blockPool;

        blockPool = block->next;

        Free(heap, block->pool);
        Delete(heap, block);
    }

    blockPool = 0;
    backBlock = 0;
    blockCount = 0;
}

template <class T>
void BlockAllocatorArray<T>::PushBlock() {
    if (blockPool != 0) {
        backBlock->next = new (heap, memTag) Block;
        backBlock->next->prev = backBlock;
        backBlock = backBlock->next;
        backBlock->pool =
            (T*)Memory::AllocGlobalHeap(blockSize * sizeof(T), heap, memTag,
                                        false);
        backBlock->idx = 1;
    } else {
        blockPool = new (heap, memTag) Block;
        blockPool->pool =
            (T*)Memory::AllocGlobalHeap(blockSize * sizeof(T), heap, memTag,
                                        false);
        blockPool->idx = 0;
        backBlock = blockPool;
    }

    blockCount = blockCount + 1;
}

}  // namespace Util

namespace Math {
class Matrix33 {
public:
    Matrix33();
};
}  // namespace Math

namespace Domains {
class DomainPriv;
}  // namespace Domains

// One enStage serves both the domain activities and the load stream's
// connect stage -- the DWARF names the same enum for ActLoadParcel::stage
// and ConnectInfo::stage, so it is not inside either namespace.
enum enStage {
    INACTIVE,
    READY,
    FREEMERGE,
    TETRIS,
    CLEANING,
    FLATLINE,
    SIRTET,
};

namespace HEStructs {

enum enParcelType {
    PARCEL_TYPE_UNDEFINED,
    PARCEL_TYPE_EXCLUSIVE,
    PARCEL_TYPE_SHARED,
    PARCEL_TYPE_FROMDOMAIN,
};

}  // namespace HEStructs

namespace Loader {

enum enStatus { LS_STATUS_ };

enum enWorkState {
    WORK_STATE_UNSPECIFIED,
    WORK_STATE_CONNECT,
    WORK_STATE_TRANSFER,
    WORK_STATE_DISCONNECT,
};

enum enPriority { LS_PRIORITY_ };

class SectionKey {
public:
    void SetValues(int typeID, const char* name, unsigned short langID,
                   HEStructs::enParcelType type, const char* fromDomain,
                   int key);

    int sectionTypeID;
    char* sectionName;
    unsigned short packLangID;
    unsigned char _pad0[0xC - 0xA];
    int userKey;
    HEStructs::enParcelType parcelType;
    char* fromDomainName;
};

class ConnectInfo {
public:
    enStage stage;
    SectionKey sectionKey;
    unsigned char _pad0[0x2C - 0x1C];
    enPriority readPriority;
};

class LSWorker {
public:
    enStatus status;
    enWorkState workState;
    bool isError;
    unsigned char _pad0[0xC - 0x9];
    ConnectInfo connectInfo;
    unsigned char _pad1[0x50 - 0x3C];
};

class LoadStream {
public:
    unsigned char mediaFile[0x68];
    LSWorker worker;
    unsigned char _pad0[0x208 - 0xB8];
};

class LSPoolModule {
public:
    LoadStream* GrabLoadStream();
    void FreeLoadStream(LoadStream* stream);
};

extern LSPoolModule lsPoolMod;

}  // namespace Loader

namespace Domains {

enum enStatus {
    INITING,
    INITED,
    STARTING,
    LOADING,
    LOADED,
    ACTIVATING,
    ACTIVE,
    UNLOADING,
    OBSOLETE,
};

class Domain;

}  // namespace Domains

namespace Util {

// 36 bytes an entry, `order` at +28 -- the layout RTTID.cpp recovered.
struct RTTIDData {
    unsigned char _pad0[0x14];
    RTTIDData* child;
    RTTIDData* next;
    unsigned int order;
    void* create;
};

extern RTTIDData g_rttidDataTable[];

}  // namespace Util

namespace World {

// The tree node lives at +44 and +48, inside the padding: Insert writes
// the new node's left at 44 and its right/colour/balance word at 48.
class EntityHandleBase {
public:
    unsigned long long id;
    unsigned char _pad0[0x34 - 0x8];
    unsigned int typeID;
    unsigned char _pad1[0x48 - 0x38];
};

}  // namespace World

// The right pointer carries the node's balance in its low two bits, so
// every read masks with ~3 and every write goes through SetRight. Same
// class as EntityManager's, where this tree matches at offsets 28 and 36;
// here it is the same template at offset 44 with another comparator.
class EmbeddedTreeNode {
public:
    EmbeddedTreeNode& operator=(const EmbeddedTreeNode& other);
    void SetRight(void* right);

    void* Right() const { return (void*)(right_color_bal & ~3); }
    int Bal() const { return (right_color_bal & 3) - 1; }
    void SetBal(int b) {
        right_color_bal = (right_color_bal & ~3) | (b + 1);
    }

    void* left;
    long right_color_bal;
};

EmbeddedTreeNode& EmbeddedTreeNode::operator=(const EmbeddedTreeNode& other) {
    left = other.left;
    right_color_bal = other.right_color_bal;
    return *this;
}

// The iterator keeps a 56-entry path array, not EntityManager's 28-entry
// stack: 0xE8, which is what makes ActRegisterEnts 244 bytes.
template <class T, class Cmp, int OFFSET>
class EmbeddedTreeAVL : public Cmp {
public:
    class Iterator {
    public:
        class FixedKeyArray {
        public:
            void push_back(T* item);

            int count;
            T* data[56];
        };

        Iterator() {
            itpath.count = 0;
            owner = 0;
        }

        EmbeddedTreeNode* NodeBack() const;
        void SubtreeMin();
        void operator++();

        EmbeddedTreeAVL<T, Cmp, OFFSET>* owner;
        FixedKeyArray itpath;
    };

    T* Insert(T* node, T* item, int& change);

    unsigned int count;
    T* root;
};

// 5 of 7 words, size exact, and all of it one register: retail computes
// count + 1 before the element address and we compute it after. Three
// statement orders were measured -- the store first, a named local for
// count + 1, and the count store first -- and none moved it. SubtreeMin
// carries the same difference (9 of 24, size exact) because the same
// statements are written out inside its loop.
template <class T, class Cmp, int OFFSET>
void EmbeddedTreeAVL<T, Cmp, OFFSET>::Iterator::FixedKeyArray::push_back(
    T* item) {
    int n = count;

    data[n] = item;
    count = n + 1;
}

template <class T, class Cmp, int OFFSET>
EmbeddedTreeNode* EmbeddedTreeAVL<T, Cmp, OFFSET>::Iterator::NodeBack() const {
    return (EmbeddedTreeNode*)((char*)itpath.data[itpath.count - 1] + OFFSET);
}

template <class T, class Cmp, int OFFSET>
void EmbeddedTreeAVL<T, Cmp, OFFSET>::Iterator::operator++() {
    // 35 OF 44 WORDS, 176 bytes against 184. Retail's third guard
    // compiles to a branch INTO the body followed by a branch past it,
    // where ours is one beq -- two instructions, and the rest of the
    // difference is the register the right pointer takes. Three separate
    // early returns and this one && chain measure the same, so the shape
    // of the guard is not what does it.
    if (owner != 0 && owner->root != 0 && itpath.count != 0) {
        T* right = (T*)NodeBack()->Right();

        if (right != 0) {
            int n = itpath.count;

            itpath.data[n] = right;
            itpath.count = n + 1;
            SubtreeMin();
            return;
        }

        for (;;) {
            T* child = itpath.data[itpath.count - 1];

            itpath.count = itpath.count - 1;

            if (itpath.count == 0) {
                return;
            }

            if (NodeBack()->left == child) {
                return;
            }
        }
    }
}

// 269 OF 320 WORDS, 1,280 bytes against retail's 1,284 -- one
// instruction short, and every difference downstream of one register:
// retail holds `item` in r29 and we hold it in r28.
//
// Retail's Insert has three things inlined that mwcc will not inline for
// us, so all three are written out at their call sites here: BalanceLeft,
// BalanceRight and the comparator. The image holds no BalanceLeft or
// BalanceRight for this instantiation and no DomainHandleCmp::operator(),
// so retail's compiler took all three; ours takes none. Twenty-three
// spellings were measured across this file and Util::BlockAllocatorArray:
// `inline` on the definition and on the declaration, #pragma
// always_inline inside the class and around it, inline_max_size,
// inline_max_auto_size and inline_depth at the callee and at the top of
// the unit, a member of the Cmp base, a free function template, and four
// spellings of the comparator including two ternary chains. Every one of
// them stood out of line. What DOES inline into a class template's
// member is a tiny free template (Free reaches DeleteBlocks) and a
// one-expression accessor (Bal, SetBal and Right reach here), so the
// rule is not simply about templates.
//
// After writing all three out, the remaining difference is register
// allocation, and neither the comparison directions (four combinations)
// nor the placement of the balance locals (hoisted to the branch or
// declared in the case) moves it.
template <class T, class Cmp, int OFFSET>
T* EmbeddedTreeAVL<T, Cmp, OFFSET>::Insert(T* node, T* item, int& change) {
    if (node == 0) {
        EmbeddedTreeNode* in = (EmbeddedTreeNode*)((char*)item + OFFSET);

        change = 1;
        count = count + 1;
        in->left = 0;
        in->right_color_bal = 1;
        return item;
    }

    int c;

    if (Util::g_rttidDataTable[node->typeID].order <
        Util::g_rttidDataTable[item->typeID].order) {
        c = -1;
    } else if (Util::g_rttidDataTable[node->typeID].order >
               Util::g_rttidDataTable[item->typeID].order) {
        c = 1;
    } else if (node->id < item->id) {
        c = -1;
    } else {
        c = item->id < node->id;
    }

    if (c < 0) {
        EmbeddedTreeNode* n = (EmbeddedTreeNode*)((char*)node + OFFSET);

        n->left = Insert((T*)n->left, item, change);

        if (change != 0) {
            switch (n->Bal()) {
            case 1:
                n->SetBal(0);

                if (change > 0) {
                    change = 0;
                }

                break;

            case 0:
                n->SetBal(-1);

                if (change < 0) {
                    change = 0;
                }

                break;

            case -1: {
                T* mid;
                EmbeddedTreeNode* mn;
                T* left = (T*)n->left;
                EmbeddedTreeNode* ln =
                    (EmbeddedTreeNode*)((char*)left + OFFSET);
                long raw = ln->right_color_bal;
                int b = (raw & 3) - 1;

                if (b < 0) {
                    n->left = (void*)(raw & ~3);
                    ln->SetRight(node);
                    n->SetBal(0);
                    node = left;
                    ln->SetBal(0);
                } else if (b == 0) {
                    n->left = (void*)(raw & ~3);
                    ln->SetRight(node);
                    n->SetBal(-1);
                    node = left;
                    ln->SetBal(1);

                    if (change < 0) {
                        change = 0;
                    }
                } else {
                    mid = (T*)(raw & ~3);
                    mn = (EmbeddedTreeNode*)((char*)mid + OFFSET);

                    ln->SetRight(mn->left);
                    mn->left = left;
                    n->left = mn->Right();
                    mn->SetRight(node);

                    if ((mn->right_color_bal & 3) == 0) {
                        n->SetBal(1);
                    } else {
                        n->SetBal(0);
                    }

                    if (mn->Bal() == 1) {
                        ln->SetBal(-1);
                    } else {
                        ln->SetBal(0);
                    }

                    node = mid;
                    mn->SetBal(0);
                }

                if (change > 0) {
                    change = 0;
                }

                break;
            }
            }
        }
    } else if (c > 0) {
        T* right;
        T* mid;
        EmbeddedTreeNode* mn;
        EmbeddedTreeNode* rn;
        EmbeddedTreeNode* n = (EmbeddedTreeNode*)((char*)node + OFFSET);

        n->SetRight(Insert((T*)n->Right(), item, change));

        if (change != 0) {
            int bal = n->Bal();

            switch (bal) {
            case -1:
                n->SetBal(0);

                if (change > 0) {
                    change = 0;
                }

                break;

            case 0:
                n->SetBal(1);

                if (change < 0) {
                    change = 0;
                }

                break;

            case 1: {
                right = (T*)n->Right();
                rn = (EmbeddedTreeNode*)((char*)right + OFFSET);
                int b = rn->Bal();

                if (b > 0) {
                    n->SetRight(rn->left);
                    rn->left = node;
                    n->SetBal(0);
                    node = right;
                    rn->SetBal(0);
                } else if (b == 0) {
                    n->SetRight(rn->left);
                    rn->left = node;
                    n->SetBal(1);
                    node = right;
                    rn->SetBal(-1);

                    if (change < 0) {
                        change = 0;
                    }
                } else {
                    mid = (T*)rn->left;
                    mn = (EmbeddedTreeNode*)((char*)mid + OFFSET);

                    rn->left = mn->Right();
                    mn->SetRight(right);
                    n->SetRight(mn->left);
                    mn->left = node;

                    if (mn->Bal() == 1) {
                        n->SetBal(-1);
                    } else {
                        n->SetBal(0);
                    }

                    if ((mn->right_color_bal & 3) == 0) {
                        rn->SetBal(1);
                    } else {
                        rn->SetBal(0);
                    }

                    node = mid;
                    mn->SetBal(0);
                }

                if (change > 0) {
                    change = 0;
                }

                break;
            }
            }
        }
    } else {
        *(EmbeddedTreeNode*)((char*)item + OFFSET) =
            *(EmbeddedTreeNode*)((char*)node + OFFSET);
        change = 0;
        node = item;
    }

    return node;
}
template <class T, class Cmp, int OFFSET>
void EmbeddedTreeAVL<T, Cmp, OFFSET>::Iterator::SubtreeMin() {
    T* left = (T*)NodeBack()->left;

    while (left != 0) {
        // push_back written out: retail inlines it here and nothing
        // inlines a member into a class template's member.
        int n = itpath.count;

        itpath.data[n] = left;
        itpath.count = n + 1;

        left = (T*)NodeBack()->left;
    }
}

// The queue's constructor is folded onto Math::Matrix33's in the image:
// identical code, one symbol kept. PoolList is {int size; NodeHeader
// tail; NodeHeader* freeList; NodeHeader* poolHead; int poolSize; int
// peakSize;} -- 0x1C bytes.
namespace Util {

class PoolListBase {
public:
    class NodeHeader {
    public:
        NodeHeader* prev;
        NodeHeader* next;
    };

    PoolListBase();

    void PushBack();
    void PopFront();
    void Clear();
    void Erase(NodeHeader* node);

    int size;
    NodeHeader tail;
    NodeHeader* freeList;
    NodeHeader* poolHead;
    int poolSize;
    int peakSize;
};

// SetPool's branch resolves to the PoolList<zBTTask*> instantiation --
// identical code, folded. Another symbol a fragment cannot reproduce.
template <class T>
class PoolList : public PoolListBase {
public:
    class NodeType : public NodeHeader {
    public:
        T data;
    };

    void SetPool(NodeType* pool, int count);
};

}  // namespace Util

namespace Domains {

// Inlined into Insert by retail, which is what a plain class's member
// does here -- unlike anything belonging to a class template. Handles
// order by their TYPE's sort order first and by uid within a type.
class DomainHandleCmp {
public:
    int operator()(const ::World::EntityHandleBase* item,
                   const ::World::EntityHandleBase* node) const {
        unsigned int a = Util::g_rttidDataTable[node->typeID].order;
        unsigned int b = Util::g_rttidDataTable[item->typeID].order;

        if (a < b) {
            return -1;
        }

        if (a > b) {
            return 1;
        }

        if (node->id < item->id) {
            return -1;
        }

        return item->id < node->id;
    }
};

enum enActType {
    eActType_Checkpoint,
    eActType_LoadParcel,
    eActType_LoadTextureParcel,
    eActType_3,
    eActType_LoadMemFastParcel,
    eActType_5,
    eActType_6,
    eActType_RegisterEnts,
    eActType_UnloadAll,
};

class DomainPriv;

class Activity {
public:
    bool justBorn;

    // Declared BELOW the data member, so the vtable pointer takes +4 and
    // justBorn +0 -- CreateActivity stores the vtable at 4(r3).
    virtual void Init(DomainPriv* dom);
    virtual void Kill();
    virtual void Prepare(DomainPriv* dom);
    virtual void Finish(DomainPriv* dom);
    virtual void Execute(DomainPriv* dom);
    virtual void Abort(DomainPriv* dom);
    virtual bool IsDone();
};

// Only two of the monitor's vtable slots are reached from this file,
// +0x10 and +0x14; the two below them are declared to place those. Each
// is called with the domain and two zeroes -- r4 still holds the domain
// at the call and mwcc would have loaded it otherwise.
class ProgressCB {
public:
    virtual void Reserved0();
    virtual void Reserved1();
    virtual void OnLoaded(DomainPriv* dom, int a, int b);
    virtual void OnActive(DomainPriv* dom, int a, int b);
};

class ActCheckpoint : public Activity {
public:
    void Execute(DomainPriv* dom);

    int checkPoint;
};

class AllocEntry {
public:
    unsigned char _pad0[0xC];
    void* block;
};

class MemHandle {
public:
    unsigned char lockFlags;
    unsigned char memCBidx;
    unsigned short mustAlign;
    AllocEntry* entry;
};

class DynaMem {
public:
    static void Release(MemHandle* handle);
};

class ActLoadParcel : public Activity {
public:
    void Prepare(DomainPriv* dom);
    void Finish(DomainPriv* dom);
    void MakePackTOCArray(int n);
    void KillPackTOCArray();
    void InsertPackTOC(MemHandle* handle);
    void* GetNthPackTOC(int n);

    enStage stage;
    MemHandle* memDomData;
    unsigned short parcelLangID;
    int parcelUserKey;
    MemHandle** tocHandles;
    int tocArraySize;
    int tocCount;
};

class ActLoadTextureParcel : public ActLoadParcel {
public:
    void Prepare(DomainPriv* dom);
};

class ActLoadMemFastParcel : public ActLoadParcel {
public:
    void Prepare(DomainPriv* dom);
};

typedef ::EmbeddedTreeAVL< ::World::EntityHandleBase, DomainHandleCmp, 44>
    HandleTree;

// 0xF4: Activity 8, the stage, and the tree iterator's 0xE8 -- 56 path
// entries and a count.
class ActRegisterEnts : public Activity {
public:
    void Init(DomainPriv* dom);

    enStage stage;
    HandleTree::Iterator it_handle;
};

class ActUnloadAll : public Activity {
public:
    enStage stage;
    Util::BlockAllocatorArray<unsigned long long>::Iterator it_buid;
};


class DomainPriv {
public:
    DomainPriv();
    ~DomainPriv();

    void Init(const char* name, unsigned int mask, Domain* parent);
    void StartLoad(unsigned short langID);
    void StartUnload();
    void HandOffToWorld();
    Activity* CreateActivity(enActType type);
    void DestroyActivity(Activity* act);
    void QueueActivity(Activity* act);
    void AbortActivity(Activity* act);
    void MakeActivityQue();
    void KillActivityQue();
    void ProcActivityQue();
    void KillUIDArray();
    void AddUIDItem(::World::EntityHandleBase* handle);

    Domain* parentDom;
    char domainName[128];
    Loader::LoadStream* loadStream;
    Util::PoolList<Activity*> activityQueue;
    Util::BlockAllocatorArray<unsigned long long> myUIDsArray;
    ::EmbeddedTreeAVL< ::World::EntityHandleBase, DomainHandleCmp, 44>
        myHandleTree;
    int UidProcessIdx;
    unsigned int refMask;
    unsigned short currLangID;
};

class Domain {
public:
    Domain();
    ~Domain();

    int GetAssetCount();
    unsigned long long GetAssetIDFromList(int idx);

    DomainPriv* domainPriv;
    enStatus domStatus;
    ProgressCB* progressMon;
};

Domain::Domain() {
    domainPriv = new (Memory::GlobalHeap, (eMemMgrTag)68) DomainPriv;
    progressMon = 0;
    domStatus = INITING;
}

Domain::~Domain() {
    Delete(Memory::GlobalHeap, domainPriv);
    domainPriv = 0;
}

int Domain::GetAssetCount() {
    return domainPriv->myUIDsArray.size;
}

unsigned long long Domain::GetAssetIDFromList(int idx) {
    Util::BlockAllocatorArray<unsigned long long>::Block* block =
        domainPriv->myUIDsArray.blockPool;
    int n = idx / domainPriv->myUIDsArray.blockSize;
    int i = idx % domainPriv->myUIDsArray.blockSize;

    while (n != 0) {
        block = block->next;
        n--;
    }

    return block->pool[i];
}

DomainPriv::DomainPriv() {
    myUIDsArray.size = 0;
    myUIDsArray.blockPool = 0;
    myUIDsArray.backBlock = 0;
    myUIDsArray.blockCount = 0;
    myUIDsArray.heap = Memory::GlobalHeap;
    myUIDsArray.memTag = (eMemMgrTag)68;
    myUIDsArray.DeleteBlocks();
    myUIDsArray.blockSize = 256;
    myUIDsArray.PushBlock();
    myHandleTree.count = 0;
    myHandleTree.root = 0;
    domainName[0] = 0;
    loadStream = 0;
}

DomainPriv::~DomainPriv() {}

void DomainPriv::KillUIDArray() {
    Util::BlockAllocatorArray<unsigned long long>::Block* b =
        myUIDsArray.blockPool->next;

    while (b != 0) {
        Util::BlockAllocatorArray<unsigned long long>::Block* cur = b;

        b = b->next;
        Free(myUIDsArray.heap, cur->pool);
        Delete(myUIDsArray.heap, cur);
    }

    myUIDsArray.blockPool->next = 0;
    myUIDsArray.backBlock = myUIDsArray.blockPool;
    myUIDsArray.blockCount = 1;
    myUIDsArray.size = 0;
    myHandleTree.count = 0;
    myHandleTree.root = 0;
}

void DomainPriv::AddUIDItem(::World::EntityHandleBase* handle) {
    unsigned long long id = handle->id;

    if (myUIDsArray.size == myUIDsArray.blockCount * myUIDsArray.blockSize) {
        myUIDsArray.PushBlock();
    }

    myUIDsArray.size = myUIDsArray.size + 1;
    myUIDsArray.backBlock
        ->pool[(myUIDsArray.size - 1) % myUIDsArray.blockSize] = id;

    int change = 0;

    myHandleTree.root = myHandleTree.Insert(myHandleTree.root, handle, change);
}

void DomainPriv::Init(const char* name, unsigned int mask, Domain* parent) {
    strcpy(domainName, name);
    parentDom = parent;
    currLangID = 0;
    loadStream = 0;
    MakeActivityQue();
    refMask = mask;
    parentDom->domStatus = INITED;
}

void DomainPriv::StartLoad(unsigned short langID) {
    // ONE WORD SHORT of retail, and it is this store: retail writes a
    // WORD of zero over langs[0] and langs[1] and then puts langs[1] and
    // langs[2] on top of it; we write a halfword. Seven spellings were
    // measured -- a zero initialiser, a redundant langs[1] = 0, a fourth
    // element, the two assignments swapped, and a word store through a
    // cast. The two that do produce the word store (the initialiser and
    // the cast) cost eight bytes elsewhere and read 42 of 70; the rest
    // are eliminated back to a halfword. 1 of 70 words, size exact.
    unsigned short langs[3];
    int i;

    langs[0] = 0;
    langs[1] = langID & 0x3FF;
    langs[2] = langID;
    currLangID = langID;

    for (i = 0; i < 3; i++) {
        int j;
        ActLoadParcel* act;

        if (i > 0 && langs[i] == langs[i - 1]) {
            break;
        }

        act = (ActLoadParcel*)CreateActivity(eActType_LoadMemFastParcel);
        act->parcelLangID = langs[i];
        act->parcelUserKey = 0;
        QueueActivity(act);

        act = (ActLoadParcel*)CreateActivity(eActType_LoadParcel);
        act->parcelLangID = langs[i];
        act->parcelUserKey = 0;
        QueueActivity(act);

        for (j = 0; j < 2; j++) {
            act = (ActLoadParcel*)CreateActivity(eActType_LoadTextureParcel);
            act->parcelLangID = langs[i];
            act->parcelUserKey = j;
            QueueActivity(act);
        }
    }

    {
        ActCheckpoint* cp = (ActCheckpoint*)CreateActivity(eActType_Checkpoint);

        cp->checkPoint = 0;
        QueueActivity(cp);
    }

    parentDom->domStatus = LOADING;
}

void DomainPriv::StartUnload() {
    ActCheckpoint* cp;

    AbortActivity(0);
    QueueActivity(CreateActivity(eActType_UnloadAll));

    cp = (ActCheckpoint*)CreateActivity(eActType_Checkpoint);
    cp->checkPoint = 2;
    QueueActivity(cp);

    parentDom->domStatus = UNLOADING;
}

void DomainPriv::HandOffToWorld() {
    ActCheckpoint* cp;

    QueueActivity(CreateActivity(eActType_RegisterEnts));

    cp = (ActCheckpoint*)CreateActivity(eActType_Checkpoint);
    cp->checkPoint = 1;
    QueueActivity(cp);

    parentDom->domStatus = ACTIVATING;
}

Activity* DomainPriv::CreateActivity(enActType type) {
    Activity* act = 0;

    switch (type) {
    case eActType_Checkpoint:
        act = new (Memory::GlobalHeap, (eMemMgrTag)68) ActCheckpoint;
        break;
    case eActType_LoadParcel:
        act = new (Memory::GlobalHeap, (eMemMgrTag)68) ActLoadParcel;
        break;
    case eActType_LoadTextureParcel:
        act = new (Memory::GlobalHeap, (eMemMgrTag)68) ActLoadTextureParcel;
        break;
    case eActType_LoadMemFastParcel:
        act = new (Memory::GlobalHeap, (eMemMgrTag)68) ActLoadMemFastParcel;
        break;
    case eActType_RegisterEnts:
        act = new (Memory::GlobalHeap, (eMemMgrTag)68) ActRegisterEnts;
        break;
    case eActType_UnloadAll:
        act = new (Memory::GlobalHeap, (eMemMgrTag)68) ActUnloadAll;
        break;
    }

    if (act != 0) {
        act->justBorn = true;
        act->Init(this);
    }

    return act;
}

// SIX WORDS SHORT, size exact, and all six are one register: retail
// keeps the second branch's node in r31 -- reusing the register the
// first branch gave to &tail -- and we keep it in r30, reusing the one
// the first branch gave to its node. Eight spellings were measured, four
// on each branch: an explicit end local before or after the node, a for
// loop, and the address written inline. Only the order n-then-end moved
// the count, and it doubled it to 12.
void DomainPriv::AbortActivity(Activity* act) {
    typedef Util::PoolList<Activity*>::NodeType Node;

    if (act == 0) {
        Util::PoolListBase::NodeHeader* n = activityQueue.tail.next;

        while (n != &activityQueue.tail) {
            ((Node*)n)->data->Abort(this);
            DestroyActivity(((Node*)n)->data);
            n = n->next;
        }

        activityQueue.Clear();
    } else {
        Util::PoolListBase::NodeHeader* n = activityQueue.tail.next;

        while (n != &activityQueue.tail) {
            if (((Node*)n)->data == act) {
                ((Node*)n)->data->Abort(this);
                DestroyActivity(((Node*)n)->data);
                activityQueue.Erase(n);
                break;
            }

            n = n->next;
        }
    }
}

void DomainPriv::ProcActivityQue() {
    typedef Util::PoolList<Activity*>::NodeType Node;
    bool done;

    do {
        Activity* act;

        if (activityQueue.size == 0) {
            break;
        }

        act = ((Node*)activityQueue.tail.next)->data;

        if (act->justBorn) {
            act->Prepare(this);
            act->justBorn = false;
        }

        act->Execute(this);
        done = act->IsDone();

        if (done) {
            act->Finish(this);
            activityQueue.PopFront();
            DestroyActivity(act);
        }
    } while (done);
}

void DomainPriv::DestroyActivity(Activity* act) {
    act->Kill();
    Free(Memory::GlobalHeap, act);
}

void DomainPriv::QueueActivity(Activity* act) {
    activityQueue.PushBack();
    ((Util::PoolList<Activity*>::NodeType*)activityQueue.tail.prev)->data = act;
}

void DomainPriv::MakeActivityQue() {
    activityQueue.SetPool(
        (Util::PoolList<Activity*>::NodeType*)Alloc(
            Memory::GlobalHeap, (eMemMgrTag)68,
            64 * sizeof(Util::PoolList<Activity*>::NodeType)),
        64);
}

void DomainPriv::KillActivityQue() {
    Free(Memory::GlobalHeap, activityQueue.poolHead);
    activityQueue.SetPool(0, 0);
}

// 8 OF 49 WORDS, size exact, and the same two differences in each of the
// two notifying cases: retail keeps the domain in a temporary of its own
// where we reuse the result register, and it materialises the two zero
// arguments in the opposite order. A named local for the domain does not
// move the first, and five parameter typings -- int/int, void*/int,
// int/void*, bool/int, int/bool -- do not move the second.
void ActCheckpoint::Execute(DomainPriv* dom) {
    switch (checkPoint) {
    case 0: {
        Domain* owner = dom->parentDom;
        ProgressCB* mon = owner->progressMon;

        if (mon != 0) {
            mon->OnLoaded(dom, 0, 0);
        }

        dom->parentDom->domStatus = LOADED;
        break;
    }
    case 1: {
        Domain* owner = dom->parentDom;
        ProgressCB* mon = owner->progressMon;

        if (mon != 0) {
            mon->OnActive(dom, 0, 0);
        }

        dom->parentDom->domStatus = ACTIVE;
        break;
    }
    case 2:
        dom->parentDom->domStatus = OBSOLETE;
        break;
    }
}

void ActRegisterEnts::Init(DomainPriv* dom) {
    it_handle.owner = &dom->myHandleTree;
    it_handle.itpath.count = 0;

    if (dom->myHandleTree.root != 0) {
        it_handle.itpath.push_back(dom->myHandleTree.root);
        it_handle.SubtreeMin();
    }

    stage = READY;
}

// THE THREE PREPARE FUNCTIONS all come out at retail's exact size with
// exactly one difference, the same in each: retail holds the shared zero
// in r31 and the load stream in r30, and we hold them the other way --
// 7 differing words of 32 and of 34. Four orderings were measured: the
// two zero stores swapped, the read priority set after the stage and
// work state rather than before, and the stream grabbed before the zero
// stores instead of after. The last costs nine more words; the others
// change nothing.
void ActLoadParcel::Prepare(DomainPriv* dom) {
    stage = INACTIVE;
    memDomData = 0;

    Loader::LoadStream* ls = Loader::lsPoolMod.GrabLoadStream();

    dom->loadStream = ls;
    ls->worker.connectInfo.sectionKey.SetValues(
        'P   ', dom->domainName, parcelLangID,
        HEStructs::PARCEL_TYPE_EXCLUSIVE, 0, 0);
    ls->worker.connectInfo.readPriority = (Loader::enPriority)0;
    ls->worker.connectInfo.stage = READY;
    ls->worker.workState = Loader::WORK_STATE_CONNECT;
}

void ActLoadTextureParcel::Prepare(DomainPriv* dom) {
    stage = INACTIVE;
    memDomData = 0;

    Loader::LoadStream* ls = Loader::lsPoolMod.GrabLoadStream();

    dom->loadStream = ls;
    ls->worker.connectInfo.sectionKey.SetValues(
        'PTEX', dom->domainName, parcelLangID,
        HEStructs::PARCEL_TYPE_EXCLUSIVE, 0, parcelUserKey);
    ls->worker.connectInfo.readPriority = (Loader::enPriority)0;
    ls->worker.connectInfo.stage = READY;
    ls->worker.workState = Loader::WORK_STATE_CONNECT;
    dom->UidProcessIdx = dom->myUIDsArray.size;
}

void ActLoadMemFastParcel::Prepare(DomainPriv* dom) {
    stage = INACTIVE;
    memDomData = 0;

    Loader::LoadStream* ls = Loader::lsPoolMod.GrabLoadStream();

    dom->loadStream = ls;
    ls->worker.connectInfo.sectionKey.SetValues(
        'PFST', dom->domainName, parcelLangID,
        HEStructs::PARCEL_TYPE_EXCLUSIVE, 0, parcelUserKey);
    ls->worker.connectInfo.readPriority = (Loader::enPriority)0;
    ls->worker.connectInfo.stage = READY;
    ls->worker.workState = Loader::WORK_STATE_CONNECT;
    dom->UidProcessIdx = dom->myUIDsArray.size;
}

void ActLoadParcel::Finish(DomainPriv* dom) {
    Loader::lsPoolMod.FreeLoadStream(dom->loadStream);
    dom->loadStream = 0;
}

void ActLoadParcel::MakePackTOCArray(int n) {
    tocHandles = NewArray<MemHandle*>(Memory::GlobalHeap, (eMemMgrTag)68, n);
    tocCount = 0;
    tocArraySize = n;
}

void ActLoadParcel::KillPackTOCArray() {
    if (tocHandles != 0) {
        int i;

        for (i = 0; i < tocCount; i++) {
            MemHandle* handle = tocHandles[i];

            tocHandles[i] = 0;
            handle->lockFlags = handle->lockFlags & ~1;
            DynaMem::Release(handle);
        }

        DeleteArray(Memory::GlobalHeap, tocHandles, tocArraySize);
        tocHandles = 0;
        tocArraySize = 0;
        tocCount = 0;
    }
}

void ActLoadParcel::InsertPackTOC(MemHandle* handle) {
    tocHandles[tocCount] = handle;
    tocCount = tocCount + 1;
}

void* ActLoadParcel::GetNthPackTOC(int n) {
    return tocHandles[n]->entry->block;
}

}  // namespace Domains

template <class H, class T>
void Delete(H& heap, T* p) {
    if (p != 0) {
        p->~T();
    }

    Free(heap, p);
}

template <class T, class H>
T* NewArray(const H& heap, eMemMgrTag tag, unsigned long count) {
    return (T*)Memory::AllocGlobalHeap(count * sizeof(T), heap, tag, false);
}

// Nothing in this file calls the iterator's increment yet -- retail's
// Execute for ActRegisterEnts and ActUnloadAll do, and neither is
// written. Instantiated explicitly so the body is emitted and measured.
template void ::EmbeddedTreeAVL< ::World::EntityHandleBase,
                                 Domains::DomainHandleCmp,
                                 44>::Iterator::operator++();

// Nothing here calls these yet either -- Init for ActUnloadAll does, and
// it is not written.
template Util::BlockAllocatorArray<unsigned long long>::Iterator
Util::BlockAllocatorArray<unsigned long long>::CreateIterator(
    unsigned long long* p) const;
template Util::BlockAllocatorArray<unsigned long long>::Iterator
Util::BlockAllocatorArray<unsigned long long>::End();
