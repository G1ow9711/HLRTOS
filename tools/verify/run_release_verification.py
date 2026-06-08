#!/usr/bin/env python3
"""统一执行 MyRTOS 发布验收检查。"""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path
from typing import NamedTuple


ROOT = Path(__file__).resolve().parents[2]


class Step(NamedTuple):
    """单个验证步骤。"""

    name: str
    command: list[str]
    description: str


def build_steps(require_hardware: bool = False) -> list[Step]:
    """构建发布验收步骤清单。

    参数:
        require_hardware: 是否要求真实 STM32/DSP 板级证据通过。

    返回:
        按执行顺序排列的步骤列表。
    """
    steps = [
        Step(
            name="host-tests",
            command=[sys.executable, str(ROOT / "tools" / "run_host_tests.py")],
            description="运行全部 host 单元、仿真和耦合测试",
        ),
        Step(
            name="api-manual-coverage",
            command=[sys.executable, str(ROOT / "tools" / "verify" / "check_api_manual_coverage.py")],
            description="检查手册 API 覆盖",
        ),
        Step(
            name="manual-structure",
            command=[sys.executable, str(ROOT / "tools" / "verify" / "check_manual_structure.py")],
            description="检查中文参考手册章节顺序、API 分类导航和参考手册字段结构",
        ),
        Step(
            name="api-catalog-prototypes",
            command=[sys.executable, str(ROOT / "tools" / "verify" / "check_api_catalog_prototypes.py")],
            description="检查 API 目录与源码原型一致",
        ),
        Step(
            name="chinese-comments",
            command=[sys.executable, str(ROOT / "tools" / "verify" / "check_chinese_comments.py")],
            description="检查中文函数注释覆盖",
        ),
        Step(
            name="original-symbols",
            command=[sys.executable, str(ROOT / "tools" / "verify" / "check_original_symbols.py")],
            description="检查原创符号约束",
        ),
        Step(
            name="stm32-context-scaffold",
            command=[sys.executable, str(ROOT / "tests" / "static" / "test_stm32_context_scaffold.py")],
            description="检查 STM32 Cortex-M SVC/PendSV 汇编骨架和 smoke 构建接线",
        ),
        Step(
            name="dsp-context-scaffold",
            command=[sys.executable, str(ROOT / "tests" / "static" / "test_dsp_context_scaffold.py")],
            description="检查 DSP C28x 上下文切换汇编骨架和真实移植边界",
        ),
        Step(
            name="dsp-c2000-project-scaffold",
            command=[sys.executable, str(ROOT / "tests" / "static" / "test_dsp_c2000_project_scaffold.py")],
            description="检查 DSP C2000 板级 smoke 启动、链接和 ISR glue 骨架",
        ),
        Step(
            name="embedded-smoke",
            command=[sys.executable, str(ROOT / "tools" / "verify" / "check_embedded_smoke_projects.py")],
            description="检查 STM32/DSP embedded smoke 工程",
        ),
        Step(
            name="hardware-smoke-preflight",
            command=[sys.executable, str(ROOT / "tools" / "verify" / "check_hardware_smoke_preflight.py")],
            description="检查真实板级 smoke 采集前置配置",
        ),
        Step(
            name="hardware-smoke-raw-log-schema",
            command=[sys.executable, str(ROOT / "tools" / "verify" / "check_hardware_smoke_raw_log_schema.py")],
            description="检查真实板级 smoke 原始日志 schema 与生成器、文档、schema C 头文件、完整报告 emitter 和 check_hardware_smoke_raw_log.py 一致",
        ),
        Step(
            name="hardware-smoke-raw-log-checker",
            command=[sys.executable, str(ROOT / "tests" / "static" / "test_hardware_smoke_raw_log_checker.py")],
            description="直接检查真实板级 smoke 原始 UART/trace 日志字段、PASS 状态和数值规则",
        ),
        Step(
            name="hardware-smoke-capture-runner",
            command=[sys.executable, str(ROOT / "tests" / "static" / "test_hardware_smoke_capture_runner.py")],
            description="检查真实板级 smoke 采集执行器的 dry-run 和执行契约",
        ),
        Step(
            name="hardware-evidence-checker",
            command=[sys.executable, str(ROOT / "tests" / "static" / "test_hardware_smoke_evidence_checker.py")],
            description="检查真实板级烟雾证据校验器本身",
        ),
        Step(
            name="hardware-evidence-generator",
            command=[sys.executable, str(ROOT / "tests" / "static" / "test_hardware_smoke_evidence_generator.py")],
            description="检查原始 UART/trace 日志到最终证据文件的生成器",
        ),
        Step(
            name="release-verification-runner",
            command=[sys.executable, str(ROOT / "tests" / "static" / "test_release_verification_runner.py")],
            description="检查统一 release 验证入口本身",
        ),
    ]
    if require_hardware:
        steps.append(
            Step(
                name="hardware-smoke-evidence",
                command=[sys.executable, str(ROOT / "tools" / "verify" / "check_hardware_smoke_evidence.py")],
                description="检查真实 STM32/DSP 板级 smoke 证据",
            )
        )
    return steps


def run_step(step: Step) -> int:
    """执行单个验证步骤。"""
    print(f"[release] {step.name}: {step.description}")
    completed = subprocess.run(step.command, cwd=ROOT)
    return completed.returncode


def run_steps(steps: list[Step]) -> int:
    """顺序执行全部验证步骤。"""
    failures = 0
    for step in steps:
        if run_step(step) != 0:
            failures += 1
    if failures:
        print(f"[release] {failures} step(s) failed")
        return 1
    print(f"[release] {len(steps)} step(s) passed")
    return 0


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    """解析命令行参数。"""
    parser = argparse.ArgumentParser(description="Run MyRTOS release verification steps")
    parser.add_argument(
        "--require-hardware",
        action="store_true",
        help="include real STM32/DSP board smoke evidence gate",
    )
    parser.add_argument(
        "--list",
        action="store_true",
        help="print the planned steps and exit",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    """程序入口。"""
    args = parse_args(argv)
    steps = build_steps(require_hardware=args.require_hardware)

    if args.list:
        for step in steps:
            print(f"{step.name}: {step.description}")
        return 0

    return run_steps(steps)


if __name__ == "__main__":
    raise SystemExit(main())
