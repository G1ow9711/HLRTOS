#include "myrtos/mrt_semaphore.h"
#include "myrtos/mrt_port.h"
#include "mrt_task_internal.h"

/**
 * @brief 使用调用方提供的控制块静态创建二值信号量。
 * @param initially_available true 表示初始计数为 1，false 表示初始计数为 0。
 * @param storage 信号量控制块存储，不能为空。
 * @param out_semaphore 输出信号量句柄，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示创建成功；参数非法时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * static MRT_Semaphore sem_cb;
 * MRT_SemaphoreHandle sem;
 * MRT_SemaphoreCreateBinaryStatic(true, &sem_cb, &sem);
 */
MRT_Result MRT_SemaphoreCreateBinaryStatic(bool initially_available,
                                           MRT_Semaphore *storage,
                                           MRT_SemaphoreHandle *out_semaphore)
{
    /* 信号量控制块不能为空，否则无法保存对象状态。 */
    if (storage == 0) {
        /* 返回参数错误，提示调用方提供静态控制块。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 输出句柄不能为空，否则创建成功后调用方无法使用对象。 */
    if (out_semaphore == 0) {
        /* 返回参数错误，提示调用方提供输出句柄地址。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 二值信号量最大计数固定为 1。 */
    storage->max_count = 1u;

    /* 根据初始可用标志设置当前计数。 */
    storage->count = initially_available ? 1u : 0u;

    /* 初始化等待获取该信号量的任务链表。 */
    MRT_ListInitialize(&storage->waiting_takers);

    /* 标记对象使用静态存储创建。 */
    storage->static_storage = true;

    /* 输出信号量句柄给调用方。 */
    *out_semaphore = storage;

    /* 静态二值信号量创建成功。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 使用调用方提供的控制块静态创建计数信号量。
 * @param max_count 最大计数，必须大于 0。
 * @param initial_count 初始计数，必须小于或等于 max_count。
 * @param storage 信号量控制块存储，不能为空。
 * @param out_semaphore 输出信号量句柄，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示创建成功；参数非法时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * static MRT_Semaphore sem_cb;
 * MRT_SemaphoreHandle sem;
 * MRT_SemaphoreCreateCountingStatic(4, 2, &sem_cb, &sem);
 */
MRT_Result MRT_SemaphoreCreateCountingStatic(size_t max_count,
                                             size_t initial_count,
                                             MRT_Semaphore *storage,
                                             MRT_SemaphoreHandle *out_semaphore)
{
    /* 最大计数必须大于 0，否则信号量永远不可获取。 */
    if (max_count == 0u) {
        /* 返回参数错误，提示调用方提供有效最大计数。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 初始计数不能超过最大计数。 */
    if (initial_count > max_count) {
        /* 返回参数错误，提示调用方修正初始计数。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 信号量控制块不能为空，否则无法保存对象状态。 */
    if (storage == 0) {
        /* 返回参数错误，提示调用方提供静态控制块。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 输出句柄不能为空，否则创建成功后调用方无法使用对象。 */
    if (out_semaphore == 0) {
        /* 返回参数错误，提示调用方提供输出句柄地址。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 保存最大计数。 */
    storage->max_count = max_count;

    /* 保存当前计数。 */
    storage->count = initial_count;

    /* 初始化等待获取该信号量的任务链表。 */
    MRT_ListInitialize(&storage->waiting_takers);

    /* 标记对象使用静态存储创建。 */
    storage->static_storage = true;

    /* 输出信号量句柄给调用方。 */
    *out_semaphore = storage;

    /* 静态计数信号量创建成功。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 查询信号量当前计数。
 * @param semaphore 待查询信号量句柄，不能为空。
 * @return size_t 返回当前计数；信号量句柄为空时返回 0。
 * @example
 * size_t count = MRT_SemaphoreGetCount(sem);
 */
size_t MRT_SemaphoreGetCount(MRT_SemaphoreHandle semaphore)
{
    /* 空信号量句柄没有可查询对象。 */
    if (semaphore == 0) {
        /* 返回 0 表示无可用计数。 */
        return 0u;
    }

    /* 返回信号量当前计数。 */
    return semaphore->count;
}

/**
 * @brief 获取一个信号量计数。
 * @param semaphore 信号量句柄，不能为空。
 * @param timeout 等待可用计数的 tick 数；当前非调度上下文下非 0 会返回 MRT_RESULT_TIMEOUT。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示获取成功；非阻塞空信号量返回 MRT_RESULT_OBJECT_EMPTY；
 *         参数非法返回 MRT_RESULT_INVALID_ARGUMENT；等待未完成返回 MRT_RESULT_TIMEOUT。
 * @example
 * MRT_SemaphoreTake(sem, 0);
 */
MRT_Result MRT_SemaphoreTake(MRT_SemaphoreHandle semaphore, MRT_Timeout timeout)
{
    /* 信号量句柄不能为空，否则无法读取和修改计数。 */
    if (semaphore == 0) {
        /* 返回参数错误，提示调用方传入有效信号量。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 当前计数大于 0 表示信号量可立即获取。 */
    if (semaphore->count > 0u) {
        /* 消耗一个可用计数。 */
        semaphore->count--;

        /* 获取成功。 */
        return MRT_RESULT_OK;
    }

    /* 非阻塞获取空信号量时立即返回对象为空。 */
    if (timeout == 0u) {
        /* 告诉调用方本次没有获取到计数。 */
        return MRT_RESULT_OBJECT_EMPTY;
    }

    /* 将当前任务挂入信号量等待链表，并设置 tick 超时。 */
    return MRT_TaskKernelBlockCurrentOnObject(&semaphore->waiting_takers,
                                              timeout,
                                              MRT_TASK_WAIT_REASON_SEMAPHORE_TAKE,
                                              MRT_RESULT_TIMEOUT);
}

/**
 * @brief 释放一个信号量计数。
 * @param semaphore 信号量句柄，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示释放成功；计数已满返回 MRT_RESULT_OBJECT_FULL；
 *         参数非法返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * MRT_SemaphoreGive(sem);
 */
MRT_Result MRT_SemaphoreGive(MRT_SemaphoreHandle semaphore)
{
    /* 信号量句柄不能为空，否则无法读取和修改计数。 */
    if (semaphore == 0) {
        /* 返回参数错误，提示调用方传入有效信号量。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 如果已有任务等待获取该信号量，本次释放直接交给最高优先级等待者。 */
    if (!MRT_ListIsEmpty(&semaphore->waiting_takers)) {
        /* 唤醒等待任务并立即按优先级重调度；计数不增加，因为令牌被等待者消费。 */
        (void)MRT_TaskKernelWakeFirstObjectWaiter(&semaphore->waiting_takers, MRT_RESULT_OK, true);

        /* 释放给等待任务成功完成。 */
        return MRT_RESULT_OK;
    }

    /* 当前计数达到最大计数时不能继续释放。 */
    if (semaphore->count == semaphore->max_count) {
        /* 返回对象已满，提示调用方释放次数超过获取次数或资源容量。 */
        return MRT_RESULT_OBJECT_FULL;
    }

    /* 增加一个可用计数。 */
    semaphore->count++;

    /* 释放成功。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 在 ISR 上下文释放一个信号量计数。
 * @param semaphore 信号量句柄，不能为空。
 * @param should_yield 输出是否需要在 ISR 退出前触发调度切换；允许为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示释放成功；计数已满返回 MRT_RESULT_OBJECT_FULL；
 *         参数非法返回 MRT_RESULT_INVALID_ARGUMENT；非 ISR 上下文返回 MRT_RESULT_INVALID_CONTEXT。
 * @example
 * bool yield;
 * MRT_SemaphoreGiveFromISR(sem, &yield);
 */
MRT_Result MRT_SemaphoreGiveFromISR(MRT_SemaphoreHandle semaphore, bool *should_yield)
{
    /* 如果调用方提供 yield 输出指针，先写入保守的 false 默认值。 */
    if (should_yield != 0) {
        /* 默认不请求 ISR 退出后的任务切换。 */
        *should_yield = false;
    }

    /* FromISR API 必须在 ISR 上下文调用。 */
    if (!MRT_PortIsInsideISR()) {
        /* 返回非法上下文，提示调用方改用任务上下文 API。 */
        return MRT_RESULT_INVALID_CONTEXT;
    }

    /* 信号量句柄不能为空，否则无法读取和修改计数。 */
    if (semaphore == 0) {
        /* 返回参数错误，提示调用方传入有效信号量。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 如果已有任务等待获取该信号量，本次释放直接交给等待者。 */
    if (!MRT_ListIsEmpty(&semaphore->waiting_takers)) {
        /* ISR 中只让等待任务 ready，不立即切换当前任务。 */
        bool woke_task = MRT_TaskKernelWakeFirstObjectWaiter(&semaphore->waiting_takers, MRT_RESULT_OK, false);

        /* 唤醒等待任务时，请求端口层在 ISR 退出后调度。 */
        if ((should_yield != 0) && woke_task) {
            /* 写入 true，调用方可传给 MRT_PortYieldFromISR。 */
            *should_yield = true;
        }

        /* 令牌已转交给等待任务。 */
        return MRT_RESULT_OK;
    }

    /* 当前计数达到最大计数时不能继续释放。 */
    if (semaphore->count == semaphore->max_count) {
        /* 返回对象已满。 */
        return MRT_RESULT_OBJECT_FULL;
    }

    /* 增加一个可用计数。 */
    semaphore->count++;

    /* ISR give 成功。 */
    return MRT_RESULT_OK;
}
