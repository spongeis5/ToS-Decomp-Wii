// C:/branches/SB09/main/GM/Engine/Game/zModuleDebugMetrics.cpp
//
// Layout from the Wii build's DWARF (tools/dwarf_types.py):
//
//   class zMODDebugMetrics  /* 0x1C bytes */
//   {
//       /* +0x0  */ zModule _base0;
//       /* +0x18 */ bool isStartOfNewScene;
//   };
//
//   lbz   r0, 0x18(r3)      isStartOfNewScene
//   cmpwi r0, 0
//   beqlr                   if (!isStartOfNewScene) return;
//   li    r0, 0
//   stb   r0, 0x18(r3)      isStartOfNewScene = false;
//   blr
//
// `zModule` is stood in for by its 0x18 bytes rather than declared. The
// real base has virtual functions, and naming it would make mwcc emit a
// vtable into this object -- the same collision xOGEntity.cpp documents.
// Only the field offset matters here, and 0x18 is what the DWARF says.

namespace ModuleDebugMetrics {

class zMODDebugMetrics {
public:
    void PostTimestep(float dt);

    unsigned char _base0[0x18];
    bool isStartOfNewScene;
};

void zMODDebugMetrics::PostTimestep(float dt) {
    if (isStartOfNewScene) {
        isStartOfNewScene = false;
    }
}

}  // namespace ModuleDebugMetrics
