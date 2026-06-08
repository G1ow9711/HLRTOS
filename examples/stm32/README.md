# MyRTOS STM32 Smoke Example

## 目的
这个目录提供一个面向 STM32 Cortex-M 的最小 smoke 工程，用来验证：

- MyRTOS 内核和 STM32 端口 helper 可以被 ARM GCC 正常交叉编译。
- SysTick、PendSV、SVC、队列 ISR、软件定时器和任务创建的接线方式清晰。
- 真实板级工程后续只需要把这里的烟雾测试骨架替换成具体 HAL/CMSIS 接口。

## 目录说明

- `main.c`：烟雾测试入口，组装任务、队列、定时器和 tick 回调，并把 STM32 初始任务栈帧写回 TCB `stack_top`。
- `mrt_port_stm32_smoke.c`：STM32 端口 smoke 实现，提供公共 `MRT_Port*` 接口。
- `src/portable/stm32_cm/mrt_port_stm32_cm_context.S`：Cortex-M SVC/PendSV 汇编入口骨架，提供 `SVC_Handler`、`PendSV_Handler` 和首任务启动入口符号。
- `startup_stm32cm.c`：最小向量表和 Reset_Handler。
- `runtime_stubs.c`：freestanding 链接所需的基础 C 运行时桩。
- `linker.ld`：最小链接脚本，保留 FLASH、RAM、向量表、data/bss。

## 构建方式

当前仓库使用 `tools/verify/check_embedded_smoke_projects.py` 作为统一验证入口。脚本会：

1. 检查 `examples/stm32/` 是否齐全。
2. 使用 `arm-none-eabi-gcc` 编译并链接 STM32 smoke ELF。
3. 在 host 上编译并运行 DSP 模型 smoke。

## STM32 迁移步骤

1. 先保留本目录的 `main.c`、`startup_stm32cm.c` 和 `linker.ld`，确认工程能交叉编译。
2. 把 `mrt_port_stm32_smoke.c` 中的临界区、低功耗和寄存器访问桩替换为真实 CMSIS/HAL 实现；当前 `main.c` 已调用 `MRT_PortStm32CmInitializeStack()` 与 `MRT_TaskKernelSetStackTop()` 写入初始 PSP，`MRT_PortStm32CmPendSvHook()` 已调用内核 TCB 栈顶契约保存旧 PSP 并返回当前任务 PSP，移植时需要用真实板级日志验证该链路。
3. 在 `SysTick_Handler` 中调用 `MRT_KernelTick()`。
4. 在 `PendSV_Handler` 中验证真实上下文切换汇编，至少保存/恢复 PSP、R4-R11、LR/EXC_RETURN 和目标芯片需要的软件保存寄存器；带 FPU 的芯片还要明确浮点上下文保存策略。
5. 在 `SVC_Handler` 中接入首任务启动路径。
6. 用 `USARTx_IRQHandler`、`TIMx_IRQHandler`、`DMAx_IRQHandler` 等真实外设 ISR 复用 `FromISR` API。
7. 将 `MRT_TicklessEnterIdle()` 和 `MRT_PortSuppressTicksAndSleep()` 接到低功耗路径。

## smoke 关注点

- 任务栈必须 8 字节对齐。
- 队列 ISR 只能做短时间复制和唤醒。
- 软件定时器控制命令必须先进入服务队列，再由服务路径执行。
- 真实板级 UART/trace 原始日志必须按 `docs/verification/hardware_smoke/raw_log_schema.md` 输出字段；可复用 `examples/hardware_smoke/mrt_hardware_smoke_log_schema.h` 中的字段常量，可用 `examples/hardware_smoke/mrt_hardware_smoke_log.h` 把 UART/SWO 单字符发送函数封装成 `Key: Value` 行输出，也可用 `examples/hardware_smoke/mrt_hardware_smoke_report.h` 的 `MRT_SmokeEmitStm32Report()` 输出 STM32 必填字段全集。
- 当前示例只证明接线和编译链路，不替代真实板级跑测。
