#!/usr/bin/env python3
"""从原始 UART/trace 日志生成最终硬件 smoke 证据文件。"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]

COMMON_REQUIRED_FIELDS = [
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
]

STM32_REQUIRED_FIELDS = [
    "Clock-Hz",
    "Tick-Hz",
    "NVIC-Priority-Bits",
    "Critical-Section",
    "SysTick",
    "PendSV-SVC",
    "ISR-Queue",
]

DSP_REQUIRED_FIELDS = [
    "ABI",
    "Stack-Direction",
    "Timer-Tick",
    "Software-Interrupt-Switch",
    "ISR-Nesting",
    "Queue-Or-Pool",
]


def parse_fields(text: str) -> dict[str, str]:
    """从原始日志中提取 `Key: Value` 字段。"""
    fields: dict[str, str] = {}
    for raw_line in text.splitlines():
        line = raw_line.strip()
        if not line:
            continue
        if line.startswith("#"):
            continue
        if ":" not in line:
            continue
        key, value = line.split(":", 1)
        fields[key.strip()] = value.strip()
    return fields


def parse_uint(value: str) -> int | None:
    """解析非负整数。"""
    if not re.fullmatch(r"[0-9]+", value):
        return None
    return int(value)


def validate_fields(fields: dict[str, str], expected_target: str) -> None:
    """验证证据字段是否满足最终验收规则。"""
    required = [*COMMON_REQUIRED_FIELDS]
    if expected_target == "STM32":
        required.extend(STM32_REQUIRED_FIELDS)
    elif expected_target == "DSP":
        required.extend(DSP_REQUIRED_FIELDS)
    else:
        raise ValueError(f"unsupported target: {expected_target}")

    for field in required:
        if not fields.get(field):
            raise ValueError(f"missing field {field}")

    target = fields.get("MyRTOS-Hardware-Smoke", "")
    if target.upper() != expected_target:
        raise ValueError(f"MyRTOS-Hardware-Smoke must be {expected_target}")

    if fields.get("Evidence-Status", "").upper() != "PASS":
        raise ValueError("Evidence-Status must be PASS")

    for field in [
        "Software-Timer",
        "Tickless",
        "SysTick",
        "PendSV-SVC",
        "ISR-Queue",
        "Timer-Tick",
        "Software-Interrupt-Switch",
        "ISR-Nesting",
        "Queue-Or-Pool",
    ]:
        if field in fields and fields[field].upper() != "PASS":
            raise ValueError(f"{field} must be PASS")

    runtime = parse_uint(fields["Runtime-Minutes"])
    if runtime is None or runtime < 30:
        raise ValueError("Runtime-Minutes must be >= 30")

    assert_failures = parse_uint(fields["Assert-Failures"])
    if assert_failures is None or assert_failures != 0:
        raise ValueError("Assert-Failures must be 0")

    heap_min_free = parse_uint(fields["Heap-Min-Free-Bytes"])
    if heap_min_free is None or heap_min_free == 0:
        raise ValueError("Heap-Min-Free-Bytes must be > 0")

    if "Clock-Hz" in fields:
        clock_hz = parse_uint(fields["Clock-Hz"])
        if clock_hz is None or clock_hz == 0:
            raise ValueError("Clock-Hz must be > 0")

    if "Tick-Hz" in fields:
        tick_hz = parse_uint(fields["Tick-Hz"])
        if tick_hz is None or tick_hz == 0:
            raise ValueError("Tick-Hz must be > 0")

    if "NVIC-Priority-Bits" in fields:
        nvic_bits = parse_uint(fields["NVIC-Priority-Bits"])
        if nvic_bits is None or nvic_bits == 0:
            raise ValueError("NVIC-Priority-Bits must be > 0")

    smoke_date = fields["Smoke-Date"]
    if not re.fullmatch(r"[0-9]{4}-[0-9]{2}-[0-9]{2}", smoke_date):
        raise ValueError("Smoke-Date must use YYYY-MM-DD")


def render_fields(fields: dict[str, str], expected_target: str) -> str:
    """按固定字段顺序渲染最终证据文件。"""
    ordered = [*COMMON_REQUIRED_FIELDS]
    if expected_target == "STM32":
        ordered.extend(STM32_REQUIRED_FIELDS)
    else:
        ordered.extend(DSP_REQUIRED_FIELDS)

    lines = [f"{field}: {fields[field]}" for field in ordered]
    return "\n".join(lines) + "\n"


def generate_from_raw_log(raw_log_path: Path, output_path: Path, expected_target: str) -> int:
    """从原始日志生成最终证据文件。

    参数:
        raw_log_path: 原始 UART/trace 日志路径。
        output_path: 最终证据文件路径。
        expected_target: 目标类型，必须是 STM32 或 DSP。

    返回:
        生成成功返回 0。
    """
    raw_text = raw_log_path.read_text(encoding="utf-8")
    fields = parse_fields(raw_text)
    validate_fields(fields, expected_target)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(render_fields(fields, expected_target), encoding="utf-8")
    return 0


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    """解析命令行参数。"""
    parser = argparse.ArgumentParser(description="Generate hardware smoke evidence from raw logs")
    parser.add_argument("--input", required=True, help="raw UART/trace log path")
    parser.add_argument("--output", required=True, help="final evidence output path")
    parser.add_argument(
        "--target",
        required=True,
        choices=["STM32", "DSP"],
        help="hardware target type",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    """程序入口。"""
    args = parse_args(argv)
    return generate_from_raw_log(Path(args.input), Path(args.output), args.target)


if __name__ == "__main__":
    sys.exit(main())
