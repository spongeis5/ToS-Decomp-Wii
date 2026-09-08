"""Generate the image's 116-byte animation callbacks.

    python tools/gen_animcb.py --survey
    python tools/gen_animcb.py <unit>

An `an<X>Check` that an animation table passes as a transition callback
is 29 instructions and says one thing:

    unsigned int r = 0;
    if (owner->_v5())              lwz r12,12(o); lwz r12,28(r12); bctrl
        if (owner->X(t, s))        bl X__<class>FP15xAnimTransitionP11xAnimSingle
            r = 1;
    return r;

where `owner` is `((AnimCBHolder*)a0)->slot->owner` -- +4 then +0x90 --
and is RE-READ for the second call rather than kept, which is the two
load pairs retail has. Slot 5 is (28 - 8) / 4 of the `lwz r12,28(r12)`,
a CodeWarrior vtable pointer sitting eight bytes into its table.

THERE ARE TWO SPELLINGS AND THE BYTES SAY WHICH. 35 of the image's take
that chain off a1 rather than a0 -- `lwz r5,4(r4)` and `lwz r3,4(r30)`
where the others have r3 and r29 -- which is the chain the short
forwarders already use. Those two words are read, not guessed, and a
function whose two reads disagree with each other is refused.

NOTHING HERE IS INFERRED FROM THE NAME. A candidate is accepted only
when all 29 words equal the template's outside the two relocated branch
fields, so the slot, the offsets and the branch structure are all
checked rather than assumed; and the function the second branch reaches
must be the symbol the name implies, `an` stripped and the `Pv`
dropped, or the candidate is refused and counted. The template is
zPlayerJump::anJumpCheck, written by hand and matched before this
existed.

The unit must already carry the two holder structs, a zPlayerAction
stub whose sixth virtual is declared, and each class this writes into --
derived from zPlayerAction, since the cast reaches slot 5 through it.
Anything missing is named and the merge refuses; none of it is invented,
because getting a base wrong here moves every member offset.
"""
import argparse
import json
import re
import struct
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

import disasm as D

NL = chr(10)
DONOR = "anJumpCheck__11zPlayerJumpFP15xAnimTransitionP11xAnimSinglePv"
CBSIG = "P15xAnimTransitionP11xAnimSinglePv"
NEEDS = ["struct AnimCBHolder", "struct AnimCBSlot", "virtual bool _v5();"]

BODY = NL.join([
    "unsigned int %(cls)s::%(an)s(xAnimTransition* a0, xAnimSingle* a1,",
    "%(pad)svoid* a2) {",
    "    unsigned int result = 0;",
    "",
    "    if (((%(cls)s*)((AnimCBHolder*)%(h)s)->slot->owner)->_v5()) {",
    "        if (((%(cls)s*)((AnimCBHolder*)%(h)s)->slot->owner)"
    "->%(target)s(a0, a1)) {",
    "            result = 1;",
    "        }",
    "    }",
    "",
    "    return result;",
    "}"])


def load():
    raw, secs, funcs, objs = D.load()
    byname = {}
    for a, (nm, sz) in funcs.items():
        byname[nm] = (a, sz)
    return raw, secs, funcs, objs, byname


def words(raw, secs, a, sz):
    blob = D.read(raw, secs, a, sz)
    if blob is None:
        return None
    return list(struct.unpack(">" + "I" * (sz // 4), blob))


def candidates():
    """-> {unit: [(addr, mangled, cls, an, target, holder)]}, refusals."""
    raw, secs, funcs, objs, byname = load()
    if DONOR not in byname:
        raise SystemExit("gen_animcb: the template %s is not in the image"
                         % DONOR)
    da, dsz = byname[DONOR]
    tmpl = words(raw, secs, da, dsz)
    branch = set(i for i, w in enumerate(tmpl) if (w >> 26) == 18)
    # The two words that read the holder: `lwz r5,4(r3)` in the
    # prologue and `lwz r3,4(r29)` before the forwarding call. 35 of
    # the image's carry r4 and r30 there instead -- the same chain
    # taken off a1 rather than a0, which is what the short
    # forwarders already use. Reading those two words is what says
    # which; a function that mixes them is refused.
    CHAIN = (7, 15)
    A0 = (0x80A30004, 0x807D0004)
    A1 = (0x80A40004, 0x807E0004)
    assert tuple(tmpl[i] for i in CHAIN) == A0, tmpl[7]
    branch = branch | set(CHAIN)

    matched = set()
    rep = json.loads((ROOT / "build/R8IE78/report.json").read_text())
    for u in rep.get("units", []):
        for f in u.get("functions", []):
            if f.get("fuzzy_match_percent", 0) >= 100:
                matched.add(f["name"])

    ranges = D.unit_ranges()

    def unit_of(addr):
        for u, rs in ranges:
            for lo, hi in rs:
                if lo <= addr < hi:
                    return u
        return None

    out, refused = defaultdict(list), defaultdict(int)
    for a in sorted(funcs):
        nm, sz = funcs[a]
        if sz != dsz or nm in matched:
            continue
        ws = words(raw, secs, a, sz)
        if ws is None or len(ws) != len(tmpl):
            continue
        if any(ws[i] != tmpl[i] for i in range(len(tmpl))
               if i not in branch):
            continue
        m = re.match(r"^(an[A-Za-z_]\w*)__(\d+)(\w+)$", nm)
        if not m:
            refused["the name is not an<X>__<class>F..."] += 1
            continue
        n = int(m.group(2))
        cls, params = m.group(3)[:n], m.group(3)[n:]
        if params != "F" + CBSIG:
            refused["not a transition callback signature"] += 1
            continue
        target = m.group(1)[2:]
        want = "%s__%d%sF%s" % (target, n, cls,
                                CBSIG[:-len("Pv")])
        if want not in byname:
            refused["no %s to forward to" % "<X>"] += 1
            continue
        u = unit_of(a)
        if u is None:
            refused["outside every split"] += 1
            continue
        chain = tuple(ws[i] for i in CHAIN)
        if chain == A0:
            holder = "a0"
        elif chain == A1:
            holder = "a1"
        else:
            refused["the two holder reads disagree"] += 1
            continue
        out[u].append((a, nm, cls, m.group(1), target, holder))
    return out, refused, dsz


def reaches(text, cls, root="zPlayerAction", depth=8):
    """Does `cls` reach zPlayerAction through its bases, read out of this
    unit's own text? A one-level substring test says no for
    `zBoardPlayerAction : public zCommonPlayerAction`, which does reach
    it two links up -- and would say yes for any class whose base merely
    CONTAINS the name, which is the same trap the other way round."""
    seen = set()
    while depth and cls not in seen:
        if cls == root:
            return True
        seen.add(cls)
        m = re.search(r"(?m)^class %s\b([^{;]*)\{" % re.escape(cls), text)
        if m is None:
            return False
        b = re.search(r":\s*(?:public|protected|private)?\s*([A-Za-z_]\w*)",
                      m.group(1))
        if b is None:
            return False
        cls = b.group(1)
        depth -= 1
    return cls == root


def merge(unit, rows):
    path = ROOT / "src" / unit
    if not path.exists():
        raise SystemExit("gen_animcb: %s has no source file" % unit)
    text = path.read_text(encoding="utf-8")
    missing = [n for n in NEEDS if n not in text]
    if missing:
        raise SystemExit("gen_animcb: %s is missing %s; add it by hand "
                         "(zCommonPlayerActions.cpp is the model)"
                         % (unit, ", ".join(missing)))

    bodies, problems, done = [], [], 0
    for _a, _nm, cls, an, target, holder in sorted(rows, key=lambda r: r[1]):
        hm = re.search(r"(?m)^class %s\b([^{;]*)\{" % re.escape(cls), text)
        if hm is None:
            problems.append("%s has no class in this unit" % cls)
            continue
        if not reaches(text, cls):
            problems.append("%s does not reach zPlayerAction" % cls)
            continue
        decls = [
            "    static unsigned int %s(xAnimTransition* a0, "
            "xAnimSingle* a1, void* a2);" % an,
            "    bool %s(xAnimTransition* a0, xAnimSingle* a1);" % target]
        j = text.index(NL + "};", hm.start())
        block = text[hm.start():j]
        # By NAME: gen_accessors wrote some of these already and
        # without the parameter names, so comparing whole lines
        # declares a second one and the compiler says `redefined`.
        add = []
        for d in decls:
            who = d.strip().split("(")[0].split()[-1]
            # WITH A BOUNDARY. `DeathCheck(` is a substring of
            # `anDeathCheck(`, so a plain `in` finds the forwarding
            # declaration inside the callback that forwards to it, adds
            # nothing, and the body then does not compile. This
            # repository has paid for that shape of bug before.
            if re.search(r"[^A-Za-z0-9_]%s\(" % re.escape(who), block):
                continue
            add.append(d)
        if add:
            text = text[:j] + NL + NL.join(add) + text[j:]
        pad = " " * (len("unsigned int %s::%s(" % (cls, an)))
        bodies.append(BODY % {"cls": cls, "an": an, "target": target,
                              "pad": pad, "h": holder})
        done += 1

    if problems:
        for p in sorted(set(problems)):
            print("  %s" % p)
        raise SystemExit("gen_animcb: refusing to write %s" % unit)

    text = text.rstrip(NL) + NL + NL + (NL + NL).join(bodies) + NL
    path.write_text(text, encoding="utf-8")
    print("  merged %d callback(s) into %s" % (done, unit))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("unit", nargs="?")
    ap.add_argument("--survey", action="store_true")
    args = ap.parse_args()

    out, refused, size = candidates()
    total = sum(len(v) for v in out.values())
    if args.survey or not args.unit:
        print("  %d function(s) match the template word for word, "
              "%d bytes" % (total, total * size))
        for k, v in sorted(refused.items(), key=lambda kv: -kv[1]):
            print("  refused x%-4d %s" % (v, k))
        print("")
        print("  N     BYTES   UNIT")
        for u, v in sorted(out.items(), key=lambda kv: -len(kv[1])):
            src = ROOT / "src" / u
            mark = "" if src.exists() else "   (NO SOURCE FILE)"
            print("  %-5d %-7d %s%s" % (len(v), len(v) * size, u, mark))
        return
    if args.unit not in out:
        raise SystemExit("gen_animcb: nothing of this shape left in %s"
                         % args.unit)
    merge(args.unit, out[args.unit])


if __name__ == "__main__":
    main()
