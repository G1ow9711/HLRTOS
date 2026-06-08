#include "mrt_test.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "../../examples/hardware_smoke/mrt_hardware_smoke_log.h"
#include "../../examples/hardware_smoke/mrt_hardware_smoke_log_schema.h"

typedef struct
{
    /* 捕获 helper 输出的字符缓冲。 */
    char bytes[256u];

    /* 当前已经写入的字节数。 */
    size_t length;
} SmokeCapture;

/**
 * @brief 把单个日志字符写入测试缓冲。
 * @param context 指向 `SmokeCapture` 的测试上下文。
 * @param ch 需要写入的字符。
 * @return void 无返回值；缓冲满时丢弃后续字符。
 * @example
 * CaptureWriteChar(&capture, 'A');
 */
static void CaptureWriteChar(void *context, char ch)
{
    /* 取出调用方传入的捕获缓冲。 */
    SmokeCapture *capture = (SmokeCapture *)context;

    /* 预留一个结尾 NUL，便于测试按 C 字符串比较。 */
    if (capture->length + 1u >= sizeof(capture->bytes))
    {
        return;
    }

    /* 写入当前字符并推进长度。 */
    capture->bytes[capture->length] = ch;
    capture->length++;

    /* 始终保持缓冲以 NUL 结尾。 */
    capture->bytes[capture->length] = '\0';
}

/**
 * @brief 验证字符串字段按 `Key: Value` 格式输出。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_smoke_log_writes_key_value_line();
 */
static void assert_smoke_log_writes_key_value_line(void)
{
    /* 初始化捕获缓冲。 */
    SmokeCapture capture = {{0}, 0u};

    /* 构造按字符写出的日志 writer。 */
    MRT_SmokeLogWriter writer = {CaptureWriteChar, &capture};

    /* 输出一个真实证据状态字段。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_SMOKE_LOG_OK,
                           (unsigned)MRT_SmokeLogWritePair(&writer,
                                                           MRT_SMOKE_FIELD_EVIDENCE_STATUS,
                                                           "PASS"));

    /* 字段和值之间必须严格使用冒号加空格，并以换行结束。 */
    MRT_TEST_ASSERT_TRUE(strcmp(capture.bytes, "Evidence-Status: PASS\n") == 0);
}

/**
 * @brief 验证无符号整数会按十进制写入日志。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_smoke_log_writes_uint32_decimal();
 */
static void assert_smoke_log_writes_uint32_decimal(void)
{
    /* 初始化捕获缓冲。 */
    SmokeCapture capture = {{0}, 0u};

    /* 构造按字符写出的日志 writer。 */
    MRT_SmokeLogWriter writer = {CaptureWriteChar, &capture};

    /* 输出运行分钟数。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_SMOKE_LOG_OK,
                           (unsigned)MRT_SmokeLogWriteU32(&writer,
                                                          MRT_SMOKE_FIELD_RUNTIME_MINUTES,
                                                          30u));

    /* 输出 0 值，覆盖整数转换边界。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_SMOKE_LOG_OK,
                           (unsigned)MRT_SmokeLogWriteU32(&writer,
                                                          MRT_SMOKE_FIELD_ASSERT_FAILURES,
                                                          0u));

    /* 两行整数日志必须是十进制且不带单位。 */
    MRT_TEST_ASSERT_TRUE(strcmp(capture.bytes,
                                "Runtime-Minutes: 30\n"
                                "Assert-Failures: 0\n") == 0);
}

/**
 * @brief 验证非法参数不会写出半行日志。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_smoke_log_rejects_invalid_arguments();
 */
static void assert_smoke_log_rejects_invalid_arguments(void)
{
    /* 初始化捕获缓冲。 */
    SmokeCapture capture = {{0}, 0u};

    /* 构造按字符写出的日志 writer。 */
    MRT_SmokeLogWriter writer = {CaptureWriteChar, &capture};

    /* 空 writer 必须被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_SMOKE_LOG_INVALID_ARGUMENT,
                           (unsigned)MRT_SmokeLogWritePair(0,
                                                           MRT_SMOKE_FIELD_BOARD,
                                                           "board"));

    /* 空字段名必须被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_SMOKE_LOG_INVALID_ARGUMENT,
                           (unsigned)MRT_SmokeLogWritePair(&writer, 0, "board"));

    /* 空字段值必须被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_SMOKE_LOG_INVALID_ARGUMENT,
                           (unsigned)MRT_SmokeLogWritePair(&writer,
                                                           MRT_SMOKE_FIELD_BOARD,
                                                           0));

    /* 失败路径不得留下半行日志，避免误导后续证据生成器。 */
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)capture.length);
}

/**
 * @brief 运行硬件 smoke 日志 helper 测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证字符串字段输出。 */
    assert_smoke_log_writes_key_value_line();

    /* 验证整数十进制输出。 */
    assert_smoke_log_writes_uint32_decimal();

    /* 验证非法参数处理。 */
    assert_smoke_log_rejects_invalid_arguments();

    /* 所有硬件 smoke 日志 helper 测试均通过。 */
    return 0;
}
