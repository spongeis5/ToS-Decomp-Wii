"""Generate the Sext assets' `Fix(long)` -- the event-link fixup.

    python tools/gen_assetfix.py --survey
    python tools/gen_assetfix.py <unit>

An asset's Fix relocates whatever the asset owns and then walks its
event links:

    CustomFix(base);
    <0..n>  member.Fix(base);          a sub-object at a known offset
    <0..1>  other = (void*)((long)other + base);
    p = (EventLinkNew*)((long)links + base);
    links = p;
    end = p + linkCount;
    while (p != end) {
        RTTID_Fix<T>(&p->src, base);
        FixWmlType(base, p->srcType, p->src.Get());
        RTTID_Fix<T>(&p->dst, base);
        FixWmlType(base, p->dstType, p->dst.Get());
        p++;
    }

THE TAIL IS FIXED AND THE HEAD IS NOT. Everything from the link count
onward is one of two shapes -- with the extra pointer or without -- and
this checks the candidate's tail against the matching one WORD FOR WORD
outside the offsets and the relocated branches. What varies is the head:
a run of `mr r4,r29 / addi r3,r31,N / bl Fix__<T>Fl` triples, each a
sub-object of the asset fixed at its own offset, and however many there
are is read rather than assumed. Both tails were written by hand and
matched before this existed.

`end` IS DECLARED BEFORE THE CURSOR, and that is not cosmetic: with the
cursor first, retail's r30 and r31 come out swapped in fourteen of the
forty words. Three spellings were compiled to settle it.

AND THE BODIES GO INSIDE `#pragma dont_inline`. Without it mwcc takes
Util::RTTID_Fix<T> -- it is one line -- and the branch reaches T::Fix
directly where retail reaches the wrapper. Every word is still equal, so
only reloc_audit sees it, and reloc_audit is what caught it. A wrapper
for a T new to the unit is instantiated OUTSIDE that block, or the
inlining it DOES want does not happen either.

The link is 40 bytes -- the `mulli r0,r0,40` -- in every one of them.

The unit must already declare the class as a `Fix(long)` stub, and the
type of each sub-object as one too. Anything missing is named and the
merge refuses; a member whose Fix is not in the unit is not invented.
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
PRELUDE = "class EventLinkNew {"
STRIDE = 0x1C000028                      # mulli r0,r0,40

# The two tails, as the words that follow the head. Offsets are read out
# of the words named in `holes`; every other word must be equal, except
# the branches, which are relocated.
TAILS = [
    # `store` is every word that writes an offset back, and there is one
    # per relocated pointer -- the extra-pointer tail has TWO, and
    # leaving its first out of the skip set refused all fifteen of them.
    {"template": "Fix__Q24Sext13xCounterAssetFl", "at": 7,
     "holes": {"count": 0, "array": 1}, "store": (4,),
     "branch": (9, 11, 15, 18, 20, 24), "extra": False},
    {"template": "Fix__Q24Sext10zCondAssetFl", "at": 7,
     "holes": {"count": 0, "other": 1, "array": 2}, "store": (5, 7),
     "branch": (12, 14, 18, 21, 23, 27), "extra": True},
]

LOOP = [
    "    while (p != end) {",
    "        Util::RTTID_Fix<%(rttid)s>(&p->src, base);",
    "        Sext::FixWmlType(base, p->srcType, p->src.Get());",
    "        Util::RTTID_Fix<%(rttid)s>(&p->dst, base);",
    "        Sext::FixWmlType(base, p->dstType, p->dst.Get());",
    "        p++;",
    "    }",
    "}"]


def qualified(sym):
    m = re.match(r"^Q(\d)(.*)$", sym)
    parts, rest = [], m.group(2) if m else sym
    want = int(m.group(1)) if m else 1
    for _ in range(want):
        g = re.match(r"(\d+)", rest)
        if not g:
            return None
        n = int(g.group(1))
        rest = rest[g.end():]
        parts.append(rest[:n])
        rest = rest[n:]
    return "::".join(parts) if rest == "" else None


def load():
    raw, secs, funcs, objs = D.load()
    byname = {}
    for a, (nm, sz) in funcs.items():
        byname[nm] = (a, sz)
    return raw, secs, funcs, objs, byname


def candidates():
    raw, secs, funcs, objs, byname = load()

    def words(a, sz):
        b = D.read(raw, secs, a, sz)
        return None if b is None else list(
            struct.unpack(">" + "I" * (sz // 4), b))

    for t in TAILS:
        if t["template"] not in byname:
            raise SystemExit("gen_assetfix: template %s is not in the image"
                             % t["template"])
        a, sz = byname[t["template"]]
        t["words"] = words(a, sz)[t["at"]:]

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

    def target(a, i, w):
        d = w & 0x03FFFFFC
        if d & 0x02000000:
            d -= 0x04000000
        return D.name_at(funcs, objs, (d if (w & 2) else a + 4 * i + d))

    prologue = TAILS[0]["words"]  # only used for its length below
    head_tmpl = words(*byname[TAILS[0]["template"]])[:7]

    out, refused = defaultdict(list), defaultdict(int)
    for a in sorted(funcs):
        nm, sz = funcs[a]
        if nm in matched or not nm.startswith("Fix__Q24Sext"):
            continue
        ws = words(a, sz)
        if ws is None or len(ws) < len(head_tmpl) + 20:
            continue
        # the prologue and the CustomFix call
        if any(ws[i] != head_tmpl[i] for i in range(6)):
            continue
        cf = target(a, 6, ws[6]) or ""
        cm = re.match(r"^CustomFix__(Q\d.*|\d+\w*)Fl$", cf)
        cbase = qualified(cm.group(1)) if cm else None
        if cbase is None:
            continue
        # a run of `mr r4,r29 / addi r3,r31,N / bl Fix__<T>Fl`
        i, members, bad = 7, [], False
        # `mr r4,r29` then `addi r3,r31,N` then a bl. rD is bits 21..25
        # and rA is bits 16..20 -- the first version had them the other
        # way round -- and `&` binds LOOSER than `==` in Python, so each
        # field test needs its own parentheses or it reads as `& False`.
        while (i + 2 < len(ws) and ws[i] == 0x7FA4EB78
               and (ws[i + 1] >> 26) == 14
               and ((ws[i + 1] >> 21) & 31) == 3
               and ((ws[i + 1] >> 16) & 31) == 31
               and (ws[i + 2] >> 26) == 18 and (ws[i + 2] & 1)):
            t = target(a, i + 2, ws[i + 2]) or ""
            tm = re.match(r"^Fix__(Q\d.*|\d+\w*)Fl$", t)
            ty = qualified(tm.group(1)) if tm else None
            if ty is None:
                bad = True
                break
            members.append((ws[i + 1] & 0xFFFF, ty))
            i += 3
        if bad:
            refused["a sub-object Fix whose class does not read"] += 1
            continue
        rest = ws[i:]
        hit = None
        for t in TAILS:
            tw = t["words"]
            if len(rest) != len(tw):
                continue
            skip = set(t["branch"]) | set(t["holes"].values()) | set(t["store"])
            if any(rest[k] != tw[k] for k in range(len(tw)) if k not in skip):
                continue
            hit = t
            break
        if hit is None:
            refused["the tail is not one of the two known shapes"] += 1
            continue
        if rest[3 if hit["extra"] else 2] != STRIDE:
            refused["the link stride is not 40"] += 1
            continue
        holes = {k: rest[v] & 0xFFFF for k, v in hit["holes"].items()}
        rt = set()
        for k in hit["branch"][:1] + hit["branch"][3:4]:
            t = target(a, i + k, rest[k]) or ""
            g = re.match(r"^RTTID_Fix<(.*)>__4UtilFPvl_v$", t)
            rt.add(qualified(g.group(1)) if g else None)
        if len(rt) != 1 or None in rt:
            refused["the two RTTID calls disagree or do not read"] += 1
            continue
        m = re.match(r"^Fix__(Q\d.*|\d+\w*)Fl$", nm)
        full = qualified(m.group(1)) if m else None
        if full is None or not full.startswith("Sext::") \
                or "::" in full[len("Sext::"):]:
            refused["the class is not a plain Sext one"] += 1
            continue
        u = unit_of(a)
        if u is None:
            refused["outside every split"] += 1
            continue
        out[u].append((full[len("Sext::"):], cbase, rt.pop(), holes,
                       members, hit["extra"]))
    return out, refused


def declare(cls, base, holes, members):
    """The class, with every member at the offset the bytes load it
    from and padding between. Nothing here knows the order; the bytes
    do."""
    # A sub-object stub is an EMPTY class and therefore ONE byte, not
    # four: assuming four put every later member three bytes early and
    # the `addi r3,r31,N` offsets came out as 0xED where retail has
    # 0xF0. The size each field occupies is what the padding is measured
    # against.
    fields = [(off, ty, "m%X" % off, 1) for off, ty in members]
    for k, (ty, nm) in (("other", ("void*", "other")),
                        ("count", ("int", "linkCount")),
                        ("array", ("EventLinkNew*", "links"))):
        if k in holes:
            fields.append((holes[k], ty, nm, 4))
    fields.sort()
    out = ["class %s : public %s {" % (cls, base), "public:",
           "    void Fix(long base);", ""]
    at, n = 0, 0
    for off, ty, nm, size in fields:
        if off > at:
            out.append("    unsigned char _pad%d[0x%X];" % (n, off - at))
            n += 1
        out.append("    %s %s;" % (ty, nm))
        at = off + size
    out.append("};")
    return NL.join(out)


def body(cls, rttid, holes, members, extra):
    out = ["void Sext::%s::Fix(long base) {" % cls,
           "    EventLinkNew* end;",
           "    EventLinkNew* p;",
           "",
           "    CustomFix(base);",
           ""]
    for off, _ty in members:
        out.append("    m%X.Fix(base);" % off)
    if extra:
        out.append("    other = (void*)((long)other + base);")
    out += ["    p = (EventLinkNew*)((long)links + base);",
            "    links = p;",
            "    end = p + linkCount;",
            ""]
    return NL.join(out + [l % {"rttid": rttid} for l in LOOP])


def merge(unit, rows):
    path = ROOT / "src" / unit
    if not path.exists():
        raise SystemExit("gen_assetfix: %s has no source file" % unit)
    text = path.read_text(encoding="utf-8")
    if PRELUDE not in text:
        raise SystemExit("gen_assetfix: %s has no EventLinkNew; add the "
                         "prelude by hand (WAD00_32.cpp is the model)" % unit)

    bodies, problems, rttids = [], [], set()
    for cls, cbase, rttid, holes, members, extra in sorted(rows,
                                                           key=lambda r: r[0]):
        stub = "class %s { public: void Fix(long); };" % cls
        if stub not in text:
            problems.append("%s has no Fix(long) stub in this unit" % cls)
            continue
        missing = [ty for _o, ty in members
                   if ("class %s " % ty.split("::")[-1]) not in text
                   and ("class %s;" % ty.split("::")[-1]) not in text
                   and ("class %s {" % ty.split("::")[-1]) not in text]
        if missing:
            problems.append("%s holds %s, which this unit does not declare"
                            % (cls, ", ".join(sorted(set(missing)))))
            continue
        b = cbase[len("Sext::"):] if cbase.startswith("Sext::") else cbase
        text = text.replace(stub, declare(cls, b, holes, members), 1)
        bodies.append(body(cls, rttid, holes, members, extra))
        rttids.add(rttid)

    if problems:
        for p in sorted(set(problems)):
            print("  %s" % p)
        raise SystemExit("gen_assetfix: refusing to write %s" % unit)

    inst = [t for t in sorted(rttids)
            if ("Util::RTTID_Fix<%s>(void*, long);" % t) not in text]
    block = NL.join(
        ["template void Util::RTTID_Fix<%s>(void*, long);" % t for t in inst]
        + ["", "#pragma dont_inline on"]
        + [(NL + NL).join(bodies)]
        + ["#pragma dont_inline off"])
    path.write_text(text.rstrip(NL) + NL + NL + block + NL, encoding="utf-8")
    print("  merged %d Fix bodies into %s" % (len(bodies), unit))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("unit", nargs="?")
    ap.add_argument("--survey", action="store_true")
    args = ap.parse_args()
    out, refused = candidates()
    total = sum(len(v) for v in out.values())
    if args.survey or not args.unit:
        print("  %d function(s) read completely" % total)
        for k, v in sorted(refused.items(), key=lambda kv: -kv[1]):
            print("  refused x%-4d %s" % (v, k))
        print("")
        for u, v in sorted(out.items(), key=lambda kv: -len(kv[1])):
            print("  %-5d %s" % (len(v), u))
        return
    if args.unit not in out:
        raise SystemExit("gen_assetfix: nothing of this shape left in %s"
                         % args.unit)
    merge(args.unit, out[args.unit])


if __name__ == "__main__":
    main()
