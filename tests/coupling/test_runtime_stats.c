#include "mrt_test.h"
#include "myrtos/mrt_kernel.h"
#include "myrtos/mrt_stats.h"
#include "myrtos/mrt_task.h"

/**
 * @brief 测试用空任务入口函数。
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
 * @param out_task 输出任务句柄，不能为空。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * create_task("high", 5u, &storage, stack, &task);
 */
static void create_task(const char *name,
                        MRT_Priority priority,
                        MRT_Task *storage,
                        MRT_StackType *stack,
                        MRT_TaskHandle *out_task)
{
    /* 通过真实静态任务创建 API 接入调度器。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic(name,
                                                          DummyTask,
                                                          0,
                                                          priority,
                                                          stack,
                                                          128u,
                                                          storage,
                                                          out_task));
}

/**
 * @brief 读取任务运行统计并断言等于期望值。
 * @param task 待查询任务句柄，不能为空。
 * @param expected_ticks 期望运行 tick 数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_runtime_ticks(task, 2u);
 */
static void assert_runtime_ticks(MRT_TaskHandle task, uint64_t expected_ticks)
{
    /* 运行统计输出变量先置为无关值，证明 API 会写回。 */
    uint64_t runtime_ticks = UINT64_MAX;

    /* 查询运行统计必须成功。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_StatsGetTaskRuntime(task, &runtime_ticks));

    /* 当前测试只使用很小 tick 值，可安全收窄为 unsigned 比较。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)expected_ticks, (unsigned)runtime_ticks);
}

/**
 * @brief 验证运行统计按当前运行任务累计 tick。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_runtime_stats_accumulate_on_kernel_tick();
 */
static void assert_runtime_stats_accumulate_on_kernel_tick(void)
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

    /* 初始化内核和调度器。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 创建低优先级任务。 */
    create_task("low", 1u, &low_storage, low_stack, &low_task);

    /* 创建高优先级任务。 */
    create_task("high", 5u, &high_storage, high_stack, &high_task);

    /* 启动后高优先级任务运行。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelStart());
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == high_task);

    /* 初始运行统计应为 0。 */
    assert_runtime_ticks(high_task, 0u);
    assert_runtime_ticks(low_task, 0u);

    /* 高优先级任务连续运行两个 tick。 */
    MRT_KernelTick();
    MRT_KernelTick();

    /* 两个 tick 均应计入高优先级任务。 */
    assert_runtime_ticks(high_task, 2u);
    assert_runtime_ticks(low_task, 0u);

    /* 高优先级任务阻塞两个 tick，让低优先级任务运行。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskDelay(2u));
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == low_task);

    /* 第一个阻塞 tick 由低优先级任务运行。 */
    MRT_KernelTick();
    assert_runtime_ticks(high_task, 2u);
    assert_runtime_ticks(low_task, 1u);
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == low_task);

    /* 第二个阻塞 tick 仍由低优先级任务运行，随后高优先级任务唤醒抢占。 */
    MRT_KernelTick();
    assert_runtime_ticks(high_task, 2u);
    assert_runtime_ticks(low_task, 2u);
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == high_task);

    /* 再推进一个 tick，高优先级任务重新累计运行时间。 */
    MRT_KernelTick();
    assert_runtime_ticks(high_task, 3u);
    assert_runtime_ticks(low_task, 2u);
}

/**
 * @brief 验证运行统计 API 的参数保护和已删除任务保护。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_runtime_stats_reject_invalid_arguments();
 */
static void assert_runtime_stats_reject_invalid_arguments(void)
{
    /* 定义任务控制块、任务栈和句柄。 */
    MRT_Task task_storage;
    MRT_StackType task_stack[128u];
    MRT_TaskHandle task = 0;

    /* 初始化内核。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 创建并启动一个任务。 */
    create_task("worker", 3u, &task_storage, task_stack, &task);
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelStart());

    /* 空任务句柄应被拒绝。 */
    uint64_t runtime_ticks = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_StatsGetTaskRuntime(0, &runtime_ticks));

    /* 空输出指针应被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_StatsGetTaskRuntime(task, 0));

    /* 删除任务后，统计查询应拒绝读取已失效任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskDelete(task));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_StatsGetTaskRuntime(task, &runtime_ticks));
}

/**
 * @brief 运行任务运行统计耦合测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时进程提前退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证当前任务运行 tick 累计。 */
    assert_runtime_stats_accumulate_on_kernel_tick();

    /* 验证参数保护和已删除任务保护。 */
    assert_runtime_stats_reject_invalid_arguments();

    /* 所有运行统计测试均通过。 */
    return 0;
}
