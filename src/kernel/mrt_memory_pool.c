#include "myrtos/mrt_config.h"
#include "myrtos/mrt_memory_pool.h"

/**
 * @brief 计算固定块内存池需要使用的对齐粒度。
 * @param void 无输入参数。
 * @return size_t 返回不小于指针大小的对齐粒度。
 * @example
 * size_t alignment = MRT_MemoryPoolAlignment();
 */
static size_t MRT_MemoryPoolAlignment(void)
{
    /* 空闲块内部要存放指针，因此对齐粒度至少为指针大小。 */
    if (MRT_CFG_HEAP_ALIGNMENT < sizeof(void *)) {
        /* 配置对齐不足时提升到指针大小。 */
        return sizeof(void *);
    }

    /* 默认复用堆对齐配置，便于系统内存对象保持一致。 */
    return MRT_CFG_HEAP_ALIGNMENT;
}

/**
 * @brief 将字节数向上规整到指定对齐粒度。
 * @param size 原始字节数。
 * @param alignment 对齐粒度，必须大于 0。
 * @return size_t 返回向上对齐后的字节数；溢出时返回 0。
 * @example
 * size_t aligned = MRT_MemoryPoolAlignSizeUp(13, 8);
 */
static size_t MRT_MemoryPoolAlignSizeUp(size_t size, size_t alignment)
{
    /* 对齐粒度为 0 没有有效语义。 */
    if (alignment == 0u) {
        /* 返回 0 表示无法对齐。 */
        return 0u;
    }

    /* 加法前检查是否会溢出。 */
    if (size > (SIZE_MAX - (alignment - 1u))) {
        /* 返回 0 表示请求过大。 */
        return 0u;
    }

    /* 使用除法形式，避免要求 alignment 必须是 2 的幂。 */
    return ((size + alignment - 1u) / alignment) * alignment;
}

/**
 * @brief 判断指针是否满足内存池对齐要求。
 * @param ptr 待检查指针。
 * @param alignment 对齐粒度。
 * @return bool 返回 true 表示已对齐，返回 false 表示未对齐。
 * @example
 * if (!MRT_MemoryPoolPointerIsAligned(buffer, alignment)) { return error; }
 */
static bool MRT_MemoryPoolPointerIsAligned(const void *ptr, size_t alignment)
{
    /* 将指针转换为整数以执行取模检查。 */
    uintptr_t value = (uintptr_t)ptr;

    /* 余数为 0 表示满足对齐。 */
    return (value % alignment) == 0u;
}

/**
 * @brief 获取空闲块中存放的下一个空闲块指针。
 * @param block 空闲块起始地址。
 * @return void* 返回下一个空闲块地址。
 * @example
 * void *next = MRT_MemoryPoolReadNextFree(block);
 */
static void *MRT_MemoryPoolReadNextFree(void *block)
{
    /* 空闲块开头存放一个 void* 指针。 */
    return *(void **)block;
}

/**
 * @brief 在空闲块中写入下一个空闲块指针。
 * @param block 空闲块起始地址。
 * @param next 下一个空闲块地址。
 * @return void 无返回值。
 * @example
 * MRT_MemoryPoolWriteNextFree(block, next);
 */
static void MRT_MemoryPoolWriteNextFree(void *block, void *next)
{
    /* 空闲块开头存放一个 void* 指针。 */
    *(void **)block = next;
}

/**
 * @brief 判断块指针是否属于指定内存池且位于块边界。
 * @param pool 目标内存池。
 * @param block 待检查块指针。
 * @return bool 返回 true 表示指针属于该池且位于块起点，否则返回 false。
 * @example
 * if (!MRT_MemoryPoolPointerBelongsToPool(pool, block)) { return error; }
 */
static bool MRT_MemoryPoolPointerBelongsToPool(MRT_MemoryPoolHandle pool, void *block)
{
    /* 将底层存储起点转换为整数。 */
    uintptr_t start = (uintptr_t)pool->buffer;

    /* 将待检查指针转换为整数。 */
    uintptr_t value = (uintptr_t)block;

    /* 指针不能位于底层存储起点之前。 */
    if (value < start) {
        /* 指针不属于该内存池。 */
        return false;
    }

    /* 计算指针相对池起点的偏移。 */
    size_t offset = (size_t)(value - start);

    /* 偏移必须落在底层存储范围内，避免使用 start + total_size 产生地址回绕风险。 */
    if (offset >= pool->total_size) {
        /* 指针不属于该内存池。 */
        return false;
    }

    /* 偏移必须正好落在固定块边界。 */
    return (offset % pool->block_size) == 0u;
}

/**
 * @brief 判断块指针是否已经存在于空闲链表中。
 * @param pool 目标内存池。
 * @param block 待检查块指针。
 * @return bool 返回 true 表示该块已经空闲，返回 false 表示该块当前未在空闲链表中。
 * @example
 * if (MRT_MemoryPoolBlockIsAlreadyFree(pool, block)) { return error; }
 */
static bool MRT_MemoryPoolBlockIsAlreadyFree(MRT_MemoryPoolHandle pool, void *block)
{
    /* 从空闲链表头开始扫描。 */
    void *current = pool->free_list;

    /* 遍历所有当前空闲块。 */
    while (current != 0) {
        /* 找到相同地址说明该块已经空闲。 */
        if (current == block) {
            /* 返回 true 表示重复释放。 */
            return true;
        }

        /* 读取下一个空闲块。 */
        current = MRT_MemoryPoolReadNextFree(current);
    }

    /* 未在空闲链表中找到该块。 */
    return false;
}

/**
 * @brief 使用调用方提供的控制块和字节存储静态创建固定块内存池。
 * @param block_size 用户请求的单块字节数，必须至少能容纳一个指针。
 * @param block_count 固定块数量，必须大于 0。
 * @param buffer 底层块存储起始地址，必须按指针对齐。
 * @param storage 内存池控制块存储，不能为 NULL。
 * @param out_pool 输出内存池句柄，不能为 NULL。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示创建成功；参数非法时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * MRT_MemoryPoolCreateStatic(32, 8, pool_storage, &pool_cb, &pool);
 */
MRT_Result MRT_MemoryPoolCreateStatic(size_t block_size,
                                      size_t block_count,
                                      void *buffer,
                                      MRT_MemoryPool *storage,
                                      MRT_MemoryPoolHandle *out_pool)
{
    /* 底层存储、控制块和输出句柄都不能为空。 */
    if ((buffer == 0) || (storage == 0) || (out_pool == 0)) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 固定块数量必须大于 0。 */
    if (block_count == 0u) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 单块至少要能存放一个空闲链表指针。 */
    if (block_size < sizeof(void *)) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 计算内存池使用的对齐粒度。 */
    size_t alignment = MRT_MemoryPoolAlignment();

    /* 底层存储必须满足对齐要求。 */
    if (!MRT_MemoryPoolPointerIsAligned(buffer, alignment)) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 将用户块大小向上规整。 */
    size_t aligned_block_size = MRT_MemoryPoolAlignSizeUp(block_size, alignment);

    /* 对齐失败或溢出时拒绝创建。 */
    if (aligned_block_size == 0u) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 检查总字节数乘法是否会溢出。 */
    if (block_count > (SIZE_MAX / aligned_block_size)) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 初始化底层存储起始地址。 */
    storage->buffer = (uint8_t *)buffer;

    /* 保存对齐后的单块大小。 */
    storage->block_size = aligned_block_size;

    /* 保存固定块总数量。 */
    storage->block_count = block_count;

    /* 保存底层存储总字节数。 */
    storage->total_size = aligned_block_size * block_count;

    /* 初始状态全部块均为空闲。 */
    storage->free_count = block_count;

    /* 空闲链表从第一个块开始。 */
    storage->free_list = storage->buffer;

    /* 遍历每个块并写入下一个空闲块指针。 */
    for (size_t index = 0u; index < block_count; index++) {
        /* 计算当前块地址。 */
        uint8_t *current = storage->buffer + (index * aligned_block_size);

        /* 最后一个块的 next 为 NULL，其余块指向下一个块。 */
        void *next = (index + 1u < block_count)
                         ? (void *)(current + aligned_block_size)
                         : 0;

        /* 在当前空闲块开头写入 next 指针。 */
        MRT_MemoryPoolWriteNextFree(current, next);
    }

    /* 输出内存池句柄。 */
    *out_pool = storage;

    /* 创建成功。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 从固定块内存池分配一个块。
 * @param pool 目标内存池句柄，不能为 NULL。
 * @param out_block 输出分配到的块地址，不能为 NULL；空池时会写入 NULL。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示分配成功；参数非法时返回 MRT_RESULT_INVALID_ARGUMENT；
 *         没有空闲块时返回 MRT_RESULT_OBJECT_EMPTY。
 * @example
 * MRT_MemoryPoolAlloc(pool, &block);
 */
MRT_Result MRT_MemoryPoolAlloc(MRT_MemoryPoolHandle pool, void **out_block)
{
    /* 池句柄和输出指针不能为空。 */
    if ((pool == 0) || (out_block == 0)) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 默认先清空输出，避免失败路径留下旧指针。 */
    *out_block = 0;

    /* 没有空闲块时返回对象空。 */
    if ((pool->free_count == 0u) || (pool->free_list == 0)) {
        /* 返回对象空。 */
        return MRT_RESULT_OBJECT_EMPTY;
    }

    /* 取出空闲链表头。 */
    void *block = pool->free_list;

    /* 空闲链表头后移到下一个空闲块。 */
    pool->free_list = MRT_MemoryPoolReadNextFree(block);

    /* 空闲块数量减一。 */
    pool->free_count--;

    /* 输出分配到的块地址。 */
    *out_block = block;

    /* 分配成功。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 将一个块归还给固定块内存池。
 * @param pool 目标内存池句柄，不能为 NULL。
 * @param block 待释放块地址，必须是该内存池分配出的块起始地址。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示释放成功；空参数、堆外指针、非块边界指针
 *         或重复释放时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * MRT_MemoryPoolFree(pool, block);
 */
MRT_Result MRT_MemoryPoolFree(MRT_MemoryPoolHandle pool, void *block)
{
    /* 池句柄和块指针不能为空。 */
    if ((pool == 0) || (block == 0)) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 待释放指针必须属于该池并位于固定块边界。 */
    if (!MRT_MemoryPoolPointerBelongsToPool(pool, block)) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 如果块已经在空闲链表中，说明调用方重复释放。 */
    if (MRT_MemoryPoolBlockIsAlreadyFree(pool, block)) {
        /* 返回参数错误，保护空闲链表不被破坏。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 空闲计数不应超过总块数，超过说明控制块状态已异常。 */
    if (pool->free_count >= pool->block_count) {
        /* 返回参数错误，避免计数继续溢出。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 将当前空闲链表头写入待释放块。 */
    MRT_MemoryPoolWriteNextFree(block, pool->free_list);

    /* 待释放块成为新的空闲链表头。 */
    pool->free_list = block;

    /* 空闲块数量加一。 */
    pool->free_count++;

    /* 释放成功。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 查询固定块内存池当前空闲块数量。
 * @param pool 目标内存池句柄，不能为 NULL。
 * @param out_free_count 输出当前空闲块数量，不能为 NULL。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示查询成功；参数非法时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * MRT_MemoryPoolGetFreeCount(pool, &free_count);
 */
MRT_Result MRT_MemoryPoolGetFreeCount(MRT_MemoryPoolHandle pool, size_t *out_free_count)
{
    /* 池句柄和输出指针不能为空。 */
    if ((pool == 0) || (out_free_count == 0)) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 写回当前空闲块数量。 */
    *out_free_count = pool->free_count;

    /* 查询成功。 */
    return MRT_RESULT_OK;
}
