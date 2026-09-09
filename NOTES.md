# Truth or Square (Wii) -- working notes

What has been established, what is still open, and the reasons. Figures are
from `ninja` and `build/R8IE78/report.json`; re-run rather than trusting the
numbers here, which move.

## State at time of writing

```
Game Code:  67 of 777 files complete  368,912 / 2,116,616 bytes  3,129 / 10,697 fn
            17.4293% of game code

Of those 3,129 functions, 856 are GENERATED -- machine-recognised
shapes, not one of which is decompiling. They are real matched
functions and the offsets and constants are recovered fact, but a
count of them is not a count of decompiled code. HAND-WRITTEN IS
2,273, across 265 units and 334,612 bytes, and that is the figure to
compare against earlier ones.

Data:       4 unit(s) carry their own, 412 bytes; 134 more could
All:        7.28% matched              main.dol reproduces byte for byte
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

## The compiler is the one the game was built with

Three independent lines of evidence, gathered 2026-09-02 when the question
was asked, and none of them a recollection.

**The disc says so.** `orig/R8IE78/files/SB09WiiMASTERWAD.elf`, the
unstripped debug link shipped in the game's files, carries the build
machine's include paths in its DWARF: `C:\Program Files\Freescale\CW for
Wii v1.1\PowerPC_EABI_Support\Msl\...`, beside `C:\RVL_SDK\include\...`
and the source tree under `C:\branches\SB09\main`. Its producer string
names no version, only `MW EABI PPC C-Compiler`.

**The bytes agree, with one tie.** `tools/compiler_sweep.py` rebuilds
every game unit with a source file under each Wii compiler on disk and
counts byte-identical functions with `unitcmp`'s own compare. Over 244
units and 1,167 functions:

| compiler (version, build) | exact | functions retail lacks |
|---|---|---|
| 1.0RC1, 1.0a, 1.0 (4.3 build 145 and earlier) | 1,149 | 0 |
| 0x4201_127 (4.2 build 142) | 1,145 | 0 |
| **1.1 (4.3 build 151)** | **1,151** | 0 |
| 1.3 (4.3 build 172) | 1,151 | 0 |
| 1.5, 1.6, 1.7 (builds 188 to 213) | 1,107 | 16 |

The 1.0 family loses `xSpringy` and `xMat3x3Tolocal`. The later three
stop inlining and shed 44 functions, most in `zSBPlayerActions` and
`xString`, while emitting 16 that retail does not have. Only 1.1 and
1.3 reproduce everything written so far and no unit separates them, so
the bytes alone cannot exclude 1.3; the install path on the disc does.

**The SDK was built with the release before.** Every `<< RVL_SDK - ...
release build ... >>` stamp in the retail image reads `0x4302_145`,
dated February to May 2009. Verified against the binaries, not the
archive's inventory: `mwcceppc.exe -version` in `build/compilers/Wii/1.0`
prints `Version 4.3 build 145` and preprocesses `__MWERKS__` to `0x4302`,
the two halves of the stamp; 1.1 is build 151, 1.3 build 172, 1.5 to
1.7 builds 188, 202 and 213, all `0x4302`, and the three 4.2 builds
(1.0RC1, 1.0a, 0x4201_127) preprocess to `0x4201`. So the stamp names
one binary on disk, and "Wii 1.0" is the archive's label for it. Those
are Nintendo's prebuilt libraries.
`configure.py` had been compiling the Revolution library with 1.1;
switched to 1.0 and rebuilt, the Revolution SDK category went from 680
to 690 exact functions (99,356 to 103,236 bytes) over 218 units, eight
units better (`WPAD` 14 -> 17, `scsystem`, `OSRtc`, `GXBump`, `dsp`,
`OSAudioSystem`, `scapi_prdinfo`, `Pad`) and none worse, main.dol OK.
`hbm` was already on 1.0.

The caveat any decompilation carries: nothing proves the retail DOL and
the debug link came off the same machine, only that the same studio in
the same months had v1.1 installed and that v1.1 reproduces every byte
written so far.

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
| `gen_animcb.py` | generate the 116-byte animation callbacks: every candidate verified WORD FOR WORD against one written by hand, both holder spellings read off the bytes; `--survey` for what is left |
| `gen_assetfix.py` | generate the assets' Fix(long): the bytes are READ into the five-op program they carry, not matched against a silhouette; `--validate` renders the ones that already match and looks for them in the file; `--survey` for what is left |
| `shape_census.py` | what the unmatched short functions LOOK like, as a population, by opcode signature |
| `unitcmp_pins.py` | re-measure `unitcmp_check`'s pins; refuses to lower one |
| `written_vs_generated.py` | the split, from the banner in each source file |
| `notes_state.py` | writes the State block at the top of this file; `--check` fails when it is stale |
| `next_functions.py` | what is left ranked by functions rather than bytes |
| `gen_rttid.py` | the RTTID_Fix<T> family -- 175 functions from one template; `--survey` |
| `reloc_audit.py` | which already-matched functions branch somewhere retail does not |
| `disasm.py` | read one retail function, symbols resolved; `--unit`. 100% of the splits decode |
| `compiler_sweep.py` | rebuild every unit with source under each Wii compiler and count exact functions; `--lib PREFIX` |
| `twin_census.py` | which unmatched functions are BYTE-TWINS of ones already written; `--unsolved` names every member |
| `transplant.py` | the fill sheet: every field that differs between a solved function and its twin, and source when a template fits; `--emit --out` |

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

## The unity builds, cut a second time: 164 files, 988,576 bytes

The splitter refused any file with more than one run, and
zSBPlayerActions.cpp had **62**. Every one of its 61 gaps held nothing
but functions the DWARF files under zSBPlayerActions.h -- the inline
callbacks between its transition tables, emitted out-of-line. The
tool's own header says such a function belongs to the .cpp that used
it and that a header is never a unit; it just never applied that to
placement. So 139,880 bytes sat unreachable behind a rule the tool
already stated. zBoardPlayerActions.cpp was 72 runs the same way, and
zCommonPlayerActions.cpp 26: 71 of 71 and 25 of 25 gaps header-only.

`absorb_headers` gives a header-attributed function to the .cpp whose
nearest source-file neighbours on BOTH sides are the same file. One
whose neighbours differ stays where the DWARF put it, because which
side it belongs to is not known. **1,619 of 2,534** header functions
move; .cpp files with exactly one run go from 259 to 421; **162 files
and 985,656 bytes** become single-run -- nearly half of Game Code.

Three more things had to change before `--apply` went through, each
found by its refusal:

  * A remainder chunk `<unit>_N.cpp` is a PARENT too: the second round
    cuts inside what the first round left, and zSBPlayerActions lies
    in WAD03_22.cpp, not WAD03.cpp. But only the chunks themselves,
    directly under SB/GM/Engine and SB/NG/Engine -- `Core/Wii/Env/
    WAD00.cpp` is a recovered unit NAMED after its blob, has no
    remainder, and tripped the orphaned-data guard.
  * A chunk consumed entirely by recovered files keeps its data
    sections as a unit with no `.text` line, or goes away if it owns
    none. The guard used to refuse instead; WAD02_18.cpp is such a
    chunk.
  * New chunk names skip every name already in use. `WAD00_1.cpp` from
    the first round already existed, and dtk read the duplicate as a
    cycle.

Applied: **1,778 units, 777 of them game**, from 1,507 and 498.
`main.dol: OK`, 0 failures in 179 build lines, and after
`gen_units.py` re-derived the generated units for the new layout the
matched figures came back to the byte: 24,568 and 993. Nothing was
lost and nothing was gained yet -- every new unit starts NonMatching.
What changed is reach: `zSBPlayerActions.cpp` (139,880 bytes) and
`zCommonPlayerActions.cpp` (25,652) are units a file can be written
for. zBoardPlayerActions.cpp is NOT: two runs survive absorption, and
a file with two runs still needs two units of one name. That is the
next rule to relax, with a suffix, and it is 100,252 bytes.

What the checks said afterwards, and what each one cost:

  * **Seven pins were lowered by hand**, which `unitcmp_pins` refuses
    to do and is right to refuse. The seven remainder chunks lost 236
    functions to 86 new units -- WAD00 alone went 183 to 6 -- and the
    build-wide count stayed at 993, which is the evidence that nothing
    was lost. The reason is written beside them in `unitcmp_check.py`,
    which is the only place a pin is ever lowered.
  * **The branch-target guard reported itself DEAD**, correctly: it
    sampled WAD00, whose 183 tail-call forwarders were the whole point,
    and WAD00 now holds six functions with no relocated branch at all.
    It samples `WAD00_32` now, where 200 of the forwarders landed.
  * **Two `unitcmp` consumers at once produce phantom relocations.** A
    check run while the audit was still compiling into the same cache
    aborted on `R_PPC_120 at 0x3be00000` -- a type that exists nowhere
    and an offset that is an instruction word. Run alone it never
    recurred, in 981 dtk objects and 203 of our own. Until the cache
    is locked, run `unitcmp_pins`, `unitcmp_check` and `reloc_audit`
    one after another, never side by side.
  * `gen_poolprefix` counted seven unrelated constants from other units
    as pool referrers, three of them mid-string; a reference is now an
    offset added to a register holding the pool's EXACT base. Neither
    prefix moved.

## Writing units is routine now, and the disassembler is why

Nothing here disassembled anything. `unitcmp -v` falls back to hex
words when no PowerPC disassembler is importable and none is, so
reading a 130-byte function meant decoding it by hand -- which is the
part of writing a unit that should never be done by hand.

`tools/disasm.py` resolves branch targets to names, folds `lis`/`addi`
pairs into the address they build and looks that up, and refuses:
an encoding it does not know prints as `.word` and is COUNTED, with
the count stated. It decodes **1,664,209 of 1,664,209** instructions
in the splits -- 100% -- and getting there found three things:

  * a placeholder written as `150 + 1: None` in the opcode table
    silently replaced `stwx`, and 2,615 instructions stopped decoding.
    A sweep over the whole image is what said so.
  * `psq_lx` and `psq_stx` are extended opcodes 6 and 7, read off the
    image rather than a manual: 7 appears in prologues beside `stfd
    f31,N(r1)` and 6 in epilogues beside `lfd`, which is the
    two-halves save of a paired-single register.
  * a label branched to twice was RENUMBERED on the second branch, so
    two addresses printed the same name.

SEVEN UNITS WERE WRITTEN WITH IT in one sitting, five of them
byte-identical on the first compile: `zCombatAttack`, `zSoundReverb`,
`zHitParameters`, `xScene`, `zLaserScanner`, `RTTID` and
`zPlayerAction` -- the last being 25 functions of which 24 match.
Hand-written went from 92 functions to 123 and from 5,628 bytes to
8,404 -- the State block above owns those figures, not this line.

Five compiler facts fell out, each costing one round:

| what | the tell |
|---|---|
| every byte of a fourcc is masked, even the one the store truncates | `(v>>16)` alone gives a 16-bit mask, `& 0xFF` gives the 8-bit one retail has |
| a loop counter compared `> 0` is UNSIGNED | signed gives `ble`, unsigned gives the `beq` after `addic.` |
| a global read twice is reloaded | retail keeps it in r31 across a call, so the original read it into a variable |
| the fall-through branch is the one NOT written as the early return | `if (p) return f(p);` tail-calls, `if (!p) return -1;` does not |
| placement new null-checks its pointer | `addic. ; beq` that retail does not have, and no spelling suppresses it |

## A matched unit is not a linked one, and the gap is DATA

Six units came out fully byte-identical and `complete` did not move,
because a unit is only linked when `configure.py` marks it `Matching`
-- until then dtk links the carved object and ours is only compared.
Flipping all six at once broke the link outright:

    undefined: 'sxAnimTempTranPool'
      Referenced from 'xSceneInit(xScene*)' in xScene.o

which is the data tier arriving from the other direction. A unit that
is linked has to SUPPLY its own statics, not just reference them.

Three of the six went through and main.dol stayed byte-identical:
`zCombatAttack`, `zHitParameters` and `RTTID` -- **complete units 25
-> 28**, and the first hand-written units in the project to be linked
rather than merely matched. The three that did not:

  * `xScene` and `zSoundReverb` reference file-scope statics
    (`sxAnimTempTranPool`, `sxAnimTempStatePool`, `reverbMgrInstance`)
    that nothing in our object defines -- `dwarf_data_carve.py` is
    exactly the tool for that, and it says 75 units could take theirs.
  * `zLaserScanner` LINKS but changes main.dol. It is the only one of
    the three with a float literal, so its `.rodata` pool is landing
    somewhere retail's does not.

So the route to `complete` is: write the unit, give it its data with
`dwarf_data_carve.py`, then flip it in `configure.py` -- and read
`main.dol: OK` afterwards, because the link is the only thing that
can tell you the placement was right.

## FixWmlType: 11,944 bytes to the byte, and TWO words left

`FixWmlType__4SextFliPv` is **11,944 bytes in one function** -- 0.56%
of Game Code by itself. It is a `switch` on a type hash that mwcc
compiled to a binary search, it pays all of that or nothing, and half
of it is dispatch over 307 case values. It is generated by
`tools/gen_wmltypes.py`, which reads the case table out of the image.

**The tree depends only on the case SET.** That was written here,
then retracted, and the retraction was wrong. Sorted order,
body-address order, reverse order and grouped labels all give an
identical comparison sequence; so does every uniform body length from
4 to 30 instructions -- seven probes, same root `FE17E3AC`, which is
retail's root. Nothing about the original's source order or its body
sizes has to be recovered.

**What actually collapsed the agreement to 63 of 307 was an empty
case body.** mwcc DELETES a case that runs nothing, because it cannot
be told apart from the default. Three of these cases run nothing;
written `case X: break;` all three vanish from the search, the value
set drops from 307 to 304, the median moves and every pivot at or
below it moves with it. Written `case X: return;` the emitted code is
the same lone branch to the epilogue and the case survives:
63 of 307 becomes 303 of 307, in one line.

**The last four were one wrong case value.** Resolving a compared
value by walking BACKWARDS through the instruction stream finds
whichever `lis` is textually nearest, and mwcc hoists a `lis` into
r6/r7 and shares it across a subtree -- so at `80049F20` the backwards
walk finds `lis r6,0xFA88` from the far side of a `bge`, which never
executes on the path that reaches the `addi`. The value is FB510572,
not FA880572. One wrong value in 307 moved the median of a nine-case
subtree and put four comparisons in a different order. Resolving
along the CONTROL-FLOW PATH gives **307 of 307**, and the two search
trees are now the same shape node for node.

**Then the bodies, where opcode 18 is `bl` as well as `b`.** The body
extractor ended a body at the first opcode-18 instruction, so every
body making more than one call was truncated at its first call. That
was 109 of the function's 2,986 instructions. A body runs to the
start of the NEXT body -- which the case table already gives -- and
fixing it also fixed the register allocation, because a body that
still needs `p` after a call is exactly what makes mwcc keep p in a
callee-saved register. 270 differing runs became 9.

Reading the bodies whole also corrected two written from the
truncated dumps: `8004ADB0`'s three conditionals are SEQUENTIAL, not
nested -- each `beq` lands on the next test, never on the epilogue --
and `8004ABB8` has a second recursive call that was never visible.

**Where it stands.** 307 of 307 comparisons in order; the function
is **11,944 bytes, retail's size to the byte**, and **2,984 of
retail's 2,986 instructions align**. The unit's other function,
`RTTID_Fix<Sext::DTRMovieSettings>`, is byte-identical (16 bytes):
retail's instantiation is `lwz, add, stw, blr`, so that class gets a
defined inline `Fix` rather than a stub declaration.

**Three things got it from 1,799 differing words to 2**, and two of
them were recorded here as ruled out. They were not: each
measurement was right and the conclusion drawn from it was wrong,
which is the thing this project keeps re-proving.

  * **The re-load is a volatile read in the test.** Retail loads a
    field, tests it, branches, and LOADS IT AGAIN before adding to
    it -- and in the body at `8004ADB0` it also HOISTS the test's
    load above the three stores in front of it. A plain read is one
    load, CSE'd, which is neither. A volatile read is both: it may
    move across plain stores and is never folded into one. Five
    tests spelled `*(long volatile*)` took 1,799 differing words to
    826. This was measured before and set aside as "not plausible
    source for a pointer-fixup routine" -- but plausibility is not
    the test, and the same lever is already load-bearing in
    zUIModel.cpp, where a member is read through a volatile view so
    the test's load is not folded into the four uses after it.

  * **The two orphaned branches are two empty cases IN THE RIGHT
    PLACE.** Inserting one anywhere changes nothing, which is what
    was measured and recorded. mwcc lays a switch's bodies out in
    SOURCE order, and the generator sorts them by body address, so
    the three cases that run nothing -- which have no body address --
    all land at the end and emit one block between them. Retail's
    two orphans sit at `8004BCA8` and `8004BCFC`, immediately after
    the bodies of case 1157347722 and case -1755207715; written
    there, each emits its own `b epilogue`, branch-to-branch folding
    orphans it, and the function reaches 11,944 bytes exactly.
    `EMPTY_AT` in gen_wmltypes.py is that map. Which of the three
    values goes where the bytes cannot say -- the tree is built from
    the values and is the same either way -- and only the PLACE is
    being fixed.

  * **Two of the three walking loops declare `end` before `e`.**
    That is what puts the cursor in r28 and the end in r29 in the
    0x18 loop, and the cursor in r29 in the 0x1C loop, and with it
    the four-register `stmw r28,16(r1)` retail's prologue has. The
    two loops are ONE allocation: fixing either alone makes the
    other worse (9 -> 11 or 9 -> 18), and only both together give 2.
    The 2x2 was measured, not reasoned about.

**The two words that are left** are one register:

    2044  ours 7f9d0214  add r28,r29,r0     retail 7ffd0214  add r31,r29,r0
    2059  ours 7c1de040  cmplw r29,r28      retail 7c1df840  cmplw r29,r31

The 0x1C loop's `end` wants r31 -- p's own register, free by then --
and takes r28, because the 0x18 loop put r28 in the function's pool
and mwcc prefers it to reusing r31. r28 appears nowhere else in the
11,944 bytes, so that one loop is the whole reason for the
four-register prologue.

**Tried for those two words and no better** (each compiled and the
whole function counted): the end declared first, both declared up
front in either order, the end computed from the field rather than
from `e`, the count in its own local, the slot in its own local, the
store-back moved after the end, the end typed `long*` or `void*`,
the end reusing `a` or `aend` from the block's first loop, the end
assigned to `p` itself, a `char*` view of `p` reassigned to the end,
the loop as a guarded do-while (1,728 words, much worse), a typed
cursor (does not compile), and the third loop at `8004ABEC` given
the same treatment in all eight combinations.
## CreateAnimTable MATCHES, and the game's strings are POOLED per unity unit

`CreateAnimTable__Q213zNPCUPGeneric4TypeFP10xAnimTable` -- **4,388
bytes, byte-identical**, 1,097 words with 186 masked by relocation.
Sixty consecutive calls to sixteen-parameter `xAnimTableNewState`;
the call table came out of the image with 0 of 60 calls carrying an
unresolved argument. It took two facts about how the game was built,
both new to this file and both measured:

**1. String constants were pooled.** A function reaches a string as
`addi r4,r29,K` off ONE base register, with K baked into the
instruction -- and the image holds **208 `@stringBase0` objects**,
`scope:local`, one per translation unit. Plain `-str reuse` never
emits that shape; `-str reuse,pool,readonly` does, and it is what the
MSL, TRK, Havok and runtime libraries here were already built with.
It also fixed the register order that stalled this function: with
pooling the zero constant is created before the string base, r28 then
r29, as retail has them. Recompiling every unit with source under
both settings gives **993 byte-identical functions either way, 0
verdicts changed**, and main.dol stayed OK with all 28 linked units.
`cflags_game` carries it now, and `unitcmp`'s drift guard knows.

**2. The pool is filled in order of first appearance across the
whole UNITY unit, and dtk's per-file units are fragments of it.**
zNPCUPGeneric's `IDLE` is at +2982 in a 5,231-byte pool because 270
strings from thirty earlier files precede it -- `BalloonB` from
zFountain first -- and `IDLE`, `WALK`, `RUN`, `JUMP` were first used by
an earlier file and are merely REUSED here. Compiled alone, the unit
starts an empty pool and every K comes out small and wrong. So the
translation unit is the thing that owns a string offset, and the blob
names do not bound it: dtk's WAD02 text range holds TWO pools, because
a unit whose first file was fully recovered has no remainder chunk at
its head to carry the WADnn name. **A unit's TU is the pool its own
functions build**, and every function that builds the 0x8068BE28 base
-- 96 of 96 -- lies inside the range those referrers span.

`tools/gen_poolprefix.py <unit>` reads the unit's pool, finds the
earliest string the unit is first to reference, and writes
`<unit>.pool.h`: a file-scope table of every pool string before it,
in pool order. Included FIRST in the unit, those strings enter our
pool in the same order, `reuse` folds the unit's references onto
them, and the offsets come out as retail has them. **The table is
data read from the image, not source** -- retail has no such table,
it has the files in front -- so it lets a fragment be COMPARED byte
for byte; a fragment that is to be LINKED needs the unity unit rebuilt
instead, and that is a different, larger job.

**This opens the AnimTable family.** 395 symbols in the image share
CreateAnimTable's shape -- straight-line runs of constant-argument
calls whose only variable parts are a string, one or two flag words,
and which pool the string lives in. The largest is 6,580 bytes.

What was ruled out before the mechanism was found, so it is not
re-tried: three call-site shapes including default arguments and an
inlined wrapper, five spellings of the trailing zeros, a hoisted zero
local, the names as static arrays, thirteen optimisation settings, all
nine Wii compilers, and the definitions in retail's order -- which the
file is in now regardless, CreateAnimTable first, because a linked
unit will need it.

**`zNPCUPGeneric`'s other two.** `Activate` (536 bytes) went from 110
to 123 of 134 words and stays a near-miss. Four things fell to the
listing: the first flag insert takes bit 0 of the template flags
moved UP four (`rlwimi r4,r0,4,27,27`), where ours shifted down --
a value bug, not a spelling; each insert is one `rlwimi` with no
separate mask, which is a one-bit bitfield copied into a one-bit
bitfield on a local copy of the byte (`zNPCStateBits`,
`zNPCTemplateFlagBits`), where a shift-and-or is a word each; the
five limits and the flag are read into f27-f31 and r30 BEFORE
`InitBoneTracker` and kept across it, which is locals declared
before that call; and `modelInfo` is read once into r30 for both
model blocks, a local. The eleven left are the two model blocks:
retail loads the call's arguments before storing the scale and ours
stores first; a real `xVec3` with an inline constructor is not
inlined at all (a `bl`, like every helper with a local in it), so
the field form with its cast stays. `SystemEvent` (200) does now: retail reads
the event's uid into callee-saved registers BEFORE calling
`World::GetEntityManager()` and keeps it across the call, and no
spelling of the call expression does that -- a local
`unsigned long long uid = *(unsigned long long*)event;` declared
inside the fallback branch, before the manager call, does. Seven of
the eight functions the unit's object defines match; the eighth is
Activate.

**The units pinned below their totals, read one function at a time.**
`xString` went 5 to 7 of 7. `xStrHash(str, len)` tests the length
before the character and keeps the raw byte, re-extending it in the
body: `while (i < len && *str != 0) { char c = *str; ... }`, where a
`for (;;)` with two breaks is top-tested and a `while (i < len)`
alone becomes a counted loop. `xStricmp` is a rotated loop whose
condition holds the loads and the compares, s1's side on the left
(mwcc evaluates the right operand first), with NO character
variables -- `xToUpper(*s1) == xToUpper(*s2)` with the case helpers
as MACROS, so each use re-reads the folded load and the byte is
extended only where a use needs it; an `inline` function parameter
extends it once at the call and every later use is a word off. The
tail is a result variable: 0, then 1, then -1 under `u1 < u2`, which
hoists `li r3,0` before the `beqlr` and pushes the range-check
bools into r4. A helper with a local variable in it is NOT inlined
at all (`bl`), inline or plain static -- measured twice.

`zBTNodeReference` stays 4 of 7, and the three are recorded so they
are not redone. `SelfDone` and `ChildDone` (2 and 1 words short)
test the done states as a BIT TEST -- `addi r6,r4,-1; cmplwi r6,4;
bgt; li r0,1; slw r0,r0,r6; andi. r0,r0,0x13` -- with the predicate's
result materialised in r5 and tested after; ours compiles the same
three-case switch to compares. Tried: five explicit cases, the
inverted sense (`!IsRunningState`), a flag variable (un-inlines),
a plain static (auto-inlined, still compares), and -O4,p on the
unit. `CreateTask` (3 words) lacks the null check retail makes on
the allocator's result before the constructor call -- `bne; li r3,0;
b` -- and no allocation form emits it: the in-class operator new,
the same with `throw()`, a global inline replacement, a placement
form on a factory reference, placement on AllocMem's result, and
`new (std::nothrow)`. Read whole, ours HAS the constructor-skip
check; retail has that and, before it, a second test on the same
compare that forces the result to zero -- the shape of an inlined
allocator with its own null test -- but an operator new written
that way makes mwcc drop the allocation call altogether (17 words).
zPlayerAction.cpp records the opposite problem, a check that cannot
be suppressed; the two are one rule read from neither side yet. `InitTypeParameters` needs
`Memory::Creator<N,T,U>` with a function-local static, whose guard
symbol has to come out as `@GUARD@...@_inst`, and `Initialize` calls
`World::EntityManager::FindAsset` through a loaded address rather than
a direct branch, which no plain call reproduces.
## The AnimTable vein: 21 tables of zSBPlayerActions match

`zSBPlayerActions.cpp` became a unit in the second cut, 547 functions
and 136,272 bytes, 173 of them branchless tables. The accessor
generator had already written it -- the weak `an...Check` callbacks
the tables name are the animcb shape -- so the tables are MERGED into
that file and it is hand-owned now; `gen_units.py` holds it and
reports, and a new accessor candidate is merged by hand.

`tools/gen_animtables.py` merges tables from the image; 31 were
merged, 400 calls, every argument resolved, and **21 match** -- 48 of
the unit's 61 written functions, and Game Code 1.39% to 1.71%. The
first three were `AddInternalTransitions` of `zSBPlayerBungeeBall`
(1,748 bytes), `zSBPlayerHammerPowerupAttack` (1,532) and
`zSBPlayerPuckPowerupAttack` (1,528); the other eighteen followed in
one batch once four things were true, each found by one diff:

  * **The tables are `void`.** One instruction over in all three, and
    it was the `li r3,0` of a `return 0;` the emitter wrote. The
    mangled name does not carry a return type; the bytes do.
  * **A callback passed to `xAnimTableNewTransition` returns
    `unsigned int`**, because the parameter is `PF..._Ui` and a `bool`
    function pointer does not convert. The generated forwarders were
    retyped, declaration and definition, scoped to their class --
    `anFirePuckCB` exists in two classes, and a file-wide replace
    retyped the wrong one -- and all 27 still match.
  * **A string pointer that NAMES a symbol is that symbol.**
    `@STRING@GetIdleString__13zPlayerIdleSBFv` in `.data` is the
    literal an inlined `GetIdleString()` returns, so the table passes
    `GetIdleString()`; spelling the bytes as a literal would add a
    copy to the pool and move every offset after it.
  * **The pool header has to carry the WHOLE pool** for a unit written
    a function at a time. A prefix that stops at the unit's first new
    string leaves the unit's own strings to fall in OUR order -- merge
    order, with the unwritten functions between them missing -- and
    every offset after the first disagreement is wrong. 48 words per
    table, all immediates. `gen_poolprefix.py --whole` writes all 569;
    reuse then folds every reference onto a string already at its
    retail offset, whatever order the functions are written in.

The extractor also lost `f31` at every call until it stopped wiping
the callee-saved FPRs -- `fmr f3,f31` is how a table passes a float it
keeps -- and the DWARF names that float: `AgingIdleBlendTime`, a local
declared at line 999 of the original, mid-table.

**What does not match**, and it is three different things. First,
four `AddActionTransitions` -- Cheat, Springboard, Jump, DoubleJump --
came out at half their retail length: they call only
xAnimTableNewTransition but carry other statements between the
calls, so "calls only the table function" is not "is a table".
They were removed rather than left half-written. Two more are one
word short (`zSBPlayerHammerAttack` and `zPlayerWalkSB`
AddInternalTransitions) and not yet looked at. Then the three below:

  * `zPlayerIdleSB::AddStates` (3,308) is not a pure table. It fills
    `extraIdleTable[k]`: `numVariants`, `noRepeats`, and each
    `variants[j] = xAnimTableNewState(...)`, in the order 0, 2, 4, 1,
    3, 5, with `extraIdleTable[1].variants[0] =
    extraIdleTable[0].variants[0]`. Every store is decoded against the
    DWARF layout; the body has to carry the assignments.
  * `zPlayerIdleSB::AddInternalTransitions` (6,580) is 3 words short
    with the local in place. Retail passes 999 twice and never hoists
    it -- `li r0,999` both times -- while ours hoists it into r18 and
    shares a base for the floats instead; all eighteen callee-saved
    registers are in use on both sides, so this is the CreateAnimTable
    ordering question again, one register deep.
  * `zPlayerRunSB::AddInternalTransitions` (2,928) and every
    `AddStates` -- Run, Hit, Defeated -- are exact or near in length
    and permuted throughout. RunSB::AddStates says it plainly: the
    same three hoisted values, the pool base, the callback base and
    the zero, take r27, r28, r29 in retail in that order and r29,
    r27, r28 in ours. Ruled out, none moving a word: the zeros typed
    as their parameter types, as NULL, and the long long alone; the
    function first in the file and last; the callback DEFINED in the
    file rather than declared; a pool string used by an earlier
    function. And retail itself orders the same trio differently in
    the Hammer table, which matches -- there the zero comes before
    the pool. Whatever decides it is not in the source text that has
    been varied, and NewState tables, whose first stack slot is a
    callback, are the ones it bites.

The two `AddActionTransitions` of zPlayerIdleSB and zPlayerWalkSB are
not tables at all: 27 and 32 `bctrl` -- the manager's virtual
`AddStandardTransitions` family -- and no direct call.

**WAD01_28, the board player's tables: 18 of 23 match**, 51 of the
unit's 57 written functions, Game Code 1.71% to 2.19%. The merger
needed one more rule to get there -- a generated stub can carry a base
clause, `class zBoardPlayerHammerPowerupAttack : public zPlayerWalk {`,
and a search for `class X {` misses it, appends a second stub and
retypes a definition whose declaration it never found. The five that
do not match are each ONE WORD short, and it is one mechanism: ours
loads both float constants off a single base register, retail gives
each its own `lis`. The same string settings give the same 142 words
in all three, so it is not the flag; it is where the literals land in
our object relative to the pool. `GetRigidBodyHeight` in that unit
was already the one accessor not matching before any of this -- its
member reads at +0xAA8 against retail's +0xAA4 with no include at
all -- and the pin, 33 of 34, already said so.

**zCommonPlayerActions: two of its eight tables pass a callback the
image cannot name.** The transition Run# -> RunFastStart01 passes
`0x8001BED0`, an eight-byte weak `return 0` that dtk names
`World::ShaderCodeBlobAsset::Create` -- the linker folded every
identical weak body onto one, and the DWARF holds only that one
subprogram at the address. So the callback's real name is gone from
the image, the merger refuses (any correctly typed function would
match the bytes, since the reference is relocated, and would be a
lie for the link), and those two tables -- zPlayerRun and
zPlayerFall -- wait for a source that names it.

The other six went in: **five match** -- CustomAnim, Idle,
FallToDeath, Dash, Land -- and Ledge is one word short in the same
way the five in WAD01_28 are. 25 of the unit's 26 written functions.

**The one-word misses, read to the bottom.** Ours addresses the float
literals SECTION-RELATIVE -- `lfs f1,1580(r26)` with r26 the .rodata
section base, `lfs f3,1584(r26)` off the same register -- where retail
gives each literal its own `lis`. Retail saves eight callee-saved
registers in WalkBoard and ours seven: ours spent one register fewer
by sharing the base, retail spent the register. The tables that match
have two distinct floats; the six that miss have three or more. It is
not the pool table's section -- moving it to .data changed nothing in
any of five units -- and not the strings flag. It is the allocator
deciding to fold under pressure, the same family as the r27/r28/r29
permutation, recorded and left -- until the three helpers and the
padding, after which both idle units went in whole: zPlanktonPlayer
6 of 6 and zShootingPlayer 2 of 2 (a unit that had no source at
all). The two `AddStates` tables took the NewState helper; Plankton's
also stores its 10th and 11th states into `talkState[0..1]` (DWARF).
The two `AddInternalTransitions` were 20 bytes short with every word
wrong for the reason IdleSB's was: retail keeps one float in f31
across every call, and `dwarf_locals.py` names it `FAST_BLEND_TIME`,
a local on the function's second line; declared with the image's
value (0.06666667f, @255783) and passed where the calls pass it,
both are exact. The 124-byte `AddTransitionsFrom` beside each is
the action helper with its `c == 0` test kept, since its second
callback is a parameter there, and was written from the listing.

**The `bctrl` tables: 36 of the 38 in zSBPlayerActions match, and the
bytes named two inlined helpers.** The shape is `lwz r3,0(this)`,
`lwz r3,0(r3)`, `lwz r3,4k(r3)`, vptr at +12, slot 1, 2 or 3: the
manager's action array and zPlayerAction's three transition-adding
virtuals. The merger now keeps a symbolic `this`, follows loads from
it, and spells the object of each virtual call from what it read;
a chain it cannot spell stops the merge. Written as the direct call,
`manager->actions[k]->AddTransitions(...)`, a table is a register
pair the other way round at its first call -- the pool-string temp
is created before the load chain, retail after it -- and the direct
`xAnimTableNewTransition` whose third callback is `ActionChange` puts
the hoisted zero before the pool base where retail has the pool
first. Both are one thing: zPlayerAction.cpp defines
`AddActionTransition` (`if (c == 0) c = ActionChange;` then the call)
and `zPlayerActionManager::AddTransitionsTo` with its two siblings
(`actions[id]->...(...)`), retail's unity build held that file in
the same translation unit as the tables (WAD03: 80105550..801894A0
holds both), and -O4's auto-inliner took them. Spelled through the
helpers, defined `inline` in the unit so the fragment inlines them
the same way and emits no copy: zPlayerCheatSB 21 of 150 words to 0,
zPlayerRunSB 5 of 299 to 0, zSBPlayerPuckAttack 9 of 164 to 0, and
the SingleCustomAnim near-miss with them. A class that makes such a
call derives from the unit's zPlayerAction stub (three members, then
the virtuals, so the vptr lands at +12) and its leading padding
shrinks by the base; the accessor generator had read
zPlayerLandHighSB::End's load as `this+4`, which is the base's
`player`. A free function with a mangled signature is spelled by the
ABI from the walk's snapshot, and every `bl` is recorded so an
unspellable callee refuses the merge rather than vanishing.

**`GetRigidBodyHeight`, the one accessor of WAD01_28 that never
matched, was the generator's padding rule, and the rule was wrong for
every stub whose virtuals come from vtable calls.** `gen_accessors.py`
puts a class's first data member four bytes in only when a
CONSTRUCTOR stores the vtable; a stub that declares virtuals because
its methods call through slots (zBoardPlayer, 162 of them) gets the
same vptr at 0 from the compiler and got no allowance, so `fAA4`
sat at 0xAA8 and retail reads 0xAA4 (`halfExtents.y`). One line: the
vptr counts when the stub declares any virtual. Regenerating every
generated unit changed two -- WAD01_28 to 57 of 57 and
zFloatingCollectible to 2 of 2 -- and nothing else.

**The NewState permutation was the third inlined helper, and the four
`AddStates` tables match.** zPlayerAction.cpp defines the member
`NewState(table, name, a..g, h..k, l)` as `xAnimTableNewState(...,
this, h, i, j, k, 0, l)`, and retail's tables pass `this` in r10 the
way the helper does. Spelled direct, zPlayerHitSB::AddStates had 250
of 385 words wrong -- the hoisted callback, pool base, zero and two
float bases in another order -- and through the helper it had 11, all
of them member stores around the calls. `store_seq` reading off the
image gave each table's statements in order: HitSB fills
`variants[0..1]` after `noRepeats = false; numVariants = 2;` (the
two scalars in THAT order -- swapped, two words), DefeatedSB fills
four variant tables with their counts, IdleSB opened with
`extraIdleTable[0].noRepeats = false; numVariants = 1;` that the
hand-written body lacked. The members came from `dwarf_types.py`
(zPlayerHitSB 0x60 bytes, zPlayerDefeatedSB 0x6C). RunSB matched on
the respelling alone. zSBPlayerActions 89 to 93 of 95; the merger
spells a direct NewState with `this` as owner and a zero `m` through
the helper, and refuses when the unit lacks the inline definition.

**The one-word float-base misses are one compiler rule, measured to
the bottom, and ten of them fell.** mwcc addresses a function's float
literals by a fixed cost: under 32 KB into `.rodata` it shares one
base for three or more literals (`lis` once, section-relative offsets
baked in, the relocation against the section symbol), past 32 KB it
gives each literal its own `lis` up to three and forms an `addis`
base for four or more. Retail never shares under three -- seven
consecutive four-byte literals in xFontPrintTopText each get a `lis`
-- because a game file's literals sit tens of KB into its unity
unit's `.rodata`; a fragment compiled alone puts them at 12 KB and
shares. What was ruled out, each by a probe that compiled and read
the object: every Wii mwcc on disk (nine, identical), -O levels and
-inline, -pooldata (one word the other way, and it unpools the
strings), -sdata2, -sym and -g, prior emission of the literals by
earlier functions in every arrangement, the literal pool's size to
24,000 entries, and the internal object numbering to 270,000. What
reproduces retail is the measured distance itself: gen_poolprefix.py
now scans the TU for the lowest `.rodata` address its code forms and
the unit's first float literal, and emits that many bytes as an
unreferenced const array ahead of the pool -- 64,980 for
zSBPlayerActions, 41,320 for WAD01_28 and zCommonPlayerActions,
50,872 for zNPCUPGeneric. With it: zPlayerFallSB, RunSB's and
HammerAttack's and WalkSB's internal tables (zSBPlayerActions 85 to
89 of 95), WAD01_28's five one-word misses (51 to 56 of 57), and
zCommonPlayerActions' Ledge (25 to 26 of 26). The one that stays is
the four-literal `zPlayerWalkSB::AddActionTransitions`: retail spells
four `lis`, the rule forms a base for four, and no setting tried
moves that line.

**Measured again from the other side (2026-09-02): retail's game units
never share a literal base at all.** A scan of every sized function in
the game's text (10,780 of them, 80006000..80230000) for one `lis`
that feeds two or more float-literal loads found 44 that share, every
one in the engine's WAD01 unity unit (801C28C0..801E3DA0) or in Havok,
against 1,521 that load two or more literals through a `lis` each.
The debug link kept its relocation tables, and they say why the bytes
differ: the engine's shared loads relocate against the section label
`...rodata.0`, the game's every load against its own `@N` literal,
and no game object defines a `...rodata.0` at all -- the first in the
image is the engine unit's, at 80692C08. Our compiler emits that
label and the shared form whenever its cost rule pays, so a game
fragment compiled alone reaches retail's shape only where the rule
would not share: three literals or fewer, past 32 KB of `.rodata`.
`WAD00_7_1`'s SetRadius measured 24 bytes ahead in the image and
retail spells three `lis`; with 32 KB ahead it matches, and 41,320,
45,000, 60,000, 70,000 and 140,000 give the same bytes, so
`gen_poolprefix.py` now floors the padding at 32 KB. The image cannot
say how far retail's literals really sat: the linker script's
FORCEACTIVE block exists because mwld drops unreferenced objects, and
the DWARF describes no variables (0 in the ten unity units) and no
dropped functions (0 of 10,062 subprograms lie outside the text), so
what the compiler had ahead of a literal is a lower bound only.

What the four-literal wall is NOT, each ruled out by a compile that
was read back: every `-opt` sub-option (nocse, nolifetimes, noprop,
noloopinvariants, nopeephole, nostrength, nointrinsics), -O2/-O3/-O4
for size and for speed in the past-32 KB regime, -inline
off/all/deferred, -ipa file, -Cpp_exceptions on, -RTTI on, -common,
-fp_contract off, -str in every combination, -sdata2 4, -once,
-func_align, -schedule, -use_lmw_stmw off, -g and -sym, CATS on,
-proc, -align, -char, -enum, -abi, -model, every GameCube mwcc on
disk (1.0 to 3.0a5.2; those before 3.0 reject `-enc`, dropped for the
probe), `#pragma pool_data off` (the `@floatBase0` pool, one word the
other way, as the flag), `#pragma section` with far_abs, near_abs and
renamed constant, data and code sections, a string pool of 1, 3, 8
and 40 KB, 2,000 literals ahead, 3,200 functions ahead, 80 KB of
text, 70 KB of `.data` and of `.bss` ahead, and the literals emitted
first by an unreferenced static function -- which the compiler does
emit (48 bytes, LOCAL). The bouncer's four literals form a base at
41,320, 70,000 and 140,000 bytes ahead under every one of these.
Retail's game functions that are the first to load four or more of
their literals number 37 of the 2,170 that load any (15 load six or
more; `zBoardPlayer::Reset` 19 of 27), and every one spells a `lis`
per literal. One earlier note here was wrong and is withdrawn: the
bouncer object's by-symbol literal was not a reused one addressed
differently, it was `Setup`'s single literal; the four fresh ones
shared a base in every arrangement tried.

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

## unitcmp COMPILED EVERY RUN TO ONE OBJECT PATH

Found while fanning a batch of small units out to four parallel
agents, which CLAUDE.md recommends. `compile_unit` wrote
`build/_unitcmp.o` -- one fixed name, no lock -- and then read it
back. Two runs at once therefore share it: the second run's compile
lands between the first run's compile and its read, and the first
run reports the SECOND unit's functions as its own answer. Nothing
looks wrong when it happens. The names in that object are real
retail names, so every line reads as an ordinary verdict, and the
count at the bottom is a real count -- of the wrong unit.

The object is now named per process and removed at exit. Validated
the way CLAUDE.md asks: against six units whose answers were already
recorded -- zEventSpy 6 of 6, RenderModeEntity 7 of 7, xUIDMgr 6 of
6, zGameState 7 of 9, zDecal 4 of 6, zBTDepot 4 of 5 -- and then
three runs started at once, each of which reported its own unit.

Worth stating plainly, because the fix is small and the lesson is
not: the tool had been right for every serial run since it was
written, and the first thing that made it lie was using it the way
the instructions recommend. A shared temporary is a correctness
question the moment anything runs twice at once.

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

Run these first. They say what is true rather than what was true, and
the third one takes a while because it now covers every unit with
source rather than only the generated ones:

```bash
ninja                                  # main.dol: OK, and report.json
python tools/notes_state.py            # rewrite the State block above
python tools/unitcmp_check.py          # 206 pins, 0 failures expected
python tools/reloc_audit.py --quiet    # 0 wrong branches expected
```

**`tools/disasm.py` is how a unit gets written now.** It resolves
branch targets to names, folds `lis`/`addi` pairs into the address
they build, and prints `.word` for anything it cannot decode rather
than guessing. It reads 100% of the instructions in the splits.
`disasm.py --unit <unit>` then writing the source against it is the
whole loop; `dwarf_lines.py --unit` and `dwarf_locals.py --unit` add
the original's statement structure and register allocation when a
function resists.

Then one of these, in the order they are worth doing:

1. **Write another unit.** This is the only column that means
   decompiling and it is the one that moved most this session: 92
   hand-written functions to 128. Seven units were written in one
   sitting and five were byte-identical on the first compile. Measured
   on 2026-09-02 with the scratch `near_complete.py` over report.json:
   **110 game units are within three functions of complete** once units
   with no source at all are counted, and the smallest are one or two
   functions of 16 to 120 bytes -- `WAD04_6_1`'s `FoundPath` (16),
   `Graphics/Material`'s `RenderAttach`/`RenderDetach` (32), `WAD03_33`'s
   `AddScale` (36), `Math/Quaternion`'s `Format` (56), `WAD00_7_1`'s
   `SetRadius` (60), `WAD03_34`'s `Sub`/`dot` (64), `Math/Random`'s
   `MakeSeed` (80). A unit whose split holds no data section is complete
   the moment its functions match and are defined in retail's order
   (entry 2 says the gate); the rest are matched bytes only.

   Re-derive the list rather than trusting it: it is a query over
   report.json for game units whose unmatched functions are few,
   EXCLUDING any symbol named `pad_*`. Those are dtk's alignment
   padding, there are 95 of them, and a ranking that leaves them in
   claims 146 units are one function from complete when they are one
   PADDING WORD from it.

2. **Make a matched unit a linked one.** `complete` only moves when
   `configure.py` marks the unit `Matching`; until then dtk links the
   carved object and ours is merely compared. 28 -> 32 complete on
   2026-09-02, and the gate is now measured rather than guessed. A
   unit links as it is when THREE things hold: its object carries no
   data section at all (a float literal or a pooled string puts
   `.rodata` where the split has none -- `zLaserScanner`); every
   symbol it references is `scope:global` in symbols.txt (`xScene`
   references two file statics, `sxAnimTemp*Pool`, that the carved
   object keeps local, and the link says `undefined`); and its
   functions are DEFINED IN RETAIL'S ORDER, because the link honours
   definition order (`ScreenShot` shifted main.dol with two matched
   functions the other way round). The scratch `linkscan.py` compiles
   every fully matched NonMatching unit and reports the object's
   non-text sections and its text against the split: 153 units had
   no data in the SPLIT, two objects filled their split, and only
   `GameWindow` linked; `WAD03`'s one function matches but its
   remainder owns the unity unit's whole .rodata/.data/.bss. The
   others need their DATA: a linked unit has to SUPPLY its file-scope
   statics, which is what `dwarf_data_carve.py --survey` (134 units,
   15 with no cut needed) is for. ALWAYS read `main.dol: OK` after
   flipping one, and flip one at a time when a set fails.

   A FOURTH gate, found the same day with ten tiny units: the split
   must END where the unit's last function ends. dtk gives a chunk
   the alignment padding after its last function, our object ends
   where the function does, and the carved object that follows is
   placed right behind it -- the DOL diff was a twelve-byte shift of
   everything after `WAD03_33`, 44,088 bytes in 233 runs. Leading
   padding is harmless (`WAD04_6_1` starts four bytes before its
   function and links, because the linker aligns our text). The
   recipe is to move the split boundary in splits.txt back to the
   last function's end, so the padding heads the NEXT unit's carved
   chunk, then configure and read main.dol: OK -- three units went
   through it at once and it held. 32 -> 39 complete.

   A FIFTH gate, from two destructors that matched and would not
   link: the unit must not be the VTABLE'S HOME. The compiler emits a
   class's vtable in the unit that defines its first non-inline
   virtual function, and a destructor unit written with the class's
   only virtual being that destructor emits `__vt` -- `multiply-
   defined ... previously defined in WAD00.o`, because retail's copy
   is in the blob. Declare an undefined virtual AHEAD of the
   destructor and the vtable goes with it; the destructor's bytes do
   not change (`WAD00_12_1`, `WAD00_16`, both exact before and after).

   And the fourth gate holds for .rodata as it does for .text.
   `LinkFastSqrt`'s split owns a 32-byte .rodata chunk: its float
   0.5 (`@20`) at +0, its double 0.5 (`@22`) at +8, and sixteen
   bytes of padding up to the neighbour's 32-aligned rodata. Our
   object emits exactly the sixteen bytes of literals, so flipped
   as-is the DOL failed. Moving the .rodata end to the double's end
   (0x8067D290) and `WAD00.cpp`'s .rodata start with it linked,
   main.dol: OK -- the first unit linked with a literal pool of its
   own, and the recipe for the float-literal units the first gate
   turned away (`zLaserScanner`, `WAD02_24`): when the split's
   .rodata holds nothing but the unit's literals, end it where they
   end. `xFMV`'s function-local static is .data and untried.

   The ten written for it, from the `near_complete.py --all` list:
   `WAD04_6_1` FoundPath, `Graphics/Material` RenderAttach and
   RenderDetach, `WAD03_33` xVec2::AddScale, `WAD03_34` Sub and dot,
   `WAD02_9` DeptLookup's copy assignment, `WAD00_5_2` RemoveOwner,
   `Math/Random` MakeSeed (STATIC: r3 is the seed, r4 the value),
   `WAD02_24` the zNPCRayHitCollector constructor (the entity
   assigned in the body, after the fraction; a float literal keeps
   it from linking), `Math/Quaternion` Format (a pooled format string,
   so a pool header and no link), and `WAD00_26`, where one of two
   `Fix`es stays 6 of 7 words -- retail re-reads the member after the
   test and four spellings fold it (the file says which).

   The next twelve from the same list, eleven exact and nine linked
   (39 -> 48 complete, five more split ends moved): three module
   constructors on the System::Module base (`Globals`, `Primitive`,
   `TestSuite`, each storing its id into the first event stages),
   `zEmbeddedStartupIcon`'s constructor and StopCallback,
   `zStoryMoment::GetInstance`, `zPlayerInput::GetViewportIndex`,
   `Memory::FreePhysicalMemory1`, and four destructors (`WAD00_11`,
   `WAD00_11_3`, `WAD00_9`, `WAD02_1_1`). Three things they taught:

   * A destructor whose bytes call `hkBaseObject`'s destructor
     DIRECTLY on a member, with the don't-delete flag, has a member
     of type hkBaseObject at that offset, whatever the DWARF names
     the type. A member class of its own -- derived from the Havok
     object, containing it, with an explicit empty inline destructor
     -- gets a destructor of its own emitted (80 bytes, a vtable
     store) and called. Measured three ways.
   * A `cmpwi r3,0` after an allocation call with nothing to
     construct is the null check of a `new` whose class has an EMPTY
     INLINE constructor: the branch folds away and the compare stays
     (`zStoryMoment`, exact once the constructor is declared).
   * The double null test -- `cmpwi; bne; li r3,0; b; beq; bl ctor`
     -- is `mem ? new (mem) T : 0` where the placement new's own
     check did not fold into the conditional's. Ours folds it: 5 of
     23 words in `zBTNodeReference::CreateTask` and 5 of 18 in
     `WAD01_5`'s `zBTFactory::Create<T>` (a template instantiation
     that emits only when defined OUT of the class). An `inline`
     helper folds too; a plain static template is not auto-inlined
     and just adds a function. Recorded, both kept in the ternary
     form, the right length and closest.

   The seven after those, all seven exact and six linked (48 -> 54
   complete, three split ends moved): two more Havok destructors
   (`WAD00_12_1`, `WAD00_16`), two `Create` statics over the global
   heap (`xSubtitlesAsset`, `zUPQuestCard`), `xResponseCurve::
   find_active_node` (`WAD00_8`), `xFMV`'s two (matched, not linked:
   its function-local static is .data) and `LinkFastSqrt`'s `sqrtf`
   and `sqrt`, hand-fused code that took seventeen probes and is the
   first linked unit to SUPPLY DATA of its own (Game Code data 116
   -> 132 bytes). What they taught:

   * The hkBaseObject rule holds for BASES too: bytes that call the
     Havok destructor on `this` with the flag clear have it as the
     DIRECT base. The DWARF's `GeometryEntity`/`BlobEntity` in
     between each get an 80-byte destructor of their own emitted and
     called (`EXTRA ... NOT IN RETAIL` in unitcmp).
   * A single `cmpwi r3,0; mr r31,r3; beq` right after `memset` is the
     placement new's OWN null test on memset's return value: `return
     new (memset(alloc, 0, n)) T(h);`. An `if (p)` around the new adds
     a second `beq`; testing the allocation instead of memset's return
     moves the `mr` a call earlier. Retail keeps the check here, unlike
     the two double-test misses above and the no-check one in the
     table -- the same construct, three different byte shapes.
   * Named locals take registers in DECLARATION ORDER. `last` declared
     before `node` gave them r6/r7 as retail has, and `stride *
     active_node` (the stride first) gave `mullw r0,r4,r0`. The same
     order rule reaches the FPRs: `register`-qualified variables land
     on f0, f2, f3, f4 in the order they are declared, f1 being the
     parameter -- and f0 is where retail keeps its 0.5, so `half` is
     declared FIRST.
   * `LinkFastSqrt` is C with `register` variables and asm statements:
     `asm { frsqrte e, x }`, `asm { fnmsubs t, t, h, half }` and
     `asm { fsel e, e, e, x }` around ordinary arithmetic. The
     intrinsics cannot spell it: `__frsqrte` and `__fsel` return double
     and the `(float)` cast is an `frsp` retail does not have,
     `__frsqrtes` does not exist in this compiler, and the C Newton
     step is forwarded into a temporary that takes the lowest free
     register (f0 once the constant dies) where retail writes it back
     into `t`'s f3. A parameter read by asm must be `register` too.
     The scratch `sq*.cpp` probes are the seventeen spellings.

   Eleven more, ten exact and five linked (54 -> 59 complete):
   `zNPCStatus` ResetToNPCAsset, `WAD01_30` xMat4x3FromTransform,
   `WAD02` xMat3x3Tolocal, `WAD00_9_1` xMath2NearestPointOnLine,
   `Main` main, `ClipEntity` Create, `zNPCBTActionBuilder` Build,
   `zNavMarker` Create and IsOn, `zModuleMgr_Registry` Startup (on a
   pool header; it matches, and its two managers are file statics so
   it does not link), and `zPlayerAction`'s BeginUpdate, which makes
   that unit 25 of 25 after weeks at 24. `zNPCType::Setup` is 11 of 19
   and its file says what was tried. What they taught:

   * A CALL BY THE MANGLED NAME reaches what no spelling does. `extern
     "C" void __ct__16zPlayerInventoryFv(void*)` calls the constructor
     with no null test -- the one word BeginUpdate was short of -- and
     `__ct__Q24Math6VectorFfff(&member, x, y, z)` is how zNPCStatus and
     xMat4x3FromTransform construct into members: this compiler rejects
     `p->T::T()` outright (error 10409) and placement new tests the
     result. The inventory constructor and zUpContextActionManager's
     both zero one bool and folded; the address BeginUpdate hands it is
     the manager's context, reached through the player. And 0x800075C0
     is a lone `blr` the image names Math::Matrix33's constructor, the
     weak empty function every empty function folded into: `main` and
     the registry startup call it by that name. The audit passes, since
     the branch reaches the symbol the image has there.
   * `#pragma always_inline on` is a lever. -inline auto takes a
     constructor with one store in its body and declines it with two;
     retail's ClipEntity has it in line, so the original forced it.
     This is the wall the flag-and-local helpers hit in the AnimTable
     section, untried there.
   * A register freed by an argument store is taken by the next value
     defined, whatever the declaration order says. zNPCType's index
     lands in r4 after the first store and its copy of the zero cannot
     rise above the store that reads r4; retail's index took r9 before
     any store. Seven spellings leave it there.
   * A SIXTH link gate, open: `Main` matches and does not link. It is a
     FRAGMENT of the NG WAD02 unity build, and `OSInitFastCast` -- the
     SDK header's static inline, instantiated once for the whole unity
     unit as a local symbol right after main, 52 bytes -- is called
     from HomeMenu's fragment too, under the name dtk gives a local
     symbol other carved objects reach, `OSInitFastCast_801FBA60`.
     Defined in Main.cpp (static, the asm body keeps it out of line as
     retail's `bl` shows) with the split end moved past it, the bytes
     matched and HomeMenu's reference went `undefined`. What is left is
     for the fragment to export the instance under that name without
     losing the name unitcmp and the report match it by.

   Twelve more, nine exact and four linked (59 -> 63 complete): the
   next by size from the near-complete list. Exact: `Renderable`
   (Create, InitLocalColorMultiplier), `zSoundMask`, `zPlayerAI-
   CommandGroup`, `WAD02_35_1` (the AVL insert of the handle tree),
   `SaveErrorMsgBox` (on a pool header), `GlobalFXEntity`, `zEnv`,
   `WAD00_12` (LangStringToLangID) and `zPlayerAISearchMapLinkCost-
   Calculator`. Three near-misses, each with its file saying what was
   tried: `Text` RenderText one word (the first allocation's size is
   computed before the allocator's address in retail, after it in
   ours, six spellings), `zNPCSearchMapLinkCostCalculator` 49 of 74
   (its sibling matched with the same constructs; what is left is the
   callee-saved order of its seven values, `this` last in retail),
   and `zBouncer` BouncePlayer 136 of 184, every remaining word the
   four-literal base and the register it costs. What they taught:

   * `const float& scale = 1.0f;` keeps a multiply by one that a local
     `float` folds away, and `x / scale` is a real division the
     compiler does not fold either. Both calculators multiply their
     cost by an f31 loaded from the pool; the reference is the
     spelling. Their three nav tests are one `||` condition (one
     refusal block, not three) and the type dispatch is a `switch`
     (the compare chain), with the group in a case-local so the entity
     stays in a volatile register; a net declared ahead of the group
     takes the higher register.
   * A temporary of an empty class is value-initialised, which zeroes
     its byte on the stack; a named default-initialised local is not
     (`Util::Referrer referrer;` in GlobalFXEntity).
   * Struct assignment calls the implicit operator= OUT OF LINE:
     `request.clip = args.clipRect` emitted `__as__Q22UI4Rect...` as an
     extra function and called it, where retail copies member-wise;
     and `dir = normal` in zBouncer IS such a call, `__as__5xVec3...`
     being the image's own out-of-line implicit operator=.
   * The compiler does not build `T x = f();` in place: it returns
     into a temporary and copies. Retail's BouncePlayer names none of
     its operator results -- the reflection is one expression and the
     velocity is assigned from the product -- and only `dir` is a copy.
   * `__attribute__((aligned(16)))` on a class's storage is honoured
     and gives the dynamic frame alignment (zEnv, zBouncer); on the
     class itself or through `__declspec` it is not.
   * A comparator that is an EMPTY BASE of the template sits at offset
     zero with the count (the AVL insert calls it with the tree's own
     `this`): the empty-base optimisation holds. A member template
     instantiated explicitly by name avoids instantiating members the
     unit does not define.
   * `TRCMsgBox`'s vptr follows its twelve members (0x34); the
     DWARF's size said so and one word said where.
   * `gen_poolprefix.py --whole` now writes a padding-only header for
     a unit that builds no string pool but loads float literals,
     finding its translation unit as the pool whose referrers span it
     (`zBouncer.pool.h`, 41,320 bytes ahead).
   * The four-literal wall again: BouncePlayer loads 2.0f, 1e-5f, 1.0f
     and 0.0f, retail with a `lis` each, ours with an `addis` base
     past the padding. Two mechanisms were tried and ruled out: the
     literals introduced by a function ahead in the unit (reused, not
     new) and introduced scattered among others (as retail's sit, 320
     bytes apart). Neither moves the base. The same wall as
     zPlayerWalkSB's and zPlayerIdleSB's.

   The four written whole to get there, all exact on the first
   compile: `WAD03`'s `NewArray<float, GlobalHeapEnum>` (a template
   instantiation, 24 bytes, the return type in the mangled name),
   `ScreenShot`'s `frameDumpGetNextFileName`, `zBTFactory`'s
   `SceneInit`/`Destroy`/`Allocate`, and the `VirtualKeyboardModule`
   constructor on the `System::Module` base GameWindow already used
   (name, four event stages, then the virtuals, vptr at 0x14).
   `near_complete.py` over report.json lists what is within N
   functions of complete, `pad_*` excluded; what is left there is the
   exhausted near-misses and `WADSpeed`'s static initialiser, which
   needs its unit's data.

   Nine more, eight exact and two linked (63 -> 65 complete), and the
   literal-base question measured to its floor. Exact: `WAD01_11`
   (`zBlackboard::Register<int>`, an explicit instantiation on a
   `zVariableBase` whose vptr follows its eleven words, a class-level
   `operator new` on the global heap), `WAD01_22` in NG
   (`FreePhysicalMemory2`; the unit's other function is the compiler's
   out-of-line `_GXRenderModeObj::operator=`, which an unused inline
   assignment does not emit -- measured), `FactoryMemTypeRegistry`
   (a guarded local static, a memset constructor), `ComboAnimBlob-
   Entity`, `zPOWManager` (a module with its vptr after five words;
   the manager is a file static in retail, so it matches and does not
   link), `zPOWObject` (`Init` non-const, or the mangled name gains a
   C), `zBase` (the RTTI parent walk as a do-while, the dispatch a
   switch) and `WAD00_7_1` (SetRadius, on the padding floor below).
   `MathUtil` 1 of 2: `DampSpring` exact, `StartupMathUtil` 14 of 35
   -- retail keeps the row in r10, the next row in r9, the count in
   r8, and three loop spellings each placed them elsewhere; the
   paired-single `ConvertOBBToAABB` is not attempted. What they
   taught:

   * A game fragment's literals reach retail's shape (a `lis` per
     literal) only past 32 KB of `.rodata`, and the distance the
     image shows is a lower bound on retail's, so `gen_poolprefix.py`
     floors its padding there: WAD00_7_1 measured 24 bytes and
     matched at 32,768. The float-base section carries the whole
     measurement -- 0 of 10,780 game functions share a base, 44
     engine and Havok ones do, and the relocations of the debug link
     say why -- with the list of everything that does not move the
     four-literal wall, which BouncePlayer and the two AnimTable
     tables still stand behind.
   * `-pooldata off` is measured, not assumed: it pools the floats
     into a `@floatBase0` and re-bases with `addi`, one word the
     other way from retail (13 of 15 on SetRadius).
   * A `T x = f();` of a struct is not built in place, and a struct
     assignment's implicit `operator=` is out of line; both again.
   * The compiler emits an unreferenced `static` function (48 bytes,
     LOCAL) and its literals; the linker would drop it. A unit that
     matches through a padding header cannot link either way: its
     literals are its own `.rodata` and the split has none.

   Two more, both exact and both linked (65 -> 67 complete), from the
   within-three list: `zBTNodeReference` 7 of 7 and `WAD01_5`
   (`zBTFactory::Create<zBTNode::RandomChildIterator>`) 1 of 1. What
   they taught, each a shape the file had recorded as exhausted:

   * The conditional's operand order places its blocks. `mem ? new
     (mem) T : 0` puts the constructor path first and branches to a
     trailing `li r3,0`; `!mem ? 0 : new (mem) T` puts the null block
     first (`bne ; li r3,0 ; b`) with the placement new's own test on
     the same compare after it. CreateTask and Create both wanted the
     second, and Create as an early return (`if (!mem) return 0;`)
     gives the same bytes.
   * The bit-mask lowering (`addi r6,r4,-1 ; cmpli r6,4 ; 1 << r6 ;
     andi. 0x13`) is not a switch: six switch spellings all gave a
     range test. It is an or-chain of equalities, and its polarity is
     read off the default: retail loads 1 before the range test and
     clears it inside the mask, so the helper is `state != 1 && state
     != 2 && state != 5` and both call sites test its negation.
   * A virtual call's slot is read off the retail vtable, not the
     order the handlers were written in: `__vt__16zBTReferenceTask`
     is Execute, Cleanup, SetObserver, Setup, SelfDone, ChildDone, and
     the parent's ChildDone goes through +28. The vtable lives in
     WAD01's data, so an undefined override (`Cleanup`) is declared
     ahead of Execute in the derived class: the home moves, the slots
     do not, and the unit links.
   * A unit whose object's `.text` is 16-aligned (mwcc's default) may
     sit eight bytes into its split: WAD01_5's split now ends where
     its function does (0x800811F8) and the eight zero bytes belong
     to zBTNodeSequence's lead, as with WAD00_8 and LinkFastSqrt.

   Five engine and game units written whole from the within-six list,
   four exact and one near, none of them linkable (each carries data of
   its own): `PowerControl` 7 of 7 (the shutdown callbacks and
   ProcessShutdown), `TRCModule` 5 of 5, `Graphics` 8 of 8 (the render
   thread and frame steps), `MediaFile` 8 of 8 (the async file
   commands), and `zBTDepot` 4 of 5 with SceneInit at 130 of 145 words,
   every one of them a register number, the file listing the orders
   tried. Game Code 5.11%, 1,221 functions. What they taught:

   * With these flags the compiler inlines NO user function of four
     stores or more -- a plain member, a template member, a static
     inline free function, with or without `#pragma always_inline` --
     only one-expression helpers and compiler-generated constructors.
     A retail body that reads as an inlined early-return helper was
     written in the caller; its exits are gotos to the continuation.
   * A file-scope flag set from a callback and polled is `volatile`:
     the two callbacks whose zero constant retail materialises one slot
     later than ours both match volatile and nothing else moves them
     (twelve spellings, the scheduler and processor flags).
   * A varargs function with its formatting compiled out is an EMPTY
     `...` body: the compiler saves the argument registers for any
     such function, and a `va_start` adds three stores and 16 bytes of
     frame retail does not have.
   * Section-relative data: the flags of PowerControl sit 0x8A70 past
     the unity unit's first .bss object and are reached through the
     section label with one `addis`, so the unit carries that distance
     as an unreferenced .bss array, the padding lever for .bss.
   * A heap enum passed by const reference to an inline NewArray binds
     the enumerator to a static temporary and loads it back: the
     anonymous zero data words every caller of the thread-stack
     operator new has, one per call site.
   * The unnamed-namespace mangling follows the MAIN file's name, and
     neither `#line` nor an `#include` moves it, so a fragment cannot
     name a unity unit's file static; the reference is a masked data
     relocation, and the unit's own definition is the honest source.
   * The retail vtable gives the slot: System::Module's virtuals are
     GetPriority (2), Startup (3) and Update (9), its vptr after the
     name and the event set at +0x14; a member whose first word lands
     at +0x8 needs a word-aligned declaration or it follows the base's
     trailing bool three bytes early.

   Six more from the same list, five exact and one near, none linkable:
   `RenderModeEntity` 6 of 6 (a pointer-to-member call through
   `__ptmf_scall`, an explicit virtual destructor call, and the weak
   DeleteArray instance the unit emits byte-identical to the image's),
   `zEventSpy` 6 of 6, `xUIDMgr` 6 of 6, `SystemCache` 5 of 5 (the Wii
   NAND memory cache), `zPIDController` 5 of 5 (both controllers, on a
   padding header the generator now writes with its 32 KB floor when no
   pool's span covers the unit), and `zNPCAnimViewer` 4 of 5 with
   Activate at 58 of 67 words, its file recording nine spellings. Game
   Code 5.25%, 1,246 functions. What they taught:

   * A derived class's first member can sit in its base's tail padding
     (zEventSpy's asset at +0x3C inside xOGEntity's 0x40); the size then
     rounds to the base's alignment, so the count carries
     `__attribute__((aligned(8)))` and the object is 80 bytes, not 76.
   * A chain of four event ids whose bodies all follow the compares is a
     `switch`; the two ids that share a body are one case pair.
   * A comparison's operand order in the bytes does not follow the
     source: `written != size` came out either way as `cmpw size,
     result` until the call's result was held in a named local, and
     `memory == cachedMemory[slot]` wanted the parameter first.
   * The fused multiply-subtract's operand order is not the source's
     either: the integral term `(integral * kI) * (1 / sum)` and the
     derivative quotient in a named local were what put kD in the
     multiplier slot; the integral must be declared before the summed
     step to take f4 before f6.
   * A second base reached through a reference (`*npcEntity`) gives the
     `stwu` that fuses the owner store with the pointer update; through
     a pointer conversion the compiler adds the null adjustment retail
     does not have.
   * A pointer to member held in a local and called (`(this->*fn)()`)
     is the 12-byte constant copied to the stack and `__ptmf_scall`;
     `this->~T()` on a polymorphic class is the first slot called with
     -1. A char-typed read cannot be hoisted past an int store, so
     where retail loads two asset bytes before storing either, the
     source read them into locals first.
   * Where a fragment's translation unit cannot be told from the image,
     `gen_poolprefix.py --whole` now emits the padding floor and says so
     in the header rather than refusing.

   Three more written whole, one exact: `zUIModel` 6 of 6, `zGameState`
   7 of 9 and `zDecal` 4 of 6, with `zEventSpy`'s event wrapper made the
   file static retail has (the report pairs a local symbol by name and
   address, so a global of the same name went uncounted). Game Code
   5.33%, 1,257 functions; 67 linked, unchanged, since each unit carries
   the unity unit's literals. What they taught:

   * A pointer to member called in a LOOP is declared inside it. Retail
     reloads the constant's three words every iteration, so hoisting the
     declaration out (which loads them once and keeps them) costs 22 of
     45 words; and reading the loop's object into its own local BEFORE
     the declaration is what leaves r3 occupied and sends the three
     words to r6, r5 and r0 as retail has them.
   * A member retail reads twice -- once for a test, again for the call
     that follows it -- is folded into one load however the two reads
     are spelled: through a const conversion operator, through a local
     copy that dies at the test, or written out twice. The test reads
     through a volatile view instead, and that is the whole of
     zUIModel::DoUpdate's remaining word.
   * The game state's mode mapping is a `switch` over all eleven
     combined states, not an if-chain over their ranges: the clusters
     are tested from the top and the title pair last, which no chain
     gives. Each timer branch keeps its time in a one-member struct,
     copied once, because retail stores it to two stack slots per
     branch, one lo-first as a call result and one hi-first as a
     variable -- the memory-resident shape of an aggregate.
   * Once a unit's init stores its asset, it reads every field back
     through the member and not through the parameter: retail reloads
     `this->asset` after each call. zDecal's curve copy is seven float
     assignments and not a struct copy for the same reason.
   * The four-literal base now blocks a THIRD unit. zDecal::init loads
     four distinct literals seven times, retail spells a `lis` per load,
     and ours forms one `addis` base in r30 -- 139 of 159 words. The
     padding header is already at the measured 41,320 bytes and the wall
     stands at every distance tried; the float-base section above lists
     what has been ruled out for it. zBouncer and zSBPlayerActions stand
     at the same line.

   **Thirty-five units at once, written in parallel.** Game Code 5.33% to
   5.92%, 1,257 functions to 1,377; report.json counts 121 of the 132
   functions in the new units byte-identical, 12,592 bytes. 67 linked,
   unchanged, because nearly every one carries its unity unit's literals.

   TWENTY-EIGHT units are exact whole by `unitcmp.py`, which is the
   stricter of the two counts: `MemoryUtil` 8 of 8, `WAD00_1_3` 8, `xTimer`
   8, `zNavLink` 7, `WAD00_1_2` 6, `zBTNodeSequence` 5, `zPlayerBase` 5,
   `zReference` 5, `ImmediatePrototypeEntity` 5, `zRandomModelList` 4,
   `View` 4, `zCombatSystem` 4, `zFXRibbonPool` 4, `zNPCGroupBase` 4,
   `zUIController` 4, `RawBlobEntity` 3, `xTagParser` 3, `xTextDepot` 3,
   `zNPCAsset` 3, `zPhysicsObjectEntity` 3, `zProjectileEntity` 3,
   `Channel` 2, and `MediaConfig`, `StartupConfig`, `WAD00_27`,
   `WAD03_28`, `xRegionSupport` and `keycode` at one each. Seven are
   partial and say so in their own files: `LightKitEntity` 5 of 6,
   `zSoundPhysics` 3 of 5, `Exception` 2 of 3, `WAD01_5` 2 of 3,
   `zNPCQuickTimeCombat` 2 of 4, `zNPCHelper` 1 of 3, and `WADSpeed_3`,
   which needs a flag and a symbol name this batch did not change.

   The two counts disagree in exactly two places, and each disagreement is
   a tool answering a different question. `keycode` is 1 of 1 by unitcmp
   and 0 of 1 in the report, because retail's symbol is LOCAL and objdiff
   names a local target with its address appended, so nothing in our
   object pairs with it; zEventSpy's event wrapper is the same case. And
   `Exception` is 3 of 3 in the report and 2 of 3 by unitcmp: every
   instruction word of its MapFileData destructor is identical and the one
   that differs is a relocation's NAME, which the report cannot see -- the
   blindness the section above this one is about. Where they differ, the
   count that checks the branch target is the one to believe.

   The method was six agents at a time on triaged lists, and every claim
   re-measured here with `unitcmp.py` before it was believed. Two things
   made that work and one nearly broke it:

   * **`unitcmp.py` compiled every run to one object path.** Found by
     running it the way this file recommends -- several agents at once --
     and it is worth its own section above. A run could read the object
     another had just written and report THAT unit's functions as its
     answer, and nothing about the output looks wrong when it happens.
   * **`disasm.py` annotated addresses it had already adjusted.** A
     `lis` followed by an `addi` finishes an address, and the tool went
     on annotating later accesses from the bare `lis`: MemoryUtil read as
     though its initialiser and its getters touched two different sets of
     variables. It also kept annotating through registers that had since
     been LOADED from memory, so a member access off a runtime pointer
     got a confident address in the engine's data. Both are fixed, the
     high half now travels through `addi`/`addis` and dies on any write,
     and the twelve units re-disassembled to check it changed 90
     annotations and not one instruction.
   * **A triage pass first.** Two things make a unit unreachable as a
     fragment -- four or more distinct float literals in one function,
     and defining or calling a symbol in a unity unit's unnamed namespace
     -- and both are visible from the image before any source is written.
     Checking them first is what kept 34 of 36 attempts productive, and
     `tools/unit_triage.py` is that check: it reads report.json for
     what is left, disassembles each candidate once, and prints the
     two blockers beside the size. Of the 23 untouched units at most
     700 bytes and six functions, it calls 12 clear.

   What the units taught, each recorded in the file that used it:

   * A conditional EXPRESSION whose true side is the value already in
     hand gives retail's branch-to-else-then-jump-past pair; an `if`
     inverts the branch and costs a word, and an `if` with an empty then
     is folded away to the same thing. zNPCAsset's Prepare and zNavLink's
     two nearest-distance choices are all this shape.
   * A `return` written between the tests and the work stays there;
     folded into the function's final return it moves to the end and
     inverts the last branch.
   * Declaration order picks the callee-saved pair, even for a local
     assigned only inside a branch: zNPCAsset's template uid is declared
     UNINITIALISED above the character id to take r31:r30.
   * A function returning a class by value is copied into a NAMED
     variable and not into an argument temporary, so xTagParser's font is
     the temporary passed straight to the text box's create -- naming it
     costs a second copy loop.
   * A function-local static with a CONSTRUCTOR gets the guard byte the
     image holds as @GUARD@; without one it is initialised statically and
     the function is half the size.
   * A pointer to member called in a loop is declared INSIDE the loop,
     after the loop's object is read into its own local.
   * A member read for a test and again for the call after it folds into
     one load however it is spelled; where retail has two, the test reads
     through a volatile view.
   * The .bss padding lever again: three globals addressed from one base
     put MemoryUtil's flag at displacement 6524, not 0.
   * Assignment order picks floating-point registers too -- cross_ypos
     assigns its negation first and cross_xpos its zero first, and the
     stores come out in the same order either way.

   And two units are matched by the bytes but not by the report, for the
   same reason: `keycode` and `zEventSpy`'s event wrapper are LOCAL
   symbols in retail, which objdiff names with their address appended, so
   nothing in our object pairs with them. Each file says so.

   **Sixteen more, the same way.** Game Code 5.92% to 6.40%, 1,377
   functions to 1,454; 77 of the 84 functions in the new units are
   byte-identical, 10,092 bytes. Eleven are exact whole: `zBTNodeCondition`
   8, `zCamPool` 8, `MallocDebugWrapper` 6, `zLetterbox` 6,
   `zNPCExtraModel` 6, `zUIUserString` 6, `MemTracker` 5, `zNPCBTAction` 5,
   `zWallNetGroup` 5, `zFMV` 4 and `zSweptCircle` 3. Five are partial and
   say so: `BuildMemory` 4 of 6, `Texture` 4 of 5, `iMath3` 3 of 4,
   `SkeletonBlobEntity` 3 of 5 and `zPlayerTemplate` 2 of 3.

   `tools/unit_triage.py` came out of this batch and is described above.

   What they taught, each recorded in the file that used it:

   * A conditional expression's TRUE side wants to be the value already
     in hand: `0.0f > curDist ? 0.0f : curDist` gives the bare branch
     retail has, where every `<=` form adds a cror.
   * A ternary over ENUM constants stays a branch; over plain ints mwcc
     folds it to `1 + (x == 4)` with cntlzw and a shift.
   * A loop bound the DWARF types `int` can still be compared unsigned --
     `i < 4U` is what gives cmplwi.
   * mwcc lays a scope's locals out in REVERSE declaration order, which
     is how eight output slots at fixed displacements were placed.
   * A 64-bit equality has an operand order too: the argument pair first.
   * A `NewArray` template needs an explicit `inline` even under
     -inline auto, and even when its body is one expression.
   * An appended character goes through TWO locals, an int then a char;
     one char local drops retail's extsb. And a subscript carries its own
     step, `buffer[length++]` and `buffer[--length]`.
   * An and-chain of inequalities assigned to a LOCAL becomes the
     shift-and-mask; used directly as an if's condition it short-circuits
     into compares instead.
   * A base the DWARF gives four bytes and no members is the derived
     class's vtable pointer: declare it EMPTY, since a virtual of its own
     pushes every slot below it down by one.
   * `#pragma pool_strings off` is right for a unit whose strings the
     image keeps as separate 4-aligned objects with duplicates unfolded.

   Two things already written down needed amending, both measured:

   * **The inliner's floor is not only four stores.** A helper with NO
     stores and a loop is refused too, `#pragma always_inline on`
     included -- zWallNetGroup's containment test, which had to be
     written out in its caller.
   * **unitcmp cannot tell an inlined one-expression predicate from a
     bool local**, because both give the same bytes. `dwarf_lines.py` and
     `dwarf_locals.py` can: the first puts the whole guard on one source
     line, the second says the unit has no named local. Two units were
     rewritten on that evidence with the bytes unchanged.

   And one shape now has three instances and no answer: a loop's COUNTER
   and the pointer it walks come out in the opposite callee-saved
   registers from retail's, with every other word identical --
   BuildMemory's two module hooks, Texture's LOD loop and
   SkeletonBlobEntity's Create. Between them some forty spellings have
   been swept -- the counter outside the loop, pre-increment, `!=`,
   while, do/while, `register`, `long`, `short`, a const bound, pointer
   arithmetic, every declaration order of the locals involved, and
   compiler 1.3 -- and none moves a word. It is the same question
   zNPCType::Setup has been asking since it was written.

   **Nine more, cut short.** Game Code 6.40% to 6.70%, 1,454 functions to
   1,498; 44 of the 54 functions in the new units are byte-identical, 6,412
   bytes. Four are exact whole -- `LightKitSceneEntity` 7, `PoolManager` 7,
   `ScaleformAllocator` 6 and `zCamTargetSpline` 6 -- and five are partial:
   `CMeshBlobEntity` 5 of 7 by unitcmp, `ImmediateInstanceArticle` 5 of 7,
   `xEvent` 4 of 5, `GeometryEntity` 3 of 6 and `zNPCTemplate` 3 of 5.

   The session limit stopped three agents mid-unit, so four of those files
   had no near-miss paragraph when they landed. Each now carries one
   written from a measurement taken here, saying which functions miss, by
   how many aligned words, and -- plainly -- that NO spelling has been
   tried and rejected for them yet. A file that has not been worked on is
   worth more when it says so: the next session can tell the difference
   between an exhausted search and an unopened one.

   Two things were learned from picking up units whose misses nobody had
   read:

   * zNPCQuickTimeCombat's RenderCounter went from 18 to 21 of 24 words on
     the SAME lever zBTNodeCondition's SelfDone needed: retail materialises
     a float comparison into a register (fcmpo, cror, mfcr, rlwinm., bnelr)
     rather than branching on the condition register, and that is what a
     comparison assigned to a bool LOCAL gives. Nesting the guards that
     follow inside the test above them was another word.
   * zSoundPhysics's InitMemory turns out to be a fifth instance of the
     counter-and-pointer register pair, not a new problem: 25 of 31 words,
     all of them that pair. Declaring the counter above the block, which is
     the lever that works elsewhere, does nothing here either.

   **Six more, and two of them turned into levers.** Game Code 6.70% to
   6.95%, 1,498 functions to 1,529. Of the 37 functions the six new units
   define, 34 are byte-identical: `zBuyScreen` 8 of 8, `WAD01_15` 3 of 3
   and `iCameraNG` 8 of 8 are exact whole, `GenericShader` is 7 of 8,
   `zNPCUpdateLOD` 4 of 5 and `WAD02_20_1` 4 of 5. Both agents on this batch
   were stopped by a 529 from the server rather than by anything in the
   work, and every unit they had measured already carried its near-miss
   paragraph, because the brief now says to write that paragraph when the
   unit is measured and not at the end.

   `WAD01_15` is the one worth reading. It was committed-shaped as a
   0-of-2 near miss whose record said the blocker was how a vector
   constant is COPIED -- retail moves each as words, ours as floats -- and
   that every unit assigning a Math vector would meet it. That record was
   right about the mechanism and wrong about being stuck: the section
   below is what it turned into, and the unit went to 3 of 3.
   `iCameraNG` is the same story told twice. Its record said **NO NEAR
   MISS, 6 of 6 byte-identical** where the tool said 3 of 7 -- a figure
   that was never re-measured after the file changed -- and it explained,
   as settled fact, that xMat4x3 had to be spelled FLAT because the image
   has one operator= symbol. The image has both: retail's
   `__as__7xMat4x3FRC7xMat4x3` is 21 instructions whose fourth is a call to
   `__as__7xMat3x3FRC7xMat3x3`. Deriving it matched both operators at
   once and added a function the unit did not have. Two more levers
   finished it: a COMPOUND assignment for SetFOV, where retail keeps the
   running product in one register and a single three-term expression
   does not; and declaring the end sentinel BEFORE the iterator in the
   two viewport-list walks, which were otherwise retail's shape exactly
   and differed only in which register held which. 3 of 7 to 8 of 8.

   Both units say the same thing about records: the mechanism written
   down was worth keeping, the conclusion drawn from it was not.
   **Four units off the twelve-largest list.** Game Code 7.18% to
   7.30%, 1,576 matched functions to 1,606. `zHudSB` 15 of the 19 its
   object defines (1,540 bytes), `zNeoDrive` 8 of 9 (676),
   `zHintSphere` 6 of 7 (332) and `zDestructibles` 3 of 6 (324). None
   is written whole -- these are the tractable functions in units whose
   large ones are still open -- and every one carries what differs.

   Four shapes recurred and are worth having in one place:

   * **The mangled name says static or member.** zHintSphere needed
     both directions in one file: World::EntityManager::FindAsset is
     STATIC, because retail calls GetEntityManager and then overwrites
     r3 -- the manager it just returned -- with the first argument,
     which is what a static reached through an object expression does;
     and zHintSoundSourceManager::HintIsPlaying is a MEMBER, because
     retail reads its argument from r4 and not r3.
   * **A word load is not a bool.** zDestructibles tests the idle
     sound with lwz where a bool member gives lbz, so what is tested is
     the sound's first pointer.
   * **cmpwi against cmplwi names the signedness of a field.**
     HitFilters::gameType is signed, and that was a whole function's
     single differing word.
   * **xMat4x3 deriving from xMat3x3 pays twice.** zNeoDrive got both
     assignment operators byte-identical for free from the shape
     iCameraNG recovered.
   **`MaterialDepot`, the first of the twelve.** 6 of the 9 functions
   its object defines, 344 bytes; the five large ones and the
   std::swap instantiation are not written. Two mangled names settled
   the list types and were worth more than any spelling: retail's
   `PushBack__Q24Util12NodeListBaseFPQ34Util12NodeListBase10NodeHeader`
   puts three qualifiers on the parameter, so NodeHeader is nested
   inside NodeListBase rather than at Util scope, and `Unlink` is
   STATIC -- retail passes the node in r3, where a member call would
   put `this`. Those two took Erase from 3 of 13 words to exact and
   AddMaterial and AddEffect from one differing word each to exact.
   **One more, written in this window rather than by an agent.**
   `WAD01_14` is 4 of the 5 functions its object defines, 436 of the
   unit's 624 bytes, no extra. Game Code 7.14% to 7.16%, 1,566 matched
   functions to 1,570. Three of the four came from levers already
   written down -- the failure-last shape for Activate, the two-type
   split the mangled names forced, and the pragma placement above.

   Its one near miss is worth naming because the tool is right and the
   unit is not: Create is 16 of 46 words and the whole of it is ONE
   instruction, the string. Retail forms the pool base in r31 and
   spells each AddCamera argument `addi r4,r31,6514`; ours has the empty
   string at offset 0 of its own pool and reaches it in one addi.
   `gen_poolprefix.py` cannot fix it, in either mode: it builds a prefix
   from the strings a unit is the FIRST to reference, and this unit
   introduces none -- the empty string at +6514 is the tail of a string
   an earlier file in WAD01.cpp put there. Refusing is the right
   behaviour. What is missing is a mode that takes the prefix from a
   pool OFFSET the unit references rather than from one it introduces,
   and that is a small, well-defined change to a tool that generates
   data rather than to one that decides whether something matches.
   **Seven more, and the record rule paid for itself six times and
   failed once.** Game Code 6.95% to 7.14%, 1,529 matched functions to
   1,566. Of the 51 functions the seven new units define, 38 are
   byte-identical: `zSoundWiimoteSpeaker` 9 of 9, `zBTClient` 9 of 9,
   `WAD01_12_1` 7 of 7 and `RVLFaceLibEntity` 4 of 4 are exact whole;
   `zNPCNinjaManager` is 4 of 7, `WAD00_17` 3 of 12 and `WAD02_4_1` 2
   of 3. `zSoundPhysics` went 3 of 5 to 4 of 5 on top of that.

   All three agents writing this batch were killed when the process they
   ran in exited, with no chance to report. Six of the seven units still
   carried a full record in their own file -- what matched, with the
   denominator, and for the ones that did not, which words differ and
   what had been tried. Nothing had to be re-derived. That is the whole
   argument for writing the record when the unit is MEASURED rather than
   when the session ends, and it is the first time the rule has been
   tested by losing every session at once.

   The seventh is the warning. `WAD00_17` carried **EXACT: 7 of the 7
   functions the object defines are byte-identical** and measured 3 of
   12, with five EXTRA symbols retail does not have anywhere -- checked
   against the symbol table one at a time. The record was not invented:
   it describes a state the file was in, and the session was killed
   while editing past it. So a record written when the unit is measured
   is only true until the file changes again, and the count has to be
   re-taken before the file is committed, not just before it is written
   about. That is the second time a stale count has been found in the
   flattering direction -- `iCameraNG` said 6 of 6 and measured 3 of 7 --
   and both were caught by re-running the tool rather than by reading.

   `WAD02_4_1` is where the next reader should start: Slerp is 8 of its
   121 words and all eight are one 16-byte struct copy, retail doing it
   two registers at a time where ours uses four. The type shape was
   checked and is not the cause, so the question is what retail is
   holding live across that copy that ours has already finished with.

3. **FixWmlType's last TWO instructions.** 11,944 bytes in one
   function, generated by `tools/gen_wmltypes.py`. The dispatch
   matches, the size is retail's to the byte, and 2,984 of 2,986
   instructions align. The re-loads and the two orphaned branches
   are done -- a volatile read in the test, and the two empty cases
   placed where retail's orphans are. What is left is the 0x1C
   loop's `end`: r28 here, r31 -- p's own register -- in retail.
   The section above lists what has been tried for it.

4. **The AnimTable family, mostly closed.** `zSBPlayerActions.cpp` is
   at 89 of 95, WAD01_28 at 56 of 57, zCommonPlayerActions at 26 of
   26. `tools/gen_animtables.py` merges both shapes -- the direct
   tables and the `bctrl` tables that go through the manager -- and
   the section above says the rules, the two inlined helpers the
   bytes named, and the float-base rule that `gen_poolprefix.py` now
   answers with a measured `.rodata` distance, and the NewState
   helper that put the four `AddStates` tables at 100%. What is left
   in zSBPlayerActions (93 of 95) is the two four-literal tables,
   `zPlayerWalkSB::AddActionTransitions` and
   `zPlayerIdleSB::AddInternalTransitions`: the compiler forms an
   `addis` base for four literals past 32 KB and retail spells four
   `lis`, and no setting tried moves that line -- see the float-base
   section for the list. The 439 `bctrl` functions image-wide are
   mostly Havok and Scaleform, and the next game rows -- zPlantTrap
   (5, 1,496 bytes) and WAD01_21 (11, 1,396 bytes) -- were read: they
   are `Reset`, `Init`, `Save`, `Load` and the like, one virtual call
   inside ordinary code, not tables. A census of branchless functions
   whose only calls are the table functions, over every game unit,
   found one left: `zNPCGeneric::Type::CreateAnimTable` in WAD02_26,
   3,236 bytes, 44 states with no owner and no callback, written by
   a script in zNPCUPGeneric.cpp's spelling because the merger
   cannot parse a `Q2` nested name; exact on the first compile. The
   table vein is closed; what is left of it is ordinary
   decompilation.

5. **Another shape.** `tools/shape_census.py` still ranks what is
   left. The biggest row is `addi b`, 97 functions and 776 bytes, of
   which most are `@N@` multiple-inheritance adjustor thunks -- the
   compiler emits those from a class declaration, so the work is
   recovering an MI layout. Anonymous-namespace support in
   `split_symbol` would unlock several rows at once and the units are
   already named after their blobs.

WHAT NOT TO DO. Do not read "the generators are exhausted" off
`gen_accessors --survey` again. That is what it said before ten shapes
were added on top of it; it measures the shapes the tool already
knows, and `shape_census.py` measures the population.

AND RUN `tools/reloc_audit.py` after anything that emits a call.
report.json cannot see a branch target at all, so a wrong one is
silently counted as a match. It has found twelve of those once and a
mangling bug once, and `unitcmp` catching a wrong symbol is what
found the one word zPlayerAction::Update was out by.
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
   - `zNPCType` 11 of 19 words -- register allocation only; structure
     identical. The index takes r4, the argument register the first
     store frees, and its copy of the zero cannot rise above that
     store; retail's index took r9 before any store. Twelve spellings
     over two days (the file lists the seven of 2026-09-02).
   - `zNPCStatus` FELL on 2026-09-02, exact: retail constructs `Math::Vector`
     in place at the member, and the spelling that does it is a call
     of the constructor by its mangled symbol -- the seven spellings
     and eight flag sets recorded here were all C++ that this compiler
     either rejects or gives a null test. See the pick-up section.
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

## THE COUNTER-AND-POINTER PAIR, answered twice

The oldest open shape here is a loop's counter and the pointer it walks
coming out in each other's callee-saved registers, everything else in
the function identical. It had five instances and no answer, and the
thing every attempt reached for -- the ORDER the two are declared in --
moved nothing in any of them.

It is now closed in two: `zSoundWiimoteSpeakerList::Init` and
`zSoundSourcesPhysics::InitMemory`. What closed both was a THIRD local.
Take the allocation into a `void*` and cast that to the array in a
separate statement, AND declare the loop counter above it. Neither half
alone moves a word in either function; together they match. In Init the
two halves are visibly doing different jobs -- the `void*` puts the
array pointer into retail's register (7 differing words to 6) and only
then does the declaration order decide the counter (6 to 0) -- so the
reason order had never worked is that the pointer was in the way of the
register order had to reach.

So the lever for this shape is not the order of the two registers that
differ. It is that a third value, one whose live range crosses theirs,
can be what lets the order matter at all.

It is not universal, and the two places it fails say something. 
`Graphics::Texture::SetImageFromFileInMemory` has the same symptom in
the same direction, and all four combinations were measured: as written
8 of 95 words, the `void*` alone 8, the counter first alone 13, both
together 13. `BuildMemory`'s two loops have no allocation for a third
local to hold, so the nearest thing was tried -- a local pointer to the
buffer they walk -- and it makes them WORSE: RenderStartup 6 of 41 to
11 and RenderShutdown 7 of 30 to 13, with both changes 8 and 10.

So the third local helps in two functions and hurts in three, which
means the mechanism is not simply register pressure and the lever is
not the whole answer. `zNPCType::Setup` and `SkeletonBlobEntity` are
the two instances nobody has tried it on.

**Answered a third time, in a template.** `zBlackboard::Write<T>` --
five instantiations from one body -- had the pointer the loop walks
(`var`, retail r29) and the loop counter (retail r30) swapped in every
instantiation, 8 or 9 words each, at retail's exact sizes. There is no
allocation here for the third local to hold; the pointer comes back
from `Cast(v, result)` through a reference. The same two halves closed
it anyway: `void* p = result; var = (zVariable<T>*)p;` in two
statements, AND `unsigned int i;` declared above `var`. Each half alone
was measured and moved nothing, and thirty other shapes across six
sweeps -- every declaration order, a named observer local, a pointer
walk of the observers, an inlined setter, a reference or a pointer to
the value -- moved nothing either. So when the symptom is this exact
one, the recipe is worth trying whole before anything else, and the
sweep that finds it is five variants, not thirty.

One more thing came out of closing InitMemory, and it is not about
registers. Its last differing word was a branch whose target NAME was
wrong: xMemAlloc's size parameter had been declared `unsigned long`,
which mangles Ul where the image has Ui. reloc_audit does not catch
that by itself, because it only audits functions that already match --
a wrong name inside a function that differs for other reasons stays
invisible until those other reasons are gone.
## A CONSTANT THAT IS NOT CONST -- Math's vectors live in .bss

`vec2Zero`, `vec2OneX`, `vec3Zero`, `vec4Zero` and `_matIdentity` are all
in **.bss**, not .rodata. They are built at runtime, and they are not
const. Declaring them `extern const` -- which is what they look like, and
what anyone would write -- changes the code mwcc generates around every
loop that reads them, because a const global's loads may be hoisted past
stores that could alias it and a plain one's may not.

In `IO::PadInput::Reset` the difference is stark. Declared const, mwcc
pre-loaded the four words of `vec2Zero` and `vec2OneX` into registers,
SPILLED all four to the stack, and ran a nine-instruction loop body.
Retail keeps only the two addresses live, re-reads inside the loop, and
runs thirteen. Dropping the `const` took that function from 67 of 68
words wrong to exact, and `IO::PadTilt::Reset` from 80 of 80 to 19 of 81
in the same compile.

Two more levers were needed for the unit, and both generalise:

**A vector's one member is itself a class.** The DWARF gives `Vector2` a
`DataType` of two floats, `Vector3` one of three, `Vector4` one of four,
and `Vector` derives from `Vector4` (`tools/dwarf_types.py --all`; they
are in there under bare names, so `--type Math::Vector2` finds nothing
and `--type Vector2` finds it). That nesting is what makes the copy a
block move of words. Spelled flat, as four float members, mwcc
synthesises a memberwise lfs/stfs `operator=` and emits it OUT OF LINE as
`__as__Q24Math7Vector2` and two siblings retail does not have;
`#pragma always_inline on` hides the symbols and keeps the floats.
Spelling them as plain PODs with no access specifier does not help --
mwcc still synthesised all three.

**The store order names a CHAIN.** Retail loads `vec4Zero`'s four words
once and stores them to two destinations, `vec3Zero`'s three once to
three, and each run of scalars goes out highest-address-first. That is
`a = b = c = k`: the rightmost target is assigned first, so reading the
store order backwards gives the chain. `IO::PadIRData::Reset` is
`x = y = 0.0f` followed by two stores, ten instructions, and it matched
first try once it was read that way. Where retail stores ASCENDING the
statements are separate -- `flags = 0; changed = 0;` in the same
function -- so the order distinguishes the two spellings rather than
leaving it to taste.
## always_inline AT THE END OF THE FILE AIMS AT A TEMPLATE ALONE

mwcc instantiates a template at the END of the translation unit, so the
state of `#pragma always_inline` THERE is what the instantiation sees --
and an ordinary function, compiled where it appears, never sees it. That
makes the bottom of the file a way to force inlining inside a template
without touching anything else, and it is not obvious from the pragma's
own description.

`WAD01_14` needed it. `zCamMG2P`'s constructor is empty and its whole
body is the implicit vtable store plus two member constructors, which
retail has inline inside `zCamPool<zCamMG2P>::AddCamera`; -inline auto
does not take it in any of four spellings (implicit, empty in-class,
declared and defined inline outside the class, and the class given its
own constructor). always_inline does, and the three placements are three
different answers:

  * before the class and left on -- AddCamera matches and EventCB is
    ruined, 49 words against retail's 20, because Activate gets inlined
    into it where retail tail-calls it;
  * before the class with an off after it, or around the template with
    an off after -- no effect at all, AddCamera stays 23 of 24;
  * at the very end of the file, after every function -- AddCamera
    matches and nothing else moves.

It is still a pragma and still the closest measured state rather than
what the original said. But the placement rule is a fact about the
compiler, and it applies to every template this project has yet to
write.
## WHAT 10% OF GAME CODE COSTS, measured

Asked for 10%, the arithmetic is worth writing down rather than
re-deriving. Game Code is 2,116,616 bytes. 10% is 211,661; at 7.18% the
shortfall is about **60,000 bytes**. Three routes were measured, not
guessed:

**The generators are spent.** `gen_survey.py` finds 5 functions across 4
units still holding a known shape. Whatever comes next is hand-written,
or a shape nobody has recognised yet.

**The big functions hold the mass but are the hardest.** The 50 largest
unmatched game functions are 207,416 bytes, three and a half times the
shortfall; the largest 200 are 477,212. But the biggest single one,
`FixWmlType` at 11,944 bytes, is already 2,984 of 2,986 instructions
aligned with seven left, and the section above lists what has been ruled
out for those seven. Size and difficulty rise together.

**The practical route is the largest blocker-free units.**
`tools/unit_triage.py --bytes 12000 --functions 40` lists 81 units clear
of both blockers, and the twelve largest come to roughly 61,700 bytes --
just over the shortfall. In order: SkinBuilderWii 9,304, MaterialDepot
6,472, zHudSB 5,976, EntityManager 5,348, zNeoDrive 5,168, zHintSphere
5,056, zDestructibles 4,420, zPathFinderSearchPathEvaluator 4,164,
WAD01_17_1 4,152, wiitextures 4,096, zBTBuilder 3,808, WAD02_22_1 3,752.
Allow for the functions that will not fall and it is nearer twenty units.

**The rate.** The session that measured this moved Game Code 6.70% to
7.18% -- about 10,000 bytes -- across seven commits, and roughly a third
of that came from three parallel agents rather than from the main
session. So 60,000 bytes is on the order of six more sessions of that
size, and parallel agents are the single largest lever on it. That is
the number to plan against; it is not a figure anyone should have to
discover twice.
## A UNIT WRITTEN A FUNCTION AT A TIME NEEDS gen_poolprefix --whole

Nine of `zHudSB`'s functions were each exactly ONE word wrong, and
the word was the string offset every time. The default prefix is
everything in the pool BEFORE the first string the unit introduces --
which is right for a unit written whole, and wrong for one written a
function at a time, because the strings between that first one and the
ones the written functions use live in functions not written yet. Every
later offset then comes out short by exactly the bytes of the missing
strings.

`python tools/gen_poolprefix.py --whole <unit>.cpp` emits the pool's
whole contents instead -- 360 strings rather than 39 for zHudSB -- and
the nine matched at once. The tool's own help says this (`--whole`:
"a unit written a function at a time needs its own strings at their
retail offsets too"), which is easy to read past when the default has
just produced a header that looks right.

It is not a cure for everything: run against the four other near-miss
units carrying a pool header -- zGameState, CMeshBlobEntity,
zNPCUPGeneric and zSBPlayerActions -- it changes none of their counts,
so their remaining functions differ for other reasons.
## ONE TEMPLATE BODY, TWO INSTANTIATIONS, TWICE THE BYTES

EntityManager instantiates `EmbeddedTreeAVL<T, Cmp, OFFSET>` at node
offsets 28 and 36 -- two of the three EmbeddedTreeNode offsets in
EntityHandleBase -- so four of its members appear in the image ONCE PER
INSTANTIATION. Five bodies -- AuxiliaryDelete, BalanceRight,
BalanceLeft, Delete and Insert -- matched 3,640 bytes in the object,
3,352 of them credited to the unit (the splits give Insert<36> to
WAD02_35_1, where it was already matched). Nothing else on the
twelve-largest list pays twice like this; it is worth looking for the
pattern before picking a unit.

**mwcc 1.1 takes explicit instantiation.** `template class Foo<...>;` at
file scope emits every member that has a definition, and the mangled
names come out exactly right, offsets included. Without it nothing is
emitted at all, because a template member nothing calls is never
instantiated -- and a unit written a function at a time has not written
the callers yet. The template is at GLOBAL scope here, which the
mangled name says: no namespace qualifier on EmbeddedTreeAVL itself.

**The balance factor is biased.** The node packs the right child and
two balance bits into one word, and the accessor returns
`(word & 3) - 1` -- the classic -1/0/+1 AVL factor. Retail computes
`rlwinm` then `addi -1` and compares against 1, 0 and -1; spelling the
balance as a plain 0/1/2 gives three direct compares and 120 differing
words instead of 28. One test inside the double rotation is the
exception and reads the RAW bits, because retail has a single
`rlwinm.` and `bne` there where the biased form costs two more
instructions.

**Two register levers finished it, and both are already recorded
elsewhere in this file.** Declaring the double rotation's two locals at
the TOP of the case -- ahead of the ones the earlier branches use --
took BalanceRight from 28 differing words to 6. Reading the child's
packed word into ONE named local, and deriving both the balance and the
right pointer from it rather than calling two accessors, took the last
six: retail reads that word once and keeps it in a scratch register.

**The mirror keeps the ORDER of the tests.** BalanceLeft's double
rotation tests the node first and the child second, exactly like
BalanceRight; mirroring swaps which test each gets, not their order.
That was 54 differing words to 41.

**Declare the case's locals before `n`.** Retail has the node in r28
and the right child in r31; ours had them swapped. Ten declaration
orders were swept and the only ones that reached 7 words declared all
four case locals (right, mid, mn, rn) at the top of the FUNCTION,
ahead of `n`. BalanceRight wanted its two double-rotation locals
first within the case; BalanceLeft wants everything ahead of the
node pointer. Same allocator, different answer, and only the sweep
says which.

**A named local for the switch value.** The last seven words were the
switch head: retail keeps the node's packed word in r4 and the switch
value in r0, ours had the word in r0 and reused r3. `int bal = n->Bal();`
`switch (bal)` gives retail's allocation exactly. Spelling the switch
expression inline -- `switch ((n->right_color_bal & 3) - 1)` -- is one
instruction SHORTER and 120 words off, which is what the earlier
measurement that 'hoisting the node's word makes it worse' was really
seeing: the hoisted form had also inlined the expression.

**Insert and Delete want `n` per branch.** Retail folds node+OFFSET
into the displacement where it is only a base, and computes the
address in the branch that passes it to SetRight or operator=. A
function-scope `n` is hoisted above the null check and materialised
in r31 for every use, 36 and 68 words off; declared inside each
branch, both matched at once. The balance routines are the opposite
because their switch needs the node immediately.
## WHAT -O4's AUTO-INLINER TAKES, and where the body sits in the file

zBlackboard's 1,036-byte payload dispatcher is the unit's only plain
function bigger than a screen, and retail's has three copies of
`GetVariableType` and one of `Read<xVec3>` INLINED into it -- while the
four `Read<T>` the image holds out of line are called. The unit is now
25 of 25 and every byte of it, and four things were measured on the
way there.

**The auto-inliner judges the source, not the bytes.** GetVariableType
as `if (v != 0) { return v->type; } return eVarType_Invalid;` compiles
to 52 bytes and is never inlined; as `return v != 0 ? v->type :
eVarType_Invalid;` it compiles to the SAME 52 bytes and is inlined at
every call. Nothing in the flags moves that line: `#pragma
inline_max_auto_size(20/40/100)`, `inline_max_size`, `inline_depth`,
`auto_inline on`, placed at the top of the file or before the caller,
all leave every call standing, and `inline` on an out-of-class member
template definition changes nothing. So when retail inlined a small
function and yours will not, the lever is a more compact spelling of
the CALLEE, and a ternary is the compact one.

**A callee's locals are allocated LAST, and that is how a slot lands at
+8.** The inlined Read<xVec3> keeps its cast pointer at 8(r1) and the
GetValue temporary at 64(r1) -- the lowest slot of each size class --
because mwcc assigns stack slots in declaration order within a size
class and an inlined body's locals come after all of the caller's. No
spelling of Read<T> is compact enough to inline (six were measured,
and the compact ones break the standalone bytes), so the case is
written out by hand around a tiny in-class helper that IS taken:
`template <class T> zVariable<T>* CastTo(zVariableBase* v) const` that
declares the pointer, calls Cast and returns it. Its one local is the
one that lands at +8. Written flat in the case, the same pointer takes
+32 in declaration order and pushes every later local down a word.

**`== false` lays the early return inline.** `if (!Read(source, v))
return false; return Write(...)` comes out with the Write as the
fall-through and the `li r3,0` after it; retail has the false return
inline and branches over it to the Write. An explicit `else`, a bool
local, the ternary and the Write-first form all give the first layout.
`if (Read(source, v) == false) return false;` gives retail's, in all
five cases at once -- 36 words to 1.

**A named temporary flips one side of an equality compare.**
`if (GetVariableType(writeTo) != sourceType)` and its mirror both emit
`cmpw r29,r0`; retail has `cmpw r0,r29`. Neither the operand order nor
the type of sourceType (int, unsigned) moves it. `eVarType targetType =
GetVariableType(writeTo); if (targetType != sourceType)` does. The
same function also wanted `eVarType sourceType;` DECLARED before
`unsigned int source` and assigned after -- r28/r29 the other way
round otherwise, nine words.

**And the inliner cannot take a body it has not read yet.** zBTBuilder's
`Build` calls a 20-byte `NewArray<zBTNode*>` in retail and inlined it in
ours -- 84 of 90 words, and 8 bytes too long. Two statements in the body
is the lever that stopped the six `BuildXNode` from being inlined and it
does nothing here; neither does `#pragma dont_inline on` around the
definition. `#pragma dont_inline` around the CALLER does stop it and
takes three wrappers out of line with it -- the unit goes from 12 of 12
functions to 10 of 15, and Build to 67 of 88 at 352 bytes. What works
is moving the template's DEFINITION to the foot of the file, below
its callers, leaving a declaration where it was: 84 words to 2, and the
size exact. So position in the translation unit is a lever, and it is the
one to reach for when the callee is a template and no spelling of it is
uninviting enough.

That call was worth more than its own bytes, and this is the part worth
carrying forward. Keeping it a call keeps the ADDRESS of a
reference-bound constant live, and that was the THIRD reference into
this unit's `.data` from `Build`. With two, mwcc spent a hi/lo
relocation pair on each -- two instructions where retail has one, which
is the whole 8-byte overrun. With three it materialised the section base
in r30 and reached all of them at displacements (168, 172, 176), which
is retail's shape and one a hi/lo pair cannot be made to look like.

Two counts are not a threshold and this does not say where the line is;
what it says is that the number of references to a section decides the
addressing, so a function that is one reference short of retail's is a
function whose CALLS are wrong, not whose loads are. The only other
point measured is the same function's `.bss`: nine references to three
statics, base register, no help needed.

**The count is only half of it: the other half is REACH, and in a split
unit reach is not a property of the source.** zNPCPerception's
`IsInDirectPath` reads three constants out of `.rodata` -- 0.0f, 1e-05f,
1.0f -- and by the paragraph above that is enough to hoist a base, which
is exactly what ours does: `lis`/`addi` into r31 and three displacements.
Retail spends a high half per reference instead, so the extra `addi`
makes us 69 words where retail has 70, and taking r31 for the base pushes
every other local down a register. Nothing in the source is wrong.
Compiling the same text with 36,000 bytes of `.rodata` placed AHEAD of
the three constants -- so that no signed 16-bit displacement can reach
them from the section base -- makes the function byte-identical at 280.

So mwcc anchors at the SECTION, not at the first constant, and the
hoist is available only while the whole span fits the displacement
field. That is a fact about the retail image as much as about us: it
says WAD02.cpp carries at least 32KB of `.rodata` before these three,
where our split of it carries 0x18 bytes in total. A function whose
only difference is a hoisted base against a small section is therefore
not a function to keep sweeping -- it is one waiting on the rest of its
translation unit, and padding the section to fake the distance would put
data in the object that the manifest does not name. Worth checking
before assuming a near-miss is a source problem: how big is the
section, and can one register reach all of it?

**And the same thing happens to STRINGS, where nothing can be done at
all.** zVar's five formatting functions are each one word from exact at
retail's size, and the word is `addi r4,r4,<offset>` -- where the format
string sits inside the pooled string section. Retail's offset is 10,018
and ours is 3, because `-str reuse,pool,readonly` pools the strings of a
whole translation unit and ours holds only the ones on this page. That
offset is a compile-time constant rather than a relocation, so unitcmp
does not mask it and no spelling of the source moves it. Where the
offset is small enough mwcc also folds it into the low half of the
address and spends one instruction where retail spends two, which is why
one of the five is four bytes short as well as one word wrong.

So the rule to carry is bigger than the base register: **a split unit
cannot reproduce a compile-time offset into a pooled section**, and both
`.rodata` constants and string literals are pooled. A function that
formats a string is not a good target in a split, and one that reads
three or more float constants is a coin toss. Neither is a reason to
write the source differently -- it is a reason to check the section
before spending a sweep on it.

**Run the padded build as a DIAGNOSTIC, because it separates two
questions that otherwise stay tangled.** zNPCCombat's `HandleNPCDamage`
measured 141 words out of 162 and looked like a rough draft. With the
pad it measured 77 of 165 -- so the addressing was most of it, and what
remained was small enough to sweep. Two real differences came out of
that sweep and would have been invisible under the noise: `before` has
to be a separate local in each case of the switch, because retail gives
one branch f31 and the other f30; and the damage multiplier has to be a
local, because spelled inline the product comes out with its operands
the other way round no matter which order it is written. With both, the
padded build is 168 of 168 and the real one is 165 of 168. The pad
never goes in a commit -- it is a way to ask whether the source is
wrong, and here the answer was no.

Read this section as a set: each of the five was the whole remaining
difference at the time, and each was found by a sweep that included the
obvious spelling and the obvious spelling lost.
## A CLASS TEMPLATE'S MEMBERS NEVER INLINE, and what constness does to a heap

Domains (WAD00_1) is built on two families of helper -- `Util::
BlockAllocatorArray<T>` and a set of allocate/free templates that take
the heap as a parameter -- and both taught something the next unit that
uses them will want.

**A class's member function does not inline into a class template's
member.** DomainPriv's constructor has the array's vtable pointer and its
six fields written out, and the AVL tree's Insert has BalanceLeft,
BalanceRight and its comparator written out, because TWENTY-THREE
spellings between them all came out as a CALL: a constructor taking
(heap, tag) defined in the class, defined out of it, with `inline`, with
a member-initialiser list instead of assignments, an ordinary `Init`
member instead of a constructor, one with no calls of its own, `#pragma
always_inline on` inside the class body and again around the whole
template at namespace scope, `inline_max_size`, `inline_max_auto_size`
and `inline_depth` at the callee and at the top of the unit, a
comparator reached through the tree's Cmp base, the same comparator as a
free function template, and four spellings of it including two ternary
chains. Every one of them: a call.

Two things DO inline into a class template's member, which is how the
rule is bounded rather than guessed at. A tiny FREE template does --
`Free()` reaches `BlockAllocatorArray<T>::DeleteBlocks`, and matching it
depended on that. And a one-expression accessor of a plain class does --
`EmbeddedTreeNode::Bal`, `SetBal` and `Right` are all inlined into
Insert. What never arrives is a class's member function carrying real
code. The same file inlines a PLAIN class's constructor into a plain
function without being asked: CreateActivity takes two iterator
constructors and the vtable-pointer stores of six Activity subclasses.
So the rule is about the CALLER being a class template's member, not
about constructors and not about size, and the answer when retail has a
body inline there is to write the statements at the call site.

**`const H&` makes the static; a by-value copy moves the load.** The
image holds four unnamed 4-byte STT_OBJECTs for this file (@21996,
@22450, @22661, @22708) and reads a heap enum out of each. They are
reference-bound constants -- mwcc materialises one when a CONSTANT binds
to a `const H&`, and refuses the deduction outright for a non-const `H&`
("does not match"), which is how retail's `Delete` mangling can say `R`
while the free that binds a constant must be a different function. And
the LOAD through that reference lands where the reference is copied, not
where it is used: `Free(const H& heap, void* p)` whose body starts
`H h = heap;` puts the load ahead of the null test, which is where
retail has it in all four callers. Written without the copy the load
sits inside the `if`, and four functions are wrong by the same four
words.

**A fold is not a difference, and the two tools disagree on purpose.**
Six branches in this file name a symbol the linker folded away -- an
empty constructor onto `Math::Matrix33::Matrix33()` (four bytes, one
`blr`), a `Block` constructor and a `Delete` onto the
`BlockAllocatorArray<void*>` instantiation, a `SetPool` onto
`PoolList<zBTTask*>`'s. unitcmp checks the branch-target NAME and reads
the unit 13 of 21; report.json resolves the branch by ADDRESS in the
linked image, where the folded symbol is the same bytes, and reads 18 of
21. Neither is wrong and neither should be quoted without saying which
question it answers. reloc_audit already separates them: this file moved
its folded count from 4 to 10 and its overstated count not at all.
## `+=` IS NOT `x = x + y`, and declaration order picks the register

zNPCPerception (WAD02_29_1) is 24 of 24 and every one of them came down
to how a statement was spelled rather than what it did. Four spellings
are worth carrying.

**`+=` emits the accumulator first; the spelled-out form emits it
second.** `types[i].perceivedTimer = types[i].perceivedTimer + dt`
compiles to `fadds f0,f31,f0` -- the increment first -- and so does
`dt + types[i].perceivedTimer`, so it is not operand order in the source
that decides it. `types[i].perceivedTimer += dt` compiles to `fadds
f0,f0,f31`, which is retail. A local for the old value does the same.
One word of 75, and no reading of the source would have found it: the
two forms are the same expression.

**A do-while has no guard and no counter.** A loop over a fixed-size
member array wants a BOTTOM-TESTED pointer walk with the end written
inline in the condition. A counted `for` costs one instruction (mwcc puts
the trip count in ctr); the same pointer walk as a `while` costs five,
because a top test needs a guard and a computed trip count; and hoisting
`&types[6]` into a local of its own is eight words out. Four forms
measured, one match. Where the array is a MEMBER with a constructor, do
not write the loop at all -- mwcc emits exactly this shape itself, and
the loop's position then tells you which statements belong to the base
constructor and which to the derived one's body.

**An if chain and a switch are different shapes on the same values.** A
chain lays each body immediately after its own test and branches past
it; a switch puts every test first and the bodies after. This file needs
one of each on the same four type ids -- GetTargetEntityCenter is a
chain, the two radius accessors are switches -- and swapping either is
17 words and four bytes out. The image says which; nothing else does.

**Which local is DECLARED first decides which register it gets**, and it
is not first USE. Three functions here turned on it: Setup wanted its
trip count declared before its counter, SetAssetAuto wanted the 64-bit id
declared (uninitialised) before the result pointer and assigned after,
and AllAttached wanted the asset pointer in a local of its own so the
store between the two loads landed there. In every case the variable
declared FIRST takes the higher register, initialising at the point of
declaration reorders them, and no rewriting of the loop or the
expression moves it.
## A CONSTRUCTOR YOU NEVER CALL STILL CHANGES THE CODE

zNPCCombat's two smallest puzzles turned out to be one question asked
twice: is `xVec3` a POD? Nothing in the unit constructs one, and the
answer still decides two functions.

`zNPCGetsDamageInfo::operator=` is compiler-generated, and retail's is
a flat copy of all eleven words -- nine general registers and, when
those run out, two floating-point ones -- with the xVec3 member copied
as three raw words. Declare `xVec3& operator=(const xVec3&)` and the
generated copy calls it instead, member by member: 30 words against
retail's 23. Leave it out and the copy is retail's exactly.

But `SetFromCombatDamageInfo` in the same unit assigns one xVec3 and
retail CALLS `__as__5xVec3FRC5xVec3` for it. With no declaration mwcc
still emits and still calls that function -- the call was never the
problem. What changed was the scheduling around it: with a plain POD
the compiler hoisted four loads of the source struct above the stores
into the destination and used four registers where retail walks
through r0 one field at a time. Seventeen words of 43.

**Declaring a CONSTRUCTOR fixes it -- on the class being READ, not on
the one being written and not on xVec3.** A constructor that is never
defined and never called makes its class non-POD, which is enough to
stop mwcc reordering reads of it across writes to something else, and
it leaves the copy-assignment trivial, so the generated operator=
stays the flat eleven-word copy.

WHICH class matters, and the first answer here was wrong. Putting the
constructor on `xVec3` also makes SetFromCombatDamageInfo match, and
it costs `HandleNPCDamage` in the same unit: a non-POD xVec3 turns
`xVec3 at = model->position;` from three inline word moves into a
call, and that function goes from 3 words out to 75. Putting it on
`zCombatDamageInfo` -- the struct whose members are being read --
fixes the scheduling and leaves xVec3 alone. All eight combinations of
a constructor on the three classes were measured; exactly the ones
with zCombatDamageInfo non-POD and xVec3 POD get both functions.

So POD-ness is a lever on ALIASING, the copy-assignment is a separate
lever, and a constructor separates them; reaching for `operator=`
moves both at once and only one of them the right way. And the class
to put it on is the one whose loads are moving, which is worth
checking rather than assuming -- a constructor on a widely used type
like a vector reaches every copy-initialisation in the file.

The corollary is worth stating plainly: a class in one of these files
should be given the members it really has, not the minimum that
compiles. Leaving a constructor out is not a neutral omission.
## Traps worth knowing

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

**An APPENDED compiler flag can be silently ignored.** mwcc keeps the
FIRST `-O` it is given, and unitcmp passes extra arguments after its own
BASE, so `python tools/unitcmp.py <unit> '-O4,p'` compiles at -O4,s and
prints the -O4,s answer. Nothing warns. It is the benign-looking value
again, and worse than usual because it reads as a REFUTATION -- the
claim under test was that a unit wants -O4,p, and the run says it
changes nothing. To test an -O flag it has to REPLACE the one in BASE.
And check the replacing harness against an answer already recorded
before believing it about a new one: configure.py records
zPlayerContainer::ContainsEnt as exact at -O4,s and 13 words against
retail's 14 at -O4,p, and a harness that cannot reproduce that pair is
not evidence about anything. This was found by running that control
after an append-based test had already produced a confident wrong
answer.
**objdiff scoring 100% does not mean the unit links.** `zCamSplineCommonMix`
compiled to byte-identical instructions while referring to a symbol that did
not exist -- `Follower` is nested in `zCameraCurve`, and the DWARF gives leaf
names only. objdiff scores instructions; it does not check that a relocation
names something real. Always `ninja` and check `main.dol: OK`.

## 113 INSTANTIATIONS OF ONE TEMPLATE: the BT action factory

`WAD01_1_1` is one function template written 113 times, and it went
from 2 of 2 to 112 of 115 in one sitting -- 13,340 bytes, Game Code
8.5882% to 9.2185%. Every class in it came out of four facts per
instantiation, all of them in the bytes: the `li r4,N` that is
`sizeof(T)`, the `bl __ct__...` that names whose constructor runs, the
`addi r3,r31,N` in front of each member constructor call, and the
stores between them. `disasm.py --unit` prints all four, and the sizes
of the member types fall out of the gaps between the offsets, so a
short script turned the dump into declarations.

**The body is the placement new `zBTNodeCondition::CreateTask` already
needed**, null case first:

    void* mem = factory.AllocMem(sizeof(T),
                                 (Memory::eFactoryMemType)14);
    T* action = !mem ? 0 : new (mem) T();

Retail tests the allocation TWICE -- once for the expression and once
inside the new -- and lays the zero block before the constructor. 93 of
the 113 need nothing else; the other twenty construct members.

**`#pragma always_inline on` IS WHAT PUTS THOSE CONSTRUCTORS IN LINE,
and here it goes at the TOP of the file.** -inline auto takes a
constructor whose body is the base call and one vtable store, and
declines it the moment a member constructor or a second store joins
them -- four spellings were tried before the pragma, including the
constructor declared and defined `inline` outside the class. Without
it mwcc emits `__ct__<T>` as its own function and calls it, and those
twenty come out 112 bytes against retail's 136 to 248 while the object
defines twenty constructors retail's unit does not have. With it, 111
of 115 matched in one compile and nothing that already matched moved.
The always_inline section above records the same lever needing the END
of the file in WAD01_14; the difference is that this file has no
ordinary function for a leading pragma to ruin -- its two accessors are
one store each and stayed matched.

**AND EACH T DECLARES AN OVERRIDE IT DOES NOT DEFINE.** A class whose
first virtual is defined nowhere in the unit gets no vtable of its own
here: mwcc REFERENCES `__vt__<T>` rather than emitting it, which is
what keeps this object from defining 113 tables the manifest does not
name. Leave the override off and the class defines one -- measured on
the intermediate `zNPCBTAction`, 48 bytes of it, before every T got a
`virtual void _v1();`.

Three are still out, each recorded at the class it builds:

  * `Create<zNPCBTJumpAction>` is the four-float blocker
    `tools/unit_triage.py` counts, seen here rather than assumed: the
    constructor loads 16, 2, 5 and 10, and this object anchors ONE base
    register and reads all four off it where retail -- past 32 KB of
    constants -- spells a `lis` per literal. 45 of 57 words at exactly
    retail's 228 bytes.
  * `Create<zNPCBTEscortAction>` (42 of 47) and
    `Create<zNPCBTPathFollowMPAction>` (39 of 60) miss the same way as
    each other: retail HOLDS a register on the sub-object being built
    and stores its vtable pointer through that, where this folds
    sub-object plus field into one displacement off the object and so
    spends one or two registers fewer. That is also why retail's
    PathFollowMP saves r28..r31 through `_savegpr` and ours stores two
    by hand. Same instructions, same offsets, same order.
### The same unit's own constructors: where the two pragmas sit

The eleven constructors and the `Destroy` this unit defines took it to
120 of 126, 195,688 bytes and 9.2453%. Writing them made the pragma
above fight itself, and the arrangement that works is worth stating
exactly, because both halves of it were measured wrong first:

  * `always_inline on` at the FOOT of the file -- the placement the
    WAD01_14 section recommends -- reaches the templates AND every
    constructor defined above it, so the Creates fold those in too:
    96 of 126.
  * `always_inline on` at the top with `dont_inline on` round the
    definitions is right, and gives 116 of 128 -- but **dont_inline
    stops inlining INTO a function as well as out of it**. Under it
    `zNPCBTMoveToAction`'s constructor called two constructors retail
    folds, and the object gained both as functions retail's unit does
    not have.
  * So the two constructors whose own body needs a fold go AFTER the
    explicit instantiations, outside the block: nothing is left to
    inline them into, and the fold inside them happens. 120 of 126.

One of those two folds went away for a different reason worth keeping:
the intermediate `zNPCBTAction` exists only to add four bytes, and
deriving the two MoveTo actions from `zBTAction` directly -- padding
the four bytes by hand -- removes the constructor mwcc was emitting
for it.

**THE FLOAT ANCHOR FORMS AT THREE LITERALS, NOT FOUR.**
`tools/unit_triage.py` counts a function unreachable at four or more
distinct float literals. In this unit two is fine and three is not:
`zNPCBTActionAnim` and `zSteeringPath` load two each and match, while
`zWanderData` (5, 0.5, 10) and `zWallAvoidanceData` (24, 1, 0) each
form one `lis`/`addi` base and read all three off it where retail
spells a `lis` per literal -- 20 of 26 and 9 of 11, and nothing else
in either differs. That is six functions of evidence from one unit,
which is why the tool's threshold has NOT been changed on it: doing
that would need the same count taken over the matched functions
already in the tree, and a guard that fires on correct input is worse
than none. The measurement is here so the next person starts from it.

And `Destroy` is 21 of 61 words at 244 against 248, with everything
up to the id test byte-identical, the case blocks already in retail's
order and one word between them: retail's second id test branches TO
the deallocation with a `b` to the exit in front of it, and ours
branches past it. Five spellings give the same bytes -- an and-chain
of inequalities round the call, an or-chain of equalities that breaks,
the same with an explicit else, an EMPTY then with the call in the
else, and a nested switch on the id -- and so does making slot 1
return an enum instead of an unsigned int.
### And Build matched on the first compile: 3,792 bytes, 948 words

`zBTActionBuilder::Build` is the switch that turns a type id into an
action, 115 cases over the 113 Creates and two shared globals, and it
came out byte-identical with 121 relocations masked WITHOUT ONE
SPELLING TRIED. That is worth stating because it is the second time a
large mwcc switch has done it -- `FixWmlType`'s 307-case dispatch is
the other -- and it means the compiler's binary search over case
values is fully determined by the SET of values and needs nothing from
the source but them:

  * The id for a case is the constant the compare in front of its
    `beq` builds, which `disasm.py` already prints as `= XXXXXXXX` on
    the `addi`.
  * The BODY is whatever the label sits on -- here one `bl Create<T>`
    each, or a global for the two that skip the setup.
  * The block order in the image IS the source order, so listing the
    cases by address gives the switch to write.

Two traps in reading it. The `beq` that ends the null test at the FOOT
of the function is not a case, so a compare only arms the next branch
when it was against the switch's own value -- taking every `beq`
invented a 116th case. And the setup block comes FIRST with the
fallback last, because retail branches PAST the setup to the block
that loads the always-fail action; written the other way round the
whole tail moves.

It also answered a question from the function above it. The two
constants `Destroy` refuses to free are exactly the two ids Build
answers with `gActionAlwaysComplete` and `gActionAlwaysFail` -- one
unit, and the id slot, the shared globals and the deallocation rule
all agree.
## THE SAME SHAPE A SECOND TIME: zBTConditionBuilder, 63 of 63

The condition side of the behaviour tree is the action side again --
one function template per owner, 60 instantiations, a switch that
turns a type id into one, a Destroy -- and writing it from the dump
took one sitting and no spellings: **9,708 bytes, the whole unit**,
Game Code 9.4245% to 9.8828%.

It was found by grouping every symbol with a `<` in it by what is
left when the type between the brackets is removed, and reading off
the byte counts and how many members are already matched.
`Create<>__15zNPCBTConditionFPCc` was 54 members and 6,264 bytes with
none matched, sitting next to the 106 that had just been done.
`tools/twin_census.py` now asks the more general form of that
question and does not need the names to be similar at all.

Three things differed from the action side, all of them in the bytes:

  * The condition keeps two words in front of its vtable pointer at
    +8, not three in front of one at +0xC, and there is no base
    constructor to call -- so every Create is the allocation, the
    vtable store and one virtual, and the six on the plain factory
    never touch a callee-saved register at all.
  * **The NPC side's Create takes the condition's NAME**, and retail
    reaches it as one pooled base plus a baked-in offset. That is the
    string-pool blocker, and `gen_poolprefix.py --whole` is the
    answer: 395 strings, 4,712 bytes of prefix, and the offsets come
    out right.
  * The asset setter Build calls is named **zBTAction's**. The
    condition's own would be `stw r4,0(r3); blr`, byte for byte the
    action's, and the image holds one function under that name, so
    the call is written through it.

Three traps in reading a switch out of a dump, all of which cost a
compile here:

  * **A case block is not a fixed number of lines.** Reading five
    from the label let the three-word block that answers with
    `gConditionFalse` swallow the next block's `bl` and come out as a
    Create. Cut at the next label.
  * **Not every case makes something.** Two answer with a shared
    global and then run the same setup, and dropping them left the
    binary search on a different SET of values: identical tree shape,
    different constants from the second compare on, 595 of 669 words.
  * **A setter defined in the same unit gets folded into the
    caller.** `SetBTClient` came out as `lwz`/`stw` in line where
    retail calls it; `#pragma dont_inline` round the definition is
    the guard, and there is nothing inside it to block.
## 10% OF GAME CODE, and the shape that got there

Game Code is **10.0559%** -- 212,844 of 2,116,616 bytes, 2,060
functions, 1,305 of them hand-written across 210 units. It moved
8.5882% to 10.0559% in one session, and 27,000 of those 31,000 bytes
came from the SAME QUESTION asked three times:

    which template families are left, and how big are they?

Group every function symbol with a `<` in it by what is left when the
type between the brackets is removed, sum the sizes, and count how
many report.json already calls matched. Three families came out of
one run and all three fell:

  * `Create<>__12zNPCBTAction` -- 106 members, 13,304 bytes, in
    WAD01_1_1 with its Build and Destroy: 121 of 127.
  * `Create<>__15zNPCBTConditionFPCc` -- 54 members, 6,264 bytes, in
    zBTConditionBuilder: 63 of 63, the whole unit.
  * `zScene_SetupEach<>__FUi_v` -- 42 members, 3,664 bytes, in
    WAD03_24: 40 of 43.

A family is worth more than its byte count suggests, because the
members differ in only a few measurable ways and the dump prints all
of them. WAD03_24's 42 are one loop over a scene's per-subtype list;
everything but the step is identical, and the step is a named member,
a free function taking the scene too, a virtual through the object's
own table, or two calls in a row. Written as an overloaded one-line
helper the template calls, 39 of the 42 matched on the first compile.

The other three name the honest limit of this method. `zSoundMask`'s
and `zTiki`'s Setup and `zDispatcherData`'s setup helper are EMPTY,
and the linker folded all three onto `Math::Matrix33`'s constructor.
Each is one word out -- the branch target -- and `reloc_audit` counts
them in the category it has for this, taking the folded total from 14
to 17 with 0 overstated. report.json resolves the branch by ADDRESS
and counts the bytes; the linked image genuinely cannot say which of
the three names was written.
## TWO QUESTIONS NOBODY HAD ASKED, AND WHAT THEY PAID

### Does the .rodata padding actually convert the float anchor?

`gen_poolprefix.py` has emitted a padding array since the pool work,
and its docstring states the rule exactly -- under 32 KB into
`.rodata` mwcc shares one base among a function's literals from three
up; past 32 KB it spells a `lis` each up to three and forms an
`addis` base for four or more. Nobody had put a header on a unit that
builds NO string pool just for the padding, and nobody had tested the
rule from both sides at once.

WAD01_1_1 is that test. Its first literal sits 41,320 bytes into its
translation unit's `.rodata`; the fragment compiled alone puts it at
nothing. With the header in:

  * `zWanderData` and `zWallAvoidanceData`, THREE literals each, went
    from one word short to byte-identical. 121 of 127 to 123 of 127.
  * `Create<zNPCBTJumpAction>`, FOUR literals, got WORSE -- 228 bytes
    and 45 words out became 232 and 50 -- which is the `addis` base
    the same rule predicts past 32 KB.

So the rule holds in both directions, and it reconciles a
contradiction this session had already recorded: the measurement said
the anchor forms at THREE literals and `unit_triage.py` counts the
blocker at FOUR. Neither is wrong. **Three is the threshold for a
fragment compiled bare; four is the threshold for one carrying its
pool header.** `unit_triage.py` states the padded rule and does not
say so, which is why a unit it calls clear can still lose its
three-literal functions.

### Is any unmatched function a byte-twin of one already written?

`shape_census.py` asks what the unmatched functions share with each
other, by primary opcode, so a generator can be written for the
biggest shape. The question nobody had asked is the one that puts the
matched and the unmatched in ONE clustering: **which unsolved
function is the same shape as something already solved?**

`tools/twin_census.py` answers it. The key is every instruction with
its registers, with only the branch displacements and the low sixteen
bits of the immediate forms blanked -- stricter than an opcode
signature, looser than the bytes. Of 10,284 game functions read,
**110 clusters hold both a solved member and an unsolved one: 530
unsolved functions, 26,408 bytes**, which is 1.25% of Game Code
sitting in shapes somebody has already written.

The largest is one family across four sizes: `Sext::<Asset>::Create`,
which takes a block from the global heap, memsets it, places the
entity and runs an init -- 31 unsolved at 96 bytes against 2 written,
23 at 124 against 7, 7 at 136 against 3. Then 14
`AddTransitionsFrom` at 124 against 2, several destructor clusters,
and 83 eight-byte adjustor thunks against 3.

**The method was tested before it was believed.**
`Sext::xGroupAsset::Create` was written by reading
`Sext::UI_Model::Create` in zUIModel.cpp and changing the type, its
size and which init runs. It matched on the first compile, 124 bytes,
and its retail disassembly was never read for anything but the three
constants.

A cluster is a LEAD, not a proof: the key blanks the sizes and
offsets, and those are exactly what the source still has to get
right. What it removes is not knowing the shape at all, which is the
part that costs days.
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

## A TABLE BODY CAN STORE AS WELL AS CALL -- 3,916 bytes of it

`gen_animtables.py`'s `walk()` records CALLS. Nine of the 344 Add*
functions in the image also STORE, and the emitted bodies were short
by exactly those words with nothing to say so: the size is the only
symptom, and the size is not what the merge checks. Two shapes:

  * **Floats set on the action before the states.** Four bodies do
    `this->+0x10 = 2.0f` and `0.0f` into the three words after it.
    576 bytes: `AddStates` for zPlayerSlamFallSB, zPlayerFluidBurstSB
    and their two Board twins.

  * **The state the call RETURNS, kept in a member.** `stw r3,K(this)`
    straight after the `bl` -- the variant set zPlayerHitSB already
    had spelled out and matched: fifteen states, a count, a valid
    count and a no-repeats flag, 0x48 bytes in all. Five bodies,
    3,340 bytes: zPlayerHitBoard (1,200), zPlayerIdleBoard (964),
    zPlayerDefeatedBoard (616), zPlayerTriggered (336) and
    zPlayerSingleCustomAnimSB (224). zPlayerIdleBoard has TWO variant
    sets, at +0x10 and +0x58, which is what puts its second count at
    +0x94: 0x58 + 15*4 = 0x94, and that arithmetic is what fixes the
    array's length.

**And zPlayerAction::NewState returns the state it made.** Two units
declared it `void`. The mangled name carries no return type, so both
spellings name the same symbol and a call that discards the result
compiles the same either way -- which is why it went unnoticed for as
long as no body kept the result.

**A scan keyed on r3 finds three of the four float stores.** The SB
pair copies `this` into r10 at the top and stores the last one
through that, so `stfs fN,K(r3)` misses it -- and the scan reported
three, twice, without a hint that it was wrong. What settled it was
reading one function's disassembly end to end. The walk now tests the
VALUE in the base register, which it already carried symbolically,
and finds four of four.

`walk()` returns the stores, `emit()` writes a result-keeping call as
an assignment and declares the member it needs, and any other store
on `this` is a problem that stops the merge -- named, not dropped.

The lesson is the one the near-miss list keeps giving: **a body that
is a few words SHORT is not a register-allocation problem.** Words
that are absent are source that is absent. Both of these looked like
permuted register allocation in the word diff, because everything
after the missing store shifts by one.

## THE PLAYER'S LAYOUT IS IN THE DWARF, AND FIFTY BODIES FOLLOW FROM IT

zSBPlayerActions is 54,788 unmatched bytes of small methods, and what
made them writable is not a generator: it is `dwarf_types.py`. The
retail link kept debug info for WAD03.cpp, so zSBPlayer is 0x1DD0
bytes with `attackState` at +0x8C8, `canDoubleJump` at +0x9F1,
`quicksandSinkDistance` at +0xA1C and `performCelebration` at +0xA3C --
named, not guessed. Fifty bodies were written against that in six
batches and every one of them matched.

**The player is reached through a CAST, not a base.** `player` is a
`zPlayer*` and the offsets are zSBPlayer's, so every body says
`((zSBPlayer*)player)->`. That is already the file's convention --
`zPlayerLandHighSB::End` was written that way -- and it keeps
zPlayerAction's own layout untouched.

**Six idioms cover most of the small ones**, and each is a whole
body:

    cntlzw r0,r0 / srwi r3,r0,5         return x == 0;
    addic r0,r3,-1 / subfe r3,r0,r3     return x != 0;
    fcmpo / cror 2,1,2 / mfcr / rlwinm  return f >= K;
    fcmpo / cror 2,0,2 / mfcr / rlwinm  return f <= K;
    rlwinm rA,rS,0,28,26                x &= ~0x10;
    rlwinm. r0,r3,0,17,17               x & 0x4000

the AND-masks being the wrapping form, whose mask is everything
OUTSIDE the range: 0,28,26 clears bit 27, which is 0x10.

**WHICH BRANCH FALLS THROUGH IS THE SOURCE'S SHAPE.**
`zSBPlayerHammerAttack::End` came out three words off because retail
`beq`s to the reset and increments on the fall-through, so the test
is `!= 16000` and not `== 16000`. Same fact, opposite spelling, and
the bytes say which.

**A CLAMP IS A TERNARY.** `if (v < -30) v = -30;` gives one branch
and one store; retail branches BOTH ways into a single store, which
is what `v = (v > -30) ? v : -30;` gives.

**AND && AND || ARE NOT INTERCHANGEABLE HERE.** `beqlr`/`bnelr` --
conditional RETURNS off one `li r3,0` -- is what `&&` gives;
separate ifs each with their own return is not. But a range on ONE
variable is the exception: `state < 2 || state > 4` folds into an
unsigned subtract and a single compare, which retail does not do,
and neither the `&&` form, two ifs, nor an enum-typed comparison
avoids the fold. Two bodies (40 and 52 bytes) are left unwritten for
that reason rather than written wrong.

## THE OTHER LOOP IS AN INDEX, AND ITS COUNTER IS UNSIGNED

One Fix body does not walk a cursor to a computed end: it counts
`i` from zero to a member RELOADED each iteration and addresses the
element as `this + i * stride`. The compare is a `blt` against that
member, not a `!=` against an end.

**A REFUSAL BUCKET IS NOT A CENSUS OF WHAT THE BODIES ARE.** Six
were refused with `a cursor loop whose bounds do not read`, and I
wrote the section saying six were index loops. `findop.py` says
1 of 158: the other five got PAST the bound once the index loop was
in the vocabulary, and are refused further on -- two on an
instruction, one on a Fix call the reader cannot name, one on a
store, one on a prologue. A bucket counts where a reader STOPPED,
which is the first thing it could not do and not the thing the body
is.

    for (i = 0; i < m20; i++) {
        char* e = (char*)this + i * 112;
        *(long*)(e + 44) += base;
        ...
    }

**AND `i` IS UNSIGNED.** With `int i` the whole of HudUp's 316 bytes
came out right except one word -- `cmpw` where retail has `cmplw`.
The member's own type does not decide that: the conversion goes the
other way, so a signed index makes the comparison signed whatever
the count is declared as.

## INSTANTIATE THE MEMBER, NOT THE CLASS

`Memory::Creator<N, T, B>` is one template body:

    void* p = f->AllocMem(sizeof(T), (eFactoryMemType)N);
    if (p == 0) { return 0; }
    return new (p) T;

-- and `zNPCType::sAllocateNPC<T>` is another. Sixteen functions,
1,400 bytes, and the holes are a size, a tag and a constructor, all
three in the bytes: `li r4,SIZE`, `li r5,N`, and either a `bl` to
T's constructor or the vtable store its INLINE one makes.

**THE SECOND NULL TEST IS THE PLACEMENT NEW's.** `if (p == 0)
return 0;` gives the `bne` and the `li r3,0`; `new (p) T` guards the
pointer AGAIN on its own account, which is the `beq` that follows
and looks redundant.

**`template class X<...>;` EMITS A FUNCTION THE IMAGE DOES NOT
HAVE.** Retail carries `Create` for three of these instantiations
and `CreateV` for the rest, never both -- so instantiating the CLASS
gave nine EXTRA functions and unitcmp said so by name. Instantiating
the MEMBER that retail has emits exactly that one. And `CreateV`
carries the whole body rather than `return Create(f);`: written as a
call it stayed a call, eight bytes of `b Create`.

**A NAMESPACE AND A CLASS MANGLE THE SAME, and that is what lets a
name be both.** Four of the T's were already NAMESPACES in this
file, holding a nested `Type` whose constructor was matched --
`__ct__Q214zNPCAnimViewer4TypeFv`. Turning the namespace into a
class keeps that symbol exactly and makes the name available as the
type being allocated.

**AND A MEMBER STORED BEFORE THE VTABLE IS IN A BASE.**
zNPCQuickTimeCombat came out five words off: retail zeroes the word
at +0 and THEN stores the vtable at +4, where ours did the vtable
first. A class's own constructor cannot run before its base's, so
the member belongs to the base -- the DWARF names it zNPCComponent
and puts it at +0 of a 0xC0-byte class, which is the size the
allocation asks for.

`#pragma always_inline on` around the instantiations again: a
constructor with an initialiser list is past what `-inline auto`
takes on its own, and without it mwcc emits
`__ct__18zNPCFXLoopFXScriptFv` out of line -- a function NOT IN
RETAIL, which is what unitcmp calls it.

## A SILHOUETTE READ SIX OF EIGHTY-FOUR; READING THE PROGRAM READ ALL OF THEM

`gen_assetfix.py` matched a candidate's TAIL word for word against
one of two bodies written by hand, and reported six. It was not
reporting six of eighty-four -- it `continue`d past everything whose
prologue differed, with no count, so seventy-eight functions and
10,656 bytes read as nothing at all. **A tool that could not read a
thing must not report it as nothing**, and the first fix was to
bucket every one of them by what actually stopped it. Twenty-five
had a different prologue, seventeen another, ten another: not one
family of exceptions, just a template that was too narrow.

**THE FAMILY IS A VOCABULARY, NOT A TEMPLATE.** All eighty-four do
the same five things in different orders:

    CustomFix(base);                    the base class's own fixup
    m1C = (void*)((long)m1C + base);    a pointer member relocated
    Sext::FixWmlType(base, m08, m0C);   a (type, pointer) pair fixed
    m50.Fix(base);                      a sub-object at a known offset
    if (m50 == 3) { ... }               a member tested against a constant

plus one loop -- a cursor over an array whose FILE OFFSET is a
member and whose length is another -- whose body is the same five
things applied to the cursor. So the tool now READS the program out
of the bytes: a register file carrying five symbolic values (`this`,
`base`, a word loaded from a member, that word plus base, a count
times a stride) and an instruction vocabulary that refuses anything
else BY NAME. 139 of 156 read; 58 of 58 readable ones in WAD00_32
are byte-identical.

**THE VALIDATION IS THE ONES THAT ALREADY MATCH.** Seventy-eight of
them were already written and already matched, and every one is a
known-good answer: `--validate` renders each and looks for the
rendering IN THE FILE. It found four separate emitter bugs before a
single new body was compiled -- a duplicated array relocation, the
wrong member names, a sub-object type written without its namespace,
a stray blank line. None of those would have been visible in a diff
against bytes nobody had compiled yet.

**EACH LOOP HAS ITS OWN CURSOR PAIR.** BehaviorTree has four loops
and VehicleConfiguration two, and in BOTH the FIRST loop is the odd
one out: retail gives it cursor r28 / end r29 where every later loop
takes r29 / r28. Two variables cannot hold two allocations, so the
source has a pair per loop and mwcc coalesces the disjoint live
ranges. Declaring one pair and reusing it left those two bodies six
and eight words wrong; a pair each made them exact, and made the six
bodies refused for `two loops over different element types`
writable as well. `end` is still declared before its cursor.

**A LOOP IS WRITTEN IN BYTES, NOT IN ELEMENTS.** `p + n` needs
`sizeof(T)` to BE the stride -- but the sub-object stubs are
deliberately EMPTY, one byte, because every member offset after them
is measured against that size. Sizing `LinkAssetBaseNew` at its
40-byte stride moved `Sext::BSP`'s members and broke a body that was
already matched. `(char*)p + n * S` needs nothing from the type and
emits the same multiply and add, so the two facts stop fighting.
The EventLinkNew loop keeps the element form: that type is fully
declared, and its 77 bodies already match in that spelling.

**AND THREE SINGLE WORDS.** `lbz` where we wrote `lwz`: the loop
count is a BYTE, and the width it is loaded at is the width the
member is declared at. `cmplwi` where we wrote `cmpwi`: the member
is UNSIGNED -- and only a NONZERO test can say so, because equality
with zero is `cmpwi` either way, so CurveCamera reads as signed on
its `== 0` and unsigned on its `== 1`, `== 2` and `== 3`. And
`addi r3,r30,16` where we wrote `mr r3,r30`: the loop's Fix is
called on a sub-object SIXTEEN BYTES INTO the element, and the
reader had decoded the offset and the writer had dropped it.

**THE MERGE IS KEYED ON THE FILE, NOT ON THE REPORT.** Skipping
rows that report.json says already match makes the result depend on
when the report was last generated: after a build, re-running the
merge from a clean file wrote three bodies and dropped forty-five.
It now skips a row whose rendered body is already in the file, which
is the same question asked of the thing that actually answers it.

## 16% OF GAME CODE, and the last 8,000 bytes of it

**CreateAnimTable is a SEARCH, not a table** -- which is why
gen_animtables refuses all seven of them, and rightly: they open
by walking the module's list of tables looking for one whose id
matches, and hand that one back if it is there.

    xAnimTable* zSpinner::CreateAnimTable(unsigned long long id) {
        xAnimTable* table = animSpinnerTables;

        while (table) {
            if (table->id && *table->id == id) break;
            table = table->next;
        }

        if (table == 0) { <eight bytes for the id, then the calls> }

        return table;
    }

The list node IS an xAnimTable -- `next` at +0, the id pointer at
+0x1C -- which is why the found node is what comes back.
`gen_createanimtable.py` writes the head from a template checked
word for word against a matched one, and takes the calls from
gen_animtables' own walk. A second shape keeps the id in a static
of its own instead of taking eight bytes for it, so its head is two
words shorter; both are tried and the reason given is the first's.

**THE POOL HEADER IS NOT OPTIONAL, and it moves the registers.**
Written without one, zSpinner's came out 684 bytes of 688 with 148
of 171 words differing -- and the register allocation was among
them, which reads like a hard problem. It is not: the strings fall
at the unit's own offsets rather than the unity build's, `addi
r4,r30,8` where retail has `addi r4,r27,9659`, and the addressing
is what the allocation follows. With `gen_poolprefix.py --whole` it
is byte-identical. The generator refuses a unit that has no pool
header for that reason.

Three of the seven keep their tables in an ANONYMOUS namespace,
mangled with the unity build's filename, which a split-out unit
cannot name (see anon_blocked.py). 2,692 bytes, refused rather than
written as something that names nothing.

**THE DESTRUCTOR FLAG SAYS BASE OR MEMBER.** EngineOG::SceneData's
80-byte destructor came out one word off: `li r4,0` where retail
has `li r4,-1`. Zero is a BASE subobject and -1 a complete one, so
what it destroys is a MEMBER at +0, not a base -- `hkBaseObject m;`
rather than `: public hkBaseObject`. Everything else about the two
spellings is identical.

**And a GFx array deletes through its own operator.** The two
GArray destructors were also one word off, and the word was a
MASKED one: our `bl` named `__dl__FPv` and retail's reaches
`Free__11GMemoryHeapFPv`. GArrayBase derives from GNewOverrideBase,
whose `static void operator delete(void* p) { GMemoryHeap::Free(p); }`
-inline auto takes at the call site. A masked word can still be
wrong, and unitcmp checks those by NAME -- there is no `<<` on the
row, only the count.

**The 124-byte Sext::*::Create template now covers four more**:
an entity whose base is not World::xOGEntity (the base is a call,
so it is declared with the vtable pointer the constructor stores at
+0 and nothing else, and the entity carries the whole allocation as
padding); an asset class at global scope rather than in Sext; and a
NESTED entity, FX::Ribbon::zRibbon, whose definition and forward
declaration are wrapped in namespaces because `class A::B::C : ...`
is not C++ -- a single mangled qualifier is written bare and two or
more take Q<n>, so a namespace and a class give the same symbol.

## THE BRANCH THAT IS NOT IN THE SOURCE -- 9,852 bytes of it

`gen_animtables` refuses a table with a branch in it: that is a
different shape. Twenty-three tables in zSBPlayerActions.cpp have a
branch and no branching source -- it is inside the INLINED helper.

`zPlayerAction::AddActionTransition` ends `c == 0 ? ActionChange : c`.
Where `c` is a constant mwcc folds the test away, which is why every
table written before this one is branchless. Where `c` is the table's
own PARAMETER the test survives:

    cmpwi rX,0 ; ... ; bne +12 ; lis rD,hi ; addi rD,rD,lo

with the two skipped words forming ActionChange. Two things follow,
and BOTH are needed -- either alone is wrong:

  * the walk SKIPS those three words, so rD keeps the parameter,
    which is what the call is actually given, and the branch is not
    counted;
  * the call has to be spelled THROUGH the helper, whose inline body
    compiles the test back. Skipping the test and then emitting
    xAnimTableNewTransition directly leaves the body six words short.

Which calls those are is recorded where the test is skipped, not
guessed from the arguments: with `c` a parameter the existing test
(r8 == ActionChange) cannot fire, which is exactly why these bodies
would otherwise come out direct.

Only that exact shape -- a `bne` over exactly two words that form
ActionChange and nothing else. Any other branch still refuses the
table, and the seven `CreateAnimTable__*FUx` bodies still do: those
open with a real search of a linked list of tables and an early
return, which is source, not an inlined test.

**And `Ux` was not a scalar type.** Seven CreateAnimTable bodies take
an `unsigned long long` id, and the mangled token for it was missing
from the table -- so the signature did not read and the whole
function was refused before its shape was even looked at. `Fv`, a
function of no arguments, was read as a function of one void for the
same reason.

**The regression, run against the tool as it was.** 13 tables that
the new tool writes and the old one refused come out APPENDED rather
than merged: `into()` looks for the head as one line, and those were
hand-wrapped with the parameters named a and b where the generator
names them c and d. A duplicate definition does not compile, so
measuring the unit after each symbol catches it; comparing the two
tools' output directly is what said the difference was in the
MERGE and not in the bodies.

## WAD01_28 IS WHOLE -- 256 of 256, and what the walk could not see

The unit went 240 of 246 to **256 of 256** in one sitting, and ten of
those functions had never been written at all: report.json shows a
function our object does not define as 0.0%, which reads like a bad
near miss and is not one. `unwritten_tables` is the query -- an Add*
in a unit that HAS source, at 0.0% -- and it found 57 of them, 24,840
bytes.

**Four things gen_animtables' walk could not see**, each of which
refused a table that is otherwise complete:

  * **An incoming argument that arrives on the STACK.** param_src has
    always named those -- it hands back ("stackarg", slot) for every
    parameter past r10 -- but nothing ever produced one, so a table
    that forwards its ninth argument read it as unresolved. The slot
    is the ABI's: 8 bytes into the CALLER's frame, frame+8 off r1.

  * **A prologue's register save read as an outgoing argument.**
    `stw r31,28(r1)` is not st28. A save is r14 and up with no value
    the walk has seen -- the register's incoming value. `stw r29,8(r1)`
    where r29 holds a zero the body put there IS an argument, and the
    test keeps it.

  * **`addi rD,rS,N` where rS holds an argument.**
    zBoardPlayerHammerAttack passes its priority parameter and then
    that parameter plus ten. Folding addi only for ints dropped the
    register and read three calls as unresolved.

  * **`rlwinm rD,rS,0,16,31` is the PARAMETER'S TYPE, not an
    expression.** `h + 10` passed where the parameter is
    `unsigned short` compiles to addi and then that mask; the value
    to carry is still `h + 10`, and the mask re-appears when it is
    compiled. Any other mask on a symbolic value is still dropped --
    that would be an expression, and this refuses to guess one.

**And the manager in a local.** `manager->f(); manager->g();` reloads
the member before each call, because a call can change it; retail
reads it ONCE into a callee-saved register wherever the source had a
local. Which it was is not in the argument lists -- both give
r3 = ('ld', this, 0) for every call -- but it is in the instruction
stream: count the `lwz rD,0(rA)` with rA holding `this` against the
calls made on the result. The generator emits the local when there is
ONE load and two or more calls. Three bodies load it twice for four
calls or four times for six -- a local for a RUN of them -- and those
are still near misses: zPlayerLand 114 of 117, zPlayerLedge 50 of 69,
zCommonPlayerDash 51 of 59, each one or two words SHORT as well, so
there is something missing in them besides the hoist.

zPlayerHit needed the STRING in a local too -- retail forms
`addi r28,r4,7499` once and passes `mr r6,r28` to all three calls --
and zPlayerHitLaunch, three calls of the same shape in the same file,
does NOT: there retail re-forms the address per call from a base it
keeps. So it is a difference in the source, not a rule, and the
generator does not guess at it.

**The regression that made all this safe**: regenerate every table
already in the unit, one at a time, and require the source to come
back identical. 126 of 130 do; one is the hand-applied hoist and
three are the variant-set bodies the new store guard refuses. Run as
a batch instead, ONE refusal stops the merge and nothing is checked,
which reads as a pass and is not one.

## THE FILL SHEET, and two guards that saw what the oracle could not

`twin_census.py` says which unsolved function has the same instruction
skeleton as one already written. It stopped there, and the rest was
hand work: read both listings, spot what differs, retype the source.
`transplant.py` does that step. Two functions in one cluster agree on
every opcode and every register, so only three kinds of field can
differ -- a branch displacement, a 16-bit immediate, and the halves of
a lis/addi address. Walking the two word arrays in lockstep therefore
yields the COMPLETE list of what the source has to change, with
nothing else in it. A near-miss hunted by eye is a search; this is a
form.

For zHintSphere against xGroup: 31 instructions, 23 byte-identical,
3 differing only in a displacement while reaching the same symbol --
not holes, the source that produced them is unchanged -- and FOUR
holes: the allocation size spelled twice, the vtable, and the init.

THE SKELETON IS NOT THE SHAPE, which is the same lesson `size is not
shape` taught one level up. The 124-byte Sext Create cluster holds
eight solved members and a first version of `describe()` read all
eight as `entity->Init(asset)`. Five of them are not: they call a FREE
function -- `zEnvInit(entity, asset)`, `xTimerInit`, `zUI_Init` -- and
the entity goes in r3 either way, as `this` or as the first argument.
One skeleton, two C++ constructs, and the instructions cannot tell
them apart. THE MANGLED NAME CAN: a member carries `__<class>F` and a
free function carries `__F`. Validating against all eight rather than
one is what found it, and 3 of 8 was the score before the fix.

THE SAME CLUSTER ALSO HIDES THE BASE. Three of the eight derive from
`zUI` and one from `zEnt`, not from `World::xOGEntity`, and emitting
the xOGEntity layout for one of those places the vtable and every
offset wrong while compiling cleanly. The template refuses instead.

AND IT HIDES THE SIGNATURE, which cost a rebuild and is the best part.
The template declared every init as taking the Sext asset BY POINTER.
Retail does not: `xCamTransition::Init` takes `const
Sext::transition_time&`, `xScreenFade::load` takes
`xScreenFade::asset_type&` -- a class nested in the entity --  and
`zCameraCurve::Init` takes a `zCameraCurveAsset*` that is not in
namespace Sext at all. A pointer and a reference are both an address
in r4, so ALL ELEVEN UNITS MATCHED BYTE FOR BYTE EITHER WAY and
report.json called them 100% both times.

Two guards said otherwise, independently, and both were right:
`unitcmp.py` resolves branch targets by NAME and scored those five
units 0 of 1, and `reloc_audit.py` put them under `folded` -- a symbol
nowhere in the image -- beside the real linker folds. Neither needed
the other. The true signature never had to be guessed: the `bl` in the
target's own disassembly resolves through the retail symbol table, so
its mangled parameter list was in hand the whole time. Declaring what
retail declares took folded from 22 back to 17, left the bytes
untouched, and raised those five pins from 0/1 to 1/1.

WHAT THE COMPILER ALREADY WROTE DOWN. The retail ELF is an unstripped
CodeWarrior link carrying 4.68 MB of DWARF 2, and for this cluster it
placed 22 of 22 unsolved members in a named source file -- including
the 17 that `twin_census` could only call `(no source file yet)`. The
line table for zHintSphere's Create reads 923 through 927 over five
lines, which says the original was one `new` expression, one init call
and a return: the shape that was already being written. That is not
the compiler being run backwards. It is the compiler having been asked
to record the correspondence at the time, and nobody stripping it.

Eleven of the cluster's members were emitted, compiled and matched on
the first build: xScreenFade, xCounter, xCamTransition, xMovePoint,
zConditional, zCameraCurve, zPortal, zLensFlareSpawnPt,
zUIFlashOnScreenText, zScript and ztextbox. Eight of the remaining
eleven the template refuses by name -- another base, a nested entity
type, an unqualified asset -- and three name a unit configure.py has
no Object row for.

THEY COUNT AS GENERATED, and that is the point of the split. A tool
emitted those eleven files; the decompiling happened once, by hand, on
the donor. `written_vs_generated.py` learned a third banner and
hand-written stayed at 1,363 across 223 units while generated went 726
to 737. Eleven more matched functions and not one more function of
decompiling -- which is exactly what that figure exists to keep
separate.

### The second cluster: thirteen animation tables, three holes each

The fill sheet is not about asset Creates. Pointed at
`AddTransitionsFrom`, 2 solved against 14 unsolved at 124 bytes, it
read 27 of 31 instructions byte-identical, ONE call reaching the same
symbol from a different address, and exactly THREE holes: the
destination state string and the two halves of a check callback's
address. The donor's source -- zPlayerIdlePlankton, already matched --
is a single call to the action helper, so the whole of the work was
reading two values per target out of the image.

Both are there to be read. The state string is a POOLED literal, so
the `addi` forms `@stringBase0 + K` and the bytes at that address are
the string; the callback is a plain symbol. 16 of 16 members gave up
both, and the two already written came back "Idle01" and anIdleCheck,
which is what their source says -- the check that the extraction was
right before any of it was used.

THE CHECK IS NOT ALWAYS THE ACTION'S OWN, and assuming it was cost a
compile. zPlayerLandSB calls `zSBPlayerAction::anSBLandCheck`, and
zPlayerIdleShooting -- matched long before this -- calls
`zPlayerIdlePlankton::anIdleCheck`. The mangled symbol carries the
owner; the first version threw it away and kept only the method name.
That is the same mistake as reading a free init as a member, one
cluster later: the name has more in it than the shape does.

Four of the strings this file had never used are inside the pool
prefix `gen_poolprefix.py` already wrote, so `-str reuse` folded the
references onto them and no offset moved. Thirteen matched on the
first compile; zSBPlayerActions went 93 of 95 to 106 of 108, and the
two that still differ are the ones that already did -- zPlayerIdleSB's
6,568-byte table and zPlayerWalkSB's four-literal one, which
gen_poolprefix.py's own header records as out of reach.

The fourteenth, zPlayerIdleHub, is in WAD03_22.cpp: a gen_accessors
file with no pool prefix, where taking it over for one function would
move three generated functions into the written column. Left.

### The second template: 96 bytes, and five ways a name resolves wrong

`sext_create_ctor` is the other half of the asset-Create family: the
entity's own constructor takes the handle AND the asset, so nothing
stores a vtable and nothing runs afterwards. It needs no base class at
all -- the constructor is a call, so the only thing about the layout
the file knows is sizeof, read off the `li r3,N`. Thirteen emitted,
thirteen matched.

Validated against all 18 already-matched members first: 12 described
correctly, 0 wrong, and 6 REFUSED because their entity is a nested
type -- FX::zFXInstance, xRumble::boxEmitter -- which cannot be
defined by a qualified name. A refusal is not a miss; it is the
template saying it does not describe this one.

THE COMPILER FOUND FIVE BUGS AND EVERY ONE WAS A SCOPE QUESTION. In
order, because the order is the lesson:

  * `param_start` took the FIRST `__` in the symbol, which for
    `__ct__` opens the name rather than separating it from the class.
    Every constructor parsed to nothing. Matching the separator by
    what FOLLOWS it -- F, or a class -- is what class_of already did.
  * `Pv` is a void pointer and `v` alone is the no-parameter marker;
    telling them apart needs the count of modifiers consumed. And `U`
    is the unsigned prefix of a builtin, not a modifier beside P and
    R -- nothing in the first cluster used one, so that bug could not
    show, and it would have declared `int` where retail has
    `unsigned int`.
  * The entity class is emitted BEFORE the Sext namespace and its
    constructor names the asset, so the asset needs a forward
    declaration ahead of it. The template only wrote one for the
    uncommon case.
  * zSoundsNamed's constructor takes the asset BY REFERENCE. Same
    lesson as the init signatures one cluster earlier -- a pointer and
    a reference are one address in the register and two spellings in
    the source -- except this one does not compile, so mwcc caught it
    rather than reloc_audit.
  * AND THE ENTITY CAN SHARE ITS NAME WITH THE ASSET. Sext::ScreenWarp
    ::Create returns the global ScreenWarp, and inside that
    definition the unqualified name finds the enclosing class first,
    so `sizeof(ScreenWarp)` measured the asset. Both templates now
    write the entity from the global scope inside the Create body.
    This is the third time the same rule has bitten: an unqualified
    name inside `Sext::<Asset>::Create` is not the global one.

Two of the thirteen -- Sext::ScreenWarp and Sext::FXScreenWarp -- land
in one unit, so that file carries one preamble and both bodies.

Generated went 737 to 750 and hand-written did not move.

### The third shape: the init is a VIRTUAL, and the slot is in the word

136 bytes, 3 solved against 7 unsolved. Same as the 124-byte Create --
base constructor, vtable stored here -- except the init is dispatched
through the vtable, so the hole set gains a SLOT and loses the init
symbol. A call through a vtable names nothing, which is why the
parameter type is the one thing about this shape the image cannot
say; it is spelled as the asset, which is what it is called with.

The slot is (K - 8) / 4 of the `lwz r12,K(r12)`, because a CodeWarrior
vtable pointer points eight bytes past its start. zEventSpy, already
matched, reads 88 -> slot 20, and its source declares Init as the
twenty-first virtual. That is the check that the arithmetic is right.

10 of 10 members gave up entity, asset, size, base and slot. Five
were written and matched: zSlope and zSinkingSurface at slot 32 on
zInteractionUP, zTrampoline at 32 on zEnt, zLedge and zFog at 20 on
World::xOGEntity.

THE BASE CARRIES NOTHING BUT ITS CONSTRUCTOR, and that is not a
shortcut. The vptr the derived class's own virtuals create sits at +0
either way, so how sizeof splits between base and derived changes no
code -- all the padding rides on the derived class and the file makes
no claim about a layout it cannot see. zEventSpy.cpp declares
World::xOGEntity exactly that way and matches.

zDirection was skipped: its file exists and is a gen_accessors one,
so taking it over would move generated functions into the written
column for one function. TriggerPhantom has no Object row at all.

### The 72-byte Init, and what happens when you skip the fill sheet

xBaseInit, then three stores: the asset, an event wrapper, and
asset+N. 14 members, 2 solved. The fill sheet for zCamDrivable read
three holes -- the two halves of the wrapper's address and N -- so a
script was written to pull those two facts for the whole cluster and
the store offsets were taken from the donor.

THAT WAS WRONG AND THE SKELETON SAYS SO. `stw` is a D-form, so
twin_census blanks its displacement: two members storing to different
offsets are one skeleton, and where the members sit is a HOLE. zFog
keeps its asset at +0x3C like the donor; zSlope keeps it at +0x68 and
zSinkingSurface at +0x50. Both came out 1 of 18 words wrong, at
exactly that store.

The fill sheet would have printed it. The script built on top of the
tool would not, because it had already decided which fields mattered.
That is the same failure as SIZE IS NOT SHAPE two commits before --
the tool was right and the query was not -- and it is worth stating
as a rule: IF A FIELD IS BLANKED BY THE SKELETON IT IS A HOLE, and
the only fields the skeleton does not blank are opcodes and
registers. Read the sheet.

Measured after the fix, over all fourteen: the event function is at
+0x30 and the link array at +0x28 in every one, and only the asset's
offset moves.

zPortal needed its class restructured before the Init would fit. It
is 0x40 bytes, exactly what template A's xBase/xOGEntity scaffolding
already occupied, so there was nowhere to put members at +0x28,
+0x30 and +0x3C. Spelled as zPOWObject.cpp does -- xBase forward-
declared only, xOGEntity carrying nothing but its constructor, every
offset riding on the entity -- both its functions match.

Extending four transplant-emitted files by hand took their banners
off, so the four Creates in them moved from the generated column to
the written one along with the four Inits: written 1,376 -> 1,384,
generated 755 -> 751.

### The destructor clusters, and the flag that says base or member

A CodeWarrior destructor takes a hidden second argument, and it is
the whole of what distinguishes the two shapes. `addi r3,r3,N` with
r4 = -1 destroys a MEMBER at +N as a complete subobject; `mr r3,this`
with r4 = 0 destroys the BASE at +0 as a base subobject. Then
`__dl__FPv` runs when the CALLER's flag is positive. So the source is
read straight off the instruction stream: which offsets, which
destructors, and for each whether it is inherited or held.

84 bytes is one member and 96 is two, and 19 of 19 and 16 of 16
members of the two clusters gave up every offset, flag and symbol.
Eight were written and matched: World::VertexDeclEntity at +24,
World::TextureResourceEntity at +32, World::EffectEntity at +28,
xCamBlend at +340 and System::CoreJobProcessor::Slot at +12 in the
one-member shape; zFXScriptSpawnPtMgr::SpawnSlot, FX::zFXSpawn and
OGUpdateList in the two-member one.

EVERY MEMBER DESTRUCTOR IS THE SAME SYMBOL, and that is a fold.
`__dt__12hkBaseObjectFv` is 64 bytes of null test, conditional
operator delete and `return this` -- exactly what a trivial
destructor compiles to -- so every trivial destructor in the image
collapsed onto it and the real type is gone. Spelling the member as
`hkBaseObject` is not a claim about what it was; it is what makes the
relocation name the symbol that survived, which is the only thing
the linked image can be checked against.

A BASE WITH NO DECLARED DESTRUCTOR GETS NO CALL. FX::zFXSpawn came
out 84 bytes against retail's 96 -- one call where retail has two --
because the base was declared with nothing but its constructor. The
sentence that used to stand here said WAD00_17.cpp recorded the same
rule from the other side -- that an intermediate class with no
declared destructor is what STOPS a spurious one being emitted -- and
that is backwards; the next section but one measures it. Here the
base needs one, and it needs to
be VIRTUAL as well as declared: zFXSpawn carries a virtual of its
own, so its vptr is at +0, and a non-polymorphic base would take that
spot and push every member offset by four.

Four of these units were gen_accessors output and taking them over
moved nine generated functions into the written column alongside the
eight added: written 1,384 -> 1,401, generated 751 -> 742. The
alternative was gen_units.py deleting the work on its next run.

### The 68-byte constructor, and where the vtable store lands

Run the base constructor, store the vtable, put ONE constant in ONE
member. 21 members, 8 solved, and all 21 gave up the base, the
vtable, the constant and both offsets.

THE VTABLE'S OFFSET SAYS WHICH SHAPE IT IS. System::Module declares
two members ahead of its first virtual, so anything derived from it
puts its vptr at +0x14 and the constant goes into `events.stage[i]`;
World::Entity is polymorphic from +0, so an entity's vptr is at +0
and the constant is the type id at +0x10. One number in the
instruction tells the two apart before anything else is read.
zSBKelpTrapBehavior is a third: +0x14 like a module, but its base is
nested in zPlantTrap and is not polymorphic at all.

Seven written and matched: HavokModule (stage[1] = 3),
IO::ConsolePadDeviceModule (15), Overseer::CoordinatorModule (99),
UI::Font (type id 14), UI::FontAssetBlobEntity (15),
zNPCGenericSwarm and zSBKelpTrapBehavior.

A DECLARED VIRTUAL DESTRUCTOR IN THE BASE COSTS THREE EXTRA
FUNCTIONS. World::Entity was transcribed from LightKitSceneEntity.cpp
with its `virtual ~Entity()`, and mwcc then emitted an implicit
destructor for Font, for FontAssetBlobEntity and for BlobEntity --
none of which retail has. Spelling slot 0 as a plain virtual instead
removed all three: nothing here calls it, and the vtable is
referenced rather than emitted either way. LightKitSceneEntity.cpp
needs the destructor because it declares one of its own; a file that
does not should not carry it.

AND EVERY BYTE CAN MATCH WHILE THE RELOCATION NAMES THE WRONG THING.
zSBKelpTrapBehavior's constructor came out word-for-word identical
and unitcmp still called it 1 of 17, because the base was written as
an invented `zPlantTrapBehaviorBase` where retail's mangled name says
`zPlantTrap::CustomBehavior` -- nested, not free. report.json cannot
see that: a relocated field's bits are zero on both sides. This is
the third time in this session that a by-name check has caught what
the oracle could not.

Five of these units were gen_accessors output; taking them over moved
five generated functions across alongside the seven added: written
1,401 -> 1,413, generated 742 -> 737.

### The pointer-to-member call, and why a cluster is not a population

One statement: take the address of one of your own members, call it
through the pointer. mwcc puts a twelve-byte constant in `.data`,
copies it onto the stack, points r12 at the copy and branches to
`__ptmf_scall`. Seventeen words, 68 bytes, and the whole of what the
source says is in those twelve bytes: a delta, a vtable offset, and a
function address.

THE CONSTANT NAMES THE TARGET, so nothing has to be guessed. All 24
of the image's 68-byte ones read delta 0, vtable offset -1 (not
virtual) and a third word that is the address of the caller's own
`Set<same name>` or `Destroy`. `Amend<X>` calls `Set<X>`;
`Deactivate` calls `Destroy`. Retail puts each Set immediately after
its own Amend -- AmendRendering at 801D51A0 and SetRendering at
801D51F0, AmendLodEnabled at 801D5380 and SetLodEnabled at 801D53D0
-- so they were written as pairs.

THE SURVEY'S CLUSTER WAS HALF OF IT. twin_census filed 12 of the 24
together and the other 12 elsewhere, because it clusters on opcodes
AND REGISTERS and the two groups use different ones: r6/r5 in the
first, r7/r6 in the second. That is not noise, it is the argument
list showing through. __ptmf_scall adjusts r3 and jumps, so every
argument register is already in place and none is touched -- but the
constant has to be copied through a register the arguments are not
using, and the compiler takes the first free GPR. Nought, one, two,
three and four FLOATS all give r6/r5, because floats go in f1..f4 and
take no GPR at all; one pointer, bool, enum or const reference gives
r7/r6; two give r8/r7. AmendFOVY(float) and AmendRelativeCorner(float,
float, float, float) are the same seventeen words to the instruction.

So the question to ask was not `what else is in this cluster` but
`what else calls this function`. 109 of the image's 23,359 function
symbols branch to __ptmf_scall, in 63 distinct sizes from 44 bytes to
7,220; 24 of them are the 68-byte shape. Twenty-two were written and
matched here. The other two were already written: CMeshBlobEntity.cpp's
Deactivate, which was the donor, and Texture.cpp's
AmendImageFromFileInMemory -- and that second one is exactly why the
survey never offered its half. A cluster with a solved member does not
look like a backlog.

THE LINE TABLE SAYS THE DECLARATION IS A SEPARATE STATEMENT AND THEN
DISAPPEARS. Retail's rows for every one of these are seven over three
lines: the function's own line, the CALL's line, the function's line
again, then three more on the call's, then the closing brace's. The
line between them -- the declaration -- has no row at all. Written as
two statements it reproduces those rows exactly, which is what the
matched donor already did.

A SINGLE QUALIFIER IS WRITTEN BARE, AND TWO ARE NOT. gen_accessors.py
had spelled zMainOGModule.cpp's `Graphics` as a class, because
`HackGetScreenView__8GraphicsFv` cannot tell a class from a namespace
-- CodeWarrior only reaches for `Q<n>` at two qualifiers or more. The
second name in the same unit does tell: AmendAddView is
`Q28Graphics5Scene`, and Scene is a class in namespace Graphics
everywhere else in the image. Changing it to a namespace left both
accessors byte-identical, which is the check that says the rename cost
nothing.

WHERE A FUNCTION LANDED IS THE LINKER'S BUSINESS, AND THE DWARF SAYS
WHICH ONES TO EXPECT TO BE STRANGE. 8 of the 24 are declared in a
header (Scene.h, Renderable.h, Renderable3D.h) and 16 in a .cpp, 0 not
said -- and it is exactly the eight header ones that sit in units with
nothing to do with them. They are inlines this run emitted out of line,
and they came to rest wherever there was room: AmendAddRenderable at
8018F7D0 is in the middle of xtextbox's tag parsers, AmendLOD between
two Graphics::Model functions, AmendColorMulAlpha in zPlatform. The
sixteen declared in a .cpp are all in the unit their own name suggests.
Two of those units had no source file at all until this batch and now
hold one function each of the 154 and the 9 the linker put in them.

Twenty-two written and matched, 1,496 bytes: eleven in Viewport.cpp,
three in RenderCustomizerEntity.cpp, two in WAD03_24.cpp and one each
in zPlatform.cpp, zMainOGModule.cpp, zPhysicsObject.cpp,
ModelInstanceArticle.cpp, Model.cpp and WAD04.cpp. Six of those units
were gen_accessors output, so taking them over moved twelve generated
functions across as well: written 1,413 -> 1,447, generated 737 -> 725.

### Nine more destructors, and the sentence that was backwards

The 96-byte shape is the 68-byte one with a second call: the
null-this test, a member at +N destroyed with the don't-delete flag,
then ONE MORE on `this`, then operator delete when the CALLER's flag
is positive, and `return this`. The second call's flag is the whole
of what says which -- r4 = 0 is a BASE subobject, r4 = -1 is a
complete one and therefore a member at +0. Eleven members, five
solved; nine of the eleven are written here and matched, two at 96
bytes each.

zNGLoadingScreen and zViewport take r4 = -1 twice and so have no base
at all -- two members, the far one declared last because members are
destroyed in reverse declaration order. The other seven take a base.
Three of the nine had no source file: Graphics::StaticBuilder's
destructor is in StaticGeometryEntity.cpp (the unit that first needed
one emitted it), TextureRenderTargetCommon's in RenderTargetWii.cpp
and Scaleform::HeapAllocatorWii's in WAD02_14.cpp.

AND THE INTERMEDIATE-DESTRUCTOR RULE IS THE OTHER WAY ROUND. This
file used to say that an intermediate class with NO declared
destructor is what stops a spurious one being emitted, on the
strength of a paragraph in WAD00_17.cpp that described a state of
that file which was never preserved. WAD00_17 held five EXTRA
symbols, four of them exactly those implicit destructors, with the
intermediates spelled as the note prescribed. Declaring a destructor
on each of Graphics::Node, Graphics::RenderMode, World::Entity and
World::ShaderEntity -- declared, never defined -- removed all four,
and both of the unit's 96-byte destructors came out at retail's own
size with EVERY WORD equal.

The mechanism is the virtual destructor above them. hkBaseObject's is
virtual, so every class under it needs a destructor entry in its
vtable; give a class none and the compiler supplies one AND EMITS it,
declare one and the compiler leaves it to whoever defines it.

WHAT THE NAMES THEN SAY IS `FOLDED`, and that is measured rather than
assumed. The two destructors differ from retail in nothing but the
symbol three relocated branches name: ours reach ~ShaderEntity,
~RenderMode and ~Node, retail reaches `__dt__12hkBaseObjectFv`. That
symbol is 64 bytes of null test, conditional operator delete and
`return this` -- exactly what a destructor with nothing to destroy
compiles to -- so all four intermediates compiled to those same 64
bytes in retail and the linker folded them onto the one survivor.
Their names are nowhere in the image, which is what reloc_audit files
as FOLDED rather than overstated, and it is the same answer WAD03_24
already records for its three empty Setups. reloc_audit went 17 -> 20
and overstated stayed at 0.

Four of these units were gen_accessors output and taking them over
moved six generated functions into the written column alongside the
eleven added: written 1,447 -> 1,464 over 244 -> 251 units, generated
725 -> 719 over 127 -> 123. WAD03_16.cpp was a fifth kind: it still
carried the generator's banner while holding a hand-written asset
Create, so gen_units.py would have deleted that work on its next run.

### Two clusters closed, and what `(no source file)` was hiding

The 84-byte destructor is the one-member shape with a longer prologue:
the null-this test, ONE member destroyed with the don't-delete flag,
operator delete when the caller's flag is positive, `return this`, and
no second call at all -- so nothing it derives from has a destructor.
Eleven solved, eight unsolved, six written and matched here.

The 68-byte module constructor's last six went with them: run
System::Module's constructor, store the vtable at +0x14, put one
constant in one events.stage slot. `events` starts at +4, so the store
at +4 is stage[0] and the one at +8 is stage[1] -- the donor
HavokModule is the only one of the sixteen that uses stage[1].
Memory::AllocModule takes 35, Domains::DomainModule 67,
Graphics::RenderStateModule 128, Graphics::SceneGraphModule 1999, and
IO::MediaModuleLFS takes 3, which is the donor's own constant and why
its fill sheet shows no immediate hole at all. World::CurveEntity is
the sixth and is not a module: its vptr is at +0 and its constant is
the type id 18 at +0x10, which is the offset telling the two shapes
apart before anything else is read.

`(no source file)` WAS NOT A BLOCKER, AND IT ACCOUNTED FOR EIGHT OF
THE TWELVE. Every one of those units already had its `Object(...)` row
in configure.py; what was missing was the .cpp, and creating it is one
file plus a `python configure.py`. Two 84-byte destructors and all six
of the remaining 68-byte constructors were sitting behind that marker,
and the previous pass had read it as a reason to skip them. Seven
files created, eight functions, 576 bytes. Check configure.py before
believing the survey's marker.

ONE CONSTRUCTOR HAD TO BE RE-SPELLED TO GET ITS DESTRUCTOR.
xDecal::decal_instance's constructor was already matched as
`{ f54 = 0; }`, and the destructor needs +0x54 to be a
World::xOGModelRefPtr so the branch reaches that type's own
destructor. Giving the member its type and the type a default
constructor that nulls its one word -- the layout xOGModelRefPtr.cpp
already records from the DWARF -- leaves the constructor emitting the
same `li r0,0; stw r0,84(r3); blr` it did before. Both are
byte-identical now; the accessor was not damaged to get the
destructor.

TWO ARE LEFT AND THEY ARE THE ONES anon_blocked.py NAMES.
`@unnamed@WAD00_cpp@13xMemWatermark` and
`@unnamed@WAD00_cpp@16intersect_env_CB` live in anonymous namespaces,
and CodeWarrior mangles an anonymous namespace with the name of the
TRANSLATION UNIT it compiled. Split out of the blob, ours would carry
`@unnamed@WAD00_31_cpp@` and never pair. Two ways round it are already
measured and excluded: `#line` does not move it (the mangler reads the
real input filename), and naming our file WAD00.cpp collides with the
parent unit and dtk refuses it. They are reachable as part of the
whole blob and not before.

Four of these units were gen_accessors output and taking them over
moved 33 generated functions into the written column alongside the
twelve added: written 1,464 -> 1,509 over 251 -> 262 units, generated
719 -> 686 over 123 -> 119.

### 12%, and four refusals that were holding 93,368 bytes

The clusters had 15,672 unsolved bytes left in them and the target was
7,652, so the clusters were not the answer. Asking a different
question was: 395 functions in the image carry `AnimTable` in the
name, 125 of them were matched and 270 were not, and those 270 carry
93,368 bytes. There has been a generator for that shape
(tools/gen_animtables.py) since the first tables were written.

IT REFUSED ALL 158 BRANCHLESS ONES, and the refusals were four and not
158. Counting them first is what made that visible -- one script over
every unmatched table, printing which callee blocked how many callers
and how many bytes -- rather than reading the first refusal and
concluding the shape was hard. `NewStateMany`, the first refusal seen,
blocks 2 callers and 228 bytes; the four that mattered are:

  * A CALL TO A MEMBER FUNCTION could not be spelled at all. The
    tool knew `xAnimTableNewState`, `xAnimTableNewTransition` and the
    inlined forms of three helpers, and anything else that was a
    member went to `problems`.
  * A SIGNATURE OUTSIDE A TWO-ENTRY TABLE was refused as `not a table
    signature`, which is every `AddTransitionsFrom` in the game --
    nine parameters instead of one.
  * Q<n> QUALIFIED TYPES did not parse, so
    `Q213zPlayerAction14SpecialActions` stopped the signature that
    carried it.
  * ONLY r4 AND r5 WERE SEEDED as incoming arguments, so a table that
    FORWARDS its own parameters to the call reported five unresolved
    registers. `AddTransitionsFrom` forwards six.

None of the four is a check being weakened. Each still ends in a
refusal when the tool cannot read what it needs, and one of them is a
check of its own: whether a member call is STATIC is read from r3 --
`this` means non-static and arguments from r4, anything else means the
callee's first argument is already there -- and reading it wrong
changes the argument COUNT, so it cannot pass silently.

THE SAME HELPER IS INLINED IN ONE UNIT AND NOT IN ANOTHER, and that is
the fact underneath all of it. zSBPlayerActions.cpp reaches NewState,
AddActionTransition and the manager's AddTransitionsTo family through
the inline spelling, because retail's unity build inlined them there,
and the file defines them `inline` so our fragment does the same.
zCommonPlayerActions.cpp's tables `bl` straight to those same four
symbols, and so do 62 more in WAD01_28. A unit of the second kind
needs the identical zPlayerAction stub with the helpers DECLARED and
left undefined. The bytes say which, unit by unit; nothing else does.

122 tables merged over three units, 113 of them byte-identical:
61 of 62 in WAD01_28.cpp, 31 of 34 in zSBPlayerActions.cpp and 21 of
26 in zCommonPlayerActions.cpp. Game Code 10.6385% -> 12.1668%,
225,176 -> 257,524 bytes, 2,195 -> 2,308 functions -- 32,348 bytes for
four rules and two stubs.

THE NINE THAT DIFFER ARE NOT EXAMINED and the pins say so: they are
`AddActionTransitions` and `AddStates` bodies with many calls, merged
by the tool and left at what it produced, and the two zSBPlayerActions
ones that were already differing before any of this. A pin of
(47, 52) rather than (47, 47) is what keeps them visible.

What is left of the seam, measured after: 72 of the 270 have a branch
in them, which the tool refuses as a different shape, and the rest are
in units that need the same stub added.

### 14%, a second generator, and the check that took 640 bytes back

Three seams, and the largest of them was one two-character token.

`PUs` STOPPED 18,252 BYTES. gen_animtables.py reads a mangled
parameter list, and its pointer rule wants a digit after the P
because it reads a class name by its length. `Pf` and `PUs` fell
through it, and `PUs` is in zPlayerAction::NewState's signature, so
every one of the 53 tables that calls NewState was refused --
counted, correctly, as `cannot read the signature`. One rule of ten
lines. That is the second time this session that counting the
refusals rather than reading the first one was worth five figures.

THE 116-BYTE ANIMATION CALLBACK, and a generator of its own
(tools/gen_animcb.py). An `an<X>Check` that a table passes as a
transition callback is 29 instructions and says one thing:

    unsigned int r = 0;
    if (owner->_v5())
        if (owner->X(t, s))
            r = 1;
    return r;

`owner` is `((AnimCBHolder*)a)->slot->owner`, +4 then +0x90, RE-READ
for the second call rather than kept. Slot 5 is (28 - 8) / 4 of the
`lwz r12,28(r12)`. The first was written by hand and matched on the
first compile; the generator then verifies every candidate WORD FOR
WORD against it, so the slot, the offsets and the branch structure
are checked and not assumed.

TWO SPELLINGS, AND THE BYTES SAY WHICH. 35 of the 207 take the chain
off a1 rather than a0 -- `lwz r5,4(r4)` where the others have r3 --
which is the chain the image's short forwarders already use. Reading
those two words is what tells them apart; a function whose two reads
disagree is refused. 203 written, 23,548 bytes, over four units.

THE 80-BYTE BASE-ONLY DESTRUCTOR: null test, the BASE's destructor
with the flag CLEAR, operator delete when the caller's flag is
positive, `return this`, and no member call at all. Spelled
non-virtual throughout -- the call is a direct `bl` either way, and a
virtual destructor would make the unit the home of a vtable retail
keeps elsewhere. 20 written of the 43 the image has.

AND EIGHT OF THEM WERE TAKEN BACK OUT, which is the part worth
keeping. The GFx and Scaleform ones came out with EVERY WORD equal
and report.json credited all eight -- and reloc_audit called them
OVERSTATED: retail's `operator delete` on a GFx class is
`Free__11GMemoryHeapFPv`, the class's own, where ours reaches the
global `__dl__FPv`. The bits agreed and the call did not. That is
precisely the case the relocation check exists for, and it is the
difference between 14.03% and 14.01%: 640 bytes given back because
the only tool that could see the difference said so. They are
reachable, and what they need first is the class's own operator
delete -- the GNewOverrideBase idiom -- not a different destructor.

Game Code 12.1668% -> 14.0084%, 257,524 -> 296,504 bytes, 2,308 ->
2,578 functions.

### The GFx operator delete, and a filter that hid 47 functions

A GFX CLASS DELETES THROUGH ITS OWN OPERATOR, and a one-line one is
INLINED. The eight base-only destructors handed back last time --
every word equal, credited by report.json, called overstated by
reloc_audit because retail branches to `Free__11GMemoryHeapFPv` and
ours reached the global `__dl__FPv` -- come back with six lines:

    class GNewOverrideBase {
    public:
        static void operator delete(void* p) { GMemoryHeap::Free(p); }
    };

-inline auto takes it at the call site, so the heap's Free lands in
the destructor itself rather than a call to the operator. All eight
match and reloc_audit stays at 0 overstated. Four more went with them
whose base is a TEMPLATE instantiation -- `26GRefCountBase<8GFxState,
2>` is GRefCountBase<GFxState, 2>, which GFxState itself derives
from, the CRTP the Scaleform headers use.

AND A FILTER OF MY OWN HID 47 OF 72 TABLES. The driver that offers
symbols to gen_animtables had its own `branchless()` test, and it
counted `bctrl` as a branch. gen_animtables does not -- it HANDLES a
vtable slot call, and its own test is exactly `bc` and an
unconditional `b`. So every table whose only `branch` was a virtual
call was filtered out before the tool ever saw it, and the 72 recorded
here as `has a branch, which the tool refuses as a different shape`
were 25. The rule in CLAUDE.md is about logs -- `a filter returns only
what was already suspected` -- and it applies just as well to a filter
in front of a tool. Ask the tool what it refuses; do not re-implement
its test.

WHAT IS ACTUALLY LEFT, measured rather than estimated, because the
next target asked for 40,518 more bytes and the seams do not hold
them:

  * every mixed cluster twin_census finds: 449 unsolved members,
    18,400 bytes, and the big ones are the recorded hard remainders;
  * the largest single repeated opcode signature among the 7,743
    unmatched functions of 3,000 bytes or less: 28 functions, 4,480
    bytes. The top ten signatures together are 13,812;
  * 7,743 unmatched functions carrying 1,637,928 bytes across more
    than 5,388 distinct signatures.

So there is no 40 KB seam. The three that existed -- the animation
tables, the 116-byte callbacks and the destructor shapes -- are worked
out to what their generators can verify. Past here the bytes come one
function at a time, out of the two player-action units that hold
260,000 bytes between them, and that is decompilation rather than
transplanting.

### The asset Fix, a walker instead of a template, and a HELD file

`shape_census.py`'s biggest remaining row was 28 functions of 160
bytes, all called `Fix__Q24Sext<something>AssetFl`, all in
WAD00_32.cpp. An asset's Fix relocates what the asset owns and then
walks its event links:

    CustomFix(base);
    <0..n>  member.Fix(base);
    <0..1>  other = (void*)((long)other + base);
    p = (EventLinkNew*)((long)links + base);
    links = p;
    end = p + linkCount;
    while (p != end) { RTTID_Fix<T>(&p->src, base); ... p++; }

`end` IS DECLARED BEFORE THE CURSOR. With the cursor first, fourteen
of the forty words come out with r30 and r31 swapped -- the cursor
takes r30 and `end` takes over the register `this` was in. Three
spellings were compiled to settle it, and the fourth question that
kind of difference asks is always the same one: which variable gets
the lower callee-saved register, and the source is what decides.

A TAIL MATCH BEATS A TEMPLATE MATCH. The first version measured a
candidate against a whole 160-byte template and took 28; the family
is 103 functions over a dozen sizes, and what they share is the TAIL
-- everything from the link count onward, in one of two shapes. The
head is a run of `mr r4,r29 / addi r3,r31,N / bl Fix__<T>Fl` triples,
one per sub-object the asset owns, and however many there are is read
rather than assumed. That took 77 of the 103, and every one of them
is still checked word for word against its tail.

AN EMPTY CLASS IS ONE BYTE, NOT FOUR, and that is what put every
member after the first three bytes early: `addi r3,r31,0xED` where
retail has 0xF0. The sub-object stubs carry nothing but a Fix, so
each occupies one byte, and the padding between them is measured
against that.

AND THE BODIES GO INSIDE `#pragma dont_inline`. Without it mwcc takes
Util::RTTID_Fix<T> -- one line -- and the branch reaches T::Fix
directly where retail reaches the wrapper. Every word is still equal
and report.json credits it; reloc_audit is what caught it, again. The
wrapper for a T new to the unit has to be instantiated OUTSIDE that
block, or the inlining it DOES want does not happen either --
DTRMovieSettings' wrapper is sixteen bytes because its own Fix is
taken into it.

A THIRD STATE FOR A GENERATED FILE: HELD. gen_units.py and
written_vs_generated.py both key on the gen_accessors banner and want
opposite things from it here. The accounting is right to call
WAD00_32.cpp generated -- every one of its 255 functions came from a
generator -- but gen_units would regenerate it and delete 78 of them.
`// HELD: another generator has added to this file` says so: gen_units
leaves it alone, the accounting is unchanged, and nothing had to be
weakened to get there.

255 of 255 in that unit, up from 177. Game Code 14.0857% ->
14.7184%.

Two things that are NOT levers, measured rather than assumed: which
overload of a name gets picked (CodeWarrior mangles static and non-static
members identically -- read the registers instead, see Blobloids.cpp), and
operand order in a commutative expression.

## A BOOL IS ARITHMETIC OR IT IS A FLAG, and the bytes say which

The twenty-six predicates zBoardPlayerAction and its two powerup
actions are made of are all the same four lines of C++ with different
members in them, and the first twelve went in at three of twelve. Every
one of the nine misses was one of five spellings, and each of the five
is readable in the bytes BEFORE a line is written.

**`addic rX,rY,-1 ; subfe rX,rX,rY` is an int being converted to a
bool as a VALUE.** Not a branch -- the value ends up in a register and
the branch, if there is one, comes from the record form. Count them:
anBoardMoveCheck has three, which is one per `||` operand plus one for
the conversion on the way out, and that is the whole shape of the
function. `return a || b;` does NOT give it: mwcc sets a flag register
from two branches instead, which is eight bytes and a saved register
wrong. What gives it is the value spelled as a value --

```cpp
unsigned int moved = BoardWalkCheck(a0, a1) != 0;

if (!moved) {
    moved = BoardRunCheck(a0, a1) != 0;
}

return moved;                 // the bool return is normalise #3
```

-- and the operands have to be int-returning for the first two to
exist at all. mwcc knows a bool-returning call is already 0 or 1 and
skips the conversion, so a `bool BoardWalkCheck` costs two of the
three. The return type is not in the mangled name, so it is free to
change, and here the CALLERS are what fix it.

**`cntlzw ; srwi. r0,r0,5` on `x - k` is `x == k` as a value**, the
same idea one step earlier. KnockbackFrontCheck is `bool hit = lastDamageType
== 9; if (hit) { hit = LaunchFrontCheck(a0, a1) != 0; } return hit;` --
no saved register, no flag, r0 all the way. Spelled `return lastDamageType
== 9 && LaunchFrontCheck(a0, a1);` it saves r31 and sets a flag: eight
bytes and fifteen of seventeen words.

**The left operand of an `&&` is normalised too if it returns int.**
BoardQuicksandMoveCheck was one word from exact with a spurious
`addic ; subfe` right after `bl IsOnQuicksand`, and the fix was the
callee's declared return type -- `bool IsOnQuicksand()` gives the
`cmpwi r3,0 ; beq` retail has. Two functions in the same file, one
wanting bool and one wanting int, and the only evidence is which of
the two sequences the caller holds.

**A materialised flag is a function boundary.** `li r30,1` before a
call and `li r30,0` after it is a bool mwcc had to keep, and mwcc only
keeps one when the value crossed a call it inlined. Where retail has a
flag and the obvious source does not, retail called something. Nine of
the twenty-six here forward to a member with no symbol of its own:
anBoardQuicksandStopCheck is the standard wrapper around an inlined
`BoardQuicksandStopCheck`, and BoardWalkCheck is `!BoardRunCheck() &&
!BoardStopCheck()` with both taken in -- which is why it reads the
input magnitude twice.

**`#pragma always_inline on` goes around the CALL SITE.** Both
invented members grew past what `-inline auto` takes and mwcc emitted
them out of line -- unitcmp says EXTRA, NOT IN RETAIL, which is the
check that catches it. The pragma around the three callers inlines
them and the out-of-line copies stop being emitted; around the
DEFINITION it does nothing, because it is a property of the call.

**And a local is not a cast.** `((zBoardPlayer*)player)->playerInput->
GetMag() >= ((zBoardPlayer*)player)->GetRunStartMag()` loads the
player, calls, and loads it AGAIN, because mwcc cannot prove a call
leaves `this->player` alone. Retail loads it once and takes the input
pointer BEFORE the call:

```cpp
zBoardPlayer* p = (zBoardPlayer*)player;
zPlayerInput* input = p->playerInput;

return input->_v27(0, 2) >= p->GetRunStartMag();
```

Two locals, four words, exact. And BoardWalkCheck needs the pair
TWICE, in two scopes, because retail re-derives both for the second
threshold -- one pair held in registers would not.

STILL UNREACHED, with its mechanism recorded: GainSidekickPowerupCheck
(52 B). Retail tests `>= 9` and `<= 11` with two signed compares;
every spelling of a range on one variable folds to `addi r0,r3,-9 ;
cmplwi r0,2` -- one expression, three nested ifs, and three equalities
joined by `||` were each tried, and all three fold. It is the same
fold that holds zSBPlayerActions' PowerupStateCheck, so the answer is
worth two functions and not one.

Twenty-five of twenty-six, 2,332 bytes. WAD01_28 309 of 310. Game Code
16.6403% -> 16.75%.

## THE LAST LOCAL DECLARED TAKES r31, and three other orderings

Nineteen more of the board player's predicates -- the whole
zPlayerHitBoard family, the run and walk checks, three of the idle
ones.  Most went in first time on the levers the previous section
found.  Four did not, and each was a question about ORDER rather than
about shape.

**The last local declared takes r31; the rest take r30 downward in
declaration order.** HitElectricArcCheck was eight words out of 31 and
every one of them was a register number: `p` in r31 where retail has
r30, `result` in r30 where retail has r31.  Moving `bool result` above
`zBoardPlayer* p` fixed it exactly. The same move fixed RunBraveCheck
and RunSuccessCheck, which want result, notSlippery, p, run -- p third
of four, which is not where anyone would write it, and is what the
bytes say.

**A threshold read from memory wants its own local.** WalkCheck's goo
branch is `input->GetMag(0, 2) >= p->fA78`, and retail loads fA78 into
f31 BEFORE the virtual call.  Written as a member read it lands after:
the player is still live, so mwcc keeps it in a callee-saved register
and defers the load, which costs a fifth saved register and eight
bytes of frame. `float mag = p->fA78;` on its own line puts the load
where retail has it and lets the player die into r5.

**`bool ok = A && B;` keeps the flag; `if (A && B) ok = true;` folds
it.** The two are the same program and mwcc treats them differently:
the assignment materialises the 0/1 the way retail does, and the `if`
lets mwcc prove the flag dead and thread the branches straight into
the next test.  That is worth 16 bytes and four words on a 144-byte
predicate, and it is the difference the previous session's three-flag
chains hid -- with three the fold does not fire, so the `if` form
happened to be right there.

**And the pragma is a region, not a call.** `#pragma always_inline on`
around IdleSlipperyCheck took the invented helper AND DefaultStateCheck,
whose definition was already above it.  Moving DefaultStateCheck to the
end of the file puts it out of every inliner's reach and leaves the
pragma one thing it can take.

STILL UNREACHED, and one step short: Run- and WalkSlipperyCheck
(144 B each).  With `bool ok = A && B;` they are retail's size with
the right instruction in every slot; what is left is the COLOURING.
`ok` dies at its test, so mwcc puts `result` back into r31 and saves
four registers, where retail saves five and holds `result` in r30 from
the top.  Six spellings of the second half were measured: every one
that keeps the size reuses the register, and every one that splits
them folds the flag.  zPlayerIdleBoard's IdleSlipperyCheck is the same
function again, and IdleRegularCheck (304 B) inlines it, so the answer
is worth four functions and 736 bytes rather than one.

Nineteen functions, 2,420 bytes.  WAD01_28 328 of 329. Game Code
16.75% -> 16.86%.

## TWENTY-ONE CALLBACKS, AND FOUR THINGS TO READ OFF EACH

The transition tables call 116-byte static callbacks, and WAD01_28
holds about sixty of them.  Twenty-one went in here, and the whole of
reading one is four questions:

**Which parameter carries the holder.** `lwz r5,4(r3)` is a0 and
`lwz r3,4(r4)` is a1, and this unit uses both -- anSprayCheck takes
a0 and anSprayEndCheck, its own mirror image, takes a1.  Nothing but
the register says which.

**Whether there is one flag or two.** One is `_v5() && <test>` with
the test written out; two means the member it forwards to was inlined,
and the inner flag is that member's own `result`.  Four of the
twenty-one are the second kind, and they go ABOVE the block that
defines what they call, because `#pragma always_inline on` is a region
and would otherwise take BoardStopCheck in as well.

**Whether the second test is a branch or a value.** `cmpwi r0,0 ; beq`
is a branch and belongs inside the `if`; `cntlzw ; srwi.` on a call
result, or `mfcr ; rlwinm.` on a float compare, is a bool built in a
register and then tested, which is a bool LOCAL.  Three of the
twenty-one wanted the local and were four words short without it.

**And what the tail is.** `bctr` with no epilogue after it is a tail
call -- `return owner->player->_v74();` and nothing else.  `addi
r0,r3,-K ; cntlzw ; srwi` with no record bit is `== K` returned
directly; two of the 28-byte ones are one expression each.

The same four questions cover zPlayerHitBoard's whole family, the
cheat and spray pairs, the two hammer interrupts, and five of
zBoardPlayerBungeeBall's six.

STILL UNREACHED: anSBBungeeBallHitCB (96 B), and the eight bytes that
are wrong are a `li r3,0` retail does not emit -- its false path
branches straight to the epilogue with r3 already zero from the
`cmpwi`.  Four spellings were measured (return-1, a `result` flag, an
early `return 0`, and an inlined member behind `always_inline`) and
none of them drops it.

Twenty-one functions, 2,272 bytes.  WAD01_28 349 of 350. Game Code
16.86% -> 16.97%.

## AN `||` CHAIN IS A BITMASK AND A SWITCH IS A TREE -- 17% of Game Code

Eighteen accessors and predicates off the bottom of WAD01_28's
remaining list, and two of them were worth more than their bytes.

**`a == 1 || a == 2 || a == 6` compiles to a bitmask; the switch with
those three cases compiles to a decision tree.** IsInAnyGooState is
`addi r4,r4,-1 ; cmplwi r4,5 ; bgtlr ; li r0,1 ; slw r0,r0,r4 ;
andi. r0,r0,0x23`, which reads as a jump-free switch and is not one:
written as a switch mwcc emits a range test for 1 and 2 and an
equality for 6, four bytes short.  The mask is what the `||` chain
gives, and the mask NAMES THE CASES -- 0x23 is bits 0, 1 and 5, and
the `-1` in front makes those 1, 2 and 6.  So a mask in the bytes is
a list of equalities in the source, however much it looks like a
table.

**`cmpw` and not `cmpwi` is a member compared against another
member.** SpinPowerupCheck tests `powerupState == 2` and then
`powerupModelState == powerupState` -- and the second value IS 2 on
that path, so `== 2` compiles and runs identically and is four bytes
wrong.  The register in the compare says which of the two the source
wrote.

The rest are the leaf idioms already recorded: `beqlr`/`bnelr` pairs
for an `&&` with no frame, the surface pointer at +0x6C with a type
word at +0x10 and a friction float at +0x38, and two returns of a
constant float chosen by one compare.

STILL UNREACHED, and now measured six ways: AimPuckCheck (28 B).
Retail has two exits with the FALSE one first; `return x != 0;`,
`if (x) return true; return false;`, `if (x == 0) return false;
return true;` and that last one with an explicit `else` all fold to
the 16-byte `cntlzw ; srwi` form.  It and GainSidekickPowerupCheck
are the two in this unit whose difference is mwcc folding where
retail did not.

Eighteen functions, 764 bytes.  WAD01_28 367 of 368. Game Code
16.97% -> 17.01% -- 359,988 of 2,116,616 bytes, 3,011 of 10,697
functions.

## COUNT THE CONVERSIONS AND THE TYPES FALL OUT

Nineteen more: seven Begin and End bodies, seven jump-fall-land
predicates and five of zBoardPlayerBungeeBall's.  Twelve went in
first time; the other seven were all the same question, and it is
worth stating as a procedure rather than as a lever.

**Count the `addic ; subfe` pairs and assign the types backwards.**
Each one is an int being made into a bool, and every one has to come
from somewhere in the source:

  * one on a CALL RESULT means that callee returns int, not bool --
    which is how BoardFallCheck, BoardLandCheck and SBJumpCheck got
    their return types before any of them was written;
  * one at the RETURN means the function returns bool and the value
    it returns is an int local;
  * and one that is NOT there means the two types already agree.

LandRunCheck was 7 of 31 words out with an extra pair on the way out,
and the fix was `bool ok` where `unsigned int ok` had been: the
comparison that assigns it is already 0 or 1, so a bool local needs
no conversion and an int one needs it at the return.  FallMovingCheck
wanted the opposite -- its fourth conversion is INSIDE the `if`, which
is an unsigned inner variable assigned to a bool outer one.

**And an `if` whose false arm returns the value already in r3 costs
nothing.** The four magnitude checks are `bool ok = <call> != 0;
if (ok) { ok = <compare>; } return ok;` -- one variable that the
second test ASSIGNS.  Written as `if (<call>) { return <compare>; }
return false;` mwcc emits a `li r3,0` retail does not have, four
bytes each, because it no longer knows the false path already holds
zero.

The order rule from the last section applied twice more: the two
TransToSpinPowerupChecks call a 28-byte predicate that was already
defined above them and got it inlined, so they moved above the block
that defines it.

Nineteen functions, 1,548 bytes.  WAD01_28 386 of 387. Game Code
17.01% -> 17.08%.

## A LOCAL CAN BE DECLARED AFTER A STORE, and the bytes say it was

Fourteen more -- the four *MovingChecks, the two magnitude getters,
the powerup timer, the five zBoard*BE callbacks and two zBoardPlayer
members.  Thirteen went in first time on the conversion-counting
procedure from the last section; the fourteenth was one line in the
wrong place.

**SetPowerupTimerToMax stores the member BEFORE it declares the local
it sends.** Retail is `stfs f0,2232(r3)` and then `stfs f0,12(r1)`,
both from the same register; written with the local first the two
stores swap and four of thirty-two words are wrong.  So:

```cpp
f8B8 = -1.0f;

float param = -1.0f;        // declared AFTER the store

zEntEventAllOfType((xBase*)this, 0, 0x373264EF, ...);
```

and the two branches use different stack slots -- 12(r1) and 8(r1) --
which is two locals in two scopes and not one hoisted above the `if`.

**Five callbacks, one number.** zBoardHitByHammerBE and its four
siblings are the same eight lines with 8, 0, 6, 7 and -- for
GainPowerupProp -- the powerup state itself, which is a `lwz` before
the `stw` where the others have a `li`.  The third parameter is the
player: `mr r31,r5` and every offset off r5.

Fourteen functions, 1,308 bytes.  WAD01_28 400 of 401. Game Code
17.08% -> 17.14%.

## THE CONSTANT ON THE LEFT: a range that does not fold

`x >= 2 && x <= 4` compiles to `addi r0,x,-2 ; cmplwi r0,2 ; bgt` --
one unsigned compare instead of two signed ones -- and retail has the
two.  That fold has stood since the previous session and has been the
recorded reason for three separate near-misses.

**`2 <= x && x <= 4` does not fold.** The constant on the LEFT of the
first comparison, and mwcc emits `cmpwi r0,2 ; blt ; cmpwi r0,4 ;
bgt` -- exactly retail.  It is not a commutation: `2 <= x` and
`x >= 2` are the same predicate written with the operator reversed,
and mwcc's range recogniser only fires on one of the two spellings.

The same move works on the negated form: `3 > x || 5 < x` keeps the
two compares where `x < 3 || x > 5` folds.

Everything else had been measured first and none of it worked:
chained `&&`, three nested `if`s, three equalities joined by `||`, a
switch over the three contiguous cases, and the whole thing on one
line or on three.  Nine spellings across two sessions, and the tenth
was two characters.

That closed zPlayerDefeatedSB::PowerupStateCheck (40 B),
zSBPlayerFillWithGoo::FillWithGooFrom100Check (96 B) and --
reapplied across the file -- zBoardPlayerGainPowerup::
GainSidekickPowerupCheck (52 B), which had been the last unmatched
function of WAD01_28's 416.

**WAD01_28 is now 416 of 416**, and zSBPlayerActions 351 of 353.  The
note in `Traps worth knowing` that operand order in a commutative
expression is not a lever still holds -- a comparison reversed is not
a commutation, and this is the case that shows the difference.

Thirty-three functions across the two files, 2,384 bytes.  Game Code
17.25% -> 17.30%.
