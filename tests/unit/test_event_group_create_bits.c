#include "mrt_test.h"
#include "myrtos/mrt_event_group.h"

/**
 * @brief 验证静态事件组创建后初始 bit 为 0。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_create_static_initializes_empty_bits();
 */
static void assert_create_static_initializes_empty_bits(void)
{
    /* 定义事件组控制块。 */
    MRT_EventGroup storage;

    /* 定义事件组句柄。 */
    MRT_EventGroupHandle group = 0;

    /* 静态创建事件组。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_EventGroupCreateStatic(&storage, &group));

    /* 创建成功后输出句柄应指向调用方提供的控制块。 */
    MRT_TEST_ASSERT_TRUE(group == &storage);

    /* 新事件组初始不含任何事件 bit。 */
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)MRT_EventGroupGetBits(group));
}

/**
 * @brief 验证设置事件 bit 使用按位或语义并写回当前 bit 集。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_set_bits_or_accumulates_bits();
 */
static void assert_set_bits_or_accumulates_bits(void)
{
    /* 定义事件组控制块。 */
    MRT_EventGroup storage;

    /* 定义事件组句柄。 */
    MRT_EventGroupHandle group = 0;

    /* 定义设置后的 bit 输出。 */
    MRT_EventBits bits_after_set = 0u;

    /* 创建空事件组。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_EventGroupCreateStatic(&storage, &group));

    /* 设置 bit0 和 bit2。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_EventGroupSetBits(group, 0x5u, &bits_after_set));

    /* 输出值应包含已经设置的两个 bit。 */
    MRT_TEST_ASSERT_EQ_U32(0x5u, (unsigned)bits_after_set);

    /* 再设置 bit1，应与已有 bit 累积。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_EventGroupSetBits(group, 0x2u, &bits_after_set));

    /* 当前 bit 集应为 bit0、bit1、bit2 均置位。 */
    MRT_TEST_ASSERT_EQ_U32(0x7u, (unsigned)bits_after_set);
    MRT_TEST_ASSERT_EQ_U32(0x7u, (unsigned)MRT_EventGroupGetBits(group));
}

/**
 * @brief 验证清除事件 bit 使用按位与非语义并写回当前 bit 集。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_clear_bits_removes_selected_bits();
 */
static void assert_clear_bits_removes_selected_bits(void)
{
    /* 定义事件组控制块。 */
    MRT_EventGroup storage;

    /* 定义事件组句柄。 */
    MRT_EventGroupHandle group = 0;

    /* 定义操作后的 bit 输出。 */
    MRT_EventBits bits_after_operation = 0u;

    /* 创建空事件组。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_EventGroupCreateStatic(&storage, &group));

    /* 先设置 bit0、bit1、bit2。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_EventGroupSetBits(group, 0x7u, &bits_after_operation));

    /* 清除 bit1。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_EventGroupClearBits(group, 0x2u, &bits_after_operation));

    /* 当前 bit 集应只保留 bit0 和 bit2。 */
    MRT_TEST_ASSERT_EQ_U32(0x5u, (unsigned)bits_after_operation);
    MRT_TEST_ASSERT_EQ_U32(0x5u, (unsigned)MRT_EventGroupGetBits(group));
}

/**
 * @brief 验证事件组基础 API 的参数校验。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_event_group_basic_api_rejects_invalid_arguments();
 */
static void assert_event_group_basic_api_rejects_invalid_arguments(void)
{
    /* 定义事件组控制块。 */
    MRT_EventGroup storage;

    /* 定义事件组句柄。 */
    MRT_EventGroupHandle group = 0;

    /* 创建时控制块不能为空。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_EventGroupCreateStatic(0, &group));

    /* 创建时输出句柄不能为空。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_EventGroupCreateStatic(&storage, 0));

    /* 创建合法事件组供后续参数校验使用。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_EventGroupCreateStatic(&storage, &group));

    /* 空句柄设置 bit 应返回参数错误。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_EventGroupSetBits(0, 0x1u, 0));

    /* 设置 0 bit 没有意义，应返回参数错误。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_EventGroupSetBits(group, 0u, 0));

    /* 空句柄清除 bit 应返回参数错误。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_EventGroupClearBits(0, 0x1u, 0));

    /* 清除 0 bit 没有意义，应返回参数错误。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_EventGroupClearBits(group, 0u, 0));

    /* 空句柄读取 bit 应返回 0。 */
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)MRT_EventGroupGetBits(0));
}

/**
 * @brief 运行事件组创建和 bit 操作测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证静态创建初始状态。 */
    assert_create_static_initializes_empty_bits();

    /* 验证 set bits 累积语义。 */
    assert_set_bits_or_accumulates_bits();

    /* 验证 clear bits 清除语义。 */
    assert_clear_bits_removes_selected_bits();

    /* 验证参数校验。 */
    assert_event_group_basic_api_rejects_invalid_arguments();

    /* 所有事件组创建和 bit 操作测试均通过。 */
    return 0;
}
