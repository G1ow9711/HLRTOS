#include "mrt_test.h"
#include "myrtos/mrt_port.h"
#include "myrtos/mrt_queue.h"

/**
 * @brief 创建一个测试用 uint32_t 队列。
 * @param capacity 队列容量，必须大于 0。
 * @param storage 队列控制块存储，不能为空。
 * @param buffer 队列数据缓冲区，不能为空。
 * @param out_queue 输出队列句柄，不能为空。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * create_u32_queue(2, &queue_storage, buffer, &queue);
 */
static void create_u32_queue(size_t capacity, MRT_Queue *storage, uint8_t *buffer, MRT_QueueHandle *out_queue)
{
    /* 使用固定 uint32_t 元素大小创建测试队列。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_QueueCreateStatic(capacity, sizeof(uint32_t), buffer, storage, out_queue));
}

/**
 * @brief 验证 ISR 发送和 ISR 接收能完成一次非阻塞 FIFO 传递。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_isr_send_receive_transfers_one_item();
 */
static void assert_isr_send_receive_transfers_one_item(void)
{
    /* 初始化 mock 端口状态。 */
    MRT_PortInitialize();

    /* 切换到模拟 ISR 上下文。 */
    MRT_PortMockSetInsideISR(true);

    /* 定义队列控制块。 */
    MRT_Queue storage;

    /* 定义两个 uint32_t 槽位的队列缓冲区。 */
    uint8_t buffer[2u * sizeof(uint32_t)];

    /* 定义队列句柄。 */
    MRT_QueueHandle queue = 0;

    /* 创建测试队列。 */
    create_u32_queue(2u, &storage, buffer, &queue);

    /* 定义 ISR 待发送值。 */
    uint32_t sent = 99u;

    /* 预置 yield 标志为 true，用于确认 API 会明确写回 false。 */
    bool should_yield = true;

    /* 从 ISR 上下文发送一个元素。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_QueueSendFromISR(queue, &sent, &should_yield));

    /* 当前尚未接入等待任务，ISR 发送不应请求切换。 */
    MRT_TEST_ASSERT_TRUE(!should_yield);

    /* 验证队列已有一个消息。 */
    MRT_TEST_ASSERT_EQ_U32(1u, (unsigned)MRT_QueueMessagesWaiting(queue));

    /* 定义 ISR 接收输出变量。 */
    uint32_t received = 0u;

    /* 再次预置 yield 标志为 true。 */
    should_yield = true;

    /* 从 ISR 上下文接收一个元素。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_QueueReceiveFromISR(queue, &received, &should_yield));

    /* 当前尚未接入等待发送任务，ISR 接收不应请求切换。 */
    MRT_TEST_ASSERT_TRUE(!should_yield);

    /* 验证接收值等于发送值。 */
    MRT_TEST_ASSERT_EQ_U32(99u, received);
}

/**
 * @brief 验证 ISR 发送满队列时立即返回满状态。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_isr_send_full_queue_returns_full();
 */
static void assert_isr_send_full_queue_returns_full(void)
{
    /* 初始化 mock 端口状态。 */
    MRT_PortInitialize();

    /* 切换到模拟 ISR 上下文。 */
    MRT_PortMockSetInsideISR(true);

    /* 定义队列控制块。 */
    MRT_Queue storage;

    /* 定义一个 uint32_t 槽位的队列缓冲区。 */
    uint8_t buffer[1u * sizeof(uint32_t)];

    /* 定义队列句柄。 */
    MRT_QueueHandle queue = 0;

    /* 创建容量为 1 的测试队列。 */
    create_u32_queue(1u, &storage, buffer, &queue);

    /* 定义测试值。 */
    uint32_t value = 5u;

    /* 先填满唯一槽位。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_QueueSend(queue, &value, 0u));

    /* 预置 yield 标志为 true。 */
    bool should_yield = true;

    /* ISR 再发送时应立即返回满状态。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OBJECT_FULL,
                           (unsigned)MRT_QueueSendFromISR(queue, &value, &should_yield));

    /* 队列满失败不会唤醒任何任务。 */
    MRT_TEST_ASSERT_TRUE(!should_yield);
}

/**
 * @brief 验证 ISR 接收空队列时立即返回空状态。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_isr_receive_empty_queue_returns_empty();
 */
static void assert_isr_receive_empty_queue_returns_empty(void)
{
    /* 初始化 mock 端口状态。 */
    MRT_PortInitialize();

    /* 切换到模拟 ISR 上下文。 */
    MRT_PortMockSetInsideISR(true);

    /* 定义队列控制块。 */
    MRT_Queue storage;

    /* 定义一个 uint32_t 槽位的队列缓冲区。 */
    uint8_t buffer[1u * sizeof(uint32_t)];

    /* 定义队列句柄。 */
    MRT_QueueHandle queue = 0;

    /* 创建容量为 1 的测试队列。 */
    create_u32_queue(1u, &storage, buffer, &queue);

    /* 定义接收输出变量。 */
    uint32_t received = 0u;

    /* 预置 yield 标志为 true。 */
    bool should_yield = true;

    /* ISR 接收空队列应立即返回空状态。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OBJECT_EMPTY,
                           (unsigned)MRT_QueueReceiveFromISR(queue, &received, &should_yield));

    /* 队列空失败不会唤醒任何任务。 */
    MRT_TEST_ASSERT_TRUE(!should_yield);
}

/**
 * @brief 验证 FromISR API 在任务上下文调用时返回非法上下文。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_from_isr_rejects_task_context();
 */
static void assert_from_isr_rejects_task_context(void)
{
    /* 初始化 mock 端口状态，默认处于任务上下文。 */
    MRT_PortInitialize();

    /* 显式保持任务上下文。 */
    MRT_PortMockSetInsideISR(false);

    /* 定义队列控制块。 */
    MRT_Queue storage;

    /* 定义一个 uint32_t 槽位的队列缓冲区。 */
    uint8_t buffer[1u * sizeof(uint32_t)];

    /* 定义队列句柄。 */
    MRT_QueueHandle queue = 0;

    /* 创建容量为 1 的测试队列。 */
    create_u32_queue(1u, &storage, buffer, &queue);

    /* 定义测试值。 */
    uint32_t value = 8u;

    /* 预置 yield 标志为 true。 */
    bool should_yield = true;

    /* 任务上下文调用 ISR 发送接口应被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_CONTEXT,
                           (unsigned)MRT_QueueSendFromISR(queue, &value, &should_yield));

    /* 非法上下文不会触发调度切换。 */
    MRT_TEST_ASSERT_TRUE(!should_yield);

    /* 任务上下文调用 ISR 接收接口也应被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_CONTEXT,
                           (unsigned)MRT_QueueReceiveFromISR(queue, &value, &should_yield));

    /* 非法上下文仍不触发调度切换。 */
    MRT_TEST_ASSERT_TRUE(!should_yield);
}

/**
 * @brief 运行队列 ISR API 测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证 ISR 非阻塞收发。 */
    assert_isr_send_receive_transfers_one_item();

    /* 验证 ISR 满队列发送。 */
    assert_isr_send_full_queue_returns_full();

    /* 验证 ISR 空队列接收。 */
    assert_isr_receive_empty_queue_returns_empty();

    /* 验证 ISR API 上下文限制。 */
    assert_from_isr_rejects_task_context();

    /* 所有 ISR 队列测试均通过。 */
    return 0;
}
