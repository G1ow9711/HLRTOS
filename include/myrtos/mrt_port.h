#ifndef MYRTOS_MRT_PORT_H
#define MYRTOS_MRT_PORT_H

/**
 * @file mrt_port.h
 * @brief MyRTOS CPU/编译器移植层公共接口。
 *
 * 内核通过本接口访问上下文切换、临界区、ISR 状态和 tick 配置。
 * STM32、DSP 和 host mock 端口都必须实现同一组接口。
 */

#include "myrtos/mrt_types.h"

/**
 * @brief 初始化端口层状态。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * MRT_PortInitialize();
 */
void MRT_PortInitialize(void);

/**
 * @brief 启动第一个任务。
 * @param void 无输入参数。
 * @return void 无返回值；真实端口通常不会返回。
 * @example
 * MRT_PortStartFirstTask();
 */
void MRT_PortStartFirstTask(void);

/**
 * @brief 在任务上下文请求一次调度切换。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * MRT_PortYield();
 */
void MRT_PortYield(void);

/**
 * @brief 在 ISR 退出前按需请求调度切换。
 * @param should_yield true 表示需要切换，false 表示不需要切换。
 * @return void 无返回值。
 * @example
 * MRT_PortYieldFromISR(should_yield);
 */
void MRT_PortYieldFromISR(bool should_yield);

/**
 * @brief 抑制周期 tick 并进入低功耗睡眠。
 * @param expected_idle_ticks 内核允许端口层连续睡眠的最大 tick 数。
 * @param out_slept_ticks 输出端口层实际睡眠的 tick 数，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示端口睡眠过程完成；参数为空时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * MRT_Tick slept;
 * MRT_PortSuppressTicksAndSleep(10u, &slept);
 */
MRT_Result MRT_PortSuppressTicksAndSleep(MRT_Tick expected_idle_ticks, MRT_Tick *out_slept_ticks);

/**
 * @brief 进入临界区并保存旧中断状态。
 * @param void 无输入参数。
 * @return MRT_IntState 返回进入临界区前的端口状态，用于退出时恢复。
 * @example
 * MRT_IntState state = MRT_PortEnterCritical();
 */
MRT_IntState MRT_PortEnterCritical(void);

/**
 * @brief 退出临界区并恢复旧中断状态。
 * @param state MRT_PortEnterCritical 返回的旧状态。
 * @return void 无返回值。
 * @example
 * MRT_PortExitCritical(state);
 */
void MRT_PortExitCritical(MRT_IntState state);

/**
 * @brief 判断当前是否处于 ISR 上下文。
 * @param void 无输入参数。
 * @return bool 返回 true 表示当前在 ISR 中，返回 false 表示当前在任务上下文。
 * @example
 * if (MRT_PortIsInsideISR()) { MRT_QueueSendFromISR(queue, item, &yield); }
 */
bool MRT_PortIsInsideISR(void);

/**
 * @brief 设置 host mock 的 ISR 状态。
 * @param inside_isr true 表示模拟 ISR 上下文，false 表示模拟任务上下文。
 * @return void 无返回值。
 * @example
 * MRT_PortMockSetInsideISR(true);
 */
void MRT_PortMockSetInsideISR(bool inside_isr);

/**
 * @brief 查询 host mock 是否收到上下文切换请求。
 * @param void 无输入参数。
 * @return bool 返回 true 表示已经请求切换，返回 false 表示尚未请求切换。
 * @example
 * bool requested = MRT_PortMockWasYieldRequested();
 */
bool MRT_PortMockWasYieldRequested(void);

/**
 * @brief 查询 host mock 当前临界区嵌套深度。
 * @param void 无输入参数。
 * @return uint32_t 返回当前临界区嵌套深度。
 * @example
 * uint32_t depth = MRT_PortMockGetCriticalDepth();
 */
uint32_t MRT_PortMockGetCriticalDepth(void);

/**
 * @brief 设置 host mock 下一次 tickless 睡眠实际经过的 tick 数。
 * @param slept_ticks 模拟端口层真实睡眠 tick 数。
 * @return void 无返回值。
 * @example
 * MRT_PortMockSetSuppressedSleepTicks(4u);
 */
void MRT_PortMockSetSuppressedSleepTicks(MRT_Tick slept_ticks);

/**
 * @brief 查询 host mock 收到 tickless 睡眠请求的次数。
 * @param void 无输入参数。
 * @return uint32_t 返回 tickless 睡眠请求次数。
 * @example
 * uint32_t calls = MRT_PortMockGetSuppressSleepCallCount();
 */
uint32_t MRT_PortMockGetSuppressSleepCallCount(void);

/**
 * @brief 查询 host mock 最近一次收到的预计可睡眠 tick 数。
 * @param void 无输入参数。
 * @return MRT_Tick 返回最近一次 MRT_PortSuppressTicksAndSleep 的 expected_idle_ticks 参数。
 * @example
 * MRT_Tick requested = MRT_PortMockGetLastExpectedIdleTicks();
 */
MRT_Tick MRT_PortMockGetLastExpectedIdleTicks(void);

#endif
