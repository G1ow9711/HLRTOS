#include "myrtos/mrt_queue.h"

#include <string.h>

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
/**
 * @brief 将一个元素复制发送到队列尾部。
 * @param queue 目标队列句柄，不能为空。
 * @param item 待发送元素地址，指向的数据大小必须至少为创建队列时的 item_size。
 * @param timeout 等待空位的 tick 数；当前阶段仅支持 0，非 0 会返回 MRT_RESULT_TIMEOUT。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示发送成功；队列满且 timeout 为 0 时返回 MRT_RESULT_OBJECT_FULL；
 *         参数非法时返回 MRT_RESULT_INVALID_ARGUMENT；非 0 timeout 暂未接入阻塞等待时返回 MRT_RESULT_TIMEOUT。
 * @example
 * uint32_t value = 0x12345678u;
 * MRT_QueueSend(queue, &value, 0);
 */
MRT_Result MRT_QueueSend(MRT_QueueHandle queue, const void *item, MRT_Timeout timeout)
{
    /* 队列句柄不能为空，否则无法定位队列控制块。 */
    if (queue == 0) {
        /* 返回参数错误，提示调用方传入有效队列句柄。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 待发送元素地址不能为空，否则无法执行按值复制。 */
    if (item == 0) {
        /* 返回参数错误，提示调用方传入有效元素地址。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 如果队列已经写满，则当前没有空槽可保存新元素。 */
    if (queue->count == queue->capacity) {
        /* 非阻塞发送在队列满时立即返回对象已满。 */
        if (timeout == 0u) {
            /* 告诉调用方本次发送没有写入任何数据。 */
            return MRT_RESULT_OBJECT_FULL;
        }

        /* 阻塞等待尚未在本任务中接入，先用超时结果表达未完成。 */
        return MRT_RESULT_TIMEOUT;
    }

    /* 根据写下标计算目标槽位的字节地址。 */
    uint8_t *slot = &queue->buffer[queue->write_index * queue->item_size];

    /* 将调用方元素完整复制到队列内部缓冲区。 */
    memcpy(slot, item, queue->item_size);

    /* 写下标前进一个槽位，到达队尾后回绕到 0。 */
    queue->write_index = (queue->write_index + 1u) % queue->capacity;

    /* 队列中有效元素数量增加 1。 */
    queue->count++;

    /* 本次发送成功完成。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 从队列头部复制接收一个元素。
 * @param queue 源队列句柄，不能为空。
 * @param out_item 接收缓冲区地址，大小必须至少为创建队列时的 item_size。
 * @param timeout 等待数据的 tick 数；当前阶段仅支持 0，非 0 会返回 MRT_RESULT_TIMEOUT。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示接收成功；队列空且 timeout 为 0 时返回 MRT_RESULT_OBJECT_EMPTY；
 *         参数非法时返回 MRT_RESULT_INVALID_ARGUMENT；非 0 timeout 暂未接入阻塞等待时返回 MRT_RESULT_TIMEOUT。
 * @example
 * uint32_t value;
 * MRT_QueueReceive(queue, &value, 0);
 */
MRT_Result MRT_QueueReceive(MRT_QueueHandle queue, void *out_item, MRT_Timeout timeout)
{
    /* 队列句柄不能为空，否则无法定位队列控制块。 */
    if (queue == 0) {
        /* 返回参数错误，提示调用方传入有效队列句柄。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 接收输出缓冲区不能为空，否则无法把队列元素交给调用方。 */
    if (out_item == 0) {
        /* 返回参数错误，提示调用方传入有效输出地址。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 如果队列没有元素，则当前没有数据可以接收。 */
    if (queue->count == 0u) {
        /* 非阻塞接收在队列空时立即返回对象为空。 */
        if (timeout == 0u) {
            /* 告诉调用方本次接收没有读出任何数据。 */
            return MRT_RESULT_OBJECT_EMPTY;
        }

        /* 阻塞等待尚未在本任务中接入，先用超时结果表达未完成。 */
        return MRT_RESULT_TIMEOUT;
    }

    /* 根据读下标计算源槽位的字节地址。 */
    const uint8_t *slot = &queue->buffer[queue->read_index * queue->item_size];

    /* 将队列内部元素完整复制到调用方输出缓冲区。 */
    memcpy(out_item, slot, queue->item_size);

    /* 读下标前进一个槽位，到达队尾后回绕到 0。 */
    queue->read_index = (queue->read_index + 1u) % queue->capacity;

    /* 队列中有效元素数量减少 1。 */
    queue->count--;

    /* 本次接收成功完成。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 复制读取队列头部元素但不将其移出队列。
 * @param queue 源队列句柄，不能为空。
 * @param out_item 接收缓冲区地址，大小必须至少为创建队列时的 item_size。
 * @param timeout 等待数据的 tick 数；当前阶段仅支持 0，非 0 会返回 MRT_RESULT_TIMEOUT。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示读取成功；队列空且 timeout 为 0 时返回 MRT_RESULT_OBJECT_EMPTY；
 *         参数非法时返回 MRT_RESULT_INVALID_ARGUMENT；非 0 timeout 暂未接入阻塞等待时返回 MRT_RESULT_TIMEOUT。
 * @example
 * uint32_t value;
 * MRT_QueuePeek(queue, &value, 0);
 */
MRT_Result MRT_QueuePeek(MRT_QueueHandle queue, void *out_item, MRT_Timeout timeout)
{
    /* 队列句柄不能为空，否则无法定位队列控制块。 */
    if (queue == 0) {
        /* 返回参数错误，提示调用方传入有效队列句柄。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 输出缓冲区不能为空，否则无法复制队头元素。 */
    if (out_item == 0) {
        /* 返回参数错误，提示调用方传入有效输出地址。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 队列为空时没有可读取的队头元素。 */
    if (queue->count == 0u) {
        /* 非阻塞 Peek 在队列空时立即返回对象为空。 */
        if (timeout == 0u) {
            /* 告诉调用方本次没有读到数据。 */
            return MRT_RESULT_OBJECT_EMPTY;
        }

        /* 阻塞等待尚未接入，非零 timeout 暂时返回超时。 */
        return MRT_RESULT_TIMEOUT;
    }

    /* 根据读下标计算当前队头槽位地址。 */
    const uint8_t *slot = &queue->buffer[queue->read_index * queue->item_size];

    /* 复制队头元素，但不修改读下标和元素计数。 */
    memcpy(out_item, slot, queue->item_size);

    /* Peek 成功完成。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 将一个元素复制发送到队列头部。
 * @param queue 目标队列句柄，不能为空。
 * @param item 待发送元素地址，指向的数据大小必须至少为创建队列时的 item_size。
 * @param timeout 等待空位的 tick 数；当前阶段仅支持 0，非 0 会返回 MRT_RESULT_TIMEOUT。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示发送成功；队列满且 timeout 为 0 时返回 MRT_RESULT_OBJECT_FULL；
 *         参数非法时返回 MRT_RESULT_INVALID_ARGUMENT；非 0 timeout 暂未接入阻塞等待时返回 MRT_RESULT_TIMEOUT。
 * @example
 * uint32_t urgent = 1u;
 * MRT_QueueSendFront(queue, &urgent, 0);
 */
MRT_Result MRT_QueueSendFront(MRT_QueueHandle queue, const void *item, MRT_Timeout timeout)
{
    /* 队列句柄不能为空，否则无法定位队列控制块。 */
    if (queue == 0) {
        /* 返回参数错误，提示调用方传入有效队列句柄。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 待发送元素地址不能为空，否则无法执行按值复制。 */
    if (item == 0) {
        /* 返回参数错误，提示调用方传入有效元素地址。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 队列写满时没有空间在队头插入新元素。 */
    if (queue->count == queue->capacity) {
        /* 非阻塞队头发送在队列满时立即返回对象已满。 */
        if (timeout == 0u) {
            /* 告诉调用方本次没有写入数据。 */
            return MRT_RESULT_OBJECT_FULL;
        }

        /* 阻塞等待尚未接入，非零 timeout 暂时返回超时。 */
        return MRT_RESULT_TIMEOUT;
    }

    /* 如果读下标在 0，则向前回绕到最后一个槽位。 */
    if (queue->read_index == 0u) {
        /* 回绕读下标，为队头插入腾出位置。 */
        queue->read_index = queue->capacity - 1u;
    } else {
        /* 普通情况下读下标向前移动一个槽位。 */
        queue->read_index--;
    }

    /* 根据新的读下标计算队头槽位地址。 */
    uint8_t *slot = &queue->buffer[queue->read_index * queue->item_size];

    /* 将元素复制到新的队头槽位。 */
    memcpy(slot, item, queue->item_size);

    /* 队列中有效元素数量增加 1。 */
    queue->count++;

    /* 队头发送成功完成。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 覆盖写入单槽队列。
 * @param queue 目标队列句柄，不能为空，且容量必须为 1。
 * @param item 待覆盖写入元素地址，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示覆盖成功；参数非法或队列容量不是 1 时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * uint32_t latest = adc_sample;
 * MRT_QueueOverwrite(queue, &latest);
 */
MRT_Result MRT_QueueOverwrite(MRT_QueueHandle queue, const void *item)
{
    /* 队列句柄不能为空，否则无法定位队列控制块。 */
    if (queue == 0) {
        /* 返回参数错误，提示调用方传入有效队列句柄。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 待覆盖元素地址不能为空，否则无法执行按值复制。 */
    if (item == 0) {
        /* 返回参数错误，提示调用方传入有效元素地址。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 覆盖写入只允许用于单槽队列，避免多槽队列丢失 FIFO 语义。 */
    if (queue->capacity != 1u) {
        /* 多槽队列调用覆盖写入属于 API 使用错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 将最新值复制到唯一槽位。 */
    memcpy(&queue->buffer[0], item, queue->item_size);

    /* 单槽队列读下标始终复位到唯一槽位。 */
    queue->read_index = 0u;

    /* 单槽队列写下标也保持在唯一槽位。 */
    queue->write_index = 0u;

    /* 覆盖后队列必定包含一个最新元素。 */
    queue->count = 1u;

    /* 覆盖写入成功完成。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 清空队列中的全部元素并复位读写位置。
 * @param queue 目标队列句柄，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示复位成功；参数非法时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * MRT_QueueReset(queue);
 */
MRT_Result MRT_QueueReset(MRT_QueueHandle queue)
{
    /* 队列句柄不能为空，否则无法定位队列控制块。 */
    if (queue == 0) {
        /* 返回参数错误，提示调用方传入有效队列句柄。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 将读下标复位到缓冲区起点。 */
    queue->read_index = 0u;

    /* 将写下标复位到缓冲区起点。 */
    queue->write_index = 0u;

    /* 清空有效元素计数，旧数据字节保留但不再可读。 */
    queue->count = 0u;

    /* 队列复位成功完成。 */
    return MRT_RESULT_OK;
}

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
