#include "myrtos/mrt_heap.h"
#include "myrtos/mrt_kernel.h"
#include "myrtos/mrt_port.h"
#include "myrtos/mrt_queue.h"
#include "myrtos/mrt_task.h"
#include "myrtos/mrt_timer.h"
#include "myrtos/portable/mrt_port_dsp_c28x.h"

#include <stdbool.h>
#include <stdint.h>

extern MRT_Result MRT_PortDspModelEnterIsr(void);
extern MRT_Result MRT_PortDspModelExitIsr(bool *out_should_switch);
extern void MRT_PortDspModelAcknowledgeYield(void);
extern bool MRT_PortDspModelWasYieldRequested(void);
extern MRT_Tick MRT_PortDspModelGetLastSleptTicks(void);

#define DSP_HEAP_BYTES 4096u
#define DSP_QUEUE_ITEMS 4u
#define DSP_STACK_WORDS 128u
#define DSP_TIMER_PERIOD_TICKS 64u

/** @brief DSP smoke heap。 */
static uint8_t g_heap[DSP_HEAP_BYTES];

/** @brief DSP 采样队列控制块。 */
static MRT_Queue g_sample_queue_storage;

/** @brief DSP 采样队列缓冲。 */
static uint16_t g_sample_queue_buffer[DSP_QUEUE_ITEMS];

/** @brief DSP 采样队列句柄。 */
static MRT_QueueHandle g_sample_queue;

/** @brief DSP 周期定时器控制块。 */
static MRT_Timer g_tick_timer_storage;

/** @brief DSP 周期定时器句柄。 */
static MRT_TimerHandle g_tick_timer;

/** @brief DSP 采样任务控制块。 */
static MRT_Task g_sample_task_storage;

/** @brief DSP 采样任务栈。 */
static MRT_StackType g_sample_task_stack[DSP_STACK_WORDS];

/** @brief DSP 采样任务句柄。 */
static MRT_TaskHandle g_sample_task;

/** @brief DSP 通信任务控制块。 */
static MRT_Task g_comm_task_storage;

/** @brief DSP 通信任务栈。 */
static MRT_StackType g_comm_task_stack[DSP_STACK_WORDS];

/** @brief DSP 通信任务句柄。 */
static MRT_TaskHandle g_comm_task;

/** @brief 最近一次由 ISR 注入的采样值。 */
static volatile uint16_t g_last_sample;

/** @brief 定时器回调触发次数。 */
static volatile uint32_t g_tick_timer_count;

/**
 * @brief 采样处理任务。
 * @param arg 用户参数，当前示例不使用。
 * @return void 无返回值。
 * @example
 * DspSampleTask(0);
 */
static void DspSampleTask(void *arg)
{
    /* 参数仅用于展示任务入口签名。 */
    (void)arg;

    /* 任务主体采用无限循环，等待采样数据到来。 */
    for (;;) {
        /* 从队列里取出最新采样。 */
        uint16_t sample = 0u;

        /* 阻塞等待队列中的数据。 */
        if (MRT_QueueReceive(g_sample_queue, &sample, 10u) == MRT_RESULT_OK) {
            /* 保存最近一次采样。 */
            g_last_sample = sample;
        }
    }
}

/**
 * @brief 通信任务。
 * @param arg 用户参数，当前示例不使用。
 * @return void 无返回值。
 * @example
 * DspCommTask(0);
 */
static void DspCommTask(void *arg)
{
    /* 参数仅用于展示任务入口签名。 */
    (void)arg;

    /* 任务主体采用无限循环，周期性处理通信工作。 */
    for (;;) {
        /* 通过通知或延时让出 CPU。 */
        (void)MRT_TaskDelay(2u);
    }
}

/**
 * @brief DSP 周期定时器回调。
 * @param timer 到期定时器句柄。
 * @param arg 用户参数，当前示例不使用。
 * @return void 无返回值。
 * @example
 * DspTickTimerCallback(timer, 0);
 */
static void DspTickTimerCallback(MRT_TimerHandle timer, void *arg)
{
    /* 参数仅用于展示回调签名。 */
    (void)timer;
    (void)arg;

    /* 记录回调次数。 */
    g_tick_timer_count++;
}

/**
 * @brief 任务意外返回时的兜底处理函数。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * DspTaskExit();
 */
static void DspTaskExit(void)
{
    /* 任务函数不应返回；若返回则停机等待调试。 */
    for (;;) {
        /* 保持故障现场。 */
    }
}

/**
 * @brief 初始化 DSP smoke model 的基本对象。
 * @param void 无输入参数。
 * @return MRT_Result 返回初始化结果。
 * @example
 * MRT_Result result = DspInitializeSmokeObjects();
 */
static MRT_Result DspInitializeSmokeObjects(void)
{
    /* 初始化内核。 */
    MRT_KernelInitialize();

    /* 初始化全局堆。 */
    if (MRT_HeapInitialize(g_heap, sizeof(g_heap), MRT_HEAP_MODE_COALESCING) != MRT_RESULT_OK) {
        /* heap 初始化失败。 */
        return MRT_RESULT_INTERNAL_ERROR;
    }

    /* 初始化采样队列。 */
    if (MRT_QueueCreateStatic(DSP_QUEUE_ITEMS,
                              sizeof(uint16_t),
                              g_sample_queue_buffer,
                              &g_sample_queue_storage,
                              &g_sample_queue) != MRT_RESULT_OK) {
        /* 队列初始化失败。 */
        return MRT_RESULT_INTERNAL_ERROR;
    }

    /* 初始化采样任务。 */
    if (MRT_TaskCreateStatic("sample",
                             DspSampleTask,
                             0,
                             4u,
                             g_sample_task_stack,
                             DSP_STACK_WORDS,
                             &g_sample_task_storage,
                             &g_sample_task) != MRT_RESULT_OK) {
        /* 任务初始化失败。 */
        return MRT_RESULT_INTERNAL_ERROR;
    }

    /* 初始化通信任务。 */
    if (MRT_TaskCreateStatic("comm",
                             DspCommTask,
                             0,
                             3u,
                             g_comm_task_stack,
                             DSP_STACK_WORDS,
                             &g_comm_task_storage,
                             &g_comm_task) != MRT_RESULT_OK) {
        /* 任务初始化失败。 */
        return MRT_RESULT_INTERNAL_ERROR;
    }

    /* 初始化周期定时器。 */
    if (MRT_TimerCreateStatic("tick",
                              DSP_TIMER_PERIOD_TICKS,
                              true,
                              0,
                              DspTickTimerCallback,
                              &g_tick_timer_storage,
                              &g_tick_timer) != MRT_RESULT_OK) {
        /* 定时器初始化失败。 */
        return MRT_RESULT_INTERNAL_ERROR;
    }

    /* 启动周期定时器。 */
    if (MRT_TimerStart(g_tick_timer, 0u) != MRT_RESULT_OK) {
        /* 定时器启动失败。 */
        return MRT_RESULT_INTERNAL_ERROR;
    }

    /* 立即运行 pending 命令，让定时器进入活动状态。 */
    MRT_TimerServiceRunPending();

    /* 让内核至少推进一次 tick，证明 tick/timer 语义连通。 */
    MRT_KernelTick();

    /* 读取高水位，确认任务对象可查询。 */
    size_t sample_stack_words = 0u;
    if (MRT_TaskGetStackHighWaterMark(g_sample_task, &sample_stack_words) != MRT_RESULT_OK) {
        /* 栈水位查询失败。 */
        return MRT_RESULT_INTERNAL_ERROR;
    }

    /* 高水位不应为空。 */
    if (sample_stack_words == 0u) {
        /* smoke 中的任务栈必须至少能被识别。 */
        return MRT_RESULT_INTERNAL_ERROR;
    }

    /* 任务模型初始化成功。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 构造并验证 DSP 栈帧 helper。
 * @param void 无输入参数。
 * @return MRT_Result 返回验证结果。
 * @example
 * MRT_Result result = DspValidateStackHelper();
 */
static MRT_Result DspValidateStackHelper(void)
{
    /* 为 helper 准备一个独立栈。 */
    MRT_StackType stack[DSP_STACK_WORDS];

    /* 接收初始化后的栈顶。 */
    MRT_StackType *stack_top = 0;

    /* 把 helper 里的栈帧写出来。 */
    MRT_Result result = MRT_PortDspC28xInitializeStack(stack,
                                                        DSP_STACK_WORDS,
                                                        DspSampleTask,
                                                        0,
                                                        DspTaskExit,
                                                        &stack_top);
    if (result != MRT_RESULT_OK) {
        /* 栈帧初始化失败。 */
        return result;
    }

    /* 栈顶不能为空。 */
    if (stack_top == 0) {
        /* 栈顶输出失败。 */
        return MRT_RESULT_INTERNAL_ERROR;
    }

    /* helper 验证成功。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 运行一次 ISR 模型，验证队列 FromISR 接线。
 * @param void 无输入参数。
 * @return MRT_Result 返回验证结果。
 * @example
 * MRT_Result result = DspExerciseIsrPath();
 */
static MRT_Result DspExerciseIsrPath(void)
{
    /* 先把模型切到 ISR。 */
    MRT_Result result = MRT_PortDspModelEnterIsr();
    if (result != MRT_RESULT_OK) {
        /* 进入 ISR 失败。 */
        return result;
    }

    /* 准备一个采样值。 */
    uint16_t sample = 0x1234u;

    /* 记录是否应在退出前切换。 */
    bool should_yield = false;

    /* 通过 ISR 安全接口把采样送入队列。 */
    result = MRT_QueueSendFromISR(g_sample_queue, &sample, &should_yield);
    if (result != MRT_RESULT_OK) {
        /* 队列发送失败。 */
        return result;
    }

    /* smoke model 主动请求一次延迟切换，验证 DSP 软件中断路径。 */
    MRT_PortYieldFromISR(true);

    /* 退出 ISR。 */
    result = MRT_PortDspModelExitIsr(&should_yield);
    if (result != MRT_RESULT_OK) {
        /* 退出 ISR 失败。 */
        return result;
    }

    /* smoke model 里至少要把请求记为 true 或者保持可观测。 */
    if (MRT_PortDspModelWasYieldRequested() == false) {
        /* 没有记录到切换请求。 */
        return MRT_RESULT_INTERNAL_ERROR;
    }

    /* 退出后把切换请求确认掉。 */
    MRT_PortDspModelAcknowledgeYield();

    /* ISR 路径验证成功。 */
    return MRT_RESULT_OK;
}

/**
 * @brief DSP smoke model 主函数。
 * @param void 无输入参数。
 * @return int 返回 0 表示 smoke 验证通过；错误时返回非 0。
 * @example
 * int rc = main();
 */
int main(void)
{
    /* 先初始化端口模型。 */
    MRT_PortInitialize();

    /* 再初始化 DSP helper 的上下文模型。 */
    MRT_PortDspC28xContextModelReset();

    /* 验证 helper 栈帧契约。 */
    if (DspValidateStackHelper() != MRT_RESULT_OK) {
        /* helper 栈帧验证失败。 */
        return 1;
    }

    /* 创建并检查对象。 */
    if (DspInitializeSmokeObjects() != MRT_RESULT_OK) {
        /* 对象初始化失败。 */
        return 2;
    }

    /* 运行一次 ISR 模型，验证队列 FromISR 接线。 */
    if (DspExerciseIsrPath() != MRT_RESULT_OK) {
        /* ISR 验证失败。 */
        return 3;
    }

    /* 请求一次软件中断式切换，并确认挂起状态可观测。 */
    if (MRT_PortDspC28xRequestContextSwitch() != MRT_RESULT_OK) {
        /* 请求切换失败。 */
        return 4;
    }
    if (!MRT_PortDspC28xIsContextSwitchPending()) {
        /* 挂起状态不应为空。 */
        return 5;
    }

    /* 确认一次挂起请求已经被服务。 */
    if (MRT_PortDspC28xAcknowledgeContextSwitch() != MRT_RESULT_OK) {
        /* 确认失败。 */
        return 6;
    }

    /* 检查模型态睡眠路径。 */
    MRT_Tick slept = 0u;
    if (MRT_PortSuppressTicksAndSleep(4u, &slept) != MRT_RESULT_OK) {
        /* 睡眠路径失败。 */
        return 7;
    }

    /* 睡眠回报值应与请求相同。 */
    if ((slept != 4u) || (MRT_PortDspModelGetLastSleptTicks() != 4u)) {
        /* 睡眠结果不一致。 */
        return 8;
    }

    /* 让内核和定时器再走一步，确保对象路径没有被破坏。 */
    MRT_KernelTick();
    MRT_TimerServiceRunPending();

    /* 最终返回成功。 */
    return 0;
}
