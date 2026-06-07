#include "mrt_test.h"
#include "myrtos/mrt_kernel.h"
#include "myrtos/mrt_semaphore.h"
#include "myrtos/mrt_task.h"

/**
 * @brief 测试用任务入口函数。
 * @param arg 用户参数，本测试不使用。
 * @return void 无返回值。
 * @example
 * DummyTask(NULL);
 */
static void DummyTask(void *arg)
{
    /* 显式丢弃未使用参数，避免编译器告警。 */
    (void)arg;
}

/**
 * @brief 验证空信号量带 timeout 获取会阻塞当前任务并在 tick 到期后超时唤醒。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_semaphore_take_blocks_and_times_out_current_task();
 */
static void assert_semaphore_take_blocks_and_times_out_current_task(void)
{
    /* 定义高优先级等待任务控制块。 */
    MRT_Task waiter_storage;

    /* 定义低优先级后备任务控制块。 */
    MRT_Task low_storage;

    /* 定义高优先级任务栈。 */
    MRT_StackType waiter_stack[128];

    /* 定义低优先级任务栈。 */
    MRT_StackType low_stack[128];

    /* 定义高优先级任务句柄。 */
    MRT_TaskHandle waiter_task = 0;

    /* 定义低优先级任务句柄。 */
    MRT_TaskHandle low_task = 0;

    /* 定义信号量控制块。 */
    MRT_Semaphore semaphore_storage;

    /* 定义信号量句柄。 */
    MRT_SemaphoreHandle semaphore = 0;

    /* 初始化内核。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 创建低优先级后备任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("low", DummyTask, 0, 1u, low_stack, 128u, &low_storage, &low_task));

    /* 创建高优先级等待任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("waiter",
                                                          DummyTask,
                                                          0,
                                                          5u,
                                                          waiter_stack,
                                                          128u,
                                                          &waiter_storage,
                                                          &waiter_task));

    /* 创建初始计数为 0 的二值信号量。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_SemaphoreCreateBinaryStatic(false, &semaphore_storage, &semaphore));

    /* 启动调度器后应先运行高优先级等待任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelStart());
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == waiter_task);

    /* 空信号量带 3 tick timeout 获取应阻塞当前任务，并在 host 仿真中返回超时结果。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_TIMEOUT, (unsigned)MRT_SemaphoreTake(semaphore, 3u));

    /* 阻塞后低优先级任务应接管运行。 */
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == low_task);

    /* 信号量等待链表应包含该等待任务。 */
    MRT_TEST_ASSERT_EQ_U32(1u, (unsigned)MRT_ListGetCount(&semaphore_storage.waiting_takers));

    /* 推进第 1 个 tick，等待尚未到期。 */
    MRT_KernelTick();
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == low_task);

    /* 推进第 2 个 tick，等待仍未到期。 */
    MRT_KernelTick();
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == low_task);

    /* 推进第 3 个 tick，等待任务应超时醒来并抢占低优先级任务。 */
    MRT_KernelTick();
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == waiter_task);

    /* 等待链表应在 timeout 清理后为空。 */
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)MRT_ListGetCount(&semaphore_storage.waiting_takers));

    /* 信号量计数仍应为 0。 */
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)MRT_SemaphoreGetCount(semaphore));
}

/**
 * @brief 运行信号量 timeout 耦合测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证信号量获取 timeout 与任务调度、tick 唤醒的耦合行为。 */
    assert_semaphore_take_blocks_and_times_out_current_task();

    /* 所有信号量 timeout 耦合测试均通过。 */
    return 0;
}
