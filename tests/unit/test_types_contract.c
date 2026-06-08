#include "mrt_test.h"
#include "myrtos/mrt_types.h"

/**
 * @brief 运行基础类型契约测试。
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证公共返回值枚举具备稳定的零成功码。 */
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)MRT_RESULT_OK);

    /* 验证 tick 和栈元素类型满足 32 位嵌入式模型。 */
    MRT_TEST_ASSERT_TRUE(sizeof(MRT_Tick) == 4u);
    MRT_TEST_ASSERT_TRUE(sizeof(MRT_StackType) >= 4u);

    /* 验证任务句柄是指针宽度。 */
    MRT_TEST_ASSERT_TRUE(sizeof(MRT_TaskHandle) == sizeof(void *));

    /* 基础类型契约测试通过。 */
    return 0;
}
