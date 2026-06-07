#ifndef MYRTOS_MRT_KERNEL_H
#define MYRTOS_MRT_KERNEL_H

/**
 * @file mrt_kernel.h
 * @brief MyRTOS 内核基础控制接口。
 *
 * 本文件定义内核初始化、启动、tick 推进、主动让出和调度器挂起恢复接口。
 * 完整任务调度器会在后续计划中扩展这些基础接口。
 */

#include "myrtos/mrt_types.h"

/**
 * @brief 初始化 MyRTOS 内核基础状态。
 * @param void 无输入参数。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示初始化成功。
 * @example
 * MRT_Result result = MRT_KernelInitialize();
 */
MRT_Result MRT_KernelInitialize(void);

/**
 * @brief 启动 MyRTOS 调度器。
 * @param void 无输入参数。
 * @return MRT_Result 无可运行任务时返回 MRT_RESULT_NOT_STARTED；后续调度器接入后成功启动返回 MRT_RESULT_OK。
 * @example
 * MRT_Result result = MRT_KernelStart();
 */
MRT_Result MRT_KernelStart(void);

/**
 * @brief 查询调度器是否正在运行。
 * @param void 无输入参数。
 * @return bool 返回 true 表示调度器正在运行，返回 false 表示尚未运行。
 * @example
 * if (MRT_KernelIsRunning()) { MRT_KernelYield(); }
 */
bool MRT_KernelIsRunning(void);

/**
 * @brief 获取当前系统 tick。
 * @param void 无输入参数。
 * @return MRT_Tick 返回当前系统 tick 计数。
 * @example
 * MRT_Tick now = MRT_KernelGetTick();
 */
MRT_Tick MRT_KernelGetTick(void);

/**
 * @brief 推进一个系统 tick。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * void SysTick_Handler(void) { MRT_KernelTick(); }
 */
void MRT_KernelTick(void);

#if MRT_TESTING
/**
 * @brief 测试环境直接设置当前系统 tick。
 * @param tick 要写入的系统 tick 值。
 * @return void 无返回值。
 * @example
 * MRT_KernelTestSetTick(UINT32_MAX - 1u);
 */
void MRT_KernelTestSetTick(MRT_Tick tick);
#endif

/**
 * @brief 当前任务主动让出 CPU。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * MRT_KernelYield();
 */
void MRT_KernelYield(void);

/**
 * @brief 挂起调度器。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * MRT_KernelSuspendAll();
 */
void MRT_KernelSuspendAll(void);

/**
 * @brief 恢复调度器。
 * @param void 无输入参数。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示恢复成功；未挂起时返回 MRT_RESULT_INVALID_CONTEXT。
 * @example
 * MRT_Result result = MRT_KernelResumeAll();
 */
MRT_Result MRT_KernelResumeAll(void);

#endif
