// C:/branches/SB09/main/GM/Engine/Game/zPathFinderNodeFinder.cpp
//
// Layout from the Wii build's DWARF (tools/dwarf_types.py):
//
//   class zSearchMap     /* 0x18 */ {
//       /* +0x0  */ unsigned long long id;
//       /* +0x8  */ int nodeCount;
//       /* +0xC  */ zSearchMapNode* nodes;
//       /* +0x10 */ xBase* source;
//   };
//   class zSearchMapNode /* 0xC */ { int id; int linkCount; zSearchMapLink* links; };
//   class zPathFinderNodeFinder /* 0x1 */ {};
//
// The finder is an EMPTY class and every entry point is a non-static member
// that never touches `this` -- r3 is overwritten by the first load in all
// three. FindNode's two arms are tail branches, which is only possible
// because `this` is still in r3 to be passed through.
//
// map->source is an xBase*, read as a zWallNet* in one arm and a
// zWallNetGroup* in the other; FindNode picks by its baseType. No
// enumeration in the DWARF contains both 0x62 and 0xB1, so the two constants
// are recovered but their NAMES are not -- they are spelled as the values
// the compare instructions hold.
//
// The search loop RELOADS map->nodes on every iteration. Nothing in the loop
// stores, so that is not aliasing; it is what `-O4,s` does with `map->nodes[i]`
// written inside the body, and hoisting it into a local before the loop does
// not match.

class xVec3 {
public:
    float x;
    float y;
    float z;
};

class xBase {
public:
    unsigned char _head[0x20];
    unsigned int baseType;
};

class zSearchMapLink;

class zSearchMapNode {
public:
    int id;
    int linkCount;
    zSearchMapLink* links;
};

class zSearchMap {
public:
    unsigned long long id;
    int nodeCount;
    zSearchMapNode* nodes;
    xBase* source;
};

class zWallNet {
public:
    int FindTriangleIDXZ(const xVec3& pos) const;
};

class zWallNetGroup {
public:
    const zWallNet* FindWallNetWithPos(const xVec3& pos) const;
    int GetIndexOfWallNet(const zWallNet* net) const;
};

class zPathFinderNodeFinder {
public:
    zSearchMapNode* FindNavMeshNode(const xVec3* pos, const zSearchMap* map);
    zSearchMapNode* FindNavMeshGroupNode(const xVec3* pos,
                                         const zSearchMap* map);
    zSearchMapNode* FindNode(const xVec3* pos, const zSearchMap* map);
};

zSearchMapNode* zPathFinderNodeFinder::FindNavMeshNode(const xVec3* pos,
                                                       const zSearchMap* map) {
    int triID = ((zWallNet*)map->source)->FindTriangleIDXZ(*pos);
    zSearchMapNode* found = 0;
    int i;

    for (i = 0; i < map->nodeCount; i++) {
        if (triID == map->nodes[i].id) {
            found = &map->nodes[i];
            break;
        }
    }

    return found;
}

zSearchMapNode* zPathFinderNodeFinder::FindNavMeshGroupNode(
    const xVec3* pos, const zSearchMap* map) {
    zWallNetGroup* group = (zWallNetGroup*)map->source;
    int idx = group->GetIndexOfWallNet(group->FindWallNetWithPos(*pos));
    zSearchMapNode* found = 0;
    int i;

    for (i = 0; i < map->nodeCount; i++) {
        if (idx == map->nodes[i].id) {
            found = &map->nodes[i];
            break;
        }
    }

    return found;
}

zSearchMapNode* zPathFinderNodeFinder::FindNode(const xVec3* pos,
                                                const zSearchMap* map) {
    switch (map->source->baseType) {
    case 0x62:
        return FindNavMeshNode(pos, map);
    case 0xB1:
        return FindNavMeshGroupNode(pos, map);
    }

    return 0;
}
