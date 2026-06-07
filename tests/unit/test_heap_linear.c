#include "mrt_test.h"
#include "myrtos/mrt_config.h"
#include "myrtos/mrt_heap.h"

/**
 * @brief 将测试用字节数向上规整到堆对齐粒度。
 * @param size 原始字节数。
 * @return size_t 返回对齐后的字节数。
 * @example
 * size_t aligned = AlignUpForTest(3);
 */
static size_t AlignUpForTest(size_t size)
{
    /* 计算对齐掩码。 */
    size_t mask = MRT_CFG_HEAP_ALIGNMENT - 1u;

    /* 向上对齐并返回。 */
    return (size + mask) & ~mask;
}

/**
 * @brief 判断指针是否满足堆对齐要求。
 * @param ptr 待检查指针。
 * @return bool 返回 true 表示指针对齐，返回 false 表示指针未对齐。
 * @example
 * MRT_TEST_ASSERT_TRUE(PointerIsHeapAligned(ptr));
 */
static bool PointerIsHeapAligned(const void *ptr)
{
    /* 将指针转换为整数后检查低位。 */
    return (((uintptr_t)ptr) & ((uintptr_t)MRT_CFG_HEAP_ALIGNMENT - 1u)) == 0u;
}

/**
 * @brief 验证线性堆按对齐粒度分配并更新水位统计。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_linear_heap_allocates_aligned_blocks_and_tracks_watermark();
 */
static void assert_linear_heap_allocates_aligned_blocks_and_tracks_watermark(void)
{
    /* 定义自然对齐的测试堆。 */
    uintptr_t heap_words[16u];

    /* 初始化线性堆。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_HeapInitialize(heap_words,
                                                        sizeof(heap_words),
                                                        MRT_HEAP_MODE_LINEAR));

    /* 分配 1 字节，实际消耗应按堆对齐粒度规整。 */
    void *first = MRT_Malloc(1u);
    MRT_TEST_ASSERT_TRUE(first != 0);
    MRT_TEST_ASSERT_TRUE(PointerIsHeapAligned(first));

    /* 分配 9 字节，继续保持返回地址对齐。 */
    void *second = MRT_Malloc(9u);
    MRT_TEST_ASSERT_TRUE(second != 0);
    MRT_TEST_ASSERT_TRUE(PointerIsHeapAligned(second));

    /* 计算两次分配后的预期剩余空间。 */
    size_t expected_free = sizeof(heap_words) - AlignUpForTest(1u) - AlignUpForTest(9u);

    /* 当前剩余空间应等于预期值。 */
    size_t free_size = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_HeapGetFreeSize(&free_size));
    MRT_TEST_ASSERT_EQ_U32((unsigned)expected_free, (unsigned)free_size);

    /* 历史最低剩余空间也应等于当前剩余空间。 */
    size_t minimum_free = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_HeapGetMinimumEverFreeSize(&minimum_free));
    MRT_TEST_ASSERT_EQ_U32((unsigned)expected_free, (unsigned)minimum_free);
}

/**
 * @brief 验证线性堆零长度分配和耗尽路径。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_linear_heap_handles_zero_size_and_exhaustion();
 */
static void assert_linear_heap_handles_zero_size_and_exhaustion(void)
{
    /* 定义刚好 16 字节的自然对齐堆。 */
    uintptr_t heap_words[2u];

    /* 初始化线性堆。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_HeapInitialize(heap_words,
                                                        sizeof(heap_words),
                                                        MRT_HEAP_MODE_LINEAR));

    /* 零长度分配没有明确所有权，必须返回空指针。 */
    MRT_TEST_ASSERT_TRUE(MRT_Malloc(0u) == 0);

    /* 一次分配整个堆应成功。 */
    void *whole_heap = MRT_Malloc(sizeof(heap_words));
    MRT_TEST_ASSERT_TRUE(whole_heap != 0);
    MRT_TEST_ASSERT_TRUE(PointerIsHeapAligned(whole_heap));

    /* 堆耗尽后再次分配应失败。 */
    MRT_TEST_ASSERT_TRUE(MRT_Malloc(1u) == 0);

    /* 当前剩余空间和历史最低水位都应为 0。 */
    size_t free_size = 99u;
    size_t minimum_free = 99u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_HeapGetFreeSize(&free_size));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_HeapGetMinimumEverFreeSize(&minimum_free));
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)free_size);
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)minimum_free);
}

/**
 * @brief 验证线性堆释放策略。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_linear_heap_free_policy_is_no_reuse();
 */
static void assert_linear_heap_free_policy_is_no_reuse(void)
{
    /* 定义自然对齐的测试堆。 */
    uintptr_t heap_words[8u];

    /* 初始化线性堆。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_HeapInitialize(heap_words,
                                                        sizeof(heap_words),
                                                        MRT_HEAP_MODE_LINEAR));

    /* 分配一个小块。 */
    void *block = MRT_Malloc(3u);
    MRT_TEST_ASSERT_TRUE(block != 0);

    /* 空指针释放应作为无操作成功，便于调用方清理路径简化。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_Free(0));

    /* 线性堆不支持单块释放，应返回对象忙并保持水位不变。 */
    size_t free_before = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_HeapGetFreeSize(&free_before));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OBJECT_BUSY, (unsigned)MRT_Free(block));

    /* 释放失败后空闲空间不能变化。 */
    size_t free_after = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_HeapGetFreeSize(&free_after));
    MRT_TEST_ASSERT_EQ_U32((unsigned)free_before, (unsigned)free_after);
}

/**
 * @brief 运行线性堆测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证对齐分配和水位统计。 */
    assert_linear_heap_allocates_aligned_blocks_and_tracks_watermark();

    /* 验证零长度和耗尽路径。 */
    assert_linear_heap_handles_zero_size_and_exhaustion();

    /* 验证线性堆释放策略。 */
    assert_linear_heap_free_policy_is_no_reuse();

    /* 所有线性堆测试均通过。 */
    return 0;
}
