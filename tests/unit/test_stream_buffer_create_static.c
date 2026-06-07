#include "mrt_test.h"
#include "myrtos/mrt_stream_buffer.h"

/**
 * @brief 验证静态创建流缓冲会初始化基础字段和查询结果。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_stream_buffer_create_static_initializes_fields();
 */
static void assert_stream_buffer_create_static_initializes_fields(void)
{
    /* 定义流缓冲控制块。 */
    MRT_StreamBuffer storage;

    /* 定义流缓冲底层字节存储。 */
    uint8_t buffer[8u];

    /* 定义输出句柄。 */
    MRT_StreamBufferHandle stream = 0;

    /* 创建容量为 8 字节、触发水位为 3 字节的流缓冲。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_StreamBufferCreateStatic(8u, 3u, buffer, &storage, &stream));

    /* 输出句柄应指向调用方提供的控制块。 */
    MRT_TEST_ASSERT_TRUE(stream == &storage);

    /* 创建后没有可读字节。 */
    size_t bytes = 99u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_StreamBufferBytesAvailable(stream, &bytes));
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)bytes);

    /* 创建后全部空间都可写。 */
    size_t spaces = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_StreamBufferSpacesAvailable(stream, &spaces));
    MRT_TEST_ASSERT_EQ_U32(8u, (unsigned)spaces);

    /* 控制块字段应记录创建参数。 */
    MRT_TEST_ASSERT_TRUE(storage.buffer == buffer);
    MRT_TEST_ASSERT_EQ_U32(8u, (unsigned)storage.capacity);
    MRT_TEST_ASSERT_EQ_U32(3u, (unsigned)storage.trigger_level);
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)storage.read_index);
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)storage.write_index);
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)storage.bytes_used);
    MRT_TEST_ASSERT_TRUE(storage.static_storage);
}

/**
 * @brief 验证触发水位为 1 字节时静态创建成功。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_stream_buffer_create_static_accepts_minimum_trigger_level();
 */
static void assert_stream_buffer_create_static_accepts_minimum_trigger_level(void)
{
    /* 定义流缓冲控制块和底层存储。 */
    MRT_StreamBuffer storage;
    uint8_t buffer[4u];
    MRT_StreamBufferHandle stream = 0;

    /* 触发水位 1 表示任意一个字节到达即可唤醒读者。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_StreamBufferCreateStatic(4u, 1u, buffer, &storage, &stream));

    /* 创建成功后句柄非空。 */
    MRT_TEST_ASSERT_TRUE(stream != 0);
}

/**
 * @brief 验证流缓冲静态创建和查询 API 参数校验。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_stream_buffer_create_static_rejects_invalid_arguments();
 */
static void assert_stream_buffer_create_static_rejects_invalid_arguments(void)
{
    /* 定义流缓冲控制块和底层存储。 */
    MRT_StreamBuffer storage;
    uint8_t buffer[4u];
    MRT_StreamBufferHandle stream = 0;

    /* 容量不能为 0。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_StreamBufferCreateStatic(0u, 1u, buffer, &storage, &stream));

    /* 触发水位不能为 0。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_StreamBufferCreateStatic(4u, 0u, buffer, &storage, &stream));

    /* 触发水位不能大于容量。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_StreamBufferCreateStatic(4u, 5u, buffer, &storage, &stream));

    /* 字节存储不能为空。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_StreamBufferCreateStatic(4u, 1u, 0, &storage, &stream));

    /* 控制块不能为空。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_StreamBufferCreateStatic(4u, 1u, buffer, 0, &stream));

    /* 输出句柄不能为空。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_StreamBufferCreateStatic(4u, 1u, buffer, &storage, 0));

    /* 空句柄不能查询可读字节。 */
    size_t value = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_StreamBufferBytesAvailable(0, &value));

    /* 可读字节输出指针不能为空。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_StreamBufferBytesAvailable(&storage, 0));

    /* 空句柄不能查询可写空间。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_StreamBufferSpacesAvailable(0, &value));

    /* 可写空间输出指针不能为空。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_StreamBufferSpacesAvailable(&storage, 0));
}

/**
 * @brief 运行流缓冲静态创建测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证基础字段初始化。 */
    assert_stream_buffer_create_static_initializes_fields();

    /* 验证最小触发水位。 */
    assert_stream_buffer_create_static_accepts_minimum_trigger_level();

    /* 验证参数错误路径。 */
    assert_stream_buffer_create_static_rejects_invalid_arguments();

    /* 所有静态创建测试均通过。 */
    return 0;
}
