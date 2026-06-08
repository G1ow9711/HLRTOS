#!/usr/bin/env python3
"""检查 MyRTOS 源码和手册的符号原创性与版权头卫生。"""

from __future__ import annotations

import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SCAN_DIRS = [ROOT / "include", ROOT / "src", ROOT / "examples", ROOT / "tests", ROOT / "docs" / "manual"]
BANNED_PATTERNS = [
    r"\bxTask[A-Za-z0-9_]*\b",
    r"\bvTask[A-Za-z0-9_]*\b",
    r"\bxQueue[A-Za-z0-9_]*\b",
    r"\bvQueue[A-Za-z0-9_]*\b",
    r"\bport[A-Z][A-Za-z0-9_]*\b",
    r"FreeRTOS-Kernel",
    r"Copyright\s*\(C\)\s*Amazon",
    r"tskKERNEL_VERSION",
]


def iter_text_files() -> list[Path]:
    """列出需要扫描的文本文件。"""
    files: list[Path] = []
    for directory in SCAN_DIRS:
        if not directory.exists():
            continue
        for path in sorted(directory.rglob("*")):
            if path.suffix in {".c", ".h", ".md"}:
                files.append(path)
    return files


def main() -> int:
    """执行原创符号扫描。"""
    failures: list[str] = []
    compiled = [re.compile(pattern) for pattern in BANNED_PATTERNS]
    for path in iter_text_files():
        text = path.read_text(encoding="utf-8")
        for pattern, regex in zip(BANNED_PATTERNS, compiled):
            if regex.search(text):
                failures.append(f"{path.relative_to(ROOT)}: matched banned pattern {pattern}")

    if failures:
        for failure in failures:
            print(f"[original-symbols] {failure}")
        print(f"[original-symbols] {len(failures)} failure(s)")
        return 1

    print("[original-symbols] no banned FreeRTOS-style public symbols found")
    return 0


if __name__ == "__main__":
    sys.exit(main())
