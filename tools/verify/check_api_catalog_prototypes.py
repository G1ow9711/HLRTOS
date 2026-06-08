#!/usr/bin/env python3
"""检查 MyRTOS API 目录、公共头文件和源文件原型是否一致。"""

from __future__ import annotations

import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
API_CATALOG = ROOT / "docs" / "api" / "myrtos_api_catalog.md"
INCLUDE_DIR = ROOT / "include"
SOURCE_DIRS = [ROOT / "src", ROOT / "examples"]
EXCLUDED_API_PREFIXES = ("MRT_PortMock", "MRT_KernelTest", "MRT_List", "MRT_PriorityBitmap")
MACRO_APIS = {"MRT_ASSERT"}
SIGNATURE_PREFIXES = (
    "MRT_Result ",
    "void ",
    "bool ",
    "size_t ",
    "const char *",
    "void *",
    "MRT_TaskHandle ",
    "uint32_t ",
    "uint64_t ",
    "uintptr_t ",
    "MRT_Tick ",
    "MRT_IntState ",
    "MRT_EventBits ",
    "MRT_NotifyValue ",
)


def read_text(path: Path) -> str:
    """读取 UTF-8 文本文件。"""
    return path.read_text(encoding="utf-8")


def strip_block_comments(text: str) -> str:
    """移除 C 风格块注释，保留代码原文。"""
    return re.sub(r"/\*.*?\*/", "", text, flags=re.S)


def collect_statements(path: Path) -> list[str]:
    """将代码按语句聚合为可比较的声明/定义片段。"""
    statements: list[str] = []
    buffer: list[str] = []
    text = strip_block_comments(read_text(path))

    for raw_line in text.splitlines():
        line = raw_line.strip()
        if not line:
            continue
        if line.startswith("#"):
            continue
        if line.startswith("//"):
            continue

        buffer.append(line)

        if line.endswith(";") or line.endswith("{") or line.endswith("}"):
            statement = " ".join(buffer)
            statements.append(" ".join(statement.split()))
            buffer = []

    return statements


def extract_catalog_apis(text: str) -> list[str]:
    """从 API 目录中抽取公开 API 名称。"""
    apis: list[str] = []
    for match in re.finditer(r"\|\s+`(?P<api>MRT_[A-Za-z0-9_]+)(?:\(|`)", text):
        api = match.group("api")
        if api not in apis:
            apis.append(api)
    return apis


def is_public_api_name(name: str) -> bool:
    """判断名称是否属于需要检查的公开 API。"""
    if not name.startswith("MRT_"):
        return False
    for prefix in EXCLUDED_API_PREFIXES:
        if name.startswith(prefix):
            return False
    return True


def is_signature_statement(statement: str, terminator: str) -> bool:
    """判断语句是否像一个函数原型或定义。"""
    if not statement.endswith(terminator):
        return False
    stripped = statement.lstrip()
    return stripped.startswith(SIGNATURE_PREFIXES)


def index_statements(statements: list[tuple[str, str]], terminator: str) -> dict[str, tuple[str, str]]:
    """按 API 名称索引最匹配的原型/定义语句。"""
    indexed: dict[str, tuple[str, str]] = {}
    for path_statement in statements:
        path, statement = path_statement
        if not is_signature_statement(statement, terminator):
            continue
        match = re.search(r"\b(MRT_[A-Za-z0-9_]+)\s*\(", statement)
        if match is None:
            continue
        name = match.group(1)
        if not is_public_api_name(name):
            continue
        if name not in indexed:
            indexed[name] = (path, statement)
    return indexed


def collect_path_statements(paths: list[Path]) -> list[tuple[str, str]]:
    """收集多个文件中的语句并保留来源路径。"""
    collected: list[tuple[str, str]] = []
    for path in paths:
        if not path.exists():
            continue
        for statement in collect_statements(path):
            collected.append((str(path.relative_to(ROOT)), statement))
    return collected


def normalize_signature(statement: str) -> str:
    """规整语句文本，便于跨文件比较。"""
    return " ".join(statement.rstrip("{;").split())


def main() -> int:
    """执行原型一致性检查。"""
    failures: list[str] = []

    if not API_CATALOG.exists():
        print(f"[api-catalog] missing API catalog: {API_CATALOG}")
        return 1

    catalog_text = read_text(API_CATALOG)
    catalog_apis = extract_catalog_apis(catalog_text)
    if not catalog_apis:
        print("[api-catalog] no APIs found in catalog")
        return 1

    header_paths = sorted(INCLUDE_DIR.rglob("*.h"))
    source_paths: list[Path] = []
    for directory in SOURCE_DIRS:
        if directory.exists():
            source_paths.extend(sorted(directory.rglob("*.c")))

    header_index = index_statements(collect_path_statements(header_paths), ";")
    source_index = index_statements(collect_path_statements(source_paths), "{")

    for api in catalog_apis:
        if api in MACRO_APIS:
            header_text = "\n".join(read_text(path) for path in header_paths)
            if f"#define {api}" not in header_text:
                failures.append(f"missing header macro: {api}")
            continue

        header_entry = header_index.get(api)
        source_entry = source_index.get(api)
        if header_entry is None:
            failures.append(f"missing header prototype: {api}")
        if source_entry is None:
            failures.append(f"missing source definition: {api}")
        if header_entry is None or source_entry is None:
            continue

        header_path, header_statement = header_entry
        source_path, source_statement = source_entry
        if normalize_signature(header_statement) != normalize_signature(source_statement):
            failures.append(f"signature mismatch: {api} :: {header_path} != {source_path}")

    header_only = sorted(name for name in header_index if name not in catalog_apis)
    for name in header_only:
        failures.append(f"header API not in catalog: {name}")

    if failures:
        for failure in failures:
            print(f"[api-catalog] {failure}")
        print(f"[api-catalog] {len(failures)} failure(s)")
        return 1

    print(f"[api-catalog] {len(catalog_apis)} API prototype(s) aligned")
    return 0


if __name__ == "__main__":
    sys.exit(main())
