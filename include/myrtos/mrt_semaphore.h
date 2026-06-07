#ifndef MYRTOS_MRT_SEMAPHORE_H
#define MYRTOS_MRT_SEMAPHORE_H

/**
 * @file mrt_semaphore.h
 * @brief MyRTOS 信号量公共接口。
 *
 * 信号量用于任务之间或 ISR 与任务之间同步资源可用状态。MyRTOS 当前提供二值信号量
 * 和计数信号量的静态创建接口，后续任务会继续扩展 take/give、ISR 和调度耦合行为。
 */

#include "myrtos/mrt_list.h"
#include "myrtos/mrt_types.h"

/**
 * @brief MyRTOS 信号量控制块。
 *
 * 静态创建信号量时，调用方提供该结构体作为控制块存储。结构体公开是为了支持无动态内存
 * 的嵌入式工程；应用代码不应直接修改字段。
 */
typedef struct MRT_Semaphore {
    /** @brief 信号量允许达到的最大计数。 */
    size_t max_count;
    /** @brief 信号量当前可获取计数。 */
    size_t count;
    /** @brief 等待获取该信号量的任务链表，后续阻塞 take 会使用。 */
    MRT_List waiting_takers;
    /** @brief 是否使用静态存储创建。 */
    bool static_storage;
} MRT_Semaphore;

/**
 * @brief 使用调用方提供的控制块静态创建二值信号量。
 * @param initially_available true 表示初始计数为 1，false 表示初始计数为 0。
 * @param storage 信号量控制块存储，不能为空。
 * @param out_semaphore 输出信号量句柄，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示创建成功；参数非法时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * static MRT_Semaphore sem_cb;
 * MRT_SemaphoreHandle sem;
 * MRT_SemaphoreCreateBinaryStatic(true, &sem_cb, &sem);
 */
MRT_Result MRT_SemaphoreCreateBinaryStatic(bool initially_available,
                                           MRT_Semaphore *storage,
                                           MRT_SemaphoreHandle *out_semaphore);

/**
 * @brief 使用调用方提供的控制块静态创建计数信号量。
 * @param max_count 最大计数，必须大于 0。
 * @param initial_count 初始计数，必须小于或等于 max_count。
 * @param storage 信号量控制块存储，不能为空。
 * @param out_semaphore 输出信号量句柄，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示创建成功；参数非法时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * static MRT_Semaphore sem_cb;
 * MRT_SemaphoreHandle sem;
 * MRT_SemaphoreCreateCountingStatic(4, 2, &sem_cb, &sem);
 */
MRT_Result MRT_SemaphoreCreateCountingStatic(size_t max_count,
                                             size_t initial_count,
                                             MRT_Semaphore *storage,
                                             MRT_SemaphoreHandle *out_semaphore);

/**
 * @brief 从 MyRTOS 全局堆动态创建二值信号量。
 * @param initially_available true 表示初始计数为 1，false 表示初始计数为 0。
 * @param out_semaphore 输出信号量句柄，不能为空；失败时写入空指针。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示创建成功；参数非法返回 MRT_RESULT_INVALID_ARGUMENT；
 *         动态分配关闭或堆空间不足时返回 MRT_RESULT_NO_MEMORY。
 * @example
 * MRT_SemaphoreHandle sem;
 * MRT_SemaphoreCreateBinary(false, &sem);
 */
MRT_Result MRT_SemaphoreCreateBinary(bool initially_available, MRT_SemaphoreHandle *out_semaphore);

/**
 * @brief 从 MyRTOS 全局堆动态创建计数信号量。
 * @param max_count 最大计数，必须大于 0。
 * @param initial_count 初始计数，必须小于或等于 max_count。
 * @param out_semaphore 输出信号量句柄，不能为空；失败时写入空指针。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示创建成功；参数非法返回 MRT_RESULT_INVALID_ARGUMENT；
 *         动态分配关闭或堆空间不足时返回 MRT_RESULT_NO_MEMORY。
 * @example
 * MRT_SemaphoreHandle sem;
 * MRT_SemaphoreCreateCounting(8, 0, &sem);
 */
MRT_Result MRT_SemaphoreCreateCounting(size_t max_count,
                                       size_t initial_count,
                                       MRT_SemaphoreHandle *out_semaphore);

/**
 * @brief 删除动态创建的信号量并归还堆内存。
 * @param semaphore 待删除信号量句柄，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示删除成功；空句柄返回 MRT_RESULT_INVALID_ARGUMENT；
 *         静态信号量或仍有等待任务时返回 MRT_RESULT_OBJECT_BUSY。
 * @example
 * MRT_SemaphoreDelete(sem);
 */
MRT_Result MRT_SemaphoreDelete(MRT_SemaphoreHandle semaphore);

/**
 * @brief 查询信号量当前计数。
 * @param semaphore 待查询信号量句柄，不能为空。
 * @return size_t 返回当前计数；信号量句柄为空时返回 0。
 * @example
 * size_t count = MRT_SemaphoreGetCount(sem);
 */
size_t MRT_SemaphoreGetCount(MRT_SemaphoreHandle semaphore);

/**
 * @brief 获取一个信号量计数。
 * @param semaphore 信号量句柄，不能为空。
 * @param timeout 等待可用计数的 tick 数；当前非调度上下文下非 0 会返回 MRT_RESULT_TIMEOUT。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示获取成功；非阻塞空信号量返回 MRT_RESULT_OBJECT_EMPTY；
 *         参数非法返回 MRT_RESULT_INVALID_ARGUMENT；等待未完成返回 MRT_RESULT_TIMEOUT。
 * @example
 * MRT_SemaphoreTake(sem, 0);
 */
MRT_Result MRT_SemaphoreTake(MRT_SemaphoreHandle semaphore, MRT_Timeout timeout);

/**
 * @brief 释放一个信号量计数。
 * @param semaphore 信号量句柄，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示释放成功；计数已满返回 MRT_RESULT_OBJECT_FULL；
 *         参数非法返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * MRT_SemaphoreGive(sem);
 */
MRT_Result MRT_SemaphoreGive(MRT_SemaphoreHandle semaphore);

/**
 * @brief 在 ISR 上下文释放一个信号量计数。
 * @param semaphore 信号量句柄，不能为空。
 * @param should_yield 输出是否需要在 ISR 退出前触发调度切换；允许为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示释放成功；计数已满返回 MRT_RESULT_OBJECT_FULL；
 *         参数非法返回 MRT_RESULT_INVALID_ARGUMENT；非 ISR 上下文返回 MRT_RESULT_INVALID_CONTEXT。
 * @example
 * bool yield;
 * MRT_SemaphoreGiveFromISR(sem, &yield);
 */
MRT_Result MRT_SemaphoreGiveFromISR(MRT_SemaphoreHandle semaphore, bool *should_yield);

#endif
