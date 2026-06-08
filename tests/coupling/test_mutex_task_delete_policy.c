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
 * @brief 验证持有互斥锁的任务不能被删除。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_owner_task_delete_is_rejected_while_mutex_is_locked();
 */
static void assert_owner_task_delete_is_rejected_while_mutex_is_locked(void)
{
    /* 定义持锁任务控制块。 */
    MRT_Task owner_storage;

    /* 定义等待锁任务控制块。 */
    MRT_Task waiter_storage;

    /* 定义持锁任务栈。 */
    MRT_StackType owner_stack[128];

    /* 定义等待锁任务栈。 */
    MRT_StackType waiter_stack[128];

    /* 定义持锁任务句柄。 */
    MRT_TaskHandle owner_task = 0;

    /* 定义等待锁任务句柄。 */
    MRT_TaskHandle waiter_task = 0;

    /* 定义互斥锁控制块。 */
    MRT_Mutex mutex_storage;

    /* 定义互斥锁句柄。 */
    MRT_MutexHandle mutex = 0;

    /* 初始化内核。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 创建低优先级持锁任务。 */
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

    /* 启动调度器，使持锁任务先运行。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelStart());
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == owner_task);

    /* 持锁任务获得互斥锁。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MutexLock(mutex, 0u));

    /* 运行中创建高优先级等待任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("waiter",
                                                          DummyTask,
                                                          0,
                                                          5u,
                                                          waiter_stack,
                                                          128u,
                                                          &waiter_storage,
                                                          &waiter_task));

    /* 切换到高优先级等待任务。 */
    MRT_KernelYield();
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == waiter_task);

    /* 高优先级任务等待互斥锁，触发持锁任务继承优先级。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_TIMEOUT, (unsigned)MRT_MutexLock(mutex, 20u));
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == owner_task);

    /* 持锁任务删除请求必须被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OBJECT_BUSY, (unsigned)MRT_TaskDelete(owner_task));

    /* 当前任务仍应是持锁任务。 */
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == owner_task);

    /* 持锁任务状态仍应为 running。 */
    MRT_TaskState owner_state = MRT_TASK_STATE_DELETED;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskGetState(owner_task, &owner_state));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_TASK_STATE_RUNNING, (unsigned)owner_state);

    /* 互斥锁拥有者仍应是原持锁任务。 */
    MRT_TaskHandle owner = 0;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MutexGetOwner(mutex, &owner));
    MRT_TEST_ASSERT_TRUE(owner == owner_task);

    /* 等待链表仍应保留正在等待锁的任务。 */
    MRT_TEST_ASSERT_EQ_U32(1u, (unsigned)MRT_ListGetCount(&mutex_storage.waiting_lockers));
}

/**
 * @brief 运行互斥锁持有者任务删除策略测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证持锁任务删除被拒绝，避免互斥锁进入无拥有者但仍上锁的状态。 */
    assert_owner_task_delete_is_rejected_while_mutex_is_locked();

    /* 所有互斥锁任务删除策略测试均通过。 */
    return 0;
}
