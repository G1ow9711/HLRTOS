#include "mrt_test.h"
#include "myrtos/portable/mrt_port_stm32_cm.h"

#include <stddef.h>
#include <stdint.h>

/**
 * @brief 测试用 STM32 任务入口函数。
 * @param argument 任务参数，本测试不使用。
 * @return void 无返回值。
 * @example
 * test_task_entry(argument);
 */
static void test_task_entry(void *argument)
{
    /* 显式标记参数已使用，避免编译器告警。 */
    (void)argument;
}

/**
 * @brief 测试用 STM32 任务退出兜底函数。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * test_task_exit();
 */
static void test_task_exit(void)
{
}

/**
 * @brief 验证 STM32 初始栈帧会写入核心寄存器槽位。
 * @param void 无输入参数。
 * @return void 无返回值；断言失败会直接退出测试进程。
 * @example
 * test_stm32_stack_frame_places_core_registers();
 */
static void test_stm32_stack_frame_places_core_registers(void)
{
    /* 准备一段调用者提供的任务栈。 */
    MRT_StackType stack[40u] = {0u};

    /* 准备接收端口 helper 回传的初始栈顶。 */
    MRT_StackType *top = 0;

    /* 构造一个可识别的任务入口参数。 */
    void *argument = (void *)(uintptr_t)0x12345678u;

    /* 调用 STM32 Cortex-M 栈帧初始化 helper。 */
    MRT_Result result = MRT_PortStm32CmInitializeStack(stack,
                                                       40u,
                                                       test_task_entry,
                                                       argument,
                                                       test_task_exit,
                                                       &top);

    /* 验证调用成功。 */
    MRT_TEST_ASSERT_EQ_U32(MRT_RESULT_OK, result);

    /* 验证 helper 回传了有效栈顶。 */
    MRT_TEST_ASSERT_TRUE(top != 0);

    /* 验证栈顶满足 8 字节对齐。 */
    MRT_TEST_ASSERT_EQ_U32(0u, ((uintptr_t)top) & 0x7u);

    /* 验证栈顶仍位于调用者提供的栈区内。 */
    MRT_TEST_ASSERT_TRUE(top >= stack);

    /* 验证完整初始帧没有越过栈区末尾。 */
    MRT_TEST_ASSERT_TRUE(top + MRT_PORT_STM32_CM_INITIAL_FRAME_WORDS <= stack + 40u);

    /* 验证 R0 槽位保存任务入口参数。 */
    MRT_TEST_ASSERT_EQ_U32((MRT_StackType)(uintptr_t)argument,
                           top[MRT_PORT_STM32_CM_FRAME_R0]);

    /* 验证 LR 槽位保存任务退出兜底函数。 */
    MRT_TEST_ASSERT_EQ_U32((MRT_StackType)(uintptr_t)test_task_exit,
                           top[MRT_PORT_STM32_CM_FRAME_LR]);

    /* 验证 PC 槽位保存任务入口函数。 */
    MRT_TEST_ASSERT_EQ_U32((MRT_StackType)(uintptr_t)test_task_entry,
                           top[MRT_PORT_STM32_CM_FRAME_PC]);

    /* 验证 xPSR 槽位置位 Thumb bit。 */
    MRT_TEST_ASSERT_EQ_U32(MRT_PORT_STM32_CM_XPSR_THUMB_BIT,
                           top[MRT_PORT_STM32_CM_FRAME_XPSR]);

    /* 验证 R4 占位值按约定写入。 */
    MRT_TEST_ASSERT_EQ_U32(MRT_PORT_STM32_CM_CALLEE_PATTERN_BASE + 4u,
                           top[MRT_PORT_STM32_CM_FRAME_R4]);

    /* 验证 R11 占位值按约定写入。 */
    MRT_TEST_ASSERT_EQ_U32(MRT_PORT_STM32_CM_CALLEE_PATTERN_BASE + 11u,
                           top[MRT_PORT_STM32_CM_FRAME_R11]);
}

/**
 * @brief 验证 STM32 栈帧 helper 会拒绝非法输入。
 * @param void 无输入参数。
 * @return void 无返回值；断言失败会直接退出测试进程。
 * @example
 * test_stm32_stack_frame_rejects_invalid_inputs();
 */
static void test_stm32_stack_frame_rejects_invalid_inputs(void)
{
    /* 准备最小测试栈。 */
    MRT_StackType stack[16u] = {0u};

    /* 准备接收栈顶输出。 */
    MRT_StackType *top = 0;

    /* 空栈指针必须被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32(MRT_RESULT_INVALID_ARGUMENT,
                           MRT_PortStm32CmInitializeStack(0,
                                                          16u,
                                                          test_task_entry,
                                                          0,
                                                          test_task_exit,
                                                          &top));

    /* 栈空间不足必须被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32(MRT_RESULT_INVALID_ARGUMENT,
                           MRT_PortStm32CmInitializeStack(stack,
                                                          MRT_PORT_STM32_CM_INITIAL_FRAME_WORDS - 1u,
                                                          test_task_entry,
                                                          0,
                                                          test_task_exit,
                                                          &top));

    /* 空任务入口必须被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32(MRT_RESULT_INVALID_ARGUMENT,
                           MRT_PortStm32CmInitializeStack(stack,
                                                          16u,
                                                          0,
                                                          0,
                                                          test_task_exit,
                                                          &top));

    /* 空任务退出处理函数必须被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32(MRT_RESULT_INVALID_ARGUMENT,
                           MRT_PortStm32CmInitializeStack(stack,
                                                          16u,
                                                          test_task_entry,
                                                          0,
                                                          0,
                                                          &top));

    /* 空输出指针必须被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32(MRT_RESULT_INVALID_ARGUMENT,
                           MRT_PortStm32CmInitializeStack(stack,
                                                          16u,
                                                          test_task_entry,
                                                          0,
                                                          test_task_exit,
                                                          0));
}

/**
 * @brief 执行 STM32 栈帧端口契约测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过。
 * @example
 * test_port_stm32_stack.exe
 */
int main(void)
{
    /* 验证正常路径的栈帧布局。 */
    test_stm32_stack_frame_places_core_registers();

    /* 验证非法参数防御路径。 */
    test_stm32_stack_frame_rejects_invalid_inputs();

    /* 所有断言通过，返回成功。 */
    return 0;
}
