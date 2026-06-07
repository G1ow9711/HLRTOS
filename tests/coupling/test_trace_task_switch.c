#include "mrt_test.h"
#include "myrtos/mrt_kernel.h"
#include "myrtos/mrt_task.h"
#include "myrtos/mrt_trace.h"

/** @brief 最多记录的 trace 事件数量。 */
#define TRACE_EVENT_CAPACITY 8u

/** @brief 捕获到的 trace 事件数组。 */
static MRT_TraceEvent g_trace_events[TRACE_EVENT_CAPACITY];

/** @brief 捕获到的 trace 事件数量。 */
static uint32_t g_trace_event_count;

/**
 * @brief 测试用空任务入口。
 * @param arg 用户参数，本测试不使用。
 * @return void 无返回值。
 * @example
 * DummyTask(NULL);
 */
static void DummyTask(void *arg)
{
    /* 显式丢弃未使用参数，避免编译告警。 */
    (void)arg;
}

/**
 * @brief 捕获 trace 事件的测试 sink。
 * @param event trace 模块传入的事件指针，不能为空。
 * @param user 用户参数，本测试不使用。
 * @return void 无返回值。
 * @example
 * MRT_TraceSetSink(CaptureTraceEvent, NULL);
 */
static void CaptureTraceEvent(const MRT_TraceEvent *event, void *user)
{
    /* 显式丢弃用户参数。 */
    (void)user;

    /* 只记录容量范围内的事件，避免测试数组越界。 */
    if ((event != 0) && (g_trace_event_count < TRACE_EVENT_CAPACITY)) {
        /* 复制事件快照，避免后续内核复用栈上事件对象。 */
        g_trace_events[g_trace_event_count] = *event;

        /* 推进已捕获事件数量。 */
        g_trace_event_count++;
    }
}

/**
 * @brief 清空 trace 捕获状态。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * ResetTraceCapture();
 */
static void ResetTraceCapture(void)
{
    /* 清空捕获数量。 */
    g_trace_event_count = 0u;

    /* 清空第 0 个事件，便于调试初始状态。 */
    g_trace_events[0].kind = MRT_TRACE_EVENT_NONE;
}

/**
 * @brief 验证任务阻塞和唤醒会产生任务切换 trace 事件。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_trace_records_task_switches();
 */
static void assert_trace_records_task_switches(void)
{
    /* 定义高低优先级任务控制块。 */
    MRT_Task high_storage;
    MRT_Task low_storage;

    /* 定义高低优先级任务栈。 */
    MRT_StackType high_stack[128];
    MRT_StackType low_stack[128];

    /* 定义高低优先级任务句柄。 */
    MRT_TaskHandle high_task = 0;
    MRT_TaskHandle low_task = 0;

    /* 初始化内核状态。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 清空 trace 捕获状态。 */
    ResetTraceCapture();

    /* 设置 trace sink。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TraceSetSink(CaptureTraceEvent, 0));

    /* 创建低优先级任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("low", DummyTask, 0, 1u, low_stack, 128u, &low_storage, &low_task));

    /* 创建高优先级任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("high", DummyTask, 0, 5u, high_stack, 128u, &high_storage, &high_task));

    /* 启动调度器，高优先级任务成为当前任务；本测试只检查后续切换。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelStart());

    /* 高优先级任务阻塞 2 tick，调度器应切换到低优先级任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskDelay(2u));

    /* 第一个 trace 事件应记录 high -> low。 */
    MRT_TEST_ASSERT_EQ_U32(1u, (unsigned)g_trace_event_count);
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_TRACE_EVENT_TASK_SWITCH, (unsigned)g_trace_events[0].kind);
    MRT_TEST_ASSERT_TRUE(g_trace_events[0].task == high_task);
    MRT_TEST_ASSERT_TRUE(g_trace_events[0].related_task == low_task);
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)g_trace_events[0].tick);

    /* 推进第 1 个 tick，高优先级任务尚未醒来。 */
    MRT_KernelTick();
    MRT_TEST_ASSERT_EQ_U32(1u, (unsigned)g_trace_event_count);

    /* 推进第 2 个 tick，高优先级任务醒来并抢占低优先级任务。 */
    MRT_KernelTick();

    /* 第二个 trace 事件应记录 low -> high。 */
    MRT_TEST_ASSERT_EQ_U32(2u, (unsigned)g_trace_event_count);
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_TRACE_EVENT_TASK_SWITCH, (unsigned)g_trace_events[1].kind);
    MRT_TEST_ASSERT_TRUE(g_trace_events[1].task == low_task);
    MRT_TEST_ASSERT_TRUE(g_trace_events[1].related_task == high_task);
    MRT_TEST_ASSERT_EQ_U32(2u, (unsigned)g_trace_events[1].tick);

    /* 测试结束后关闭 sink，避免后续路径继续写入测试数组。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TraceSetSink(0, 0));
}

/**
 * @brief 运行任务切换 trace 耦合测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时进程提前退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证任务切换 trace。 */
    assert_trace_records_task_switches();

    /* 所有 trace 任务切换测试均通过。 */
    return 0;
}
