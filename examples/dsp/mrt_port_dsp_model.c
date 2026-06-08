#include "myrtos/mrt_port.h"
#include "myrtos/portable/mrt_port_dsp_c28x.h"

#include <stdint.h>

/** @brief 当前是否处于 ISR 上下文。 */
static bool g_inside_isr;

/** @brief 当前临界区嵌套深度。 */
static uint32_t g_critical_depth;

/** @brief 当前是否有待服务的上下文切换请求。 */
static bool g_yield_requested;

/** @brief 最近一次实际请求保存的睡眠 tick。 */
static MRT_Tick g_last_slept_ticks;

/**
 * @brief 复位 DSP smoke model 的公共 port 状态。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * MRT_PortInitialize();
 */
void MRT_PortInitialize(void)
{
    /* 默认切回任务上下文。 */
    g_inside_isr = false;

    /* 临界区嵌套深度归零。 */
    g_critical_depth = 0u;

    /* 清除挂起切换请求。 */
    g_yield_requested = false;

    /* 清除最近一次睡眠 tick 记录。 */
    g_last_slept_ticks = 0u;

    /* 同时复位 DSP helper 的内部状态。 */
    MRT_PortDspC28xContextModelReset();
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
    /* smoke model 只标记启动请求，不做真实寄存器恢复。 */
    g_yield_requested = true;
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
    /* 记录一次任务主动让出。 */
    g_yield_requested = true;

    /* 把请求同步到 DSP helper 统计中。 */
    (void)MRT_PortDspC28xRequestContextSwitch();
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
    /* 只有真切换请求才需要进入挂起态。 */
    if (should_yield) {
        /* 记录一次 ISR 驱动的切换请求。 */
        g_yield_requested = true;

        /* 把请求同步到 DSP helper 统计中。 */
        (void)MRT_PortDspC28xRequestContextSwitch();
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
    /* 输出参数不能为空。 */
    if (out_slept_ticks == 0) {
        /* 参数非法。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 直接回报期望的睡眠 tick 数。 */
    *out_slept_ticks = expected_idle_ticks;

    /* 记录本次模型睡眠值。 */
    g_last_slept_ticks = expected_idle_ticks;

    /* smoke model 不真的关闭时钟，只验证调用链。 */
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
    /* 保存当前嵌套深度。 */
    MRT_IntState state = (MRT_IntState)g_critical_depth;

    /* 增加临界区深度。 */
    g_critical_depth++;

    /* 返回进入前的深度。 */
    return state;
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
    /* 把深度恢复到调用方保存的值。 */
    g_critical_depth = (uint32_t)state;
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
    /* 返回当前模型记录的上下文状态。 */
    return g_inside_isr;
}

/**
 * @brief 查询最近一次模型睡眠 tick。
 * @param void 无输入参数。
 * @return MRT_Tick 返回最近一次 MRT_PortSuppressTicksAndSleep 回报的 tick。
 * @example
 * MRT_Tick slept = MRT_PortDspModelGetLastSleptTicks();
 */
MRT_Tick MRT_PortDspModelGetLastSleptTicks(void)
{
    /* 返回最近一次睡眠 tick 记录。 */
    return g_last_slept_ticks;
}

/**
 * @brief 查询最近一次切换请求是否已挂起。
 * @param void 无输入参数。
 * @return bool 返回 true 表示曾请求切换且尚未清除。
 * @example
 * bool pending = MRT_PortDspModelWasYieldRequested();
 */
bool MRT_PortDspModelWasYieldRequested(void)
{
    /* 返回当前挂起标志。 */
    return g_yield_requested;
}

/**
 * @brief 模型态进入 ISR。
 * @param void 无输入参数。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示进入成功。
 * @example
 * MRT_PortDspModelEnterIsr();
 */
MRT_Result MRT_PortDspModelEnterIsr(void)
{
    /* 先让 helper 记录一层 ISR 嵌套。 */
    MRT_Result result = MRT_PortDspC28xEnterInterrupt();
    if (result != MRT_RESULT_OK) {
        /* helper 失败时直接返回。 */
        return result;
    }

    /* 再把公共端口状态切到 ISR。 */
    g_inside_isr = true;

    /* 进入成功。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 模型态退出 ISR。
 * @param out_should_switch 输出是否应在最外层退出后进行切换，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示退出成功；参数为空或层级下溢时返回对应错误。
 * @example
 * bool should_switch;
 * MRT_PortDspModelExitIsr(&should_switch);
 */
MRT_Result MRT_PortDspModelExitIsr(bool *out_should_switch)
{
    /* 输出参数不能为空。 */
    if (out_should_switch == 0) {
        /* 参数非法。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 先把退出语义交给 DSP helper。 */
    MRT_Result result = MRT_PortDspC28xExitInterrupt(out_should_switch);
    if (result != MRT_RESULT_OK) {
        /* helper 失败时直接返回。 */
        return result;
    }

    /* 当嵌套深度回到 0 时，公共端口状态也回到任务上下文。 */
    if (MRT_PortDspC28xGetInterruptNesting() == 0u) {
        g_inside_isr = false;
    }

    /* 返回成功。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 清除模型态的挂起切换请求。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * MRT_PortDspModelAcknowledgeYield();
 */
void MRT_PortDspModelAcknowledgeYield(void)
{
    /* 清除公共模型里的挂起标志。 */
    g_yield_requested = false;

    /* 同步清除 helper 里的挂起请求。 */
    (void)MRT_PortDspC28xAcknowledgeContextSwitch();
}
