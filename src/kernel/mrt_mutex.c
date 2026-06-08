#include "myrtos/mrt_mutex.h"
#include "myrtos/mrt_config.h"
#include "myrtos/mrt_heap.h"
#include "mrt_mutex_internal.h"
#include "mrt_task_internal.h"

#include <stddef.h>

/** @brief 已创建互斥锁注册表，用于生命周期策略检查。 */
static MRT_List g_mutex_registry;

/** @brief 互斥锁注册表是否已经初始化。 */
static bool g_mutex_registry_initialized;

/**
 * @brief 确保互斥锁内部注册表已经初始化。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * MRT_MutexEnsureRegistryInitialized();
 */
static void MRT_MutexEnsureRegistryInitialized(void)
{
    /* 已经初始化时直接返回。 */
    if (g_mutex_registry_initialized) {
        /* 避免重复初始化清空已注册对象。 */
        return;
    }

    /* 初始化互斥锁全局注册表。 */
    MRT_ListInitialize(&g_mutex_registry);

    /* 标记注册表可用。 */
    g_mutex_registry_initialized = true;
}

/**
 * @brief 初始化互斥锁模块内部注册表。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * MRT_MutexKernelInitialize();
 */
void MRT_MutexKernelInitialize(void)
{
    /* 初始化注册表链表，清除前一次测试或系统重启留下的注册关系。 */
    MRT_ListInitialize(&g_mutex_registry);

    /* 标记注册表已经初始化。 */
    g_mutex_registry_initialized = true;
}

/**
 * @brief 把互斥锁加入内部注册表。
 * @param mutex 互斥锁句柄，不能为空。
 * @return void 无返回值。
 * @example
 * MRT_MutexRegister(mutex);
 */
static void MRT_MutexRegister(MRT_MutexHandle mutex)
{
    /* 确保注册表存在。 */
    MRT_MutexEnsureRegistryInitialized();

    /* 初始化注册节点，节点 item 指回互斥锁控制块。 */
    MRT_ListNodeInitialize(&mutex->registry_node, mutex, 0u);

    /* 将互斥锁追加到注册表尾部。 */
    MRT_ListInsertTail(&g_mutex_registry, &mutex->registry_node);
}

/**
 * @brief 从内部注册表移除互斥锁。
 * @param mutex 互斥锁句柄，不能为空。
 * @return void 无返回值。
 * @example
 * MRT_MutexUnregister(mutex);
 */
static void MRT_MutexUnregister(MRT_MutexHandle mutex)
{
    /* 未入链时无需移除。 */
    if (!MRT_ListNodeIsLinked(&mutex->registry_node)) {
        /* 对象不在注册表中。 */
        return;
    }

    /* 从注册表摘除该互斥锁。 */
    MRT_ListRemove(&mutex->registry_node);
}

/**
 * @brief 根据互斥锁剩余等待者重新计算拥有者有效优先级。
 * @param mutex 互斥锁句柄，不能为空且必须仍有拥有者。
 * @return void 无返回值。
 * @example
 * MRT_MutexRecalculateOwnerPriority(mutex);
 */
static void MRT_MutexRecalculateOwnerPriority(MRT_MutexHandle mutex)
{
    /* 互斥锁句柄不能为空。 */
    if (mutex == 0) {
        /* 无目标对象时直接返回。 */
        return;
    }

    /* 没有拥有者时不存在优先级继承关系。 */
    if (mutex->owner == 0) {
        /* 无拥有者时直接返回。 */
        return;
    }

    /* 以拥有者基础优先级作为回滚下限。 */
    MRT_Priority target_priority = mutex->owner->base_priority;

    /* 从等待链表头部开始扫描剩余等待者。 */
    MRT_ListNode *node = MRT_ListGetHead(&mutex->waiting_lockers);

    /* 遍历所有剩余等待者，找到最高有效优先级。 */
    while (node != 0) {
        /* 从等待节点恢复任务句柄。 */
        MRT_TaskHandle waiter = (MRT_TaskHandle)node->item;

        /* 读取当前等待者有效优先级。 */
        MRT_Priority waiter_priority = 0u;
        (void)MRT_TaskGetPriority(waiter, &waiter_priority);

        /* 如果等待者优先级高于当前目标值，则更新继承目标。 */
        if (waiter_priority > target_priority) {
            /* 保存更高的等待者优先级。 */
            target_priority = waiter_priority;
        }

        /* 到达链表尾部后结束遍历。 */
        if (node->next == &mutex->waiting_lockers.sentinel) {
            /* 没有更多真实节点。 */
            break;
        }

        /* 继续扫描下一个等待节点。 */
        node = MRT_ListGetNext(node);
    }

    /* 按剩余等待者计算结果设置拥有者有效优先级。 */
    MRT_TaskKernelSetEffectivePriority(mutex->owner, target_priority);
}

/**
 * @brief 处理互斥锁等待任务因 tick 超时而离开等待链表后的优先级回滚。
 * @param waiting_lockers 已经移除超时任务后的互斥锁等待链表，不能为空。
 * @return void 无返回值。
 * @example
 * MRT_MutexKernelHandleLockTimeout(wait_list);
 */
void MRT_MutexKernelHandleLockTimeout(MRT_List *waiting_lockers)
{
    /* 等待链表不能为空。 */
    if (waiting_lockers == 0) {
        /* 参数非法时无法定位互斥锁。 */
        return;
    }

    /* waiting_lockers 是 MRT_Mutex 内嵌字段，可由字段地址反推出互斥锁控制块地址。 */
    MRT_MutexHandle mutex = (MRT_MutexHandle)((char *)waiting_lockers - offsetof(MRT_Mutex, waiting_lockers));

    /* 根据剩余等待者重新计算拥有者继承优先级；无剩余等待者时会回落到基础优先级。 */
    MRT_MutexRecalculateOwnerPriority(mutex);
}

/**
 * @brief 判断任务是否可以安全删除。
 * @param task 待删除任务句柄，不能为空。
 * @return bool 返回 true 表示任务未持有互斥锁，可以继续删除；返回 false 表示任务仍持有互斥锁。
 * @example
 * if (!MRT_MutexKernelCanDeleteTask(task)) { return MRT_RESULT_OBJECT_BUSY; }
 */
bool MRT_MutexKernelCanDeleteTask(MRT_TaskHandle task)
{
    /* 空任务句柄不能通过删除检查。 */
    if (task == 0) {
        /* 调用方应先处理参数错误。 */
        return false;
    }

    /* 确保注册表已初始化，避免调度启动前创建互斥锁时漏查。 */
    MRT_MutexEnsureRegistryInitialized();

    /* 从注册表头部开始扫描所有已创建互斥锁。 */
    MRT_ListNode *node = MRT_ListGetHead(&g_mutex_registry);

    /* 遍历注册表，查找是否有互斥锁仍由目标任务持有。 */
    while (node != 0) {
        /* 从注册节点恢复互斥锁句柄。 */
        MRT_MutexHandle mutex = (MRT_MutexHandle)node->item;

        /* 如果目标任务仍是拥有者，删除必须被拒绝。 */
        if ((mutex->owner == task) && (mutex->lock_count != 0u)) {
            /* 持锁任务不能安全删除。 */
            return false;
        }

        /* 到达注册表尾部后结束遍历。 */
        if (node->next == &g_mutex_registry.sentinel) {
            /* 没有更多互斥锁。 */
            break;
        }

        /* 继续扫描下一个互斥锁。 */
        node = MRT_ListGetNext(node);
    }

    /* 未发现目标任务持有互斥锁，可以继续执行删除。 */
    return true;
}

/**
 * @brief 使用调用方提供的控制块静态创建普通互斥锁。
 * @param storage 互斥锁控制块存储，不能为空。
 * @param out_mutex 输出互斥锁句柄，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示创建成功；参数非法时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * static MRT_Mutex mutex_cb;
 * MRT_MutexHandle mutex;
 * MRT_MutexCreateStatic(&mutex_cb, &mutex);
 */
MRT_Result MRT_MutexCreateStatic(MRT_Mutex *storage, MRT_MutexHandle *out_mutex)
{
    /* 互斥锁控制块不能为空，否则无法保存锁状态。 */
    if (storage == 0) {
        /* 返回参数错误，提示调用方提供静态控制块。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 输出句柄不能为空，否则创建成功后调用方无法使用对象。 */
    if (out_mutex == 0) {
        /* 返回参数错误，提示调用方提供输出句柄地址。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 新互斥锁没有拥有者。 */
    storage->owner = 0;

    /* 新互斥锁初始锁定深度为 0。 */
    storage->lock_count = 0u;

    /* 普通互斥锁不允许递归加锁。 */
    storage->recursive = false;

    /* 初始化等待加锁任务链表。 */
    MRT_ListInitialize(&storage->waiting_lockers);

    /* 标记对象使用静态存储创建。 */
    storage->static_storage = true;

    /* 将互斥锁登记到内部注册表，供任务删除策略检查。 */
    MRT_MutexRegister(storage);

    /* 输出互斥锁句柄给调用方。 */
    *out_mutex = storage;

    /* 静态互斥锁创建成功。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 使用调用方提供的控制块静态创建递归互斥锁。
 * @param storage 互斥锁控制块存储，不能为空。
 * @param out_mutex 输出互斥锁句柄，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示创建成功；参数非法时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * static MRT_Mutex mutex_cb;
 * MRT_MutexHandle mutex;
 * MRT_MutexCreateRecursiveStatic(&mutex_cb, &mutex);
 */
MRT_Result MRT_MutexCreateRecursiveStatic(MRT_Mutex *storage, MRT_MutexHandle *out_mutex)
{
    /* 互斥锁控制块不能为空，否则无法保存锁状态。 */
    if (storage == 0) {
        /* 返回参数错误，提示调用方提供静态控制块。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 输出句柄不能为空，否则创建成功后调用方无法使用对象。 */
    if (out_mutex == 0) {
        /* 返回参数错误，提示调用方提供输出句柄地址。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 新递归互斥锁没有拥有者。 */
    storage->owner = 0;

    /* 新递归互斥锁初始锁定深度为 0。 */
    storage->lock_count = 0u;

    /* 标记该互斥锁允许同一拥有者递归加锁。 */
    storage->recursive = true;

    /* 初始化等待加锁任务链表。 */
    MRT_ListInitialize(&storage->waiting_lockers);

    /* 标记对象使用静态存储创建。 */
    storage->static_storage = true;

    /* 将递归互斥锁登记到内部注册表，供任务删除策略检查。 */
    MRT_MutexRegister(storage);

    /* 输出互斥锁句柄给调用方。 */
    *out_mutex = storage;

    /* 静态递归互斥锁创建成功。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 从 MyRTOS 全局堆动态创建普通互斥锁。
 * @param out_mutex 输出互斥锁句柄，不能为空；失败时写入空指针。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示创建成功；参数非法返回 MRT_RESULT_INVALID_ARGUMENT；
 *         堆不可用或空间不足时返回 MRT_RESULT_NO_MEMORY。
 * @example
 * MRT_MutexHandle mutex;
 * MRT_MutexCreate(&mutex);
 */
MRT_Result MRT_MutexCreate(MRT_MutexHandle *out_mutex)
{
    /* 输出句柄不能为空。 */
    if (out_mutex == 0) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 失败路径默认清空输出句柄。 */
    *out_mutex = 0;

    /* 动态分配关闭时不能创建堆对象。 */
    if (MRT_CFG_SUPPORT_DYNAMIC_ALLOCATION == 0u) {
        /* 返回内存不足。 */
        return MRT_RESULT_NO_MEMORY;
    }

    /* 申请一个互斥锁控制块。 */
    MRT_Mutex *mutex = (MRT_Mutex *)MRT_Malloc(sizeof(MRT_Mutex));

    /* 堆空间不足时创建失败。 */
    if (mutex == 0) {
        /* 返回内存不足。 */
        return MRT_RESULT_NO_MEMORY;
    }

    /* 复用静态创建逻辑初始化控制块。 */
    MRT_Result result = MRT_MutexCreateStatic(mutex, out_mutex);

    /* 初始化失败时释放堆块。 */
    if (result != MRT_RESULT_OK) {
        /* 释放控制块。 */
        (void)MRT_Free(mutex);

        /* 清空输出句柄。 */
        *out_mutex = 0;

        /* 返回实际错误。 */
        return result;
    }

    /* 标记互斥锁归动态堆所有。 */
    mutex->static_storage = false;

    /* 动态普通互斥锁创建成功。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 从 MyRTOS 全局堆动态创建递归互斥锁。
 * @param out_mutex 输出互斥锁句柄，不能为空；失败时写入空指针。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示创建成功；参数非法返回 MRT_RESULT_INVALID_ARGUMENT；
 *         堆不可用或空间不足时返回 MRT_RESULT_NO_MEMORY。
 * @example
 * MRT_MutexHandle mutex;
 * MRT_MutexCreateRecursive(&mutex);
 */
MRT_Result MRT_MutexCreateRecursive(MRT_MutexHandle *out_mutex)
{
    /* 输出句柄不能为空。 */
    if (out_mutex == 0) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 失败路径默认清空输出句柄。 */
    *out_mutex = 0;

    /* 动态分配关闭时不能创建堆对象。 */
    if (MRT_CFG_SUPPORT_DYNAMIC_ALLOCATION == 0u) {
        /* 返回内存不足。 */
        return MRT_RESULT_NO_MEMORY;
    }

    /* 申请一个互斥锁控制块。 */
    MRT_Mutex *mutex = (MRT_Mutex *)MRT_Malloc(sizeof(MRT_Mutex));

    /* 堆空间不足时创建失败。 */
    if (mutex == 0) {
        /* 返回内存不足。 */
        return MRT_RESULT_NO_MEMORY;
    }

    /* 复用静态递归创建逻辑初始化控制块。 */
    MRT_Result result = MRT_MutexCreateRecursiveStatic(mutex, out_mutex);

    /* 初始化失败时释放堆块。 */
    if (result != MRT_RESULT_OK) {
        /* 释放控制块。 */
        (void)MRT_Free(mutex);

        /* 清空输出句柄。 */
        *out_mutex = 0;

        /* 返回实际错误。 */
        return result;
    }

    /* 标记互斥锁归动态堆所有。 */
    mutex->static_storage = false;

    /* 动态递归互斥锁创建成功。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 删除动态创建的互斥锁并归还堆内存。
 * @param mutex 待删除互斥锁句柄，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示删除成功；空句柄返回 MRT_RESULT_INVALID_ARGUMENT；
 *         静态互斥锁、已上锁互斥锁或仍有等待任务时返回 MRT_RESULT_OBJECT_BUSY。
 * @example
 * MRT_MutexDelete(mutex);
 */
MRT_Result MRT_MutexDelete(MRT_MutexHandle mutex)
{
    /* 互斥锁句柄不能为空。 */
    if (mutex == 0) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 静态互斥锁内存不归堆释放路径所有。 */
    if (mutex->static_storage) {
        /* 返回对象忙。 */
        return MRT_RESULT_OBJECT_BUSY;
    }

    /* 已有拥有者或锁深度非零时不能删除。 */
    if ((mutex->owner != 0) || (mutex->lock_count != 0u)) {
        /* 返回对象忙，调用方必须先解锁。 */
        return MRT_RESULT_OBJECT_BUSY;
    }

    /* 仍有任务等待锁时不能删除。 */
    if (!MRT_ListIsEmpty(&mutex->waiting_lockers)) {
        /* 返回对象忙。 */
        return MRT_RESULT_OBJECT_BUSY;
    }

    /* 从内部注册表摘除该动态互斥锁。 */
    MRT_MutexUnregister(mutex);

    /* 动态互斥锁控制块就是堆块起始地址。 */
    return MRT_Free(mutex);
}

/**
 * @brief 获取互斥锁。
 * @param mutex 互斥锁句柄，不能为空。
 * @param timeout 等待锁可用的 tick 数；当前任务未运行时不能调用。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示加锁成功；已被占用且 timeout 为 0 时返回 MRT_RESULT_OBJECT_BUSY；
 *         参数非法返回 MRT_RESULT_INVALID_ARGUMENT；无当前任务返回 MRT_RESULT_INVALID_CONTEXT。
 * @example
 * MRT_MutexLock(mutex, 0);
 */
MRT_Result MRT_MutexLock(MRT_MutexHandle mutex, MRT_Timeout timeout)
{
    /* 互斥锁句柄不能为空，否则无法读取和修改锁状态。 */
    if (mutex == 0) {
        /* 返回参数错误，提示调用方传入有效互斥锁。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 互斥锁必须由当前运行任务持有。 */
    MRT_TaskHandle current = MRT_TaskGetCurrent();

    /* 当前任务为空表示调度器尚未运行或当前不在任务上下文。 */
    if (current == 0) {
        /* 返回非法上下文，提示调用方只能在任务上下文加锁。 */
        return MRT_RESULT_INVALID_CONTEXT;
    }

    /* 无拥有者时，当前任务可以立即获得互斥锁。 */
    if (mutex->owner == 0) {
        /* 设置当前任务为拥有者。 */
        mutex->owner = current;

        /* 普通互斥锁锁定深度设为 1。 */
        mutex->lock_count = 1u;

        /* 加锁成功。 */
        return MRT_RESULT_OK;
    }

    /* 当前任务已经拥有该锁。 */
    if (mutex->owner == current) {
        /* 普通互斥锁不允许重复加锁。 */
        if (!mutex->recursive) {
            /* 返回对象忙，提示调用方避免非递归重入。 */
            return MRT_RESULT_OBJECT_BUSY;
        }

        /* 递归互斥锁后续任务会启用该路径。 */
        mutex->lock_count++;

        /* 递归加锁成功。 */
        return MRT_RESULT_OK;
    }

    /* 非阻塞加锁遇到已有拥有者时立即返回忙。 */
    if (timeout == 0u) {
        /* 当前无法获得互斥锁。 */
        return MRT_RESULT_OBJECT_BUSY;
    }

    /* 查询当前等待者优先级。 */
    MRT_Priority waiter_priority = 0u;
    (void)MRT_TaskGetPriority(current, &waiter_priority);

    /* 查询当前拥有者有效优先级。 */
    MRT_Priority owner_priority = 0u;
    (void)MRT_TaskGetPriority(mutex->owner, &owner_priority);

    /* 高优先级任务等待低优先级拥有者时，提升拥有者有效优先级。 */
    if (waiter_priority > owner_priority) {
        /* 执行优先级继承，降低优先级反转时间。 */
        MRT_TaskKernelSetEffectivePriority(mutex->owner, waiter_priority);
    }

    /* 将当前任务挂入互斥锁等待链表，并设置 tick 超时。 */
    return MRT_TaskKernelBlockCurrentOnObject(&mutex->waiting_lockers,
                                              timeout,
                                              MRT_TASK_WAIT_REASON_MUTEX_LOCK,
                                              MRT_RESULT_TIMEOUT);
}

/**
 * @brief 释放当前任务持有的互斥锁。
 * @param mutex 互斥锁句柄，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示解锁成功；非拥有者解锁返回 MRT_RESULT_OWNER_ERROR；
 *         参数非法返回 MRT_RESULT_INVALID_ARGUMENT；无当前任务返回 MRT_RESULT_INVALID_CONTEXT。
 * @example
 * MRT_MutexUnlock(mutex);
 */
MRT_Result MRT_MutexUnlock(MRT_MutexHandle mutex)
{
    /* 互斥锁句柄不能为空，否则无法读取和修改锁状态。 */
    if (mutex == 0) {
        /* 返回参数错误，提示调用方传入有效互斥锁。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 解锁必须由当前运行任务发起。 */
    MRT_TaskHandle current = MRT_TaskGetCurrent();

    /* 当前任务为空表示调度器尚未运行或当前不在任务上下文。 */
    if (current == 0) {
        /* 返回非法上下文，提示调用方只能在任务上下文解锁。 */
        return MRT_RESULT_INVALID_CONTEXT;
    }

    /* 只有拥有者可以解锁互斥锁。 */
    if (mutex->owner != current) {
        /* 返回所有权错误，互斥锁状态保持不变。 */
        return MRT_RESULT_OWNER_ERROR;
    }

    /* 如果锁定深度大于 1，本次只减少递归深度。 */
    if (mutex->lock_count > 1u) {
        /* 递减锁定深度。 */
        mutex->lock_count--;

        /* 仍由当前任务持有。 */
        return MRT_RESULT_OK;
    }

    /* 保存旧拥有者，用于恢复基础优先级。 */
    MRT_TaskHandle old_owner = mutex->owner;

    /* 如果存在等待者，互斥锁直接转交给最高优先级等待任务。 */
    if (!MRT_ListIsEmpty(&mutex->waiting_lockers)) {
        /* 读取等待链表头部任务，作为新的互斥锁拥有者。 */
        MRT_ListNode *wait_node = MRT_ListGetHead(&mutex->waiting_lockers);

        /* 从等待节点恢复任务句柄。 */
        MRT_TaskHandle next_owner = (MRT_TaskHandle)wait_node->item;

        /* 设置新的拥有者。 */
        mutex->owner = next_owner;

        /* 新拥有者获得一层锁。 */
        mutex->lock_count = 1u;

        /* 旧拥有者释放锁后恢复基础优先级。 */
        MRT_TaskKernelRestoreBasePriority(old_owner);

        /* 唤醒新的拥有者，并允许其按优先级立即抢占。 */
        (void)MRT_TaskKernelWakeFirstObjectWaiter(&mutex->waiting_lockers, MRT_RESULT_OK, true);

        /* 解锁并转交成功。 */
        return MRT_RESULT_OK;
    }

    /* 最后一层解锁且没有等待者时清空拥有者。 */
    mutex->owner = 0;

    /* 清空锁定深度。 */
    mutex->lock_count = 0u;

    /* 没有等待者时同样恢复旧拥有者基础优先级。 */
    MRT_TaskKernelRestoreBasePriority(old_owner);

    /* 解锁成功。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 查询互斥锁当前拥有者。
 * @param mutex 互斥锁句柄，不能为空。
 * @param out_owner 输出拥有者任务句柄，不能为空；无拥有者时写入空指针。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示查询成功；参数非法返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * MRT_TaskHandle owner;
 * MRT_MutexGetOwner(mutex, &owner);
 */
MRT_Result MRT_MutexGetOwner(MRT_MutexHandle mutex, MRT_TaskHandle *out_owner)
{
    /* 互斥锁句柄不能为空，否则无法读取拥有者。 */
    if (mutex == 0) {
        /* 返回参数错误，提示调用方传入有效互斥锁。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 输出指针不能为空，否则无法写回拥有者。 */
    if (out_owner == 0) {
        /* 返回参数错误，提示调用方提供输出地址。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 写回当前拥有者；无拥有者时为空。 */
    *out_owner = mutex->owner;

    /* 查询成功。 */
    return MRT_RESULT_OK;
}
