#!/usr/bin/env python3
"""检查 MyRTOS 中文参考手册是否覆盖 API 目录中的公共接口。"""

from __future__ import annotations

import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
API_CATALOG = ROOT / "docs" / "api" / "myrtos_api_catalog.md"
MANUAL = ROOT / "docs" / "manual" / "MyRTOS_Reference_Manual_zh.md"

REQUIRED_API_FIELDS = [
    "函数原型",
    "功能说明",
    "参数",
    "返回值",
    "调用上下文",
    "阻塞行为",
    "ISR 限制",
    "配置宏",
    "调用示例",
    "常见错误",
]

PORTING_REQUIRED_TERMS = [
    "移植前准备",
    "工程分层",
    "关键接入顺序",
    "首次联调",
    "板级验收",
    "STM32 Cortex-M 移植步骤",
    "DSP 移植步骤",
    "工具链",
    "启动文件",
    "向量表",
    "tick",
    "上下文切换",
    "栈布局",
    "临界区",
    "低功耗",
    "示例",
    "排错",
]


def read_text(path: Path) -> str:
    """读取 UTF-8 文本文件。"""
    return path.read_text(encoding="utf-8")


def extract_catalog_apis(text: str) -> list[str]:
    """从 API 目录表格中抽取公共 API 名称。"""
    apis: list[str] = []
    for match in re.finditer(r"\|\s+`(?P<api>MRT_[A-Za-z0-9_]+)(?:\(|`)", text):
        api = match.group("api")
        if api not in apis:
            apis.append(api)
    return apis


def extract_section(text: str, api: str) -> str:
    """抽取指定 API 的手册章节片段。"""
    pattern = re.compile(rf"^###\s+{re.escape(api)}\b.*?$", re.MULTILINE)
    match = pattern.search(text)
    if match is None:
        return ""
    next_match = re.search(r"^###\s+MRT_[A-Za-z0-9_]+\b.*?$", text[match.end() :], re.MULTILINE)
    if next_match is None:
        return text[match.start() :]
    return text[match.start() : match.end() + next_match.start()]


def main() -> int:
    """执行手册覆盖检查。"""
    failures: list[str] = []

    if not API_CATALOG.exists():
        print(f"[manual-coverage] missing API catalog: {API_CATALOG}")
        return 1

    if not MANUAL.exists():
        print(f"[manual-coverage] missing manual: {MANUAL}")
        return 1

    api_text = read_text(API_CATALOG)
    manual_text = read_text(MANUAL)
    apis = extract_catalog_apis(api_text)

    if not apis:
        failures.append("no APIs found in catalog")

    for api in apis:
        section = extract_section(manual_text, api)
        if not section:
            failures.append(f"missing API section: {api}")
            continue
        for field in REQUIRED_API_FIELDS:
            if field not in section:
                failures.append(f"{api}: missing field {field}")

    for term in PORTING_REQUIRED_TERMS:
        if term not in manual_text:
            failures.append(f"manual missing porting term: {term}")

    if failures:
        for failure in failures:
            print(f"[manual-coverage] {failure}")
        print(f"[manual-coverage] {len(failures)} failure(s)")
        return 1

    print(f"[manual-coverage] {len(apis)} API section(s) covered")
    return 0


if __name__ == "__main__":
    sys.exit(main())
