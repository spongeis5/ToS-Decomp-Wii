// C:/branches/SB09/main/GM/Engine/Game/zPerformanceDisplay.cpp
//
// Two static readers over two globals in .bss, attributed by
// tools/dwarf_data.py -- both referenced by this unit and no other:
//
//   80753FF0  bss  gSuperSlow__11PerfDisplay
//   80753FF4  bss  gFrameSkip__11PerfDisplay
//
//   lis r3, 0x8075 ; lwz r3, 0x3ff0(r3) ; blr
//   lis r3, 0x8075 ; lwz r3, 0x3ff4(r3) ; blr
//
// 0x80750000 + 0x3FF0 = 0x80753FF0. The mangled names put both in
// PerfDisplay, and r3 arrives unused -- these take no `this`, so they are
// static members.
//
// The object matches 100%, and it is NonMatching for a SECOND reason,
// different from iTime.cpp's. This one was chosen as the alignment test --
// its .bss starts at 0x80753FF0, which is 16-aligned, where iTime's
// sGameTime is only 4-aligned -- and it never got that far:
//
//   Split 8:0x80753FF0..8:0x80753FF8 overlaps with previous split
//
// The range is INSIDE WAD03's .bss (0x80753F80..0x8075B260), 0x70 bytes in.
// Carving it out leaves WAD03 with .bss on both sides of the hole, which is
// the same ordering problem the .text split hit -- except that naming the
// remainder does not settle it here. The text order already fixes the unit
// order, so a unit's .bss must fall between its neighbours' .bss, and that
// means the parent's data has to be partitioned across ITS CHUNKS in link
// order, not left whole on the first one.
//
// So there are two distinct blockers on the data tier and only one of them
// is alignment:
//
//   iTime.cpp               mwcc emits .bss align 8; a 4-aligned .bss
//                           start cannot be honoured and the image shifts
//   zPerformanceDisplay.cpp the parent's .bss is one range on one chunk,
//                           so an interior unit's data has nowhere to go
//
// Both objects are byte-identical. Neither is a finished unit.

class PerfDisplay {
public:
    static int IsSuperSlowOn();
    static int FrameSkip();

    static int gSuperSlow;
    static int gFrameSkip;
};

int PerfDisplay::gSuperSlow;
int PerfDisplay::gFrameSkip;

int PerfDisplay::IsSuperSlowOn() {
    return gSuperSlow;
}

int PerfDisplay::FrameSkip() {
    return gFrameSkip;
}
