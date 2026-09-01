"""Compile one unit with the game flags and compare each FUNCTION, by NAME,
against the retail ELF.

    python tools/unitcmp.py SB/NG/Source/Engine/Util/Containers [-v]

THIS IS AN ITERATION AID, NOT THE ORACLE.  `ninja` decides whether a unit is
matched and whether the image still links, and it is the only thing that can:
a function can be byte-identical here while naming a symbol that does not
exist, which is exactly how zCamSplineCommonMix scored 100% and failed to
link.  Use this to iterate in seconds, then run ninja and read
`main.dol: OK`.

It exists because objdiff answers "how close is this unit" and the question
while writing a function is "which of these five is still wrong, and where".

ONE implementation of that comparison lives here, imported by every sweep
rather than copied into it.  Copies are how a sweep and a check come to
disagree, always in the direction that gets believed.

Two things it is careful about:

RELOCATED FIELDS ARE MASKED, and the masked count is printed with every
verdict.  An unlinked object holds 0 where a relocation will go; retail holds
the resolved value.  Comparing those raw reports differences that do not
exist -- it made FixedAllocator's Alloc and Free read as 33/36 and 23/25 when
both were already right.  Masking too much is the worse error, so the mask is
per relocation TYPE and covers only the bits that type writes; an unknown
type is refused rather than guessed at.

THE FLAGS ARE CHECKED AGAINST configure.py at import.  They have to be
repeated here (configure.py does its work at import time and cannot be
imported for its variables), and a stale copy would measure a build nobody
makes.  So the copy is verified, and the module refuses to load if it drifts.
"""
import re
import subprocess
import sys
from pathlib import Path

from elftools.elf.elffile import ELFFile
from elftools.elf.relocation import RelocationSection

REPO = Path(__file__).resolve().parent.parent
RETAIL = REPO / "orig/R8IE78/files/SB09WiiMASTERWAD.elf"
CC = "./build/compilers/Wii/1.1/mwcceppc.exe"
CACHE = REPO / "build/retail_funcs.pickle"

# The subset of cflags_base that affects codegen, plus the include paths a
# unit actually needs.  _check_flags() below asserts this still agrees with
# configure.py.
BASE = ["-c", "-nodefaults", "-proc", "gekko", "-align", "powerpc",
        "-enum", "int", "-fp", "hardware", "-Cpp_exceptions", "off",
        "-inline", "auto", "-pragma", "cats off",
        "-pragma", "warn_notinlined off", "-maxerrors", "1",
        "-maxwarnings", "1", "-nosyspath", "-RTTI", "off",
        "-fp_contract", "on", "-str", "reuse", "-enc", "SJIS",
        "-DBUILD_VERSION=0", "-DREVOLUTION", "-DNDEBUG=1", "-lang=c++",
        "-O4,s", "-sdata", "0", "-sdata2", "0", "-use_lmw_stmw", "on",
        "-i", "src", "-i", "src/MSL_C/include", "-i", "src/Revolution/include",
        "-i", "src/SB/include"]

# What cflags_game adds on top of cflags_base, and what it drops.
GAME_EXTRA = ["-O4,s", "-sdata", "0", "-sdata2", "0", "-use_lmw_stmw", "on"]
GAME_DROPS = "-O4,p"

RELOC_BITS = {
    "R_PPC_ADDR32": 0xFFFFFFFF,
    "R_PPC_ADDR16": 0x0000FFFF,
    "R_PPC_ADDR16_LO": 0x0000FFFF,
    "R_PPC_ADDR16_HI": 0x0000FFFF,
    "R_PPC_ADDR16_HA": 0x0000FFFF,
    "R_PPC_ADDR24": 0x03FFFFFC,
    "R_PPC_ADDR14": 0x0000FFFC,
    "R_PPC_REL24": 0x03FFFFFC,
    "R_PPC_REL14": 0x0000FFFC,
    "R_PPC_REL32": 0xFFFFFFFF,
    "R_PPC_UADDR32": 0xFFFFFFFF,
    "R_PPC_EMB_SDA21": 0x001FFFFF,
    "R_PPC_SDAREL16": 0x0000FFFF,
}

_PPC_RELOC = {
    1: "R_PPC_ADDR32", 3: "R_PPC_ADDR16", 4: "R_PPC_ADDR16_LO",
    5: "R_PPC_ADDR16_HI", 6: "R_PPC_ADDR16_HA", 10: "R_PPC_REL24",
    11: "R_PPC_REL14", 26: "R_PPC_REL32", 109: "R_PPC_EMB_SDA21",
}


def _check_flags():
    """Refuse to run if BASE has drifted from configure.py's cflags_game."""
    txt = (REPO / "configure.py").read_text()

    m = re.search(r"cflags_game\s*=\s*\((.*?)\)\s*\n", txt, re.S)
    if not m:
        raise SystemExit("unitcmp: cannot find cflags_game in configure.py -- "
                         "refusing to measure a build I cannot verify")
    expr = m.group(1)

    extra = re.findall(r'"([^"]+)"', expr.split("+", 1)[1])
    if extra != GAME_EXTRA:
        raise SystemExit(
            "unitcmp: configure.py's cflags_game now adds %r, this module has "
            "%r. Update BASE and GAME_EXTRA together." % (extra, GAME_EXTRA))

    drops = re.search(r'!=\s*"([^"]+)"', expr)
    if not drops or drops.group(1) != GAME_DROPS:
        raise SystemExit(
            "unitcmp: configure.py's cflags_game no longer drops %r"
            % GAME_DROPS)

    base = re.search(r"cflags_base\s*=\s*\[(.*?)\n\]", txt, re.S)
    if not base:
        raise SystemExit("unitcmp: cannot find cflags_base in configure.py")

    # COMMENTED-OUT flags are not flags.  Scanning the block raw made this
    # guard fire on '-W all', which configure.py carries only as `# "-W all",`
    # -- a guard that fires on correct input is worse than no guard, because
    # it teaches you to reach past guards.
    lines = [ln for ln in base.group(1).splitlines()
             if not ln.lstrip().startswith("#")]

    have = " ".join(BASE)
    for flag in re.findall(r'"([^"]*)"', "\n".join(lines)):
        flag = flag.strip()
        if not flag or flag.startswith(("-i", "-D", "-ir")) or flag == GAME_DROPS:
            continue                      # paths and defines are set above
        if flag not in have:
            raise SystemExit(
                "unitcmp: configure.py's cflags_base carries %r and this "
                "module does not. Update BASE." % flag)


_check_flags()

# -v prints mnemonics when a PowerPC disassembler is importable, and raw
# hex words otherwise.  Either way the COMPARISON is on bytes, so a missing
# disassembler changes what you read, never what the tool decides.
try:
    import ppcdis
except ImportError:
    ppcdis = None

_retail_cache = {}


def _reloc_name(t):
    return _PPC_RELOC.get(t, "R_PPC_%d" % t)


def load(path, want_relocs):
    """name -> (bytes, {word: mask}, {word: (symbol, addend)}).

    The third element is only for the REL24 relocations -- a branch whose
    whole field is masked, and therefore the one place where masking would
    otherwise leave nothing measured at all.
    """
    out = {}
    with open(path, "rb") as fh:
        f = ELFFile(fh)
        secs = list(f.iter_sections())

        relocs = {}
        branches = {}
        if want_relocs:
            for sec in secs:
                if not isinstance(sec, RelocationSection):
                    continue
                d = relocs.setdefault(sec.header["sh_info"], {})
                bd = branches.setdefault(sec.header["sh_info"], {})
                symtab = f.get_section(sec.header["sh_link"])
                for r in sec.iter_relocations():
                    nm = _reloc_name(r["r_info_type"])
                    if nm not in RELOC_BITS:
                        raise SystemExit(
                            "unitcmp: unknown relocation %r at %#x -- refusing "
                            "to guess which bits it writes"
                            % (nm, r["r_offset"]))
                    d[r["r_offset"]] = d.get(r["r_offset"], 0) | RELOC_BITS[nm]
                    if nm == "R_PPC_REL24":
                        sym = symtab.get_symbol(r["r_info_sym"])
                        bd[r["r_offset"]] = (
                            sym.name, r["r_addend"] if "r_addend" in
                            r.entry else 0)

        for sec in secs:
            if sec.header["sh_type"] != "SHT_SYMTAB":
                continue
            for s in sec.iter_symbols():
                if s["st_info"]["type"] != "STT_FUNC" or not s["st_size"]:
                    continue
                if not isinstance(s["st_shndx"], int):
                    continue
                tgt = f.get_section(s["st_shndx"])
                off = s["st_value"] - (tgt.header["sh_addr"] or 0)
                body = tgt.data()[off:off + s["st_size"]]
                m = {}
                for ro, mask in relocs.get(s["st_shndx"], {}).items():
                    if off <= ro < off + s["st_size"]:
                        m[(ro - off) // 4] = mask
                br = {}
                for ro, ref in branches.get(s["st_shndx"], {}).items():
                    if off <= ro < off + s["st_size"]:
                        br[(ro - off) // 4] = ref
                out[s.name] = (body, m, br)
    return out


def retail():
    """Retail's function bytes, cached on disk.

    Walking the retail symtab takes most of a minute and the file does not
    change, so the cache is keyed on its size and mtime and rebuilt when
    either moves -- never reused blind.
    """
    if "x" in _retail_cache:
        return _retail_cache["x"]

    import pickle
    st = RETAIL.stat()
    key = (st.st_size, int(st.st_mtime))
    if CACHE.exists():
        with open(CACHE, "rb") as fh:
            got, data = pickle.load(fh)
        if got == key:
            _retail_cache["x"] = data
            return data

    data = load(RETAIL, False)
    CACHE.parent.mkdir(parents=True, exist_ok=True)
    with open(CACHE, "wb") as fh:
        pickle.dump((key, data), fh, protocol=4)
    _retail_cache["x"] = data
    return data


_ADDR_CACHE = {}
ADDRS = REPO / "build/retail_addrs.pickle"


def retail_addrs():
    """(name -> address, address -> name) for retail's functions.

    Needed to answer what a resolved branch in the retail image points AT.
    Cached beside the body cache and keyed the same way; a separate file
    rather than a wider one, so an existing cache is never read as if it
    held a field it does not.
    """
    import pickle
    if "x" in _ADDR_CACHE:
        return _ADDR_CACHE["x"]
    st = RETAIL.stat()
    key = (st.st_size, int(st.st_mtime))
    if ADDRS.exists():
        with open(ADDRS, "rb") as fh:
            got, data = pickle.load(fh)
        if got == key:
            _ADDR_CACHE["x"] = data
            return data

    # LAST wins, exactly as retail() does for the bodies. A name the image
    # carries twice would otherwise take its body from one copy and its
    # address from the other, and every branch would resolve from the wrong
    # base -- which is how this check first fired on correct input.
    byname, byaddr = {}, {}
    with open(RETAIL, "rb") as fh:
        for sec in ELFFile(fh).iter_sections():
            if sec.header["sh_type"] != "SHT_SYMTAB":
                continue
            for s in sec.iter_symbols():
                if s["st_info"]["type"] == "STT_FUNC" and s["st_size"]:
                    byname[s.name] = s["st_value"]
                # EVERY named symbol goes in the target map, sized or not.
                # _savegpr_29 and _restgpr_29 are interior labels of one
                # routine and carry no size, so a sized-functions-only map
                # has no name at the address every prologue branches to --
                # and called two correct calls wrong.
                if s.name:
                    byaddr.setdefault(s["st_value"], set()).add(s.name)
    data = (byname, byaddr)
    ADDRS.parent.mkdir(parents=True, exist_ok=True)
    with open(ADDRS, "wb") as fh:
        pickle.dump((key, data), fh, protocol=4)
    _ADDR_CACHE["x"] = data
    return data


def branch_target(addr, word):
    """Where a resolved b/bl at `addr` goes, or None if it is not one."""
    op = word >> 26
    if op != 18:
        return None
    disp = word & 0x03FFFFFC
    if disp & 0x02000000:
        disp -= 0x04000000
    return disp if word & 2 else addr + disp


def words(b):
    return [int.from_bytes(b[i:i + 4], "big") for i in range(0, len(b), 4)]


def dis(b):
    if ppcdis is None:
        return ["%08x" % w for w in words(b)]
    return [t for _v, _w, t in ppcdis.words(words(b))]


SOURCE_EXTS = (".cpp", ".cxx", ".cc", ".c")


def source_of(unit):
    """Not every unit is a .cpp -- keycode is a .cxx, and appending .cpp to
    it silently compiles nothing and reports zero functions."""
    if unit.endswith(SOURCE_EXTS):
        return "src/" + unit
    for ext in SOURCE_EXTS:
        if (REPO / ("src/" + unit + ext)).exists():
            return "src/" + unit + ext
    return "src/" + unit + ".cpp"


def compile_unit(unit, extra=()):
    obj = REPO / "build/_unitcmp.o"
    r = subprocess.run(
        [CC] + BASE + list(extra) + ["-o", str(obj), source_of(unit)],
        cwd=str(REPO), capture_output=True, text=True)
    if r.returncode:
        return None, "COMPILE FAILED (exit %d)\n%s%s" % (
            r.returncode, r.stdout, r.stderr)
    return obj, None


def compare(unit, extra=()):
    """unit -> {name: (differing, compared, masked, unmeasured)}.

    words_differing is -1 for a function this object defines that retail does
    not have.  Returns the compiler's message as a plain STRING when the unit
    will not build, so a caller cannot read a failed compile as a clean sweep.

    A masked REL24 is checked by NAME: our relocation says which symbol the
    branch is to, retail's resolved displacement lands on one, and the two
    have to agree.  `unmeasured` counts the masked words where even that was
    not possible, so a function with nothing compared cannot read as a match.
    """
    obj, err = compile_unit(unit, extra)
    if err:
        return err

    mine, want = load(obj, True), retail()
    byname, byaddr = retail_addrs()
    res = {}
    for name, (got, masks, brs) in mine.items():
        if name not in want:
            res[name] = (-1, 0, 0, 0)
            continue
        a, b = words(got), words(want[name][0])
        n = min(len(a), len(b))
        bad = sum(1 for i in range(n)
                  if (a[i] & ~masks.get(i, 0)) != (b[i] & ~masks.get(i, 0)))

        here = byname.get(name)
        unmeasured = 0
        for i in range(n):
            if i not in masks:
                continue
            ref = brs.get(i)
            if ref is None:
                continue          # not a branch; its other fields compared
            tgt = branch_target(here + 4 * i, b[i]) if here is not None \
                else None
            if tgt is None or ref[1]:
                # An addend, or a retail word that is not a branch at all.
                # Nothing in this word was compared, so say so.
                unmeasured += 1
                continue
            names = byaddr.get(tgt)
            if not names:
                # Nothing is named there, so there is nothing to compare
                # this against. Not evidence of a wrong target.
                unmeasured += 1
                continue
            if ref[0] not in names:
                bad += 1
        res[name] = (bad + abs(len(a) - len(b)), n,
                     len([i for i in range(n) if i in masks]), unmeasured)
    return res


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    unit = sys.argv[1]
    verbose = "-v" in sys.argv[2:]

    res = compare(unit)
    if isinstance(res, str):
        print(res)
        return 2

    obj, err = compile_unit(unit)
    mine, want = load(obj, True), retail()

    total = ok = unmeas = 0
    for name in sorted(res, key=lambda n: -len(mine[n][0])):
        bad, n, masked, unmeasured = res[name]
        got = mine[name][0]
        total += 1
        if bad < 0:
            print("  %-6s %-5d %s   NOT IN RETAIL" % ("EXTRA", len(got), name))
            continue
        if bad == 0 and unmeasured == n:
            # Nothing was compared: every word is a relocated field whose
            # target could not be resolved to a name. Reporting MATCH here
            # is the benign value a failed measurement must never produce.
            unmeas += 1
            print("  %-6s %-5d %s   %d words, ALL masked and unresolvable"
                  % ("UNMEAS", len(got), name, n))
            continue
        if bad == 0:
            ok += 1
            print("  %-6s %-5d %s   %d words, %d masked by relocation"
                  "%s"
                  % ("MATCH", len(got), name, n, masked,
                     ", %d unmeasured" % unmeasured if unmeasured else ""))
            continue
        exp = want[name][0]
        print("  %-6s %-5d %s   %d of %d words differ "
              "(%d masked; retail %d B)"
              % ("DIFFER", len(got), name, bad, n, masked, len(exp)))
        if verbose:
            da, db, masks = dis(got), dis(exp), mine[name][1]
            a, b = words(got), words(exp)
            for i in range(max(len(da), len(db))):
                x = da[i] if i < len(da) else ""
                y = db[i] if i < len(db) else ""
                if i in masks:
                    tag = "  (reloc)"
                elif i >= len(a) or i >= len(b):
                    tag = "  <<"
                elif (a[i] & ~masks.get(i, 0)) != (b[i] & ~masks.get(i, 0)):
                    tag = "  <<"
                else:
                    tag = ""
                print("      %-3d %-34s %-34s%s" % (i, x, y, tag))

    print("")
    if unmeas:
        print("  %d function(s) could not be measured at all -- every word a"
              " relocated field with no name to check against." % unmeas)
    print("  %d of %d function(s) defined by the object are byte-identical"
          % (ok, total))
    print("  This is not the oracle: run `ninja` and read `main.dol: OK`.")
    return 0 if total and ok == total else 1


if __name__ == "__main__":
    sys.exit(main())
