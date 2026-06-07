#include "mrt_test.h"
#include "myrtos/mrt_semaphore.h"

/**
 * @brief 创建测试用计数信号量。
 * @param max_count 最大计数，必须大于 0。
 * @param initial_count 初始计数，不能大于 max_count。
 * @param storage 信号量控制块，不能为空。
 * @param out_semaphore 输出信号量句柄，不能为空。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * create_counting_semaphore(2, 1, &storage, &sem);
 */
static void create_counting_semaphore(size_t max_count,
                                      size_t initial_count,
                                      MRT_Semaphore *storage,
                                      MRT_SemaphoreHandle *out_semaphore)
{
    /* 创建指定最大计数和初始计数的测试信号量。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_SemaphoreCreateCountingStatic(max_count,
                                                                       initial_count,
                                                                       storage,
                                                                       out_semaphore));
}

/**
 * @brief 验证非阻塞 take 会消耗一个可用计数。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_take_consumes_available_count();
 */
static void assert_take_consumes_available_count(void)
{
    /* 定义信号量控制块。 */
    MRT_Semaphore storage;

    /* 定义信号量句柄。 */
    MRT_SemaphoreHandle semaphore = 0;

    /* 创建当前计数为 1 的二值信号量。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_SemaphoreCreateBinaryStatic(true, &storage, &semaphore));

    /* 获取一次信号量应成功。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_SemaphoreTake(semaphore, 0u));

    /* 获取后当前计数应降为 0。 */
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)MRT_SemaphoreGetCount(semaphore));
}

/**
 * @brief 验证空信号量非阻塞 take 返回空状态。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_take_empty_semaphore_returns_empty();
 */
static void assert_take_empty_semaphore_returns_empty(void)
{
    /* 定义信号量控制块。 */
    MRT_Semaphore storage;

    /* 定义信号量句柄。 */
    MRT_SemaphoreHandle semaphore = 0;

    /* 创建当前计数为 0 的二值信号量。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_SemaphoreCreateBinaryStatic(false, &storage, &semaphore));

    /* 非阻塞获取空信号量应返回对象为空。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OBJECT_EMPTY, (unsigned)MRT_SemaphoreTake(semaphore, 0u));
}

/**
 * @brief 验证非零 timeout 在无当前任务时返回超时。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_take_timeout_without_current_task_returns_timeout();
 */
static void assert_take_timeout_without_current_task_returns_timeout(void)
{
    /* 定义信号量控制块。 */
    MRT_Semaphore storage;

    /* 定义信号量句柄。 */
    MRT_SemaphoreHandle semaphore = 0;

    /* 创建当前计数为 0 的二值信号量。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_SemaphoreCreateBinaryStatic(false, &storage, &semaphore));

    /* 当前没有调度运行任务，非零 timeout 只能表现为超时。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_TIMEOUT, (unsigned)MRT_SemaphoreTake(semaphore, 5u));
}

/**
 * @brief 验证 give 会增加一个计数。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_give_increments_count();
 */
static void assert_give_increments_count(void)
{
    /* 定义信号量控制块。 */
    MRT_Semaphore storage;

    /* 定义信号量句柄。 */
    MRT_SemaphoreHandle semaphore = 0;

    /* 创建最大计数为 2、当前计数为 0 的计数信号量。 */
    create_counting_semaphore(2u, 0u, &storage, &semaphore);

    /* 释放一次信号量应成功。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_SemaphoreGive(semaphore));

    /* 释放后当前计数应增加为 1。 */
    MRT_TEST_ASSERT_EQ_U32(1u, (unsigned)MRT_SemaphoreGetCount(semaphore));
}

/**
 * @brief 验证满信号量 give 返回满状态。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_give_full_semaphore_returns_full();
 */
static void assert_give_full_semaphore_returns_full(void)
{
    /* 定义信号量控制块。 */
    MRT_Semaphore storage;

    /* 定义信号量句柄。 */
    MRT_SemaphoreHandle semaphore = 0;

    /* 创建最大计数为 2、当前计数也为 2 的满信号量。 */
    create_counting_semaphore(2u, 2u, &storage, &semaphore);

    /* 满信号量继续 give 应返回对象已满。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OBJECT_FULL, (unsigned)MRT_SemaphoreGive(semaphore));
}

/**
 * @brief 验证 take/give 参数校验。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_take_give_reject_null_handle();
 */
static void assert_take_give_reject_null_handle(void)
{
    /* 空句柄 take 应返回参数错误。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT, (unsigned)MRT_SemaphoreTake(0, 0u));

    /* 空句柄 give 应返回参数错误。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT, (unsigned)MRT_SemaphoreGive(0));
}

/**
 * @brief 运行信号量 take/give 测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证 take 消耗计数。 */
    assert_take_consumes_available_count();

    /* 验证空信号量非阻塞 take。 */
    assert_take_empty_semaphore_returns_empty();

    /* 验证无当前任务 timeout 行为。 */
    assert_take_timeout_without_current_task_returns_timeout();

    /* 验证 give 增加计数。 */
    assert_give_increments_count();

    /* 验证满信号量 give。 */
    assert_give_full_semaphore_returns_full();

    /* 验证参数校验。 */
    assert_take_give_reject_null_handle();

    /* 所有信号量 take/give 测试均通过。 */
    return 0;
}
