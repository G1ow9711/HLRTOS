#include "mrt_test.h"
#include "myrtos/mrt_kernel.h"
#include "myrtos/mrt_queue.h"
#include "myrtos/mrt_trace.h"

/** @brief 最多记录的 trace 事件数量。 */
#define TRACE_EVENT_CAPACITY 8u

/** @brief 捕获到的 trace 事件数组。 */
static MRT_TraceEvent g_trace_events[TRACE_EVENT_CAPACITY];

/** @brief 捕获到的 trace 事件数量。 */
static uint32_t g_trace_event_count;

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
 * @brief 验证队列发送和接收会产生 trace 事件。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_trace_records_queue_send_receive();
 */
static void assert_trace_records_queue_send_receive(void)
{
    /* 初始化内核状态。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 清空 trace 捕获状态。 */
    ResetTraceCapture();

    /* 设置 trace sink。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TraceSetSink(CaptureTraceEvent, 0));

    /* 定义队列控制块和存储区。 */
    MRT_Queue queue_storage;
    uint32_t queue_buffer[2];
    MRT_QueueHandle queue = 0;

    /* 创建静态队列。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_QueueCreateStatic(2u,
                                                           sizeof(uint32_t),
                                                           queue_buffer,
                                                           &queue_storage,
                                                           &queue));

    /* 发送一个元素。 */
    uint32_t send_value = 0x12345678u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_QueueSend(queue, &send_value, 0u));

    /* 接收一个元素。 */
    uint32_t receive_value = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_QueueReceive(queue, &receive_value, 0u));
    MRT_TEST_ASSERT_EQ_U32(send_value, receive_value);

    /* 应捕获发送和接收两个事件。 */
    MRT_TEST_ASSERT_EQ_U32(2u, (unsigned)g_trace_event_count);

    /* 第一个事件记录队列发送，value 保存发送后的队列元素数。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_TRACE_EVENT_QUEUE_SEND, (unsigned)g_trace_events[0].kind);
    MRT_TEST_ASSERT_TRUE(g_trace_events[0].object == queue);
    MRT_TEST_ASSERT_EQ_U32(1u, (unsigned)g_trace_events[0].value);
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)g_trace_events[0].result);

    /* 第二个事件记录队列接收，value 保存接收后的队列元素数。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_TRACE_EVENT_QUEUE_RECEIVE, (unsigned)g_trace_events[1].kind);
    MRT_TEST_ASSERT_TRUE(g_trace_events[1].object == queue);
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)g_trace_events[1].value);
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)g_trace_events[1].result);

    /* 测试结束后关闭 sink。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TraceSetSink(0, 0));
}

/**
 * @brief 运行队列 trace 耦合测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时进程提前退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证队列 send/receive trace。 */
    assert_trace_records_queue_send_receive();

    /* 所有队列 trace 测试均通过。 */
    return 0;
}
