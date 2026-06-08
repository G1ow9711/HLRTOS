#!/usr/bin/env python3
"""验证硬件 smoke 证据生成器的核心规则。"""

from __future__ import annotations

import hashlib
import importlib.util
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "tools" / "verify" / "generate_hardware_smoke_evidence.py"


def load_generator():
    """加载待实现的证据生成器。"""
    spec = importlib.util.spec_from_file_location("generate_hardware_smoke_evidence", SCRIPT)
    if spec is None or spec.loader is None:
        raise AssertionError("generator module spec missing")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def test_stm32_raw_log_is_normalized() -> None:
    """STM32 原始日志应被规范化为最终证据文件。"""
    generator = load_generator()
    with tempfile.TemporaryDirectory() as temp_dir:
        root = Path(temp_dir)
        raw_path = root / "stm32_uart_raw.log"
        out_path = root / "stm32_board_smoke.md"
        raw_path.write_text(
            """
boot noise
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
Trace-Or-UART-Log: uart.log tick=1800000
tail noise
""".strip()
            + "\n",
            encoding="utf-8",
        )
        result = generator.generate_from_raw_log(raw_path, out_path, expected_target="STM32")
        assert result == 0
        assert out_path.exists()
        text = out_path.read_text(encoding="utf-8")
        assert text.startswith("MyRTOS-Hardware-Smoke: STM32")
        assert "boot noise" not in text
        assert "tail noise" not in text
        assert f"Raw-Log-Path: {raw_path}" in text
        assert f"Raw-Log-SHA256: {hashlib.sha256(raw_path.read_bytes()).hexdigest()}" in text


def test_missing_required_field_fails() -> None:
    """缺少必需字段时应失败。"""
    generator = load_generator()
    with tempfile.TemporaryDirectory() as temp_dir:
        root = Path(temp_dir)
        raw_path = root / "dsp_uart_raw.log"
        out_path = root / "dsp_board_smoke.md"
        raw_path.write_text(
            """
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
Trace-Or-UART-Log: uart.log tick=1800000
""".strip()
            + "\n",
            encoding="utf-8",
        )
        try:
            generator.generate_from_raw_log(raw_path, out_path, expected_target="DSP")
        except ValueError as exc:
            assert "missing field Heap-Min-Free-Bytes" in str(exc)
        else:
            raise AssertionError("expected ValueError")


def main() -> int:
    """运行静态测试。"""
    test_stm32_raw_log_is_normalized()
    test_missing_required_field_fails()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
