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
 * @brief 验证高优先级任务等待互斥锁时触发拥有者优先级继承。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_mutex_waiter_boosts_owner_and_unlock_transfers_ownership();
 */
static void assert_mutex_waiter_boosts_owner_and_unlock_transfers_ownership(void)
{
    /* 定义低优先级拥有者任务控制块。 */
    MRT_Task low_storage;

    /* 定义高优先级等待者任务控制块。 */
    MRT_Task high_storage;

    /* 定义低优先级任务栈。 */
    MRT_StackType low_stack[128];

    /* 定义高优先级任务栈。 */
    MRT_StackType high_stack[128];

    /* 定义低优先级任务句柄。 */
    MRT_TaskHandle low_task = 0;

    /* 定义高优先级任务句柄。 */
    MRT_TaskHandle high_task = 0;

    /* 定义互斥锁控制块。 */
    MRT_Mutex mutex_storage;

    /* 定义互斥锁句柄。 */
    MRT_MutexHandle mutex = 0;

    /* 初始化内核。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 创建低优先级任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("low", DummyTask, 0, 1u, low_stack, 128u, &low_storage, &low_task));

    /* 创建互斥锁。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MutexCreateStatic(&mutex_storage, &mutex));

    /* 启动调度器，此时只有低优先级任务可运行。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelStart());
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == low_task);

    /* 低优先级任务获得互斥锁。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MutexLock(mutex, 0u));

    /* 运行中创建高优先级任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("high", DummyTask, 0, 5u, high_stack, 128u, &high_storage, &high_task));

    /* 主动 yield 后高优先级任务应成为当前任务。 */
    MRT_KernelYield();
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == high_task);

    /* 高优先级任务等待低优先级任务持有的互斥锁。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_TIMEOUT, (unsigned)MRT_MutexLock(mutex, 20u));

    /* 高优先级任务阻塞后，低优先级拥有者应重新运行。 */
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == low_task);

    /* 低优先级拥有者应继承高优先级等待者的优先级。 */
    MRT_Priority low_priority = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskGetPriority(low_task, &low_priority));
    MRT_TEST_ASSERT_EQ_U32(5u, (unsigned)low_priority);

    /* 等待锁链表应包含高优先级任务。 */
    MRT_TEST_ASSERT_EQ_U32(1u, (unsigned)MRT_ListGetCount(&mutex_storage.waiting_lockers));

    /* 低优先级任务解锁，应把锁转交给高优先级等待者。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MutexUnlock(mutex));

    /* 解锁后高优先级任务应被唤醒并抢占。 */
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == high_task);

    /* 低优先级任务应恢复基础优先级。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskGetPriority(low_task, &low_priority));
    MRT_TEST_ASSERT_EQ_U32(1u, (unsigned)low_priority);

    /* 互斥锁拥有者应转为高优先级任务。 */
    MRT_TaskHandle owner = 0;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MutexGetOwner(mutex, &owner));
    MRT_TEST_ASSERT_TRUE(owner == high_task);

    /* 等待链表应清空。 */
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)MRT_ListGetCount(&mutex_storage.waiting_lockers));
}

/**
 * @brief 运行互斥锁优先级继承测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证互斥锁优先级继承和所有权转移。 */
    assert_mutex_waiter_boosts_owner_and_unlock_transfers_ownership();

    /* 所有互斥锁优先级继承测试均通过。 */
    return 0;
}
