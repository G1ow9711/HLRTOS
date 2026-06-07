#ifndef MYRTOS_MRT_TASK_INTERNAL_H
#define MYRTOS_MRT_TASK_INTERNAL_H

/**
 * @file mrt_task_internal.h
 * @brief MyRTOS 任务调度器内部接口。
 *
 * 本文件只供内核内部模块使用，不作为用户 API。
 */

#include <stdbool.h>

void MRT_TaskKernelInitialize(void);
bool MRT_TaskKernelStartScheduler(void);

#endif
