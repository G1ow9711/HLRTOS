#include "mrt_test.h"
#include "myrtos/mrt_event_group.h"
#include "myrtos/mrt_kernel.h"
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
 * @brief 验证事件组等待不满足时会阻塞当前任务并在 tick 到期后超时唤醒。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_event_wait_blocks_and_times_out_current_task();
 */
static void assert_event_wait_blocks_and_times_out_current_task(void)
{
    /* 定义高优先级等待任务控制块。 */
    MRT_Task waiter_storage;

    /* 定义低优先级后备任务控制块。 */
    MRT_Task low_storage;

    /* 定义高优先级任务栈。 */
    MRT_StackType waiter_stack[128];

    /* 定义低优先级任务栈。 */
    MRT_StackType low_stack[128];

    /* 定义高优先级任务句柄。 */
    MRT_TaskHandle waiter_task = 0;

    /* 定义低优先级任务句柄。 */
    MRT_TaskHandle low_task = 0;

    /* 定义事件组控制块。 */
    MRT_EventGroup event_storage;

    /* 定义事件组句柄。 */
    MRT_EventGroupHandle events = 0;

    /* 初始化内核。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 创建低优先级后备任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("low", DummyTask, 0, 1u, low_stack, 128u, &low_storage, &low_task));

    /* 创建高优先级等待任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("waiter",
                                                          DummyTask,
                                                          0,
                                                          5u,
                                                          waiter_stack,
                                                          128u,
                                                          &waiter_storage,
                                                          &waiter_task));

    /* 创建空事件组。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_EventGroupCreateStatic(&event_storage, &events));

    /* 启动调度器后应先运行高优先级等待任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelStart());
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == waiter_task);

    /* 定义等待返回的 bit 快照。 */
    MRT_EventBits observed_bits = 0u;

    /* 等待 bit2，当前事件组为空，带 3 tick timeout 应阻塞当前任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_TIMEOUT,
                           (unsigned)MRT_EventGroupWaitBits(events, 0x4u, false, false, 3u, &observed_bits));

    /* host 仿真立即返回等待结局，返回快照应为阻塞前的空 bit 集。 */
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)observed_bits);

    /* 阻塞后低优先级任务应接管运行。 */
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == low_task);

    /* 高优先级等待任务应处于 blocked 状态。 */
    MRT_TaskState waiter_state = MRT_TASK_STATE_DELETED;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskGetState(waiter_task, &waiter_state));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_TASK_STATE_BLOCKED, (unsigned)waiter_state);

    /* 事件组等待链表应包含该等待任务。 */
    MRT_TEST_ASSERT_EQ_U32(1u, (unsigned)MRT_ListGetCount(&event_storage.waiting_tasks));

    /* 推进第 1 个 tick，等待尚未到期。 */
    MRT_KernelTick();
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == low_task);

    /* 推进第 2 个 tick，等待仍未到期。 */
    MRT_KernelTick();
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == low_task);

    /* 推进第 3 个 tick，等待任务应超时醒来并抢占低优先级任务。 */
    MRT_KernelTick();
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == waiter_task);

    /* 高优先级等待任务应恢复为 running 状态。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskGetState(waiter_task, &waiter_state));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_TASK_STATE_RUNNING, (unsigned)waiter_state);

    /* 超时清理后事件组等待链表应为空。 */
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)MRT_ListGetCount(&event_storage.waiting_tasks));

    /* 超时等待不应修改事件组 bit。 */
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)MRT_EventGroupGetBits(events));
}

/**
 * @brief 运行事件组 timeout 耦合测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证事件组等待 timeout 与任务调度、tick 唤醒的耦合行为。 */
    assert_event_wait_blocks_and_times_out_current_task();

    /* 所有事件组 timeout 耦合测试均通过。 */
    return 0;
}
