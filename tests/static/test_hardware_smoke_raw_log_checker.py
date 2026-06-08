#!/usr/bin/env python3
"""验证真实板级 smoke 原始日志直检工具的字段规则。"""

from __future__ import annotations

import importlib.util
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "tools" / "verify" / "check_hardware_smoke_raw_log.py"


def load_checker():
    """加载待测试的原始日志直检工具。"""
    spec = importlib.util.spec_from_file_location("check_hardware_smoke_raw_log", SCRIPT)
    if spec is None or spec.loader is None:
        raise AssertionError("raw log checker module spec missing")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def valid_stm32_raw_log() -> str:
    """返回一份满足 STM32 板级 smoke 字段规则的原始日志。"""
    return """
boot banner line
MyRTOS-Hardware-Smoke: STM32
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
Trace-Or-UART-Log: stm32 raw log fixture
tail noise line
"""


def invalid_dsp_raw_log() -> str:
    """返回一份缺少堆剩余字段且状态失败的 DSP 原始日志。"""
    return """
MyRTOS-Hardware-Smoke: DSP
Evidence-Status: FAIL
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
Runtime-Minutes: 10
Assert-Failures: 1
Trace-Or-UART-Log: dsp raw log fixture
"""


def test_valid_stm32_raw_log_passes_without_embedded_hash_fields() -> None:
    """原始日志不需要自带 Raw-Log 字段，直检工具应派生路径和 SHA-256 后校验。"""
    checker = load_checker()
    with tempfile.TemporaryDirectory() as temp_dir:
        raw_log = Path(temp_dir) / "stm32_uart_raw.log"
        raw_log.write_text(valid_stm32_raw_log().strip() + "\n", encoding="utf-8")
        failures = checker.check_raw_log_file(raw_log, "STM32")
        assert failures == [], failures


def test_invalid_raw_log_reports_all_visible_failures() -> None:
    """原始日志缺字段时仍应继续报告已存在的 FAIL、运行时长和断言次数错误。"""
    checker = load_checker()
    with tempfile.TemporaryDirectory() as temp_dir:
        raw_log = Path(temp_dir) / "dsp_uart_raw.log"
        raw_log.write_text(invalid_dsp_raw_log().strip() + "\n", encoding="utf-8")
        failures = checker.check_raw_log_file(raw_log, "DSP")
        joined = "\n".join(failures)
        assert "missing field Heap-Min-Free-Bytes" in joined
        assert "Evidence-Status must be PASS" in joined
        assert "Runtime-Minutes must be >= 30" in joined
        assert "Assert-Failures must be 0" in joined


def test_wrong_target_is_rejected() -> None:
    """目标参数与日志内目标字段不一致时必须失败。"""
    checker = load_checker()
    with tempfile.TemporaryDirectory() as temp_dir:
        raw_log = Path(temp_dir) / "stm32_uart_raw.log"
        raw_log.write_text(valid_stm32_raw_log().strip() + "\n", encoding="utf-8")
        failures = checker.check_raw_log_file(raw_log, "DSP")
        joined = "\n".join(failures)
        assert "MyRTOS-Hardware-Smoke must be DSP" in joined


def main() -> int:
    """运行原始日志直检工具静态测试。"""
    test_valid_stm32_raw_log_passes_without_embedded_hash_fields()
    test_invalid_raw_log_reports_all_visible_failures()
    test_wrong_target_is_rejected()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
