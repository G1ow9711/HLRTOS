#include "myrtos/mrt_list.h"

/**
 * @brief 初始化链表对象。
 * @param list 待初始化的链表指针，不能为空。
 * @return void 无返回值。
 * @example
 * MRT_List ready_list;
 * MRT_ListInitialize(&ready_list);
 */
void MRT_ListInitialize(MRT_List *list)
{
    /* 让哨兵节点的前向指针指向自身，表示当前没有尾节点。 */
    list->sentinel.prev = &list->sentinel;

    /* 让哨兵节点的后向指针指向自身，表示当前没有头节点。 */
    list->sentinel.next = &list->sentinel;

    /* 记录哨兵节点所属链表，便于调试时识别链表对象。 */
    list->sentinel.owner = list;

    /* 哨兵节点不承载用户对象，因此 item 固定为空。 */
    list->sentinel.item = 0;

    /* 哨兵节点排序值设置为最大值，使有序插入能自然停在哨兵前。 */
    list->sentinel.value = UINT32_MAX;

    /* 初始化真实节点数量为 0。 */
    list->count = 0u;
}

/**
 * @brief 初始化链表节点。
 * @param node 待初始化的节点指针，不能为空。
 * @param item 节点关联的用户对象指针，允许为空。
 * @param value 节点排序值，含义由使用该链表的模块决定。
 * @return void 无返回值。
 * @example
 * MRT_ListNode node;
 * MRT_ListNodeInitialize(&node, task, wake_tick);
 */
void MRT_ListNodeInitialize(MRT_ListNode *node, void *item, MRT_Tick value)
{
    /* 清空前向指针，表示节点尚未加入链表。 */
    node->prev = 0;

    /* 清空后向指针，表示节点尚未加入链表。 */
    node->next = 0;

    /* 清空所属链表指针，作为未入链状态标记。 */
    node->owner = 0;

    /* 保存节点关联对象，后续调度器可由节点回到任务对象。 */
    node->item = item;

    /* 保存节点排序值，供延时链表或定时器链表使用。 */
    node->value = value;
}

/**
 * @brief 判断链表是否为空。
 * @param list 待检查的链表指针，不能为空。
 * @return bool 返回 true 表示链表为空，返回 false 表示链表存在真实节点。
 * @example
 * if (MRT_ListIsEmpty(&ready_list)) { MRT_KernelYield(); }
 */
bool MRT_ListIsEmpty(const MRT_List *list)
{
    /* 真实节点数量为 0 时，链表为空。 */
    return list->count == 0u;
}

/**
 * @brief 获取链表真实节点数量。
 * @param list 待查询的链表指针，不能为空。
 * @return size_t 返回链表中的真实节点数量。
 * @example
 * size_t ready_count = MRT_ListGetCount(&ready_list);
 */
size_t MRT_ListGetCount(const MRT_List *list)
{
    /* 直接返回链表维护的节点计数。 */
    return list->count;
}

/**
 * @brief 将节点插入链表尾部。
 * @param list 目标链表指针，不能为空。
 * @param node 待插入节点指针，不能为空且必须处于未入链状态。
 * @return void 无返回值。
 * @example
 * MRT_ListInsertTail(&ready_list, &task->ready_node);
 */
void MRT_ListInsertTail(MRT_List *list, MRT_ListNode *node)
{
    /* 读取当前尾节点；空链表时尾节点就是哨兵节点。 */
    MRT_ListNode *tail = list->sentinel.prev;

    /* 新节点的后继指向哨兵，表示新节点将成为尾节点。 */
    node->next = &list->sentinel;

    /* 新节点的前驱指向旧尾节点。 */
    node->prev = tail;

    /* 记录新节点所属链表，表示节点已经入链。 */
    node->owner = list;

    /* 旧尾节点的后继改为新节点。 */
    tail->next = node;

    /* 哨兵的前驱改为新节点，完成尾部链接。 */
    list->sentinel.prev = node;

    /* 链表真实节点数量加 1。 */
    list->count++;
}

/**
 * @brief 按 value 从小到大将节点插入链表。
 * @param list 目标链表指针，不能为空。
 * @param node 待插入节点指针，不能为空且必须处于未入链状态。
 * @return void 无返回值。
 * @example
 * MRT_ListInsertOrdered(&delay_list, &task->delay_node);
 */
void MRT_ListInsertOrdered(MRT_List *list, MRT_ListNode *node)
{
    /* 从第一个真实节点开始查找插入位置。 */
    MRT_ListNode *cursor = list->sentinel.next;

    /* 跳过所有排序值小于或等于新节点的节点，保持相同 value 的 FIFO 顺序。 */
    while ((cursor != &list->sentinel) && (cursor->value <= node->value)) {
        /* 移动到下一个节点继续比较。 */
        cursor = cursor->next;
    }

    /* 新节点的后继指向第一个排序值更大的节点或哨兵。 */
    node->next = cursor;

    /* 新节点的前驱指向插入位置之前的节点。 */
    node->prev = cursor->prev;

    /* 记录新节点所属链表。 */
    node->owner = list;

    /* 前驱节点的后继改为新节点。 */
    cursor->prev->next = node;

    /* 后继节点的前驱改为新节点。 */
    cursor->prev = node;

    /* 链表真实节点数量加 1。 */
    list->count++;
}

/**
 * @brief 从所属链表移除节点。
 * @param node 待移除节点指针，不能为空且必须已经入链。
 * @return void 无返回值。
 * @example
 * MRT_ListRemove(&task->ready_node);
 */
void MRT_ListRemove(MRT_ListNode *node)
{
    /* 保存所属链表，用于更新节点计数。 */
    MRT_List *owner = node->owner;

    /* 让前驱节点越过当前节点，直接指向当前节点的后继。 */
    node->prev->next = node->next;

    /* 让后继节点越过当前节点，直接指向当前节点的前驱。 */
    node->next->prev = node->prev;

    /* 所属链表真实节点数量减 1。 */
    owner->count--;

    /* 清空当前节点前驱，防止误用旧链路。 */
    node->prev = 0;

    /* 清空当前节点后继，防止误用旧链路。 */
    node->next = 0;

    /* 清空所属链表，表示节点已经脱链。 */
    node->owner = 0;
}

/**
 * @brief 获取链表头部真实节点。
 * @param list 待查询的链表指针，不能为空。
 * @return MRT_ListNode* 返回头部节点；链表为空时返回空指针。
 * @example
 * MRT_ListNode *node = MRT_ListGetHead(&ready_list);
 */
MRT_ListNode *MRT_ListGetHead(MRT_List *list)
{
    /* 如果链表为空，返回空指针给调用方。 */
    if (list->count == 0u) {
        /* 空链表没有真实头节点。 */
        return 0;
    }

    /* 非空链表的哨兵后继就是头部真实节点。 */
    return list->sentinel.next;
}

/**
 * @brief 获取指定节点的后继节点。
 * @param node 当前节点指针，不能为空。
 * @return MRT_ListNode* 返回后继节点；如果当前节点是尾节点，则返回所属链表的哨兵节点。
 * @example
 * MRT_ListNode *next = MRT_ListGetNext(current);
 */
MRT_ListNode *MRT_ListGetNext(MRT_ListNode *node)
{
    /* 直接返回节点保存的后继指针。 */
    return node->next;
}

/**
 * @brief 判断节点是否已经加入某个链表。
 * @param node 待检查的节点指针，不能为空。
 * @return bool 返回 true 表示节点已入链，返回 false 表示节点未入链。
 * @example
 * if (!MRT_ListNodeIsLinked(&task->ready_node)) { MRT_ListInsertTail(&ready_list, &task->ready_node); }
 */
bool MRT_ListNodeIsLinked(const MRT_ListNode *node)
{
    /* owner 非空表示节点当前属于某个链表。 */
    return node->owner != 0;
}
