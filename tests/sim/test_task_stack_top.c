#include "mrt_test.h"
#include "myrtos/mrt_heap.h"
#include "myrtos/mrt_kernel.h"
#include "myrtos/mrt_task.h"
#include "mrt_task_internal.h"

/**
 * @brief 测试用任务入口函数。
 * @param arg 用户参数，本测试不使用。
 * @return void 无返回值。
 * @example
 * DummyTask(NULL);
 */
static void DummyTask(void *arg)
{
    /* 显式丢弃未使用参数，避免编译器警告。 */
    (void)arg;
}

/**
 * @brief 验证静态任务创建后，内核栈顶默认指向用户栈末端。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_static_task_stack_top_defaults_to_stack_end();
 */
static void assert_static_task_stack_top_defaults_to_stack_end(void)
{
    /* 定义静态任务控制块。 */
    MRT_Task task_storage;

    /* 定义静态任务栈。 */
    MRT_StackType stack[128u];

    /* 定义输出任务句柄。 */
    MRT_TaskHandle task = 0;

    /* 初始化内核，确保任务调度器状态确定。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 创建一个静态任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("static",
                                                          DummyTask,
                                                          0,
                                                          2u,
                                                          stack,
                                                          128u,
                                                          &task_storage,
                                                          &task));

    /* 任务刚创建时，运行期栈顶应指向向下增长栈的空栈顶。 */
    MRT_TEST_ASSERT_TRUE(MRT_TaskKernelGetStackTop(task) == &stack[128u]);
}

/**
 * @brief 验证动态任务创建后，运行期栈顶与动态栈范围一致。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_dynamic_task_stack_top_defaults_to_allocated_stack_end();
 */
static void assert_dynamic_task_stack_top_defaults_to_allocated_stack_end(void)
{
    /* 定义自然对齐的堆存储。 */
    uintptr_t heap_words[512u];

    /* 初始化内核，清空调度器状态。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 初始化合并堆，供动态任务分配 TCB 和栈。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_HeapInitialize(heap_words,
                                                        sizeof(heap_words),
                                                        MRT_HEAP_MODE_COALESCING));

    /* 动态创建一个任务。 */
    MRT_TaskHandle task = 0;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreate("dynamic", DummyTask, 0, 2u, 96u, &task));

    /* 动态任务的栈顶应指向该任务动态栈的末端。 */
    MRT_TEST_ASSERT_TRUE(MRT_TaskKernelGetStackTop(task) == (task->stack + task->stack_words));

    /* 删除动态任务，避免堆状态影响后续测试。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskDelete(task));
}

/**
 * @brief 验证内部栈顶设置接口会拒绝非法任务和非法栈顶。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_stack_top_helpers_reject_invalid_inputs();
 */
static void assert_stack_top_helpers_reject_invalid_inputs(void)
{
    /* 定义静态任务控制块。 */
    MRT_Task task_storage;

    /* 定义静态任务栈。 */
    MRT_StackType stack[64u];

    /* 定义输出任务句柄。 */
    MRT_TaskHandle task = 0;

    /* 初始化内核状态。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 空任务句柄查询应返回空栈顶。 */
    MRT_TEST_ASSERT_TRUE(MRT_TaskKernelGetStackTop(0) == 0);

    /* 空任务句柄设置应被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_TaskKernelSetStackTop(0, &stack[32u]));

    /* 创建一个合法任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("static",
                                                          DummyTask,
                                                          0,
                                                          2u,
                                                          stack,
                                                          64u,
                                                          &task_storage,
                                                          &task));

    /* 空栈顶设置应被拒绝，避免端口层恢复到非法 PSP。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_TaskKernelSetStackTop(task, 0));

    /* 删除任务后，栈顶查询应返回空。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskDelete(task));
    MRT_TEST_ASSERT_TRUE(MRT_TaskKernelGetStackTop(task) == 0);

    /* 已删除任务不允许再次写入栈顶。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_TaskKernelSetStackTop(task, &stack[32u]));
}

/**
 * @brief 验证上下文切换栈顶 helper 会保存旧任务 PSP 并返回新任务 PSP。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_switch_stack_top_saves_previous_and_returns_current();
 */
static void assert_switch_stack_top_saves_previous_and_returns_current(void)
{
    /* 定义两个同优先级任务控制块。 */
    MRT_Task first_storage;
    MRT_Task second_storage;

    /* 定义两个任务各自的栈。 */
    MRT_StackType first_stack[128u];
    MRT_StackType second_stack[128u];

    /* 定义两个任务句柄。 */
    MRT_TaskHandle first_task = 0;
    MRT_TaskHandle second_task = 0;

    /* 初始化内核状态。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 先创建第一个任务，使其在同优先级 ready list 中排在前面。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("first",
                                                          DummyTask,
                                                          0,
                                                          3u,
                                                          first_stack,
                                                          128u,
                                                          &first_storage,
                                                          &first_task));

    /* 再创建第二个任务，用于 yield 后切换。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("second",
                                                          DummyTask,
                                                          0,
                                                          3u,
                                                          second_stack,
                                                          128u,
                                                          &second_storage,
                                                          &second_task));

    /* 启动调度器后，第一个任务应成为当前任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelStart());
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == first_task);

    /* 给第二个任务写入一个可识别的待恢复 PSP。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskKernelSetStackTop(second_task, &second_stack[72u]));

    /* 主动 yield 让调度器选中第二个任务。 */
    MRT_KernelYield();
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == second_task);

    /* 模拟 PendSV 已经把第一个任务寄存器压栈后的 PSP。 */
    MRT_StackType *saved_first_psp = &first_stack[48u];

    /* helper 应保存第一个任务 PSP，并返回当前任务待恢复 PSP。 */
    MRT_TEST_ASSERT_TRUE(MRT_TaskKernelSwitchStackTop(saved_first_psp) == &second_stack[72u]);
    MRT_TEST_ASSERT_TRUE(MRT_TaskKernelGetStackTop(first_task) == saved_first_psp);
}

/**
 * @brief 运行任务栈顶契约测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证静态任务初始栈顶。 */
    assert_static_task_stack_top_defaults_to_stack_end();

    /* 验证动态任务初始栈顶。 */
    assert_dynamic_task_stack_top_defaults_to_allocated_stack_end();

    /* 验证非法参数和已删除任务保护。 */
    assert_stack_top_helpers_reject_invalid_inputs();

    /* 验证上下文保存任务和当前任务栈顶切换契约。 */
    assert_switch_stack_top_saves_previous_and_returns_current();

    /* 所有任务栈顶契约测试均通过。 */
    return 0;
}
