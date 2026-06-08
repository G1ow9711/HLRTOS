#!/usr/bin/env python3
"""检查真实板级 smoke 采集前置配置是否完整、可执行、无占位符。"""

from __future__ import annotations

import argparse
import json
import shlex
import shutil
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[2]
DEFAULT_CONFIG = ROOT / "docs" / "verification" / "hardware_smoke" / "hardware_smoke_preflight.json"

TARGETS = ("STM32", "DSP")
COMMON_STRING_FIELDS = (
    "target",
    "chip",
    "board",
    "compiler_command",
    "build_command",
    "flash_command",
    "capture_command",
    "raw_log",
    "evidence_output",
)
PLACEHOLDER_WORDS = ("TODO", "TBD", "PENDING", "PLACEHOLDER", "CHANGE_ME", "FILL_ME")
REQUIRED_EXPECTED_FIELDS = {
    "STM32": ("SysTick", "PendSV-SVC", "ISR-Queue", "Software-Timer", "Tickless"),
    "DSP": (
        "Timer-Tick",
        "Software-Interrupt-Switch",
        "ISR-Nesting",
        "Queue-Or-Pool",
        "Software-Timer",
        "Tickless",
    ),
}


def normalize_target_filter(target_filter: tuple[str, ...] | list[str] | None) -> tuple[str, ...]:
    """规整需要预检的目标集合。
    参数:
        target_filter: 目标名列表；为 None 时表示检查全部 STM32/DSP 目标。
    返回值:
        返回按固定顺序去重后的目标元组。
    调用示例:
        `targets = normalize_target_filter(["STM32"])`
    """
    if target_filter is None:
        return TARGETS

    ordered: list[str] = []
    for target in target_filter:
        normalized = str(target).upper()
        if normalized not in TARGETS:
            raise ValueError(f"unsupported target: {target}")
        if normalized not in ordered:
            ordered.append(normalized)
    if not ordered:
        raise ValueError("target filter must not be empty")
    return tuple(ordered)


def is_placeholder(value: str) -> bool:
    """判断字符串是否仍像待替换占位内容。

    参数:
        value: 待检查的配置字符串。
    返回值:
        含有 TODO/PENDING 等占位词时返回 True，否则返回 False。
    调用示例:
        `if is_placeholder(board_name): ...`
    """
    normalized = value.strip().upper()
    for word in PLACEHOLDER_WORDS:
        if word in normalized:
            return True
    return normalized in {"", "-", "N/A"}


def command_executable(command: str) -> str:
    """提取命令行中的可执行文件名。

    参数:
        command: 例如 `arm-none-eabi-gcc --version` 的命令字符串。
    返回值:
        返回第一个 token；命令为空时返回空字符串。
    调用示例:
        `exe = command_executable("python tools\\verify\\x.py")`
    """
    try:
        parts = shlex.split(command, posix=False)
    except ValueError:
        parts = command.split()
    if not parts:
        return ""
    return parts[0].strip("\"'")


def require_mapping(value: Any, label: str) -> tuple[dict[str, Any] | None, list[str]]:
    """检查 JSON 节点是否为对象。

    参数:
        value: 待检查的 JSON 节点。
        label: 错误信息中的字段路径。
    返回值:
        成功时返回 `(对象, [])`，失败时返回 `(None, [错误])`。
    调用示例:
        `targets, failures = require_mapping(data.get("targets"), "targets")`
    """
    if not isinstance(value, dict):
        return None, [f"{label} must be an object"]
    return value, []


def validate_string_field(config: dict[str, Any], target: str, field: str) -> list[str]:
    """检查单个字符串字段存在、非空、无占位符。

    参数:
        config: 单个目标的配置对象。
        target: 目标名称，通常为 STM32 或 DSP。
        field: 需要检查的字段名。
    返回值:
        返回错误列表；无错误时为空列表。
    调用示例:
        `failures.extend(validate_string_field(stm32, "STM32", "board"))`
    """
    value = config.get(field)
    if not isinstance(value, str) or not value.strip():
        return [f"{target}.{field} must be a non-empty string"]
    if is_placeholder(value):
        return [f"{target}.{field} contains placeholder"]
    return []


def validate_expected_fields(config: dict[str, Any], target: str) -> list[str]:
    """检查板级日志预期字段集合是否覆盖目标必需项。

    参数:
        config: 单个目标的配置对象。
        target: 目标名称。
    返回值:
        返回缺失字段错误列表。
    调用示例:
        `validate_expected_fields(config, "STM32")`
    """
    fields = config.get("expected_fields")
    if not isinstance(fields, list) or not all(isinstance(item, str) for item in fields):
        return [f"{target}.expected_fields must be a string list"]
    failures: list[str] = []
    field_set = set(fields)
    for required in REQUIRED_EXPECTED_FIELDS[target]:
        if required not in field_set:
            failures.append(f"{target}: missing expected field {required}")
    return failures


def validate_runtime(config: dict[str, Any], target: str) -> list[str]:
    """检查最小烟测运行时长。

    参数:
        config: 单个目标的配置对象。
        target: 目标名称。
    返回值:
        小于 30 分钟或类型错误时返回错误列表。
    调用示例:
        `validate_runtime(config, "DSP")`
    """
    value = config.get("minimum_runtime_minutes")
    if not isinstance(value, int):
        return [f"{target}.minimum_runtime_minutes must be an integer"]
    if value < 30:
        return [f"{target}.minimum_runtime_minutes must be >= 30"]
    return []


def validate_tool_commands(config: dict[str, Any], target: str, check_tools: bool) -> list[str]:
    """按需检查命令行可执行文件是否能在当前 PATH 中找到。

    参数:
        config: 单个目标的配置对象。
        target: 目标名称。
        check_tools: 为 True 时执行 `shutil.which` 检查；否则只做结构校验。
    返回值:
        返回缺失工具错误列表。
    调用示例:
        `validate_tool_commands(config, "STM32", check_tools=True)`
    """
    failures: list[str] = []
    for field in ("compiler_command", "build_command", "flash_command", "capture_command"):
        command = config.get(field, "")
        exe = command_executable(command)
        if not exe:
            failures.append(f"{target}.{field} executable missing")
            continue
        if check_tools and shutil.which(exe) is None:
            failures.append(f"{target}.{field}: {exe} tool not found")
    return failures


def validate_stm32(config: dict[str, Any]) -> list[str]:
    """检查 STM32 专属预检参数。

    参数:
        config: STM32 配置对象。
    返回值:
        返回错误列表。
    调用示例:
        `failures.extend(validate_stm32(stm32_config))`
    """
    stm32, failures = require_mapping(config.get("stm32"), "STM32.stm32")
    if stm32 is None:
        return failures
    for field in ("clock_hz", "tick_hz", "nvic_priority_bits"):
        value = stm32.get(field)
        if not isinstance(value, int) or value <= 0:
            failures.append(f"STM32.stm32.{field} must be > 0")
    value = stm32.get("context_switch")
    if not isinstance(value, str) or is_placeholder(value):
        failures.append("STM32.stm32.context_switch must be a non-placeholder string")
    return failures


def validate_dsp(config: dict[str, Any]) -> list[str]:
    """检查 DSP 专属预检参数。

    参数:
        config: DSP 配置对象。
    返回值:
        返回错误列表。
    调用示例:
        `failures.extend(validate_dsp(dsp_config))`
    """
    dsp, failures = require_mapping(config.get("dsp"), "DSP.dsp")
    if dsp is None:
        return failures
    for field in ("abi", "stack_direction", "timer_source", "context_switch"):
        value = dsp.get(field)
        if not isinstance(value, str) or is_placeholder(value):
            failures.append(f"DSP.dsp.{field} must be a non-placeholder string")
    direction = dsp.get("stack_direction")
    if isinstance(direction, str) and direction.lower() not in {"up", "down"}:
        failures.append("DSP.dsp.stack_direction must be up or down")
    return failures


def validate_target(target_config: Any, target: str, check_tools: bool) -> list[str]:
    """检查单个硬件目标的完整预检配置。

    参数:
        target_config: JSON 中的目标配置节点。
        target: 目标名称，必须为 STM32 或 DSP。
        check_tools: 是否检查本机 PATH 中的工具可用性。
    返回值:
        返回错误列表。
    调用示例:
        `validate_target(targets["STM32"], "STM32", False)`
    """
    config, failures = require_mapping(target_config, target)
    if config is None:
        return failures

    for field in COMMON_STRING_FIELDS:
        failures.extend(validate_string_field(config, target, field))

    configured_target = config.get("target")
    if isinstance(configured_target, str) and configured_target.upper() != target:
        failures.append(f"{target}.target must be {target}")

    failures.extend(validate_runtime(config, target))
    failures.extend(validate_expected_fields(config, target))
    failures.extend(validate_tool_commands(config, target, check_tools))

    if target == "STM32":
        failures.extend(validate_stm32(config))
    else:
        failures.extend(validate_dsp(config))

    return failures


def check_config_data(data: Any,
                      check_tools: bool,
                      target_filter: tuple[str, ...] | list[str] | None = None) -> list[str]:
    """检查已解析 JSON 数据是否满足硬件 smoke 预检规则。

    参数:
        data: `json.loads` 得到的对象。
        check_tools: 是否检查本机 PATH 工具。
    返回值:
        返回错误列表；为空表示通过。
    调用示例:
        `failures = check_config_data(json_data, check_tools=False)`
    """
    try:
        targets_to_check = normalize_target_filter(target_filter)
    except ValueError as exc:
        return [str(exc)]

    root, failures = require_mapping(data, "root")
    if root is None:
        return failures

    targets, target_failures = require_mapping(root.get("targets"), "targets")
    failures.extend(target_failures)
    if targets is None:
        return failures

    for target in targets_to_check:
        if target not in targets:
            failures.append(f"missing target {target}")
            continue
        failures.extend(validate_target(targets[target], target, check_tools))
    return failures


def check_config_file(config_path: Path,
                      check_tools: bool,
                      target_filter: tuple[str, ...] | list[str] | None = None) -> list[str]:
    """读取并检查硬件 smoke 预检配置文件。

    参数:
        config_path: JSON 配置文件路径。
        check_tools: 是否检查本机 PATH 工具。
    返回值:
        返回错误列表；为空表示配置通过。
    调用示例:
        `check_config_file(Path("hardware_smoke_preflight.json"), False)`
    """
    if not config_path.exists():
        return [f"{config_path}: missing config file"]
    try:
        data = json.loads(config_path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        return [f"{config_path}: invalid JSON at line {exc.lineno}: {exc.msg}"]
    return check_config_data(data, check_tools=check_tools, target_filter=target_filter)


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    """解析命令行参数。

    参数:
        argv: 可选参数数组；为 None 时使用进程命令行。
    返回值:
        argparse 命名空间。
    调用示例:
        `args = parse_args(["--config", "x.json"])`
    """
    parser = argparse.ArgumentParser(description="Check hardware smoke preflight configuration")
    parser.add_argument("--config", default=str(DEFAULT_CONFIG), help="hardware smoke preflight JSON path")
    parser.add_argument("--check-tools", action="store_true", help="verify command executables are on PATH")
    parser.add_argument("--target", default="all", choices=["all", "STM32", "DSP"], help="target section to check")
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    """程序入口。

    参数:
        argv: 可选命令行参数。
    返回值:
        检查通过返回 0；存在错误返回 1。
    调用示例:
        `raise SystemExit(main())`
    """
    args = parse_args(argv)
    target_filter = None if args.target == "all" else (args.target,)
    failures = check_config_file(Path(args.config), check_tools=args.check_tools, target_filter=target_filter)
    if failures:
        for failure in failures:
            print(f"[hardware-preflight] {failure}")
        print("[hardware-preflight] hardware smoke preflight config rejected")
        return 1
    print("[hardware-preflight] hardware smoke preflight config accepted")
    return 0


if __name__ == "__main__":
    sys.exit(main())
