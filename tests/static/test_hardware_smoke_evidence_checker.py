#!/usr/bin/env python3
"""验证真实板级 smoke 证据校验脚本的核心规则。"""

from __future__ import annotations

import importlib.util
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "tools" / "verify" / "check_hardware_smoke_evidence.py"


def load_checker():
    """加载待测试的硬件 smoke 证据校验脚本。"""
    spec = importlib.util.spec_from_file_location("check_hardware_smoke_evidence", SCRIPT)
    if spec is None or spec.loader is None:
        raise AssertionError("checker module spec missing")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def write_text(path: Path, text: str) -> None:
    """写入 UTF-8 测试日志。"""
    path.write_text(text.strip() + "\n", encoding="utf-8")


def valid_stm32_log() -> str:
    """返回一份满足 STM32 板级 smoke 规则的测试日志。"""
    return """
MyRTOS-Hardware-Smoke: STM32
Evidence-Status: PASS
Smoke-Date: 2026-06-08
Chip: STM32F407VG
Board: STM32F4DISCOVERY
Compiler: arm-none-eabi-gcc 14.2.Rel1
Clock-Hz: 168000000
Tick-Hz: 1000
NVIC-Priority-Bits: 4
Critical-Section: PRIMASK and BASEPRI validated
Context-Switch: PendSV/SVC assembly validated
SysTick: PASS
PendSV-SVC: PASS
ISR-Queue: PASS
Software-Timer: PASS
Tickless: PASS
Runtime-Minutes: 30
Assert-Failures: 0
Heap-Min-Free-Bytes: 2048
Trace-Or-UART-Log: tick=1800000 queue=256 timer=3600
"""


def valid_dsp_log() -> str:
    """返回一份满足 DSP 板级 smoke 规则的测试日志。"""
    return """
MyRTOS-Hardware-Smoke: DSP
Evidence-Status: PASS
Smoke-Date: 2026-06-08
Chip: TMS320F28379D
Board: LAUNCHXL-F28379D
Compiler: ti-cgt-c2000 22.6
ABI: eabi
Stack-Direction: down
Context-Switch: software interrupt assembly validated
Timer-Tick: PASS
Software-Interrupt-Switch: PASS
ISR-Nesting: PASS
Queue-Or-Pool: PASS
Software-Timer: PASS
Tickless: PASS
Runtime-Minutes: 30
Assert-Failures: 0
Heap-Min-Free-Bytes: 2048
Trace-Or-UART-Log: tick=1800000 queue=512 timer=3600
"""


def test_valid_evidence_directory_passes() -> None:
    """验证完整 STM32/DSP 证据目录通过校验。"""
    checker = load_checker()
    with tempfile.TemporaryDirectory() as temp_dir:
        root = Path(temp_dir)
        write_text(root / "stm32_board_smoke.md", valid_stm32_log())
        write_text(root / "dsp_board_smoke.md", valid_dsp_log())
        failures = checker.check_evidence_dir(root)
        assert failures == [], failures


def test_missing_and_invalid_evidence_fails() -> None:
    """验证缺少文件或关键字段非法会失败。"""
    checker = load_checker()
    with tempfile.TemporaryDirectory() as temp_dir:
        root = Path(temp_dir)
        write_text(
            root / "stm32_board_smoke.md",
            """
MyRTOS-Hardware-Smoke: STM32
Evidence-Status: FAIL
Runtime-Minutes: 3
Assert-Failures: 1
Heap-Min-Free-Bytes: 0
""",
        )
        failures = checker.check_evidence_dir(root)
        joined = "\n".join(failures)
        assert "dsp_board_smoke.md" in joined
        assert "Evidence-Status" in joined
        assert "Runtime-Minutes" in joined
        assert "Assert-Failures" in joined
        assert "Heap-Min-Free-Bytes" in joined


def main() -> int:
    """运行静态测试。"""
    test_valid_evidence_directory_passes()
    test_missing_and_invalid_evidence_fails()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
