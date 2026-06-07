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
    /** @brief 阻塞延时到期 tick；ready 状态下该字段无效。 */
    MRT_Tick wake_tick;
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

#endif
