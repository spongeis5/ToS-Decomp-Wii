// xTextDepot.cpp -- three functions, read from the image with
// tools/disasm.py. A text depot is a fixed array of 32 string pointers
// over one 1024-byte character buffer, packed end to end, with a
// running count of the bytes used. The constructor zeroes the count,
// then the pointers, then the buffer. GetString returns the slot, or
// the empty string when the slot is null. SetString reserves room for
// the new text: with the slot already occupied it slides everything
// after the old string down over it with memmove, walks all 32 slots
// and pulls back every pointer that sat above the one being replaced,
// then points the slot at the end of the buffer and moves the count by
// the difference in lengths; with the slot empty it appends. Either way
// the text is copied in last.
//
// Layout from the DWARF (tools/dwarf_types.py): xTextDepot is 0x484
// bytes -- char* strings[32] at +0, char characterBuffer[1024] at +0x80
// and int characterCount at +0x480. Nothing else is touched.
//
// The empty string is at +79 of the unity unit's pool
// (@stringBase0 0x8067F560), so the eight strings that precede it have
// to enter our pool first or the `addi r3,r3,79` comes out with a
// different immediate -- that word is a plain immediate, not a
// relocated field, so it is compared. gen_poolprefix.py REFUSES this
// unit: it writes the prefix that precedes the earliest string a unit
// is FIRST to reference, and this unit is first to reference none --
// ShowMessageBox at 0x800327C0 put the empty string in the pool. The
// table below is the same prefix (every string of that pool with
// offset < 79) read from the image with that tool's own pool_strings
// and c_string, and like the ones it writes it is DATA, not source:
// retail has no such table, it has the files in front of this one.
//
// Three shapes the bytes fixed. The constructor's first memset clears
// 32 bytes and not the 128 the array is -- one per slot, which is what
// the image says and not what the type says. Every mention of
// strings[index] is a fresh read: retail folds the null test and the
// strlen argument into one load, because nothing is between them, and
// reloads after each call and inside the loop, because a store through
// a char* can alias the slot. And the two length differences are
// separate statements, so characterCount is read again after the slot
// is written.

extern "C" {
unsigned long strlen(const char* s);
void* memset(void* dst, int c, unsigned long n);
void* memcpy(void* dst, const void* src, unsigned long n);
void* memmove(void* dst, const void* src, unsigned long n);
}

// The pool prefix: see the note above. Read from @stringBase0 at
// 0x8067F560, every string before +79, in pool order.
static const char* const kUnityPoolPrefix[] = {
    "{}()<>",                  // +0
    " ,\t\n\x0D",              // +7
    "#+*?{}()<>|;",            // +13
    "SBB3",                    // +26
    "SpongebobIdleDialogRef",  // +31
    "%d",                      // +54
    "%.*s",                    // +57
    "_root.promptType",        // +62
};

class xTextDepot {
public:
    xTextDepot();

    char* GetString(int index);
    void SetString(int index, const char* str);

    char* strings[32];
    char characterBuffer[1024];
    int characterCount;
};

xTextDepot::xTextDepot() {
    characterCount = 0;

    memset(strings, 0, 32);
    memset(characterBuffer, 0, 1024);
}

char* xTextDepot::GetString(int index) {
    char* str = strings[index];

    if (str == 0) {
        return "";
    }

    return str;
}

void xTextDepot::SetString(int index, const char* str) {
    int newLen = strlen(str) + 1;

    if (strings[index] != 0) {
        int oldLen = strlen(strings[index]) + 1;

        memmove(strings[index], strings[index] + oldLen,
                characterCount - ((strings[index] - characterBuffer) + oldLen));

        for (int i = 0; i < 32; i++) {
            if (strings[i] > strings[index]) {
                strings[i] -= oldLen;
            }
        }

        strings[index] = &characterBuffer[characterCount - oldLen];
        characterCount += newLen - oldLen;
    } else {
        strings[index] = &characterBuffer[characterCount];
        characterCount += newLen;
    }

    memcpy(strings[index], str, newLen);
}
