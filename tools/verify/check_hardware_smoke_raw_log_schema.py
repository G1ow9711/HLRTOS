#!/usr/bin/env python3
"""检查硬件 smoke 原始日志字段 schema 与生成器、文档、示例头文件是否一致。"""

from __future__ import annotations

import argparse
import importlib.util
import sys
from pathlib import Path
from types import ModuleType


ROOT = Path(__file__).resolve().parents[2]
GENERATOR = ROOT / "tools" / "verify" / "generate_hardware_smoke_evidence.py"
RAW_LOG_CHECKER = ROOT / "tools" / "verify" / "check_hardware_smoke_raw_log.py"
SCHEMA_DOC = ROOT / "docs" / "verification" / "hardware_smoke" / "raw_log_schema.md"
SCHEMA_HEADER = ROOT / "examples" / "hardware_smoke" / "mrt_hardware_smoke_log_schema.h"
REPORT_HEADER = ROOT / "examples" / "hardware_smoke" / "mrt_hardware_smoke_report.h"
GUIDE_FILES = (
    ROOT / "docs" / "verification" / "hardware_smoke" / "README.md",
    ROOT / "docs" / "verification" / "hardware_smoke" / "collection_checklist.md",
    ROOT / "examples" / "stm32" / "README.md",
    ROOT / "examples" / "dsp" / "README.md",
)
HEADER_PREFIX_TOKEN = "MRT_SMOKE_FIELD"


def read_text(path: Path) -> str:
    """读取 UTF-8 文本文件。
    参数:
        path: 需要读取的文件路径。
    返回:
        返回完整文本内容。
    调用示例:
        `text = read_text(SCHEMA_DOC)`
    """
    return path.read_text(encoding="utf-8")


def load_generator() -> ModuleType:
    """加载硬件 smoke 证据生成器模块。
    参数:
        无。
    返回:
        返回已加载的 Python 模块，用于读取 COMMON_REQUIRED_FIELDS 等字段常量。
    调用示例:
        `generator = load_generator()`
    """
    spec = importlib.util.spec_from_file_location("generate_hardware_smoke_evidence", GENERATOR)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"module spec missing for {GENERATOR}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def expected_fields() -> dict[str, list[str]]:
    """从证据生成器读取目标必填字段。
    参数:
        无。
    返回:
        返回包含 STM32 和 DSP 完整字段列表的字典。
    调用示例:
        `fields = expected_fields()["STM32"]`
    """
    generator = load_generator()
    common = list(generator.COMMON_REQUIRED_FIELDS)
    return {
        "STM32": [*common, *generator.STM32_REQUIRED_FIELDS],
        "DSP": [*common, *generator.DSP_REQUIRED_FIELDS],
    }


def unique_fields(fields_by_target: dict[str, list[str]]) -> list[str]:
    """按首次出现顺序合并 STM32/DSP 字段。
    参数:
        fields_by_target: 由目标名映射到字段列表的字典。
    返回:
        返回去重后的字段列表。
    调用示例:
        `for field in unique_fields(expected_fields()): ...`
    """
    ordered: list[str] = []
    seen: set[str] = set()
    for fields in fields_by_target.values():
        for field in fields:
            if field in seen:
                continue
            seen.add(field)
            ordered.append(field)
    return ordered


def field_macro_token(field: str) -> str:
    """把 raw log 字段名转换为示例 C 头文件里的字段宏名。
    参数:
        field: 生成器要求的原始日志字段名，例如 `Evidence-Status`。
    返回:
        返回对应的 `MRT_SMOKE_FIELD_*` 宏名字符串。
    调用示例:
        `token = field_macro_token("Evidence-Status")`
    """
    macro_body = "".join(character if character.isalnum() else "_" for character in field)
    return f"MRT_SMOKE_FIELD_{macro_body.upper()}"


def check_artifacts_exist() -> list[str]:
    """检查 schema 文档和示例头文件是否存在。
    参数:
        无。
    返回:
        返回缺失文件错误列表。
    调用示例:
        `failures = check_artifacts_exist()`
    """
    failures: list[str] = []
    for path in (RAW_LOG_CHECKER, SCHEMA_DOC, SCHEMA_HEADER, REPORT_HEADER):
        if not path.exists():
            failures.append(f"missing {path.relative_to(ROOT)}")
    return failures


def check_field_coverage(fields: list[str], doc_text: str, header_text: str) -> list[str]:
    """检查字段是否同时出现在 schema 文档和示例头文件中。
    参数:
        fields: 需要覆盖的字段名列表。
        doc_text: schema 文档内容。
        header_text: 示例 C 头文件内容。
    返回:
        返回字段缺失错误列表。
    调用示例:
        `failures = check_field_coverage(fields, doc, header)`
    """
    failures: list[str] = []
    for field in fields:
        if field not in doc_text:
            failures.append(f"raw_log_schema.md missing field {field}")
        if field not in header_text:
            failures.append(f"mrt_hardware_smoke_log_schema.h missing field {field}")
    if HEADER_PREFIX_TOKEN not in header_text:
        failures.append(f"header missing {HEADER_PREFIX_TOKEN} constants")
    return failures


def check_report_header_coverage(fields: list[str], report_text: str) -> list[str]:
    """检查完整报告 emitter 是否引用所有生成器必填字段。
    参数:
        fields: 需要由 STM32/DSP 完整报告输出的去重字段列表。
        report_text: `mrt_hardware_smoke_report.h` 的文件内容。
    返回:
        返回 report header 字段引用缺失错误列表。
    调用示例:
        `failures = check_report_header_coverage(fields, report_text)`
    """
    failures: list[str] = []
    for field in fields:
        token = field_macro_token(field)
        if token not in report_text:
            failures.append(f"mrt_hardware_smoke_report.h missing field token {token}")
    if "MRT_SmokeEmitStm32Report" not in report_text:
        failures.append("mrt_hardware_smoke_report.h missing MRT_SmokeEmitStm32Report")
    if "MRT_SmokeEmitDspReport" not in report_text:
        failures.append("mrt_hardware_smoke_report.h missing MRT_SmokeEmitDspReport")
    return failures


def check_raw_log_checker_coverage(fields: list[str], checker_text: str) -> list[str]:
    """妫€鏌ュ師濮嬫棩蹇楃洿妫€鑴氭湰鏄惁瑕嗙洊鐢熸垚鍣ㄥ繀濉瓧娈点€?
    鍙傛暟:
        fields: 闇€瑕佺洿妫€鐨勫師濮嬫棩蹇楀瓧娈靛垪琛ㄣ€?
        checker_text: `check_hardware_smoke_raw_log.py` 鐨勬枃浠跺唴瀹广€?
    杩斿洖:
        杩斿洖 raw-log checker 瀛楁缂哄け閿欒鍒楄〃銆?
    璋冪敤绀轰緥:
        `failures = check_raw_log_checker_coverage(fields, checker_text)`
    """
    failures: list[str] = []
    for field in fields:
        if field not in checker_text:
            failures.append(f"check_hardware_smoke_raw_log.py missing field {field}")
    return failures


def check_guides_link_schema() -> list[str]:
    """检查硬件 smoke 指南是否链接统一 raw log schema。
    参数:
        无。
    返回:
        返回缺少链接的文件错误列表。
    调用示例:
        `failures.extend(check_guides_link_schema())`
    """
    failures: list[str] = []
    for path in GUIDE_FILES:
        text = read_text(path)
        if "raw_log_schema.md" not in text:
            failures.append(f"{path.relative_to(ROOT)} must mention raw_log_schema.md")
    return failures


def check_schema() -> list[str]:
    """执行完整 raw log schema 静态校验。
    参数:
        无。
    返回:
        返回所有发现的错误；空列表表示通过。
    调用示例:
        `failures = check_schema()`
    """
    failures = check_artifacts_exist()
    if failures:
        return failures

    fields = unique_fields(expected_fields())
    doc_text = read_text(SCHEMA_DOC)
    header_text = read_text(SCHEMA_HEADER)
    report_text = read_text(REPORT_HEADER)
    checker_text = read_text(RAW_LOG_CHECKER)
    failures.extend(check_field_coverage(fields, doc_text, header_text))
    failures.extend(check_report_header_coverage(fields, report_text))
    failures.extend(check_raw_log_checker_coverage(fields, checker_text))
    failures.extend(check_guides_link_schema())
    return failures


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    """解析命令行参数。
    参数:
        argv: 可选命令行参数列表；为 None 时读取进程参数。
    返回:
        返回 argparse 解析结果。
    调用示例:
        `args = parse_args(["--quiet"])`
    """
    parser = argparse.ArgumentParser(description="Check hardware smoke raw log schema alignment")
    parser.add_argument("--quiet", action="store_true", help="only print failures")
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    """程序入口。
    参数:
        argv: 可选命令行参数列表。
    返回:
        校验通过返回 0，否则返回 1。
    调用示例:
        `raise SystemExit(main())`
    """
    args = parse_args(argv)
    failures = check_schema()
    if failures:
        for failure in failures:
            print(f"[hardware-raw-log-schema] {failure}", file=sys.stderr)
        return 1
    if not args.quiet:
        print("[hardware-raw-log-schema] schema fields aligned")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
