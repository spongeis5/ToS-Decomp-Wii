// zHitParameters -- one function: find a hit-source four-character code by
// the hash of its spelling.
//
// Read from the image with tools/disasm.py. The loop keeps TWO induction
// variables, which is what `-O4,s` does here: r29 counts 0..61 and is the
// value returned, while r31 walks the table four bytes at a time and is
// what `lwzx` indexes with.
//
//   lwzx   r0,r30,r31          zHitSourceCCs[i]
//   stb    r0,11(r1)           the low byte first -- scheduling, not order
//   srwi   r5,r0,24            then the top three, in place
//   rlwinm r4,r0,16,24,31
//   rlwinm r0,r0,24,24,31
//   stb    r5,8(r1) ; stb r4,9(r1) ; stb r0,10(r1)
//   addi   r3,r1,8 ; bl xStrHash
//   cmplw  r28,r3              UNSIGNED, so the hash is unsigned
//   cmpwi  r29,62              the table has 62 entries
//   li     r3,-1               and -1 when none matches
//
// THE BUFFER IS NOT TERMINATED. Four bytes are written at r1+8..11 and
// xStrHash is handed a `const char*`; nothing writes r1+12. That is what
// the image does, and reproducing it is the job -- the frame is 32 bytes
// and r1+16 upward is the register save area, so the byte it reads as a
// terminator is whatever the previous call left at r1+12.

// EVERY BYTE IS MASKED, including the one whose store would truncate
// it anyway. `(v >> 16)` without the `& 0xFF` compiles to a rotate
// with a 16-bit mask and `(v >> 8)` to a 24-bit one -- two words out
// of thirty-three. With the mask written, all three become the same
// rotate-and-mask-to-eight, and `cc[3]`'s mask folds away into the
// byte store on its own. So the original masked uniformly and let the
// compiler drop the one that was redundant.

unsigned int xStrHash(const char* s);

extern unsigned int zHitSourceCCs[62];

int getSourceCCIdx(unsigned int hash) {
    for (int i = 0; i < 62; i++) {
        char cc[4];
        unsigned int v = zHitSourceCCs[i];

        cc[3] = (char)(v & 0xFF);
        cc[0] = (char)((v >> 24) & 0xFF);
        cc[1] = (char)((v >> 16) & 0xFF);
        cc[2] = (char)((v >> 8) & 0xFF);

        if (hash == xStrHash(cc)) {
            return i;
        }
    }

    return -1;
}
