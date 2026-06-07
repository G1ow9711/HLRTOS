#include "myrtos/mrt_message_buffer.h"

/** @brief 消息缓冲中每条消息前置长度字段字节数。 */
#define MRT_MESSAGE_BUFFER_LENGTH_FIELD_SIZE 4u

/** @brief 消息缓冲静态创建允许的最小容量。 */
#define MRT_MESSAGE_BUFFER_MIN_CAPACITY (MRT_MESSAGE_BUFFER_LENGTH_FIELD_SIZE + 1u)

/**
 * @brief 使用调用方提供的控制块和字节存储静态创建消息缓冲。
 * @param capacity 字节存储容量，必须至少能容纳 4 字节长度头和 1 字节消息。
 * @param buffer 底层字节存储，大小至少为 capacity，不能为空。
 * @param storage 消息缓冲控制块存储，不能为空。
 * @param out_message_buffer 输出消息缓冲句柄，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示创建成功；参数非法时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * static MRT_MessageBuffer msg_cb;
 * static uint8_t msg_storage[64];
 * MRT_MessageBufferHandle msg;
 * MRT_MessageBufferCreateStatic(64, msg_storage, &msg_cb, &msg);
 */
MRT_Result MRT_MessageBufferCreateStatic(size_t capacity,
                                         void *buffer,
                                         MRT_MessageBuffer *storage,
                                         MRT_MessageBufferHandle *out_message_buffer)
{
    /* 容量必须至少能容纳长度头和一个消息字节。 */
    if (capacity < MRT_MESSAGE_BUFFER_MIN_CAPACITY) {
        /* 返回参数错误，提示调用方提供更大的存储区。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 底层字节存储不能为空，否则无法保存消息。 */
    if (buffer == 0) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 控制块不能为空，否则无法保存消息缓冲状态。 */
    if (storage == 0) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 输出句柄不能为空，避免创建成功后调用方无法使用对象。 */
    if (out_message_buffer == 0) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 保存底层字节存储地址。 */
    storage->buffer = (uint8_t *)buffer;

    /* 保存字节存储容量。 */
    storage->capacity = capacity;

    /* 初始化读取位置为起点。 */
    storage->read_index = 0u;

    /* 初始化写入位置为起点。 */
    storage->write_index = 0u;

    /* 创建后没有已保存消息字节。 */
    storage->bytes_used = 0u;

    /* 初始化等待可写空间的任务链表。 */
    MRT_ListInitialize(&storage->waiting_writers);

    /* 初始化等待完整消息的任务链表。 */
    MRT_ListInitialize(&storage->waiting_readers);

    /* 标记对象使用静态存储创建。 */
    storage->static_storage = true;

    /* 输出消息缓冲句柄给调用方。 */
    *out_message_buffer = storage;

    /* 静态消息缓冲创建成功。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 查询消息缓冲当前已使用字节数。
 * @param message_buffer 消息缓冲句柄，不能为空。
 * @param out_bytes 输出已使用字节数，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示查询成功；参数非法时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * size_t bytes;
 * MRT_MessageBufferBytesAvailable(message_buffer, &bytes);
 */
MRT_Result MRT_MessageBufferBytesAvailable(MRT_MessageBufferHandle message_buffer, size_t *out_bytes)
{
    /* 消息缓冲句柄不能为空，否则无法读取状态。 */
    if (message_buffer == 0) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 输出指针不能为空，否则无法写回已使用字节数。 */
    if (out_bytes == 0) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 写回当前已使用字节数。 */
    *out_bytes = message_buffer->bytes_used;

    /* 查询成功。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 查询消息缓冲当前可写空闲字节数。
 * @param message_buffer 消息缓冲句柄，不能为空。
 * @param out_spaces 输出可写空闲字节数，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示查询成功；参数非法时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * size_t spaces;
 * MRT_MessageBufferSpacesAvailable(message_buffer, &spaces);
 */
MRT_Result MRT_MessageBufferSpacesAvailable(MRT_MessageBufferHandle message_buffer, size_t *out_spaces)
{
    /* 消息缓冲句柄不能为空，否则无法读取容量和使用量。 */
    if (message_buffer == 0) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 输出指针不能为空，否则无法写回空闲字节数。 */
    if (out_spaces == 0) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 空闲空间等于总容量减去已使用字节数。 */
    *out_spaces = message_buffer->capacity - message_buffer->bytes_used;

    /* 查询成功。 */
    return MRT_RESULT_OK;
}
