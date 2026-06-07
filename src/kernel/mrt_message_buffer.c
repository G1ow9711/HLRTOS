#include "myrtos/mrt_message_buffer.h"

/** @brief 消息缓冲中每条消息前置长度字段字节数。 */
#define MRT_MESSAGE_BUFFER_LENGTH_FIELD_SIZE 4u

/** @brief 消息缓冲静态创建允许的最小容量。 */
#define MRT_MESSAGE_BUFFER_MIN_CAPACITY (MRT_MESSAGE_BUFFER_LENGTH_FIELD_SIZE + 1u)

/**
 * @brief 推进消息缓冲环形索引。
 * @param message_buffer 消息缓冲句柄，不能为空。
 * @param index 当前索引。
 * @return size_t 返回推进一个字节后的索引，必要时回绕到 0。
 * @example
 * next = MRT_MessageNextIndex(message_buffer, current);
 */
static size_t MRT_MessageNextIndex(MRT_MessageBufferHandle message_buffer, size_t index)
{
    /* 索引先向后移动一个字节位置。 */
    index++;

    /* 如果索引到达容量末尾之后，则回绕到起点。 */
    if (index >= message_buffer->capacity) {
        /* 环形缓冲从 0 继续读写。 */
        index = 0u;
    }

    /* 返回推进后的索引。 */
    return index;
}

/**
 * @brief 向消息缓冲内部写入一个字节。
 * @param message_buffer 目标消息缓冲句柄，不能为空。
 * @param byte 要写入的字节。
 * @return void 无返回值。
 * @example
 * MRT_MessageWriteByte(message_buffer, byte);
 */
static void MRT_MessageWriteByte(MRT_MessageBufferHandle message_buffer, uint8_t byte)
{
    /* 将字节写入当前写索引位置。 */
    message_buffer->buffer[message_buffer->write_index] = byte;

    /* 推进写索引，必要时回绕。 */
    message_buffer->write_index = MRT_MessageNextIndex(message_buffer, message_buffer->write_index);

    /* 已使用字节数增加。 */
    message_buffer->bytes_used++;
}

/**
 * @brief 从消息缓冲内部读取并移除一个字节。
 * @param message_buffer 源消息缓冲句柄，不能为空。
 * @return uint8_t 返回读取到的字节。
 * @example
 * byte = MRT_MessageReadByte(message_buffer);
 */
static uint8_t MRT_MessageReadByte(MRT_MessageBufferHandle message_buffer)
{
    /* 读取当前读索引位置的字节。 */
    uint8_t byte = message_buffer->buffer[message_buffer->read_index];

    /* 推进读索引，必要时回绕。 */
    message_buffer->read_index = MRT_MessageNextIndex(message_buffer, message_buffer->read_index);

    /* 已使用字节数减少。 */
    message_buffer->bytes_used--;

    /* 返回读取到的字节。 */
    return byte;
}

/**
 * @brief 从消息缓冲指定偏移处窥视一个字节但不移除。
 * @param message_buffer 源消息缓冲句柄，不能为空。
 * @param offset 从当前读索引开始计算的偏移。
 * @return uint8_t 返回窥视到的字节。
 * @example
 * byte = MRT_MessagePeekByte(message_buffer, 0);
 */
static uint8_t MRT_MessagePeekByte(MRT_MessageBufferHandle message_buffer, size_t offset)
{
    /* 从当前读索引开始计算目标位置。 */
    size_t index = message_buffer->read_index;

    /* 按偏移推进目标位置。 */
    for (size_t count = 0u; count < offset; count++) {
        /* 每次推进一个字节并处理环绕。 */
        index = MRT_MessageNextIndex(message_buffer, index);
    }

    /* 返回目标位置字节，不修改读索引和已使用字节数。 */
    return message_buffer->buffer[index];
}

/**
 * @brief 写入 32 位小端消息长度。
 * @param message_buffer 目标消息缓冲句柄，不能为空。
 * @param length 消息载荷长度。
 * @return void 无返回值。
 * @example
 * MRT_MessageWriteLength(message_buffer, length);
 */
static void MRT_MessageWriteLength(MRT_MessageBufferHandle message_buffer, uint32_t length)
{
    /* 写入最低 8 bit。 */
    MRT_MessageWriteByte(message_buffer, (uint8_t)(length & 0xFFu));

    /* 写入第 8 到 15 bit。 */
    MRT_MessageWriteByte(message_buffer, (uint8_t)((length >> 8u) & 0xFFu));

    /* 写入第 16 到 23 bit。 */
    MRT_MessageWriteByte(message_buffer, (uint8_t)((length >> 16u) & 0xFFu));

    /* 写入第 24 到 31 bit。 */
    MRT_MessageWriteByte(message_buffer, (uint8_t)((length >> 24u) & 0xFFu));
}

/**
 * @brief 窥视下一条消息的 32 位小端长度。
 * @param message_buffer 源消息缓冲句柄，不能为空。
 * @return uint32_t 返回下一条消息载荷长度。
 * @example
 * length = MRT_MessagePeekLength(message_buffer);
 */
static uint32_t MRT_MessagePeekLength(MRT_MessageBufferHandle message_buffer)
{
    /* 读取最低 8 bit。 */
    uint32_t byte0 = (uint32_t)MRT_MessagePeekByte(message_buffer, 0u);

    /* 读取第 8 到 15 bit。 */
    uint32_t byte1 = (uint32_t)MRT_MessagePeekByte(message_buffer, 1u);

    /* 读取第 16 到 23 bit。 */
    uint32_t byte2 = (uint32_t)MRT_MessagePeekByte(message_buffer, 2u);

    /* 读取第 24 到 31 bit。 */
    uint32_t byte3 = (uint32_t)MRT_MessagePeekByte(message_buffer, 3u);

    /* 合成小端 32 位长度。 */
    return byte0 | (byte1 << 8u) | (byte2 << 16u) | (byte3 << 24u);
}

/**
 * @brief 丢弃下一条消息的长度头。
 * @param message_buffer 消息缓冲句柄，不能为空。
 * @return void 无返回值。
 * @example
 * MRT_MessageDiscardLength(message_buffer);
 */
static void MRT_MessageDiscardLength(MRT_MessageBufferHandle message_buffer)
{
    /* 读取并丢弃长度头第 0 字节。 */
    (void)MRT_MessageReadByte(message_buffer);

    /* 读取并丢弃长度头第 1 字节。 */
    (void)MRT_MessageReadByte(message_buffer);

    /* 读取并丢弃长度头第 2 字节。 */
    (void)MRT_MessageReadByte(message_buffer);

    /* 读取并丢弃长度头第 3 字节。 */
    (void)MRT_MessageReadByte(message_buffer);
}

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
 * @brief 向消息缓冲写入一条完整消息。
 * @param message_buffer 目标消息缓冲句柄，不能为空。
 * @param message 待写入消息地址；length 大于 0 时不能为空。
 * @param length 消息载荷字节数，必须大于 0。
 * @param timeout 等待可写空间的 tick 数；当前阶段非阻塞路径会在空间不足时返回。
 * @param out_sent 输出实际写入的消息载荷字节数，允许为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示整条消息写入成功；空间不足返回 MRT_RESULT_OBJECT_FULL；
 *         参数非法返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * size_t sent;
 * MRT_MessageBufferSend(message_buffer, data, len, 0, &sent);
 */
MRT_Result MRT_MessageBufferSend(MRT_MessageBufferHandle message_buffer,
                                 const void *message,
                                 size_t length,
                                 MRT_Timeout timeout,
                                 size_t *out_sent)
{
    /* 如果调用方提供输出指针，先清零，保证失败路径结果确定。 */
    if (out_sent != 0) {
        /* 默认写入 0 字节载荷。 */
        *out_sent = 0u;
    }

    /* 当前阶段未接入写者阻塞，timeout 暂不参与空间等待。 */
    (void)timeout;

    /* 消息缓冲句柄不能为空。 */
    if (message_buffer == 0) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 消息长度必须大于 0，避免只有长度头的空记录。 */
    if (length == 0u) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 消息指针不能为空。 */
    if (message == 0) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 当前长度字段为 32 位，超过范围的消息无法编码。 */
    if (length > UINT32_MAX) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 计算整条记录所需空间：长度头加消息载荷。 */
    size_t required = MRT_MESSAGE_BUFFER_LENGTH_FIELD_SIZE + length;

    /* 单条消息超过总容量时，永远无法写入。 */
    if (required > message_buffer->capacity) {
        /* 返回对象满，表示目标缓冲无法容纳该消息。 */
        return MRT_RESULT_OBJECT_FULL;
    }

    /* 当前空闲空间不足时，不写入任何半条消息。 */
    if ((message_buffer->capacity - message_buffer->bytes_used) < required) {
        /* 返回对象满，保持已有消息不变。 */
        return MRT_RESULT_OBJECT_FULL;
    }

    /* 写入 32 位小端消息长度。 */
    MRT_MessageWriteLength(message_buffer, (uint32_t)length);

    /* 将待写入消息转换为字节指针。 */
    const uint8_t *bytes = (const uint8_t *)message;

    /* 逐字节写入消息载荷。 */
    for (size_t index = 0u; index < length; index++) {
        /* 写入当前载荷字节。 */
        MRT_MessageWriteByte(message_buffer, bytes[index]);
    }

    /* 写回实际写入载荷长度。 */
    if (out_sent != 0) {
        /* 告诉调用方整条消息载荷已写入。 */
        *out_sent = length;
    }

    /* 发送成功。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 从消息缓冲读取一条完整消息。
 * @param message_buffer 源消息缓冲句柄，不能为空。
 * @param out_message 输出消息地址，不能为空。
 * @param output_capacity 输出缓冲容量，单位为字节。
 * @param timeout 等待完整消息的 tick 数；当前阶段非阻塞路径会在空缓冲时返回。
 * @param out_received 输出实际读取的消息载荷字节数，允许为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示读取成功；无消息返回 MRT_RESULT_OBJECT_EMPTY；
 *         输出缓冲太小返回 MRT_RESULT_OBJECT_FULL 且不移除消息；参数非法返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * size_t received;
 * MRT_MessageBufferReceive(message_buffer, out, sizeof(out), 0, &received);
 */
MRT_Result MRT_MessageBufferReceive(MRT_MessageBufferHandle message_buffer,
                                    void *out_message,
                                    size_t output_capacity,
                                    MRT_Timeout timeout,
                                    size_t *out_received)
{
    /* 如果调用方提供输出指针，先清零，保证失败路径结果确定。 */
    if (out_received != 0) {
        /* 默认读取 0 字节载荷。 */
        *out_received = 0u;
    }

    /* 当前阶段未接入读者阻塞，timeout 暂不参与等待。 */
    (void)timeout;

    /* 消息缓冲句柄不能为空。 */
    if (message_buffer == 0) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 输出消息指针不能为空。 */
    if (out_message == 0) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 没有任何消息时返回对象空。 */
    if (message_buffer->bytes_used == 0u) {
        /* 告诉调用方当前无完整消息。 */
        return MRT_RESULT_OBJECT_EMPTY;
    }

    /* 防御性检查：已使用字节不足长度头表示内部状态异常。 */
    if (message_buffer->bytes_used < MRT_MESSAGE_BUFFER_LENGTH_FIELD_SIZE) {
        /* 返回内部错误。 */
        return MRT_RESULT_INTERNAL_ERROR;
    }

    /* 窥视下一条消息长度但不移除消息。 */
    uint32_t message_length = MRT_MessagePeekLength(message_buffer);

    /* 计算完整记录长度。 */
    size_t required = MRT_MESSAGE_BUFFER_LENGTH_FIELD_SIZE + (size_t)message_length;

    /* 防御性检查：记录长度超过已使用字节表示内部状态异常。 */
    if (required > message_buffer->bytes_used) {
        /* 返回内部错误。 */
        return MRT_RESULT_INTERNAL_ERROR;
    }

    /* 输出缓冲太小时不移除消息，避免产生半包。 */
    if (output_capacity < (size_t)message_length) {
        /* 返回对象满，表示调用方输出空间不足。 */
        return MRT_RESULT_OBJECT_FULL;
    }

    /* 丢弃长度头，准备读取载荷。 */
    MRT_MessageDiscardLength(message_buffer);

    /* 将输出地址转换为字节指针。 */
    uint8_t *out = (uint8_t *)out_message;

    /* 逐字节读取完整载荷。 */
    for (size_t index = 0u; index < (size_t)message_length; index++) {
        /* 读取当前载荷字节。 */
        out[index] = MRT_MessageReadByte(message_buffer);
    }

    /* 写回实际读取载荷长度。 */
    if (out_received != 0) {
        /* 告诉调用方读出了完整消息长度。 */
        *out_received = (size_t)message_length;
    }

    /* 接收成功。 */
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

/**
 * @brief 清空消息缓冲并复位读写索引。
 * @param message_buffer 消息缓冲句柄，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示复位成功；参数非法时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * MRT_MessageBufferReset(message_buffer);
 */
MRT_Result MRT_MessageBufferReset(MRT_MessageBufferHandle message_buffer)
{
    /* 消息缓冲句柄不能为空，否则无法修改状态。 */
    if (message_buffer == 0) {
        /* 返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 读取索引复位到起点。 */
    message_buffer->read_index = 0u;

    /* 写入索引复位到起点。 */
    message_buffer->write_index = 0u;

    /* 清空已使用字节数。 */
    message_buffer->bytes_used = 0u;

    /* 复位成功。 */
    return MRT_RESULT_OK;
}
