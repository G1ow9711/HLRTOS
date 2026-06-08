#!/usr/bin/env python3
"""验证 DSP C2000 板级 smoke 工程骨架的静态契约。"""

from __future__ import annotations

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
DSP_DIR = ROOT / "examples" / "dsp"
README = DSP_DIR / "README.md"
STARTUP = DSP_DIR / "startup_c28x.c"
BOARD_GLUE = DSP_DIR / "mrt_port_dsp_c2000_smoke.c"
LINKER = DSP_DIR / "linker_c28x.cmd"


def read_text(path: Path) -> str:
    """读取 UTF-8 文本文件并返回完整内容。"""
    return path.read_text(encoding="utf-8")


def normalize(text: str) -> str:
    """规整文本大小写和空白，便于检查关键符号。"""
    return " ".join(text.lower().replace("\t", " ").split())


def test_c2000_scaffold_files_exist() -> None:
    """DSP C2000 工程骨架必须包含启动、板级 glue 和链接脚本文件。"""
    for path in [STARTUP, BOARD_GLUE, LINKER]:
        assert path.exists(), f"missing {path.relative_to(ROOT)}"


def test_c2000_startup_exposes_vector_and_isr_placeholders() -> None:
    """启动文件必须暴露 timer、软件中断和外设 ISR 占位入口。"""
    text = read_text(STARTUP)
    for marker in [
        "MRT_DspC2000DefaultIsr",
        "MRT_DspC2000Startup",
        "MRT_DspC2000InstallVectors",
        "CpuTimer0Isr",
        "MRT_DspC2000SoftwareInterruptIsr",
        "MRT_DspC2000AdcIsr",
    ]:
        assert marker in text
    assert "extern void CpuTimer0Isr(void);" in text
    assert "extern void MRT_DspC2000SoftwareInterruptIsr(void);" in text
    assert "extern void MRT_DspC2000AdcIsr(void);" in text
    assert "MRT_DSP_C2000_WEAK" not in text


def test_c2000_board_glue_wires_tick_isr_and_context_switch() -> None:
    """板级 glue 必须记录 tick、FromISR 队列和软件中断切换接线。"""
    normalized = normalize(read_text(BOARD_GLUE))
    for marker in [
        "mrt_kerneltick",
        "mrt_portyieldfromisr",
        "mrt_queuesendfromisr",
        "mrt_portdspc28xinitializeStack".lower(),
        "mrt_portdspc28xsoftwareinterrupthandler",
        "mrt_portdspc28xrequestcontextswitch",
    ]:
        assert marker in normalized


def test_c2000_linker_declares_required_memory_sections() -> None:
    """链接脚本必须保留 RTOS heap、任务栈和 DMA/采样缓冲区分区。"""
    normalized = normalize(read_text(LINKER))
    for marker in [
        "memory",
        "sections",
        ".mrtos_heap",
        ".mrtos_tasks",
        ".mrtos_dma",
        ".mrtos_trace",
    ]:
        assert marker in normalized


def test_dsp_readme_mentions_c2000_scaffold_boundary() -> None:
    """DSP README 必须说明 C2000 骨架用途和非真实板级证据边界。"""
    text = read_text(README)
    for marker in [
        "startup_c28x.c",
        "mrt_port_dsp_c2000_smoke.c",
        "linker_c28x.cmd",
        "不替代真实 DSP 板级 smoke",
    ]:
        assert marker in text


def main() -> int:
    """运行 DSP C2000 工程骨架静态测试。"""
    test_c2000_scaffold_files_exist()
    test_c2000_startup_exposes_vector_and_isr_placeholders()
    test_c2000_board_glue_wires_tick_isr_and_context_switch()
    test_c2000_linker_declares_required_memory_sections()
    test_dsp_readme_mentions_c2000_scaffold_boundary()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
