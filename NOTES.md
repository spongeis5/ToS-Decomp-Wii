# Truth or Square (Wii) -- working notes

What has been established, what is still open, and the reasons. Figures are
from `ninja` and `build/R8IE78/report.json`; re-run rather than trusting the
numbers here, which move.

## State at time of writing

```
Game Code:  67 of 777 files complete  111,068 / 2,116,616 bytes  1,246 / 10,697 fn
            5.2474% of game code

Of those 1,246 functions, 765 are GENERATED -- machine-recognised
shapes, not one of which is decompiling. They are real matched
functions and the offsets and constants are recovered fact, but a
count of them is not a count of decompiled code. HAND-WRITTEN IS
481, across 118 units and 102,356 bytes, and that is the figure to
compare against earlier ones.

Data:       4 unit(s) carry their own, 412 bytes; 134 more could
All:        3.41% matched              main.dol reproduces byte for byte
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
| `shape_census.py` | what the unmatched short functions LOOK like, as a population, by opcode signature |
| `unitcmp_pins.py` | re-measure `unitcmp_check`'s pins; refuses to lower one |
| `written_vs_generated.py` | the split, from the banner in each source file |
| `notes_state.py` | writes the State block at the top of this file; `--check` fails when it is stale |
| `next_functions.py` | what is left ranked by functions rather than bytes |
| `gen_rttid.py` | the RTTID_Fix<T> family -- 175 functions from one template; `--survey` |
| `reloc_audit.py` | which already-matched functions branch somewhere retail does not |
| `disasm.py` | read one retail function, symbols resolved; `--unit`. 100% of the splits decode |
| `compiler_sweep.py` | rebuild every unit with source under each Wii compiler and count exact functions; `--lib PREFIX` |

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

## FixWmlType: the dispatch MATCHES, 2,979 of 2,986 instructions align

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

**Where it stands.** 307 of 307 comparisons in order; **2,979 of
retail's 2,986 instructions align**, 99.06%. The unit's other
function, `RTTID_Fix<Sext::DTRMovieSettings>`, IS byte-identical (16
bytes): retail's instantiation is `lwz, add, stw, blr`, so that class
gets a defined inline `Fix` rather than a stub declaration, or the
template emits a call.

**The seven instructions that differ**, and they are two shapes:

  * **Five re-loads.** Retail loads a field, tests it, branches, and
    then LOADS IT AGAIN before adding to it; we keep the tested value
    in a register. It happens in three bodies, and in two of the five
    there is a free register and nothing at all between the test and
    the use, so it is not the allocator running out of registers.
  * **Two orphaned branches.** Retail carries two `b epilogue`
    instructions that directly follow another `b epilogue` and that
    nothing branches to, after the bodies for 44FBB98A and 9761A7DD.
    432 branches reach the epilogue in retail, 430 in ours.

Plus one allocation difference that costs no bytes but does change
them: retail saves r28-r31 and uses r28/r29 for the loop in body
`8004AE5C`, where we save r29-r31 and reuse r31.

**What has been RULED OUT for the re-load**, so the next attempt does
not repeat it. None of these changed the output by a single
instruction:

  * 12 spellings of `if (X) X += l;` -- `!= 0`, `X = X + l`, a local
    `long*`, braces, an inverted test;
  * 7 pointer types for the read and the write independently --
    `long*`, `char**`, `void**`, `int*`, two distinct class types at
    the same offset, and the whole file converted to `char**`;
  * 4 helper forms -- a `long&` parameter, a `long*` parameter, a
    `char**` parameter, an inline member function;
  * 13 optimisation settings -- `-O0` through `-O4`, `,s` and `,p`,
    `-inline off/auto/all`, and `-opt nocse`, `nolifetimes`,
    `nodeadstore`, `noprop`, `nostrength`, `noloop`, `nodeadcode`,
    `nopeep`, `noschedule`;
  * all 28 installed compilers -- every Wii version gives exactly the
    current result, and the GC 3.0 alphas are worse.

The one thing that DOES reproduce it is `volatile` on the read in the
test, which recovers four of the five and takes the alignment to
99.43%. That is not plausible source for a pointer-fixup routine, so
it is recorded as evidence about the shape of the answer -- the value
genuinely is dead after the compare in retail -- and not used.

Likewise the two orphaned branches: an empty case leaves NOTHING
behind (tested -- inserting one changes no instruction), and mwcc
folds `return; break;`, `break; break;` and `return; return;` down to
a single branch. Something in those two cases emits a control
transfer the compiler cannot see is redundant, and it is not any of
those.
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

3. **FixWmlType's last seven instructions.** 11,944 bytes in one
   function, generated by `tools/gen_wmltypes.py`. The dispatch
   matches and 2,979 of 2,986 instructions align. What is left is
   five re-loads and two orphaned branches; the section above lists
   the 64 things already ruled out, so do not start there.

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
