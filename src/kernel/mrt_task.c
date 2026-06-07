#include "myrtos/mrt_task.h"
#include "myrtos/mrt_kernel.h"
#include "myrtos/mrt_priority.h"
#include "mrt_task_internal.h"

/** @brief 每个优先级一个 ready list。 */
static MRT_List g_ready_lists[MRT_CFG_MAX_PRIORITIES];

/** @brief 记录哪些优先级存在 ready 任务。 */
static MRT_PriorityBitmap g_ready_bitmap;

/** @brief 按唤醒 tick 排序的延时任务链表。 */
static MRT_List g_delayed_list;

/** @brief 当前正在运行的任务；调度器未启动时为空。 */
static MRT_Task *g_current_task;

/** @brief 任务调度器内部结构是否已经初始化。 */
static bool g_task_kernel_initialized;

/**
 * @brief 判断 now 是否已经到达 wake_tick。
 * @param now 当前系统 tick。
 * @param wake_tick 任务计划唤醒 tick。
 * @return bool 返回 true 表示已经到期，返回 false 表示尚未到期。
 * @example
 * if (MRT_TaskTickReached(now, task->wake_tick)) { MRT_TaskAddReady(task); }
 */
static bool MRT_TaskTickReached(MRT_Tick now, MRT_Tick wake_tick)
{
    /* 使用有符号差值处理无符号 tick 回绕。 */
    return (int32_t)(now - wake_tick) >= 0;
}

/**
 * @brief 选择当前最高优先级 ready 任务。
 * @param void 无输入参数。
 * @return MRT_Task* 返回被选中的任务；没有 ready 任务时返回空指针。
 * @example
 * MRT_Task *next = MRT_TaskSelectHighestReady();
 */
static MRT_Task *MRT_TaskSelectHighestReady(void)
{
    /* 定义最高 ready 优先级输出变量。 */
    MRT_Priority highest_priority = 0u;

    /* 查询 ready 位图；如果位图为空，则没有可运行任务。 */
    if (!MRT_PriorityBitmapFindHighest(&g_ready_bitmap, &highest_priority)) {
        /* 没有 ready 任务可选。 */
        return 0;
    }

    /* 获取最高优先级 ready list 的头节点。 */
    MRT_ListNode *head = MRT_ListGetHead(&g_ready_lists[highest_priority]);

    /* 头节点为空说明位图和链表不一致，保守返回空指针。 */
    if (head == 0) {
        /* 没有可选择任务。 */
        return 0;
    }

    /* 从链表节点恢复任务控制块指针。 */
    return (MRT_Task *)head->item;
}

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
 * @brief 从 ready list 移除任务。
 * @param task 待移除任务指针，不能为空。
 * @return void 无返回值。
 * @example
 * MRT_TaskRemoveReady(task);
 */
static void MRT_TaskRemoveReady(MRT_Task *task)
{
    /* 如果任务节点未入链，则无需移除。 */
    if (!MRT_ListNodeIsLinked(&task->state_node)) {
        /* 直接返回调用方。 */
        return;
    }

    /* 保存任务所在优先级，移除后要用它检查 ready list 是否为空。 */
    MRT_Priority priority = task->priority;

    /* 从当前 ready list 中移除任务节点。 */
    MRT_ListRemove(&task->state_node);

    /* 如果该优先级 ready list 已空，则清除位图对应 bit。 */
    if (MRT_ListIsEmpty(&g_ready_lists[priority])) {
        /* 清除该优先级 ready 标记。 */
        MRT_PriorityBitmapClear(&g_ready_bitmap, priority);
    }
}

/**
 * @brief 重新选择最高优先级 ready 任务作为当前任务。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * MRT_TaskSwitchToHighestReady();
 */
static void MRT_TaskSwitchToHighestReady(void)
{
    /* 如果当前任务仍处于 running，切换前先恢复为 ready。 */
    if ((g_current_task != 0) && (g_current_task->state == MRT_TASK_STATE_RUNNING)) {
        /* 当前任务仍在 ready list 中，只是失去运行权。 */
        g_current_task->state = MRT_TASK_STATE_READY;
    }

    /* 选择当前最高优先级 ready 任务。 */
    MRT_Task *next_task = MRT_TaskSelectHighestReady();

    /* 如果没有 ready 任务，则清空当前任务。 */
    if (next_task == 0) {
        /* 当前无任务可运行。 */
        g_current_task = 0;

        /* 返回调用方。 */
        return;
    }

    /* 保存新的当前任务。 */
    g_current_task = next_task;

    /* 将新当前任务标记为 running。 */
    g_current_task->state = MRT_TASK_STATE_RUNNING;
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

    /* 初始化延时任务链表。 */
    MRT_ListInitialize(&g_delayed_list);

    /* 当前任务清空，表示调度器尚未选择任何任务。 */
    g_current_task = 0;

    /* 标记任务调度器内部状态已经初始化。 */
    g_task_kernel_initialized = true;
}

/**
 * @brief 启动任务调度器并选择第一个运行任务。
 * @param void 无输入参数。
 * @return bool 返回 true 表示找到可运行任务，返回 false 表示没有 ready 任务。
 * @example
 * if (MRT_TaskKernelStartScheduler()) { MRT_PortStartFirstTask(); }
 */
bool MRT_TaskKernelStartScheduler(void)
{
    /* 选择当前最高优先级 ready 任务。 */
    MRT_Task *task = MRT_TaskSelectHighestReady();

    /* 没有 ready 任务时不能启动调度器。 */
    if (task == 0) {
        /* 返回 false 告诉内核启动失败。 */
        return false;
    }

    /* 保存当前任务指针。 */
    g_current_task = task;

    /* 将被选中的任务状态标记为 running。 */
    g_current_task->state = MRT_TASK_STATE_RUNNING;

    /* 成功选中第一个运行任务。 */
    return true;
}

/**
 * @brief 当前任务主动 yield 时执行一次调度选择。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * MRT_TaskKernelYield();
 */
void MRT_TaskKernelYield(void)
{
    /* 如果当前任务为空，说明调度器尚未启动，无需调度。 */
    if (g_current_task == 0) {
        /* 没有当前任务，直接返回。 */
        return;
    }

    /* 如果当前任务仍在 ready list 中，则可以参与同优先级轮转。 */
    if (MRT_ListNodeIsLinked(&g_current_task->state_node)) {
        /* 将当前任务状态恢复为 ready，表示它让出运行权。 */
        g_current_task->state = MRT_TASK_STATE_READY;

        /* 从 ready list 当前位置移除当前任务节点。 */
        MRT_ListRemove(&g_current_task->state_node);

        /* 将当前任务节点插入同优先级 ready list 尾部，实现 FIFO 轮转。 */
        MRT_ListInsertTail(&g_ready_lists[g_current_task->priority], &g_current_task->state_node);
    }

    /* 重新选择最高优先级 ready 任务。 */
    MRT_TaskSwitchToHighestReady();
}

/**
 * @brief 处理一个系统 tick 上的延时任务到期。
 * @param now 当前系统 tick。
 * @return void 无返回值。
 * @example
 * MRT_TaskKernelTick(MRT_KernelGetTick());
 */
void MRT_TaskKernelTick(MRT_Tick now)
{
    /* 记录本 tick 是否唤醒过任务。 */
    bool woke_task = false;

    /* 只要延时链表非空，就检查头部最早到期任务。 */
    while (!MRT_ListIsEmpty(&g_delayed_list)) {
        /* 获取延时链表头部节点。 */
        MRT_ListNode *head = MRT_ListGetHead(&g_delayed_list);

        /* 从节点恢复任务控制块。 */
        MRT_Task *task = (MRT_Task *)head->item;

        /* 如果头部任务尚未到期，后续任务也不需要处理。 */
        if (!MRT_TaskTickReached(now, task->wake_tick)) {
            /* 退出循环等待后续 tick。 */
            break;
        }

        /* 从延时链表移除到期任务。 */
        MRT_ListRemove(&task->state_node);

        /* 如果任务同时挂在某个对象等待链表上，说明对象等待超时，需要同步摘除。 */
        if (MRT_ListNodeIsLinked(&task->wait_node)) {
            /* 从队列、信号量等对象等待链表中移除该任务。 */
            MRT_ListRemove(&task->wait_node);
        }

        /* 超时唤醒后任务不再等待具体对象。 */
        task->wait_reason = MRT_TASK_WAIT_REASON_NONE;

        /* 将到期任务重新加入 ready list。 */
        MRT_TaskAddReady(task);

        /* 标记本 tick 唤醒了任务。 */
        woke_task = true;
    }

    /* 如果唤醒了任务，则可能需要抢占当前任务。 */
    if (woke_task) {
        /* 重新选择最高优先级 ready 任务。 */
        MRT_TaskSwitchToHighestReady();
    }
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
/**
 * @brief 将当前任务阻塞到指定内核对象等待链表，并设置 tick 超时。
 * @param wait_list 队列、信号量等对象的等待链表，不能为空。
 * @param ticks 最大等待 tick 数；当前调用方应保证大于 0。
 * @param wait_reason 任务等待原因，用于调试和超时清理。
 * @param wait_result 等待到期时返回给调用方的结果。
 * @return MRT_Result 返回 wait_result 表示当前 host 仿真中的等待结局；参数非法时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * MRT_TaskKernelBlockCurrentOnObject(&queue->waiting_receivers, 3, MRT_TASK_WAIT_REASON_QUEUE_RECEIVE, MRT_RESULT_TIMEOUT);
 */
MRT_Result MRT_TaskKernelBlockCurrentOnObject(MRT_List *wait_list,
                                              MRT_Tick ticks,
                                              MRT_TaskWaitReason wait_reason,
                                              MRT_Result wait_result)
{
    /* 对象等待链表不能为空，否则无法记录任务正在等待哪个对象。 */
    if (wait_list == 0) {
        /* 返回参数错误，提示调用方提供有效等待链表。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 如果当前没有运行任务，host 仿真无法真正挂起调用方，按等待结果返回。 */
    if (g_current_task == 0) {
        /* 保持早期非调度上下文行为：非零 timeout 直接表现为等待结果。 */
        return wait_result;
    }

    /* 保存需要阻塞的当前任务。 */
    MRT_Task *task = g_current_task;

    /* 将当前任务从 ready list 移除，使调度器不再选择它运行。 */
    MRT_TaskRemoveReady(task);

    /* 计算任务等待到期 tick，允许无符号自然回绕。 */
    task->wake_tick = MRT_KernelGetTick() + ticks;

    /* 标记任务进入 blocked 状态。 */
    task->state = MRT_TASK_STATE_BLOCKED;

    /* 保存任务等待原因，供调试和对象清理逻辑使用。 */
    task->wait_reason = wait_reason;

    /* 保存等待结果，host 仿真中立即返回给调用方。 */
    task->wait_result = wait_result;

    /* 使用唤醒 tick 作为 delay list 排序值。 */
    task->state_node.value = task->wake_tick;

    /* 使用反向优先级作为对象等待链表排序值，数值越小代表任务优先级越高。 */
    task->wait_node.value = UINT32_MAX - task->priority;

    /* 如果等待节点意外仍在某个链表中，先摘除，避免重复入链。 */
    if (MRT_ListNodeIsLinked(&task->wait_node)) {
        /* 清理旧等待关系。 */
        MRT_ListRemove(&task->wait_node);
    }

    /* 将任务加入对象等待链表，后续发送/释放对象时可按优先级唤醒。 */
    MRT_ListInsertOrdered(wait_list, &task->wait_node);

    /* 将任务加入 delay list，tick 到期时自动超时唤醒。 */
    MRT_ListInsertOrdered(&g_delayed_list, &task->state_node);

    /* 当前任务已经阻塞，清空当前任务指针。 */
    g_current_task = 0;

    /* 重新选择下一个最高优先级 ready 任务运行。 */
    MRT_TaskSwitchToHighestReady();

    /* 返回等待结果；真实端口后续会在任务恢复时从同一 API 继续返回。 */
    return task->wait_result;
}

/**
 * @brief 唤醒对象等待链表中的第一个任务。
 * @param wait_list 队列、信号量等对象的等待链表，不能为空。
 * @param wait_result 写入被唤醒任务的等待结果。
 * @param switch_now true 表示立即重选当前任务，false 表示只让任务 ready，由 ISR 退出后再切换。
 * @return bool 返回 true 表示成功唤醒了一个任务，返回 false 表示等待链表为空或参数非法。
 * @example
 * bool woke = MRT_TaskKernelWakeFirstObjectWaiter(&queue->waiting_receivers, MRT_RESULT_OK, true);
 */
bool MRT_TaskKernelWakeFirstObjectWaiter(MRT_List *wait_list, MRT_Result wait_result, bool switch_now)
{
    /* 等待链表不能为空。 */
    if (wait_list == 0) {
        /* 参数非法时没有任务可唤醒。 */
        return false;
    }

    /* 空等待链表表示没有任务正在等待该对象。 */
    if (MRT_ListIsEmpty(wait_list)) {
        /* 没有唤醒任何任务。 */
        return false;
    }

    /* 取出等待链表头部任务；链表按反向优先级排序，头部优先级最高。 */
    MRT_ListNode *wait_node = MRT_ListGetHead(wait_list);

    /* 从等待节点恢复任务控制块指针。 */
    MRT_Task *task = (MRT_Task *)wait_node->item;

    /* 从对象等待链表移除该任务。 */
    MRT_ListRemove(&task->wait_node);

    /* 如果该任务还在 delay list 中等待 timeout，需要同步移除。 */
    if (MRT_ListNodeIsLinked(&task->state_node)) {
        /* 移除 timeout 节点，防止后续 tick 再次唤醒同一任务。 */
        MRT_ListRemove(&task->state_node);
    }

    /* 写入对象等待结果。 */
    task->wait_result = wait_result;

    /* 被对象唤醒后不再等待具体对象。 */
    task->wait_reason = MRT_TASK_WAIT_REASON_NONE;

    /* 重新加入 ready list，等待调度器选择。 */
    MRT_TaskAddReady(task);

    /* 任务上下文唤醒时需要立即重选当前任务。 */
    if (switch_now) {
        /* 如果被唤醒任务优先级更高，将立即成为当前任务。 */
        MRT_TaskSwitchToHighestReady();
    }

    /* 已成功唤醒一个等待任务。 */
    return true;
}

/**
 * @brief 唤醒指定任务。
 * @param task 待唤醒任务句柄，不能为空。
 * @param wait_result 写入被唤醒任务的等待结果。
 * @param switch_now true 表示立即重选当前任务，false 表示只让任务 ready。
 * @return bool 返回 true 表示成功唤醒任务，返回 false 表示参数非法。
 * @example
 * MRT_TaskKernelWakeTask(waiter, MRT_RESULT_OK, true);
 */
bool MRT_TaskKernelWakeTask(MRT_TaskHandle task, MRT_Result wait_result, bool switch_now)
{
    /* 任务句柄不能为空。 */
    if (task == 0) {
        /* 没有目标任务时无法唤醒。 */
        return false;
    }

    /* 如果任务还挂在对象等待链表上，需要先摘除。 */
    if (MRT_ListNodeIsLinked(&task->wait_node)) {
        /* 移除队列、信号量、事件组等对象等待关系。 */
        MRT_ListRemove(&task->wait_node);
    }

    /* 如果任务还挂在 delay list 上，需要同步移除 timeout 节点。 */
    if (MRT_ListNodeIsLinked(&task->state_node)) {
        /* 移除延时节点，避免 tick 后续重复唤醒同一任务。 */
        MRT_ListRemove(&task->state_node);
    }

    /* 写入对象等待结果。 */
    task->wait_result = wait_result;

    /* 被唤醒后不再等待具体对象。 */
    task->wait_reason = MRT_TASK_WAIT_REASON_NONE;

    /* 将任务重新加入 ready list。 */
    MRT_TaskAddReady(task);

    /* 如果调用方要求立即切换，则重选最高优先级任务。 */
    if (switch_now) {
        /* 可能让刚唤醒的高优先级任务抢占当前任务。 */
        MRT_TaskSwitchToHighestReady();
    }

    /* 唤醒成功。 */
    return true;
}

/**
 * @brief 设置任务当前有效优先级。
 * @param task 目标任务句柄，不能为空。
 * @param priority 新有效优先级，必须小于 MRT_CFG_MAX_PRIORITIES。
 * @return void 无返回值；参数非法时直接返回。
 * @example
 * MRT_TaskKernelSetEffectivePriority(owner, waiter_priority);
 */
void MRT_TaskKernelSetEffectivePriority(MRT_TaskHandle task, MRT_Priority priority)
{
    /* 任务句柄不能为空。 */
    if (task == 0) {
        /* 无目标任务时直接返回。 */
        return;
    }

    /* 新优先级必须在配置范围内。 */
    if (priority >= MRT_CFG_MAX_PRIORITIES) {
        /* 非法优先级不修改任务状态。 */
        return;
    }

    /* 优先级没有变化时无需重排 ready list。 */
    if (task->priority == priority) {
        /* 直接返回调用方。 */
        return;
    }

    /* 记录任务节点当前是否在 ready list 或运行任务的 ready 节点中。 */
    bool was_linked = MRT_ListNodeIsLinked(&task->state_node);

    /* 记录修改前的任务状态，用于保持 running 状态不被 ready 插入逻辑覆盖。 */
    MRT_TaskState old_state = task->state;

    /* 如果任务节点已入链，需要先从旧优先级链表移除。 */
    if (was_linked) {
        /* 使用旧优先级维护 ready bitmap 和链表计数。 */
        MRT_TaskRemoveReady(task);
    }

    /* 写入新的有效优先级。 */
    task->priority = priority;

    /* 如果原来已入链，则按新优先级重新插入对应 ready list。 */
    if (was_linked) {
        /* 将任务节点插入新优先级 ready list 尾部。 */
        MRT_ListInsertTail(&g_ready_lists[task->priority], &task->state_node);

        /* 标记新优先级存在 ready/running 任务节点。 */
        MRT_PriorityBitmapSet(&g_ready_bitmap, task->priority);

        /* 恢复调用前任务状态，避免 running 任务被误标 ready。 */
        task->state = old_state;
    }
}

/**
 * @brief 将任务当前有效优先级恢复为基础优先级。
 * @param task 目标任务句柄，不能为空。
 * @return void 无返回值。
 * @example
 * MRT_TaskKernelRestoreBasePriority(owner);
 */
void MRT_TaskKernelRestoreBasePriority(MRT_TaskHandle task)
{
    /* 任务句柄不能为空。 */
    if (task == 0) {
        /* 无目标任务时直接返回。 */
        return;
    }

    /* 复用有效优先级设置函数恢复基础优先级。 */
    MRT_TaskKernelSetEffectivePriority(task, task->base_priority);
}

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

    /* 初始化对象等待链表节点，item 同样指回任务控制块。 */
    MRT_ListNodeInitialize(&storage->wait_node, storage, 0u);

    /* 新创建任务尚未阻塞，唤醒 tick 清零。 */
    storage->wake_tick = 0u;

    /* 新任务没有等待任何对象。 */
    storage->wait_reason = MRT_TASK_WAIT_REASON_NONE;

    /* 新任务没有挂起的等待结果。 */
    storage->wait_result = MRT_RESULT_OK;

    /* 新任务没有事件组等待掩码。 */
    storage->event_wait_bits = 0u;

    /* 新任务没有事件组匹配结果。 */
    storage->event_matched_bits = 0u;

    /* 新任务默认不处于 wait-all 事件等待。 */
    storage->event_wait_all = false;

    /* 新任务默认不请求事件等待退出清位。 */
    storage->event_clear_on_exit = false;

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
 * @brief 让当前任务阻塞指定 tick 数。
 * @param ticks 需要延时的 tick 数；为 0 时等价于主动让出 CPU。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示延时成功；调度器未运行或无当前任务时返回 MRT_RESULT_INVALID_CONTEXT。
 * @example
 * MRT_TaskDelay(10);
 */
MRT_Result MRT_TaskDelay(MRT_Tick ticks)
{
    /* 当前任务为空时，说明调度器未启动或无可运行任务。 */
    if (g_current_task == 0) {
        /* 延时只能由当前运行任务调用。 */
        return MRT_RESULT_INVALID_CONTEXT;
    }

    /* 延时 0 tick 等价于主动让出 CPU。 */
    if (ticks == 0u) {
        /* 复用 yield 调度路径。 */
        MRT_TaskKernelYield();

        /* 0 tick yield 完成。 */
        return MRT_RESULT_OK;
    }

    /* 保存需要阻塞的当前任务。 */
    MRT_Task *task = g_current_task;

    /* 将当前任务从 ready list 移除。 */
    MRT_TaskRemoveReady(task);

    /* 计算任务唤醒 tick，允许无符号自然回绕。 */
    task->wake_tick = MRT_KernelGetTick() + ticks;

    /* 将任务状态设置为 blocked。 */
    task->state = MRT_TASK_STATE_BLOCKED;

    /* 标记当前阻塞原因是纯 tick 延时。 */
    task->wait_reason = MRT_TASK_WAIT_REASON_DELAY;

    /* 纯延时醒来后没有对象等待错误，结果保持 OK。 */
    task->wait_result = MRT_RESULT_OK;

    /* 使用唤醒 tick 作为延时链表排序值。 */
    task->state_node.value = task->wake_tick;

    /* 将任务插入延时链表。 */
    MRT_ListInsertOrdered(&g_delayed_list, &task->state_node);

    /* 当前任务已经阻塞，先清空当前任务指针。 */
    g_current_task = 0;

    /* 选择下一个最高优先级 ready 任务运行。 */
    MRT_TaskSwitchToHighestReady();

    /* 延时操作完成。 */
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
