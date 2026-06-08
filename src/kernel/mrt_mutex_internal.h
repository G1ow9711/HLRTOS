#ifndef MYRTOS_MRT_MUTEX_INTERNAL_H
#define MYRTOS_MRT_MUTEX_INTERNAL_H

/**
 * @file mrt_mutex_internal.h
 * @brief MyRTOS 互斥锁内核内部接口。
 *
 * 本文件只供内核模块之间协作使用，不属于用户公开 API。
 */

#include "myrtos/mrt_list.h"
#include "myrtos/mrt_task.h"

/**
 * @brief 初始化互斥锁模块内部注册表。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * MRT_MutexKernelInitialize();
 */
void MRT_MutexKernelInitialize(void);

/**
 * @brief 处理互斥锁等待任务因 tick 超时而离开等待链表后的优先级回滚。
 * @param waiting_lockers 已经移除超时任务后的互斥锁等待链表，不能为空。
 * @return void 无返回值。
 * @example
 * MRT_MutexKernelHandleLockTimeout(wait_list);
 */
void MRT_MutexKernelHandleLockTimeout(MRT_List *waiting_lockers);

/**
 * @brief 判断任务是否可以安全删除。
 * @param task 待删除任务句柄，不能为空。
 * @return bool 返回 true 表示任务未持有互斥锁，可以继续删除；返回 false 表示任务仍持有互斥锁。
 * @example
 * if (!MRT_MutexKernelCanDeleteTask(task)) { return MRT_RESULT_OBJECT_BUSY; }
 */
bool MRT_MutexKernelCanDeleteTask(MRT_TaskHandle task);

#endif
