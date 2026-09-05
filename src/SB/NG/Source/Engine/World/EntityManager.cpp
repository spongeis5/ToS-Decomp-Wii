// EntityManager.cpp -- twenty-seven functions, 5,348 bytes, read from
// the image with tools/disasm.py. The entity manager owns a pool of
// EntityHandleBase and three AVL trees over them, and most of the unit's
// bytes are those trees: EmbeddedTreeAVL is instantiated TWICE, at node
// offsets 28 and 36, so Delete, BalanceLeft, BalanceRight and
// AuxiliaryDelete each appear in the image twice -- 3,064 bytes between
// them, and every byte of it is one template written once.
//
// Layouts from the DWARF (tools/dwarf_types.py): EntityHandleBase 0x48
// on Blobloid 0x1C, with three EmbeddedTreeNode at +0x1C, +0x24 and
// +0x2C -- 28, 36 and 44, which is exactly the template's third argument
// -- then typeID at +0x34, the entity at +0x38, a reference counter at
// +0x3C and the flags from +0x40. EntityMgrModule 0x9C on System::Module
// with the manager at +0x98. EntityManager 0x1C, a PoolList.
//
// MEASURED: 12 of the 17 functions this object defines are
// byte-identical, 3,724 bytes -- and ten of the twelve are FIVE
// template bodies written once. EmbeddedTreeAVL is instantiated at
// node offsets 28 and 36, so AuxiliaryDelete (140 bytes), BalanceRight
// (508), BalanceLeft (512), Delete (372) and Insert (288) each match in
// both instantiations. The splits give Insert<36> (0x80206B30) to
// WAD02_35_1, where it was already matched from its own source, so this
// unit's own credit is 11 functions and 3,436 bytes.
//
// THE TEMPLATE IS FORCED WITH EXPLICIT INSTANTIATION. mwcc 1.1 takes
// `template class EmbeddedTreeAVL<...>;` at file scope, and without it
// nothing is emitted at all, because a template member that is never
// called is never instantiated and this unit does not yet write the
// callers. The mangled names come out exactly right, offsets and all.
// The template is at GLOBAL scope, not in World -- the mangled name
// carries no namespace on EmbeddedTreeAVL itself. The comparator is an
// empty BASE of the tree, not a member: Insert and Delete call
// EntityHandleCmp::operator() with the tree's own `this` in r3, and
// `count` is read at +0.
//
// THE BALANCE FACTOR IS BIASED. EmbeddedTreeNode packs the right child
// and two balance bits into one word, and the accessor returns
// `(word & 3) - 1` -- the classic -1/0/+1 AVL factor. That is not a
// guess: retail computes `rlwinm` then `addi -1` and compares the
// result against 1, 0 and -1, and spelling the balance as a plain 0/1/2
// gives three direct compares and 120 differing words instead of 28.
// One test inside the double rotation is the exception and reads the
// RAW bits -- retail has `rlwinm.` and a single `bne` there where the
// biased form costs two more instructions, so it is written
// `(mn->right_color_bal & 3) == 0`.
//
// TWO REGISTER LEVERS FINISHED BalanceRight, and both are recorded
// elsewhere in NOTES: declaring the double rotation's two locals at the
// TOP of the case, ahead of the ones the earlier branches use, took it
// from 28 differing words to 6; and reading the child's packed word
// into ONE named local and deriving both the balance and the right
// pointer from it -- rather than calling two accessors -- took the last
// six. Retail reads that word once and keeps it in a scratch register.
//
// BalanceLeft IS NOT A MIRROR OF THE ORDER. Its double rotation tests
// the node first and the right child second, like BalanceRight; the
// mirror swaps WHICH test each gets (`mn->Bal() == 1` for the node,
// the raw bits for the child). That was 54 -> 41 words. The register
// swap -- ours had the node in r31 and the right child in r28 where
// retail has them the other way round -- fell to declaring all four of
// the case's locals (right, mid, mn, rn) at the TOP of the function,
// ahead of `n`: 41 -> 7. Ten orderings were swept and only the ones
// with `n` declared last reached 7. The last seven words are the
// switch head, where retail keeps the node's packed word in r4 and the
// switch value in r0, and a NAMED LOCAL for the switch value is what
// gives that: `int bal = n->Bal(); switch (bal)`. Spelling the switch
// expression inline instead (`(n->right_color_bal & 3) - 1`) drops an
// instruction and lands 120 words off -- which is what the earlier
// measurement that hoisting the packed word 'makes it worse' was
// actually seeing.
//
// Insert AND Delete WANT `n` PER BRANCH, NOT PER FUNCTION. Retail folds
// node+OFFSET into the displacement in the c<0 branch and computes the
// address only where SetRight or operator= needs it as an argument; a
// function-scope `n` is hoisted above the null check and materialised
// in r31 for every use, 36 and 68 words off. Declared inside each
// branch, both matched at once. The three balance routines are the
// opposite, because their switch needs the node immediately.
//
// FindEntityByType AND CountEntityByType ARE NON-STATIC. Both read
// their first argument from r4, not r3 -- an unused `this` sits in r3,
// and the mangled names carry no `S`, so `static` is wrong. Dropping it
// matched both. FindEntityByType also keeps the non-null path as the
// fallthrough (`if (handle != 0) return handle->entity; return 0;`),
// which is retail's beq-to-the-zero, not a bne-skip.
//
// NEAR MISS -- FindHandle, 19 of 30 words at 120 vs retail's 124 B. It
// walks the by-uid tree (offset-28 nodes) inlining the three-way uid
// compare, and reads g_handleSortedTree, a SECOND static member 8 bytes
// ahead of g_handleTypeTrees. Retail keeps `id` in the argument pair
// r3:r4 the whole walk and holds `node` in r5, moving it to r3 at a
// single `mr r3,r5; blr`; ours lets `id` die and walks in r3, returning
// early with `blelr`. A separate found-pointer (`found = node; break;`)
// keeps id live and reaches retail's exact 124 bytes, but then puts
// node in r8 -- 30 of 31 words. The exact spelling that pins node to r5
// with id preserved has not fallen out of ten loop shapes; it is a
// register-allocation target, not a structural one.
//
// STILL UNWRITTEN, and this is where the rest of the unit's bytes are:
// Init (236), Done (240), FindEntityHandleByType (316), InsertHandle,
// SetPool, the two RemoveHandles, ByTypeInsert and the Iterator's
// SubtreeMin and NodeBack -- the last two need a caller before they
// instantiate.
//// Two more shapes the bytes fixed. System::Module's vtable pointer
// lands at +0x14, so the first virtual is declared there and the
// derived module's constructor gets its vtable store implicitly.
// EntityManager::g_handleTypeTrees is a static member POINTER, not an
// array: retail loads it and then indexes.
//
// NEAR MISSES, none swept: Startup 9 of 20 words and one instruction
// short, Shutdown 13 of 21 and one short, EntityHandleCmp::operator()
// 10 of 16 -- the 64-bit three-way compare, where retail's subfc/subfe
// pair runs in the opposite operand order from ours -- FindEntityByType
// 3 of 13, and CountEntityByType 3 of 5.
typedef unsigned long long uid;

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {
enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };

void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap,
                      unsigned long align, eMemMgrTag tag, bool clear);
void FreeGlobalHeap(void* p, GlobalHeapEnum heap);
}  // namespace Memory

class Entity;

namespace System {

// The vtable pointer lands at +0x14, where the first virtual is
// declared -- the same spelling zSoundPhysics.cpp carries for
// zSoundAsset.
class Module {
public:
    Module();

    unsigned char _pad0[0x4];
    int state;
    unsigned char _pad1[0x14 - 0x8];

    virtual void _v0();

    unsigned char _pad2[0x98 - 0x18];
};

}  // namespace System

// 8 bytes: the left child, and the right child with two balance bits
// packed into its low bits -- which is why every read of it is masked
// with rlwinm 0,0,29 and every write goes through SetRight.
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

namespace World {

class EntityHandleBase {
public:
    uid id;
    unsigned char _pad0[0x34 - 0x8];
    unsigned int typeID;
    Entity* entity;
    unsigned char _pad1[0x48 - 0x3C];
};

class EntityHandleCmp {
public:
    int operator()(const EntityHandleBase* a, const EntityHandleBase* b) const;
};

class PoolList {
public:
    unsigned char _pad0[0x1C];
};

class TypeTree;

class EntityManager {
public:
    void Init();
    void Done();

    EntityHandleBase* FindEntityHandleByType(unsigned int type,
                                                    unsigned int index);
    Entity* FindEntityByType(unsigned int type, unsigned int index);
    unsigned int CountEntityByType(unsigned int type);
    EntityHandleBase* FindHandle(uid id);

    static TypeTree g_handleSortedTree;
    static TypeTree* g_handleTypeTrees;

    PoolList handleList;
};

class TypeTree {
public:
    unsigned int count;
    void* root;
};

class EntityMgrModule : public System::Module {
public:
    EntityMgrModule();

    virtual void Startup(int phase);
    virtual void Shutdown(int phase);

    EntityManager* entityMgr;
};

extern EntityMgrModule entityMgrMod;

EntityManager* GetEntityManager();

}  // namespace World

// The AVL tree the handles live in. It is instantiated twice, at the
// node offsets 28 and 36, and four of its members appear in the image
// once per instantiation.
template <class T, class Cmp, int OFFSET>
class EmbeddedTreeAVL : public Cmp {
public:
    class Iterator {
    public:
        EmbeddedTreeNode* NodeBack() const;
        void SubtreeMin();

        void* owner;
        int depth;
        T* stack[28];
    };

    T* Insert(T* node, T* item, int& change);
    T* Delete(T* node, T* item, int& change);
    T* BalanceLeft(T* node, int& change);
    T* BalanceRight(T* node, int& change);
    T* AuxiliaryDelete(T* node, T*& out, int& change);

    unsigned int count;
    T* root;
};

template <class T, class Cmp, int OFFSET>
EmbeddedTreeNode* EmbeddedTreeAVL<T, Cmp, OFFSET>::Iterator::NodeBack() const {
    return (EmbeddedTreeNode*)((char*)stack[depth - 1] + OFFSET);
}

template <class T, class Cmp, int OFFSET>
void EmbeddedTreeAVL<T, Cmp, OFFSET>::Iterator::SubtreeMin() {
    T* left = (T*)NodeBack()->left;

    while (left != 0) {
        stack[depth] = left;
        depth = depth + 1;

        left = (T*)NodeBack()->left;
    }
}

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

    int c = Cmp::operator()(item, node);

    if (c < 0) {
        EmbeddedTreeNode* n = (EmbeddedTreeNode*)((char*)node + OFFSET);

        n->left = Insert((T*)n->left, item, change);

        if (change != 0) {
            node = BalanceRight(node, change);
        }
    } else if (c > 0) {
        EmbeddedTreeNode* n = (EmbeddedTreeNode*)((char*)node + OFFSET);

        n->SetRight(Insert((T*)n->Right(), item, change));

        if (change != 0) {
            node = BalanceLeft(node, change);
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
T* EmbeddedTreeAVL<T, Cmp, OFFSET>::Delete(T* node, T* item, int& change) {
    if (node == 0) {
        return 0;
    }

    int c = Cmp::operator()(item, node);

    if (c < 0) {
        EmbeddedTreeNode* n = (EmbeddedTreeNode*)((char*)node + OFFSET);

        n->left = Delete((T*)n->left, item, change);

        if (change < 0) {
            node = BalanceLeft(node, change);
        }
    } else if (c > 0) {
        EmbeddedTreeNode* n = (EmbeddedTreeNode*)((char*)node + OFFSET);

        n->SetRight(Delete((T*)n->Right(), item, change));

        if (change < 0) {
            node = BalanceRight(node, change);
        }
    } else {
        EmbeddedTreeNode* n = (EmbeddedTreeNode*)((char*)node + OFFSET);
        T* tmp;

        count = count - 1;
        node = (T*)n->Right();

        if (node == 0) {
            node = (T*)n->left;
            change = -1;
        } else if (n->left == 0) {
            change = -1;
        } else {
            n->left = AuxiliaryDelete((T*)n->left, tmp, change);
            *(EmbeddedTreeNode*)((char*)tmp + OFFSET) = *n;
            node = tmp;

            if (change < 0) {
                node = BalanceLeft(node, change);
            }
        }
    }

    return node;
}

template <class T, class Cmp, int OFFSET>
T* EmbeddedTreeAVL<T, Cmp, OFFSET>::BalanceLeft(T* node, int& change) {
    T* right;
    T* mid;
    EmbeddedTreeNode* mn;
    EmbeddedTreeNode* rn;
    EmbeddedTreeNode* n = (EmbeddedTreeNode*)((char*)node + OFFSET);
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

    return node;
}

template <class T, class Cmp, int OFFSET>
T* EmbeddedTreeAVL<T, Cmp, OFFSET>::BalanceRight(T* node, int& change) {
    EmbeddedTreeNode* n = (EmbeddedTreeNode*)((char*)node + OFFSET);

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
        EmbeddedTreeNode* ln = (EmbeddedTreeNode*)((char*)left + OFFSET);
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

    return node;
}

template <class T, class Cmp, int OFFSET>
T* EmbeddedTreeAVL<T, Cmp, OFFSET>::AuxiliaryDelete(T* node, T*& out,
                                                    int& change) {
    EmbeddedTreeNode* n = (EmbeddedTreeNode*)((char*)node + OFFSET);
    T* right = (T*)(n->right_color_bal & ~3);

    if (right != 0) {
        n->SetRight(AuxiliaryDelete(right, out, change));

        if (change < 0) {
            node = BalanceRight(node, change);
        }
    } else {
        out = node;
        node = (T*)n->left;
        change = -1;
    }

    return node;
}


World::EntityHandleBase* World::EntityManager::FindHandle(uid id) {
    EntityHandleBase* node = (EntityHandleBase*)g_handleSortedTree.root;

    while (node != 0) {
        int c;

        if (id < node->id) {
            c = -1;
        } else {
            c = node->id < id;
        }

        if (c < 0) {
            node = (EntityHandleBase*)
                ((EmbeddedTreeNode*)((char*)node + 28))->left;
        } else if (c > 0) {
            EmbeddedTreeNode* n = (EmbeddedTreeNode*)((char*)node + 28);
            node = (EntityHandleBase*)(n->right_color_bal & ~3);
        } else {
            return node;
        }
    }

    return 0;
}

World::EntityManager* World::GetEntityManager() {
    return entityMgrMod.entityMgr;
}

unsigned int World::EntityManager::CountEntityByType(unsigned int type) {
    return g_handleTypeTrees[type].count;
}

Entity* World::EntityManager::FindEntityByType(unsigned int type,
                                               unsigned int index) {
    EntityHandleBase* handle = FindEntityHandleByType(type, index);

    if (handle != 0) {
        return handle->entity;
    }

    return 0;
}

int World::EntityHandleCmp::operator()(const EntityHandleBase* a,
                                       const EntityHandleBase* b) const {
    if (a->id < b->id) {
        return -1;
    }

    return b->id < a->id;
}

World::EntityMgrModule::EntityMgrModule() {
    state = 3;
}

void World::EntityMgrModule::Startup(int phase) {
    if (phase == 0) {
        entityMgr = (EntityManager*)Memory::AllocGlobalHeap(
            sizeof(EntityManager), (Memory::GlobalHeapEnum)0, 16,
            (eMemMgrTag)69, false);

        entityMgr->Init();
    }
}

void World::EntityMgrModule::Shutdown(int phase) {
    if (phase == 0) {
        entityMgr->Done();

        if (entityMgr != 0) {
            Memory::FreeGlobalHeap(entityMgr, (Memory::GlobalHeapEnum)0);
        }

        entityMgr = 0;
    }
}

// The two instantiations the image has: the by-uid tree hangs its node
// at +28 in EntityHandleBase and the by-type tree at +36.
template class EmbeddedTreeAVL<World::EntityHandleBase,
                               World::EntityHandleCmp, 28>;
template class EmbeddedTreeAVL<World::EntityHandleBase,
                               World::EntityHandleCmp, 36>;
