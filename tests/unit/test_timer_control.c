#include "mrt_test.h"
#include "myrtos/mrt_kernel.h"
#include "myrtos/mrt_list.h"
#include "myrtos/mrt_timer.h"

/** @brief 测试回调被控制 API 保存但不会在本单元测试中执行。 */
static uint32_t g_timer_callback_count;

/**
 * @brief 测试用定时器回调函数。
 * @param timer 到期定时器句柄，本测试不使用。
 * @param arg 用户参数，本测试不使用。
 * @return void 无返回值。
 * @example
 * DummyTimerCallback(timer, arg);
 */
static void DummyTimerCallback(MRT_TimerHandle timer, void *arg)
{
    /* 显式丢弃未使用的定时器句柄。 */
    (void)timer;

    /* 显式丢弃未使用的用户参数。 */
    (void)arg;

    /* 记录回调次数，便于确认控制 API 本身不会提前触发回调。 */
    g_timer_callback_count++;
}

/**
 * @brief 创建一个测试用软件定时器。
 * @param storage 定时器控制块存储，不能为空。
 * @param name 定时器名称。
 * @param period_ticks 定时器周期。
 * @param auto_reload 是否自动重载。
 * @return MRT_TimerHandle 返回创建成功的定时器句柄。
 * @example
 * MRT_TimerHandle timer = CreateTimerOrFail(&storage, "t", 3u, false);
 */
static MRT_TimerHandle CreateTimerOrFail(MRT_Timer *storage,
                                         const char *name,
                                         MRT_Tick period_ticks,
                                         bool auto_reload)
{
    /* 定义输出句柄并初始化为空。 */
    MRT_TimerHandle timer = 0;

    /* 调用静态创建 API 并要求成功。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TimerCreateStatic(name,
                                                           period_ticks,
                                                           auto_reload,
                                                           0,
                                                           DummyTimerCallback,
                                                           storage,
                                                           &timer));

    /* 返回创建得到的句柄。 */
    return timer;
}

/**
 * @brief 验证启动和停止会维护活动状态、到期 tick 和链表节点。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_timer_start_stop_controls_active_state();
 */
static void assert_timer_start_stop_controls_active_state(void)
{
    /* 初始化内核，使 tick 和定时器全局状态从确定值开始。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 定义定时器控制块。 */
    MRT_Timer storage;

    /* 创建周期为 5 tick 的单次定时器。 */
    MRT_TimerHandle timer = CreateTimerOrFail(&storage, "control", 5u, false);

    /* 启动定时器，timeout 当前作为兼容参数传入 0。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TimerStart(timer, 0u));

    /* 启动命令尚未被服务任务处理前，定时器不应立即活动。 */
    bool active = true;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TimerIsActive(timer, &active));
    MRT_TEST_ASSERT_TRUE(!active);

    /* 运行定时器服务任务，真正处理启动命令。 */
    MRT_TimerServiceRunPending();

    /* 启动命令处理后，定时器应处于活动状态。 */
    active = false;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TimerIsActive(timer, &active));
    MRT_TEST_ASSERT_TRUE(active);

    /* 当前 tick 为 0，周期为 5，因此下一次到期 tick 应为 5。 */
    MRT_TEST_ASSERT_EQ_U32(5u, (unsigned)storage.expiry_tick);

    /* 活动定时器节点应已经加入全局链表。 */
    MRT_TEST_ASSERT_TRUE(MRT_ListNodeIsLinked(&storage.node));

    /* 控制 API 不应直接触发到期回调。 */
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)g_timer_callback_count);

    /* 停止定时器。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TimerStop(timer, 0u));

    /* 停止命令尚未被服务任务处理前，定时器仍应保持活动。 */
    active = false;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TimerIsActive(timer, &active));
    MRT_TEST_ASSERT_TRUE(active);

    /* 运行定时器服务任务，真正处理停止命令。 */
    MRT_TimerServiceRunPending();

    /* 停止命令处理后，定时器不再活动。 */
    active = true;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TimerIsActive(timer, &active));
    MRT_TEST_ASSERT_TRUE(!active);

    /* 停止后定时器节点应从活动链表脱离。 */
    MRT_TEST_ASSERT_TRUE(!MRT_ListNodeIsLinked(&storage.node));
}

/**
 * @brief 验证 reset 会基于当前 tick 重新计算到期时间。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_timer_reset_recalculates_expiry_from_current_tick();
 */
static void assert_timer_reset_recalculates_expiry_from_current_tick(void)
{
    /* 初始化内核状态。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 定义定时器控制块。 */
    MRT_Timer storage;

    /* 创建周期为 4 tick 的定时器。 */
    MRT_TimerHandle timer = CreateTimerOrFail(&storage, "reset", 4u, false);

    /* 启动后第一次到期 tick 应为 4。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TimerStart(timer, 0u));
    MRT_TimerServiceRunPending();
    MRT_TEST_ASSERT_EQ_U32(4u, (unsigned)storage.expiry_tick);

    /* 推进两个 tick，当前 tick 变为 2。 */
    MRT_KernelTick();
    MRT_KernelTick();

    /* reset 应从当前 tick 重新装载周期。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TimerReset(timer, 0u));

    /* reset 命令在服务任务处理前不应立刻改变到期时间。 */
    MRT_TEST_ASSERT_EQ_U32(4u, (unsigned)storage.expiry_tick);

    /* 运行定时器服务任务，真正处理 reset 命令。 */
    MRT_TimerServiceRunPending();

    /* 当前 tick 为 2，周期为 4，因此新到期 tick 应为 6。 */
    MRT_TEST_ASSERT_EQ_U32(6u, (unsigned)storage.expiry_tick);

    /* reset 后定时器仍应保持活动。 */
    bool active = false;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TimerIsActive(timer, &active));
    MRT_TEST_ASSERT_TRUE(active);

    /* 测试结束前停止定时器，避免全局活动链表保留栈上控制块。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TimerStop(timer, 0u));
    MRT_TimerServiceRunPending();
}

/**
 * @brief 验证改周期会更新周期字段，并在活动时重算到期时间。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_timer_change_period_updates_period_and_active_expiry();
 */
static void assert_timer_change_period_updates_period_and_active_expiry(void)
{
    /* 初始化内核状态。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 定义定时器控制块。 */
    MRT_Timer storage;

    /* 创建周期为 8 tick 的定时器。 */
    MRT_TimerHandle timer = CreateTimerOrFail(&storage, "change", 8u, true);

    /* 启动定时器。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TimerStart(timer, 0u));
    MRT_TimerServiceRunPending();

    /* 推进一个 tick，使当前 tick 为 1。 */
    MRT_KernelTick();

    /* 修改周期为 3 tick。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TimerChangePeriod(timer, 3u, 0u));

    /* 改周期命令在服务任务处理前不应立刻生效。 */
    MRT_TEST_ASSERT_EQ_U32(8u, (unsigned)storage.period_ticks);
    MRT_TEST_ASSERT_EQ_U32(8u, (unsigned)storage.expiry_tick);

    /* 运行定时器服务任务，真正处理改周期命令。 */
    MRT_TimerServiceRunPending();

    /* 周期字段应被更新。 */
    MRT_TEST_ASSERT_EQ_U32(3u, (unsigned)storage.period_ticks);

    /* 活动定时器应按当前 tick 重新计算到期点：1 + 3 = 4。 */
    MRT_TEST_ASSERT_EQ_U32(4u, (unsigned)storage.expiry_tick);

    /* 活动状态应保持不变。 */
    bool active = false;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TimerIsActive(timer, &active));
    MRT_TEST_ASSERT_TRUE(active);

    /* 测试结束前停止定时器，避免全局活动链表保留栈上控制块。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TimerStop(timer, 0u));
    MRT_TimerServiceRunPending();
}

/**
 * @brief 验证非活动定时器修改周期不会自动启动。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_timer_change_period_on_inactive_timer_does_not_start();
 */
static void assert_timer_change_period_on_inactive_timer_does_not_start(void)
{
    /* 初始化内核状态。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 定义定时器控制块。 */
    MRT_Timer storage;

    /* 创建但不启动定时器。 */
    MRT_TimerHandle timer = CreateTimerOrFail(&storage, "inactive-change", 9u, false);

    /* 修改周期为 2 tick。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TimerChangePeriod(timer, 2u, 0u));

    /* 改周期命令在服务任务处理前不应立刻更新。 */
    MRT_TEST_ASSERT_EQ_U32(9u, (unsigned)storage.period_ticks);

    /* 运行定时器服务任务，真正处理改周期命令。 */
    MRT_TimerServiceRunPending();

    /* 周期字段应更新。 */
    MRT_TEST_ASSERT_EQ_U32(2u, (unsigned)storage.period_ticks);

    /* 未活动定时器不应被自动加入活动链表。 */
    bool active = true;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TimerIsActive(timer, &active));
    MRT_TEST_ASSERT_TRUE(!active);
    MRT_TEST_ASSERT_TRUE(!MRT_ListNodeIsLinked(&storage.node));
}

/**
 * @brief 验证控制 API 参数校验。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_timer_control_rejects_invalid_arguments();
 */
static void assert_timer_control_rejects_invalid_arguments(void)
{
    /* 初始化内核状态。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 定义定时器控制块。 */
    MRT_Timer storage;

    /* 创建有效定时器用于非法周期测试。 */
    MRT_TimerHandle timer = CreateTimerOrFail(&storage, "invalid", 5u, false);

    /* 空定时器不能启动。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT, (unsigned)MRT_TimerStart(0, 0u));

    /* 空定时器不能停止。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT, (unsigned)MRT_TimerStop(0, 0u));

    /* 空定时器不能 reset。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT, (unsigned)MRT_TimerReset(0, 0u));

    /* 空定时器不能改周期。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT, (unsigned)MRT_TimerChangePeriod(0, 1u, 0u));

    /* 新周期不能为 0。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_TimerChangePeriod(timer, 0u, 0u));
}

/**
 * @brief 运行软件定时器控制 API 测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 清空回调次数计数。 */
    g_timer_callback_count = 0u;

    /* 验证启动和停止。 */
    assert_timer_start_stop_controls_active_state();

    /* 验证 reset 到期时间重算。 */
    assert_timer_reset_recalculates_expiry_from_current_tick();

    /* 验证活动定时器改周期。 */
    assert_timer_change_period_updates_period_and_active_expiry();

    /* 验证非活动定时器改周期。 */
    assert_timer_change_period_on_inactive_timer_does_not_start();

    /* 验证控制 API 参数错误路径。 */
    assert_timer_control_rejects_invalid_arguments();

    /* 所有控制 API 测试均通过。 */
    return 0;
}
