# 真实板级 Smoke 证据

本目录用于存放最终板级烟雾测试证据。

## 约定

- `stm32_board_smoke.md`：真实 STM32 板级日志
- `dsp_board_smoke.md`：真实 DSP 板级日志
- `hardware_smoke_preflight.json`：真实板级采集前置配置，记录目标芯片、板卡、编译命令、烧录命令、采集命令、原始日志路径、最终证据路径、最短运行时长和必须出现的日志字段。
- `*.template.md`：填写前的模板，不可直接当作最终证据

## 校验命令

```powershell
python tools\verify\run_release_verification.py
python tools\verify\check_hardware_smoke_preflight.py
python tools\verify\check_hardware_smoke_preflight.py --check-tools
python tools\verify\generate_hardware_smoke_evidence.py --target STM32 --input docs\verification\hardware_smoke\stm32_uart_raw.log --output docs\verification\hardware_smoke\stm32_board_smoke.md
python tools\verify\generate_hardware_smoke_evidence.py --target DSP --input docs\verification\hardware_smoke\dsp_uart_raw.log --output docs\verification\hardware_smoke\dsp_board_smoke.md
python tools\verify\check_hardware_smoke_evidence.py
```

## 采集指南

- 先阅读 `collection_checklist.md`，按 STM32 或 DSP 对应章节采集原始 UART/trace 日志。
- 采集前先运行 `check_hardware_smoke_preflight.py`，确认 `hardware_smoke_preflight.json` 不含 TODO/PENDING 等占位符，且 STM32/DSP 都写明芯片、板卡、命令、运行时长和必需字段。`--check-tools` 会额外检查当前机器是否能找到配置里的可执行文件；没有真实工具链或烧录器时不要把该失败解释成板级 smoke 失败。
- 若原始日志已经包含 `Key: Value` 字段，可先运行 `generate_hardware_smoke_evidence.py` 生成最终证据文件。
- 若不使用生成器，再把模板复制成最终文件名并填写真实结果。
- 最后运行校验命令。校验失败时，应回到原始日志补证，不要把模板字段改成 `PASS` 规避检查。

## 填写要求

- 日志必须来自真实板卡运行，不接受仅 host model 或交叉编译结果。
- `Evidence-Status` 必须为 `PASS`。
- `Runtime-Minutes` 至少 30。
- `Assert-Failures` 必须为 0。
- `Heap-Min-Free-Bytes` 必须大于 0。
- 记录编译器版本、芯片型号、板卡型号、时钟、tick 频率、上下文切换证据和 UART/trace 日志摘要。
