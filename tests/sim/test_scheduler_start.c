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
 * @brief 验证启动调度器会选择最高优先级 ready 任务。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_kernel_start_selects_highest_priority_ready_task();
 */
static void assert_kernel_start_selects_highest_priority_ready_task(void)
{
    /* 定义低优先级任务控制块。 */
    MRT_Task low_storage;

    /* 定义高优先级任务控制块。 */
    MRT_Task high_storage;

    /* 定义低优先级任务栈。 */
    MRT_StackType low_stack[128];

    /* 定义高优先级任务栈。 */
    MRT_StackType high_stack[128];

    /* 定义低优先级任务句柄。 */
    MRT_TaskHandle low_task = 0;

    /* 定义高优先级任务句柄。 */
    MRT_TaskHandle high_task = 0;

    /* 初始化内核。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 先创建低优先级任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("low", DummyTask, 0, 1u, low_stack, 128u, &low_storage, &low_task));

    /* 再创建高优先级任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("high", DummyTask, 0, 5u, high_stack, 128u, &high_storage, &high_task));

    /* 启动调度器应成功。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelStart());

    /* 内核运行状态应变为 true。 */
    MRT_TEST_ASSERT_TRUE(MRT_KernelIsRunning());

    /* 当前任务应为高优先级任务。 */
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == high_task);

    /* 高优先级任务状态应为 running。 */
    MRT_TaskState high_state = MRT_TASK_STATE_DELETED;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskGetState(high_task, &high_state));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_TASK_STATE_RUNNING, (unsigned)high_state);

    /* 低优先级任务仍应保持 ready。 */
    MRT_TaskState low_state = MRT_TASK_STATE_DELETED;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskGetState(low_task, &low_state));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_TASK_STATE_READY, (unsigned)low_state);

    /* 端口层应记录启动首任务请求。 */
    MRT_TEST_ASSERT_TRUE(MRT_PortMockWasYieldRequested());
}

/**
 * @brief 运行调度器启动测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证调度器启动时选择最高优先级任务。 */
    assert_kernel_start_selects_highest_priority_ready_task();

    /* 所有调度器启动测试均通过。 */
    return 0;
}
