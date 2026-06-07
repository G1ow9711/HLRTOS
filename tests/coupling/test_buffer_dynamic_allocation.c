#include "mrt_test.h"
#include "myrtos/mrt_heap.h"
#include "myrtos/mrt_message_buffer.h"
#include "myrtos/mrt_stream_buffer.h"

/**
 * @brief 初始化一个可合并堆并读取初始空闲空间。
 * @param heap_words 堆存储起始地址，不能为空。
 * @param heap_bytes 堆存储字节数。
 * @param out_free 输出初始空闲空间，不能为空。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * init_heap(heap_words, sizeof(heap_words), &free_before);
 */
static void init_heap(uintptr_t *heap_words, size_t heap_bytes, size_t *out_free)
{
    /* 初始化为合并堆，便于失败路径验证堆空闲水位不变。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_HeapInitialize(heap_words, heap_bytes, MRT_HEAP_MODE_COALESCING));

    /* 读取初始空闲空间。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_HeapGetFreeSize(out_free));
}

/**
 * @brief 验证动态流缓冲可以保存和读取字节流。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_dynamic_stream_buffer_send_receive();
 */
static void assert_dynamic_stream_buffer_send_receive(void)
{
    /* 定义堆并初始化。 */
    uintptr_t heap_words[256u];
    size_t free_before = 0u;
    init_heap(heap_words, sizeof(heap_words), &free_before);

    /* 动态创建流缓冲。 */
    MRT_StreamBufferHandle stream = 0;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_StreamBufferCreate(64u, 4u, &stream));
    MRT_TEST_ASSERT_TRUE(stream != 0);

    /* 写入字节序列。 */
    const uint8_t input[5u] = {1u, 2u, 3u, 4u, 5u};
    size_t sent = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_StreamBufferSend(stream, input, sizeof(input), 0u, &sent));
    MRT_TEST_ASSERT_EQ_U32(5u, (unsigned)sent);

    /* 读取并验证 FIFO 字节。 */
    uint8_t output[5u] = {0u};
    size_t received = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_StreamBufferReceive(stream, output, sizeof(output), 0u, &received));
    MRT_TEST_ASSERT_EQ_U32(5u, (unsigned)received);
    for (size_t index = 0u; index < sizeof(input); index++) {
        /* 每个字节都必须保持 FIFO 顺序。 */
        MRT_TEST_ASSERT_EQ_U32((unsigned)input[index], (unsigned)output[index]);
    }
}

/**
 * @brief 验证动态流缓冲失败路径会清空输出句柄并保持堆水位。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_dynamic_stream_buffer_failure_and_invalid_arguments();
 */
static void assert_dynamic_stream_buffer_failure_and_invalid_arguments(void)
{
    /* 定义小堆并初始化。 */
    uintptr_t heap_words[32u];
    size_t free_before = 0u;
    init_heap(heap_words, sizeof(heap_words), &free_before);

    /* 请求远大于堆容量的流缓冲。 */
    MRT_StreamBufferHandle stream = (MRT_StreamBufferHandle)heap_words;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_NO_MEMORY,
                           (unsigned)MRT_StreamBufferCreate(4096u, 4u, &stream));
    MRT_TEST_ASSERT_TRUE(stream == 0);

    /* 失败创建不能改变堆空闲空间。 */
    size_t free_after_fail = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_HeapGetFreeSize(&free_after_fail));
    MRT_TEST_ASSERT_EQ_U32((unsigned)free_before, (unsigned)free_after_fail);

    /* 参数非法应被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_StreamBufferCreate(0u, 1u, &stream));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_StreamBufferCreate(8u, 0u, &stream));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_StreamBufferCreate(8u, 9u, &stream));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_StreamBufferCreate(8u, 1u, 0));
}

/**
 * @brief 验证动态消息缓冲可以保存和读取完整消息。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_dynamic_message_buffer_send_receive();
 */
static void assert_dynamic_message_buffer_send_receive(void)
{
    /* 定义堆并初始化。 */
    uintptr_t heap_words[256u];
    size_t free_before = 0u;
    init_heap(heap_words, sizeof(heap_words), &free_before);

    /* 动态创建消息缓冲。 */
    MRT_MessageBufferHandle message_buffer = 0;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_MessageBufferCreate(64u, &message_buffer));
    MRT_TEST_ASSERT_TRUE(message_buffer != 0);

    /* 发送一条完整消息。 */
    const uint8_t input[4u] = {9u, 8u, 7u, 6u};
    size_t sent = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_MessageBufferSend(message_buffer, input, sizeof(input), 0u, &sent));
    MRT_TEST_ASSERT_EQ_U32(4u, (unsigned)sent);

    /* 接收并验证完整消息。 */
    uint8_t output[4u] = {0u};
    size_t received = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_MessageBufferReceive(message_buffer, output, sizeof(output), 0u, &received));
    MRT_TEST_ASSERT_EQ_U32(4u, (unsigned)received);
    for (size_t index = 0u; index < sizeof(input); index++) {
        /* 每个消息字节都必须一致。 */
        MRT_TEST_ASSERT_EQ_U32((unsigned)input[index], (unsigned)output[index]);
    }
}

/**
 * @brief 验证动态消息缓冲失败路径和参数保护。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_dynamic_message_buffer_failure_and_invalid_arguments();
 */
static void assert_dynamic_message_buffer_failure_and_invalid_arguments(void)
{
    /* 定义小堆并初始化。 */
    uintptr_t heap_words[32u];
    size_t free_before = 0u;
    init_heap(heap_words, sizeof(heap_words), &free_before);

    /* 请求远大于堆容量的消息缓冲。 */
    MRT_MessageBufferHandle message_buffer = (MRT_MessageBufferHandle)heap_words;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_NO_MEMORY,
                           (unsigned)MRT_MessageBufferCreate(4096u, &message_buffer));
    MRT_TEST_ASSERT_TRUE(message_buffer == 0);

    /* 失败创建不能改变堆空闲空间。 */
    size_t free_after_fail = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_HeapGetFreeSize(&free_after_fail));
    MRT_TEST_ASSERT_EQ_U32((unsigned)free_before, (unsigned)free_after_fail);

    /* 容量太小和空输出句柄应被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_MessageBufferCreate(4u, &message_buffer));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_MessageBufferCreate(8u, 0));
}

/**
 * @brief 运行动态缓冲对象测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证动态流缓冲成功路径。 */
    assert_dynamic_stream_buffer_send_receive();

    /* 验证动态流缓冲失败路径。 */
    assert_dynamic_stream_buffer_failure_and_invalid_arguments();

    /* 验证动态消息缓冲成功路径。 */
    assert_dynamic_message_buffer_send_receive();

    /* 验证动态消息缓冲失败路径。 */
    assert_dynamic_message_buffer_failure_and_invalid_arguments();

    /* 所有动态缓冲测试均通过。 */
    return 0;
}
