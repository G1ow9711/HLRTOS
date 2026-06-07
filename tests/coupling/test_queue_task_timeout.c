#include "mrt_test.h"
#include "myrtos/mrt_kernel.h"
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
 * @brief 验证空队列带 timeout 接收会阻塞当前任务并在 tick 到期后超时唤醒。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_queue_receive_blocks_and_times_out_current_task();
 */
static void assert_queue_receive_blocks_and_times_out_current_task(void)
{
    /* 定义高优先级接收任务控制块。 */
    MRT_Task receiver_storage;

    /* 定义低优先级后备任务控制块。 */
    MRT_Task low_storage;

    /* 定义高优先级任务栈。 */
    MRT_StackType receiver_stack[128];

    /* 定义低优先级任务栈。 */
    MRT_StackType low_stack[128];

    /* 定义高优先级任务句柄。 */
    MRT_TaskHandle receiver_task = 0;

    /* 定义低优先级任务句柄。 */
    MRT_TaskHandle low_task = 0;

    /* 定义队列控制块。 */
    MRT_Queue queue_storage;

    /* 定义一个 uint32_t 槽位的队列缓冲区。 */
    uint8_t queue_buffer[1u * sizeof(uint32_t)];

    /* 定义队列句柄。 */
    MRT_QueueHandle queue = 0;

    /* 初始化内核，清空 tick、ready list 和端口 mock 状态。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 创建低优先级后备任务，用于高优先级任务阻塞后的调度接管。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("low",
                                                          DummyTask,
                                                          0,
                                                          1u,
                                                          low_stack,
                                                          128u,
                                                          &low_storage,
                                                          &low_task));

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

    /* 验证当前任务为高优先级接收任务。 */
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == receiver_task);

    /* 定义接收输出变量。 */
    uint32_t received = 0u;

    /* 空队列带 3 tick timeout 接收应进入等待，并在仿真 API 中返回超时结果。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_TIMEOUT, (unsigned)MRT_QueueReceive(queue, &received, 3u));

    /* 接收任务阻塞后，当前任务应切换为低优先级后备任务。 */
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == low_task);

    /* 高优先级接收任务应处于 blocked 状态。 */
    MRT_TaskState receiver_state = MRT_TASK_STATE_DELETED;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskGetState(receiver_task, &receiver_state));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_TASK_STATE_BLOCKED, (unsigned)receiver_state);

    /* 队列等待接收链表应包含该接收任务。 */
    MRT_TEST_ASSERT_EQ_U32(1u, (unsigned)MRT_ListGetCount(&queue_storage.waiting_receivers));

    /* 推进第 1 个 tick，等待尚未到期。 */
    MRT_KernelTick();
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == low_task);

    /* 推进第 2 个 tick，等待仍未到期。 */
    MRT_KernelTick();
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == low_task);

    /* 推进第 3 个 tick，接收任务应超时醒来并抢占低优先级任务。 */
    MRT_KernelTick();
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == receiver_task);

    /* 高优先级接收任务应恢复为 running 状态。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskGetState(receiver_task, &receiver_state));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_TASK_STATE_RUNNING, (unsigned)receiver_state);

    /* 超时醒来后队列等待接收链表应被清空。 */
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)MRT_ListGetCount(&queue_storage.waiting_receivers));

    /* 超时接收不应凭空产生队列消息。 */
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)MRT_QueueMessagesWaiting(queue));
}

/**
 * @brief 运行队列接收 timeout 耦合测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证队列接收 timeout 与任务调度、tick 唤醒的耦合行为。 */
    assert_queue_receive_blocks_and_times_out_current_task();

    /* 所有队列 timeout 耦合测试均通过。 */
    return 0;
}
