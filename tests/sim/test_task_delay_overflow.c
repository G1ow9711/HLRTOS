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
 * @brief 验证任务延时跨越 tick 回绕后仍按正确 tick 数唤醒。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_task_delay_wakes_across_tick_overflow();
 */
static void assert_task_delay_wakes_across_tick_overflow(void)
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

    /* 启动调度器，当前任务应为高优先级任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelStart());
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == high_task);

    /* 将 tick 设置到即将溢出的位置。 */
    MRT_KernelTestSetTick(UINT32_MAX - 1u);

    /* 高优先级任务延时 3 tick，唤醒 tick 会回绕到 1。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskDelay(3u));
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == low_task);

    /* 第 1 tick：tick 到 UINT32_MAX，尚未到期。 */
    MRT_KernelTick();
    MRT_TEST_ASSERT_EQ_U32(UINT32_MAX, MRT_KernelGetTick());
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == low_task);

    /* 第 2 tick：tick 回绕到 0，仍未到期。 */
    MRT_KernelTick();
    MRT_TEST_ASSERT_EQ_U32(0u, MRT_KernelGetTick());
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == low_task);

    /* 第 3 tick：tick 到 1，延时到期，高优先级任务醒来。 */
    MRT_KernelTick();
    MRT_TEST_ASSERT_EQ_U32(1u, MRT_KernelGetTick());
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == high_task);
}

/**
 * @brief 运行 tick 溢出延时测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证跨 tick 溢出的延时唤醒。 */
    assert_task_delay_wakes_across_tick_overflow();

    /* 所有 tick 溢出延时测试均通过。 */
    return 0;
}
