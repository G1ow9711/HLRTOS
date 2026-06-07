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
 * @brief 验证静态创建任务会拒绝非法参数。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_static_create_rejects_invalid_arguments();
 */
static void assert_static_create_rejects_invalid_arguments(void)
{
    /* 定义任务控制块存储。 */
    MRT_Task task_storage;

    /* 定义任务栈存储。 */
    MRT_StackType stack[128];

    /* 定义输出句柄。 */
    MRT_TaskHandle task = 0;

    /* 空入口函数必须被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_TaskCreateStatic("bad", 0, 0, 1u, stack, 128u, &task_storage, &task));

    /* 空栈指针必须被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_TaskCreateStatic("bad", DummyTask, 0, 1u, 0, 128u, &task_storage, &task));

    /* 栈长度为 0 必须被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_TaskCreateStatic("bad", DummyTask, 0, 1u, stack, 0u, &task_storage, &task));

    /* 空任务控制块存储必须被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_TaskCreateStatic("bad", DummyTask, 0, 1u, stack, 128u, 0, &task));

    /* 越界优先级必须被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_TaskCreateStatic("bad", DummyTask, 0, MRT_CFG_MAX_PRIORITIES, stack, 128u, &task_storage, &task));
}

/**
 * @brief 验证静态创建任务会初始化任务属性并返回句柄。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_static_create_initializes_task_as_ready();
 */
static void assert_static_create_initializes_task_as_ready(void)
{
    /* 定义任务控制块存储。 */
    MRT_Task task_storage;

    /* 定义任务栈存储。 */
    MRT_StackType stack[128];

    /* 定义输出句柄。 */
    MRT_TaskHandle task = 0;

    /* 初始化内核，确保 ready list 处于确定状态。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 静态创建一个优先级为 3 的任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("worker", DummyTask, (void *)0x1234u, 3u, stack, 128u, &task_storage, &task));

    /* 创建成功后输出句柄应指向调用方提供的任务控制块。 */
    MRT_TEST_ASSERT_TRUE(task == &task_storage);

    /* 新任务应处于 ready 状态。 */
    MRT_TaskState state = MRT_TASK_STATE_DELETED;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskGetState(task, &state));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_TASK_STATE_READY, (unsigned)state);

    /* 新任务优先级应等于创建时传入的优先级。 */
    MRT_Priority priority = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskGetPriority(task, &priority));
    MRT_TEST_ASSERT_EQ_U32(3u, priority);

    /* 新任务名称应等于创建时传入的名称指针。 */
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetName(task) == (const char *)"worker");
}

/**
 * @brief 运行静态任务创建测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证非法参数处理。 */
    assert_static_create_rejects_invalid_arguments();

    /* 验证成功创建后的任务属性。 */
    assert_static_create_initializes_task_as_ready();

    /* 所有静态任务创建测试均通过。 */
    return 0;
}
