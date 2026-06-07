#include "mrt_test.h"
#include "myrtos/mrt_config.h"
#include "myrtos/mrt_heap.h"

/**
 * @brief 验证默认堆对齐配置满足嵌入式指针对齐要求。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_heap_alignment_config_is_valid();
 */
static void assert_heap_alignment_config_is_valid(void)
{
    /* 堆对齐必须至少能容纳一个指针，避免块头或用户指针未对齐。 */
    MRT_TEST_ASSERT_TRUE(MRT_CFG_HEAP_ALIGNMENT >= sizeof(void *));

    /* 堆对齐必须是 2 的幂，便于使用掩码执行快速对齐。 */
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)(MRT_CFG_HEAP_ALIGNMENT & (MRT_CFG_HEAP_ALIGNMENT - 1u)));
}

/**
 * @brief 验证堆初始化会建立可查询的初始空闲空间和历史最低水位。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_heap_initialize_sets_query_values();
 */
static void assert_heap_initialize_sets_query_values(void)
{
    /* 定义自然对齐的测试堆存储。 */
    uintptr_t heap_words[16u];

    /* 初始化线性堆。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_HeapInitialize(heap_words,
                                                        sizeof(heap_words),
                                                        MRT_HEAP_MODE_LINEAR));

    /* 初始化后空闲空间等于对齐后的堆容量。 */
    size_t free_size = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_HeapGetFreeSize(&free_size));
    MRT_TEST_ASSERT_EQ_U32((unsigned)sizeof(heap_words), (unsigned)free_size);

    /* 初始化后历史最低剩余空间等于当前剩余空间。 */
    size_t minimum_free = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_HeapGetMinimumEverFreeSize(&minimum_free));
    MRT_TEST_ASSERT_EQ_U32((unsigned)free_size, (unsigned)minimum_free);
}

/**
 * @brief 验证重复初始化会重置堆区域、模式和统计值。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_heap_reinitialize_resets_statistics();
 */
static void assert_heap_reinitialize_resets_statistics(void)
{
    /* 定义两个不同大小的自然对齐堆区域。 */
    uintptr_t small_heap[8u];
    uintptr_t large_heap[24u];

    /* 先初始化较小堆。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_HeapInitialize(small_heap,
                                                        sizeof(small_heap),
                                                        MRT_HEAP_MODE_LINEAR));

    /* 再初始化较大堆，应覆盖旧统计值。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_HeapInitialize(large_heap,
                                                        sizeof(large_heap),
                                                        MRT_HEAP_MODE_COALESCING));

    /* 查询当前空闲空间，必须来自较大堆。 */
    size_t free_size = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_HeapGetFreeSize(&free_size));
    MRT_TEST_ASSERT_EQ_U32((unsigned)sizeof(large_heap), (unsigned)free_size);

    /* 历史最低水位也应重置为较大堆的初始空闲空间。 */
    size_t minimum_free = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_HeapGetMinimumEverFreeSize(&minimum_free));
    MRT_TEST_ASSERT_EQ_U32((unsigned)sizeof(large_heap), (unsigned)minimum_free);
}

/**
 * @brief 验证堆初始化和查询 API 会拒绝非法参数。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_heap_initialize_rejects_invalid_arguments();
 */
static void assert_heap_initialize_rejects_invalid_arguments(void)
{
    /* 定义测试堆存储。 */
    uintptr_t heap_words[8u];

    /* 空堆缓冲不能初始化。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_HeapInitialize(0, sizeof(heap_words), MRT_HEAP_MODE_LINEAR));

    /* 堆区域小于最小对齐粒度时不能初始化。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_HeapInitialize(heap_words,
                                                        MRT_CFG_HEAP_ALIGNMENT - 1u,
                                                        MRT_HEAP_MODE_LINEAR));

    /* 未定义的堆模式不能初始化。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_HeapInitialize(heap_words,
                                                        sizeof(heap_words),
                                                        (MRT_HeapMode)99u));

    /* 先建立一个有效堆，便于验证查询参数。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_HeapInitialize(heap_words,
                                                        sizeof(heap_words),
                                                        MRT_HEAP_MODE_FREE_LIST));

    /* 空输出指针不能查询当前空闲空间。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_HeapGetFreeSize(0));

    /* 空输出指针不能查询历史最低剩余空间。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_HeapGetMinimumEverFreeSize(0));
}

/**
 * @brief 运行堆初始化测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证默认堆对齐配置。 */
    assert_heap_alignment_config_is_valid();

    /* 验证初始化后的查询值。 */
    assert_heap_initialize_sets_query_values();

    /* 验证重复初始化会重置状态。 */
    assert_heap_reinitialize_resets_statistics();

    /* 验证初始化和查询参数错误路径。 */
    assert_heap_initialize_rejects_invalid_arguments();

    /* 所有堆初始化测试均通过。 */
    return 0;
}
