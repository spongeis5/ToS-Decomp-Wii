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
// THE NAMES HERE ARE WRONG, AND THE PLACEMENT IS RIGHT. Retail's three
// symbols are `collBSPCount__19@unnamed@WAD00_cpp@` and the same for the
// other two: an ANONYMOUS NAMESPACE, carrying the unity blob's basename.
// These are plain globals. main.dol is byte-identical anyway, because
// nothing outside this unit reaches them and the addresses come from the
// split rather than from name matching -- but this is not what the
// original said, and two things follow from that:
//
//   * objdiff cannot pair the symbols, so this unit reports complete_data
//     100% and NO matched_data, where Math/Collide.cpp -- whose six names
//     are exactly retail's -- reports both;
//   * these three now have EXTERNAL linkage where retail's are internal,
//     so a later unit defining `collBSP` collides with this one.
//
// tools/dwarf_data_carve.py refuses this unit for exactly that reason, and
// it was written after this file, which is how the mistake was found. The
// faithful version needs the anonymous-namespace trick -- the source file
// named after the blob at a different path, as Util/Sort/WAD02.cpp does --
// and that renames the unit away from Env.cpp.

int collBSPCount;
double collBSP[16];
double rigidBodies[16];

void EnvClearTriMeshList() {
    collBSPCount = 0;
}
