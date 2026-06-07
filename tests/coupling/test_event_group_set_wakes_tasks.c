#include "mrt_test.h"
#include "myrtos/mrt_event_group.h"
#include "myrtos/mrt_kernel.h"
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
 * @brief 创建三个测试任务并启动调度器。
 * @param low_storage 低优先级任务控制块，不能为空。
 * @param mid_storage 中优先级任务控制块，不能为空。
 * @param high_storage 高优先级任务控制块，不能为空。
 * @param low_stack 低优先级任务栈，不能为空。
 * @param mid_stack 中优先级任务栈，不能为空。
 * @param high_stack 高优先级任务栈，不能为空。
 * @param out_low 输出低优先级任务句柄，不能为空。
 * @param out_mid 输出中优先级任务句柄，不能为空。
 * @param out_high 输出高优先级任务句柄，不能为空。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * create_three_tasks_and_start(&low, &mid, &high, low_stack, mid_stack, high_stack, &low_task, &mid_task, &high_task);
 */
static void create_three_tasks_and_start(MRT_Task *low_storage,
                                         MRT_Task *mid_storage,
                                         MRT_Task *high_storage,
                                         MRT_StackType *low_stack,
                                         MRT_StackType *mid_stack,
                                         MRT_StackType *high_stack,
                                         MRT_TaskHandle *out_low,
                                         MRT_TaskHandle *out_mid,
                                         MRT_TaskHandle *out_high)
{
    /* 初始化内核。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 创建低优先级任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("low", DummyTask, 0, 1u, low_stack, 128u, low_storage, out_low));

    /* 创建中优先级任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("mid", DummyTask, 0, 4u, mid_stack, 128u, mid_storage, out_mid));

    /* 创建高优先级任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("high", DummyTask, 0, 5u, high_stack, 128u, high_storage, out_high));

    /* 启动调度器后应运行最高优先级任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelStart());
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == *out_high);
}

/**
 * @brief 验证设置事件 bit 会唤醒所有匹配等待者并切换到最高优先级等待者。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_set_bits_wakes_all_matching_waiters();
 */
static void assert_set_bits_wakes_all_matching_waiters(void)
{
    /* 定义三个任务控制块。 */
    MRT_Task low_storage;
    MRT_Task mid_storage;
    MRT_Task high_storage;

    /* 定义三个任务栈。 */
    MRT_StackType low_stack[128];
    MRT_StackType mid_stack[128];
    MRT_StackType high_stack[128];

    /* 定义三个任务句柄。 */
    MRT_TaskHandle low_task = 0;
    MRT_TaskHandle mid_task = 0;
    MRT_TaskHandle high_task = 0;

    /* 定义事件组控制块和句柄。 */
    MRT_EventGroup event_storage;
    MRT_EventGroupHandle events = 0;

    /* 创建任务并启动调度器。 */
    create_three_tasks_and_start(&low_storage,
                                 &mid_storage,
                                 &high_storage,
                                 low_stack,
                                 mid_stack,
                                 high_stack,
                                 &low_task,
                                 &mid_task,
                                 &high_task);

    /* 创建空事件组。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_EventGroupCreateStatic(&event_storage, &events));

    /* 高优先级任务等待 bit0 或 bit1。 */
    MRT_EventBits observed_bits = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_TIMEOUT,
                           (unsigned)MRT_EventGroupWaitBits(events, 0x3u, false, false, 20u, &observed_bits));

    /* 高优先级任务阻塞后，中优先级任务应运行。 */
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == mid_task);

    /* 中优先级任务等待 bit0 和 bit1 同时满足。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_TIMEOUT,
                           (unsigned)MRT_EventGroupWaitBits(events, 0x3u, true, false, 20u, &observed_bits));

    /* 两个等待者都阻塞后，低优先级任务应运行。 */
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == low_task);

    /* 事件组等待链表应包含两个等待任务。 */
    MRT_TEST_ASSERT_EQ_U32(2u, (unsigned)MRT_ListGetCount(&event_storage.waiting_tasks));

    /* 设置 bit0 和 bit1，应同时满足两个等待者。 */
    MRT_EventBits bits_after_set = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_EventGroupSetBits(events, 0x3u, &bits_after_set));

    /* 设置完成后 bit 集应包含 bit0 和 bit1。 */
    MRT_TEST_ASSERT_EQ_U32(0x3u, (unsigned)bits_after_set);

    /* 最高优先级等待者应立即抢占低优先级任务。 */
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == high_task);

    /* 中优先级等待者也应被唤醒为 ready。 */
    MRT_TaskState mid_state = MRT_TASK_STATE_DELETED;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskGetState(mid_task, &mid_state));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_TASK_STATE_READY, (unsigned)mid_state);

    /* 等待链表应被清空。 */
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)MRT_ListGetCount(&event_storage.waiting_tasks));
}

/**
 * @brief 验证 clear-on-exit 在多等待者匹配后统一清除匹配 bit。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_clear_on_exit_clears_union_after_matching_all_waiters();
 */
static void assert_clear_on_exit_clears_union_after_matching_all_waiters(void)
{
    /* 定义三个任务控制块。 */
    MRT_Task low_storage;
    MRT_Task mid_storage;
    MRT_Task high_storage;

    /* 定义三个任务栈。 */
    MRT_StackType low_stack[128];
    MRT_StackType mid_stack[128];
    MRT_StackType high_stack[128];

    /* 定义三个任务句柄。 */
    MRT_TaskHandle low_task = 0;
    MRT_TaskHandle mid_task = 0;
    MRT_TaskHandle high_task = 0;

    /* 定义事件组控制块和句柄。 */
    MRT_EventGroup event_storage;
    MRT_EventGroupHandle events = 0;

    /* 创建任务并启动调度器。 */
    create_three_tasks_and_start(&low_storage,
                                 &mid_storage,
                                 &high_storage,
                                 low_stack,
                                 mid_stack,
                                 high_stack,
                                 &low_task,
                                 &mid_task,
                                 &high_task);

    /* 创建空事件组。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_EventGroupCreateStatic(&event_storage, &events));

    /* 高优先级任务等待 bit0，并请求满足后清位。 */
    MRT_EventBits observed_bits = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_TIMEOUT,
                           (unsigned)MRT_EventGroupWaitBits(events, 0x1u, false, true, 20u, &observed_bits));

    /* 中优先级任务等待 bit0 和 bit1，并请求满足后清位。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_TIMEOUT,
                           (unsigned)MRT_EventGroupWaitBits(events, 0x3u, true, true, 20u, &observed_bits));

    /* 两个等待者都阻塞后，低优先级任务应运行。 */
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == low_task);

    /* 设置 bit0、bit1 和一个无关 bit2，应满足两个等待者。 */
    MRT_EventBits bits_after_set = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_EventGroupSetBits(events, 0x7u, &bits_after_set));

    /* 输出快照应是清位前的完整 bit 集。 */
    MRT_TEST_ASSERT_EQ_U32(0x7u, (unsigned)bits_after_set);

    /* bit0 和 bit1 属于被匹配等待条件，应被清除；无关 bit2 应保留。 */
    MRT_TEST_ASSERT_EQ_U32(0x4u, (unsigned)MRT_EventGroupGetBits(events));

    /* 最高优先级等待者应已经运行。 */
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == high_task);
}

/**
 * @brief 运行事件组置位唤醒等待者测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证 set bits 唤醒全部匹配等待者。 */
    assert_set_bits_wakes_all_matching_waiters();

    /* 验证多等待者 clear-on-exit 清位规则。 */
    assert_clear_on_exit_clears_union_after_matching_all_waiters();

    /* 所有事件组置位唤醒测试均通过。 */
    return 0;
}
