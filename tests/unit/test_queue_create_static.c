#include "mrt_test.h"
#include "myrtos/mrt_queue.h"

/**
 * @brief 验证静态创建队列会拒绝非法参数。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_queue_create_static_rejects_invalid_arguments();
 */
static void assert_queue_create_static_rejects_invalid_arguments(void)
{
    /* 定义队列控制块存储。 */
    MRT_Queue queue_storage;

    /* 定义队列数据缓冲区。 */
    uint8_t buffer[4u * sizeof(uint32_t)];

    /* 定义输出队列句柄。 */
    MRT_QueueHandle queue = 0;

    /* 空控制块必须被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_QueueCreateStatic(4u, sizeof(uint32_t), buffer, 0, &queue));

    /* 空缓冲区必须被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_QueueCreateStatic(4u, sizeof(uint32_t), 0, &queue_storage, &queue));

    /* 队列容量为 0 必须被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_QueueCreateStatic(0u, sizeof(uint32_t), buffer, &queue_storage, &queue));

    /* 元素大小为 0 必须被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_QueueCreateStatic(4u, 0u, buffer, &queue_storage, &queue));

    /* 输出句柄为空必须被拒绝，避免调用方拿不到创建结果。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_QueueCreateStatic(4u, sizeof(uint32_t), buffer, &queue_storage, 0));
}

/**
 * @brief 验证静态创建队列会初始化计数和空间。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_queue_create_static_initializes_empty_queue();
 */
static void assert_queue_create_static_initializes_empty_queue(void)
{
    /* 定义队列控制块存储。 */
    MRT_Queue queue_storage;

    /* 定义队列数据缓冲区。 */
    uint8_t buffer[4u * sizeof(uint32_t)];

    /* 定义输出队列句柄。 */
    MRT_QueueHandle queue = 0;

    /* 创建一个可存放 4 个 uint32_t 的静态队列。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_QueueCreateStatic(4u, sizeof(uint32_t), buffer, &queue_storage, &queue));

    /* 创建成功后句柄应指向调用方提供的控制块。 */
    MRT_TEST_ASSERT_TRUE(queue == &queue_storage);

    /* 新队列没有任何消息。 */
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)MRT_QueueMessagesWaiting(queue));

    /* 新队列剩余空间等于容量。 */
    MRT_TEST_ASSERT_EQ_U32(4u, (unsigned)MRT_QueueSpacesAvailable(queue));
}

/**
 * @brief 运行静态队列创建测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证非法参数处理。 */
    assert_queue_create_static_rejects_invalid_arguments();

    /* 验证成功创建后的初始状态。 */
    assert_queue_create_static_initializes_empty_queue();

    /* 所有静态队列创建测试均通过。 */
    return 0;
}
