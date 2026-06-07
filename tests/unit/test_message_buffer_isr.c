#include "mrt_test.h"
#include "myrtos/mrt_kernel.h"
#include "myrtos/mrt_message_buffer.h"
#include "myrtos/mrt_port.h"
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
 * @brief 创建测试用消息缓冲。
 * @param storage 消息缓冲控制块，不能为 NULL。
 * @param buffer 底层字节存储，不能为 NULL。
 * @param capacity 字节容量，必须能容纳长度头和至少一个消息字节。
 * @return MRT_MessageBufferHandle 返回创建成功的消息缓冲句柄。
 * @example
 * MRT_MessageBufferHandle mb = CreateMessageBufferOrFail(&storage, buffer, sizeof(buffer));
 */
static MRT_MessageBufferHandle CreateMessageBufferOrFail(MRT_MessageBuffer *storage,
                                                         uint8_t *buffer,
                                                         size_t capacity)
{
    /* 定义输出句柄。 */
    MRT_MessageBufferHandle message_buffer = 0;

    /* 静态创建测试消息缓冲。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_MessageBufferCreateStatic(capacity,
                                                                   buffer,
                                                                   storage,
                                                                   &message_buffer));

    /* 返回创建得到的消息缓冲句柄。 */
    return message_buffer;
}

/**
 * @brief 验证 ISR 消息缓冲发送和接收保持完整包边界。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_message_buffer_isr_send_receive_preserves_packet();
 */
static void assert_message_buffer_isr_send_receive_preserves_packet(void)
{
    /* 初始化端口 mock 并切换到 ISR 上下文。 */
    MRT_PortInitialize();
    MRT_PortMockSetInsideISR(true);

    /* 定义消息缓冲控制块和底层存储。 */
    MRT_MessageBuffer storage;
    uint8_t buffer[16u];
    MRT_MessageBufferHandle message_buffer = CreateMessageBufferOrFail(&storage, buffer, sizeof(buffer));

    /* 准备一条 ISR 待发送消息。 */
    const uint8_t input[3u] = {7u, 8u, 9u};

    /* 从 ISR 写入完整消息。 */
    size_t sent = 0u;
    bool should_yield = true;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_MessageBufferSendFromISR(message_buffer,
                                                                  input,
                                                                  sizeof(input),
                                                                  &sent,
                                                                  &should_yield));

    /* 没有阻塞读者时不应请求延后调度。 */
    MRT_TEST_ASSERT_EQ_U32(3u, (unsigned)sent);
    MRT_TEST_ASSERT_TRUE(!should_yield);

    /* 从 ISR 读取完整消息。 */
    uint8_t output[4u] = {0u, 0u, 0u, 0u};
    size_t received = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_MessageBufferReceiveFromISR(message_buffer,
                                                                     output,
                                                                     sizeof(output),
                                                                     &received));

    /* 验证读取长度和消息内容。 */
    MRT_TEST_ASSERT_EQ_U32(3u, (unsigned)received);
    MRT_TEST_ASSERT_EQ_U32(7u, (unsigned)output[0]);
    MRT_TEST_ASSERT_EQ_U32(8u, (unsigned)output[1]);
    MRT_TEST_ASSERT_EQ_U32(9u, (unsigned)output[2]);

    /* 消息被取走后，ISR 再接收应返回对象为空。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OBJECT_EMPTY,
                           (unsigned)MRT_MessageBufferReceiveFromISR(message_buffer,
                                                                     output,
                                                                     sizeof(output),
                                                                     &received));
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)received);

    /* 恢复任务上下文，避免影响后续测试。 */
    MRT_PortMockSetInsideISR(false);
}

/**
 * @brief 验证 ISR 消息缓冲 API 的上下文、容量和小输出缓冲边界。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_message_buffer_isr_rejects_invalid_context_and_capacity_edges();
 */
static void assert_message_buffer_isr_rejects_invalid_context_and_capacity_edges(void)
{
    /* 初始化端口 mock，默认处于任务上下文。 */
    MRT_PortInitialize();

    /* 定义刚好可容纳 2 字节消息的缓冲。 */
    MRT_MessageBuffer storage;
    uint8_t buffer[6u];
    MRT_MessageBufferHandle message_buffer = CreateMessageBufferOrFail(&storage, buffer, sizeof(buffer));

    /* 任务上下文调用 FromISR 发送应返回非法上下文，并清零输出。 */
    const uint8_t two_bytes[2u] = {1u, 2u};
    size_t transferred = 99u;
    bool should_yield = true;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_CONTEXT,
                           (unsigned)MRT_MessageBufferSendFromISR(message_buffer,
                                                                  two_bytes,
                                                                  sizeof(two_bytes),
                                                                  &transferred,
                                                                  &should_yield));
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)transferred);
    MRT_TEST_ASSERT_TRUE(!should_yield);

    /* 任务上下文调用 FromISR 接收也应返回非法上下文。 */
    uint8_t output[2u] = {0u, 0u};
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_CONTEXT,
                           (unsigned)MRT_MessageBufferReceiveFromISR(message_buffer,
                                                                     output,
                                                                     sizeof(output),
                                                                     &transferred));
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)transferred);

    /* 切换到 ISR 上下文。 */
    MRT_PortMockSetInsideISR(true);

    /* 非零长度发送必须提供消息地址。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_MessageBufferSendFromISR(message_buffer, 0, 1u, &transferred, &should_yield));

    /* 单条消息超过总容量时必须被拒绝。 */
    const uint8_t too_large[3u] = {3u, 4u, 5u};
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OBJECT_FULL,
                           (unsigned)MRT_MessageBufferSendFromISR(message_buffer,
                                                                  too_large,
                                                                  sizeof(too_large),
                                                                  &transferred,
                                                                  &should_yield));
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)transferred);

    /* 写入一条刚好填满缓冲的 2 字节消息。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_MessageBufferSendFromISR(message_buffer,
                                                                  two_bytes,
                                                                  sizeof(two_bytes),
                                                                  &transferred,
                                                                  &should_yield));
    MRT_TEST_ASSERT_EQ_U32(2u, (unsigned)transferred);

    /* 缓冲已满时，再发送 1 字节消息也不能写入半包。 */
    const uint8_t one_byte[1u] = {6u};
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OBJECT_FULL,
                           (unsigned)MRT_MessageBufferSendFromISR(message_buffer,
                                                                  one_byte,
                                                                  sizeof(one_byte),
                                                                  &transferred,
                                                                  &should_yield));
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)transferred);

    /* 输出缓冲过小时，ISR 接收不应移除待取消息。 */
    uint8_t small_output[1u] = {0u};
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OBJECT_FULL,
                           (unsigned)MRT_MessageBufferReceiveFromISR(message_buffer,
                                                                     small_output,
                                                                     sizeof(small_output),
                                                                     &transferred));
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)transferred);

    /* 使用足够大的输出缓冲应能读出原消息。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_MessageBufferReceiveFromISR(message_buffer,
                                                                     output,
                                                                     sizeof(output),
                                                                     &transferred));
    MRT_TEST_ASSERT_EQ_U32(2u, (unsigned)transferred);
    MRT_TEST_ASSERT_EQ_U32(1u, (unsigned)output[0]);
    MRT_TEST_ASSERT_EQ_U32(2u, (unsigned)output[1]);

    /* 恢复任务上下文。 */
    MRT_PortMockSetInsideISR(false);
}

/**
 * @brief 验证任务态空接收会阻塞，ISR 发送完整消息后唤醒读者。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_message_buffer_isr_send_wakes_blocked_reader();
 */
static void assert_message_buffer_isr_send_wakes_blocked_reader(void)
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

    /* 定义消息缓冲控制块和底层存储。 */
    MRT_MessageBuffer message_storage;
    uint8_t message_bytes[12u];
    MRT_MessageBufferHandle message_buffer = 0;

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

    /* 创建消息缓冲。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_MessageBufferCreateStatic(sizeof(message_bytes),
                                                                   message_bytes,
                                                                   &message_storage,
                                                                   &message_buffer));

    /* 启动调度器后应先运行高优先级读者。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelStart());
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == reader_task);

    /* 高优先级读者等待空消息缓冲，进入读等待链表。 */
    uint8_t read_output[4u] = {0u, 0u, 0u, 0u};
    size_t received = 0u;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_TIMEOUT,
                           (unsigned)MRT_MessageBufferReceive(message_buffer,
                                                             read_output,
                                                             sizeof(read_output),
                                                             20u,
                                                             &received));

    /* 读者阻塞后低优先级后台任务应成为当前任务。 */
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == background_task);

    /* 读者任务应记录消息缓冲接收等待原因。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_TASK_WAIT_REASON_MESSAGE_RECEIVE,
                           (unsigned)reader_storage.wait_reason);

    /* 读等待链表应包含高优先级读者。 */
    MRT_TEST_ASSERT_EQ_U32(1u, (unsigned)MRT_ListGetCount(&message_storage.waiting_readers));

    /* 切换到模拟 ISR 上下文。 */
    MRT_PortMockSetInsideISR(true);

    /* ISR 写入一条完整消息。 */
    const uint8_t isr_message[2u] = {4u, 5u};
    size_t sent = 0u;
    bool should_yield = false;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_MessageBufferSendFromISR(message_buffer,
                                                                  isr_message,
                                                                  sizeof(isr_message),
                                                                  &sent,
                                                                  &should_yield));

    /* ISR 发送应写入整条消息，并请求 ISR 退出后切换。 */
    MRT_TEST_ASSERT_EQ_U32(2u, (unsigned)sent);
    MRT_TEST_ASSERT_TRUE(should_yield);

    /* ISR 内不应立即切换当前任务。 */
    MRT_TEST_ASSERT_TRUE(MRT_TaskGetCurrent() == background_task);

    /* 等待读者链表应被清空。 */
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)MRT_ListGetCount(&message_storage.waiting_readers));

    /* 读者应回到 ready 状态。 */
    MRT_TaskState reader_state = MRT_TASK_STATE_DELETED;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TaskGetState(reader_task, &reader_state));
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_TASK_STATE_READY, (unsigned)reader_state);

    /* 恢复任务上下文。 */
    MRT_PortMockSetInsideISR(false);
}

/**
 * @brief 运行消息缓冲 ISR API 测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证 ISR 完整消息发送接收。 */
    assert_message_buffer_isr_send_receive_preserves_packet();

    /* 验证 ISR 上下文和容量边界。 */
    assert_message_buffer_isr_rejects_invalid_context_and_capacity_edges();

    /* 验证 ISR 发送唤醒阻塞读者。 */
    assert_message_buffer_isr_send_wakes_blocked_reader();

    /* 所有消息缓冲 ISR 测试均通过。 */
    return 0;
}
