#ifndef MYRTOS_MRT_TASK_INTERNAL_H
#define MYRTOS_MRT_TASK_INTERNAL_H

/**
 * @file mrt_task_internal.h
 * @brief MyRTOS 任务调度器内部接口。
 *
 * 本文件只供内核内部模块使用，不作为用户 API。
 */

#include "myrtos/mrt_list.h"
#include "myrtos/mrt_task.h"

#include <stdbool.h>

void MRT_TaskKernelInitialize(void);
bool MRT_TaskKernelStartScheduler(void);
void MRT_TaskKernelYield(void);
void MRT_TaskKernelTick(MRT_Tick now);
MRT_Result MRT_TaskKernelBlockCurrentOnObject(MRT_List *wait_list,
                                              MRT_Tick ticks,
                                              MRT_TaskWaitReason wait_reason,
                                              MRT_Result wait_result);
bool MRT_TaskKernelWakeFirstObjectWaiter(MRT_List *wait_list, MRT_Result wait_result, bool switch_now);
bool MRT_TaskKernelWakeTask(MRT_TaskHandle task, MRT_Result wait_result, bool switch_now);
void MRT_TaskKernelSetEffectivePriority(MRT_TaskHandle task, MRT_Priority priority);
void MRT_TaskKernelRestoreBasePriority(MRT_TaskHandle task);

#endif
