#ifndef MYRTOS_MRT_TASK_H
#define MYRTOS_MRT_TASK_H

/**
 * @file mrt_task.h
 * @brief MyRTOS 任务管理公共接口。
 *
 * 本文件定义任务入口函数、任务状态、任务控制块和静态任务创建接口。
 * 任务调度器后续会在此基础上扩展启动、延时、优先级调整和任务删除。
 */

#include "myrtos/mrt_config.h"
#include "myrtos/mrt_list.h"
#include "myrtos/mrt_types.h"

/**
 * @brief 任务入口函数类型。
 * @param arg 创建任务时传入的用户参数。
 * @return void 任务函数不返回；若返回，端口层或调度器后续会按任务退出处理。
 * @example
 * static void LedTask(void *arg) { (void)arg; for (;;) { } }
 */
typedef void (*MRT_TaskEntry)(void *arg);

/**
 * @brief 任务运行状态。
 *
 * 调度器和调试接口使用该枚举描述任务当前所在状态。
 */
typedef enum MRT_TaskState {
    /** @brief 任务已经就绪，位于某个 ready list 中。 */
    MRT_TASK_STATE_READY = 0,
    /** @brief 任务正在运行，是当前任务。 */
    MRT_TASK_STATE_RUNNING,
    /** @brief 任务因延时或等待对象而阻塞。 */
    MRT_TASK_STATE_BLOCKED,
    /** @brief 任务被显式挂起。 */
    MRT_TASK_STATE_SUSPENDED,
    /** @brief 任务已删除或尚未初始化。 */
    MRT_TASK_STATE_DELETED
} MRT_TaskState;

/**
 * @brief 任务当前等待原因。
 *
 * 内核在调试、对象等待和超时唤醒时使用该枚举记录任务为何进入 blocked 状态。
 */
typedef enum MRT_TaskWaitReason {
    /** @brief 任务没有等待任何内核对象。 */
    MRT_TASK_WAIT_REASON_NONE = 0,
    /** @brief 任务正在执行纯 tick 延时。 */
    MRT_TASK_WAIT_REASON_DELAY,
    /** @brief 任务正在等待队列出现可接收数据。 */
    MRT_TASK_WAIT_REASON_QUEUE_RECEIVE,
    /** @brief 任务正在等待队列出现可发送空间。 */
    MRT_TASK_WAIT_REASON_QUEUE_SEND,
    /** @brief 任务正在等待信号量出现可获取计数。 */
    MRT_TASK_WAIT_REASON_SEMAPHORE_TAKE,
    /** @brief 任务正在等待互斥锁解锁。 */
    MRT_TASK_WAIT_REASON_MUTEX_LOCK,
    /** @brief 任务正在等待事件组 bit 条件满足。 */
    MRT_TASK_WAIT_REASON_EVENT_BITS,
    /** @brief 任务正在等待本任务通知到达。 */
    MRT_TASK_WAIT_REASON_NOTIFY_WAIT
} MRT_TaskWaitReason;

/**
 * @brief 任务通知写入动作。
 *
 * 任务通知把每个任务内建的一个 32 位值当作轻量同步对象。发送方通过该枚举
 * 指定如何合并新值和旧值。
 */
typedef enum MRT_NotifyAction {
    /** @brief 将 value 中为 1 的 bit 设置到任务通知值中。 */
    MRT_NOTIFY_SET_BITS = 0,
    /** @brief 将任务通知值加 1，value 参数被忽略。 */
    MRT_NOTIFY_INCREMENT,
    /** @brief 不管旧状态如何，直接用 value 覆盖任务通知值。 */
    MRT_NOTIFY_OVERWRITE,
    /** @brief 仅当任务没有 pending 通知时写入 value；已有 pending 时返回忙。 */
    MRT_NOTIFY_NO_OVERWRITE
} MRT_NotifyAction;

/**
 * @brief MyRTOS 任务控制块。
 *
 * 静态创建任务时，用户提供该结构体存储任务元数据。
 * 结构体公开是为了支持无动态分配的嵌入式工程；应用代码不应直接修改字段。
 */
typedef struct MRT_Task {
    /** @brief 任务名称，仅用于调试和手册示例，不参与调度。 */
    const char *name;
    /** @brief 任务入口函数。 */
    MRT_TaskEntry entry;
    /** @brief 传递给任务入口函数的用户参数。 */
    void *arg;
    /** @brief 任务基础优先级，互斥锁优先级继承会使用该值恢复优先级。 */
    MRT_Priority base_priority;
    /** @brief 任务当前有效优先级。 */
    MRT_Priority priority;
    /** @brief 用户提供的任务栈存储区。 */
    MRT_StackType *stack;
    /** @brief 任务栈元素数量，单位为 MRT_StackType。 */
    size_t stack_words;
    /** @brief 当前任务状态。 */
    MRT_TaskState state;
    /** @brief 任务进入 ready/delay 等链表时使用的节点。 */
    MRT_ListNode state_node;
    /** @brief 任务等待队列、信号量等同步对象时使用的对象等待链表节点。 */
    MRT_ListNode wait_node;
    /** @brief 阻塞延时到期 tick；ready 状态下该字段无效。 */
    MRT_Tick wake_tick;
    /** @brief 任务进入 blocked 状态的原因，用于超时清理和调试查询。 */
    MRT_TaskWaitReason wait_reason;
    /** @brief 任务从对象等待中恢复时传递给等待 API 的结果。 */
    MRT_Result wait_result;
    /** @brief 事件组等待时请求的 bit 掩码。 */
    MRT_EventBits event_wait_bits;
    /** @brief 事件组等待被满足时匹配到的 bit。 */
    MRT_EventBits event_matched_bits;
    /** @brief 事件组等待是否要求全部请求 bit 均满足。 */
    bool event_wait_all;
    /** @brief 事件组等待成功退出时是否清除匹配 bit。 */
    bool event_clear_on_exit;
    /** @brief 任务通知值，用于轻量事件、计数或 bit 标志。 */
    MRT_NotifyValue notify_value;
    /** @brief 是否存在尚未被任务读取的通知。 */
    bool notify_pending;
    /** @brief 是否使用静态存储创建。 */
    bool static_storage;
} MRT_Task;

/**
 * @brief 使用调用方提供的 TCB 和栈静态创建任务。
 * @param name 任务名称，允许为空，仅用于调试显示。
 * @param entry 任务入口函数，不能为空。
 * @param arg 传递给任务入口函数的用户参数。
 * @param priority 任务优先级，必须小于 MRT_CFG_MAX_PRIORITIES，数值越大优先级越高。
 * @param stack 调用方提供的任务栈缓冲区，不能为空。
 * @param stack_words 任务栈长度，单位为 MRT_StackType，必须大于 0。
 * @param storage 调用方提供的任务控制块存储，不能为空。
 * @param out_task 输出任务句柄，允许为空；非空时创建成功后写入句柄。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示创建成功；参数非法时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * static MRT_Task led_tcb;
 * static MRT_StackType led_stack[256];
 * MRT_TaskHandle led;
 * MRT_TaskCreateStatic("led", LedTask, NULL, 3, led_stack, 256, &led_tcb, &led);
 */
MRT_Result MRT_TaskCreateStatic(const char *name,
                                MRT_TaskEntry entry,
                                void *arg,
                                MRT_Priority priority,
                                MRT_StackType *stack,
                                size_t stack_words,
                                MRT_Task *storage,
                                MRT_TaskHandle *out_task);

/**
 * @brief 让当前任务阻塞指定 tick 数。
 * @param ticks 需要延时的 tick 数；为 0 时等价于主动让出 CPU。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示延时成功；调度器未运行或无当前任务时返回 MRT_RESULT_INVALID_CONTEXT。
 * @example
 * MRT_TaskDelay(10);
 */
MRT_Result MRT_TaskDelay(MRT_Tick ticks);

/**
 * @brief 获取当前正在运行的任务句柄。
 * @param void 无输入参数。
 * @return MRT_TaskHandle 返回当前任务句柄；调度器尚未启动时返回空指针。
 * @example
 * MRT_TaskHandle current = MRT_TaskGetCurrent();
 */
MRT_TaskHandle MRT_TaskGetCurrent(void);

/**
 * @brief 查询任务当前状态。
 * @param task 待查询任务句柄，不能为空。
 * @param out_state 输出任务状态，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示查询成功；参数非法时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * MRT_TaskState state;
 * MRT_TaskGetState(task, &state);
 */
MRT_Result MRT_TaskGetState(MRT_TaskHandle task, MRT_TaskState *out_state);

/**
 * @brief 查询任务当前有效优先级。
 * @param task 待查询任务句柄，不能为空。
 * @param out_priority 输出任务优先级，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示查询成功；参数非法时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * MRT_Priority priority;
 * MRT_TaskGetPriority(task, &priority);
 */
MRT_Result MRT_TaskGetPriority(MRT_TaskHandle task, MRT_Priority *out_priority);

/**
 * @brief 获取任务名称。
 * @param task 待查询任务句柄，不能为空。
 * @return const char* 返回任务名称指针；任务句柄为空时返回空指针。
 * @example
 * const char *name = MRT_TaskGetName(task);
 */
const char *MRT_TaskGetName(MRT_TaskHandle task);

/**
 * @brief 向指定任务发送通知。
 * @param task 目标任务句柄，不能为空。
 * @param value 通知值，具体含义由 action 决定。
 * @param action 通知写入动作，必须是 MRT_NotifyAction 中的有效枚举值。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示通知写入成功；no-overwrite 遇到 pending 通知时返回
 *         MRT_RESULT_OBJECT_BUSY；参数非法返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * MRT_TaskNotify(worker, 0x01u, MRT_NOTIFY_SET_BITS);
 */
MRT_Result MRT_TaskNotify(MRT_TaskHandle task, MRT_NotifyValue value, MRT_NotifyAction action);

/**
 * @brief 等待当前任务收到通知并读取通知值。
 * @param clear_on_entry 进入等待前需要清除的通知值 bit 掩码。
 * @param clear_on_exit 成功读取后需要清除的通知值 bit 掩码。
 * @param timeout 等待通知到达的 tick 数；为 0 时只检查一次并立即返回。
 * @param out_value 输出读取到的通知值，允许为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示读取到通知；非阻塞无通知返回 MRT_RESULT_OBJECT_EMPTY；
 *         无当前任务返回 MRT_RESULT_INVALID_CONTEXT；等待未完成返回 MRT_RESULT_TIMEOUT。
 * @example
 * MRT_NotifyValue value;
 * MRT_TaskNotifyWait(0, 0xffffffffu, 10u, &value);
 */
MRT_Result MRT_TaskNotifyWait(MRT_NotifyValue clear_on_entry,
                              MRT_NotifyValue clear_on_exit,
                              MRT_Timeout timeout,
                              MRT_NotifyValue *out_value);

/**
 * @brief 以计数信号量方式等待并获取当前任务通知值。
 * @param clear_count_on_exit true 表示成功获取后把通知计数清零；false 表示只递减 1。
 * @param timeout 等待通知计数非 0 的 tick 数；为 0 时只检查一次并立即返回。
 * @param out_count 输出获取前的通知计数，允许为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示获取到计数；非阻塞无计数返回 MRT_RESULT_OBJECT_EMPTY；
 *         无当前任务返回 MRT_RESULT_INVALID_CONTEXT；等待未完成返回 MRT_RESULT_TIMEOUT。
 * @example
 * MRT_NotifyValue count;
 * MRT_TaskNotifyTake(true, 10u, &count);
 */
MRT_Result MRT_TaskNotifyTake(bool clear_count_on_exit, MRT_Timeout timeout, MRT_NotifyValue *out_count);

/**
 * @brief 清除指定任务的 pending 通知状态。
 * @param task 目标任务句柄，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示清除成功；参数非法返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * MRT_TaskNotifyStateClear(worker);
 */
MRT_Result MRT_TaskNotifyStateClear(MRT_TaskHandle task);

/**
 * @brief 清除指定任务通知值中的 bit。
 * @param task 目标任务句柄，不能为空。
 * @param bits_to_clear 需要清除的 bit 掩码；为 0 时不改变通知值。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示清除成功；参数非法返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * MRT_TaskNotifyValueClear(worker, 0x01u);
 */
MRT_Result MRT_TaskNotifyValueClear(MRT_TaskHandle task, MRT_NotifyValue bits_to_clear);

#endif
