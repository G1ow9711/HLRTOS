#ifndef MYRTOS_MRT_CONFIG_H
#define MYRTOS_MRT_CONFIG_H

/**
 * @file mrt_config.h
 * @brief MyRTOS 默认配置项。
 *
 * 本文件提供内核可裁剪能力的默认值。用户工程可以在编译选项或
 * 工程级配置头中预定义同名宏，从而覆盖这里的默认配置。
 */

/**
 * @brief 最大任务优先级数量。
 *
 * 默认 32 个优先级，便于使用 32 位位图快速查找最高就绪优先级。
 */
#ifndef MRT_CFG_MAX_PRIORITIES
#define MRT_CFG_MAX_PRIORITIES 32u
#endif

/**
 * @brief 系统 tick 频率，单位 Hz。
 *
 * 默认 1000 Hz，表示 1 ms 一个系统节拍，适合常见 STM32 示例和 host 仿真。
 */
#ifndef MRT_CFG_TICK_RATE_HZ
#define MRT_CFG_TICK_RATE_HZ 1000u
#endif

/**
 * @brief 最小任务栈长度，单位为 MRT_StackType。
 *
 * 默认 128 个栈元素，后续端口层会按 CPU ABI 和示例任务需求继续校准。
 */
#ifndef MRT_CFG_MINIMAL_STACK_WORDS
#define MRT_CFG_MINIMAL_STACK_WORDS 128u
#endif

/**
 * @brief 是否启用抢占式调度。
 *
 * 1 表示高优先级任务就绪后可以抢占当前任务；0 表示仅在主动让出或阻塞时切换。
 */
#ifndef MRT_CFG_USE_PREEMPTION
#define MRT_CFG_USE_PREEMPTION 1u
#endif

/**
 * @brief 是否启用同优先级时间片轮转。
 *
 * 1 表示同优先级任务按 tick 轮转；0 表示同优先级任务运行到阻塞或主动让出。
 */
#ifndef MRT_CFG_USE_TIME_SLICING
#define MRT_CFG_USE_TIME_SLICING 1u
#endif

/**
 * @brief 是否支持静态对象创建。
 *
 * 1 表示允许用户提供 TCB、栈、队列缓冲等内存，适合强确定性嵌入式项目。
 */
#ifndef MRT_CFG_SUPPORT_STATIC_ALLOCATION
#define MRT_CFG_SUPPORT_STATIC_ALLOCATION 1u
#endif

/**
 * @brief 是否支持动态对象创建。
 *
 * 1 表示允许内核从 MyRTOS heap 中分配对象；0 表示只允许静态创建。
 */
#ifndef MRT_CFG_SUPPORT_DYNAMIC_ALLOCATION
#define MRT_CFG_SUPPORT_DYNAMIC_ALLOCATION 1u
#endif

/**
 * @brief 堆分配返回地址的最小对齐字节数。
 *
 * 默认 8 字节对齐，可覆盖常见 32/64 位 host、STM32 FPU 数据和 DSP 双字访问需求。
 * 该值必须保持为 2 的幂，堆实现会用它执行地址和大小规整。
 */
#ifndef MRT_CFG_HEAP_ALIGNMENT
#define MRT_CFG_HEAP_ALIGNMENT 8u
#endif

/**
 * @brief 是否启用 trace hook。
 *
 * 默认关闭，避免最小系统产生额外代码和运行开销。
 */
#ifndef MRT_CFG_USE_TRACE
#define MRT_CFG_USE_TRACE 0u
#endif

/**
 * @brief 是否启用 tickless idle。
 *
 * 默认开启该能力，具体是否进入低功耗由端口层和空闲任务共同决定。
 */
#ifndef MRT_CFG_USE_TICKLESS_IDLE
#define MRT_CFG_USE_TICKLESS_IDLE 1u
#endif

/**
 * @brief 软件定时器 pending function 队列长度。
 *
 * pending function 用于把轻量延后执行函数投递到定时器服务路径中。
 * 默认 8 项适合小型 STM32/DSP 工程；需要更高突发能力时可由用户工程覆盖。
 */
#ifndef MRT_CFG_TIMER_PENDING_FUNCTION_QUEUE_LENGTH
#define MRT_CFG_TIMER_PENDING_FUNCTION_QUEUE_LENGTH 8u
#endif

/**
 * @brief 软件定时器服务命令队列长度。
 *
 * 定时器启动、停止、复位、改周期、到期回调和 pending function 都通过该队列交给
 * 定时器服务任务顺序处理。默认沿用 pending function 队列长度，便于保持小系统配置简单。
 */
#ifndef MRT_CFG_TIMER_COMMAND_QUEUE_LENGTH
#define MRT_CFG_TIMER_COMMAND_QUEUE_LENGTH MRT_CFG_TIMER_PENDING_FUNCTION_QUEUE_LENGTH
#endif

#endif
