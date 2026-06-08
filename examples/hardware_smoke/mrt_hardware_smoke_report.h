#ifndef MRT_HARDWARE_SMOKE_REPORT_H
#define MRT_HARDWARE_SMOKE_REPORT_H

#include <stdint.h>

#include "mrt_hardware_smoke_log.h"
#include "mrt_hardware_smoke_log_schema.h"

/*
 * 文件: mrt_hardware_smoke_report.h
 * 目的: 为真实 STM32/DSP 板级 smoke 提供完整必填字段报告输出 helper。
 * 边界: 本文件只把调用方已经判定好的结果序列化为 raw log，不判断硬件是否 PASS，不生成最终证据文件。
 */

/* 私有写字符串字段宏: 避免头文件静态扫描把函数调用误判为公有 API 原型。 */
#define SMOKE_REPORT_WRITE_PAIR(writer, field, value) MRT_SmokeLogWritePair((writer), (field), (value))

/* 私有写整数字段宏: 避免头文件静态扫描把函数调用误判为公有 API 原型。 */
#define SMOKE_REPORT_WRITE_U32(writer, field, value) MRT_SmokeLogWriteU32((writer), (field), (value))

/* 私有写通用报告宏: 避免头文件静态扫描把函数调用误判为公有 API 原型。 */
#define SMOKE_REPORT_EMIT_COMMON(writer, report) MRT_SmokeEmitCommonReport((writer), (report))

typedef struct
{
    /* 目标类型，取值通常为 STM32 或 DSP。 */
    const char *target;

    /* 总体验收状态，真实板级通过后才填写 PASS。 */
    const char *evidence_status;

    /* smoke 日期，格式为 YYYY-MM-DD。 */
    const char *smoke_date;

    /* 芯片完整型号。 */
    const char *chip;

    /* 板卡型号或硬件版本。 */
    const char *board;

    /* 编译器名称和版本。 */
    const char *compiler;

    /* 上下文切换证据摘要。 */
    const char *context_switch;

    /* 软件定时器验证结果。 */
    const char *software_timer;

    /* tickless 或低功耗补偿验证结果。 */
    const char *tickless;

    /* 连续运行分钟数。 */
    uint32_t runtime_minutes;

    /* assert hook 触发次数。 */
    uint32_t assert_failures;

    /* 运行期间最小剩余堆字节数。 */
    uint32_t heap_min_free_bytes;

    /* UART/trace 证据摘要。 */
    const char *trace_or_uart_log;

    /* 原始 UART/trace 日志路径。 */
    const char *raw_log_path;

    /* 原始 UART/trace 日志 SHA-256。 */
    const char *raw_log_sha256;
} MRT_SmokeCommonReport;

typedef struct
{
    /* 通用必填字段。 */
    MRT_SmokeCommonReport common;

    /* STM32 系统时钟频率。 */
    uint32_t clock_hz;

    /* STM32 内核 tick 频率。 */
    uint32_t tick_hz;

    /* STM32 NVIC 优先级有效位数。 */
    uint32_t nvic_priority_bits;

    /* STM32 临界区嵌套与恢复证据。 */
    const char *critical_section;

    /* STM32 SysTick 调用内核 tick 的验证结果。 */
    const char *systick;

    /* STM32 PendSV/SVC 上下文切换验证结果。 */
    const char *pendsv_svc;

    /* STM32 外设 ISR 队列唤醒验证结果。 */
    const char *isr_queue;
} MRT_SmokeStm32Report;

typedef struct
{
    /* 通用必填字段。 */
    MRT_SmokeCommonReport common;

    /* DSP 编译 ABI。 */
    const char *abi;

    /* DSP 任务栈增长方向。 */
    const char *stack_direction;

    /* DSP timer tick 验证结果。 */
    const char *timer_tick;

    /* DSP 软件中断延迟切换验证结果。 */
    const char *software_interrupt_switch;

    /* DSP ISR 嵌套与最外层退出切换验证结果。 */
    const char *isr_nesting;

    /* DSP ISR 到任务的数据通路验证结果。 */
    const char *queue_or_pool;
} MRT_SmokeDspReport;

/**
 * @brief 判断 C 字符串字段是否适合作为 smoke 报告值输出。
 * @param value 需要检查的 NUL 结尾字符串。
 * @return 非空且首字符不是 NUL 返回 1，否则返回 0。
 * @example
 * if (MRT_SmokeReportCStringIsValid(report->chip)) { ... }
 */
static inline int MRT_SmokeReportCStringIsValid(const char *value)
{
    /* 空指针不能作为必填字段输出。 */
    if (value == 0)
    {
        return 0;
    }

    /* 空字符串同样会让最终证据缺失有效内容。 */
    if (value[0] == '\0')
    {
        return 0;
    }

    /* 字符串具备最小可输出内容。 */
    return 1;
}

/**
 * @brief 校验通用 smoke 报告字段是否完整。
 * @param report 指向通用 smoke 报告的指针。
 * @return 字段完整返回 1，否则返回 0。
 * @example
 * if (MRT_SmokeCommonReportIsValid(&report.common)) { ... }
 */
static inline int MRT_SmokeCommonReportIsValid(const MRT_SmokeCommonReport *report)
{
    /* 报告结构本身必须存在。 */
    if (report == 0)
    {
        return 0;
    }

    /* 目标类型是 raw-log schema 的第一项必填字段。 */
    if (!MRT_SmokeReportCStringIsValid(report->target))
    {
        return 0;
    }

    /* 总体验收状态必须由板级测试逻辑明确填写。 */
    if (!MRT_SmokeReportCStringIsValid(report->evidence_status))
    {
        return 0;
    }

    /* smoke 日期必须可追溯。 */
    if (!MRT_SmokeReportCStringIsValid(report->smoke_date))
    {
        return 0;
    }

    /* 芯片型号缺失会让板级证据不可复现。 */
    if (!MRT_SmokeReportCStringIsValid(report->chip))
    {
        return 0;
    }

    /* 板卡型号缺失会让硬件环境不可复现。 */
    if (!MRT_SmokeReportCStringIsValid(report->board))
    {
        return 0;
    }

    /* 编译器版本缺失会让 ABI 和优化条件不可追溯。 */
    if (!MRT_SmokeReportCStringIsValid(report->compiler))
    {
        return 0;
    }

    /* 上下文切换摘要必须描述真实切换路径。 */
    if (!MRT_SmokeReportCStringIsValid(report->context_switch))
    {
        return 0;
    }

    /* 软件定时器结果必须明确输出。 */
    if (!MRT_SmokeReportCStringIsValid(report->software_timer))
    {
        return 0;
    }

    /* tickless 结果必须明确输出。 */
    if (!MRT_SmokeReportCStringIsValid(report->tickless))
    {
        return 0;
    }

    /* UART/trace 摘要必须指向可人工复核的日志片段。 */
    if (!MRT_SmokeReportCStringIsValid(report->trace_or_uart_log))
    {
        return 0;
    }

    /* 原始日志路径必须非空，最终 checker 会进一步验证文件存在。 */
    if (!MRT_SmokeReportCStringIsValid(report->raw_log_path))
    {
        return 0;
    }

    /* 原始日志哈希必须非空，最终 checker 会进一步验证 SHA-256 值。 */
    if (!MRT_SmokeReportCStringIsValid(report->raw_log_sha256))
    {
        return 0;
    }

    /* 所有字符串字段均具备最小有效内容。 */
    return 1;
}

/**
 * @brief 校验 STM32 smoke 报告字段是否完整。
 * @param report 指向 STM32 smoke 报告的指针。
 * @return 字段完整返回 1，否则返回 0。
 * @example
 * if (MRT_SmokeStm32ReportIsValid(&report)) { ... }
 */
static inline int MRT_SmokeStm32ReportIsValid(const MRT_SmokeStm32Report *report)
{
    /* STM32 报告结构本身必须存在。 */
    if (report == 0)
    {
        return 0;
    }

    /* 先复用通用字段校验，避免后续输出半份报告。 */
    if (!MRT_SmokeCommonReportIsValid(&report->common))
    {
        return 0;
    }

    /* 临界区验证结果必须明确输出。 */
    if (!MRT_SmokeReportCStringIsValid(report->critical_section))
    {
        return 0;
    }

    /* SysTick 验证结果必须明确输出。 */
    if (!MRT_SmokeReportCStringIsValid(report->systick))
    {
        return 0;
    }

    /* PendSV/SVC 验证结果必须明确输出。 */
    if (!MRT_SmokeReportCStringIsValid(report->pendsv_svc))
    {
        return 0;
    }

    /* ISR 队列验证结果必须明确输出。 */
    if (!MRT_SmokeReportCStringIsValid(report->isr_queue))
    {
        return 0;
    }

    /* STM32 字符串字段完整。 */
    return 1;
}

/**
 * @brief 校验 DSP smoke 报告字段是否完整。
 * @param report 指向 DSP smoke 报告的指针。
 * @return 字段完整返回 1，否则返回 0。
 * @example
 * if (MRT_SmokeDspReportIsValid(&report)) { ... }
 */
static inline int MRT_SmokeDspReportIsValid(const MRT_SmokeDspReport *report)
{
    /* DSP 报告结构本身必须存在。 */
    if (report == 0)
    {
        return 0;
    }

    /* 先复用通用字段校验，避免后续输出半份报告。 */
    if (!MRT_SmokeCommonReportIsValid(&report->common))
    {
        return 0;
    }

    /* ABI 字段必须明确输出。 */
    if (!MRT_SmokeReportCStringIsValid(report->abi))
    {
        return 0;
    }

    /* 栈增长方向必须明确输出。 */
    if (!MRT_SmokeReportCStringIsValid(report->stack_direction))
    {
        return 0;
    }

    /* timer tick 验证结果必须明确输出。 */
    if (!MRT_SmokeReportCStringIsValid(report->timer_tick))
    {
        return 0;
    }

    /* 软件中断切换验证结果必须明确输出。 */
    if (!MRT_SmokeReportCStringIsValid(report->software_interrupt_switch))
    {
        return 0;
    }

    /* ISR 嵌套验证结果必须明确输出。 */
    if (!MRT_SmokeReportCStringIsValid(report->isr_nesting))
    {
        return 0;
    }

    /* 队列或内存池数据通路验证结果必须明确输出。 */
    if (!MRT_SmokeReportCStringIsValid(report->queue_or_pool))
    {
        return 0;
    }

    /* DSP 字符串字段完整。 */
    return 1;
}

/**
 * @brief 输出通用 smoke 报告字段。
 * @param writer 指向日志 writer 的指针。
 * @param report 指向通用 smoke 报告的指针。
 * @return 成功返回 `MRT_SMOKE_LOG_OK`；参数非法返回 `MRT_SMOKE_LOG_INVALID_ARGUMENT`。
 * @example
 * MRT_SmokeEmitCommonReport(&writer, &report.common);
 */
static inline MRT_SmokeLogResult MRT_SmokeEmitCommonReport(const MRT_SmokeLogWriter *writer,
                                                           const MRT_SmokeCommonReport *report)
{
    /* 先完整校验 writer 和报告字段，保证失败路径不写半行。 */
    if ((!MRT_SmokeLogWriterIsValid(writer)) || (!MRT_SmokeCommonReportIsValid(report)))
    {
        return MRT_SMOKE_LOG_INVALID_ARGUMENT;
    }

    /* 输出目标类型字段。 */
    (void)SMOKE_REPORT_WRITE_PAIR(writer, MRT_SMOKE_FIELD_MYRTOS_HARDWARE_SMOKE, report->target);

    /* 输出总体验收状态字段。 */
    (void)SMOKE_REPORT_WRITE_PAIR(writer, MRT_SMOKE_FIELD_EVIDENCE_STATUS, report->evidence_status);

    /* 输出 smoke 日期字段。 */
    (void)SMOKE_REPORT_WRITE_PAIR(writer, MRT_SMOKE_FIELD_SMOKE_DATE, report->smoke_date);

    /* 输出芯片型号字段。 */
    (void)SMOKE_REPORT_WRITE_PAIR(writer, MRT_SMOKE_FIELD_CHIP, report->chip);

    /* 输出板卡型号字段。 */
    (void)SMOKE_REPORT_WRITE_PAIR(writer, MRT_SMOKE_FIELD_BOARD, report->board);

    /* 输出编译器版本字段。 */
    (void)SMOKE_REPORT_WRITE_PAIR(writer, MRT_SMOKE_FIELD_COMPILER, report->compiler);

    /* 输出上下文切换摘要字段。 */
    (void)SMOKE_REPORT_WRITE_PAIR(writer, MRT_SMOKE_FIELD_CONTEXT_SWITCH, report->context_switch);

    /* 输出软件定时器结果字段。 */
    (void)SMOKE_REPORT_WRITE_PAIR(writer, MRT_SMOKE_FIELD_SOFTWARE_TIMER, report->software_timer);

    /* 输出 tickless 结果字段。 */
    (void)SMOKE_REPORT_WRITE_PAIR(writer, MRT_SMOKE_FIELD_TICKLESS, report->tickless);

    /* 输出运行时长整数字段。 */
    (void)SMOKE_REPORT_WRITE_U32(writer, MRT_SMOKE_FIELD_RUNTIME_MINUTES, report->runtime_minutes);

    /* 输出 assert 失败次数整数字段。 */
    (void)SMOKE_REPORT_WRITE_U32(writer, MRT_SMOKE_FIELD_ASSERT_FAILURES, report->assert_failures);

    /* 输出最小剩余堆整数字段。 */
    (void)SMOKE_REPORT_WRITE_U32(writer, MRT_SMOKE_FIELD_HEAP_MIN_FREE_BYTES, report->heap_min_free_bytes);

    /* 输出 UART/trace 摘要字段。 */
    (void)SMOKE_REPORT_WRITE_PAIR(writer, MRT_SMOKE_FIELD_TRACE_OR_UART_LOG, report->trace_or_uart_log);

    /* 输出原始日志路径字段。 */
    (void)SMOKE_REPORT_WRITE_PAIR(writer, MRT_SMOKE_FIELD_RAW_LOG_PATH, report->raw_log_path);

    /* 输出原始日志哈希字段。 */
    (void)SMOKE_REPORT_WRITE_PAIR(writer, MRT_SMOKE_FIELD_RAW_LOG_SHA256, report->raw_log_sha256);

    /* 通用报告输出完成。 */
    return MRT_SMOKE_LOG_OK;
}

/**
 * @brief 输出 STM32 完整 smoke 报告字段。
 * @param writer 指向日志 writer 的指针。
 * @param report 指向 STM32 smoke 报告的指针。
 * @return 成功返回 `MRT_SMOKE_LOG_OK`；参数非法返回 `MRT_SMOKE_LOG_INVALID_ARGUMENT`。
 * @example
 * MRT_SmokeEmitStm32Report(&writer, &stm32_report);
 */
static inline MRT_SmokeLogResult MRT_SmokeEmitStm32Report(const MRT_SmokeLogWriter *writer,
                                                          const MRT_SmokeStm32Report *report)
{
    /* 先完整校验 writer 和 STM32 报告字段，保证失败路径不写半行。 */
    if ((!MRT_SmokeLogWriterIsValid(writer)) || (!MRT_SmokeStm32ReportIsValid(report)))
    {
        return MRT_SMOKE_LOG_INVALID_ARGUMENT;
    }

    /* 先输出通用字段，保持 raw-log schema 顺序。 */
    (void)SMOKE_REPORT_EMIT_COMMON(writer, &report->common);

    /* 输出 STM32 系统时钟整数字段。 */
    (void)SMOKE_REPORT_WRITE_U32(writer, MRT_SMOKE_FIELD_CLOCK_HZ, report->clock_hz);

    /* 输出 STM32 tick 频率整数字段。 */
    (void)SMOKE_REPORT_WRITE_U32(writer, MRT_SMOKE_FIELD_TICK_HZ, report->tick_hz);

    /* 输出 STM32 NVIC 优先级位数字段。 */
    (void)SMOKE_REPORT_WRITE_U32(writer, MRT_SMOKE_FIELD_NVIC_PRIORITY_BITS, report->nvic_priority_bits);

    /* 输出 STM32 临界区验证字段。 */
    (void)SMOKE_REPORT_WRITE_PAIR(writer, MRT_SMOKE_FIELD_CRITICAL_SECTION, report->critical_section);

    /* 输出 STM32 SysTick 验证字段。 */
    (void)SMOKE_REPORT_WRITE_PAIR(writer, MRT_SMOKE_FIELD_SYSTICK, report->systick);

    /* 输出 STM32 PendSV/SVC 验证字段。 */
    (void)SMOKE_REPORT_WRITE_PAIR(writer, MRT_SMOKE_FIELD_PENDSV_SVC, report->pendsv_svc);

    /* 输出 STM32 ISR 队列验证字段。 */
    (void)SMOKE_REPORT_WRITE_PAIR(writer, MRT_SMOKE_FIELD_ISR_QUEUE, report->isr_queue);

    /* STM32 完整报告输出完成。 */
    return MRT_SMOKE_LOG_OK;
}

/**
 * @brief 输出 DSP 完整 smoke 报告字段。
 * @param writer 指向日志 writer 的指针。
 * @param report 指向 DSP smoke 报告的指针。
 * @return 成功返回 `MRT_SMOKE_LOG_OK`；参数非法返回 `MRT_SMOKE_LOG_INVALID_ARGUMENT`。
 * @example
 * MRT_SmokeEmitDspReport(&writer, &dsp_report);
 */
static inline MRT_SmokeLogResult MRT_SmokeEmitDspReport(const MRT_SmokeLogWriter *writer,
                                                        const MRT_SmokeDspReport *report)
{
    /* 先完整校验 writer 和 DSP 报告字段，保证失败路径不写半行。 */
    if ((!MRT_SmokeLogWriterIsValid(writer)) || (!MRT_SmokeDspReportIsValid(report)))
    {
        return MRT_SMOKE_LOG_INVALID_ARGUMENT;
    }

    /* 先输出通用字段，保持 raw-log schema 顺序。 */
    (void)SMOKE_REPORT_EMIT_COMMON(writer, &report->common);

    /* 输出 DSP ABI 字段。 */
    (void)SMOKE_REPORT_WRITE_PAIR(writer, MRT_SMOKE_FIELD_ABI, report->abi);

    /* 输出 DSP 栈方向字段。 */
    (void)SMOKE_REPORT_WRITE_PAIR(writer, MRT_SMOKE_FIELD_STACK_DIRECTION, report->stack_direction);

    /* 输出 DSP timer tick 字段。 */
    (void)SMOKE_REPORT_WRITE_PAIR(writer, MRT_SMOKE_FIELD_TIMER_TICK, report->timer_tick);

    /* 输出 DSP 软件中断切换字段。 */
    (void)SMOKE_REPORT_WRITE_PAIR(writer,
                                  MRT_SMOKE_FIELD_SOFTWARE_INTERRUPT_SWITCH,
                                  report->software_interrupt_switch);

    /* 输出 DSP ISR 嵌套字段。 */
    (void)SMOKE_REPORT_WRITE_PAIR(writer, MRT_SMOKE_FIELD_ISR_NESTING, report->isr_nesting);

    /* 输出 DSP 队列或内存池字段。 */
    (void)SMOKE_REPORT_WRITE_PAIR(writer, MRT_SMOKE_FIELD_QUEUE_OR_POOL, report->queue_or_pool);

    /* DSP 完整报告输出完成。 */
    return MRT_SMOKE_LOG_OK;
}

#undef SMOKE_REPORT_EMIT_COMMON
#undef SMOKE_REPORT_WRITE_U32
#undef SMOKE_REPORT_WRITE_PAIR

#endif /* MRT_HARDWARE_SMOKE_REPORT_H */
