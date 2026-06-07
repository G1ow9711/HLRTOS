#include "myrtos/mrt_queue.h"

/**
 * @brief 使用调用方提供的控制块和缓冲区静态创建队列。
 * @param capacity 队列容量，表示最多保存多少个元素，必须大于 0。
 * @param item_size 每个元素的字节数，必须大于 0。
 * @param buffer 队列数据缓冲区，大小至少为 capacity * item_size，不能为空。
 * @param storage 队列控制块存储，不能为空。
 * @param out_queue 输出队列句柄，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示创建成功；参数非法时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * static MRT_Queue queue_cb;
 * static uint8_t queue_buffer[8 * sizeof(uint32_t)];
 * MRT_QueueHandle queue;
 * MRT_QueueCreateStatic(8, sizeof(uint32_t), queue_buffer, &queue_cb, &queue);
 */
MRT_Result MRT_QueueCreateStatic(size_t capacity,
                                 size_t item_size,
                                 void *buffer,
                                 MRT_Queue *storage,
                                 MRT_QueueHandle *out_queue)
{
    /* 队列容量必须大于 0，否则队列永远无法存放元素。 */
    if (capacity == 0u) {
        /* 返回参数错误，提示调用方提供有效容量。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 元素大小必须大于 0，否则复制队列无法计算元素位置。 */
    if (item_size == 0u) {
        /* 返回参数错误，提示调用方提供有效元素大小。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 数据缓冲区不能为空。 */
    if (buffer == 0) {
        /* 返回参数错误，提示调用方提供队列存储区。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 队列控制块不能为空。 */
    if (storage == 0) {
        /* 返回参数错误，提示调用方提供控制块存储。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 输出句柄不能为空，避免创建成功后调用方无法使用队列。 */
    if (out_queue == 0) {
        /* 返回参数错误，提示调用方提供输出句柄地址。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 保存队列数据缓冲区地址。 */
    storage->buffer = (uint8_t *)buffer;

    /* 保存队列容量。 */
    storage->capacity = capacity;

    /* 保存单个元素大小。 */
    storage->item_size = item_size;

    /* 初始化读取下标为 0。 */
    storage->read_index = 0u;

    /* 初始化写入下标为 0。 */
    storage->write_index = 0u;

    /* 初始化已有元素数量为 0。 */
    storage->count = 0u;

    /* 初始化等待发送者链表。 */
    MRT_ListInitialize(&storage->waiting_senders);

    /* 初始化等待接收者链表。 */
    MRT_ListInitialize(&storage->waiting_receivers);

    /* 标记队列使用静态存储。 */
    storage->static_storage = true;

    /* 输出队列句柄。 */
    *out_queue = storage;

    /* 静态队列创建成功。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 查询队列中已有元素数量。
 * @param queue 待查询队列句柄，不能为空。
 * @return size_t 返回当前队列中的元素数量；队列为空指针时返回 0。
 * @example
 * size_t used = MRT_QueueMessagesWaiting(queue);
 */
size_t MRT_QueueMessagesWaiting(MRT_QueueHandle queue)
{
    /* 空队列句柄没有可查询对象，返回 0。 */
    if (queue == 0) {
        /* 返回 0 表示无消息。 */
        return 0u;
    }

    /* 返回队列当前元素数量。 */
    return queue->count;
}

/**
 * @brief 查询队列剩余可写空间。
 * @param queue 待查询队列句柄，不能为空。
 * @return size_t 返回剩余可写元素数量；队列为空指针时返回 0。
 * @example
 * size_t free_slots = MRT_QueueSpacesAvailable(queue);
 */
size_t MRT_QueueSpacesAvailable(MRT_QueueHandle queue)
{
    /* 空队列句柄没有可查询对象，返回 0。 */
    if (queue == 0) {
        /* 返回 0 表示无空间可用。 */
        return 0u;
    }

    /* 用容量减去当前元素数量得到剩余空间。 */
    return queue->capacity - queue->count;
}
