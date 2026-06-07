#ifndef MYRTOS_MRT_EVENT_GROUP_H
#define MYRTOS_MRT_EVENT_GROUP_H

/**
 * @file mrt_event_group.h
 * @brief MyRTOS 事件组公共接口。
 *
 * 事件组用一个位集合表达多个事件条件，适合任务之间或 ISR 与任务之间传递
 * “某些事件已经发生”的状态。本模块采用静态创建优先的嵌入式设计，调用方
 * 提供控制块存储，内核只维护 bit 状态和等待任务链表。
 */

#include "myrtos/mrt_list.h"
#include "myrtos/mrt_types.h"

/**
 * @brief MyRTOS 事件组控制块。
 *
 * 静态创建事件组时，调用方提供该结构体作为控制块存储。结构体公开是为了
 * 支持无动态内存的嵌入式工程；应用代码不应直接修改字段。
 */
typedef struct MRT_EventGroup {
    /** @brief 当前已经置位的事件 bit 集合。 */
    MRT_EventBits bits;
    /** @brief 等待该事件组条件满足的任务链表，后续阻塞 wait 会使用。 */
    MRT_List waiting_tasks;
    /** @brief 是否使用静态存储创建。 */
    bool static_storage;
} MRT_EventGroup;

/**
 * @brief 使用调用方提供的控制块静态创建事件组。
 * @param storage 事件组控制块存储，不能为空。
 * @param out_group 输出事件组句柄，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示创建成功；参数非法时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * static MRT_EventGroup event_cb;
 * MRT_EventGroupHandle events;
 * MRT_EventGroupCreateStatic(&event_cb, &events);
 */
MRT_Result MRT_EventGroupCreateStatic(MRT_EventGroup *storage, MRT_EventGroupHandle *out_group);

/**
 * @brief 从 MyRTOS 全局堆动态创建事件组。
 * @param out_group 输出事件组句柄，不能为空；失败时写入空指针。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示创建成功；参数非法返回 MRT_RESULT_INVALID_ARGUMENT；
 *         动态分配关闭或堆空间不足时返回 MRT_RESULT_NO_MEMORY。
 * @example
 * MRT_EventGroupHandle group;
 * MRT_EventGroupCreate(&group);
 */
MRT_Result MRT_EventGroupCreate(MRT_EventGroupHandle *out_group);

/**
 * @brief 删除动态创建的事件组并归还堆内存。
 * @param group 待删除事件组句柄，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示删除成功；空句柄返回 MRT_RESULT_INVALID_ARGUMENT；
 *         静态事件组或仍有等待任务时返回 MRT_RESULT_OBJECT_BUSY。
 * @example
 * MRT_EventGroupDelete(group);
 */
MRT_Result MRT_EventGroupDelete(MRT_EventGroupHandle group);

/**
 * @brief 设置事件组中的一个或多个 bit。
 * @param group 事件组句柄，不能为空。
 * @param bits_to_set 需要置位的 bit 掩码，不能为 0。
 * @param out_bits 输出设置后的完整 bit 集合，允许为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示设置成功；参数非法时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * MRT_EventBits bits;
 * MRT_EventGroupSetBits(events, 0x01u, &bits);
 */
MRT_Result MRT_EventGroupSetBits(MRT_EventGroupHandle group, MRT_EventBits bits_to_set, MRT_EventBits *out_bits);

/**
 * @brief 在 ISR 上下文设置事件组中的一个或多个 bit。
 * @param group 事件组句柄，不能为空。
 * @param bits_to_set 需要置位的 bit 掩码，不能为 0。
 * @param should_yield 输出是否需要在 ISR 退出前触发调度切换；允许为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示设置成功；参数非法时返回 MRT_RESULT_INVALID_ARGUMENT；
 *         非 ISR 上下文返回 MRT_RESULT_INVALID_CONTEXT。
 * @example
 * bool yield;
 * MRT_EventGroupSetBitsFromISR(events, 0x01u, &yield);
 */
MRT_Result MRT_EventGroupSetBitsFromISR(MRT_EventGroupHandle group, MRT_EventBits bits_to_set, bool *should_yield);

/**
 * @brief 清除事件组中的一个或多个 bit。
 * @param group 事件组句柄，不能为空。
 * @param bits_to_clear 需要清除的 bit 掩码，不能为 0。
 * @param out_bits 输出清除后的完整 bit 集合，允许为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示清除成功；参数非法时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * MRT_EventBits bits;
 * MRT_EventGroupClearBits(events, 0x01u, &bits);
 */
MRT_Result MRT_EventGroupClearBits(MRT_EventGroupHandle group, MRT_EventBits bits_to_clear, MRT_EventBits *out_bits);

/**
 * @brief 等待事件组中的指定 bit 条件。
 * @param group 事件组句柄，不能为空。
 * @param bits_to_wait 需要等待的 bit 掩码，不能为 0。
 * @param wait_all true 表示所有请求 bit 均置位才满足；false 表示任意请求 bit 置位即满足。
 * @param clear_on_exit true 表示成功满足后清除请求范围内已经匹配的 bit。
 * @param timeout 等待条件满足的 tick 数；为 0 时只检查一次并立即返回。
 * @param out_bits 输出等待完成时的事件组 bit 快照，允许为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示条件满足；非阻塞未满足返回 MRT_RESULT_OBJECT_EMPTY；
 *         参数非法返回 MRT_RESULT_INVALID_ARGUMENT；等待未完成返回 MRT_RESULT_TIMEOUT。
 * @example
 * MRT_EventBits bits;
 * MRT_EventGroupWaitBits(events, 0x03u, false, true, 10u, &bits);
 */
MRT_Result MRT_EventGroupWaitBits(MRT_EventGroupHandle group,
                                  MRT_EventBits bits_to_wait,
                                  bool wait_all,
                                  bool clear_on_exit,
                                  MRT_Timeout timeout,
                                  MRT_EventBits *out_bits);

/**
 * @brief 查询事件组当前 bit 集合。
 * @param group 事件组句柄，不能为空。
 * @return MRT_EventBits 返回当前 bit 集合；事件组句柄为空时返回 0。
 * @example
 * MRT_EventBits bits = MRT_EventGroupGetBits(events);
 */
MRT_EventBits MRT_EventGroupGetBits(MRT_EventGroupHandle group);

#endif
