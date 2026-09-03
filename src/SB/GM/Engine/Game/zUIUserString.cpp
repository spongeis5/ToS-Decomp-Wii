// zUIUserString.cpp -- six functions, read from the image with
// tools/disasm.py. A user string is the UI widget the player types into.
// DoInit runs the text widget's own init, points the link array at the
// asset's event links, takes one block of hardMaxChars + 1 bytes from the
// global heap (tag 40) for the buffer and hands it to SetText. DoExit
// frees the buffer. DoReset re-shows the buffer, takes the soft maximum
// from the asset, empties the buffer, and copies the widget's current
// text into it when there is any. DoHandleEvent answers four ids: one
// looks a text id up and copies the string in, sending an event when the
// lookup fails or the result fills the field; one appends a character
// taken from the event's float, sending an event when the field is then
// full; one erases the last character, sending an event when the field is
// then empty; and one sets the maximum from the event's second float,
// clamped to the asset's hard maximum, truncating the text if it no
// longer fits. Anything else goes to the base's handler. GetSortKey is
// the asset's uid. The asset's Create takes one 416-byte block from the
// global heap (tag 16), places the widget on it and runs zUI_Init.
//
// Layouts from the DWARF (tools/dwarf_types.py): zUIUserString is 0x1A0
// with textBuffer at +0x194, length at +0x198 and maxLength at +0x19C,
// on zUIText 0x1A0 whose text is at +0xFC and whose own last member,
// dirty, is at +0x190, on zUI 0x100 with the asset at +0x40, on
// World::xOGEntity 0x40 whose ogModel is at +0x34, on xBase 0x38 with
// the link array at +0x28. Each of those sizes is the next class's first
// member sitting in its predecessor's TAIL PADDING, and the padding here
// is bytes rather than the real members: retail's chain is 8-aligned at
// xBase's uid and 16-aligned at the Math::Vector inside zUI's state
// blocks, and a member of either alignment rounds zUIText's size up past
// 0x194 and moves the three members this file owns. Bytes keep the class
// 4-aligned, so sizeof(zUIUserString) is 0x1A0 -- the 416 the allocation
// asks for -- and every offset above is the DWARF's.
// The asset, Sext::UI_User_String, is 0xF0 on Sext::UIText 0xE0 on
// Sext::zUI 0xB0, with its uid first, hardMaxChars at +0xD8,
// softMaxChars at +0xD9 and EventLinksNew at +0xDC.
// Sext::EventAny is 1 byte with NOTHING described in the DWARF, so its
// three fields here are only what these bytes read: a float at +0, a
// float at +4 and an unsigned int at +0x14.
//
// __vt__13zUIUserString at 806C29C4 is 0xE4 bytes -- two leading zero
// words and 55 virtuals, with DoInit at +164, DoReset at +172,
// DoHandleEvent at +180, DoExit at +192 and GetSortKey at +88. Nothing
// here dispatches through it; what needs it is Create, whose constructor
// stores it, so the class only has to be POLYMORPHIC. One undefined
// virtual on xBase does that, the vptr lands at +0, and since the class
// defines no virtual of its own the vtable's home stays in the unity
// unit's data.
//
// Eight shapes the bytes fixed.
//
// The buffer is taken through the game's NewArray, not AllocGlobalHeap
// directly: retail builds the heap argument with `lis; lwz` off the head
// of the unity unit's .data rather than `li r4,0`, which is the static
// temporary a const reference binds an enumerator to -- one anonymous
// zero word per call site, the same shape WAD03's out-of-line
// NewArray<float> instantiation takes its heap by. Create, two functions
// later, calls AllocGlobalHeap itself and does have `li r4,0`. The
// template has to be written `inline`: left plain it is emitted
// out-of-line and called, a whole extra function and eight wrong words,
// even though its body is the single expression -inline auto otherwise
// takes. And the size is a NAMED local computed before the first of the
// two empty calls, because retail computes hardMaxChars + 1 ahead of
// that call and an argument written at the call site lands after it.
//
// The two calls around that allocation are the weak empty body every
// empty function folded into, which the image names Math::Matrix33's
// constructor, so they are called by that mangled symbol as Text.cpp and
// zRandomModelList.cpp call it. Neither sets up an argument: what the
// original passed is not in the bytes, and a no-argument call is what
// reproduces them.
//
// The four event ids are a `switch`, not a chain: all four compares come
// first, each body follows in case order, and the base handler sits after
// the last of them as the default. The ids are read off the `addis`/
// `cmplwi` pairs -- 0x2E646593, 0xC1EF376F, 0x67F908F5 and 0x497D4EB3 --
// and so are the two the widget sends, 0x406AFEB0 and 0x43139E6C.
//
// DoReset reads `text` into a LOCAL. Retail loads it once into r31 and
// keeps it across strcpy and strlen -- which is why its prologue saves
// two callee-saved registers with `stmw r30` and not one -- where the
// member written out three times is reloaded each time and the function
// comes out 24 of 29 words wrong, one word short.
//
// The text id's default is an `if`, not a conditional expression: retail
// materialises the zero and then branches PAST the load, where
// `any ? any->textID : 0` branches to the load and jumps past the zero,
// one word the other way. Its length output is left UNINITIALISED; a
// `= 0` on it is a store to the frame slot retail does not have.
//
// The two float-to-character conversions are not the same shape, and the
// difference is one word. The appended character goes through TWO
// locals, an int and then a char: retail converts with fctiwz and then
// EXTSB, and a single `char c = (char)any->character;` drops the
// extension because the only use is a byte store (123 of 129 words; six
// spellings tie there, and the int-then-char pair is what reaches 125).
// The maximum, in the fourth case, is a plain `(int)` with no extension.
//
// And both index expressions carry their own step: `textBuffer[length++]
// = c` and `textBuffer[--length] = 0`. Written as a separate statement
// the increment reloads the member -- a char store can alias an int
// member, so the compiler must -- and the decrement's temporary and the
// zero constant swap registers, r0 against r5. Ten spellings of the
// decrement were tried and only the pre-decrement in the subscript gives
// retail's pair.
//
// NO NEAR MISS. `python tools/unitcmp.py
// SB/GM/Engine/Game/zUIUserString.cpp` reports 6 of 6 byte-identical,
// 900 bytes, which is every function the split holds. That is unitcmp's
// answer and not the oracle's: the unit has not been through ninja and
// is not marked Matching in configure.py. It would not link as it
// stands either -- the heap reference's static temporary is a .data
// word this fragment emits and the split does not own.

typedef unsigned long long uid;

enum eMemMgrTag { eMemMgrTag_ = 0x7FFFFFFF };

namespace Memory {
enum GlobalHeapEnum { GlobalHeapEnum_ = 0x7FFFFFFF };

void* AllocGlobalHeap(unsigned long size, GlobalHeapEnum heap, eMemMgrTag tag,
                      bool clear);
}  // namespace Memory

extern "C" {
void* memset(void* dst, int c, unsigned long n);
char* strcpy(char* dst, const char* src);
unsigned long strlen(const char* s);

// The lone `blr` at 800075C0 the image names Math::Matrix33's
// constructor: the weak empty function every empty body folded onto.
void __ct__Q24Math8Matrix33Fv();
}

inline void* operator new(unsigned long, void* p) { return p; }

// The game's array allocator: the heap comes by const reference, which is
// what puts the enumerator in a static temporary and loads it back.
template <class T, class H>
inline T* NewArray(const H& heap, eMemMgrTag tag, unsigned long count) {
    return (T*)Memory::AllocGlobalHeap(count * sizeof(T), heap, tag, false);
}

class LinkAsset;
class xBase;
class zUI;
class zUIUserString;

namespace World {
class EntityHandleBase;
}

namespace Sext {

// One byte and nothing described in the DWARF; these three fields are
// what this file's bytes read out of it.
class EventAny {
public:
    float character;
    float maxChars;
    unsigned char _pad0[0x14 - 0x8];
    unsigned int textID;
};

class zUI {
public:
    uid id;
    unsigned char _pad0[0xB0 - 0x8];
};

// UIText's own last member ends at +0xD8 and its 0xE0 size is the uid
// alignment, so the derived asset's two byte fields sit in that padding.
class UIText : public zUI {
public:
    unsigned char _pad0[0xD8 - 0xB0];
};

class UI_User_String : public UIText {
public:
    static zUIUserString* Create(World::EntityHandleBase* handle,
                                 UI_User_String* asset);

    unsigned char hardMaxChars;
    unsigned char softMaxChars;
    unsigned char _pad1[2];
    unsigned char EventLinksNew[0xF0 - 0xDC];
};

}  // namespace Sext

enum ForceEvent { ForceEvent_ = 0x7FFFFFFF };

void zEntEvent(xBase* from, unsigned int fromEvent, xBase* to,
               unsigned int event, Sext::EventAny* any, ForceEvent force);

char* xTextFindString(unsigned int id, unsigned int* length);

void freeWrapper(void* p);

void zUI_Init(zUI* ui, Sext::zUI* asset);

class xBase {
public:
    virtual void _v0();

    unsigned char _pad0[0x28 - 0x4];
    LinkAsset* linkArray;
    unsigned char _pad1[0x34 - 0x2C];
};

namespace World {

class xOGEntity : public xBase {
public:
    unsigned char ogModel[8];
};

}  // namespace World

class zUI : public World::xOGEntity {
public:
    zUI(World::EntityHandleBase* handle);

    void DoHandleEvent(xBase* from, unsigned int event, Sext::EventAny* any);

    unsigned int UIViewportMask;
    Sext::zUI* asset;
    unsigned char _pad0[0xF6 - 0x44];
};

class zUIText : public zUI {
public:
    zUIText(World::EntityHandleBase* handle) : zUI(handle) {}

    void DoInit();
    void DoReset();
    void SetText(const char* s);

    unsigned int shadowColor;
    char* text;
    unsigned char _pad0[0x190 - 0x100];
    bool dirty;
};

class zUIUserString : public zUIText {
public:
    zUIUserString(World::EntityHandleBase* handle) : zUIText(handle) {}

    void DoInit();
    void DoExit();
    void DoReset();
    void DoHandleEvent(xBase* from, unsigned int event, Sext::EventAny* any);
    uid GetSortKey() const;

    char* textBuffer;
    int length;
    int maxLength;
};

void zUIUserString::DoInit() {
    zUIText::DoInit();

    linkArray = (LinkAsset*)((Sext::UI_User_String*)asset)->EventLinksNew;

    {
        unsigned long size = ((Sext::UI_User_String*)asset)->hardMaxChars + 1;
        char* buffer;

        __ct__Q24Math8Matrix33Fv();

        buffer = NewArray<char, Memory::GlobalHeapEnum>(
            (Memory::GlobalHeapEnum)0, (eMemMgrTag)40, size);

        __ct__Q24Math8Matrix33Fv();

        textBuffer = buffer;

        SetText(buffer);
    }
}

void zUIUserString::DoExit() {
    freeWrapper(textBuffer);
}

void zUIUserString::DoReset() {
    zUIText::DoReset();

    SetText(textBuffer);

    {
        char* current = text;

        maxLength = ((Sext::UI_User_String*)asset)->softMaxChars;
        length = 0;
        textBuffer[0] = 0;

        if (current != 0) {
            strcpy(textBuffer, current);

            length = strlen(current);
        }
    }
}

void zUIUserString::DoHandleEvent(xBase* from, unsigned int event,
                                  Sext::EventAny* any) {
    switch (event) {
    case 0x2E646593: {
        unsigned int found;
        unsigned int id = 0;
        char* str;

        if (any != 0) {
            id = any->textID;
        }

        str = xTextFindString(id, &found);

        if (str == 0) {
            textBuffer[0] = 0;
            length = 0;

            zEntEvent(this, 0, this, 0x406AFEB0, 0, (ForceEvent)1);
        } else {
            strcpy(textBuffer, str + 1);

            length = found;

            if (length >= maxLength) {
                zEntEvent(this, 0, this, 0x406AFEB0, 0, (ForceEvent)1);
            }
        }

        break;
    }
    case 0xC1EF376F: {
        int v = (int)any->character;
        char c = (char)v;

        if (length < maxLength) {
            textBuffer[length++] = c;
            textBuffer[length] = 0;

            if (length == maxLength) {
                zEntEvent(this, 0, this, 0x43139E6C, 0, (ForceEvent)1);
            }
        }

        break;
    }
    case 0x67F908F5:
        if (length > 0) {
            textBuffer[--length] = 0;

            if (length == 0) {
                zEntEvent(this, 0, this, 0x406AFEB0, 0, (ForceEvent)1);
            }
        }

        break;
    case 0x497D4EB3:
        maxLength = (int)any->maxChars;

        if (maxLength > ((Sext::UI_User_String*)asset)->hardMaxChars) {
            maxLength = ((Sext::UI_User_String*)asset)->hardMaxChars;
        }

        if (length > maxLength) {
            length = maxLength;
            textBuffer[length] = 0;
        }

        break;
    default:
        zUI::DoHandleEvent(from, event, any);
        break;
    }
}

uid zUIUserString::GetSortKey() const {
    return asset->id;
}

zUIUserString* Sext::UI_User_String::Create(World::EntityHandleBase* handle,
                                            UI_User_String* asset) {
    zUIUserString* ui = new (memset(
        Memory::AllocGlobalHeap(sizeof(zUIUserString),
                                (Memory::GlobalHeapEnum)0, (eMemMgrTag)16,
                                false),
        0, sizeof(zUIUserString))) zUIUserString(handle);

    zUI_Init(ui, asset);

    return ui;
}
