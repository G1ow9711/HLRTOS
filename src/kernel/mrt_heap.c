#include "myrtos/mrt_config.h"
#include "myrtos/mrt_heap.h"

/**
 * @brief MyRTOS 全局堆运行状态。
 *
 * 当前阶段只需要记录初始化区域和统计值；后续任务会在该结构基础上增加分配偏移和
 * 空闲块链表。
 */
typedef struct MRT_HeapState {
    /** @brief 对齐后的堆起始地址。 */
    uint8_t *start;
    /** @brief 对齐后的堆总容量，单位为字节。 */
    size_t total_size;
    /** @brief 当前剩余空闲字节数。 */
    size_t free_size;
    /** @brief 初始化以来观察到的最低剩余空闲字节数。 */
    size_t minimum_ever_free_size;
    /** @brief 当前堆管理模式。 */
    MRT_HeapMode mode;
    /** @brief 堆是否已经初始化。 */
    bool initialized;
} MRT_HeapState;

/** @brief 全局堆状态，所有动态内存 API 都访问该状态。 */
static MRT_HeapState g_heap;

/**
 * @brief 判断堆模式是否是公开枚举中的有效值。
 * @param mode 待检查的堆模式。
 * @return bool 返回 true 表示模式有效，返回 false 表示模式非法。
 * @example
 * if (!MRT_HeapModeIsValid(mode)) { return MRT_RESULT_INVALID_ARGUMENT; }
 */
static bool MRT_HeapModeIsValid(MRT_HeapMode mode)
{
    /* 逐个列出合法模式，避免未来枚举扩展时错误接受未知值。 */
    return (mode == MRT_HEAP_MODE_LINEAR) ||
           (mode == MRT_HEAP_MODE_FREE_LIST) ||
           (mode == MRT_HEAP_MODE_COALESCING);
}

/**
 * @brief 判断配置的堆对齐值是否是可用的 2 的幂。
 * @param void 无输入参数。
 * @return bool 返回 true 表示对齐配置有效，返回 false 表示配置非法。
 * @example
 * if (!MRT_HeapAlignmentIsValid()) { return MRT_RESULT_INVALID_ARGUMENT; }
 */
static bool MRT_HeapAlignmentIsValid(void)
{
    /* 对齐值不能为 0，否则无法执行取模或掩码规整。 */
    if (MRT_CFG_HEAP_ALIGNMENT == 0u) {
        /* 配置非法。 */
        return false;
    }

    /* 2 的幂满足 value & (value - 1) == 0。 */
    return (MRT_CFG_HEAP_ALIGNMENT & (MRT_CFG_HEAP_ALIGNMENT - 1u)) == 0u;
}

/**
 * @brief 将地址向上规整到堆对齐边界。
 * @param address 原始地址整数值。
 * @return uintptr_t 返回对齐后的地址整数值。
 * @example
 * aligned = MRT_HeapAlignAddressUp(raw);
 */
static uintptr_t MRT_HeapAlignAddressUp(uintptr_t address)
{
    /* 计算对齐掩码；该函数只在对齐值有效时调用。 */
    uintptr_t mask = (uintptr_t)MRT_CFG_HEAP_ALIGNMENT - 1u;

    /* 加上掩码后清除低位，实现向上对齐。 */
    return (address + mask) & ~mask;
}

/**
 * @brief 将字节数向下规整到堆对齐粒度。
 * @param size 原始字节数。
 * @return size_t 返回对齐后的字节数。
 * @example
 * usable = MRT_HeapAlignSizeDown(size);
 */
static size_t MRT_HeapAlignSizeDown(size_t size)
{
    /* 计算对齐掩码；该函数只在对齐值有效时调用。 */
    size_t mask = MRT_CFG_HEAP_ALIGNMENT - 1u;

    /* 清除低位，实现向下对齐。 */
    return size & ~mask;
}

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
MRT_Result MRT_HeapInitialize(void *buffer, size_t size, MRT_HeapMode mode)
{
    /* 堆对齐配置必须有效，否则后续地址规整没有可靠语义。 */
    if (!MRT_HeapAlignmentIsValid()) {
        /* 返回参数错误，提示配置需要修正。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 堆区域不能为空。 */
    if (buffer == 0) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 堆模式必须是公开枚举中的有效值。 */
    if (!MRT_HeapModeIsValid(mode)) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 原始容量必须至少达到一个对齐粒度。 */
    if (size < MRT_CFG_HEAP_ALIGNMENT) {
        /* 返回参数错误，避免初始化无法分配任何对齐块的堆。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 将起始地址转换为整数，便于对齐计算。 */
    uintptr_t raw_start = (uintptr_t)buffer;

    /* 将起始地址向上规整到堆对齐边界。 */
    uintptr_t aligned_start = MRT_HeapAlignAddressUp(raw_start);

    /* 计算因为起始地址对齐而损失的前导字节数。 */
    size_t leading_loss = (size_t)(aligned_start - raw_start);

    /* 如果对齐损失已经耗尽区域，则该堆不可用。 */
    if (leading_loss >= size) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 从剩余容量中扣除前导损失。 */
    size_t remaining = size - leading_loss;

    /* 将可用容量向下规整到堆对齐粒度。 */
    size_t aligned_size = MRT_HeapAlignSizeDown(remaining);

    /* 规整后容量仍需至少达到一个对齐粒度。 */
    if (aligned_size < MRT_CFG_HEAP_ALIGNMENT) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 记录对齐后的堆起始地址。 */
    g_heap.start = (uint8_t *)aligned_start;

    /* 记录对齐后的总容量。 */
    g_heap.total_size = aligned_size;

    /* 初始化时全部堆空间均为空闲。 */
    g_heap.free_size = aligned_size;

    /* 初始化时历史最低水位等于当前空闲空间。 */
    g_heap.minimum_ever_free_size = aligned_size;

    /* 保存堆管理模式。 */
    g_heap.mode = mode;

    /* 标记全局堆已经初始化。 */
    g_heap.initialized = true;

    /* 堆初始化成功。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 查询当前堆剩余空闲字节数。
 * @param out_free_size 输出当前空闲字节数，不能为 NULL。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示查询成功；堆尚未初始化返回 MRT_RESULT_NOT_STARTED；
 *         参数非法时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * size_t free_size;
 * MRT_HeapGetFreeSize(&free_size);
 */
MRT_Result MRT_HeapGetFreeSize(size_t *out_free_size)
{
    /* 输出指针不能为空。 */
    if (out_free_size == 0) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 堆必须先初始化才能查询。 */
    if (!g_heap.initialized) {
        /* 返回未启动状态。 */
        return MRT_RESULT_NOT_STARTED;
    }

    /* 写回当前空闲空间。 */
    *out_free_size = g_heap.free_size;

    /* 查询成功。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 查询堆初始化后历史最低剩余空闲字节数。
 * @param out_minimum_free_size 输出历史最低剩余空闲字节数，不能为 NULL。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示查询成功；堆尚未初始化返回 MRT_RESULT_NOT_STARTED；
 *         参数非法时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * size_t minimum_free;
 * MRT_HeapGetMinimumEverFreeSize(&minimum_free);
 */
MRT_Result MRT_HeapGetMinimumEverFreeSize(size_t *out_minimum_free_size)
{
    /* 输出指针不能为空。 */
    if (out_minimum_free_size == 0) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 堆必须先初始化才能查询。 */
    if (!g_heap.initialized) {
        /* 返回未启动状态。 */
        return MRT_RESULT_NOT_STARTED;
    }

    /* 写回历史最低剩余空间。 */
    *out_minimum_free_size = g_heap.minimum_ever_free_size;

    /* 查询成功。 */
    return MRT_RESULT_OK;
}
