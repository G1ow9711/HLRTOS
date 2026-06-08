#include "myrtos/mrt_port.h"
#include "myrtos/mrt_config.h"
#include "myrtos/mrt_heap.h"
#include "myrtos/mrt_stream_buffer.h"
#include "mrt_task_internal.h"

#include <stdint.h>

/**
 * @brief 返回两个 size_t 值中的较小值。
 * @param left 第一个数值。
 * @param right 第二个数值。
 * @return size_t 返回较小的数值。
 * @example
 * size_t count = MRT_StreamMin(requested, available);
 */
static size_t MRT_StreamMin(size_t left, size_t right)
{
    /* 左值小于右值时返回左值，否则返回右值。 */
    return (left < right) ? left : right;
}

/**
 * @brief 将字节数向上规整到堆对齐粒度。
 * @param size 原始字节数。
 * @return size_t 返回规整后的字节数；溢出时返回 0。
 * @example
 * size_t aligned = MRT_StreamAlignSizeUp(sizeof(MRT_StreamBuffer));
 */
static size_t MRT_StreamAlignSizeUp(size_t size)
{
    /* 计算对齐掩码。 */
    const size_t mask = (size_t)MRT_CFG_HEAP_ALIGNMENT - 1u;

    /* 检查向上取整加法是否溢出。 */
    if (size > (SIZE_MAX - mask)) {
        /* 返回 0 表示无法表示。 */
        return 0u;
    }

    /* 使用掩码执行向上对齐。 */
    return (size + mask) & ~mask;
}

/**
 * @brief 推进环形缓冲索引。
 * @param stream 流缓冲句柄，不能为空。
 * @param index 当前索引。
 * @return size_t 返回推进一个字节后的索引，必要时回绕到 0。
 * @example
 * stream->write_index = MRT_StreamNextIndex(stream, stream->write_index);
 */
static size_t MRT_StreamNextIndex(MRT_StreamBufferHandle stream, size_t index)
{
    /* 索引先向后移动一个字节位置。 */
    index++;

    /* 如果索引到达容量末尾之后，则回绕到起点。 */
    if (index >= stream->capacity) {
        /* 环形缓冲从 0 继续写入或读取。 */
        index = 0u;
    }

    /* 返回推进后的索引。 */
    return index;
}

/**
 * @brief 向流缓冲内部写入指定数量字节。
 * @param stream 目标流缓冲句柄，不能为空。
 * @param data 待写入字节指针，不能为空。
 * @param length 实际写入字节数。
 * @return void 无返回值。
 * @example
 * MRT_StreamWriteBytes(stream, data, writable);
 */
static void MRT_StreamWriteBytes(MRT_StreamBufferHandle stream, const uint8_t *data, size_t length)
{
    /* 逐字节写入，保持逻辑简单并天然支持环绕。 */
    for (size_t index = 0u; index < length; index++) {
        /* 将当前输入字节复制到写索引位置。 */
        stream->buffer[stream->write_index] = data[index];

        /* 推进写索引，必要时回绕。 */
        stream->write_index = MRT_StreamNextIndex(stream, stream->write_index);

        /* 已使用字节数增加。 */
        stream->bytes_used++;
    }
}

/**
 * @brief 从流缓冲内部读取指定数量字节。
 * @param stream 源流缓冲句柄，不能为空。
 * @param out_data 输出字节指针，不能为空。
 * @param length 实际读取字节数。
 * @return void 无返回值。
 * @example
 * MRT_StreamReadBytes(stream, out, readable);
 */
static void MRT_StreamReadBytes(MRT_StreamBufferHandle stream, uint8_t *out_data, size_t length)
{
    /* 逐字节读取，保持 FIFO 顺序并天然支持环绕。 */
    for (size_t index = 0u; index < length; index++) {
        /* 从当前读索引位置复制一个字节。 */
        out_data[index] = stream->buffer[stream->read_index];

        /* 推进读索引，必要时回绕。 */
        stream->read_index = MRT_StreamNextIndex(stream, stream->read_index);

        /* 已使用字节数减少。 */
        stream->bytes_used--;
    }
}

/**
 * @brief 在写入后按触发水位唤醒等待读者。
 * @param stream 流缓冲句柄，不能为空。
 * @param switch_now true 表示任务上下文立即重选调度，false 表示 ISR 延后切换。
 * @return bool 返回 true 表示唤醒了一个读者，返回 false 表示没有读者被唤醒。
 * @example
 * bool woke = MRT_StreamWakeReaderIfTriggered(stream, true);
 */
static bool MRT_StreamWakeReaderIfTriggered(MRT_StreamBufferHandle stream, bool switch_now)
{
    /* 未达到触发水位时不唤醒读者。 */
    if (stream->bytes_used < stream->trigger_level) {
        /* 没有满足唤醒条件。 */
        return false;
    }

    /* 等待读者链表为空时没有任务可唤醒。 */
    if (MRT_ListIsEmpty(&stream->waiting_readers)) {
        /* 没有读者等待。 */
        return false;
    }

    /* 唤醒最高优先级等待读者。 */
    return MRT_TaskKernelWakeFirstObjectWaiter(&stream->waiting_readers, MRT_RESULT_OK, switch_now);
}

/**
 * @brief 在流缓冲释放出可写空间后唤醒一个等待写者。
 * @param stream 流缓冲句柄，不能为空。
 * @param switch_now true 表示任务上下文立即重选调度，false 表示延后到 ISR 退出。
 * @return bool 返回 true 表示唤醒了一个写者，返回 false 表示没有满足条件的写者。
 * @example
 * bool woke = MRT_StreamWakeWriterIfSpace(stream, true);
 */
static bool MRT_StreamWakeWriterIfSpace(MRT_StreamBufferHandle stream, bool switch_now)
{
    /* 没有等待写者时无需唤醒。 */
    if (MRT_ListIsEmpty(&stream->waiting_writers)) {
        /* 没有任务等待可写空间。 */
        return false;
    }

    /* 计算当前可写空间。 */
    size_t spaces = stream->capacity - stream->bytes_used;

    /* 没有空间时无法唤醒写者。 */
    if (spaces == 0u) {
        /* 保持写者阻塞状态。 */
        return false;
    }

    /* 头部等待者是最高优先级写者。 */
    MRT_ListNode *head = MRT_ListGetHead(&stream->waiting_writers);
    if (head == 0) {
        /* 链表状态异常时保守不唤醒。 */
        return false;
    }

    /* 从等待节点恢复任务控制块。 */
    MRT_Task *task = (MRT_Task *)head->item;
    if (task == 0) {
        /* 节点内容异常时保守不唤醒。 */
        return false;
    }

    /* 流缓冲写者至少需要 1 字节空间才能重新尝试。 */
    size_t required = task->object_wait_bytes;
    if (required == 0u) {
        /* 未记录请求时按 1 字节处理。 */
        required = 1u;
    }

    /* 仍不满足头部写者需求时保持等待状态。 */
    if (spaces < required) {
        /* 空间不足。 */
        return false;
    }

    /* 唤醒最高优先级等待写者。 */
    return MRT_TaskKernelWakeFirstObjectWaiter(&stream->waiting_writers, MRT_RESULT_OK, switch_now);
}

/**
 * @brief 使用调用方提供的控制块和字节存储静态创建流缓冲。
 * @param capacity 字节存储容量，单位为字节，必须大于 0。
 * @param trigger_level 读者唤醒触发水位，必须在 1 到 capacity 之间。
 * @param buffer 底层字节存储，大小至少为 capacity，不能为空。
 * @param storage 流缓冲控制块存储，不能为空。
 * @param out_stream 输出流缓冲句柄，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示创建成功；参数非法时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * static MRT_StreamBuffer stream_cb;
 * static uint8_t stream_storage[64];
 * MRT_StreamBufferHandle stream;
 * MRT_StreamBufferCreateStatic(64, 8, stream_storage, &stream_cb, &stream);
 */
MRT_Result MRT_StreamBufferCreateStatic(size_t capacity,
                                        size_t trigger_level,
                                        void *buffer,
                                        MRT_StreamBuffer *storage,
                                        MRT_StreamBufferHandle *out_stream)
{
    /* 容量必须大于 0，否则缓冲区永远无法保存字节。 */
    if (capacity == 0u) {
        /* 返回参数错误，提示调用方提供有效字节容量。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 触发水位必须大于 0，否则读者等待条件没有明确语义。 */
    if (trigger_level == 0u) {
        /* 返回参数错误，提示调用方提供有效触发水位。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 触发水位不能超过容量，否则永远无法达到唤醒条件。 */
    if (trigger_level > capacity) {
        /* 返回参数错误，提示调用方降低触发水位或增加容量。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 底层字节存储不能为空，否则无法保存写入数据。 */
    if (buffer == 0) {
        /* 返回参数错误，提示调用方提供字节数组。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 控制块不能为空，否则无法保存流缓冲状态。 */
    if (storage == 0) {
        /* 返回参数错误，提示调用方提供控制块。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 输出句柄不能为空，避免创建成功后调用方无法使用对象。 */
    if (out_stream == 0) {
        /* 返回参数错误，提示调用方提供输出句柄地址。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 保存底层字节存储地址。 */
    storage->buffer = (uint8_t *)buffer;

    /* 保存字节存储容量。 */
    storage->capacity = capacity;

    /* 保存读者唤醒触发水位。 */
    storage->trigger_level = trigger_level;

    /* 初始化读取位置为起点。 */
    storage->read_index = 0u;

    /* 初始化写入位置为起点。 */
    storage->write_index = 0u;

    /* 创建后还没有可读字节。 */
    storage->bytes_used = 0u;

    /* 初始化等待可写空间的任务链表。 */
    MRT_ListInitialize(&storage->waiting_writers);

    /* 初始化等待可读字节的任务链表。 */
    MRT_ListInitialize(&storage->waiting_readers);

    /* 标记对象使用静态存储创建。 */
    storage->static_storage = true;

    /* 输出流缓冲句柄给调用方。 */
    *out_stream = storage;

    /* 静态流缓冲创建成功。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 从 MyRTOS 全局堆动态创建流缓冲。
 * @param capacity 字节存储容量，单位为字节，必须大于 0。
 * @param trigger_level 读者唤醒触发水位，必须在 1 到 capacity 之间。
 * @param out_stream 输出流缓冲句柄，不能为空；失败时写入空指针。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示创建成功；参数非法返回 MRT_RESULT_INVALID_ARGUMENT；
 *         堆不可用或空间不足时返回 MRT_RESULT_NO_MEMORY。
 * @example
 * MRT_StreamBufferHandle stream;
 * MRT_StreamBufferCreate(128, 16, &stream);
 */
MRT_Result MRT_StreamBufferCreate(size_t capacity,
                                  size_t trigger_level,
                                  MRT_StreamBufferHandle *out_stream)
{
    /* 输出句柄不能为空。 */
    if (out_stream == 0) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 失败路径默认清空输出句柄。 */
    *out_stream = 0;

    /* 复用静态创建参数规则提前校验容量和触发水位。 */
    if ((capacity == 0u) || (trigger_level == 0u) || (trigger_level > capacity)) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 动态分配关闭时不能创建堆对象。 */
    if (MRT_CFG_SUPPORT_DYNAMIC_ALLOCATION == 0u) {
        /* 返回内存不足。 */
        return MRT_RESULT_NO_MEMORY;
    }

    /* 控制块之后紧跟字节存储，需要对控制块大小向上对齐。 */
    size_t control_bytes = MRT_StreamAlignSizeUp(sizeof(MRT_StreamBuffer));

    /* 对齐溢出时按内存不足处理。 */
    if (control_bytes == 0u) {
        /* 返回内存不足。 */
        return MRT_RESULT_NO_MEMORY;
    }

    /* 检查总大小加法是否溢出。 */
    if (capacity > (SIZE_MAX - control_bytes)) {
        /* 返回内存不足。 */
        return MRT_RESULT_NO_MEMORY;
    }

    /* 计算一个堆块所需总字节数。 */
    size_t total_bytes = control_bytes + capacity;

    /* 分配控制块和字节存储所在堆块。 */
    void *memory = MRT_Malloc(total_bytes);

    /* 堆空间不足时创建失败。 */
    if (memory == 0) {
        /* 返回内存不足。 */
        return MRT_RESULT_NO_MEMORY;
    }

    /* 堆块起始处保存控制块。 */
    MRT_StreamBuffer *stream = (MRT_StreamBuffer *)memory;

    /* 字节存储紧跟对齐后的控制块。 */
    uint8_t *buffer = ((uint8_t *)memory) + control_bytes;

    /* 复用静态创建逻辑初始化控制块。 */
    MRT_Result result = MRT_StreamBufferCreateStatic(capacity, trigger_level, buffer, stream, out_stream);

    /* 初始化失败时释放堆块。 */
    if (result != MRT_RESULT_OK) {
        /* 释放控制块和字节存储。 */
        (void)MRT_Free(memory);

        /* 清空输出句柄。 */
        *out_stream = 0;

        /* 返回实际错误。 */
        return result;
    }

    /* 标记流缓冲归动态堆所有。 */
    stream->static_storage = false;

    /* 动态流缓冲创建成功。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 删除动态创建的流缓冲并归还堆内存。
 * @param stream 待删除流缓冲句柄，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示删除成功；空句柄返回 MRT_RESULT_INVALID_ARGUMENT；
 *         静态对象或仍有等待任务时返回 MRT_RESULT_OBJECT_BUSY。
 * @example
 * MRT_StreamBufferDelete(stream);
 */
MRT_Result MRT_StreamBufferDelete(MRT_StreamBufferHandle stream)
{
    /* 流缓冲句柄不能为空。 */
    if (stream == 0) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 静态流缓冲内存不归堆释放路径所有。 */
    if (stream->static_storage) {
        /* 返回对象忙，提示调用方不能释放静态对象。 */
        return MRT_RESULT_OBJECT_BUSY;
    }

    /* 仍有任务等待可读字节时不能删除。 */
    if (!MRT_ListIsEmpty(&stream->waiting_readers)) {
        /* 返回对象忙，避免等待链表悬空。 */
        return MRT_RESULT_OBJECT_BUSY;
    }

    /* 仍有任务等待可写空间时不能删除。 */
    if (!MRT_ListIsEmpty(&stream->waiting_writers)) {
        /* 返回对象忙，避免等待链表悬空。 */
        return MRT_RESULT_OBJECT_BUSY;
    }

    /* 动态流缓冲控制块就是堆块起始地址。 */
    return MRT_Free(stream);
}

/**
 * @brief 查询流缓冲当前可读字节数。
 * @param stream 流缓冲句柄，不能为空。
 * @param out_bytes 输出可读字节数，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示查询成功；参数非法时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * size_t bytes;
 * MRT_StreamBufferBytesAvailable(stream, &bytes);
 */
MRT_Result MRT_StreamBufferBytesAvailable(MRT_StreamBufferHandle stream, size_t *out_bytes)
{
    /* 流缓冲句柄不能为空，否则无法读取状态。 */
    if (stream == 0) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 输出指针不能为空，否则无法写回可读字节数。 */
    if (out_bytes == 0) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 写回当前已使用字节数。 */
    *out_bytes = stream->bytes_used;

    /* 查询成功。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 向流缓冲写入字节流。
 * @param stream 目标流缓冲句柄，不能为空。
 * @param data 待写入数据地址；length 大于 0 时不能为空。
 * @param length 请求写入字节数。
 * @param timeout 等待可写空间的 tick 数；当前阶段非阻塞路径会在无空间时返回。
 * @param out_sent 输出实际写入字节数，允许为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示写入了请求字节或部分字节；无空间且 timeout 为 0 时返回
 *         MRT_RESULT_OBJECT_FULL；无空间且 timeout 非 0 时在当前 host 模型中返回 MRT_RESULT_TIMEOUT 并
 *         将当前任务挂入写等待链表；参数非法时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * size_t sent;
 * MRT_StreamBufferSend(stream, data, len, 0, &sent);
 */
MRT_Result MRT_StreamBufferSend(MRT_StreamBufferHandle stream,
                                const void *data,
                                size_t length,
                                MRT_Timeout timeout,
                                size_t *out_sent)
{
    /* 如果调用方提供输出指针，先清零，保证失败路径也有确定结果。 */
    if (out_sent != 0) {
        /* 默认实际写入 0 字节。 */
        *out_sent = 0u;
    }

    /* 流缓冲句柄不能为空。 */
    if (stream == 0) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 非零长度写入必须提供数据地址。 */
    if ((length != 0u) && (data == 0)) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 零长度写入是空操作，直接成功。 */
    if (length == 0u) {
        /* 没有字节需要写入。 */
        return MRT_RESULT_OK;
    }

    /* 计算当前可写空间。 */
    size_t spaces = stream->capacity - stream->bytes_used;

    /* 没有任何空间时无法写入。 */
    if (spaces == 0u) {
        /* 非零 timeout 表示调用方愿意等待可写空间。 */
        if (timeout != 0u) {
            /* 当前任务存在时，记录重新尝试所需的最小空间。 */
            MRT_TaskHandle current = MRT_TaskGetCurrent();
            if (current != 0) {
                /* 流缓冲允许部分写入，因此 1 字节空间即可唤醒写者重试。 */
                current->object_wait_bytes = 1u;
            }

            /* 将当前任务加入流缓冲写等待链表。 */
            return MRT_TaskKernelBlockCurrentOnObject(&stream->waiting_writers,
                                                      timeout,
                                                      MRT_TASK_WAIT_REASON_STREAM_SEND,
                                                      MRT_RESULT_TIMEOUT);
        }

        /* 返回对象满。 */
        return MRT_RESULT_OBJECT_FULL;
    }

    /* 实际写入数量为请求长度和空闲空间中的较小值。 */
    size_t writable = MRT_StreamMin(length, spaces);

    /* 执行环形写入。 */
    MRT_StreamWriteBytes(stream, (const uint8_t *)data, writable);

    /* 如果写入后达到触发水位，唤醒等待可读数据的任务。 */
    (void)MRT_StreamWakeReaderIfTriggered(stream, true);

    /* 写回实际写入数量。 */
    if (out_sent != 0) {
        /* 告诉调用方本次写入了多少字节。 */
        *out_sent = writable;
    }

    /* 写入成功。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 从流缓冲读取字节流。
 * @param stream 源流缓冲句柄，不能为空。
 * @param out_data 接收数据地址；length 大于 0 时不能为空。
 * @param length 请求读取字节数。
 * @param timeout 等待可读字节的 tick 数；当前阶段非阻塞路径会在无数据时返回。
 * @param out_received 输出实际读取字节数，允许为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示读取了请求字节或部分字节；无数据时返回 MRT_RESULT_OBJECT_EMPTY；
 *         参数非法时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * size_t received;
 * MRT_StreamBufferReceive(stream, out, sizeof(out), 0, &received);
 */
MRT_Result MRT_StreamBufferReceive(MRT_StreamBufferHandle stream,
                                   void *out_data,
                                   size_t length,
                                   MRT_Timeout timeout,
                                   size_t *out_received)
{
    /* 如果调用方提供输出指针，先清零，保证失败路径也有确定结果。 */
    if (out_received != 0) {
        /* 默认实际读取 0 字节。 */
        *out_received = 0u;
    }

    /* 当前阶段还没有读者阻塞队列，timeout 仅用于无数据时区分结果。 */
    (void)timeout;

    /* 流缓冲句柄不能为空。 */
    if (stream == 0) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 非零长度读取必须提供输出地址。 */
    if ((length != 0u) && (out_data == 0)) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 零长度读取是空操作，直接成功。 */
    if (length == 0u) {
        /* 没有字节需要读取。 */
        return MRT_RESULT_OK;
    }

    /* 没有可读字节时返回对象空。 */
    if (stream->bytes_used == 0u) {
        /* 非阻塞读取直接返回对象空。 */
        if (timeout == 0u) {
            /* 告诉调用方当前无数据。 */
            return MRT_RESULT_OBJECT_EMPTY;
        }

        /* 非零 timeout 时，将当前任务加入流缓冲读等待链表。 */
        return MRT_TaskKernelBlockCurrentOnObject(&stream->waiting_readers,
                                                  timeout,
                                                  MRT_TASK_WAIT_REASON_STREAM_RECEIVE,
                                                  MRT_RESULT_TIMEOUT);
    }

    /* 目前已有数据，timeout 不参与非阻塞读取路径。 */
    (void)timeout;

    /* 实际读取数量为请求长度和可读字节数中的较小值。 */
    size_t readable = MRT_StreamMin(length, stream->bytes_used);

    /* 执行环形读取。 */
    MRT_StreamReadBytes(stream, (uint8_t *)out_data, readable);

    /* 读取释放出空间后，尝试唤醒一个等待写者。 */
    (void)MRT_StreamWakeWriterIfSpace(stream, true);

    /* 写回实际读取数量。 */
    if (out_received != 0) {
        /* 告诉调用方本次读取了多少字节。 */
        *out_received = readable;
    }

    /* 读取成功。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 在 ISR 上下文向流缓冲写入字节流。
 * @param stream 目标流缓冲句柄，不能为空。
 * @param data 待写入数据地址；length 大于 0 时不能为空。
 * @param length 请求写入字节数。
 * @param out_sent 输出实际写入字节数，允许为空。
 * @param should_yield 输出是否需要在 ISR 退出前请求调度切换，允许为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示写入成功；无空间时返回 MRT_RESULT_OBJECT_FULL；
 *         参数非法时返回 MRT_RESULT_INVALID_ARGUMENT；非 ISR 上下文调用时返回 MRT_RESULT_INVALID_CONTEXT。
 * @example
 * bool yield;
 * MRT_StreamBufferSendFromISR(stream, data, len, &sent, &yield);
 */
MRT_Result MRT_StreamBufferSendFromISR(MRT_StreamBufferHandle stream,
                                       const void *data,
                                       size_t length,
                                       size_t *out_sent,
                                       bool *should_yield)
{
    /* 如果调用方提供输出指针，先清零，保证失败路径结果确定。 */
    if (out_sent != 0) {
        /* 默认没有写入字节。 */
        *out_sent = 0u;
    }

    /* 如果调用方提供 yield 输出，先清零。 */
    if (should_yield != 0) {
        /* 默认不请求切换。 */
        *should_yield = false;
    }

    /* FromISR API 只能在 ISR 上下文调用。 */
    if (!MRT_PortIsInsideISR()) {
        /* 返回非法上下文。 */
        return MRT_RESULT_INVALID_CONTEXT;
    }

    /* 流缓冲句柄不能为空。 */
    if (stream == 0) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 非零长度写入必须提供数据地址。 */
    if ((length != 0u) && (data == 0)) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 零长度写入是空操作。 */
    if (length == 0u) {
        /* 空操作成功。 */
        return MRT_RESULT_OK;
    }

    /* 计算当前可写空间。 */
    size_t spaces = stream->capacity - stream->bytes_used;

    /* ISR 中没有空间时不能阻塞等待。 */
    if (spaces == 0u) {
        /* 告诉调用方当前没有可写空间。 */
        return MRT_RESULT_OBJECT_FULL;
    }

    /* 实际写入数量为请求长度和空闲空间中的较小值。 */
    size_t writable = MRT_StreamMin(length, spaces);

    /* 执行环形写入。 */
    MRT_StreamWriteBytes(stream, (const uint8_t *)data, writable);

    /* 写回实际写入数量。 */
    if (out_sent != 0) {
        /* 告诉调用方本次写入了多少字节。 */
        *out_sent = writable;
    }

    /* 如果达到触发水位并唤醒了读者，则要求 ISR 退出后切换。 */
    if (MRT_StreamWakeReaderIfTriggered(stream, false)) {
        /* 写回延迟切换请求。 */
        if (should_yield != 0) {
            /* 提示端口层在 ISR 末尾请求调度。 */
            *should_yield = true;
        }
    }

    /* ISR 写入成功。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 在 ISR 上下文从流缓冲读取字节流。
 * @param stream 源流缓冲句柄，不能为空。
 * @param out_data 接收数据地址；length 大于 0 时不能为空。
 * @param length 请求读取字节数。
 * @param out_received 输出实际读取字节数，允许为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示读取成功；无数据时返回 MRT_RESULT_OBJECT_EMPTY；
 *         参数非法时返回 MRT_RESULT_INVALID_ARGUMENT；非 ISR 上下文调用时返回 MRT_RESULT_INVALID_CONTEXT。
 * @example
 * MRT_StreamBufferReceiveFromISR(stream, out, sizeof(out), &received);
 */
MRT_Result MRT_StreamBufferReceiveFromISR(MRT_StreamBufferHandle stream,
                                          void *out_data,
                                          size_t length,
                                          size_t *out_received)
{
    /* 如果调用方提供输出指针，先清零，保证失败路径结果确定。 */
    if (out_received != 0) {
        /* 默认没有读取字节。 */
        *out_received = 0u;
    }

    /* FromISR API 只能在 ISR 上下文调用。 */
    if (!MRT_PortIsInsideISR()) {
        /* 返回非法上下文。 */
        return MRT_RESULT_INVALID_CONTEXT;
    }

    /* 流缓冲句柄不能为空。 */
    if (stream == 0) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 非零长度读取必须提供输出地址。 */
    if ((length != 0u) && (out_data == 0)) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 零长度读取是空操作。 */
    if (length == 0u) {
        /* 空操作成功。 */
        return MRT_RESULT_OK;
    }

    /* ISR 中没有可读字节时不能阻塞等待。 */
    if (stream->bytes_used == 0u) {
        /* 告诉调用方当前无数据。 */
        return MRT_RESULT_OBJECT_EMPTY;
    }

    /* 实际读取数量为请求长度和可读字节数中的较小值。 */
    size_t readable = MRT_StreamMin(length, stream->bytes_used);

    /* 执行环形读取。 */
    MRT_StreamReadBytes(stream, (uint8_t *)out_data, readable);

    /* 读取释放出空间后，尝试唤醒一个等待写者。 */
    (void)MRT_StreamWakeWriterIfSpace(stream, false);

    /* 写回实际读取数量。 */
    if (out_received != 0) {
        /* 告诉调用方本次读取了多少字节。 */
        *out_received = readable;
    }

    /* ISR 读取成功。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 查询流缓冲当前可写空闲字节数。
 * @param stream 流缓冲句柄，不能为空。
 * @param out_spaces 输出可写空闲字节数，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示查询成功；参数非法时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * size_t spaces;
 * MRT_StreamBufferSpacesAvailable(stream, &spaces);
 */
MRT_Result MRT_StreamBufferSpacesAvailable(MRT_StreamBufferHandle stream, size_t *out_spaces)
{
    /* 流缓冲句柄不能为空，否则无法读取容量和使用量。 */
    if (stream == 0) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 输出指针不能为空，否则无法写回空闲空间。 */
    if (out_spaces == 0) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 空闲空间等于总容量减去已使用字节数。 */
    *out_spaces = stream->capacity - stream->bytes_used;

    /* 查询成功。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 清空流缓冲并复位读写索引。
 * @param stream 流缓冲句柄，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示复位成功；参数非法时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * MRT_StreamBufferReset(stream);
 */
MRT_Result MRT_StreamBufferReset(MRT_StreamBufferHandle stream)
{
    /* 流缓冲句柄不能为空，否则无法修改状态。 */
    if (stream == 0) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 读取索引复位到起点。 */
    stream->read_index = 0u;

    /* 写入索引复位到起点。 */
    stream->write_index = 0u;

    /* 清空已使用字节数。 */
    stream->bytes_used = 0u;

    /* 复位后拥有全部可写空间，尝试唤醒一个写者。 */
    (void)MRT_StreamWakeWriterIfSpace(stream, true);

    /* 复位成功。 */
    return MRT_RESULT_OK;
}
