#include "myrtos/mrt_priority.h"

/** @brief 位图数组中的 32 位字数量。 */
#define MRT_PRIORITY_BITMAP_WORD_COUNT ((MRT_CFG_MAX_PRIORITIES + 31u) / 32u)

/**
 * @brief 初始化优先级位图为空。
 * @param bitmap 待初始化的位图指针，不能为空。
 * @return void 无返回值。
 * @example
 * MRT_PriorityBitmap bitmap;
 * MRT_PriorityBitmapInitialize(&bitmap);
 */
void MRT_PriorityBitmapInitialize(MRT_PriorityBitmap *bitmap)
{
    /* 从第一个位图字开始清零。 */
    for (size_t index = 0u; index < MRT_PRIORITY_BITMAP_WORD_COUNT; index++) {
        /* 清除当前 32 位字中的所有优先级标记。 */
        bitmap->words[index] = 0u;
    }
}

/**
 * @brief 将指定优先级标记为存在就绪任务。
 * @param bitmap 待修改的位图指针，不能为空。
 * @param priority 待置位的优先级，必须小于 MRT_CFG_MAX_PRIORITIES；越界值会被忽略。
 * @return void 无返回值。
 * @example
 * MRT_PriorityBitmapSet(&bitmap, task_priority);
 */
void MRT_PriorityBitmapSet(MRT_PriorityBitmap *bitmap, MRT_Priority priority)
{
    /* 越界优先级不是有效调度优先级，直接忽略以保护位图内存。 */
    if (priority >= MRT_CFG_MAX_PRIORITIES) {
        /* 返回调用方；有效位图内容保持不变。 */
        return;
    }

    /* 计算优先级所在的 32 位字下标。 */
    size_t word_index = (size_t)(priority / 32u);

    /* 计算优先级在该 32 位字内的 bit 掩码。 */
    uint32_t bit_mask = (uint32_t)(1u << (priority % 32u));

    /* 将对应 bit 置 1，表示该优先级存在就绪任务。 */
    bitmap->words[word_index] |= bit_mask;
}

/**
 * @brief 将指定优先级标记为没有就绪任务。
 * @param bitmap 待修改的位图指针，不能为空。
 * @param priority 待清除的优先级，必须小于 MRT_CFG_MAX_PRIORITIES；越界值会被忽略。
 * @return void 无返回值。
 * @example
 * MRT_PriorityBitmapClear(&bitmap, task_priority);
 */
void MRT_PriorityBitmapClear(MRT_PriorityBitmap *bitmap, MRT_Priority priority)
{
    /* 越界优先级不是有效调度优先级，直接忽略以保护位图内存。 */
    if (priority >= MRT_CFG_MAX_PRIORITIES) {
        /* 返回调用方；有效位图内容保持不变。 */
        return;
    }

    /* 计算优先级所在的 32 位字下标。 */
    size_t word_index = (size_t)(priority / 32u);

    /* 计算优先级在该 32 位字内的 bit 掩码。 */
    uint32_t bit_mask = (uint32_t)(1u << (priority % 32u));

    /* 将对应 bit 清 0，表示该优先级当前没有就绪任务。 */
    bitmap->words[word_index] &= (uint32_t)~bit_mask;
}

/**
 * @brief 查找当前最高的已置位优先级。
 * @param bitmap 待查询的位图指针，不能为空。
 * @param out_priority 输出最高优先级；允许为空，表示只查询是否存在就绪优先级。
 * @return bool 返回 true 表示找到优先级，返回 false 表示位图为空。
 * @example
 * MRT_Priority highest;
 * if (MRT_PriorityBitmapFindHighest(&bitmap, &highest)) { MRT_SchedulePriority(highest); }
 */
bool MRT_PriorityBitmapFindHighest(const MRT_PriorityBitmap *bitmap, MRT_Priority *out_priority)
{
    /* 从最大配置优先级向下扫描，确保第一个命中的就是最高优先级。 */
    for (MRT_Priority priority = MRT_CFG_MAX_PRIORITIES; priority > 0u; priority--) {
        /* 将循环计数转换为真实候选优先级。 */
        MRT_Priority candidate = priority - 1u;

        /* 计算候选优先级所在的 32 位字下标。 */
        size_t word_index = (size_t)(candidate / 32u);

        /* 计算候选优先级在该 32 位字内的 bit 掩码。 */
        uint32_t bit_mask = (uint32_t)(1u << (candidate % 32u));

        /* 检查候选优先级是否已经置位。 */
        if ((bitmap->words[word_index] & bit_mask) != 0u) {
            /* 如果调用方提供输出指针，则写入最高优先级。 */
            if (out_priority != 0) {
                /* 保存查找到的最高优先级。 */
                *out_priority = candidate;
            }

            /* 已找到最高优先级，返回 true。 */
            return true;
        }
    }

    /* 没有任何 bit 置位，表示当前没有就绪优先级。 */
    return false;
}
