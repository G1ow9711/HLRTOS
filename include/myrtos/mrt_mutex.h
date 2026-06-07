#ifndef MYRTOS_MRT_MUTEX_H
#define MYRTOS_MRT_MUTEX_H

/**
 * @file mrt_mutex.h
 * @brief MyRTOS 互斥锁公共接口。
 *
 * 互斥锁用于保护共享资源，并记录拥有者任务。后续任务会继续扩展优先级继承、
 * 等待队列和递归互斥锁行为。
 */

#include "myrtos/mrt_list.h"
#include "myrtos/mrt_task.h"
#include "myrtos/mrt_types.h"

/**
 * @brief MyRTOS 互斥锁控制块。
 *
 * 静态创建互斥锁时，调用方提供该结构体作为控制块存储。结构体公开是为了支持无动态内存
 * 的嵌入式工程；应用代码不应直接修改字段。
 */
typedef struct MRT_Mutex {
    /** @brief 当前拥有互斥锁的任务；无拥有者时为空。 */
    MRT_TaskHandle owner;
    /** @brief 当前锁定深度；普通互斥锁为 0 或 1，递归互斥锁可大于 1。 */
    size_t lock_count;
    /** @brief 是否允许同一拥有者递归加锁。 */
    bool recursive;
    /** @brief 等待锁可用的任务链表，后续阻塞 lock 会使用。 */
    MRT_List waiting_lockers;
    /** @brief 是否使用静态存储创建。 */
    bool static_storage;
} MRT_Mutex;

/**
 * @brief 使用调用方提供的控制块静态创建普通互斥锁。
 * @param storage 互斥锁控制块存储，不能为空。
 * @param out_mutex 输出互斥锁句柄，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示创建成功；参数非法时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * static MRT_Mutex mutex_cb;
 * MRT_MutexHandle mutex;
 * MRT_MutexCreateStatic(&mutex_cb, &mutex);
 */
MRT_Result MRT_MutexCreateStatic(MRT_Mutex *storage, MRT_MutexHandle *out_mutex);

/**
 * @brief 获取互斥锁。
 * @param mutex 互斥锁句柄，不能为空。
 * @param timeout 等待锁可用的 tick 数；当前任务未运行时不能调用。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示加锁成功；已被占用且 timeout 为 0 时返回 MRT_RESULT_OBJECT_BUSY；
 *         参数非法返回 MRT_RESULT_INVALID_ARGUMENT；无当前任务返回 MRT_RESULT_INVALID_CONTEXT。
 * @example
 * MRT_MutexLock(mutex, 0);
 */
MRT_Result MRT_MutexLock(MRT_MutexHandle mutex, MRT_Timeout timeout);

/**
 * @brief 释放当前任务持有的互斥锁。
 * @param mutex 互斥锁句柄，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示解锁成功；非拥有者解锁返回 MRT_RESULT_OWNER_ERROR；
 *         参数非法返回 MRT_RESULT_INVALID_ARGUMENT；无当前任务返回 MRT_RESULT_INVALID_CONTEXT。
 * @example
 * MRT_MutexUnlock(mutex);
 */
MRT_Result MRT_MutexUnlock(MRT_MutexHandle mutex);

/**
 * @brief 查询互斥锁当前拥有者。
 * @param mutex 互斥锁句柄，不能为空。
 * @param out_owner 输出拥有者任务句柄，不能为空；无拥有者时写入空指针。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示查询成功；参数非法返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * MRT_TaskHandle owner;
 * MRT_MutexGetOwner(mutex, &owner);
 */
MRT_Result MRT_MutexGetOwner(MRT_MutexHandle mutex, MRT_TaskHandle *out_owner);

#endif
