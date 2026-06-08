#!/usr/bin/env python3
"""验证硬件 smoke 原始日志字段契约的静态一致性。"""

from __future__ import annotations

import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
GENERATOR = ROOT / "tools" / "verify" / "generate_hardware_smoke_evidence.py"
SCHEMA_CHECKER = ROOT / "tools" / "verify" / "check_hardware_smoke_raw_log_schema.py"
RAW_LOG_CHECKER = ROOT / "tools" / "verify" / "check_hardware_smoke_raw_log.py"
SCHEMA_DOC = ROOT / "docs" / "verification" / "hardware_smoke" / "raw_log_schema.md"
SCHEMA_HEADER = ROOT / "examples" / "hardware_smoke" / "mrt_hardware_smoke_log_schema.h"
REPORT_HEADER = ROOT / "examples" / "hardware_smoke" / "mrt_hardware_smoke_report.h"
HARDWARE_README = ROOT / "docs" / "verification" / "hardware_smoke" / "README.md"
COLLECTION_CHECKLIST = ROOT / "docs" / "verification" / "hardware_smoke" / "collection_checklist.md"
STM32_README = ROOT / "examples" / "stm32" / "README.md"
DSP_README = ROOT / "examples" / "dsp" / "README.md"
RUNNER = ROOT / "tools" / "verify" / "run_release_verification.py"


def read_text(path: Path) -> str:
    """读取 UTF-8 文本文件并返回完整内容。"""
    return path.read_text(encoding="utf-8")


def load_module(path: Path, name: str):
    """按文件路径加载 Python 模块，便于读取生成器和 release runner 常量。"""
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise AssertionError(f"module spec missing for {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def expected_fields() -> dict[str, list[str]]:
    """从证据生成器读取 STM32 与 DSP 必填字段。"""
    generator = load_module(GENERATOR, "generate_hardware_smoke_evidence")
    common = list(generator.COMMON_REQUIRED_FIELDS)
    return {
        "STM32": [*common, *generator.STM32_REQUIRED_FIELDS],
        "DSP": [*common, *generator.DSP_REQUIRED_FIELDS],
    }


def field_macro_token(field: str) -> str:
    """把 raw log 字段名转换为 schema 头文件中的 `MRT_SMOKE_FIELD_*` 宏名。"""
    macro_body = "".join(character if character.isalnum() else "_" for character in field)
    return f"MRT_SMOKE_FIELD_{macro_body.upper()}"


def test_schema_artifacts_exist() -> None:
    """raw log schema 必须有文档、示例头文件和独立校验脚本。"""
    for path in [SCHEMA_CHECKER, RAW_LOG_CHECKER, SCHEMA_DOC, SCHEMA_HEADER, REPORT_HEADER]:
        assert path.exists(), f"missing {path.relative_to(ROOT)}"


def test_schema_doc_and_header_cover_generator_fields() -> None:
    """schema 文档和示例头文件必须覆盖生成器所有必填字段。"""
    doc_text = read_text(SCHEMA_DOC)
    header_text = read_text(SCHEMA_HEADER)
    for fields in expected_fields().values():
        for field in fields:
            assert field in doc_text
            assert field in header_text


def test_report_header_covers_generator_fields() -> None:
    """完整报告 emitter 必须引用生成器要求的 STM32/DSP 必填字段常量。"""
    report_text = read_text(REPORT_HEADER)
    for fields in expected_fields().values():
        for field in fields:
            assert field_macro_token(field) in report_text
    assert "MRT_SmokeEmitStm32Report" in report_text
    assert "MRT_SmokeEmitDspReport" in report_text


def test_raw_log_checker_covers_generator_fields() -> None:
    """原始日志直检工具必须覆盖生成器要求的 STM32/DSP 目标专属字段。"""
    checker_text = read_text(RAW_LOG_CHECKER)
    for field in expected_fields()["STM32"]:
        if field in ["Raw-Log-Path", "Raw-Log-SHA256"]:
            continue
        assert field in checker_text
    for field in expected_fields()["DSP"]:
        if field in ["Raw-Log-Path", "Raw-Log-SHA256"]:
            continue
        assert field in checker_text


def test_guides_point_to_raw_log_schema() -> None:
    """硬件 smoke 指南和示例说明必须指向统一 raw log schema。"""
    for path in [HARDWARE_README, COLLECTION_CHECKLIST, STM32_README, DSP_README]:
        text = read_text(path)
        assert "raw_log_schema.md" in text


def test_schema_checker_and_release_runner_are_wired() -> None:
    """schema 校验脚本必须纳入默认 release 验证链路。"""
    checker_text = read_text(SCHEMA_CHECKER)
    assert "COMMON_REQUIRED_FIELDS" in checker_text
    assert "MRT_SMOKE_FIELD" in checker_text
    assert "mrt_hardware_smoke_report.h" in checker_text
    assert "check_hardware_smoke_raw_log.py" in checker_text
    runner = load_module(RUNNER, "run_release_verification")
    names = [step.name for step in runner.build_steps(require_hardware=False)]
    assert "hardware-smoke-raw-log-schema" in names


def main() -> int:
    """运行硬件 smoke raw log schema 静态测试。"""
    test_schema_artifacts_exist()
    test_schema_doc_and_header_cover_generator_fields()
    test_report_header_covers_generator_fields()
    test_raw_log_checker_covers_generator_fields()
    test_guides_point_to_raw_log_schema()
    test_schema_checker_and_release_runner_are_wired()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
