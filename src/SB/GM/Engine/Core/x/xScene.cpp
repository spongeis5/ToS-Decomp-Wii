// xScene -- one function, the scene-wide init.
//
// Read from the image with tools/disasm.py:
//
//   sth  r0(=0), 4(r3)        a TWO-byte store, so the field is a short,
//                             and r3 is still the parameter here
//   memset(&sxAnimTempTranPool,  0, 28)
//   memset(&sxAnimTempStatePool, 0, 28)
//   addi r3,r31,12 ; li r4,50 ; li r5,1 ; li r6,1 ; li r7,4
//   bl   xAnimPoolInit        so the pool being initialised is a MEMBER at
//                             offset 12, passed by address
//   bl   xModelPoolInit
//
// The 28 is written as a sizeof because the two pools are the same size
// and that is what the single `li r5,28` before both calls says. What is
// IN those 28 bytes is not in this function, so they are bytes.

class xMemPool;

class xScene {
public:
    unsigned char _pad0[0x4];
    short f4;
    unsigned char _pad1[0x6];
    unsigned char animPool[0x4];
};

extern unsigned char sxAnimTempTranPool[0x1C];
extern unsigned char sxAnimTempStatePool[0x1C];

extern "C" void* memset(void* dst, int c, unsigned long n);

void xAnimPoolInit(xMemPool* pool, unsigned int a, unsigned int b,
                   unsigned int c, unsigned int d);
void xModelPoolInit();

void xSceneInit(xScene* s) {
    s->f4 = 0;

    memset(sxAnimTempTranPool, 0, sizeof(sxAnimTempTranPool));
    memset(sxAnimTempStatePool, 0, sizeof(sxAnimTempStatePool));

    xAnimPoolInit((xMemPool*)s->animPool, 50, 1, 1, 4);
    xModelPoolInit();
}
