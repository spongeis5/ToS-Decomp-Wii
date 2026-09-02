// C:/branches/SB09/main/GM/Engine/Core/x/xString.cpp
//
// The hash folds case ARITHMETICALLY, without a table or a branch:
//
//     c - ((c >> 1) & c & 0x20)
//
// For a lowercase letter, `c & 0x20` is 0x20 and `c >> 1` also has that bit
// set, so 0x20 comes off and 'a' becomes 'A'. For an uppercase letter
// `c & 0x20` is already zero and nothing is subtracted. The result is cast
// back to `char` before it is accumulated -- that is the `extsb` between
// the subtract and the add, and without it the multiply-accumulate is done
// in the wrong width.
//
// The multiplier is 131, emitted as `mulli`.
//
// The case tests in xStrupr and xStrlwr are inlined PREDICATES, not plain
// ifs. Each materialises 0 or 1 into a register and then tests it, which a
// bare `if (c >= 'a' && c <= 'z')` does not do -- it branches straight to
// the body.
//
// And the adjustment is a TERNARY, not an if. Retail materialises `(char)c`
// BEFORE the branch, into the same register the predicate's 0/1 was just
// tested out of; an `if` that mutates `c` and stores afterwards defers that
// `extsb` past the branch and is one instruction out of place. Four other
// spellings -- naming the bool, testing `*p` directly, and two orderings of
// the two -- all stop at 9 of 18 or 4 of 18.
//
// Both return `str`. Retail copies the argument into another register and
// never touches r3 again, which a void function has no reason to do.

typedef unsigned long ulong;

#define xIsLower(c) ((c) >= 'a' && (c) <= 'z')

static inline bool xIsUpper(char c) {
    return c >= 'A' && c <= 'Z';
}

#define xToUpper(c) (xIsLower(c) ? (c) - 0x20 : (c))

extern "C" double atof(const char* s);

unsigned int xStrHash(const char* str) {
    unsigned int hash = 0;

    while (*str != 0) {
        char c = *str;

        str++;
        hash = (char)(c - ((c >> 1) & c & 0x20)) + hash * 131;
    }

    return hash;
}

unsigned int xStrHash(const char* str, ulong len) {
    unsigned int hash = 0;
    ulong i = 0;

    while (i < len && *str != 0) {
        char c = *str;

        i++;
        str++;
        hash = (char)(c - ((c >> 1) & c & 0x20)) + hash * 131;
    }

    return hash;
}

char* xStrupr(char* str) {
    char* p = str;

    while (*p != 0) {
        char c = *p;

        *p = xIsLower(c) ? c - 0x20 : c;
        p++;
    }

    return str;
}

char* xStrlwr(char* str) {
    char* p = str;

    while (*p != 0) {
        char c = *p;

        *p = xIsUpper(c) ? c + 0x20 : c;
        p++;
    }

    return str;
}

// NEAR MISS, 272 bytes against 264. Two structural findings landed -- the
// end-of-string test is an `||` and not an if/else-if, and the final
// comparison evaluates s2's side first, which is what puts `li r3, 1` early
// -- and what is left is BLOCK LAYOUT. Retail places the continue part
// (the end-of-string test and the two increments) ABOVE the body and enters
// the loop with a jump past it; ours puts it below. Same control flow graph,
// two instructions apart.
//
// xStrHash(const char*, ulong) is a near miss too, at 16 of 20 words, and
// there the difference is purely where the scheduler puts `cmplw i, len`.
// Five statement orderings move it not at all.
int xStricmp(const char* s1, const char* s2) {
    int atEnd = 0;

    while (xToUpper(*s1) == xToUpper(*s2) && !atEnd) {
        if (*s1 == 0 || *s2 == 0) {
            atEnd = 1;
        } else {
            s1++;
            s2++;
        }
    }

    int result = 0;

    if (*s1 != *s2) {
        result = 1;

        if (xToUpper(*s1) < xToUpper(*s2)) {
            result = -1;
        }
    }

    return result;
}

float xStrParseFloat(const char* s) {
    return atof(s);
}

// Case-insensitive memcmp. The fold is the OTHER direction from the hash's:
// `c | ((c >> 1) & 0x20)` SETS bit five for a letter, turning 'A' into 'a'.
//
// It has to go through a HELPER. Written inline as an expression, the two
// folded values take fresh registers; retail reuses the register the raw
// byte was loaded into. Four spellings with the expression written out --
// both declaration orders, locals at the top, and the index last -- all sit
// at 10 of 20. The helper's body arrives after the caller's expressions are
// already placed, and that is the whole difference.
static inline int xFoldLower(char c) {
    return c | (((unsigned char)c >> 1) & 0x20);
}

int imemcmp(const void* a, const void* b, unsigned long n) {
    const char* p = (const char*)a;
    const char* q = (const char*)b;
    unsigned long i;

    for (i = 0; i < n; i++) {
        int ca = xFoldLower(*p);
        int cb = xFoldLower(*q);

        if (ca != cb) {
            return ca - cb;
        }

        p++;
        q++;
    }

    return 0;
}
