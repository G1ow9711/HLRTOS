#include "myrtos/mrt_kernel.h"
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
