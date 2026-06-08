#include "myrtos/mrt_heap.h"
#include "myrtos/mrt_kernel.h"
#include "myrtos/mrt_port.h"
#include "myrtos/mrt_queue.h"
#include "myrtos/mrt_task.h"
#include "myrtos/mrt_timer.h"
#include "myrtos/portable/mrt_port_stm32_cm.h"
#include "mrt_task_internal.h"

#include <stdbool.h>
#include <stdint.h>

extern uint32_t MRT_PortStm32SmokeGetLastReload(void);
extern uint32_t MRT_PortStm32SmokeGetLastBasepri(void);
extern void MRT_PortStm32SmokeRecordTiming(uint32_t reload, uint32_t basepri);

#define SMOKE_HEAP_BYTES 4096u
#define SMOKE_QUEUE_ITEMS 8u
#define SMOKE_STACK_WORDS 128u
#define SMOKE_TIMER_PERIOD_TICKS 500u
#define SMOKE_CORE_CLOCK_HZ 168000000u

/** @brief 烟雾测试 heap。 */
static uint8_t g_heap[SMOKE_HEAP_BYTES];

/** @brief 烟雾测试 UART 队列控制块。 */
static MRT_Queue g_uart_queue_storage;

/** @brief 烟雾测试 UART 队列缓冲。 */
static uint8_t g_uart_queue_buffer[SMOKE_QUEUE_ITEMS * sizeof(uint8_t)];

/** @brief 烟雾测试 UART 队列句柄。 */
static MRT_QueueHandle g_uart_queue;

/** @brief 烟雾测试 LED 定时器控制块。 */
static MRT_Timer g_led_timer_storage;

/** @brief 烟雾测试 LED 定时器句柄。 */
static MRT_TimerHandle g_led_timer;

/** @brief 烟雾测试 LED 任务控制块。 */
static MRT_Task g_led_task_storage;

/** @brief 烟雾测试 LED 任务栈。 */
static MRT_StackType g_led_task_stack[SMOKE_STACK_WORDS];

/** @brief 烟雾测试 LED 任务句柄。 */
static MRT_TaskHandle g_led_task;

/** @brief 烟雾测试 UART 任务控制块。 */
static MRT_Task g_uart_task_storage;

/** @brief 烟雾测试 UART 任务栈。 */
static MRT_StackType g_uart_task_stack[SMOKE_STACK_WORDS];

/** @brief 烟雾测试 UART 任务句柄。 */
static MRT_TaskHandle g_uart_task;

/** @brief 模拟的 LED0 状态。 */
static volatile bool g_led0_on;

/** @brief 模拟的 LED1 状态。 */
static volatile bool g_led1_on;

/** @brief 模拟的 UART 接收寄存器影子值。 */
static volatile uint8_t g_uart_rx_shadow;

/** @brief 最近一次从 UART ISR 收到的字节。 */
static volatile uint8_t g_last_uart_byte;

/** @brief LED 定时器回调次数。 */
static volatile uint32_t g_led_timer_ticks;

/** @brief UART 任务接收次数。 */
static volatile uint32_t g_uart_task_receives;

/**
 * @brief 切换模拟 LED0 状态。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * SmokeToggleLed0();
 */
static void SmokeToggleLed0(void)
{
    /* 取反当前状态，模拟 GPIO 输出翻转。 */
    g_led0_on = !g_led0_on;
}

/**
 * @brief 切换模拟 LED1 状态。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * SmokeToggleLed1();
 */
static void SmokeToggleLed1(void)
{
    /* 取反当前状态，模拟第二路 GPIO 输出翻转。 */
    g_led1_on = !g_led1_on;
}

/**
 * @brief LED 周期定时器回调。
 * @param timer 到期定时器句柄。
 * @param arg 用户参数，当前示例不使用。
 * @return void 无返回值。
 * @example
 * SmokeLedTimerCallback(timer, 0);
 */
static void SmokeLedTimerCallback(MRT_TimerHandle timer, void *arg)
{
    /* 参数仅用于展示定时器回调签名。 */
    (void)timer;
    (void)arg;

    /* 记录回调进入次数。 */
    g_led_timer_ticks++;

    /* 翻转模拟 LED1，证明定时器回调路径接通。 */
    SmokeToggleLed1();
}

/**
 * @brief LED 任务主体。
 * @param arg 用户参数，当前示例不使用。
 * @return void 无返回值。
 * @example
 * SmokeLedTask(0);
 */
static void SmokeLedTask(void *arg)
{
    /* 参数仅用于展示任务入口签名。 */
    (void)arg;

    /* 任务主体采用无限循环，符合 RTOS 任务约定。 */
    for (;;) {
        /* 翻转模拟 LED0。 */
        SmokeToggleLed0();

        /* 周期性延时，给其他任务和 ISR 留出调度机会。 */
        (void)MRT_TaskDelay(500u);
    }
}

/**
 * @brief UART 任务主体。
 * @param arg 用户参数，当前示例不使用。
 * @return void 无返回值。
 * @example
 * SmokeUartTask(0);
 */
static void SmokeUartTask(void *arg)
{
    /* 参数仅用于展示任务入口签名。 */
    (void)arg;

    /* 任务主体采用无限循环，持续接收 ISR 推入的字节。 */
    for (;;) {
        /* 从队列中取出一个字节，阻塞等待 ISR 投递。 */
        uint8_t byte = 0u;

        /* 这里保留阻塞调用，真实板级时会由 UART RX ISR 唤醒。 */
        MRT_Result result = MRT_QueueReceive(g_uart_queue, &byte, 10u);

        /* 仅在收到字节时更新最近接收值。 */
        if (result == MRT_RESULT_OK) {
            /* 记录最近一次收到的字节。 */
            g_last_uart_byte = byte;

            /* 记录 UART 任务接收次数。 */
            g_uart_task_receives++;
        }
    }
}

/**
 * @brief 任务函数意外返回时的兜底处理入口。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * SmokeTaskExit();
 */
static void SmokeTaskExit(void)
{
    /* RTOS 任务按约定不应从入口函数返回；若返回则停在现场便于调试。 */
    for (;;) {
        /* 保持故障现场，真实板级可在这里点亮错误 LED 或触发断言。 */
    }
}

/**
 * @brief 为 STM32 smoke 任务构造初始异常帧并写入 TCB 运行期栈顶。
 * @param task 已创建的任务句柄，不能为空。
 * @param stack 任务栈首地址，不能为空。
 * @param stack_words 任务栈元素数量，必须足够容纳端口初始帧。
 * @param entry 任务入口函数，不能为空。
 * @param arg 传递给任务入口函数的参数，可为空。
 * @return MRT_Result 成功返回 MRT_RESULT_OK；栈帧构造或 TCB 写入失败时返回对应错误。
 * @example
 * SmokeInitializeTaskStack(task, stack, 128u, SmokeLedTask, 0);
 */
static MRT_Result SmokeInitializeTaskStack(MRT_TaskHandle task,
                                           MRT_StackType *stack,
                                           size_t stack_words,
                                           void (*entry)(void *),
                                           void *arg)
{
    /* 准备接收端口层构造出的初始 PSP。 */
    MRT_StackType *stack_top = 0;

    /* 先让 Cortex-M helper 写入 R4-R11、R0、LR、PC、xPSR 等初始上下文槽位。 */
    MRT_Result result = MRT_PortStm32CmInitializeStack(stack, stack_words, entry, arg, SmokeTaskExit, &stack_top);
    if (result != MRT_RESULT_OK) {
        /* 栈空间、入口函数或对齐不满足端口要求时，直接返回错误。 */
        return result;
    }

    /* 再把初始 PSP 写回任务控制块，供 SVC/PendSV 恢复首任务或后续任务。 */
    return MRT_TaskKernelSetStackTop(task, stack_top);
}

/**
 * @brief 配置烟雾测试的系统时钟参数。
 * @param void 无输入参数。
 * @return MRT_Result 返回计算和记录结果。
 * @example
 * MRT_Result result = SmokeConfigureClock();
 */
static MRT_Result SmokeConfigureClock(void)
{
    /* 保存 SysTick reload 计算结果。 */
    uint32_t reload = 0u;

    /* 保存 BASEPRI 编码结果。 */
    uint32_t basepri = 0u;

    /* 计算 1 kHz tick 的 reload。 */
    MRT_Result result = MRT_PortStm32CmCalculateSysTickReload(SMOKE_CORE_CLOCK_HZ,
                                                               MRT_CFG_TICK_RATE_HZ,
                                                               &reload);
    if (result != MRT_RESULT_OK) {
        /* reload 计算失败时直接返回错误。 */
        return result;
    }

    /* 按 NVIC 位宽计算一个可屏蔽的 BASEPRI 结果。 */
    result = MRT_PortStm32CmEncodeBasepri(4u, 5u, &basepri);
    if (result != MRT_RESULT_OK) {
        /* BASEPRI 编码失败时直接返回错误。 */
        return result;
    }

    /* 把结果记录到端口 smoke 状态中。 */
    MRT_PortStm32SmokeRecordTiming(reload, basepri);

    /* 使用结果不依赖真实寄存器写入，只做接线验证。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 重新装载 UART 影子字节，模拟串口接收器硬件缓冲。
 * @param byte 模拟收到的串口字节。
 * @return void 无返回值。
 * @example
 * SmokePrimeUartRx(0x55u);
 */
static void SmokePrimeUartRx(uint8_t byte)
{
    /* 保存一个字节到模拟 UART 数据寄存器影子。 */
    g_uart_rx_shadow = byte;
}

/**
 * @brief STM32 SysTick 中断入口。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * SysTick_Handler();
 */
void SysTick_Handler(void)
{
    /* 直接推进内核 tick。 */
    MRT_KernelTick();
}

/**
 * @brief STM32 UART 中断入口。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * USART1_IRQHandler();
 */
void USART1_IRQHandler(void)
{
    /* 读取模拟 UART 数据寄存器。 */
    uint8_t byte = g_uart_rx_shadow;

    /* 记录是否需要在 ISR 退出前触发调度切换。 */
    bool should_yield = false;

    /* 把收到的字节送入队列，供 UART 任务处理。 */
    (void)MRT_QueueSendFromISR(g_uart_queue, &byte, &should_yield);

    /* 若高优先级任务被唤醒，则请求延迟切换。 */
    MRT_PortYieldFromISR(should_yield);
}

/**
 * @brief STM32 LED smoke 主函数。
 * @param void 无输入参数。
 * @return int 返回 0 表示 smoke 配置链路完成；错误时返回非 0。
 * @example
 * int rc = main();
 */
int main(void)
{
    /* 先初始化端口层状态。 */
    MRT_PortInitialize();

    /* 再初始化内核全局状态。 */
    MRT_KernelInitialize();

    /* 配置全局堆，供后续动态对象使用。 */
    if (MRT_HeapInitialize(g_heap, sizeof(g_heap), MRT_HEAP_MODE_COALESCING) != MRT_RESULT_OK) {
        /* heap 初始化失败时返回错误。 */
        return 1;
    }

    /* 把时钟和优先级编码参数写入 smoke 记录。 */
    if (SmokeConfigureClock() != MRT_RESULT_OK) {
        /* 时钟参数计算失败时返回错误。 */
        return 2;
    }

    /* 确认 smoke 端口记录到了有效 reload 和 BASEPRI 编码。 */
    if ((MRT_PortStm32SmokeGetLastReload() == 0u) || (MRT_PortStm32SmokeGetLastBasepri() == 0u)) {
        /* 记录值异常时返回错误。 */
        return 3;
    }

    /* 创建静态 UART 队列。 */
    if (MRT_QueueCreateStatic(SMOKE_QUEUE_ITEMS,
                              sizeof(uint8_t),
                              g_uart_queue_buffer,
                              &g_uart_queue_storage,
                              &g_uart_queue) != MRT_RESULT_OK) {
        /* 队列创建失败时返回错误。 */
        return 4;
    }

    /* 创建 LED 周期定时器。 */
    if (MRT_TimerCreateStatic("led",
                              SMOKE_TIMER_PERIOD_TICKS,
                              true,
                              0,
                              SmokeLedTimerCallback,
                              &g_led_timer_storage,
                              &g_led_timer) != MRT_RESULT_OK) {
        /* 定时器创建失败时返回错误。 */
        return 5;
    }

    /* 创建 LED 任务。 */
    if (MRT_TaskCreateStatic("led",
                             SmokeLedTask,
                             0,
                             3u,
                             g_led_task_stack,
                             SMOKE_STACK_WORDS,
                             &g_led_task_storage,
                             &g_led_task) != MRT_RESULT_OK) {
        /* 任务创建失败时返回错误。 */
        return 6;
    }

    /* 为 LED 任务构造 Cortex-M 初始异常帧，并把初始 PSP 写入任务 TCB。 */
    if (SmokeInitializeTaskStack(g_led_task, g_led_task_stack, SMOKE_STACK_WORDS, SmokeLedTask, 0) != MRT_RESULT_OK) {
        /* LED 任务初始栈帧接线失败时返回错误。 */
        return 8;
    }

    /* 创建 UART 任务。 */
    if (MRT_TaskCreateStatic("uart",
                             SmokeUartTask,
                             0,
                             4u,
                             g_uart_task_stack,
                             SMOKE_STACK_WORDS,
                             &g_uart_task_storage,
                             &g_uart_task) != MRT_RESULT_OK) {
        /* 任务创建失败时返回错误。 */
        return 7;
    }

    /* 为 UART 任务构造 Cortex-M 初始异常帧，并把初始 PSP 写入任务 TCB。 */
    if (SmokeInitializeTaskStack(g_uart_task, g_uart_task_stack, SMOKE_STACK_WORDS, SmokeUartTask, 0) != MRT_RESULT_OK) {
        /* UART 任务初始栈帧接线失败时返回错误。 */
        return 9;
    }

    /* 投递定时器启动命令。 */
    if (MRT_TimerStart(g_led_timer, 0u) != MRT_RESULT_OK) {
        /* 定时器启动命令投递失败时返回错误。 */
        return 10;
    }

    /* 让服务路径立即处理启动命令。 */
    MRT_TimerServiceRunPending();

    /* 模拟串口接收路径已经收到了一个字节。 */
    SmokePrimeUartRx(0x55u);

    /* 让调度器进入运行状态。 */
    (void)MRT_KernelStart();

    /* smoke 例程不应回到 main。 */
    for (;;) {
        /* 等待真实板级上下文切换接管 CPU。 */
    }
}
