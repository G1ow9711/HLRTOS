#include "mrt_test.h"
#include "myrtos/portable/mrt_port_dsp_c28x.h"

#include <stddef.h>
#include <stdint.h>

/**
 * @brief 测试用 DSP 任务入口函数。
 * @param argument 任务参数，本测试不使用。
 * @return void 无返回值。
 * @example
 * test_dsp_task_entry(argument);
 */
static void test_dsp_task_entry(void *argument)
{
    /* 显式标记参数已使用，避免编译器告警。 */
    (void)argument;
}

/**
 * @brief 测试用 DSP 任务退出兜底函数。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * test_dsp_task_exit();
 */
static void test_dsp_task_exit(void)
{
}

/**
 * @brief 验证 DSP 初始栈帧会写入入口、参数和保存寄存器槽位。
 * @param void 无输入参数。
 * @return void 无返回值；断言失败会直接退出测试进程。
 * @example
 * test_dsp_stack_frame_places_entry_argument_and_registers();
 */
static void test_dsp_stack_frame_places_entry_argument_and_registers(void)
{
    /* 准备一段调用者提供的 DSP 任务栈。 */
    MRT_StackType stack[48u] = {0u};

    /* 准备接收端口 helper 回传的初始栈顶。 */
    MRT_StackType *top = 0;

    /* 构造一个可识别的任务入口参数。 */
    void *argument = (void *)(uintptr_t)0x2468ACE0u;

    /* 调用 DSP C28x 风格栈帧初始化 helper。 */
    MRT_Result result = MRT_PortDspC28xInitializeStack(stack,
                                                       48u,
                                                       test_dsp_task_entry,
                                                       argument,
                                                       test_dsp_task_exit,
                                                       &top);

    /* 验证调用成功。 */
    MRT_TEST_ASSERT_EQ_U32(MRT_RESULT_OK, result);

    /* 验证 helper 回传了有效栈顶。 */
    MRT_TEST_ASSERT_TRUE(top != 0);

    /* 验证栈顶满足 8 字节对齐。 */
    MRT_TEST_ASSERT_EQ_U32(0u, ((uintptr_t)top) & 0x7u);

    /* 验证栈顶没有低于调用者提供的栈区。 */
    MRT_TEST_ASSERT_TRUE(top >= stack);

    /* 验证完整初始帧没有越过栈区末尾。 */
    MRT_TEST_ASSERT_TRUE(top + MRT_PORT_DSP_C28X_INITIAL_FRAME_WORDS <= stack + 48u);

    /* 验证初始帧从栈尾向低地址方向生长。 */
    MRT_TEST_ASSERT_TRUE(top < stack + 48u - MRT_PORT_DSP_C28X_INITIAL_FRAME_WORDS + 1u);

    /* 验证 ST1 初始状态字符合契约。 */
    MRT_TEST_ASSERT_EQ_U32(MRT_PORT_DSP_C28X_INITIAL_STATUS_WORD,
                           top[MRT_PORT_DSP_C28X_FRAME_ST1]);

    /* 验证任务入口参数槽位。 */
    MRT_TEST_ASSERT_EQ_U32((MRT_StackType)(uintptr_t)argument,
                           top[MRT_PORT_DSP_C28X_FRAME_ARGUMENT]);

    /* 验证任务退出兜底函数槽位。 */
    MRT_TEST_ASSERT_EQ_U32((MRT_StackType)(uintptr_t)test_dsp_task_exit,
                           top[MRT_PORT_DSP_C28X_FRAME_EXIT]);

    /* 验证任务入口 PC 槽位。 */
    MRT_TEST_ASSERT_EQ_U32((MRT_StackType)(uintptr_t)test_dsp_task_entry,
                           top[MRT_PORT_DSP_C28X_FRAME_PC]);

    /* 验证 XAR4 占位值按约定写入。 */
    MRT_TEST_ASSERT_EQ_U32(MRT_PORT_DSP_C28X_REGISTER_PATTERN_BASE + 4u,
                           top[MRT_PORT_DSP_C28X_FRAME_XAR4]);

    /* 验证 XAR7 占位值按约定写入。 */
    MRT_TEST_ASSERT_EQ_U32(MRT_PORT_DSP_C28X_REGISTER_PATTERN_BASE + 7u,
                           top[MRT_PORT_DSP_C28X_FRAME_XAR7]);
}

/**
 * @brief 验证 DSP 栈帧 helper 会拒绝非法输入。
 * @param void 无输入参数。
 * @return void 无返回值；断言失败会直接退出测试进程。
 * @example
 * test_dsp_stack_frame_rejects_invalid_inputs();
 */
static void test_dsp_stack_frame_rejects_invalid_inputs(void)
{
    /* 准备最小测试栈。 */
    MRT_StackType stack[16u] = {0u};

    /* 准备接收栈顶输出。 */
    MRT_StackType *top = 0;

    /* 空栈指针必须被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32(MRT_RESULT_INVALID_ARGUMENT,
                           MRT_PortDspC28xInitializeStack(0,
                                                          16u,
                                                          test_dsp_task_entry,
                                                          0,
                                                          test_dsp_task_exit,
                                                          &top));

    /* 栈空间不足必须被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32(MRT_RESULT_INVALID_ARGUMENT,
                           MRT_PortDspC28xInitializeStack(stack,
                                                          MRT_PORT_DSP_C28X_INITIAL_FRAME_WORDS - 1u,
                                                          test_dsp_task_entry,
                                                          0,
                                                          test_dsp_task_exit,
                                                          &top));

    /* 空任务入口必须被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32(MRT_RESULT_INVALID_ARGUMENT,
                           MRT_PortDspC28xInitializeStack(stack,
                                                          16u,
                                                          0,
                                                          0,
                                                          test_dsp_task_exit,
                                                          &top));

    /* 空任务退出兜底函数必须被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32(MRT_RESULT_INVALID_ARGUMENT,
                           MRT_PortDspC28xInitializeStack(stack,
                                                          16u,
                                                          test_dsp_task_entry,
                                                          0,
                                                          0,
                                                          &top));

    /* 空输出指针必须被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32(MRT_RESULT_INVALID_ARGUMENT,
                           MRT_PortDspC28xInitializeStack(stack,
                                                          16u,
                                                          test_dsp_task_entry,
                                                          0,
                                                          test_dsp_task_exit,
                                                          0));
}

/**
 * @brief 执行 DSP 栈帧端口契约测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过。
 * @example
 * test_port_dsp_stack.exe
 */
int main(void)
{
    /* 验证正常路径的 DSP 栈帧布局。 */
    test_dsp_stack_frame_places_entry_argument_and_registers();

    /* 验证非法参数防御路径。 */
    test_dsp_stack_frame_rejects_invalid_inputs();

    /* 所有断言通过，返回成功。 */
    return 0;
}
