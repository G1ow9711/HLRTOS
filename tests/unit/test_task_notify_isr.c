#include "mrt_test.h"
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
    /* 创建一个普通测试任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("notify", DummyTask, 0, 3u, stack, 128u, storage, out_task));
}

/**
 * @brief 验证 ISR notify 在没有阻塞等待者时写入通知且不请求切换。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_isr_notify_without_waiter_sets_value_without_yield();
 */
static void assert_isr_notify_without_waiter_sets_value_without_yield(void)
{
    /* 初始化端口 mock。 */
    MRT_PortInitialize();

    /* 切换到模拟 ISR 上下文。 */
    MRT_PortMockSetInsideISR(true);

    /* 定义任务控制块、栈和句柄。 */
    MRT_Task task_storage;
    MRT_StackType task_stack[128];
    MRT_TaskHandle task = 0;

    /* 创建测试任务。 */
    create_task(&task_storage, task_stack, &task);

    /* 预置 yield 标志为 true，用于确认 API 会写回 false。 */
    bool should_yield = true;

    /* ISR overwrite 通知应成功。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskNotifyFromISR(task, 0x11u, MRT_NOTIFY_OVERWRITE, &should_yield));

    /* 没有唤醒任务时不需要切换。 */
    MRT_TEST_ASSERT_TRUE(!should_yield);

    /* 通知值应写入并标记 pending。 */
    MRT_TEST_ASSERT_EQ_U32(0x11u, (unsigned)task_storage.notify_value);
    MRT_TEST_ASSERT_TRUE(task_storage.notify_pending);

    /* 恢复任务上下文。 */
    MRT_PortMockSetInsideISR(false);
}

/**
 * @brief 验证 ISR notify 唤醒阻塞等待者时设置 should_yield 且不立即切换。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_isr_notify_wakes_waiter_and_defers_switch();
 */
static void assert_isr_notify_wakes_waiter_and_defers_switch(void)
{
    /* 定义高低优先级任务控制块。 */
    MRT_Task high_storage;
    MRT_Task low_storage;

    /* 定义任务栈。 */
    MRT_StackType high_stack[128];
    MRT_StackType low_stack[128];

    /* 定义任务句柄。 */
    MRT_TaskHandle high_task = 0;
    MRT_TaskHandle low_task = 0;

    /* 初始化内核。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 创建低优先级任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("low", DummyTask, 0, 1u, low_stack, 128u, &low_storage, &low_task));

    /* 创建高优先级任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("high", DummyTask, 0, 5u, high_stack, 128u, &high_storage, &high_task));

    /* 启动后高优先级任务运行。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelStart());
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == high_task);

    /* 高优先级任务等待通知。 */
    MRT_NotifyValue value = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_TIMEOUT, (unsigned)MRT_TaskNotifyWait(0u, 0u, 20u, &value));

    /* 低优先级任务接管运行。 */
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == low_task);

    /* 切换到模拟 ISR 上下文。 */
    MRT_PortMockSetInsideISR(true);

    /* 定义 yield 输出标志。 */
    bool should_yield = false;

    /* ISR 通知高优先级等待任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskNotifyFromISR(high_task, 0xAAu, MRT_NOTIFY_OVERWRITE, &should_yield));

    /* ISR 中唤醒高优先级任务应请求延迟切换。 */
    MRT_TEST_ASSERT_TRUE(should_yield);

    /* ISR API 不应立即切换当前任务。 */
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == low_task);

    /* 高优先级等待任务应回到 ready 状态。 */
    MRT_TaskState high_state = MRT_TASK_STATE_DELETED;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskGetState(high_task, &high_state));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_TASK_STATE_READY, (unsigned)high_state);

    /* 通知值应保持 pending，供真实恢复路径读取。 */
    MRT_TEST_ASSERT_TRUE(high_storage.notify_pending);
    MRT_TEST_ASSERT_EQ_U32(0xAAu, (unsigned)high_storage.notify_value);

    /* 恢复任务上下文。 */
    MRT_PortMockSetInsideISR(false);
}

/**
 * @brief 验证 FromISR notify 拒绝任务上下文调用。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_isr_notify_rejects_task_context();
 */
static void assert_isr_notify_rejects_task_context(void)
{
    /* 初始化端口 mock，默认是任务上下文。 */
    MRT_PortInitialize();

    /* 显式保持任务上下文。 */
    MRT_PortMockSetInsideISR(false);

    /* 定义任务控制块、栈和句柄。 */
    MRT_Task task_storage;
    MRT_StackType task_stack[128];
    MRT_TaskHandle task = 0;

    /* 创建测试任务。 */
    create_task(&task_storage, task_stack, &task);

    /* 预置 yield 标志为 true。 */
    bool should_yield = true;

    /* 任务上下文调用 FromISR API 应被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_CONTEXT,
                           (unsigned)MRT_TaskNotifyFromISR(task, 1u, MRT_NOTIFY_SET_BITS, &should_yield));

    /* 非法上下文不应请求切换。 */
    MRT_TEST_ASSERT_TRUE(!should_yield);
}

/**
 * @brief 验证 ISR no-overwrite 遇到 pending 通知时返回忙且不改旧值。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_isr_notify_no_overwrite_busy_preserves_value();
 */
static void assert_isr_notify_no_overwrite_busy_preserves_value(void)
{
    /* 初始化端口 mock。 */
    MRT_PortInitialize();

    /* 定义任务控制块、栈和句柄。 */
    MRT_Task task_storage;
    MRT_StackType task_stack[128];
    MRT_TaskHandle task = 0;

    /* 创建测试任务。 */
    create_task(&task_storage, task_stack, &task);

    /* 任务上下文先写入 pending 通知。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskNotify(task, 0x10u, MRT_NOTIFY_OVERWRITE));

    /* 切换到模拟 ISR 上下文。 */
    MRT_PortMockSetInsideISR(true);

    /* 定义 yield 输出标志。 */
    bool should_yield = true;

    /* no-overwrite 遇到 pending 通知应返回忙。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OBJECT_BUSY,
                           (unsigned)MRT_TaskNotifyFromISR(task, 0x20u, MRT_NOTIFY_NO_OVERWRITE, &should_yield));

    /* 忙路径不应请求切换。 */
    MRT_TEST_ASSERT_TRUE(!should_yield);

    /* 原通知值应保持不变。 */
    MRT_TEST_ASSERT_EQ_U32(0x10u, (unsigned)task_storage.notify_value);

    /* 恢复任务上下文。 */
    MRT_PortMockSetInsideISR(false);
}

/**
 * @brief 验证 ISR notify 参数校验。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_isr_notify_rejects_invalid_arguments();
 */
static void assert_isr_notify_rejects_invalid_arguments(void)
{
    /* 初始化端口 mock。 */
    MRT_PortInitialize();

    /* 切换到模拟 ISR 上下文。 */
    MRT_PortMockSetInsideISR(true);

    /* 空任务句柄应返回参数错误。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_TaskNotifyFromISR(0, 1u, MRT_NOTIFY_SET_BITS, 0));

    /* 定义任务控制块、栈和句柄。 */
    MRT_Task task_storage;
    MRT_StackType task_stack[128];
    MRT_TaskHandle task = 0;

    /* 创建测试任务。 */
    create_task(&task_storage, task_stack, &task);

    /* 非法通知动作应返回参数错误。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_TaskNotifyFromISR(task, 1u, (MRT_NotifyAction)99u, 0));

    /* 恢复任务上下文。 */
    MRT_PortMockSetInsideISR(false);
}

/**
 * @brief 运行任务通知 ISR 测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证无等待者 ISR notify。 */
    assert_isr_notify_without_waiter_sets_value_without_yield();

    /* 验证 ISR notify 唤醒等待者但延迟切换。 */
    assert_isr_notify_wakes_waiter_and_defers_switch();

    /* 验证非法上下文。 */
    assert_isr_notify_rejects_task_context();

    /* 验证 no-overwrite busy。 */
    assert_isr_notify_no_overwrite_busy_preserves_value();

    /* 验证参数校验。 */
    assert_isr_notify_rejects_invalid_arguments();

    /* 所有任务通知 ISR 测试均通过。 */
    return 0;
}
