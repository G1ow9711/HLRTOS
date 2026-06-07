#include "myrtos/mrt_config.h"
#include "myrtos/mrt_heap.h"

/**
 * @brief 可释放堆的块头。
 *
 * 空闲链表堆和合并堆都把堆区切分为若干地址连续的块。每个块从该头部开始，
 * `size` 记录包含块头、对齐填充和用户载荷在内的整块字节数；`next` 与
 * `previous` 按地址顺序串联所有块；`allocated` 标记该块是否已经交给用户。
 */
typedef struct MRT_HeapBlock {
    /** @brief 当前块总字节数，包含块头、填充和用户载荷。 */
    size_t size;
    /** @brief 地址顺序上的下一个块。 */
    struct MRT_HeapBlock *next;
    /** @brief 地址顺序上的上一个块。 */
    struct MRT_HeapBlock *previous;
    /** @brief true 表示已分配，false 表示空闲。 */
    bool allocated;
} MRT_HeapBlock;

/**
 * @brief MyRTOS 全局堆运行状态。
 *
 * 该状态只描述一个调用方提供的堆区。线性堆使用 `allocation_offset` 递增分配；
 * 可释放堆使用 `first_block` 维护地址顺序块表。所有统计值均以堆内实际占用的
 * 整块字节数为单位，因此可释放堆的块头开销也会反映在剩余空间中。
 */
typedef struct MRT_HeapState {
    /** @brief 对齐后的堆起始地址。 */
    uint8_t *start;
    /** @brief 对齐后的堆总容量，单位为字节。 */
    size_t total_size;
    /** @brief 线性堆下一次分配的偏移位置，单位为字节。 */
    size_t allocation_offset;
    /** @brief 当前剩余空闲字节数。 */
    size_t free_size;
    /** @brief 初始化以来观察到的最低剩余空闲字节数。 */
    size_t minimum_ever_free_size;
    /** @brief 当前堆管理模式。 */
    MRT_HeapMode mode;
    /** @brief 可释放堆的首个块头；线性堆不使用该字段。 */
    MRT_HeapBlock *first_block;
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
 * uintptr_t aligned = MRT_HeapAlignAddressUp(raw);
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
 * size_t usable = MRT_HeapAlignSizeDown(size);
 */
static size_t MRT_HeapAlignSizeDown(size_t size)
{
    /* 计算对齐掩码；该函数只在对齐值有效时调用。 */
    size_t mask = MRT_CFG_HEAP_ALIGNMENT - 1u;

    /* 清除低位，实现向下对齐。 */
    return size & ~mask;
}

/**
 * @brief 将字节数向上规整到堆对齐粒度。
 * @param size 原始字节数。
 * @return size_t 返回向上对齐后的字节数。
 * @example
 * size_t aligned = MRT_HeapAlignSizeUp(size);
 */
static size_t MRT_HeapAlignSizeUp(size_t size)
{
    /* 计算对齐掩码；该函数只在对齐值有效时调用。 */
    size_t mask = MRT_CFG_HEAP_ALIGNMENT - 1u;

    /* 加上掩码后清除低位，实现向上对齐。 */
    return (size + mask) & ~mask;
}

/**
 * @brief 计算可释放堆块头占用的对齐后字节数。
 * @param void 无输入参数。
 * @return size_t 返回块头与载荷之间需要保留的对齐后字节数。
 * @example
 * size_t header_size = MRT_HeapBlockHeaderSize();
 */
static size_t MRT_HeapBlockHeaderSize(void)
{
    /* 用户载荷紧跟块头，因此块头大小必须向上对齐。 */
    return MRT_HeapAlignSizeUp(sizeof(MRT_HeapBlock));
}

/**
 * @brief 计算可继续作为空闲块管理的最小块大小。
 * @param void 无输入参数。
 * @return size_t 返回块头加一个最小对齐载荷的总字节数。
 * @example
 * if (remaining >= MRT_HeapMinimumFreeBlockSize()) { split(); }
 */
static size_t MRT_HeapMinimumFreeBlockSize(void)
{
    /* 剩余块必须至少能容纳自己的块头。 */
    size_t header_size = MRT_HeapBlockHeaderSize();

    /* 块头之后还必须能容纳一个对齐粒度的用户载荷。 */
    return header_size + MRT_CFG_HEAP_ALIGNMENT;
}

/**
 * @brief 在空闲空间降低后刷新历史最低水位。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * MRT_HeapUpdateMinimumFreeSize();
 */
static void MRT_HeapUpdateMinimumFreeSize(void)
{
    /* 只有当前空闲空间低于历史记录时才更新水位。 */
    if (g_heap.free_size < g_heap.minimum_ever_free_size) {
        /* 保存新的历史最低剩余空间。 */
        g_heap.minimum_ever_free_size = g_heap.free_size;
    }
}

/**
 * @brief 初始化可释放堆的首个空闲块。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * MRT_HeapInitializeFirstFreeBlock();
 */
static void MRT_HeapInitializeFirstFreeBlock(void)
{
    /* 可释放堆从堆首地址放置第一个块头。 */
    MRT_HeapBlock *block = (MRT_HeapBlock *)g_heap.start;

    /* 初始块覆盖整个对齐后的堆区域。 */
    block->size = g_heap.total_size;

    /* 初始状态只有一个块，因此没有后继块。 */
    block->next = 0;

    /* 初始状态只有一个块，因此没有前驱块。 */
    block->previous = 0;

    /* 初始块尚未分配给用户。 */
    block->allocated = false;

    /* 全局状态记录首块，后续 first-fit 从这里开始扫描。 */
    g_heap.first_block = block;
}

/**
 * @brief 从用户指针查找对应的堆块。
 * @param ptr 用户传入的待释放指针。
 * @return MRT_HeapBlock* 找到匹配载荷起始地址时返回块头；未找到时返回 NULL。
 * @example
 * MRT_HeapBlock *block = MRT_HeapFindBlockByPayload(ptr);
 */
static MRT_HeapBlock *MRT_HeapFindBlockByPayload(void *ptr)
{
    /* 计算块头与用户载荷之间的固定偏移。 */
    size_t header_size = MRT_HeapBlockHeaderSize();

    /* 从首块开始按地址顺序扫描全部堆块。 */
    MRT_HeapBlock *block = g_heap.first_block;

    /* 遍历直到链表结束。 */
    while (block != 0) {
        /* 用户可见地址位于块头之后的对齐边界。 */
        uint8_t *payload = ((uint8_t *)block) + header_size;

        /* 只有载荷起始地址完全一致才视为该块所有者。 */
        if (payload == (uint8_t *)ptr) {
            /* 返回匹配到的块头。 */
            return block;
        }

        /* 继续检查下一个块。 */
        block = block->next;
    }

    /* 未找到匹配块，说明指针不属于当前堆分配结果。 */
    return 0;
}

/**
 * @brief 将空闲块按请求大小拆分为已分配块和剩余空闲块。
 * @param block 待拆分的空闲块。
 * @param allocated_size 本次分配需要占用的整块字节数，包含块头和载荷。
 * @return void 无返回值。
 * @example
 * MRT_HeapSplitBlockIfUseful(block, required_size);
 */
static void MRT_HeapSplitBlockIfUseful(MRT_HeapBlock *block, size_t allocated_size)
{
    /* 计算原始块扣除本次分配后剩余的字节数。 */
    size_t remaining = block->size - allocated_size;

    /* 剩余空间不足以容纳一个新块时，整块分配，避免产生不可管理碎片。 */
    if (remaining < MRT_HeapMinimumFreeBlockSize()) {
        /* 不拆分时保持原块大小不变。 */
        return;
    }

    /* 新空闲块位于已分配区域之后。 */
    MRT_HeapBlock *new_block = (MRT_HeapBlock *)(((uint8_t *)block) + allocated_size);

    /* 新块获得原块剩余的全部空间。 */
    new_block->size = remaining;

    /* 新块继承原块的后继。 */
    new_block->next = block->next;

    /* 新块的前驱是当前块。 */
    new_block->previous = block;

    /* 新块保持空闲状态。 */
    new_block->allocated = false;

    /* 如果原块后面还有块，需要让后继块回指新块。 */
    if (new_block->next != 0) {
        /* 修正后继块的前驱链接。 */
        new_block->next->previous = new_block;
    }

    /* 当前块大小收缩为本次分配实际占用的大小。 */
    block->size = allocated_size;

    /* 当前块后继改为拆分出来的新空闲块。 */
    block->next = new_block;
}

/**
 * @brief 从可释放堆执行 first-fit 分配。
 * @param aligned_size 已按堆对齐粒度规整的用户请求字节数。
 * @return void* 分配成功返回用户载荷地址；空间不足或碎片不满足时返回 NULL。
 * @example
 * void *ptr = MRT_HeapAllocateFromFreeList(aligned_size);
 */
static void *MRT_HeapAllocateFromFreeList(size_t aligned_size)
{
    /* 计算块头对齐后大小。 */
    size_t header_size = MRT_HeapBlockHeaderSize();

    /* 防止块头加用户载荷时发生 size_t 回绕。 */
    if (aligned_size > (SIZE_MAX - header_size)) {
        /* 回绕风险视为空间不足。 */
        return 0;
    }

    /* 本次需要占用的整块大小包含块头和用户载荷。 */
    size_t required_size = header_size + aligned_size;

    /* 如果统计层面的总空闲字节都不够，直接失败。 */
    if (required_size > g_heap.free_size) {
        /* 返回空指针表示堆空间不足。 */
        return 0;
    }

    /* 从首块开始执行 first-fit 扫描。 */
    MRT_HeapBlock *block = g_heap.first_block;

    /* 遍历全部块，寻找第一个足够大的空闲块。 */
    while (block != 0) {
        /* 只有空闲且容量足够的块才能承载本次分配。 */
        if ((!block->allocated) && (block->size >= required_size)) {
            /* 若剩余空间足够形成新块，则拆分；否则整块分配。 */
            MRT_HeapSplitBlockIfUseful(block, required_size);

            /* 标记当前块已经交给用户。 */
            block->allocated = true;

            /* 空闲字节减少当前块实际占用的整块大小。 */
            g_heap.free_size -= block->size;

            /* 更新历史最低空闲水位。 */
            MRT_HeapUpdateMinimumFreeSize();

            /* 返回块头之后的对齐载荷地址。 */
            return ((uint8_t *)block) + header_size;
        }

        /* 当前块不可用，继续扫描后继块。 */
        block = block->next;
    }

    /* 总空闲空间可能足够但碎片不满足 first-fit 容量要求。 */
    return 0;
}

/**
 * @brief 将用户块归还给可释放堆。
 * @param ptr 用户传入的待释放指针。
 * @return MRT_Result 释放成功返回 MRT_RESULT_OK；堆外指针或重复释放返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * MRT_Free(ptr);
 */
static MRT_Result MRT_HeapFreeToFreeList(void *ptr)
{
    /* 扫描块表确认该指针是否正好等于某个载荷起始地址。 */
    MRT_HeapBlock *block = MRT_HeapFindBlockByPayload(ptr);

    /* 未找到匹配块时，说明指针不属于当前堆。 */
    if (block == 0) {
        /* 拒绝堆外指针或非载荷起点指针。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 已经处于空闲状态的块不能再次释放。 */
    if (!block->allocated) {
        /* 重复释放会破坏统计值，必须拒绝。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 将块状态改回空闲。 */
    block->allocated = false;

    /* 归还当前块实际占用的整块字节数。 */
    g_heap.free_size += block->size;

    /* 空闲链表模式不合并相邻块；合并模式会在后续任务单独实现。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 使用调用方提供的内存区域初始化 MyRTOS 全局堆。
 * @param buffer 堆区域起始地址，不能为 NULL。
 * @param size 堆区域字节数，必须至少能容纳所选模式需要的最小堆块。
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

    /* 可释放堆还必须能容纳一个块头和一个最小载荷。 */
    if ((mode != MRT_HEAP_MODE_LINEAR) &&
        (aligned_size < MRT_HeapMinimumFreeBlockSize())) {
        /* 返回参数错误，避免建立无法分配用户块的可释放堆。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 记录对齐后的堆起始地址。 */
    g_heap.start = (uint8_t *)aligned_start;

    /* 记录对齐后的总容量。 */
    g_heap.total_size = aligned_size;

    /* 初始化线性分配偏移，从堆起点开始分配。 */
    g_heap.allocation_offset = 0u;

    /* 初始化时全部堆空间均为空闲空间。 */
    g_heap.free_size = aligned_size;

    /* 初始化时历史最低水位等于当前空闲空间。 */
    g_heap.minimum_ever_free_size = aligned_size;

    /* 保存堆管理模式。 */
    g_heap.mode = mode;

    /* 默认没有可释放块表，线性模式会保持该状态。 */
    g_heap.first_block = 0;

    /* 标记全局堆已经初始化。 */
    g_heap.initialized = true;

    /* 可释放堆需要在堆首建立初始空闲块。 */
    if (mode != MRT_HEAP_MODE_LINEAR) {
        /* 初始化首个空闲块。 */
        MRT_HeapInitializeFirstFreeBlock();
    }

    /* 堆初始化成功。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 从 MyRTOS 全局堆分配一段对齐内存。
 * @param size 请求分配的用户字节数，必须大于 0。
 * @return void* 返回分配成功的对齐地址；堆未初始化、size 为 0 或空间不足时返回 NULL。
 * @example
 * void *block = MRT_Malloc(128);
 */
void *MRT_Malloc(size_t size)
{
    /* 堆未初始化时不能分配。 */
    if (!g_heap.initialized) {
        /* 返回空指针表示失败。 */
        return 0;
    }

    /* 零长度分配没有明确所有权，直接失败。 */
    if (size == 0u) {
        /* 返回空指针表示无需分配。 */
        return 0;
    }

    /* 将请求大小向上规整到堆对齐粒度。 */
    size_t aligned_size = MRT_HeapAlignSizeUp(size);

    /* 对齐后的大小不能回绕。 */
    if (aligned_size < size) {
        /* 返回空指针表示请求过大。 */
        return 0;
    }

    /* 可释放堆使用块头和 first-fit 扫描。 */
    if (g_heap.mode != MRT_HEAP_MODE_LINEAR) {
        /* 交给可释放堆分配路径处理。 */
        return MRT_HeapAllocateFromFreeList(aligned_size);
    }

    /* 线性堆要求当前剩余空间能够直接容纳对齐后的载荷。 */
    if (aligned_size > g_heap.free_size) {
        /* 返回空指针表示空间不足。 */
        return 0;
    }

    /* 线性堆的用户块从当前偏移处开始。 */
    uint8_t *block = g_heap.start + g_heap.allocation_offset;

    /* 推进下一次分配偏移。 */
    g_heap.allocation_offset += aligned_size;

    /* 扣减当前空闲空间。 */
    g_heap.free_size -= aligned_size;

    /* 更新历史最低剩余空间。 */
    MRT_HeapUpdateMinimumFreeSize();

    /* 返回对齐后的用户块地址。 */
    return block;
}

/**
 * @brief 释放由 MyRTOS 全局堆分配的内存块。
 * @param ptr 待释放指针；NULL 指针被视为无操作成功。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示释放成功；线性堆不支持释放单块，返回 MRT_RESULT_OBJECT_BUSY；
 *         堆尚未初始化返回 MRT_RESULT_NOT_STARTED；非法指针或重复释放返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * MRT_Free(block);
 */
MRT_Result MRT_Free(void *ptr)
{
    /* 空指针释放是无操作，便于调用方在清理路径直接调用。 */
    if (ptr == 0) {
        /* 返回成功。 */
        return MRT_RESULT_OK;
    }

    /* 堆未初始化时不能判断指针归属。 */
    if (!g_heap.initialized) {
        /* 返回未启动状态。 */
        return MRT_RESULT_NOT_STARTED;
    }

    /* 线性堆不支持回收单个块。 */
    if (g_heap.mode == MRT_HEAP_MODE_LINEAR) {
        /* 返回对象忙，表示该堆模式不能立即释放该分配。 */
        return MRT_RESULT_OBJECT_BUSY;
    }

    /* 可释放堆检查指针归属并归还块。 */
    return MRT_HeapFreeToFreeList(ptr);
}

/**
 * @brief 查询当前堆剩余空闲空间字节数。
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

    /* 堆必须先初始化才可以查询。 */
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
 * @brief 查询堆初始化后的历史最低剩余空闲空间字节数。
 * @param out_minimum_free_size 输出历史最低剩余空间字节数，不能为 NULL。
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

    /* 堆必须先初始化才可以查询。 */
    if (!g_heap.initialized) {
        /* 返回未启动状态。 */
        return MRT_RESULT_NOT_STARTED;
    }

    /* 写回历史最低剩余空间。 */
    *out_minimum_free_size = g_heap.minimum_ever_free_size;

    /* 查询成功。 */
    return MRT_RESULT_OK;
}
