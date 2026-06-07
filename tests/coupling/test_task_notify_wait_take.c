#include "mrt_test.h"
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
 * @brief 创建并启动单个测试任务。
 * @param storage 任务控制块，不能为空。
 * @param stack 任务栈，不能为空。
 * @param out_task 输出任务句柄，不能为空。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * create_and_start_one_task(&storage, stack, &task);
 */
static void create_and_start_one_task(MRT_Task *storage, MRT_StackType *stack, MRT_TaskHandle *out_task)
{
    /* 初始化内核。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 创建测试任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("task", DummyTask, 0, 3u, stack, 128u, storage, out_task));

    /* 启动调度器。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelStart());
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == *out_task);
}

/**
 * @brief 验证 NotifyWait 即时读取 pending 通知并执行进入/退出清位。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_notify_wait_immediate_applies_clear_masks();
 */
static void assert_notify_wait_immediate_applies_clear_masks(void)
{
    /* 定义任务控制块、栈和句柄。 */
    MRT_Task task_storage;
    MRT_StackType task_stack[128];
    MRT_TaskHandle task = 0;

    /* 创建并启动任务。 */
    create_and_start_one_task(&task_storage, task_stack, &task);

    /* 写入通知值 bit0 到 bit3。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskNotify(task, 0xFu, MRT_NOTIFY_OVERWRITE));

    /* 定义读取到的通知值。 */
    MRT_NotifyValue value = 0u;

    /* 进入时清 bit0，退出时清 bit2。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskNotifyWait(0x1u, 0x4u, 0u, &value));

    /* 返回值应是进入清位后的快照。 */
    MRT_TEST_ASSERT_EQ_U32(0xEu, (unsigned)value);

    /* 退出清位后应保留 bit1 和 bit3。 */
    MRT_TEST_ASSERT_EQ_U32(0xAu, (unsigned)task_storage.notify_value);

    /* NotifyWait 读取后 pending 状态应清除。 */
    MRT_TEST_ASSERT_TRUE(!task_storage.notify_pending);
}

/**
 * @brief 验证 NotifyWait 非阻塞无 pending 通知时返回空状态。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_notify_wait_empty_nonblocking_returns_empty();
 */
static void assert_notify_wait_empty_nonblocking_returns_empty(void)
{
    /* 定义任务控制块、栈和句柄。 */
    MRT_Task task_storage;
    MRT_StackType task_stack[128];
    MRT_TaskHandle task = 0;

    /* 创建并启动任务。 */
    create_and_start_one_task(&task_storage, task_stack, &task);

    /* 定义读取到的通知值。 */
    MRT_NotifyValue value = 99u;

    /* 无 pending 通知且 timeout 为 0，应立即返回对象为空。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OBJECT_EMPTY, (unsigned)MRT_TaskNotifyWait(0u, 0u, 0u, &value));

    /* 输出值应为当前通知值 0。 */
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)value);
}

/**
 * @brief 验证 NotifyTake 的清零模式和递减模式。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_notify_take_count_modes();
 */
static void assert_notify_take_count_modes(void)
{
    /* 定义任务控制块、栈和句柄。 */
    MRT_Task task_storage;
    MRT_StackType task_stack[128];
    MRT_TaskHandle task = 0;

    /* 创建并启动任务。 */
    create_and_start_one_task(&task_storage, task_stack, &task);

    /* 发送两次 increment 通知。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskNotify(task, 0u, MRT_NOTIFY_INCREMENT));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskNotify(task, 0u, MRT_NOTIFY_INCREMENT));

    /* 定义取出的计数。 */
    MRT_NotifyValue count = 0u;

    /* 递减模式应返回当前计数 2，并把内部计数减为 1。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskNotifyTake(false, 0u, &count));
    MRT_TEST_ASSERT_EQ_U32(2u, (unsigned)count);
    MRT_TEST_ASSERT_EQ_U32(1u, (unsigned)task_storage.notify_value);
    MRT_TEST_ASSERT_TRUE(task_storage.notify_pending);

    /* 清零模式应返回当前计数 1，并把内部计数清零。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskNotifyTake(true, 0u, &count));
    MRT_TEST_ASSERT_EQ_U32(1u, (unsigned)count);
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)task_storage.notify_value);
    MRT_TEST_ASSERT_TRUE(!task_storage.notify_pending);
}

/**
 * @brief 验证 NotifyWait 阻塞后可由任务上下文 notify 唤醒并抢占。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_notify_wait_blocks_and_notify_wakes_task();
 */
static void assert_notify_wait_blocks_and_notify_wakes_task(void)
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

    /* 高优先级任务阻塞后，低优先级任务运行。 */
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == low_task);

    /* 低优先级任务通知高优先级任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskNotify(high_task, 0x55u, MRT_NOTIFY_OVERWRITE));

    /* 被通知的高优先级任务应被唤醒并抢占。 */
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == high_task);

    /* host 仿真中等待 API 已返回，通知值保持 pending，供后续真实恢复语义扩展。 */
    MRT_TEST_ASSERT_TRUE(high_storage.notify_pending);
    MRT_TEST_ASSERT_EQ_U32(0x55u, (unsigned)high_storage.notify_value);
}

/**
 * @brief 验证 NotifyTake 在计数为 0 时阻塞并在 tick 到期后超时唤醒。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_notify_take_blocks_and_times_out();
 */
static void assert_notify_take_blocks_and_times_out(void)
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

    /* 计数为 0 时带 timeout take 应阻塞并在 host 仿真中返回超时。 */
    MRT_NotifyValue count = 99u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_TIMEOUT, (unsigned)MRT_TaskNotifyTake(true, 3u, &count));
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)count);

    /* 阻塞后低优先级任务应运行。 */
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == low_task);

    /* 推进 3 个 tick，等待任务应超时醒来并抢占。 */
    MRT_KernelTick();
    MRT_KernelTick();
    MRT_KernelTick();
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == high_task);
}

/**
 * @brief 运行任务通知 wait/take 测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证 NotifyWait 即时读取和清位。 */
    assert_notify_wait_immediate_applies_clear_masks();

    /* 验证 NotifyWait 非阻塞无通知。 */
    assert_notify_wait_empty_nonblocking_returns_empty();

    /* 验证 NotifyTake 计数模式。 */
    assert_notify_take_count_modes();

    /* 验证 NotifyWait 阻塞后被通知唤醒。 */
    assert_notify_wait_blocks_and_notify_wakes_task();

    /* 验证 NotifyTake timeout。 */
    assert_notify_take_blocks_and_times_out();

    /* 所有任务通知 wait/take 测试均通过。 */
    return 0;
}
