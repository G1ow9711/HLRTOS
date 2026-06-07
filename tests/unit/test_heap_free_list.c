#include "mrt_test.h"
#include "myrtos/mrt_heap.h"

/**
 * @brief 验证可释放堆会复用已释放的块，并在大块复用为小块时保留剩余空闲空间。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_free_list_heap_reuses_released_block_and_splits_remainder();
 */
static void assert_free_list_heap_reuses_released_block_and_splits_remainder(void)
{
    /* 定义自然对齐的测试堆，容量足够容纳块头、拆分剩余块和保护块。 */
    uintptr_t heap_words[96u];

    /* 初始化为可释放空闲链表堆。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_HeapInitialize(heap_words,
                                                        sizeof(heap_words),
                                                        MRT_HEAP_MODE_FREE_LIST));

    /* 先分配一个较大的块，后续释放后应作为 first-fit 候选块。 */
    void *large = MRT_Malloc(128u);
    MRT_TEST_ASSERT_TRUE(large != 0);

    /* 再分配一个保护块，避免 large 成为堆尾唯一块而掩盖链表复用问题。 */
    void *guard = MRT_Malloc(32u);
    MRT_TEST_ASSERT_TRUE(guard != 0);

    /* 记录两个块都被占用时的剩余空间。 */
    size_t free_after_two_allocations = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_HeapGetFreeSize(&free_after_two_allocations));

    /* 释放第一个大块，空闲空间必须增加。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_Free(large));

    /* 查询释放后的空闲空间，用于确认释放操作更新统计值。 */
    size_t free_after_release = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_HeapGetFreeSize(&free_after_release));
    MRT_TEST_ASSERT_TRUE(free_after_release > free_after_two_allocations);

    /* 分配一个更小的块，应复用刚释放的大块起始地址。 */
    void *small = MRT_Malloc(32u);
    MRT_TEST_ASSERT_TRUE(small == large);

    /* 查询复用后的空闲空间。 */
    size_t free_after_reuse = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_HeapGetFreeSize(&free_after_reuse));

    /* 复用会消耗部分释放空间，因此空闲空间应低于释放后。 */
    MRT_TEST_ASSERT_TRUE(free_after_reuse < free_after_release);

    /* 大块被拆分后，剩余空间应仍高于两个原始块同时占用时的水位。 */
    MRT_TEST_ASSERT_TRUE(free_after_reuse > free_after_two_allocations);

    /* 防止编译器在极端优化设置下认为保护块未使用。 */
    MRT_TEST_ASSERT_TRUE(guard != small);
}

/**
 * @brief 验证可释放堆会拒绝非本堆指针，并保持空闲空间不变。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_free_list_heap_rejects_pointer_outside_heap();
 */
static void assert_free_list_heap_rejects_pointer_outside_heap(void)
{
    /* 定义测试堆和一个位于堆外的普通变量。 */
    uintptr_t heap_words[32u];
    uintptr_t outside_word = 0u;

    /* 初始化为可释放空闲链表堆。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_HeapInitialize(heap_words,
                                                        sizeof(heap_words),
                                                        MRT_HEAP_MODE_FREE_LIST));

    /* 分配一个合法块，使堆内部状态进入已分配路径。 */
    void *block = MRT_Malloc(24u);
    MRT_TEST_ASSERT_TRUE(block != 0);

    /* 记录非法释放前的空闲空间。 */
    size_t free_before = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_HeapGetFreeSize(&free_before));

    /* 堆外指针不属于任何分配块，必须拒绝。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_Free(&outside_word));

    /* 非法释放不能改变堆统计值。 */
    size_t free_after = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_HeapGetFreeSize(&free_after));
    MRT_TEST_ASSERT_EQ_U32((unsigned)free_before, (unsigned)free_after);
}

/**
 * @brief 验证可释放堆会拒绝重复释放同一个块。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_free_list_heap_rejects_double_free();
 */
static void assert_free_list_heap_rejects_double_free(void)
{
    /* 定义自然对齐的测试堆。 */
    uintptr_t heap_words[32u];

    /* 初始化为可释放空闲链表堆。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_HeapInitialize(heap_words,
                                                        sizeof(heap_words),
                                                        MRT_HEAP_MODE_FREE_LIST));

    /* 分配一个合法块。 */
    void *block = MRT_Malloc(40u);
    MRT_TEST_ASSERT_TRUE(block != 0);

    /* 第一次释放应成功。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_Free(block));

    /* 记录第一次释放后的空闲空间。 */
    size_t free_after_first_release = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_HeapGetFreeSize(&free_after_first_release));

    /* 第二次释放同一指针属于重复释放，必须拒绝。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_Free(block));

    /* 重复释放不能再次增加空闲空间。 */
    size_t free_after_second_release = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_HeapGetFreeSize(&free_after_second_release));
    MRT_TEST_ASSERT_EQ_U32((unsigned)free_after_first_release,
                           (unsigned)free_after_second_release);
}

/**
 * @brief 运行可释放空闲链表堆测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证释放块复用、块拆分和空闲空间统计。 */
    assert_free_list_heap_reuses_released_block_and_splits_remainder();

    /* 验证堆外指针释放保护。 */
    assert_free_list_heap_rejects_pointer_outside_heap();

    /* 验证重复释放保护。 */
    assert_free_list_heap_rejects_double_free();

    /* 所有可释放堆测试均通过。 */
    return 0;
}
