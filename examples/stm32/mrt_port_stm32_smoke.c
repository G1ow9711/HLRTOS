#include "myrtos/mrt_port.h"
#include "myrtos/portable/mrt_port_stm32_cm.h"

#include <stdint.h>

#define MRT_STM32_SCB_ICSR (*(volatile uint32_t *)0xE000ED04u)
#define MRT_STM32_SCB_SHPR3 (*(volatile uint32_t *)0xE000ED20u)
#define MRT_STM32_SCB_ICSR_PENDSVSET (1u << 28)
#define MRT_STM32_SHPR3_PENDSV_PRI_SHIFT 16u
#define MRT_STM32_SHPR3_SYSTICK_PRI_SHIFT 24u
#define MRT_STM32_LOWEST_EXCEPTION_PRIORITY 0xFFu

/** @brief 当前是否已经请求 PendSV。 */
static volatile bool g_pending_switch;

/** @brief 当前是否处于 ISR 上下文。 */
static volatile bool g_inside_isr;

/** @brief 当前临界区嵌套深度。 */
static volatile uint32_t g_critical_depth;

/** @brief 最近一次系统定时器 reload。 */
static volatile uint32_t g_last_reload;

/** @brief 最近一次 BASEPRI 编码结果。 */
static volatile uint32_t g_last_basepri;

/** @brief SVC 汇编入口最近一次传入的异常帧地址。 */
static volatile uintptr_t g_last_svc_frame;

/** @brief SVC 汇编入口最近一次传入的 EXC_RETURN。 */
static volatile uint32_t g_last_svc_exc_return;

/** @brief PendSV 汇编入口最近一次传入的 PSP 栈顶。 */
static volatile uintptr_t g_last_pendsv_stack_top;

/** @brief PendSV 汇编入口最近一次传入的 EXC_RETURN。 */
static volatile uint32_t g_last_pendsv_exc_return;

/** @brief SVC 钩子进入次数，用于 smoke 诊断。 */
static volatile uint32_t g_svc_hook_count;

/** @brief PendSV 钩子进入次数，用于 smoke 诊断。 */
static volatile uint32_t g_pendsv_hook_count;

/**
 * @brief 读取当前 PRIMASK 状态。
 * @param void 无输入参数。
 * @return uint32_t 返回当前 PRIMASK 取值。
 * @example
 * uint32_t primask = mrt_stm32_read_primask();
 */
static uint32_t mrt_stm32_read_primask(void)
{
    /* 把 PRIMASK 读取到普通寄存器，供临界区退出时恢复。 */
    uint32_t primask;
    __asm volatile ("MRS %0, primask" : "=r"(primask) :: "memory");
    return primask;
}

/**
 * @brief 写入 PRIMASK 状态。
 * @param primask 需要写回的 PRIMASK 值。
 * @return void 无返回值。
 * @example
 * mrt_stm32_write_primask(primask);
 */
static void mrt_stm32_write_primask(uint32_t primask)
{
    /* 直接恢复进入临界区前保存的中断屏蔽状态。 */
    __asm volatile ("MSR primask, %0" :: "r"(primask) : "memory");
}

/**
 * @brief 读取当前 IPSR 状态。
 * @param void 无输入参数。
 * @return uint32_t 返回当前异常号；0 表示任务上下文。
 * @example
 * uint32_t ipsr = mrt_stm32_read_ipsr();
 */
static uint32_t mrt_stm32_read_ipsr(void)
{
    /* 把 IPSR 读取出来，用于判断当前是否处于异常上下文。 */
    uint32_t ipsr;
    __asm volatile ("MRS %0, ipsr" : "=r"(ipsr) :: "memory");
    return ipsr;
}

/**
 * @brief 在 SCB 中设置 PendSV pending 位。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * mrt_stm32_pend_pendsv();
 */
static void mrt_stm32_pend_pendsv(void)
{
    /* 写 ICSR 的 PENDSVSET 位，通知 Cortex-M 在异常退出后执行 PendSV。 */
    MRT_STM32_SCB_ICSR = MRT_STM32_SCB_ICSR_PENDSVSET;
}

/**
 * @brief 初始化 STM32 Cortex-M smoke 端口状态。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * MRT_PortInitialize();
 */
void MRT_PortInitialize(void)
{
    /* 清空 PendSV 请求标志，便于 smoke 过程从干净状态开始。 */
    g_pending_switch = false;

    /* 默认认为当前位于任务上下文。 */
    g_inside_isr = false;

    /* 临界区嵌套深度清零。 */
    g_critical_depth = 0u;

    /* 清空最近一次记录的 SysTick reload。 */
    g_last_reload = 0u;

    /* 清空最近一次记录的 BASEPRI 编码。 */
    g_last_basepri = 0u;

    /* 将 PendSV 和 SysTick 置为最低优先级，符合 MyRTOS 端口契约。 */
    MRT_STM32_SCB_SHPR3 =
        ((uint32_t)MRT_STM32_LOWEST_EXCEPTION_PRIORITY << MRT_STM32_SHPR3_PENDSV_PRI_SHIFT) |
        ((uint32_t)MRT_STM32_LOWEST_EXCEPTION_PRIORITY << MRT_STM32_SHPR3_SYSTICK_PRI_SHIFT);
}

/**
 * @brief 启动第一个任务。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * MRT_PortStartFirstTask();
 */
void MRT_PortStartFirstTask(void)
{
    /* smoke 示例只记录启动动作，真实工程会在这里恢复第一个任务上下文。 */
    g_pending_switch = true;
}

/**
 * @brief 记录 SVC 汇编入口转交的异常现场。
 * @param exception_stack SVC 发生时硬件自动异常帧所在的 MSP 或 PSP 地址，可为空。
 * @param exc_return Cortex-M EXC_RETURN 值，用于判断异常返回模式和栈来源。
 * @return void 无返回值。
 * @example
 * MRT_PortStm32CmSvcHook(exception_stack, exc_return);
 */
void MRT_PortStm32CmSvcHook(uint32_t *exception_stack, uint32_t exc_return)
{
    /* 保存异常帧地址，便于真板 smoke 通过调试器或 UART 输出检查。 */
    g_last_svc_frame = (uintptr_t)exception_stack;

    /* 保存 EXC_RETURN，便于确认 SVC 入口来自 MSP 还是 PSP。 */
    g_last_svc_exc_return = exc_return;

    /* 记录 SVC 钩子被触发次数，证明向量表覆盖和汇编跳转已经接通。 */
    g_svc_hook_count++;
}

/**
 * @brief 记录 PendSV 汇编入口转交的 PSP 并返回待恢复栈顶。
 * @param stack_top 已保存 R4-R11 后的 PSP 栈顶，当前 smoke 骨架可为空。
 * @param exc_return Cortex-M EXC_RETURN 值，用于判断异常返回路径。
 * @return uint32_t* 返回要恢复的 PSP；当前 smoke 骨架返回原值。
 * @example
 * uint32_t *next = MRT_PortStm32CmPendSvHook(stack_top, exc_return);
 */
uint32_t *MRT_PortStm32CmPendSvHook(uint32_t *stack_top, uint32_t exc_return)
{
    /* 保存 PendSV 传入的栈顶，真实端口会在这里切换到下一任务栈顶。 */
    g_last_pendsv_stack_top = (uintptr_t)stack_top;

    /* 保存 EXC_RETURN，便于检查异常返回是否使用 PSP 线程模式。 */
    g_last_pendsv_exc_return = exc_return;

    /* 记录 PendSV 钩子被触发次数，证明延迟切换入口已经接通。 */
    g_pendsv_hook_count++;

    /* smoke 阶段不切换 TCB 栈顶，直接返回原 PSP。 */
    return stack_top;
}

/**
 * @brief 在任务上下文请求一次调度切换。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * MRT_PortYield();
 */
void MRT_PortYield(void)
{
    /* 记录一次任务主动让出请求。 */
    g_pending_switch = true;

    /* 通知 Cortex-M 在异常退出后执行 PendSV。 */
    mrt_stm32_pend_pendsv();
}

/**
 * @brief 在 ISR 退出前按需请求调度切换。
 * @param should_yield true 表示需要切换，false 表示不需要切换。
 * @return void 无返回值。
 * @example
 * MRT_PortYieldFromISR(should_yield);
 */
void MRT_PortYieldFromISR(bool should_yield)
{
    /* 只有真的需要切换时才写 PendSV。 */
    if (should_yield) {
        /* 记录一次 ISR 驱动的延迟切换请求。 */
        g_pending_switch = true;

        /* 安排 PendSV 在异常退出后处理上下文切换。 */
        mrt_stm32_pend_pendsv();
    }
}

/**
 * @brief 抑制周期 tick 并进入低功耗睡眠。
 * @param expected_idle_ticks 内核允许端口层连续睡眠的最大 tick 数。
 * @param out_slept_ticks 输出端口层实际睡眠的 tick 数，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示端口睡眠过程完成；参数为空时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * MRT_Tick slept;
 * MRT_PortSuppressTicksAndSleep(10u, &slept);
 */
MRT_Result MRT_PortSuppressTicksAndSleep(MRT_Tick expected_idle_ticks, MRT_Tick *out_slept_ticks)
{
    /* 输出指针为空时无法把真实睡眠 tick 回传给内核。 */
    if (out_slept_ticks == 0) {
        /* 参数非法。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 默认把实际睡眠 tick 视为内核允许值。 */
    *out_slept_ticks = expected_idle_ticks;

    /* 进入 WFI 之前先执行一次同步屏障，防止之前的寄存器写入乱序。 */
    __asm volatile ("dsb");
    __asm volatile ("wfi");
    __asm volatile ("isb");

    /* smoke 端口只证明调用链，真实工程会在这里读回低功耗计时器。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 进入临界区并保存旧中断状态。
 * @param void 无输入参数。
 * @return MRT_IntState 返回进入临界区前的端口状态，用于退出时恢复。
 * @example
 * MRT_IntState state = MRT_PortEnterCritical();
 */
MRT_IntState MRT_PortEnterCritical(void)
{
    /* 读取当前 PRIMASK，作为退出时恢复的旧状态。 */
    uint32_t previous = mrt_stm32_read_primask();

    /* 关闭可屏蔽中断，形成临界区。 */
    __asm volatile ("cpsid i" ::: "memory");

    /* 记录临界区嵌套深度，便于 smoke 诊断。 */
    g_critical_depth++;

    /* 返回进入前的 PRIMASK 值。 */
    return (MRT_IntState)previous;
}

/**
 * @brief 退出临界区并恢复旧中断状态。
 * @param state MRT_PortEnterCritical 返回的旧状态。
 * @return void 无返回值。
 * @example
 * MRT_PortExitCritical(state);
 */
void MRT_PortExitCritical(MRT_IntState state)
{
    /* 恢复进入临界区前的 PRIMASK。 */
    mrt_stm32_write_primask((uint32_t)state);

    /* 维护一个有限的调试计数器，避免在 smoke 过程中不断累加。 */
    if (g_critical_depth != 0u) {
        /* 出口时减 1，保持和入口配对。 */
        g_critical_depth--;
    }
}

/**
 * @brief 判断当前是否处于 ISR 上下文。
 * @param void 无输入参数。
 * @return bool 返回 true 表示当前在 ISR 中，返回 false 表示当前在任务上下文。
 * @example
 * if (MRT_PortIsInsideISR()) { MRT_QueueSendFromISR(queue, item, &yield); }
 */
bool MRT_PortIsInsideISR(void)
{
    /* IPSR 非 0 代表正在异常上下文中。 */
    return mrt_stm32_read_ipsr() != 0u;
}

/**
 * @brief 查询最近一次烟雾测试写入的 SysTick reload。
 * @param void 无输入参数。
 * @return uint32_t 返回最近一次写入的 SysTick reload。
 * @example
 * uint32_t reload = MRT_PortStm32SmokeGetLastReload();
 */
uint32_t MRT_PortStm32SmokeGetLastReload(void)
{
    /* 返回最近一次记录的 reload，便于主函数做自检。 */
    return g_last_reload;
}

/**
 * @brief 查询最近一次烟雾测试写入的 BASEPRI 编码。
 * @param void 无输入参数。
 * @return uint32_t 返回最近一次记录的 BASEPRI 编码。
 * @example
 * uint32_t basepri = MRT_PortStm32SmokeGetLastBasepri();
 */
uint32_t MRT_PortStm32SmokeGetLastBasepri(void)
{
    /* 返回最近一次记录的 BASEPRI 编码。 */
    return g_last_basepri;
}

/**
 * @brief 记录 smoke 例程计算出的 tick 和优先级参数。
 * @param reload SysTick reload。
 * @param basepri BASEPRI 编码结果。
 * @return void 无返回值。
 * @example
 * MRT_PortStm32SmokeRecordTiming(reload, basepri);
 */
void MRT_PortStm32SmokeRecordTiming(uint32_t reload, uint32_t basepri)
{
    /* 保存 reload 供后续检查。 */
    g_last_reload = reload;

    /* 保存 BASEPRI 编码供后续检查。 */
    g_last_basepri = basepri;
}
