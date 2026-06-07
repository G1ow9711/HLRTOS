#include "mrt_test.h"
#include "myrtos/mrt_kernel.h"
#include "myrtos/mrt_timer.h"

/** @brief 测试回调累计执行次数。 */
static uint32_t g_callback_count;

/** @brief 测试回调记录的最后一个定时器句柄。 */
static MRT_TimerHandle g_last_timer;

/** @brief 测试回调记录的触发顺序。 */
static uint32_t g_callback_order[4];

/** @brief 测试回调触发顺序数组中的有效元素数量。 */
static uint32_t g_callback_order_count;

/**
 * @brief 清空测试回调观测状态。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * ResetCallbackState();
 */
static void ResetCallbackState(void)
{
    /* 清空回调次数。 */
    g_callback_count = 0u;

    /* 清空最后触发的定时器句柄。 */
    g_last_timer = 0;

    /* 清空触发顺序计数。 */
    g_callback_order_count = 0u;

    /* 清空触发顺序第 0 项。 */
    g_callback_order[0] = 0u;

    /* 清空触发顺序第 1 项。 */
    g_callback_order[1] = 0u;

    /* 清空触发顺序第 2 项。 */
    g_callback_order[2] = 0u;

    /* 清空触发顺序第 3 项。 */
    g_callback_order[3] = 0u;
}

/**
 * @brief 测试用定时器到期回调。
 * @param timer 到期定时器句柄。
 * @param arg 指向 uint32_t 标识值的用户参数，允许为空。
 * @return void 无返回值。
 * @example
 * RecordTimerCallback(timer, &id);
 */
static void RecordTimerCallback(MRT_TimerHandle timer, void *arg)
{
    /* 记录回调执行次数。 */
    g_callback_count++;

    /* 记录最后触发的定时器句柄。 */
    g_last_timer = timer;

    /* 如果调用方提供了标识值，并且顺序数组还有空间，则记录触发顺序。 */
    if ((arg != 0) && (g_callback_order_count < 4u)) {
        /* 将用户参数转换为标识值指针。 */
        uint32_t *id = (uint32_t *)arg;

        /* 保存本次触发的标识值。 */
        g_callback_order[g_callback_order_count] = *id;

        /* 推进触发顺序计数。 */
        g_callback_order_count++;
    }
}

/**
 * @brief 创建一个测试用软件定时器。
 * @param storage 定时器控制块存储，不能为空。
 * @param period_ticks 定时器周期。
 * @param auto_reload 是否自动重载。
 * @param arg 用户回调参数。
 * @return MRT_TimerHandle 返回创建成功的定时器句柄。
 * @example
 * MRT_TimerHandle timer = CreateTimerOrFail(&storage, 3u, false, &id);
 */
static MRT_TimerHandle CreateTimerOrFail(MRT_Timer *storage,
                                         MRT_Tick period_ticks,
                                         bool auto_reload,
                                         void *arg)
{
    /* 定义输出句柄并初始化为空。 */
    MRT_TimerHandle timer = 0;

    /* 创建静态软件定时器。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TimerCreateStatic("tick",
                                                           period_ticks,
                                                           auto_reload,
                                                           arg,
                                                           RecordTimerCallback,
                                                           storage,
                                                           &timer));

    /* 返回创建得到的定时器句柄。 */
    return timer;
}

/**
 * @brief 验证单次定时器在指定 tick 到期且只触发一次。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_one_shot_timer_fires_once_on_expiry_tick();
 */
static void assert_one_shot_timer_fires_once_on_expiry_tick(void)
{
    /* 初始化内核并清空定时器全局状态。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 清空回调观测状态。 */
    ResetCallbackState();

    /* 定义定时器控制块。 */
    MRT_Timer storage;

    /* 创建周期为 3 tick 的单次定时器。 */
    MRT_TimerHandle timer = CreateTimerOrFail(&storage, 3u, false, 0);

    /* 启动定时器。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TimerStart(timer, 0u));

    /* 第 1 个 tick 尚未到期。 */
    MRT_KernelTick();
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)g_callback_count);

    /* 第 2 个 tick 尚未到期。 */
    MRT_KernelTick();
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)g_callback_count);

    /* 第 3 个 tick 到期并执行一次回调。 */
    MRT_KernelTick();
    MRT_TEST_ASSERT_EQ_U32(1u, (unsigned)g_callback_count);
    MRT_TEST_ASSERT_TRUE(g_last_timer == timer);

    /* 单次定时器到期后应自动停止。 */
    bool active = true;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TimerIsActive(timer, &active));
    MRT_TEST_ASSERT_TRUE(!active);

    /* 后续 tick 不应重复触发。 */
    MRT_KernelTick();
    MRT_KernelTick();
    MRT_TEST_ASSERT_EQ_U32(1u, (unsigned)g_callback_count);
}

/**
 * @brief 验证自动重载定时器按周期反复触发并保持活动。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_auto_reload_timer_rearms_after_callback();
 */
static void assert_auto_reload_timer_rearms_after_callback(void)
{
    /* 初始化内核并清空定时器全局状态。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 清空回调观测状态。 */
    ResetCallbackState();

    /* 定义定时器控制块。 */
    MRT_Timer storage;

    /* 创建周期为 2 tick 的自动重载定时器。 */
    MRT_TimerHandle timer = CreateTimerOrFail(&storage, 2u, true, 0);

    /* 启动定时器。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TimerStart(timer, 0u));

    /* 第 1 个 tick 尚未到期。 */
    MRT_KernelTick();
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)g_callback_count);

    /* 第 2 个 tick 第一次到期。 */
    MRT_KernelTick();
    MRT_TEST_ASSERT_EQ_U32(1u, (unsigned)g_callback_count);

    /* 自动重载后下一次到期 tick 应为 4。 */
    MRT_TEST_ASSERT_EQ_U32(4u, (unsigned)storage.expiry_tick);

    /* 第 3 个 tick 不触发。 */
    MRT_KernelTick();
    MRT_TEST_ASSERT_EQ_U32(1u, (unsigned)g_callback_count);

    /* 第 4 个 tick 第二次到期。 */
    MRT_KernelTick();
    MRT_TEST_ASSERT_EQ_U32(2u, (unsigned)g_callback_count);

    /* 自动重载定时器应保持活动。 */
    bool active = false;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TimerIsActive(timer, &active));
    MRT_TEST_ASSERT_TRUE(active);

    /* 测试结束前停止定时器。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TimerStop(timer, 0u));
}

/**
 * @brief 验证不同到期时间的定时器按到期先后触发。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_timers_fire_in_expiry_order();
 */
static void assert_timers_fire_in_expiry_order(void)
{
    /* 初始化内核并清空定时器全局状态。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 清空回调观测状态。 */
    ResetCallbackState();

    /* 定义两个定时器标识值。 */
    uint32_t first_id = 1u;
    uint32_t second_id = 2u;

    /* 定义两个定时器控制块。 */
    MRT_Timer first_storage;
    MRT_Timer second_storage;

    /* 创建第一个定时器，4 tick 到期。 */
    MRT_TimerHandle first = CreateTimerOrFail(&first_storage, 4u, false, &first_id);

    /* 创建第二个定时器，2 tick 到期。 */
    MRT_TimerHandle second = CreateTimerOrFail(&second_storage, 2u, false, &second_id);

    /* 先启动晚到期的定时器，验证链表排序不依赖启动顺序。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TimerStart(first, 0u));

    /* 再启动早到期的定时器。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TimerStart(second, 0u));

    /* 推进到 tick 2，第二个定时器应先触发。 */
    MRT_KernelTick();
    MRT_KernelTick();
    MRT_TEST_ASSERT_EQ_U32(1u, (unsigned)g_callback_order_count);
    MRT_TEST_ASSERT_EQ_U32(2u, (unsigned)g_callback_order[0]);

    /* 推进到 tick 4，第一个定时器随后触发。 */
    MRT_KernelTick();
    MRT_KernelTick();
    MRT_TEST_ASSERT_EQ_U32(2u, (unsigned)g_callback_order_count);
    MRT_TEST_ASSERT_EQ_U32(1u, (unsigned)g_callback_order[1]);
}

/**
 * @brief 运行软件定时器 tick 到期耦合测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证单次定时器到期行为。 */
    assert_one_shot_timer_fires_once_on_expiry_tick();

    /* 验证周期定时器自动重载行为。 */
    assert_auto_reload_timer_rearms_after_callback();

    /* 验证多个定时器按到期顺序触发。 */
    assert_timers_fire_in_expiry_order();

    /* 所有 tick 到期耦合测试均通过。 */
    return 0;
}
