#include "mrt_test.h"
#include "myrtos/mrt_assert.h"

#include <string.h>

/** @brief 断言 hook 捕获次数。 */
static uint32_t g_assert_count;

/** @brief 断言 hook 最近一次捕获的表达式。 */
static const char *g_last_expression;

/** @brief 断言 hook 最近一次捕获的文件名。 */
static const char *g_last_file;

/** @brief 断言 hook 最近一次捕获的行号。 */
static uint32_t g_last_line;

/** @brief 断言 hook 最近一次捕获的用户指针。 */
static void *g_last_user;

/**
 * @brief 捕获断言失败信息的测试 hook。
 * @param expression 失败表达式字符串。
 * @param file 触发断言的文件名。
 * @param line 触发断言的行号。
 * @param user 设置 hook 时传入的用户指针。
 * @return void 无返回值。
 * @example
 * MRT_AssertSetHook(CaptureAssertHook, user);
 */
static void CaptureAssertHook(const char *expression, const char *file, uint32_t line, void *user)
{
    /* 记录捕获次数。 */
    g_assert_count++;

    /* 保存失败表达式。 */
    g_last_expression = expression;

    /* 保存失败文件名。 */
    g_last_file = file;

    /* 保存失败行号。 */
    g_last_line = line;

    /* 保存用户指针。 */
    g_last_user = user;
}

/**
 * @brief 清空断言捕获状态。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * ResetAssertCapture();
 */
static void ResetAssertCapture(void)
{
    /* 清空捕获次数。 */
    g_assert_count = 0u;

    /* 清空表达式指针。 */
    g_last_expression = 0;

    /* 清空文件名指针。 */
    g_last_file = 0;

    /* 清空行号。 */
    g_last_line = 0u;

    /* 清空用户指针。 */
    g_last_user = 0;
}

/**
 * @brief 验证 MRT_ASSERT 在表达式失败时调用用户 hook。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_macro_dispatches_failed_expression();
 */
static void assert_macro_dispatches_failed_expression(void)
{
    /* 定义用户指针，用于验证 hook 透传。 */
    uint32_t user_cookie = 0x55aau;

    /* 清空捕获状态。 */
    ResetAssertCapture();

    /* 设置断言 hook。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_AssertSetHook(CaptureAssertHook, &user_cookie));

    /* 表达式成立时不应调用 hook。 */
    MRT_ASSERT(1u == 1u);
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)g_assert_count);

    /* 表达式失败时应调用 hook，但 host 测试继续运行。 */
    MRT_ASSERT(1u == 2u);

    /* hook 应捕获一次失败。 */
    MRT_TEST_ASSERT_EQ_U32(1u, (unsigned)g_assert_count);

    /* 失败表达式应由宏字符串化后传入。 */
    MRT_TEST_ASSERT_TRUE(g_last_expression != 0);
    MRT_TEST_ASSERT_TRUE(strcmp(g_last_expression, "1u == 2u") == 0);

    /* 文件名应非空。 */
    MRT_TEST_ASSERT_TRUE(g_last_file != 0);

    /* 行号应非零。 */
    MRT_TEST_ASSERT_TRUE(g_last_line != 0u);

    /* 用户指针应原样传回。 */
    MRT_TEST_ASSERT_TRUE(g_last_user == &user_cookie);
}

/**
 * @brief 验证直接调用 MRT_AssertFailed 会转发指定文件和行号。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_direct_failed_dispatch_preserves_location();
 */
static void assert_direct_failed_dispatch_preserves_location(void)
{
    /* 清空捕获状态。 */
    ResetAssertCapture();

    /* 设置断言 hook。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_AssertSetHook(CaptureAssertHook, 0));

    /* 直接分发一条断言失败记录。 */
    MRT_AssertFailed("manual check", "manual_file.c", 123u);

    /* hook 应捕获指定表达式、文件和行号。 */
    MRT_TEST_ASSERT_EQ_U32(1u, (unsigned)g_assert_count);
    MRT_TEST_ASSERT_TRUE(strcmp(g_last_expression, "manual check") == 0);
    MRT_TEST_ASSERT_TRUE(strcmp(g_last_file, "manual_file.c") == 0);
    MRT_TEST_ASSERT_EQ_U32(123u, (unsigned)g_last_line);
}

/**
 * @brief 验证清空 hook 后断言失败会被静默忽略。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_null_hook_disables_dispatch();
 */
static void assert_null_hook_disables_dispatch(void)
{
    /* 清空捕获状态。 */
    ResetAssertCapture();

    /* 清空断言 hook。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_AssertSetHook(0, 0));

    /* 未设置 hook 时，断言失败应静默返回。 */
    MRT_AssertFailed("ignored", "none", 1u);

    /* 捕获次数保持为 0。 */
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)g_assert_count);
}

/**
 * @brief 运行断言 hook 单元测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时进程提前退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证 MRT_ASSERT 宏分发失败表达式。 */
    assert_macro_dispatches_failed_expression();

    /* 验证直接失败分发保留定位信息。 */
    assert_direct_failed_dispatch_preserves_location();

    /* 验证空 hook 会关闭分发。 */
    assert_null_hook_disables_dispatch();

    /* 所有断言 hook 测试均通过。 */
    return 0;
}
