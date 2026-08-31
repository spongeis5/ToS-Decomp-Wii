// C:/branches/SB09/main/GM/Engine/Game/zNPCInfoNode.cpp
//
// zNPCInfoNodePathInfo is NOT in the recovered type table -- no DIE in the
// DWARF describes it, so its layout here comes only from the two offsets
// the function touches: a pointer at +0x10 and a pointer at +0x14. The
// rest is padding, and the member names are chosen, not recovered.
//
// The float argument is never read.
//
// The branch over `li r0, 0` is a TERNARY, not an if: the null test on the
// loaded value happens once, after both arms rejoin. Written as
//     if (node != 0 && node->target != 0) current = node->target;
// the compiler tests twice and the shape is different.

class zNPCInfoNodePath;

class zNPCInfoNodePathNode {
public:
    zNPCInfoNodePath* target;
};

class zNPCInfoNodePathInfo {
public:
    void Update(float dt);

    unsigned char _head[0x10];
    zNPCInfoNodePathNode* node;
    zNPCInfoNodePath* current;
};

void zNPCInfoNodePathInfo::Update(float dt) {
    zNPCInfoNodePath* next = (node != 0) ? node->target : 0;

    if (next == 0) {
        return;
    }

    current = next;
}
