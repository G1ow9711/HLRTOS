#ifndef MYRTOS_PORTABLE_MRT_PORT_DSP_C28X_H
#define MYRTOS_PORTABLE_MRT_PORT_DSP_C28X_H

/**
 * @file mrt_port_dsp_c28x.h
 * @brief DSP C28x 风格移植层契约辅助接口。
 *
 * 本文件定义一个可在 host 上验证的 DSP 端口契约。它借鉴 C28x 常见的
 * 向下增长栈、状态字、扩展地址寄存器和软件中断调度模型，但不绑定某一颗
 * 具体芯片或某一版编译器 ABI。真实 DSP 端口可以按目标芯片手册替换底层汇编，
 * 但应保持这里的输入输出语义和测试覆盖。
 */

#include "myrtos/mrt_types.h"

#include <stddef.h>
#include <stdint.h>

/** @brief DSP 初始任务栈帧需要写入的 MRT_StackType 字数量。 */
#define MRT_PORT_DSP_C28X_INITIAL_FRAME_WORDS 16u

/** @brief DSP 初始状态字，默认不预置嵌套中断或异常标志。 */
#define MRT_PORT_DSP_C28X_INITIAL_STATUS_WORD 0x00000000u

/** @brief DSP 保存寄存器占位值的基准，便于 host 测试和调试识别。 */
#define MRT_PORT_DSP_C28X_REGISTER_PATTERN_BASE 0xD5C28000u

/**
 * @brief DSP C28x 风格初始栈帧槽位。
 *
 * 栈顶从 ST0 槽位开始。真实端口的汇编恢复顺序可按该槽位表读取，也可以
 * 在具体芯片端口中用同等语义重新映射。
 */
typedef enum MRT_PortDspC28xFrameSlot {
    /** @brief 状态寄存器 ST0 槽位。 */
    MRT_PORT_DSP_C28X_FRAME_ST0 = 0u,
    /** @brief 状态寄存器 ST1 槽位。 */
    MRT_PORT_DSP_C28X_FRAME_ST1,
    /** @brief 中断允许寄存器 IER 槽位。 */
    MRT_PORT_DSP_C28X_FRAME_IER,
    /** @brief 调试状态或保留状态槽位。 */
    MRT_PORT_DSP_C28X_FRAME_DEBUG,
    /** @brief 扩展地址寄存器 XAR4 槽位。 */
    MRT_PORT_DSP_C28X_FRAME_XAR4,
    /** @brief 扩展地址寄存器 XAR5 槽位。 */
    MRT_PORT_DSP_C28X_FRAME_XAR5,
    /** @brief 扩展地址寄存器 XAR6 槽位。 */
    MRT_PORT_DSP_C28X_FRAME_XAR6,
    /** @brief 扩展地址寄存器 XAR7 槽位。 */
    MRT_PORT_DSP_C28X_FRAME_XAR7,
    /** @brief 累加器 ACC 低位抽象槽位。 */
    MRT_PORT_DSP_C28X_FRAME_ACC_LOW,
    /** @brief 累加器 ACC 高位抽象槽位。 */
    MRT_PORT_DSP_C28X_FRAME_ACC_HIGH,
    /** @brief 乘法结果寄存器 P 抽象槽位。 */
    MRT_PORT_DSP_C28X_FRAME_P,
    /** @brief 扩展寄存器 XT 抽象槽位。 */
    MRT_PORT_DSP_C28X_FRAME_XT,
    /** @brief 任务入口参数槽位。 */
    MRT_PORT_DSP_C28X_FRAME_ARGUMENT,
    /** @brief 任务意外返回后的退出处理函数槽位。 */
    MRT_PORT_DSP_C28X_FRAME_EXIT,
    /** @brief 任务入口 PC 槽位。 */
    MRT_PORT_DSP_C28X_FRAME_PC,
    /** @brief 对齐和后续 ABI 扩展保留槽位。 */
    MRT_PORT_DSP_C28X_FRAME_RESERVED
} MRT_PortDspC28xFrameSlot;

/**
 * @brief 初始化 DSP C28x 风格任务初始栈帧。
 * @param stack_memory 调用者提供的任务栈首地址，元素类型必须为 MRT_StackType。
 * @param stack_words 任务栈元素数量，必须至少容纳 MRT_PORT_DSP_C28X_INITIAL_FRAME_WORDS。
 * @param entry 任务入口函数，不能为空。
 * @param argument 传给任务入口函数的用户参数，可为空。
 * @param task_exit 任务函数意外返回时调用的处理函数，不能为空。
 * @param out_stack_top 输出初始化后的栈顶指针，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示栈帧已写入；参数非法或空间不足时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * MRT_StackType stack[128];
 * MRT_StackType *top;
 * MRT_PortDspC28xInitializeStack(stack, 128, dsp_task, arg, dsp_task_exit, &top);
 */
MRT_Result MRT_PortDspC28xInitializeStack(MRT_StackType *stack_memory,
                                          size_t stack_words,
                                          void (*entry)(void *argument),
                                          void *argument,
                                          void (*task_exit)(void),
                                          MRT_StackType **out_stack_top);

/**
 * @brief 复位 DSP C28x 风格上下文切换模型状态。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * MRT_PortDspC28xContextModelReset();
 */
void MRT_PortDspC28xContextModelReset(void);

/**
 * @brief 请求一次 DSP 软件中断式上下文切换。
 * @param void 无输入参数。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示请求已记录；请求计数溢出时返回 MRT_RESULT_INTERNAL_ERROR。
 * @example
 * MRT_PortDspC28xRequestContextSwitch();
 */
MRT_Result MRT_PortDspC28xRequestContextSwitch(void);

/**
 * @brief 确认一次挂起的 DSP 上下文切换已经被服务。
 * @param void 无输入参数。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示挂起标志已清除。
 * @example
 * MRT_PortDspC28xAcknowledgeContextSwitch();
 */
MRT_Result MRT_PortDspC28xAcknowledgeContextSwitch(void);

/**
 * @brief 进入 DSP 中断嵌套层级。
 * @param void 无输入参数。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示嵌套计数已增加；计数溢出时返回 MRT_RESULT_INTERNAL_ERROR。
 * @example
 * MRT_PortDspC28xEnterInterrupt();
 */
MRT_Result MRT_PortDspC28xEnterInterrupt(void);

/**
 * @brief 退出 DSP 中断嵌套层级并报告是否应执行延迟切换。
 * @param out_should_switch 输出是否应在最外层 ISR 退出后执行上下文切换，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示退出成功；输出参数为空返回 MRT_RESULT_INVALID_ARGUMENT；没有匹配的进入记录时返回 MRT_RESULT_INVALID_CONTEXT。
 * @example
 * bool should_switch;
 * MRT_PortDspC28xExitInterrupt(&should_switch);
 */
MRT_Result MRT_PortDspC28xExitInterrupt(bool *out_should_switch);

/**
 * @brief 查询 DSP 上下文切换请求是否挂起。
 * @param void 无输入参数。
 * @return bool 返回 true 表示已有挂起切换请求，false 表示没有挂起请求。
 * @example
 * bool pending = MRT_PortDspC28xIsContextSwitchPending();
 */
bool MRT_PortDspC28xIsContextSwitchPending(void);

/**
 * @brief 查询 DSP 上下文切换请求累计次数。
 * @param void 无输入参数。
 * @return uint32_t 返回累计请求次数。
 * @example
 * uint32_t count = MRT_PortDspC28xGetContextSwitchRequestCount();
 */
uint32_t MRT_PortDspC28xGetContextSwitchRequestCount(void);

/**
 * @brief 查询当前 DSP 中断嵌套深度。
 * @param void 无输入参数。
 * @return uint32_t 返回当前嵌套深度，0 表示任务上下文。
 * @example
 * uint32_t depth = MRT_PortDspC28xGetInterruptNesting();
 */
uint32_t MRT_PortDspC28xGetInterruptNesting(void);

#endif
