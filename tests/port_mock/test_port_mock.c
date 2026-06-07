#include "mrt_test.h"
#include "myrtos/mrt_port.h"

/**
 * @brief 验证端口初始化会清空 mock 状态。
 *
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_port_initialize_resets_mock_state();
 */
static void assert_port_initialize_resets_mock_state(void)
{
    /* 初始化 mock 端口状态。 */
    MRT_PortInitialize();

    /* 验证初始化后不处于 ISR 上下文。 */
    MRT_TEST_ASSERT_TRUE(!MRT_PortIsInsideISR());

    /* 验证初始化后没有挂起上下文切换请求。 */
    MRT_TEST_ASSERT_TRUE(!MRT_PortMockWasYieldRequested());

    /* 验证初始化后临界区嵌套深度为 0。 */
    MRT_TEST_ASSERT_EQ_U32(0u, MRT_PortMockGetCriticalDepth());
}

/**
 * @brief 验证普通任务上下文可以请求调度切换。
 *
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_task_yield_requests_switch();
 */
static void assert_task_yield_requests_switch(void)
{
    /* 初始化 mock 端口状态。 */
    MRT_PortInitialize();

    /* 在任务上下文请求一次切换。 */
    MRT_PortYield();

    /* 验证 mock 记录了切换请求。 */
    MRT_TEST_ASSERT_TRUE(MRT_PortMockWasYieldRequested());
}

/**
 * @brief 验证 ISR 版本 yield 只在 should_yield 为 true 时请求切换。
 *
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_isr_yield_respects_flag();
 */
static void assert_isr_yield_respects_flag(void)
{
    /* 初始化 mock 端口状态。 */
    MRT_PortInitialize();

    /* 传入 false，不应请求上下文切换。 */
    MRT_PortYieldFromISR(false);

    /* 验证 false 分支没有触发切换。 */
    MRT_TEST_ASSERT_TRUE(!MRT_PortMockWasYieldRequested());

    /* 传入 true，应请求上下文切换。 */
    MRT_PortYieldFromISR(true);

    /* 验证 true 分支触发切换。 */
    MRT_TEST_ASSERT_TRUE(MRT_PortMockWasYieldRequested());
}

/**
 * @brief 验证临界区入口返回旧状态且出口恢复旧深度。
 *
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_critical_section_restores_previous_depth();
 */
static void assert_critical_section_restores_previous_depth(void)
{
    /* 初始化 mock 端口状态。 */
    MRT_PortInitialize();

    /* 第一次进入临界区，旧深度应为 0。 */
    MRT_IntState outer_state = MRT_PortEnterCritical();

    /* 验证进入后深度为 1。 */
    MRT_TEST_ASSERT_EQ_U32(1u, MRT_PortMockGetCriticalDepth());

    /* 第二次进入临界区，旧深度应为 1。 */
    MRT_IntState inner_state = MRT_PortEnterCritical();

    /* 验证嵌套后深度为 2。 */
    MRT_TEST_ASSERT_EQ_U32(2u, MRT_PortMockGetCriticalDepth());

    /* 退出内层临界区，恢复到旧深度 1。 */
    MRT_PortExitCritical(inner_state);

    /* 验证内层退出后深度为 1。 */
    MRT_TEST_ASSERT_EQ_U32(1u, MRT_PortMockGetCriticalDepth());

    /* 退出外层临界区，恢复到旧深度 0。 */
    MRT_PortExitCritical(outer_state);

    /* 验证完全退出后深度为 0。 */
    MRT_TEST_ASSERT_EQ_U32(0u, MRT_PortMockGetCriticalDepth());
}

/**
 * @brief 验证测试可切换 mock ISR 上下文标志。
 *
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_isr_state_can_be_mocked();
 */
static void assert_isr_state_can_be_mocked(void)
{
    /* 初始化 mock 端口状态。 */
    MRT_PortInitialize();

    /* 将 mock 状态设置为 ISR 上下文。 */
    MRT_PortMockSetInsideISR(true);

    /* 验证端口报告当前处于 ISR。 */
    MRT_TEST_ASSERT_TRUE(MRT_PortIsInsideISR());

    /* 将 mock 状态恢复为任务上下文。 */
    MRT_PortMockSetInsideISR(false);

    /* 验证端口报告当前不处于 ISR。 */
    MRT_TEST_ASSERT_TRUE(!MRT_PortIsInsideISR());
}

/**
 * @brief 运行 host mock port 全部单元测试。
 *
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证端口初始化状态。 */
    assert_port_initialize_resets_mock_state();

    /* 验证任务上下文 yield。 */
    assert_task_yield_requests_switch();

    /* 验证 ISR 上下文 yield 标志。 */
    assert_isr_yield_respects_flag();

    /* 验证临界区嵌套恢复。 */
    assert_critical_section_restores_previous_depth();

    /* 验证 ISR 状态 mock 能力。 */
    assert_isr_state_can_be_mocked();

    /* 所有端口 mock 测试均通过，返回 0。 */
    return 0;
}
