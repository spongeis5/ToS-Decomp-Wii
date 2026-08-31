"""Refuse to let identifying information reach a public repository.

    python tools/test_privacy.py

This repository is public. One tracked file carried the author's Windows
account name inside a hardcoded absolute path, and it went out in a commit
and sat in the tree of sixteen more before anyone thought to look. Thinking
to look is not a mechanism. This is the mechanism.

NOTHING PERSONAL IS WRITTEN IN THIS FILE, and that is a requirement rather
than a nicety: a checker that blocklists a name would publish that name to
every reader of the repository it is meant to protect. So every rule is
either DERIVED at runtime or an ALLOWLIST of shapes known to be safe.

  * The account name comes from `Path.home().name` on the machine running
    the check. It is never stored.
  * Commit identities are checked against a PATTERN -- GitHub's
    `<id>+<user>@users.noreply.github.com` privacy form, or an explicit
    noreply -- rather than against a list of forbidden addresses.
  * Home-directory paths are matched by shape: `C:/Users/<anything>/`,
    `/home/<anything>/`, `/Users/<anything>/`.

THE IDENTITY CHECK ONLY LOOKS AT THIS FORK'S OWN COMMITS. The history
inherited from upstream carries the upstream author's own address in their
own public repository; that is their identity, correctly attributed, and
rewriting it would put someone else's name on work under a different one.
The fork point is found from the upstream remote, and if that remote is
missing the check says so instead of quietly widening or narrowing.

Content in a tracked file can simply be edited. An identity already in
COMMIT HISTORY needs a history rewrite and a force-push, and once anyone
has cloned or forked the repository it cannot be recalled at all -- which
is why this runs before a commit rather than after a push.
"""

import os
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

# Two of the checks ask about THIS MACHINE, and a hosted CI runner is not it.
# They are not skipped for convenience -- they have no power there, and both
# answers they can give are wrong. A runner's home account is a service
# account, so searching tracked files for its name finds the word "runner"
# in every workflow file (a false FAIL); and on a runner whose account name
# is generic the check reports "ok" while never having looked for a person's
# name at all (a false PASS, which is the worse half).
#
# The condition is two-part on purpose, so setting CI=true on a real machine
# disables nothing: the environment must say hosted CI AND the account must
# be a service account rather than a person's.
SERVICE_ACCOUNTS = {"runner", "runneradmin", "root", "containeradmin",
                    "circleci", "jenkins", "buildkite-agent"}


def local_gate_reason():
    """-> why the machine-specific checks cannot run here, or None."""
    on_ci = (os.environ.get("GITHUB_ACTIONS") == "true"
             or os.environ.get("CI") == "true")
    if not on_ci:
        return None
    who = (Path.home().name or "").lower()
    if who not in SERVICE_ACCOUNTS:
        return None
    return ("hosted CI: the home account is the service account %r, so "
            "'this machine' is not a person's machine" % who)


# Identities allowed as commit author or committer. Shapes, not addresses:
# anything routing to a real mailbox is rejected by not matching.
ALLOWED_EMAIL = re.compile(
    r"^(?:[0-9]+\+)?[A-Za-z0-9._-]+@users\.noreply\.github\.com$"
    r"|^noreply@[A-Za-z0-9.-]+$")

# A home directory in any tracked file. The captured group is the account
# name, and it is exactly what must not be published.
HOME_PATH = re.compile(
    r"(?:[A-Za-z]:[/\\]Users[/\\]|/home/|/Users/)([A-Za-z0-9._-]+)")

# Files where a home-shaped path is documentation rather than a real path.
# Each needs a reason; "it was already there" is not one.
ALLOWED_PATHS = {
    # This file. It spells the three home-path shapes out in order to match
    # them, so without this entry it refuses the commit that introduces it.
    # The account components here are regex classes, not anyone's name.
    "test_privacy.py",
}

RESULTS = []
SKIPPED = []


def check(name, ok, detail=""):
    RESULTS.append(ok)
    print("  %-4s %s%s" % ("ok" if ok else "FAIL", name,
                           ("  -- " + detail) if detail else ""))


def not_here(name, why):
    """A check that cannot run in this environment. NOT a pass.

    Kept out of RESULTS entirely, so it can never be counted as evidence,
    and printed at the end where it cannot be skimmed past.
    """
    SKIPPED.append((name, why))
    print("  n/a  %s  -- %s" % (name, why))


def git(*args):
    r = subprocess.run(["git"] + list(args), cwd=str(ROOT),
                       capture_output=True, text=True)
    return r.stdout if r.returncode == 0 else None


def tracked():
    out = git("ls-files")
    return [l for l in (out or "").splitlines() if l.strip()]


def own_commit_range():
    """-> a rev range covering this fork's own commits, or None with a reason.

    Returns (range, None) or (None, reason).
    """
    remotes = (git("remote") or "").split()
    if "upstream" not in remotes:
        return None, ("no 'upstream' remote, so the fork point is unknown -- "
                      "refusing to guess whether inherited history counts")
    ref = None
    for cand in ("upstream/main", "upstream/master"):
        if git("rev-parse", "--verify", "--quiet", cand):
            ref = cand
            break
    if ref is None:
        return None, "the upstream remote has no fetched main or master branch"
    return "%s..HEAD" % ref, None


def main():
    print("privacy -- nothing here names a person; the rules are derived")
    print("")

    local_only = local_gate_reason()

    # 1. The account name of whoever is running this, in any tracked file.
    account = Path.home().name
    hits = []
    if local_only:
        not_here("this machine's account name is not in any tracked file",
                 local_only)
    elif account and len(account) >= 3:
        pat = re.compile(re.escape(account), re.I)
        for rel in tracked():
            try:
                text = (ROOT / rel).read_text(encoding="utf-8",
                                              errors="ignore")
            except OSError:
                continue
            for i, line in enumerate(text.splitlines(), 1):
                if pat.search(line):
                    hits.append((rel, i, line.strip()[:70]))
    else:
        not_here("this machine's account name is not in any tracked file",
                 "the home account name is too short to search for safely")
    if not local_only and account and len(account) >= 3:
        check("this machine's account name is not in any tracked file",
              not hits, "%d hit(s)" % len(hits) if hits else "")
        for rel, i, line in hits[:8]:
            print("       %s:%d  %s" % (rel, i, line))

    # 2. Any home-directory-shaped path, whoever it belongs to.
    hp = []
    for rel in tracked():
        if Path(rel).name in ALLOWED_PATHS:
            continue
        try:
            text = (ROOT / rel).read_text(encoding="utf-8", errors="ignore")
        except OSError:
            continue
        for i, line in enumerate(text.splitlines(), 1):
            if HOME_PATH.search(line):
                hp.append((rel, i, line.strip()[:70]))
    check("no tracked file contains a home-directory path", not hp,
          "%d hit(s)" % len(hp) if hp else "")
    for rel, i, line in hp[:8]:
        print("       %s:%d  %s" % (rel, i, line))

    # 3. Every identity on THIS FORK'S OWN commits must be a privacy address.
    rng, why = own_commit_range()
    if rng is None:
        check("every own-commit identity is a privacy address", False, why)
    else:
        log = git("log", rng, "--format=%an <%ae>%n%cn <%ce>")
        if log is None:
            check("every own-commit identity is a privacy address", False,
                  "git log %s unavailable" % rng)
        else:
            bad = set()
            lines = [l.strip() for l in log.splitlines() if "<" in l]
            for line in lines:
                email = line[line.rindex("<") + 1:line.rindex(">")]
                if not ALLOWED_EMAIL.match(email):
                    bad.add(email)
            check("every own-commit identity is a privacy address", not bad,
                  ", ".join(sorted(bad)) if bad
                  else "%d identity line(s) over %s" % (len(lines), rng))

    # 4. And the identity git would use for the NEXT commit.
    nxt = (git("config", "user.email") or "").strip()
    if local_only and not nxt:
        not_here("the configured commit email is a privacy address",
                 local_only + "; and no identity is configured")
    else:
        check("the configured commit email is a privacy address",
              bool(nxt) and bool(ALLOWED_EMAIL.match(nxt)),
              nxt or "user.email is unset")

    print("")
    bad_n = RESULTS.count(False)
    print("%d of %d applicable check(s) passed"
          % (len(RESULTS) - bad_n, len(RESULTS)))
    if SKIPPED:
        print("")
        print("%d CHECK(S) DID NOT RUN HERE and are not counted above:"
              % len(SKIPPED))
        for name, why in SKIPPED:
            print("  - %s  (%s)" % (name, why))
        print("Do not read this run as clearing what they cover.")
    if bad_n:
        print("")
        print("Publishing now would put the above into a public repository.")
        print("Content in a tracked file can simply be edited. An identity")
        print("already in COMMIT HISTORY needs a history rewrite and a")
        print("force-push, and once the repository has been cloned or forked")
        print("it cannot be recalled at all.")
    return 1 if bad_n else 0


if __name__ == "__main__":
    sys.exit(main())
