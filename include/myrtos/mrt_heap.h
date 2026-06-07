#ifndef MYRTOS_MRT_HEAP_H
#define MYRTOS_MRT_HEAP_H

/**
 * @file mrt_heap.h
 * @brief MyRTOS 堆内存管理公共接口。
 *
 * 本模块使用调用方提供的连续字节区域作为内核堆，支持线性堆、可释放空闲链表堆和
 * 空闲块合并堆三种策略。当前文件只暴露初始化和查询接口，分配/释放接口会在后续
 * 任务中按 TDD 增量加入。
 */

#include "myrtos/mrt_types.h"

/**
 * @brief MyRTOS 堆管理策略。
 *
 * 不同策略服务不同嵌入式场景：线性堆适合启动期一次性分配，可释放堆适合对象生命周期
 * 明确的系统，合并堆用于降低释放相邻块后的碎片。
 */
typedef enum MRT_HeapMode {
    /** @brief 线性增长堆，分配后不支持回收单个块。 */
    MRT_HEAP_MODE_LINEAR = 0,
    /** @brief 空闲链表堆，释放后的块可复用但不合并相邻块。 */
    MRT_HEAP_MODE_FREE_LIST,
    /** @brief 合并堆，释放相邻块后自动合并以减少碎片。 */
    MRT_HEAP_MODE_COALESCING
} MRT_HeapMode;

/**
 * @brief 使用调用方提供的内存区域初始化 MyRTOS 全局堆。
 * @param buffer 堆区域起始地址，不能为 NULL。
 * @param size 堆区域字节数，必须至少能容纳一个对齐粒度。
 * @param mode 堆管理策略，必须是 MRT_HeapMode 中的有效值。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示初始化成功；参数非法时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * static uint8_t heap[4096];
 * MRT_HeapInitialize(heap, sizeof(heap), MRT_HEAP_MODE_COALESCING);
 */
MRT_Result MRT_HeapInitialize(void *buffer, size_t size, MRT_HeapMode mode);

/**
 * @brief 查询当前堆剩余空闲字节数。
 * @param out_free_size 输出当前空闲字节数，不能为 NULL。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示查询成功；堆尚未初始化返回 MRT_RESULT_NOT_STARTED；
 *         参数非法时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * size_t free_size;
 * MRT_HeapGetFreeSize(&free_size);
 */
MRT_Result MRT_HeapGetFreeSize(size_t *out_free_size);

/**
 * @brief 查询堆初始化后历史最低剩余空闲字节数。
 * @param out_minimum_free_size 输出历史最低剩余空闲字节数，不能为 NULL。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示查询成功；堆尚未初始化返回 MRT_RESULT_NOT_STARTED；
 *         参数非法时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * size_t minimum_free;
 * MRT_HeapGetMinimumEverFreeSize(&minimum_free);
 */
MRT_Result MRT_HeapGetMinimumEverFreeSize(size_t *out_minimum_free_size);

#endif
