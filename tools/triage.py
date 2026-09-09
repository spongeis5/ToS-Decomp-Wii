"""What KIND of difference does each near-miss have?

    python tools/triage.py <unit-path-under-src> [...]
    python tools/triage.py --selftest

`nearmiss.py` says which written functions do not land and what they are
worth.  This says which of them are worth ATTACKING, because the two are
not the same question: three of the four biggest near-misses turned out
to be register-allocation ties that no source respelling reaches, and
each was discovered by spending an hour on it.

Every differing word pair is decoded and classified by which bits moved:

  OPCODE  the primary opcode differs -- genuinely different instructions,
          so there is a source-level cause to find
  SWAP    same opcode, the SAME register numbers permuted -- operand
          order on a non-commutative operation.  Semantic and reachable,
          even though only register fields moved
  REG     same opcode, and the register fields name DIFFERENT registers
          -- an allocation tie
  IMM     same opcode, difference reaching bits 21..31 -- a displacement,
          a mask or shift field, or a pooled-string offset

SWAP is why this is not a one-line bitmask test.  iCylinderIsectVec's two
words are `fsubs f2,f2,f0` against `fsubs f2,f0,f2`: a source-level
operand order, not allocation, yet confined to register fields.  Parking
those as allocation would be exactly the "absence of evidence rendered as
evidence of absence" this project keeps paying for.

A function whose rows are ALL REG is the expensive kind and is reported
as parked.  That is a ranking, not a verdict: a conclusion that nothing
can be done has so far always been wrong here.  It says where to look
last, not where to stop.

The classifier refuses to report unless it reproduces every known answer
in SELFTEST first -- a guard that fires on correct input is worse than no
guard, and one that stays silent on wrong input is worse still.
"""
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

ROW = re.compile(r"^\s+(\d+)\s+([0-9a-f]{8})\s+([0-9a-f]{8})\s*(.*)$")
HDR = re.compile(r"^\s+DIFFER\s+(\d+)\s+(\S+)\s+(\d+) of (\d+) words differ")


def fields(w):
    """The three register-sized fields at bits 6..10, 11..15, 16..20."""
    return ((w >> 21) & 0x1F, (w >> 16) & 0x1F, (w >> 11) & 0x1F)


def classify(a, b):
    """a is ours, b is retail."""
    if (a >> 26) != (b >> 26):
        return "OPCODE"
    if (a ^ b) & 0x000007FF:
        return "IMM"
    if sorted(fields(a)) == sorted(fields(b)):
        return "SWAP"
    return "REG"


# Every answer this module has been checked against, with the function it
# was read off.  Validating against ONE would not have caught the SWAP
# case, which the first version of this file got wrong.
SELFTEST = [
    (0xEC420028, 0xEC401028, "SWAP",
     "iCylinderIsectVec  fsubs f2,f2,f0   vs  fsubs f2,f0,f2"),
    (0xEC210028, 0xEC200828, "SWAP",
     "iCylinderIsectVec  fsubs f1,f1,f0   vs  fsubs f1,f0,f1"),
    (0x827B003C, 0x829B003C, "REG",
     "CreateBuilderData  lwz r19,0x3c(r27) vs lwz r20,0x3c(r27)"),
    (0x7E669B78, 0x7E86A378, "REG",
     "CreateBuilderData  mr r6,r19        vs  mr r6,r20"),
    (0x38840006, 0x38842725, "IMM",
     "var_text_MCMaxSpace  addi r4,r4,6   vs  addi r4,r4,0x2725"),
    (0x389F001F, 0x389F0020, "IMM",
     "GetPhysicsShape  addi r4,r31,0x1f   vs  addi r4,r31,0x20"),
    (0xB3A10008, 0x93A10008, "OPCODE",
     "StartLoad  sth r29,8(r1)            vs  stw r29,8(r1)"),
    (0x7C651B78, 0x3884EC8C, "OPCODE",
     "var_text_CurrentScene  mr           vs  addi, the length shift"),
]


def selftest(quiet=False):
    bad = 0
    for a, b, want, why in SELFTEST:
        got = classify(a, b)
        if got != want:
            bad += 1
        if not quiet or got != want:
            print("  %s %-6s (want %-6s) %s"
                  % ("ok  " if got == want else "FAIL", got, want, why))
    print("%d of %d known answers reproduced" % (len(SELFTEST) - bad,
                                                 len(SELFTEST)))
    if bad:
        raise SystemExit("triage: refusing to report -- %d of %d known "
                         "answer(s) wrong" % (bad, len(SELFTEST)))


def run(unit):
    r = subprocess.run([sys.executable, "tools/unitcmp.py", unit, "-v"],
                       cwd=ROOT, capture_output=True, text=True)
    cur = None
    for line in (r.stdout + r.stderr).splitlines():
        h = HDR.match(line)
        if h:
            if cur:
                yield cur
            cur = {"name": h.group(2), "size": int(h.group(1)),
                   "nd": int(h.group(3)), "nw": int(h.group(4)), "k": {}}
            continue
        m = ROW.match(line)
        if m and cur is not None:
            if "(reloc)" in m.group(4):
                continue                      # masked, not a difference
            a, b = int(m.group(2), 16), int(m.group(3), 16)
            if a != b:
                c = classify(a, b)
                cur["k"][c] = cur["k"].get(c, 0) + 1
    if cur:
        yield cur


def main(argv):
    if argv[:1] == ["--selftest"]:
        selftest()
        return 0

    selftest(quiet=True)
    print()
    rows = []
    for unit in argv:
        for f in run(unit):
            rows.append(f)
    rows.sort(key=lambda f: -f["size"])

    for f in rows:
        kinds = ", ".join("%s=%d" % kv for kv in sorted(f["k"].items()))
        if not f["k"]:
            # Every differing row had a blank column, so the two sides are
            # different LENGTHS. Structural, and reporting it as parked
            # would be a tool returning a benign value for something it
            # did not measure.
            verdict = "UNCLASSIFIED (lengths differ)"
        elif set(f["k"]) <= {"REG"}:
            verdict = "parked (allocation)"
        elif "OPCODE" in f["k"] or "SWAP" in f["k"]:
            verdict = "ATTACK"
        else:
            verdict = "IMM only"
        print("%6d B  %3d/%-4d  %-24s %-44s %s"
              % (f["size"], f["nd"], f["nw"], kinds or "length-only",
                 f["name"][:44], verdict))

    print()
    print("%d near-miss function(s) in %d unit(s) classified"
          % (len(rows), len(argv)))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
