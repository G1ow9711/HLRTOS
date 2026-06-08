#!/usr/bin/env python3
"""检查真实 STM32/DSP 板级 smoke 证据日志是否满足最终验收格式。"""

from __future__ import annotations

import hashlib
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
DEFAULT_EVIDENCE_DIR = ROOT / "docs" / "verification" / "hardware_smoke"

STM32_FILE = "stm32_board_smoke.md"
DSP_FILE = "dsp_board_smoke.md"

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
    "Raw-Log-Path",
    "Raw-Log-SHA256",
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

PASS_FIELDS = [
    "Evidence-Status",
    "Software-Timer",
    "Tickless",
    "SysTick",
    "PendSV-SVC",
    "ISR-Queue",
    "Timer-Tick",
    "Software-Interrupt-Switch",
    "ISR-Nesting",
    "Queue-Or-Pool",
]


def read_text(path: Path) -> str:
    """读取 UTF-8 文本文件。"""
    return path.read_text(encoding="utf-8")


def parse_fields(text: str) -> dict[str, str]:
    """从 smoke 日志中解析 `Key: Value` 字段。"""
    fields: dict[str, str] = {}
    for raw_line in text.splitlines():
        line = raw_line.strip()
        if not line:
            continue
        if line.startswith("#"):
            continue
        if line.startswith("- "):
            line = line[2:].strip()
        if ":" not in line:
            continue
        key, value = line.split(":", 1)
        fields[key.strip()] = value.strip()
    return fields


def require_present(fields: dict[str, str], required: list[str], label: str) -> list[str]:
    """检查必填字段存在且非空。"""
    failures: list[str] = []
    for field in required:
        if not fields.get(field):
            failures.append(f"{label}: missing field {field}")
    return failures


def require_pass(fields: dict[str, str], label: str) -> list[str]:
    """检查存在的状态字段必须为 PASS。"""
    failures: list[str] = []
    for field in PASS_FIELDS:
        if field not in fields:
            continue
        if fields[field].upper() != "PASS":
            failures.append(f"{label}: {field} must be PASS")
    return failures


def parse_uint(fields: dict[str, str], field: str, label: str) -> tuple[int | None, list[str]]:
    """解析非负整数验收字段。"""
    value = fields.get(field, "")
    if not re.fullmatch(r"[0-9]+", value):
        return None, [f"{label}: {field} must be an integer"]
    return int(value), []


def require_numeric_rules(fields: dict[str, str], label: str) -> list[str]:
    """检查运行时长、断言次数和堆剩余量等数值规则。"""
    failures: list[str] = []

    runtime_minutes, parse_failures = parse_uint(fields, "Runtime-Minutes", label)
    failures.extend(parse_failures)
    if runtime_minutes is not None and runtime_minutes < 30:
        failures.append(f"{label}: Runtime-Minutes must be >= 30")

    assert_failures, parse_failures = parse_uint(fields, "Assert-Failures", label)
    failures.extend(parse_failures)
    if assert_failures is not None and assert_failures != 0:
        failures.append(f"{label}: Assert-Failures must be 0")

    heap_min_free, parse_failures = parse_uint(fields, "Heap-Min-Free-Bytes", label)
    failures.extend(parse_failures)
    if heap_min_free is not None and heap_min_free == 0:
        failures.append(f"{label}: Heap-Min-Free-Bytes must be > 0")

    if "Clock-Hz" in fields:
        clock_hz, parse_failures = parse_uint(fields, "Clock-Hz", label)
        failures.extend(parse_failures)
        if clock_hz is not None and clock_hz == 0:
            failures.append(f"{label}: Clock-Hz must be > 0")

    if "Tick-Hz" in fields:
        tick_hz, parse_failures = parse_uint(fields, "Tick-Hz", label)
        failures.extend(parse_failures)
        if tick_hz is not None and tick_hz == 0:
            failures.append(f"{label}: Tick-Hz must be > 0")

    if "NVIC-Priority-Bits" in fields:
        nvic_bits, parse_failures = parse_uint(fields, "NVIC-Priority-Bits", label)
        failures.extend(parse_failures)
        if nvic_bits is not None and nvic_bits == 0:
            failures.append(f"{label}: NVIC-Priority-Bits must be > 0")

    return failures


def require_date(fields: dict[str, str], label: str) -> list[str]:
    """检查 smoke 日期字段格式。"""
    value = fields.get("Smoke-Date", "")
    if not re.fullmatch(r"[0-9]{4}-[0-9]{2}-[0-9]{2}", value):
        return [f"{label}: Smoke-Date must use YYYY-MM-DD"]
    return []


def sha256_file(path: Path) -> str:
    """计算原始日志文件的 SHA-256 摘要。"""
    digest = hashlib.sha256()
    with path.open("rb") as raw_file:
        for chunk in iter(lambda: raw_file.read(65536), b""):
            digest.update(chunk)
    return digest.hexdigest()


def resolve_raw_log_path(raw_value: str, evidence_dir: Path) -> Path | None:
    """按绝对路径、仓库根路径和证据目录路径解析原始日志位置。"""
    raw_path = Path(raw_value)
    if raw_path.is_absolute():
        candidates = [raw_path]
    else:
        candidates = [ROOT / raw_path, evidence_dir / raw_path, evidence_dir / raw_path.name]
    for candidate in candidates:
        if candidate.exists():
            return candidate
    return None


def require_raw_log_hash(fields: dict[str, str], label: str, evidence_dir: Path) -> list[str]:
    """检查最终证据中的原始日志路径和 SHA-256 是否匹配。"""
    failures: list[str] = []
    raw_value = fields.get("Raw-Log-Path", "")
    expected_hash = fields.get("Raw-Log-SHA256", "")
    if not raw_value or not expected_hash:
        return failures
    if not re.fullmatch(r"[0-9a-fA-F]{64}", expected_hash):
        failures.append(f"{label}: Raw-Log-SHA256 must be 64 hex characters")
        return failures
    raw_path = resolve_raw_log_path(raw_value, evidence_dir)
    if raw_path is None:
        failures.append(f"{label}: Raw-Log-Path file not found")
        return failures
    actual_hash = sha256_file(raw_path)
    if actual_hash.lower() != expected_hash.lower():
        failures.append(f"{label}: Raw-Log-SHA256 mismatch")
    return failures


def check_file(path: Path, expected_target: str, extra_fields: list[str]) -> list[str]:
    """检查单个硬件 smoke 证据文件。"""
    label = path.name
    failures: list[str] = []

    if not path.exists():
        return [f"{label}: missing evidence file"]

    fields = parse_fields(read_text(path))
    required = [*COMMON_REQUIRED_FIELDS, *extra_fields]
    failures.extend(require_present(fields, required, label))
    target = fields.get("MyRTOS-Hardware-Smoke", "")
    if target and target.upper() != expected_target:
        failures.append(f"{label}: MyRTOS-Hardware-Smoke must be {expected_target}")

    failures.extend(require_pass(fields, label))
    failures.extend(require_date(fields, label))
    failures.extend(require_numeric_rules(fields, label))
    failures.extend(require_raw_log_hash(fields, label, path.parent))

    return failures


def check_evidence_dir(evidence_dir: Path) -> list[str]:
    """检查包含 STM32/DSP smoke 证据的目录。"""
    failures: list[str] = []
    failures.extend(check_file(evidence_dir / STM32_FILE, "STM32", STM32_REQUIRED_FIELDS))
    failures.extend(check_file(evidence_dir / DSP_FILE, "DSP", DSP_REQUIRED_FIELDS))
    return failures


def main(argv: list[str] | None = None) -> int:
    """执行硬件 smoke 证据校验。"""
    args = sys.argv[1:] if argv is None else argv
    evidence_dir = Path(args[0]) if args else DEFAULT_EVIDENCE_DIR
    failures = check_evidence_dir(evidence_dir)

    if failures:
        for failure in failures:
            print(f"[hardware-smoke] {failure}")
        print("[hardware-smoke] real STM32/DSP board smoke evidence incomplete")
        return 1

    print("[hardware-smoke] STM32 and DSP board smoke evidence accepted")
    return 0


if __name__ == "__main__":
    sys.exit(main())
