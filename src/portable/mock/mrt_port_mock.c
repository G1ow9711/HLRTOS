#include "myrtos/mrt_port.h"

/** @brief mock 当前是否处于 ISR 上下文。 */
static bool g_inside_isr;

/** @brief mock 是否已经收到调度切换请求。 */
static bool g_yield_requested;

/** @brief mock 当前临界区嵌套深度。 */
static uint32_t g_critical_depth;

/** @brief mock 下一次 tickless 睡眠实际经过的 tick 数。 */
static MRT_Tick g_mock_suppressed_sleep_ticks;

/** @brief mock 收到 tickless 睡眠请求的次数。 */
static uint32_t g_mock_suppress_sleep_call_count;

/** @brief mock 最近一次收到的预计可睡眠 tick 数。 */
static MRT_Tick g_mock_last_expected_idle_ticks;

/**
 * @brief 初始化端口层状态。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * MRT_PortInitialize();
 */
void MRT_PortInitialize(void)
{
    /* 默认进入任务上下文，便于 host 测试从普通线程开始。 */
    g_inside_isr = false;

    /* 清除挂起调度切换请求。 */
    g_yield_requested = false;

    /* 清空临界区嵌套深度。 */
    g_critical_depth = 0u;

    /* 清空下一次模拟睡眠 tick 数。 */
    g_mock_suppressed_sleep_ticks = 0u;

    /* 清空 tickless 睡眠请求次数。 */
    g_mock_suppress_sleep_call_count = 0u;

    /* 清空最近一次预计可睡眠 tick 数。 */
    g_mock_last_expected_idle_ticks = 0u;
}

/**
 * @brief 启动第一个任务。
 * @param void 无输入参数。
 * @return void 无返回值；真实端口通常不会返回。
 * @example
 * MRT_PortStartFirstTask();
 */
void MRT_PortStartFirstTask(void)
{
    /* host mock 无真实 CPU 上下文，因此用 yield 请求记录启动动作。 */
    g_yield_requested = true;
}

/**
 * @brief 在任务上下文请求一次调度切换。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * MRT_PortYield();
 */
void MRT_PortYield(void)
{
    /* 记录任务上下文已经请求调度切换。 */
    g_yield_requested = true;
}

/**
 * @brief 在 ISR 退出前按需请求调度切换。
 * @param should_yield true 表示需要切换，false 表示不需要切换。
 * @return void 无返回值。
 * @example
 * MRT_PortYieldFromISR(should_yield);
 */
void MRT_PortYieldFromISR(bool should_yield)
{
    /* 只有 ISR 唤醒更高优先级任务时才需要切换。 */
    if (should_yield) {
        /* 记录 ISR 退出时需要触发调度。 */
        g_yield_requested = true;
    }
}

/**
 * @brief 进入临界区并保存旧中断状态。
 * @param void 无输入参数。
 * @return MRT_IntState 返回进入临界区前的端口状态，用于退出时恢复。
 * @example
 * MRT_IntState state = MRT_PortEnterCritical();
 */
MRT_IntState MRT_PortEnterCritical(void)
{
    /* 保存进入前的嵌套深度，用作恢复状态。 */
    MRT_IntState previous_depth = (MRT_IntState)g_critical_depth;

    /* 增加嵌套深度，模拟中断屏蔽计数。 */
    g_critical_depth++;

    /* 返回旧状态给调用方，退出临界区时原样传回。 */
    return previous_depth;
}

/**
 * @brief 退出临界区并恢复旧中断状态。
 * @param state MRT_PortEnterCritical 返回的旧状态。
 * @return void 无返回值。
 * @example
 * MRT_PortExitCritical(state);
 */
void MRT_PortExitCritical(MRT_IntState state)
{
    /* 将嵌套深度恢复为进入临界区前保存的状态。 */
    g_critical_depth = (uint32_t)state;
}

/**
 * @brief 判断当前是否处于 ISR 上下文。
 * @param void 无输入参数。
 * @return bool 返回 true 表示当前在 ISR 中，返回 false 表示当前在任务上下文。
 * @example
 * if (MRT_PortIsInsideISR()) { MRT_QueueSendFromISR(queue, item, &yield); }
 */
bool MRT_PortIsInsideISR(void)
{
    /* 返回测试代码当前设置的 ISR 状态。 */
    return g_inside_isr;
}

/**
 * @brief 设置 host mock 的 ISR 状态。
 * @param inside_isr true 表示模拟 ISR 上下文，false 表示模拟任务上下文。
 * @return void 无返回值。
 * @example
 * MRT_PortMockSetInsideISR(true);
 */
void MRT_PortMockSetInsideISR(bool inside_isr)
{
    /* 保存调用方指定的模拟上下文状态。 */
    g_inside_isr = inside_isr;
}

/**
 * @brief 查询 host mock 是否收到上下文切换请求。
 * @param void 无输入参数。
 * @return bool 返回 true 表示已经请求切换，返回 false 表示尚未请求切换。
 * @example
 * bool requested = MRT_PortMockWasYieldRequested();
 */
bool MRT_PortMockWasYieldRequested(void)
{
    /* 返回当前记录的调度切换请求标志。 */
    return g_yield_requested;
}

/**
 * @brief 查询 host mock 当前临界区嵌套深度。
 * @param void 无输入参数。
 * @return uint32_t 返回当前临界区嵌套深度。
 * @example
 * uint32_t depth = MRT_PortMockGetCriticalDepth();
 */
uint32_t MRT_PortMockGetCriticalDepth(void)
{
    /* 返回当前 mock 临界区嵌套深度。 */
    return g_critical_depth;
}

/**
 * @brief 抑制周期 tick 并进入低功耗睡眠。
 * @param expected_idle_ticks 内核允许端口层连续睡眠的最大 tick 数。
 * @param out_slept_ticks 输出端口层实际睡眠的 tick 数，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示端口睡眠过程完成；参数为空时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * MRT_Tick slept;
 * MRT_PortSuppressTicksAndSleep(10u, &slept);
 */
MRT_Result MRT_PortSuppressTicksAndSleep(MRT_Tick expected_idle_ticks, MRT_Tick *out_slept_ticks)
{
    /* 输出指针不能为空，否则无法回报真实睡眠 tick 数。 */
    if (out_slept_ticks == 0) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 记录端口层收到了一次 tickless 睡眠请求。 */
    g_mock_suppress_sleep_call_count++;

    /* 保存本次内核允许的最大睡眠 tick 数，供测试断言。 */
    g_mock_last_expected_idle_ticks = expected_idle_ticks;

    /* 从 mock 配置读取真实睡眠 tick 数。 */
    MRT_Tick slept_ticks = g_mock_suppressed_sleep_ticks;

    /* 防御性裁剪，真实睡眠不应超过内核允许的最大值。 */
    if (slept_ticks > expected_idle_ticks) {
        /* 将回报值限制在 expected_idle_ticks 内。 */
        slept_ticks = expected_idle_ticks;
    }

    /* 写出真实睡眠 tick 数。 */
    *out_slept_ticks = slept_ticks;

    /* mock 睡眠完成。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 设置 host mock 下一次 tickless 睡眠实际经过的 tick 数。
 * @param slept_ticks 模拟端口层真实睡眠 tick 数。
 * @return void 无返回值。
 * @example
 * MRT_PortMockSetSuppressedSleepTicks(4u);
 */
void MRT_PortMockSetSuppressedSleepTicks(MRT_Tick slept_ticks)
{
    /* 保存下一次端口睡眠要回报的真实 tick 数。 */
    g_mock_suppressed_sleep_ticks = slept_ticks;
}

/**
 * @brief 查询 host mock 收到 tickless 睡眠请求的次数。
 * @param void 无输入参数。
 * @return uint32_t 返回 tickless 睡眠请求次数。
 * @example
 * uint32_t calls = MRT_PortMockGetSuppressSleepCallCount();
 */
uint32_t MRT_PortMockGetSuppressSleepCallCount(void)
{
    /* 返回当前累计的睡眠请求次数。 */
    return g_mock_suppress_sleep_call_count;
}

/**
 * @brief 查询 host mock 最近一次收到的预计可睡眠 tick 数。
 * @param void 无输入参数。
 * @return MRT_Tick 返回最近一次 MRT_PortSuppressTicksAndSleep 的 expected_idle_ticks 参数。
 * @example
 * MRT_Tick requested = MRT_PortMockGetLastExpectedIdleTicks();
 */
MRT_Tick MRT_PortMockGetLastExpectedIdleTicks(void)
{
    /* 返回最近一次端口睡眠请求的 expected_idle_ticks。 */
    return g_mock_last_expected_idle_ticks;
}
