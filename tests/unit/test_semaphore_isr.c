#include "mrt_test.h"
#include "myrtos/mrt_kernel.h"
#include "myrtos/mrt_port.h"
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
 * @brief 验证 ISR give 在没有等待任务时增加计数且不请求切换。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_isr_give_without_waiter_increments_count();
 */
static void assert_isr_give_without_waiter_increments_count(void)
{
    /* 初始化端口 mock。 */
    MRT_PortInitialize();

    /* 切换到模拟 ISR 上下文。 */
    MRT_PortMockSetInsideISR(true);

    /* 定义信号量控制块。 */
    MRT_Semaphore storage;

    /* 定义信号量句柄。 */
    MRT_SemaphoreHandle semaphore = 0;

    /* 创建初始计数为 0 的二值信号量。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_SemaphoreCreateBinaryStatic(false, &storage, &semaphore));

    /* 预置 yield 标志为 true，用于确认 API 会写回 false。 */
    bool should_yield = true;

    /* ISR give 应成功。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_SemaphoreGiveFromISR(semaphore, &should_yield));

    /* 没有等待任务时不需要切换。 */
    MRT_TEST_ASSERT_TRUE(!should_yield);

    /* 当前计数应增加到 1。 */
    MRT_TEST_ASSERT_EQ_U32(1u, (unsigned)MRT_SemaphoreGetCount(semaphore));
}

/**
 * @brief 验证 ISR give 满信号量返回满状态。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_isr_give_full_semaphore_returns_full();
 */
static void assert_isr_give_full_semaphore_returns_full(void)
{
    /* 初始化端口 mock。 */
    MRT_PortInitialize();

    /* 切换到模拟 ISR 上下文。 */
    MRT_PortMockSetInsideISR(true);

    /* 定义信号量控制块。 */
    MRT_Semaphore storage;

    /* 定义信号量句柄。 */
    MRT_SemaphoreHandle semaphore = 0;

    /* 创建初始计数为 1 的满二值信号量。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_SemaphoreCreateBinaryStatic(true, &storage, &semaphore));

    /* 预置 yield 标志为 true。 */
    bool should_yield = true;

    /* 满信号量 ISR give 应返回对象已满。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OBJECT_FULL,
                           (unsigned)MRT_SemaphoreGiveFromISR(semaphore, &should_yield));

    /* 失败路径不应请求切换。 */
    MRT_TEST_ASSERT_TRUE(!should_yield);
}

/**
 * @brief 验证 ISR give 在任务上下文调用时返回非法上下文。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_isr_give_rejects_task_context();
 */
static void assert_isr_give_rejects_task_context(void)
{
    /* 初始化端口 mock，默认是任务上下文。 */
    MRT_PortInitialize();

    /* 显式保持任务上下文。 */
    MRT_PortMockSetInsideISR(false);

    /* 定义信号量控制块。 */
    MRT_Semaphore storage;

    /* 定义信号量句柄。 */
    MRT_SemaphoreHandle semaphore = 0;

    /* 创建空二值信号量。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_SemaphoreCreateBinaryStatic(false, &storage, &semaphore));

    /* 预置 yield 标志为 true。 */
    bool should_yield = true;

    /* 任务上下文调用 FromISR API 应被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_CONTEXT,
                           (unsigned)MRT_SemaphoreGiveFromISR(semaphore, &should_yield));

    /* 非法上下文不应请求切换。 */
    MRT_TEST_ASSERT_TRUE(!should_yield);
}

/**
 * @brief 验证 ISR give 唤醒等待任务时设置 should_yield。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_isr_give_wakes_waiter_and_sets_yield();
 */
static void assert_isr_give_wakes_waiter_and_sets_yield(void)
{
    /* 定义高优先级等待任务控制块。 */
    MRT_Task waiter_storage;

    /* 定义低优先级当前任务控制块。 */
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

    /* 创建低优先级任务。 */
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

    /* 创建空二值信号量。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_SemaphoreCreateBinaryStatic(false, &semaphore_storage, &semaphore));

    /* 启动后高优先级任务运行。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelStart());
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == waiter_task);

    /* 高优先级任务等待空信号量。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_TIMEOUT, (unsigned)MRT_SemaphoreTake(semaphore, 20u));

    /* 低优先级任务成为当前任务。 */
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == low_task);

    /* 切换到模拟 ISR 上下文。 */
    MRT_PortMockSetInsideISR(true);

    /* 定义 yield 输出标志。 */
    bool should_yield = false;

    /* ISR give 应唤醒高优先级等待任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_SemaphoreGiveFromISR(semaphore, &should_yield));

    /* ISR 中唤醒任务应请求退出 ISR 后切换。 */
    MRT_TEST_ASSERT_TRUE(should_yield);

    /* ISR API 不应立即切换当前任务。 */
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == low_task);

    /* 等待任务应回到 ready 状态。 */
    MRT_TaskState waiter_state = MRT_TASK_STATE_DELETED;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskGetState(waiter_task, &waiter_state));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_TASK_STATE_READY, (unsigned)waiter_state);

    /* 信号量计数应保持 0，因为令牌直接转交给等待者。 */
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)MRT_SemaphoreGetCount(semaphore));

    /* 恢复任务上下文，避免影响后续测试。 */
    MRT_PortMockSetInsideISR(false);
}

/**
 * @brief 运行信号量 ISR give 测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证无等待者 ISR give。 */
    assert_isr_give_without_waiter_increments_count();

    /* 验证满信号量 ISR give。 */
    assert_isr_give_full_semaphore_returns_full();

    /* 验证非法上下文。 */
    assert_isr_give_rejects_task_context();

    /* 验证 ISR give 唤醒等待者。 */
    assert_isr_give_wakes_waiter_and_sets_yield();

    /* 所有信号量 ISR give 测试均通过。 */
    return 0;
}
