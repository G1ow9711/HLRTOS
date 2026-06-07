#include "myrtos/mrt_stats.h"

/**
 * @brief 查询任务累计运行时间。
 * @param task 待查询任务句柄，不能为空，且任务不能处于 deleted 状态。
 * @param out_runtime 输出累计运行计数，不能为空；当前 preview 单位为 kernel tick。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示查询成功；参数非法或任务已删除时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * uint64_t ticks;
 * MRT_StatsGetTaskRuntime(worker, &ticks);
 */
MRT_Result MRT_StatsGetTaskRuntime(MRT_TaskHandle task, uint64_t *out_runtime)
{
    /* 任务句柄不能为空。 */
    if (task == 0) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 输出指针不能为空。 */
    if (out_runtime == 0) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 已删除任务的控制块可能已经失效或被释放，不能继续查询。 */
    if (task->state == MRT_TASK_STATE_DELETED) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 写回任务累计运行时间。 */
    *out_runtime = task->runtime_ticks;

    /* 查询成功。 */
    return MRT_RESULT_OK;
}
