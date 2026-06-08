#!/usr/bin/env python3
"""验证 DSP C28x 上下文切换汇编骨架的静态契约。"""

from __future__ import annotations

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
ASM_FILE = ROOT / "src" / "portable" / "dsp_c28x" / "mrt_port_dsp_c28x_context.asm"
DSP_README = ROOT / "examples" / "dsp" / "README.md"


def read_text(path: Path) -> str:
    """读取 UTF-8 文本文件并返回完整内容。"""
    return path.read_text(encoding="utf-8")


def normalize_asm(text: str) -> str:
    """规整汇编文本，便于检查关键指令和标记。"""
    return " ".join(text.lower().replace("\t", " ").split())


def test_dsp_context_assembly_declares_required_symbols() -> None:
    """DSP 汇编骨架必须暴露首任务启动、软件中断入口和 yield 入口。"""
    text = read_text(ASM_FILE)
    assert ".global MRT_PortDspC28xStartFirstTaskAsm" in text
    assert ".global MRT_PortDspC28xSoftwareInterruptHandler" in text
    assert ".global MRT_PortDspC28xYieldAsm" in text
    assert "MRT_PortDspC28xSwitchHook" in text
    assert "MRT_PortDspC28xStartFirstTaskHook" in text


def test_dsp_context_assembly_records_save_restore_markers() -> None:
    """DSP 汇编骨架必须记录关键寄存器保存、切换钩子和恢复路径。"""
    normalized = normalize_asm(read_text(ASM_FILE))
    for marker in [
        "push xar4",
        "push xar5",
        "push xar6",
        "push xar7",
        "push st0",
        "push st1",
        "push acc",
        "push p",
        "push xt",
        "lcr mrt_portdspc28xswitchhook",
        "pop xt",
        "pop p",
        "pop acc",
        "pop st1",
        "pop st0",
        "pop xar7",
        "pop xar6",
        "pop xar5",
        "pop xar4",
        "lretr",
    ]:
        assert marker in normalized


def test_dsp_readme_mentions_scaffold_boundary() -> None:
    """DSP 说明必须明确汇编骨架用途和非实板通过边界。"""
    text = read_text(DSP_README)
    assert "mrt_port_dsp_c28x_context.asm" in text
    assert "不替代真实 DSP 板级 smoke" in text


def main() -> int:
    """运行 DSP 汇编骨架静态测试。"""
    test_dsp_context_assembly_declares_required_symbols()
    test_dsp_context_assembly_records_save_restore_markers()
    test_dsp_readme_mentions_scaffold_boundary()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
