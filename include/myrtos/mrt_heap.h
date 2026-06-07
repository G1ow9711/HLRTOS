#ifndef MYRTOS_MRT_HEAP_H
#define MYRTOS_MRT_HEAP_H

/**
 * @file mrt_heap.h
 * @brief MyRTOS 堆内存管理公共接口。
 *
 * 本模块使用调用方提供的连续字节区域作为全局堆，支持线性堆、可释放空闲链表堆、
 * 相邻块合并堆三种策略。线性堆适合启动期一次性分配；空闲链表堆适合对象生命周期
 * 明确的系统；合并堆用于降低相邻空闲块造成的碎片。所有 API 均保持 `MRT_` 前缀，
 * 避免与其他 RTOS 或芯片 SDK 的符号冲突。
 */

#include "myrtos/mrt_types.h"

/**
 * @brief MyRTOS 堆管理策略。
 *
 * 用户在 `MRT_HeapInitialize()` 中选择堆策略。初始化后再次调用
 * `MRT_HeapInitialize()` 会重置堆区域、统计信息和策略。
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
 * @param buffer 堆区域起始地址，不能为 NULL；函数会把该地址向上规整到堆对齐边界。
 * @param size 堆区域原始字节数；线性堆至少需要一个对齐粒度，可释放堆还需要容纳块头。
 * @param mode 堆管理策略，必须是 `MRT_HeapMode` 枚举中的有效值。
 * @return MRT_Result 返回 `MRT_RESULT_OK` 表示初始化成功；参数非法或容量过小时返回
 *         `MRT_RESULT_INVALID_ARGUMENT`。
 * @example
 * static uint8_t heap[4096];
 * MRT_Result result = MRT_HeapInitialize(heap, sizeof(heap), MRT_HEAP_MODE_COALESCING);
 */
MRT_Result MRT_HeapInitialize(void *buffer, size_t size, MRT_HeapMode mode);

/**
 * @brief 从 MyRTOS 全局堆分配一段对齐内存。
 * @param size 请求分配的用户字节数，必须大于 0；函数会自动向上规整到堆对齐粒度。
 * @return void* 分配成功时返回用户可写的对齐地址；堆未初始化、请求为 0、请求过大、
 *         空间不足或碎片无法满足时返回 NULL。
 * @example
 * void *block = MRT_Malloc(128);
 * if (block != 0) {
 *     MRT_Free(block);
 * }
 */
void *MRT_Malloc(size_t size);

/**
 * @brief 释放由 MyRTOS 全局堆分配的内存块。
 * @param ptr 待释放指针；NULL 指针被视为无操作成功；非载荷起始地址会被拒绝。
 * @return MRT_Result 返回 `MRT_RESULT_OK` 表示释放成功；线性堆不支持释放单块时返回
 *         `MRT_RESULT_OBJECT_BUSY`；堆尚未初始化时返回 `MRT_RESULT_NOT_STARTED`；
 *         堆外指针或重复释放时返回 `MRT_RESULT_INVALID_ARGUMENT`。
 * @example
 * void *block = MRT_Malloc(64);
 * MRT_Result result = MRT_Free(block);
 */
MRT_Result MRT_Free(void *ptr);

/**
 * @brief 查询当前堆剩余空闲空间字节数。
 * @param out_free_size 输出当前空闲字节数，不能为 NULL。
 * @return MRT_Result 返回 `MRT_RESULT_OK` 表示查询成功；堆尚未初始化时返回
 *         `MRT_RESULT_NOT_STARTED`；输出指针为空时返回 `MRT_RESULT_INVALID_ARGUMENT`。
 * @example
 * size_t free_size = 0;
 * MRT_HeapGetFreeSize(&free_size);
 */
MRT_Result MRT_HeapGetFreeSize(size_t *out_free_size);

/**
 * @brief 查询堆初始化以来的历史最低剩余空闲空间字节数。
 * @param out_minimum_free_size 输出历史最低剩余空间字节数，不能为 NULL。
 * @return MRT_Result 返回 `MRT_RESULT_OK` 表示查询成功；堆尚未初始化时返回
 *         `MRT_RESULT_NOT_STARTED`；输出指针为空时返回 `MRT_RESULT_INVALID_ARGUMENT`。
 * @example
 * size_t minimum_free = 0;
 * MRT_HeapGetMinimumEverFreeSize(&minimum_free);
 */
MRT_Result MRT_HeapGetMinimumEverFreeSize(size_t *out_minimum_free_size);

#endif
