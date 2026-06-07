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
 * @brief 验证同优先级任务在 yield 时按 FIFO 轮转。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_same_priority_tasks_round_robin_on_yield();
 */
static void assert_same_priority_tasks_round_robin_on_yield(void)
{
    /* 定义第一个任务控制块。 */
    MRT_Task first_storage;

    /* 定义第二个任务控制块。 */
    MRT_Task second_storage;

    /* 定义第一个任务栈。 */
    MRT_StackType first_stack[128];

    /* 定义第二个任务栈。 */
    MRT_StackType second_stack[128];

    /* 定义第一个任务句柄。 */
    MRT_TaskHandle first_task = 0;

    /* 定义第二个任务句柄。 */
    MRT_TaskHandle second_task = 0;

    /* 初始化内核。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 创建第一个优先级为 4 的任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("first", DummyTask, 0, 4u, first_stack, 128u, &first_storage, &first_task));

    /* 创建第二个同优先级任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("second", DummyTask, 0, 4u, second_stack, 128u, &second_storage, &second_task));

    /* 启动调度器后应选择先进入 ready list 的第一个任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelStart());
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == first_task);

    /* 当前任务主动 yield 后，应轮转到第二个同优先级任务。 */
    MRT_KernelYield();
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == second_task);

    /* 再次 yield 后，应轮转回第一个任务。 */
    MRT_KernelYield();
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == first_task);
}

/**
 * @brief 运行 round-robin 调度测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证同优先级 yield 轮转。 */
    assert_same_priority_tasks_round_robin_on_yield();

    /* 所有 round-robin 测试均通过。 */
    return 0;
}
