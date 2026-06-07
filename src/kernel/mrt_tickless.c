#include "myrtos/mrt_kernel.h"
#include "myrtos/mrt_port.h"
#include "myrtos/mrt_tickless.h"
#include "mrt_task_internal.h"
#include "mrt_timer_internal.h"

/**
 * @brief 计算从当前 tick 到指定 deadline 的剩余 tick 数。
 * @param now 当前系统 tick。
 * @param deadline 目标唤醒或到期 tick。
 * @return MRT_Tick 返回剩余 tick 数；deadline 已到或已过时返回 0。
 * @example
 * MRT_Tick remain = MRT_TicklessTicksUntil(MRT_KernelGetTick(), wake_tick);
 */
static MRT_Tick MRT_TicklessTicksUntil(MRT_Tick now, MRT_Tick deadline)
{
    /* 使用有符号差值判断 deadline 是否已经到达，保持与任务/定时器到期语义一致。 */
    if ((int32_t)(deadline - now) <= 0) {
        /* 已经到期时不能继续睡眠。 */
        return 0u;
    }

    /* 无符号减法天然支持 tick 回绕，返回距离 deadline 的 tick 数。 */
    return deadline - now;
}

/**
 * @brief 用一个 deadline 更新当前最小预计空闲 tick 数。
 * @param now 当前系统 tick。
 * @param deadline 候选唤醒或到期 tick。
 * @param has_deadline 是否已经存在更早 deadline 的标记指针，不能为空。
 * @param best_idle_ticks 当前最小预计空闲 tick 数指针，不能为空。
 * @return void 无返回值。
 * @example
 * MRT_TicklessMergeDeadline(now, wake_tick, &has_deadline, &best_idle_ticks);
 */
static void MRT_TicklessMergeDeadline(MRT_Tick now,
                                      MRT_Tick deadline,
                                      bool *has_deadline,
                                      MRT_Tick *best_idle_ticks)
{
    /* 把绝对 deadline 转换成相对当前 tick 的剩余 tick 数。 */
    MRT_Tick candidate = MRT_TicklessTicksUntil(now, deadline);

    /* 首个 deadline 直接作为当前最优值；后续 deadline 只保留更近者。 */
    if ((!*has_deadline) || (candidate < *best_idle_ticks)) {
        /* 记录已经看到至少一个 deadline。 */
        *has_deadline = true;

        /* 保存当前最近 deadline 对应的剩余 tick 数。 */
        *best_idle_ticks = candidate;
    }
}

/**
 * @brief 查询当前可以连续空闲的 tick 数。
 * @param out_expected_idle_ticks 输出预计可空闲 tick 数，不能为空；没有已知 deadline 时写入 0。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示查询成功；参数为空时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * MRT_Tick idle_ticks;
 * if (MRT_TicklessGetExpectedIdleTicks(&idle_ticks) == MRT_RESULT_OK) {
 *     // idle_ticks 可用于决定是否进入低功耗。
 * }
 */
MRT_Result MRT_TicklessGetExpectedIdleTicks(MRT_Tick *out_expected_idle_ticks)
{
    /* 输出指针不能为空，否则无法向调用方返回查询结果。 */
    if (out_expected_idle_ticks == 0) {
        /* 返回参数错误，提示调用方提供有效输出变量。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 读取当前系统 tick，后续 task/timer deadline 都以此为基准。 */
    MRT_Tick now = MRT_KernelGetTick();

    /* 标记是否已经发现任何任务或定时器 deadline。 */
    bool has_deadline = false;

    /* 保存当前发现的最小可空闲 tick 数。 */
    MRT_Tick best_idle_ticks = 0u;

    /* 定义任务唤醒 tick 临时变量。 */
    MRT_Tick task_wake_tick = 0u;

    /* 查询最近的任务唤醒 tick。 */
    if (MRT_TaskKernelGetNextWakeTick(&task_wake_tick)) {
        /* 将任务 deadline 合并进最小空闲时间。 */
        MRT_TicklessMergeDeadline(now, task_wake_tick, &has_deadline, &best_idle_ticks);
    }

    /* 定义软件定时器到期 tick 临时变量。 */
    MRT_Tick timer_expiry_tick = 0u;

    /* 查询最近的软件定时器到期 tick。 */
    if (MRT_TimerKernelGetNextExpiryTick(&timer_expiry_tick)) {
        /* 将定时器 deadline 合并进最小空闲时间。 */
        MRT_TicklessMergeDeadline(now, timer_expiry_tick, &has_deadline, &best_idle_ticks);
    }

    /* 如果没有任何 deadline，则写入 0，避免端口层进入无界睡眠。 */
    if (!has_deadline) {
        /* 返回 0 表示当前没有明确可睡眠窗口。 */
        *out_expected_idle_ticks = 0u;
    } else {
        /* 写出最近 deadline 对应的预计空闲 tick 数。 */
        *out_expected_idle_ticks = best_idle_ticks;
    }

    /* 查询完成。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 在允许范围内进入 tickless idle 并补偿实际睡眠 tick。
 * @param max_sleep_ticks 调用方允许的最大睡眠 tick 数；为 0 时不调用端口睡眠。
 * @param out_slept_ticks 输出端口层实际睡眠 tick 数，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示处理成功；参数为空时返回 MRT_RESULT_INVALID_ARGUMENT；
 *         端口层睡眠失败时返回端口层结果。
 * @example
 * MRT_Tick slept;
 * MRT_TicklessEnterIdle(100u, &slept);
 */
MRT_Result MRT_TicklessEnterIdle(MRT_Tick max_sleep_ticks, MRT_Tick *out_slept_ticks)
{
    /* 输出指针不能为空，否则无法告诉调用方真实睡眠了多少 tick。 */
    if (out_slept_ticks == 0) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 先写入 0，确保早退路径也有确定输出。 */
    *out_slept_ticks = 0u;

    /* 调用方不允许睡眠时，直接返回成功且不触碰端口层。 */
    if (max_sleep_ticks == 0u) {
        /* 没有补偿任何 tick。 */
        return MRT_RESULT_OK;
    }

    /* 定义内核估算出的最近 deadline 距离。 */
    MRT_Tick expected_idle_ticks = 0u;

    /* 查询当前任务和软件定时器共同允许的最大空闲窗口。 */
    MRT_Result result = MRT_TicklessGetExpectedIdleTicks(&expected_idle_ticks);

    /* 查询失败时直接返回错误。 */
    if (result != MRT_RESULT_OK) {
        /* 透传查询错误。 */
        return result;
    }

    /* 没有明确 deadline 时不进入端口低功耗，避免无界睡眠。 */
    if (expected_idle_ticks == 0u) {
        /* 保持输出为 0。 */
        return MRT_RESULT_OK;
    }

    /* 端口层请求不能超过调用方给出的最大睡眠限制。 */
    MRT_Tick requested_sleep_ticks = expected_idle_ticks;

    /* 如果调用方限制更小，则以调用方限制为准。 */
    if (requested_sleep_ticks > max_sleep_ticks) {
        /* 缩短本次端口睡眠请求。 */
        requested_sleep_ticks = max_sleep_ticks;
    }

    /* 定义端口层回报的真实睡眠 tick 数。 */
    MRT_Tick port_slept_ticks = 0u;

    /* 调用移植层执行具体 tick 抑制与低功耗睡眠动作。 */
    result = MRT_PortSuppressTicksAndSleep(requested_sleep_ticks, &port_slept_ticks);

    /* 端口层失败时不补偿 tick，直接透传错误。 */
    if (result != MRT_RESULT_OK) {
        /* 保持输出为 0。 */
        return result;
    }

    /* 防御性限制端口回报值，避免异常端口让内核补偿超过本次请求的 tick。 */
    if (port_slept_ticks > requested_sleep_ticks) {
        /* 将真实睡眠值裁剪到本次请求上限。 */
        port_slept_ticks = requested_sleep_ticks;
    }

    /* 按真实睡眠 tick 数逐个推进内核，复用现有任务唤醒和定时器到期路径。 */
    for (MRT_Tick tick = 0u; tick < port_slept_ticks; tick++) {
        /* 每次推进一个 tick，保持回调顺序和调度语义与普通 SysTick 一致。 */
        MRT_KernelTick();
    }

    /* 向调用方写回真实补偿的 tick 数。 */
    *out_slept_ticks = port_slept_ticks;

    /* tickless 进入和补偿完成。 */
    return MRT_RESULT_OK;
}
