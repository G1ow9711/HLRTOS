#include "mrt_test.h"
#include "myrtos/mrt_timer.h"

/**
 * @brief 测试用定时器回调函数。
 * @param timer 到期定时器句柄，本测试不使用。
 * @param arg 用户参数，本测试不使用。
 * @return void 无返回值。
 * @example
 * DummyTimerCallback(timer, arg);
 */
static void DummyTimerCallback(MRT_TimerHandle timer, void *arg)
{
    /* 显式丢弃未使用定时器句柄，避免编译器告警。 */
    (void)timer;

    /* 显式丢弃未使用用户参数，避免编译器告警。 */
    (void)arg;
}

/**
 * @brief 验证静态创建定时器会初始化基础属性且默认不活动。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_timer_create_static_initializes_fields();
 */
static void assert_timer_create_static_initializes_fields(void)
{
    /* 定义定时器控制块。 */
    MRT_Timer storage;

    /* 定义定时器句柄。 */
    MRT_TimerHandle timer = 0;

    /* 定义回调参数。 */
    uint32_t user_arg = 0x1234u;

    /* 创建周期为 5 tick 的单次定时器。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TimerCreateStatic("once",
                                                           5u,
                                                           false,
                                                           &user_arg,
                                                           DummyTimerCallback,
                                                           &storage,
                                                           &timer));

    /* 输出句柄应指向调用方提供的控制块。 */
    MRT_TEST_ASSERT_TRUE(timer == &storage);

    /* 新创建定时器默认不活动。 */
    bool active = true;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_TimerIsActive(timer, &active));
    MRT_TEST_ASSERT_TRUE(!active);

    /* 定时器名称应保持创建时传入的指针。 */
    MRT_TEST_ASSERT_TRUE(MRT_TimerGetName(timer) == (const char *)"once");

    /* 控制块字段应记录创建参数。 */
    MRT_TEST_ASSERT_EQ_U32(5u, (unsigned)storage.period_ticks);
    MRT_TEST_ASSERT_TRUE(!storage.auto_reload);
    MRT_TEST_ASSERT_TRUE(storage.callback == DummyTimerCallback);
    MRT_TEST_ASSERT_TRUE(storage.arg == &user_arg);
    MRT_TEST_ASSERT_TRUE(storage.static_storage);
}

/**
 * @brief 验证自动重载定时器创建参数被保存。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_timer_create_static_preserves_auto_reload();
 */
static void assert_timer_create_static_preserves_auto_reload(void)
{
    /* 定义定时器控制块和句柄。 */
    MRT_Timer storage;
    MRT_TimerHandle timer = 0;

    /* 创建周期定时器。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                           (unsigned)MRT_TimerCreateStatic("periodic",
                                                           10u,
                                                           true,
                                                           0,
                                                           DummyTimerCallback,
                                                           &storage,
                                                           &timer));

    /* auto-reload 标志应为 true。 */
    MRT_TEST_ASSERT_TRUE(storage.auto_reload);
}

/**
 * @brief 验证定时器创建和查询 API 参数校验。
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_timer_create_static_rejects_invalid_arguments();
 */
static void assert_timer_create_static_rejects_invalid_arguments(void)
{
    /* 定义定时器控制块和句柄。 */
    MRT_Timer storage;
    MRT_TimerHandle timer = 0;

    /* 周期不能为 0。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_TimerCreateStatic("bad", 0u, false, 0, DummyTimerCallback, &storage, &timer));

    /* 回调函数不能为空。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_TimerCreateStatic("bad", 1u, false, 0, 0, &storage, &timer));

    /* 控制块不能为空。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_TimerCreateStatic("bad", 1u, false, 0, DummyTimerCallback, 0, &timer));

    /* 输出句柄不能为空。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT,
                           (unsigned)MRT_TimerCreateStatic("bad", 1u, false, 0, DummyTimerCallback, &storage, 0));

    /* 空定时器句柄查询 active 应返回参数错误。 */
    bool active = false;
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT, (unsigned)MRT_TimerIsActive(0, &active));

    /* active 输出指针不能为空。 */
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_INVALID_ARGUMENT, (unsigned)MRT_TimerIsActive(&storage, 0));

    /* 空定时器句柄名称查询返回空指针。 */
    MRT_TEST_ASSERT_TRUE(MRT_TimerGetName(0) == 0);
}

/**
 * @brief 运行定时器静态创建测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证静态创建基础字段。 */
    assert_timer_create_static_initializes_fields();

    /* 验证自动重载参数。 */
    assert_timer_create_static_preserves_auto_reload();

    /* 验证参数校验。 */
    assert_timer_create_static_rejects_invalid_arguments();

    /* 所有定时器静态创建测试均通过。 */
    return 0;
}
