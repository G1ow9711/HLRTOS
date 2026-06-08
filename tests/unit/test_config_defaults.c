#include "mrt_test.h"
#include "myrtos/mrt_config.h"

/**
 * @brief 验证 MyRTOS 默认配置满足内核最小运行条件。
 *
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 检查优先级数量至少覆盖常见实时任务分层。 */
    MRT_TEST_ASSERT_TRUE(MRT_CFG_MAX_PRIORITIES >= 8u);

    /* 检查 tick 频率不低于基本调度和超时测试需求。 */
    MRT_TEST_ASSERT_TRUE(MRT_CFG_TICK_RATE_HZ >= 100u);

    /* 检查最小任务栈满足基础函数调用深度。 */
    MRT_TEST_ASSERT_TRUE(MRT_CFG_MINIMAL_STACK_WORDS >= 64u);

    /* 检查默认开启抢占式调度。 */
    MRT_TEST_ASSERT_EQ_U32(1u, MRT_CFG_USE_PREEMPTION);

    /* 检查默认开启同优先级时间片。 */
    MRT_TEST_ASSERT_EQ_U32(1u, MRT_CFG_USE_TIME_SLICING);

    /* 检查默认支持静态分配，便于嵌入式确定性使用。 */
    MRT_TEST_ASSERT_EQ_U32(1u, MRT_CFG_SUPPORT_STATIC_ALLOCATION);

    /* 检查定时器 pending function 队列有足够默认容量覆盖常见延后执行场景。 */
    MRT_TEST_ASSERT_TRUE(MRT_CFG_TIMER_PENDING_FUNCTION_QUEUE_LENGTH >= 4u);

    /* 检查定时器服务命令队列有足够默认容量覆盖控制命令和延后执行场景。 */
    MRT_TEST_ASSERT_TRUE(MRT_CFG_TIMER_COMMAND_QUEUE_LENGTH >= 4u);

    /* 所有配置断言均通过，返回 0 交给测试运行器统计。 */
    return 0;
}
