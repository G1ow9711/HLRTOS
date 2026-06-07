#include "mrt_test.h"
#include "myrtos/mrt_task.h"

/**
 * @brief 测试用任务入口函数。
 * @param arg 用户参数，本测试不使用。
 * @return void 无返回值。
 * @example
 * DummyTask(NULL);
 */
static void DummyTask(void *arg)
{
    /* 显式丢弃未使用参数，避免编译器告警。 */
    (void)arg;
}

/**
 * @brief 创建测试任务。
 * @param storage 任务控制块，不能为空。
 * @param stack 任务栈，不能为空。
 * @param out_task 输出任务句柄，不能为空。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * create_task(&storage, stack, &task);
 */
static void create_task(MRT_Task *storage, MRT_StackType *stack, MRT_TaskHandle *out_task)
{
    /* 创建一个无需启动调度器即可验证通知字段的任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("notify", DummyTask, 0, 3u, stack, 128u, storage, out_task));
}

/**
 * @brief 验证 set-bits 通知会按位累积通知值并标记 pending。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_notify_set_bits_accumulates_value();
 */
static void assert_notify_set_bits_accumulates_value(void)
{
    /* 定义任务控制块、栈和句柄。 */
    MRT_Task task_storage;
    MRT_StackType task_stack[128];
    MRT_TaskHandle task = 0;

    /* 创建测试任务。 */
    create_task(&task_storage, task_stack, &task);

    /* 第一次设置 bit0。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskNotify(task, 0x1u, MRT_NOTIFY_SET_BITS));

    /* 第二次设置 bit2，应与旧值累积。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskNotify(task, 0x4u, MRT_NOTIFY_SET_BITS));

    /* 通知值应为 bit0 和 bit2。 */
    MRT_TEST_ASSERT_EQ_U32(0x5u, (unsigned)task_storage.notify_value);

    /* 通知状态应标记为 pending。 */
    MRT_TEST_ASSERT_TRUE(task_storage.notify_pending);
}

/**
 * @brief 验证 increment 通知会把通知值作为计数递增。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_notify_increment_counts();
 */
static void assert_notify_increment_counts(void)
{
    /* 定义任务控制块、栈和句柄。 */
    MRT_Task task_storage;
    MRT_StackType task_stack[128];
    MRT_TaskHandle task = 0;

    /* 创建测试任务。 */
    create_task(&task_storage, task_stack, &task);

    /* 连续发送两次 increment 通知。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskNotify(task, 0u, MRT_NOTIFY_INCREMENT));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskNotify(task, 0u, MRT_NOTIFY_INCREMENT));

    /* 通知值应递增为 2。 */
    MRT_TEST_ASSERT_EQ_U32(2u, (unsigned)task_storage.notify_value);

    /* 通知状态应标记为 pending。 */
    MRT_TEST_ASSERT_TRUE(task_storage.notify_pending);
}

/**
 * @brief 验证 overwrite 和 no-overwrite 通知策略。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_notify_overwrite_and_no_overwrite_rules();
 */
static void assert_notify_overwrite_and_no_overwrite_rules(void)
{
    /* 定义任务控制块、栈和句柄。 */
    MRT_Task task_storage;
    MRT_StackType task_stack[128];
    MRT_TaskHandle task = 0;

    /* 创建测试任务。 */
    create_task(&task_storage, task_stack, &task);

    /* overwrite 应直接写入通知值。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskNotify(task, 0x10u, MRT_NOTIFY_OVERWRITE));
    MRT_TEST_ASSERT_EQ_U32(0x10u, (unsigned)task_storage.notify_value);

    /* pending 状态下 no-overwrite 应拒绝覆盖。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OBJECT_BUSY,
                           (unsigned)MRT_TaskNotify(task, 0x20u, MRT_NOTIFY_NO_OVERWRITE));
    MRT_TEST_ASSERT_EQ_U32(0x10u, (unsigned)task_storage.notify_value);

    /* 清除 pending 状态。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskNotifyStateClear(task));

    /* 无 pending 状态下 no-overwrite 应成功写入。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskNotify(task, 0x20u, MRT_NOTIFY_NO_OVERWRITE));
    MRT_TEST_ASSERT_EQ_U32(0x20u, (unsigned)task_storage.notify_value);
    MRT_TEST_ASSERT_TRUE(task_storage.notify_pending);
}

/**
 * @brief 验证通知值清位只清除指定 bit 且不改变 pending 状态。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_notify_value_clear_masks_selected_bits();
 */
static void assert_notify_value_clear_masks_selected_bits(void)
{
    /* 定义任务控制块、栈和句柄。 */
    MRT_Task task_storage;
    MRT_StackType task_stack[128];
    MRT_TaskHandle task = 0;

    /* 创建测试任务。 */
    create_task(&task_storage, task_stack, &task);

    /* 写入 bit0、bit1、bit2、bit3。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskNotify(task, 0xFu, MRT_NOTIFY_OVERWRITE));

    /* 清除 bit0 和 bit2。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskNotifyValueClear(task, 0x5u));

    /* 剩余 bit1 和 bit3。 */
    MRT_TEST_ASSERT_EQ_U32(0xAu, (unsigned)task_storage.notify_value);

    /* 清值不应清除 pending 状态。 */
    MRT_TEST_ASSERT_TRUE(task_storage.notify_pending);
}

/**
 * @brief 验证通知动作 API 参数校验。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_notify_actions_reject_invalid_arguments();
 */
static void assert_notify_actions_reject_invalid_arguments(void)
{
    /* 定义任务控制块、栈和句柄。 */
    MRT_Task task_storage;
    MRT_StackType task_stack[128];
    MRT_TaskHandle task = 0;

    /* 创建测试任务。 */
    create_task(&task_storage, task_stack, &task);

    /* 空任务句柄应返回参数错误。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_TaskNotify(0, 1u, MRT_NOTIFY_SET_BITS));

    /* 非法通知动作应返回参数错误。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_TaskNotify(task, 1u, (MRT_NotifyAction)99u));

    /* 清除空任务通知状态应返回参数错误。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT, (unsigned)MRT_TaskNotifyStateClear(0));

    /* 清除空任务通知值应返回参数错误。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT, (unsigned)MRT_TaskNotifyValueClear(0, 1u));
}

/**
 * @brief 运行任务通知动作测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证 set-bits 通知动作。 */
    assert_notify_set_bits_accumulates_value();

    /* 验证 increment 通知动作。 */
    assert_notify_increment_counts();

    /* 验证 overwrite/no-overwrite 策略。 */
    assert_notify_overwrite_and_no_overwrite_rules();

    /* 验证通知值清位。 */
    assert_notify_value_clear_masks_selected_bits();

    /* 验证参数校验。 */
    assert_notify_actions_reject_invalid_arguments();

    /* 所有任务通知动作测试均通过。 */
    return 0;
}
