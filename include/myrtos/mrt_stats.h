#ifndef MYRTOS_MRT_STATS_H
#define MYRTOS_MRT_STATS_H

/**
 * @file mrt_stats.h
 * @brief MyRTOS 运行统计公共接口。
 *
 * 本模块提供任务运行时间统计查询。当前可移植 preview 使用内核 tick 作为统计单位，
 * 后续 STM32/DSP 真实端口可以把统计源替换为高分辨率周期计数器，同时保持公共 API 不变。
 */

#include "myrtos/mrt_task.h"
#include "myrtos/mrt_types.h"

/**
 * @brief 查询任务累计运行时间。
 * @param task 待查询任务句柄，不能为空，且任务不能处于 deleted 状态。
 * @param out_runtime 输出累计运行计数，不能为空；当前 preview 单位为 kernel tick。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示查询成功；参数非法或任务已删除时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * uint64_t ticks;
 * MRT_StatsGetTaskRuntime(worker, &ticks);
 */
MRT_Result MRT_StatsGetTaskRuntime(MRT_TaskHandle task, uint64_t *out_runtime);

#endif
