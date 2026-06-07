#include "myrtos/mrt_stream_buffer.h"

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
