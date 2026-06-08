#ifndef MRT_HARDWARE_SMOKE_LOG_SCHEMA_H
#define MRT_HARDWARE_SMOKE_LOG_SCHEMA_H

/*
 * 文件: mrt_hardware_smoke_log_schema.h
 * 目的: 为真实 STM32/DSP 板级 smoke UART/trace 输出提供统一字段名。
 * 用法: 板级示例可直接打印这些字符串，保证原始日志能被证据生成器解析。
 * 说明: 本文件只定义字段常量，不包含运行时代码，也不伪造任何 PASS 结果。
 */

/* 通用字段: 目标类型，取值为 STM32 或 DSP。 */
#define MRT_SMOKE_FIELD_MYRTOS_HARDWARE_SMOKE "MyRTOS-Hardware-Smoke"
/* 通用字段: 最终证据状态，真实验收通过后填写 PASS。 */
#define MRT_SMOKE_FIELD_EVIDENCE_STATUS "Evidence-Status"
/* 通用字段: smoke 日期，格式为 YYYY-MM-DD。 */
#define MRT_SMOKE_FIELD_SMOKE_DATE "Smoke-Date"
/* 通用字段: 芯片完整型号。 */
#define MRT_SMOKE_FIELD_CHIP "Chip"
/* 通用字段: 板卡型号或自研硬件版本。 */
#define MRT_SMOKE_FIELD_BOARD "Board"
/* 通用字段: 编译器名称和版本。 */
#define MRT_SMOKE_FIELD_COMPILER "Compiler"
/* 通用字段: 上下文切换证据摘要。 */
#define MRT_SMOKE_FIELD_CONTEXT_SWITCH "Context-Switch"
/* 通用字段: 软件定时器验证结果。 */
#define MRT_SMOKE_FIELD_SOFTWARE_TIMER "Software-Timer"
/* 通用字段: tickless 或低功耗补偿验证结果。 */
#define MRT_SMOKE_FIELD_TICKLESS "Tickless"
/* 通用字段: 连续运行分钟数。 */
#define MRT_SMOKE_FIELD_RUNTIME_MINUTES "Runtime-Minutes"
/* 通用字段: assert hook 触发次数。 */
#define MRT_SMOKE_FIELD_ASSERT_FAILURES "Assert-Failures"
/* 通用字段: 运行期间最小剩余堆字节数。 */
#define MRT_SMOKE_FIELD_HEAP_MIN_FREE_BYTES "Heap-Min-Free-Bytes"
/* 通用字段: UART/trace 证据摘要。 */
#define MRT_SMOKE_FIELD_TRACE_OR_UART_LOG "Trace-Or-UART-Log"
/* 通用字段: 原始 UART/trace 日志路径。 */
#define MRT_SMOKE_FIELD_RAW_LOG_PATH "Raw-Log-Path"
/* 通用字段: 原始日志 SHA-256。 */
#define MRT_SMOKE_FIELD_RAW_LOG_SHA256 "Raw-Log-SHA256"

/* STM32 字段: 系统时钟频率。 */
#define MRT_SMOKE_FIELD_CLOCK_HZ "Clock-Hz"
/* STM32 字段: 内核 tick 频率。 */
#define MRT_SMOKE_FIELD_TICK_HZ "Tick-Hz"
/* STM32 字段: NVIC 优先级有效位数。 */
#define MRT_SMOKE_FIELD_NVIC_PRIORITY_BITS "NVIC-Priority-Bits"
/* STM32 字段: 临界区进入、嵌套和恢复证据。 */
#define MRT_SMOKE_FIELD_CRITICAL_SECTION "Critical-Section"
/* STM32 字段: SysTick 调用内核 tick 的验证结果。 */
#define MRT_SMOKE_FIELD_SYSTICK "SysTick"
/* STM32 字段: PendSV/SVC 上下文切换验证结果。 */
#define MRT_SMOKE_FIELD_PENDSV_SVC "PendSV-SVC"
/* STM32 字段: 外设 ISR 队列唤醒任务验证结果。 */
#define MRT_SMOKE_FIELD_ISR_QUEUE "ISR-Queue"

/* DSP 字段: 目标编译 ABI。 */
#define MRT_SMOKE_FIELD_ABI "ABI"
/* DSP 字段: 任务栈增长方向。 */
#define MRT_SMOKE_FIELD_STACK_DIRECTION "Stack-Direction"
/* DSP 字段: timer tick 验证结果。 */
#define MRT_SMOKE_FIELD_TIMER_TICK "Timer-Tick"
/* DSP 字段: 软件中断延迟切换验证结果。 */
#define MRT_SMOKE_FIELD_SOFTWARE_INTERRUPT_SWITCH "Software-Interrupt-Switch"
/* DSP 字段: ISR 嵌套计数验证结果。 */
#define MRT_SMOKE_FIELD_ISR_NESTING "ISR-Nesting"
/* DSP 字段: ISR 到任务的队列或内存池数据通路验证结果。 */
#define MRT_SMOKE_FIELD_QUEUE_OR_POOL "Queue-Or-Pool"

/* 通用必填字段列表: 以 X-macro 方式复用，避免头文件产生未使用变量。 */
#define MRT_SMOKE_FOR_EACH_COMMON_FIELD(MRT_SMOKE_X) \
    MRT_SMOKE_X(MRT_SMOKE_FIELD_MYRTOS_HARDWARE_SMOKE) \
    MRT_SMOKE_X(MRT_SMOKE_FIELD_EVIDENCE_STATUS) \
    MRT_SMOKE_X(MRT_SMOKE_FIELD_SMOKE_DATE) \
    MRT_SMOKE_X(MRT_SMOKE_FIELD_CHIP) \
    MRT_SMOKE_X(MRT_SMOKE_FIELD_BOARD) \
    MRT_SMOKE_X(MRT_SMOKE_FIELD_COMPILER) \
    MRT_SMOKE_X(MRT_SMOKE_FIELD_CONTEXT_SWITCH) \
    MRT_SMOKE_X(MRT_SMOKE_FIELD_SOFTWARE_TIMER) \
    MRT_SMOKE_X(MRT_SMOKE_FIELD_TICKLESS) \
    MRT_SMOKE_X(MRT_SMOKE_FIELD_RUNTIME_MINUTES) \
    MRT_SMOKE_X(MRT_SMOKE_FIELD_ASSERT_FAILURES) \
    MRT_SMOKE_X(MRT_SMOKE_FIELD_HEAP_MIN_FREE_BYTES) \
    MRT_SMOKE_X(MRT_SMOKE_FIELD_TRACE_OR_UART_LOG) \
    MRT_SMOKE_X(MRT_SMOKE_FIELD_RAW_LOG_PATH) \
    MRT_SMOKE_X(MRT_SMOKE_FIELD_RAW_LOG_SHA256)

/* STM32 专属必填字段列表: 供 STM32 板级 smoke 输出端复用。 */
#define MRT_SMOKE_FOR_EACH_STM32_FIELD(MRT_SMOKE_X) \
    MRT_SMOKE_X(MRT_SMOKE_FIELD_CLOCK_HZ) \
    MRT_SMOKE_X(MRT_SMOKE_FIELD_TICK_HZ) \
    MRT_SMOKE_X(MRT_SMOKE_FIELD_NVIC_PRIORITY_BITS) \
    MRT_SMOKE_X(MRT_SMOKE_FIELD_CRITICAL_SECTION) \
    MRT_SMOKE_X(MRT_SMOKE_FIELD_SYSTICK) \
    MRT_SMOKE_X(MRT_SMOKE_FIELD_PENDSV_SVC) \
    MRT_SMOKE_X(MRT_SMOKE_FIELD_ISR_QUEUE)

/* DSP 专属必填字段列表: 供 DSP 板级 smoke 输出端复用。 */
#define MRT_SMOKE_FOR_EACH_DSP_FIELD(MRT_SMOKE_X) \
    MRT_SMOKE_X(MRT_SMOKE_FIELD_ABI) \
    MRT_SMOKE_X(MRT_SMOKE_FIELD_STACK_DIRECTION) \
    MRT_SMOKE_X(MRT_SMOKE_FIELD_TIMER_TICK) \
    MRT_SMOKE_X(MRT_SMOKE_FIELD_SOFTWARE_INTERRUPT_SWITCH) \
    MRT_SMOKE_X(MRT_SMOKE_FIELD_ISR_NESTING) \
    MRT_SMOKE_X(MRT_SMOKE_FIELD_QUEUE_OR_POOL)

#endif /* MRT_HARDWARE_SMOKE_LOG_SCHEMA_H */
