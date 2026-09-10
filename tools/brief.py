"""Everything the DWARF and the image know about ONE function, in one read.

    python tools/brief.py 0x80039730           the whole brief
    python tools/brief.py UpdateReference      by name substring
    python tools/brief.py 0x80039730 --map     the statement skeleton only
    python tools/brief.py --unit <unit>        the skeleton of every function
    python tools/brief.py X --around 0x8004B310 --window 96
    python tools/brief.py X --from 0x... --to 0x...

Four tools already read four different halves of this and none of them
joined: `disasm.py` prints instructions with no idea which source line
wrote them, `dwarf_lines.py` prints line rows with no idea what the
instructions are, `dwarf_locals.py` names the register a variable got
without showing a single use of it, and `dwarf_types.py` holds the layout
that turns `0x18(r30)` into a field name. Reading them one after another
was the loop; four terminal round trips per function, and the join done
in my head, which is where the mistakes came from.

So this prints the disassembly with THREE things attached to it:

  * THE SOURCE LINE of every instruction, in a column, so a statement's
    instructions can be picked out of the schedule that interleaved them.
    A backward step in line number is NOT a loop -- Clear() steps back
    five times and holds no branch at all; the scheduler simply moves
    instructions across statements. A backward BRANCH is a loop, and
    that is what gets marked.

  * THE VARIABLE in every register the DWARF named, scoped by the lexical
    block that declares it. Where two variables in scope claim one
    register the tool prints BOTH, separated by `|`. It does not pick.
    A register whose variable is ambiguous is a fact worth seeing; a
    register silently attributed to the wrong local is how a wrong
    conclusion gets four hours spent on it.

  * WHICH FILE each instruction came from, when it is not the function's
    own. Read that ONE WAY ONLY: a foreign file is evidence the code came
    from there, and a clean table is evidence of NOTHING. Measured, mwcc
    attributes inlined code to the CALL SITE -- Util::QuickSortInt puts
    624 of its 648 bytes on one line of Sort.cpp and names no other file
    at all -- so only 11 of 10,064 functions in this image show a foreign
    row, and eight of those are __sinit_ blobs. The absence of a foreign
    row does not make a function reachable.

The same warning applies to every absence this tool can show. 62.9% of
the image's functions record no local variable, and 81.5% of the ones
whose mangled name can be parsed record fewer parameters than that name
demands. So `(no parameters, locals or blocks in the debug info)` is the
ORDINARY case and says nothing about the source. Only what is PRESENT
here is evidence.

`--map` drops the instructions and prints the statement skeleton: each
source line once, in address order, with the bytes it owns. That is the
shape of the original, and it is the thing to read before writing a line.
"""

import argparse
import bisect
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ELF = ROOT / "orig/R8IE78/files/SB09WiiMASTERWAD.elf"
SPLITS = ROOT / "config/R8IE78/splits.txt"

sys.path.insert(0, str(ROOT / "tools"))
import disasm as D                                        # noqa: E402
import dwarf_types as T                                   # noqa: E402
from dwarf_locals import where                            # noqa: E402


def base_name(name):
    return str(name).replace(chr(92), "/").rsplit("/", 1)[-1]


class Var(object):
    def __init__(self, kind, name, typename, line, loc, lo, hi, die=-1):
        self.kind = kind            # "param" or "local"
        self.die = die              # which DECLARATION this range is of
        self.name = name
        self.typename = typename
        self.line = line
        self.loc = loc              # "r25", "f0", "frame +216", ...
        self.lo = lo                # enclosing scope, low pc
        self.hi = hi                # enclosing scope, high pc
        self.depth = 0

    @property
    def reg(self):
        m = re.match(r"^([rf]\d+)$", self.loc)
        return m.group(1) if m else None


class Brief(object):
    """One walk of the DWARF that keeps lines, scopes and locals together."""

    def __init__(self):
        self.ty = T.Types(ELF)
        self.lists = self.ty.dw.location_lists()
        self.rows = []            # (addr, file, line, is_stmt)
        self.funcs = {}           # low_pc -> dict
        self.bad_range = 0
        self.seen_loc = 0

        for cu in self.ty.dw.iter_CUs():
            top = cu.get_top_DIE()
            a = top.attributes.get("DW_AT_low_pc")
            cu_base = a.value if a else 0

            names = []
            prog = self.ty.dw.line_program_for_CU(cu)
            if prog is not None:
                for fe in prog.header.get("file_entry", []):
                    n = fe.name
                    names.append(n.decode("utf-8", "replace")
                                 if isinstance(n, bytes) else n)

            def fname(i, _names=names):
                if 1 <= i <= len(_names):
                    return base_name(_names[i - 1])
                return "?%d" % i

            if prog is not None:
                for e in prog.get_entries():
                    s = e.state
                    if s is None or s.end_sequence:
                        continue
                    self.rows.append((s.address, fname(s.file), s.line,
                                      bool(s.is_stmt)))

            self._walk_cu(cu, cu_base, fname)

        self.rows.sort()
        self._addrs = [r[0] for r in self.rows]
        if not self.rows:
            sys.exit("brief: the line table is empty. That is a failure to "
                     "read it, not a program with no lines.")
        if not self.funcs:
            sys.exit("brief: no function DIE carried a low_pc. That is a "
                     "failure to read the DWARF, not a program with no code.")

    def _walk_cu(self, cu, cu_base, fname):
        """Collect each subprogram with its variables and their SCOPES.

        The nesting matters and a flat list loses it: three locals of
        UpdateReferenceAnimationLOD all name r0, and which one a given
        instruction means is decided by the lexical block it is inside.
        """
        stack = []                    # (depth, lo, hi) of open scopes
        fn = None
        depth = 0
        for die in cu.iter_DIEs():
            if die.is_null():
                depth -= 1
                while stack and stack[-1][0] > depth:
                    stack.pop()
                if fn is not None and depth <= fn["depth"]:
                    fn = None
                continue

            if die.tag == "DW_TAG_subprogram":
                lo = die.attributes.get("DW_AT_low_pc")
                hi = die.attributes.get("DW_AT_high_pc")
                df = die.attributes.get("DW_AT_decl_file")
                dl = die.attributes.get("DW_AT_decl_line")
                if lo is not None and hi is not None:
                    fn = {"lo": lo.value, "hi": hi.value,
                          "name": T.name_of(die) or "?",
                          "file": fname(df.value) if df else "?",
                          "line": dl.value if dl else None,
                          "ret": self.ty.type_name(self.ty.ref(die)),
                          "vars": [], "blocks": [], "depth": depth,
                          "cu_base": cu_base}
                    self.funcs[lo.value] = fn
                    stack = [(depth, lo.value, hi.value)]
                else:
                    fn = None

            elif fn is not None and die.tag == "DW_TAG_lexical_block":
                bl = die.attributes.get("DW_AT_low_pc")
                bh = die.attributes.get("DW_AT_high_pc")
                if bl is not None and bh is not None:
                    stack.append((depth, bl.value, bh.value))
                    fn["blocks"].append((bl.value, bh.value, depth))

            elif fn is not None and die.tag in ("DW_TAG_formal_parameter",
                                                "DW_TAG_variable"):
                slo, shi = (stack[-1][1], stack[-1][2]) if stack else \
                    (fn["lo"], fn["hi"])
                ref = self.ty.ref(die)
                tn = self.ty.type_name(ref) + self.ty.array_suffix(ref)
                dl = die.attributes.get("DW_AT_decl_line")
                fn["ndie"] = fn.get("ndie", 0) + 1
                which = fn["ndie"]
                for lo_, hi_, loc in self._locs(die, cu_base, fn):
                    v = Var("param" if die.tag == "DW_TAG_formal_parameter"
                            else "local",
                            T.name_of(die) or "?", tn,
                            dl.value if dl else None, loc,
                            max(lo_, slo), min(hi_, shi), which)
                    v.depth = len(stack)
                    fn["vars"].append(v)

            if die.has_children:
                depth += 1

    def _locs(self, die, cu_base, fn):
        a = die.attributes.get("DW_AT_location")
        if a is None:
            return []
        self.seen_loc += 1
        if isinstance(a.value, list):
            return [(fn["lo"], fn["hi"], where(a.value))]
        out = []
        for e in self.lists.get_location_list_at_offset(a.value):
            ex = getattr(e, "loc_expr", None)
            if ex is None:
                continue
            b, n = cu_base + e.begin_offset, cu_base + e.end_offset
            if not (fn["lo"] <= b <= fn["hi"] and fn["lo"] <= n <= fn["hi"]):
                self.bad_range += 1
            out.append((b, n, where(ex)))
        return out

    def check(self):
        if self.bad_range:
            sys.exit("brief: %d location range(s) of %d fall outside the "
                     "function that owns them. Every register named beside "
                     "one of those is suspect. REFUSING to print."
                     % (self.bad_range, self.seen_loc))

    def line_rows(self, lo, hi):
        i = bisect.bisect_left(self._addrs, lo)
        out = []
        while i < len(self.rows) and self.rows[i][0] < hi:
            out.append(self.rows[i])
            i += 1
        return out

    def line_at(self, addr, rows):
        """-> (file, line, is_stmt) of the row that owns `addr`, or None."""
        best = None
        for r in rows:
            if r[0] <= addr:
                best = r
            else:
                break
        return best

    def find(self, what):
        if what.lower().startswith("0x"):
            a = int(what, 16)
            if a not in self.funcs:
                sys.exit("brief: no function DIE starts at %08X. A function "
                         "absent from the DWARF is not one with no debug "
                         "info -- check the address." % a)
            return [self.funcs[a]]
        hits = [f for f in self.funcs.values() if what in f["name"]]
        return sorted(hits, key=lambda f: f["lo"])


def var_column(fn, addr, dc):
    """Which named variables the registers of one instruction hold."""
    live = {}
    for v in fn["vars"]:
        r = v.reg
        if r is None or not (v.lo <= addr < v.hi):
            continue
        live.setdefault(r, [])
        if v.name not in live[r]:
            live[r].append(v.name)
    regs = re.findall(r"\b([rf]\d+)\b", dc.text)
    seen, out = set(), []
    for r in regs:
        if r in seen or r not in live:
            continue
        seen.add(r)
        out.append("%s=%s" % (r, "|".join(live[r])))
    return "  ".join(out)


def skeleton(B, fn):
    """-> [(line, file, first addr, bytes)] in address order, runs merged."""
    rows = B.line_rows(fn["lo"], fn["hi"])
    out = []
    for i, (a, f, ln, _s) in enumerate(rows):
        end = rows[i + 1][0] if i + 1 < len(rows) else fn["hi"]
        n = max(0, end - a)
        if out and out[-1][0] == ln and out[-1][1] == f:
            out[-1][3] += n
        else:
            out.append([ln, f, a, n])
    return out


def backward_branches(raw, secs, funcs, objs, fn):
    """-> {source addr: target addr} for every branch that goes BACKWARD.

    A step backward in line number is not a loop -- the scheduler
    interleaves statements freely, and Clear() alone steps back five
    times without a single branch in it. A backward BRANCH is a loop,
    and that is a fact from the encoding rather than from the line
    table, so it is the one this tool is allowed to state.
    """
    size = fn["hi"] - fn["lo"]
    words, rows, _unknown = D.annotate(raw, secs, funcs, objs,
                                       fn["lo"], size)
    if words is None:
        return {}
    out = {}
    for at, _w, _lab, dc, _note in rows:
        t = getattr(dc, "target", None)
        if t is not None and fn["lo"] <= t < fn["hi"] and t < at:
            out[at] = t
    return out


def show_map(B, fn, back=None):
    back = back or {}
    sk = skeleton(B, fn)
    print("")
    print("  %s" % fn["name"])
    print("  %08X..%08X, %d bytes, declared in %s line %s"
          % (fn["lo"], fn["hi"], fn["hi"] - fn["lo"], fn["file"],
             fn["line"]))
    foreign = sorted({f for _l, f, _a, _n in sk if f != fn["file"]})
    if foreign:
        print("  INLINED FROM: %s -- those bytes belong to a header and "
              "cannot be produced by writing this function." % ", ".join(
                  foreign))
    if not sk:
        print("    (no line rows -- the table does not cover this function)")
        return
    print("")
    heads = set(back.values())
    for ln, f, a, n in sk:
        marks = []
        for src, tgt in sorted(back.items()):
            if a <= src < a + n:
                marks.append("LOOP BOTTOM, branches back to %08X" % tgt)
        if any(a <= h < a + n for h in heads):
            marks.append("loop head")
        tag = "" if f == fn["file"] else "  [%s]" % f
        print("    line %-6d %08X  %4d B%s %s"
              % (ln, a, n, tag, "; ".join(marks)))
    lines = sorted({r[0] for r in sk})
    print("")
    print("    %d run(s) over %d distinct source line(s), %d..%d"
          % (len(sk), len(lines), lines[0], lines[-1]))


def show(B, fn, raw, secs, funcs, objs, lo=None, hi=None):
    size = fn["hi"] - fn["lo"]
    words, rows, unknown = D.annotate(raw, secs, funcs, objs, fn["lo"], size)
    if words is None:
        sys.exit("brief: %08X is in no loaded section" % fn["lo"])
    lrows = B.line_rows(fn["lo"], fn["hi"])

    print("")
    print("  %s" % fn["name"])
    print("  %08X..%08X   %d bytes, %d instruction(s)"
          % (fn["lo"], fn["hi"], size, len(words)))
    print("  declared in %s line %s, returns %s"
          % (fn["file"], fn["line"], fn["ret"] or "void"))

    foreign = sorted({f for _a, f, _l, _s in lrows if f != fn["file"]})
    if foreign:
        print("  INLINED FROM: %s" % ", ".join(foreign))
        print("  Those bytes belong to a header. Correct source for THIS")
        print("  function will not produce them.")

    print("")
    print("  SCOPES AND THE REGISTERS THE DWARF NAMED")
    if not fn["vars"]:
        print("    (no parameter or local carries a location)")
    for v in sorted(fn["vars"], key=lambda v: (v.lo, v.line or 0)):
        scope = ("whole function" if (v.lo, v.hi) == (fn["lo"], fn["hi"])
                 else "%08X..%08X" % (v.lo, v.hi))
        print("    %-6s line %-5s %-24s %-26s %-12s %s"
              % (v.kind, v.line, v.name[:24], v.typename[:26], v.loc, scope))

    back = backward_branches(raw, secs, funcs, objs, fn)
    heads = set(back.values())

    print("")
    print("  CODE   (the left column is the SOURCE LINE that wrote the")
    print("          instruction; a blank line is a change of statement)")
    if lo is not None or hi is not None:
        a = lo if lo is not None else fn["lo"]
        b = hi if hi is not None else fn["hi"]
        kept = [r for r in rows if a <= r[0] <= b]
        print("    WINDOW %08X..%08X: %d of %d instruction(s). The rest of"
              % (a, b, len(kept), len(rows)))
        print("    the function is NOT shown and is not being claimed on.")
        rows = kept
        if not rows:
            sys.exit("brief: no instruction of %s lies in %08X..%08X"
                     % (fn["name"], a, b))
    last = None
    for at, w, lab, dc, note in rows:
        r = B.line_at(at, lrows)
        tag = ""
        if r is None:
            col = "  ?  "
        else:
            col = "%5d" % r[2]
            if r[1] != fn["file"]:
                tag = " [INLINED %s]" % r[1]
            if (r[1], r[2]) != last:
                print("")
                last = (r[1], r[2])
        mark = ""
        if at in back:
            mark = "  <== LOOP BOTTOM, back to %08X" % back[at]
        elif at in heads:
            mark = "  <== loop head"
        vc = var_column(fn, at, dc)
        print("    %s  %08X  %08X  %-4s %-34s%-20s %s%s%s"
              % (col, at, w, lab + ":" if lab else "", dc.text, note, vc,
                 mark, tag))

    print("")
    print("  %d of %d instruction(s) decoded; %d printed as .word"
          % (len(words) - unknown, len(words), unknown))
    if unknown:
        print("  A .word is an encoding this tool does not know. It is NOT")
        print("  a no-op and NOT a guess -- decode it before writing source.")
    return unknown


def unit_funcs(B, unit):
    rs = [r for u, r in D.unit_ranges()
          if u == unit or u == unit + ".cpp"]
    if not rs:
        sys.exit("brief: splits.txt has no unit %r" % unit)
    out = [f for f in B.funcs.values()
           if any(lo <= f["lo"] < hi for lo, hi in rs[0])]
    if not out:
        sys.exit("brief: no function of %r is in the DWARF, which is not "
                 "the same as a unit with no functions." % unit)
    return sorted(out, key=lambda f: f["lo"])


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("what", nargs="?", help="0xADDRESS or a name substring")
    ap.add_argument("--unit", help="a unit name from splits.txt")
    ap.add_argument("--map", action="store_true",
                    help="the statement skeleton only, no instructions")
    ap.add_argument("--from", dest="lo", help="first address to print")
    ap.add_argument("--to", dest="hi", help="last address to print")
    ap.add_argument("--around", help="an address to centre a window on")
    ap.add_argument("--window", type=int, default=96,
                    help="bytes either side of --around (default 96)")
    args = ap.parse_args()
    if not args.what and not args.unit:
        sys.exit(__doc__)

    B = Brief()
    B.check()

    raw, secs, funcs, objs = D.load()

    if args.unit:
        picked = unit_funcs(B, args.unit)
        for fn in picked:
            show_map(B, fn,
                     backward_branches(raw, secs, funcs, objs, fn))
        print("")
        print("  %d function(s) in %s" % (len(picked), args.unit))
        return 0

    hits = B.find(args.what)
    if not hits:
        sys.exit("brief: no function name contains %r" % args.what)
    if len(hits) > 1 and not args.map:
        print("  %d function(s) match %r -- naming an address picks one:"
              % (len(hits), args.what))
        for f in hits:
            print("    0x%08X  %5d B  %s" % (f["lo"], f["hi"] - f["lo"],
                                             f["name"]))
        return 1

    if args.map:
        for fn in hits:
            show_map(B, fn, backward_branches(raw, secs, funcs, objs, fn))
        return 0

    lo = int(args.lo, 16) if args.lo else None
    hi = int(args.hi, 16) if args.hi else None
    if args.around:
        c = int(args.around, 16)
        lo, hi = c - args.window, c + args.window
    return 1 if show(B, hits[0], raw, secs, funcs, objs, lo, hi) else 0


if __name__ == "__main__":
    sys.exit(main())
