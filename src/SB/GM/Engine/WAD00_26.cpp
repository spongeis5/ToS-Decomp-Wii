// WAD00_26 -- two functions, read from the image with tools/disasm.py:
// two pointer fix-ups of the kind serialised assets get after a load.
// Whatever2::Fix adds the delta to its word at +8 when that word is
// set; NPC_Action_MovementData::Fix adds it to +0x10 when the kind at
// +0x14 is 7. NEAR MISS, 6 of 7 words: retail re-reads Whatever2's word
// after the test and ours reuses the load. A punned add, a pointer
// member moved through a long lvalue, and an inline helper taking the
// member's address all fold the second read too; the plain form stays.

namespace Sext {

class Whatever2 {
public:
    void Fix(long delta);

    unsigned char _pad0[0x8];
    long f8;
};

class NPC_Action_MovementData {
public:
    void Fix(long delta);

    unsigned char _pad0[0x10];
    long f10;
    int f14;
};

}  // namespace Sext

void Sext::Whatever2::Fix(long delta) {
    if (f8 != 0) {
        f8 += delta;
    }
}

void Sext::NPC_Action_MovementData::Fix(long delta) {
    if (f14 == 7) {
        f10 += delta;
    }
}
