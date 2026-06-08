#!/usr/bin/env python3
"""执行或预演真实 STM32/DSP 板级 smoke 采集流水线。"""

from __future__ import annotations

import argparse
import importlib.util
import json
import subprocess
import sys
from pathlib import Path
from types import ModuleType
from typing import Any, NamedTuple


ROOT = Path(__file__).resolve().parents[2]
VERIFY_DIR = ROOT / "tools" / "verify"
DEFAULT_CONFIG = ROOT / "docs" / "verification" / "hardware_smoke" / "hardware_smoke_preflight.json"
TARGETS = ("STM32", "DSP")

TARGET_EXTRA_FIELDS = {
    "STM32": [
        "Clock-Hz",
        "Tick-Hz",
        "NVIC-Priority-Bits",
        "Critical-Section",
        "SysTick",
        "PendSV-SVC",
        "ISR-Queue",
    ],
    "DSP": [
        "ABI",
        "Stack-Direction",
        "Timer-Tick",
        "Software-Interrupt-Switch",
        "ISR-Nesting",
        "Queue-Or-Pool",
    ],
}


class CaptureStep(NamedTuple):
    """描述一次硬件 smoke 采集流水线步骤。"""

    name: str
    target: str
    action: str
    command: str


def quote_path(path: Path) -> str:
    """为命令行中的路径添加双引号。"""
    return f'"{path}"'


def load_module(module_name: str, path: Path) -> ModuleType:
    """按文件路径加载相邻验证模块。"""
    spec = importlib.util.spec_from_file_location(module_name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"{module_name}: module spec missing")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def load_config(config_path: Path) -> dict[str, Any]:
    """读取硬件 smoke 预检 JSON 配置。"""
    if not config_path.exists():
        raise ValueError(f"{config_path}: missing config file")
    try:
        data = json.loads(config_path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        raise ValueError(f"{config_path}: invalid JSON at line {exc.lineno}: {exc.msg}") from exc
    if not isinstance(data, dict):
        raise ValueError("root config must be an object")
    return data


def selected_targets(target: str) -> list[str]:
    """把命令行目标转换为实际目标列表。"""
    if target == "all":
        return list(TARGETS)
    if target not in TARGETS:
        raise ValueError(f"unsupported target: {target}")
    return [target]


def require_target_config(config: dict[str, Any], target: str) -> dict[str, Any]:
    """取得单个目标的配置对象。"""
    targets = config.get("targets")
    if not isinstance(targets, dict):
        raise ValueError("targets must be an object")
    target_config = targets.get(target)
    if not isinstance(target_config, dict):
        raise ValueError(f"missing target {target}")
    return target_config


def resolve_project_path(path_text: str) -> Path:
    """把仓库相对路径或绝对路径规整为可执行路径。"""
    path = Path(path_text)
    if path.is_absolute():
        return path
    return ROOT / path


def generated_evidence_command(target: str, target_config: dict[str, Any]) -> str:
    """生成 raw log 到最终证据文件的命令行。"""
    raw_log = resolve_project_path(str(target_config["raw_log"]))
    evidence = resolve_project_path(str(target_config["evidence_output"]))
    generator = VERIFY_DIR / "generate_hardware_smoke_evidence.py"
    return (
        f"{quote_path(Path(sys.executable))} {quote_path(generator)} "
        f"--target {target} --input {quote_path(raw_log)} --output {quote_path(evidence)}"
    )


def command_already_generates_evidence(command: str) -> bool:
    """判断采集命令是否已经调用最终证据生成器。"""
    return "generate_hardware_smoke_evidence.py" in command.replace("/", "\\")


def build_capture_plan(config_path: Path, target: str, check_tools: bool = False) -> list[CaptureStep]:
    """构建硬件 smoke 采集流水线计划。"""
    config = load_config(config_path)
    preflight_command = (
        f"{quote_path(Path(sys.executable))} {quote_path(VERIFY_DIR / 'check_hardware_smoke_preflight.py')} "
        f"--config {quote_path(config_path)}"
    )
    if target != "all":
        preflight_command += f" --target {target}"
    if check_tools:
        preflight_command += " --check-tools"

    steps = [CaptureStep("preflight", target, "preflight", preflight_command)]
    for target_name in selected_targets(target):
        target_config = require_target_config(config, target_name)
        capture_command = str(target_config["capture_command"])
        target_steps = [
            CaptureStep(f"{target_name}.compiler", target_name, "command", str(target_config["compiler_command"])),
            CaptureStep(f"{target_name}.build", target_name, "command", str(target_config["build_command"])),
            CaptureStep(f"{target_name}.flash", target_name, "command", str(target_config["flash_command"])),
            CaptureStep(f"{target_name}.capture", target_name, "command", capture_command),
        ]
        if not command_already_generates_evidence(capture_command):
            target_steps.append(
                CaptureStep(
                    f"{target_name}.generate-evidence",
                    target_name,
                    "command",
                    generated_evidence_command(target_name, target_config),
                )
            )
        target_steps.append(CaptureStep(f"{target_name}.verify-evidence", target_name, "verify-evidence", ""))
        steps.extend(target_steps)
    return steps


def print_dry_run(steps: list[CaptureStep]) -> None:
    """打印不会执行任何硬件动作的 dry-run 计划。"""
    for step in steps:
        command = step.command if step.command else "<internal target evidence check>"
        print(f"[hardware-capture] dry-run {step.name}: {command}")
    print("[hardware-capture] dry-run only; add --execute to run hardware commands")


def run_config_preflight(config_path: Path, check_tools: bool, target: str) -> int:
    """运行预检规则并输出失败信息。"""
    preflight = load_module("check_hardware_smoke_preflight", VERIFY_DIR / "check_hardware_smoke_preflight.py")
    target_filter = None if target == "all" else (target,)
    failures = preflight.check_config_file(config_path, check_tools=check_tools, target_filter=target_filter)
    if failures:
        for failure in failures:
            print(f"[hardware-capture] preflight: {failure}")
        return 1
    print("[hardware-capture] preflight accepted")
    return 0


def run_target_evidence_check(config_path: Path, target: str) -> int:
    """只校验当前目标的最终证据文件。"""
    checker = load_module("check_hardware_smoke_evidence", VERIFY_DIR / "check_hardware_smoke_evidence.py")
    config = require_target_config(load_config(config_path), target)
    evidence = resolve_project_path(str(config["evidence_output"]))
    failures = checker.check_file(evidence, target, TARGET_EXTRA_FIELDS[target])
    if failures:
        for failure in failures:
            print(f"[hardware-capture] {failure}")
        return 1
    print(f"[hardware-capture] {target} evidence accepted")
    return 0


def run_command(step: CaptureStep) -> int:
    """执行一个外部命令步骤。"""
    print(f"[hardware-capture] run {step.name}: {step.command}")
    completed = subprocess.run(step.command, cwd=ROOT, shell=True)
    return completed.returncode


def run_step(step: CaptureStep, config_path: Path, check_tools: bool) -> int:
    """执行单个采集流水线步骤。"""
    if step.action == "preflight":
        return run_config_preflight(config_path, check_tools, step.target)
    if step.action == "verify-evidence":
        return run_target_evidence_check(config_path, step.target)
    return run_command(step)


def run_capture(config_path: Path, target: str, execute: bool, check_tools: bool = False) -> int:
    """运行或预演硬件 smoke 采集流水线。"""
    steps = build_capture_plan(config_path, target, check_tools=check_tools)
    if not execute:
        print_dry_run(steps)
        return 0

    for step in steps:
        if run_step(step, config_path, check_tools) != 0:
            print(f"[hardware-capture] stopped at {step.name}")
            return 1
    print("[hardware-capture] capture pipeline passed")
    return 0


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    """解析命令行参数。"""
    parser = argparse.ArgumentParser(description="Run or dry-run real hardware smoke capture")
    parser.add_argument("--config", default=str(DEFAULT_CONFIG), help="hardware smoke preflight JSON path")
    parser.add_argument("--target", default="all", choices=["all", "STM32", "DSP"], help="target to capture")
    parser.add_argument("--execute", action="store_true", help="actually run compiler/build/flash/capture commands")
    parser.add_argument("--check-tools", action="store_true", help="also check configured executables in PATH")
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    """程序入口。"""
    args = parse_args(argv)
    try:
        return run_capture(Path(args.config), args.target, execute=args.execute, check_tools=args.check_tools)
    except ValueError as exc:
        print(f"[hardware-capture] {exc}")
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
