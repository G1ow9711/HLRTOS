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
 * @brief 验证 give 会唤醒等待获取信号量的高优先级任务。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_semaphore_give_wakes_high_priority_taker();
 */
static void assert_semaphore_give_wakes_high_priority_taker(void)
{
    /* 定义高优先级等待任务控制块。 */
    MRT_Task waiter_storage;

    /* 定义低优先级释放任务控制块。 */
    MRT_Task giver_storage;

    /* 定义高优先级任务栈。 */
    MRT_StackType waiter_stack[128];

    /* 定义低优先级任务栈。 */
    MRT_StackType giver_stack[128];

    /* 定义高优先级任务句柄。 */
    MRT_TaskHandle waiter_task = 0;

    /* 定义低优先级任务句柄。 */
    MRT_TaskHandle giver_task = 0;

    /* 定义信号量控制块。 */
    MRT_Semaphore semaphore_storage;

    /* 定义信号量句柄。 */
    MRT_SemaphoreHandle semaphore = 0;

    /* 初始化内核。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 创建低优先级释放任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("giver",
                                                          DummyTask,
                                                          0,
                                                          1u,
                                                          giver_stack,
                                                          128u,
                                                          &giver_storage,
                                                          &giver_task));

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

    /* 高优先级任务等待空信号量。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_TIMEOUT, (unsigned)MRT_SemaphoreTake(semaphore, 20u));

    /* 阻塞后低优先级释放任务应成为当前任务。 */
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == giver_task);

    /* 等待链表应包含高优先级任务。 */
    MRT_TEST_ASSERT_EQ_U32(1u, (unsigned)MRT_ListGetCount(&semaphore_storage.waiting_takers));

    /* 低优先级任务释放信号量，应直接唤醒等待者。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_SemaphoreGive(semaphore));

    /* 高优先级等待任务应抢占成为当前任务。 */
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == waiter_task);

    /* 等待链表应被清空。 */
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)MRT_ListGetCount(&semaphore_storage.waiting_takers));

    /* 释放给等待者的计数应被等待者消费，不应留存在信号量计数中。 */
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)MRT_SemaphoreGetCount(semaphore));
}

/**
 * @brief 运行信号量 give 唤醒任务耦合测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证信号量 give 唤醒高优先级等待任务。 */
    assert_semaphore_give_wakes_high_priority_taker();

    /* 所有信号量 give 唤醒测试均通过。 */
    return 0;
}
