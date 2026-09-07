// zBTBuilder.cpp -- twelve functions, 3,808 bytes, all twelve matched.
// Read from the image with tools/disasm.py. The builder turns an asset
// into a zBT: Build sizes a scratch table of NodeRecord (one per node
// of any kind), ParseAsset fills it -- seven passes, one per node kind,
// each hashing the node's id into the table through FindEmptyIndex --
// then walks the edges to find the root and to give every record its
// parent and child count, BuildNodes allocates a zBTNode subclass per
// record and wires parents and children, and Build copies the node
// pointers into the tree and frees the table.
//
// Layouts from the DWARF (tools/dwarf_types.py): NodeRecord 0x20 --
// the asset, the node, the kind, the id, the parent id, the child order
// and count, and a parsed flag. zBTNode 0x10, parent/childCount/
// children then the vtable pointer at +0xC. zBT 0x18. The node asset
// structs all start with BTNodeBase {id, order}: sequence, parallel and
// reference 12 bytes, action 16, selector 20 with its kind byte at +16,
// condition 24, decorator 28 with its kind byte at +24, an edge 12.
// BehaviorTree 0x48: the uid, then {count, data} pairs in the order
// sequence, selector, parallel, decorator, ACTION, CONDITION, reference,
// edges -- ParseAsset walks condition before action.
//
// The three statics are the builder's own: currentTreeID (a uid),
// nodeCount and nodeRecords. The four unnamed 4-byte .data objects the
// image holds for this file (@127660..@127711) are constants bound to
// the `const H&` parameter of NewArray/DeleteArray: mwcc materialises a
// reference-bound enum constant as an unnamed static, one per call.

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {
enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };
enum ThreadStackEnum { ThreadStack = 0 };
}  // namespace Memory

// Not in the DWARF: only ever named as the type of a reference-bound
// constant. An enum mangles the same as a class would.
enum xMemStaticType { xMemStatic = 0 };

void* xMemAlloc(Memory::GlobalHeapEnum heap, unsigned int size, int align,
                eMemMgrTag tag);
extern "C" void* memset(void* dst, int c, unsigned long n);

void* operator new(unsigned long size, Memory::ThreadStackEnum stack,
                   eMemMgrTag tag);
void operator delete[](void* block, Memory::ThreadStackEnum stack,
                       unsigned long size);

inline void* operator new(unsigned long size, const xMemStaticType& heap,
                          eMemMgrTag tag) {
    return xMemAlloc((Memory::GlobalHeapEnum)0, size, 0, tag);
}

inline void* operator new(unsigned long, void* p) { return p; }

// The zBTNode*/xMemStaticType instantiation is a real function in the
// image (weak, 20 bytes, called twice); the NodeRecord/ThreadStackEnum
// one and DeleteArray are inlined into Build. So the generic template
// stays a plain template and the thread-stack forms are inlined.
//
// DECLARED here and DEFINED at the foot of the file, after Build. -O4's
// auto-inliner can only take a body it has already read, so a template
// defined below its caller is called rather than inlined -- and that is
// the whole of Build's remaining 84 differing words: the call keeps the
// ADDRESS of the reference-bound xMemStatic constant live (@811, the
// third .data reference in the function), and three references are what
// makes mwcc hoist a .data base register instead of spending a hi/lo
// relocation pair on each. Two statements in the body does not do it,
// and neither does #pragma dont_inline at the definition; only the
// position does. See NOTES, what the auto-inliner takes.
template <class T, class H>
T* NewArray(const H& heap, eMemMgrTag tag, unsigned long count);

// Named apart from NewArray because mwcc 1.1 cannot order the two
// templates and calls the pair ambiguous; inlined, the name is never
// emitted.
#pragma always_inline on
template <class T>
inline T* NewStackArray(const Memory::ThreadStackEnum& heap, eMemMgrTag tag,
                        unsigned long count) {
    return (T*)operator new(count * sizeof(T), heap, tag);
}

template <class T>
inline void DeleteStackArray(const Memory::ThreadStackEnum& heap, T* array,
                             unsigned long count) {
    operator delete[](array, heap, count * sizeof(T));
}
#pragma always_inline off

namespace Sext {

class BTNodeBase {
public:
    int id;
    int order;
};

class SequenceNode : public BTNodeBase {
public:
    unsigned char _pad0[0xC - 0x8];
};

class SelectorNode : public BTNodeBase {
public:
    unsigned char _pad0[0x10 - 0x8];
    unsigned char selectorType;
    unsigned char _pad1[0x14 - 0x11];
};

class ParallelNode : public BTNodeBase {
public:
    unsigned char SuccessPolicy;
    unsigned char FailPolicy;
    unsigned char _pad0[0xC - 0xA];
};

class DecoratorNode : public BTNodeBase {
public:
    unsigned char _pad0[0x18 - 0x8];
    unsigned char decoratorType;
    unsigned char _pad1[0x1C - 0x19];
};

class ConditionNode : public BTNodeBase {
public:
    unsigned char _pad0[0x18 - 0x8];
};

class ActionNode : public BTNodeBase {
public:
    unsigned char _pad0[0x10 - 0x8];
};

class ReferenceNode : public BTNodeBase {
public:
    unsigned char _pad0[0xC - 0x8];
};

class Edge {
public:
    int id;
    int from;
    int to;
};

template <class T>
class NodeList {
public:
    unsigned int count;
    T* data;
};

class BehaviorTree {
public:
    unsigned long long id;
    NodeList<SequenceNode> sequenceNodes;
    NodeList<SelectorNode> selectorNodes;
    NodeList<ParallelNode> parallelNodes;
    NodeList<DecoratorNode> decoratorNodes;
    NodeList<ActionNode> actionNodes;
    NodeList<ConditionNode> conditionNodes;
    NodeList<ReferenceNode> referenceNodes;
    NodeList<Edge> edges;
};

}  // namespace Sext

// Three members, then the virtuals, so the vtable pointer lands at
// +0xC; the one virtual this unit calls is slot 0. Nothing here defines
// a virtual, so no vtable is emitted for any of these.
class zBTNode {
public:
    zBTNode* parent;
    int childCount;
    zBTNode** children;

    virtual void Init(Sext::BTNodeBase* asset);
};

class zBTNodeSequence : public zBTNode {
public:
    unsigned char _pad0[0x14 - 0x10];
};

class zBTNodePrioritySelector : public zBTNode {};

class zBTNodeProbabilitySelector : public zBTNode {
public:
    unsigned char _pad0[0x14 - 0x10];
};

class zBTNodeRandomSelector : public zBTNode {};

class zBTNodeParallel : public zBTNode {
public:
    unsigned char _pad0[0x14 - 0x10];
};

class zBTNodeLoopDecorator : public zBTNode {
public:
    unsigned char _pad0[0x1C - 0x10];
};

class zBTNodeTimerDecorator : public zBTNode {
public:
    unsigned char _pad0[0x18 - 0x10];
};

class zBTNodeForceStatusDecorator : public zBTNode {
public:
    unsigned char _pad0[0x14 - 0x10];
};

class zBTNodeRandomChanceDecorator : public zBTNode {
public:
    unsigned char _pad0[0x1C - 0x10];
};

class zBTNodeTimeDelayDecorator : public zBTNode {
public:
    unsigned char _pad0[0x20 - 0x10];
};

class zBTNodeEventHandlerDecorator : public zBTNode {
public:
    unsigned char _pad0[0x1C - 0x10];
};

class zBTNodeTryAndCatchDecorator : public zBTNode {
public:
    unsigned char _pad0[0x14 - 0x10];
};

class zBTNodeCondition : public zBTNode {
public:
    unsigned char _pad0[0x18 - 0x10];
};

class zBTNodeAction : public zBTNode {
public:
    unsigned char _pad0[0x14 - 0x10];
};

// The one node with an out-of-line constructor.
class zBTNodeReference : public zBTNode {
public:
    zBTNodeReference();

    unsigned char _pad0[0x14 - 0x10];
};

class zBT {
public:
    unsigned long long id;
    zBTNode* root;
    zBTNode** nodes;
    int nodeCount;
};

enum eNodeType {
    eNodeType_Sequence = 0,
    eNodeType_Selector = 1,
    eNodeType_Parallel = 2,
    eNodeType_Decorator = 3,
    eNodeType_Condition = 4,
    eNodeType_Action = 5,
    eNodeType_Reference = 6
};

class zBTBuilder {
public:
    class NodeRecord {
    public:
        Sext::BTNodeBase* nodeAsset;
        zBTNode* node;
        eNodeType nodeType;
        int id;
        int parentId;
        int childOrder;
        int childCount;
        bool parsed;
    };

    static zBT* Build(Sext::BehaviorTree* asset);
    static int ParseAsset(Sext::BehaviorTree* asset);
    static void BuildNodes();
    static void BuildSequenceNode(NodeRecord* rec);
    static void BuildSelectorNode(NodeRecord* rec);
    static void BuildParallelNode(NodeRecord* rec);
    static void BuildDecoratorNode(NodeRecord* rec);
    static void BuildConditionNode(NodeRecord* rec);
    static void BuildActionNode(NodeRecord* rec);
    static int FindIndex(int id);
    static int FindEmptyIndex(int id);

    static unsigned long long currentTreeID;
    static int nodeCount;
    static NodeRecord* nodeRecords;
};

// Build addresses all three statics from ONE base register -- the
// unity unit's .bss start, 0x8072EC80 -- with the statics' section
// offsets baked into the loads as plain displacements (9584, 9592,
// 9596). Those words carry no relocation, so a fragment compiled alone
// needs the same 9,584 bytes of .bss ahead of currentTreeID: this array
// is that distance, referenced by nothing and holding nothing. The
// other four functions reach the statics through hi/lo pairs, which
// are relocations and need nothing.
static unsigned char kUnityBssAhead[9584];

// And Build reaches its three reference-bound temporaries (@127702,
// @127704, @127711) from ONE base too -- the unity unit's .data start,
// 0x806B4E98, with displacements 168, 172 and 176 baked in. The
// temporaries are emitted in call order after everything the unit puts
// in .data, so this holds the 164 bytes that precede the first of them
// (BuildNodes' @127660 at 0xA4). Initialised so it lands in .data.
static unsigned char kUnityDataAhead[164] = {1};

unsigned long long zBTBuilder::currentTreeID;
int zBTBuilder::nodeCount;
zBTBuilder::NodeRecord* zBTBuilder::nodeRecords;

int zBTBuilder::FindIndex(int id) {
    int start = (unsigned int)id % nodeCount;
    int i = start;
    int found = 0;

    do {
        if (nodeRecords[i].id == id) {
            found = 1;
            break;
        }

        i++;

        if (i >= nodeCount) {
            i = 0;
        }
    } while (i != start);

    if (!found) {
        i = -1;
    }

    return i;
}

int zBTBuilder::FindEmptyIndex(int id) {
    int start = (unsigned int)id % nodeCount;
    int i = start;
    int found = 0;

    do {
        if (nodeRecords[i].id == 0) {
            found = 1;
            break;
        }

        i++;

        if (i >= nodeCount) {
            i = 0;
        }
    } while (i != start);

    if (found) {
        return i;
    }

    return -1;
}

// One pass per node kind: every node of the kind gets a fresh record,
// found by hashing its id, filled in the same eight stores. The table
// pointer is a static that the stores could alias, so it is re-read for
// every store -- which is what the image shows.
#define PARSE_NODES(list, kind)                                            \
    for (unsigned int i = 0; i < asset->list.count; i++) {                 \
        int idx = FindEmptyIndex(asset->list.data[i].id);                  \
                                                                           \
        nodeRecords[idx].nodeAsset = &asset->list.data[i];                 \
        nodeRecords[idx].nodeType = kind;                                  \
        nodeRecords[idx].node = 0;                                         \
        nodeRecords[idx].id = asset->list.data[i].id;                      \
        nodeRecords[idx].parentId = -1;                                    \
        nodeRecords[idx].childOrder = asset->list.data[i].order;           \
        nodeRecords[idx].childCount = 0;                                   \
        nodeRecords[idx].parsed = false;                                   \
    }

int zBTBuilder::ParseAsset(Sext::BehaviorTree* asset) {
    PARSE_NODES(sequenceNodes, eNodeType_Sequence)
    PARSE_NODES(selectorNodes, eNodeType_Selector)
    PARSE_NODES(parallelNodes, eNodeType_Parallel)
    PARSE_NODES(decoratorNodes, eNodeType_Decorator)
    PARSE_NODES(conditionNodes, eNodeType_Condition)
    PARSE_NODES(actionNodes, eNodeType_Action)
    PARSE_NODES(referenceNodes, eNodeType_Reference)

    // The root is whichever node an edge joins to id 0.
    int rootId = -1;
    unsigned int i;

    for (i = 0; i < asset->edges.count; i++) {
        int from = asset->edges.data[i].from;
        int to = asset->edges.data[i].to;

        if (from == 0) {
            rootId = to;
            break;
        }

        if (to == 0) {
            rootId = from;
            break;
        }
    }

    nodeRecords[FindIndex(rootId)].parentId = 0;

    // Then parent every node below one already parented, a pass at a
    // time until a pass changes nothing.
    int done = 0;

    while (done == 0) {
        unsigned int j;

        done = 1;

        for (j = 0; j < nodeCount; j++) {
            unsigned int k;

            if (nodeRecords[j].parentId == -1) {
                continue;
            }

            if (nodeRecords[j].parsed) {
                continue;
            }

            for (k = 0; k < asset->edges.count; k++) {
                int other = -1;
                int from = asset->edges.data[k].from;
                int to = asset->edges.data[k].to;

                if (from == nodeRecords[j].id) {
                    other = to;
                } else if (to == nodeRecords[j].id) {
                    other = from;
                }

                if (other != -1 && other != nodeRecords[j].parentId) {
                    nodeRecords[j].childCount++;
                    nodeRecords[FindIndex(other)].parentId = nodeRecords[j].id;
                }
            }

            nodeRecords[j].parsed = true;
            done = 0;
        }
    }

    return rootId;
}

void zBTBuilder::BuildSequenceNode(NodeRecord* rec) {
    void* mem = xMemAlloc((Memory::GlobalHeapEnum)0, sizeof(zBTNodeSequence), 0,
                          (eMemMgrTag)85);

    rec->node = new (mem) zBTNodeSequence;
}

void zBTBuilder::BuildSelectorNode(NodeRecord* rec) {
    void* mem;

    switch (((Sext::SelectorNode*)rec->nodeAsset)->selectorType) {
    case 0:
        mem = xMemAlloc((Memory::GlobalHeapEnum)0, sizeof(zBTNodePrioritySelector), 0,
                              (eMemMgrTag)85);

        rec->node = new (mem) zBTNodePrioritySelector;
        break;
    case 1:
        mem = xMemAlloc((Memory::GlobalHeapEnum)0, sizeof(zBTNodeProbabilitySelector), 0,
                              (eMemMgrTag)85);

        rec->node = new (mem) zBTNodeProbabilitySelector;
        break;
    case 2:
        mem = xMemAlloc((Memory::GlobalHeapEnum)0, sizeof(zBTNodeRandomSelector), 0,
                              (eMemMgrTag)85);

        rec->node = new (mem) zBTNodeRandomSelector;
        break;
    }
}

void zBTBuilder::BuildParallelNode(NodeRecord* rec) {
    void* mem = xMemAlloc((Memory::GlobalHeapEnum)0, sizeof(zBTNodeParallel), 0,
                          (eMemMgrTag)85);

    rec->node = new (mem) zBTNodeParallel;
}

// The cases are in the image's compare order: 0, 2, 1, 3, 4, 5, 6.
void zBTBuilder::BuildDecoratorNode(NodeRecord* rec) {
    void* mem;

    switch (((Sext::DecoratorNode*)rec->nodeAsset)->decoratorType) {
    case 0:
        mem = xMemAlloc((Memory::GlobalHeapEnum)0, sizeof(zBTNodeLoopDecorator), 0,
                              (eMemMgrTag)85);

        rec->node = new (mem) zBTNodeLoopDecorator;
        break;
    case 2:
        mem = xMemAlloc((Memory::GlobalHeapEnum)0, sizeof(zBTNodeTimerDecorator), 0,
                              (eMemMgrTag)85);

        rec->node = new (mem) zBTNodeTimerDecorator;
        break;
    case 1:
        mem = xMemAlloc((Memory::GlobalHeapEnum)0, sizeof(zBTNodeForceStatusDecorator), 0,
                              (eMemMgrTag)85);

        rec->node = new (mem) zBTNodeForceStatusDecorator;
        break;
    case 3:
        mem = xMemAlloc((Memory::GlobalHeapEnum)0, sizeof(zBTNodeRandomChanceDecorator), 0,
                              (eMemMgrTag)85);

        rec->node = new (mem) zBTNodeRandomChanceDecorator;
        break;
    case 4:
        mem = xMemAlloc((Memory::GlobalHeapEnum)0, sizeof(zBTNodeTimeDelayDecorator), 0,
                              (eMemMgrTag)85);

        rec->node = new (mem) zBTNodeTimeDelayDecorator;
        break;
    case 5:
        mem = xMemAlloc((Memory::GlobalHeapEnum)0, sizeof(zBTNodeEventHandlerDecorator), 0,
                              (eMemMgrTag)85);

        rec->node = new (mem) zBTNodeEventHandlerDecorator;
        break;
    case 6:
        mem = xMemAlloc((Memory::GlobalHeapEnum)0, sizeof(zBTNodeTryAndCatchDecorator), 0,
                              (eMemMgrTag)85);

        rec->node = new (mem) zBTNodeTryAndCatchDecorator;
        break;
    }
}

void zBTBuilder::BuildConditionNode(NodeRecord* rec) {
    void* mem = xMemAlloc((Memory::GlobalHeapEnum)0, sizeof(zBTNodeCondition), 0,
                          (eMemMgrTag)85);

    rec->node = new (mem) zBTNodeCondition;
}

void zBTBuilder::BuildActionNode(NodeRecord* rec) {
    void* mem = xMemAlloc((Memory::GlobalHeapEnum)0, sizeof(zBTNodeAction), 0,
                          (eMemMgrTag)85);

    rec->node = new (mem) zBTNodeAction;
}

void zBTBuilder::BuildNodes() {
    for (unsigned int i = 0; i < nodeCount; i++) {
        NodeRecord* rec = &nodeRecords[i];

        switch (rec->nodeType) {
        case eNodeType_Sequence:
            BuildSequenceNode(rec);
            break;
        case eNodeType_Selector:
            BuildSelectorNode(rec);
            break;
        case eNodeType_Parallel:
            BuildParallelNode(rec);
            break;
        case eNodeType_Decorator:
            BuildDecoratorNode(rec);
            break;
        case eNodeType_Condition:
            BuildConditionNode(rec);
            break;
        case eNodeType_Action:
            BuildActionNode(rec);
            break;
        case eNodeType_Reference:
            void* mem = xMemAlloc((Memory::GlobalHeapEnum)0, sizeof(zBTNodeReference), 0,
                                  (eMemMgrTag)85);

            rec->node = new (mem) zBTNodeReference;
            break;
        }

        nodeRecords[i].node->childCount = nodeRecords[i].childCount;

        if (nodeRecords[i].node->childCount > 0) {
            nodeRecords[i].node->children = NewArray<zBTNode*>(
                xMemStatic, (eMemMgrTag)85, nodeRecords[i].childCount);
        } else {
            nodeRecords[i].node->children = 0;
        }

        nodeRecords[i].node->Init(nodeRecords[i].nodeAsset);
    }

    for (unsigned int j = 0; j < nodeCount; j++) {
        if (nodeRecords[j].parentId == 0) {
            nodeRecords[j].node->parent = 0;
        } else {
            int p = FindIndex(nodeRecords[j].parentId);

            nodeRecords[j].node->parent = nodeRecords[p].node;
            nodeRecords[p].node->children[nodeRecords[j].childOrder] =
                nodeRecords[j].node;
        }
    }
}

zBT* zBTBuilder::Build(Sext::BehaviorTree* asset) {
    currentTreeID = asset->id;

    zBT* tree = (zBT*)xMemAlloc((Memory::GlobalHeapEnum)0, sizeof(zBT), 0,
                                (eMemMgrTag)85);

    tree->id = currentTreeID;

    // Condition before action, as in ParseAsset's walk. The seven terms
    // fold into a balanced tree from the right, so the LAST three decide
    // which two of the loads pair first: retail adds reference to action
    // and condition to that, which is this order and not the struct's.
    nodeCount = asset->sequenceNodes.count + asset->selectorNodes.count +
                asset->parallelNodes.count + asset->decoratorNodes.count +
                asset->conditionNodes.count + asset->actionNodes.count +
                asset->referenceNodes.count;

    nodeRecords = NewStackArray<NodeRecord>(Memory::ThreadStack,
                                            (eMemMgrTag)85, nodeCount);
    memset(nodeRecords, 0, nodeCount * sizeof(NodeRecord));

    int rootId = ParseAsset(asset);

    BuildNodes();

    tree->root = nodeRecords[FindIndex(rootId)].node;
    tree->nodes = NewArray<zBTNode*>(xMemStatic, (eMemMgrTag)85, nodeCount);

    unsigned int i;

    for (i = 0; i < nodeCount; i++) {
        tree->nodes[i] = nodeRecords[i].node;
    }

    tree->nodeCount = nodeCount;

    DeleteStackArray(Memory::ThreadStack, nodeRecords, nodeCount);

    return tree;
}

// Defined here, below its only two callers, so the auto-inliner has not
// seen it when it compiles them. Moving these four lines back up to the
// declaration costs Build 82 words and 8 bytes.
template <class T, class H>
T* NewArray(const H& heap, eMemMgrTag tag, unsigned long count) {
    return (T*)operator new(count * sizeof(T), heap, tag);
}
