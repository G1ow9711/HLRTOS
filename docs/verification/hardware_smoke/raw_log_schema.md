# Hardware Smoke Raw Log Schema

本文档规定真实 STM32/DSP 板级 smoke 原始日志的 `Key: Value` 字段格式。该 schema 用于：

- 让 UART/trace 原始日志能被 `tools/verify/generate_hardware_smoke_evidence.py` 转换成最终证据文件。
- 让最终证据文件能被 `tools/verify/check_hardware_smoke_evidence.py` 校验。
- 让 STM32/DSP 示例工程在输出日志时使用统一字段名。

## 基本规则

- 原始日志必须来自真实板卡运行，不接受 host model、交叉编译日志或人工伪造结果。
- 每行采用 `Key: Value`；字段名大小写和连字符必须与本文档完全一致。
- 最终证据不得保留 `PENDING`、`FAIL`、`TODO`、`TBD`、`PLACEHOLDER` 或空值。
- `Raw-Log-Path` 必须指向保留的原始 UART/trace 日志。
- `Raw-Log-SHA256` 必须等于 `Raw-Log-Path` 指向文件的实际 SHA-256。
- `Runtime-Minutes` 必须大于等于 30。
- `Assert-Failures` 必须为 0。
- `Heap-Min-Free-Bytes` 必须大于 0。

## 通用必填字段

| 字段 | 说明 |
| --- | --- |
| `MyRTOS-Hardware-Smoke` | 目标类型，只能为 `STM32` 或 `DSP`。 |
| `Evidence-Status` | 总体验收状态，最终验收必须为 `PASS`。 |
| `Smoke-Date` | 真实板级 smoke 日期，格式为 `YYYY-MM-DD`。 |
| `Chip` | 芯片完整型号。 |
| `Board` | 板卡型号或自研硬件版本。 |
| `Compiler` | 编译器名称和版本。 |
| `Context-Switch` | 上下文切换证据摘要。 |
| `Software-Timer` | 软件定时器结果，最终验收必须为 `PASS`。 |
| `Tickless` | tickless 或低功耗补偿结果，最终验收必须为 `PASS`。 |
| `Runtime-Minutes` | 连续运行分钟数，十进制整数。 |
| `Assert-Failures` | assert hook 触发次数，十进制整数。 |
| `Heap-Min-Free-Bytes` | 运行期间最小剩余堆字节数，十进制整数。 |
| `Trace-Or-UART-Log` | UART/trace 证据摘要。 |
| `Raw-Log-Path` | 原始 UART/trace 日志路径。 |
| `Raw-Log-SHA256` | 原始日志 SHA-256。 |

## STM32 必填字段

| 字段 | 说明 |
| --- | --- |
| `Clock-Hz` | 系统时钟频率，十进制整数。 |
| `Tick-Hz` | 内核 tick 频率，十进制整数。 |
| `NVIC-Priority-Bits` | NVIC 优先级有效位数，十进制整数。 |
| `Critical-Section` | PRIMASK/BASEPRI 临界区嵌套恢复证据。 |
| `SysTick` | SysTick 调用 `MRT_KernelTick()` 的验证结果，最终验收必须为 `PASS`。 |
| `PendSV-SVC` | PendSV/SVC 上下文切换验证结果，最终验收必须为 `PASS`。 |
| `ISR-Queue` | 外设 ISR 使用 FromISR 队列唤醒任务的结果，最终验收必须为 `PASS`。 |

## DSP 必填字段

| 字段 | 说明 |
| --- | --- |
| `ABI` | 目标 DSP 编译 ABI。 |
| `Stack-Direction` | 任务栈增长方向。 |
| `Timer-Tick` | 硬件 timer 调用 `MRT_KernelTick()` 的验证结果，最终验收必须为 `PASS`。 |
| `Software-Interrupt-Switch` | 软件中断延迟切换验证结果，最终验收必须为 `PASS`。 |
| `ISR-Nesting` | ISR 嵌套计数和最外层退出切换验证结果，最终验收必须为 `PASS`。 |
| `Queue-Or-Pool` | ISR 到任务的数据通路结果，最终验收必须为 `PASS`。 |

## STM32 原始日志示例

```text
MyRTOS-Hardware-Smoke: STM32
Evidence-Status: PASS
Smoke-Date: 2026-06-09
Chip: STM32F407VG
Board: STM32F4DISCOVERY
Compiler: arm-none-eabi-gcc 14.2.Rel1
Context-Switch: SysTick requested PendSV; PSP saved/restored for LED/UART tasks
Software-Timer: PASS
Tickless: PASS
Runtime-Minutes: 30
Assert-Failures: 0
Heap-Min-Free-Bytes: 4096
Trace-Or-UART-Log: stm32_uart_raw.log; tick delta 30000; queue peak 1
Raw-Log-Path: docs/verification/hardware_smoke/stm32_uart_raw.log
Raw-Log-SHA256: <64 lowercase hex characters>
Clock-Hz: 168000000
Tick-Hz: 1000
NVIC-Priority-Bits: 4
Critical-Section: BASEPRI nested enter/exit restored
SysTick: PASS
PendSV-SVC: PASS
ISR-Queue: PASS
```

## DSP 原始日志示例

```text
MyRTOS-Hardware-Smoke: DSP
Evidence-Status: PASS
Smoke-Date: 2026-06-09
Chip: TMS320F28379D
Board: LAUNCHXL-F28379D
Compiler: TI C2000 compiler with EABI
Context-Switch: CPU Timer0 requested software interrupt; task stack top saved/restored
Software-Timer: PASS
Tickless: PASS
Runtime-Minutes: 30
Assert-Failures: 0
Heap-Min-Free-Bytes: 4096
Trace-Or-UART-Log: dsp_uart_raw.log; timer tick 30000; pool min free 2
Raw-Log-Path: docs/verification/hardware_smoke/dsp_uart_raw.log
Raw-Log-SHA256: <64 lowercase hex characters>
ABI: eabi
Stack-Direction: down
Timer-Tick: PASS
Software-Interrupt-Switch: PASS
ISR-Nesting: PASS
Queue-Or-Pool: PASS
```

## 对齐校验

修改字段清单后必须运行：

```powershell
python tools\verify\check_hardware_smoke_raw_log_schema.py
python tests\static\test_hardware_smoke_raw_log_schema.py
python tools\verify\run_release_verification.py
```

