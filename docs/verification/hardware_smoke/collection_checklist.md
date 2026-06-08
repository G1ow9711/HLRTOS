# 真实板级 Smoke 采集清单

本清单用于把 STM32/DSP 实机运行结果转成可校验的 `stm32_board_smoke.md` 与 `dsp_board_smoke.md`。它只规定采集流程，不替代真实板卡运行。

## 通用流程

0. 预检配置：先按当前板卡修改 `hardware_smoke_preflight.json`，运行 `python tools\verify\check_hardware_smoke_preflight.py --target STM32` 或 `python tools\verify\check_hardware_smoke_preflight.py --target DSP`。单目标模式只检查被选目标，适合先调通一块板；不带 `--target` 时默认检查 STM32/DSP 两个节点是否齐全、是否还有 TODO/PENDING 占位符、最短运行时长是否至少 30 分钟、预期日志字段是否覆盖必需项。需要确认工具链、烧录器和采集工具在当前机器可执行时，再追加 `--check-tools`。
0. 字段对齐：先阅读 `raw_log_schema.md`，确认 UART/trace 将输出完整 `Key: Value` 字段；修改字段清单后运行 `python tools\verify\check_hardware_smoke_raw_log_schema.py`，确保生成器、文档和示例头文件一致。
0. 流水线预演：运行 `python tools\verify\run_hardware_smoke_capture.py --target STM32` 或 `--target DSP`，先只看 dry-run 顺序。确认 `capture_command` 会写入 `raw_log` 后，再运行同一命令并追加 `--execute`；不要在未连接真实板卡时执行烧录命令。执行器会在采集后先调用 `check_hardware_smoke_raw_log.py` 检查原始日志，再进入证据生成和最终证据校验。
1. 保留原始日志：UART、trace、调试器输出或仿真器控制台日志建议另存为 `stm32_uart_raw.log`、`dsp_uart_raw.log` 或等价文件名。
2. 运行负载：至少 30 分钟，期间必须覆盖任务切换、ISR 唤醒、软件定时器、tickless、heap 查询和断言 hook。
3. 填写摘要：把原始日志中的关键结果归纳到 `Key: Value` 字段，不要把模板中的 `PENDING`、`FAIL`、`TODO` 留在最终证据文件中。
4. 原始日志直检：若手动采集或整理原始日志，先运行 `python tools\verify\check_hardware_smoke_raw_log.py --target STM32 --input docs\verification\hardware_smoke\stm32_uart_raw.log` 或 `python tools\verify\check_hardware_smoke_raw_log.py --target DSP --input docs\verification\hardware_smoke\dsp_uart_raw.log`，确认字段、PASS 状态、运行时长、断言次数和 heap 最小剩余量满足规则。
5. 自动生成：若原始日志已经包含完整字段，运行 `python tools\verify\generate_hardware_smoke_evidence.py --target STM32 --input docs\verification\hardware_smoke\stm32_uart_raw.log --output docs\verification\hardware_smoke\stm32_board_smoke.md` 或 `python tools\verify\generate_hardware_smoke_evidence.py --target DSP --input docs\verification\hardware_smoke\dsp_uart_raw.log --output docs\verification\hardware_smoke\dsp_board_smoke.md`。生成器会自动写入 `Raw-Log-Path` 和 `Raw-Log-SHA256`。
6. 手动填写：若不使用生成器，再从 `stm32_board_smoke.template.md` 复制为 `stm32_board_smoke.md`，或从 `dsp_board_smoke.template.md` 复制为 `dsp_board_smoke.md`，并填入真实板级结果。
7. 执行校验：运行 `python tools\verify\check_hardware_smoke_evidence.py`。只有该命令通过，真实板级 smoke 才能作为最终验收证据。

## STM32 采集步骤

1. 构建镜像：使用目标工程或 `examples/stm32/` 派生工程，记录完整 `arm-none-eabi-gcc`、CMake、Keil 或 IAR 构建命令，以及 ELF/map 文件路径。
2. 烧录启动：记录芯片型号、板卡型号、系统时钟、tick 频率、NVIC 优先级位宽和 FLASH/RAM 布局。
3. tick 证据：UART/trace 中记录 1 秒内 `MRT_KernelGetTick()` 的增量，确认与 `MRT_CFG_TICK_RATE_HZ` 一致。
4. 切换证据：触发 `SysTick_Handler`、`PendSV_Handler`、`SVC_Handler` 和一个使用 `MRT_QueueSendFromISR()` 的外设 ISR，确认高优先级任务在 ISR 退出后运行。
5. 临界区证据：记录 PRIMASK/BASEPRI 进入前、嵌套期间、退出后的状态，确认恢复到进入前状态。
6. 定时器证据：运行一个 one-shot 软件定时器和一个 auto-reload 软件定时器，确认回调由 `MRT_TimerServiceRunPending()` 或真实服务任务路径执行。
7. tickless 证据：进入低功耗路径后唤醒，确认任务延时、软件定时器到期顺序和补偿 tick 没有乱序。
8. 内存证据：周期性记录 `MRT_HeapGetFreeSize()` 或等价统计，填写 30 分钟内最小剩余字节。
9. 断言证据：注册 assert hook，记录 `Assert-Failures`。最终验收必须为 0。
10. 填写 `stm32_board_smoke.md`，并运行硬件证据校验命令。

## DSP 采集步骤

1. 构建镜像：使用目标 DSP 工具链，记录编译器版本、ABI 模式、链接脚本和镜像/map 文件路径。
2. 启动记录：记录 DSP 型号、板卡型号、栈方向、任务栈地址范围、heap 区域、DMA/采样缓冲区位置。
3. tick 证据：由 CPU timer、ePWM timer 或片上通用 timer 调用 `MRT_KernelTick()`，记录 30 分钟内 tick 计数与预期频率。
4. 软件中断证据：记录 `MRT_PortDspC28xRequestContextSwitch()` 请求、确认和清除过程，确认切换在最外层 ISR 退出后发生。
5. ISR 嵌套证据：记录嵌套深度峰值，确认进入/退出计数不下溢，内层 ISR 退出不直接切换任务。
6. 数据通路证据：让 ADC/DMA 或等价 ISR 通过队列、事件组或固定块内存池唤醒处理任务，记录队列峰值或内存池空闲块最小值。
7. 定时器证据：运行 one-shot 和 auto-reload 软件定时器，确认回调不在硬件 timer ISR 中长时间执行。
8. tickless 证据：若目标 DSP 支持低功耗，记录睡眠前后 tick 补偿；若芯片不支持对应模式，必须在端口说明里写明替代策略。
9. 内存与断言证据：记录 heap 最小剩余字节和 assert hook 触发次数。最终验收要求 assert 次数为 0。
10. 填写 `dsp_board_smoke.md`，并运行硬件证据校验命令。

## 字段填写规则

- `Evidence-Status`、`Software-Timer`、`Tickless`、STM32 的 `SysTick`、`PendSV-SVC`、`ISR-Queue`，以及 DSP 的 `Timer-Tick`、`Software-Interrupt-Switch`、`ISR-Nesting`、`Queue-Or-Pool` 只能在真实结果满足条件时填写 `PASS`。
- `Runtime-Minutes`、`Assert-Failures`、`Heap-Min-Free-Bytes`、`Clock-Hz`、`Tick-Hz`、`NVIC-Priority-Bits` 必须只写十进制整数，不带单位。
- `Trace-Or-UART-Log` 应写可追溯摘要，例如日志文件名、关键 tick 计数、队列峰值、定时器回调次数和 heap 最小值。
- `Raw-Log-Path` 必须指向保留的原始 UART/trace 日志；可写仓库相对路径或绝对路径。
- `Raw-Log-SHA256` 必须写 64 个十六进制字符，且与 `Raw-Log-Path` 指向文件的实际 SHA-256 一致。
- 不要提交仍含 `PENDING`、`FAIL` 或 `TODO` 的最终证据文件。
