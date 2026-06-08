#!/usr/bin/env python3
"""验证 STM32 Cortex-M 上下文切换汇编骨架的静态契约。"""

from __future__ import annotations

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
ASM_FILE = ROOT / "src" / "portable" / "stm32_cm" / "mrt_port_stm32_cm_context.S"
SMOKE_CHECKER = ROOT / "tools" / "verify" / "check_embedded_smoke_projects.py"
STM32_PORT = ROOT / "examples" / "stm32" / "mrt_port_stm32_smoke.c"
STM32_MAIN = ROOT / "examples" / "stm32" / "main.c"
STARTUP = ROOT / "examples" / "stm32" / "startup_stm32cm.c"


def read_text(path: Path) -> str:
    """读取 UTF-8 文本文件并返回完整内容。"""
    return path.read_text(encoding="utf-8")


def normalize_asm(text: str) -> str:
    """规整汇编文本，便于检查关键指令是否存在。"""
    return " ".join(text.lower().replace("\t", " ").split())


def test_context_assembly_declares_required_symbols() -> None:
    """汇编骨架必须暴露 SVC、PendSV 和首任务启动入口。"""
    text = read_text(ASM_FILE)
    assert ".global SVC_Handler" in text
    assert ".global PendSV_Handler" in text
    assert ".global MRT_PortStm32CmStartFirstTaskAsm" in text
    assert "MRT_PortStm32CmSvcHook" in text
    assert "MRT_PortStm32CmPendSvHook" in text


def test_context_assembly_records_register_save_restore_markers() -> None:
    """PendSV 骨架必须包含 PSP、R4-R11 保存恢复和异常返回路径。"""
    normalized = normalize_asm(read_text(ASM_FILE))
    assert "mrs r0, psp" in normalized
    assert "stmdb r0!, {r4-r11}" in normalized
    assert "bl mrt_portstm32cmpendsvhook" in normalized
    assert "ldmia r0!, {r4-r11}" in normalized
    assert "msr psp, r0" in normalized
    assert "bx lr" in normalized


def test_context_scaffold_is_wired_into_stm32_smoke_build() -> None:
    """STM32 smoke 交叉编译必须纳入汇编骨架并提供 C 钩子。"""
    checker_text = read_text(SMOKE_CHECKER)
    port_text = read_text(STM32_PORT)
    main_text = read_text(STM32_MAIN)
    startup_text = read_text(STARTUP)

    assert "mrt_port_stm32_cm_context.S" in checker_text
    assert "-Isrc/kernel" in checker_text
    assert "MRT_PortStm32CmSvcHook" in port_text
    assert "MRT_PortStm32CmPendSvHook" in port_text
    assert "MRT_TaskKernelSwitchStackTop" in port_text
    assert "MRT_PortStm32CmInitializeStack" in main_text
    assert "MRT_TaskKernelSetStackTop" in main_text
    assert "__attribute__((weak)) void SVC_Handler" in startup_text
    assert "__attribute__((weak)) void PendSV_Handler" in startup_text


def main() -> int:
    """运行静态测试。"""
    test_context_assembly_declares_required_symbols()
    test_context_assembly_records_register_save_restore_markers()
    test_context_scaffold_is_wired_into_stm32_smoke_build()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
