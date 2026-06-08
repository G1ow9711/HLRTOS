#include "mrt_test.h"
#include "myrtos/portable/mrt_port_stm32_cm.h"

/**
 * @brief 验证 MPU 区域布局会把任意内存范围规整为可写入的 MPU 区域。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_normalize_mpu_region_rounds_up_and_aligns_down();
 */
static void assert_normalize_mpu_region_rounds_up_and_aligns_down(void)
{
    /* 准备一个不会天然对齐到 16 KiB 的 RAM 地址。 */
    uintptr_t base_address = (uintptr_t)0x20001234u;

    /* 准备一个和起始偏移相加后需要规整到 16 KiB 的长度。 */
    size_t size_bytes = 6000u;

    /* 接收规整后的 MPU 区域起始地址。 */
    uintptr_t region_base = 0u;

    /* 接收规整后的 MPU 区域大小。 */
    size_t region_size = 0u;

    /* 接收 MPU 区域大小的 log2 编码。 */
    uint32_t region_shift = 0u;

    /* 规整后的区域应覆盖原始地址范围。 */
    MRT_Result result = MRT_PortStm32CmNormalizeMpuRegion(base_address,
                                                           size_bytes,
                                                           &region_base,
                                                           &region_size,
                                                           &region_shift);

    /* 目前 helper 尚未实现；RED 期应先看到编译失败。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)result);

    /* 规整起始地址应向下对齐到 16 KiB 边界。 */
    MRT_TEST_ASSERT_EQ_U32(0x20000000u, (unsigned)region_base);

    /* 规整大小应向上扩展到 16 KiB。 */
    MRT_TEST_ASSERT_EQ_U32(16384u, (unsigned)region_size);

    /* 16 KiB 对应的 shift 应为 14。 */
    MRT_TEST_ASSERT_EQ_U32(14u, region_shift);
}

/**
 * @brief 验证 MPU 区域布局在本来就满足对齐时不会额外扩大区域。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_normalize_mpu_region_keeps_power_of_two_region();
 */
static void assert_normalize_mpu_region_keeps_power_of_two_region(void)
{
    /* 准备一个已经满足 32 KiB 对齐的基址。 */
    uintptr_t base_address = (uintptr_t)0x20008000u;

    /* 准备一个本来就是 32 KiB 的区域长度。 */
    size_t size_bytes = 32768u;

    /* 接收规整后的 MPU 区域起始地址。 */
    uintptr_t region_base = 0u;

    /* 接收规整后的 MPU 区域大小。 */
    size_t region_size = 0u;

    /* 接收 MPU 区域大小的 log2 编码。 */
    uint32_t region_shift = 0u;

    /* 规整后的区域应与原始范围一致。 */
    MRT_Result result = MRT_PortStm32CmNormalizeMpuRegion(base_address,
                                                           size_bytes,
                                                           &region_base,
                                                           &region_size,
                                                           &region_shift);

    /* 目前 helper 尚未实现；RED 期应先看到编译失败。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)result);

    /* 区域基址应保持不变。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)base_address, (unsigned)region_base);

    /* 区域大小应保持不变。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)size_bytes, (unsigned)region_size);

    /* 32 KiB 对应的 shift 应为 15。 */
    MRT_TEST_ASSERT_EQ_U32(15u, region_shift);
}

/**
 * @brief 验证 MPU 区域布局会拒绝非法参数。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_normalize_mpu_region_rejects_invalid_arguments();
 */
static void assert_normalize_mpu_region_rejects_invalid_arguments(void)
{
    /* 基址、大小和输出参数不能有空洞。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_PortStm32CmNormalizeMpuRegion(0x20000000u, 0u, 0, 0, 0));
}

/**
 * @brief 运行 STM32 MPU 布局 helper 测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证规整与对齐逻辑。 */
    assert_normalize_mpu_region_rounds_up_and_aligns_down();

    /* 验证幂等 power-of-two 区域不会被额外扩大。 */
    assert_normalize_mpu_region_keeps_power_of_two_region();

    /* 验证非法参数路径。 */
    assert_normalize_mpu_region_rejects_invalid_arguments();

    /* MPU 布局 helper 测试完成。 */
    return 0;
}
