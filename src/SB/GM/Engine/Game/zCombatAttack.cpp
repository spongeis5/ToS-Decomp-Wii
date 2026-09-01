// zCombatAttack -- the whole unit is its constructor.
//
// Read from the image with tools/disasm.py, not guessed:
//
//   memset(this, 0, 216)      the size is `li r5,216`, so sizeof is 216
//   stw r4(=1),  148(r31)     f94 = 1
//   stw r6(=13), 140(r31)     f8C = 13
//   stb r5(=0),  152(r31)     f98 = 0
//   mtctr 6 ; add r3,r31,r5 ; addi r5,r5,20 ; sth r4,20(r3) ; bdnz
//                             six shorts, twenty bytes apart, the first at
//                             this+20 -- an array of six 20-byte elements
//                             beginning at offset 20, each written at its
//                             own offset 0
//
// The stride and the count are recovered fact. WHAT is in the other
// eighteen bytes of an element, and what occupies the twenty bytes before
// the array, is not: the constructor never touches them because the memset
// already zeroed them.
//
// 0xFFFF rather than -1 for the same reason: retail builds the value with
// `lis r3,1 ; addi r4,r3,-1`, which is how mwcc materialises 65535. A -1
// would have been one `li`.

extern "C" void* memset(void* dst, int c, unsigned long n);

struct zCombatAttackSlot {
    unsigned short id;
    unsigned char _pad[0x12];
};

class zCombatAttack {
public:
    zCombatAttack();

    unsigned char _pad0[0x14];
    zCombatAttackSlot slots[6];
    int f8C;
    int f90;
    int f94;
    unsigned char f98;
    unsigned char _pad1[0x3F];
};

zCombatAttack::zCombatAttack() {
    memset(this, 0, sizeof(zCombatAttack));

    f94 = 1;
    f8C = 13;
    f98 = 0;

    for (int i = 0; i < 6; i++) {
        slots[i].id = 0xFFFF;
    }
}
