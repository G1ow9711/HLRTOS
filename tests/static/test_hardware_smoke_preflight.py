#!/usr/bin/env python3
"""验证真实板级 smoke 预检工具的配置规则。"""

from __future__ import annotations

import importlib.util
import json
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "tools" / "verify" / "check_hardware_smoke_preflight.py"


def load_preflight():
    """加载待测试的硬件 smoke 预检模块。"""
    spec = importlib.util.spec_from_file_location("check_hardware_smoke_preflight", SCRIPT)
    if spec is None or spec.loader is None:
        raise AssertionError("preflight module spec missing")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def write_json(path: Path, data: dict) -> None:
    """写入 UTF-8 JSON 测试配置文件。"""
    path.write_text(json.dumps(data, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")


def valid_config() -> dict:
    """返回一份同时包含 STM32 和 DSP 的合法预检配置。"""
    return {
        "targets": {
            "STM32": {
                "target": "STM32",
                "chip": "STM32F407VG",
                "board": "STM32F4DISCOVERY",
                "compiler_command": "arm-none-eabi-gcc --version",
                "build_command": "python tools\\verify\\check_embedded_smoke_projects.py",
                "flash_command": "openocd -f board/stm32f4discovery.cfg -c program",
                "capture_command": "python tools\\verify\\generate_hardware_smoke_evidence.py --target STM32",
                "raw_log": "docs/verification/hardware_smoke/stm32_uart_raw.log",
                "evidence_output": "docs/verification/hardware_smoke/stm32_board_smoke.md",
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
                "compiler_command": "tiarmclang --version",
                "build_command": "python tools\\verify\\check_embedded_smoke_projects.py",
                "flash_command": "uniflash --target TMS320F28379D --program",
                "capture_command": "python tools\\verify\\generate_hardware_smoke_evidence.py --target DSP",
                "raw_log": "docs/verification/hardware_smoke/dsp_uart_raw.log",
                "evidence_output": "docs/verification/hardware_smoke/dsp_board_smoke.md",
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


def test_valid_preflight_config_passes_schema_check() -> None:
    """完整 STM32/DSP 预检配置应通过结构检查。"""
    preflight = load_preflight()
    with tempfile.TemporaryDirectory() as temp_dir:
        config_path = Path(temp_dir) / "hardware_smoke_preflight.json"
        write_json(config_path, valid_config())
        failures = preflight.check_config_file(config_path, check_tools=False)
        assert failures == [], failures
        assert preflight.main(["--config", str(config_path)]) == 0


def test_invalid_preflight_config_reports_actionable_failures() -> None:
    """缺字段、占位符和不足运行时长必须被报告。"""
    preflight = load_preflight()
    config = valid_config()
    del config["targets"]["DSP"]
    config["targets"]["STM32"]["board"] = "TODO"
    config["targets"]["STM32"]["minimum_runtime_minutes"] = 5
    config["targets"]["STM32"]["expected_fields"] = ["SysTick"]
    with tempfile.TemporaryDirectory() as temp_dir:
        config_path = Path(temp_dir) / "bad_preflight.json"
        write_json(config_path, config)
        failures = preflight.check_config_file(config_path, check_tools=False)
        joined = "\n".join(failures)
        assert "missing target DSP" in joined
        assert "STM32.board contains placeholder" in joined
        assert "minimum_runtime_minutes must be >= 30" in joined
        assert "missing expected field PendSV-SVC" in joined


def test_optional_tool_lookup_reports_missing_executable() -> None:
    """启用工具检查时，不存在的可执行文件必须失败。"""
    preflight = load_preflight()
    config = valid_config()
    config["targets"]["STM32"]["compiler_command"] = "definitely_missing_myrtos_tool --version"
    with tempfile.TemporaryDirectory() as temp_dir:
        config_path = Path(temp_dir) / "missing_tool.json"
        write_json(config_path, config)
        failures = preflight.check_config_file(config_path, check_tools=True)
        joined = "\n".join(failures)
        assert "definitely_missing_myrtos_tool" in joined
        assert "tool not found" in joined


def main() -> int:
    """运行静态预检测试。"""
    test_valid_preflight_config_passes_schema_check()
    test_invalid_preflight_config_reports_actionable_failures()
    test_optional_tool_lookup_reports_missing_executable()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
