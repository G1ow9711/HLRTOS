#include "myrtos/mrt_kernel.h"
#include "myrtos/mrt_port.h"
#include "myrtos/mrt_queue.h"
#include "myrtos/portable/mrt_port_dsp_c28x.h"
#include "mrt_task_internal.h"

#include <stdint.h>

#define MRT_DSP_C2000_ADC_QUEUE_LENGTH 8u
#define MRT_DSP_C2000_TASK_STACK_WORDS 128u

extern void MRT_PortDspC28xStartFirstTaskAsm(void);
extern void MRT_PortDspC28xSoftwareInterruptHandler(void);
extern void MRT_PortDspC28xYieldAsm(void);

/** @brief C2000 smoke 当前是否位于 ISR 上下文。 */
static volatile bool g_mrt_dsp_c2000_inside_isr;

/** @brief C2000 smoke 临界区嵌套深度。 */
static volatile uint32_t g_mrt_dsp_c2000_critical_depth;

/** @brief C2000 smoke 最近一次 tick ISR 是否请求切换。 */
static volatile bool g_mrt_dsp_c2000_last_tick_yield;

/** @brief C2000 smoke ADC 队列控制块。 */
static MRT_Queue g_mrt_dsp_c2000_adc_queue_storage;

/** @brief C2000 smoke ADC 队列数据区。 */
static uint8_t g_mrt_dsp_c2000_adc_queue_buffer[MRT_DSP_C2000_ADC_QUEUE_LENGTH * sizeof(uint16_t)];

/** @brief C2000 smoke ADC 队列句柄。 */
static MRT_QueueHandle g_mrt_dsp_c2000_adc_queue;

/** @brief C2000 smoke 首任务栈区。 */
static MRT_StackType g_mrt_dsp_c2000_first_task_stack[MRT_DSP_C2000_TASK_STACK_WORDS];

/** @brief C2000 smoke 首任务初始化后的栈顶。 */
static MRT_StackType *g_mrt_dsp_c2000_first_stack_top;

/**
 * @brief 首任务异常返回兜底函数。
 * @param void 无输入参数。
 * @return void 无返回值；任务不应返回到此处。
 * @example
 * mrt_dsp_c2000_task_exit();
 */
static void mrt_dsp_c2000_task_exit(void)
{
    /* 任务入口意外返回时停在此处，便于真实板级 smoke 记录错误。 */
    for (;;) {
        /* 保持停机状态，等待调试器读取栈和任务状态。 */
    }
}

/**
 * @brief C2000 smoke 首任务占位入口。
 * @param argument 用户参数，当前 smoke 不使用。
 * @return void 无返回值。
 * @example
 * mrt_dsp_c2000_first_task(0);
 */
static void mrt_dsp_c2000_first_task(void *argument)
{
    /* 当前占位任务不使用传入参数。 */
    (void)argument;

    /* 真实板级 smoke 可在此切换 GPIO、输出 UART 心跳或运行混合负载。 */
    for (;;) {
        /* 保持任务存活，等待 tick、ADC ISR 和软件中断切换路径触发。 */
    }
}

/**
 * @brief 初始化 C2000 smoke 板级资源。
 * @param void 无输入参数。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示队列和首任务栈帧已准备好。
 * @example
 * MRT_DspC2000BoardSmokeInit();
 */
MRT_Result MRT_DspC2000BoardSmokeInit(void)
{
    /* 先清空端口可观测状态，保证 smoke 从确定状态开始。 */
    MRT_PortInitialize();

    /* 创建 ADC ISR 到任务的静态队列，避免板级 smoke 依赖动态堆。 */
    MRT_Result result = MRT_QueueCreateStatic(MRT_DSP_C2000_ADC_QUEUE_LENGTH,
                                              sizeof(uint16_t),
                                              g_mrt_dsp_c2000_adc_queue_buffer,
                                              &g_mrt_dsp_c2000_adc_queue_storage,
                                              &g_mrt_dsp_c2000_adc_queue);
    if (result != MRT_RESULT_OK) {
        /* 队列创建失败时直接返回错误，真实板级 smoke 应把该状态写入日志。 */
        return result;
    }

    /* 按 C28x 栈帧契约初始化首任务栈顶，真实汇编启动入口会恢复该帧。 */
    return MRT_PortDspC28xInitializeStack(g_mrt_dsp_c2000_first_task_stack,
                                          MRT_DSP_C2000_TASK_STACK_WORDS,
                                          mrt_dsp_c2000_first_task,
                                          0,
                                          mrt_dsp_c2000_task_exit,
                                          &g_mrt_dsp_c2000_first_stack_top);
}

/**
 * @brief 初始化 MyRTOS C2000 端口状态。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * MRT_PortInitialize();
 */
void MRT_PortInitialize(void)
{
    /* 清除 ISR 上下文标志。 */
    g_mrt_dsp_c2000_inside_isr = false;

    /* 清除临界区深度。 */
    g_mrt_dsp_c2000_critical_depth = 0u;

    /* 清除最近一次 tick 切换请求记录。 */
    g_mrt_dsp_c2000_last_tick_yield = false;
}

/**
 * @brief 启动第一个 C2000 任务。
 * @param void 无输入参数。
 * @return void 无返回值；完整目标端口通常不会返回。
 * @example
 * MRT_PortStartFirstTask();
 */
void MRT_PortStartFirstTask(void)
{
    /* 转入 C28x 汇编骨架，由汇编恢复首任务栈帧。 */
    MRT_PortDspC28xStartFirstTaskAsm();
}

/**
 * @brief 请求一次任务上下文切换。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * MRT_PortYield();
 */
void MRT_PortYield(void)
{
    /* 先记录 C 模型中的挂起切换请求，便于 smoke 日志审计。 */
    (void)MRT_PortDspC28xRequestContextSwitch();

    /* 再触发目标软件中断入口，真实工程应在汇编中置位 IFR/PIE 标志。 */
    MRT_PortDspC28xYieldAsm();
}

/**
 * @brief 在 ISR 退出前按需请求延迟上下文切换。
 * @param should_yield true 表示 ISR 唤醒了更高优先级任务。
 * @return void 无返回值。
 * @example
 * MRT_PortYieldFromISR(should_yield);
 */
void MRT_PortYieldFromISR(bool should_yield)
{
    /* 只有真正需要切换时才触发软件中断，避免每次 ISR 退出都扰动调度器。 */
    if (should_yield) {
        /* 复用任务上下文 yield 路径，保持软件中断切换入口一致。 */
        MRT_PortYield();
    }
}

/**
 * @brief 抑制周期 tick 并进入 C2000 低功耗占位路径。
 * @param expected_idle_ticks 内核预计可睡眠 tick 数。
 * @param out_slept_ticks 输出实际睡眠 tick 数，不能为 NULL。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示占位低功耗路径完成。
 * @example
 * MRT_Tick slept;
 * MRT_PortSuppressTicksAndSleep(10u, &slept);
 */
MRT_Result MRT_PortSuppressTicksAndSleep(MRT_Tick expected_idle_ticks, MRT_Tick *out_slept_ticks)
{
    /* 输出指针为空时无法把补偿 tick 交还内核。 */
    if (out_slept_ticks == 0) {
        /* 返回统一参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 当前骨架把实际睡眠视为等于内核允许值，真实端口应读取低功耗计时器修正。 */
    *out_slept_ticks = expected_idle_ticks;

    /* 低功耗入口占位完成。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 进入 C2000 临界区并返回旧中断状态。
 * @param void 无输入参数。
 * @return MRT_IntState 返回进入前的抽象中断状态。
 * @example
 * MRT_IntState state = MRT_PortEnterCritical();
 */
MRT_IntState MRT_PortEnterCritical(void)
{
    /* 读取当前抽象临界区深度，真实端口应读取 INTM/IER/PIEIER 状态。 */
    MRT_IntState previous = (MRT_IntState)g_mrt_dsp_c2000_critical_depth;

    /* 增加嵌套深度，模拟屏蔽可抢占中断。 */
    g_mrt_dsp_c2000_critical_depth++;

    /* 返回旧状态，供退出时恢复。 */
    return previous;
}

/**
 * @brief 退出 C2000 临界区并恢复旧中断状态。
 * @param state MRT_PortEnterCritical 返回的旧状态。
 * @return void 无返回值。
 * @example
 * MRT_PortExitCritical(state);
 */
void MRT_PortExitCritical(MRT_IntState state)
{
    /* 按旧状态恢复抽象深度，真实端口应恢复 INTM/IER/PIEIER。 */
    g_mrt_dsp_c2000_critical_depth = (uint32_t)state;
}

/**
 * @brief 判断当前是否处于 ISR 上下文。
 * @param void 无输入参数。
 * @return bool 返回 true 表示当前处于 ISR。
 * @example
 * bool inside = MRT_PortIsInsideISR();
 */
bool MRT_PortIsInsideISR(void)
{
    /* 返回板级 ISR 入口维护的上下文标志。 */
    return g_mrt_dsp_c2000_inside_isr;
}

/**
 * @brief C28x 汇编首任务启动钩子。
 * @param void 无输入参数。
 * @return MRT_StackType* 返回首任务初始化栈顶。
 * @example
 * MRT_StackType *top = MRT_PortDspC28xStartFirstTaskHook();
 */
MRT_StackType *MRT_PortDspC28xStartFirstTaskHook(void)
{
    /* 把 C 初始化得到的首任务栈顶交给汇编恢复路径。 */
    return g_mrt_dsp_c2000_first_stack_top;
}

/**
 * @brief C28x 汇编上下文切换钩子。
 * @param current_stack_top 当前任务保存寄存器后的栈顶。
 * @return MRT_StackType* 返回下一个任务应恢复的栈顶。
 * @example
 * MRT_StackType *next = MRT_PortDspC28xSwitchHook(current);
 */
MRT_StackType *MRT_PortDspC28xSwitchHook(MRT_StackType *current_stack_top)
{
    /* 复用内核 TCB 栈顶契约：保存旧栈顶并取出当前任务栈顶。 */
    MRT_StackType *next_stack_top = MRT_TaskKernelSwitchStackTop(current_stack_top);

    /* 如果当前没有可恢复任务，保持原栈顶作为诊断兜底。 */
    if (next_stack_top == 0) {
        /* 返回原栈顶，避免汇编恢复空指针。 */
        return current_stack_top;
    }

    /* 返回调度器选择的任务栈顶。 */
    return next_stack_top;
}

/**
 * @brief C2000 CPU Timer0 tick ISR。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * CpuTimer0Isr();
 */
void CpuTimer0Isr(void)
{
    /* 标记进入 ISR，上层 FromISR API 可据此判断调用上下文。 */
    g_mrt_dsp_c2000_inside_isr = true;

    /* 记录 DSP ISR 嵌套进入，便于最外层退出时决定是否切换。 */
    (void)MRT_PortDspC28xEnterInterrupt();

    /* 推进 MyRTOS tick，唤醒延时任务并投递软件定时器事件。 */
    MRT_KernelTick();

    /* 从 DSP 端口模型读取最外层 ISR 是否需要延迟切换。 */
    bool should_yield = false;
    (void)MRT_PortDspC28xExitInterrupt(&should_yield);

    /* 记录 tick ISR 是否触发切换，真实板级 smoke 应输出该字段。 */
    g_mrt_dsp_c2000_last_tick_yield = should_yield;

    /* 退出 ISR 标志。 */
    g_mrt_dsp_c2000_inside_isr = false;

    /* 如有需要，触发软件中断式上下文切换。 */
    MRT_PortYieldFromISR(should_yield);
}

/**
 * @brief C2000 软件中断上下文切换 ISR。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * MRT_DspC2000SoftwareInterruptIsr();
 */
void MRT_DspC2000SoftwareInterruptIsr(void)
{
    /* 进入汇编上下文切换入口，保存当前任务并恢复下一个任务。 */
    MRT_PortDspC28xSoftwareInterruptHandler();

    /* 汇编返回后确认挂起请求已被服务，便于 smoke 诊断。 */
    (void)MRT_PortDspC28xAcknowledgeContextSwitch();
}

/**
 * @brief C2000 ADC/DMA 数据就绪 ISR。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * MRT_DspC2000AdcIsr();
 */
void MRT_DspC2000AdcIsr(void)
{
    /* 真实工程应从 ADC result 或 DMA buffer 读取采样值。 */
    uint16_t sample = 0u;

    /* 标记进入 ISR。 */
    g_mrt_dsp_c2000_inside_isr = true;

    /* 进入 DSP ISR 嵌套模型。 */
    (void)MRT_PortDspC28xEnterInterrupt();

    /* 用 FromISR API 把采样值送入队列，并取得是否需要切换的结果。 */
    bool queue_should_yield = false;
    (void)MRT_QueueSendFromISR(g_mrt_dsp_c2000_adc_queue, &sample, &queue_should_yield);

    /* 退出 DSP ISR 嵌套模型，得到上下文切换请求。 */
    bool context_should_yield = false;
    (void)MRT_PortDspC28xExitInterrupt(&context_should_yield);

    /* 清除 ISR 标志。 */
    g_mrt_dsp_c2000_inside_isr = false;

    /* 队列唤醒或嵌套模型请求任一成立时触发软件中断切换。 */
    MRT_PortYieldFromISR(queue_should_yield || context_should_yield);
}
