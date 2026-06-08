#include <stddef.h>
#include <stdint.h>

/**
 * @brief freestanding 环境下的 memcpy 桩。
 * @param dest 目标内存地址。
 * @param src 源内存地址。
 * @param count 需要复制的字节数。
 * @return void* 返回目标地址。
 * @example
 * memcpy(dst, src, len);
 */
void *memcpy(void *dest, const void *src, size_t count)
{
    /* 把目标地址转换为逐字节写入指针。 */
    uint8_t *out = (uint8_t *)dest;

    /* 把源地址转换为逐字节读取指针。 */
    const uint8_t *in = (const uint8_t *)src;

    /* 逐字节搬运直到指定长度复制完成。 */
    for (size_t index = 0; index < count; ++index) {
        /* 复制当前字节。 */
        out[index] = in[index];
    }

    /* 返回目标地址，保持标准库语义。 */
    return dest;
}

/**
 * @brief freestanding 环境下的 memset 桩。
 * @param dest 目标内存地址。
 * @param value 要写入的字节值。
 * @param count 需要填充的字节数。
 * @return void* 返回目标地址。
 * @example
 * memset(buf, 0, sizeof(buf));
 */
void *memset(void *dest, int value, size_t count)
{
    /* 把目标地址转换为逐字节写入指针。 */
    uint8_t *out = (uint8_t *)dest;

    /* 将填充值规整到单字节。 */
    const uint8_t byte = (uint8_t)value;

    /* 逐字节填充直到指定长度写入完成。 */
    for (size_t index = 0; index < count; ++index) {
        /* 复制当前填充值。 */
        out[index] = byte;
    }

    /* 返回目标地址，保持标准库语义。 */
    return dest;
}
