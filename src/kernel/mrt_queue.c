#include "myrtos/mrt_queue.h"
#include "myrtos/mrt_config.h"
#include "myrtos/mrt_heap.h"
#include "myrtos/mrt_kernel.h"
#include "myrtos/mrt_port.h"
#include "myrtos/mrt_trace.h"
#include "mrt_task_internal.h"

#include <string.h>

/**
 * @brief 发布队列 trace 事件。
 * @param kind 队列事件类型，必须是发送或接收事件。
 * @param queue 队列句柄，不能为空。
 * @param result 队列操作结果。
 * @return void 无返回值。
 * @example
 * MRT_QueueTraceEvent(MRT_TRACE_EVENT_QUEUE_SEND, queue, MRT_RESULT_OK);
 */
static void MRT_QueueTraceEvent(MRT_TraceEventKind kind, MRT_QueueHandle queue, MRT_Result result)
{
    /* 队列句柄为空时没有可观察对象，直接忽略。 */
    if (queue == 0) {
        /* 不发布不完整事件。 */
        return;
    }

    /* 构造 trace 事件快照。 */
    MRT_TraceEvent event = {0};

    /* 写入事件类型。 */
    event.kind = kind;

    /* 记录事件发生时的系统 tick。 */
    event.tick = MRT_KernelGetTick();

    /* 队列事件的主任务是当前任务；未启动调度器时允许为空。 */
    event.task = MRT_TaskGetCurrent();

    /* 队列事件没有关联任务。 */
    event.related_task = 0;

    /* 保存关联队列对象。 */
    event.object = queue;

    /* 保存操作后的队列元素数量，便于 trace 后端观察水位。 */
    event.value = (uint32_t)queue->count;

    /* 保存操作结果。 */
    event.result = result;

    /* 发布事件；未设置 sink 时该调用会静默返回。 */
    MRT_TraceEmit(&event);
}

/**
 * @brief 将字节数向上规整到队列动态分配对齐粒度。
 * @param size 原始字节数。
 * @return size_t 返回对齐后的字节数；溢出时返回 0。
 * @example
 * size_t aligned = MRT_QueueAlignSizeUp(sizeof(MRT_Queue));
 */
static size_t MRT_QueueAlignSizeUp(size_t size)
{
    /* 队列动态分配复用堆对齐配置。 */
    size_t alignment = MRT_CFG_HEAP_ALIGNMENT;

    /* 对齐配置为 0 时无法计算有效布局。 */
    if (alignment == 0u) {
        /* 返回 0 表示布局失败。 */
        return 0u;
    }

    /* 加法前检查是否会溢出。 */
    if (size > (SIZE_MAX - (alignment - 1u))) {
        /* 返回 0 表示请求过大。 */
        return 0u;
    }

    /* 使用除法形式规整，避免对配置值做额外幂次假设。 */
    return ((size + alignment - 1u) / alignment) * alignment;
}

/**
 * @brief 计算动态队列数据区字节数。
 * @param item_size 每个元素的字节数。
 * @param capacity 队列容量。
 * @param out_bytes 输出数据区字节数。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示计算成功；参数非法或乘法溢出时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * MRT_QueueCalculateBufferBytes(item_size, capacity, &bytes);
 */
static MRT_Result MRT_QueueCalculateBufferBytes(size_t item_size,
                                                size_t capacity,
                                                size_t *out_bytes)
{
    /* 输出指针不能为空。 */
    if (out_bytes == 0) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 元素大小和容量都必须大于 0。 */
    if ((item_size == 0u) || (capacity == 0u)) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 检查 capacity * item_size 是否会溢出。 */
    if (capacity > (SIZE_MAX / item_size)) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 写回数据区总字节数。 */
    *out_bytes = capacity * item_size;

    /* 计算成功。 */
    return MRT_RESULT_OK;
}

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
/**
 * @brief 从 MyRTOS 全局堆动态创建队列。
 * @param item_size 每个元素的字节数，必须大于 0。
 * @param capacity 队列容量，表示最多保存多少个元素，必须大于 0。
 * @param out_queue 输出队列句柄，不能为 NULL；创建失败时写入 NULL。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示创建成功；参数非法时返回 MRT_RESULT_INVALID_ARGUMENT；
 *         动态分配被关闭或堆空间不足时返回 MRT_RESULT_NO_MEMORY。
 * @example
 * MRT_QueueHandle queue;
 * MRT_QueueCreate(sizeof(uint32_t), 8, &queue);
 */
MRT_Result MRT_QueueCreate(size_t item_size, size_t capacity, MRT_QueueHandle *out_queue)
{
    /* 输出句柄不能为空。 */
    if (out_queue == 0) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 失败路径默认清空输出句柄，避免调用方误用旧值。 */
    *out_queue = 0;

    /* 动态分配关闭时不能创建堆队列。 */
    if (MRT_CFG_SUPPORT_DYNAMIC_ALLOCATION == 0u) {
        /* 返回内存不足，表达当前系统不提供动态对象空间。 */
        return MRT_RESULT_NO_MEMORY;
    }

    /* 计算队列数据区大小并验证 item_size/capacity。 */
    size_t buffer_bytes = 0u;
    MRT_Result result = MRT_QueueCalculateBufferBytes(item_size, capacity, &buffer_bytes);

    /* 计算失败说明参数非法或乘法溢出。 */
    if (result != MRT_RESULT_OK) {
        /* 返回参数错误。 */
        return result;
    }

    /* 控制块之后要放队列数据区，所以控制块大小需要向上对齐。 */
    size_t control_bytes = MRT_QueueAlignSizeUp(sizeof(MRT_Queue));

    /* 对齐失败时视为无法动态分配。 */
    if (control_bytes == 0u) {
        /* 返回内存不足。 */
        return MRT_RESULT_NO_MEMORY;
    }

    /* 检查控制块大小与数据区大小相加是否溢出。 */
    if (buffer_bytes > (SIZE_MAX - control_bytes)) {
        /* 返回内存不足，表示请求布局超过可表示范围。 */
        return MRT_RESULT_NO_MEMORY;
    }

    /* 动态队列使用一个堆块同时保存控制块和数据区。 */
    size_t total_bytes = control_bytes + buffer_bytes;

    /* 从 MyRTOS 堆分配完整队列内存。 */
    void *memory = MRT_Malloc(total_bytes);

    /* 堆空间不足时创建失败。 */
    if (memory == 0) {
        /* 返回内存不足。 */
        return MRT_RESULT_NO_MEMORY;
    }

    /* 队列控制块位于堆块起始处。 */
    MRT_Queue *queue = (MRT_Queue *)memory;

    /* 数据区紧跟对齐后的控制块。 */
    uint8_t *buffer = ((uint8_t *)memory) + control_bytes;

    /* 复用静态创建逻辑初始化控制块和等待链表。 */
    result = MRT_QueueCreateStatic(capacity, item_size, buffer, queue, out_queue);

    /* 理论上参数已验证，但仍处理初始化失败路径。 */
    if (result != MRT_RESULT_OK) {
        /* 归还刚分配的堆块。 */
        (void)MRT_Free(memory);

        /* 清空输出句柄。 */
        *out_queue = 0;

        /* 返回静态初始化给出的错误。 */
        return result;
    }

    /* 标记该队列归动态堆所有，允许 MRT_QueueDelete 释放。 */
    queue->static_storage = false;

    /* 动态队列创建成功。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 删除动态创建的队列并归还其堆内存。
 * @param queue 待删除队列句柄，不能为 NULL。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示删除成功；空句柄返回 MRT_RESULT_INVALID_ARGUMENT；
 *         静态队列不是堆对象，返回 MRT_RESULT_OBJECT_BUSY。
 * @example
 * MRT_QueueDelete(queue);
 */
MRT_Result MRT_QueueDelete(MRT_QueueHandle queue)
{
    /* 队列句柄不能为空。 */
    if (queue == 0) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 静态队列的内存由调用方管理，不能由动态删除 API 释放。 */
    if (queue->static_storage) {
        /* 返回对象忙，表示该对象不归堆释放路径所有。 */
        return MRT_RESULT_OBJECT_BUSY;
    }

    /* 动态队列控制块就是 MRT_Malloc 返回的堆块起始地址。 */
    return MRT_Free(queue);
}

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

    /* 如果有任务正在等待接收该队列，唤醒最高优先级等待者并立即重调度。 */
    (void)MRT_TaskKernelWakeFirstObjectWaiter(&queue->waiting_receivers, MRT_RESULT_OK, true);

    /* 本次发送成功完成。 */
    /* 发布队列发送成功 trace，value 保存发送后的队列元素数量。 */
    MRT_QueueTraceEvent(MRT_TRACE_EVENT_QUEUE_SEND, queue, MRT_RESULT_OK);

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

        /* 将当前任务挂入队列接收等待链表，并设置 tick 超时。 */
        return MRT_TaskKernelBlockCurrentOnObject(&queue->waiting_receivers,
                                                  timeout,
                                                  MRT_TASK_WAIT_REASON_QUEUE_RECEIVE,
                                                  MRT_RESULT_TIMEOUT);
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
    /* 发布队列接收成功 trace，value 保存接收后的队列元素数量。 */
    MRT_QueueTraceEvent(MRT_TRACE_EVENT_QUEUE_RECEIVE, queue, MRT_RESULT_OK);

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

/**
 * @brief 在 ISR 上下文将一个元素复制发送到队列尾部。
 * @param queue 目标队列句柄，不能为空。
 * @param item 待发送元素地址，不能为空。
 * @param should_yield 输出是否需要在 ISR 退出前触发调度切换；允许为空，当前阶段非空时总写入 false。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示发送成功；队列满时返回 MRT_RESULT_OBJECT_FULL；
 *         参数非法时返回 MRT_RESULT_INVALID_ARGUMENT；非 ISR 上下文调用时返回 MRT_RESULT_INVALID_CONTEXT。
 * @example
 * bool yield;
 * MRT_QueueSendFromISR(queue, &value, &yield);
 */
MRT_Result MRT_QueueSendFromISR(MRT_QueueHandle queue, const void *item, bool *should_yield)
{
    /* 如果调用方提供 yield 输出指针，先写入保守的 false 默认值。 */
    if (should_yield != 0) {
        /* 当前尚未接入等待任务唤醒，所以不会请求 ISR 退出切换。 */
        *should_yield = false;
    }

    /* FromISR API 必须在 ISR 上下文调用。 */
    if (!MRT_PortIsInsideISR()) {
        /* 返回非法上下文，提示调用方改用任务上下文 API。 */
        return MRT_RESULT_INVALID_CONTEXT;
    }

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

    /* ISR 发送不能等待空间，队列满时立即返回。 */
    if (queue->count == queue->capacity) {
        /* 返回对象已满，调用方可在下次中断或任务上下文重试。 */
        return MRT_RESULT_OBJECT_FULL;
    }

    /* 根据写下标计算目标槽位的字节地址。 */
    uint8_t *slot = &queue->buffer[queue->write_index * queue->item_size];

    /* 将调用方元素完整复制到队列内部缓冲区。 */
    memcpy(slot, item, queue->item_size);

    /* 写下标前进一个槽位，到达队尾后回绕到 0。 */
    queue->write_index = (queue->write_index + 1u) % queue->capacity;

    /* 队列中有效元素数量增加 1。 */
    queue->count++;

    /* ISR 上下文只让等待接收任务 ready，不在函数内部直接切换当前任务。 */
    bool woke_receiver = MRT_TaskKernelWakeFirstObjectWaiter(&queue->waiting_receivers, MRT_RESULT_OK, false);

    /* 如果唤醒了接收任务，需要通知端口层在 ISR 退出时请求调度。 */
    if ((should_yield != 0) && woke_receiver) {
        /* 写入 true，调用方随后可传给 MRT_PortYieldFromISR。 */
        *should_yield = true;
    }

    /* ISR 发送成功完成。 */
    /* 发布 ISR 队列发送成功 trace，value 保存发送后的队列元素数量。 */
    MRT_QueueTraceEvent(MRT_TRACE_EVENT_QUEUE_SEND, queue, MRT_RESULT_OK);

    return MRT_RESULT_OK;
}

/**
 * @brief 在 ISR 上下文从队列头部复制接收一个元素。
 * @param queue 源队列句柄，不能为空。
 * @param out_item 接收缓冲区地址，不能为空。
 * @param should_yield 输出是否需要在 ISR 退出前触发调度切换；允许为空，当前阶段非空时总写入 false。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示接收成功；队列空时返回 MRT_RESULT_OBJECT_EMPTY；
 *         参数非法时返回 MRT_RESULT_INVALID_ARGUMENT；非 ISR 上下文调用时返回 MRT_RESULT_INVALID_CONTEXT。
 * @example
 * bool yield;
 * MRT_QueueReceiveFromISR(queue, &value, &yield);
 */
MRT_Result MRT_QueueReceiveFromISR(MRT_QueueHandle queue, void *out_item, bool *should_yield)
{
    /* 如果调用方提供 yield 输出指针，先写入保守的 false 默认值。 */
    if (should_yield != 0) {
        /* 当前尚未接入等待发送任务唤醒，所以不会请求 ISR 退出切换。 */
        *should_yield = false;
    }

    /* FromISR API 必须在 ISR 上下文调用。 */
    if (!MRT_PortIsInsideISR()) {
        /* 返回非法上下文，提示调用方改用任务上下文 API。 */
        return MRT_RESULT_INVALID_CONTEXT;
    }

    /* 复用普通非阻塞接收逻辑，timeout 固定为 0。 */
    return MRT_QueueReceive(queue, out_item, 0u);
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
