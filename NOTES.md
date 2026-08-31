# Truth or Square (Wii) -- working notes

What has been established, what is still open, and the reasons. Figures are
from `ninja` and `build/R8IE78/report.json`; re-run rather than trusting the
numbers here, which move.

## State at time of writing

```
Game Code:  23 of 498 files complete   9,852 / 2,115,452 bytes  510 / 10,559 fn
            0.4657% of game code

Of those 510 functions, 424 are GENERATED -- six machine-recognised shapes,
not one of which is decompiling. They are real matched functions and the
offsets and constants are recovered fact, but a count of them is not a count
of decompiled code. HAND-WRITTEN IS 86, across 29 units and 5,384 bytes --
UNCHANGED by any of it -- and that is the figure to compare against earlier
ones.
`python tools/written_vs_generated.py` prints the split, decided by the
banner in the file that defines each function, and refuses to report at all
if the two halves do not add up to the category total.
All:        1.82% matched              main.dol reproduces byte for byte
```

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
| `unitcmp.py` | compile ONE unit and compare each function by name against retail; `-v` for a word-by-word diff |
| `unitcmp_check.py` | validates `unitcmp.py` against every unit it has a known answer for, and proves its drift guard fires |
| `anon_blocked.py` | which units can never match while they are split out of their unity blob |
| `gen_typeids.py` | the constant-return codec -- decode, re-encode, demangle. `gen_accessors` imports it; it no longer writes files |
| `gen_survey.py` | where else the constant-return shape lives |
| `gen_accessors.py` | generate every member-only shape for a unit; `--survey` for what is left |
| `gen_units.py` | run that over EVERY unit that has candidates, and withdraw the files that no longer do |
| `shape_census.py` | what the unmatched short functions LOOK like, as a population, by opcode signature |
| `unitcmp_pins.py` | re-measure `unitcmp_check`'s pins; refuses to lower one |
| `written_vs_generated.py` | the split, from the banner in each source file |
| `next_functions.py` | what is left ranked by functions rather than bytes |

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

## Open problems, in order of value

1. **The data tier.** 205 units would gain data, and none can take it yet.
   Two distinct blockers, both measured:
   - mwcc emits `.bss` with align 8 and no option changes it. A 4-aligned
     `.bss` start (like `iTime`'s `sGameTime` at `0x8072E17C`) cannot be
     honoured; linking it shifts 10,115 bytes of the DOL.
   - An interior unit's data range sits inside the parent's, and carving it
     out needs the parent's data partitioned across its chunks **in link
     order**. That is only mechanical if data follows text order, and it
     does not: 62% / 60% / 74% in order for `.rodata` / `.data` / `.bss`.
     Likely cause is `-inline auto` -- a static belonging to file A,
     referenced only through A's function inlined into B, is attributed
     to B.

   `iTime.cpp` and `zPerformanceDisplay.cpp` are written and their objects
   match 100%; both are `NonMatching` because of the above.

2. **Four near-misses with exhausted searches.** Do not redo these:
   - `zNPCType` 84.53% -- register allocation only; structure identical.
     Five declaration orders tried, all 8 of 19 words.
   - `zNPCStatus` 67.55% -- retail constructs `Math::Vector` in place at the
     member; ours builds a temporary and copies. **Seven spellings and
     eight flag sets tried**, all 6 of 22. What is left is the real
     declaration of `Math::Vector`, which the DWARF does not pin down.
   - `keycode.cxx` 5 of 26 words, and all five are ONE register swap:
     retail keeps the walking destination in r4 and the character in r5,
     and every spelling puts them the other way round. **Thirteen
     spellings across two sweeps**, listed in the file. Folding the three
     loop tests into a single condition is what took it from 15 to 5 --
     with `break`s inside a bounded loop mwcc proves the trip count and
     emits `mtctr`/`bdnz`, which retail does not have.
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

`gen_accessors.py` now recognises six shapes, all of them member-only or
constant, and `gen_units.py` runs it over every unit that has any. That took
Game Code from 8,528 to 9,852 bytes -- and **not one byte of it is
decompiling**, which is why `written_vs_generated.py` exists.

The population was found by measuring rather than by noticing. Both earlier
generators were written after someone spotted a shape by accident, so
`shape_census.py` asks the question directly: group every unmatched game
function by its sequence of opcodes and count. 1,727 of the 9,960 unmatched
functions are 32 bytes or less, carrying 29,124 bytes across 600 distinct
signatures, and the top of that list is where the next generator goes. The
four biggest that are still unwritten:

| N | bytes | signature | what it is |
|---|---|---|---|
| 261 | 1,044 | `b` | a tail call; needs the target's full signature |
| 97 | 776 | `addi b` | a tail call after adjusting `this` |
| 54 | 1,296 | `lwz or or lwz or b` | a member loaded, then a tail call |
| 38 | 608 | `addis addi stw bclr` | a member set to a constant ADDRESS |
| 17 | 340 | `lwz lwz lwz mtspr bcctr` | a virtual call forwarded |

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
land inside match NOT AT ALL. A perfect split, and the generator now refuses
the second group.

This is the sharpest form so far of the rule already in this file: unitcmp
is not the oracle. It answers "are these bytes the same", which is not the
same question as "does this unit match".

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
