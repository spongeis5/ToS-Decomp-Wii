// xRegionSupport.cpp -- one function, read from the image with
// tools/disasm.py. xRegionSetDiscLanguage walks a table of region codes
// looking for the one it was handed and, on the first match, ORs that
// row's flag into the disc configuration word and returns.
//
// The DWARF (tools/dwarf_locals.py) gives the function two names: the
// parameter `pszRegionCode` in r27, declared at line 73, and one local
// `i`, an int in r28, declared at line 75 -- so `i` is declared in the
// body and not in the `for` init, and r28 is its register. It does NOT
// name the second local the bytes require; see below, where that is
// measured rather than assumed. The lexical block it records,
// 8003F284..8003F2E0, is the loop.
//
// THE TABLE IS READ OUT OF THE IMAGE, NOT INVENTED. `.data` at
// 806B4060 holds twenty pairs of {const char*, unsigned int} whose
// strings are the region codes below and whose flags are 1<<0 through
// 1<<19 in order, and the word at 806B4100 -- the size the loop
// compares against -- is 20, the same count. Eight bytes per row is
// what the access says too: the code is `lwzx` off the row and the flag
// is `lwz` at +4 of the same row.
//
// The three statics are an anonymous namespace in retail
// (gRegionFlagTable__19@unnamed@WAD00_cpp@ and its two neighbours), so
// they are an anonymous namespace here. CodeWarrior mangles such a name
// with the TRANSLATION UNIT's basename, and this unit is a fragment
// carved out of WAD00's unity build, so ours come out as
// @unnamed@xRegionSupport_cpp@ and cannot be retail's symbols. The
// references are data relocations, which unitcmp masks by field -- it
// name-checks REL24 branches only -- so the unit can be COMPARED byte
// for byte while it can never link as a fragment. The unit's own
// definitions are the honest source for it; the split is .text only,
// and these three would be WAD00's data.
//
// Two shapes the bytes fixed.
//
// The loop is TOP-TESTED and rotated: retail branches straight to the
// `cmpw r28,r29` at the bottom and falls back into the body, which is
// the plain `for` with the return inside it.
//
// THE SIZE IS READ INTO A LOCAL, and the DWARF does not name that
// local. Retail loads the size once before the loop and keeps it in
// callee-saved r29 across every strcmp; it is loaded rather than
// folded, so it is a real int in .data and not a constant. Written as a
// plain global read in the loop condition, mwcc RELOADS it every
// iteration -- `lwz r0,0(r30)` in the test -- which is 26 of 33 words,
// because the register assignment then shifts under everything else.
// mwcc does not hoist a global across a call even when the object has
// internal linkage and its address is never taken; that is the rule
// NOTES records, measured again here. So a local was written, and the
// DWARF listing only `i` is absence of a DIE rather than absence of a
// variable -- the bytes are what say so.
//
// ITS DECLARATION COMES FIRST, and that is the last four words. The two
// callee-saved registers are handed out in the order the locals are
// DECLARED, not the order they are assigned: `i` first gives i r29 and
// the size r28, which is retail's pair the wrong way round even though
// `i = 0` is emitted before the size is loaded either way. The size
// declared first gives i r28 and the size r29, as retail has them.
//
// The row address is strength reduced the way -O4,s does it everywhere
// in this image: r31 walks a byte offset eight at a time beside the
// counter r28, and the row is reached with `lwzx` and an `add` rather
// than by scaling the index.

extern "C" int strcmp(const char* s1, const char* s2);

namespace {

struct xRegionFlag {
    const char* code;
    unsigned int flag;
};

// 806B4060: twenty rows, flags 1<<0 .. 1<<19 in declaration order.
xRegionFlag gRegionFlagTable[] = {
    {"EN_US", 0x00000001}, {"EN_UK", 0x00000002}, {"DA_DK", 0x00000004},
    {"NL_NL", 0x00000008}, {"FI_FI", 0x00000010}, {"FR_FR", 0x00000020},
    {"DE_DE", 0x00000040}, {"GR_GR", 0x00000080}, {"IT_IT", 0x00000100},
    {"JA_JP", 0x00000200}, {"KO_KR", 0x00000400}, {"NO_NO", 0x00000800},
    {"PT_PT", 0x00001000}, {"PT_BZ", 0x00002000}, {"RU_RU", 0x00004000},
    {"ES_ES", 0x00008000}, {"SV_SE", 0x00010000}, {"AR_AE", 0x00020000},
    {"CZ_CZ", 0x00040000}, {"PO_PO", 0x00080000},
};

// 806B4100 holds 20, and it is loaded rather than folded.
int gRegionFlagTableSize = sizeof(gRegionFlagTable) / sizeof(gRegionFlagTable[0]);

unsigned int gDiscConfig;

}  // namespace

void xRegionSetDiscLanguage(const char* pszRegionCode) {
    int count = gRegionFlagTableSize;
    int i;

    for (i = 0; i < count; i++) {
        if (strcmp(pszRegionCode, gRegionFlagTable[i].code) == 0) {
            gDiscConfig |= gRegionFlagTable[i].flag;
            return;
        }
    }
}
