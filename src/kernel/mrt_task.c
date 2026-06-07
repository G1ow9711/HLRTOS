#include "myrtos/mrt_task.h"
#include "myrtos/mrt_priority.h"
#include "mrt_task_internal.h"

/** @brief 每个优先级一个 ready list。 */
static MRT_List g_ready_lists[MRT_CFG_MAX_PRIORITIES];

/** @brief 记录哪些优先级存在 ready 任务。 */
static MRT_PriorityBitmap g_ready_bitmap;

/** @brief 当前正在运行的任务；调度器未启动时为空。 */
static MRT_Task *g_current_task;

/** @brief 任务调度器内部结构是否已经初始化。 */
static bool g_task_kernel_initialized;

/**
 * @brief 将任务加入指定优先级 ready list。
 * @param task 待加入 ready list 的任务指针，不能为空。
 * @return void 无返回值。
 * @example
 * MRT_TaskAddReady(task);
 */
static void MRT_TaskAddReady(MRT_Task *task)
{
    /* 将任务状态设置为 ready，表示任务可被调度器选择。 */
    task->state = MRT_TASK_STATE_READY;

    /* 使用任务当前优先级对应的 ready list 保存任务节点。 */
    MRT_ListInsertTail(&g_ready_lists[task->priority], &task->state_node);

    /* 标记该优先级存在至少一个 ready 任务。 */
    MRT_PriorityBitmapSet(&g_ready_bitmap, task->priority);
}

/**
 * @brief 初始化任务调度器内部状态。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * MRT_TaskKernelInitialize();
 */
void MRT_TaskKernelInitialize(void)
{
    /* 从优先级 0 开始初始化每个 ready list。 */
    for (MRT_Priority priority = 0u; priority < MRT_CFG_MAX_PRIORITIES; priority++) {
        /* 初始化当前优先级的 ready list 哨兵节点和计数。 */
        MRT_ListInitialize(&g_ready_lists[priority]);
    }

    /* 清空 ready priority bitmap。 */
    MRT_PriorityBitmapInitialize(&g_ready_bitmap);

    /* 当前任务清空，表示调度器尚未选择任何任务。 */
    g_current_task = 0;

    /* 标记任务调度器内部状态已经初始化。 */
    g_task_kernel_initialized = true;
}

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
                                MRT_TaskHandle *out_task)
{
    /* 任务入口函数不能为空，否则调度后无法执行任务。 */
    if (entry == 0) {
        /* 返回参数错误，调用方需要提供合法入口。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 栈指针不能为空，否则端口层无法构造任务栈帧。 */
    if (stack == 0) {
        /* 返回参数错误，调用方需要提供栈存储。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 栈长度不能为 0，否则任务没有可用栈空间。 */
    if (stack_words == 0u) {
        /* 返回参数错误，调用方需要提供非零栈长度。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 静态任务控制块存储不能为空。 */
    if (storage == 0) {
        /* 返回参数错误，调用方需要提供 TCB 存储。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 优先级必须位于配置范围内。 */
    if (priority >= MRT_CFG_MAX_PRIORITIES) {
        /* 返回参数错误，调用方需要降低优先级或调整配置。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 如果用户未显式初始化内核，则先初始化任务调度器内部结构。 */
    if (!g_task_kernel_initialized) {
        /* 初始化 ready lists 和位图，保证后续入队安全。 */
        MRT_TaskKernelInitialize();
    }

    /* 保存任务名称指针。 */
    storage->name = name;

    /* 保存任务入口函数。 */
    storage->entry = entry;

    /* 保存任务入口参数。 */
    storage->arg = arg;

    /* 保存任务基础优先级。 */
    storage->base_priority = priority;

    /* 保存任务当前有效优先级。 */
    storage->priority = priority;

    /* 保存任务栈起始地址。 */
    storage->stack = stack;

    /* 保存任务栈长度。 */
    storage->stack_words = stack_words;

    /* 初始化任务链表节点，item 指回任务控制块。 */
    MRT_ListNodeInitialize(&storage->state_node, storage, 0u);

    /* 新创建任务尚未阻塞，唤醒 tick 清零。 */
    storage->wake_tick = 0u;

    /* 标记该任务使用静态存储。 */
    storage->static_storage = true;

    /* 将任务加入 ready list，等待调度器选择。 */
    MRT_TaskAddReady(storage);

    /* 如果调用方需要输出句柄，则写入任务控制块地址。 */
    if (out_task != 0) {
        /* 输出任务句柄给调用方。 */
        *out_task = storage;
    }

    /* 静态任务创建成功。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 获取当前正在运行的任务句柄。
 * @param void 无输入参数。
 * @return MRT_TaskHandle 返回当前任务句柄；调度器尚未启动时返回空指针。
 * @example
 * MRT_TaskHandle current = MRT_TaskGetCurrent();
 */
MRT_TaskHandle MRT_TaskGetCurrent(void)
{
    /* 返回当前任务指针，未启动时为空。 */
    return g_current_task;
}

/**
 * @brief 查询任务当前状态。
 * @param task 待查询任务句柄，不能为空。
 * @param out_state 输出任务状态，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示查询成功；参数非法时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * MRT_TaskState state;
 * MRT_TaskGetState(task, &state);
 */
MRT_Result MRT_TaskGetState(MRT_TaskHandle task, MRT_TaskState *out_state)
{
    /* 任务句柄不能为空。 */
    if (task == 0) {
        /* 返回参数错误，调用方需要提供合法任务句柄。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 输出指针不能为空。 */
    if (out_state == 0) {
        /* 返回参数错误，调用方需要提供输出存储。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 写入任务当前状态。 */
    *out_state = task->state;

    /* 查询成功。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 查询任务当前有效优先级。
 * @param task 待查询任务句柄，不能为空。
 * @param out_priority 输出任务优先级，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示查询成功；参数非法时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * MRT_Priority priority;
 * MRT_TaskGetPriority(task, &priority);
 */
MRT_Result MRT_TaskGetPriority(MRT_TaskHandle task, MRT_Priority *out_priority)
{
    /* 任务句柄不能为空。 */
    if (task == 0) {
        /* 返回参数错误，调用方需要提供合法任务句柄。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 输出指针不能为空。 */
    if (out_priority == 0) {
        /* 返回参数错误，调用方需要提供输出存储。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 写入任务当前有效优先级。 */
    *out_priority = task->priority;

    /* 查询成功。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 获取任务名称。
 * @param task 待查询任务句柄，不能为空。
 * @return const char* 返回任务名称指针；任务句柄为空时返回空指针。
 * @example
 * const char *name = MRT_TaskGetName(task);
 */
const char *MRT_TaskGetName(MRT_TaskHandle task)
{
    /* 空任务句柄没有名称，返回空指针。 */
    if (task == 0) {
        /* 返回空指针给调用方。 */
        return 0;
    }

    /* 返回任务控制块中保存的名称指针。 */
    return task->name;
}
