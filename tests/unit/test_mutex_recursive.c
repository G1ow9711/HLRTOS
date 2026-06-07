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
 * @brief 创建并启动一个测试任务。
 * @param storage 任务控制块存储，不能为空。
 * @param stack 任务栈存储，不能为空。
 * @param out_task 输出任务句柄，不能为空。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * create_and_start_task(&task_storage, stack, &task);
 */
static void create_and_start_task(MRT_Task *storage, MRT_StackType *stack, MRT_TaskHandle *out_task)
{
    /* 初始化内核。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 创建测试任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("task", DummyTask, 0, 3u, stack, 128u, storage, out_task));

    /* 启动调度器。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelStart());

    /* 当前任务应为测试任务。 */
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == *out_task);
}

/**
 * @brief 验证递归互斥锁允许同一任务重复加锁并逐层解锁。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_recursive_mutex_allows_same_owner_relock();
 */
static void assert_recursive_mutex_allows_same_owner_relock(void)
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

    /* 创建并启动测试任务。 */
    create_and_start_task(&task_storage, task_stack, &task);

    /* 创建递归互斥锁。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MutexCreateRecursiveStatic(&mutex_storage, &mutex));

    /* 第一次加锁应成功。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MutexLock(mutex, 0u));

    /* 同一任务第二次递归加锁也应成功。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MutexLock(mutex, 0u));

    /* 第一次解锁只减少递归深度，拥有者仍是当前任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MutexUnlock(mutex));

    /* 查询拥有者应仍为当前任务。 */
    MRT_TaskHandle owner = 0;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MutexGetOwner(mutex, &owner));
    MRT_TEST_ASSERT_TRUE(owner == task);

    /* 第二次解锁释放最后一层。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MutexUnlock(mutex));

    /* 递归深度归零后拥有者应为空。 */
    owner = task;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MutexGetOwner(mutex, &owner));
    MRT_TEST_ASSERT_TRUE(owner == 0);
}

/**
 * @brief 验证普通互斥锁拒绝同一任务重复加锁。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_plain_mutex_relock_returns_busy();
 */
static void assert_plain_mutex_relock_returns_busy(void)
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

    /* 创建并启动测试任务。 */
    create_and_start_task(&task_storage, task_stack, &task);

    /* 创建普通互斥锁。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MutexCreateStatic(&mutex_storage, &mutex));

    /* 第一次加锁应成功。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MutexLock(mutex, 0u));

    /* 普通互斥锁同一任务重复加锁应返回忙。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OBJECT_BUSY, (unsigned)MRT_MutexLock(mutex, 0u));
}

/**
 * @brief 验证递归互斥锁拒绝非拥有者释放且保持递归深度不变。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_recursive_mutex_rejects_non_owner_unlock();
 */
static void assert_recursive_mutex_rejects_non_owner_unlock(void)
{
    /* 定义低优先级拥有者任务控制块。 */
    MRT_Task low_storage;

    /* 定义高优先级非拥有者任务控制块。 */
    MRT_Task high_storage;

    /* 定义低优先级任务栈。 */
    MRT_StackType low_stack[128];

    /* 定义高优先级任务栈。 */
    MRT_StackType high_stack[128];

    /* 定义低优先级任务句柄。 */
    MRT_TaskHandle low_task = 0;

    /* 定义高优先级任务句柄。 */
    MRT_TaskHandle high_task = 0;

    /* 定义递归互斥锁控制块。 */
    MRT_Mutex mutex_storage;

    /* 定义递归互斥锁句柄。 */
    MRT_MutexHandle mutex = 0;

    /* 初始化内核。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 创建低优先级任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("low", DummyTask, 0, 1u, low_stack, 128u, &low_storage, &low_task));

    /* 创建递归互斥锁。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MutexCreateRecursiveStatic(&mutex_storage, &mutex));

    /* 启动调度器，让低优先级任务成为当前任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelStart());
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == low_task);

    /* 低优先级任务第一次获得递归互斥锁。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MutexLock(mutex, 0u));

    /* 低优先级任务第二次递归获得同一把锁。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MutexLock(mutex, 0u));

    /* 运行中创建高优先级任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("high", DummyTask, 0, 5u, high_stack, 128u, &high_storage, &high_task));

    /* 主动 yield，让高优先级非拥有者成为当前任务。 */
    MRT_KernelYield();
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == high_task);

    /* 非拥有者释放递归互斥锁应返回所有权错误。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OWNER_ERROR, (unsigned)MRT_MutexUnlock(mutex));

    /* 查询拥有者，应仍然是低优先级任务。 */
    MRT_TaskHandle owner = 0;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MutexGetOwner(mutex, &owner));
    MRT_TEST_ASSERT_TRUE(owner == low_task);

    /* 递归深度应保持 2，不被错误释放路径修改。 */
    MRT_TEST_ASSERT_EQ_U32(2u, (unsigned)mutex_storage.lock_count);
}

/**
 * @brief 验证递归互斥锁创建参数校验。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_recursive_mutex_create_rejects_invalid_arguments();
 */
static void assert_recursive_mutex_create_rejects_invalid_arguments(void)
{
    /* 定义互斥锁控制块。 */
    MRT_Mutex storage;

    /* 定义互斥锁句柄。 */
    MRT_MutexHandle mutex = 0;

    /* 控制块不能为空。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_MutexCreateRecursiveStatic(0, &mutex));

    /* 输出句柄不能为空。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_MutexCreateRecursiveStatic(&storage, 0));
}

/**
 * @brief 运行递归互斥锁测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证递归互斥锁重入和逐层释放。 */
    assert_recursive_mutex_allows_same_owner_relock();

    /* 验证普通互斥锁拒绝重入。 */
    assert_plain_mutex_relock_returns_busy();

    /* 验证递归互斥锁拒绝非拥有者释放且不改变状态。 */
    assert_recursive_mutex_rejects_non_owner_unlock();

    /* 验证递归互斥锁创建参数校验。 */
    assert_recursive_mutex_create_rejects_invalid_arguments();

    /* 所有递归互斥锁测试均通过。 */
    return 0;
}
