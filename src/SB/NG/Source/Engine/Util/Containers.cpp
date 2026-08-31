// C:/branches/SB09/main/NG/Source/Engine/Util/Containers.cpp
//
// Layout from the Wii build's DWARF (tools/dwarf_types.py):
//
//   class PoolListBase /* 0x1C */ {
//       /* +0x0  */ int size;
//       /* +0x4  */ NodeHeader tail;      // prev at +0x4, next at +0x8
//       /* +0xC  */ NodeHeader* freeList;
//       /* +0x10 */ NodeHeader* poolHead;
//       /* +0x14 */ int poolSize;
//       /* +0x18 */ int peakSize;
//   };
//   class NodeListBase      /* 0x8  */ { NodeHeader tail; };
//   class PoolAllocatorBase /* 0x14 */ {
//       int blockSize; int maxBlocks; int size;
//       unsigned char* buffer; NodeType* top;
//   };
//   class NodeHeader /* 0x8 */ { NodeHeader* prev; NodeHeader* next; };
//
// The whole unit is leaf code: not one branch-with-link in 584 bytes.
//
// NodeListBase::Unlink is STATIC. Its first instruction reads 4(r3) and its
// second overwrites r4, so r3 is the NODE, not a `this` with the node in r4
// -- a non-static spelling puts the node in r4 and cannot produce these
// bytes. CodeWarrior mangles static and non-static members alike, so the
// name does not say which; the register use does.
//
// Almost every member here is RE-READ after each store rather than held in
// a register, because a store through a NodeHeader* can alias the list head
// that lives inside `this`. Hoisting `freeList` into a local before the
// sequence is smaller and does not match. The two places a local IS used
// are the ones where the value is read BEFORE the store that would destroy
// it: the saved `freeList->next` in PushBack and Insert, and the saved
// `node->next` that Erase returns.

namespace Util {

class PoolListBase {
public:
    class NodeHeader {
    public:
        NodeHeader* prev;
        NodeHeader* next;
    };

    void Clear();
    void PopFront();
    void PushBack();
    void SetPool(NodeHeader* pool, int count);
    NodeHeader* Erase(NodeHeader* node);
    NodeHeader* Insert(NodeHeader* node);

    int size;
    NodeHeader tail;
    NodeHeader* freeList;
    NodeHeader* poolHead;
    int poolSize;
    int peakSize;
};

class NodeListBase {
public:
    class NodeHeader {
    public:
        NodeHeader* prev;
        NodeHeader* next;
    };

    void PushBack(NodeHeader* node);
    static void Unlink(NodeHeader* node);

    NodeHeader tail;
};

class PoolAllocatorBase {
public:
    void Reset();
    void SetPool(void* buf, int size, int count);

    int blockSize;
    int maxBlocks;
    int size;
    unsigned char* buffer;
    void* top;
};

void PoolListBase::Clear() {
    if (size == 0) {
        return;
    }

    tail.prev->next = freeList;
    freeList = tail.next;
    tail.next = &tail;
    tail.prev = &tail;
    size = 0;
}

void PoolListBase::PopFront() {
    NodeHeader* node = tail.next;
    NodeHeader* next = node->next;

    tail.next = next;
    next->prev = &tail;
    node->next = freeList;
    freeList = node;
    size--;
}

void PoolListBase::PushBack() {
    NodeHeader* next = freeList->next;

    freeList->prev = tail.prev;
    freeList->next = &tail;
    tail.prev->next = freeList;
    tail.prev = freeList;
    freeList = next;

    size++;
    if (peakSize < size) {
        peakSize = size;
    }
}

void PoolListBase::SetPool(NodeHeader* pool, int count) {
    size = 0;
    tail.next = &tail;
    tail.prev = &tail;
    freeList = pool;
    poolHead = pool;
    poolSize = count;
}

PoolListBase::NodeHeader* PoolListBase::Erase(NodeHeader* node) {
    NodeHeader* next = node->next;

    node->next->prev = node->prev;
    node->prev->next = node->next;
    node->next = freeList;
    freeList = node;
    size--;

    return next;
}

PoolListBase::NodeHeader* PoolListBase::Insert(NodeHeader* node) {
    NodeHeader* next = freeList->next;

    freeList->prev = node->prev;
    freeList->next = node;
    node->prev->next = freeList;
    node->prev = freeList;
    freeList = next;

    size++;
    if (peakSize < size) {
        peakSize = size;
    }

    return node->prev;
}

void NodeListBase::PushBack(NodeHeader* node) {
    node->prev = tail.prev;
    node->next = &tail;
    tail.prev->next = node;
    tail.prev = node;
}

void NodeListBase::Unlink(NodeHeader* node) {
    node->next->prev = node->prev;
    node->prev->next = node->next;
}

void PoolAllocatorBase::Reset() {
    unsigned char* p;
    unsigned char* last;

    if (maxBlocks == 0) {
        return;
    }

    p = buffer;
    last = p + blockSize * (maxBlocks - 1);

    while (p != last) {
        unsigned char* next = p + blockSize;

        *(unsigned char**)p = next;
        p = next;
    }

    top = buffer;
    *(unsigned char**)last = 0;
}

void PoolAllocatorBase::SetPool(void* buf, int size, int count) {
    buffer = (unsigned char*)buf;
    blockSize = size;
    maxBlocks = count;
    this->size = 0;

    Reset();
}

}  // namespace Util
