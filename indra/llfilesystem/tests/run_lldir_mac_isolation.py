#!/usr/bin/env python3

from __future__ import annotations

import os
from pathlib import Path
import stat
import subprocess
import sys
import tempfile


def snapshot_tree(path: Path) -> tuple[tuple[str, int, int, int, int], ...] | None:
    if not path.exists():
        return None

    entries = [path, *path.rglob("*")]
    snapshot = []
    for entry in entries:
        metadata = entry.lstat()
        snapshot.append(
            (
                str(entry.relative_to(path)),
                stat.S_IFMT(metadata.st_mode),
                metadata.st_size,
                metadata.st_mtime_ns,
                metadata.st_ctime_ns,
            )
        )
    return tuple(sorted(snapshot))


def run_test(executable: Path, mode: str, root: Path) -> None:
    environment = os.environ.copy()
    environment["LLDIR_TEST_MODE"] = mode
    environment["LLDIR_TEST_ROOT"] = str(root)

    if mode == "override":
        environment["SECONDLIFE_USER_DIR"] = str(root / "user")
        environment.pop("CFFIXED_USER_HOME", None)
    else:
        environment.pop("SECONDLIFE_USER_DIR", None)
        environment["CFFIXED_USER_HOME"] = str(root / "home")

    subprocess.run([executable], env=environment, check=True)


def main() -> int:
    if len(sys.argv) != 2:
        raise SystemExit("usage: run_lldir_mac_isolation.py TEST_EXECUTABLE")

    executable = Path(sys.argv[1]).resolve()
    standard_profile = Path.home() / "Library" / "Application Support" / "SecondLife"
    standard_cache = Path.home() / "Library" / "Caches" / "SecondLife"
    before = (snapshot_tree(standard_profile), snapshot_tree(standard_cache))

    with tempfile.TemporaryDirectory(prefix="lldir-mac-isolation-") as temp_name:
        temp = Path(temp_name)
        run_test(executable, "override", temp / "override")
        run_test(executable, "standard", temp / "standard")

    after = (snapshot_tree(standard_profile), snapshot_tree(standard_cache))
    if before != after:
        raise RuntimeError("the isolated LLDir tests changed the normal macOS profile or cache")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
