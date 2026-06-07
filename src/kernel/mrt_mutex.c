#include "myrtos/mrt_mutex.h"

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

    /* 输出互斥锁句柄给调用方。 */
    *out_mutex = storage;

    /* 静态互斥锁创建成功。 */
    return MRT_RESULT_OK;
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

    /* 阻塞和优先级继承由后续任务接入；当前先返回超时。 */
    return MRT_RESULT_TIMEOUT;
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

    /* 最后一层解锁后清空拥有者。 */
    mutex->owner = 0;

    /* 清空锁定深度。 */
    mutex->lock_count = 0u;

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
