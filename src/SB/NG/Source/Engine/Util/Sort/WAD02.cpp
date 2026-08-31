// C:/branches/SB09/main/NG/Source/Engine/Util/Sort.cpp
//
// THE FILE NAME IS NOT A MISTAKE. Three of this unit's symbols live in
// an anonymous namespace, and CodeWarrior mangles those with the name of
// the TRANSLATION UNIT: retail's carry `@unnamed@WAD02_cpp@` because
// Sort.cpp was compiled as part of the WAD02 unity blob. The mangler
// reads the input file's BASENAME -- `#line 1 "WAD02.cpp"` does NOT move
// it, measured -- so a file called Sort.cpp produces
// `@unnamed@Sort_cpp@` and the symbols can never pair however right the
// bytes are. Naming the file WAD02.cpp at a different path reproduces
// the mangling exactly, and both functors below match byte for byte
// because of it. tools/anon_blocked.py lists every unit in that
// position.
//
// AND THE BODY IS A MACRO, not a function template. Retail has one
// 648-byte QuickSortInt with the whole sort inside it and no call to a
// shared routine. CodeWarrior will not produce that from a function:
// the body contains loops, and NOTHING inlines a loop body at these
// flags -- `inline`, `#pragma inline_max_size`,
// `#pragma inline_max_auto_size`, `#pragma inline_depth`,
// `-inline all`, `-inline on`, `-inline auto,level=4` and
// `-inline deferred` all leave the call standing. Only `-opt full`
// inlines it, and that regresses four units that already match.
//
// The comparison IS a functor. SortUintFunctor and SortUint64Functor
// are real symbols with an `offset` member, emitted straight after the
// function that first used them, which is why the definitions
// interleave in retail's address order. The Int comparison is a single
// subtraction and inlines away, which is why there is no SortIntFunctor
// symbol and why QuickSortInt is 648 bytes where the other two are 732.
//
// Both functors' tails are the tell: `subfic` then `subfe` is a
// set-if-zero, so the second half is `-(x != y)` and not a second
// compare and branch.
//
// Three things the bytes state that the obvious spelling gets wrong:
//
//   * The SHORT range is the FALLTHROUGH. Retail's first branch is
//     `bgt` to a later block, so the insertion sort sits directly after
//     the test and the partition is the one jumped to. Writing
//     `if (hi - lo > threshold)` the natural way round lays it out the
//     other way and moves every word in the function.
//
//   * The swap's bound is `se = sa; se += size;` -- a copy and an
//     add-assign, NOT `se = sa + size`. As one expression the compiler
//     folds it with whatever register already holds that value (usually
//     `i`, which is `lo + size`); three `add`s disappear and it needs a
//     fourth callee-saved register to keep the folded value alive.
//     Retail recomputes it every time. That one change took the
//     function from 640 bytes to exactly 648, and from 138 to 162 of
//     162 instructions agreeing in shape.
//
//   * Argument ORDER matters at one swap site: retail's third
//     median-of-three swap is SORT_SWAP(i, lo), not SORT_SWAP(lo, i).
//     The first argument is the one whose end bounds the loop, so the
//     other way round makes the bound `lo + size` -- foldable again.
//
// NEAR MISS. All three sorts come out at exactly the retail SIZE (648,
// 732, 732) with every instruction identical in shape. QuickSortInt has
// 34 of 162 words differing and the other two 67 of 183, and every one
// of them is a register assignment inside a swap loop. Registers r0
// through r10 -- lo, j, size, offset, i, hi, sp, threshold and the
// stack base -- all map identically. What is left is that retail keeps
// the insertion sort's p and q in r30/r29 and the swap's own locals in
// r12/r11, and ours is the other way round.
//
// Already excluded, do not redo:
//   * 8 top-level declaration orders, then 20 placements of p and q
//     once hoisted out of the branch. Hoisting j, i, mid and half is
//     what fixed the outer registers (115 words -> 76); p and q move
//     nothing at all.
//   * all 48 orderings of the swap macro's four locals, with the `+=`
//     in two places. Best is `st, se, sb, sa` at 34; the natural
//     `sa, sb, se` is 49, and computing `se` directly is 107.
//   * 7 spellings of the bound itself and 6 shapes of the outer body.
//
// RelativeSortRecur and RelativeSort are not written yet, which is the
// unit's other reason for being NonMatching.

namespace Util {

namespace {

struct SortIntFunctor {
    SortIntFunctor(int off) : offset(off) {}

    int operator()(const void* a, const void* b) const {
        return *(int*)((char*)a + offset) - *(int*)((char*)b + offset);
    }

    int offset;
};

struct SortUintFunctor {
    SortUintFunctor(int off) : offset(off) {}

    int operator()(const void* a, const void* b) const {
        unsigned int x = *(unsigned int*)((char*)a + offset);
        unsigned int y = *(unsigned int*)((char*)b + offset);

        if (x > y) {
            return 1;
        }

        return -(x != y);
    }

    int offset;
};

struct SortUint64Functor {
    SortUint64Functor(int off) : offset(off) {}

    int operator()(const void* a, const void* b) const {
        unsigned long long x = *(unsigned long long*)((char*)a + offset);
        unsigned long long y = *(unsigned long long*)((char*)b + offset);

        if (x > y) {
            return 1;
        }

        return -(x != y);
    }

    int offset;
};

}  // namespace

// Word at a time over `size` bytes, and the loop is entered at its TEST --
// a `while`, not a `do`. The two pointers walk independently, so the
// element's own end is what stops it.
// Word at a time over `size` bytes, and the loop is entered at its TEST
// -- a `while`, not a `do`. The two pointers walk independently, so the
// FIRST argument's end is what stops it, and which argument comes first
// is therefore load-bearing.
//
// It has to be a macro. As a function it is never inlined: mwcc will not
// inline a body containing a loop at these flags, and `inline`,
// `#pragma inline_max_size`, `inline_max_auto_size`, `inline_depth`,
// `-inline all`, `-inline on`, `-inline auto,level=4` and
// `-inline deferred` all leave the call standing. Only `-opt full` does
// it, and that regresses four units that already match.
#define SORT_SWAP(a, b, size)                                             \
    {                                                                     \
        int st;                                                           \
        char* se = (a);                                                   \
        char* sb = (b);                                                   \
        char* sa = (a);                                                   \
                                                                          \
        se += (size);                                                     \
                                                                          \
        while (sa != se) {                                                \
            st = *(int*)sa;                                               \
                                                                          \
            *(int*)sa = *(int*)sb;                                        \
            sa += 4;                                                      \
            *(int*)sb = st;                                               \
            sb += 4;                                                      \
        }                                                                 \
    }


// An explicit stack rather than recursion, and the LARGER half is pushed so
// the depth stays logarithmic. Ranges of seven elements or fewer are left
// to the insertion sort.
#define SORT_BODY(base, count, size, compare)                              \
    char* j;                                                              \
    char* i;                                                              \
    char* mid;                                                            \
    int half;                                                             \
    char* stack[42];                                                      \
    char* lo = (char*)(base);                                             \
    char* hi = lo + (count) * (size);                                     \
    char** sp = stack;                                                    \
    int threshold = (size) * 7;                                           \
                                                                          \
    for (;;) {                                                             \
        if (hi - lo <= threshold) {                                        \
            char* p = lo;                                                  \
            char* q = lo + (size);                                         \
                                                                           \
            while (q < hi) {                                               \
                while (compare(p, p + (size)) > 0) {                       \
                    SORT_SWAP(p, p + (size), size)                        \
                                                                           \
                    if (p == lo) {                                         \
                        break;                                             \
                    }                                                      \
                                                                           \
                    p -= (size);                                           \
                }                                                          \
                                                                           \
                p = q;                                                     \
                q += (size);                                               \
            }                                                              \
                                                                           \
            if (sp == stack) {                                             \
                break;                                                     \
            }                                                              \
                                                                           \
            sp -= 2;                                                       \
            lo = sp[0];                                                    \
            hi = sp[1];                                                    \
        } else {                                                           \
            half = (hi - lo) >> 1;                                        \
                                                                           \
            half -= half % (size);                                         \
            mid = lo + half;                                               \
                                                                           \
            SORT_SWAP(mid, lo, size)                                      \
                                                                           \
            i = lo + (size);                                               \
            j = hi - (size);                                               \
                                                                           \
            if (compare(i, j) > 0) {                                       \
                SORT_SWAP(i, j, size)                                     \
            }                                                              \
                                                                           \
            if (compare(lo, j) > 0) {                                      \
                SORT_SWAP(lo, j, size)                                    \
            } else if (compare(i, lo) > 0) {                               \
                SORT_SWAP(i, lo, size)                                    \
            }                                                              \
                                                                           \
            for (;;) {                                                     \
                do {                                                       \
                    i += (size);                                           \
                } while (compare(i, lo) < 0);                              \
                                                                           \
                do {                                                       \
                    j -= (size);                                           \
                } while (compare(j, lo) > 0);                              \
                                                                           \
                if (i > j) {                                               \
                    break;                                                 \
                }                                                          \
                                                                           \
                SORT_SWAP(i, j, size)                                     \
            }                                                              \
                                                                           \
            SORT_SWAP(lo, j, size)                                        \
                                                                           \
            if (j - lo > hi - i) {                                         \
                sp[0] = lo;                                                \
                sp[1] = j;                                                 \
                lo = i;                                                    \
            } else {                                                       \
                sp[0] = i;                                                 \
                sp[1] = hi;                                                \
                hi = j;                                                    \
            }                                                              \
                                                                           \
            sp += 2;                                                       \
        }                                                                  \
    }

void QuickSortInt(void* base, int count, int size, int offset) {
    SortIntFunctor compare(offset);

    SORT_BODY(base, count, size, compare)
}

void QuickSortUint(void* base, int count, int size, int offset) {
    SortUintFunctor compare(offset);

    SORT_BODY(base, count, size, compare)
}

void QuickSortUint64(void* base, int count, int size, int offset) {
    SortUint64Functor compare(offset);

    SORT_BODY(base, count, size, compare)
}

}  // namespace Util
