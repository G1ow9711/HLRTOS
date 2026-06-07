#include "mrt_test.h"
#include "myrtos/mrt_kernel.h"
#include "myrtos/mrt_port.h"
#include "myrtos/mrt_task.h"
#include "myrtos/mrt_tickless.h"
#include "myrtos/mrt_timer.h"

/** @brief 测试定时器回调执行次数。 */
static uint32_t g_timer_callback_count;

/**
 * @brief 测试用空任务入口。
 * @param arg 用户参数，本测试不使用。
 * @return void 无返回值。
 * @example
 * DummyTask(NULL);
 */
static void DummyTask(void *arg)
{
    /* 显式丢弃未使用参数，避免编译告警。 */
    (void)arg;
}

/**
 * @brief 记录 tickless 补偿后定时器是否被执行。
 * @param timer 到期的软件定时器句柄，本测试不使用。
 * @param arg 用户参数，本测试不使用。
 * @return void 无返回值。
 * @example
 * CountTimerCallback(timer, NULL);
 */
static void CountTimerCallback(MRT_TimerHandle timer, void *arg)
{
    /* 显式丢弃定时器句柄。 */
    (void)timer;

    /* 显式丢弃用户参数。 */
    (void)arg;

    /* 记录回调执行次数。 */
    g_timer_callback_count++;
}

/**
 * @brief 验证 tickless 睡眠会按真实睡眠 tick 补偿软件定时器。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_tickless_sleep_compensates_timer_expiry();
 */
static void assert_tickless_sleep_compensates_timer_expiry(void)
{
    /* 初始化内核和端口 mock 状态。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 清空测试回调计数。 */
    g_timer_callback_count = 0u;

    /* 定义定时器控制块和句柄。 */
    MRT_Timer timer_storage;
    MRT_TimerHandle timer = 0;

    /* 创建 4 tick 后到期的单次定时器。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TimerCreateStatic("timer",
                                                           4u,
                                                           false,
                                                           0,
                                                           CountTimerCallback,
                                                           &timer_storage,
                                                           &timer));

    /* 启动定时器，让 tickless 查询到最近 deadline。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TimerStart(timer, 0u));

    /* 配置 mock 端口模拟实际睡眠 4 个 tick。 */
    MRT_PortMockSetSuppressedSleepTicks(4u);

    /* 进入 tickless idle，最大允许睡眠 10 tick，但应被定时器限制为 4 tick。 */
    MRT_Tick slept_ticks = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TicklessEnterIdle(10u, &slept_ticks));

    /* 端口层收到的睡眠请求应被限制为 4 tick。 */
    MRT_TEST_ASSERT_EQ_U32(1u, MRT_PortMockGetSuppressSleepCallCount());
    MRT_TEST_ASSERT_EQ_U32(4u, (unsigned)MRT_PortMockGetLastExpectedIdleTicks());

    /* 内核应补偿实际睡眠 4 tick。 */
    MRT_TEST_ASSERT_EQ_U32(4u, (unsigned)slept_ticks);
    MRT_TEST_ASSERT_EQ_U32(4u, (unsigned)MRT_KernelGetTick());

    /* 软件定时器应在补偿到第 4 tick 时执行回调。 */
    MRT_TEST_ASSERT_EQ_U32(1u, (unsigned)g_timer_callback_count);
}

/**
 * @brief 验证 tickless 睡眠会按真实睡眠 tick 唤醒延时任务。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_tickless_sleep_compensates_task_wakeup();
 */
static void assert_tickless_sleep_compensates_task_wakeup(void)
{
    /* 定义高低优先级任务控制块。 */
    MRT_Task high_storage;
    MRT_Task low_storage;

    /* 定义高低优先级任务栈。 */
    MRT_StackType high_stack[128];
    MRT_StackType low_stack[128];

    /* 定义任务句柄。 */
    MRT_TaskHandle high_task = 0;
    MRT_TaskHandle low_task = 0;

    /* 初始化内核和端口 mock 状态。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 创建低优先级任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("low", DummyTask, 0, 1u, low_stack, 128u, &low_storage, &low_task));

    /* 创建高优先级任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("high", DummyTask, 0, 5u, high_stack, 128u, &high_storage, &high_task));

    /* 启动调度器，高优先级任务成为当前任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelStart());
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == high_task);

    /* 高优先级任务延时 3 tick，当前任务切到低优先级任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskDelay(3u));
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == low_task);

    /* 配置 mock 端口模拟真实睡眠 3 tick。 */
    MRT_PortMockSetSuppressedSleepTicks(3u);

    /* 进入 tickless idle。 */
    MRT_Tick slept_ticks = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TicklessEnterIdle(10u, &slept_ticks));

    /* 内核 tick 和返回值应反映真实睡眠时长。 */
    MRT_TEST_ASSERT_EQ_U32(3u, (unsigned)slept_ticks);
    MRT_TEST_ASSERT_EQ_U32(3u, (unsigned)MRT_KernelGetTick());

    /* 延时到期后，高优先级任务应被唤醒并重新成为当前任务。 */
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == high_task);
}

/**
 * @brief 验证调用方给出的最大睡眠 tick 会限制端口睡眠请求。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_tickless_idle_honors_max_sleep_ticks();
 */
static void assert_tickless_idle_honors_max_sleep_ticks(void)
{
    /* 初始化内核和端口 mock 状态。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 清空测试回调计数。 */
    g_timer_callback_count = 0u;

    /* 定义定时器控制块和句柄。 */
    MRT_Timer timer_storage;
    MRT_TimerHandle timer = 0;

    /* 创建 5 tick 后到期的单次定时器。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TimerCreateStatic("timer",
                                                           5u,
                                                           false,
                                                           0,
                                                           CountTimerCallback,
                                                           &timer_storage,
                                                           &timer));

    /* 启动定时器。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TimerStart(timer, 0u));

    /* 配置 mock 端口模拟实际睡眠 2 tick。 */
    MRT_PortMockSetSuppressedSleepTicks(2u);

    /* 调用方只允许睡眠 2 tick，即使最近 deadline 是 5 tick。 */
    MRT_Tick slept_ticks = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TicklessEnterIdle(2u, &slept_ticks));

    /* 端口层请求应被 max_ticks 限制为 2。 */
    MRT_TEST_ASSERT_EQ_U32(1u, MRT_PortMockGetSuppressSleepCallCount());
    MRT_TEST_ASSERT_EQ_U32(2u, (unsigned)MRT_PortMockGetLastExpectedIdleTicks());

    /* 内核只补偿 2 tick，定时器还未到期。 */
    MRT_TEST_ASSERT_EQ_U32(2u, (unsigned)slept_ticks);
    MRT_TEST_ASSERT_EQ_U32(2u, (unsigned)MRT_KernelGetTick());
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)g_timer_callback_count);

    /* 测试结束前停止仍处于活动状态的栈上定时器，避免后续内核重初始化访问失效控制块。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TimerStop(timer, 0u));
}

/**
 * @brief 验证没有明确 deadline 时 tickless 不调用端口睡眠。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_tickless_idle_skips_sleep_without_deadline();
 */
static void assert_tickless_idle_skips_sleep_without_deadline(void)
{
    /* 初始化内核和端口 mock 状态。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 即使 mock 设置了可睡眠 tick，没有 deadline 时也不能进入端口睡眠。 */
    MRT_PortMockSetSuppressedSleepTicks(5u);

    /* 调用 tickless idle。 */
    MRT_Tick slept_ticks = 99u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TicklessEnterIdle(10u, &slept_ticks));

    /* 未知 deadline 场景下不调用端口睡眠，返回实际睡眠 0。 */
    MRT_TEST_ASSERT_EQ_U32(0u, MRT_PortMockGetSuppressSleepCallCount());
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)slept_ticks);
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)MRT_KernelGetTick());
}

/**
 * @brief 运行 tickless 睡眠补偿耦合测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时进程提前退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证软件定时器补偿。 */
    assert_tickless_sleep_compensates_timer_expiry();

    /* 验证任务延时唤醒补偿。 */
    assert_tickless_sleep_compensates_task_wakeup();

    /* 验证调用方最大睡眠限制。 */
    assert_tickless_idle_honors_max_sleep_ticks();

    /* 验证无 deadline 时跳过端口睡眠。 */
    assert_tickless_idle_skips_sleep_without_deadline();

    /* 所有 tickless 补偿测试均通过。 */
    return 0;
}
