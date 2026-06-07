#include "mrt_test.h"
#include "myrtos/mrt_kernel.h"
#include "myrtos/mrt_task.h"
#include "myrtos/mrt_tickless.h"
#include "myrtos/mrt_timer.h"

/**
 * @brief 测试用空任务入口。
 * @param arg 用户参数，本测试不使用。
 * @return void 无返回值。
 * @example
 * DummyTask(NULL);
 */
static void DummyTask(void *arg)
{
    /* 显式丢弃未使用参数，避免开启 -Werror 时产生编译告警。 */
    (void)arg;
}

/**
 * @brief 测试用空定时器回调。
 * @param timer 到期的软件定时器句柄，本测试不使用。
 * @param arg 用户参数，本测试不使用。
 * @return void 无返回值。
 * @example
 * DummyTimerCallback(timer, NULL);
 */
static void DummyTimerCallback(MRT_TimerHandle timer, void *arg)
{
    /* 显式丢弃定时器句柄，保持回调签名完整。 */
    (void)timer;

    /* 显式丢弃用户参数，避免编译告警。 */
    (void)arg;
}

/**
 * @brief 验证无延时任务和无活动定时器时不建议进入 tickless 睡眠。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_no_deadline_returns_zero_idle_ticks();
 */
static void assert_no_deadline_returns_zero_idle_ticks(void)
{
    /* 初始化内核，清空任务、定时器和 tick 状态。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 定义输出变量并预置为非零，确认 API 会主动写入结果。 */
    MRT_Tick expected_idle_ticks = 99u;

    /* 查询期望空闲 tick 数。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TicklessGetExpectedIdleTicks(&expected_idle_ticks));

    /* 没有任何已知 deadline 时，host 模型返回 0，避免无界睡眠。 */
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)expected_idle_ticks);
}

/**
 * @brief 验证 tickless 会把阻塞任务的唤醒 tick 作为最近唤醒点。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_delayed_task_limits_expected_idle_ticks();
 */
static void assert_delayed_task_limits_expected_idle_ticks(void)
{
    /* 定义两个任务控制块，分别作为高低优先级任务。 */
    MRT_Task high_storage;
    MRT_Task low_storage;

    /* 定义两个任务栈。 */
    MRT_StackType high_stack[128];
    MRT_StackType low_stack[128];

    /* 定义任务句柄。 */
    MRT_TaskHandle high_task = 0;
    MRT_TaskHandle low_task = 0;

    /* 初始化内核状态。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 创建低优先级任务，供高优先级任务阻塞后继续运行。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("low", DummyTask, 0, 1u, low_stack, 128u, &low_storage, &low_task));

    /* 创建高优先级任务，启动后会成为当前任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("high", DummyTask, 0, 5u, high_stack, 128u, &high_storage, &high_task));

    /* 启动调度器，确认高优先级任务正在运行。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelStart());
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == high_task);

    /* 让当前高优先级任务阻塞 6 个 tick。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskDelay(6u));

    /* 查询期望空闲 tick 数，应等于最近任务唤醒距离。 */
    MRT_Tick expected_idle_ticks = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TicklessGetExpectedIdleTicks(&expected_idle_ticks));
    MRT_TEST_ASSERT_EQ_U32(6u, (unsigned)expected_idle_ticks);

    /* 推进一个 tick 后，剩余可空闲 tick 数应减少为 5。 */
    MRT_KernelTick();
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TicklessGetExpectedIdleTicks(&expected_idle_ticks));
    MRT_TEST_ASSERT_EQ_U32(5u, (unsigned)expected_idle_ticks);

    /* 使用低优先级任务句柄，避免变量只写不读。 */
    MRT_TEST_ASSERT_TRUE(low_task != 0);
}

/**
 * @brief 验证活动软件定时器会限制 tickless 预计睡眠时长。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_active_timer_limits_expected_idle_ticks();
 */
static void assert_active_timer_limits_expected_idle_ticks(void)
{
    /* 初始化内核状态。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 定义定时器控制块和句柄。 */
    MRT_Timer timer_storage;
    MRT_TimerHandle timer = 0;

    /* 创建 4 tick 后到期的单次定时器。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TimerCreateStatic("timer",
                                                           4u,
                                                           false,
                                                           0,
                                                           DummyTimerCallback,
                                                           &timer_storage,
                                                           &timer));

    /* 启动定时器，使其进入活动链表。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TimerStart(timer, 0u));

    /* 查询 tickless 预计空闲时长，应由定时器到期点限制为 4。 */
    MRT_Tick expected_idle_ticks = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TicklessGetExpectedIdleTicks(&expected_idle_ticks));
    MRT_TEST_ASSERT_EQ_U32(4u, (unsigned)expected_idle_ticks);
}

/**
 * @brief 验证任务和定时器同时存在时取最近 deadline。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_expected_idle_uses_nearest_deadline();
 */
static void assert_expected_idle_uses_nearest_deadline(void)
{
    /* 定义任务控制块、栈和句柄。 */
    MRT_Task high_storage;
    MRT_Task low_storage;
    MRT_StackType high_stack[128];
    MRT_StackType low_stack[128];
    MRT_TaskHandle high_task = 0;
    MRT_TaskHandle low_task = 0;

    /* 定义定时器控制块和句柄。 */
    MRT_Timer timer_storage;
    MRT_TimerHandle timer = 0;

    /* 初始化内核状态。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 创建低优先级任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("low", DummyTask, 0, 1u, low_stack, 128u, &low_storage, &low_task));

    /* 创建高优先级任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("high", DummyTask, 0, 5u, high_stack, 128u, &high_storage, &high_task));

    /* 启动调度器。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelStart());

    /* 让高优先级任务延时 8 tick。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskDelay(8u));

    /* 创建 3 tick 后到期的单次定时器。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TimerCreateStatic("timer",
                                                           3u,
                                                           false,
                                                           0,
                                                           DummyTimerCallback,
                                                           &timer_storage,
                                                           &timer));

    /* 启动定时器。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TimerStart(timer, 0u));

    /* 最近 deadline 是定时器 3 tick，而不是任务 8 tick。 */
    MRT_Tick expected_idle_ticks = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TicklessGetExpectedIdleTicks(&expected_idle_ticks));
    MRT_TEST_ASSERT_EQ_U32(3u, (unsigned)expected_idle_ticks);

    /* 使用任务句柄，避免变量只写不读。 */
    MRT_TEST_ASSERT_TRUE(low_task != 0);
}

/**
 * @brief 验证 tick 回绕时仍能计算正确的预计空闲 tick 数。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_expected_idle_handles_tick_wrap();
 */
static void assert_expected_idle_handles_tick_wrap(void)
{
    /* 定义任务控制块、栈和句柄。 */
    MRT_Task high_storage;
    MRT_Task low_storage;
    MRT_StackType high_stack[128];
    MRT_StackType low_stack[128];
    MRT_TaskHandle high_task = 0;
    MRT_TaskHandle low_task = 0;

    /* 初始化内核状态。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 将 tick 设置到接近 32 位回绕的位置。 */
    MRT_KernelTestSetTick(UINT32_MAX - 2u);

    /* 创建低优先级任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("low", DummyTask, 0, 1u, low_stack, 128u, &low_storage, &low_task));

    /* 创建高优先级任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("high", DummyTask, 0, 5u, high_stack, 128u, &high_storage, &high_task));

    /* 启动调度器。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelStart());

    /* 让高优先级任务延时 5 tick，唤醒点会自然回绕。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskDelay(5u));

    /* 查询预计空闲时长，仍应得到 5。 */
    MRT_Tick expected_idle_ticks = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TicklessGetExpectedIdleTicks(&expected_idle_ticks));
    MRT_TEST_ASSERT_EQ_U32(5u, (unsigned)expected_idle_ticks);

    /* 使用任务句柄，避免变量只写不读。 */
    MRT_TEST_ASSERT_TRUE(low_task != 0);
}

/**
 * @brief 运行 tickless 预计空闲时间测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时进程提前退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证无 deadline 场景。 */
    assert_no_deadline_returns_zero_idle_ticks();

    /* 验证任务延时限制 tickless 睡眠。 */
    assert_delayed_task_limits_expected_idle_ticks();

    /* 验证软件定时器限制 tickless 睡眠。 */
    assert_active_timer_limits_expected_idle_ticks();

    /* 验证多种 deadline 同时存在时选择最近者。 */
    assert_expected_idle_uses_nearest_deadline();

    /* 验证 tick 回绕计算。 */
    assert_expected_idle_handles_tick_wrap();

    /* 所有 tickless expected idle 测试均通过。 */
    return 0;
}
