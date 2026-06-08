#ifndef MYRTOS_MRT_TASK_INTERNAL_H
#define MYRTOS_MRT_TASK_INTERNAL_H

/**
 * @file mrt_task_internal.h
 * @brief MyRTOS 任务调度器内部接口。
 *
 * 本文件只供内核内部模块使用，不作为用户 API。
 */

#include "myrtos/mrt_list.h"
#include "myrtos/mrt_task.h"

#include <stdbool.h>

/**
 * @brief 初始化任务调度器内部状态。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * MRT_TaskKernelInitialize();
 */
void MRT_TaskKernelInitialize(void);

/**
 * @brief 启动任务调度器并选择第一个运行任务。
 * @param void 无输入参数。
 * @return bool 返回 true 表示存在可运行任务并已请求端口启动；false 表示没有任务可运行。
 * @example
 * bool started = MRT_TaskKernelStartScheduler();
 */
bool MRT_TaskKernelStartScheduler(void);

/**
 * @brief 在任务上下文请求一次调度器让出。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * MRT_TaskKernelYield();
 */
void MRT_TaskKernelYield(void);

/**
 * @brief 推进任务调度器 tick 相关状态。
 * @param now 当前内核 tick 值。
 * @return void 无返回值。
 * @example
 * MRT_TaskKernelTick(MRT_KernelGetTick());
 */
void MRT_TaskKernelTick(MRT_Tick now);

/**
 * @brief 把经过的运行 tick 累计到当前任务。
 * @param elapsed_ticks 已经过的运行 tick 数；为 0 时不改变统计。
 * @return void 无返回值。
 * @example
 * MRT_TaskKernelAccumulateCurrentRuntime(1u);
 */
void MRT_TaskKernelAccumulateCurrentRuntime(MRT_Tick elapsed_ticks);

/**
 * @brief 查询指定任务保存的运行期栈顶。
 * @param task 待查询任务句柄；为空或已删除时返回空指针。
 * @return MRT_StackType* 返回任务当前栈顶；无有效任务时返回空指针。
 * @example
 * MRT_StackType *top = MRT_TaskKernelGetStackTop(task);
 */
MRT_StackType *MRT_TaskKernelGetStackTop(MRT_TaskHandle task);

/**
 * @brief 写入指定任务的运行期栈顶。
 * @param task 目标任务句柄，不能为空且不能为已删除任务。
 * @param stack_top 端口层保存或初始化后的栈顶指针，不能为空。
 * @return MRT_Result 成功返回 MRT_RESULT_OK；参数非法返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * MRT_TaskKernelSetStackTop(task, saved_psp);
 */
MRT_Result MRT_TaskKernelSetStackTop(MRT_TaskHandle task, MRT_StackType *stack_top);

/**
 * @brief 保存刚换出任务的栈顶并返回当前任务待恢复栈顶。
 * @param current_stack_top PendSV 已保存 R4-R11 后得到的旧任务 PSP；为空时仅查询当前任务栈顶。
 * @return MRT_StackType* 返回当前任务待恢复栈顶；当前无运行任务时返回空指针。
 * @example
 * MRT_StackType *next_psp = MRT_TaskKernelSwitchStackTop(saved_psp);
 */
MRT_StackType *MRT_TaskKernelSwitchStackTop(MRT_StackType *current_stack_top);

/**
 * @brief 查询最近的任务唤醒 tick。
 * @param out_tick 输出最近唤醒 tick，不能为空。
 * @return bool 返回 true 表示存在延时任务 deadline；false 表示没有延时任务。
 * @example
 * MRT_Tick wake;
 * bool has_wake = MRT_TaskKernelGetNextWakeTick(&wake);
 */
bool MRT_TaskKernelGetNextWakeTick(MRT_Tick *out_tick);

/**
 * @brief 将当前任务阻塞到对象等待链表并设置超时。
 * @param wait_list 对象等待链表，不能为空。
 * @param ticks 等待 tick 数，0 表示不阻塞。
 * @param wait_reason 等待原因，用于超时清理和调试。
 * @param wait_result 阻塞 API 当前阶段返回给调用方的结果。
 * @return MRT_Result 返回对象等待 API 的结果，通常为 wait_result 或参数错误。
 * @example
 * MRT_TaskKernelBlockCurrentOnObject(&queue->waiting_receivers, ticks, reason, MRT_RESULT_TIMEOUT);
 */
MRT_Result MRT_TaskKernelBlockCurrentOnObject(MRT_List *wait_list,
                                              MRT_Tick ticks,
                                              MRT_TaskWaitReason wait_reason,
                                              MRT_Result wait_result);

/**
 * @brief 将当前任务阻塞到纯任务等待状态。
 * @param ticks 等待 tick 数，0 表示不阻塞。
 * @param wait_reason 等待原因，用于 tick 超时处理。
 * @param wait_result 阻塞 API 当前阶段返回给调用方的结果。
 * @return MRT_Result 返回等待 API 的结果，通常为 wait_result 或参数错误。
 * @example
 * MRT_TaskKernelBlockCurrent(ticks, MRT_TASK_WAIT_REASON_NOTIFICATION, MRT_RESULT_TIMEOUT);
 */
MRT_Result MRT_TaskKernelBlockCurrent(MRT_Tick ticks, MRT_TaskWaitReason wait_reason, MRT_Result wait_result);

/**
 * @brief 唤醒对象等待链表中的最高优先级任务。
 * @param wait_list 对象等待链表，不能为空。
 * @param wait_result 写入被唤醒任务的等待结果。
 * @param switch_now true 表示任务上下文立即调度；false 表示 ISR 路径只置 ready。
 * @return bool 返回 true 表示确实唤醒了任务，false 表示没有等待者。
 * @example
 * bool woke = MRT_TaskKernelWakeFirstObjectWaiter(&sem->waiting_tasks, MRT_RESULT_OK, true);
 */
bool MRT_TaskKernelWakeFirstObjectWaiter(MRT_List *wait_list, MRT_Result wait_result, bool switch_now);

/**
 * @brief 唤醒指定任务并写入等待结果。
 * @param task 要唤醒的任务句柄，不能为空。
 * @param wait_result 写入任务的等待结果。
 * @param switch_now true 表示任务上下文立即调度；false 表示只进入 ready。
 * @return bool 返回 true 表示任务被唤醒，false 表示任务不在可唤醒等待状态。
 * @example
 * bool woke = MRT_TaskKernelWakeTask(task, MRT_RESULT_OK, false);
 */
bool MRT_TaskKernelWakeTask(MRT_TaskHandle task, MRT_Result wait_result, bool switch_now);

/**
 * @brief 设置任务有效优先级。
 * @param task 目标任务句柄，不能为空。
 * @param priority 新有效优先级。
 * @return void 无返回值。
 * @example
 * MRT_TaskKernelSetEffectivePriority(owner, waiter_priority);
 */
void MRT_TaskKernelSetEffectivePriority(MRT_TaskHandle task, MRT_Priority priority);

/**
 * @brief 将任务有效优先级恢复为基础优先级。
 * @param task 目标任务句柄，不能为空。
 * @return void 无返回值。
 * @example
 * MRT_TaskKernelRestoreBasePriority(owner);
 */
void MRT_TaskKernelRestoreBasePriority(MRT_TaskHandle task);

#endif
