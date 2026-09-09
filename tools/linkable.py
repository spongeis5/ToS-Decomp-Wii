"""Which units could be LINKED, and which of those actually reproduce the image.

    python tools/linkable.py scan              candidates, with the flag each has
    python tools/linkable.py set <list> Matching|NonMatching
    python tools/linkable.py search <list>     the largest subset that LINKS

MATCHED IS NOT LINKED. `Object(Matching, ...)` in configure.py means "this
object matches and should be linked", and the build then substitutes our
object for retail's bytes and checks config/R8IE78/build.sha1 over the
whole main.dol. So a unit whose every function is byte-identical is a
CANDIDATE and nothing more: data, padding and symbol ORDER are not in that
test. Of 55 candidates offered in one sitting, 13 reproduced the image;
the rest linked cleanly and failed the checksum, or failed to link at all.

`search` therefore reports three outcomes and never conflates them:

    LINK-ERROR   the linker refused -- a symbol is missing
    SHA-FAIL     it linked, and the image differs from retail
    OK           it linked, and reproduced retail byte for byte

It is greedy: hold a set known to link, offer a chunk, keep it only if the
image still matches, halve the chunk when it does not. A chunk is only
ever accepted while sitting on everything already accepted, so
interactions between units are covered rather than assumed away.

TWO THINGS THAT WENT WRONG HERE, BOTH NOW GUARDED.

`set` matched `Object(NonMatching, "x")` as an exact string, and
configure.py also writes `Object(NonMatching,"x")` and `Object(
NonMatching,"x")`. Seven units were silently not flipped, and the search
then scored them OK without ever having tried them. It is a regex now, and
`set` reports what it could NOT find instead of returning quietly.

A unit with no data omits `total_data`/`matched_data` from the report
entirely. Absent is not zero, so the scan prints `-` for those and counts
them separately.
"""
import argparse
import json
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CFG = ROOT / "configure.py"
REPORT = ROOT / "build/R8IE78/report.json"
NL = chr(10)

OBJ = re.compile(r'Object\(\s*(Matching|NonMatching|MatchingFor\([^)]*\))'
                 r'\s*,\s*"([^"]+)"')


def flags():
    return {m.group(2): m.group(1)
            for m in OBJ.finditer(CFG.read_text(encoding="utf-8"))}


def report():
    if not REPORT.is_file():
        sys.exit("linkable: %s is missing -- run ninja first." % REPORT)
    return json.loads(REPORT.read_text(encoding="utf-8"))


def candidates():
    """-> ([(code, dataGap|None, flag, unit, src)], already_matching)."""
    R, fl = report(), flags()
    rows, already = [], 0
    for u in R["units"]:
        md = u.get("metadata") or {}
        src = md.get("source_path")
        if not src:
            continue
        key = src[4:] if src.startswith("src/") else src
        m = u["measures"]
        tc = int(m.get("total_code", 0))
        mc = int(m.get("matched_code", 0))
        tf, mf = m.get("total_functions", 0), m.get("matched_functions", 0)
        if not tc or mc != tc or not tf or mf != tf:
            continue
        if "total_data" in m and "matched_data" in m:
            gap = int(m["total_data"]) - int(m["matched_data"])
        elif "total_data" in m:
            gap = int(m["total_data"])
        else:
            gap = None
        f = fl.get(key, "?")
        if f == "Matching":
            already += 1
            continue
        rows.append((tc, gap, f, u["name"], key))
    rows.sort(key=lambda r: (-r[0], r[4]))
    return rows, already


def do_scan():
    rows, already = candidates()
    fl = flags()
    print("configure.py declares %d object(s), %d of them Matching"
          % (len(fl), sum(1 for v in fl.values() if v == "Matching")))
    print("")
    print("%-7s %-8s %-12s %s" % ("code", "dataGap", "configure", "unit"))
    for tc, gap, f, name, _key in rows:
        print("%-7d %-8s %-12s %s"
              % (tc, "-" if gap is None else str(gap), f, name))
    nodata = sum(1 for r in rows if r[1] is None)
    clean = sum(1 for r in rows if r[1] == 0)
    print("")
    print("%d candidate(s): every function byte-identical, not yet Matching"
          % len(rows))
    print("  %d have zero unmatched data, %d have NO data figure at all "
          "(shown as -, not as zero), %d have a data gap"
          % (clean, nodata, len(rows) - clean - nodata))
    print("  their code totals %d byte(s)" % sum(r[0] for r in rows))
    print("  %d fully-matched unit(s) are already Matching" % already)
    print("")
    print("  A candidate is not a result. Run `search` -- the sha1 decides.")
    return 0


def do_set(paths, want):
    other = "NonMatching" if want == "Matching" else "Matching"
    text = CFG.read_text(encoding="utf-8")
    changed, already, missing = 0, 0, []
    for p in paths:
        pat = re.compile(r'Object\(\s*' + other + r'\s*,\s*"'
                         + re.escape(p) + r'"\s*\)')
        text, n = pat.subn('Object(%s, "%s")' % (want, p), text, count=1)
        if n:
            changed += 1
            continue
        if re.search(r'Object\(\s*' + want + r'\s*,\s*"'
                     + re.escape(p) + r'"\s*\)', text):
            already += 1
        else:
            missing.append(p)
    CFG.write_text(text, encoding="utf-8")
    print("%d of %d path(s) set to %s; %d already were; %d NOT FOUND"
          % (changed, len(paths), want, already, len(missing)))
    for p in missing:
        print("  not found: %s" % p)
    return 1 if missing else 0


def run(args):
    return subprocess.run(args, cwd=str(ROOT), capture_output=True, text=True)


def trial(keep, baseline):
    CFG.write_text(baseline, encoding="utf-8")
    if keep:
        tmp = ROOT / "build/linkable_keep.txt"
        tmp.parent.mkdir(parents=True, exist_ok=True)
        tmp.write_text(NL.join(keep) + NL, encoding="utf-8")
        do_set(keep, "Matching")
    run([sys.executable, "configure.py"])
    ok = ROOT / "build/R8IE78/ok"
    if ok.exists():
        ok.unlink()
    r = run(["ninja", "build/R8IE78/ok"])
    out = r.stdout + r.stderr
    if "main.dol: OK" in out:
        return "OK", 0
    errs = out.count("Linker Error")
    return ("LINK-ERROR", errs) if errs else ("SHA-FAIL", 0)


def do_search(cands, chunk):
    baseline = CFG.read_text(encoding="utf-8")
    print("baseline: nothing flipped")
    v, _e = trial([], baseline)
    print("  %s" % v)
    if v != "OK":
        CFG.write_text(baseline, encoding="utf-8")
        sys.exit("linkable: the tree does not link clean -- this is not "
                 "about the candidates.")

    good, bad, tests = [], [], 1
    queue = [cands[i:i + chunk] for i in range(0, len(cands), chunk)]
    while queue:
        block = queue.pop(0)
        v, errs = trial(good + block, baseline)
        tests += 1
        print("try %2d on top of %2d good -> %-10s%s"
              % (len(block), len(good), v,
                 (" (%d error(s))" % errs) if errs else ""))
        if v == "OK":
            good += block
        elif len(block) == 1:
            bad.append(block[0])
            print("    REJECT %s" % block[0])
        else:
            mid = len(block) // 2
            queue.insert(0, block[mid:])
            queue.insert(0, block[:mid])

    CFG.write_text(baseline, encoding="utf-8")
    if good:
        do_set(good, "Matching")
    print("")
    print("%d of %d candidate(s) link AND reproduce main.dol; %d rejected; "
          "%d trial(s)" % (len(good), len(cands), len(bad), tests))
    for p in good:
        print("  link %s" % p)
    return 0


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("mode", choices=("scan", "set", "search"))
    ap.add_argument("list", nargs="?")
    ap.add_argument("flag", nargs="?")
    ap.add_argument("--chunk", type=int, default=8)
    a = ap.parse_args()

    if a.mode == "scan":
        return do_scan()

    if a.mode == "set":
        if not a.list or a.flag not in ("Matching", "NonMatching"):
            sys.exit("linkable: set needs a list file and Matching/NonMatching")
        paths = [l.strip() for l in
                 Path(a.list).read_text(encoding="utf-8").splitlines()
                 if l.strip()]
        return do_set(paths, a.flag)

    if not a.list:
        sys.exit("linkable: search needs a list file (see `scan`)")
    cands = [l.strip() for l in
             Path(a.list).read_text(encoding="utf-8").splitlines() if l.strip()]
    return do_search(cands, a.chunk)


sys.exit(main())
