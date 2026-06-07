#include "mrt_test.h"
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

    /* 创建触发水位为 1 的测试流缓冲。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_StreamBufferCreateStatic(capacity, 1u, buffer, storage, &stream));

    /* 返回创建得到的句柄。 */
    return stream;
}

/**
 * @brief 验证流缓冲按 FIFO 顺序写入和读取字节。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_stream_buffer_send_receive_preserves_fifo_order();
 */
static void assert_stream_buffer_send_receive_preserves_fifo_order(void)
{
    /* 定义流缓冲控制块和底层存储。 */
    MRT_StreamBuffer storage;
    uint8_t buffer[5u];
    MRT_StreamBufferHandle stream = CreateStreamOrFail(&storage, buffer, sizeof(buffer));

    /* 准备 3 字节输入数据。 */
    const uint8_t input[3u] = {1u, 2u, 3u};

    /* 写入 3 字节。 */
    size_t sent = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_StreamBufferSend(stream, input, sizeof(input), 0u, &sent));
    MRT_TEST_ASSERT_EQ_U32(3u, (unsigned)sent);

    /* 查询可读和可写计数。 */
    size_t bytes = 0u;
    size_t spaces = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_StreamBufferBytesAvailable(stream, &bytes));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_StreamBufferSpacesAvailable(stream, &spaces));
    MRT_TEST_ASSERT_EQ_U32(3u, (unsigned)bytes);
    MRT_TEST_ASSERT_EQ_U32(2u, (unsigned)spaces);

    /* 读取 2 字节。 */
    uint8_t output[2u] = {0u, 0u};
    size_t received = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_StreamBufferReceive(stream, output, sizeof(output), 0u, &received));

    /* 验证读取字节数量和 FIFO 顺序。 */
    MRT_TEST_ASSERT_EQ_U32(2u, (unsigned)received);
    MRT_TEST_ASSERT_EQ_U32(1u, (unsigned)output[0]);
    MRT_TEST_ASSERT_EQ_U32(2u, (unsigned)output[1]);

    /* 缓冲中应剩余 1 字节。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_StreamBufferBytesAvailable(stream, &bytes));
    MRT_TEST_ASSERT_EQ_U32(1u, (unsigned)bytes);
}

/**
 * @brief 验证读写索引环绕后仍保持字节顺序。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_stream_buffer_wraparound_keeps_byte_order();
 */
static void assert_stream_buffer_wraparound_keeps_byte_order(void)
{
    /* 定义容量为 5 的流缓冲。 */
    MRT_StreamBuffer storage;
    uint8_t buffer[5u];
    MRT_StreamBufferHandle stream = CreateStreamOrFail(&storage, buffer, sizeof(buffer));

    /* 写入 4 字节，让写索引靠近尾部。 */
    const uint8_t first_write[4u] = {'A', 'B', 'C', 'D'};
    size_t sent = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_StreamBufferSend(stream, first_write, sizeof(first_write), 0u, &sent));
    MRT_TEST_ASSERT_EQ_U32(4u, (unsigned)sent);

    /* 读取 3 字节，让读索引也前移。 */
    uint8_t discard[3u] = {0u, 0u, 0u};
    size_t received = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_StreamBufferReceive(stream, discard, sizeof(discard), 0u, &received));
    MRT_TEST_ASSERT_EQ_U32(3u, (unsigned)received);

    /* 再写入 4 字节，写索引必须从尾部环绕到头部。 */
    const uint8_t second_write[4u] = {'E', 'F', 'G', 'H'};
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_StreamBufferSend(stream, second_write, sizeof(second_write), 0u, &sent));
    MRT_TEST_ASSERT_EQ_U32(4u, (unsigned)sent);

    /* 读取所有剩余 5 字节。 */
    uint8_t output[5u] = {0u, 0u, 0u, 0u, 0u};
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_StreamBufferReceive(stream, output, sizeof(output), 0u, &received));
    MRT_TEST_ASSERT_EQ_U32(5u, (unsigned)received);

    /* 验证跨环绕后的顺序仍为 D E F G H。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)'D', (unsigned)output[0]);
    MRT_TEST_ASSERT_EQ_U32((unsigned)'E', (unsigned)output[1]);
    MRT_TEST_ASSERT_EQ_U32((unsigned)'F', (unsigned)output[2]);
    MRT_TEST_ASSERT_EQ_U32((unsigned)'G', (unsigned)output[3]);
    MRT_TEST_ASSERT_EQ_U32((unsigned)'H', (unsigned)output[4]);
}

/**
 * @brief 验证空间不足时非阻塞写入只写可容纳部分。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_stream_buffer_partial_send_writes_available_space();
 */
static void assert_stream_buffer_partial_send_writes_available_space(void)
{
    /* 定义容量为 4 的流缓冲。 */
    MRT_StreamBuffer storage;
    uint8_t buffer[4u];
    MRT_StreamBufferHandle stream = CreateStreamOrFail(&storage, buffer, sizeof(buffer));

    /* 先写入 3 字节。 */
    const uint8_t first_write[3u] = {10u, 11u, 12u};
    size_t sent = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_StreamBufferSend(stream, first_write, sizeof(first_write), 0u, &sent));
    MRT_TEST_ASSERT_EQ_U32(3u, (unsigned)sent);

    /* 再请求写入 3 字节，但只剩 1 字节空间。 */
    const uint8_t second_write[3u] = {13u, 14u, 15u};
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_StreamBufferSend(stream, second_write, sizeof(second_write), 0u, &sent));
    MRT_TEST_ASSERT_EQ_U32(1u, (unsigned)sent);

    /* 缓冲区已满，再写入应返回对象满。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OBJECT_FULL,
                           (unsigned)MRT_StreamBufferSend(stream, second_write, sizeof(second_write), 0u, &sent));
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)sent);
}

/**
 * @brief 验证空流缓冲读取、reset 和参数校验。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_stream_buffer_empty_reset_and_invalid_arguments();
 */
static void assert_stream_buffer_empty_reset_and_invalid_arguments(void)
{
    /* 定义容量为 4 的流缓冲。 */
    MRT_StreamBuffer storage;
    uint8_t buffer[4u];
    MRT_StreamBufferHandle stream = CreateStreamOrFail(&storage, buffer, sizeof(buffer));

    /* 空缓冲读取应返回对象空。 */
    uint8_t output[2u] = {0u, 0u};
    size_t received = 99u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OBJECT_EMPTY,
                           (unsigned)MRT_StreamBufferReceive(stream, output, sizeof(output), 0u, &received));
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)received);

    /* 写入数据后 reset 应清空计数并复位索引。 */
    const uint8_t input[2u] = {1u, 2u};
    size_t sent = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_StreamBufferSend(stream, input, sizeof(input), 0u, &sent));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_StreamBufferReset(stream));
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)storage.bytes_used);
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)storage.read_index);
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)storage.write_index);

    /* 空句柄不能 reset。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT, (unsigned)MRT_StreamBufferReset(0));

    /* 非零长度发送时数据指针不能为空。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_StreamBufferSend(stream, 0, 1u, 0u, &sent));

    /* 非零长度接收时输出指针不能为空。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_StreamBufferReceive(stream, 0, 1u, 0u, &received));
}

/**
 * @brief 运行流缓冲发送接收测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证基础 FIFO 语义。 */
    assert_stream_buffer_send_receive_preserves_fifo_order();

    /* 验证环绕顺序。 */
    assert_stream_buffer_wraparound_keeps_byte_order();

    /* 验证部分写入和满缓冲。 */
    assert_stream_buffer_partial_send_writes_available_space();

    /* 验证空读、reset 和参数错误路径。 */
    assert_stream_buffer_empty_reset_and_invalid_arguments();

    /* 所有发送接收测试均通过。 */
    return 0;
}
