#ifndef MYRTOS_MRT_TIMER_INTERNAL_H
#define MYRTOS_MRT_TIMER_INTERNAL_H

/**
 * @file mrt_timer_internal.h
 * @brief MyRTOS 软件定时器内核内部接口。
 *
 * 本文件只供内核核心调用，不属于用户公共 API。公共应用代码应使用
 * include/myrtos/mrt_timer.h 中声明的软件定时器接口。
 */

#include "myrtos/mrt_types.h"

/**
 * @brief 初始化软件定时器内核内部状态。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * MRT_TimerKernelInitialize();
 */
void MRT_TimerKernelInitialize(void);

/**
 * @brief 处理当前 tick 上已经到期的软件定时器。
 * @param now 当前系统 tick。
 * @return void 无返回值。
 * @example
 * MRT_TimerKernelTick(MRT_KernelGetTick());
 */
void MRT_TimerKernelTick(MRT_Tick now);

/**
 * @brief 查询活动软件定时器链表中最近的到期 tick。
 * @param out_tick 输出最近定时器到期 tick，不能为空。
 * @return bool 返回 true 表示存在活动定时器；返回 false 表示没有活动定时器或参数为空。
 * @example
 * MRT_Tick expiry_tick;
 * bool exists = MRT_TimerKernelGetNextExpiryTick(&expiry_tick);
 * (void)exists;
 */
bool MRT_TimerKernelGetNextExpiryTick(MRT_Tick *out_tick);

#endif
