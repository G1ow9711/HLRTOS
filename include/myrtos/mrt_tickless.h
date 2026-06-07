#ifndef MYRTOS_MRT_TICKLESS_H
#define MYRTOS_MRT_TICKLESS_H

/**
 * @file mrt_tickless.h
 * @brief MyRTOS tickless idle 低功耗空闲管理接口。
 *
 * tickless idle 用于在系统没有立即需要运行的任务时，估算距离最近唤醒点还有多少个 tick，
 * 并把实际睡眠交给移植层实现。该模块只负责内核时间与任务/定时器语义，具体低功耗寄存器配置
 * 由 STM32、DSP 或 host mock 端口实现。
 */

#include "myrtos/mrt_types.h"

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
MRT_Result MRT_TicklessGetExpectedIdleTicks(MRT_Tick *out_expected_idle_ticks);

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
MRT_Result MRT_TicklessEnterIdle(MRT_Tick max_sleep_ticks, MRT_Tick *out_slept_ticks);

#endif
