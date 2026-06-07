#include "mrt_test.h"
#include "myrtos/mrt_message_buffer.h"

/**
 * @brief 验证静态创建消息缓冲会初始化基础字段和查询结果。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_message_buffer_create_static_initializes_fields();
 */
static void assert_message_buffer_create_static_initializes_fields(void)
{
    /* 定义消息缓冲控制块。 */
    MRT_MessageBuffer storage;

    /* 定义底层字节存储。 */
    uint8_t buffer[16u];

    /* 定义输出句柄。 */
    MRT_MessageBufferHandle message_buffer = 0;

    /* 创建容量为 16 字节的消息缓冲。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_MessageBufferCreateStatic(16u, buffer, &storage, &message_buffer));

    /* 输出句柄应指向调用方提供的控制块。 */
    MRT_TEST_ASSERT_TRUE(message_buffer == &storage);

    /* 创建后没有可读消息字节。 */
    size_t bytes = 99u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_MessageBufferBytesAvailable(message_buffer, &bytes));
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)bytes);

    /* 创建后全部空间可写。 */
    size_t spaces = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_MessageBufferSpacesAvailable(message_buffer, &spaces));
    MRT_TEST_ASSERT_EQ_U32(16u, (unsigned)spaces);

    /* 控制块字段应记录创建参数。 */
    MRT_TEST_ASSERT_TRUE(storage.buffer == buffer);
    MRT_TEST_ASSERT_EQ_U32(16u, (unsigned)storage.capacity);
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)storage.read_index);
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)storage.write_index);
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)storage.bytes_used);
    MRT_TEST_ASSERT_TRUE(storage.static_storage);
}

/**
 * @brief 验证消息缓冲静态创建参数校验。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_message_buffer_create_static_rejects_invalid_arguments();
 */
static void assert_message_buffer_create_static_rejects_invalid_arguments(void)
{
    /* 定义消息缓冲控制块和底层存储。 */
    MRT_MessageBuffer storage;
    uint8_t buffer[8u];
    MRT_MessageBufferHandle message_buffer = 0;

    /* 容量必须至少能保存 4 字节长度头和 1 字节消息。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_MessageBufferCreateStatic(4u, buffer, &storage, &message_buffer));

    /* 底层字节存储不能为空。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_MessageBufferCreateStatic(8u, 0, &storage, &message_buffer));

    /* 控制块不能为空。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_MessageBufferCreateStatic(8u, buffer, 0, &message_buffer));

    /* 输出句柄不能为空。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_MessageBufferCreateStatic(8u, buffer, &storage, 0));

    /* 空句柄不能查询可读字节。 */
    size_t value = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_MessageBufferBytesAvailable(0, &value));

    /* 可读字节输出指针不能为空。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_MessageBufferBytesAvailable(&storage, 0));

    /* 空句柄不能查询可写空间。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_MessageBufferSpacesAvailable(0, &value));

    /* 可写空间输出指针不能为空。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_MessageBufferSpacesAvailable(&storage, 0));
}

/**
 * @brief 运行消息缓冲静态创建测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证基础字段初始化。 */
    assert_message_buffer_create_static_initializes_fields();

    /* 验证参数错误路径。 */
    assert_message_buffer_create_static_rejects_invalid_arguments();

    /* 所有静态创建测试均通过。 */
    return 0;
}
