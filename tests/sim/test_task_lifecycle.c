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
 * @brief 创建一个静态测试任务。
 * @param name 任务名称，不能为空。
 * @param priority 任务优先级。
 * @param storage 任务控制块存储，不能为空。
 * @param stack 任务栈存储，不能为空。
 * @param stack_words 任务栈元素数量。
 * @param out_task 输出任务句柄，不能为空。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * create_static_task("worker", 3u, &storage, stack, 128u, &task);
 */
static void create_static_task(const char *name,
                               MRT_Priority priority,
                               MRT_Task *storage,
                               MRT_StackType *stack,
                               size_t stack_words,
                               MRT_TaskHandle *out_task)
{
    /* 调用公开静态创建 API，所有测试都通过真实任务创建路径进入调度器。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic(name,
                                                          DummyTask,
                                                          0,
                                                          priority,
                                                          stack,
                                                          stack_words,
                                                          storage,
                                                          out_task));
}

/**
 * @brief 读取并断言任务状态。
 * @param task 任务句柄，不能为空。
 * @param expected_state 期望状态。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_task_state(task, MRT_TASK_STATE_READY);
 */
static void assert_task_state(MRT_TaskHandle task, MRT_TaskState expected_state)
{
    /* 定义状态输出变量，初值故意设为 deleted 以证明 API 会写回。 */
    MRT_TaskState state = MRT_TASK_STATE_DELETED;

    /* 查询任务状态必须成功。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskGetState(task, &state));

    /* 实际状态必须等于调用方期望值。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)expected_state, (unsigned)state);
}

/**
 * @brief 验证删除当前静态任务会移出调度器并切换到下一个 ready 任务。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_delete_current_static_task_switches_to_next_ready();
 */
static void assert_delete_current_static_task_switches_to_next_ready(void)
{
    /* 定义高低优先级任务控制块。 */
    MRT_Task high_storage;
    MRT_Task low_storage;

    /* 定义高低优先级任务栈。 */
    MRT_StackType high_stack[128u];
    MRT_StackType low_stack[128u];

    /* 定义任务句柄。 */
    MRT_TaskHandle high_task = 0;
    MRT_TaskHandle low_task = 0;

    /* 初始化内核和调度器状态。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 创建低优先级任务。 */
    create_static_task("low", 1u, &low_storage, low_stack, 128u, &low_task);

    /* 创建高优先级任务。 */
    create_static_task("high", 5u, &high_storage, high_stack, 128u, &high_task);

    /* 启动后高优先级任务应运行。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelStart());
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == high_task);

    /* 删除当前高优先级任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskDelete(high_task));

    /* 被删除任务应标记为 deleted。 */
    assert_task_state(high_task, MRT_TASK_STATE_DELETED);

    /* 调度器应切换到剩余低优先级任务。 */
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == low_task);
    assert_task_state(low_task, MRT_TASK_STATE_RUNNING);
}

/**
 * @brief 验证挂起和恢复当前任务会正确更新状态并触发调度。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_suspend_and_resume_task_reorders_scheduler();
 */
static void assert_suspend_and_resume_task_reorders_scheduler(void)
{
    /* 定义高低优先级任务控制块。 */
    MRT_Task high_storage;
    MRT_Task low_storage;

    /* 定义任务栈。 */
    MRT_StackType high_stack[128u];
    MRT_StackType low_stack[128u];

    /* 定义任务句柄。 */
    MRT_TaskHandle high_task = 0;
    MRT_TaskHandle low_task = 0;

    /* 初始化内核。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 创建低优先级任务。 */
    create_static_task("low", 1u, &low_storage, low_stack, 128u, &low_task);

    /* 创建高优先级任务。 */
    create_static_task("high", 5u, &high_storage, high_stack, 128u, &high_task);

    /* 启动后高优先级任务运行。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelStart());
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == high_task);

    /* 挂起当前高优先级任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskSuspend(high_task));

    /* 高优先级任务应变为 suspended，低优先级任务接管运行。 */
    assert_task_state(high_task, MRT_TASK_STATE_SUSPENDED);
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == low_task);

    /* 从任务上下文恢复高优先级任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskResume(high_task));

    /* 高优先级任务恢复后应立即抢占低优先级任务。 */
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == high_task);
    assert_task_state(high_task, MRT_TASK_STATE_RUNNING);
    assert_task_state(low_task, MRT_TASK_STATE_READY);
}

/**
 * @brief 验证 ISR 恢复任务只报告延迟切换而不立即改变当前任务。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_resume_from_isr_defers_switch();
 */
static void assert_resume_from_isr_defers_switch(void)
{
    /* 定义高低优先级任务控制块。 */
    MRT_Task high_storage;
    MRT_Task low_storage;

    /* 定义任务栈。 */
    MRT_StackType high_stack[128u];
    MRT_StackType low_stack[128u];

    /* 定义任务句柄。 */
    MRT_TaskHandle high_task = 0;
    MRT_TaskHandle low_task = 0;

    /* 初始化内核。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 创建低优先级任务。 */
    create_static_task("low", 1u, &low_storage, low_stack, 128u, &low_task);

    /* 创建高优先级任务。 */
    create_static_task("high", 5u, &high_storage, high_stack, 128u, &high_task);

    /* 启动后高优先级任务运行。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelStart());
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == high_task);

    /* 挂起高优先级任务，让低优先级任务运行。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskSuspend(high_task));
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == low_task);

    /* 任务上下文调用 FromISR API 必须被拒绝。 */
    bool should_yield = true;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_CONTEXT,
                           (unsigned)MRT_TaskResumeFromISR(high_task, &should_yield));
    MRT_TEST_ASSERT_TRUE(!should_yield);

    /* 切换到模拟 ISR 上下文。 */
    MRT_PortMockSetInsideISR(true);

    /* ISR 恢复高优先级任务应成功并请求延迟切换。 */
    should_yield = false;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskResumeFromISR(high_task, &should_yield));
    MRT_TEST_ASSERT_TRUE(should_yield);

    /* ISR 路径不应立即切换当前任务。 */
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == low_task);
    assert_task_state(high_task, MRT_TASK_STATE_READY);

    /* 恢复任务上下文，避免影响后续测试。 */
    MRT_PortMockSetInsideISR(false);
}

/**
 * @brief 验证周期延时会推进基准 tick 并按绝对唤醒点阻塞当前任务。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_delay_until_blocks_until_absolute_period();
 */
static void assert_delay_until_blocks_until_absolute_period(void)
{
    /* 定义高低优先级任务控制块。 */
    MRT_Task high_storage;
    MRT_Task low_storage;

    /* 定义任务栈。 */
    MRT_StackType high_stack[128u];
    MRT_StackType low_stack[128u];

    /* 定义任务句柄。 */
    MRT_TaskHandle high_task = 0;
    MRT_TaskHandle low_task = 0;

    /* 初始化内核。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 创建低优先级任务。 */
    create_static_task("low", 1u, &low_storage, low_stack, 128u, &low_task);

    /* 创建高优先级任务。 */
    create_static_task("high", 5u, &high_storage, high_stack, 128u, &high_task);

    /* 启动后高优先级任务运行。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelStart());
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == high_task);

    /* 使用当前 tick 作为周期基准。 */
    MRT_Tick previous = MRT_KernelGetTick();

    /* 周期延时 5 tick。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskDelayUntil(&previous, 5u));

    /* API 应把上一周期基准推进到下一次绝对唤醒 tick。 */
    MRT_TEST_ASSERT_EQ_U32(5u, (unsigned)previous);

    /* 高优先级任务应阻塞，低优先级任务运行。 */
    assert_task_state(high_task, MRT_TASK_STATE_BLOCKED);
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == low_task);

    /* 前 4 个 tick 不应唤醒高优先级任务。 */
    for (unsigned i = 0u; i < 4u; i++) {
        /* 推进一个系统 tick。 */
        MRT_KernelTick();

        /* 仍由低优先级任务运行。 */
        MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == low_task);
    }

    /* 第 5 个 tick 到达绝对周期点，高优先级任务应醒来并抢占。 */
    MRT_KernelTick();
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == high_task);
    assert_task_state(high_task, MRT_TASK_STATE_RUNNING);
}

/**
 * @brief 验证修改优先级会重排 ready list 并影响当前任务选择。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_set_priority_reorders_ready_tasks();
 */
static void assert_set_priority_reorders_ready_tasks(void)
{
    /* 定义两个任务控制块。 */
    MRT_Task high_storage;
    MRT_Task low_storage;

    /* 定义任务栈。 */
    MRT_StackType high_stack[128u];
    MRT_StackType low_stack[128u];

    /* 定义任务句柄。 */
    MRT_TaskHandle high_task = 0;
    MRT_TaskHandle low_task = 0;

    /* 初始化内核。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 创建低优先级任务。 */
    create_static_task("low", 1u, &low_storage, low_stack, 128u, &low_task);

    /* 创建当前会运行的高优先级任务。 */
    create_static_task("high", 4u, &high_storage, high_stack, 128u, &high_task);

    /* 启动后 high 运行。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelStart());
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == high_task);

    /* 提升 low 的优先级到最高。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskSetPriority(low_task, 6u));

    /* low 应因为更高优先级而成为当前任务。 */
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == low_task);
    assert_task_state(low_task, MRT_TASK_STATE_RUNNING);
    assert_task_state(high_task, MRT_TASK_STATE_READY);

    /* 查询 low 的有效优先级，应为新设置值。 */
    MRT_Priority priority = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskGetPriority(low_task, &priority));
    MRT_TEST_ASSERT_EQ_U32(6u, (unsigned)priority);

    /* 越界优先级应被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_TaskSetPriority(low_task, MRT_CFG_MAX_PRIORITIES));
}

/**
 * @brief 验证栈高水位查询在 host 模型中返回任务栈容量。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_stack_high_water_mark_reports_stack_words();
 */
static void assert_stack_high_water_mark_reports_stack_words(void)
{
    /* 定义任务控制块、栈和句柄。 */
    MRT_Task task_storage;
    MRT_StackType task_stack[96u];
    MRT_TaskHandle task = 0;

    /* 初始化内核。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 创建一个 96 word 栈的任务。 */
    create_static_task("water", 2u, &task_storage, task_stack, 96u, &task);

    /* 查询栈高水位。 */
    size_t water_words = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskGetStackHighWaterMark(task, &water_words));

    /* 当前 host 模型尚未模拟栈消耗，剩余水位等于栈容量。 */
    MRT_TEST_ASSERT_EQ_U32(96u, (unsigned)water_words);

    /* 空参数应被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_TaskGetStackHighWaterMark(0, &water_words));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_TaskGetStackHighWaterMark(task, 0));
}

/**
 * @brief 运行任务生命周期测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证删除当前静态任务。 */
    assert_delete_current_static_task_switches_to_next_ready();

    /* 验证任务挂起和恢复。 */
    assert_suspend_and_resume_task_reorders_scheduler();

    /* 验证 ISR 恢复路径。 */
    assert_resume_from_isr_defers_switch();

    /* 验证周期延时。 */
    assert_delay_until_blocks_until_absolute_period();

    /* 验证优先级动态调整。 */
    assert_set_priority_reorders_ready_tasks();

    /* 验证栈高水位查询。 */
    assert_stack_high_water_mark_reports_stack_words();

    /* 所有任务生命周期测试均通过。 */
    return 0;
}
