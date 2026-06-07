#ifndef MYRTOS_PORTABLE_MRT_PORT_STM32_CM_H
#define MYRTOS_PORTABLE_MRT_PORT_STM32_CM_H

/**
 * @file mrt_port_stm32_cm.h
 * @brief STM32 Cortex-M 移植层契约辅助接口。
 *
 * 本文件只暴露可在 host 上测试的 Cortex-M 移植契约：任务初始栈帧、
 * tick 参数计算和中断优先级编码。真实 SysTick、PendSV、SVC 和
 * BASEPRI/PRIMASK 寄存器访问由后续板级端口文件按本契约接入。
 */

#include "myrtos/mrt_types.h"

#include <stddef.h>
#include <stdint.h>

/** @brief Cortex-M 初始任务栈帧需要写入的 32 位字数量。 */
#define MRT_PORT_STM32_CM_INITIAL_FRAME_WORDS 16u

/** @brief xPSR 中表示 Thumb 状态的 T bit，Cortex-M 任务入口必须置位。 */
#define MRT_PORT_STM32_CM_XPSR_THUMB_BIT 0x01000000u

/** @brief 软件保存寄存器占位值的基准，便于测试和调试辨认初始帧。 */
#define MRT_PORT_STM32_CM_CALLEE_PATTERN_BASE 0xC0DEF000u

/** @brief Cortex-M SysTick reload 寄存器的 24 位最大装载值。 */
#define MRT_PORT_STM32_CM_SYSTICK_RELOAD_MAX 0x00FFFFFFu

/**
 * @brief Cortex-M 初始栈帧内各寄存器槽位。
 *
 * 栈顶从 `MRT_PORT_STM32_CM_FRAME_R4` 开始，先放软件恢复的 R4-R11，
 * 再放硬件异常返回会弹出的 R0-R3、R12、LR、PC、xPSR。
 */
typedef enum MRT_PortStm32CmFrameSlot {
    /** @brief 软件保存寄存器 R4 槽位。 */
    MRT_PORT_STM32_CM_FRAME_R4 = 0u,
    /** @brief 软件保存寄存器 R5 槽位。 */
    MRT_PORT_STM32_CM_FRAME_R5,
    /** @brief 软件保存寄存器 R6 槽位。 */
    MRT_PORT_STM32_CM_FRAME_R6,
    /** @brief 软件保存寄存器 R7 槽位。 */
    MRT_PORT_STM32_CM_FRAME_R7,
    /** @brief 软件保存寄存器 R8 槽位。 */
    MRT_PORT_STM32_CM_FRAME_R8,
    /** @brief 软件保存寄存器 R9 槽位。 */
    MRT_PORT_STM32_CM_FRAME_R9,
    /** @brief 软件保存寄存器 R10 槽位。 */
    MRT_PORT_STM32_CM_FRAME_R10,
    /** @brief 软件保存寄存器 R11 槽位。 */
    MRT_PORT_STM32_CM_FRAME_R11,
    /** @brief 任务入口参数 R0 槽位。 */
    MRT_PORT_STM32_CM_FRAME_R0,
    /** @brief 自动异常帧 R1 槽位。 */
    MRT_PORT_STM32_CM_FRAME_R1,
    /** @brief 自动异常帧 R2 槽位。 */
    MRT_PORT_STM32_CM_FRAME_R2,
    /** @brief 自动异常帧 R3 槽位。 */
    MRT_PORT_STM32_CM_FRAME_R3,
    /** @brief 自动异常帧 R12 槽位。 */
    MRT_PORT_STM32_CM_FRAME_R12,
    /** @brief 任务函数返回时进入的 LR 槽位。 */
    MRT_PORT_STM32_CM_FRAME_LR,
    /** @brief 任务入口 PC 槽位。 */
    MRT_PORT_STM32_CM_FRAME_PC,
    /** @brief 初始 xPSR 槽位。 */
    MRT_PORT_STM32_CM_FRAME_XPSR
} MRT_PortStm32CmFrameSlot;

/**
 * @brief 初始化 STM32 Cortex-M 任务初始栈帧。
 * @param stack_memory 调用者提供的任务栈首地址，元素类型必须为 MRT_StackType。
 * @param stack_words 任务栈元素数量，必须至少容纳 MRT_PORT_STM32_CM_INITIAL_FRAME_WORDS。
 * @param entry 任务入口函数，参数通过 R0 传入，不能为空。
 * @param argument 传给任务入口函数的用户参数，可为空。
 * @param task_exit 任务函数意外返回时跳转的处理函数，不能为空。
 * @param out_stack_top 输出初始化后的栈顶指针，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示栈帧已写入；参数非法或空间不足时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * MRT_StackType stack[128];
 * MRT_StackType *top;
 * MRT_PortStm32CmInitializeStack(stack, 128, app_task, arg, task_exit, &top);
 */
MRT_Result MRT_PortStm32CmInitializeStack(MRT_StackType *stack_memory,
                                          size_t stack_words,
                                          void (*entry)(void *argument),
                                          void *argument,
                                          void (*task_exit)(void),
                                          MRT_StackType **out_stack_top);

/**
 * @brief 计算 STM32 Cortex-M SysTick reload 装载值。
 * @param cpu_clock_hz SysTick 输入时钟频率，单位 Hz，通常为内核时钟或其分频时钟。
 * @param tick_rate_hz 目标 RTOS tick 频率，单位 Hz，必须大于 0。
 * @param out_reload 输出 reload 值，即写入 SysTick LOAD 的数值，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示计算成功；参数非法、每 tick 时钟数为 0 或超出 24 位范围时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * uint32_t reload;
 * MRT_PortStm32CmCalculateSysTickReload(168000000u, 1000u, &reload);
 */
MRT_Result MRT_PortStm32CmCalculateSysTickReload(uint32_t cpu_clock_hz,
                                                 uint32_t tick_rate_hz,
                                                 uint32_t *out_reload);

/**
 * @brief 按 NVIC 实现优先级位数计算 BASEPRI 编码值。
 * @param nvic_priority_bits MCU 实际实现的 NVIC 优先级 bit 数，常见 STM32 为 4。
 * @param logical_priority 未左移的逻辑优先级，0 表示最高优先级且不能用于 BASEPRI 屏蔽。
 * @param out_encoded_priority 输出左对齐到 8 bit 优先级字段后的 BASEPRI 数值，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示编码成功；位宽、优先级或输出指针非法时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * uint32_t basepri;
 * MRT_PortStm32CmEncodeBasepri(4u, 5u, &basepri);
 */
MRT_Result MRT_PortStm32CmEncodeBasepri(uint32_t nvic_priority_bits,
                                        uint32_t logical_priority,
                                        uint32_t *out_encoded_priority);

#endif
