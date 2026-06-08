#!/usr/bin/env python3
"""验证中文参考手册的整体结构契约。"""

from __future__ import annotations

import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
CHECKER = ROOT / "tools" / "verify" / "check_manual_structure.py"
MANUAL = ROOT / "docs" / "manual" / "MyRTOS_Reference_Manual_zh.md"

TOP_LEVEL_HEADINGS = [
    "# MyRTOS 中文参考手册",
    "## 1. 关于本手册",
    "## 2. API 使用规则",
    "## 3. 快速上手",
    "## 4. API 参考",
    "## 5. STM32 Cortex-M 移植步骤",
    "## 6. DSP 移植步骤",
    "## 7. 附录",
]

API_FAMILIES = [
    "内核控制",
    "任务管理",
    "队列",
    "信号量",
    "互斥锁",
    "事件组",
    "任务通知",
    "软件定时器",
    "流缓冲区",
    "消息缓冲区",
    "内存管理",
    "低功耗与诊断",
    "端口层",
]


def read_text(path: Path) -> str:
    """读取 UTF-8 文本文件。"""
    return path.read_text(encoding="utf-8")


def load_checker():
    """按路径加载手册结构检查脚本。"""
    spec = importlib.util.spec_from_file_location("check_manual_structure", CHECKER)
    if spec is None or spec.loader is None:
        raise AssertionError("checker module spec missing")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def test_manual_structure_checker_exists() -> None:
    """手册结构检查脚本必须存在。"""
    assert CHECKER.exists()


def test_manual_has_reference_manual_order() -> None:
    """手册顶层章节必须按参考手册阅读顺序排列。"""
    text = read_text(MANUAL)
    last_index = -1
    for heading in TOP_LEVEL_HEADINGS:
        index = text.find(heading)
        assert index > last_index
        last_index = index


def test_manual_has_api_family_navigation() -> None:
    """API 参考章必须提供模块族导航，便于按官方手册式分类查找。"""
    text = read_text(MANUAL)
    assert "### 4.1 API 分类导航" in text
    for family in API_FAMILIES:
        assert family in text


def test_checker_accepts_current_manual() -> None:
    """手册结构检查器必须接受当前手册。"""
    checker = load_checker()
    failures = checker.check_manual_structure(read_text(MANUAL))
    assert failures == []


def main() -> int:
    """运行手册结构静态测试。"""
    test_manual_structure_checker_exists()
    test_manual_has_reference_manual_order()
    test_manual_has_api_family_navigation()
    test_checker_accepts_current_manual()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
