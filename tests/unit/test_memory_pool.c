#include "mrt_test.h"
#include "myrtos/mrt_memory_pool.h"

/**
 * @brief 验证固定块内存池可以静态创建并正确报告空闲块数量。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_memory_pool_create_static_sets_initial_state();
 */
static void assert_memory_pool_create_static_sets_initial_state(void)
{
    /* 使用 uintptr_t 数组保证底层存储天然满足指针对齐。 */
    uintptr_t storage_words[8u];

    /* 定义调用方提供的内存池控制块。 */
    MRT_MemoryPool pool_storage;

    /* 定义输出句柄。 */
    MRT_MemoryPoolHandle pool = 0;

    /* 创建 4 个 16 字节固定块。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_MemoryPoolCreateStatic(16u,
                                                                 4u,
                                                                 storage_words,
                                                                 &pool_storage,
                                                                 &pool));

    /* 静态创建应返回调用方提供的控制块地址。 */
    MRT_TEST_ASSERT_TRUE(pool == &pool_storage);

    /* 初始空闲块数量应等于创建时的块数量。 */
    size_t free_count = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_MemoryPoolGetFreeCount(pool, &free_count));
    MRT_TEST_ASSERT_EQ_U32(4u, (unsigned)free_count);
}

/**
 * @brief 验证固定块内存池会拒绝非法创建参数。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_memory_pool_create_static_rejects_invalid_arguments();
 */
static void assert_memory_pool_create_static_rejects_invalid_arguments(void)
{
    /* 准备合法参数组合，用于逐项替换非法参数。 */
    uintptr_t storage_words[8u];
    MRT_MemoryPool pool_storage;
    MRT_MemoryPoolHandle pool = 0;

    /* 空底层存储必须被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_MemoryPoolCreateStatic(16u,
                                                                 4u,
                                                                 0,
                                                                 &pool_storage,
                                                                 &pool));

    /* 空控制块必须被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_MemoryPoolCreateStatic(16u,
                                                                 4u,
                                                                 storage_words,
                                                                 0,
                                                                 &pool));

    /* 空输出句柄必须被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_MemoryPoolCreateStatic(16u,
                                                                 4u,
                                                                 storage_words,
                                                                 &pool_storage,
                                                                 0));

    /* 固定块必须至少能容纳一个空闲链表指针。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_MemoryPoolCreateStatic(sizeof(void *) - 1u,
                                                                 4u,
                                                                 storage_words,
                                                                 &pool_storage,
                                                                 &pool));

    /* 块数量为 0 时没有可管理对象，必须拒绝。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_MemoryPoolCreateStatic(16u,
                                                                 0u,
                                                                 storage_words,
                                                                 &pool_storage,
                                                                 &pool));
}

/**
 * @brief 验证固定块内存池可以分配到空并在空池时返回对象空。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_memory_pool_allocates_until_empty();
 */
static void assert_memory_pool_allocates_until_empty(void)
{
    /* 准备 3 个 16 字节固定块的底层存储。 */
    uintptr_t storage_words[6u];
    MRT_MemoryPool pool_storage;
    MRT_MemoryPoolHandle pool = 0;

    /* 创建固定块内存池。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_MemoryPoolCreateStatic(16u,
                                                                 3u,
                                                                 storage_words,
                                                                 &pool_storage,
                                                                 &pool));

    /* 依次分配三个块。 */
    void *first = 0;
    void *second = 0;
    void *third = 0;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MemoryPoolAlloc(pool, &first));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MemoryPoolAlloc(pool, &second));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MemoryPoolAlloc(pool, &third));

    /* 三个块都应有效且互不相同。 */
    MRT_TEST_ASSERT_TRUE(first != 0);
    MRT_TEST_ASSERT_TRUE(second != 0);
    MRT_TEST_ASSERT_TRUE(third != 0);
    MRT_TEST_ASSERT_TRUE(first != second);
    MRT_TEST_ASSERT_TRUE(second != third);
    MRT_TEST_ASSERT_TRUE(first != third);

    /* 分配到空后空闲块数量应为 0。 */
    size_t free_count = 99u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_MemoryPoolGetFreeCount(pool, &free_count));
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)free_count);

    /* 再次分配应返回对象空，并把输出指针清为 NULL。 */
    void *empty_block = (void *)storage_words;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OBJECT_EMPTY,
                           (unsigned)MRT_MemoryPoolAlloc(pool, &empty_block));
    MRT_TEST_ASSERT_TRUE(empty_block == 0);
}

/**
 * @brief 验证释放块会归还给内存池并可被再次分配。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_memory_pool_free_returns_block_to_pool();
 */
static void assert_memory_pool_free_returns_block_to_pool(void)
{
    /* 准备 2 个 16 字节固定块的底层存储。 */
    uintptr_t storage_words[4u];
    MRT_MemoryPool pool_storage;
    MRT_MemoryPoolHandle pool = 0;

    /* 创建固定块内存池。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_MemoryPoolCreateStatic(16u,
                                                                 2u,
                                                                 storage_words,
                                                                 &pool_storage,
                                                                 &pool));

    /* 分配一个块。 */
    void *block = 0;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MemoryPoolAlloc(pool, &block));
    MRT_TEST_ASSERT_TRUE(block != 0);

    /* 释放该块。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MemoryPoolFree(pool, block));

    /* 空闲块数量应恢复到 2。 */
    size_t free_count = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_MemoryPoolGetFreeCount(pool, &free_count));
    MRT_TEST_ASSERT_EQ_U32(2u, (unsigned)free_count);

    /* 下一次分配应优先返回刚释放的块。 */
    void *reused = 0;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MemoryPoolAlloc(pool, &reused));
    MRT_TEST_ASSERT_TRUE(reused == block);
}

/**
 * @brief 验证固定块内存池会拒绝非法释放和重复释放。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_memory_pool_rejects_invalid_and_duplicate_free();
 */
static void assert_memory_pool_rejects_invalid_and_duplicate_free(void)
{
    /* 准备 2 个 16 字节固定块的底层存储。 */
    uintptr_t storage_words[4u];
    uintptr_t outside_word = 0u;
    MRT_MemoryPool pool_storage;
    MRT_MemoryPoolHandle pool = 0;

    /* 创建固定块内存池。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_MemoryPoolCreateStatic(16u,
                                                                 2u,
                                                                 storage_words,
                                                                 &pool_storage,
                                                                 &pool));

    /* 分配一个合法块。 */
    void *block = 0;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MemoryPoolAlloc(pool, &block));

    /* 记录非法释放前的空闲块数量。 */
    size_t free_before = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_MemoryPoolGetFreeCount(pool, &free_before));

    /* 堆外指针必须被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_MemoryPoolFree(pool, &outside_word));

    /* 非法释放不能改变空闲块数量。 */
    size_t free_after_invalid = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_MemoryPoolGetFreeCount(pool, &free_after_invalid));
    MRT_TEST_ASSERT_EQ_U32((unsigned)free_before, (unsigned)free_after_invalid);

    /* 第一次释放合法块应成功。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MemoryPoolFree(pool, block));

    /* 记录第一次释放后的空闲块数量。 */
    size_t free_after_first = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_MemoryPoolGetFreeCount(pool, &free_after_first));

    /* 第二次释放同一块属于重复释放，必须拒绝。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_MemoryPoolFree(pool, block));

    /* 重复释放不能再次增加空闲块数量。 */
    size_t free_after_second = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_MemoryPoolGetFreeCount(pool, &free_after_second));
    MRT_TEST_ASSERT_EQ_U32((unsigned)free_after_first, (unsigned)free_after_second);
}

/**
 * @brief 验证固定块内存池运行期 API 会拒绝空参数。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_memory_pool_runtime_apis_reject_null_arguments();
 */
static void assert_memory_pool_runtime_apis_reject_null_arguments(void)
{
    /* 准备合法内存池。 */
    uintptr_t storage_words[4u];
    MRT_MemoryPool pool_storage;
    MRT_MemoryPoolHandle pool = 0;
    void *block = 0;
    size_t free_count = 0u;

    /* 创建固定块内存池。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_MemoryPoolCreateStatic(16u,
                                                                 2u,
                                                                 storage_words,
                                                                 &pool_storage,
                                                                 &pool));

    /* 空池句柄或空输出指针必须被分配 API 拒绝。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_MemoryPoolAlloc(0, &block));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_MemoryPoolAlloc(pool, 0));

    /* 空池句柄或空块指针必须被释放 API 拒绝。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_MemoryPoolFree(0, storage_words));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_MemoryPoolFree(pool, 0));

    /* 空池句柄或空输出指针必须被查询 API 拒绝。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_MemoryPoolGetFreeCount(0, &free_count));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_MemoryPoolGetFreeCount(pool, 0));
}

/**
 * @brief 运行固定块内存池测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证静态创建初始状态。 */
    assert_memory_pool_create_static_sets_initial_state();

    /* 验证创建参数校验。 */
    assert_memory_pool_create_static_rejects_invalid_arguments();

    /* 验证分配到空路径。 */
    assert_memory_pool_allocates_until_empty();

    /* 验证释放后复用。 */
    assert_memory_pool_free_returns_block_to_pool();

    /* 验证非法释放和重复释放保护。 */
    assert_memory_pool_rejects_invalid_and_duplicate_free();

    /* 验证运行期 API 空参数保护。 */
    assert_memory_pool_runtime_apis_reject_null_arguments();

    /* 所有固定块内存池测试均通过。 */
    return 0;
}
