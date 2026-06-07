#include "myrtos/mrt_event_group.h"

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

    /* 如果调用方需要观察结果，则写回完整 bit 集合。 */
    if (out_bits != 0) {
        /* 写回设置后的事件集合。 */
        *out_bits = group->bits;
    }

    /* bit 设置成功。 */
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
