#include "mrt_test.h"
#include "myrtos/mrt_kernel.h"
#include "myrtos/mrt_port.h"
#include "myrtos/mrt_stream_buffer.h"
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
 * @brief 验证 ISR 写入达到触发水位后唤醒阻塞读者。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_stream_buffer_isr_send_wakes_blocked_reader();
 */
static void assert_stream_buffer_isr_send_wakes_blocked_reader(void)
{
    /* 定义高优先级读者任务控制块。 */
    MRT_Task reader_storage;

    /* 定义低优先级后台任务控制块。 */
    MRT_Task background_storage;

    /* 定义任务栈。 */
    MRT_StackType reader_stack[128u];
    MRT_StackType background_stack[128u];

    /* 定义任务句柄。 */
    MRT_TaskHandle reader_task = 0;
    MRT_TaskHandle background_task = 0;

    /* 定义流缓冲控制块和底层存储。 */
    MRT_StreamBuffer stream_storage;
    uint8_t stream_bytes[8u];
    MRT_StreamBufferHandle stream = 0;

    /* 初始化内核。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());

    /* 创建低优先级后台任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("background",
                                                          DummyTask,
                                                          0,
                                                          1u,
                                                          background_stack,
                                                          128u,
                                                          &background_storage,
                                                          &background_task));

    /* 创建高优先级读者任务。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TaskCreateStatic("reader",
                                                          DummyTask,
                                                          0,
                                                          5u,
                                                          reader_stack,
                                                          128u,
                                                          &reader_storage,
                                                          &reader_task));

    /* 创建触发水位为 3 字节的流缓冲。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_StreamBufferCreateStatic(8u, 3u, stream_bytes, &stream_storage, &stream));

    /* 启动调度器后应先运行高优先级读者。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelStart());
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == reader_task);

    /* 高优先级读者等待空流缓冲，进入读等待链表。 */
    uint8_t read_output[4u] = {0u, 0u, 0u, 0u};
    size_t received = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_TIMEOUT,
                           (unsigned)MRT_StreamBufferReceive(stream, read_output, sizeof(read_output), 20u, &received));

    /* 读者阻塞后低优先级后台任务应成为当前任务。 */
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == background_task);

    /* 读等待链表应包含高优先级读者。 */
    MRT_TEST_ASSERT_EQ_U32(1u, (unsigned)MRT_ListGetCount(&stream_storage.waiting_readers));

    /* 切换到模拟 ISR 上下文。 */
    MRT_PortMockSetInsideISR(true);

    /* ISR 写入 3 字节，刚好达到触发水位。 */
    const uint8_t isr_data[3u] = {4u, 5u, 6u};
    size_t sent = 0u;
    bool should_yield = false;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_StreamBufferSendFromISR(stream,
                                                                 isr_data,
                                                                 sizeof(isr_data),
                                                                 &sent,
                                                                 &should_yield));

    /* ISR 发送应写入全部 3 字节并请求 ISR 退出后切换。 */
    MRT_TEST_ASSERT_EQ_U32(3u, (unsigned)sent);
    MRT_TEST_ASSERT_TRUE(should_yield);

    /* ISR 内不应立即切换当前任务。 */
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == background_task);

    /* 等待读者链表应被清空。 */
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)MRT_ListGetCount(&stream_storage.waiting_readers));

    /* 读者应回到 ready 状态。 */
    MRT_TaskState reader_state = MRT_TASK_STATE_DELETED;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskGetState(reader_task, &reader_state));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_TASK_STATE_READY, (unsigned)reader_state);

    /* 恢复任务上下文。 */
    MRT_PortMockSetInsideISR(false);
}

/**
 * @brief 运行流缓冲 ISR 唤醒读者耦合测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证 ISR 写入唤醒阻塞读者。 */
    assert_stream_buffer_isr_send_wakes_blocked_reader();

    /* 所有流缓冲 ISR 唤醒测试均通过。 */
    return 0;
}
