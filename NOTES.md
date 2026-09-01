# Truth or Square (Wii) -- working notes

What has been established, what is still open, and the reasons. Figures are
from `ninja` and `build/R8IE78/report.json`; re-run rather than trusting the
numbers here, which move.

## State at time of writing

```
Game Code:  25 of 498 files complete  16,936 / 2,115,452 bytes  961 / 10,559 fn
            0.8006% of game code

Of those 961 functions, 869 are GENERATED -- machine-recognised
shapes, not one of which is decompiling. They are real matched
functions and the offsets and constants are recovered fact, but a
count of them is not a count of decompiled code. HAND-WRITTEN IS
92, across 32 units and 5,628 bytes, and that is the figure to
compare against earlier ones.

Data:       3 unit(s) carry their own, 396 bytes; 75 more could
All:        1.94% matched              main.dol reproduces byte for byte
```

Every number above is written by `python tools/notes_state.py`,
which reads report.json, `written_vs_generated.py` and
`dwarf_data_carve.py`. Do not edit it by hand -- it has drifted
three times, always in the flattering direction. `--check` fails
when it is stale.
The other categories (Revolution SDK 15.60%, SDK Code 5.29%) were already
there; `Game Code` is the column this work moves.

## The compiler flags -- the highest-value findings

The game library builds with **`-O4,s -sdata 0 -sdata2 0 -use_lmw_stmw on`**
on top of the base flags. See `cflags_game` in `configure.py`, which carries
the reason for each. Each was found the same way: write a function, find the
BODY already identical, and look at what the compiler did around it.

| flag | found by | symptom without it |
|---|---|---|
| `-O4,s` | `zPlayerContainer` | loop strength-reduced onto `this`; retail keeps `this` and walks a byte offset with `lwzx` |
| `-sdata 0 -sdata2 0` | `iTime` | statics reached through r13, so each function loses its `lis` |
| `-use_lmw_stmw on` | `MediaObject` | prologue saves with two `stw` where retail uses one `stmw` |

Every one was checked against **all** units that already matched before
being adopted. A flag that fixes one and breaks one is not an answer.

Twelve alternatives have since been swept against every Matching unit of the
game library at once: `-schedule off`, `-opt noschedule`, `-opt speed`,
`-opt level=3`, `-inline on`, `-inline all`, `-inline auto,level=2`,
`-opt nopeephole`, `-proc 750`, `-func_align 4`, `-opt nospace`. **Five
regress units that match today** -- turning scheduling off alone costs 15
functions -- and the other six change nothing anywhere. The four in
`cflags_game` are the best of the thirteen sets measured.

Independent corroboration for `-sdata 0`: `tools/dwarf_data.py` measured
7,054 data references from the recovered units and found **zero** through
r13 or r2.

## Tools added

| tool | what it does |
|---|---|
| `dwarf_types.py` | C++ type declarations from the DWARF; `--type X`, `--conflicts`. 3,739 named types |
| `dwarf_types_check.py` | how many of them actually compile: 71 of 120 sampled (59%) |
| `dwarf_splits.py` | cuts the WAD unity builds into real source files, `--apply` |
| `dwarf_targets.py` | ranks the recovered units by what their code needs (data / calls / neither) |
| `dwarf_data.py` | attributes `.data`/`.bss`/`.rodata` by who references it |
| `dwarf_locals.py` | every local and parameter of a function: type, declaration line, and WHICH REGISTER or frame slot it got |
| `dwarf_lines.py` | which source line and FILE each instruction came from; `--report` for the population |
| `dwarf_data_decl.py` | attributes data to files by DECLARATION, where `dwarf_data.py` does it by reference |
| `dwarf_data_carve.py` | the three edits that give a unit its data: definitions, split, `force_active`; `--survey` for who can |
| `unitcmp.py` | compile ONE unit and compare each function by name against retail; `-v` for a word-by-word diff |
| `unitcmp_check.py` | validates `unitcmp.py` against every unit it has a known answer for, and proves its drift guard fires |
| `anon_blocked.py` | which units can never match while they are split out of their unity blob |
| `gen_typeids.py` | the constant-return codec -- decode, re-encode, demangle. `gen_accessors` imports it; it no longer writes files |
| `gen_survey.py` | where else the constant-return shape lives |
| `gen_accessors.py` | generate every mechanical shape for a unit -- members, globals, constants, constructors; `--survey` for what is left |
| `gen_units.py` | run that over EVERY unit that has candidates, and withdraw the files that no longer do |
| `shape_census.py` | what the unmatched short functions LOOK like, as a population, by opcode signature |
| `unitcmp_pins.py` | re-measure `unitcmp_check`'s pins; refuses to lower one |
| `written_vs_generated.py` | the split, from the banner in each source file |
| `notes_state.py` | writes the State block at the top of this file; `--check` fails when it is stale |
| `next_functions.py` | what is left ranked by functions rather than bytes |
| `gen_rttid.py` | the RTTID_Fix<T> family -- 175 functions from one template; `--survey` |
| `reloc_audit.py` | which already-matched functions branch somewhere retail does not |

`pip install pyelftools` is required for all of them.

`unitcmp.py` is the iteration loop: seconds per attempt instead of a full
`configure.py` + `ninja`, and it names WHICH function is still wrong and at
which word. It is **not** the oracle -- see the second trap below -- so every
result still goes through `ninja` before a unit is called Matching. It masks
relocated fields (an unlinked object holds 0 where retail holds the resolved
value; comparing those raw reported two already-correct functions as 33/36
and 23/25), and it refuses to load if its copy of the flags drifts from
`configure.py`.

## How the unity builds were split

`DW_AT_decl_file` resolves for **all 10,064** functions across 819 source
files, so the WAD blobs can be cut back into translation units. The trick
that made the interior reachable: carving a file out of the middle leaves
the parent with text on both sides of the hole, which dtk correctly refuses
as a link-order cycle -- but only because both sides carry the same NAME.
Each unity unit is now an alternating sequence of recovered files and
`<unit>_N.cpp` remainder chunks. That took it from 7 units to 257.

Headers are deliberately NOT units: an inline emitted out-of-line belongs to
the `.cpp` that used it, and dtk rejects the fiction anyway.

## report.json IS BLIND TO RELOCATION TARGETS

The oracle compares the BITS of a relocated field, and both sides hold
zero there. So a function can branch to entirely the wrong place and
still be counted, and for a four-byte tail call -- one word, all of it
a REL24 field -- **nothing whatever is compared**.

Measured, not reasoned: re-base one RTTID_Fix stub on the wrong class,
so its single instruction calls a different function, rebuild, and
report.json still says 100.0% and the byte count does not move.

`unitcmp.py` now asks the other half of the question. Our object's
relocation NAMES the symbol; retail's resolved displacement lands on
one; the two have to agree. A masked word it still cannot resolve is
counted as `unmeasured`, and a function where that is ALL of them
reports UNMEAS rather than MATCH -- a measurement that did not happen
must not read as a benign one.

Three things that check found, in the order they were found:

1. **Two bugs in the check itself, both caught by it firing on input
   report.json calls correct.** The address map took the FIRST symbol
   of a duplicated name where `retail()` takes the last, so bodies and
   addresses came from different copies; and it held only sized
   functions, so `_savegpr_29` and `_restgpr_29` -- interior labels of
   one routine, no size -- had no name at the address every prologue
   branches to. A guard that fires on correct input is worse than none,
   and both were fixed before anything was believed.

2. **Twelve functions were counted as matched while calling the wrong
   constructor.** Eleven in WAD01, one in WAD03_22. The cause was ours:
   a base constructor DEFINED in the same generated file gets inlined
   into the derived constructors that call it, and the call then emits
   to the base's own base instead. `#pragma dont_inline on` around the
   definitions fixes it, `gen_accessors.py` emits that pragma whenever
   a unit has a `basector`, and 21 generated units now carry it.

3. **Nothing else.** `tools/reloc_audit.py` re-run afterwards: 0 wrong
   branches of 799 game functions checked, out of the 1,578 report.json
   calls matched -- the other 779 are in the 130 SDK and library units
   `unitcmp` cannot build with the game flags, and they were NOT
   audited rather than being assumed clean.

The mutation test lives in `unitcmp_check.py` beside the flag-drift
one: move every retail address by four and the byte-identical count
MUST fall, or the comparison is dead.

## The generators were not exhausted; the SHAPE SET was

NOTES said the generators were finished because `gen_accessors
--survey` reported one candidate left. That survey asks about the ten
shapes the tool knew, and its symbol scan only looked at functions of
8, 12, 16, 20 or 60 bytes -- so a four-byte function was invisible to
it, and 261 of them were sitting in `shape_census` as the single
commonest unmatched shape in the game.

Nine shapes were added in one sitting and every one of them was read
out of the population first -- how many carry it, how many register
assignments they use, what varies. None was guessed at from one
example. They took Game Code from 0.6355% to 0.8006%.

| shape | what it is | found |
|---|---|---|
| `rttid` | `RTTID_Fix<T>` -- one template, 175 instantiations | 700 B |
| `animcb` | a static callback forwarding through `xAnimSingle` | 1,296 B |
| `animcbdata` | the same, with the object taken from the `void*` | 432 B |
| `eqconst` | `return fX == K;` -- the one row with no relocation | 300 B |
| `vcall` | `this->V(args)` through the vtable | 208 B |
| `vcallm` | `fX->V(args)` through the vtable | 340 B |
| `basefwd` | a bare `b`: a member call with nothing moved | ~200 B |
| `memfwd` | the same through a member: `fX->G(args)` | ~96 B |
| `gcall` | `GLOBAL.M(args)` -- an address built, then a tail call | ~96 B |
| `argcall` | `mr r3,r4`: an argument becomes the object | ~48 B |

The vtable ones need no relocation at all, which is the point: the
slot is a plain immediate, so nothing NAMES the method and nothing has
to. What must be reproduced is the INDEX, and mwcc puts the Nth
virtual at 8 + 4N -- compiled and compared before any of it was
written. The class then gets index+1 virtuals, DECLARED and never
defined, so no vtable lands in the object; that was checked too,
because a vtable of undefined entries in a unit that links would break
the link.

Three things the compiler and the audit caught, in order:

1. **Two shapes disagreed about one method.** `zPlantTrap::HitCheck`
   is the target of a callback AND has its own `return f5C == 5;`, so
   one shape declared it `void` and the other `bool`. Callbacks are
   `bool` now and return -- the return type is not in a CodeWarrior
   symbol and does not change a tail call's bytes -- and a callback
   does not re-declare a method the class declares for another reason.
2. **A scope that is also a class.** Every qualified name is emitted
   as nested namespaces, and `World::TextureResourceEntity::TextureContainer`
   is nested in a CLASS. mwcc says `illegal namespace`; the nested one
   is refused rather than guessed at.
3. **`__ct__` is a spelling, not a name.** A forwarding call whose
   target is a constructor emitted a method literally called `__ct__`,
   which mangles to `__ct____Q24Math8Matrix33Fv` -- one `__` too many.
   `main.dol` was still byte-identical and report.json still said
   100%, because a relocated field holds zero on both sides.
   `tools/reloc_audit.py` is the only thing that could have found it.

## The RTTID_Fix<T> family -- 175 functions from one template

`shape_census.py` had been saying for a while that the commonest shape
among unmatched game functions is a bare `b` -- 261 of them -- and that
176 carry the name `RTTID_Fix<...>__4UtilFPvl_v`. NOTES said the
generators were exhausted; what was exhausted was `gen_accessors`'
SHAPE SET, and its symbol scan does not even look at four-byte
functions. The census was pointing at the answer the whole time.

    namespace Util {
    template <class T> void RTTID_Fix(void* p, long l) { ((T*)p)->Fix(l); }
    }

compiles to exactly one word per instantiation, and mwcc spells the
instantiation `RTTID_Fix<Q24Sext5Curve>__4UtilFPvl_v` -- retail's own
symbol, checked before anything was generated.

The difficulty is that the target is not always T's own Fix: 137 of the
175 branch to `Fix__<T>Fl` and 38 branch to another class's, because T
inherits it or the linker folded two identical bodies. Both are
reproduced without deciding which: declare T as deriving from the class
that owns the symbol retail branches to. Single inheritance at offset 0
leaves r3 alone, so the body stays four bytes.

One is refused and stays refused: `RTTID_Fix<Sext::CylinderAsset>`
branches to `CustomFix__Q24Sext10xBaseAssetFl`, a differently NAMED
method, which no derivation reaches.

## Where to pick up

Run these three first; they take about four minutes together and they say
what is true rather than what was true:

```bash
ninja                                  # main.dol: OK, and report.json
python tools/notes_state.py            # rewrite the State block above
python tools/unitcmp_check.py          # 117 pins, 0 failures expected
```

Then one of these, in the order they are worth doing:

1. **Write another unit.** `zBTNodeReference.cpp` went from nothing to four
   of seven functions in one sitting, and the method is now routine: pick a
   unit, run `dwarf_lines.py --unit` for the original's statement
   structure, `dwarf_locals.py --unit` for each `this` and every local's
   register, and resolve the branch and `lis`/`addi` targets from the
   symbol table before writing a line. The candidates are small recovered
   units that are not yet Matching -- `View.cpp` (4 functions, 336 bytes),
   `zPOWObject.cpp` (3, 252), `zBTNodeSequence.cpp` (5, 468),
   `zRandomModelList.cpp` (4, 424), `Renderable.cpp` (2, 224). This is the
   only column that means decompiling and it is the one to move.

1b. **Another shape.** `python tools/shape_census.py` still ranks what is
   left, and the biggest row by far is `addi b` -- 97 functions, 776 bytes
   -- of which 689 of the 787 image-wide are `@N@` MULTIPLE-INHERITANCE
   ADJUSTOR THUNKS. The compiler emits those from a class declaration, so
   the work is recovering an MI layout rather than writing a body. After
   that: `lwz or or lwz or mtspr bcctr` (14, 392 B) is the event-wrapper
   family, five of whose fourteen are in anonymous namespaces; `lfs stfs
   bclr` (14, 168 B) is `update_tag_*`, free functions in an anonymous
   namespace that `split_symbol` refuses. Anonymous-namespace support in
   `split_symbol` would unlock several rows at once, and the unit is
   already named after its blob, so the mangling would come out right.

2. **Give a unit its data.** `dwarf_data_carve.py --survey` says 75 could
   take theirs. It prints all three edits; make them as printed. Three
   units carry data today and each cost one new lesson, all of them now in
   the tool: cut the parent rather than extending it, force-active for
   anything nothing references (`config.yml` for a global, `#pragma
   force_active on` for an anonymous-namespace local), and the trailing
   padding belongs to the unit.

3. **The near misses.** Four are recorded with their exclusion lists, and
   two of those lists are long enough that re-running them is waste. What
   moved `keycode.cxx` from 5 words to 2 was the DWARF saying it had no
   variable we had invented; that is the kind of thing worth asking again,
   and `dwarf_lines.py` has only been pointed at two of the four.

WHAT NOT TO DO. Do not read "the generators are exhausted" off
`gen_accessors --survey` again. That is what it said before nine shapes
were added on top of it, and what it actually measures is the shapes the
tool already knows. `tools/shape_census.py` measures the population, and
that is the one to ask.

AND RUN `tools/reloc_audit.py` after anything that generates a call.
report.json cannot see a branch target at all, so a wrong one is silently
counted -- it has found twelve of those once and a mangling bug once.

## Open problems, in order of value

1. **The data tier. THREE UNITS CARRY THEIR OWN DATA, 396 bytes.**
   `Collide.cpp` owns 96 bytes of `.bss` at `0x8077AAA8` -- six static data
   members of `Math::QuickCull20` and `Math::QuickCull15` -- and it links,
   at exactly those addresses, with `main.dol` still byte-identical. Game
   Code data went 4 -> 100 bytes. The recipe, all three steps needed:

   1. **The variables**, from `tools/dwarf_data_decl.py` plus the retail
      symbol table for the mangled names. Defined in ADDRESS ORDER; the
      order in the file is the order in `.bss`. `float[4]` for a 16-byte
      slot: nothing with align 16 can be right, because `0x8077AAA8` is
      not 16-aligned and that is where retail put it.
   2. **The split**, and the parent has to be CUT, not extended. Leaving
      both halves under the parent's own name gives
      `Cyclic dependency ... WAD00.cpp -> Blobloids.cpp`, because the
      parent then has to come both before and after. Naming the upper half
      after the chunk that FOLLOWS in text order breaks it -- the same
      trick `dwarf_splits.py` already uses for `.text`.
   3. **`force_active` in `config.yml`.** Nothing in the image references
      those six statics, so the linker threw them away and everything
      after them moved down by 96 bytes. The object was perfect the whole
      time -- `.bss` 96 bytes, align 8, six symbols at 0/16/32/48/64/80 --
      and the failure looked exactly like a layout mistake. Retail's own
      link kept them; ours has to be told to.

   `Env.cpp` is the second, and the cheapest kind there is: one function,
   264 bytes of `.bss`, and a range `0x8072DE70..0x8072DF78` that NO unit
   in splits.txt owned -- so step 2 was not needed at all, nothing was cut
   and no link order changed. Its lesson is the FOUR-BYTE HOLE: `collBSP`
   starts at `DE78` and not `DE74`, so it is 8-aligned and the compiler
   leaves the hole itself. `double[16]` reproduces that -- 128 bytes,
   align 8 -- where `float[32]` packs it against `collBSPCount` and moves
   everything after. The element type is a guess; the SIZE and the
   ALIGNMENT are the recovered facts, and they are what decide the layout.

   `tools/dwarf_data_carve.py` does the reading: it prints the
   definitions, the splits edits and the force_active names for a unit,
   and refuses rather than guessing. It surveys **54 units that could take
   their data** -- 8 with no cut needed, 46 needing the parent cut -- and
   refuses 31 for anonymous namespaces and 17 for alignment.

   It caught a mistake in `Env.cpp`, which was written before it. Those
   three variables are `collBSPCount__19@unnamed@WAD00_cpp@` and friends in
   retail -- an anonymous namespace -- and the source declared plain
   globals. The PLACEMENT was right and main.dol was byte-identical,
   because nothing outside the unit reaches them; the NAMES were not, so
   objdiff could not pair them and the three had external linkage where
   retail's are internal.

   **The anonymous-namespace trick works for DATA too**, which is what
   fixed it. The unit is now `Core/Wii/Env/WAD00.cpp` -- named after the
   blob, at a path that says what it is, the way `Util/Sort/WAD02.cpp`
   already does for text -- and all three symbols come out exactly as
   retail spells them, with main.dol still byte-identical. That took
   `dwarf_data_carve.py --survey` from 54 units to **75**, since an
   anonymous namespace is now an instruction rather than a refusal.

   One thing that does NOT work: those symbols are LOCAL, so `force_active`
   in config.yml cannot hold them, and the linker says so out loud --
   `FORCEACTIVE symbol '@unnamed@WAD00_cpp@::rigidBodies' is either not a
   global symbol or doesn't exist. Ignored.` **`#pragma force_active on`
   around the definitions does it instead**, and that is the only reason
   the unit links.

   `PostRenderChannel.cpp` is the third and the first with INITIALISED
   data -- 17 bytes of `.bss` and 16 of `.data`, whose values are read out
   of the image (1, 2, 3, 4) rather than guessed, and being non-zero is
   exactly why they are in `.data` where the other five are in `.bss`. Its
   one function is `PostRenderChannel::buffer = Channel::buffer;`, which
   the two addresses in it say outright.

   It also found the last edge in the recipe: **the trailing padding
   belongs to the unit**. `commit` is one byte at `80779F98`, so the range
   ended at `F99` and the remainder started there --

       Invalid alignment for split: ... .bss 8:0x80779F99

   -- where retail's next variable is at `F9C` and the three bytes between
   are this unit's own. `dwarf_data_carve.py` rounds the end up to 4 now,
   without crossing whatever is declared next, and reproduces that range on
   its own.

   What is still blocked, and by what:
   - **The anonymous namespace, not alignment.** `Blobloids.cpp` was the
     first candidate and failed on its one variable being
     `blobMemCB__Q27Domains19@unnamed@WAD00_cpp@` -- an anonymous namespace
     inside `Domains`, so the mangled name carries the BLOB's basename. See
     `tools/anon_blocked.py`; the way through is naming the source file
     after the blob.
   - The two blockers below, re-measured.

   Two distinct blockers, both measured:
   - mwcc emits `.bss` with align 8 and no option changes it. A 4-aligned
     `.bss` start (like `iTime`'s `sGameTime` at `0x8072E17C`) cannot be
     honoured; linking it shifts 10,115 bytes of the DOL.

     **Re-measured 2026-08-31, and it blocks a THIRD of the tier, not all
     of it.** iTime was linked for real to check: `sGameTime` moves from
     `0x8072E17C` to `0x8072E180`, `.bss` grows by 8, and the 10,115 bytes
     are 8,982 runs of ONE byte each, every one the low half of an address
     immediate that moved by 8. So the mechanism is exactly as recorded.

     What was never counted is how many placements it touches. Over the
     341 objects this project compiles, mwcc emits **no data section below
     align 8** -- `.bss`, `.data`, `.rodata`, `.sdata`, `.sbss` are 8, 32
     or 64, never 4 -- and `.text` is 16 in 317 of them. But of the 492
     (file, section) placements the DWARF gives, **356 need a start that
     is already 8-aligned or better** and mwcc's alignment does not move
     them. Only 136 do not. By file: 199 of 321 are clean. Counting only
     files that are units in `splits.txt`: **72 unblocked, 39 blocked.**

     Two options were checked against the SECTION ALIGNMENT rather than
     against matched-function counts, which is what the twelve-flag sweep
     asked. `-func_align 4` does move `.text` from 16 to 4 -- and drops
     the inter-function padding with it, 44 bytes to 32, so it is not a
     route. `-align` is documented in the compiler's own help as
     "structure/array alignment" and moves neither. Nothing in that help
     addresses a data section's alignment.

     A trap to avoid repeating: the "target" object dtk carves has `.text`
     align 4 and `.bss` align 4, and iTime.cpp's comment compares against
     those as if they were the original build's. They are dtk's defaults
     for a reconstructed object. `-O4` implies `func_align 16` by the
     compiler's own help, so the original build's objects were align 16
     too. The only thing worth comparing against is whether the LINK comes
     out right.
   - An interior unit's data range sits inside the parent's, and carving it
     out needs the parent's data partitioned across its chunks **in link
     order**. Attributing data by WHO REFERENCES IT gets 62% / 60% / 74%
     in order for `.rodata` / `.data` / `.bss`, which is not mechanical
     enough to split on.

     **Attributing it by DECLARATION does much better**, and that was
     available all along: `tools/dwarf_data_decl.py` reads the 2,853
     file-scope variables whose location list holds a fixed address, each
     with a `DW_AT_decl_file`. 230 of 272 files own a contiguous run of
     `.bss` (85%), 148 of 153 of `.data` (97%) and 63 of 65 of `.rodata`
     (97%), with no address claimed by two files. `dwarf_data.py` said in
     its docstring that the DWARF "places no data at all"; that was a
     generalisation from some location lists to all of them, and the
     correction is written out at the top of that file.

     It does not cover everything: anonymous data -- literals, pools,
     jump tables -- has no DIE, which is 96.7% of `.rodata`, and only the
     eleven compile units with debug info are covered at all.

     The old guess for the disorder was `-inline auto` -- a static
     belonging to file A, referenced only through A's code inlined into
     B, is attributed to B. Declaration-attribution does not care either
     way, so that guess no longer has to be settled to make progress. It
     also cannot be settled from the debug info: this producer emits no
     `DW_TAG_inlined_subroutine` at all, and its line table keeps the
     CALLER's line for inlined code -- `xOGModelRefPtr::IsSet` is 68 bytes
     over two source lines and is known to have `IsValid` inlined into it.

   `iTime.cpp` and `zPerformanceDisplay.cpp` are written and their objects
   match 100%; both are `NonMatching` because of the above.

2. **Four near-misses with exhausted searches.** Do not redo these:
   - `zNPCType` 84.53% -- register allocation only; structure identical.
     Five declaration orders tried, all 8 of 19 words.
   - `zNPCStatus` 67.55% -- retail constructs `Math::Vector` in place at the
     member; ours builds a temporary and copies. **Seven spellings and
     eight flag sets tried**, all 6 of 22. What is left is the real
     declaration of `Math::Vector`, which the DWARF does not pin down.
   - `keycode.cxx` **2 of 26 words**, down from 5 on 2026-08-31, and the
     three that fell fell to the DWARF. `tools/dwarf_locals.py` says the
     function has exactly two variables -- `keyValue`, a `char[11]` in a
     frame slot, and `i`, an int in r6 -- and NO walking destination
     pointer. Ours declared one, and it occupied the register the compiler
     otherwise gives the character. Indexing the array instead had been
     tried before, but only with the `break` form of the loop, where the
     counted loop drowns the difference; with the folded condition it is
     worth three words. What is left is two `addi rX,rX,1` in the opposite
     order, and eight spellings of the increments all tie at 2.
   - `xOGModelRefPtr` 3 of 4 -- `GetWeakPtr` is right in all thirty words
     except where the first load sits in the prologue. **Seven spellings and
     twelve flag sets tried**, six of the spellings emitting identical bytes.
     The lever is the instruction scheduler's placement, and it is reachable
     from neither the source text nor any option swept so far. The file
     carries the full exclusion list.

3. **More units.** `tools/dwarf_targets.py` ranks them. The remaining
   no-data tier is now down to the three functions of `StaticBuilder.cpp`
   that are not written -- Create takes EIGHTEEN parameters, and
   CreateRenderable and SetBuffers reach further into the graphics types
   than has been recovered -- and the five of `Sort.cpp`.

   Beware the file name: `Engine/Graphics/Scaleform.cpp` and
   `Game/zScaleform.cpp` are different units in different unity blobs, and
   a suffix match on the first returns both.

4. **Anonymous namespaces pin a unit to its blob.** CodeWarrior mangles an
   anonymous namespace with the name of the TRANSLATION UNIT, so a function
   in one carries `@unnamed@WAD02_cpp@` in retail and would carry
   `@unnamed@<our file>_cpp@` in ours. `#line` does not move it -- the
   mangler reads the input file's BASENAME, measured.
   `tools/anon_blocked.py` lists the units affected and what they are worth.

   The way through is to NAME THE SOURCE FILE after the blob, at a different
   path: `Util/Sort/WAD02.cpp` reproduces `@unnamed@WAD02_cpp@` exactly, and
   dtk accepts the duplicate basename because the object paths differ. Both
   of Sort.cpp's functors matched byte for byte the first time because of
   it.

## Nothing personal in tracked files

`python tools/test_privacy.py` refuses to let identifying information reach
a public repository, and `hooks/pre-commit` runs it before a commit exists.
Run `git config core.hooksPath hooks` after any fresh clone -- a hook that
lives only in `.git/hooks` does not survive one.

It exists because it was needed and was not there. `tools/dwarf_types_check.py`
carried a hardcoded absolute path with the author's Windows account name in
it; that went out in a commit and sat in the tree of sixteen more before
anyone thought to look. Thinking to look is not a mechanism.

The FIRST version of the checker only asked about the working tree, which is
the narrower question and the one that feels like an answer. `git grep`
reports the TREE; a push sends the OBJECTS. The two rules are therefore
asked twice, once of tracked files and once of every git object, reachable
and unreachable -- an unreachable blob is not published but is still on the
disk, and the remedies differ (a rewrite versus a `git gc`). One batched
`git cat-file` does it in about a tenth of a second, because a check too
slow to run every time becomes an opt-in check, which is a check that does
not run.

Nothing personal is written in the checker itself -- a blocklist of a name
publishes that name to every reader. The account name is derived from
`Path.home().name` at runtime, commit identities are matched against the
SHAPE of a GitHub privacy address, and home directories are matched by
shape too. `tools/test_privacy_guard.py` plants all three home-path shapes
to prove the checker rejects them, and `tools/test_privacy_history_guard.py`
plants git OBJECTS -- reachable, unreachable, and one placeholder that must
NOT fire -- because a guard that has never been seen to fire is not known to
work, and one that fires on correct input is worse than none.

That second failure mode is not hypothetical here either. The home-path rule
allows one placeholder account component, `redacted`, so that it does not
fire on the evidence of a completed rewrite. The first version of that
allowlist held eight words including `user` and `someone`, and it silently
disarmed the other guard's own fixtures. The guard caught it.

The identity check deliberately covers `upstream/main..HEAD` only. The
inherited history carries the upstream author's own address in their own
public repository; that is correct attribution, and rewriting it would put
someone else's work under a different name.

## The generated shapes, and what they cost to get right

`gen_accessors.py` now recognises ten shapes -- member-only, constant, one
global, or a constructor -- and `gen_units.py` runs it over every unit that
has any. That took Game Code from 8,528 to 13,200 bytes across 94 units, and
**not one byte of it is decompiling**, which is why
`written_vs_generated.py` exists.

The tenth is the largest single row the census had left, and the first that
is a whole function rather than one instruction with a return:

    stwu r1,-16(r1) ; mflr r0 ; stw r0,20(r1) ; stw r31,12(r1)
    mr r31,r3 ; bl __ct__<Base> ; lis r4,HI ; mr r3,r31
    addi r4,r4,LO ; stw r4,N(r31) ; <epilogue> ; blr

A constructor that calls its base and then stores its own vtable pointer:
`Derived::Derived(args) : Base(args) {}`, 60 bytes, 45 of them. The base is
read out of the `bl` -- its symbol IS `__ct__<Base>` -- and its argument
list has to be the same as ours, which for 46 of the 47 it is. **N says how
the base is declared**: at 0 the base shares the vtable pointer and is
declared polymorphic, above 0 the base sits in front of it and is declared
padded to exactly N bytes. Both were tried by hand against the image before
any of this was written, one of each kind, and both matched first time.

A class that has a base subobject does not have its members where a bare
offset says, so where one class has both a base constructor and a measured
layout the BASE CONSTRUCTOR is dropped -- nothing that already matched can
be lost that way.

The last three came from the same census rows. A function whose whole body
touches ONE GLOBAL -- `return g`, `return &g`, `g = v` -- is reachable
because a variable carries the same qualifier a function does
(`activeViewport__Q28Graphics7Display`), and CodeWarrior spells that the
same whether the scope is a class or a namespace, so an `extern` in nested
namespaces reproduces the symbol. Where the scope IS a class this file
declares, it goes in as a static member instead. 42 of the 56 twelve-byte
`lis`-based functions point at a symbol nameable that way; the other 14 are
static locals, anonymous namespaces and string literals, which no
declaration can name.

Two of those shapes collide, and the collision cost 139 functions for one
build: `lis r3,HI ; addi r3,r3,LO ; blr` is BOTH "return the address of a
global" and "return this 32-bit constant". The tie-break is the test
already measured for the constant returns -- inside a loaded section it is
an address, outside it is a number -- and until that was put in, WAD02_36's
164 constant returns were read as global references, found no symbol, and
were dropped.

The population was found by measuring rather than by noticing. Both earlier
generators were written after someone spotted a shape by accident, so
`shape_census.py` asks the question directly: group every unmatched game
function by its sequence of opcodes and count. 1,546 of the 9,736 unmatched
functions are 32 bytes or less, carrying 27,032 bytes across 591 distinct
signatures, and the top of that list is where the next generator goes. The
five biggest that are still unwritten:

| N | bytes | signature | what it is |
|---|---|---|---|
| 261 | 1,044 | `b` | a tail call; needs the target's full signature |
| 97 | 776 | `addi b` | a tail call after adjusting `this` |
| 54 | 1,296 | `lwz or or lwz or b` | a member loaded, then a tail call |
| 38 | 608 | `addis addi stw bclr` | a member set to the address of a static local or an anonymous-namespace object -- unnameable |
| 23 | 368 | `or or or b` | registers shuffled, then a tail call |

`b` was measured before being written off: of the 262 four-byte functions,
only 55 branch to a target whose parameter list is the same as the caller's,
and 32 of those 55 are a derived class forwarding to its base. 199 branch to
something with a different signature. So the biggest row on the census is
worth about 220 bytes, not 1,044, and it is not the next thing to do.

**A constructor that stores its own vtable pointer is reachable, and that
was worth 19 of the 57 in the fourth row.** The body is `lis`/`addi` of
`__vt__<class>`, a store at offset 0, and `blr`. ONE declared virtual makes
mwcc emit `__vt__<class>` itself and makes the constructor store it, so
nothing has to be named by hand: the relocation lands on the same symbol
retail relocates against. What the real class's virtuals were is not in
those four words, so our vtable has one entry and retail's has its own --
the CONSTRUCTOR matches, the table does not, and since the unit is
NonMatching nothing is linked from it either way. The generator refuses the
case where the store is not at offset 0, which means a base class sits in
front of the pointer and how many bytes of it there are is not in the
function.

**A constant return whose value is an ADDRESS cannot be written as a
number.** Retail reaches it through a relocation; writing the constant
reproduces the instruction word and not the relocation, so the object
differs by that field. `unitcmp` masks relocated fields and calls such a
function byte-identical -- and it is right to, for its own question -- while
`report.json` never agreed. Fifteen functions sat in the tree in exactly
that state. The test is whether the value lands inside a loaded section, and
it was validated before being adopted: of the 245 constant returns in the
game code, the 230 that land outside a section ALL match and the 15 that
land inside match NOT AT ALL. A perfect split, and the generator refuses to
write the second group as a number. Some of those 15 came back later by the
other route: an address whose symbol can be NAMED is `return &g`, which is
a relocation against the same symbol retail relocates against, and that
does match. The refusal is of writing an address as a literal, not of the
function.

This is the sharpest form so far of the rule already in this file: unitcmp
is not the oracle. It answers "are these bytes the same", which is not the
same question as "does this unit match".

## The debug info, and what is still not read

`SB09WiiMASTERWAD.elf` carries DWARF for 11 compile units, and those 11 are
exactly the `Game Code` region. The type graph and `DW_AT_decl_file` are
mined already. What was not, until `dwarf_locals.py`:

* **26,602 locals and parameters with a location**, of which **22,413 name
  an exact register** and 4,087 a frame slot. That is the register
  allocator's own answer for the whole game library, and all four recorded
  near-misses are register or scheduling problems.
* **39,519 `DW_AT_decl_line`** -- the declaration ORDER of every local, and
  declaration order is a lever this file names three times.
* **7,211 lexical blocks** with PC ranges: the brace structure of each
  function, so a variable declared inside an `if` is distinguishable from
  one at the top.

A DWARF 2 location list holds offsets from the compile unit's own `low_pc`,
not addresses, and reading them as addresses gives small plausible numbers
that are wrong. `dwarf_locals.py` asserts every resolved range lies inside
the function that owns it and refuses to print if one does not: 26,602 of
26,602 pass, which is what makes the registers beside them trustworthy.

Two things it is NOT. It says what the compiler DID, not what source text
produces it -- eight spellings of `keycode.cxx`'s loop all emit the same
bytes, and the DWARF cannot say which one was written. And it covers the
game library only; the SDK and middleware have no debug info at all.

Still unread: **`.debug_line`, 990 KB**, the statement boundaries -- which
address belongs to which source line, and therefore how many statements a
function has and which ones repeat.

## Three traps worth knowing

**A survey that cannot see what is finished reports finished work as
remaining.** `gen_survey.py` said in its own docstring that it counted only
unmatched functions; it built the set to do that and then never filled it,
so it reported 164 already-generated, already-matching functions of
`WAD02_36` as available work. `gen_accessors --survey` had the same hole.
Both now take `report.json` as REQUIRED rather than optional -- with it
missing they exited zero and surveyed everything, which is the benign-looking
value again.

**Adding a `.cpp` needs `python configure.py`, not just `ninja`.** Until you
re-run it, objdiff has no `base_path` for the unit and the object you are
looking at is the TARGET, not your build. This produced a confident and
completely wrong "byte-identical" reading once.

**objdiff scoring 100% does not mean the unit links.** `zCamSplineCommonMix`
compiled to byte-identical instructions while referring to a symbol that did
not exist -- `Follower` is nested in `zCameraCurve`, and the DWARF gives leaf
names only. objdiff scores instructions; it does not check that a relocation
names something real. Always `ninja` and check `main.dol: OK`.

## What the misses have actually been

Across every unit so far, the source text has almost never been the lever:
three compiler flags, one symbol's namespace, and one case of integer
versus pointer arithmetic (`(char*)base + offset` puts the pointer in the
first operand where `x + base` does not -- mwcc normalises commutative adds
before the emitter, so operand order in the text does not survive).

When a body is already identical, stop editing C++ and look at the flags.

`zLaser` settled one open question by measurement. It is the only function
in the tree whose prologue CALLS `__save_gpr` instead of emitting `stmw`,
and it came out of the current flags unchanged on the first attempt. So
`-use_lmw_stmw on` really does PERMIT rather than force, and the choice
between `stmw`, `stw` pairs and the helper is the compiler's, made per
function. Nothing needs doing about it.

The one place the source text HAS been the lever is **where a value lives**,
and it cuts both ways:

* **Reuse a value the compiler already has.** `p += size` after
  `*(T**)p = p + size` recomputes the sum; assigning `p` from a local that
  holds it emits `mr` instead of a second `add`. Two units turned on this.
* **Hold a pointer in a local when a store could clobber the member.**
  `xOGModelHandle::~xOGModelHandle` reloads `autoptr` after writing through
  `autoptr->mData->mParent`, because that store can alias `this->autoptr`.
  Retail reads it once, so a local says what the member spelling cannot.
* **And do NOT hold one where retail re-reads.** `Containers.cpp` re-reads
  `freeList` after every store for that same aliasing reason, and hoisting it
  into a local is smaller code that does not match.
* **Declaration order picks registers.** `PoolAllocatorBase::Reset` was
  twenty words with seven differing, and all seven were r5-versus-r6:
  whichever of `p` and `last` is written first gets the loaded register.
* **A bool member tested after being stored is not the parameter.** The
  member load truncates (`clrlwi.`); testing the parameter does not
  (`cmpwi`). That was the last word of `FixedAllocator::Create`.
* **Writing a derived value back into the PARAMETER is not the same as
  putting it in a local.** `Blobloid::RemDomainRef` opens with
  `nor r4, r4, r4` -- the complement lands in the register the argument
  arrived in. Four spellings with a local, of both widths, all stopped
  four words short; `domRefMask = ~domRefMask;` closed it.
* **Which block comes LAST is a source decision.** `DoOverWrite` puts its
  `return false` after the computation, so the test has to be written the
  other way round from the obvious one. Same instructions, different
  order, six words apart.

Two things that are NOT levers, measured rather than assumed: which
overload of a name gets picked (CodeWarrior mangles static and non-static
members identically -- read the registers instead, see Blobloids.cpp), and
operand order in a commutative expression.
