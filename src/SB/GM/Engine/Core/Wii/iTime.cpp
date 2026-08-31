// C:/branches/SB09/main/GM/Engine/Core/Wii/iTime.cpp
//
// The first unit with its own DATA, and the first thing this project tried.
// It was blocked then by the link order -- carving a file out of the MIDDLE
// of WAD00 left the parent with text on both sides of the hole:
//
//   Cyclic dependency encountered while resolving link order:
//   SB/GM/Engine/WAD00.cpp -> SB/GM/Engine/Core/Wii/iTime.cpp
//
// Naming the remainder chunks fixed that, and tools/dwarf_data.py supplied
// the .bss on its own -- one address referenced by this unit and no other:
//
//   8072E17C  bss  sGameTime
//
// which is exactly the range that had to be worked out by hand the first
// time round.
//
//   iTimeGameAdvance   lis r3,0x8073 ; lfs f0,-0x1e84(r3)
//                      fadds f0,f0,f1 ; stfs f0,-0x1e84(r3) ; blr
//   iTimeSetGame       lis r3,0x8073 ; stfs f1,-0x1e84(r3) ; blr
//
// 0x80730000 - 0x1E84 = 0x8072E17C. The DWARF names the static and gives
// its type; the code says one is read-modify-write and the other a store.
//
// SMALL DATA HAD TO BE TURNED OFF for this to come out right. At the
// default, mwcc reached sGameTime through r13 and each function lost its
// `lis` -- 6 words against retail's 11. `-sdata 0` is exact, all 11 words
// including the padding, and it breaks none of the eight units that already
// matched. It also agrees with what the retail code says about itself: of
// 7,054 data references from the recovered units, ZERO use r13 or r2.
//
// STILL NonMatching, and the reason is NOT the code. The object's .text is
// byte-identical (objdiff: 100%), but linking it moves the image, because
// mwcc emits this object with `.text` aligned to 16 and `.bss` to 8 where
// the target object has both at 4:
//
//   TARGET   .text size=44 align=4    .bss size=4 align=4
//   OURS     .text size=44 align=16   .bss size=4 align=8
//
// and that shifts .bss for everything after it -- 10,115 bytes of the DOL
// differ, starting inside .init's BSS table. `align:4` on the split line
// does not help: that describes the TARGET, and the target is already 4.
// This is the first unit here to own data, so it is the first time the
// distinction between "the object matches" and "the image links" has had
// anything to say. Both are true statements and only one of them is a
// finished unit.

static float sGameTime;

void iTimeGameAdvance(float dt) {
    sGameTime += dt;
}

void iTimeSetGame(float t) {
    sGameTime = t;
}
