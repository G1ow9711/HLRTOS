#include "mrt_test.h"
#include "myrtos/mrt_message_buffer.h"

/**
 * @brief 创建测试用消息缓冲。
 * @param storage 消息缓冲控制块，不能为空。
 * @param buffer 底层字节存储，不能为空。
 * @param capacity 字节容量。
 * @return MRT_MessageBufferHandle 返回创建成功的消息缓冲句柄。
 * @example
 * MRT_MessageBufferHandle mb = CreateMessageBufferOrFail(&storage, buffer, sizeof(buffer));
 */
static MRT_MessageBufferHandle CreateMessageBufferOrFail(MRT_MessageBuffer *storage,
                                                         uint8_t *buffer,
                                                         size_t capacity)
{
    /* 定义输出句柄。 */
    MRT_MessageBufferHandle message_buffer = 0;

    /* 静态创建测试消息缓冲。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_MessageBufferCreateStatic(capacity, buffer, storage, &message_buffer));

    /* 返回创建得到的句柄。 */
    return message_buffer;
}

/**
 * @brief 验证消息缓冲按完整消息边界发送和接收。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_message_buffer_preserves_packet_boundaries();
 */
static void assert_message_buffer_preserves_packet_boundaries(void)
{
    /* 定义消息缓冲控制块和底层存储。 */
    MRT_MessageBuffer storage;
    uint8_t buffer[16u];
    MRT_MessageBufferHandle message_buffer = CreateMessageBufferOrFail(&storage, buffer, sizeof(buffer));

    /* 准备两条不同长度消息。 */
    const uint8_t first[2u] = {'h', 'i'};
    const uint8_t second[3u] = {'b', 'y', 'e'};

    /* 发送第一条消息。 */
    size_t sent = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_MessageBufferSend(message_buffer, first, sizeof(first), 0u, &sent));
    MRT_TEST_ASSERT_EQ_U32(2u, (unsigned)sent);

    /* 发送第二条消息。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_MessageBufferSend(message_buffer, second, sizeof(second), 0u, &sent));
    MRT_TEST_ASSERT_EQ_U32(3u, (unsigned)sent);

    /* 接收第一条消息，不能读到第二条消息内容。 */
    uint8_t output[4u] = {0u, 0u, 0u, 0u};
    size_t received = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_MessageBufferReceive(message_buffer, output, sizeof(output), 0u, &received));
    MRT_TEST_ASSERT_EQ_U32(2u, (unsigned)received);
    MRT_TEST_ASSERT_EQ_U32((unsigned)'h', (unsigned)output[0]);
    MRT_TEST_ASSERT_EQ_U32((unsigned)'i', (unsigned)output[1]);

    /* 接收第二条消息。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_MessageBufferReceive(message_buffer, output, sizeof(output), 0u, &received));
    MRT_TEST_ASSERT_EQ_U32(3u, (unsigned)received);
    MRT_TEST_ASSERT_EQ_U32((unsigned)'b', (unsigned)output[0]);
    MRT_TEST_ASSERT_EQ_U32((unsigned)'y', (unsigned)output[1]);
    MRT_TEST_ASSERT_EQ_U32((unsigned)'e', (unsigned)output[2]);
}

/**
 * @brief 验证输出缓冲太小时不移除下一条消息。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_message_buffer_small_output_keeps_message_pending();
 */
static void assert_message_buffer_small_output_keeps_message_pending(void)
{
    /* 定义消息缓冲控制块和底层存储。 */
    MRT_MessageBuffer storage;
    uint8_t buffer[12u];
    MRT_MessageBufferHandle message_buffer = CreateMessageBufferOrFail(&storage, buffer, sizeof(buffer));

    /* 发送一条 3 字节消息。 */
    const uint8_t message[3u] = {1u, 2u, 3u};
    size_t sent = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_MessageBufferSend(message_buffer, message, sizeof(message), 0u, &sent));

    /* 记录当前已使用空间，包含 4 字节长度头和 3 字节载荷。 */
    size_t bytes_before = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_MessageBufferBytesAvailable(message_buffer, &bytes_before));
    MRT_TEST_ASSERT_EQ_U32(7u, (unsigned)bytes_before);

    /* 使用过小输出缓冲接收，应返回对象满并保留消息。 */
    uint8_t small_output[2u] = {0u, 0u};
    size_t received = 99u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OBJECT_FULL,
                           (unsigned)MRT_MessageBufferReceive(message_buffer,
                                                              small_output,
                                                              sizeof(small_output),
                                                              0u,
                                                              &received));
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)received);

    /* 消息仍应保留在缓冲中。 */
    size_t bytes_after = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_MessageBufferBytesAvailable(message_buffer, &bytes_after));
    MRT_TEST_ASSERT_EQ_U32((unsigned)bytes_before, (unsigned)bytes_after);

    /* 使用足够大的输出缓冲应能读出原消息。 */
    uint8_t output[3u] = {0u, 0u, 0u};
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_MessageBufferReceive(message_buffer,
                                                              output,
                                                              sizeof(output),
                                                              0u,
                                                              &received));
    MRT_TEST_ASSERT_EQ_U32(3u, (unsigned)received);
    MRT_TEST_ASSERT_EQ_U32(1u, (unsigned)output[0]);
    MRT_TEST_ASSERT_EQ_U32(2u, (unsigned)output[1]);
    MRT_TEST_ASSERT_EQ_U32(3u, (unsigned)output[2]);
}

/**
 * @brief 验证容量不足时不产生半条消息。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_message_buffer_rejects_oversized_and_partial_messages();
 */
static void assert_message_buffer_rejects_oversized_and_partial_messages(void)
{
    /* 定义容量为 10 的消息缓冲。 */
    MRT_MessageBuffer storage;
    uint8_t buffer[10u];
    MRT_MessageBufferHandle message_buffer = CreateMessageBufferOrFail(&storage, buffer, sizeof(buffer));

    /* 单条消息超过总容量时应被拒绝。 */
    const uint8_t too_large[7u] = {0u, 1u, 2u, 3u, 4u, 5u, 6u};
    size_t sent = 99u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OBJECT_FULL,
                           (unsigned)MRT_MessageBufferSend(message_buffer, too_large, sizeof(too_large), 0u, &sent));
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)sent);

    /* 先写入一条 3 字节消息，占用 7 字节。 */
    const uint8_t first[3u] = {10u, 11u, 12u};
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_MessageBufferSend(message_buffer, first, sizeof(first), 0u, &sent));
    MRT_TEST_ASSERT_EQ_U32(3u, (unsigned)sent);

    /* 剩余 3 字节不足以保存下一条 1 字节消息的 4 字节头和载荷。 */
    const uint8_t second[1u] = {13u};
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OBJECT_FULL,
                           (unsigned)MRT_MessageBufferSend(message_buffer, second, sizeof(second), 0u, &sent));
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)sent);

    /* 缓冲中仍只有第一条完整消息。 */
    uint8_t output[3u] = {0u, 0u, 0u};
    size_t received = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_MessageBufferReceive(message_buffer, output, sizeof(output), 0u, &received));
    MRT_TEST_ASSERT_EQ_U32(3u, (unsigned)received);
    MRT_TEST_ASSERT_EQ_U32(10u, (unsigned)output[0]);
    MRT_TEST_ASSERT_EQ_U32(11u, (unsigned)output[1]);
    MRT_TEST_ASSERT_EQ_U32(12u, (unsigned)output[2]);
}

/**
 * @brief 验证空消息缓冲读取、reset 和参数校验。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_message_buffer_empty_reset_and_invalid_arguments();
 */
static void assert_message_buffer_empty_reset_and_invalid_arguments(void)
{
    /* 定义消息缓冲控制块和底层存储。 */
    MRT_MessageBuffer storage;
    uint8_t buffer[8u];
    MRT_MessageBufferHandle message_buffer = CreateMessageBufferOrFail(&storage, buffer, sizeof(buffer));

    /* 空消息缓冲接收应返回对象空。 */
    uint8_t output[2u] = {0u, 0u};
    size_t received = 99u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OBJECT_EMPTY,
                           (unsigned)MRT_MessageBufferReceive(message_buffer, output, sizeof(output), 0u, &received));
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)received);

    /* 发送消息后 reset 应清空状态。 */
    const uint8_t message[1u] = {5u};
    size_t sent = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_MessageBufferSend(message_buffer, message, sizeof(message), 0u, &sent));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MessageBufferReset(message_buffer));
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)storage.bytes_used);
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)storage.read_index);
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)storage.write_index);

    /* 空句柄不能 reset。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT, (unsigned)MRT_MessageBufferReset(0));

    /* 非零长度发送时消息指针不能为空。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_MessageBufferSend(message_buffer, 0, 1u, 0u, &sent));

    /* 接收时输出指针不能为空。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_MessageBufferReceive(message_buffer, 0, 1u, 0u, &received));
}

/**
 * @brief 运行消息缓冲发送接收测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证消息边界。 */
    assert_message_buffer_preserves_packet_boundaries();

    /* 验证输出缓冲不足不移除消息。 */
    assert_message_buffer_small_output_keeps_message_pending();

    /* 验证容量不足不产生半包。 */
    assert_message_buffer_rejects_oversized_and_partial_messages();

    /* 验证空读、reset 和参数错误路径。 */
    assert_message_buffer_empty_reset_and_invalid_arguments();

    /* 所有消息缓冲发送接收测试均通过。 */
    return 0;
}
