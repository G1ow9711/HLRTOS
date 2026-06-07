#include "mrt_test.h"
#include "myrtos/mrt_event_group.h"
#include "myrtos/mrt_heap.h"
#include "myrtos/mrt_kernel.h"
#include "myrtos/mrt_task.h"
#include "myrtos/mrt_timer.h"

/** @brief 记录定时器回调次数。 */
static unsigned g_timer_callback_count;

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
 * @brief 测试用定时器回调函数。
 * @param timer 到期定时器，本测试不使用。
 * @param arg 用户参数，本测试不使用。
 * @return void 无返回值。
 * @example
 * TimerCallback(timer, NULL);
 */
static void TimerCallback(MRT_TimerHandle timer, void *arg)
{
    /* 显式丢弃定时器句柄。 */
    (void)timer;

    /* 显式丢弃用户参数。 */
    (void)arg;

    /* 记录回调被调用。 */
    g_timer_callback_count++;
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
    /* 初始化为合并堆，便于删除对象后检查完整回收。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_HeapInitialize(heap_words, heap_bytes, MRT_HEAP_MODE_COALESCING));

    /* 读取初始空闲空间。 */
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
 * @brief 验证动态事件组创建、置位、查询和删除会归还堆空间。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_dynamic_event_group_create_set_delete();
 */
static void assert_dynamic_event_group_create_set_delete(void)
{
    /* 定义堆并初始化。 */
    uintptr_t heap_words[128u];
    size_t free_before = 0u;
    init_heap(heap_words, sizeof(heap_words), &free_before);

    /* 动态创建事件组。 */
    MRT_EventGroupHandle group = 0;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_EventGroupCreate(&group));
    MRT_TEST_ASSERT_TRUE(group != 0);

    /* 设置 bit 并验证可查询。 */
    MRT_EventBits bits = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_EventGroupSetBits(group, 0x03u, &bits));
    MRT_TEST_ASSERT_EQ_U32(0x03u, (unsigned)bits);
    MRT_TEST_ASSERT_EQ_U32(0x03u, (unsigned)MRT_EventGroupGetBits(group));

    /* 删除动态事件组应归还堆空间。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_EventGroupDelete(group));
    size_t free_after_delete = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_HeapGetFreeSize(&free_after_delete));
    MRT_TEST_ASSERT_EQ_U32((unsigned)free_before, (unsigned)free_after_delete);
}

/**
 * @brief 验证事件组删除会拒绝静态对象和存在等待者的对象。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_event_group_delete_rejects_static_and_waiters();
 */
static void assert_event_group_delete_rejects_static_and_waiters(void)
{
    /* 定义任务、事件组和堆存储。 */
    MRT_Task high_storage;
    MRT_Task low_storage;
    MRT_StackType high_stack[128u];
    MRT_StackType low_stack[128u];
    MRT_TaskHandle high_task = 0;
    MRT_TaskHandle low_task = 0;
    uintptr_t heap_words[128u];
    size_t free_before = 0u;

    /* 初始化内核并创建两个任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("low", DummyTask, 0, 1u, low_stack, 128u, &low_storage, &low_task));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("high", DummyTask, 0, 5u, high_stack, 128u, &high_storage, &high_task));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelStart());

    /* 初始化堆并创建动态事件组。 */
    init_heap(heap_words, sizeof(heap_words), &free_before);
    MRT_EventGroupHandle group = 0;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_EventGroupCreate(&group));

    /* 高优先级任务等待事件，制造等待者。 */
    MRT_EventBits bits = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_TIMEOUT,
                           (unsigned)MRT_EventGroupWaitBits(group, 0x01u, false, false, 10u, &bits));

    /* 存在等待者时不能删除事件组。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OBJECT_BUSY, (unsigned)MRT_EventGroupDelete(group));

    /* 设置 bit 唤醒等待者后才能删除。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_EventGroupSetBits(group, 0x01u, &bits));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_EventGroupDelete(group));

    /* 静态事件组不能由动态删除 API 释放。 */
    MRT_EventGroup static_storage;
    MRT_EventGroupHandle static_group = 0;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_EventGroupCreateStatic(&static_storage, &static_group));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OBJECT_BUSY, (unsigned)MRT_EventGroupDelete(static_group));
}

/**
 * @brief 验证动态事件组创建失败和空参数保护。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_event_group_failure_and_invalid_arguments();
 */
static void assert_event_group_failure_and_invalid_arguments(void)
{
    /* 定义堆并耗尽。 */
    uintptr_t heap_words[64u];
    size_t free_before = 0u;
    init_heap(heap_words, sizeof(heap_words), &free_before);
    /* 耗尽堆，使后续动态事件组创建稳定进入无内存失败路径。 */
    size_t free_before_fail = 0u;
    exhaust_heap_for_failure_path(&free_before_fail);

    /* 创建失败时应清空输出句柄且不改变堆水位。 */
    MRT_EventGroupHandle group = (MRT_EventGroupHandle)heap_words;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_NO_MEMORY, (unsigned)MRT_EventGroupCreate(&group));
    MRT_TEST_ASSERT_TRUE(group == 0);
    size_t free_after_fail = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_HeapGetFreeSize(&free_after_fail));
    MRT_TEST_ASSERT_EQ_U32((unsigned)free_before_fail, (unsigned)free_after_fail);

    /* 空参数应被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT, (unsigned)MRT_EventGroupCreate(0));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT, (unsigned)MRT_EventGroupDelete(0));
}

/**
 * @brief 验证动态定时器创建、启动、停止和删除。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_dynamic_timer_create_start_stop_delete();
 */
static void assert_dynamic_timer_create_start_stop_delete(void)
{
    /* 定义堆并初始化。 */
    uintptr_t heap_words[128u];
    size_t free_before = 0u;
    init_heap(heap_words, sizeof(heap_words), &free_before);

    /* 初始化内核和回调计数。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());
    g_timer_callback_count = 0u;

    /* 动态创建定时器。 */
    MRT_TimerHandle timer = 0;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TimerCreate("dyn", 2u, false, 0, TimerCallback, &timer));
    MRT_TEST_ASSERT_TRUE(timer != 0);

    /* 启动后再删除，删除应停止活动定时器并释放内存。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TimerStart(timer, 0u));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TimerDelete(timer));

    /* 推进 tick 不应再触发已删除定时器回调。 */
    MRT_KernelTick();
    MRT_KernelTick();
    MRT_TEST_ASSERT_EQ_U32(0u, g_timer_callback_count);

    /* 删除后堆空闲空间恢复。 */
    size_t free_after_delete = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_HeapGetFreeSize(&free_after_delete));
    MRT_TEST_ASSERT_EQ_U32((unsigned)free_before, (unsigned)free_after_delete);
}

/**
 * @brief 验证动态定时器失败路径和静态删除保护。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_timer_failure_validation_and_static_delete();
 */
static void assert_timer_failure_validation_and_static_delete(void)
{
    /* 定义堆并初始化。 */
    uintptr_t heap_words[64u];
    size_t free_before = 0u;
    init_heap(heap_words, sizeof(heap_words), &free_before);

    /* 参数非法应被拒绝。 */
    MRT_TimerHandle timer = 0;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_TimerCreate("bad", 0u, false, 0, TimerCallback, &timer));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_TimerCreate("bad", 1u, false, 0, 0, &timer));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_TimerCreate("bad", 1u, false, 0, TimerCallback, 0));

    /* 静态定时器不能由动态删除 API 释放。 */
    MRT_Timer static_storage;
    MRT_TimerHandle static_timer = 0;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TimerCreateStatic("static", 1u, false, 0, TimerCallback, &static_storage, &static_timer));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OBJECT_BUSY, (unsigned)MRT_TimerDelete(static_timer));

    /* 耗尽堆后动态创建应失败并保持空闲空间不变。 */
    size_t free_before_fail = 0u;
    exhaust_heap_for_failure_path(&free_before_fail);
    timer = (MRT_TimerHandle)heap_words;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_NO_MEMORY,
                           (unsigned)MRT_TimerCreate("dyn", 1u, false, 0, TimerCallback, &timer));
    MRT_TEST_ASSERT_TRUE(timer == 0);
    size_t free_after_fail = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_HeapGetFreeSize(&free_after_fail));
    MRT_TEST_ASSERT_EQ_U32((unsigned)free_before_fail, (unsigned)free_after_fail);

    /* 空删除句柄应被拒绝。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT, (unsigned)MRT_TimerDelete(0));
}

/**
 * @brief 运行动态事件组和定时器测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证动态事件组成功路径。 */
    assert_dynamic_event_group_create_set_delete();

    /* 验证事件组删除忙状态。 */
    assert_event_group_delete_rejects_static_and_waiters();

    /* 验证事件组失败路径。 */
    assert_event_group_failure_and_invalid_arguments();

    /* 验证动态定时器成功路径。 */
    assert_dynamic_timer_create_start_stop_delete();

    /* 验证动态定时器失败和静态删除保护。 */
    assert_timer_failure_validation_and_static_delete();

    /* 所有动态事件/定时器测试均通过。 */
    return 0;
}
