// WAD00_1.cpp -- the Domains subsystem, 51 functions and 8,632 bytes in
// the image. THIS FILE COVERS 21 OF THEM, 2,264 bytes: 18 byte-identical
// and two recorded near-misses at retail's exact size (StartLoad by one
// word, AbortActivity by six), plus one function the image does not hold
// under the name we give it.
//
// unitcmp reads that as 13 of 21 and report.json as 18 of 21, and both
// are right. Five of the eighteen reach a symbol the linker FOLDED:
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
// FIVE FUNCTIONS HERE REACH A SYMBOL THE IMAGE FOLDED AWAY. PoolList's
// constructor is empty, and every empty constructor in the image folded
// onto Math::Matrix33's -- four bytes, one blr, at 0x800075C0.
// BlockAllocatorArray<unsigned long long>::Block's constructor and the
// Delete that frees a Block folded onto the <void*> instantiation's, and
// PoolList<Activity*>::SetPool onto PoolList<zBTTask*>'s. A fragment
// cannot reproduce a fold: it can only name its own symbol and say so.
// The five are DomainPriv's constructor (1 differing word of 34),
// PushBlock (2 of 66), DeleteBlocks (1 of 28), MakeActivityQue (1 of 20)
// and KillActivityQue (1 of 21) -- every other word of each agrees.
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

template <class T, class H>
inline T* NewArray(const H& heap, eMemMgrTag tag, unsigned long count) {
    return (T*)Memory::AllocGlobalHeap(count * sizeof(T), heap, tag, false);
}

// Non-const H& here, which is what retail's mangled name says (R, not
// RC), and DeleteBlocks does pass the array's own heap member. The image
// holds only the BlockAllocatorArray<void*> instantiation of this: the
// <unsigned long long> one is identical code and the linker folded them,
// so a fragment naming <Ux> names a symbol the image does not have.
// reloc_audit reports that as folded, which it is.
template <class H, class T>
void Delete(H& heap, T* p) {
    if (p != 0) {
        p->~T();
    }

    Free(heap, p);
}

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

    class ConstIterator {
    public:
        ConstIterator() {
            it = 0;
            owner = 0;
        }

        T* it;
        Block* block;
        const BlockAllocatorArray<T>* owner;
    };

    class Iterator : public ConstIterator {};

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

enum enStage {
    INACTIVE,
    READY,
    FREEMERGE,
    TETRIS,
    CLEANING,
    FLATLINE,
    SIRTET,
};

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

class ProgressCB;
class LoadStream;
class Domain;

}  // namespace Domains

namespace World {
class EntityHandleBase;
}  // namespace World

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

class DomainHandleCmp {};

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

class ActCheckpoint : public Activity {
public:
    int checkPoint;
};

class MemHandle;

class ActLoadParcel : public Activity {
public:
    enStage stage;
    MemHandle* memDomData;
    unsigned short parcelLangID;
    int parcelUserKey;
    MemHandle** tocHandles;
    int tocArraySize;
    int tocCount;
};

class ActLoadTextureParcel : public ActLoadParcel {};

class ActLoadMemFastParcel : public ActLoadParcel {};

// 56 entries and a count: 0xE4, and the tree iterator that holds one is
// 0xE8, which is what makes ActRegisterEnts 244 bytes.
class FixedKeyArray {
public:
    int count;
    World::EntityHandleBase* data[56];
};

class HandleTree;

class HandleTreeIterator {
public:
    HandleTreeIterator() {
        itpath.count = 0;
        owner = 0;
    }

    HandleTree* owner;
    FixedKeyArray itpath;
};

class ActRegisterEnts : public Activity {
public:
    enStage stage;
    HandleTreeIterator it_handle;
};

class ActUnloadAll : public Activity {
public:
    enStage stage;
    Util::BlockAllocatorArray<unsigned long long>::Iterator it_buid;
};

template <class Cmp>
class CmpHolder : public Cmp {
public:
    unsigned long size;
};

template <class T, class Cmp, int OFFSET>
class EmbeddedTreeAVL {
public:
    CmpHolder<Cmp> cmp;
    T* m_root;
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

    Domain* parentDom;
    char domainName[128];
    LoadStream* loadStream;
    Util::PoolList<Activity*> activityQueue;
    Util::BlockAllocatorArray<unsigned long long> myUIDsArray;
    EmbeddedTreeAVL<World::EntityHandleBase, DomainHandleCmp, 44> myHandleTree;
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
    myHandleTree.cmp.size = 0;
    myHandleTree.m_root = 0;
    domainName[0] = 0;
    loadStream = 0;
}

DomainPriv::~DomainPriv() {}

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
        NewArray<Util::PoolList<Activity*>::NodeType>(
            Memory::GlobalHeap, (eMemMgrTag)68, 64),
        64);
}

void DomainPriv::KillActivityQue() {
    Free(Memory::GlobalHeap, activityQueue.poolHead);
    activityQueue.SetPool(0, 0);
}

}  // namespace Domains
