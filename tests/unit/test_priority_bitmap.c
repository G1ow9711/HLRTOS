#include "mrt_test.h"
#include "myrtos/mrt_priority.h"

/**
 * @brief 验证空位图无法找到最高优先级。
 *
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_empty_bitmap_has_no_highest_priority();
 */
static void assert_empty_bitmap_has_no_highest_priority(void)
{
    /* 定义待测优先级位图。 */
    MRT_PriorityBitmap bitmap;

    /* 初始化位图为空。 */
    MRT_PriorityBitmapInitialize(&bitmap);

    /* 空位图不应找到任何最高优先级。 */
    MRT_TEST_ASSERT_TRUE(!MRT_PriorityBitmapFindHighest(&bitmap, 0));
}

/**
 * @brief 验证位图能返回当前最高的已置位优先级。
 *
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_find_highest_returns_largest_set_priority();
 */
static void assert_find_highest_returns_largest_set_priority(void)
{
    /* 定义待测优先级位图。 */
    MRT_PriorityBitmap bitmap;

    /* 定义输出优先级变量。 */
    MRT_Priority highest = 0u;

    /* 初始化位图为空。 */
    MRT_PriorityBitmapInitialize(&bitmap);

    /* 设置较低优先级 3。 */
    MRT_PriorityBitmapSet(&bitmap, 3u);

    /* 设置较高优先级 7。 */
    MRT_PriorityBitmapSet(&bitmap, 7u);

    /* 验证查找操作成功。 */
    MRT_TEST_ASSERT_TRUE(MRT_PriorityBitmapFindHighest(&bitmap, &highest));

    /* 验证返回最大已置位优先级 7。 */
    MRT_TEST_ASSERT_EQ_U32(7u, highest);
}

/**
 * @brief 验证清除最高优先级后会回退到下一个已置位优先级。
 *
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_clear_highest_exposes_next_priority();
 */
static void assert_clear_highest_exposes_next_priority(void)
{
    /* 定义待测优先级位图。 */
    MRT_PriorityBitmap bitmap;

    /* 定义输出优先级变量。 */
    MRT_Priority highest = 0u;

    /* 初始化位图为空。 */
    MRT_PriorityBitmapInitialize(&bitmap);

    /* 设置优先级 3。 */
    MRT_PriorityBitmapSet(&bitmap, 3u);

    /* 设置优先级 7。 */
    MRT_PriorityBitmapSet(&bitmap, 7u);

    /* 清除当前最高优先级 7。 */
    MRT_PriorityBitmapClear(&bitmap, 7u);

    /* 验证仍然能找到剩余优先级。 */
    MRT_TEST_ASSERT_TRUE(MRT_PriorityBitmapFindHighest(&bitmap, &highest));

    /* 验证剩余最高优先级为 3。 */
    MRT_TEST_ASSERT_EQ_U32(3u, highest);

    /* 清除最后一个优先级。 */
    MRT_PriorityBitmapClear(&bitmap, 3u);

    /* 验证位图再次变为空。 */
    MRT_TEST_ASSERT_TRUE(!MRT_PriorityBitmapFindHighest(&bitmap, &highest));
}

/**
 * @brief 验证越界优先级不会污染有效位。
 *
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_out_of_range_priority_is_ignored();
 */
static void assert_out_of_range_priority_is_ignored(void)
{
    /* 定义待测优先级位图。 */
    MRT_PriorityBitmap bitmap;

    /* 定义输出优先级变量。 */
    MRT_Priority highest = 0u;

    /* 初始化位图为空。 */
    MRT_PriorityBitmapInitialize(&bitmap);

    /* 尝试设置越界优先级，应被安全忽略。 */
    MRT_PriorityBitmapSet(&bitmap, MRT_CFG_MAX_PRIORITIES);

    /* 越界设置不能让空位图变成非空。 */
    MRT_TEST_ASSERT_TRUE(!MRT_PriorityBitmapFindHighest(&bitmap, &highest));

    /* 设置最大有效优先级。 */
    MRT_PriorityBitmapSet(&bitmap, MRT_CFG_MAX_PRIORITIES - 1u);

    /* 尝试清除越界优先级，应被安全忽略。 */
    MRT_PriorityBitmapClear(&bitmap, MRT_CFG_MAX_PRIORITIES);

    /* 验证最大有效优先级仍然存在。 */
    MRT_TEST_ASSERT_TRUE(MRT_PriorityBitmapFindHighest(&bitmap, &highest));

    /* 验证最高优先级等于最大有效优先级。 */
    MRT_TEST_ASSERT_EQ_U32(MRT_CFG_MAX_PRIORITIES - 1u, highest);
}

/**
 * @brief 运行优先级位图模块全部单元测试。
 *
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证空位图行为。 */
    assert_empty_bitmap_has_no_highest_priority();

    /* 验证最高优先级查找。 */
    assert_find_highest_returns_largest_set_priority();

    /* 验证清除后回退到下一个优先级。 */
    assert_clear_highest_exposes_next_priority();

    /* 验证越界优先级保护。 */
    assert_out_of_range_priority_is_ignored();

    /* 所有优先级位图测试均通过，返回 0。 */
    return 0;
}
