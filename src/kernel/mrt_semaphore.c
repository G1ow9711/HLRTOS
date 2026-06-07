#include "myrtos/mrt_semaphore.h"

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
