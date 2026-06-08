#include "mrt_test.h"
#include "myrtos/mrt_kernel.h"
#include "myrtos/mrt_timer.h"

/** @brief 测试定时器回调执行次数。 */
static uint32_t g_callback_count;

/** @brief 测试回调记录的最后一个定时器句柄。 */
static MRT_TimerHandle g_last_timer;

/**
 * @brief 清空定时器服务任务测试观测状态。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * ResetTimerServiceRecords();
 */
static void ResetTimerServiceRecords(void)
{
    /* 清空回调执行次数。 */
    g_callback_count = 0u;

    /* 清空最后回调定时器句柄。 */
    g_last_timer = 0;
}

/**
 * @brief 测试用软件定时器回调。
 * @param timer 到期的软件定时器句柄。
 * @param arg 用户参数，本测试不使用。
 * @return void 无返回值。
 * @example
 * TimerCallback(timer, NULL);
 */
static void TimerCallback(MRT_TimerHandle timer, void *arg)
{
    /* 显式丢弃未使用参数。 */
    (void)arg;

    /* 记录回调执行次数。 */
    g_callback_count++;

    /* 记录本次到期定时器句柄。 */
    g_last_timer = timer;
}

/**
 * @brief 验证启动命令只入队，服务任务执行后才激活定时器。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_timer_start_command_waits_for_service_task();
 */
static void assert_timer_start_command_waits_for_service_task(void)
{
    /* 初始化内核和定时器服务队列。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 清空测试观测状态。 */
    ResetTimerServiceRecords();

    /* 定义静态定时器控制块。 */
    MRT_Timer storage;

    /* 定义定时器句柄。 */
    MRT_TimerHandle timer = 0;

    /* 创建 5 tick 单次定时器。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TimerCreateStatic("svc-start",
                                                           5u,
                                                           false,
                                                           0,
                                                           TimerCallback,
                                                           &storage,
                                                           &timer));

    /* 启动 API 只投递命令到服务队列。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TimerStart(timer, 0u));

    /* 服务任务尚未运行前，定时器不能立即变为活动。 */
    bool active = true;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TimerIsActive(timer, &active));
    MRT_TEST_ASSERT_TRUE(!active);

    /* 执行定时器服务任务，启动命令才真正生效。 */
    MRT_TimerServiceRunPending();

    /* 服务任务处理后定时器应变为活动。 */
    active = false;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TimerIsActive(timer, &active));
    MRT_TEST_ASSERT_TRUE(active);

    /* 当前 tick 为 0，周期为 5，因此启动命令处理后的到期 tick 为 5。 */
    MRT_TEST_ASSERT_EQ_U32(5u, (unsigned)storage.expiry_tick);
}

/**
 * @brief 验证 tick 到期只投递回调事件，服务任务执行后才调用用户回调。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_timer_expiry_callback_runs_in_service_task();
 */
static void assert_timer_expiry_callback_runs_in_service_task(void)
{
    /* 初始化内核和定时器服务队列。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 清空测试观测状态。 */
    ResetTimerServiceRecords();

    /* 定义静态定时器控制块。 */
    MRT_Timer storage;

    /* 定义定时器句柄。 */
    MRT_TimerHandle timer = 0;

    /* 创建 2 tick 单次定时器。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TimerCreateStatic("svc-expire",
                                                           2u,
                                                           false,
                                                           0,
                                                           TimerCallback,
                                                           &storage,
                                                           &timer));

    /* 投递启动命令并执行服务任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TimerStart(timer, 0u));
    MRT_TimerServiceRunPending();

    /* 推进第 1 个 tick，尚未到期。 */
    MRT_KernelTick();
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)g_callback_count);

    /* 推进第 2 个 tick，tick 路径只投递回调事件。 */
    MRT_KernelTick();
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)g_callback_count);

    /* 服务任务运行后才执行用户回调。 */
    MRT_TimerServiceRunPending();
    MRT_TEST_ASSERT_EQ_U32(1u, (unsigned)g_callback_count);
    MRT_TEST_ASSERT_TRUE(g_last_timer == timer);
}

/**
 * @brief 验证控制命令按 FIFO 顺序由服务任务处理。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_timer_commands_are_processed_fifo_by_service_task();
 */
static void assert_timer_commands_are_processed_fifo_by_service_task(void)
{
    /* 初始化内核和定时器服务队列。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 清空测试观测状态。 */
    ResetTimerServiceRecords();

    /* 定义静态定时器控制块。 */
    MRT_Timer storage;

    /* 定义定时器句柄。 */
    MRT_TimerHandle timer = 0;

    /* 创建 10 tick 周期定时器。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TimerCreateStatic("svc-fifo",
                                                           10u,
                                                           false,
                                                           0,
                                                           TimerCallback,
                                                           &storage,
                                                           &timer));

    /* 先投递启动命令。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TimerStart(timer, 0u));

    /* 再投递改周期命令，要求服务任务按 FIFO 先启动再改周期。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TimerChangePeriod(timer, 3u, 0u));

    /* 服务任务运行前命令不应提前生效。 */
    bool active = true;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TimerIsActive(timer, &active));
    MRT_TEST_ASSERT_TRUE(!active);
    MRT_TEST_ASSERT_EQ_U32(10u, (unsigned)storage.period_ticks);

    /* 执行服务任务，按 FIFO 处理启动和改周期命令。 */
    MRT_TimerServiceRunPending();

    /* 处理后周期更新为 3，且活动定时器以当前 tick 重新计算到期点。 */
    MRT_TEST_ASSERT_EQ_U32(3u, (unsigned)storage.period_ticks);
    MRT_TEST_ASSERT_EQ_U32(3u, (unsigned)storage.expiry_tick);
}

/**
 * @brief 运行软件定时器服务任务耦合测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证启动命令由服务任务异步处理。 */
    assert_timer_start_command_waits_for_service_task();

    /* 验证到期回调由服务任务执行。 */
    assert_timer_expiry_callback_runs_in_service_task();

    /* 验证控制命令 FIFO 顺序。 */
    assert_timer_commands_are_processed_fifo_by_service_task();

    /* 所有定时器服务任务耦合测试均通过。 */
    return 0;
}
