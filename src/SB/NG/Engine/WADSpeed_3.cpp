// WADSpeed_3.cpp -- one function, the WADSpeed translation unit's static
// initialiser, read from the image with tools/disasm.py. It constructs the
// three file-scope objects the unity unit declares, in address order:
// Debug::quaternionFormatter (four bytes, its vtable pointer), Math::_matZero
// (all twelve floats zero) and Math::_matIdentity (the 3x4 identity, ones at
// v[0].x, v[1].y and v[2].z). It calls nothing: both constructors are inlined
// into it, so the whole body is two literal loads and twenty-five stores.
//
// Layouts from the DWARF (tools/dwarf_types.py): Math::Matrix33 is 0x30 with
// Vector4 v[3], and Vector4 is 0x10 -- four floats -- so twelve floats reach
// from +0 to +0x2C; Debug::QuaternionFormatter is 0x4 on a Debug::Formatter
// base of 0x4 whose only member the DWARF does not describe, which is the
// vtable pointer. The three objects' addresses come from the retail symbol
// table: quaternionFormatter__5Debug at .bss+0, _matZero__4Math at +8 and
// _matIdentity__4Math at +56, all scope:local, so all three are static here
// and mwcc reproduces the four-byte hole after the formatter by itself.
//
// Three shapes the bytes fixed. The two float values are LITERALS, not named
// constants: the relocations of the debug link (tools/disasm.py plus the
// image's own .rela) name @45187 and @45188, one HA/LO pair each, which is
// what mwcc spells for two literals -- it shares one base only at three or
// more, so the generated padding header below is included because the unit
// loads literals and the recipe says to, not because it is what makes this
// one match: measured with it and without, the bytes are the same.
// The formatter's vtable is reached by its own symbol,
// __vt__Q25Debug19QuaternionFormatter, so the class is Debug::QuaternionFormatter
// deriving from Debug::Formatter and declaring one virtual; that vtable is
// emitted into this object's .data, which the split does not have, so the unit
// matches and cannot link. And the base register the retail code forms for
// each matrix -- r5 = base+8 and r3 = base+56, with each matrix's FIRST store
// written off the section base instead -- comes out of the compiler on its
// own; nothing in the source chooses it.
//
// NEAR MISS -- unitcmp reports EXTRA, not a word count, and two things cause
// it, neither of them the source text. Compiled with the flags this project
// uses for the game library (cflags_game, -O4,s) the two constructors are NOT
// inlined: mwcc emits __ct__Q24Math8Matrix33Fffffffffffff (68 bytes, EXTRA --
// retail has no such function) and __ct__Q25Debug19QuaternionFormatterFv,
// which unitcmp reports as MATCH because retail does have that one, elsewhere
// in the image, and ours is byte-identical to it -- and calls them both. The
// initialiser then comes out 180 bytes, 45 words against retail's 36, with 36
// of them differing. Compiled
// with cflags_base (-O4,p) instead, this file is byte-identical: 36 words of
// 36, 0 differing, on the first spelling. That is the flag the unity unit's
// name suggests -- WADSpeed is the speed-optimised blob, WAD00..WAD04 the
// size-optimised ones -- and it was checked the way NOTES.md requires before
// being reported: the one WADSpeed unit that already has source,
// SB/NG/Engine/WADSpeed.cpp, has 18 functions matching, and all 18 still match
// at -O4,p (18 of 18 both ways, 0 verdicts changed). Nothing was measured for
// the units with no source at all.
//
// The second cause is the SYMBOL NAME and it is not reachable from source at
// all. CodeWarrior names a static initialiser after the main source file's
// BASENAME: this file compiles to __sinit_\WADSpeed_3_cpp where retail has
// __sinit_\WADSpeed_cpp, so unitcmp pairs nothing and prints EXTRA. The
// compiler has no option that sets that name -- its own -help lists none --
// and `#line 1 "WADSpeed.cpp"` at the top of the file does not move it either,
// measured: the symbol still follows the real file. That is the same rule
// NOTES.md records for anonymous namespaces, asked of the initialiser's name
// rather than inherited. The way through is the one used for Util/Sort/WAD02.cpp:
// give the unit a path whose basename is the blob's, which is an edit to
// splits.txt and configure.py and not to this file.
//
// Two things measured since, both of which the next attempt needs.
//
// APPENDING -O4,p DOES NOTHING. mwcc keeps the FIRST -O it is given,
// so `python tools/unitcmp.py SB/NG/Engine/WADSpeed_3 `-O4,p`` --
// unitcmp passes extra arguments after its own BASE -- compiles at
// -O4,s and prints the -O4,s answer, which reads exactly like a
// refutation of the paragraph above. The flag has to REPLACE -O4,s,
// not follow it. Any harness that does the replacing should be checked
// against a unit configure.py already records as flag-sensitive before
// it is believed: zPlayerContainer::ContainsEnt is exact at -O4,s and
// comes out 13 words against retail's 14 at -O4,p, and a harness that
// cannot reproduce that is not evidence about anything else. Done that
// way, -O4,p on this unit does what the paragraph says: both EXTRA
// constructors disappear -- they inline into the initialiser -- and the
// object is left holding the initialiser alone.
//
// AND THE TOOL WOULD NOT FOLLOW. project.py does support per-object
// flags, but unitcmp's BASE is a hardcoded list carrying -O4,s and its
// drift guard only checks that list against cflags_game. Give this unit
// its own cflags in configure.py and the build compiles it at -O4,p
// while unitcmp, unitcmp_pins, unitcmp_check and reloc_audit all keep
// measuring it at -O4,s -- the tools would disagree with the build and
// say so about the wrong thing. So the route is three edits, not two:
// the path, the per-object flags, and teaching unitcmp to take a unit's
// flags from configure.py rather than from a constant. The third is the
// one to be careful with, because every other unit's verdict goes
// through it.
//
// Spellings tried for the inlining under -O4,s, none of them moving a word:
// the constructor defined in the class body, defined outside it as `inline`,
// with #pragma always_inline on around the class and around the object
// definitions, and with -inline all. A one-store constructor and a
// one-expression `inline` free function are not inlined into a static
// initialiser either, so this is not the four-store wall NOTES.md records --
// under -O4,s mwcc inlines nothing at all into __sinit_.

#include "SB/NG/Engine/WADSpeed_3.pool.h"

namespace Debug {

class Formatter {
public:
    virtual void _v0();
};

class QuaternionFormatter : public Formatter {};

static QuaternionFormatter quaternionFormatter;

}  // namespace Debug

namespace Math {

class Vector4 {
public:
    float x;
    float y;
    float z;
    float w;
};

class Matrix33 {
public:
    Matrix33(float m00, float m01, float m02, float m03, float m10, float m11,
             float m12, float m13, float m20, float m21, float m22,
             float m23) {
        v[0].x = m00;
        v[0].y = m01;
        v[0].z = m02;
        v[0].w = m03;
        v[1].x = m10;
        v[1].y = m11;
        v[1].z = m12;
        v[1].w = m13;
        v[2].x = m20;
        v[2].y = m21;
        v[2].z = m22;
        v[2].w = m23;
    }

    Vector4 v[3];
};

static Matrix33 _matZero(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                         0.0f, 0.0f, 0.0f);
static Matrix33 _matIdentity(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                             0.0f, 0.0f, 1.0f, 0.0f);

}  // namespace Math
