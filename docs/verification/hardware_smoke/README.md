# 真实板级 Smoke 证据

本目录用于存放最终板级烟雾测试证据。

## 约定

- `stm32_board_smoke.md`：真实 STM32 板级日志
- `dsp_board_smoke.md`：真实 DSP 板级日志
- `*.template.md`：填写前的模板，不可直接当作最终证据

## 校验命令

```powershell
python tools\verify\check_hardware_smoke_evidence.py
```

## 填写要求

- 日志必须来自真实板卡运行，不接受仅 host model 或交叉编译结果。
- `Evidence-Status` 必须为 `PASS`。
- `Runtime-Minutes` 至少 30。
- `Assert-Failures` 必须为 0。
- `Heap-Min-Free-Bytes` 必须大于 0。
- 记录编译器版本、芯片型号、板卡型号、时钟、tick 频率、上下文切换证据和 UART/trace 日志摘要。

