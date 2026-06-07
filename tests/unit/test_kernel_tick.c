#include "mrt_test.h"
#include "myrtos/mrt_kernel.h"
#include "myrtos/mrt_port.h"

/**
 * @brief 验证内核初始化会复位 tick 和运行状态。
 *
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_kernel_initialize_resets_tick_and_state();
 */
static void assert_kernel_initialize_resets_tick_and_state(void)
{
    /* 初始化内核和端口 mock。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 验证初始化后调度器未运行。 */
    MRT_TEST_ASSERT_TRUE(!MRT_KernelIsRunning());

    /* 验证初始化后 tick 从 0 开始。 */
    MRT_TEST_ASSERT_EQ_U32(0u, MRT_KernelGetTick());
}

/**
 * @brief 验证内核 tick 接口会推进系统时间。
 *
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_kernel_tick_increments_time();
 */
static void assert_kernel_tick_increments_time(void)
{
    /* 初始化内核，保证 tick 从 0 开始。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 推进一个系统 tick。 */
    MRT_KernelTick();

    /* 验证 tick 计数变为 1。 */
    MRT_TEST_ASSERT_EQ_U32(1u, MRT_KernelGetTick());

    /* 再推进一个系统 tick。 */
    MRT_KernelTick();

    /* 验证 tick 计数变为 2。 */
    MRT_TEST_ASSERT_EQ_U32(2u, MRT_KernelGetTick());
}

/**
 * @brief 验证尚无任务时启动内核返回未启动。
 *
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_kernel_start_without_tasks_returns_not_started();
 */
static void assert_kernel_start_without_tasks_returns_not_started(void)
{
    /* 初始化内核但不创建任何任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 无任务可运行时启动调度器应返回未启动。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_NOT_STARTED, (unsigned)MRT_KernelStart());
}

/**
 * @brief 验证内核 yield 会转发到端口层。
 *
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_kernel_yield_requests_port_yield();
 */
static void assert_kernel_yield_requests_port_yield(void)
{
    /* 初始化内核和端口 mock 状态。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 验证初始状态没有调度切换请求。 */
    MRT_TEST_ASSERT_TRUE(!MRT_PortMockWasYieldRequested());

    /* 通过内核接口请求让出 CPU。 */
    MRT_KernelYield();

    /* 验证端口 mock 收到了切换请求。 */
    MRT_TEST_ASSERT_TRUE(MRT_PortMockWasYieldRequested());
}

/**
 * @brief 验证调度器挂起和恢复的嵌套计数行为。
 *
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_scheduler_suspend_resume_tracks_depth();
 */
static void assert_scheduler_suspend_resume_tracks_depth(void)
{
    /* 初始化内核全局状态。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 未挂起时恢复调度器应返回非法上下文。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_CONTEXT, (unsigned)MRT_KernelResumeAll());

    /* 第一次挂起调度器。 */
    MRT_KernelSuspendAll();

    /* 第二次挂起调度器，形成嵌套。 */
    MRT_KernelSuspendAll();

    /* 第一次恢复只退出内层挂起。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelResumeAll());

    /* 第二次恢复退出外层挂起。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelResumeAll());

    /* 再次恢复时已经没有挂起层级，应返回非法上下文。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_CONTEXT, (unsigned)MRT_KernelResumeAll());
}

/**
 * @brief 运行内核 tick shell 全部单元测试。
 *
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证内核初始化行为。 */
    assert_kernel_initialize_resets_tick_and_state();

    /* 验证 tick 推进行为。 */
    assert_kernel_tick_increments_time();

    /* 验证无任务启动行为。 */
    assert_kernel_start_without_tasks_returns_not_started();

    /* 验证 yield 转发到端口层。 */
    assert_kernel_yield_requests_port_yield();

    /* 验证调度器挂起和恢复基础行为。 */
    assert_scheduler_suspend_resume_tracks_depth();

    /* 所有 kernel shell 测试均通过，返回 0。 */
    return 0;
}
