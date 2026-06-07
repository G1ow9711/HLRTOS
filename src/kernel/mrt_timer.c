#include "myrtos/mrt_timer.h"

/**
 * @brief 使用调用方提供的控制块静态创建软件定时器。
 * @param name 定时器名称，允许为空，仅用于调试显示。
 * @param period_ticks 定时器周期，单位为 tick，必须大于 0。
 * @param auto_reload true 表示周期定时器，false 表示单次定时器。
 * @param arg 用户回调参数。
 * @param callback 定时器到期回调函数，不能为空。
 * @param storage 定时器控制块存储，不能为空。
 * @param out_timer 输出定时器句柄，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示创建成功；参数非法时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * static MRT_Timer timer_cb;
 * MRT_TimerHandle timer;
 * MRT_TimerCreateStatic("blink", 100, true, NULL, BlinkCallback, &timer_cb, &timer);
 */
MRT_Result MRT_TimerCreateStatic(const char *name,
                                 MRT_Tick period_ticks,
                                 bool auto_reload,
                                 void *arg,
                                 MRT_TimerCallback callback,
                                 MRT_Timer *storage,
                                 MRT_TimerHandle *out_timer)
{
    /* 周期不能为 0，否则定时器会在同一个 tick 中无限到期。 */
    if (period_ticks == 0u) {
        /* 返回参数错误，提示调用方提供非零周期。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 回调函数不能为空，否则定时器到期后无法执行用户逻辑。 */
    if (callback == 0) {
        /* 返回参数错误，提示调用方提供有效回调。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 定时器控制块不能为空，否则无法保存定时器状态。 */
    if (storage == 0) {
        /* 返回参数错误，提示调用方提供静态控制块。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 输出句柄不能为空，否则创建成功后调用方无法使用对象。 */
    if (out_timer == 0) {
        /* 返回参数错误，提示调用方提供输出句柄地址。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 保存定时器名称指针。 */
    storage->name = name;

    /* 保存定时器周期。 */
    storage->period_ticks = period_ticks;

    /* 保存自动重载策略。 */
    storage->auto_reload = auto_reload;

    /* 保存用户回调参数。 */
    storage->arg = arg;

    /* 保存到期回调函数。 */
    storage->callback = callback;

    /* 新创建定时器默认不活动。 */
    storage->active = false;

    /* 新创建定时器还没有到期 tick。 */
    storage->expiry_tick = 0u;

    /* 初始化定时器链表节点，后续启动时加入活动定时器链表。 */
    MRT_ListNodeInitialize(&storage->node, storage, 0u);

    /* 标记对象使用静态存储创建。 */
    storage->static_storage = true;

    /* 输出定时器句柄给调用方。 */
    *out_timer = storage;

    /* 静态定时器创建成功。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 查询软件定时器是否处于活动状态。
 * @param timer 定时器句柄，不能为空。
 * @param out_active 输出活动状态，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示查询成功；参数非法时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * bool active;
 * MRT_TimerIsActive(timer, &active);
 */
MRT_Result MRT_TimerIsActive(MRT_TimerHandle timer, bool *out_active)
{
    /* 定时器句柄不能为空，否则无法读取状态。 */
    if (timer == 0) {
        /* 返回参数错误，提示调用方传入有效定时器。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 输出指针不能为空，否则无法写回状态。 */
    if (out_active == 0) {
        /* 返回参数错误，提示调用方提供输出存储。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 写回当前活动状态。 */
    *out_active = timer->active;

    /* 查询成功。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 获取软件定时器名称。
 * @param timer 定时器句柄，不能为空。
 * @return const char* 返回定时器名称指针；定时器句柄为空时返回空指针。
 * @example
 * const char *name = MRT_TimerGetName(timer);
 */
const char *MRT_TimerGetName(MRT_TimerHandle timer)
{
    /* 空句柄没有名称，返回空指针。 */
    if (timer == 0) {
        /* 返回空指针给调用方。 */
        return 0;
    }

    /* 返回控制块中保存的名称指针。 */
    return timer->name;
}
