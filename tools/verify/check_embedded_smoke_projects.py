import shutil
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
BUILD = ROOT / "build" / "embedded-smoke"

KERNEL_SOURCES = [
    ROOT / "src" / "kernel" / "mrt_assert.c",
    ROOT / "src" / "kernel" / "mrt_event_group.c",
    ROOT / "src" / "kernel" / "mrt_heap.c",
    ROOT / "src" / "kernel" / "mrt_kernel.c",
    ROOT / "src" / "kernel" / "mrt_list.c",
    ROOT / "src" / "kernel" / "mrt_memory_pool.c",
    ROOT / "src" / "kernel" / "mrt_message_buffer.c",
    ROOT / "src" / "kernel" / "mrt_mutex.c",
    ROOT / "src" / "kernel" / "mrt_priority.c",
    ROOT / "src" / "kernel" / "mrt_queue.c",
    ROOT / "src" / "kernel" / "mrt_semaphore.c",
    ROOT / "src" / "kernel" / "mrt_stats.c",
    ROOT / "src" / "kernel" / "mrt_stream_buffer.c",
    ROOT / "src" / "kernel" / "mrt_task.c",
    ROOT / "src" / "kernel" / "mrt_tickless.c",
    ROOT / "src" / "kernel" / "mrt_timer.c",
    ROOT / "src" / "kernel" / "mrt_trace.c",
]


def require_path(path):
    if not path.exists():
        print(f"[embedded-smoke] missing required path: {path.relative_to(ROOT)}")
        return 1
    return 0


def run(command):
    printable = " ".join(str(part) for part in command)
    print(f"[embedded-smoke] run: {printable}")
    completed = subprocess.run(command, cwd=ROOT)
    return completed.returncode


def compile_stm32_smoke():
    arm_gcc = shutil.which("arm-none-eabi-gcc")
    if arm_gcc is None:
        print("[embedded-smoke] arm-none-eabi-gcc not found")
        return 1

    output = BUILD / "stm32_smoke.elf"
    command = [
        arm_gcc,
        "-std=c99",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-ffreestanding",
        "-fno-builtin",
        "-fdata-sections",
        "-ffunction-sections",
        "-mcpu=cortex-m4",
        "-mthumb",
        "-nostdlib",
        "-Iinclude",
        "-Iexamples/stm32",
        *[str(source) for source in KERNEL_SOURCES],
        "src/portable/stm32_cm/mrt_port_stm32_cm.c",
        "src/portable/stm32_cm/mrt_port_stm32_cm_context.S",
        "examples/stm32/main.c",
        "examples/stm32/mrt_port_stm32_smoke.c",
        "examples/stm32/startup_stm32cm.c",
        "examples/stm32/runtime_stubs.c",
        "-Wl,--gc-sections",
        "-Wl,-T,examples/stm32/linker.ld",
        "-Wl,-Map,build/embedded-smoke/stm32_smoke.map",
        "-lgcc",
        "-o",
        str(output),
    ]
    return run(command)


def compile_and_run_dsp_model():
    host_gcc = shutil.which("gcc")
    if host_gcc is None:
        print("[embedded-smoke] host gcc not found")
        return 1

    exe_suffix = ".exe" if sys.platform.startswith("win") else ""
    output = BUILD / f"dsp_smoke_model{exe_suffix}"
    command = [
        host_gcc,
        "-std=c99",
        "-Wall",
        "-Wextra",
        "-Werror",
        "-Iinclude",
        "-Iexamples/dsp",
        *[str(source) for source in KERNEL_SOURCES],
        "src/portable/dsp_c28x/mrt_port_dsp_c28x.c",
        "examples/dsp/main.c",
        "examples/dsp/mrt_port_dsp_model.c",
        "-o",
        str(output),
    ]
    if run(command) != 0:
        return 1
    return run([str(output)])


def main():
    required_paths = [
        ROOT / "examples" / "stm32",
        ROOT / "examples" / "stm32" / "README.md",
        ROOT / "examples" / "stm32" / "main.c",
        ROOT / "examples" / "stm32" / "mrt_port_stm32_smoke.c",
        ROOT / "examples" / "stm32" / "startup_stm32cm.c",
        ROOT / "examples" / "stm32" / "runtime_stubs.c",
        ROOT / "examples" / "stm32" / "linker.ld",
        ROOT / "src" / "portable" / "stm32_cm" / "mrt_port_stm32_cm_context.S",
        ROOT / "examples" / "dsp",
        ROOT / "examples" / "dsp" / "README.md",
        ROOT / "examples" / "dsp" / "main.c",
        ROOT / "examples" / "dsp" / "mrt_port_dsp_model.c",
    ]

    failures = 0
    for path in required_paths:
        failures += require_path(path)

    if failures != 0:
        print(f"[embedded-smoke] {failures} required path(s) missing")
        return 1

    BUILD.mkdir(parents=True, exist_ok=True)

    failures += compile_stm32_smoke()
    failures += compile_and_run_dsp_model()

    if failures != 0:
        print("[embedded-smoke] smoke verification failed")
        return 1

    print("[embedded-smoke] STM32 cross build and DSP model smoke passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
