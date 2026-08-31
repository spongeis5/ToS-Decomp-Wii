"""Prove tools/test_privacy.py FIRES, and fires for the right reasons.

    python tools/test_privacy_guard.py

The privacy checker passed on this repository for sixteen commits while a
home-directory path sat in a tracked file -- because it did not exist yet.
Now that it does, the failure mode moves: a checker that always says "ok"
looks exactly like a clean repository. So each rule is planted with a
violation it must reject, and the plant is removed again.

Nothing personal is written here either. The planted account names are
placeholders, and this file is on the checker's ALLOWED_PATHS for the same
reason the checker itself is: it has to contain the shapes it tests.
"""

import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
VICTIM = ROOT / "tools/dwarf_targets.py"      # tracked, and not allowlisted

# Built rather than written, so this file does not itself contain a literal
# home path -- the checker would refuse the commit that introduced it.
WIN = "C:" + "/Users/" + "placeholder" + "/work/thing"
NIX = "/home/" + "placeholder" + "/work/thing"
MAC = "/Users/" + "placeholder" + "/work/thing"


def run():
    r = subprocess.run([sys.executable, "tools/test_privacy.py"],
                       cwd=str(ROOT), capture_output=True, text=True)
    return r.returncode, r.stdout + r.stderr


def main():
    fails = 0

    rc, out = run()
    if rc != 0:
        print("  FAIL the checker does not pass on a clean tree:")
        print(out)
        return 1
    print("  ok   passes on the tree as it stands")

    backup = Path(tempfile.mkdtemp()) / "victim"
    shutil.copy2(VICTIM, backup)
    try:
        for label, planted in (("windows", WIN), ("linux", NIX),
                               ("macos", MAC)):
            text = backup.read_text(encoding="utf-8")
            VICTIM.write_text(text + "\n# see " + planted + "\n",
                              encoding="utf-8")
            rc, out = run()
            ok = rc != 0 and "no tracked file contains a home-directory path" \
                in out and "FAIL" in out
            fails += 0 if ok else 1
            print("  %-4s rejects a %s home path in a tracked file"
                  % ("ok" if ok else "FAIL", label))
            if not ok:
                print(out)
    finally:
        shutil.copy2(backup, VICTIM)

    rc, out = run()
    if rc != 0:
        print("  FAIL the plant was not cleaned up -- the tree is modified")
        print(out)
        fails += 1
    else:
        print("  ok   the plant was removed and the tree passes again")

    print("")
    print("  %d failure(s) of 5 check(s)" % fails)
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
