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
 * @brief 验证静态互斥锁创建后没有拥有者。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_mutex_static_creation_starts_unowned();
 */
static void assert_mutex_static_creation_starts_unowned(void)
{
    /* 定义互斥锁控制块。 */
    MRT_Mutex storage;

    /* 定义互斥锁句柄。 */
    MRT_MutexHandle mutex = 0;

    /* 创建普通互斥锁。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MutexCreateStatic(&storage, &mutex));

    /* 定义拥有者输出。 */
    MRT_TaskHandle owner = (MRT_TaskHandle)1;

    /* 查询拥有者。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MutexGetOwner(mutex, &owner));

    /* 新互斥锁没有拥有者。 */
    MRT_TEST_ASSERT_TRUE(owner == 0);
}

/**
 * @brief 验证当前任务可以加锁和解锁普通互斥锁。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_current_task_can_lock_and_unlock_mutex();
 */
static void assert_current_task_can_lock_and_unlock_mutex(void)
{
    /* 定义任务控制块。 */
    MRT_Task task_storage;

    /* 定义任务栈。 */
    MRT_StackType task_stack[128];

    /* 定义任务句柄。 */
    MRT_TaskHandle task = 0;

    /* 定义互斥锁控制块。 */
    MRT_Mutex mutex_storage;

    /* 定义互斥锁句柄。 */
    MRT_MutexHandle mutex = 0;

    /* 初始化内核。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 创建测试任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("task", DummyTask, 0, 3u, task_stack, 128u, &task_storage, &task));

    /* 创建普通互斥锁。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MutexCreateStatic(&mutex_storage, &mutex));

    /* 启动调度器后当前任务就是测试任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelStart());
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == task);

    /* 当前任务加锁应成功。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MutexLock(mutex, 0u));

    /* 查询拥有者应为当前任务。 */
    MRT_TaskHandle owner = 0;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MutexGetOwner(mutex, &owner));
    MRT_TEST_ASSERT_TRUE(owner == task);

    /* 当前任务解锁应成功。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MutexUnlock(mutex));

    /* 解锁后拥有者应为空。 */
    owner = task;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MutexGetOwner(mutex, &owner));
    MRT_TEST_ASSERT_TRUE(owner == 0);
}

/**
 * @brief 验证非拥有者不能解锁互斥锁。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_non_owner_unlock_returns_owner_error();
 */
static void assert_non_owner_unlock_returns_owner_error(void)
{
    /* 定义高优先级任务控制块。 */
    MRT_Task high_storage;

    /* 定义低优先级任务控制块。 */
    MRT_Task low_storage;

    /* 定义高优先级任务栈。 */
    MRT_StackType high_stack[128];

    /* 定义低优先级任务栈。 */
    MRT_StackType low_stack[128];

    /* 定义高优先级任务句柄。 */
    MRT_TaskHandle high_task = 0;

    /* 定义低优先级任务句柄。 */
    MRT_TaskHandle low_task = 0;

    /* 定义互斥锁控制块。 */
    MRT_Mutex mutex_storage;

    /* 定义互斥锁句柄。 */
    MRT_MutexHandle mutex = 0;

    /* 初始化内核。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 创建低优先级任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("low", DummyTask, 0, 1u, low_stack, 128u, &low_storage, &low_task));

    /* 创建高优先级任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("high", DummyTask, 0, 5u, high_stack, 128u, &high_storage, &high_task));

    /* 创建普通互斥锁。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MutexCreateStatic(&mutex_storage, &mutex));

    /* 启动后高优先级任务运行。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelStart());
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == high_task);

    /* 高优先级任务加锁。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MutexLock(mutex, 0u));

    /* 高优先级任务延时，让低优先级任务成为当前任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskDelay(3u));
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == low_task);

    /* 低优先级非拥有者解锁应返回所有权错误。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OWNER_ERROR, (unsigned)MRT_MutexUnlock(mutex));

    /* 拥有者仍应保持为高优先级任务。 */
    MRT_TaskHandle owner = 0;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MutexGetOwner(mutex, &owner));
    MRT_TEST_ASSERT_TRUE(owner == high_task);
}

/**
 * @brief 验证互斥锁 API 参数和上下文校验。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_mutex_rejects_invalid_arguments_and_context();
 */
static void assert_mutex_rejects_invalid_arguments_and_context(void)
{
    /* 重新初始化内核，清空前置测试留下的当前任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 定义互斥锁控制块。 */
    MRT_Mutex storage;

    /* 定义互斥锁句柄。 */
    MRT_MutexHandle mutex = 0;

    /* 创建时控制块不能为空。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT, (unsigned)MRT_MutexCreateStatic(0, &mutex));

    /* 创建时输出句柄不能为空。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT, (unsigned)MRT_MutexCreateStatic(&storage, 0));

    /* 正常创建供后续上下文校验使用。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MutexCreateStatic(&storage, &mutex));

    /* 没有当前任务时不能加锁互斥锁。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_CONTEXT, (unsigned)MRT_MutexLock(mutex, 0u));

    /* 没有当前任务时不能解锁互斥锁。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_CONTEXT, (unsigned)MRT_MutexUnlock(mutex));

    /* 查询拥有者时互斥锁句柄不能为空。 */
    MRT_TaskHandle owner = 0;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT, (unsigned)MRT_MutexGetOwner(0, &owner));

    /* 查询拥有者时输出指针不能为空。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT, (unsigned)MRT_MutexGetOwner(mutex, 0));
}

/**
 * @brief 运行互斥锁创建、加锁、解锁测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证静态创建后无拥有者。 */
    assert_mutex_static_creation_starts_unowned();

    /* 验证当前任务加锁和解锁。 */
    assert_current_task_can_lock_and_unlock_mutex();

    /* 验证非拥有者解锁失败。 */
    assert_non_owner_unlock_returns_owner_error();

    /* 验证参数和上下文校验。 */
    assert_mutex_rejects_invalid_arguments_and_context();

    /* 所有互斥锁创建和所有权测试均通过。 */
    return 0;
}
