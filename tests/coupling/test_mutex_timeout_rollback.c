#include "mrt_test.h"
#include "myrtos/mrt_kernel.h"
#include "myrtos/mrt_mutex.h"
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
 * @brief 验证互斥锁等待超时后，原拥有者的优先级会回落到基础值。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_mutex_timeout_restores_owner_base_priority();
 */
static void assert_mutex_timeout_restores_owner_base_priority(void)
{
    /* 定义低优先级拥有者任务控制块。 */
    MRT_Task owner_storage;

    /* 定义高优先级等待者任务控制块。 */
    MRT_Task waiter_storage;

    /* 定义低优先级任务栈。 */
    MRT_StackType owner_stack[128];

    /* 定义高优先级任务栈。 */
    MRT_StackType waiter_stack[128];

    /* 定义低优先级任务句柄。 */
    MRT_TaskHandle owner_task = 0;

    /* 定义高优先级任务句柄。 */
    MRT_TaskHandle waiter_task = 0;

    /* 定义互斥锁控制块。 */
    MRT_Mutex mutex_storage;

    /* 定义互斥锁句柄。 */
    MRT_MutexHandle mutex = 0;

    /* 初始化内核。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 创建低优先级拥有者任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("owner",
                                                          DummyTask,
                                                          0,
                                                          1u,
                                                          owner_stack,
                                                          128u,
                                                          &owner_storage,
                                                          &owner_task));

    /* 创建互斥锁。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MutexCreateStatic(&mutex_storage, &mutex));

    /* 启动调度器，低优先级任务先运行。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelStart());
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == owner_task);

    /* 低优先级任务先持有互斥锁。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MutexLock(mutex, 0u));

    /* 运行中创建高优先级等待者任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("waiter",
                                                          DummyTask,
                                                          0,
                                                          5u,
                                                          waiter_stack,
                                                          128u,
                                                          &waiter_storage,
                                                          &waiter_task));

    /* 切换到高优先级任务。 */
    MRT_KernelYield();
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == waiter_task);

    /* 高优先级任务等待互斥锁 3 tick。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_TIMEOUT, (unsigned)MRT_MutexLock(mutex, 3u));

    /* 等待者阻塞后应回到低优先级拥有者继续运行。 */
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == owner_task);

    /* 低优先级拥有者应发生优先级继承。 */
    MRT_Priority owner_priority = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskGetPriority(owner_task, &owner_priority));
    MRT_TEST_ASSERT_EQ_U32(5u, (unsigned)owner_priority);

    /* 推进 1 tick，等待尚未到期。 */
    MRT_KernelTick();
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskGetPriority(owner_task, &owner_priority));
    MRT_TEST_ASSERT_EQ_U32(5u, (unsigned)owner_priority);

    /* 推进 2 tick，等待仍未到期。 */
    MRT_KernelTick();
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskGetPriority(owner_task, &owner_priority));
    MRT_TEST_ASSERT_EQ_U32(5u, (unsigned)owner_priority);

    /* 推进 3 tick，等待者超时退出后，拥有者优先级应回落到基础值。 */
    MRT_KernelTick();

    /* 当前任务选择可受同优先级队列顺序影响，但拥有者基础优先级必须已经恢复。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskGetPriority(owner_task, &owner_priority));
    MRT_TEST_ASSERT_EQ_U32(1u, (unsigned)owner_priority);

    /* 互斥锁仍归低优先级拥有者持有。 */
    MRT_TaskHandle owner = 0;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MutexGetOwner(mutex, &owner));
    MRT_TEST_ASSERT_TRUE(owner == owner_task);
}

/**
 * @brief 运行互斥锁超时回滚测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证互斥锁等待超时后拥有者优先级回滚。 */
    assert_mutex_timeout_restores_owner_base_priority();

    /* 所有互斥锁 timeout 回滚测试均通过。 */
    return 0;
}
