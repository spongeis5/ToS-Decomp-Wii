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
// THE THREE TESTS ARE ONE CONDITION. Spelled as a bounded loop with two
// `break`s inside, mwcc proves the trip count and emits a counted loop --
// `mtctr` and `bdnz` -- which retail does not have; that is 15 of 26 words.
// Folding them into `while (i < 10 && *s != 0 && *s != ':')` removes the
// counter reduction and leaves the explicit `cmpwi r6, 0xa` retail has.
//
// THE NAMES AND THE BUFFER SIZE ARE READ, NOT GUESSED. The DWARF gives this
// function exactly two variables (tools/dwarf_locals.py):
//
//   local  line 19  keyValue  char[11]  frame +8
//   local  line 20  i         int       r6
//
// -- and that took it from 5 of 26 words to 2. There is NO walking
// destination pointer in the original at all: `dst` was our invention, and
// declaring one occupies the register the compiler otherwise gives the
// character. Indexing the array and letting the compiler build the
// induction variable itself is what retail does, and it had never been
// tried together with the folded condition: the earlier sweep tried
// indexing only with the `break` form, where the counted loop drowns the
// difference.
//
// The size is `char[11]` -- ten digits and a terminator -- and that is
// recovered fact rather than a lever: 10, 11, 12, 16 and 20 all emit the
// same bytes, and only 32 changes anything (it grows the frame, 6 of 26).
//
// The terminator is written with `stbx r0, r3, r6` -- indexed off the
// count, which is what `keyValue[i] = 0` gives where a pointer would have
// been reused.
//
// NEAR MISS, 2 of 26 words. Both are `addi rX, rX, 1` and they are the same
// two instructions in the opposite order: retail increments the SOURCE
// pointer before the destination (r6, r3, r4) and every spelling here emits
// r6, r4, r3. The destination is not a variable, so where its increment
// goes is decided by the induction-variable rewrite and not by the
// statement order.
//
// Already excluded, do not redo -- thirteen spellings in the first two
// sweeps and fourteen more since:
//   * loop form: `while (i < 10)` with breaks, `for (i = 0; i < 10; i++)`,
//     a non-constant bound, and `for (;;)` with an explicit bound test.
//     All 14-15 of 26, all still counted loops.
//   * register pressure with a `dst` pointer: an explicit `char c` declared
//     after the pointer, the counter declared first, declare-then-assign,
//     `*s` as a truth test rather than `!= 0`, subscripting `s[0]` and
//     `dst[0]`, and folding the increments into `*dst++ = *s++`. Three move
//     the count the WRONG way (7, 9 and 22); the rest sit at 5.
//   * indexing, with the increments spelled eight ways: `s++` before `i++`
//     and after, both in a `for` increment clause in either order,
//     `s = s + 1`, pointer arithmetic on the destination, and a named
//     character with `s++` first. ALL EIGHT TIE AT 2, and the tie is the
//     finding. Two that put a post-increment inside the assignment
//     (`keyValue[i] = *s++`, `keyValue[i++] = *s++`) go the wrong way to 22,
//     because the loop becomes counted again.
//
// So the lever is not the statement order, the loop form, the buffer size,
// or how the character is named. What is left is the order the two
// induction variables are incremented in, and nothing reachable from the
// source text has moved it.

class hkString {
public:
    static int atoi(const char* s, int base);
};

int hkGetKeyValue(const char* s) {
    char keyValue[11];
    int i = 0;

    while (i < 10 && *s != 0 && *s != ':') {
        keyValue[i] = *s;
        i++;
        s++;
    }

    keyValue[i] = 0;

    return hkString::atoi(keyValue, 16);
}
