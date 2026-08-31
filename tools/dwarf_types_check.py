"""How many recovered types actually COMPILE. Measured, with a denominator.

    python tools/dwarf_types_check.py [N]        default 60

`tools/dwarf_types.py` emits 3,747 types. That number is worth nothing on
its own: a tool whose headers build an unknown fraction of the time has not
been measured, and "mostly works" is not a figure. This samples N types with
a FIXED SEED, emits each with its dependencies, hands the header to the real
CodeWarrior compiler, and counts what it accepts.

Measured when written: **71 of 120 (59%)**, sample seeded so the figure is
reproducible. It started near half that, and each improvement came from
reading ONE failure rather than guessing at the class:

  * an anonymous type held by value was rendered as `void`;
  * an anonymous UNION was flattened into sequential members, so `sizeof`
    came out as the sum of the arms instead of the largest -- `xCamCoord`
    is 0x20 bytes with three arms all at +0;
  * a class with virtual functions has a vtable pointer at +0 that DWARF
    does not list as a member, so an empty class asserted a size of 4.

An earlier run of this reported 67%, and that number should not be compared
against: it was measured while the type GROUPING was order-dependent -- the
signature that decides the names was itself computed from the names -- so
the same DWARF gave 0 disagreeing types on one run and 7 on the next. The
grouping is now independent of the map it builds, and 59% is the first
figure from a run that is reproducible.

The remaining failures are dominated by `illegal constant expression`, which
is a size assertion on a type whose layout is still not fully reconstructed.
They are listed with their causes, so the next fix is chosen from a
population rather than from an impression.
"""
import random
import subprocess
import sys
from collections import Counter
from pathlib import Path

# DERIVED, never written down. An absolute path here carries the account
# name of whoever wrote it into a public repository. Content can be edited
# away; an identity already in git history cannot.
REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "tools"))
from dwarf_types import Types                                  # noqa: E402

CC = [str(REPO / "build/compilers/Wii/1.1/mwcceppc.exe"),
      "-c", "-nosyspath", "-proc", "gekko", "-fp", "hardware", "-O4,p",
      "-maxerrors", "2",
      "-i", "build", "-i", "src", "-i", "src/MSL_C/include",
      "-i", "src/Revolution/include", "-i", "src/SB/include"]

n = int(sys.argv[1]) if len(sys.argv) > 1 else 60
t = Types(REPO / "orig/R8IE78/files/SB09WiiMASTERWAD.elf")
names = sorted(t.named)
random.seed(20260830)
sample = random.sample(names, min(n, len(names)))

(REPO / "build/_cov.cpp").write_text(
    '#include <stddef.h>\n#include "_cov.h"\nint main(){return 0;}\n',
    encoding="utf-8")

ok, bad = 0, []
kinds = Counter()
for name in sample:
    r = subprocess.run([sys.executable, "tools/dwarf_types.py",
                        "--type", name, "-o", "build/_cov.h"],
                       cwd=str(REPO), capture_output=True, text=True)
    if r.returncode != 0:
        bad.append((name, "would not emit"))
        kinds["emit"] += 1
        continue
    c = subprocess.run(CC + ["-o", "build/_cov.o", "build/_cov.cpp"],
                       cwd=str(REPO), capture_output=True, text=True)
    if c.returncode == 0:
        ok += 1
        continue
    out = c.stdout + c.stderr
    why = "?"
    for line in out.splitlines():
        line = line.strip()
        if line.startswith("#   (10"):
            why = line[4:60]
            break
    kinds[why] += 1
    bad.append((name, why))

print("%d of %d sampled type(s) emit a header the real compiler accepts"
      % (ok, len(sample)))
print("(%.0f%%, sample of %d from %d named types, seed fixed)"
      % (100.0 * ok / len(sample), len(sample), len(names)))
print("")
print("why the rest failed:")
for k, c in kinds.most_common():
    print("   %-56s %d" % (k, c))
print("")
for nm, why in bad[:12]:
    print("   %-40s %s" % (nm[:40], why))
