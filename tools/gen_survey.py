"""Which units hold functions tools/gen_typeids.py could emit, and how many.

    python tools/gen_survey.py [--limit N]

The generator recognises one shape: a 32-bit constant into r3 and a return.
This asks where else that shape lives, so the cheap work is visible as a
population rather than found by accident -- WAD02_36 was found by accident.

It counts only functions NOT already matched, and it reports the total in
each unit alongside, because a unit that is 164 generatable of 177 is a
different proposition from one that is 3 of 300.
"""

import argparse
import json
import re
import struct
import sys
from pathlib import Path

from elftools.elf.elffile import ELFFile

ROOT = Path(__file__).resolve().parent.parent
ELF = ROOT / "orig/R8IE78/files/SB09WiiMASTERWAD.elf"
SPLITS = ROOT / "config/R8IE78/splits.txt"
REPORT = ROOT / "build/R8IE78/report.json"

sys.path.insert(0, str(ROOT / "tools"))
import gen_typeids as G


def unit_ranges():
    out, cur, ranges = [], None, []
    for line in SPLITS.read_text(encoding="utf-8").splitlines():
        if line and not line[0].isspace() and line.rstrip().endswith(":"):
            if cur:
                out.append((cur, ranges))
            cur, ranges = line.rstrip()[:-1], []
            continue
        m = re.match(r"\s+\.text\s+start:(0x[0-9A-Fa-f]+)\s+"
                     r"end:(0x[0-9A-Fa-f]+)", line)
        if m and cur:
            ranges.append((int(m.group(1), 16), int(m.group(2), 16)))
    if cur:
        out.append((cur, ranges))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--limit", type=int, default=20)
    args = ap.parse_args()

    raw = ELF.read_bytes()
    base = foff = None
    for sh in G.sections(raw):
        if sh[3] and sh[5] and sh[1] == 1:      # PROGBITS with an address
            if base is None or sh[3] < base:
                pass
    # Find the section that carries .text by locating any unit's start.
    ranges = unit_ranges()
    if not ranges:
        sys.exit("gen_survey: splits.txt lists no .text ranges")
    probe = ranges[0][1][0][0] if ranges[0][1] else None
    for u, rs in ranges:
        if rs:
            probe = rs[0][0]
            break
    for sh in G.sections(raw):
        if sh[3] and sh[3] <= probe < sh[3] + sh[5]:
            base, foff = sh[3], sh[4]
            break
    if base is None:
        sys.exit("gen_survey: cannot locate the section holding .text")

    f = ELFFile(open(ELF, "rb"))
    syms = []
    for sec in f.iter_sections():
        if sec.header["sh_type"] != "SHT_SYMTAB":
            continue
        for s in sec.iter_symbols():
            if s["st_info"]["type"] == "STT_FUNC" and s["st_size"]:
                syms.append((s["st_value"], s["st_size"], s.name))
    syms.sort()

    # Both the unit filter and the already-matched filter come out of
    # report.json, so it is required rather than optional. Until 2026-08-31
    # `matched` was built empty and never filled, and the survey reported 164
    # functions of WAD02_36 as available when all 164 were already generated
    # and matching -- the docstring's claim was false for two commits.
    if not REPORT.exists():
        sys.exit("gen_survey: %s is missing -- run ninja first. Without it "
                 "every already-matched function counts as remaining work."
                 % REPORT)
    rep = json.loads(REPORT.read_text(encoding="utf-8"))
    game, matched = set(), set()
    for u in rep["units"]:
        meta = u.get("metadata", {})
        if "game" not in (meta.get("progress_categories") or []):
            continue
        name = u["name"].split("/", 1)[1] if "/" in u["name"] else u["name"]
        game.add(name + ".cpp")
        for fn in u.get("functions", []):
            if fn.get("fuzzy_match_percent") == 100.0:
                matched.add(fn["name"])

    rows = []
    for unit, rs in ranges:
        if unit not in game:
            continue
        good = tot = 0
        for a, sz, nm in syms:
            if not any(b <= a < e for b, e in rs):
                continue
            tot += 1
            if nm in matched or sz not in (8, 12):
                continue
            body = raw[foff + (a - base): foff + (a - base) + sz]
            v = G.decode(body)
            if v is None or G.reencode(v, sz == 12) != body:
                continue
            if G.demangle(nm) is None:
                continue
            good += 1
        if good:
            rows.append((good, tot, unit))

    rows.sort(reverse=True)
    print("  %-6s %-6s %s" % ("GEN", "OF", "UNIT"))
    for good, tot, unit in rows[:args.limit]:
        print("  %-6d %-6d %s" % (good, tot, unit))
    print("")
    print("  %d game unit(s) hold the shape; %d function(s) in total"
          % (len(rows), sum(r[0] for r in rows)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
