#include "myrtos/mrt_kernel.h"
#include "myrtos/mrt_port.h"
#include "mrt_task_internal.h"

/** @brief 当前系统 tick 计数。 */
static MRT_Tick g_kernel_tick;

/** @brief 调度器是否正在运行。 */
static bool g_kernel_running;

/** @brief 调度器挂起嵌套深度。 */
static uint32_t g_scheduler_suspend_depth;

/**
 * @brief 初始化 MyRTOS 内核基础状态。
 * @param void 无输入参数。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示初始化成功。
 * @example
 * MRT_Result result = MRT_KernelInitialize();
 */
MRT_Result MRT_KernelInitialize(void)
{
    /* 将系统 tick 复位为 0，确保每次初始化都有确定起点。 */
    g_kernel_tick = 0u;

    /* 标记调度器尚未运行。 */
    g_kernel_running = false;

    /* 清空调度器挂起嵌套深度。 */
    g_scheduler_suspend_depth = 0u;

    /* 初始化端口层，使临界区、yield 和 ISR 状态进入已知状态。 */
    MRT_PortInitialize();

    /* 初始化任务调度器内部 ready/delay 状态。 */
    MRT_TaskKernelInitialize();

    /* 基础状态初始化完成，返回成功。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 启动 MyRTOS 调度器。
 * @param void 无输入参数。
 * @return MRT_Result 无可运行任务时返回 MRT_RESULT_NOT_STARTED；后续调度器接入后成功启动返回 MRT_RESULT_OK。
 * @example
 * MRT_Result result = MRT_KernelStart();
 */
MRT_Result MRT_KernelStart(void)
{
    /* 如果调度器已经运行，则不重复启动首任务。 */
    if (g_kernel_running) {
        /* 返回已经启动，提示调用方无需重复启动。 */
        return MRT_RESULT_ALREADY_STARTED;
    }

    /* 请求任务模块选择第一个可运行任务。 */
    if (!MRT_TaskKernelStartScheduler()) {
        /* 没有 ready 任务时无法启动调度器。 */
        return MRT_RESULT_NOT_STARTED;
    }

    /* 标记调度器已经进入运行状态。 */
    g_kernel_running = true;

    /* 通知端口层启动第一个任务。 */
    MRT_PortStartFirstTask();

    /* 调度器启动成功。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 查询调度器是否正在运行。
 * @param void 无输入参数。
 * @return bool 返回 true 表示调度器正在运行，返回 false 表示尚未运行。
 * @example
 * if (MRT_KernelIsRunning()) { MRT_KernelYield(); }
 */
bool MRT_KernelIsRunning(void)
{
    /* 返回当前调度器运行标志。 */
    return g_kernel_running;
}

/**
 * @brief 获取当前系统 tick。
 * @param void 无输入参数。
 * @return MRT_Tick 返回当前系统 tick 计数。
 * @example
 * MRT_Tick now = MRT_KernelGetTick();
 */
MRT_Tick MRT_KernelGetTick(void)
{
    /* 返回当前系统 tick 计数。 */
    return g_kernel_tick;
}

/**
 * @brief 推进一个系统 tick。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * void SysTick_Handler(void) { MRT_KernelTick(); }
 */
void MRT_KernelTick(void)
{
    /* 将系统 tick 递增 1，溢出按无符号整数自然回绕。 */
    g_kernel_tick++;

    /* 通知任务模块处理延时到期任务。 */
    MRT_TaskKernelTick(g_kernel_tick);
}

#if MRT_TESTING
/**
 * @brief 测试环境直接设置当前系统 tick。
 * @param tick 要写入的系统 tick 值。
 * @return void 无返回值。
 * @example
 * MRT_KernelTestSetTick(UINT32_MAX - 1u);
 */
void MRT_KernelTestSetTick(MRT_Tick tick)
{
    /* 直接写入内核 tick，用于构造 tick 溢出等边界场景。 */
    g_kernel_tick = tick;
}
#endif

/**
 * @brief 当前任务主动让出 CPU。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * MRT_KernelYield();
 */
void MRT_KernelYield(void)
{
    /* 先让任务模块执行 ready list 轮转和重新选主。 */
    MRT_TaskKernelYield();

    /* 将主动让出请求转发给端口层。 */
    MRT_PortYield();
}

/**
 * @brief 挂起调度器。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * MRT_KernelSuspendAll();
 */
void MRT_KernelSuspendAll(void)
{
    /* 增加挂起嵌套深度，支持多层调用安全配对。 */
    g_scheduler_suspend_depth++;
}

/**
 * @brief 恢复调度器。
 * @param void 无输入参数。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示恢复成功；未挂起时返回 MRT_RESULT_INVALID_CONTEXT。
 * @example
 * MRT_Result result = MRT_KernelResumeAll();
 */
MRT_Result MRT_KernelResumeAll(void)
{
    /* 如果没有挂起层级，则恢复操作上下文非法。 */
    if (g_scheduler_suspend_depth == 0u) {
        /* 返回非法上下文，提示调用方恢复次数多于挂起次数。 */
        return MRT_RESULT_INVALID_CONTEXT;
    }

    /* 减少一层调度器挂起深度。 */
    g_scheduler_suspend_depth--;

    /* 恢复成功。 */
    return MRT_RESULT_OK;
}
