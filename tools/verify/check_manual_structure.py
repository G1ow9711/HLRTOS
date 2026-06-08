#!/usr/bin/env python3
"""检查 MyRTOS 中文参考手册是否保持参考手册式结构。"""

from __future__ import annotations

import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
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

REFERENCE_MANUAL_MARKERS = [
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

PORTING_MARKERS = [
    "移植前准备",
    "工程分层",
    "关键接入顺序",
    "首次联调",
    "板级验收",
    "真实板级证据归档",
]


def read_text(path: Path) -> str:
    """读取 UTF-8 文本文件。
    参数:
        path: 需要读取的文件路径。
    返回:
        返回完整文本内容。
    调用示例:
        `text = read_text(MANUAL)`
    """
    return path.read_text(encoding="utf-8")


def require_ordered_markers(text: str, markers: list[str], label: str) -> list[str]:
    """检查一组标记是否按顺序出现在文本中。
    参数:
        text: 被检查的手册文本。
        markers: 必须按顺序出现的标记列表。
        label: 错误消息中的检查类别名称。
    返回:
        返回错误描述列表；空列表表示通过。
    调用示例:
        `failures = require_ordered_markers(text, TOP_LEVEL_HEADINGS, "top-level heading")`
    """
    failures: list[str] = []
    previous_index = -1
    for marker in markers:
        index = text.find(marker)
        if index < 0:
            failures.append(f"{label} missing: {marker}")
            continue
        if index <= previous_index:
            failures.append(f"{label} out of order: {marker}")
            continue
        previous_index = index
    return failures


def require_present_markers(text: str, markers: list[str], label: str) -> list[str]:
    """检查一组标记是否都出现在文本中。
    参数:
        text: 被检查的手册文本。
        markers: 必须出现的标记列表。
        label: 错误消息中的检查类别名称。
    返回:
        返回错误描述列表；空列表表示通过。
    调用示例:
        `failures = require_present_markers(text, API_FAMILIES, "API family")`
    """
    failures: list[str] = []
    for marker in markers:
        if marker not in text:
            failures.append(f"{label} missing: {marker}")
    return failures


def check_manual_structure(text: str) -> list[str]:
    """检查手册结构、API 字段模板、移植章节和分类导航。
    参数:
        text: 完整手册文本。
    返回:
        返回发现的结构错误列表；空列表表示手册结构通过。
    调用示例:
        `failures = check_manual_structure(read_text(MANUAL))`
    """
    failures: list[str] = []
    failures.extend(require_ordered_markers(text, TOP_LEVEL_HEADINGS, "top-level heading"))
    if "### 4.1 API 分类导航" not in text:
        failures.append("manual missing API family navigation")
    failures.extend(require_present_markers(text, API_FAMILIES, "API family"))
    failures.extend(require_present_markers(text, REFERENCE_MANUAL_MARKERS, "API field"))
    failures.extend(require_present_markers(text, PORTING_MARKERS, "porting marker"))
    return failures


def main() -> int:
    """执行手册结构检查。
    参数:
        无。
    返回:
        检查通过返回 0；否则返回 1。
    调用示例:
        `raise SystemExit(main())`
    """
    if not MANUAL.exists():
        print(f"[manual-structure] missing manual: {MANUAL}")
        return 1

    failures = check_manual_structure(read_text(MANUAL))
    if failures:
        for failure in failures:
            print(f"[manual-structure] {failure}")
        print(f"[manual-structure] {len(failures)} failure(s)")
        return 1

    print("[manual-structure] reference manual structure covered")
    return 0


if __name__ == "__main__":
    sys.exit(main())
