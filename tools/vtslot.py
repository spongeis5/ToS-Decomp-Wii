"""Name the function a vtable holds at each byte offset.

    python tools/vtslot.py <vtable symbol> <offset> [<offset> ...]

    python tools/vtslot.py __vt__10zNPCCombat 52
    __vt__10zNPCCombat+52 (806C117C) -> 8009B730 IsDead__10zNPCCombatCFv

A virtual call leaves no relocation, only `lwz r12,N(r12)`, so the name of
what it calls is not in the code. It is in the data: the vtable's address
comes from config/R8IE78/symbols.txt, the pointer at each offset from
main.dol (the `object:` of config.yml), and the pointer's name from
symbols.txt again. Offsets are bytes into the vtable, as the `lwz` spells
them; mwcc's first virtual sits at 8, after the RTTI word and the offset.

Before answering it resolves one slot whose name the matched code already
depends on -- zPlayer's IsAI at 428, called by every behaviour-tree unit --
and refuses to report if that does not come back, so a misread section
table cannot produce plausible wrong names. A symbol, offset or pointer
that cannot be resolved is reported as such and makes the exit code 1.
"""
import re
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SYM = re.compile(r"^(\S+) = \.[^:]+:0x([0-9A-Fa-f]+);")
KNOWN = [("__vt__7zPlayer", 428, "IsAI__7zPlayerCFv")]


def load():
    by_name, by_addr = {}, {}
    with open(ROOT / "config/R8IE78/symbols.txt", encoding="utf-8") as f:
        for line in f:
            m = SYM.match(line)
            if m:
                a = int(m.group(2), 16)
                by_name.setdefault(m.group(1), []).append(a)
                by_addr.setdefault(a, []).append(m.group(1))
    cfg = (ROOT / "config/R8IE78/config.yml").read_text(encoding="utf-8")
    m = re.search(r"^object:\s*(\S+)", cfg, re.M)
    if not m:
        sys.exit("vtslot: no object: line in config.yml")
    dol = (ROOT / m.group(1)).read_bytes()
    return by_name, by_addr, dol


def read_word(dol, addr):
    offs = struct.unpack(">18I", dol[0:72])
    starts = struct.unpack(">18I", dol[72:144])
    sizes = struct.unpack(">18I", dol[144:216])
    for off, start, size in zip(offs, starts, sizes):
        if size and start <= addr and addr + 4 <= start + size:
            pos = off + addr - start
            return struct.unpack(">I", dol[pos:pos + 4])[0]
    return None


def resolve(by_name, by_addr, dol, vt, offset):
    """-> (slot address, pointer, names) or an error string."""
    addrs = by_name.get(vt, [])
    if len(addrs) != 1:
        return "%d symbols named %s" % (len(addrs), vt)
    slot = addrs[0] + offset
    w = read_word(dol, slot)
    if w is None:
        return "%s+%d (%08X) is in no DOL section" % (vt, offset, slot)
    names = by_addr.get(w)
    if not names:
        return "%s+%d (%08X) -> %08X, no symbol there" % (vt, offset, slot, w)
    return slot, w, names


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    by_name, by_addr, dol = load()
    for vt, offset, want in KNOWN:
        got = resolve(by_name, by_addr, dol, vt, offset)
        if isinstance(got, str) or want not in got[2]:
            sys.exit("vtslot: REFUSING. Known answer %s+%d = %s came back as "
                     "%r" % (vt, offset, want, got))
    vt = sys.argv[1]
    bad = 0
    for arg in sys.argv[2:]:
        offset = int(arg, 0)
        got = resolve(by_name, by_addr, dol, vt, offset)
        if isinstance(got, str):
            print(got)
            bad += 1
            continue
        slot, w, names = got
        print("%s+%d (%08X) -> %08X %s" % (vt, offset, slot, w,
                                          " | ".join(names)))
    print("%d offset(s), %d unresolved" % (len(sys.argv) - 2, bad))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
