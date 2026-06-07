#include "mrt_test.h"
#include "myrtos/portable/mrt_port_dsp_c28x.h"

/**
 * @brief 验证任务上下文中 DSP 切换请求和确认语义。
 * @param void 无输入参数。
 * @return void 无返回值；断言失败会直接退出测试进程。
 * @example
 * test_task_context_switch_request_and_acknowledge();
 */
static void test_task_context_switch_request_and_acknowledge(void)
{
    /* 复位 DSP context 模型，保证测试从已知状态开始。 */
    MRT_PortDspC28xContextModelReset();

    /* 验证初始状态没有挂起切换请求。 */
    MRT_TEST_ASSERT_TRUE(!MRT_PortDspC28xIsContextSwitchPending());

    /* 验证初始请求计数为 0。 */
    MRT_TEST_ASSERT_EQ_U32(0u, MRT_PortDspC28xGetContextSwitchRequestCount());

    /* 验证初始 ISR 嵌套深度为 0。 */
    MRT_TEST_ASSERT_EQ_U32(0u, MRT_PortDspC28xGetInterruptNesting());

    /* 请求一次上下文切换。 */
    MRT_TEST_ASSERT_EQ_U32(MRT_RESULT_OK, MRT_PortDspC28xRequestContextSwitch());

    /* 验证请求已经挂起。 */
    MRT_TEST_ASSERT_TRUE(MRT_PortDspC28xIsContextSwitchPending());

    /* 验证请求计数增加。 */
    MRT_TEST_ASSERT_EQ_U32(1u, MRT_PortDspC28xGetContextSwitchRequestCount());

    /* 确认挂起切换已经被软件中断服务。 */
    MRT_TEST_ASSERT_EQ_U32(MRT_RESULT_OK, MRT_PortDspC28xAcknowledgeContextSwitch());

    /* 验证挂起标志被清除。 */
    MRT_TEST_ASSERT_TRUE(!MRT_PortDspC28xIsContextSwitchPending());

    /* 验证累计请求计数不会因为确认而清零。 */
    MRT_TEST_ASSERT_EQ_U32(1u, MRT_PortDspC28xGetContextSwitchRequestCount());
}

/**
 * @brief 验证嵌套 ISR 中的 DSP 切换请求会延迟到最外层退出。
 * @param void 无输入参数。
 * @return void 无返回值；断言失败会直接退出测试进程。
 * @example
 * test_nested_interrupt_defers_switch_until_outer_exit();
 */
static void test_nested_interrupt_defers_switch_until_outer_exit(void)
{
    /* 初始化输出变量，便于验证 helper 会写入 false/true。 */
    bool should_switch = true;

    /* 复位 DSP context 模型。 */
    MRT_PortDspC28xContextModelReset();

    /* 进入第一层 ISR。 */
    MRT_TEST_ASSERT_EQ_U32(MRT_RESULT_OK, MRT_PortDspC28xEnterInterrupt());

    /* 进入第二层嵌套 ISR。 */
    MRT_TEST_ASSERT_EQ_U32(MRT_RESULT_OK, MRT_PortDspC28xEnterInterrupt());

    /* 验证嵌套深度为 2。 */
    MRT_TEST_ASSERT_EQ_U32(2u, MRT_PortDspC28xGetInterruptNesting());

    /* 在嵌套 ISR 中请求上下文切换。 */
    MRT_TEST_ASSERT_EQ_U32(MRT_RESULT_OK, MRT_PortDspC28xRequestContextSwitch());

    /* 退出内层 ISR。 */
    MRT_TEST_ASSERT_EQ_U32(MRT_RESULT_OK, MRT_PortDspC28xExitInterrupt(&should_switch));

    /* 内层退出不应立即切换。 */
    MRT_TEST_ASSERT_TRUE(!should_switch);

    /* 请求仍保持挂起。 */
    MRT_TEST_ASSERT_TRUE(MRT_PortDspC28xIsContextSwitchPending());

    /* 嵌套深度应降为 1。 */
    MRT_TEST_ASSERT_EQ_U32(1u, MRT_PortDspC28xGetInterruptNesting());

    /* 退出最外层 ISR。 */
    MRT_TEST_ASSERT_EQ_U32(MRT_RESULT_OK, MRT_PortDspC28xExitInterrupt(&should_switch));

    /* 最外层退出应报告需要执行延迟切换。 */
    MRT_TEST_ASSERT_TRUE(should_switch);

    /* 在实际软件中断确认前，挂起标志仍保持为 true。 */
    MRT_TEST_ASSERT_TRUE(MRT_PortDspC28xIsContextSwitchPending());

    /* 嵌套深度应回到 0。 */
    MRT_TEST_ASSERT_EQ_U32(0u, MRT_PortDspC28xGetInterruptNesting());

    /* 确认切换完成。 */
    MRT_TEST_ASSERT_EQ_U32(MRT_RESULT_OK, MRT_PortDspC28xAcknowledgeContextSwitch());

    /* 验证挂起标志清除。 */
    MRT_TEST_ASSERT_TRUE(!MRT_PortDspC28xIsContextSwitchPending());
}

/**
 * @brief 验证 DSP ISR 退出模型会拒绝下溢和空输出参数。
 * @param void 无输入参数。
 * @return void 无返回值；断言失败会直接退出测试进程。
 * @example
 * test_interrupt_exit_rejects_underflow_and_null_output();
 */
static void test_interrupt_exit_rejects_underflow_and_null_output(void)
{
    /* 初始化输出变量为 true，用于确认错误路径会清为 false。 */
    bool should_switch = true;

    /* 复位 DSP context 模型。 */
    MRT_PortDspC28xContextModelReset();

    /* 未进入 ISR 就退出必须返回非法上下文。 */
    MRT_TEST_ASSERT_EQ_U32(MRT_RESULT_INVALID_CONTEXT,
                           MRT_PortDspC28xExitInterrupt(&should_switch));

    /* 下溢错误路径应回写 false。 */
    MRT_TEST_ASSERT_TRUE(!should_switch);

    /* 嵌套深度必须保持 0。 */
    MRT_TEST_ASSERT_EQ_U32(0u, MRT_PortDspC28xGetInterruptNesting());

    /* 进入一层 ISR。 */
    MRT_TEST_ASSERT_EQ_U32(MRT_RESULT_OK, MRT_PortDspC28xEnterInterrupt());

    /* 空输出指针必须被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32(MRT_RESULT_INVALID_ARGUMENT, MRT_PortDspC28xExitInterrupt(0));

    /* 空输出错误路径不能破坏嵌套深度。 */
    MRT_TEST_ASSERT_EQ_U32(1u, MRT_PortDspC28xGetInterruptNesting());

    /* 使用有效输出指针退出 ISR。 */
    MRT_TEST_ASSERT_EQ_U32(MRT_RESULT_OK, MRT_PortDspC28xExitInterrupt(&should_switch));

    /* 没有挂起请求时不应提示切换。 */
    MRT_TEST_ASSERT_TRUE(!should_switch);
}

/**
 * @brief 执行 DSP 上下文切换端口契约测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过。
 * @example
 * test_port_dsp_context.exe
 */
int main(void)
{
    /* 验证任务上下文请求和确认路径。 */
    test_task_context_switch_request_and_acknowledge();

    /* 验证嵌套 ISR 延迟切换路径。 */
    test_nested_interrupt_defers_switch_until_outer_exit();

    /* 验证非法 ISR 退出路径。 */
    test_interrupt_exit_rejects_underflow_and_null_output();

    /* 所有断言通过，返回成功。 */
    return 0;
}
