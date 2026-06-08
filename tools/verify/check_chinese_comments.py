#!/usr/bin/env python3
"""检查 include/src/examples/tests 中 C 代码的中文函数注释覆盖。"""

from __future__ import annotations

import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SOURCE_DIRS = [ROOT / "include", ROOT / "src", ROOT / "examples", ROOT / "tests"]
CHINESE_RE = re.compile(r"[\u4e00-\u9fff]")
CONTROL_WORDS = {"if", "for", "while", "switch", "return", "sizeof"}


def read_lines(path: Path) -> list[str]:
    """读取 UTF-8 文本并按行返回。"""
    return path.read_text(encoding="utf-8").splitlines()


def candidate_function_name(signature: str) -> str | None:
    """从函数签名文本中提取函数名。"""
    cleaned = " ".join(signature.replace("*", " * ").split())
    match = re.search(r"([A-Za-z_][A-Za-z0-9_]*)\s*\([^;{}]*\)\s*$", cleaned)
    if match is None:
        return None
    name = match.group(1)
    if name in CONTROL_WORDS:
        return None
    return name


def has_required_doc(block: str, require_param: bool) -> bool:
    """判断函数前注释块是否包含必要字段。"""
    required = ["@brief", "@return", "@example"]
    if require_param:
        required.append("@param")
    return all(item in block for item in required) and CHINESE_RE.search(block) is not None


def check_source_file(path: Path) -> list[str]:
    """检查单个 C 源文件中的函数定义注释。"""
    failures: list[str] = []
    lines = read_lines(path)
    for index, line in enumerate(lines):
        if line.strip() != "{":
            continue
        signature = "\n".join(lines[max(0, index - 8) : index])
        name = candidate_function_name(signature)
        if name is None:
            continue
        doc_block = "\n".join(lines[max(0, index - 28) : index])
        require_param = "void)" not in signature
        if not has_required_doc(doc_block, require_param):
            failures.append(f"{path.relative_to(ROOT)}:{index + 1}: {name} missing Chinese Doxygen fields")
        body_preview = "\n".join(lines[index : min(len(lines), index + 18)])
        if "/*" not in body_preview or CHINESE_RE.search(body_preview) is None:
            failures.append(f"{path.relative_to(ROOT)}:{index + 1}: {name} missing nearby Chinese step comment")
    return failures


def check_header_file(path: Path) -> list[str]:
    """检查单个头文件中的公共 API 原型注释。"""
    failures: list[str] = []
    lines = read_lines(path)
    text = "\n".join(lines)
    for match in re.finditer(r"\b(MRT_[A-Za-z0-9_]+)\s*\([^;{}]*\);", text, re.MULTILINE):
        name = match.group(1)
        line_no = text[: match.start()].count("\n") + 1
        prefix = text[max(0, match.start() - 1400) : match.start()]
        require_param = "void)" not in match.group(0)
        if not has_required_doc(prefix, require_param):
            failures.append(f"{path.relative_to(ROOT)}:{line_no}: {name} missing Chinese public API docs")
    return failures


def check_source_public_function_docs(path: Path) -> list[str]:
    """检查源文件中所有函数定义的中文注释。"""
    failures: list[str] = []
    lines = read_lines(path)
    for index, line in enumerate(lines):
        if line.strip() != "{":
            continue
        signature = "\n".join(lines[max(0, index - 8) : index])
        name = candidate_function_name(signature)
        if name is None:
            continue
        doc_block = "\n".join(lines[max(0, index - 28) : index])
        require_param = "void)" not in signature
        if not has_required_doc(doc_block, require_param):
            failures.append(f"{path.relative_to(ROOT)}:{index + 1}: {name} missing Chinese Doxygen fields")
        body_preview = "\n".join(lines[index : min(len(lines), index + 18)])
        if "/*" not in body_preview or CHINESE_RE.search(body_preview) is None:
            failures.append(f"{path.relative_to(ROOT)}:{index + 1}: {name} missing nearby Chinese step comment")
    return failures


def main() -> int:
    """执行中文注释覆盖检查。"""
    failures: list[str] = []
    for directory in SOURCE_DIRS:
        for path in sorted(directory.rglob("*")):
            if path.suffix not in {".c", ".h"}:
                continue
            if path.suffix == ".c":
                failures.extend(check_source_public_function_docs(path))
            else:
                failures.extend(check_header_file(path))

    if failures:
        for failure in failures:
            print(f"[chinese-comments] {failure}")
        print(f"[chinese-comments] {len(failures)} failure(s)")
        return 1

    print("[chinese-comments] include/src/examples/tests function comments covered")
    return 0


if __name__ == "__main__":
    sys.exit(main())
