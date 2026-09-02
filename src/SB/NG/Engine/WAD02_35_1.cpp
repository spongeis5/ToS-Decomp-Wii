// WAD02_35_1 -- one function, read from the image with tools/disasm.py:
// the AVL insert of EmbeddedTreeAVL<World::EntityHandleBase,
// World::EntityHandleCmp, 36>, the embedded node sitting 36 bytes into
// the handle. An empty root takes the node with a left of null and a
// right word of 1 and grows the count; otherwise the comparator, an
// empty base of the tree (the call passes the tree's own `this`),
// sends the node left or right, the subtree is rebalanced when it
// grew, and an equal key replaces the root's node with a copy and
// stops the growth. The right word carries the balance in its low two
// bits, masked off before descending and written back through
// SetRight. The balance functions are other members of the
// instantiation and are only declared here; the node's operator= and
// SetRight are its own, out of line. Layout from the DWARF
// (EmbeddedTreeNode 0x8, the node at EntityHandleBase+0x24).

class EmbeddedTreeNode {
public:
    EmbeddedTreeNode& operator=(const EmbeddedTreeNode& other);
    void SetRight(void* right);

    void* left;
    long right_color_bal;
};

namespace World {

class EntityHandleBase;

class EntityHandleCmp {
public:
    int operator()(const EntityHandleBase* a, const EntityHandleBase* b) const;
};

}  // namespace World

template <class T, class Cmp, int OFFSET>
class EmbeddedTreeAVL : public Cmp {
public:
    T* Insert(T* root, T* node, int& grew);
    T* BalanceLeft(T* root, int& grew);
    T* BalanceRight(T* root, int& grew);

    static EmbeddedTreeNode* NodeOf(T* item) {
        return (EmbeddedTreeNode*)((char*)item + OFFSET);
    }

    int count;
};

template <class T, class Cmp, int OFFSET>
T* EmbeddedTreeAVL<T, Cmp, OFFSET>::Insert(T* root, T* node, int& grew) {
    if (root == 0) {
        grew = 1;
        count++;
        NodeOf(node)->left = 0;
        NodeOf(node)->right_color_bal = 1;
        return node;
    }

    int cmp = Cmp::operator()(node, root);

    if (cmp < 0) {
        NodeOf(root)->left = Insert((T*)NodeOf(root)->left, node, grew);

        if (grew) {
            root = BalanceRight(root, grew);
        }
    } else if (cmp > 0) {
        EmbeddedTreeNode* n = NodeOf(root);

        n->SetRight(Insert((T*)(n->right_color_bal & ~3), node, grew));

        if (grew) {
            root = BalanceLeft(root, grew);
        }
    } else {
        *NodeOf(node) = *NodeOf(root);
        grew = 0;
        root = node;
    }

    return root;
}

template World::EntityHandleBase*
EmbeddedTreeAVL<World::EntityHandleBase, World::EntityHandleCmp, 36>::Insert(
    World::EntityHandleBase* root, World::EntityHandleBase* node, int& grew);
