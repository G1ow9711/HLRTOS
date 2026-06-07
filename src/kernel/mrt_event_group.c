#include "myrtos/mrt_event_group.h"
#include "myrtos/mrt_config.h"
#include "myrtos/mrt_heap.h"
#include "myrtos/mrt_port.h"
#include "myrtos/mrt_task.h"
#include "mrt_task_internal.h"

/**
 * @brief 判断当前事件 bit 是否满足等待条件。
 * @param current_bits 当前事件组 bit 快照。
 * @param bits_to_wait 调用方请求等待的 bit 掩码，不能为 0。
 * @param wait_all true 表示等待全部请求 bit；false 表示等待任意请求 bit。
 * @return bool 返回 true 表示条件满足，返回 false 表示条件未满足。
 * @example
 * if (MRT_EventGroupBitsMatch(group->bits, 0x03u, false)) { ... }
 */
static bool MRT_EventGroupBitsMatch(MRT_EventBits current_bits, MRT_EventBits bits_to_wait, bool wait_all)
{
    /* 计算当前已经命中的请求 bit。 */
    MRT_EventBits matched_bits = current_bits & bits_to_wait;

    /* wait-all 要求命中的 bit 与请求 bit 完全一致。 */
    if (wait_all) {
        /* 所有请求 bit 都已置位时条件满足。 */
        return matched_bits == bits_to_wait;
    }

    /* wait-any 只要求至少一个请求 bit 已置位。 */
    return matched_bits != 0u;
}

/**
 * @brief 根据事件 bit 快照唤醒所有匹配等待者。
 * @param group 事件组句柄，不能为空。
 * @param snapshot 设置 bit 后、清位前的事件组 bit 快照。
 * @param switch_now true 表示任务上下文需要立即重选当前任务，false 表示 ISR 场景只标记 ready。
 * @return bool 返回 true 表示至少唤醒一个任务，返回 false 表示没有等待者被唤醒。
 * @example
 * bool woke = MRT_EventGroupWakeMatchingTasks(group, group->bits, true);
 */
static bool MRT_EventGroupWakeMatchingTasks(MRT_EventGroupHandle group, MRT_EventBits snapshot, bool switch_now)
{
    /* 记录是否至少唤醒过一个等待任务。 */
    bool woke_task = false;

    /* 记录所有 clear-on-exit 等待者匹配到的 bit 并集。 */
    MRT_EventBits clear_mask = 0u;

    /* 从等待链表头部开始遍历；链表按任务优先级排序。 */
    MRT_ListNode *node = MRT_ListGetHead(&group->waiting_tasks);

    /* 遍历直到链表为空或走到哨兵节点。 */
    while ((node != 0) && (node != &group->waiting_tasks.sentinel)) {
        /* 先保存后继节点，因为当前节点可能在唤醒时被移除。 */
        MRT_ListNode *next = MRT_ListGetNext(node);

        /* 从等待节点恢复任务句柄。 */
        MRT_TaskHandle task = (MRT_TaskHandle)node->item;

        /* 计算该任务在快照中命中的请求 bit。 */
        MRT_EventBits matched_bits = snapshot & task->event_wait_bits;

        /* 判断该任务的 wait-all 或 wait-any 条件是否满足。 */
        if (MRT_EventGroupBitsMatch(snapshot, task->event_wait_bits, task->event_wait_all)) {
            /* 保存匹配结果，便于后续调试或扩展恢复路径。 */
            task->event_matched_bits = matched_bits;

            /* 如果任务请求成功退出后清位，则累积其匹配 bit。 */
            if (task->event_clear_on_exit) {
                /* 只清除该任务实际匹配到的请求 bit。 */
                clear_mask |= matched_bits;
            }

            /* 任务被唤醒后不再持有事件等待请求。 */
            task->event_wait_bits = 0u;

            /* 清除 wait-all 策略标记。 */
            task->event_wait_all = false;

            /* 清除退出清位策略标记。 */
            task->event_clear_on_exit = false;

            /* 唤醒该任务，但先不切换，保证本轮能检查所有等待者。 */
            (void)MRT_TaskKernelWakeTask(task, MRT_RESULT_OK, false);

            /* 标记本次 set bits 至少唤醒了一个任务。 */
            woke_task = true;
        }

        /* 移动到原链表中的下一个节点；若是哨兵，循环会结束。 */
        node = next;
    }

    /* 所有等待者判定完成后，再统一执行 clear-on-exit 清位。 */
    if (clear_mask != 0u) {
        /* 清除所有匹配等待者请求清除的 bit。 */
        group->bits &= ~clear_mask;
    }

    /* 任务上下文唤醒后，需要让高优先级被唤醒任务有机会抢占。 */
    if (woke_task && switch_now) {
        /* 复用调度器 yield 路径重选最高优先级 ready 任务。 */
        MRT_TaskKernelYield();
    }

    /* 返回是否唤醒过任务。 */
    return woke_task;
}

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
MRT_Result MRT_EventGroupCreateStatic(MRT_EventGroup *storage, MRT_EventGroupHandle *out_group)
{
    /* 事件组控制块不能为空，否则无法保存 bit 状态和等待链表。 */
    if (storage == 0) {
        /* 返回参数错误，提示调用方提供静态控制块。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 输出句柄不能为空，否则创建成功后调用方无法使用对象。 */
    if (out_group == 0) {
        /* 返回参数错误，提示调用方提供输出句柄地址。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 新事件组初始不包含任何已发生事件。 */
    storage->bits = 0u;

    /* 初始化等待任务链表，后续 wait bits 阻塞路径会使用。 */
    MRT_ListInitialize(&storage->waiting_tasks);

    /* 标记对象使用静态存储创建。 */
    storage->static_storage = true;

    /* 输出事件组句柄给调用方。 */
    *out_group = storage;

    /* 静态事件组创建成功。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 从 MyRTOS 全局堆动态创建事件组。
 * @param out_group 输出事件组句柄，不能为空；失败时写入空指针。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示创建成功；参数非法返回 MRT_RESULT_INVALID_ARGUMENT；
 *         堆不可用或空间不足时返回 MRT_RESULT_NO_MEMORY。
 * @example
 * MRT_EventGroupHandle group;
 * MRT_EventGroupCreate(&group);
 */
MRT_Result MRT_EventGroupCreate(MRT_EventGroupHandle *out_group)
{
    /* 输出句柄不能为空。 */
    if (out_group == 0) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 失败路径默认清空输出句柄。 */
    *out_group = 0;

    /* 动态分配关闭时不能创建堆对象。 */
    if (MRT_CFG_SUPPORT_DYNAMIC_ALLOCATION == 0u) {
        /* 返回内存不足。 */
        return MRT_RESULT_NO_MEMORY;
    }

    /* 申请一个事件组控制块。 */
    MRT_EventGroup *group = (MRT_EventGroup *)MRT_Malloc(sizeof(MRT_EventGroup));

    /* 堆空间不足时创建失败。 */
    if (group == 0) {
        /* 返回内存不足。 */
        return MRT_RESULT_NO_MEMORY;
    }

    /* 复用静态创建逻辑初始化控制块。 */
    MRT_Result result = MRT_EventGroupCreateStatic(group, out_group);

    /* 初始化失败时释放堆块。 */
    if (result != MRT_RESULT_OK) {
        /* 释放控制块。 */
        (void)MRT_Free(group);

        /* 清空输出句柄。 */
        *out_group = 0;

        /* 返回实际错误。 */
        return result;
    }

    /* 标记事件组归动态堆所有。 */
    group->static_storage = false;

    /* 动态事件组创建成功。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 删除动态创建的事件组并归还堆内存。
 * @param group 待删除事件组句柄，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示删除成功；空句柄返回 MRT_RESULT_INVALID_ARGUMENT；
 *         静态事件组或仍有等待任务时返回 MRT_RESULT_OBJECT_BUSY。
 * @example
 * MRT_EventGroupDelete(group);
 */
MRT_Result MRT_EventGroupDelete(MRT_EventGroupHandle group)
{
    /* 事件组句柄不能为空。 */
    if (group == 0) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 静态事件组内存不归堆释放路径所有。 */
    if (group->static_storage) {
        /* 返回对象忙。 */
        return MRT_RESULT_OBJECT_BUSY;
    }

    /* 仍有任务等待事件组时不能删除。 */
    if (!MRT_ListIsEmpty(&group->waiting_tasks)) {
        /* 返回对象忙。 */
        return MRT_RESULT_OBJECT_BUSY;
    }

    /* 动态事件组控制块就是堆块起始地址。 */
    return MRT_Free(group);
}

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
MRT_Result MRT_EventGroupSetBits(MRT_EventGroupHandle group, MRT_EventBits bits_to_set, MRT_EventBits *out_bits)
{
    /* 事件组句柄不能为空，否则无法修改 bit 集合。 */
    if (group == 0) {
        /* 返回参数错误，提示调用方传入有效事件组。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 设置 0 bit 不会产生任何事件语义，因此直接拒绝。 */
    if (bits_to_set == 0u) {
        /* 返回参数错误，提示调用方提供非零 bit 掩码。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 将请求 bit 按位或到当前事件集合。 */
    group->bits |= bits_to_set;

    /* 保存清位前快照，输出和等待者判定都基于同一个事件状态。 */
    MRT_EventBits snapshot = group->bits;

    /* 如果调用方需要观察结果，则写回完整 bit 集合。 */
    if (out_bits != 0) {
        /* 写回设置后的事件集合快照，可能早于 clear-on-exit 清位。 */
        *out_bits = snapshot;
    }

    /* 根据设置后的快照唤醒所有匹配等待者，并允许任务上下文立即调度。 */
    (void)MRT_EventGroupWakeMatchingTasks(group, snapshot, true);

    /* bit 设置成功。 */
    return MRT_RESULT_OK;
}

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
MRT_Result MRT_EventGroupSetBitsFromISR(MRT_EventGroupHandle group, MRT_EventBits bits_to_set, bool *should_yield)
{
    /* 默认不请求 ISR 退出后切换，所有失败路径保持该状态。 */
    if (should_yield != 0) {
        /* 写回 false，避免调用方沿用旧值。 */
        *should_yield = false;
    }

    /* 事件组句柄不能为空，否则无法修改 bit 集合。 */
    if (group == 0) {
        /* 返回参数错误，提示调用方传入有效事件组。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 设置 0 bit 不会产生任何事件语义，因此直接拒绝。 */
    if (bits_to_set == 0u) {
        /* 返回参数错误，提示调用方提供非零 bit 掩码。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* FromISR API 只能在 ISR 上下文调用。 */
    if (!MRT_PortIsInsideISR()) {
        /* 返回非法上下文，提示调用方使用任务上下文 API。 */
        return MRT_RESULT_INVALID_CONTEXT;
    }

    /* 将请求 bit 按位或到当前事件集合。 */
    group->bits |= bits_to_set;

    /* 保存清位前快照，等待者判定基于同一个事件状态。 */
    MRT_EventBits snapshot = group->bits;

    /* ISR 中只把匹配等待者置为 ready，不立即修改当前任务。 */
    bool woke_task = MRT_EventGroupWakeMatchingTasks(group, snapshot, false);

    /* 如果唤醒了任务，则请求端口层在 ISR 退出前执行延迟切换。 */
    if ((should_yield != 0) && woke_task) {
        /* 写回需要 yield。 */
        *should_yield = true;
    }

    /* ISR set bits 成功。 */
    return MRT_RESULT_OK;
}

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
MRT_Result MRT_EventGroupClearBits(MRT_EventGroupHandle group, MRT_EventBits bits_to_clear, MRT_EventBits *out_bits)
{
    /* 事件组句柄不能为空，否则无法修改 bit 集合。 */
    if (group == 0) {
        /* 返回参数错误，提示调用方传入有效事件组。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 清除 0 bit 不会产生任何事件语义，因此直接拒绝。 */
    if (bits_to_clear == 0u) {
        /* 返回参数错误，提示调用方提供非零 bit 掩码。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 使用按位与非清除请求 bit。 */
    group->bits &= ~bits_to_clear;

    /* 如果调用方需要观察结果，则写回完整 bit 集合。 */
    if (out_bits != 0) {
        /* 写回清除后的事件集合。 */
        *out_bits = group->bits;
    }

    /* bit 清除成功。 */
    return MRT_RESULT_OK;
}

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
                                  MRT_EventBits *out_bits)
{
    /* 事件组句柄不能为空，否则无法读取 bit 状态或挂起任务。 */
    if (group == 0) {
        /* 返回参数错误，提示调用方传入有效事件组。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 等待 0 bit 永远无法表达有效条件，因此直接拒绝。 */
    if (bits_to_wait == 0u) {
        /* 返回参数错误，提示调用方提供非零等待掩码。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 读取当前事件组快照，保证输出值与清位前状态一致。 */
    MRT_EventBits snapshot = group->bits;

    /* 如果调用方需要观察状态，先写回当前快照。 */
    if (out_bits != 0) {
        /* 写回等待完成或失败时看到的 bit 集合。 */
        *out_bits = snapshot;
    }

    /* 当前 bit 已经满足等待条件时走即时成功路径。 */
    if (MRT_EventGroupBitsMatch(snapshot, bits_to_wait, wait_all)) {
        /* 如果调用方请求退出时清位，则只清除请求范围内已经置位的 bit。 */
        if (clear_on_exit) {
            /* 清除匹配 bit，未请求的 bit 保持不变。 */
            group->bits &= ~(snapshot & bits_to_wait);
        }

        /* 条件已经满足，返回成功。 */
        return MRT_RESULT_OK;
    }

    /* 非阻塞等待不满足时立即返回对象为空。 */
    if (timeout == 0u) {
        /* 告诉调用方当前没有可消费的事件条件。 */
        return MRT_RESULT_OBJECT_EMPTY;
    }

    /* 读取当前运行任务，只有任务上下文才能进入阻塞等待。 */
    MRT_TaskHandle current = MRT_TaskGetCurrent();

    /* 没有当前任务时，host 仿真无法挂起调用方，按等待超时返回。 */
    if (current == 0) {
        /* 保持非调度上下文的保守行为。 */
        return MRT_RESULT_TIMEOUT;
    }

    /* 保存事件组等待掩码，后续 set bits 会用它判断是否唤醒该任务。 */
    current->event_wait_bits = bits_to_wait;

    /* 当前还没有匹配结果。 */
    current->event_matched_bits = 0u;

    /* 保存等待策略，供事件组置位时判定 wait-all 或 wait-any。 */
    current->event_wait_all = wait_all;

    /* 保存退出清位策略，供事件组置位时统一处理清位。 */
    current->event_clear_on_exit = clear_on_exit;

    /* 将当前任务挂入事件组等待链表，并设置 tick 超时。 */
    return MRT_TaskKernelBlockCurrentOnObject(&group->waiting_tasks,
                                              timeout,
                                              MRT_TASK_WAIT_REASON_EVENT_BITS,
                                              MRT_RESULT_TIMEOUT);
}

/**
 * @brief 查询事件组当前 bit 集合。
 * @param group 事件组句柄，不能为空。
 * @return MRT_EventBits 返回当前 bit 集合；事件组句柄为空时返回 0。
 * @example
 * MRT_EventBits bits = MRT_EventGroupGetBits(events);
 */
MRT_EventBits MRT_EventGroupGetBits(MRT_EventGroupHandle group)
{
    /* 空句柄没有可读取状态，返回 0 保持查询函数无副作用。 */
    if (group == 0) {
        /* 返回空 bit 集。 */
        return 0u;
    }

    /* 返回当前事件 bit 集合。 */
    return group->bits;
}
