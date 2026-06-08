#ifndef MYRTOS_MRT_MUTEX_INTERNAL_H
#define MYRTOS_MRT_MUTEX_INTERNAL_H

/**
 * @file mrt_mutex_internal.h
 * @brief MyRTOS 互斥锁内核内部接口。
 *
 * 本文件只供内核模块之间协作使用，不属于用户公开 API。
 */

#include "myrtos/mrt_list.h"

/**
 * @brief 处理互斥锁等待任务因 tick 超时而离开等待链表后的优先级回滚。
 * @param waiting_lockers 已经移除超时任务后的互斥锁等待链表，不能为空。
 * @return void 无返回值。
 * @example
 * MRT_MutexKernelHandleLockTimeout(wait_list);
 */
void MRT_MutexKernelHandleLockTimeout(MRT_List *waiting_lockers);

#endif
