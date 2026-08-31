// C:/branches/SB09/main/NG/Source/Tools/Havok/source/Common/Base/keycode.cxx
//
// One function: copy the leading field of "xxxxxxxx:..." into a small stack
// buffer and parse it as hexadecimal.
//
// The loop is ROTATED -- the entry `b` jumps straight to the test, and the
// three exits (ten characters copied, end of string, a colon) all land on
// the same block. The character test is `extsb.` followed by a compare of
// the SIGN-EXTENDED value against ':', so the element is a plain `char`.
//
// The terminator is written with `stbx r0, r3, r6` -- indexed off the count,
// not off the walking destination pointer, which is what `buf[i] = 0` gives
// where `*dst = 0` would have reused the pointer register.
//
// THE THREE TESTS ARE ONE CONDITION. Spelled as a bounded loop with two
// `break`s inside, mwcc proves the trip count and emits a counted loop --
// `mtctr` and `bdnz` -- which retail does not have; that is 15 of 26 words.
// Folding them into `while (i < 10 && *s != 0 && *s != ':')` removes the
// counter reduction and leaves the explicit `cmpwi r6, 0xa` retail has.
//
// NEAR MISS, 5 of 26 words, and every one of the five is the SAME SWAP:
// retail keeps the walking destination in r4 and the character in r5, and
// every spelling tried puts them the other way round. The instructions,
// their order, and the whole loop shape are identical.
//
// Already excluded, do not redo -- thirteen spellings across two sweeps:
//   * loop form: `while (i < 10)` with breaks, `for (i = 0; i < 10; i++)`,
//     a non-constant bound, `for (;;)` with an explicit bound test, and
//     indexing the buffer instead of walking a pointer. All 14-15 of 26,
//     all still counted loops.
//   * register pressure: an explicit `char c` declared after the pointer,
//     the counter declared first, declare-then-assign, `*s` as a truth
//     test rather than `!= 0`, subscripting `s[0]` and `dst[0]`, and
//     folding the increments into `*dst++ = *s++`. Three of those move the
//     count the WRONG way (7, 9 and 22); the rest sit at the same 5.
//
// So the lever is not the statement order, the loop form, or how the
// character is named. It is whatever decides which of two equally live
// values gets the lower register, and nothing tried so far reaches it.

class hkString {
public:
    static int atoi(const char* s, int base);
};

int hkGetKeyValue(const char* s) {
    char buf[16];
    char* dst = buf;
    int i = 0;

    while (i < 10 && *s != 0 && *s != ':') {
        *dst = *s;
        i++;
        s++;
        dst++;
    }

    buf[i] = 0;

    return hkString::atoi(buf, 16);
}
