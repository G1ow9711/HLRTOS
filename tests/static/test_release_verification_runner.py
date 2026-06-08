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
        "api-catalog-prototypes",
        "chinese-comments",
        "original-symbols",
        "stm32-context-scaffold",
        "dsp-context-scaffold",
        "embedded-smoke",
        "hardware-smoke-preflight",
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


def main() -> int:
    """运行静态测试。"""
    test_default_plan_skips_real_board_gate()
    test_hardware_plan_appends_real_board_gate()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
