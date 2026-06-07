#include "mrt_test.h"
#include "myrtos/mrt_heap.h"
#include "myrtos/mrt_queue.h"

/**
 * @brief 验证动态队列可以从 MyRTOS 堆创建、传输 FIFO 数据并在删除时归还堆空间。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_dynamic_queue_create_fifo_and_delete_returns_heap();
 */
static void assert_dynamic_queue_create_fifo_and_delete_returns_heap(void)
{
    /* 定义自然对齐的可合并堆存储。 */
    uintptr_t heap_words[128u];

    /* 初始化为合并堆，便于删除队列后完整归还空间。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_HeapInitialize(heap_words,
                                                        sizeof(heap_words),
                                                        MRT_HEAP_MODE_COALESCING));

    /* 记录创建前空闲空间。 */
    size_t free_before_create = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_HeapGetFreeSize(&free_before_create));

    /* 动态创建一个可保存 4 个 uint32_t 的队列。 */
    MRT_QueueHandle queue = 0;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_QueueCreate(sizeof(uint32_t), 4u, &queue));
    MRT_TEST_ASSERT_TRUE(queue != 0);

    /* 创建后堆空闲空间应减少。 */
    size_t free_after_create = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_HeapGetFreeSize(&free_after_create));
    MRT_TEST_ASSERT_TRUE(free_after_create < free_before_create);

    /* 发送两个元素验证动态队列可正常执行 FIFO 路径。 */
    uint32_t first = 0x11111111u;
    uint32_t second = 0x22222222u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_QueueSend(queue, &first, 0u));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_QueueSend(queue, &second, 0u));

    /* 按 FIFO 顺序接收两个元素。 */
    uint32_t received = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_QueueReceive(queue, &received, 0u));
    MRT_TEST_ASSERT_EQ_U32((unsigned)first, (unsigned)received);
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_QueueReceive(queue, &received, 0u));
    MRT_TEST_ASSERT_EQ_U32((unsigned)second, (unsigned)received);

    /* 删除动态队列应释放其控制块和数据区。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_QueueDelete(queue));

    /* 删除后合并堆空闲空间应恢复到创建前。 */
    size_t free_after_delete = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_HeapGetFreeSize(&free_after_delete));
    MRT_TEST_ASSERT_EQ_U32((unsigned)free_before_create, (unsigned)free_after_delete);
}

/**
 * @brief 验证动态队列创建失败时返回内存不足并保持堆水位不变。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_dynamic_queue_create_failure_keeps_heap_unchanged();
 */
static void assert_dynamic_queue_create_failure_keeps_heap_unchanged(void)
{
    /* 定义较小但足以初始化合并堆的存储。 */
    uintptr_t heap_words[16u];

    /* 初始化为合并堆。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_HeapInitialize(heap_words,
                                                        sizeof(heap_words),
                                                        MRT_HEAP_MODE_COALESCING));

    /* 记录失败创建前的空闲空间。 */
    size_t free_before = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_HeapGetFreeSize(&free_before));

    /* 请求远大于堆容量的队列，必须创建失败。 */
    MRT_QueueHandle queue = (MRT_QueueHandle)heap_words;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_NO_MEMORY,
                           (unsigned)MRT_QueueCreate(sizeof(uint32_t), 128u, &queue));

    /* 失败时输出句柄应被清空，避免调用方误用旧值。 */
    MRT_TEST_ASSERT_TRUE(queue == 0);

    /* 失败创建不能改变堆空闲空间。 */
    size_t free_after = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_HeapGetFreeSize(&free_after));
    MRT_TEST_ASSERT_EQ_U32((unsigned)free_before, (unsigned)free_after);
}

/**
 * @brief 验证静态队列不能通过动态删除 API 释放。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_static_queue_delete_is_rejected();
 */
static void assert_static_queue_delete_is_rejected(void)
{
    /* 准备静态队列控制块和数据存储。 */
    MRT_Queue queue_storage;
    uint32_t queue_buffer[2u];
    MRT_QueueHandle queue = 0;

    /* 静态创建队列。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_QueueCreateStatic(2u,
                                                            sizeof(uint32_t),
                                                            queue_buffer,
                                                            &queue_storage,
                                                            &queue));

    /* 静态队列不归动态堆所有，删除应返回对象忙。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OBJECT_BUSY, (unsigned)MRT_QueueDelete(queue));
}

/**
 * @brief 验证动态队列 API 会拒绝明显非法参数。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_dynamic_queue_apis_reject_invalid_arguments();
 */
static void assert_dynamic_queue_apis_reject_invalid_arguments(void)
{
    /* 定义自然对齐的堆存储。 */
    uintptr_t heap_words[64u];
    MRT_QueueHandle queue = 0;

    /* 初始化为合并堆。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_HeapInitialize(heap_words,
                                                        sizeof(heap_words),
                                                        MRT_HEAP_MODE_COALESCING));

    /* 创建参数不能包含零大小元素、零容量或空输出句柄。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_QueueCreate(0u, 4u, &queue));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_QueueCreate(sizeof(uint32_t), 0u, &queue));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_QueueCreate(sizeof(uint32_t), 4u, 0));

    /* 空队列句柄不能删除。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT, (unsigned)MRT_QueueDelete(0));
}

/**
 * @brief 运行动态队列与堆耦合测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证动态队列成功创建、FIFO 行为和删除释放。 */
    assert_dynamic_queue_create_fifo_and_delete_returns_heap();

    /* 验证创建失败时堆状态不变。 */
    assert_dynamic_queue_create_failure_keeps_heap_unchanged();

    /* 验证静态队列不能被动态删除。 */
    assert_static_queue_delete_is_rejected();

    /* 验证非法参数保护。 */
    assert_dynamic_queue_apis_reject_invalid_arguments();

    /* 所有动态队列耦合测试均通过。 */
    return 0;
}
