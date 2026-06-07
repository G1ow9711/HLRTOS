#include "mrt_test.h"
#include "myrtos/mrt_heap.h"

/**
 * @brief 用固定大小的小分配耗尽堆尾剩余空间。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * ExhaustTailFreeSpaceForTest();
 */
static void ExhaustTailFreeSpaceForTest(void)
{
    /* 设置循环上限，避免实现错误导致测试无限循环。 */
    unsigned guard = 0u;

    /* 持续分配小块，直到堆空间或碎片无法继续满足请求。 */
    while (MRT_Malloc(32u) != 0) {
        /* 记录一次成功分配。 */
        guard++;

        /* 当前测试堆容量有限，超过该次数说明分配器状态异常。 */
        MRT_TEST_ASSERT_TRUE(guard < 64u);
    }
}

/**
 * @brief 验证合并堆会合并两个相邻释放块并承载更大的后续分配。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_coalescing_heap_merges_adjacent_free_blocks();
 */
static void assert_coalescing_heap_merges_adjacent_free_blocks(void)
{
    /* 定义自然对齐的测试堆。 */
    uintptr_t heap_words[96u];

    /* 初始化为相邻块合并堆。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_HeapInitialize(heap_words,
                                                        sizeof(heap_words),
                                                        MRT_HEAP_MODE_COALESCING));

    /* 分配三个连续块。 */
    void *first = MRT_Malloc(64u);
    void *second = MRT_Malloc(64u);
    void *third = MRT_Malloc(64u);
    MRT_TEST_ASSERT_TRUE(first != 0);
    MRT_TEST_ASSERT_TRUE(second != 0);
    MRT_TEST_ASSERT_TRUE(third != 0);

    /* 耗尽第三个块之后的尾部空间，保证后续大分配只能来自合并后的邻居块。 */
    ExhaustTailFreeSpaceForTest();

    /* 记录耗尽尾部空间后的空闲水位。 */
    size_t free_after_exhaustion = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_HeapGetFreeSize(&free_after_exhaustion));

    /* 释放第一个块。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_Free(first));

    /* 查询释放一个块后的空闲空间。 */
    size_t free_after_first_release = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_HeapGetFreeSize(&free_after_first_release));
    MRT_TEST_ASSERT_TRUE(free_after_first_release > free_after_exhaustion);

    /* 释放第二个块，它与第一个块相邻，应触发合并。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_Free(second));

    /* 查询释放两个邻居块后的空闲空间。 */
    size_t free_after_neighbor_release = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_HeapGetFreeSize(&free_after_neighbor_release));
    MRT_TEST_ASSERT_TRUE(free_after_neighbor_release > free_after_first_release);

    /* 请求一个大于任一 64 字节单块的块；只有相邻块合并后才应成功。 */
    void *merged = MRT_Malloc(96u);
    MRT_TEST_ASSERT_TRUE(merged == first);

    /* 查询大块分配后的空闲空间。 */
    size_t free_after_merged_allocation = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_HeapGetFreeSize(&free_after_merged_allocation));

    /* 大块分配会消耗合并块的一部分，因此水位应低于释放两个邻居块后。 */
    MRT_TEST_ASSERT_TRUE(free_after_merged_allocation < free_after_neighbor_release);

    /* 合并块被拆分后仍应留下剩余空闲空间，证明统计值反映的是合并后的整块空间。 */
    MRT_TEST_ASSERT_TRUE(free_after_merged_allocation > free_after_exhaustion);

    /* 防止编译器在极端优化设置下认为第三个块未使用。 */
    MRT_TEST_ASSERT_TRUE(third != merged);
}

/**
 * @brief 验证普通空闲链表堆不会合并相邻释放块。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_free_list_heap_keeps_adjacent_free_blocks_separate();
 */
static void assert_free_list_heap_keeps_adjacent_free_blocks_separate(void)
{
    /* 定义自然对齐的测试堆。 */
    uintptr_t heap_words[96u];

    /* 初始化为普通空闲链表堆。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_HeapInitialize(heap_words,
                                                        sizeof(heap_words),
                                                        MRT_HEAP_MODE_FREE_LIST));

    /* 分配三个连续块。 */
    void *first = MRT_Malloc(64u);
    void *second = MRT_Malloc(64u);
    void *third = MRT_Malloc(64u);
    MRT_TEST_ASSERT_TRUE(first != 0);
    MRT_TEST_ASSERT_TRUE(second != 0);
    MRT_TEST_ASSERT_TRUE(third != 0);

    /* 耗尽尾部空间，避免后续大分配从尾部空闲区成功。 */
    ExhaustTailFreeSpaceForTest();

    /* 释放两个相邻块。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_Free(first));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_Free(second));

    /* 普通空闲链表堆不合并邻居块，因此大于单块容量的请求应失败。 */
    MRT_TEST_ASSERT_TRUE(MRT_Malloc(96u) == 0);

    /* 防止编译器在极端优化设置下认为第三个块未使用。 */
    MRT_TEST_ASSERT_TRUE(third != 0);
}

/**
 * @brief 运行合并堆释放测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证合并堆会合并相邻释放块。 */
    assert_coalescing_heap_merges_adjacent_free_blocks();

    /* 验证普通空闲链表堆仍保持不合并策略。 */
    assert_free_list_heap_keeps_adjacent_free_blocks_separate();

    /* 所有合并堆测试均通过。 */
    return 0;
}
