import os
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "build" / "host-tests"
TESTS = [
    ("test_types_contract", ROOT / "tests" / "unit" / "test_types_contract.c"),
    ("test_config_defaults", ROOT / "tests" / "unit" / "test_config_defaults.c"),
]


def run(command):
    completed = subprocess.run(command, cwd=ROOT)
    return completed.returncode


def main():
    BUILD.mkdir(parents=True, exist_ok=True)
    failures = 0
    for name, source in TESTS:
        exe = BUILD / f"{name}.exe"
        compile_command = [
            "gcc",
            "-std=c99",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-Iinclude",
            "-Itests/support",
            str(source),
            "-o",
            str(exe),
        ]
        print(f"[build] {name}")
        if run(compile_command) != 0:
            failures += 1
            continue
        print(f"[test] {name}")
        if run([str(exe)]) != 0:
            failures += 1
    if failures != 0:
        print(f"[summary] {failures} test target(s) failed")
        return 1
    print(f"[summary] {len(TESTS)} test target(s) passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
