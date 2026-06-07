#ifndef MYRTOS_MRT_QUEUE_H
#define MYRTOS_MRT_QUEUE_H

/**
 * @file mrt_queue.h
 * @brief MyRTOS 固定长度复制队列接口。
 *
 * 队列用于任务之间、ISR 与任务之间传递固定大小的数据项。
 * MyRTOS 队列按值复制数据，调用方发送后可立即复用原始变量。
 */

#include "myrtos/mrt_list.h"
#include "myrtos/mrt_types.h"

/**
 * @brief MyRTOS 队列控制块。
 *
 * 静态创建队列时，用户提供该结构体和数据缓冲区。结构体公开是为了支持
 * 无动态内存场景；应用代码不应直接修改字段。
 */
typedef struct MRT_Queue {
    /** @brief 队列数据缓冲区起始地址。 */
    uint8_t *buffer;
    /** @brief 队列最多可容纳的元素数量。 */
    size_t capacity;
    /** @brief 每个元素的字节数。 */
    size_t item_size;
    /** @brief 下一个读取位置的元素下标。 */
    size_t read_index;
    /** @brief 下一个写入位置的元素下标。 */
    size_t write_index;
    /** @brief 当前队列中的元素数量。 */
    size_t count;
    /** @brief 等待发送空间的任务链表，后续阻塞队列计划使用。 */
    MRT_List waiting_senders;
    /** @brief 等待接收数据的任务链表，后续阻塞队列计划使用。 */
    MRT_List waiting_receivers;
    /** @brief 是否使用静态存储创建。 */
    bool static_storage;
} MRT_Queue;

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
                                 MRT_QueueHandle *out_queue);

/**
 * @brief 查询队列中已有元素数量。
 * @param queue 待查询队列句柄，不能为空。
 * @return size_t 返回当前队列中的元素数量；队列为空指针时返回 0。
 * @example
 * size_t used = MRT_QueueMessagesWaiting(queue);
 */
size_t MRT_QueueMessagesWaiting(MRT_QueueHandle queue);

/**
 * @brief 查询队列剩余可写空间。
 * @param queue 待查询队列句柄，不能为空。
 * @return size_t 返回剩余可写元素数量；队列为空指针时返回 0。
 * @example
 * size_t free_slots = MRT_QueueSpacesAvailable(queue);
 */
size_t MRT_QueueSpacesAvailable(MRT_QueueHandle queue);

#endif
