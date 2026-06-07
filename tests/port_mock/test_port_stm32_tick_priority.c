#include "mrt_test.h"
#include "myrtos/portable/mrt_port_stm32_cm.h"

#include <stdint.h>

/**
 * @brief 验证 SysTick reload 计算符合 Cortex-M 24 位定时器规则。
 * @param void 无输入参数。
 * @return void 无返回值；断言失败会直接退出测试进程。
 * @example
 * test_systick_reload_calculation();
 */
static void test_systick_reload_calculation(void)
{
    /* 准备接收 reload 计算结果。 */
    uint32_t reload = 0u;

    /* 168 MHz 内核时钟下 1 kHz tick 应得到 168000 - 1。 */
    MRT_TEST_ASSERT_EQ_U32(MRT_RESULT_OK,
                           MRT_PortStm32CmCalculateSysTickReload(168000000u,
                                                                 1000u,
                                                                 &reload));

    /* 验证 reload 数值。 */
    MRT_TEST_ASSERT_EQ_U32(167999u, reload);

    /* 8 MHz 输入时钟下 100 Hz tick 应得到 80000 - 1。 */
    MRT_TEST_ASSERT_EQ_U32(MRT_RESULT_OK,
                           MRT_PortStm32CmCalculateSysTickReload(8000000u,
                                                                 100u,
                                                                 &reload));

    /* 验证第二组 reload 数值。 */
    MRT_TEST_ASSERT_EQ_U32(79999u, reload);
}

/**
 * @brief 验证 SysTick reload helper 会拒绝非法频率和输出参数。
 * @param void 无输入参数。
 * @return void 无返回值；断言失败会直接退出测试进程。
 * @example
 * test_systick_reload_rejects_invalid_rates();
 */
static void test_systick_reload_rejects_invalid_rates(void)
{
    /* 使用哨兵值检查失败路径不会改写输出。 */
    uint32_t reload = 0xAAAAAAAAu;

    /* CPU 时钟为 0 必须被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32(MRT_RESULT_INVALID_ARGUMENT,
                           MRT_PortStm32CmCalculateSysTickReload(0u,
                                                                 1000u,
                                                                 &reload));

    /* 验证失败路径没有改写输出值。 */
    MRT_TEST_ASSERT_EQ_U32(0xAAAAAAAAu, reload);

    /* tick 频率为 0 必须被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32(MRT_RESULT_INVALID_ARGUMENT,
                           MRT_PortStm32CmCalculateSysTickReload(168000000u,
                                                                 0u,
                                                                 &reload));

    /* tick 频率高于输入时钟必须被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32(MRT_RESULT_INVALID_ARGUMENT,
                           MRT_PortStm32CmCalculateSysTickReload(100u,
                                                                 1000u,
                                                                 &reload));

    /* reload 超过 24 位 SysTick 上限必须被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32(MRT_RESULT_INVALID_ARGUMENT,
                           MRT_PortStm32CmCalculateSysTickReload(480000000u,
                                                                 1u,
                                                                 &reload));

    /* 空输出指针必须被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32(MRT_RESULT_INVALID_ARGUMENT,
                           MRT_PortStm32CmCalculateSysTickReload(168000000u,
                                                                 1000u,
                                                                 0));
}

/**
 * @brief 验证 BASEPRI 优先级编码会按 NVIC 位宽左对齐。
 * @param void 无输入参数。
 * @return void 无返回值；断言失败会直接退出测试进程。
 * @example
 * test_basepri_priority_encoding();
 */
static void test_basepri_priority_encoding(void)
{
    /* 准备接收编码后的 BASEPRI 数值。 */
    uint32_t encoded = 0u;

    /* 4 bit 优先级中逻辑优先级 5 应编码为 0x50。 */
    MRT_TEST_ASSERT_EQ_U32(MRT_RESULT_OK,
                           MRT_PortStm32CmEncodeBasepri(4u, 5u, &encoded));

    /* 验证 4 bit 编码结果。 */
    MRT_TEST_ASSERT_EQ_U32(0x50u, encoded);

    /* 3 bit 优先级中逻辑优先级 6 应编码为 0xC0。 */
    MRT_TEST_ASSERT_EQ_U32(MRT_RESULT_OK,
                           MRT_PortStm32CmEncodeBasepri(3u, 6u, &encoded));

    /* 验证 3 bit 编码结果。 */
    MRT_TEST_ASSERT_EQ_U32(0xC0u, encoded);
}

/**
 * @brief 验证 BASEPRI helper 会拒绝非法位宽、优先级和输出参数。
 * @param void 无输入参数。
 * @return void 无返回值；断言失败会直接退出测试进程。
 * @example
 * test_basepri_priority_rejects_invalid_values();
 */
static void test_basepri_priority_rejects_invalid_values(void)
{
    /* 使用哨兵值检查失败路径不会改写输出。 */
    uint32_t encoded = 0xBBBBBBBBu;

    /* NVIC 优先级位宽为 0 必须被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32(MRT_RESULT_INVALID_ARGUMENT,
                           MRT_PortStm32CmEncodeBasepri(0u, 1u, &encoded));

    /* 验证失败路径没有改写输出值。 */
    MRT_TEST_ASSERT_EQ_U32(0xBBBBBBBBu, encoded);

    /* NVIC 优先级位宽超过 8 必须被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32(MRT_RESULT_INVALID_ARGUMENT,
                           MRT_PortStm32CmEncodeBasepri(9u, 1u, &encoded));

    /* 逻辑优先级超出 4 bit 表示范围必须被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32(MRT_RESULT_INVALID_ARGUMENT,
                           MRT_PortStm32CmEncodeBasepri(4u, 16u, &encoded));

    /* 逻辑优先级 0 会让 BASEPRI 失去屏蔽含义，必须被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32(MRT_RESULT_INVALID_ARGUMENT,
                           MRT_PortStm32CmEncodeBasepri(4u, 0u, &encoded));

    /* 空输出指针必须被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32(MRT_RESULT_INVALID_ARGUMENT,
                           MRT_PortStm32CmEncodeBasepri(4u, 1u, 0));
}

/**
 * @brief 执行 STM32 tick 和优先级端口契约测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过。
 * @example
 * test_port_stm32_tick_priority.exe
 */
int main(void)
{
    /* 验证 SysTick reload 正常计算路径。 */
    test_systick_reload_calculation();

    /* 验证 SysTick reload 参数防御路径。 */
    test_systick_reload_rejects_invalid_rates();

    /* 验证 BASEPRI 正常编码路径。 */
    test_basepri_priority_encoding();

    /* 验证 BASEPRI 参数防御路径。 */
    test_basepri_priority_rejects_invalid_values();

    /* 所有断言通过，返回成功。 */
    return 0;
}
