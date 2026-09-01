# Truth or Square (Wii) -- working notes

What has been established, what is still open, and the reasons. Figures are
from `ninja` and `build/R8IE78/report.json`; re-run rather than trusting the
numbers here, which move.

## State at time of writing

```
Game Code:  28 of 777 files complete  24,568 / 2,116,508 bytes  993 / 10,686 fn
            1.1608% of game code

Of those 993 functions, 864 are GENERATED -- machine-recognised
shapes, not one of which is decompiling. They are real matched
functions and the offsets and constants are recovered fact, but a
count of them is not a count of decompiled code. HAND-WRITTEN IS
129, across 40 units and 13,352 bytes, and that is the figure to
compare against earlier ones.

Data:       3 unit(s) carry their own, 396 bytes; 134 more could
All:        2.06% matched              main.dol reproduces byte for byte
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
| `disasm.py` | read one retail function, symbols resolved; `--unit`. 100% of the splits decode |

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

**`zNPCUPGeneric`'s other two.** `Activate` (536 bytes) and
`SystemEvent` (200) are written and do not match yet; five of the
eight functions the unit's object now defines do, and the eighth is
CreateAnimTable above. `InitTypeParameters` needs
`Memory::Creator<N,T,U>` with a function-local static, whose guard
symbol has to come out as `@GUARD@...@_inst`, and `Initialize` calls
`World::EntityManager::FindAsset` through a loaded address rather than
a direct branch, which no plain call reproduces.
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
   sitting and five were byte-identical on the first compile. Units
   within three functions of complete, measured after this session:

   | bytes | left | unit |
   |---|---|---|
   | 3,904 | 3 of 3 | `zSBPlayerCharacterProxyCollisionListener` |
   | 2,672 | 3 of 3 | `SB/NG/Engine/WAD01_13` |
   | 1,956 | 1 of 1 | `Graphics/Space` |
   | 1,920 | 1 of 1 | `x/xModelOpt` |
   | 1,584 | 3 of 3 | `zBTSet` |
   | 1,468 | 2 of 2 | `x/xParabola` |
   | 1,092 | 2 of 2 | `zAnimList` |
   | 928 | 2 of 2 | `zPathFinderSearchMapLinkCostCalculator` |

   Re-derive the list rather than trusting it: it is a query over
   report.json for game units whose unmatched functions are few,
   EXCLUDING any symbol named `pad_*`. Those are dtk's alignment
   padding, there are 95 of them, and a ranking that leaves them in
   claims 146 units are one function from complete when they are one
   PADDING WORD from it.

2. **Make a matched unit a linked one.** `complete` only moves when
   `configure.py` marks the unit `Matching`; until then dtk links the
   carved object and ours is merely compared. Three went through this
   session and main.dol stayed byte-identical (25 -> 28 complete). The
   ones that did not need their DATA: a linked unit has to SUPPLY its
   file-scope statics, not just reference them, which is what
   `dwarf_data_carve.py --survey` (75 units) is for. `zLaserScanner`
   links but shifts main.dol -- it is the only one of the three with a
   float literal, so its `.rodata` pool lands somewhere retail's does
   not. ALWAYS read `main.dol: OK` after flipping one.

3. **FixWmlType's last seven instructions.** 11,944 bytes in one
   function, generated by `tools/gen_wmltypes.py`. The dispatch
   matches and 2,979 of 2,986 instructions align. What is left is
   five re-loads and two orphaned branches; the section above lists
   the 64 things already ruled out, so do not start there.

4. **The AnimTable family, now reachable.** `CreateAnimTable`
   MATCHES, 4,388 bytes: pooled strings plus `gen_poolprefix.py` for
   the unit's pool prefix. 314 branchless functions and 143,664 bytes
   share its shape, and after the second cut the two biggest homes are
   units: `zSBPlayerActions.cpp` (66 KB of tables among 139,880) and
   `zCommonPlayerActions.cpp`. Each function is one forward walk --
   the scratch extractor recovered all 106 calls of the 6,580-byte
   `AddInternalTransitions` -- plus the unit's `.pool.h`. The callbacks
   the tables name are `scope:weak` inline members, so a table needs
   only their declarations. Run the pool generator before blaming
   the source.

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
