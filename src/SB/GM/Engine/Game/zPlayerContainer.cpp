// C:/branches/SB09/main/GM/Engine/Game/zPlayerContainer.cpp
//
// NEAR-MISS at 87.86% (objdiff), 13 words against retail's 14. Recorded
// with its mechanism rather than left as "not matching yet", because the
// mechanism is the thing that will settle it.
//
// The layout is not in question -- the DWARF gives it exactly, and it is
// what the code reads:
//
//   class zPlayerContainer  /* 0x14 bytes */
//   {
//       /* +0x0  */ zPlayer* playerArray[4];
//       /* +0x10 */ int numPlayers;
//   };
//
// THE DIFFERENCE IS INDUCTION, AND ONLY THAT:
//
//   retail                          ours
//   lwz   r0, 0x10(r3)              lwz   r0, 0x10(r3)
//   li    r5, 0                     --
//   mtctr r0                        mtctr r0
//   cmpwi r0, 0                     cmpwi r0, 0
//   ble   ...                       ble   ...
//   lwzx  r0, r3, r5                lwz   r0, 0(r3)
//   cmplw r0, r4                    cmplw r0, r4
//   bne   ...                       bne   ...
//   li    r3, 1 ; blr               li    r3, 1 ; blr
//   addi  r5, r5, 4                 addi  r3, r3, 4
//   bdnz  ...                       bdnz  ...
//   li    r3, 0 ; blr               li    r3, 0 ; blr
//
// Retail keeps `this` in r3 across the loop and walks a separate BYTE
// OFFSET in r5 with `lwzx`. mwcc strength-reduces our loop into a moving
// base pointer instead and clobbers `this`, which it is entitled to do
// because `this` is dead after the loop. So the question is what keeps
// `this` live in the original -- and it is not this file's text: ten
// spellings were compiled and scored, and every one that is still this
// function lands on exactly 87.86%.
//
//   index / cast at use ....... 87.86     pointer-add deref ....... 87.86
//   array typed xEnt* ......... 87.86     inlined accessor ........ 87.86
//   int declared outside ...... 87.86     count in a local ........ 87.86
//   while loop ................ 87.86     this-relative index ..... 87.86
//   unsigned index ............ 83.57     found flag + break ...... 78.93
//
// The two that score WORSE are informative: they are different functions.
// The eight that tie are the same function written eight ways, which says
// the lever is not a spelling of this loop.

class xEnt;
class zPlayer;

class zPlayerContainer {
public:
    bool ContainsEnt(xEnt* ent) const;

    zPlayer* playerArray[4];
    int numPlayers;
};

bool zPlayerContainer::ContainsEnt(xEnt* ent) const {
    for (int i = 0; i < numPlayers; i++) {
        if ((xEnt*)playerArray[i] == ent) {
            return true;
        }
    }

    return false;
}
