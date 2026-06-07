#include "mrt_test.h"
#include "myrtos/mrt_types.h"

int main(void)
{
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)MRT_RESULT_OK);
    MRT_TEST_ASSERT_TRUE(sizeof(MRT_Tick) == 4u);
    MRT_TEST_ASSERT_TRUE(sizeof(MRT_StackType) >= 4u);
    MRT_TEST_ASSERT_TRUE(sizeof(MRT_TaskHandle) == sizeof(void *));
    return 0;
}
