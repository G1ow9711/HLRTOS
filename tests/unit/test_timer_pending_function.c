#include "mrt_test.h"
#include "myrtos/mrt_config.h"
#include "myrtos/mrt_kernel.h"
#include "myrtos/mrt_timer.h"

/** @brief pending function 测试记录容量。 */
#define PENDING_RECORD_CAPACITY (MRT_CFG_TIMER_PENDING_FUNCTION_QUEUE_LENGTH + 1u)

/** @brief pending function 执行时记录的参数标识。 */
static uint32_t g_recorded_args[PENDING_RECORD_CAPACITY];

/** @brief pending function 执行时记录的数值参数。 */
static uint32_t g_recorded_values[PENDING_RECORD_CAPACITY];

/** @brief pending function 已执行次数。 */
static uint32_t g_recorded_count;

/**
 * @brief 清空 pending function 测试记录。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * ResetPendingRecords();
 */
static void ResetPendingRecords(void)
{
    /* 清空已执行次数。 */
    g_recorded_count = 0u;

    /* 逐项清空记录数组。 */
    for (uint32_t index = 0u; index < PENDING_RECORD_CAPACITY; index++) {
        /* 清空参数标识记录。 */
        g_recorded_args[index] = 0u;

        /* 清空数值参数记录。 */
        g_recorded_values[index] = 0u;
    }
}

/**
 * @brief 测试用 pending function。
 * @param arg 指向 uint32_t 标识值的用户参数，允许为空。
 * @param value 投递时携带的整数值。
 * @return void 无返回值。
 * @example
 * RecordPendingFunction(&id, value);
 */
static void RecordPendingFunction(void *arg, uint32_t value)
{
    /* 确认记录数组仍有空间。 */
    MRT_TEST_ASSERT_TRUE(g_recorded_count < PENDING_RECORD_CAPACITY);

    /* 如果提供了参数标识，则记录标识值；否则记录 0。 */
    if (arg != 0) {
        /* 将用户参数转换为标识值指针。 */
        uint32_t *id = (uint32_t *)arg;

        /* 保存标识值。 */
        g_recorded_args[g_recorded_count] = *id;
    } else {
        /* 空参数用 0 表示。 */
        g_recorded_args[g_recorded_count] = 0u;
    }

    /* 保存整数参数。 */
    g_recorded_values[g_recorded_count] = value;

    /* 推进执行计数。 */
    g_recorded_count++;
}

/**
 * @brief 验证 pending function 按 FIFO 顺序执行并传递参数。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_pending_function_runs_in_fifo_order();
 */
static void assert_pending_function_runs_in_fifo_order(void)
{
    /* 初始化内核和定时器服务状态。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 清空测试记录。 */
    ResetPendingRecords();

    /* 定义两个参数标识。 */
    uint32_t first_id = 11u;
    uint32_t second_id = 22u;

    /* 投递第一个 pending function。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TimerPendFunctionCall(RecordPendingFunction, &first_id, 101u, 0u));

    /* 投递第二个 pending function。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TimerPendFunctionCall(RecordPendingFunction, &second_id, 202u, 0u));

    /* 投递只入队，不应立即执行。 */
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)g_recorded_count);

    /* 运行定时器服务 pending 队列。 */
    MRT_TimerServiceRunPending();

    /* 两个函数应按投递顺序执行。 */
    MRT_TEST_ASSERT_EQ_U32(2u, (unsigned)g_recorded_count);
    MRT_TEST_ASSERT_EQ_U32(11u, (unsigned)g_recorded_args[0]);
    MRT_TEST_ASSERT_EQ_U32(101u, (unsigned)g_recorded_values[0]);
    MRT_TEST_ASSERT_EQ_U32(22u, (unsigned)g_recorded_args[1]);
    MRT_TEST_ASSERT_EQ_U32(202u, (unsigned)g_recorded_values[1]);

    /* 再次运行空队列不应重复执行。 */
    MRT_TimerServiceRunPending();
    MRT_TEST_ASSERT_EQ_U32(2u, (unsigned)g_recorded_count);
}

/**
 * @brief 验证 pending function 队列满时返回对象满。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_pending_function_queue_full_returns_object_full();
 */
static void assert_pending_function_queue_full_returns_object_full(void)
{
    /* 初始化内核和定时器服务状态。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 清空测试记录。 */
    ResetPendingRecords();

    /* 定义循环投递使用的参数标识。 */
    uint32_t id = 33u;

    /* 填满 pending function 队列。 */
    for (uint32_t index = 0u; index < MRT_CFG_TIMER_PENDING_FUNCTION_QUEUE_LENGTH; index++) {
        /* 队列未满时每次投递都应成功。 */
        MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                               (unsigned)MRT_TimerPendFunctionCall(RecordPendingFunction, &id, index, 0u));
    }

    /* 队列满后再次投递应失败。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OBJECT_FULL,
                           (unsigned)MRT_TimerPendFunctionCall(RecordPendingFunction, &id, 999u, 0u));

    /* 执行队列，确认只执行成功入队的项目。 */
    MRT_TimerServiceRunPending();
    MRT_TEST_ASSERT_EQ_U32(MRT_CFG_TIMER_PENDING_FUNCTION_QUEUE_LENGTH, (unsigned)g_recorded_count);
}

/**
 * @brief 验证 pending function 参数校验。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_pending_function_rejects_invalid_arguments();
 */
static void assert_pending_function_rejects_invalid_arguments(void)
{
    /* 初始化内核和定时器服务状态。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 空函数指针不能投递。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_TimerPendFunctionCall(0, 0, 1u, 0u));
}

/**
 * @brief 运行软件定时器 pending function 测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证 FIFO 和参数传递。 */
    assert_pending_function_runs_in_fifo_order();

    /* 验证队列满边界。 */
    assert_pending_function_queue_full_returns_object_full();

    /* 验证参数错误路径。 */
    assert_pending_function_rejects_invalid_arguments();

    /* 所有 pending function 测试均通过。 */
    return 0;
}
