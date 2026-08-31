// C:/branches/SB09/main/GM/Engine/Game/zSearchPath.cpp
//
// Layout from the Wii build's DWARF (tools/dwarf_types.py):
//
//   class zSearchPath  /* 0x10C bytes */
//   {
//       /* +0x0   */ zSearchMap* map;
//       /* +0x4   */ int tail;
//       /* +0x8   */ Step path[32];        // 8 bytes each
//       /* +0x108 */ bool completePath;
//   };
//
// The two stores land at `this + (tail+1)*8 + 8` and `+ 0xC`, which is
// path[tail] and path[tail].link once tail has been incremented -- the
// index is the NEW tail, not the old one.
//
// `tail` is RELOADED between the two stores (`lwz r0, 4(r3)`), which is
// what says it is written back to the member rather than kept in a local:
// the first store could alias it, so mwcc cannot keep it in a register.

class zSearchMap;
class zSearchMapNode;
class zSearchMapLink;

struct Step {
    const zSearchMapNode* node;
    const zSearchMapLink* link;
};

class zSearchPath {
public:
    bool AddStep(const zSearchMapNode* node, const zSearchMapLink* link);

    zSearchMap* map;
    int tail;
    Step path[32];
    bool completePath;
};

bool zSearchPath::AddStep(const zSearchMapNode* node,
                          const zSearchMapLink* link) {
    if (tail >= 32) {
        return false;
    }

    tail++;
    path[tail].node = node;
    path[tail].link = link;

    return true;
}
