#include "mrt_test.h"
#include "myrtos/mrt_heap.h"
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
 * @brief 验证动态任务创建后可进入 ready 状态，并在删除时归还堆空间。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_dynamic_task_create_ready_and_delete_returns_heap();
 */
static void assert_dynamic_task_create_ready_and_delete_returns_heap(void)
{
    /* 定义自然对齐的可合并堆存储。 */
    uintptr_t heap_words[512u];

    /* 初始化内核，清空任务调度器状态。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 初始化为合并堆，便于动态任务删除后完整回收空间。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_HeapInitialize(heap_words,
                                                        sizeof(heap_words),
                                                        MRT_HEAP_MODE_COALESCING));

    /* 记录创建前堆空闲空间。 */
    size_t free_before_create = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_HeapGetFreeSize(&free_before_create));

    /* 动态创建任务。 */
    MRT_TaskHandle task = 0;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreate("dyn", DummyTask, 0, 3u, 128u, &task));
    MRT_TEST_ASSERT_TRUE(task != 0);

    /* 动态任务创建后应处于 ready 状态。 */
    MRT_TaskState state = MRT_TASK_STATE_DELETED;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskGetState(task, &state));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_TASK_STATE_READY, (unsigned)state);

    /* 创建后堆空闲空间应减少。 */
    size_t free_after_create = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_HeapGetFreeSize(&free_after_create));
    MRT_TEST_ASSERT_TRUE(free_after_create < free_before_create);

    /* 删除动态任务应释放 TCB 和栈所在堆块。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskDelete(task));

    /* 删除后堆空闲空间应恢复到创建前。 */
    size_t free_after_delete = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_HeapGetFreeSize(&free_after_delete));
    MRT_TEST_ASSERT_EQ_U32((unsigned)free_before_create, (unsigned)free_after_delete);
}

/**
 * @brief 验证动态任务创建失败时返回内存不足、清空输出句柄且不泄漏堆空间。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_dynamic_task_create_failure_keeps_heap_unchanged();
 */
static void assert_dynamic_task_create_failure_keeps_heap_unchanged(void)
{
    /* 定义较小但足以初始化合并堆的存储。 */
    uintptr_t heap_words[16u];

    /* 初始化内核。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 初始化为合并堆。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_HeapInitialize(heap_words,
                                                        sizeof(heap_words),
                                                        MRT_HEAP_MODE_COALESCING));

    /* 记录失败创建前堆空闲空间。 */
    size_t free_before = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_HeapGetFreeSize(&free_before));

    /* 请求远大于堆容量的动态任务。 */
    MRT_TaskHandle task = (MRT_TaskHandle)heap_words;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_NO_MEMORY,
                           (unsigned)MRT_TaskCreate("too_big", DummyTask, 0, 2u, 512u, &task));

    /* 失败时输出句柄必须清空。 */
    MRT_TEST_ASSERT_TRUE(task == 0);

    /* 失败创建不能改变堆空闲空间。 */
    size_t free_after = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_HeapGetFreeSize(&free_after));
    MRT_TEST_ASSERT_EQ_U32((unsigned)free_before, (unsigned)free_after);
}

/**
 * @brief 验证动态任务 API 会拒绝非法参数。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_dynamic_task_apis_reject_invalid_arguments();
 */
static void assert_dynamic_task_apis_reject_invalid_arguments(void)
{
    /* 定义自然对齐堆存储。 */
    uintptr_t heap_words[256u];

    /* 初始化内核和堆。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_HeapInitialize(heap_words,
                                                        sizeof(heap_words),
                                                        MRT_HEAP_MODE_COALESCING));

    /* 定义输出句柄。 */
    MRT_TaskHandle task = 0;

    /* 入口函数不能为空。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_TaskCreate("bad", 0, 0, 1u, 128u, &task));

    /* 栈长度不能为 0。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_TaskCreate("bad", DummyTask, 0, 1u, 0u, &task));

    /* 优先级不能越界。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_TaskCreate("bad",
                                                    DummyTask,
                                                    0,
                                                    MRT_CFG_MAX_PRIORITIES,
                                                    128u,
                                                    &task));

    /* 输出句柄不能为空。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_TaskCreate("bad", DummyTask, 0, 1u, 128u, 0));

    /* 空任务句柄不能删除。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT, (unsigned)MRT_TaskDelete(0));
}

/**
 * @brief 运行动态任务与堆耦合测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证动态任务创建、ready 状态和删除释放。 */
    assert_dynamic_task_create_ready_and_delete_returns_heap();

    /* 验证创建失败路径不泄漏堆空间。 */
    assert_dynamic_task_create_failure_keeps_heap_unchanged();

    /* 验证非法参数保护。 */
    assert_dynamic_task_apis_reject_invalid_arguments();

    /* 所有动态任务耦合测试均通过。 */
    return 0;
}
