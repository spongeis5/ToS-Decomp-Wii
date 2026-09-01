// C:/branches/SB09/main/GM/Engine/Core/Wii/Env.cpp
//
// One function and 264 bytes of .bss. The SECOND unit in this project to
// carry its own data, and the cheapest possible case of it: the range
// 0x8072DE70..0x8072DF78 is owned by no unit in splits.txt, so nothing has
// to be cut and no link order changes.
//
// The function is four instructions and one source line (line 39 by the
// line table, with line 40 the closing brace):
//
//   lis  r3, 0x8073
//   li   r0, 0
//   stw  r0, -0x2190(r3)      0x80730000 - 0x2190 = 0x8072DE70
//   blr
//
// The three variables, from tools/dwarf_data_decl.py and the retail symbol
// table:
//
//   8072DE70    4  collBSPCount
//   8072DE78  128  collBSP           <- 8 bytes after the first, not 4
//   8072DEF8  128  rigidBodies
//
// THE FOUR-BYTE HOLE IS THE POINT. collBSP starts at DE78, not DE74, so it
// is 8-ALIGNED and the compiler puts the hole there itself. Declaring it
// `double[16]` reproduces that -- 128 bytes, align 8 -- where `float[32]`
// would pack it against collBSPCount and move everything after. The
// element type is a guess; the SIZE and the ALIGNMENT are the recovered
// facts, and they are what decides the layout.
//
// collBSP and rigidBodies are referenced by nothing in the whole image, so
// the linker dead-strips them unless config.yml's `force_active` says
// otherwise -- and the object is correct while that happens, which makes
// the failure look like a layout mistake. See Math/Collide.cpp, where that
// cost the most time.
//
// WHY THIS FILE IS CALLED WAD00.cpp. Retail's three symbols are
// `collBSPCount__19@unnamed@WAD00_cpp@` and the same for the other two: an
// ANONYMOUS NAMESPACE, and CodeWarrior mangles one with the TRANSLATION
// UNIT's basename. A file called Env.cpp cannot produce that name whatever
// it contains, so the file is named after the blob at a path that says
// what it really is -- the same trick Util/Sort/WAD02.cpp uses for text,
// used here for data.
//
// The first version of this unit declared the three as plain globals. It
// linked, and main.dol was byte-identical, because nothing outside the
// unit reaches them and the addresses come from the split rather than from
// name matching. It was still wrong twice over: objdiff could not pair the
// symbols, so the unit reported complete_data 100% with NO matched_data,
// and the three had EXTERNAL linkage where retail's are internal, so a
// later unit defining `collBSP` would have collided.
// tools/dwarf_data_carve.py refuses that shape, which is how it was
// found.

// Nothing in the image references collBSP or rigidBodies, and inside
// an anonymous namespace they are LOCAL symbols -- config.yml's
// force_active cannot hold one, and the linker says so out loud:
//   FORCEACTIVE symbol '@unnamed@WAD00_cpp@::rigidBodies' is either
//   not a global symbol or doesn't exist.  Ignored.
// so the compiler has to mark them instead.
#pragma force_active on
namespace {

int collBSPCount;
double collBSP[16];
double rigidBodies[16];

}  // namespace
#pragma force_active off

void EnvClearTriMeshList() {
    collBSPCount = 0;
}
