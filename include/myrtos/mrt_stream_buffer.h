#ifndef MYRTOS_MRT_STREAM_BUFFER_H
#define MYRTOS_MRT_STREAM_BUFFER_H

/**
 * @file mrt_stream_buffer.h
 * @brief MyRTOS 流缓冲公共接口。
 *
 * 流缓冲用于在任务和 ISR 之间传递连续字节流。它不保存消息边界，
 * 只保证写入字节按 FIFO 顺序被读取。当前模块采用调用方提供控制块和
 * 字节存储的静态创建方式，适合无动态内存或强确定性的嵌入式工程。
 */

#include "myrtos/mrt_list.h"
#include "myrtos/mrt_types.h"

/**
 * @brief MyRTOS 流缓冲控制块。
 *
 * 静态创建流缓冲时，调用方提供该结构体和底层字节数组。结构体公开是为了
 * 支持嵌入式静态分配和测试可见性；应用代码不应直接修改字段。
 */
typedef struct MRT_StreamBuffer {
    /** @brief 底层字节存储起始地址。 */
    uint8_t *buffer;
    /** @brief 底层字节存储容量，单位为字节。 */
    size_t capacity;
    /** @brief 读者唤醒触发水位，单位为字节，必须在 1 到 capacity 之间。 */
    size_t trigger_level;
    /** @brief 下一个读取字节的位置。 */
    size_t read_index;
    /** @brief 下一个写入字节的位置。 */
    size_t write_index;
    /** @brief 当前已经写入且尚未读取的字节数。 */
    size_t bytes_used;
    /** @brief 等待可写空间的任务链表。 */
    MRT_List waiting_writers;
    /** @brief 等待可读字节的任务链表。 */
    MRT_List waiting_readers;
    /** @brief 是否使用静态存储创建。 */
    bool static_storage;
} MRT_StreamBuffer;

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
                                        MRT_StreamBufferHandle *out_stream);

/**
 * @brief 向流缓冲写入字节流。
 * @param stream 目标流缓冲句柄，不能为空。
 * @param data 待写入数据地址；length 大于 0 时不能为空。
 * @param length 请求写入字节数。
 * @param timeout 等待可写空间的 tick 数；当前阶段非阻塞路径会在无空间时返回。
 * @param out_sent 输出实际写入字节数，允许为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示写入了请求字节或部分字节；无空间时返回 MRT_RESULT_OBJECT_FULL；
 *         参数非法时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * size_t sent;
 * MRT_StreamBufferSend(stream, data, len, 0, &sent);
 */
MRT_Result MRT_StreamBufferSend(MRT_StreamBufferHandle stream,
                                const void *data,
                                size_t length,
                                MRT_Timeout timeout,
                                size_t *out_sent);

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
                                   size_t *out_received);

/**
 * @brief 查询流缓冲当前可读字节数。
 * @param stream 流缓冲句柄，不能为空。
 * @param out_bytes 输出可读字节数，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示查询成功；参数非法时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * size_t bytes;
 * MRT_StreamBufferBytesAvailable(stream, &bytes);
 */
MRT_Result MRT_StreamBufferBytesAvailable(MRT_StreamBufferHandle stream, size_t *out_bytes);

/**
 * @brief 查询流缓冲当前可写空闲字节数。
 * @param stream 流缓冲句柄，不能为空。
 * @param out_spaces 输出可写空闲字节数，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示查询成功；参数非法时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * size_t spaces;
 * MRT_StreamBufferSpacesAvailable(stream, &spaces);
 */
MRT_Result MRT_StreamBufferSpacesAvailable(MRT_StreamBufferHandle stream, size_t *out_spaces);

/**
 * @brief 清空流缓冲并复位读写索引。
 * @param stream 流缓冲句柄，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示复位成功；参数非法时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * MRT_StreamBufferReset(stream);
 */
MRT_Result MRT_StreamBufferReset(MRT_StreamBufferHandle stream);

#endif
