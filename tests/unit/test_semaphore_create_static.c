#include "mrt_test.h"
#include "myrtos/mrt_semaphore.h"

/**
 * @brief 验证二值信号量可按初始可用状态创建。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_binary_semaphore_static_creation_sets_initial_count();
 */
static void assert_binary_semaphore_static_creation_sets_initial_count(void)
{
    /* 定义空初始状态二值信号量控制块。 */
    MRT_Semaphore empty_storage;

    /* 定义满初始状态二值信号量控制块。 */
    MRT_Semaphore full_storage;

    /* 定义空初始状态信号量句柄。 */
    MRT_SemaphoreHandle empty = 0;

    /* 定义满初始状态信号量句柄。 */
    MRT_SemaphoreHandle full = 0;

    /* 创建初始不可用的二值信号量。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_SemaphoreCreateBinaryStatic(false, &empty_storage, &empty));

    /* 创建初始可用的二值信号量。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_SemaphoreCreateBinaryStatic(true, &full_storage, &full));

    /* 初始不可用二值信号量计数应为 0。 */
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)MRT_SemaphoreGetCount(empty));

    /* 初始可用二值信号量计数应为 1。 */
    MRT_TEST_ASSERT_EQ_U32(1u, (unsigned)MRT_SemaphoreGetCount(full));
}

/**
 * @brief 验证计数信号量创建时保存最大计数和初始计数。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_counting_semaphore_static_creation_sets_initial_count();
 */
static void assert_counting_semaphore_static_creation_sets_initial_count(void)
{
    /* 定义计数信号量控制块。 */
    MRT_Semaphore storage;

    /* 定义计数信号量句柄。 */
    MRT_SemaphoreHandle semaphore = 0;

    /* 创建最大计数 3、初始计数 2 的计数信号量。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_SemaphoreCreateCountingStatic(3u, 2u, &storage, &semaphore));

    /* 创建后当前计数应等于初始计数。 */
    MRT_TEST_ASSERT_EQ_U32(2u, (unsigned)MRT_SemaphoreGetCount(semaphore));
}

/**
 * @brief 验证静态信号量创建参数校验。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_semaphore_static_creation_rejects_invalid_arguments();
 */
static void assert_semaphore_static_creation_rejects_invalid_arguments(void)
{
    /* 定义信号量控制块。 */
    MRT_Semaphore storage;

    /* 定义信号量句柄。 */
    MRT_SemaphoreHandle semaphore = 0;

    /* 二值信号量控制块不能为空。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_SemaphoreCreateBinaryStatic(false, 0, &semaphore));

    /* 二值信号量输出句柄不能为空。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_SemaphoreCreateBinaryStatic(false, &storage, 0));

    /* 计数信号量最大计数不能为 0。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_SemaphoreCreateCountingStatic(0u, 0u, &storage, &semaphore));

    /* 计数信号量初始计数不能大于最大计数。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_SemaphoreCreateCountingStatic(2u, 3u, &storage, &semaphore));

    /* 计数信号量控制块不能为空。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_SemaphoreCreateCountingStatic(2u, 1u, 0, &semaphore));

    /* 计数信号量输出句柄不能为空。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_SemaphoreCreateCountingStatic(2u, 1u, &storage, 0));
}

/**
 * @brief 运行静态信号量创建测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证二值信号量静态创建。 */
    assert_binary_semaphore_static_creation_sets_initial_count();

    /* 验证计数信号量静态创建。 */
    assert_counting_semaphore_static_creation_sets_initial_count();

    /* 验证参数校验。 */
    assert_semaphore_static_creation_rejects_invalid_arguments();

    /* 所有静态信号量创建测试均通过。 */
    return 0;
}
