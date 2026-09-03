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
// Folding them into `while (i < 10 && ...)` removes the counter reduction
// and leaves the explicit `cmpwi r6, 0xa` retail has.
//
// THE NAMES AND THE BUFFER SIZE ARE READ, NOT GUESSED. The DWARF gives this
// function exactly two variables (tools/dwarf_locals.py):
//
//   local  line 19  keyValue  char[11]  frame +8
//   local  line 20  i         int       r6
//
// The size is `char[11]` -- ten digits and a terminator -- and that is
// recovered fact rather than a lever: 10, 11, 12, 16 and 20 all emit the
// same bytes, and only 32 changes anything (it grows the frame, 6 of 26).
//
// The terminator is written with `stbx r0, r3, r6` -- indexed off the
// count, the array's base recomputed with a second `addi r3, r1, 8` rather
// than the walking pointer being reused, which is what `keyValue[i] = 0`
// gives.
//
// NEITHER WALKING POINTER IS A VARIABLE. This function sat at 2 of 26 words
// for days, and both were `addi rX, rX, 1`: retail increments the source
// before the destination (r6, r3, r4) and every spelling tried emitted
// r6, r4, r3. The exclusion list below is long and every entry of it holds
// -- but all of it varied where an explicit `s++` was written, and the
// answer was to not write one at all. The DWARF says there are two
// variables, `keyValue` and `i`; r3 and r4 are neither, they are the
// compiler's own induction variables, and a source that names one of them
// has already lost. Indexing BOTH sides --
//
//     while (i < 10 && s[i] != 0 && s[i] != ':') { keyValue[i] = s[i]; i++; }
//
// -- leaves `i` as the only thing the source increments, and mwcc creates
// the two pointers itself, in the order the subscripts first appear: `s[i]`
// in the condition, `keyValue[i]` in the body, hence r3 before r4. Exact,
// all 26 words. The parameter is never assigned, which is the whole
// difference; a strength-reduced `s` walks in the register it arrived in
// either way, so the bytes cannot tell the two apart except by the order.
//
// So the lever was the ABSENCE of a statement, and that is why sweeping
// spellings of the increments could not reach it: eight of them tied at 2,
// and a tie across eight spellings meant the varied thing was not the one
// that mattered.
//
// What was excluded on the way, all of it still true, so it is not redone:
//   * loop form: `while (i < 10)` with breaks, `for (i = 0; i < 10; i++)`,
//     a non-constant bound, and `for (;;)` with an explicit bound test.
//     All 14-15 of 26, all still counted loops.
//   * register pressure with a `dst` pointer: an explicit `char c` declared
//     after the pointer, the counter declared first, declare-then-assign,
//     `*s` as a truth test rather than `!= 0`, subscripting `s[0]` and
//     `dst[0]`, and folding the increments into `*dst++ = *s++`. Three move
//     the count the WRONG way (7, 9 and 22); the rest sit at 5.
//   * indexing the destination only, with the increments spelled eight
//     ways: `s++` before `i++` and after, both in a `for` increment clause
//     in either order, `s = s + 1`, pointer arithmetic on the destination,
//     and a named character with `s++` first. All eight tie at 2. Two that
//     put a post-increment inside the assignment (`keyValue[i] = *s++`,
//     `keyValue[i++] = *s++`) go the wrong way to 22, because the loop
//     becomes counted again.
//
// The report counts this unit 0 of 1 and unitcmp counts it 1 of 1,
// and both are right about different questions. Retail's symbol is
// scope:local, so objdiff names it hkGetKeyValue__FPCc_801ECDB0 --
// the mangled name with its address appended, which is how it keeps
// local symbols apart -- and our object's plain hkGetKeyValue__FPCc
// pairs with nothing. unitcmp pairs by the mangled name and compares
// the bytes, and they are identical. No C++ identifier can produce a
// name with a suffix past its own mangling, so the only spelling that
// would pair is an extern "C" function named after the mangling,
// which is not the symbol retail emitted.
//

class hkString {
public:
    static int atoi(const char* s, int base);
};

int hkGetKeyValue(const char* s) {
    char keyValue[11];
    int i = 0;

    while (i < 10 && s[i] != 0 && s[i] != ':') {
        keyValue[i] = s[i];
        i++;
    }

    keyValue[i] = 0;

    return hkString::atoi(keyValue, 16);
}
