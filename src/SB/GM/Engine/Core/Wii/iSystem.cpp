// C:/branches/SB09/main/GM/Engine/Core/Wii/iSystem.cpp
//
//   stwu r1, -0x10(r1) ; mflr r0 ; stw r0, 0x14(r1)
//   bl xMemInit ; bl xMathInit ; bl xMath3Init
//   lwz r0, 0x14(r1) ; mtlr r0 ; addi r1, r1, 0x10 ; blr
//
// The parameter is never read -- r3 is not touched before the first call --
// but it is in the mangled name (iSystemInit__FUi), so it is in the
// signature and simply unused.

void xMemInit();
void xMathInit();
void xMath3Init();

void iSystemInit(unsigned int) {
    xMemInit();
    xMathInit();
    xMath3Init();
}
