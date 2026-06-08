#!/usr/bin/env python3
"""验证统一 release 验证入口的步骤编排契约。"""

from __future__ import annotations

import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "tools" / "verify" / "run_release_verification.py"


def load_runner():
    """加载待实现的 release 验证入口。"""
    spec = importlib.util.spec_from_file_location("run_release_verification", SCRIPT)
    if spec is None or spec.loader is None:
        raise AssertionError("runner module spec missing")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def test_default_plan_skips_real_board_gate() -> None:
    """默认模式应保留 repo-side 验证并跳过真实板级 gate。"""
    runner = load_runner()
    steps = runner.build_steps(require_hardware=False)
    names = [step.name for step in steps]
    assert names == [
        "host-tests",
        "api-manual-coverage",
        "manual-structure",
        "api-catalog-prototypes",
        "chinese-comments",
        "original-symbols",
        "stm32-context-scaffold",
        "dsp-context-scaffold",
        "dsp-c2000-project-scaffold",
        "embedded-smoke",
        "hardware-smoke-preflight",
        "hardware-smoke-raw-log-schema",
        "hardware-smoke-raw-log-checker",
        "hardware-smoke-capture-runner",
        "hardware-evidence-checker",
        "hardware-evidence-generator",
        "release-verification-runner",
    ]


def test_hardware_plan_appends_real_board_gate() -> None:
    """要求硬件时应在最后加入真实板级证据检查。"""
    runner = load_runner()
    steps = runner.build_steps(require_hardware=True)
    names = [step.name for step in steps]
    assert names[-1] == "hardware-smoke-evidence"
    assert "hardware-smoke-evidence" in names


def test_raw_log_schema_step_mentions_report_header() -> None:
    """raw-log schema release 步骤说明必须覆盖完整报告 emitter 漂移检查。"""
    runner = load_runner()
    steps = runner.build_steps(require_hardware=False)
    raw_log_step = next(step for step in steps if step.name == "hardware-smoke-raw-log-schema")
    assert "完整报告 emitter" in raw_log_step.description
    assert "check_hardware_smoke_raw_log.py" in raw_log_step.description


def test_manual_structure_step_mentions_reference_manual_layout() -> None:
    """manual-structure release 步骤说明必须表明它检查参考手册式结构。"""
    runner = load_runner()
    steps = runner.build_steps(require_hardware=False)
    manual_step = next(step for step in steps if step.name == "manual-structure")
    assert "参考手册" in manual_step.description


def test_raw_log_checker_step_mentions_direct_uart_trace_check() -> None:
    """raw-log checker release 步骤说明必须表明它直接检查原始 UART/trace 日志。"""
    runner = load_runner()
    steps = runner.build_steps(require_hardware=False)
    raw_log_step = next(step for step in steps if step.name == "hardware-smoke-raw-log-checker")
    assert "原始 UART/trace" in raw_log_step.description


def main() -> int:
    """运行静态测试。"""
    test_default_plan_skips_real_board_gate()
    test_hardware_plan_appends_real_board_gate()
    test_manual_structure_step_mentions_reference_manual_layout()
    test_raw_log_schema_step_mentions_report_header()
    test_raw_log_checker_step_mentions_direct_uart_trace_check()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
