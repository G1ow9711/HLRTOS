#include "mrt_test.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "../../examples/hardware_smoke/mrt_hardware_smoke_report.h"

typedef struct
{
    /* 捕获完整 smoke 报告输出，容量覆盖 STM32/DSP 必填字段全集。 */
    char bytes[2048u];

    /* 当前已经捕获的字节数。 */
    size_t length;
} SmokeReportCapture;

/**
 * @brief 把报告 emitter 输出的单个字符写入测试缓冲。
 * @param context 指向 `SmokeReportCapture` 的测试上下文。
 * @param ch 需要写入的字符。
 * @return void 无返回值；缓冲满时丢弃后续字符并保持 NUL 结尾。
 * @example
 * CaptureReportChar(&capture, 'A');
 */
static void CaptureReportChar(void *context, char ch)
{
    /* 取出调用方传入的测试捕获缓冲。 */
    SmokeReportCapture *capture = (SmokeReportCapture *)context;

    /* 预留 NUL 结尾，避免后续 strstr/strcmp 访问越界。 */
    if (capture->length + 1u >= sizeof(capture->bytes))
    {
        return;
    }

    /* 写入当前字符并推进捕获长度。 */
    capture->bytes[capture->length] = ch;
    capture->length++;

    /* 每次写入后都保持 C 字符串形式。 */
    capture->bytes[capture->length] = '\0';
}

/**
 * @brief 构造通用 smoke 报告字段。
 * @param target 目标类型字符串，必须是 `STM32` 或 `DSP`。
 * @return MRT_SmokeCommonReport 已填充的通用报告结构。
 * @example
 * MRT_SmokeCommonReport common = MakeCommonReport("STM32");
 */
static MRT_SmokeCommonReport MakeCommonReport(const char *target)
{
    /* 使用真实 raw-log schema 中的通用字段顺序填充报告。 */
    MRT_SmokeCommonReport report = {
        target,
        "PASS",
        "2026-06-09",
        "CHIP-UNDER-TEST",
        "BOARD-UNDER-TEST",
        "compiler version",
        "context switch observed",
        "PASS",
        "PASS",
        30u,
        0u,
        4096u,
        "trace summary",
        "docs/verification/hardware_smoke/raw.log",
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"};

    /* 返回按测试目标填充的通用报告。 */
    return report;
}

/**
 * @brief 断言输出中包含指定字段行。
 * @param text 完整输出文本。
 * @param expected_line 期望出现的 `Key: Value\n` 行。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * AssertContainsLine(output, "Evidence-Status: PASS\n");
 */
static void AssertContainsLine(const char *text, const char *expected_line)
{
    /* 输出文本必须非空，避免 strstr 空指针访问。 */
    MRT_TEST_ASSERT_TRUE(text != 0);

    /* 期望行必须非空，避免测试本身失去约束。 */
    MRT_TEST_ASSERT_TRUE(expected_line != 0);

    /* 完整输出中必须能找到该字段行。 */
    MRT_TEST_ASSERT_TRUE(strstr(text, expected_line) != 0);
}

/**
 * @brief 验证 STM32 完整报告输出所有通用字段和 STM32 专属字段。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_stm32_report_outputs_all_required_fields();
 */
static void assert_stm32_report_outputs_all_required_fields(void)
{
    /* 初始化输出捕获缓冲。 */
    SmokeReportCapture capture = {{0}, 0u};

    /* 把 report writer 绑定到测试捕获回调。 */
    MRT_SmokeLogWriter writer = {CaptureReportChar, &capture};

    /* 构造包含通用字段和 STM32 专属字段的报告。 */
    MRT_SmokeStm32Report report = {
        MakeCommonReport("STM32"),
        168000000u,
        1000u,
        4u,
        "BASEPRI nested enter/exit restored",
        "PASS",
        "PASS",
        "PASS"};

    /* 输出完整 STM32 报告。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_SMOKE_LOG_OK,
                           (unsigned)MRT_SmokeEmitStm32Report(&writer, &report));

    /* 通用字段必须全部存在。 */
    AssertContainsLine(capture.bytes, "MyRTOS-Hardware-Smoke: STM32\n");
    AssertContainsLine(capture.bytes, "Evidence-Status: PASS\n");
    AssertContainsLine(capture.bytes, "Smoke-Date: 2026-06-09\n");
    AssertContainsLine(capture.bytes, "Chip: CHIP-UNDER-TEST\n");
    AssertContainsLine(capture.bytes, "Board: BOARD-UNDER-TEST\n");
    AssertContainsLine(capture.bytes, "Compiler: compiler version\n");
    AssertContainsLine(capture.bytes, "Context-Switch: context switch observed\n");
    AssertContainsLine(capture.bytes, "Software-Timer: PASS\n");
    AssertContainsLine(capture.bytes, "Tickless: PASS\n");
    AssertContainsLine(capture.bytes, "Runtime-Minutes: 30\n");
    AssertContainsLine(capture.bytes, "Assert-Failures: 0\n");
    AssertContainsLine(capture.bytes, "Heap-Min-Free-Bytes: 4096\n");
    AssertContainsLine(capture.bytes, "Trace-Or-UART-Log: trace summary\n");
    AssertContainsLine(capture.bytes, "Raw-Log-Path: docs/verification/hardware_smoke/raw.log\n");
    AssertContainsLine(capture.bytes,
                       "Raw-Log-SHA256: 0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\n");

    /* STM32 专属字段必须全部存在。 */
    AssertContainsLine(capture.bytes, "Clock-Hz: 168000000\n");
    AssertContainsLine(capture.bytes, "Tick-Hz: 1000\n");
    AssertContainsLine(capture.bytes, "NVIC-Priority-Bits: 4\n");
    AssertContainsLine(capture.bytes, "Critical-Section: BASEPRI nested enter/exit restored\n");
    AssertContainsLine(capture.bytes, "SysTick: PASS\n");
    AssertContainsLine(capture.bytes, "PendSV-SVC: PASS\n");
    AssertContainsLine(capture.bytes, "ISR-Queue: PASS\n");
}

/**
 * @brief 验证 DSP 完整报告输出所有通用字段和 DSP 专属字段。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_dsp_report_outputs_all_required_fields();
 */
static void assert_dsp_report_outputs_all_required_fields(void)
{
    /* 初始化输出捕获缓冲。 */
    SmokeReportCapture capture = {{0}, 0u};

    /* 把 report writer 绑定到测试捕获回调。 */
    MRT_SmokeLogWriter writer = {CaptureReportChar, &capture};

    /* 构造包含通用字段和 DSP 专属字段的报告。 */
    MRT_SmokeDspReport report = {
        MakeCommonReport("DSP"),
        "eabi",
        "down",
        "PASS",
        "PASS",
        "PASS",
        "PASS"};

    /* 输出完整 DSP 报告。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_SMOKE_LOG_OK,
                           (unsigned)MRT_SmokeEmitDspReport(&writer, &report));

    /* 通用字段必须包含 DSP 目标类型。 */
    AssertContainsLine(capture.bytes, "MyRTOS-Hardware-Smoke: DSP\n");

    /* DSP 专属字段必须全部存在。 */
    AssertContainsLine(capture.bytes, "ABI: eabi\n");
    AssertContainsLine(capture.bytes, "Stack-Direction: down\n");
    AssertContainsLine(capture.bytes, "Timer-Tick: PASS\n");
    AssertContainsLine(capture.bytes, "Software-Interrupt-Switch: PASS\n");
    AssertContainsLine(capture.bytes, "ISR-Nesting: PASS\n");
    AssertContainsLine(capture.bytes, "Queue-Or-Pool: PASS\n");
}

/**
 * @brief 验证完整报告遇到非法参数时不写出半行日志。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_report_rejects_invalid_arguments_without_partial_output();
 */
static void assert_report_rejects_invalid_arguments_without_partial_output(void)
{
    /* 初始化输出捕获缓冲。 */
    SmokeReportCapture capture = {{0}, 0u};

    /* 把 report writer 绑定到测试捕获回调。 */
    MRT_SmokeLogWriter writer = {CaptureReportChar, &capture};

    /* 构造一个缺失芯片字段的非法 STM32 报告。 */
    MRT_SmokeStm32Report report = {
        MakeCommonReport("STM32"),
        168000000u,
        1000u,
        4u,
        "BASEPRI nested enter/exit restored",
        "PASS",
        "PASS",
        "PASS"};
    report.common.chip = 0;

    /* 空 writer 必须被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_SMOKE_LOG_INVALID_ARGUMENT,
                           (unsigned)MRT_SmokeEmitStm32Report(0, &report));

    /* 缺失必填字段必须被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_SMOKE_LOG_INVALID_ARGUMENT,
                           (unsigned)MRT_SmokeEmitStm32Report(&writer, &report));

    /* 失败路径不得写出半行日志。 */
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)capture.length);
}

/**
 * @brief 运行硬件 smoke 完整报告 emitter 测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证 STM32 完整报告字段输出。 */
    assert_stm32_report_outputs_all_required_fields();

    /* 验证 DSP 完整报告字段输出。 */
    assert_dsp_report_outputs_all_required_fields();

    /* 验证非法输入不会污染 raw log。 */
    assert_report_rejects_invalid_arguments_without_partial_output();

    /* 所有完整报告测试通过。 */
    return 0;
}
