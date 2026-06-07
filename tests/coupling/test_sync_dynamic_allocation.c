#include "mrt_test.h"
#include "myrtos/mrt_heap.h"
#include "myrtos/mrt_kernel.h"
#include "myrtos/mrt_mutex.h"
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
 * @brief 初始化一个可合并堆并读取初始空闲空间。
 * @param heap_words 堆存储起始地址，不能为空。
 * @param heap_bytes 堆存储字节数。
 * @param out_free 输出初始空闲空间，不能为空。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * init_heap(heap_words, sizeof(heap_words), &free_before);
 */
static void init_heap(uintptr_t *heap_words, size_t heap_bytes, size_t *out_free)
{
    /* 初始化为合并堆，便于删除对象后完整恢复空闲空间。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_HeapInitialize(heap_words, heap_bytes, MRT_HEAP_MODE_COALESCING));

    /* 读取堆初始化后的空闲空间。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_HeapGetFreeSize(out_free));
}

/**
 * @brief 逐步耗尽可释放堆，直到最小可分配块也无法再申请。
 * @param out_free 输出耗尽后的剩余空闲字节数，不能为 NULL。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * exhaust_heap_for_failure_path(&free_before_fail);
 */
static void exhaust_heap_for_failure_path(size_t *out_free)
{
    /* 循环分配而不是一次申请全部空闲空间，因为可释放堆需要额外块头开销。 */
    for (;;) {
        /* 读取当前剩余空闲空间，用作本轮最大尝试申请量。 */
        size_t free_now = 0u;
        MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_HeapGetFreeSize(&free_now));

        /* 从当前空闲空间开始尝试，失败后逐步降低请求大小。 */
        size_t request = free_now;
        void *block = 0;
        while ((request > 0u) && (block == 0)) {
            /* 尝试申请一块能够占用堆空间的用户载荷。 */
            block = MRT_Malloc(request);

            /* 申请失败时缩小请求，避开块头和对齐导致的额外开销。 */
            if (block == 0) {
                request = request / 2u;
            }
        }

        /* 连 1 字节载荷都无法申请时，堆已无法满足后续动态对象创建。 */
        if (block == 0) {
            /* 输出此时水位，供失败创建前后比较。 */
            MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_HeapGetFreeSize(out_free));
            return;
        }
    }
}

/**
 * @brief 准备一个当前运行任务，供互斥锁 lock/unlock API 使用。
 * @param storage 任务控制块存储，不能为空。
 * @param stack 任务栈存储，不能为空。
 * @param out_task 输出任务句柄，不能为空。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * start_one_task(&storage, stack, &task);
 */
static void start_one_task(MRT_Task *storage, MRT_StackType *stack, MRT_TaskHandle *out_task)
{
    /* 初始化内核，清空旧调度状态。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 创建一个普通测试任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("owner", DummyTask, 0, 2u, stack, 128u, storage, out_task));

    /* 启动调度器，让该任务成为当前任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelStart());
}

/**
 * @brief 验证动态二值信号量创建、使用和删除会归还堆空间。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_dynamic_binary_semaphore_create_take_give_delete();
 */
static void assert_dynamic_binary_semaphore_create_take_give_delete(void)
{
    /* 定义堆存储和空闲空间变量。 */
    uintptr_t heap_words[128u];
    size_t free_before = 0u;

    /* 初始化堆。 */
    init_heap(heap_words, sizeof(heap_words), &free_before);

    /* 动态创建初始可用二值信号量。 */
    MRT_SemaphoreHandle semaphore = 0;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_SemaphoreCreateBinary(true, &semaphore));
    MRT_TEST_ASSERT_TRUE(semaphore != 0);

    /* 创建后堆空闲空间应减少。 */
    size_t free_after_create = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_HeapGetFreeSize(&free_after_create));
    MRT_TEST_ASSERT_TRUE(free_after_create < free_before);

    /* 初始可用二值信号量应能 take 一次。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_SemaphoreTake(semaphore, 0u));
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)MRT_SemaphoreGetCount(semaphore));

    /* give 后计数恢复为 1。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_SemaphoreGive(semaphore));
    MRT_TEST_ASSERT_EQ_U32(1u, (unsigned)MRT_SemaphoreGetCount(semaphore));

    /* 删除动态信号量应归还堆空间。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_SemaphoreDelete(semaphore));

    /* 删除后堆空闲空间恢复。 */
    size_t free_after_delete = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_HeapGetFreeSize(&free_after_delete));
    MRT_TEST_ASSERT_EQ_U32((unsigned)free_before, (unsigned)free_after_delete);
}

/**
 * @brief 验证动态计数信号量参数和删除静态对象保护。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_counting_semaphore_validation_and_static_delete_rejection();
 */
static void assert_counting_semaphore_validation_and_static_delete_rejection(void)
{
    /* 定义堆并初始化。 */
    uintptr_t heap_words[128u];
    size_t free_before = 0u;
    init_heap(heap_words, sizeof(heap_words), &free_before);

    /* 动态计数信号量应保存初始计数。 */
    MRT_SemaphoreHandle semaphore = 0;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_SemaphoreCreateCounting(4u, 2u, &semaphore));
    MRT_TEST_ASSERT_EQ_U32(2u, (unsigned)MRT_SemaphoreGetCount(semaphore));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_SemaphoreDelete(semaphore));

    /* 最大计数为 0 或初始计数超过最大计数应被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_SemaphoreCreateCounting(0u, 0u, &semaphore));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_SemaphoreCreateCounting(2u, 3u, &semaphore));

    /* 静态信号量不能由动态删除 API 释放。 */
    MRT_Semaphore static_storage;
    MRT_SemaphoreHandle static_sem = 0;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_SemaphoreCreateBinaryStatic(false, &static_storage, &static_sem));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OBJECT_BUSY,
                           (unsigned)MRT_SemaphoreDelete(static_sem));
}

/**
 * @brief 验证动态信号量创建失败时清空输出句柄且不改变堆空闲空间。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_dynamic_semaphore_create_failure_keeps_heap_unchanged();
 */
static void assert_dynamic_semaphore_create_failure_keeps_heap_unchanged(void)
{
    /* 定义堆并初始化。 */
    uintptr_t heap_words[64u];
    size_t free_before = 0u;
    init_heap(heap_words, sizeof(heap_words), &free_before);

    /* 耗尽堆，使后续动态信号量创建稳定进入无内存失败路径。 */
    size_t free_before_fail = 0u;
    exhaust_heap_for_failure_path(&free_before_fail);

    /* 堆空间不足时动态创建应失败并清空输出句柄。 */
    MRT_SemaphoreHandle semaphore = (MRT_SemaphoreHandle)heap_words;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_NO_MEMORY,
                           (unsigned)MRT_SemaphoreCreateBinary(false, &semaphore));
    MRT_TEST_ASSERT_TRUE(semaphore == 0);

    /* 失败创建不能改变堆空闲空间。 */
    size_t free_after_fail = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_HeapGetFreeSize(&free_after_fail));
    MRT_TEST_ASSERT_EQ_U32((unsigned)free_before_fail, (unsigned)free_after_fail);

    /* 空输出和空删除参数应被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_SemaphoreCreateBinary(false, 0));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_SemaphoreDelete(0));
}

/**
 * @brief 验证动态互斥锁创建、加锁、解锁和删除会归还堆空间。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_dynamic_mutex_create_lock_unlock_delete();
 */
static void assert_dynamic_mutex_create_lock_unlock_delete(void)
{
    /* 定义任务和堆存储。 */
    MRT_Task task_storage;
    MRT_StackType task_stack[128u];
    MRT_TaskHandle task = 0;
    uintptr_t heap_words[128u];
    size_t free_before = 0u;

    /* 启动一个当前任务并初始化堆。 */
    start_one_task(&task_storage, task_stack, &task);
    init_heap(heap_words, sizeof(heap_words), &free_before);

    /* 动态创建普通互斥锁。 */
    MRT_MutexHandle mutex = 0;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MutexCreate(&mutex));
    MRT_TEST_ASSERT_TRUE(mutex != 0);

    /* 当前任务应可加锁和解锁。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MutexLock(mutex, 0u));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MutexUnlock(mutex));

    /* 删除动态互斥锁应归还堆空间。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MutexDelete(mutex));
    size_t free_after_delete = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_HeapGetFreeSize(&free_after_delete));
    MRT_TEST_ASSERT_EQ_U32((unsigned)free_before, (unsigned)free_after_delete);
}

/**
 * @brief 验证动态递归互斥锁和删除忙状态保护。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_dynamic_recursive_mutex_and_busy_delete();
 */
static void assert_dynamic_recursive_mutex_and_busy_delete(void)
{
    /* 定义任务和堆存储。 */
    MRT_Task task_storage;
    MRT_StackType task_stack[128u];
    MRT_TaskHandle task = 0;
    uintptr_t heap_words[128u];
    size_t free_before = 0u;

    /* 启动一个当前任务并初始化堆。 */
    start_one_task(&task_storage, task_stack, &task);
    init_heap(heap_words, sizeof(heap_words), &free_before);

    /* 动态创建递归互斥锁。 */
    MRT_MutexHandle mutex = 0;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MutexCreateRecursive(&mutex));

    /* 同一任务可递归加锁两次。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MutexLock(mutex, 0u));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MutexLock(mutex, 0u));

    /* 持锁删除应返回对象忙。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OBJECT_BUSY, (unsigned)MRT_MutexDelete(mutex));

    /* 完整解锁后允许删除。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MutexUnlock(mutex));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MutexUnlock(mutex));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MutexDelete(mutex));

    /* 静态互斥锁不能由动态删除 API 释放。 */
    MRT_Mutex static_storage;
    MRT_MutexHandle static_mutex = 0;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_MutexCreateStatic(&static_storage, &static_mutex));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OBJECT_BUSY, (unsigned)MRT_MutexDelete(static_mutex));
}

/**
 * @brief 验证动态互斥锁分配失败和空参数保护。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_dynamic_mutex_failure_and_invalid_arguments();
 */
static void assert_dynamic_mutex_failure_and_invalid_arguments(void)
{
    /* 定义堆并初始化。 */
    uintptr_t heap_words[64u];
    size_t free_before = 0u;
    init_heap(heap_words, sizeof(heap_words), &free_before);

    /* 耗尽堆，使后续动态互斥锁创建稳定进入无内存失败路径。 */
    size_t free_before_fail = 0u;
    exhaust_heap_for_failure_path(&free_before_fail);

    /* 创建失败时清空输出句柄。 */
    MRT_MutexHandle mutex = (MRT_MutexHandle)heap_words;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_NO_MEMORY, (unsigned)MRT_MutexCreate(&mutex));
    MRT_TEST_ASSERT_TRUE(mutex == 0);

    /* 失败创建不能改变堆空闲空间。 */
    size_t free_after_fail = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_HeapGetFreeSize(&free_after_fail));
    MRT_TEST_ASSERT_EQ_U32((unsigned)free_before_fail, (unsigned)free_after_fail);

    /* 空参数应被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT, (unsigned)MRT_MutexCreate(0));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT, (unsigned)MRT_MutexCreateRecursive(0));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT, (unsigned)MRT_MutexDelete(0));
}

/**
 * @brief 运行动态同步对象分配测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证动态二值信号量。 */
    assert_dynamic_binary_semaphore_create_take_give_delete();

    /* 验证计数信号量参数和静态删除保护。 */
    assert_counting_semaphore_validation_and_static_delete_rejection();

    /* 验证动态信号量失败路径。 */
    assert_dynamic_semaphore_create_failure_keeps_heap_unchanged();

    /* 验证动态普通互斥锁。 */
    assert_dynamic_mutex_create_lock_unlock_delete();

    /* 验证动态递归互斥锁。 */
    assert_dynamic_recursive_mutex_and_busy_delete();

    /* 验证动态互斥锁失败路径和参数保护。 */
    assert_dynamic_mutex_failure_and_invalid_arguments();

    /* 所有动态同步对象测试均通过。 */
    return 0;
}
