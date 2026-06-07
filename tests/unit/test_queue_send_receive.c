#include "mrt_test.h"
#include "myrtos/mrt_queue.h"

/**
 * @brief 创建一个测试用 uint32_t 队列。
 * @param storage 队列控制块存储，不能为空。
 * @param buffer 队列数据缓冲区，不能为空。
 * @param out_queue 输出队列句柄，不能为空。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * create_u32_queue(&queue_storage, buffer, &queue);
 */
static void create_u32_queue(MRT_Queue *storage, uint8_t *buffer, MRT_QueueHandle *out_queue)
{
    /* 创建容量为 2 的 uint32_t 测试队列。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_QueueCreateStatic(2u, sizeof(uint32_t), buffer, storage, out_queue));
}

/**
 * @brief 验证队列按 FIFO 顺序发送和接收元素。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_queue_send_receive_preserves_fifo_order();
 */
static void assert_queue_send_receive_preserves_fifo_order(void)
{
    /* 定义队列控制块。 */
    MRT_Queue storage;

    /* 定义队列缓冲区。 */
    uint8_t buffer[2u * sizeof(uint32_t)];

    /* 定义队列句柄。 */
    MRT_QueueHandle queue = 0;

    /* 创建测试队列。 */
    create_u32_queue(&storage, buffer, &queue);

    /* 定义第一个发送值。 */
    uint32_t first = 11u;

    /* 定义第二个发送值。 */
    uint32_t second = 22u;

    /* 发送第一个值。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_QueueSend(queue, &first, 0u));

    /* 发送第二个值。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_QueueSend(queue, &second, 0u));

    /* 验证队列已有 2 个消息。 */
    MRT_TEST_ASSERT_EQ_U32(2u, (unsigned)MRT_QueueMessagesWaiting(queue));

    /* 定义接收输出变量。 */
    uint32_t received = 0u;

    /* 接收第一个值。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_QueueReceive(queue, &received, 0u));

    /* 验证第一个接收值保持 FIFO 顺序。 */
    MRT_TEST_ASSERT_EQ_U32(11u, received);

    /* 接收第二个值。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_QueueReceive(queue, &received, 0u));

    /* 验证第二个接收值保持 FIFO 顺序。 */
    MRT_TEST_ASSERT_EQ_U32(22u, received);

    /* 接收完后队列应为空。 */
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)MRT_QueueMessagesWaiting(queue));
}

/**
 * @brief 验证满队列非阻塞发送返回满状态。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_send_to_full_queue_returns_full();
 */
static void assert_send_to_full_queue_returns_full(void)
{
    /* 定义队列控制块。 */
    MRT_Queue storage;

    /* 定义队列缓冲区。 */
    uint8_t buffer[2u * sizeof(uint32_t)];

    /* 定义队列句柄。 */
    MRT_QueueHandle queue = 0;

    /* 创建测试队列。 */
    create_u32_queue(&storage, buffer, &queue);

    /* 定义测试值。 */
    uint32_t value = 1u;

    /* 填满第一个槽位。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_QueueSend(queue, &value, 0u));

    /* 填满第二个槽位。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_QueueSend(queue, &value, 0u));

    /* 再次非阻塞发送应返回对象已满。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OBJECT_FULL, (unsigned)MRT_QueueSend(queue, &value, 0u));
}

/**
 * @brief 验证空队列非阻塞接收返回空状态。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_receive_from_empty_queue_returns_empty();
 */
static void assert_receive_from_empty_queue_returns_empty(void)
{
    /* 定义队列控制块。 */
    MRT_Queue storage;

    /* 定义队列缓冲区。 */
    uint8_t buffer[2u * sizeof(uint32_t)];

    /* 定义队列句柄。 */
    MRT_QueueHandle queue = 0;

    /* 创建测试队列。 */
    create_u32_queue(&storage, buffer, &queue);

    /* 定义接收输出变量。 */
    uint32_t received = 0u;

    /* 空队列非阻塞接收应返回对象为空。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OBJECT_EMPTY, (unsigned)MRT_QueueReceive(queue, &received, 0u));
}

/**
 * @brief 验证当前阶段非零 timeout 暂时返回 timeout。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_nonzero_timeout_returns_timeout_until_blocking_exists();
 */
static void assert_nonzero_timeout_returns_timeout_until_blocking_exists(void)
{
    /* 定义队列控制块。 */
    MRT_Queue storage;

    /* 定义队列缓冲区。 */
    uint8_t buffer[2u * sizeof(uint32_t)];

    /* 定义队列句柄。 */
    MRT_QueueHandle queue = 0;

    /* 创建测试队列。 */
    create_u32_queue(&storage, buffer, &queue);

    /* 定义接收输出变量。 */
    uint32_t received = 0u;

    /* 当前任务阻塞机制尚未接入队列，非零 timeout 先返回超时。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_TIMEOUT, (unsigned)MRT_QueueReceive(queue, &received, 5u));
}

/**
 * @brief 运行队列非阻塞收发测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证 FIFO 收发。 */
    assert_queue_send_receive_preserves_fifo_order();

    /* 验证满队列发送。 */
    assert_send_to_full_queue_returns_full();

    /* 验证空队列接收。 */
    assert_receive_from_empty_queue_returns_empty();

    /* 验证非零 timeout 临时行为。 */
    assert_nonzero_timeout_returns_timeout_until_blocking_exists();

    /* 所有非阻塞收发测试均通过。 */
    return 0;
}
