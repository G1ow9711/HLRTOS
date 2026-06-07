#include "myrtos/mrt_assert.h"

/** @brief 当前断言失败 hook；为空表示关闭断言分发。 */
static MRT_AssertHook g_assert_hook;

/** @brief 传递给断言 hook 的用户指针。 */
static void *g_assert_user;

/**
 * @brief 设置断言失败 hook。
 * @param hook 断言失败回调；传入空指针表示关闭断言分发。
 * @param user 传递给 hook 的用户指针，允许为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示设置成功。
 * @example
 * MRT_AssertSetHook(AssertHook, NULL);
 */
MRT_Result MRT_AssertSetHook(MRT_AssertHook hook, void *user)
{
    /* 保存调用方提供的断言 hook；为空时表示关闭断言分发。 */
    g_assert_hook = hook;

    /* 保存用户指针，断言失败时原样传回。 */
    g_assert_user = user;

    /* 设置过程不分配资源，因此总是成功。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 分发一次断言失败记录。
 * @param expression 失败表达式字符串，允许为空。
 * @param file 触发断言的源文件名，允许为空。
 * @param line 触发断言的源代码行号。
 * @return void 无返回值。
 * @example
 * MRT_AssertFailed("ptr != NULL", __FILE__, __LINE__);
 */
void MRT_AssertFailed(const char *expression, const char *file, uint32_t line)
{
    /* 未设置 hook 时静默返回，避免默认 host 测试或最小系统停机。 */
    if (g_assert_hook == 0) {
        /* 没有下沉回调，不做额外处理。 */
        return;
    }

    /* 调用用户 hook，并传回失败表达式、源位置和用户指针。 */
    g_assert_hook(expression, file, line, g_assert_user);
}
