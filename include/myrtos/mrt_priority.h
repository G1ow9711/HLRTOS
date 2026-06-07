#ifndef MYRTOS_MRT_PRIORITY_H
#define MYRTOS_MRT_PRIORITY_H

/**
 * @file mrt_priority.h
 * @brief MyRTOS 调度优先级位图接口。
 *
 * 调度器使用位图记录哪些优先级存在就绪任务，从而快速查找最高就绪优先级。
 */

#include "myrtos/mrt_config.h"
#include "myrtos/mrt_types.h"

/**
 * @brief 优先级就绪位图。
 *
 * 每一个 bit 表示一个优先级是否存在至少一个就绪任务。
 * bit 为 1 表示该优先级非空，bit 为 0 表示该优先级当前没有就绪任务。
 */
typedef struct MRT_PriorityBitmap {
    /** @brief 位图存储区，每 32 个优先级占用一个 32 位字。 */
    uint32_t words[(MRT_CFG_MAX_PRIORITIES + 31u) / 32u];
} MRT_PriorityBitmap;

/**
 * @brief 初始化优先级位图为空。
 * @param bitmap 待初始化的位图指针，不能为空。
 * @return void 无返回值。
 * @example
 * MRT_PriorityBitmap bitmap;
 * MRT_PriorityBitmapInitialize(&bitmap);
 */
void MRT_PriorityBitmapInitialize(MRT_PriorityBitmap *bitmap);

/**
 * @brief 将指定优先级标记为存在就绪任务。
 * @param bitmap 待修改的位图指针，不能为空。
 * @param priority 待置位的优先级，必须小于 MRT_CFG_MAX_PRIORITIES；越界值会被忽略。
 * @return void 无返回值。
 * @example
 * MRT_PriorityBitmapSet(&bitmap, task_priority);
 */
void MRT_PriorityBitmapSet(MRT_PriorityBitmap *bitmap, MRT_Priority priority);

/**
 * @brief 将指定优先级标记为没有就绪任务。
 * @param bitmap 待修改的位图指针，不能为空。
 * @param priority 待清除的优先级，必须小于 MRT_CFG_MAX_PRIORITIES；越界值会被忽略。
 * @return void 无返回值。
 * @example
 * MRT_PriorityBitmapClear(&bitmap, task_priority);
 */
void MRT_PriorityBitmapClear(MRT_PriorityBitmap *bitmap, MRT_Priority priority);

/**
 * @brief 查找当前最高的已置位优先级。
 * @param bitmap 待查询的位图指针，不能为空。
 * @param out_priority 输出最高优先级；允许为空，表示只查询是否存在就绪优先级。
 * @return bool 返回 true 表示找到优先级，返回 false 表示位图为空。
 * @example
 * MRT_Priority highest;
 * if (MRT_PriorityBitmapFindHighest(&bitmap, &highest)) { MRT_SchedulePriority(highest); }
 */
bool MRT_PriorityBitmapFindHighest(const MRT_PriorityBitmap *bitmap, MRT_Priority *out_priority);

#endif
