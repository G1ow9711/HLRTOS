#include "mrt_test.h"
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
 * @brief 验证 Peek 读取队头元素但不移除元素。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_peek_keeps_item_in_queue();
 */
static void assert_peek_keeps_item_in_queue(void)
{
    /* 定义队列控制块。 */
    MRT_Queue storage;

    /* 定义两个 uint32_t 槽位的队列缓冲区。 */
    uint8_t buffer[2u * sizeof(uint32_t)];

    /* 定义队列句柄。 */
    MRT_QueueHandle queue = 0;

    /* 创建测试队列。 */
    create_u32_queue(2u, &storage, buffer, &queue);

    /* 定义待发送值。 */
    uint32_t sent = 33u;

    /* 将元素发送到队列尾部。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_QueueSend(queue, &sent, 0u));

    /* 定义 Peek 输出变量。 */
    uint32_t peeked = 0u;

    /* 读取队头但不移除元素。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_QueuePeek(queue, &peeked, 0u));

    /* 验证 Peek 返回了队头值。 */
    MRT_TEST_ASSERT_EQ_U32(33u, peeked);

    /* 验证 Peek 后队列元素数量保持不变。 */
    MRT_TEST_ASSERT_EQ_U32(1u, (unsigned)MRT_QueueMessagesWaiting(queue));

    /* 定义接收输出变量。 */
    uint32_t received = 0u;

    /* 正常接收队头元素。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_QueueReceive(queue, &received, 0u));

    /* 验证 Peek 没有破坏后续接收顺序和值。 */
    MRT_TEST_ASSERT_EQ_U32(33u, received);
}

/**
 * @brief 验证 SendFront 将元素插入到队头。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_send_front_places_item_before_existing_tail();
 */
static void assert_send_front_places_item_before_existing_tail(void)
{
    /* 定义队列控制块。 */
    MRT_Queue storage;

    /* 定义两个 uint32_t 槽位的队列缓冲区。 */
    uint8_t buffer[2u * sizeof(uint32_t)];

    /* 定义队列句柄。 */
    MRT_QueueHandle queue = 0;

    /* 创建测试队列。 */
    create_u32_queue(2u, &storage, buffer, &queue);

    /* 定义先入队的尾部值。 */
    uint32_t tail_value = 10u;

    /* 定义后插入队头的值。 */
    uint32_t front_value = 20u;

    /* 先按普通发送写入一个元素。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_QueueSend(queue, &tail_value, 0u));

    /* 再把新元素插入队头。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_QueueSendFront(queue, &front_value, 0u));

    /* 定义接收输出变量。 */
    uint32_t received = 0u;

    /* 第一次接收应得到队头插入的元素。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_QueueReceive(queue, &received, 0u));

    /* 验证队头元素优先被接收。 */
    MRT_TEST_ASSERT_EQ_U32(20u, received);

    /* 第二次接收应得到原先的尾部元素。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_QueueReceive(queue, &received, 0u));

    /* 验证原有元素顺序被正确保留。 */
    MRT_TEST_ASSERT_EQ_U32(10u, received);
}

/**
 * @brief 验证单槽队列 Overwrite 会替换旧元素。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_one_slot_overwrite_replaces_old_value();
 */
static void assert_one_slot_overwrite_replaces_old_value(void)
{
    /* 定义队列控制块。 */
    MRT_Queue storage;

    /* 定义一个 uint32_t 槽位的队列缓冲区。 */
    uint8_t buffer[1u * sizeof(uint32_t)];

    /* 定义队列句柄。 */
    MRT_QueueHandle queue = 0;

    /* 创建容量为 1 的测试队列。 */
    create_u32_queue(1u, &storage, buffer, &queue);

    /* 定义旧值。 */
    uint32_t old_value = 1u;

    /* 定义新值。 */
    uint32_t new_value = 2u;

    /* 先写入旧值。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_QueueSend(queue, &old_value, 0u));

    /* 使用覆盖写入替换旧值。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_QueueOverwrite(queue, &new_value));

    /* 覆盖后队列仍应只有一个元素。 */
    MRT_TEST_ASSERT_EQ_U32(1u, (unsigned)MRT_QueueMessagesWaiting(queue));

    /* 定义接收输出变量。 */
    uint32_t received = 0u;

    /* 接收覆盖后的唯一元素。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_QueueReceive(queue, &received, 0u));

    /* 验证旧值被新值替换。 */
    MRT_TEST_ASSERT_EQ_U32(2u, received);
}

/**
 * @brief 验证多槽队列拒绝 Overwrite。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_multi_slot_overwrite_is_rejected();
 */
static void assert_multi_slot_overwrite_is_rejected(void)
{
    /* 定义队列控制块。 */
    MRT_Queue storage;

    /* 定义两个 uint32_t 槽位的队列缓冲区。 */
    uint8_t buffer[2u * sizeof(uint32_t)];

    /* 定义队列句柄。 */
    MRT_QueueHandle queue = 0;

    /* 创建容量为 2 的测试队列。 */
    create_u32_queue(2u, &storage, buffer, &queue);

    /* 定义覆盖写入值。 */
    uint32_t value = 7u;

    /* 多槽队列使用覆盖语义容易隐藏旧数据，当前 API 明确拒绝。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT, (unsigned)MRT_QueueOverwrite(queue, &value));
}

/**
 * @brief 验证 Reset 清空队列计数和读写位置。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_reset_clears_queue_state();
 */
static void assert_reset_clears_queue_state(void)
{
    /* 定义队列控制块。 */
    MRT_Queue storage;

    /* 定义两个 uint32_t 槽位的队列缓冲区。 */
    uint8_t buffer[2u * sizeof(uint32_t)];

    /* 定义队列句柄。 */
    MRT_QueueHandle queue = 0;

    /* 创建测试队列。 */
    create_u32_queue(2u, &storage, buffer, &queue);

    /* 定义第一个待发送值。 */
    uint32_t first = 3u;

    /* 定义第二个待发送值。 */
    uint32_t second = 4u;

    /* 写入第一个元素。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_QueueSend(queue, &first, 0u));

    /* 写入第二个元素。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_QueueSend(queue, &second, 0u));

    /* 重置队列。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_QueueReset(queue));

    /* 重置后队列应没有可读元素。 */
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)MRT_QueueMessagesWaiting(queue));

    /* 重置后所有槽位应重新可写。 */
    MRT_TEST_ASSERT_EQ_U32(2u, (unsigned)MRT_QueueSpacesAvailable(queue));

    /* 定义接收输出变量。 */
    uint32_t received = 0u;

    /* 重置后继续接收应得到空队列结果。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OBJECT_EMPTY, (unsigned)MRT_QueueReceive(queue, &received, 0u));
}

/**
 * @brief 运行队列变体 API 测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证 Peek 不移除队头元素。 */
    assert_peek_keeps_item_in_queue();

    /* 验证 SendFront 队头插入。 */
    assert_send_front_places_item_before_existing_tail();

    /* 验证单槽覆盖写入。 */
    assert_one_slot_overwrite_replaces_old_value();

    /* 验证多槽覆盖拒绝。 */
    assert_multi_slot_overwrite_is_rejected();

    /* 验证 Reset 清空队列。 */
    assert_reset_clears_queue_state();

    /* 所有队列变体测试均通过。 */
    return 0;
}
