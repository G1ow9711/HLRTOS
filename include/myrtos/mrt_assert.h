#ifndef MYRTOS_MRT_ASSERT_H
#define MYRTOS_MRT_ASSERT_H

/**
 * @file mrt_assert.h
 * @brief MyRTOS 统一断言 hook 接口。
 *
 * 该模块把内核断言失败统一分发给用户 hook。host 测试默认不陷入死循环，便于验证表达式、文件和行号；
 * 嵌入式移植可在 hook 中执行关中断、打印、断点、复位或错误计数。
 */

#include "myrtos/mrt_types.h"

/**
 * @brief 断言失败 hook 类型。
 * @param expression 失败表达式字符串；直接调用 MRT_AssertFailed 时由调用方提供。
 * @param file 触发断言的源文件名。
 * @param line 触发断言的源代码行号。
 * @param user 设置 hook 时保存的用户指针。
 * @return void 无返回值。
 * @example
 * static void AssertHook(const char *expr, const char *file, uint32_t line, void *user)
 * {
 *     (void)expr; (void)file; (void)line; (void)user;
 * }
 */
typedef void (*MRT_AssertHook)(const char *expression, const char *file, uint32_t line, void *user);

/**
 * @brief 设置断言失败 hook。
 * @param hook 断言失败回调；传入空指针表示关闭断言分发。
 * @param user 传递给 hook 的用户指针，允许为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示设置成功。
 * @example
 * MRT_AssertSetHook(AssertHook, NULL);
 */
MRT_Result MRT_AssertSetHook(MRT_AssertHook hook, void *user);

/**
 * @brief 分发一次断言失败记录。
 * @param expression 失败表达式字符串，允许为空。
 * @param file 触发断言的源文件名，允许为空。
 * @param line 触发断言的源代码行号。
 * @return void 无返回值。
 * @example
 * MRT_AssertFailed("ptr != NULL", __FILE__, __LINE__);
 */
void MRT_AssertFailed(const char *expression, const char *file, uint32_t line);

/**
 * @brief MyRTOS 断言宏。
 *
 * 表达式为 false 时调用 MRT_AssertFailed，并把表达式文本、文件名和行号传入断言 hook。
 * 默认实现不会停止当前线程；需要停机、断点或复位时，应在 hook 中实现。
 */
#define MRT_ASSERT(expr)                         \
    do {                                         \
        if (!(expr)) {                           \
            MRT_AssertFailed(#expr, __FILE__, (uint32_t)__LINE__); \
        }                                        \
    } while (0)

#endif
