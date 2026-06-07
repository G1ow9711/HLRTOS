#include "mrt_test.h"
#include "myrtos/mrt_port.h"
#include "myrtos/mrt_stream_buffer.h"

/**
 * @brief 创建测试用流缓冲。
 * @param storage 流缓冲控制块，不能为空。
 * @param buffer 底层字节存储，不能为空。
 * @param capacity 字节容量。
 * @return MRT_StreamBufferHandle 返回创建成功的流缓冲句柄。
 * @example
 * MRT_StreamBufferHandle stream = CreateStreamOrFail(&storage, buffer, sizeof(buffer));
 */
static MRT_StreamBufferHandle CreateStreamOrFail(MRT_StreamBuffer *storage, uint8_t *buffer, size_t capacity)
{
    /* 定义输出句柄。 */
    MRT_StreamBufferHandle stream = 0;

    /* 使用触发水位 1 创建测试流缓冲。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_StreamBufferCreateStatic(capacity, 1u, buffer, storage, &stream));

    /* 返回创建得到的流缓冲句柄。 */
    return stream;
}

/**
 * @brief 验证 ISR 流缓冲发送和接收能完成一次非阻塞传输。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_stream_buffer_isr_send_receive_transfers_bytes();
 */
static void assert_stream_buffer_isr_send_receive_transfers_bytes(void)
{
    /* 初始化端口 mock 并切换到 ISR 上下文。 */
    MRT_PortInitialize();
    MRT_PortMockSetInsideISR(true);

    /* 定义流缓冲控制块和底层存储。 */
    MRT_StreamBuffer storage;
    uint8_t buffer[4u];
    MRT_StreamBufferHandle stream = CreateStreamOrFail(&storage, buffer, sizeof(buffer));

    /* 准备 ISR 待写入数据。 */
    const uint8_t input[2u] = {7u, 8u};

    /* 从 ISR 发送 2 字节。 */
    size_t sent = 0u;
    bool should_yield = true;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_StreamBufferSendFromISR(stream, input, sizeof(input), &sent, &should_yield));

    /* 当前没有等待读者，不应请求切换。 */
    MRT_TEST_ASSERT_EQ_U32(2u, (unsigned)sent);
    MRT_TEST_ASSERT_TRUE(!should_yield);

    /* 从 ISR 读取 2 字节。 */
    uint8_t output[2u] = {0u, 0u};
    size_t received = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_StreamBufferReceiveFromISR(stream, output, sizeof(output), &received));

    /* 验证读取数量和数据顺序。 */
    MRT_TEST_ASSERT_EQ_U32(2u, (unsigned)received);
    MRT_TEST_ASSERT_EQ_U32(7u, (unsigned)output[0]);
    MRT_TEST_ASSERT_EQ_U32(8u, (unsigned)output[1]);

    /* 恢复任务上下文，避免影响后续测试。 */
    MRT_PortMockSetInsideISR(false);
}

/**
 * @brief 验证 ISR 流缓冲 API 的边界和上下文校验。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_stream_buffer_isr_rejects_invalid_context_and_edges();
 */
static void assert_stream_buffer_isr_rejects_invalid_context_and_edges(void)
{
    /* 初始化端口 mock，默认处于任务上下文。 */
    MRT_PortInitialize();

    /* 定义流缓冲控制块和底层存储。 */
    MRT_StreamBuffer storage;
    uint8_t buffer[1u];
    MRT_StreamBufferHandle stream = CreateStreamOrFail(&storage, buffer, sizeof(buffer));

    /* 任务上下文调用 FromISR 发送应返回非法上下文。 */
    const uint8_t value = 9u;
    size_t transferred = 99u;
    bool should_yield = true;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_CONTEXT,
                           (unsigned)MRT_StreamBufferSendFromISR(stream, &value, 1u, &transferred, &should_yield));
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)transferred);
    MRT_TEST_ASSERT_TRUE(!should_yield);

    /* 任务上下文调用 FromISR 接收也应返回非法上下文。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_CONTEXT,
                           (unsigned)MRT_StreamBufferReceiveFromISR(stream, (void *)&value, 1u, &transferred));
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)transferred);

    /* 切换到 ISR 上下文。 */
    MRT_PortMockSetInsideISR(true);

    /* 空流缓冲 ISR 接收应返回对象空。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OBJECT_EMPTY,
                           (unsigned)MRT_StreamBufferReceiveFromISR(stream, (void *)&value, 1u, &transferred));
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)transferred);

    /* 填满唯一字节空间。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_StreamBufferSendFromISR(stream, &value, 1u, &transferred, &should_yield));

    /* 满流缓冲 ISR 再发送应返回对象满。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OBJECT_FULL,
                           (unsigned)MRT_StreamBufferSendFromISR(stream, &value, 1u, &transferred, &should_yield));
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)transferred);

    /* 恢复任务上下文。 */
    MRT_PortMockSetInsideISR(false);
}

/**
 * @brief 运行流缓冲 ISR API 测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证 ISR 非阻塞发送接收。 */
    assert_stream_buffer_isr_send_receive_transfers_bytes();

    /* 验证 ISR 边界和上下文限制。 */
    assert_stream_buffer_isr_rejects_invalid_context_and_edges();

    /* 所有流缓冲 ISR 测试均通过。 */
    return 0;
}
