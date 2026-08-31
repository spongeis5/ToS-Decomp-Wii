# Truth or Square (Wii) -- working notes

What has been established, what is still open, and the reasons. Figures are
from `ninja` and `build/R8IE78/report.json`; re-run rather than trusting the
numbers here, which move.

## State at time of writing

```
Game Code:  15 of 498 files complete   2,460 / 2,115,452 bytes   52 / 10,559 fn
All:        1.72% matched              main.dol reproduces byte for byte
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
| `unitcmp_check.py` | validates `unitcmp.py` against all seven units it has known answers for, and proves its drift guard fires |

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

2. **Three near-misses with exhausted searches.** Do not redo these:
   - `zNPCType` 84.53% -- register allocation only; structure identical.
     Five declaration orders tried, all 8 of 19 words.
   - `zNPCStatus` 67.55% -- retail constructs `Math::Vector` in place at the
     member; ours builds a temporary and copies. **Seven spellings and
     eight flag sets tried**, all 6 of 22. What is left is the real
     declaration of `Math::Vector`, which the DWARF does not pin down.
   - `xOGModelRefPtr` 3 of 4 -- `GetWeakPtr` is right in all thirty words
     except where the first load sits in the prologue. **Seven spellings and
     twelve flag sets tried**, six of the spellings emitting identical bytes.
     The lever is the instruction scheduler's placement, and it is reachable
     from neither the source text nor any option swept so far. The file
     carries the full exclusion list.

3. **More units.** `tools/dwarf_targets.py` ranks them. The remaining
   no-data tier includes `Scaleform.cpp` (5 fn), `zWallNetPosition.cpp`
   (3 fn), `Blobloids.cpp` (9 fn), `StaticBuilder.cpp` (8 fn) and
   `Sort.cpp` (7 fn, 2,860 bytes -- the largest left).

## Two traps worth knowing

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
