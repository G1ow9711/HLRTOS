#include "mrt_test.h"
#include "myrtos/mrt_kernel.h"
#include "myrtos/mrt_port.h"
#include "myrtos/mrt_queue.h"
#include "myrtos/mrt_task.h"

/**
 * @brief 测试用任务入口函数。
 * @param arg 用户参数，本测试不使用。
 * @return void 无返回值。
 * @example
 * DummyTask(NULL);
 */
static void DummyTask(void *arg)
{
    /* 显式丢弃未使用参数，避免编译器告警。 */
    (void)arg;
}

/**
 * @brief 验证队列发送会唤醒正在等待接收的高优先级任务。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_queue_send_wakes_high_priority_receiver();
 */
static void assert_queue_send_wakes_high_priority_receiver(void)
{
    /* 定义高优先级接收任务控制块。 */
    MRT_Task receiver_storage;

    /* 定义低优先级发送任务控制块。 */
    MRT_Task sender_storage;

    /* 定义高优先级任务栈。 */
    MRT_StackType receiver_stack[128];

    /* 定义低优先级任务栈。 */
    MRT_StackType sender_stack[128];

    /* 定义高优先级任务句柄。 */
    MRT_TaskHandle receiver_task = 0;

    /* 定义低优先级任务句柄。 */
    MRT_TaskHandle sender_task = 0;

    /* 定义队列控制块。 */
    MRT_Queue queue_storage;

    /* 定义一个 uint32_t 槽位的队列缓冲区。 */
    uint8_t queue_buffer[1u * sizeof(uint32_t)];

    /* 定义队列句柄。 */
    MRT_QueueHandle queue = 0;

    /* 初始化内核。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 创建低优先级发送任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("sender",
                                                          DummyTask,
                                                          0,
                                                          1u,
                                                          sender_stack,
                                                          128u,
                                                          &sender_storage,
                                                          &sender_task));

    /* 创建高优先级接收任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("receiver",
                                                          DummyTask,
                                                          0,
                                                          5u,
                                                          receiver_stack,
                                                          128u,
                                                          &receiver_storage,
                                                          &receiver_task));

    /* 创建空队列。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_QueueCreateStatic(1u,
                                                           sizeof(uint32_t),
                                                           queue_buffer,
                                                           &queue_storage,
                                                           &queue));

    /* 启动调度器后应先运行高优先级接收任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelStart());
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == receiver_task);

    /* 定义接收输出变量。 */
    uint32_t received = 0u;

    /* 高优先级任务等待空队列，进入队列接收等待链表。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_TIMEOUT, (unsigned)MRT_QueueReceive(queue, &received, 20u));

    /* 阻塞后低优先级发送任务应成为当前任务。 */
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == sender_task);

    /* 队列接收等待链表应包含高优先级接收任务。 */
    MRT_TEST_ASSERT_EQ_U32(1u, (unsigned)MRT_ListGetCount(&queue_storage.waiting_receivers));

    /* 定义发送值。 */
    uint32_t sent = 123u;

    /* 低优先级任务发送数据，应唤醒高优先级接收任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_QueueSend(queue, &sent, 0u));

    /* 发送后高优先级接收任务应抢占成为当前任务。 */
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == receiver_task);

    /* 队列接收等待链表应被清空。 */
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)MRT_ListGetCount(&queue_storage.waiting_receivers));

    /* 高优先级接收任务应处于 running 状态。 */
    MRT_TaskState receiver_state = MRT_TASK_STATE_DELETED;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskGetState(receiver_task, &receiver_state));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_TASK_STATE_RUNNING, (unsigned)receiver_state);

    /* 被唤醒的接收任务再次接收，应取得低优先级任务发送的数据。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_QueueReceive(queue, &received, 0u));

    /* 验证接收数据正确。 */
    MRT_TEST_ASSERT_EQ_U32(123u, received);

    /* 队列应恢复为空。 */
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)MRT_QueueMessagesWaiting(queue));
}

/**
 * @brief 验证 ISR 发送唤醒接收任务时只设置 yield 请求，不在 ISR 内立即切换当前任务。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_queue_send_from_isr_marks_yield_when_receiver_woken();
 */
static void assert_queue_send_from_isr_marks_yield_when_receiver_woken(void)
{
    /* 定义高优先级接收任务控制块。 */
    MRT_Task receiver_storage;

    /* 定义低优先级发送任务控制块。 */
    MRT_Task sender_storage;

    /* 定义高优先级任务栈。 */
    MRT_StackType receiver_stack[128];

    /* 定义低优先级任务栈。 */
    MRT_StackType sender_stack[128];

    /* 定义高优先级任务句柄。 */
    MRT_TaskHandle receiver_task = 0;

    /* 定义低优先级任务句柄。 */
    MRT_TaskHandle sender_task = 0;

    /* 定义队列控制块。 */
    MRT_Queue queue_storage;

    /* 定义一个 uint32_t 槽位的队列缓冲区。 */
    uint8_t queue_buffer[1u * sizeof(uint32_t)];

    /* 定义队列句柄。 */
    MRT_QueueHandle queue = 0;

    /* 初始化内核和端口 mock。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 创建低优先级发送任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("sender",
                                                          DummyTask,
                                                          0,
                                                          1u,
                                                          sender_stack,
                                                          128u,
                                                          &sender_storage,
                                                          &sender_task));

    /* 创建高优先级接收任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("receiver",
                                                          DummyTask,
                                                          0,
                                                          5u,
                                                          receiver_stack,
                                                          128u,
                                                          &receiver_storage,
                                                          &receiver_task));

    /* 创建空队列。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_QueueCreateStatic(1u,
                                                           sizeof(uint32_t),
                                                           queue_buffer,
                                                           &queue_storage,
                                                           &queue));

    /* 启动调度器后应先运行高优先级接收任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelStart());
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == receiver_task);

    /* 定义接收输出变量。 */
    uint32_t received = 0u;

    /* 高优先级任务等待空队列并进入 blocked。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_TIMEOUT, (unsigned)MRT_QueueReceive(queue, &received, 20u));

    /* 当前任务应切换到低优先级发送任务。 */
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == sender_task);

    /* 切换到模拟 ISR 上下文。 */
    MRT_PortMockSetInsideISR(true);

    /* 定义 ISR 发送值。 */
    uint32_t sent = 77u;

    /* 定义 ISR yield 输出标志。 */
    bool should_yield = false;

    /* ISR 发送应唤醒等待接收任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_QueueSendFromISR(queue, &sent, &should_yield));

    /* ISR API 不直接切换任务，但应告诉端口退出 ISR 后切换。 */
    MRT_TEST_ASSERT_TRUE(should_yield);

    /* 当前任务在 ISR 内不应立即改变。 */
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == sender_task);

    /* 等待接收链表应被清空。 */
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)MRT_ListGetCount(&queue_storage.waiting_receivers));

    /* 接收任务应回到 ready 状态，等待 ISR 退出后的调度切换。 */
    MRT_TaskState receiver_state = MRT_TASK_STATE_DELETED;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskGetState(receiver_task, &receiver_state));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_TASK_STATE_READY, (unsigned)receiver_state);

    /* 发送的数据应保留在队列中等待接收任务恢复后读取。 */
    MRT_TEST_ASSERT_EQ_U32(1u, (unsigned)MRT_QueueMessagesWaiting(queue));

    /* 恢复任务上下文，避免影响后续测试。 */
    MRT_PortMockSetInsideISR(false);
}

/**
 * @brief 运行队列发送唤醒接收任务耦合测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证队列发送会唤醒高优先级等待接收任务。 */
    assert_queue_send_wakes_high_priority_receiver();

    /* 验证 ISR 队列发送设置延迟切换请求。 */
    assert_queue_send_from_isr_marks_yield_when_receiver_woken();

    /* 所有队列发送唤醒测试均通过。 */
    return 0;
}
