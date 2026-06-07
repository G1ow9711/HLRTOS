#include "myrtos/portable/mrt_port_dsp_c28x.h"

#include <stdint.h>

/** @brief DSP context 模型中是否已有挂起的软件中断切换请求。 */
static bool g_dsp_context_switch_pending;

/** @brief DSP context 模型中累计收到的切换请求次数。 */
static uint32_t g_dsp_context_switch_request_count;

/** @brief DSP context 模型中当前 ISR 嵌套深度。 */
static uint32_t g_dsp_interrupt_nesting;

/**
 * @brief 将地址向下对齐到指定边界。
 * @param value 待对齐的地址数值。
 * @param alignment 对齐边界，必须为 2 的幂。
 * @return uintptr_t 返回向下对齐后的地址数值。
 * @example
 * uintptr_t aligned = mrt_dsp_c28x_align_down(raw_end, 8u);
 */
static uintptr_t mrt_dsp_c28x_align_down(uintptr_t value, uintptr_t alignment)
{
    /* 生成对齐掩码，alignment 为 8 时低 3 bit 会被清零。 */
    uintptr_t mask = alignment - 1u;

    /* 清除低位未对齐 bit，得到向下取整后的地址。 */
    return value & ~mask;
}

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
                                          MRT_StackType **out_stack_top)
{
    /* 输出指针为空时无法回传栈顶，直接拒绝。 */
    if (out_stack_top == 0) {
        /* 返回统一参数错误码。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 先清空输出，保证失败路径不会留下旧栈顶。 */
    *out_stack_top = 0;

    /* 栈内存、任务入口和任务退出处理函数都必须有效。 */
    if ((stack_memory == 0) || (entry == 0) || (task_exit == 0)) {
        /* 返回统一参数错误码。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 栈空间必须至少能容纳完整 DSP 初始帧。 */
    if (stack_words < MRT_PORT_DSP_C28X_INITIAL_FRAME_WORDS) {
        /* 返回统一参数错误码。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 读取栈起始地址，后续用整数形式做对齐计算。 */
    uintptr_t stack_start = (uintptr_t)stack_memory;

    /* 栈起始地址必须满足 MRT_StackType 对齐，否则无法安全写入槽位。 */
    if ((stack_start & (uintptr_t)(sizeof(MRT_StackType) - 1u)) != 0u) {
        /* 返回统一参数错误码。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 防御乘加溢出，避免极端参数绕回地址空间。 */
    if (stack_words > ((UINTPTR_MAX - stack_start) / sizeof(MRT_StackType))) {
        /* 返回统一参数错误码。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 计算调用者提供栈区的末尾地址，末尾地址指向最后一个元素之后。 */
    uintptr_t stack_end = stack_start + (stack_words * sizeof(MRT_StackType));

    /* DSP 端口契约要求初始栈顶保持 8 字节对齐，兼容双字访问和 DMA 检查。 */
    uintptr_t aligned_end = mrt_dsp_c28x_align_down(stack_end, 8u);

    /* 对齐后如果已经没有可用空间，说明输入栈区非法。 */
    if (aligned_end <= stack_start) {
        /* 返回统一参数错误码。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 计算对齐后仍可使用的栈元素数量。 */
    size_t usable_words = (size_t)((aligned_end - stack_start) / sizeof(MRT_StackType));

    /* 对齐损失后仍必须能容纳完整 DSP 初始帧。 */
    if (usable_words < MRT_PORT_DSP_C28X_INITIAL_FRAME_WORDS) {
        /* 返回统一参数错误码。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 初始栈帧从对齐末尾向低地址方向预留固定数量槽位。 */
    MRT_StackType *frame = ((MRT_StackType *)aligned_end) - MRT_PORT_DSP_C28X_INITIAL_FRAME_WORDS;

    /* ST0 初始不携带业务状态。 */
    frame[MRT_PORT_DSP_C28X_FRAME_ST0] = MRT_PORT_DSP_C28X_INITIAL_STATUS_WORD;

    /* ST1 初始采用端口契约定义的状态字。 */
    frame[MRT_PORT_DSP_C28X_FRAME_ST1] = MRT_PORT_DSP_C28X_INITIAL_STATUS_WORD;

    /* IER 初始为 0，真实端口可在启动首任务前按板级策略恢复中断允许位。 */
    frame[MRT_PORT_DSP_C28X_FRAME_IER] = 0u;

    /* DEBUG 槽位清零，保留给具体 DSP 调试状态或扩展 ABI。 */
    frame[MRT_PORT_DSP_C28X_FRAME_DEBUG] = 0u;

    /* 写入 XAR4-XAR7 的调试占位值，表示这些被保存寄存器还没有真实任务值。 */
    for (uint32_t reg = 4u; reg <= 7u; reg++) {
        /* 根据寄存器号换算到初始帧内的 XAR 槽位。 */
        size_t slot = (size_t)MRT_PORT_DSP_C28X_FRAME_XAR4 + (size_t)(reg - 4u);

        /* 写入可识别模式，方便 host 测试和调试器检查。 */
        frame[slot] = (MRT_StackType)(MRT_PORT_DSP_C28X_REGISTER_PATTERN_BASE + reg);
    }

    /* ACC 低位初始清零。 */
    frame[MRT_PORT_DSP_C28X_FRAME_ACC_LOW] = 0u;

    /* ACC 高位初始清零。 */
    frame[MRT_PORT_DSP_C28X_FRAME_ACC_HIGH] = 0u;

    /* P 寄存器抽象槽初始清零。 */
    frame[MRT_PORT_DSP_C28X_FRAME_P] = 0u;

    /* XT 寄存器抽象槽初始清零。 */
    frame[MRT_PORT_DSP_C28X_FRAME_XT] = 0u;

    /* ARGUMENT 槽保存任务入口参数。 */
    frame[MRT_PORT_DSP_C28X_FRAME_ARGUMENT] = (MRT_StackType)(uintptr_t)argument;

    /* EXIT 槽保存任务函数意外返回后的兜底处理入口。 */
    frame[MRT_PORT_DSP_C28X_FRAME_EXIT] = (MRT_StackType)(uintptr_t)task_exit;

    /* PC 槽保存任务入口函数地址。 */
    frame[MRT_PORT_DSP_C28X_FRAME_PC] = (MRT_StackType)(uintptr_t)entry;

    /* RESERVED 槽清零，保持未来 ABI 扩展可预测。 */
    frame[MRT_PORT_DSP_C28X_FRAME_RESERVED] = 0u;

    /* 回传新栈顶，任务控制块保存该指针后即可参与调度。 */
    *out_stack_top = frame;

    /* DSP 初始栈帧构造完成。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 复位 DSP C28x 风格上下文切换模型状态。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * MRT_PortDspC28xContextModelReset();
 */
void MRT_PortDspC28xContextModelReset(void)
{
    /* 清除挂起的软件中断式上下文切换请求。 */
    g_dsp_context_switch_pending = false;

    /* 清零累计请求次数，便于单元测试从已知状态开始。 */
    g_dsp_context_switch_request_count = 0u;

    /* 清零中断嵌套深度，表示当前处于任务上下文。 */
    g_dsp_interrupt_nesting = 0u;
}

/**
 * @brief 请求一次 DSP 软件中断式上下文切换。
 * @param void 无输入参数。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示请求已记录；请求计数溢出时返回 MRT_RESULT_INTERNAL_ERROR。
 * @example
 * MRT_PortDspC28xRequestContextSwitch();
 */
MRT_Result MRT_PortDspC28xRequestContextSwitch(void)
{
    /* 如果请求计数已达最大值，继续递增会造成回绕。 */
    if (g_dsp_context_switch_request_count == UINT32_MAX) {
        /* 返回内部错误，提示端口模型已进入不可继续计数的异常状态。 */
        return MRT_RESULT_INTERNAL_ERROR;
    }

    /* 设置挂起标志，模拟真实 DSP 端口触发软件中断或置位调度请求。 */
    g_dsp_context_switch_pending = true;

    /* 累计请求次数，便于测试确认请求被记录。 */
    g_dsp_context_switch_request_count++;

    /* 请求记录完成。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 确认一次挂起的 DSP 上下文切换已经被服务。
 * @param void 无输入参数。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示挂起标志已清除。
 * @example
 * MRT_PortDspC28xAcknowledgeContextSwitch();
 */
MRT_Result MRT_PortDspC28xAcknowledgeContextSwitch(void)
{
    /* 清除挂起标志，模拟软件中断服务例程已经完成上下文切换。 */
    g_dsp_context_switch_pending = false;

    /* 确认动作完成。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 进入 DSP 中断嵌套层级。
 * @param void 无输入参数。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示嵌套计数已增加；计数溢出时返回 MRT_RESULT_INTERNAL_ERROR。
 * @example
 * MRT_PortDspC28xEnterInterrupt();
 */
MRT_Result MRT_PortDspC28xEnterInterrupt(void)
{
    /* 如果嵌套计数已达最大值，继续递增会造成回绕。 */
    if (g_dsp_interrupt_nesting == UINT32_MAX) {
        /* 返回内部错误，提示调用者中断进入/退出配对已经异常。 */
        return MRT_RESULT_INTERNAL_ERROR;
    }

    /* 增加 ISR 嵌套深度。 */
    g_dsp_interrupt_nesting++;

    /* 进入记录完成。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 退出 DSP 中断嵌套层级并报告是否应执行延迟切换。
 * @param out_should_switch 输出是否应在最外层 ISR 退出后执行上下文切换，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示退出成功；输出参数为空返回 MRT_RESULT_INVALID_ARGUMENT；没有匹配的进入记录时返回 MRT_RESULT_INVALID_CONTEXT。
 * @example
 * bool should_switch;
 * MRT_PortDspC28xExitInterrupt(&should_switch);
 */
MRT_Result MRT_PortDspC28xExitInterrupt(bool *out_should_switch)
{
    /* 输出指针为空时无法报告是否应切换。 */
    if (out_should_switch == 0) {
        /* 返回统一参数错误码，并保持嵌套深度不变。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 默认不要求切换，保证错误路径不会留下调用者旧值。 */
    *out_should_switch = false;

    /* 没有进入过 ISR 却请求退出，说明调用上下文非法。 */
    if (g_dsp_interrupt_nesting == 0u) {
        /* 返回非法上下文错误。 */
        return MRT_RESULT_INVALID_CONTEXT;
    }

    /* 减少 ISR 嵌套深度。 */
    g_dsp_interrupt_nesting--;

    /* 只有最外层 ISR 退出且存在挂起请求时，才提示执行延迟切换。 */
    if ((g_dsp_interrupt_nesting == 0u) && g_dsp_context_switch_pending) {
        /* 通知调用者在 ISR 尾部进入软件中断式上下文切换路径。 */
        *out_should_switch = true;
    }

    /* ISR 退出处理完成。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 查询 DSP 上下文切换请求是否挂起。
 * @param void 无输入参数。
 * @return bool 返回 true 表示已有挂起切换请求，false 表示没有挂起请求。
 * @example
 * bool pending = MRT_PortDspC28xIsContextSwitchPending();
 */
bool MRT_PortDspC28xIsContextSwitchPending(void)
{
    /* 返回当前挂起标志。 */
    return g_dsp_context_switch_pending;
}

/**
 * @brief 查询 DSP 上下文切换请求累计次数。
 * @param void 无输入参数。
 * @return uint32_t 返回累计请求次数。
 * @example
 * uint32_t count = MRT_PortDspC28xGetContextSwitchRequestCount();
 */
uint32_t MRT_PortDspC28xGetContextSwitchRequestCount(void)
{
    /* 返回累计请求次数。 */
    return g_dsp_context_switch_request_count;
}

/**
 * @brief 查询当前 DSP 中断嵌套深度。
 * @param void 无输入参数。
 * @return uint32_t 返回当前嵌套深度，0 表示任务上下文。
 * @example
 * uint32_t depth = MRT_PortDspC28xGetInterruptNesting();
 */
uint32_t MRT_PortDspC28xGetInterruptNesting(void)
{
    /* 返回当前 ISR 嵌套深度。 */
    return g_dsp_interrupt_nesting;
}
