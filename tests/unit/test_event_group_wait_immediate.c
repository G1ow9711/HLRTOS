#include "mrt_test.h"
#include "myrtos/mrt_event_group.h"

/**
 * @brief 创建测试用事件组。
 * @param storage 事件组控制块，不能为空。
 * @param out_group 输出事件组句柄，不能为空。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * create_event_group(&storage, &group);
 */
static void create_event_group(MRT_EventGroup *storage, MRT_EventGroupHandle *out_group)
{
    /* 使用静态控制块创建事件组。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_EventGroupCreateStatic(storage, out_group));
}

/**
 * @brief 验证 wait-any 在任意请求 bit 已置位时立即成功。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_wait_any_matches_available_bit();
 */
static void assert_wait_any_matches_available_bit(void)
{
    /* 定义事件组控制块。 */
    MRT_EventGroup storage;

    /* 定义事件组句柄。 */
    MRT_EventGroupHandle group = 0;

    /* 创建事件组。 */
    create_event_group(&storage, &group);

    /* 设置 bit1。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_EventGroupSetBits(group, 0x2u, 0));

    /* 定义等待返回的 bit 快照。 */
    MRT_EventBits observed_bits = 0u;

    /* 等待 bit1 或 bit2，当前 bit1 已满足，应立即返回成功。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_EventGroupWaitBits(group, 0x6u, false, false, 0u, &observed_bits));

    /* 输出应为等待成功时的事件组快照。 */
    MRT_TEST_ASSERT_EQ_U32(0x2u, (unsigned)observed_bits);

    /* 未请求退出清位，事件组 bit 应保持不变。 */
    MRT_TEST_ASSERT_EQ_U32(0x2u, (unsigned)MRT_EventGroupGetBits(group));
}

/**
 * @brief 验证 wait-all 只有全部请求 bit 已置位时才立即成功。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_wait_all_requires_every_requested_bit();
 */
static void assert_wait_all_requires_every_requested_bit(void)
{
    /* 定义事件组控制块。 */
    MRT_EventGroup storage;

    /* 定义事件组句柄。 */
    MRT_EventGroupHandle group = 0;

    /* 创建事件组。 */
    create_event_group(&storage, &group);

    /* 设置 bit1 和 bit2。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_EventGroupSetBits(group, 0x6u, 0));

    /* 定义等待返回的 bit 快照。 */
    MRT_EventBits observed_bits = 0u;

    /* 等待 bit1 与 bit2 全部满足。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_EventGroupWaitBits(group, 0x6u, true, false, 0u, &observed_bits));

    /* 输出应包含全部已置位 bit。 */
    MRT_TEST_ASSERT_EQ_U32(0x6u, (unsigned)observed_bits);
}

/**
 * @brief 验证 clear-on-exit 在成功等待后清除请求范围内已匹配的 bit。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_clear_on_exit_clears_matched_bits_after_success();
 */
static void assert_clear_on_exit_clears_matched_bits_after_success(void)
{
    /* 定义事件组控制块。 */
    MRT_EventGroup storage;

    /* 定义事件组句柄。 */
    MRT_EventGroupHandle group = 0;

    /* 创建事件组。 */
    create_event_group(&storage, &group);

    /* 设置 bit0、bit1 和 bit2。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_EventGroupSetBits(group, 0x7u, 0));

    /* 定义等待返回的 bit 快照。 */
    MRT_EventBits observed_bits = 0u;

    /* 等待 bit1 或 bit2，并请求退出时清除匹配 bit。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_EventGroupWaitBits(group, 0x6u, false, true, 0u, &observed_bits));

    /* 输出应是清位前的事件组快照，便于调用方判断触发原因。 */
    MRT_TEST_ASSERT_EQ_U32(0x7u, (unsigned)observed_bits);

    /* bit1 和 bit2 被清除，未等待的 bit0 保留。 */
    MRT_TEST_ASSERT_EQ_U32(0x1u, (unsigned)MRT_EventGroupGetBits(group));
}

/**
 * @brief 验证非阻塞等待无匹配 bit 时返回空状态并输出当前 bit 快照。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_no_match_without_timeout_returns_empty();
 */
static void assert_no_match_without_timeout_returns_empty(void)
{
    /* 定义事件组控制块。 */
    MRT_EventGroup storage;

    /* 定义事件组句柄。 */
    MRT_EventGroupHandle group = 0;

    /* 创建事件组。 */
    create_event_group(&storage, &group);

    /* 设置 bit0。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_EventGroupSetBits(group, 0x1u, 0));

    /* 定义等待返回的 bit 快照。 */
    MRT_EventBits observed_bits = 0u;

    /* 非阻塞等待 bit1，当前不满足，应返回对象为空。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OBJECT_EMPTY,
                           (unsigned)MRT_EventGroupWaitBits(group, 0x2u, false, false, 0u, &observed_bits));

    /* 输出当前 bit 快照，帮助调用方诊断为何未满足。 */
    MRT_TEST_ASSERT_EQ_U32(0x1u, (unsigned)observed_bits);
}

/**
 * @brief 验证事件等待参数校验。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_wait_bits_rejects_invalid_arguments();
 */
static void assert_wait_bits_rejects_invalid_arguments(void)
{
    /* 定义事件组控制块。 */
    MRT_EventGroup storage;

    /* 定义事件组句柄。 */
    MRT_EventGroupHandle group = 0;

    /* 创建事件组。 */
    create_event_group(&storage, &group);

    /* 空事件组句柄应返回参数错误。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_EventGroupWaitBits(0, 0x1u, false, false, 0u, 0));

    /* 等待 0 bit 没有意义，应返回参数错误。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_EventGroupWaitBits(group, 0u, false, false, 0u, 0));
}

/**
 * @brief 运行事件组即时等待测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证 wait-any 即时成功。 */
    assert_wait_any_matches_available_bit();

    /* 验证 wait-all 即时成功。 */
    assert_wait_all_requires_every_requested_bit();

    /* 验证 clear-on-exit 语义。 */
    assert_clear_on_exit_clears_matched_bits_after_success();

    /* 验证非阻塞无匹配路径。 */
    assert_no_match_without_timeout_returns_empty();

    /* 验证参数校验。 */
    assert_wait_bits_rejects_invalid_arguments();

    /* 所有事件组即时等待测试均通过。 */
    return 0;
}
