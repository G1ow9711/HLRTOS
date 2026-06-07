#include "mrt_test.h"
#include "myrtos/mrt_event_group.h"
#include "myrtos/mrt_kernel.h"
#include "myrtos/mrt_port.h"
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
 * @brief 验证 ISR set bits 在没有等待任务时只更新 bit 且不请求切换。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_isr_set_without_waiter_sets_bits_without_yield();
 */
static void assert_isr_set_without_waiter_sets_bits_without_yield(void)
{
    /* 初始化端口 mock。 */
    MRT_PortInitialize();

    /* 切换到模拟 ISR 上下文。 */
    MRT_PortMockSetInsideISR(true);

    /* 定义事件组控制块和句柄。 */
    MRT_EventGroup storage;
    MRT_EventGroupHandle events = 0;

    /* 创建空事件组。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_EventGroupCreateStatic(&storage, &events));

    /* 预置 yield 标志为 true，用于确认 API 会写回 false。 */
    bool should_yield = true;

    /* ISR 设置 bit1 应成功。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_EventGroupSetBitsFromISR(events, 0x2u, &should_yield));

    /* 没有唤醒任务时不需要切换。 */
    MRT_TEST_ASSERT_TRUE(!should_yield);

    /* 事件组 bit1 应已置位。 */
    MRT_TEST_ASSERT_EQ_U32(0x2u, (unsigned)MRT_EventGroupGetBits(events));

    /* 恢复任务上下文，避免影响后续测试。 */
    MRT_PortMockSetInsideISR(false);
}

/**
 * @brief 验证 ISR set bits 唤醒等待者时设置 should_yield 但不立即切换。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_isr_set_wakes_waiter_and_defers_switch();
 */
static void assert_isr_set_wakes_waiter_and_defers_switch(void)
{
    /* 定义高优先级等待任务控制块。 */
    MRT_Task waiter_storage;

    /* 定义低优先级当前任务控制块。 */
    MRT_Task low_storage;

    /* 定义任务栈。 */
    MRT_StackType waiter_stack[128];
    MRT_StackType low_stack[128];

    /* 定义任务句柄。 */
    MRT_TaskHandle waiter_task = 0;
    MRT_TaskHandle low_task = 0;

    /* 定义事件组控制块和句柄。 */
    MRT_EventGroup event_storage;
    MRT_EventGroupHandle events = 0;

    /* 初始化内核。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 创建低优先级任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("low", DummyTask, 0, 1u, low_stack, 128u, &low_storage, &low_task));

    /* 创建高优先级等待任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("waiter",
                                                          DummyTask,
                                                          0,
                                                          5u,
                                                          waiter_stack,
                                                          128u,
                                                          &waiter_storage,
                                                          &waiter_task));

    /* 创建空事件组。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_EventGroupCreateStatic(&event_storage, &events));

    /* 启动后高优先级任务运行。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelStart());
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == waiter_task);

    /* 高优先级任务等待 bit2。 */
    MRT_EventBits observed_bits = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_TIMEOUT,
                           (unsigned)MRT_EventGroupWaitBits(events, 0x4u, false, false, 20u, &observed_bits));

    /* 低优先级任务接管运行。 */
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == low_task);

    /* 切换到模拟 ISR 上下文。 */
    MRT_PortMockSetInsideISR(true);

    /* 定义 yield 输出标志。 */
    bool should_yield = false;

    /* ISR 设置 bit2 应唤醒等待任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_EventGroupSetBitsFromISR(events, 0x4u, &should_yield));

    /* ISR 中唤醒高优先级任务应请求退出 ISR 后切换。 */
    MRT_TEST_ASSERT_TRUE(should_yield);

    /* ISR API 不应立即切换当前任务。 */
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == low_task);

    /* 等待任务应回到 ready 状态。 */
    MRT_TaskState waiter_state = MRT_TASK_STATE_DELETED;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskGetState(waiter_task, &waiter_state));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_TASK_STATE_READY, (unsigned)waiter_state);

    /* 等待链表应清空。 */
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)MRT_ListGetCount(&event_storage.waiting_tasks));

    /* 恢复任务上下文。 */
    MRT_PortMockSetInsideISR(false);
}

/**
 * @brief 验证 FromISR API 拒绝任务上下文调用。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_isr_set_rejects_task_context();
 */
static void assert_isr_set_rejects_task_context(void)
{
    /* 初始化端口 mock，默认是任务上下文。 */
    MRT_PortInitialize();

    /* 显式保持任务上下文。 */
    MRT_PortMockSetInsideISR(false);

    /* 定义事件组控制块和句柄。 */
    MRT_EventGroup storage;
    MRT_EventGroupHandle events = 0;

    /* 创建空事件组。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_EventGroupCreateStatic(&storage, &events));

    /* 预置 yield 标志为 true。 */
    bool should_yield = true;

    /* 任务上下文调用 FromISR API 应被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_CONTEXT,
                           (unsigned)MRT_EventGroupSetBitsFromISR(events, 0x1u, &should_yield));

    /* 非法上下文不应请求切换。 */
    MRT_TEST_ASSERT_TRUE(!should_yield);
}

/**
 * @brief 验证 ISR set bits 参数校验。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_isr_set_rejects_invalid_arguments();
 */
static void assert_isr_set_rejects_invalid_arguments(void)
{
    /* 初始化端口 mock。 */
    MRT_PortInitialize();

    /* 切换到模拟 ISR 上下文。 */
    MRT_PortMockSetInsideISR(true);

    /* 定义事件组控制块和句柄。 */
    MRT_EventGroup storage;
    MRT_EventGroupHandle events = 0;

    /* 创建空事件组。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_EventGroupCreateStatic(&storage, &events));

    /* 空事件组句柄应返回参数错误。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_EventGroupSetBitsFromISR(0, 0x1u, 0));

    /* 设置 0 bit 应返回参数错误。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_EventGroupSetBitsFromISR(events, 0u, 0));

    /* 恢复任务上下文。 */
    MRT_PortMockSetInsideISR(false);
}

/**
 * @brief 运行事件组 ISR set 测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证无等待者 ISR set。 */
    assert_isr_set_without_waiter_sets_bits_without_yield();

    /* 验证 ISR set 唤醒等待者但延迟切换。 */
    assert_isr_set_wakes_waiter_and_defers_switch();

    /* 验证非法上下文。 */
    assert_isr_set_rejects_task_context();

    /* 验证参数校验。 */
    assert_isr_set_rejects_invalid_arguments();

    /* 所有事件组 ISR set 测试均通过。 */
    return 0;
}
