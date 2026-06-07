#ifndef MYRTOS_MRT_MEMORY_POOL_H
#define MYRTOS_MRT_MEMORY_POOL_H

/**
 * @file mrt_memory_pool.h
 * @brief MyRTOS 固定块内存池公共接口。
 *
 * 固定块内存池把调用方提供的连续内存切分为相同大小的块，并用空闲块自身存放
 * 单向空闲链表指针。该模块不依赖全局堆，适合 DMA 描述符、通信报文、DSP 采样帧等
 * 生命周期频繁但大小固定的嵌入式对象。
 */

#include "myrtos/mrt_types.h"

/**
 * @brief MyRTOS 固定块内存池控制块。
 *
 * 静态创建内存池时，调用方提供该结构体和底层块存储。结构体公开是为了支持无动态
 * 内存场景；应用代码不应直接修改字段。
 */
typedef struct MRT_MemoryPool {
    /** @brief 底层块存储起始地址。 */
    uint8_t *buffer;
    /** @brief 对齐后的单块大小，单位为字节。 */
    size_t block_size;
    /** @brief 固定块总数量。 */
    size_t block_count;
    /** @brief 底层存储总字节数，等于 block_size * block_count。 */
    size_t total_size;
    /** @brief 当前空闲块数量。 */
    size_t free_count;
    /** @brief 指向第一个空闲块的单向链表头。 */
    void *free_list;
} MRT_MemoryPool;

/**
 * @brief 使用调用方提供的控制块和字节存储静态创建固定块内存池。
 * @param block_size 用户请求的单块字节数，必须至少能容纳一个指针。
 * @param block_count 固定块数量，必须大于 0。
 * @param buffer 底层块存储起始地址，必须按指针对齐，容量至少为对齐后 block_size * block_count。
 * @param storage 内存池控制块存储，不能为 NULL。
 * @param out_pool 输出内存池句柄，不能为 NULL。
 * @return MRT_Result 返回 `MRT_RESULT_OK` 表示创建成功；参数非法时返回
 *         `MRT_RESULT_INVALID_ARGUMENT`。
 * @example
 * static MRT_MemoryPool pool_cb;
 * static uintptr_t pool_storage[32];
 * MRT_MemoryPoolHandle pool;
 * MRT_MemoryPoolCreateStatic(32, 8, pool_storage, &pool_cb, &pool);
 */
MRT_Result MRT_MemoryPoolCreateStatic(size_t block_size,
                                      size_t block_count,
                                      void *buffer,
                                      MRT_MemoryPool *storage,
                                      MRT_MemoryPoolHandle *out_pool);

/**
 * @brief 从固定块内存池分配一个块。
 * @param pool 目标内存池句柄，不能为 NULL。
 * @param out_block 输出分配到的块地址，不能为 NULL；空池时会写入 NULL。
 * @return MRT_Result 返回 `MRT_RESULT_OK` 表示分配成功；参数非法时返回
 *         `MRT_RESULT_INVALID_ARGUMENT`；没有空闲块时返回 `MRT_RESULT_OBJECT_EMPTY`。
 * @example
 * void *block;
 * MRT_MemoryPoolAlloc(pool, &block);
 */
MRT_Result MRT_MemoryPoolAlloc(MRT_MemoryPoolHandle pool, void **out_block);

/**
 * @brief 将一个块归还给固定块内存池。
 * @param pool 目标内存池句柄，不能为 NULL。
 * @param block 待释放块地址，必须是该内存池分配出的块起始地址。
 * @return MRT_Result 返回 `MRT_RESULT_OK` 表示释放成功；空参数、堆外指针、非块边界指针
 *         或重复释放时返回 `MRT_RESULT_INVALID_ARGUMENT`。
 * @example
 * MRT_MemoryPoolFree(pool, block);
 */
MRT_Result MRT_MemoryPoolFree(MRT_MemoryPoolHandle pool, void *block);

/**
 * @brief 查询固定块内存池当前空闲块数量。
 * @param pool 目标内存池句柄，不能为 NULL。
 * @param out_free_count 输出当前空闲块数量，不能为 NULL。
 * @return MRT_Result 返回 `MRT_RESULT_OK` 表示查询成功；参数非法时返回
 *         `MRT_RESULT_INVALID_ARGUMENT`。
 * @example
 * size_t free_count;
 * MRT_MemoryPoolGetFreeCount(pool, &free_count);
 */
MRT_Result MRT_MemoryPoolGetFreeCount(MRT_MemoryPoolHandle pool, size_t *out_free_count);

#endif
