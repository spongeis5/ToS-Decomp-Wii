// xTagParser.cpp -- three functions, read from the image with
// tools/disasm.py. Parse lays the tagged text out in a text box and
// renders it back to a plain string: it builds a font (white on black,
// two-pixel shadow, clipped to the screen bounds), builds a text box on
// that font, copies the box, gives it the whole screen for its bounds,
// sets the text, takes a temporary layout and renders that layout to the
// caller's buffer, returning the buffer. xTagParserKludgeHasTags walks a
// string for an opening brace. xTagParserKludgeParse holds the parser as
// a function-local static -- the image keeps its guard byte and the
// object as @GUARD@ and @LOCAL@ symbols -- and tail-branches to Parse.
//
// Layouts from the DWARF (tools/dwarf_types.py): basic_rect 0x10 as four
// floats; xColor four bytes; xfont 0x38; xtextbox 0x88, which is exactly
// the 17 iterations of eight bytes the copy loop moves; xtextbox::layout
// 0x120 opening with the box it laid out. screen_bounds, g_WHITE and
// g_BLACK are the translation unit's own data, and the three literals
// (0, 1 and 2) are its pool, hence the generated header first.
//
// Parse takes `this` and never reads it: the kludge passes the static
// parser, and the bytes use r3 for the stack frame from the first
// instruction.
//
// Two shapes the bytes fixed, and both are about WHERE a returned
// class lands.
//
// A function returning a class by value writes a temporary, and the
// compiler copies that temporary into a NAMED variable rather than
// returning into it. Retail has exactly one such copy, of the 136-byte
// text box, and none of the 56-byte font -- so the font is not a
// variable at all, it is the temporary passed straight to the box's
// create. Naming it costs a second copy loop and 11 words; naming the
// box's result twice costs a third and 20.
//
// And the function-local static parser has a CONSTRUCTOR. Retail
// stores zero to the object and one to a guard byte on first entry,
// which is the guarded initialisation of a static with a non-trivial
// constructor; a plain member with no constructor is initialised
// statically, emits no guard, and the function is 8 words instead of
// 17.

#include "SB/GM/Engine/Core/x/xTagParser.pool.h"

template <class T>
class basic_rect {
public:
    T x;
    T y;
    T w;
    T h;
};

class xColor {
public:
    unsigned int rgbaU32;
};

extern const basic_rect<float> screen_bounds;
extern const xColor g_WHITE;
extern const xColor g_BLACK;

class xfont {
public:
    static xfont create(unsigned int id, float width, float height,
                        float space, xColor color,
                        const basic_rect<float>& clip, xColor shadowColor,
                        float shadowOffsetX, float shadowOffsetY);

    unsigned long long id;
    float width;
    float height;
    float space;
    xColor color;
    xColor shadowColor;
    float shadowOffsetX;
    float shadowOffsetY;
    basic_rect<float> clip;
};

class substr {
public:
    char* text;
    unsigned long size;
};

class xtextbox {
public:
    class callback;

    class layout {
    public:
        unsigned long RenderToString(const xtextbox& tb, int start, int end,
                                     char* dest, unsigned long size);

        unsigned char _pad0[0x120];
    };

    static xtextbox create(const xfont& font, const basic_rect<float>& bounds,
                           unsigned int flags, float line_space, float tab_stop,
                           float left_indent, float right_indent,
                           float draw_depth, unsigned char brightness);

    void set_text(const char* text);
    layout& temp_layout(bool recalculate) const;

    xfont font;
    unsigned char brightness;
    unsigned char pad[3];
    basic_rect<float> bounds;
    float draw_depth;
    unsigned int flags;
    float line_space;
    float tab_stop;
    float left_indent;
    float right_indent;
    callback* cb;
    void* context;
    char** texts;
    unsigned long* text_sizes;
    unsigned long texts_size;
    substr text;
    unsigned int text_hash;
    unsigned int viewportMask;
};

class xTagParser {
public:
    xTagParser() : inTag(false) {}

    char* Parse(const char* text, char* dest, unsigned long size);

    bool inTag;
};

char* xTagParser::Parse(const char* text, char* dest, unsigned long size) {
    xtextbox box = xtextbox::create(
        xfont::create(0, 0.0f, 0.0f, 0.0f, g_WHITE, screen_bounds, g_BLACK,
                      2.0f, 2.0f),
        screen_bounds, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0);

    box.bounds.x = 0.0f;
    box.bounds.y = 0.0f;
    box.bounds.w = 1.0f;
    box.bounds.h = 1.0f;

    box.set_text(text);
    box.temp_layout(true).RenderToString(box, 0, -1, dest, size);

    return dest;
}

bool xTagParserKludgeHasTags(const char* text) {
    bool found = false;

    if (text) {
        while (*text) {
            if (*text == '{') {
                found = true;
                break;
            }

            text++;
        }
    }

    return found;
}

char* xTagParserKludgeParse(const char* text, char* dest, unsigned long size) {
    static xTagParser parser;

    return parser.Parse(text, dest, size);
}
