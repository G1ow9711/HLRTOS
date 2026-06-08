#include "myrtos/mrt_kernel.h"
#include "myrtos/mrt_port.h"
#include "myrtos/mrt_config.h"
#include "myrtos/mrt_heap.h"
#include "myrtos/mrt_timer.h"
#include "mrt_timer_internal.h"

/** @brief 软件定时器服务命令类型。 */
typedef enum MRT_TimerServiceCommandType {
    /** @brief 启动定时器命令。 */
    MRT_TIMER_SERVICE_COMMAND_START = 0,
    /** @brief 停止定时器命令。 */
    MRT_TIMER_SERVICE_COMMAND_STOP,
    /** @brief 复位定时器命令。 */
    MRT_TIMER_SERVICE_COMMAND_RESET,
    /** @brief 修改定时器周期命令。 */
    MRT_TIMER_SERVICE_COMMAND_CHANGE_PERIOD,
    /** @brief 定时器到期回调命令。 */
    MRT_TIMER_SERVICE_COMMAND_EXPIRE,
    /** @brief 用户 pending function 命令。 */
    MRT_TIMER_SERVICE_COMMAND_PENDING_FUNCTION
} MRT_TimerServiceCommandType;

/**
 * @brief 软件定时器服务队列元素。
 *
 * 队列元素保存控制命令、到期回调事件或用户 pending function。服务任务出队后按 FIFO
 * 顺序处理命令，并在临界区外执行用户回调。
 */
typedef struct MRT_TimerServiceEntry {
    /** @brief 服务命令类型。 */
    MRT_TimerServiceCommandType type;
    /** @brief 命令关联的软件定时器；pending function 命令可为空。 */
    MRT_TimerHandle timer;
    /** @brief 改周期命令携带的新周期。 */
    MRT_Tick period_ticks;
    /** @brief 待执行的 pending function。 */
    MRT_TimerPendingFunction function;
    /** @brief 传递给 pending function 的用户参数。 */
    void *arg;
    /** @brief 传递给 pending function 的整数值。 */
    uint32_t value;
} MRT_TimerServiceEntry;

/** @brief 活动软件定时器链表，节点按 expiry_tick 从小到大排序。 */
static MRT_List g_timer_active_list;

/** @brief 活动软件定时器链表是否已经完成初始化。 */
static bool g_timer_list_initialized;

/** @brief 定时器服务命令固定长度环形队列。 */
static MRT_TimerServiceEntry g_timer_service_queue[MRT_CFG_TIMER_COMMAND_QUEUE_LENGTH];

/** @brief 定时器服务命令环形队列读索引。 */
static uint32_t g_timer_service_head;

/** @brief 定时器服务命令环形队列写索引。 */
static uint32_t g_timer_service_tail;

/** @brief 定时器服务命令环形队列当前元素数量。 */
static uint32_t g_timer_service_count;

/**
 * @brief 判断当前 tick 是否已经到达定时器到期 tick。
 * @param now 当前系统 tick。
 * @param expiry_tick 定时器计划到期 tick。
 * @return bool 返回 true 表示已经到期，返回 false 表示尚未到期。
 * @example
 * if (MRT_TimerTickReached(now, timer->expiry_tick)) { callback(timer, timer->arg); }
 */
static bool MRT_TimerTickReached(MRT_Tick now, MRT_Tick expiry_tick)
{
    /* 使用有符号差值处理无符号 tick 回绕。 */
    return (int32_t)(now - expiry_tick) >= 0;
}

/**
 * @brief 确保软件定时器内部链表已经初始化。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * MRT_TimerEnsureInitialized();
 */
static void MRT_TimerEnsureInitialized(void)
{
    /* 如果链表已经初始化，直接返回。 */
    if (g_timer_list_initialized) {
        /* 已初始化状态无需重复清空链表。 */
        return;
    }

    /* 初始化活动定时器链表。 */
    MRT_ListInitialize(&g_timer_active_list);

    /* 标记链表已经可用。 */
    g_timer_list_initialized = true;
}

/**
 * @brief 在已进入临界区的前提下，将定时器按到期时间加入活动链表。
 * @param timer 定时器句柄，不能为空。
 * @param expiry_tick 下一次到期 tick。
 * @return void 无返回值。
 * @example
 * MRT_TimerArmLocked(timer, MRT_KernelGetTick() + timer->period_ticks);
 */
static void MRT_TimerArmLocked(MRT_TimerHandle timer, MRT_Tick expiry_tick)
{
    /* 如果节点已经在链表中，先移除旧位置，避免重复入链。 */
    if (MRT_ListNodeIsLinked(&timer->node)) {
        /* 从当前活动链表中脱离旧节点。 */
        MRT_ListRemove(&timer->node);
    }

    /* 保存新的到期 tick。 */
    timer->expiry_tick = expiry_tick;

    /* 更新链表节点排序值，使有序插入按到期时间排序。 */
    timer->node.value = expiry_tick;

    /* 确保节点关联对象仍指向当前定时器。 */
    timer->node.item = timer;

    /* 按到期 tick 插入活动定时器链表。 */
    MRT_ListInsertOrdered(&g_timer_active_list, &timer->node);

    /* 标记定时器处于活动状态。 */
    timer->active = true;
}

/**
 * @brief 在已进入临界区的前提下，将定时器从活动链表移除。
 * @param timer 定时器句柄，不能为空。
 * @return void 无返回值。
 * @example
 * MRT_TimerDisarmLocked(timer);
 */
static void MRT_TimerDisarmLocked(MRT_TimerHandle timer)
{
    /* 如果节点处于入链状态，先从活动链表移除。 */
    if (MRT_ListNodeIsLinked(&timer->node)) {
        /* 移除节点并清空节点的 owner/prev/next。 */
        MRT_ListRemove(&timer->node);
    }

    /* 标记定时器不再活动。 */
    timer->active = false;
}

/**
 * @brief 清空定时器服务命令环形队列。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * MRT_TimerResetServiceQueue();
 */
static void MRT_TimerResetServiceQueue(void)
{
    /* 读索引复位到队列起点。 */
    g_timer_service_head = 0u;

    /* 写索引复位到队列起点。 */
    g_timer_service_tail = 0u;

    /* 当前元素数量清零。 */
    g_timer_service_count = 0u;
}

/**
 * @brief 在已进入临界区的前提下投递定时器服务命令。
 * @param entry 待入队的服务命令，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示入队成功；队列满时返回 MRT_RESULT_OBJECT_FULL。
 * @example
 * MRT_TimerQueueServiceEntryLocked(&entry);
 */
static MRT_Result MRT_TimerQueueServiceEntryLocked(const MRT_TimerServiceEntry *entry)
{
    /* 队列满时不能写入新命令。 */
    if (g_timer_service_count >= MRT_CFG_TIMER_COMMAND_QUEUE_LENGTH) {
        /* 返回对象满，让调用方知道命令未被接收。 */
        return MRT_RESULT_OBJECT_FULL;
    }

    /* 在写索引位置复制服务命令。 */
    g_timer_service_queue[g_timer_service_tail] = *entry;

    /* 写索引向后移动并按容量回绕。 */
    g_timer_service_tail = (g_timer_service_tail + 1u) % MRT_CFG_TIMER_COMMAND_QUEUE_LENGTH;

    /* 当前元素数量加 1。 */
    g_timer_service_count++;

    /* 命令入队成功。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 投递一个定时器服务命令。
 * @param type 命令类型。
 * @param timer 命令关联的软件定时器；pending function 命令可为空。
 * @param period_ticks 改周期命令携带的新周期，其他命令忽略。
 * @param function pending function 命令携带的函数指针，其他命令忽略。
 * @param arg pending function 命令携带的指针参数。
 * @param value pending function 命令携带的整数参数。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示入队成功；队列满返回 MRT_RESULT_OBJECT_FULL。
 * @example
 * MRT_TimerEnqueueServiceCommand(MRT_TIMER_SERVICE_COMMAND_START, timer, 0, 0, 0, 0);
 */
static MRT_Result MRT_TimerEnqueueServiceCommand(MRT_TimerServiceCommandType type,
                                                 MRT_TimerHandle timer,
                                                 MRT_Tick period_ticks,
                                                 MRT_TimerPendingFunction function,
                                                 void *arg,
                                                 uint32_t value)
{
    /* 确保定时器内部状态已经初始化。 */
    MRT_TimerEnsureInitialized();

    /* 组装服务命令条目。 */
    MRT_TimerServiceEntry entry;

    /* 保存命令类型。 */
    entry.type = type;

    /* 保存命令关联定时器。 */
    entry.timer = timer;

    /* 保存命令携带的新周期。 */
    entry.period_ticks = period_ticks;

    /* 保存 pending function。 */
    entry.function = function;

    /* 保存 pending function 指针参数。 */
    entry.arg = arg;

    /* 保存 pending function 整数参数。 */
    entry.value = value;

    /* 进入临界区保护服务命令队列。 */
    MRT_IntState state = MRT_PortEnterCritical();

    /* 入队服务命令。 */
    MRT_Result result = MRT_TimerQueueServiceEntryLocked(&entry);

    /* 退出临界区。 */
    MRT_PortExitCritical(state);

    /* 返回入队结果。 */
    return result;
}

/**
 * @brief 在已进入临界区的前提下从服务队列取出一条命令。
 * @param out_entry 输出服务命令，不能为空。
 * @return bool 返回 true 表示取出命令；false 表示队列为空。
 * @example
 * MRT_TimerServiceEntry entry; bool ok = MRT_TimerDequeueServiceEntryLocked(&entry);
 */
static bool MRT_TimerDequeueServiceEntryLocked(MRT_TimerServiceEntry *out_entry)
{
    /* 队列为空时没有命令可取。 */
    if (g_timer_service_count == 0u) {
        /* 通知调用方队列为空。 */
        return false;
    }

    /* 复制当前读索引处的命令。 */
    *out_entry = g_timer_service_queue[g_timer_service_head];

    /* 读索引向后移动并按容量回绕。 */
    g_timer_service_head = (g_timer_service_head + 1u) % MRT_CFG_TIMER_COMMAND_QUEUE_LENGTH;

    /* 当前元素数量减 1。 */
    g_timer_service_count--;

    /* 已取出一条命令。 */
    return true;
}

/**
 * @brief 在已进入临界区的前提下清除某个定时器关联的未处理服务命令。
 * @param timer 要清理命令的软件定时器句柄，不能为空。
 * @return void 无返回值。
 * @example
 * MRT_TimerPurgeServiceCommandsForTimerLocked(timer);
 */
static void MRT_TimerPurgeServiceCommandsForTimerLocked(MRT_TimerHandle timer)
{
    /* 定义保留命令临时数组。 */
    MRT_TimerServiceEntry retained[MRT_CFG_TIMER_COMMAND_QUEUE_LENGTH];

    /* 记录保留命令数量。 */
    uint32_t retained_count = 0u;

    /* 保存原队列数量，用于遍历现有命令。 */
    uint32_t original_count = g_timer_service_count;

    /* 遍历原队列中的每个命令。 */
    for (uint32_t index = 0u; index < original_count; index++) {
        /* 读取当前命令。 */
        MRT_TimerServiceEntry entry = g_timer_service_queue[g_timer_service_head];

        /* 推进读索引。 */
        g_timer_service_head = (g_timer_service_head + 1u) % MRT_CFG_TIMER_COMMAND_QUEUE_LENGTH;

        /* 当前命令已经被取出。 */
        g_timer_service_count--;

        /* 与目标定时器关联的命令需要丢弃，避免删除后悬空引用。 */
        if (entry.timer == timer) {
            /* 跳过该命令。 */
            continue;
        }

        /* 保留与其他对象或 pending function 相关的命令。 */
        retained[retained_count] = entry;

        /* 保留数量加 1。 */
        retained_count++;
    }

    /* 清空队列索引，准备按原顺序重建保留命令。 */
    MRT_TimerResetServiceQueue();

    /* 按原 FIFO 顺序写回保留命令。 */
    for (uint32_t index = 0u; index < retained_count; index++) {
        /* 队列容量不会溢出，因为保留命令数量不超过原数量。 */
        (void)MRT_TimerQueueServiceEntryLocked(&retained[index]);
    }
}

/**
 * @brief 初始化软件定时器内核内部状态。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * MRT_TimerKernelInitialize();
 */
void MRT_TimerKernelInitialize(void)
{
    /* 如果链表已经初始化，先清理仍处于活动链表中的定时器。 */
    if (g_timer_list_initialized) {
        /* 循环摘除所有活动节点，确保内核重新初始化后没有旧定时器残留。 */
        while (!MRT_ListIsEmpty(&g_timer_active_list)) {
            /* 读取当前最早到期的节点。 */
            MRT_ListNode *node = MRT_ListGetHead(&g_timer_active_list);

            /* 从节点反查定时器控制块。 */
            MRT_TimerHandle timer = (MRT_TimerHandle)node->item;

            /* 从活动链表中移除节点。 */
            MRT_ListRemove(node);

            /* 如果节点关联了定时器对象，则同步清空活动状态。 */
            if (timer != 0) {
                /* 标记该定时器已经不再活动。 */
                timer->active = false;
            }
        }
    }

    /* 初始化或重新初始化活动定时器链表。 */
    MRT_ListInitialize(&g_timer_active_list);

    /* 清空定时器服务队列，避免重新初始化后执行旧命令。 */
    MRT_TimerResetServiceQueue();

    /* 标记链表已经完成初始化。 */
    g_timer_list_initialized = true;
}

/**
 * @brief 处理当前 tick 上已经到期的软件定时器。
 * @param now 当前系统 tick。
 * @return void 无返回值。
 * @example
 * MRT_TimerKernelTick(MRT_KernelGetTick());
 */
void MRT_TimerKernelTick(MRT_Tick now)
{
    /* 确保活动链表可用，支持未显式调用内核初始化的 host 测试场景。 */
    MRT_TimerEnsureInitialized();

    /* 持续处理当前 tick 前已经到期的所有定时器。 */
    for (;;) {
        /* 进入临界区，保护活动定时器链表。 */
        MRT_IntState state = MRT_PortEnterCritical();

        /* 如果没有活动定时器，退出处理循环。 */
        if (MRT_ListIsEmpty(&g_timer_active_list)) {
            /* 退出临界区。 */
            MRT_PortExitCritical(state);

            /* 当前 tick 没有可处理定时器。 */
            break;
        }

        /* 读取最早到期的定时器节点。 */
        MRT_ListNode *node = MRT_ListGetHead(&g_timer_active_list);

        /* 从节点反查定时器控制块。 */
        MRT_TimerHandle timer = (MRT_TimerHandle)node->item;

        /* 如果头部定时器尚未到期，后续节点也不需要处理。 */
        if ((timer == 0) || !MRT_TimerTickReached(now, timer->expiry_tick)) {
            /* 退出临界区。 */
            MRT_PortExitCritical(state);

            /* 等待后续 tick 再处理。 */
            break;
        }

        /* 从活动链表移除到期定时器。 */
        MRT_ListRemove(&timer->node);

        /* 默认先标记为非活动；自动重载路径随后会重新置为活动。 */
        timer->active = false;

        /* 自动重载定时器需要在回调前重新入链，使回调内部可以停止或改周期。 */
        if (timer->auto_reload) {
            /* 按当前 tick 重新计算下一次到期时间。 */
            MRT_TimerArmLocked(timer, now + timer->period_ticks);
        }

        /* 组装到期回调命令，后续由定时器服务任务执行用户回调。 */
        MRT_TimerServiceEntry entry;

        /* 记录到期回调命令类型。 */
        entry.type = MRT_TIMER_SERVICE_COMMAND_EXPIRE;

        /* 保存到期定时器句柄。 */
        entry.timer = timer;

        /* 到期回调命令不携带新周期。 */
        entry.period_ticks = 0u;

        /* 到期回调命令不使用 pending function 字段。 */
        entry.function = 0;

        /* 到期回调命令不使用 pending function 指针参数。 */
        entry.arg = 0;

        /* 到期回调命令不使用 pending function 整数参数。 */
        entry.value = 0u;

        /* 将到期回调命令入队；队列满时当前 preview 丢弃本次回调事件。 */
        (void)MRT_TimerQueueServiceEntryLocked(&entry);

        /* 退出临界区，避免用户回调在关中断状态下运行过久。 */
        MRT_PortExitCritical(state);
    }
}

/**
 * @brief 使用调用方提供的控制块静态创建软件定时器。
 * @param name 定时器名称，允许为空，仅用于调试显示。
 * @param period_ticks 定时器周期，单位为 tick，必须大于 0。
 * @param auto_reload true 表示周期定时器，false 表示单次定时器。
 * @param arg 用户回调参数。
 * @param callback 定时器到期回调函数，不能为空。
 * @param storage 定时器控制块存储，不能为空。
 * @param out_timer 输出定时器句柄，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示创建成功；参数非法时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * static MRT_Timer timer_cb;
 * MRT_TimerHandle timer;
 * MRT_TimerCreateStatic("blink", 100, true, NULL, BlinkCallback, &timer_cb, &timer);
 */
MRT_Result MRT_TimerCreateStatic(const char *name,
                                 MRT_Tick period_ticks,
                                 bool auto_reload,
                                 void *arg,
                                 MRT_TimerCallback callback,
                                 MRT_Timer *storage,
                                 MRT_TimerHandle *out_timer)
{
    /* 周期不能为 0，否则定时器会在同一个 tick 中无限到期。 */
    if (period_ticks == 0u) {
        /* 返回参数错误，提示调用方提供非零周期。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 回调函数不能为空，否则定时器到期后无法执行用户逻辑。 */
    if (callback == 0) {
        /* 返回参数错误，提示调用方提供有效回调。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 定时器控制块不能为空，否则无法保存定时器状态。 */
    if (storage == 0) {
        /* 返回参数错误，提示调用方提供静态控制块。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 输出句柄不能为空，否则创建成功后调用方无法使用对象。 */
    if (out_timer == 0) {
        /* 返回参数错误，提示调用方提供输出句柄地址。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 保存定时器名称指针。 */
    storage->name = name;

    /* 保存定时器周期。 */
    storage->period_ticks = period_ticks;

    /* 保存自动重载策略。 */
    storage->auto_reload = auto_reload;

    /* 保存用户回调参数。 */
    storage->arg = arg;

    /* 保存到期回调函数。 */
    storage->callback = callback;

    /* 新创建定时器默认不活动。 */
    storage->active = false;

    /* 新创建定时器还没有到期 tick。 */
    storage->expiry_tick = 0u;

    /* 初始化定时器链表节点，后续启动时加入活动定时器链表。 */
    MRT_ListNodeInitialize(&storage->node, storage, 0u);

    /* 标记对象使用静态存储创建。 */
    storage->static_storage = true;

    /* 输出定时器句柄给调用方。 */
    *out_timer = storage;

    /* 静态定时器创建成功。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 从 MyRTOS 全局堆动态创建软件定时器。
 * @param name 定时器名称，允许为空，仅用于调试显示。
 * @param period_ticks 定时器周期，单位为 tick，必须大于 0。
 * @param auto_reload true 表示周期定时器，false 表示单次定时器。
 * @param arg 用户回调参数。
 * @param callback 定时器到期回调函数，不能为空。
 * @param out_timer 输出定时器句柄，不能为空；失败时写入空指针。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示创建成功；参数非法返回 MRT_RESULT_INVALID_ARGUMENT；
 *         堆不可用或空间不足时返回 MRT_RESULT_NO_MEMORY。
 * @example
 * MRT_TimerHandle timer;
 * MRT_TimerCreate("blink", 100, true, NULL, BlinkCallback, &timer);
 */
MRT_Result MRT_TimerCreate(const char *name,
                           MRT_Tick period_ticks,
                           bool auto_reload,
                           void *arg,
                           MRT_TimerCallback callback,
                           MRT_TimerHandle *out_timer)
{
    /* 输出句柄不能为空。 */
    if (out_timer == 0) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 失败路径默认清空输出句柄。 */
    *out_timer = 0;

    /* 周期和回调先做静态创建同款校验，避免无效参数消耗堆。 */
    if ((period_ticks == 0u) || (callback == 0)) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 动态分配关闭时不能创建堆对象。 */
    if (MRT_CFG_SUPPORT_DYNAMIC_ALLOCATION == 0u) {
        /* 返回内存不足。 */
        return MRT_RESULT_NO_MEMORY;
    }

    /* 申请一个定时器控制块。 */
    MRT_Timer *timer = (MRT_Timer *)MRT_Malloc(sizeof(MRT_Timer));

    /* 堆空间不足时创建失败。 */
    if (timer == 0) {
        /* 返回内存不足。 */
        return MRT_RESULT_NO_MEMORY;
    }

    /* 复用静态创建逻辑初始化控制块。 */
    MRT_Result result = MRT_TimerCreateStatic(name, period_ticks, auto_reload, arg, callback, timer, out_timer);

    /* 初始化失败时释放堆块。 */
    if (result != MRT_RESULT_OK) {
        /* 释放控制块。 */
        (void)MRT_Free(timer);

        /* 清空输出句柄。 */
        *out_timer = 0;

        /* 返回实际错误。 */
        return result;
    }

    /* 标记定时器归动态堆所有。 */
    timer->static_storage = false;

    /* 动态定时器创建成功。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 删除动态创建的软件定时器并归还堆内存。
 * @param timer 待删除定时器句柄，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示删除成功；空句柄返回 MRT_RESULT_INVALID_ARGUMENT；
 *         静态定时器返回 MRT_RESULT_OBJECT_BUSY。
 * @example
 * MRT_TimerDelete(timer);
 */
MRT_Result MRT_TimerDelete(MRT_TimerHandle timer)
{
    /* 定时器句柄不能为空。 */
    if (timer == 0) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 静态定时器内存不归堆释放路径所有。 */
    if (timer->static_storage) {
        /* 返回对象忙。 */
        return MRT_RESULT_OBJECT_BUSY;
    }

    /* 确保定时器内部状态已经初始化。 */
    MRT_TimerEnsureInitialized();

    /* 进入临界区，避免删除时服务队列或活动链表并发访问该定时器。 */
    MRT_IntState state = MRT_PortEnterCritical();

    /* 删除前丢弃该定时器尚未处理的控制命令和到期事件。 */
    MRT_TimerPurgeServiceCommandsForTimerLocked(timer);

    /* 删除活动定时器前直接从活动链表移除，不再投递异步停止命令。 */
    MRT_TimerDisarmLocked(timer);

    /* 退出临界区。 */
    MRT_PortExitCritical(state);

    /* 动态定时器控制块就是堆块起始地址。 */
    return MRT_Free(timer);
}

/**
 * @brief 查询软件定时器是否处于活动状态。
 * @param timer 定时器句柄，不能为空。
 * @param out_active 输出活动状态，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示查询成功；参数非法时返回 MRT_RESULT_INVALID_ARGUMENT；
 *         查询结果只反映已经由服务任务处理过的控制命令。
 * @example
 * bool active;
 * MRT_TimerIsActive(timer, &active);
 */
MRT_Result MRT_TimerIsActive(MRT_TimerHandle timer, bool *out_active)
{
    /* 定时器句柄不能为空，否则无法读取状态。 */
    if (timer == 0) {
        /* 返回参数错误，提示调用方传入有效定时器。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 输出指针不能为空，否则无法写回状态。 */
    if (out_active == 0) {
        /* 返回参数错误，提示调用方提供输出存储。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 写回当前活动状态。 */
    *out_active = timer->active;

    /* 查询成功。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 获取软件定时器名称。
 * @param timer 定时器句柄，不能为空。
 * @return const char* 返回定时器名称指针；定时器句柄为空时返回空指针。
 * @example
 * const char *name = MRT_TimerGetName(timer);
 */
const char *MRT_TimerGetName(MRT_TimerHandle timer)
{
    /* 空句柄没有名称，返回空指针。 */
    if (timer == 0) {
        /* 返回空指针给调用方。 */
        return 0;
    }

    /* 返回控制块中保存的名称指针。 */
    return timer->name;
}

/**
 * @brief 投递启动软件定时器命令。
 * @param timer 定时器句柄，不能为空。
 * @param timeout 等待内部控制资源的 tick 数；当前阶段不阻塞等待队列空位。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示命令入队成功；参数非法时返回 MRT_RESULT_INVALID_ARGUMENT；
 *         服务命令队列满时返回 MRT_RESULT_OBJECT_FULL。
 * @example
 * MRT_TimerStart(timer, 0);
 */
MRT_Result MRT_TimerStart(MRT_TimerHandle timer, MRT_Timeout timeout)
{
    /* 当前 preview 不阻塞等待服务队列空位，timeout 保留为兼容参数。 */
    (void)timeout;

    /* 定时器句柄不能为空，否则无法访问周期和链表节点。 */
    if (timer == 0) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 投递启动命令，由定时器服务任务按 FIFO 顺序真正启动。 */
    return MRT_TimerEnqueueServiceCommand(MRT_TIMER_SERVICE_COMMAND_START, timer, 0u, 0, 0, 0u);
}

/**
 * @brief 投递停止软件定时器命令。
 * @param timer 定时器句柄，不能为空。
 * @param timeout 等待内部控制资源的 tick 数；当前阶段不阻塞等待队列空位。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示命令入队成功；参数非法时返回 MRT_RESULT_INVALID_ARGUMENT；
 *         服务命令队列满时返回 MRT_RESULT_OBJECT_FULL。
 * @example
 * MRT_TimerStop(timer, 0);
 */
MRT_Result MRT_TimerStop(MRT_TimerHandle timer, MRT_Timeout timeout)
{
    /* 当前 preview 不阻塞等待服务队列空位，timeout 保留为兼容参数。 */
    (void)timeout;

    /* 定时器句柄不能为空，否则无法访问链表节点。 */
    if (timer == 0) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 投递停止命令，由定时器服务任务按 FIFO 顺序真正停止。 */
    return MRT_TimerEnqueueServiceCommand(MRT_TIMER_SERVICE_COMMAND_STOP, timer, 0u, 0, 0, 0u);
}

/**
 * @brief 投递重新装载软件定时器命令。
 * @param timer 定时器句柄，不能为空。
 * @param timeout 等待内部控制资源的 tick 数；当前阶段不阻塞等待队列空位。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示命令入队成功；参数非法时返回 MRT_RESULT_INVALID_ARGUMENT；
 *         服务命令队列满时返回 MRT_RESULT_OBJECT_FULL。
 * @example
 * MRT_TimerReset(timer, 0);
 */
MRT_Result MRT_TimerReset(MRT_TimerHandle timer, MRT_Timeout timeout)
{
    /* 当前 preview 不阻塞等待服务队列空位，timeout 保留为兼容参数。 */
    (void)timeout;

    /* 定时器句柄不能为空，否则无法访问周期和链表节点。 */
    if (timer == 0) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 投递复位命令，由定时器服务任务按 FIFO 顺序重新装载。 */
    return MRT_TimerEnqueueServiceCommand(MRT_TIMER_SERVICE_COMMAND_RESET, timer, 0u, 0, 0, 0u);
}

/**
 * @brief 投递修改软件定时器周期命令。
 * @param timer 定时器句柄，不能为空。
 * @param new_period_ticks 新周期，单位为 tick，必须大于 0。
 * @param timeout 等待内部控制资源的 tick 数；当前阶段不阻塞等待队列空位。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示命令入队成功；参数非法时返回 MRT_RESULT_INVALID_ARGUMENT；
 *         服务命令队列满时返回 MRT_RESULT_OBJECT_FULL。
 * @example
 * MRT_TimerChangePeriod(timer, 50, 0);
 */
MRT_Result MRT_TimerChangePeriod(MRT_TimerHandle timer, MRT_Tick new_period_ticks, MRT_Timeout timeout)
{
    /* 当前 preview 不阻塞等待服务队列空位，timeout 保留为兼容参数。 */
    (void)timeout;

    /* 定时器句柄不能为空，否则无法修改控制块。 */
    if (timer == 0) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 新周期不能为 0，否则会造成定时器在同一 tick 内反复到期。 */
    if (new_period_ticks == 0u) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 投递改周期命令，由定时器服务任务按 FIFO 顺序更新周期。 */
    return MRT_TimerEnqueueServiceCommand(MRT_TIMER_SERVICE_COMMAND_CHANGE_PERIOD,
                                          timer,
                                          new_period_ticks,
                                          0,
                                          0,
                                          0u);
}

/**
 * @brief 投递一个 pending function 到软件定时器服务队列。
 * @param function 待延后执行的函数指针，不能为空。
 * @param arg 传递给 function 的用户参数，允许为空。
 * @param value 传递给 function 的整数值。
 * @param timeout 等待队列空位的 tick 数；当前阶段为兼容参数，直接忽略。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示入队成功；函数为空返回 MRT_RESULT_INVALID_ARGUMENT；
 *         队列满返回 MRT_RESULT_OBJECT_FULL。
 * @example
 * MRT_TimerPendFunctionCall(DeferredWork, user, 1, 0);
 */
MRT_Result MRT_TimerPendFunctionCall(MRT_TimerPendingFunction function,
                                     void *arg,
                                     uint32_t value,
                                     MRT_Timeout timeout)
{
    /* 当前阶段不阻塞等待队列空位，timeout 保留为兼容参数。 */
    (void)timeout;

    /* pending function 指针不能为空，否则服务路径无法执行。 */
    if (function == 0) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 投递 pending function 命令，由定时器服务任务按 FIFO 顺序执行。 */
    return MRT_TimerEnqueueServiceCommand(MRT_TIMER_SERVICE_COMMAND_PENDING_FUNCTION,
                                          0,
                                          0u,
                                          function,
                                          arg,
                                          value);
}

/**
 * @brief 运行并清空当前已投递的定时器服务命令队列。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * MRT_TimerServiceRunPending();
 */
void MRT_TimerServiceRunPending(void)
{
    /* 确保定时器内部状态已经初始化。 */
    MRT_TimerEnsureInitialized();

    /* 持续出队直到服务命令队列为空。 */
    for (;;) {
        /* 进入临界区保护环形队列。 */
        MRT_IntState state = MRT_PortEnterCritical();

        /* 定义本轮要处理的服务命令。 */
        MRT_TimerServiceEntry entry;

        /* 如果队列为空，退出循环。 */
        if (!MRT_TimerDequeueServiceEntryLocked(&entry)) {
            /* 退出临界区。 */
            MRT_PortExitCritical(state);

            /* 没有待处理命令。 */
            break;
        }

        /* 根据命令类型执行定时器服务动作。 */
        switch (entry.type) {
            case MRT_TIMER_SERVICE_COMMAND_START:
                /* 启动命令按服务任务当前 tick 装载到期时间。 */
                if (entry.timer != 0) {
                    /* 将定时器加入活动链表。 */
                    MRT_TimerArmLocked(entry.timer, MRT_KernelGetTick() + entry.timer->period_ticks);
                }

                /* 退出临界区。 */
                MRT_PortExitCritical(state);
                break;

            case MRT_TIMER_SERVICE_COMMAND_STOP:
                /* 停止命令从活动链表移除定时器。 */
                if (entry.timer != 0) {
                    /* 停止定时器。 */
                    MRT_TimerDisarmLocked(entry.timer);
                }

                /* 退出临界区。 */
                MRT_PortExitCritical(state);
                break;

            case MRT_TIMER_SERVICE_COMMAND_RESET:
                /* 复位命令按服务任务当前 tick 重新装载。 */
                if (entry.timer != 0) {
                    /* 重新加入活动链表。 */
                    MRT_TimerArmLocked(entry.timer, MRT_KernelGetTick() + entry.timer->period_ticks);
                }

                /* 退出临界区。 */
                MRT_PortExitCritical(state);
                break;

            case MRT_TIMER_SERVICE_COMMAND_CHANGE_PERIOD:
                /* 改周期命令更新周期字段，并在活动时重算到期点。 */
                if (entry.timer != 0) {
                    /* 保存新周期。 */
                    entry.timer->period_ticks = entry.period_ticks;

                    /* 活动定时器按新周期重新装载。 */
                    if (entry.timer->active) {
                        /* 以服务任务当前 tick 为基准重算到期时间。 */
                        MRT_TimerArmLocked(entry.timer, MRT_KernelGetTick() + entry.timer->period_ticks);
                    }
                }

                /* 退出临界区。 */
                MRT_PortExitCritical(state);
                break;

            case MRT_TIMER_SERVICE_COMMAND_EXPIRE: {
                /* 到期回调必须在临界区外运行。 */
                MRT_TimerHandle timer = entry.timer;

                /* 读取回调函数。 */
                MRT_TimerCallback callback = (timer != 0) ? timer->callback : 0;

                /* 读取回调参数。 */
                void *arg = (timer != 0) ? timer->arg : 0;

                /* 退出临界区。 */
                MRT_PortExitCritical(state);

                /* 如果回调仍有效，则执行用户回调。 */
                if (callback != 0) {
                    /* 调用定时器到期回调。 */
                    callback(timer, arg);
                }
                break;
            }

            case MRT_TIMER_SERVICE_COMMAND_PENDING_FUNCTION: {
                /* pending function 也必须在临界区外运行。 */
                MRT_TimerPendingFunction function = entry.function;

                /* 保存用户指针参数。 */
                void *arg = entry.arg;

                /* 保存用户整数参数。 */
                uint32_t value = entry.value;

                /* 退出临界区。 */
                MRT_PortExitCritical(state);

                /* 如果函数指针有效，则执行用户延后函数。 */
                if (function != 0) {
                    /* 将保存的指针参数和整数参数传给用户函数。 */
                    function(arg, value);
                }
                break;
            }

            default:
                /* 未知命令直接丢弃。 */
                MRT_PortExitCritical(state);
                break;
        }
    }
}

/**
 * @brief 查询活动软件定时器链表中最近的到期 tick。
 * @param out_tick 输出最近定时器到期 tick，不能为空。
 * @return bool 返回 true 表示存在活动定时器；返回 false 表示没有活动定时器或参数为空。
 * @example
 * MRT_Tick expiry_tick;
 * bool exists = MRT_TimerKernelGetNextExpiryTick(&expiry_tick);
 * (void)exists;
 */
bool MRT_TimerKernelGetNextExpiryTick(MRT_Tick *out_tick)
{
    /* 输出指针不能为空，否则调用方无法获得定时器到期点。 */
    if (out_tick == 0) {
        /* 参数无效时直接报告没有可用 deadline。 */
        return false;
    }

    /* 确保活动链表已经初始化，支持在内核初始化后的任意时刻查询。 */
    MRT_TimerEnsureInitialized();

    /* 活动链表为空表示没有软件定时器限制 tickless 睡眠。 */
    if (MRT_ListIsEmpty(&g_timer_active_list)) {
        /* 没有定时器 deadline 可供 tickless 参考。 */
        return false;
    }

    /* 活动链表按 expiry_tick 排序，头节点就是最近到期定时器。 */
    MRT_ListNode *head = MRT_ListGetHead(&g_timer_active_list);

    /* 从链表节点恢复定时器控制块指针。 */
    MRT_TimerHandle timer = (MRT_TimerHandle)head->item;

    /* 防御性检查定时器指针，避免损坏链表导致空指针解引用。 */
    if (timer == 0) {
        /* 链表内容异常时不向 tickless 提供 deadline。 */
        return false;
    }

    /* 写出最近定时器到期 tick。 */
    *out_tick = timer->expiry_tick;

    /* 通知调用方已经找到定时器 deadline。 */
    return true;
}
