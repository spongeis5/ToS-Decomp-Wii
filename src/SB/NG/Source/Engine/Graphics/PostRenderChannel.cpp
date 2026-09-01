// C:/branches/SB09/main/NG/Source/Engine/Graphics/PostRenderChannel.cpp
//
// One function and both kinds of data -- 17 bytes of .bss and 16 of .data --
// which makes this the first unit here to carry INITIALISED data.
//
// The function is five instructions and one source line (95 by the line
// table, with 99 the closing brace):
//
//   lis r4, 0x8078
//   lis r3, 0x8078
//   lwz r0, -0x6088(r4)      0x80780000 - 0x6088 = 0x80779F78
//   stw r0, -0x6074(r3)      0x80780000 - 0x6074 = 0x80779F8C
//   blr
//
// and the symbol table says what those two addresses are:
//
//   80779F78  buffer__Q28Graphics7Channel              <- read
//   80779F8C  buffer__Q28Graphics17PostRenderChannel   <- written
//
// so the body is one assignment between two units' variables. Both
// arguments are ignored, and their types come from the mangled name:
// `FP8OSThreadP8OSThread`.
//
// THE .data VALUES ARE READ, NOT GUESSED. 806CE7C0..806CE7D0 holds
// 00000001 00000002 00000003 00000004, so the four are 1 to 4 -- and being
// non-zero is exactly why they are in .data where the other five are in
// .bss. Definition order inside each section is what puts them at their
// addresses; the two sections are independent of each other.
//
// Every variable but the one the function writes is referenced by nothing
// in the image, so they are listed in config.yml's force_active. They are
// GLOBAL symbols, so that works here -- an anonymous-namespace local
// cannot be forced that way and needs `#pragma force_active on` instead,
// as Core/Wii/Env/WAD00.cpp does.

class OSThread;

namespace Graphics {
namespace Channel {
extern int buffer;
}
}

namespace Graphics {
namespace PostRenderChannel {

int uniqueValue0;
int buffer;

namespace Batcher {

int head;
int size;
char commit;

}  // namespace Batcher

int uniqueValue1 = 1;
int uniqueValue2 = 2;
int uniqueValue3 = 3;
int uniqueValue4 = 4;

void Create(OSThread* a0, OSThread* a1) {
    buffer = Channel::buffer;
}

}  // namespace PostRenderChannel
}  // namespace Graphics
