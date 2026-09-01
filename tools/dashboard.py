"""Render the progress dashboard from the repository's own outputs.

    ninja                       -> build/R8IE78/report.json
    python tools/dashboard.py   -> build/dashboard.html

NOTHING HERE IS RETYPED. Every figure comes from dtk's report, from
configure.py, from tools/unitcmp_check.py and from tools/anon_blocked.py,
so the page cannot drift from the repository the way a hand-maintained
status file does. If a source is missing the page says so in the place the
figure would have been, rather than printing a zero -- a dashboard that
renders a plausible number for something it could not measure is worse than
one that fails to render.

DESIGN, recorded before building:

  COLOUR  ground #F1F4F7, a cool near-white taken from the console's own
          plastic rather than the default dark terminal; surface #FFFFFF;
          line #D7DEE5; ink #141D26; muted #66788A, a grey biased toward
          the accent so it reads as chosen; accent #0A6FB4, the pale blue
          of the disc-slot light deepened until it holds on white. Status
          green/amber/red is SEPARATE from the accent and never reused to
          colour a series. The sibling 360 dashboard is deep slate and
          amber; this one is deliberately the other way round.
  TYPE    Anybody for display, Public Sans for body, DM Mono for every
          address, byte count and figure.
  LAYOUT  A console, scanned rather than read. The hero is the honest
          figure: Game Code's matched bytes drawn against the whole
          2,115,452, where what is done is visibly almost nothing. A bare
          percentage lets a reader skim past that.
"""

import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
REPORT = ROOT / "build/R8IE78/report.json"
OUT = ROOT / "build/dashboard.html"

sys.path.insert(0, str(ROOT / "tools"))


def die(why):
    raise SystemExit("dashboard: " + why)


def load_report():
    if not REPORT.exists():
        die("build/R8IE78/report.json is missing -- run `ninja` first. "
            "There is no figure to show without it.")
    return json.loads(REPORT.read_text(encoding="utf-8"))


def n(v):
    """dtk writes byte counts as strings and counts as ints."""
    return int(v)


def near_misses():
    """(unit, ok, total) for every unit unitcmp_check pins below its total."""
    txt = (ROOT / "tools/unitcmp_check.py").read_text(encoding="utf-8")
    m = re.search(r"EXPECT = \{(.*?)\n\}", txt, re.S)
    if not m:
        die("tools/unitcmp_check.py no longer states EXPECT where expected")
    out = []
    for unit, a, b in re.findall(r'"([^"]+)":\s*\((\d+),\s*(\d+)\)',
                                 m.group(1)):
        if int(a) != int(b):
            out.append((unit, int(a), int(b)))
    return out


def blocked():
    """(units, bytes) that anonymous-namespace mangling puts out of reach."""
    try:
        import anon_blocked
    except Exception as e:
        die("tools/anon_blocked.py will not import (%s)" % e)
    syms = anon_blocked.__dict__
    import io
    import contextlib
    buf = io.StringIO()
    with contextlib.redirect_stdout(buf):
        anon_blocked.main()
    text = buf.getvalue()
    m = re.search(r"(\d+) BLOCKED by anonymous-namespace mangling "
                  r"\(([\d,]+) bytes\)", text)
    if not m:
        die("anon_blocked.py no longer reports its total where expected")
    return int(m.group(1)), int(m.group(2).replace(",", ""))


def data_tier(rep):
    """What is linked as DATA, and how many units could take theirs.

    The counts come from tools/dwarf_data_carve.py, which reads the DWARF
    and refuses what it cannot spell; nothing here decides anything.
    """
    import contextlib
    import io
    try:
        import dwarf_data_carve
    except Exception as e:
        die("tools/dwarf_data_carve.py will not import (%s)" % e)
    buf = io.StringIO()
    argv = sys.argv
    sys.argv = ["dwarf_data_carve.py", "--survey"]
    try:
        with contextlib.redirect_stdout(buf):
            dwarf_data_carve.main()
    except SystemExit:
        pass
    finally:
        sys.argv = argv
    text = buf.getvalue()
    m = re.search(r"(\d+) unit\(s\) could take their data", text)
    if not m:
        die("dwarf_data_carve.py no longer reports its total where expected")
    can = int(m.group(1))
    nocut = re.search(r"no cut needed\s+(\d+)", text)
    cut = re.search(r"needs the parent cut\s+(\d+)", text)

    units = byts = 0
    for u in rep["units"]:
        meta = u.get("metadata", {})
        if "game" not in (meta.get("progress_categories") or []):
            continue
        d = n(u["measures"].get("complete_data", 0))
        if d:
            units += 1
            byts += d
    return {"units": units, "bytes": byts, "can": can,
            "nocut": int(nocut.group(1)) if nocut else 0,
            "cut": int(cut.group(1)) if cut else 0}


def game_flags():
    txt = (ROOT / "configure.py").read_text(encoding="utf-8")
    m = re.search(r"cflags_game\s*=\s*\((.*?)\)\s*\n", txt, re.S)
    if not m:
        die("configure.py no longer states cflags_game where expected")
    extra = re.findall(r'"([^"]+)"', m.group(1).split("+", 1)[1])
    out, i = [], 0
    while i < len(extra):
        if i + 1 < len(extra) and not extra[i + 1].startswith("-"):
            out.append(extra[i] + " " + extra[i + 1])
            i += 2
        else:
            out.append(extra[i])
            i += 1
    return out


def esc(s):
    return (s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;"))


def comma(v):
    return format(int(v), ",")


def main():
    rep = load_report()
    cats = {c["id"]: c["measures"] for c in rep["categories"]}
    if "game" not in cats:
        die("the report has no 'game' category")
    g = cats["game"]

    # The CATEGORY comes from each unit's own metadata, not from its
    # path. Filtering on a path prefix returned nothing at all, and an
    # empty table looks exactly like a repository with no matched units.
    game_units = []
    for u in rep["units"]:
        um = u["measures"]
        meta = u.get("metadata", {})
        if "game" not in (meta.get("progress_categories") or []):
            continue
        if not n(um.get("matched_code", 0)):
            continue
        game_units.append({
            "name": u["name"],
            "code": n(um.get("matched_code", 0)),
            "total": n(um.get("total_code", 0)),
            "fn": um.get("matched_functions", 0),
            "fnt": um.get("total_functions", 0),
            "complete": bool(meta.get("complete")),
        })
    game_units.sort(key=lambda x: (-x["complete"], -x["code"]))

    # GENERATED vs written, from the tool that OWNS that split and refuses
    # to report when the two halves do not add up to the category total.
    # This used to be a second copy here, keyed on a banner
    # ("GENERATED by tools/gen_typeids.py") that nothing writes any more --
    # so it counted zero generated and showed every matched function as
    # hand-written, which is the flattering direction and the one this
    # project has had to correct before.
    try:
        import written_vs_generated
        wvg = written_vs_generated.split()
    except SystemExit as e:
        die("written_vs_generated.py refused: %s" % e)
    except Exception as e:
        die("tools/written_vs_generated.py will not import (%s)" % e)
    gen_units, gen_fn, _gb = wvg["generated"]
    wr_units, wr_fn, _wb = wvg["written"]

    if not game_units:
        die("the report lists no game unit with matched code -- refusing to render an empty table that would read as no progress")
    nm = near_misses()
    bl_units, bl_bytes = blocked()
    data = data_tier(rep)
    flags = game_flags()

    done = n(g["matched_code"])
    total = n(g["total_code"])
    pct = 100.0 * done / total

    # The strip is 1,000 cells over the whole of Game Code, so one cell is
    # about 2,115 bytes. Anything matched lights at least one cell -- the
    # honest reading is "almost none of it", and rounding that to zero cells
    # would say something different.
    CELLS = 1000
    lit = max(1, round(CELLS * done / total))

    rows = []
    for u in game_units[:40]:
        state = "complete" if u["complete"] else "partial"
        label = "linked" if u["complete"] else "partial"
        rows.append(
            '<tr><td class="u">%s</td>'
            '<td class="num">%s</td><td class="num">%s</td>'
            '<td class="num">%d / %d</td>'
            '<td><span class="pill %s">%s</span></td></tr>'
            % (esc(u["name"].split("/")[-1]), comma(u["code"]),
               comma(u["total"]), u["fn"], u["fnt"], state, label))

    nm_rows = []
    for unit, ok, tot in nm:
        nm_rows.append(
            '<li><span class="nm-unit">%s</span>'
            '<span class="nm-score">%d of %d</span></li>'
            % (esc(unit.split("/")[-1]), ok, tot))

    cat_rows = []
    for cid, title in (("game", "Game Code"), ("engine", "Engine Code"),
                       ("sdk", "SDK Code"), ("Rev SDK", "Revolution SDK")):
        if cid not in cats:
            continue
        c = cats[cid]
        cd, ct = n(c["matched_code"]), n(c["total_code"])
        cat_rows.append(
            '<tr><td>%s</td><td class="num">%s</td><td class="num">%s</td>'
            '<td class="num">%.2f%%</td>'
            '<td class="num">%d / %d</td></tr>'
            % (esc(title), comma(cd), comma(ct), 100.0 * cd / ct,
               c.get("complete_units", 0), c["total_units"]))

    html = TEMPLATE % {
        "pct": "%.2f" % pct,
        "done": comma(done),
        "total": comma(total),
        "cells": "".join(
            '<i class="%s"></i>' % ("on" if k < lit else "off")
            for k in range(CELLS)),
        "lit": lit,
        "cellbytes": comma(round(total / CELLS)),
        "fn": comma(g["matched_functions"]),
        "fnt": comma(g["total_functions"]),
        "units": g.get("complete_units", 0),
        "unitst": g["total_units"],
        "rows": "\n".join(rows),
        "nrows": len(game_units),
        "catrows": "\n".join(cat_rows),
        "nmrows": "\n".join(nm_rows),
        "genfn": comma(gen_fn),
        "writtenfn": comma(wr_fn),
        "genunits": gen_units,
        "writtenunits": wr_units,
        "dataunits": data["units"],
        "databytes": comma(data["bytes"]),
        "datanocut": data["nocut"],
        "datacut": data["cut"],
        "datacan": data["can"],
        "blunits": bl_units,
        "allunits": comma(rep["measures"]["total_units"]),
        "blbytes": comma(bl_bytes),
        "flags": "".join('<code>%s</code>' % esc(f) for f in flags),
        "allmatched": comma(n(rep["measures"]["matched_code"])),
        "alltotal": comma(n(rep["measures"]["total_code"])),
    }
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(html, encoding="utf-8")
    print("build/dashboard.html: Game Code %s of %s bytes (%.2f%%), "
          "%d unit(s), %d near miss(es), %d unit(s) blocked"
          % (comma(done), comma(total), pct, len(game_units), len(nm),
             bl_units))
    return 0


TEMPLATE = """<title>Truth or Square, Wii</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link rel="stylesheet" href="https://fonts.googleapis.com/css2?\
family=Anybody:wght@500;700&family=DM+Mono:wght@400;500&\
family=Public+Sans:wght@400;500;600&display=swap">
<style>
:root {
  --ground: #F1F4F7;
  --surface: #FFFFFF;
  --line: #D7DEE5;
  --ink: #141D26;
  --muted: #66788A;
  --accent: #0A6FB4;
  --accent-soft: #DCEBF6;
  --ok: #17794C;
  --warn: #9A6410;
  --dim: #C3CDD6;
  --radius: 6px;
}
@media (prefers-color-scheme: dark) {
  :root:not([data-theme="light"]) {
    --ground: #0F1519;
    --surface: #161E24;
    --line: #27333C;
    --ink: #E4EAF0;
    --muted: #8496A5;
    --accent: #58BFEC;
    --accent-soft: #17303E;
    --ok: #4FBE86;
    --warn: #D9A441;
    --dim: #2E3B45;
  }
}
:root[data-theme="dark"] {
  --ground: #0F1519;
  --surface: #161E24;
  --line: #27333C;
  --ink: #E4EAF0;
  --muted: #8496A5;
  --accent: #58BFEC;
  --accent-soft: #17303E;
  --ok: #4FBE86;
  --warn: #D9A441;
  --dim: #2E3B45;
}

* { box-sizing: border-box; }
body {
  margin: 0;
  background: var(--ground);
  color: var(--ink);
  font-family: "Public Sans", ui-sans-serif, system-ui, sans-serif;
  font-size: 15px;
  line-height: 1.55;
  -webkit-font-smoothing: antialiased;
}
.wrap { max-width: 1080px; margin: 0 auto; padding: 48px 24px 72px; }

.eyebrow {
  font-family: "DM Mono", ui-monospace, monospace;
  font-size: 11px;
  letter-spacing: 0.14em;
  text-transform: uppercase;
  color: var(--muted);
}
h1 {
  font-family: "Anybody", ui-sans-serif, system-ui, sans-serif;
  font-weight: 700;
  font-size: clamp(30px, 4.4vw, 46px);
  line-height: 1.05;
  letter-spacing: -0.015em;
  margin: 10px 0 6px;
  text-wrap: balance;
}
h2 {
  font-family: "Anybody", ui-sans-serif, system-ui, sans-serif;
  font-weight: 600;
  font-size: 19px;
  letter-spacing: -0.005em;
  margin: 0 0 4px;
}
.sub { color: var(--muted); max-width: 66ch; margin: 0; }

.hero {
  margin-top: 34px;
  background: var(--surface);
  border: 1px solid var(--line);
  border-radius: var(--radius);
  padding: 26px 26px 22px;
}
.figure {
  display: flex;
  align-items: baseline;
  gap: 14px;
  flex-wrap: wrap;
}
.big {
  font-family: "DM Mono", ui-monospace, monospace;
  font-weight: 500;
  font-size: clamp(38px, 6vw, 58px);
  letter-spacing: -0.03em;
  color: var(--accent);
  font-variant-numeric: tabular-nums;
}
.of {
  font-family: "DM Mono", ui-monospace, monospace;
  font-size: 14px;
  color: var(--muted);
  font-variant-numeric: tabular-nums;
}
.strip {
  display: grid;
  grid-template-columns: repeat(100, 1fr);
  gap: 1px;
  margin: 20px 0 10px;
}
.strip i { display: block; height: 9px; background: var(--dim); }
.strip i.on { background: var(--accent); }
.legend {
  font-family: "DM Mono", ui-monospace, monospace;
  font-size: 11.5px;
  color: var(--muted);
  display: flex;
  justify-content: space-between;
  gap: 16px;
  flex-wrap: wrap;
}

.grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(210px, 1fr));
  gap: 14px;
  margin-top: 14px;
}
.stat {
  background: var(--surface);
  border: 1px solid var(--line);
  border-radius: var(--radius);
  padding: 16px 18px;
}
.stat .v {
  font-family: "DM Mono", ui-monospace, monospace;
  font-size: 24px;
  font-weight: 500;
  font-variant-numeric: tabular-nums;
}
.stat .k {
  font-family: "DM Mono", ui-monospace, monospace;
  font-size: 11px;
  letter-spacing: 0.12em;
  text-transform: uppercase;
  color: var(--muted);
  margin-top: 2px;
}

section { margin-top: 42px; }
.head { margin-bottom: 14px; }

.panel {
  background: var(--surface);
  border: 1px solid var(--line);
  border-radius: var(--radius);
  overflow-x: auto;
}
table { border-collapse: collapse; width: 100%%; font-size: 13.5px; }
th, td {
  text-align: left;
  padding: 9px 14px;
  border-bottom: 1px solid var(--line);
  white-space: nowrap;
}
th {
  font-family: "DM Mono", ui-monospace, monospace;
  font-size: 10.5px;
  letter-spacing: 0.12em;
  text-transform: uppercase;
  color: var(--muted);
  font-weight: 400;
}
tr:last-child td { border-bottom: none; }
td.num {
  font-family: "DM Mono", ui-monospace, monospace;
  font-variant-numeric: tabular-nums;
  text-align: right;
}
td.u { font-weight: 500; }

.pill {
  font-family: "DM Mono", ui-monospace, monospace;
  font-size: 10.5px;
  letter-spacing: 0.06em;
  text-transform: uppercase;
  padding: 2px 8px;
  border-radius: 100px;
  border: 1px solid;
}
.pill.complete { color: var(--ok); border-color: var(--ok); }
.pill.partial { color: var(--warn); border-color: var(--warn); }

ul.nm { list-style: none; margin: 0; padding: 0; }
ul.nm li {
  display: flex;
  justify-content: space-between;
  gap: 16px;
  padding: 9px 14px;
  border-bottom: 1px solid var(--line);
}
ul.nm li:last-child { border-bottom: none; }
.nm-unit { font-weight: 500; }
.nm-score {
  font-family: "DM Mono", ui-monospace, monospace;
  color: var(--warn);
  font-variant-numeric: tabular-nums;
}

.note {
  color: var(--muted);
  font-size: 13.5px;
  max-width: 72ch;
  margin: 10px 0 0;
}
code {
  font-family: "DM Mono", ui-monospace, monospace;
  font-size: 12px;
  background: var(--accent-soft);
  color: var(--accent);
  padding: 2px 7px;
  border-radius: 4px;
  margin-right: 6px;
  display: inline-block;
  margin-bottom: 5px;
}
footer {
  margin-top: 48px;
  padding-top: 18px;
  border-top: 1px solid var(--line);
  color: var(--muted);
  font-size: 12.5px;
}
</style>

<div class="wrap">
  <p class="eyebrow">SpongeBob&rsquo;s Truth or Square &middot; Wii &middot; R8IE78</p>
  <h1>Game code, byte for byte</h1>
  <p class="sub">Every figure on this page is read from dtk&rsquo;s report and
  the build configuration. Nothing is typed in by hand, so it cannot drift
  from the repository.</p>

  <div class="hero">
    <div class="figure">
      <span class="big">%(pct)s%%</span>
      <span class="of">%(done)s of %(total)s bytes of game code</span>
    </div>
    <div class="strip">%(cells)s</div>
    <div class="legend">
      <span>%(lit)s of 1,000 cells lit &middot; one cell &asymp; %(cellbytes)s bytes</span>
      <span>whole image: %(allmatched)s of %(alltotal)s bytes</span>
    </div>
  </div>

  <p class="note" style="margin-top:22px">%(genfn)s of the matched functions
  are <strong>generated</strong> &mdash; one shape, a 32-bit constant into a
  register and a return, emitted from the image by
  <code>tools/gen_typeids.py</code> across %(genunits)s unit(s). They are
  real matched functions and the constants are recovered fact, but a count
  of them is not a count of decompiled code. <strong>%(writtenfn)s were
  written.</strong></p>

  <div class="grid">
    <div class="stat"><div class="v">%(units)s / %(unitst)s</div>
      <div class="k">units linked</div></div>
    <div class="stat"><div class="v">%(fn)s / %(fnt)s</div>
      <div class="k">functions matched</div></div>
    <div class="stat"><div class="v">%(writtenfn)s</div>
      <div class="k">of those, written</div></div>
    <div class="stat"><div class="v">%(genfn)s</div>
      <div class="k">of those, generated</div></div>
    <div class="stat"><div class="v">%(blunits)s / %(allunits)s</div>
      <div class="k">units blocked</div></div>
    <div class="stat"><div class="v">%(blbytes)s</div>
      <div class="k">bytes behind that block</div></div>
  </div>

  <section>
    <div class="head">
      <h2>Units with matched code</h2>
      <p class="note">%(nrows)s of them. <strong>Linked</strong> means the
      unit&rsquo;s object defines no function the split does not name and every
      one of them is byte-identical in a placed run &mdash; the figure
      decomp.dev calls complete. <strong>Partial</strong> means some functions
      match and the unit does not yet link.</p>
    </div>
    <div class="panel">
      <table>
        <thead><tr><th>Unit</th><th>Matched</th><th>Total</th>
          <th>Functions</th><th>State</th></tr></thead>
        <tbody>%(rows)s</tbody>
      </table>
    </div>
  </section>

  <section>
    <div class="head">
      <h2>By category</h2>
      <p class="note">Game code is the column this work moves. The other
      three arrived with the fork and are shown so the headline figure
      is not mistaken for the whole image.</p>
    </div>
    <div class="panel">
      <table>
        <thead><tr><th>Category</th><th>Matched</th><th>Total</th>
          <th>Share</th><th>Units linked</th></tr></thead>
        <tbody>%(catrows)s</tbody>
      </table>
    </div>
  </section>

  <section>
    <div class="head">
      <h2>Data</h2>
      <p class="note">A unit that owns statics cannot be finished by matching
      its code alone &mdash; its data still sits in the parent chunk.
      <strong>%(dataunits)s unit(s) carry their own data, %(databytes)s
      bytes</strong>, and <strong>%(datacan)s more could</strong>:
      %(datanocut)s where no chunk has to be cut and %(datacut)s where the
      parent must be split and its upper half renamed. Three edits each, and
      only one of them is source: the definitions in address order, the
      split, and a force-active entry for every symbol nothing references
      &mdash; <code>config.yml</code> for a global,
      <code>#pragma force_active on</code> for an anonymous-namespace local,
      which cannot be forced from the linker command file at all.</p>
    </div>
  </section>

  <section>
    <div class="head">
      <h2>Near misses</h2>
      <p class="note">Written, measured, and short by a known amount. Each
      source file carries the mechanism and the list of spellings already
      excluded, so the next attempt starts from the register or the scheduler
      rather than from the C++.</p>
    </div>
    <div class="panel"><ul class="nm">%(nmrows)s</ul></div>
  </section>

  <section>
    <div class="head">
      <h2>Blocked by anonymous-namespace mangling</h2>
      <p class="note">CodeWarrior mangles an anonymous namespace with the name
      of the translation unit, so a function inside one carries
      <code>@unnamed@WAD02_cpp@</code> in retail because the source was
      compiled inside that unity blob. <strong>%(blunits)s units and
      %(blbytes)s bytes</strong> sit behind it. The way through is to name the
      source file after the blob at a different path, which reproduces the
      mangling exactly &mdash; proven for text by
      <code>Util/Sort/WAD02.cpp</code> and for data by
      <code>Core/Wii/Env/WAD00.cpp</code>, whose three statics now carry
      retail's own <code>@unnamed@WAD00_cpp@</code> names. The cost is the
      unit's real filename, which is why it is a decision per unit and not
      a sweep.</p>
    </div>
  </section>

  <section>
    <div class="head">
      <h2>Game library flags</h2>
      <p class="note">On top of the base flags. Each was found by writing a
      function, finding the body already identical, and looking at what the
      compiler did around it &mdash; then checked against every unit that
      already matched before being adopted.</p>
    </div>
    <p>%(flags)s</p>
  </section>

  <footer>Generated by <code>tools/dashboard.py</code> from
  <code>build/R8IE78/report.json</code>, <code>configure.py</code>,
  <code>tools/unitcmp_check.py</code> and <code>tools/anon_blocked.py</code>.
  Run <code>ninja</code> first; the report is what makes these numbers true.
  </footer>
</div>
"""

if __name__ == "__main__":
    sys.exit(main())
