#ifndef MRT_HARDWARE_SMOKE_LOG_H
#define MRT_HARDWARE_SMOKE_LOG_H

#include <stddef.h>
#include <stdint.h>

/*
 * 文件: mrt_hardware_smoke_log.h
 * 目的: 为真实 STM32/DSP 板级 smoke 提供轻量 `Key: Value` 日志输出 helper。
 * 边界: 本文件只负责字段格式化，不判断硬件是否 PASS，也不生成最终验收证据。
 */

typedef void (*MRT_SmokeLogWriteCharFn)(void *context, char ch);

typedef struct
{
    /* 输出单个字符的板级回调，通常绑定到 UART、SWO、trace 或测试缓冲。 */
    MRT_SmokeLogWriteCharFn write_char;

    /* 传给输出回调的用户上下文。 */
    void *context;
} MRT_SmokeLogWriter;

typedef enum
{
    /* 日志写入成功。 */
    MRT_SMOKE_LOG_OK = 0,

    /* 入参为空或 writer 未绑定输出回调。 */
    MRT_SMOKE_LOG_INVALID_ARGUMENT = 1,
} MRT_SmokeLogResult;

/**
 * @brief 判断日志 writer 是否已经绑定输出回调。
 * @param writer 指向日志 writer 的指针。
 * @return 有效返回 1，无效返回 0。
 * @example
 * if (MRT_SmokeLogWriterIsValid(&writer)) { ... }
 */
static inline int MRT_SmokeLogWriterIsValid(const MRT_SmokeLogWriter *writer)
{
    /* 空 writer 不能输出任何字符。 */
    if (writer == 0)
    {
        return 0;
    }

    /* 未绑定字符输出回调时也不能输出。 */
    if (writer->write_char == 0)
    {
        return 0;
    }

    /* writer 已具备最小输出能力。 */
    return 1;
}

/**
 * @brief 输出一个 C 字符串。
 * @param writer 指向日志 writer 的指针。
 * @param text 需要输出的 NUL 结尾字符串。
 * @return 成功返回 `MRT_SMOKE_LOG_OK`；参数非法返回 `MRT_SMOKE_LOG_INVALID_ARGUMENT`。
 * @example
 * MRT_SmokeLogWriteCString(&writer, "Evidence-Status");
 */
static inline MRT_SmokeLogResult MRT_SmokeLogWriteCString(const MRT_SmokeLogWriter *writer, const char *text)
{
    /* 写入前先校验 writer，避免空回调导致异常。 */
    if (!MRT_SmokeLogWriterIsValid(writer))
    {
        return MRT_SMOKE_LOG_INVALID_ARGUMENT;
    }

    /* 空字符串指针必须拒绝，避免输出不完整字段。 */
    if (text == 0)
    {
        return MRT_SMOKE_LOG_INVALID_ARGUMENT;
    }

    /* 逐字符输出，适配没有 printf 的裸机环境。 */
    while (*text != '\0')
    {
        writer->write_char(writer->context, *text);
        text++;
    }

    /* 字符串输出完成。 */
    return MRT_SMOKE_LOG_OK;
}

/**
 * @brief 输出字段名前缀 `Key: `。
 * @param writer 指向日志 writer 的指针。
 * @param field 字段名字符串。
 * @return 成功返回 `MRT_SMOKE_LOG_OK`；参数非法返回 `MRT_SMOKE_LOG_INVALID_ARGUMENT`。
 * @example
 * MRT_SmokeLogWriteFieldPrefix(&writer, MRT_SMOKE_FIELD_BOARD);
 */
static inline MRT_SmokeLogResult MRT_SmokeLogWriteFieldPrefix(const MRT_SmokeLogWriter *writer, const char *field)
{
    /* 字段名前缀必须先整体校验，避免写出半个字段名。 */
    if ((!MRT_SmokeLogWriterIsValid(writer)) || (field == 0))
    {
        return MRT_SMOKE_LOG_INVALID_ARGUMENT;
    }

    /* 写出字段名。 */
    (void)MRT_SmokeLogWriteCString(writer, field);

    /* 写出 schema 规定的分隔符。 */
    (void)MRT_SmokeLogWriteCString(writer, ": ");

    /* 前缀输出完成。 */
    return MRT_SMOKE_LOG_OK;
}

/**
 * @brief 输出一行字符串字段。
 * @param writer 指向日志 writer 的指针。
 * @param field 字段名，建议使用 `MRT_SMOKE_FIELD_*` 常量。
 * @param value 字段值字符串。
 * @return 成功返回 `MRT_SMOKE_LOG_OK`；参数非法返回 `MRT_SMOKE_LOG_INVALID_ARGUMENT`。
 * @example
 * MRT_SmokeLogWritePair(&writer, MRT_SMOKE_FIELD_EVIDENCE_STATUS, "PASS");
 */
static inline MRT_SmokeLogResult MRT_SmokeLogWritePair(const MRT_SmokeLogWriter *writer,
                                                       const char *field,
                                                       const char *value)
{
    /* 先统一校验全部参数，确保失败路径不写出半行日志。 */
    if ((!MRT_SmokeLogWriterIsValid(writer)) || (field == 0) || (value == 0))
    {
        return MRT_SMOKE_LOG_INVALID_ARGUMENT;
    }

    /* 写出字段名前缀。 */
    (void)MRT_SmokeLogWriteFieldPrefix(writer, field);

    /* 写出字段值。 */
    (void)MRT_SmokeLogWriteCString(writer, value);

    /* 每个证据字段独占一行。 */
    writer->write_char(writer->context, '\n');

    /* 整行输出完成。 */
    return MRT_SMOKE_LOG_OK;
}

/**
 * @brief 输出一行无符号 32 位整数字段。
 * @param writer 指向日志 writer 的指针。
 * @param field 字段名，建议使用 `MRT_SMOKE_FIELD_*` 常量。
 * @param value 需要按十进制输出的整数值。
 * @return 成功返回 `MRT_SMOKE_LOG_OK`；参数非法返回 `MRT_SMOKE_LOG_INVALID_ARGUMENT`。
 * @example
 * MRT_SmokeLogWriteU32(&writer, MRT_SMOKE_FIELD_RUNTIME_MINUTES, 30u);
 */
static inline MRT_SmokeLogResult MRT_SmokeLogWriteU32(const MRT_SmokeLogWriter *writer,
                                                      const char *field,
                                                      uint32_t value)
{
    /* 先校验 writer 和字段名，保证失败时不写半行日志。 */
    if ((!MRT_SmokeLogWriterIsValid(writer)) || (field == 0))
    {
        return MRT_SMOKE_LOG_INVALID_ARGUMENT;
    }

    /* uint32 最大值需要 10 个十进制数字。 */
    char digits[10u];

    /* 记录已经生成的反序数字个数。 */
    size_t count = 0u;

    /* 复制待转换值，避免修改调用方输入语义。 */
    uint32_t remaining = value;

    /* 先写字段名前缀。 */
    (void)MRT_SmokeLogWriteFieldPrefix(writer, field);

    /* 至少输出一个数字，覆盖 value 为 0 的情况。 */
    do
    {
        digits[count] = (char)('0' + (remaining % 10u));
        count++;
        remaining /= 10u;
    } while (remaining != 0u);

    /* 反向写出数字，得到正常十进制顺序。 */
    while (count > 0u)
    {
        count--;
        writer->write_char(writer->context, digits[count]);
    }

    /* 每个整数字段同样独占一行。 */
    writer->write_char(writer->context, '\n');

    /* 整数字段输出完成。 */
    return MRT_SMOKE_LOG_OK;
}

#endif /* MRT_HARDWARE_SMOKE_LOG_H */
