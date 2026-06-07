#ifndef MYRTOS_MRT_TIMER_H
#define MYRTOS_MRT_TIMER_H

/**
 * @file mrt_timer.h
 * @brief MyRTOS 软件定时器公共接口。
 *
 * 软件定时器用于在指定 tick 周期后执行回调。当前阶段优先提供静态创建和
 * host 可验证的定时器控制逻辑，真实端口上的服务任务调度会在后续阶段继续增强。
 */

#include "myrtos/mrt_list.h"
#include "myrtos/mrt_types.h"

#include <stdint.h>

/**
 * @brief 软件定时器到期回调函数类型。
 * @param timer 到期的软件定时器句柄。
 * @param arg 创建定时器时保存的用户参数。
 * @return void 无返回值。
 * @example
 * static void LedTimerCallback(MRT_TimerHandle timer, void *arg) { (void)timer; (void)arg; }
 */
typedef void (*MRT_TimerCallback)(MRT_TimerHandle timer, void *arg);

/**
 * @brief 软件定时器 pending function 回调类型。
 * @param arg 投递 pending function 时传入的用户参数。
 * @param value 投递 pending function 时传入的整型值。
 * @return void 无返回值。
 * @example
 * static void DeferredWork(void *arg, uint32_t value) { (void)arg; (void)value; }
 */
typedef void (*MRT_TimerPendingFunction)(void *arg, uint32_t value);

/**
 * @brief MyRTOS 软件定时器控制块。
 *
 * 静态创建定时器时，调用方提供该结构体作为控制块存储。结构体公开是为了
 * 支持无动态内存的嵌入式工程；应用代码不应直接修改字段。
 */
typedef struct MRT_Timer {
    /** @brief 定时器名称，仅用于调试显示。 */
    const char *name;
    /** @brief 定时器周期，单位为 tick，必须大于 0。 */
    MRT_Tick period_ticks;
    /** @brief true 表示到期后自动按 period_ticks 重新装载。 */
    bool auto_reload;
    /** @brief 用户回调参数。 */
    void *arg;
    /** @brief 定时器到期回调函数。 */
    MRT_TimerCallback callback;
    /** @brief 定时器是否已经启动并处于活动状态。 */
    bool active;
    /** @brief 下一次到期 tick；仅 active 为 true 时有效。 */
    MRT_Tick expiry_tick;
    /** @brief 定时器进入活动定时器链表时使用的节点。 */
    MRT_ListNode node;
    /** @brief 是否使用静态存储创建。 */
    bool static_storage;
} MRT_Timer;

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
                                 MRT_TimerHandle *out_timer);

/**
 * @brief 查询软件定时器是否处于活动状态。
 * @param timer 定时器句柄，不能为空。
 * @param out_active 输出活动状态，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示查询成功；参数非法时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * bool active;
 * MRT_TimerIsActive(timer, &active);
 */
MRT_Result MRT_TimerIsActive(MRT_TimerHandle timer, bool *out_active);

/**
 * @brief 获取软件定时器名称。
 * @param timer 定时器句柄，不能为空。
 * @return const char* 返回定时器名称指针；定时器句柄为空时返回空指针。
 * @example
 * const char *name = MRT_TimerGetName(timer);
 */
const char *MRT_TimerGetName(MRT_TimerHandle timer);

#endif
