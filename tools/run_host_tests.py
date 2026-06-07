import os
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "build" / "host-tests"
TESTS = [
    ("test_types_contract", ROOT / "tests" / "unit" / "test_types_contract.c"),
    ("test_config_defaults", ROOT / "tests" / "unit" / "test_config_defaults.c"),
    ("test_list", ROOT / "tests" / "unit" / "test_list.c"),
    ("test_priority_bitmap", ROOT / "tests" / "unit" / "test_priority_bitmap.c"),
    ("test_port_mock", ROOT / "tests" / "port_mock" / "test_port_mock.c"),
    ("test_kernel_tick", ROOT / "tests" / "unit" / "test_kernel_tick.c"),
    ("test_task_create_static", ROOT / "tests" / "sim" / "test_task_create_static.c"),
    ("test_scheduler_start", ROOT / "tests" / "sim" / "test_scheduler_start.c"),
    ("test_scheduler_round_robin", ROOT / "tests" / "sim" / "test_scheduler_round_robin.c"),
    ("test_task_delay", ROOT / "tests" / "sim" / "test_task_delay.c"),
    ("test_task_delay_overflow", ROOT / "tests" / "sim" / "test_task_delay_overflow.c"),
    ("test_queue_create_static", ROOT / "tests" / "unit" / "test_queue_create_static.c"),
    ("test_queue_send_receive", ROOT / "tests" / "unit" / "test_queue_send_receive.c"),
    ("test_queue_variants", ROOT / "tests" / "unit" / "test_queue_variants.c"),
]

KERNEL_SOURCES = [
    ROOT / "src" / "kernel" / "mrt_list.c",
    ROOT / "src" / "kernel" / "mrt_kernel.c",
    ROOT / "src" / "kernel" / "mrt_priority.c",
    ROOT / "src" / "kernel" / "mrt_queue.c",
    ROOT / "src" / "kernel" / "mrt_task.c",
    ROOT / "src" / "portable" / "mock" / "mrt_port_mock.c",
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
            "-DMRT_TESTING=1",
            "-Iinclude",
            "-Itests/support",
            str(source),
            *[str(kernel_source) for kernel_source in KERNEL_SOURCES],
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
