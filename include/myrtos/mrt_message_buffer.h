#ifndef MYRTOS_MRT_MESSAGE_BUFFER_H
#define MYRTOS_MRT_MESSAGE_BUFFER_H

/**
 * @file mrt_message_buffer.h
 * @brief MyRTOS 消息缓冲公共接口。
 *
 * 消息缓冲用于传递带边界的可变长度消息。每条消息在底层字节存储中带有
 * 4 字节长度头，接收时必须一次取出完整消息，避免产生半包。
 */

#include "myrtos/mrt_list.h"
#include "myrtos/mrt_types.h"

/**
 * @brief MyRTOS 消息缓冲控制块。
 *
 * 静态创建消息缓冲时，调用方提供该结构体和底层字节数组。结构体公开是为了
 * 支持无动态内存的嵌入式工程；应用代码不应直接修改字段。
 */
typedef struct MRT_MessageBuffer {
    /** @brief 底层字节存储起始地址。 */
    uint8_t *buffer;
    /** @brief 底层字节存储容量，单位为字节。 */
    size_t capacity;
    /** @brief 下一个读取字节的位置。 */
    size_t read_index;
    /** @brief 下一个写入字节的位置。 */
    size_t write_index;
    /** @brief 当前已使用字节数，包含消息长度头和消息载荷。 */
    size_t bytes_used;
    /** @brief 等待可写空间的任务链表。 */
    MRT_List waiting_writers;
    /** @brief 等待完整消息的任务链表。 */
    MRT_List waiting_readers;
    /** @brief 是否使用静态存储创建。 */
    bool static_storage;
} MRT_MessageBuffer;

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
                                         MRT_MessageBufferHandle *out_message_buffer);

/**
 * @brief 查询消息缓冲当前已使用字节数。
 * @param message_buffer 消息缓冲句柄，不能为空。
 * @param out_bytes 输出已使用字节数，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示查询成功；参数非法时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * size_t bytes;
 * MRT_MessageBufferBytesAvailable(message_buffer, &bytes);
 */
MRT_Result MRT_MessageBufferBytesAvailable(MRT_MessageBufferHandle message_buffer, size_t *out_bytes);

/**
 * @brief 查询消息缓冲当前可写空闲字节数。
 * @param message_buffer 消息缓冲句柄，不能为空。
 * @param out_spaces 输出可写空闲字节数，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示查询成功；参数非法时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * size_t spaces;
 * MRT_MessageBufferSpacesAvailable(message_buffer, &spaces);
 */
MRT_Result MRT_MessageBufferSpacesAvailable(MRT_MessageBufferHandle message_buffer, size_t *out_spaces);

#endif
