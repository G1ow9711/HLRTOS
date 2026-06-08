#!/usr/bin/env python3
"""验证真实板级 smoke 采集执行器的 dry-run 与执行契约。"""

from __future__ import annotations

import importlib.util
import json
import sys
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "tools" / "verify" / "run_hardware_smoke_capture.py"


def load_capture_runner():
    """加载待实现的硬件 smoke 采集执行器。"""
    spec = importlib.util.spec_from_file_location("run_hardware_smoke_capture", SCRIPT)
    if spec is None or spec.loader is None:
        raise AssertionError("capture runner module spec missing")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def quote_path(path: Path) -> str:
    """为命令行中的 Python 路径添加双引号。"""
    return f'"{path}"'


def raw_log_text(target: str) -> str:
    """返回一份目标板级 smoke 原始日志字段。"""
    if target == "STM32":
        return """MyRTOS-Hardware-Smoke: STM32
Evidence-Status: PASS
Smoke-Date: 2026-06-08
Chip: STM32F407VG
Board: STM32F4DISCOVERY
Compiler: arm-none-eabi-gcc 14.2
Clock-Hz: 168000000
Tick-Hz: 1000
NVIC-Priority-Bits: 4
Critical-Section: BASEPRI
Context-Switch: PendSV/SVC
SysTick: PASS
PendSV-SVC: PASS
ISR-Queue: PASS
Software-Timer: PASS
Tickless: PASS
Runtime-Minutes: 30
Assert-Failures: 0
Heap-Min-Free-Bytes: 2048
Trace-Or-UART-Log: stm32 dry capture fixture
"""
    return """MyRTOS-Hardware-Smoke: DSP
Evidence-Status: PASS
Smoke-Date: 2026-06-08
Chip: TMS320F28379D
Board: LAUNCHXL-F28379D
Compiler: tiarmclang 4.0
ABI: eabi
Stack-Direction: down
Context-Switch: software interrupt
Timer-Tick: PASS
Software-Interrupt-Switch: PASS
ISR-Nesting: PASS
Queue-Or-Pool: PASS
Software-Timer: PASS
Tickless: PASS
Runtime-Minutes: 30
Assert-Failures: 0
Heap-Min-Free-Bytes: 2048
Trace-Or-UART-Log: dsp dry capture fixture
"""


def write_script(path: Path, body: str) -> None:
    """写入临时命令脚本。"""
    path.write_text(body, encoding="utf-8")


def write_target_scripts(root: Path, target: str, raw_log: Path) -> dict[str, str]:
    """为测试目标写入无害的编译、烧录和采集脚本。"""
    noop = root / f"{target.lower()}_noop.py"
    capture = root / f"{target.lower()}_capture.py"
    write_script(noop, "raise SystemExit(0)\n")
    write_script(
        capture,
        "from pathlib import Path\n"
        f"Path(r'{raw_log}').write_text({raw_log_text(target)!r}, encoding='utf-8')\n",
    )
    python = quote_path(Path(sys.executable))
    return {
        "compiler_command": f"{python} {quote_path(noop)}",
        "build_command": f"{python} {quote_path(noop)}",
        "flash_command": f"{python} {quote_path(noop)}",
        "capture_command": f"{python} {quote_path(capture)}",
    }


def valid_config(root: Path) -> tuple[dict, Path, Path]:
    """构造可执行的单目标测试配置。"""
    raw_log = root / "stm32_uart_raw.log"
    evidence = root / "stm32_board_smoke.md"
    commands = write_target_scripts(root, "STM32", raw_log)
    config = {
        "targets": {
            "STM32": {
                "target": "STM32",
                "chip": "STM32F407VG",
                "board": "STM32F4DISCOVERY",
                **commands,
                "raw_log": str(raw_log),
                "evidence_output": str(evidence),
                "minimum_runtime_minutes": 30,
                "expected_fields": ["SysTick", "PendSV-SVC", "ISR-Queue", "Software-Timer", "Tickless"],
                "stm32": {
                    "clock_hz": 168000000,
                    "tick_hz": 1000,
                    "nvic_priority_bits": 4,
                    "context_switch": "PendSV/SVC",
                },
            },
            "DSP": {
                "target": "DSP",
                "chip": "TMS320F28379D",
                "board": "LAUNCHXL-F28379D",
                **write_target_scripts(root, "DSP", root / "dsp_uart_raw.log"),
                "raw_log": str(root / "dsp_uart_raw.log"),
                "evidence_output": str(root / "dsp_board_smoke.md"),
                "minimum_runtime_minutes": 30,
                "expected_fields": [
                    "Timer-Tick",
                    "Software-Interrupt-Switch",
                    "ISR-Nesting",
                    "Queue-Or-Pool",
                    "Software-Timer",
                    "Tickless",
                ],
                "dsp": {
                    "abi": "eabi",
                    "stack_direction": "down",
                    "timer_source": "CPU timer",
                    "context_switch": "software interrupt",
                },
            },
        }
    }
    return config, raw_log, evidence


def test_capture_plan_has_ordered_target_steps() -> None:
    """dry-run 计划必须显示预检、编译、构建、烧录、采集、生成和目标证据校验顺序。"""
    runner = load_capture_runner()
    with tempfile.TemporaryDirectory() as temp_dir:
        root = Path(temp_dir)
        config, _, _ = valid_config(root)
        config_path = root / "hardware_smoke_preflight.json"
        config_path.write_text(json.dumps(config), encoding="utf-8")
        steps = runner.build_capture_plan(config_path, "STM32", check_tools=False)
        names = [step.name for step in steps]
        assert names == [
            "preflight",
            "STM32.compiler",
            "STM32.build",
            "STM32.flash",
            "STM32.capture",
            "STM32.generate-evidence",
            "STM32.verify-evidence",
        ]


def test_execute_runs_capture_and_generates_traceable_evidence() -> None:
    """执行模式必须运行采集命令、生成证据并写入可回溯原始日志的 SHA-256。"""
    runner = load_capture_runner()
    with tempfile.TemporaryDirectory() as temp_dir:
        root = Path(temp_dir)
        config, raw_log, evidence = valid_config(root)
        config_path = root / "hardware_smoke_preflight.json"
        config_path.write_text(json.dumps(config), encoding="utf-8")
        assert runner.run_capture(config_path, "STM32", execute=True, check_tools=False) == 0
        assert raw_log.exists()
        text = evidence.read_text(encoding="utf-8")
        assert f"Raw-Log-Path: {raw_log}" in text
        assert "Raw-Log-SHA256:" in text


def test_generator_capture_command_is_not_duplicated() -> None:
    """采集命令已经生成证据时，执行器不应追加第二次生成步骤。"""
    runner = load_capture_runner()
    with tempfile.TemporaryDirectory() as temp_dir:
        root = Path(temp_dir)
        config, raw_log, evidence = valid_config(root)
        config["targets"]["STM32"]["capture_command"] = (
            f"{quote_path(Path(sys.executable))} "
            f"{quote_path(ROOT / 'tools' / 'verify' / 'generate_hardware_smoke_evidence.py')} "
            f"--target STM32 --input {quote_path(raw_log)} --output {quote_path(evidence)}"
        )
        config_path = root / "hardware_smoke_preflight.json"
        config_path.write_text(json.dumps(config), encoding="utf-8")
        steps = runner.build_capture_plan(config_path, "STM32", check_tools=False)
        names = [step.name for step in steps]
        assert "STM32.capture" in names
        assert "STM32.generate-evidence" not in names
        assert names[-1] == "STM32.verify-evidence"


def main() -> int:
    """运行硬件 smoke 采集执行器静态测试。"""
    test_capture_plan_has_ordered_target_steps()
    test_execute_runs_capture_and_generates_traceable_evidence()
    test_generator_capture_command_is_not_duplicated()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
