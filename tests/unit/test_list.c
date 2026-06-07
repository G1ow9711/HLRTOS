#include "mrt_test.h"
#include "myrtos/mrt_list.h"

/**
 * @brief 验证空链表初始化后处于空状态。
 *
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_empty_list_has_sentinel_links();
 */
static void assert_empty_list_has_sentinel_links(void)
{
    /* 定义待测链表对象。 */
    MRT_List list;

    /* 初始化链表，使哨兵节点形成自环。 */
    MRT_ListInitialize(&list);

    /* 验证初始化后的链表为空。 */
    MRT_TEST_ASSERT_TRUE(MRT_ListIsEmpty(&list));

    /* 验证初始化后的节点数量为 0。 */
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)MRT_ListGetCount(&list));
}

/**
 * @brief 验证尾插操作保持 FIFO 顺序。
 *
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_insert_tail_preserves_fifo_order();
 */
static void assert_insert_tail_preserves_fifo_order(void)
{
    /* 定义待测链表对象。 */
    MRT_List list;

    /* 定义第一个待插入节点。 */
    MRT_ListNode first;

    /* 定义第二个待插入节点。 */
    MRT_ListNode second;

    /* 初始化链表。 */
    MRT_ListInitialize(&list);

    /* 初始化第一个节点，排序值为 10。 */
    MRT_ListNodeInitialize(&first, (void *)1u, 10u);

    /* 初始化第二个节点，排序值为 20。 */
    MRT_ListNodeInitialize(&second, (void *)2u, 20u);

    /* 将第一个节点插入链表尾部。 */
    MRT_ListInsertTail(&list, &first);

    /* 将第二个节点插入链表尾部。 */
    MRT_ListInsertTail(&list, &second);

    /* 验证头节点仍然是第一个插入的节点。 */
    MRT_TEST_ASSERT_TRUE(MRT_ListGetHead(&list) == &first);

    /* 验证第一个节点的后继是第二个节点。 */
    MRT_TEST_ASSERT_TRUE(MRT_ListGetNext(&first) == &second);

    /* 验证链表节点数量为 2。 */
    MRT_TEST_ASSERT_EQ_U32(2u, (unsigned)MRT_ListGetCount(&list));
}

/**
 * @brief 验证有序插入按节点 value 从小到大排列。
 *
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_ordered_insert_sorts_by_value();
 */
static void assert_ordered_insert_sorts_by_value(void)
{
    /* 定义待测链表对象。 */
    MRT_List list;

    /* 定义排序值较高的节点。 */
    MRT_ListNode high;

    /* 定义排序值较低的节点。 */
    MRT_ListNode low;

    /* 初始化链表。 */
    MRT_ListInitialize(&list);

    /* 初始化高排序值节点。 */
    MRT_ListNodeInitialize(&high, (void *)1u, 50u);

    /* 初始化低排序值节点。 */
    MRT_ListNodeInitialize(&low, (void *)2u, 10u);

    /* 先插入高排序值节点。 */
    MRT_ListInsertOrdered(&list, &high);

    /* 再插入低排序值节点。 */
    MRT_ListInsertOrdered(&list, &low);

    /* 验证链表头节点变成排序值更低的节点。 */
    MRT_TEST_ASSERT_TRUE(MRT_ListGetHead(&list) == &low);
}

/**
 * @brief 验证移除节点会解除节点与链表的连接。
 *
 * @param void 无输入参数。
 * @return void 断言失败时测试进程直接退出。
 * @example
 * assert_remove_detaches_node();
 */
static void assert_remove_detaches_node(void)
{
    /* 定义待测链表对象。 */
    MRT_List list;

    /* 定义待插入和移除的节点。 */
    MRT_ListNode node;

    /* 初始化链表。 */
    MRT_ListInitialize(&list);

    /* 初始化节点。 */
    MRT_ListNodeInitialize(&node, (void *)1u, 10u);

    /* 将节点插入链表尾部。 */
    MRT_ListInsertTail(&list, &node);

    /* 从链表移除该节点。 */
    MRT_ListRemove(&node);

    /* 验证移除后链表为空。 */
    MRT_TEST_ASSERT_TRUE(MRT_ListIsEmpty(&list));

    /* 验证节点不再处于任何链表中。 */
    MRT_TEST_ASSERT_TRUE(!MRT_ListNodeIsLinked(&node));
}

/**
 * @brief 运行链表模块全部单元测试。
 *
 * @param void 无输入参数。
 * @return int 返回 0 表示测试通过；断言失败时测试进程直接退出。
 * @example
 * python tools\run_host_tests.py
 */
int main(void)
{
    /* 验证空链表初始化。 */
    assert_empty_list_has_sentinel_links();

    /* 验证尾插 FIFO 顺序。 */
    assert_insert_tail_preserves_fifo_order();

    /* 验证按 value 有序插入。 */
    assert_ordered_insert_sorts_by_value();

    /* 验证节点移除和脱链。 */
    assert_remove_detaches_node();

    /* 所有链表测试均通过，返回 0。 */
    return 0;
}
