#include "mrt_test.h"
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
 * @brief 验证当前任务延时后会阻塞并在 tick 到期后重新就绪。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_task_delay_blocks_and_wakes_on_tick();
 */
static void assert_task_delay_blocks_and_wakes_on_tick(void)
{
    /* 定义高优先级任务控制块。 */
    MRT_Task high_storage;

    /* 定义低优先级任务控制块。 */
    MRT_Task low_storage;

    /* 定义高优先级任务栈。 */
    MRT_StackType high_stack[128];

    /* 定义低优先级任务栈。 */
    MRT_StackType low_stack[128];

    /* 定义高优先级任务句柄。 */
    MRT_TaskHandle high_task = 0;

    /* 定义低优先级任务句柄。 */
    MRT_TaskHandle low_task = 0;

    /* 初始化内核。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 创建低优先级任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("low", DummyTask, 0, 1u, low_stack, 128u, &low_storage, &low_task));

    /* 创建高优先级任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("high", DummyTask, 0, 5u, high_stack, 128u, &high_storage, &high_task));

    /* 启动后当前任务应为高优先级任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelStart());
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == high_task);

    /* 当前高优先级任务延时 3 tick。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskDelay(3u));

    /* 延时后当前任务应切换到低优先级任务。 */
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == low_task);

    /* 高优先级任务应处于 blocked 状态。 */
    MRT_TaskState high_state = MRT_TASK_STATE_DELETED;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskGetState(high_task, &high_state));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_TASK_STATE_BLOCKED, (unsigned)high_state);

    /* 推进第 1 个 tick，高优先级任务还未到期。 */
    MRT_KernelTick();
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == low_task);

    /* 推进第 2 个 tick，高优先级任务仍未到期。 */
    MRT_KernelTick();
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == low_task);

    /* 推进第 3 个 tick，高优先级任务应醒来并抢占低优先级任务。 */
    MRT_KernelTick();
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == high_task);

    /* 高优先级任务状态应回到 running。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskGetState(high_task, &high_state));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_TASK_STATE_RUNNING, (unsigned)high_state);
}

/**
 * @brief 运行任务延时测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证任务延时阻塞和 tick 唤醒。 */
    assert_task_delay_blocks_and_wakes_on_tick();

    /* 所有任务延时测试均通过。 */
    return 0;
}
