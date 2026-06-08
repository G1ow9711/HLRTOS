#!/usr/bin/env python3
"""检查真实 STM32/DSP 板级 smoke 原始 UART/trace 日志是否满足生成证据前置规则。"""

from __future__ import annotations

import argparse
import importlib.util
import sys
from pathlib import Path
from types import ModuleType


ROOT = Path(__file__).resolve().parents[2]
VERIFY_DIR = ROOT / "tools" / "verify"

COMMON_RAW_LOG_FIELDS = [
    "MyRTOS-Hardware-Smoke",
    "Evidence-Status",
    "Smoke-Date",
    "Chip",
    "Board",
    "Compiler",
    "Context-Switch",
    "Software-Timer",
    "Tickless",
    "Runtime-Minutes",
    "Assert-Failures",
    "Heap-Min-Free-Bytes",
    "Trace-Or-UART-Log",
    "Raw-Log-Path",
    "Raw-Log-SHA256",
]

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


def load_module(module_name: str, path: Path) -> ModuleType:
    """按文件路径加载相邻校验模块。
    参数:
        module_name: 需要注册到解释器中的模块名。
        path: 模块文件路径。
    返回值:
        加载后的 Python 模块对象。
    调用示例:
        `generator = load_module("generate_hardware_smoke_evidence", VERIFY_DIR / "generate_hardware_smoke_evidence.py")`
    """
    spec = importlib.util.spec_from_file_location(module_name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"{module_name}: module spec missing")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def build_raw_log_fields(raw_log_path: Path) -> dict[str, str]:
    """读取原始日志并派生最终证据所需的 raw-log 路径和 SHA-256 字段。
    参数:
        raw_log_path: 原始 UART/trace 日志文件路径。
    返回值:
        返回从 `Key: Value` 行提取并补齐 `Raw-Log-Path`、`Raw-Log-SHA256` 的字段表。
    调用示例:
        `fields = build_raw_log_fields(Path("stm32_uart_raw.log"))`
    """
    generator = load_module("generate_hardware_smoke_evidence", VERIFY_DIR / "generate_hardware_smoke_evidence.py")
    raw_text = raw_log_path.read_text(encoding="utf-8")
    fields = generator.parse_fields(raw_text)
    fields["Raw-Log-Path"] = str(raw_log_path)
    fields["Raw-Log-SHA256"] = generator.sha256_file(raw_log_path)
    return fields


def check_raw_log_fields(fields: dict[str, str], expected_target: str, label: str) -> list[str]:
    """检查原始日志字段是否满足目标板级 smoke 规则。
    参数:
        fields: 已解析并派生 raw-log 字段的键值表。
        expected_target: 期望目标，必须是 `STM32` 或 `DSP`。
        label: 错误输出中的日志名称。
    返回值:
        返回失败描述列表；空列表表示通过。
    调用示例:
        `failures = check_raw_log_fields(fields, "STM32", "stm32_uart_raw.log")`
    """
    evidence_checker = load_module("check_hardware_smoke_evidence", VERIFY_DIR / "check_hardware_smoke_evidence.py")
    if expected_target not in TARGET_EXTRA_FIELDS:
        return [f"{label}: unsupported target {expected_target}"]

    required = [*COMMON_RAW_LOG_FIELDS, *TARGET_EXTRA_FIELDS[expected_target]]
    failures: list[str] = []

    failures.extend(evidence_checker.require_present(fields, required, label))

    # 目标字段即使存在错误，也继续检查其他可见字段，便于一次修完采集脚本问题。
    target = fields.get("MyRTOS-Hardware-Smoke", "")
    if target and target.upper() != expected_target:
        failures.append(f"{label}: MyRTOS-Hardware-Smoke must be {expected_target}")

    # PASS、日期和数值规则复用最终 evidence checker，避免两条验收链规则漂移。
    failures.extend(evidence_checker.require_pass(fields, label))
    failures.extend(evidence_checker.require_date(fields, label))
    failures.extend(evidence_checker.require_numeric_rules(fields, label))
    return failures


def check_raw_log_file(raw_log_path: Path, expected_target: str) -> list[str]:
    """检查单个真实板级 smoke 原始日志文件。
    参数:
        raw_log_path: 原始 UART/trace 日志路径。
        expected_target: 目标类型，必须是 `STM32` 或 `DSP`。
    返回值:
        返回失败描述列表；空列表表示日志可以进入证据生成步骤。
    调用示例:
        `failures = check_raw_log_file(Path("docs/verification/hardware_smoke/stm32_uart_raw.log"), "STM32")`
    """
    label = raw_log_path.name
    if not raw_log_path.exists():
        return [f"{label}: missing raw log file"]
    fields = build_raw_log_fields(raw_log_path)
    return check_raw_log_fields(fields, expected_target, label)


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    """解析原始日志直检命令行参数。
    参数:
        argv: 可选命令行参数列表；为 None 时使用进程参数。
    返回值:
        返回 argparse 解析结果。
    调用示例:
        `args = parse_args(["--target", "STM32", "--input", "stm32_uart_raw.log"])`
    """
    parser = argparse.ArgumentParser(description="Check raw STM32/DSP hardware smoke UART/trace log")
    parser.add_argument("--target", required=True, choices=["STM32", "DSP"], help="hardware target type")
    parser.add_argument("--input", required=True, help="raw UART/trace log path")
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    """执行原始日志直检命令。
    参数:
        argv: 可选命令行参数列表；为 None 时使用进程参数。
    返回值:
        通过返回 0，失败返回 1。
    调用示例:
        `raise SystemExit(main())`
    """
    args = parse_args(argv)
    failures = check_raw_log_file(Path(args.input), args.target)
    if failures:
        for failure in failures:
            print(f"[hardware-raw-log] {failure}")
        print("[hardware-raw-log] raw hardware smoke log incomplete")
        return 1
    print(f"[hardware-raw-log] {args.target} raw log accepted")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
