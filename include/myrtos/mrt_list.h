#ifndef MYRTOS_MRT_LIST_H
#define MYRTOS_MRT_LIST_H

/**
 * @file mrt_list.h
 * @brief MyRTOS 内核侵入式双向链表接口。
 *
 * 内核调度器、延时链表、等待链表和定时器列表都会复用该链表。
 * 链表节点由对象自身持有，避免运行时额外分配节点内存。
 */

#include "myrtos/mrt_types.h"

/** @brief 链表对象前置声明，用于节点保存所属链表指针。 */
typedef struct MRT_List MRT_List;

/**
 * @brief 侵入式链表节点。
 *
 * 每个可进入链表的内核对象都内嵌一个或多个 MRT_ListNode，
 * 从而在不分配额外节点内存的情况下加入就绪、延时或等待链表。
 */
typedef struct MRT_ListNode {
    /** @brief 前一个节点，未入链时为空。 */
    struct MRT_ListNode *prev;
    /** @brief 后一个节点，未入链时为空。 */
    struct MRT_ListNode *next;
    /** @brief 当前所属链表，未入链时为空。 */
    MRT_List *owner;
    /** @brief 节点关联的用户对象，通常是任务控制块或定时器对象。 */
    void *item;
    /** @brief 排序值，延时链表通常存放唤醒 tick。 */
    MRT_Tick value;
} MRT_ListNode;

/**
 * @brief 双向循环链表。
 *
 * 链表使用哨兵节点简化头尾插入和移除逻辑，空链表时哨兵节点指向自身。
 */
struct MRT_List {
    /** @brief 哨兵节点，不承载用户对象。 */
    MRT_ListNode sentinel;
    /** @brief 当前链表中的真实节点数量。 */
    size_t count;
};

/**
 * @brief 初始化链表对象。
 * @param list 待初始化的链表指针，不能为空。
 * @return void 无返回值。
 * @example
 * MRT_List ready_list;
 * MRT_ListInitialize(&ready_list);
 */
void MRT_ListInitialize(MRT_List *list);

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
void MRT_ListNodeInitialize(MRT_ListNode *node, void *item, MRT_Tick value);

/**
 * @brief 判断链表是否为空。
 * @param list 待检查的链表指针，不能为空。
 * @return bool 返回 true 表示链表为空，返回 false 表示链表存在真实节点。
 * @example
 * if (MRT_ListIsEmpty(&ready_list)) { MRT_KernelYield(); }
 */
bool MRT_ListIsEmpty(const MRT_List *list);

/**
 * @brief 获取链表真实节点数量。
 * @param list 待查询的链表指针，不能为空。
 * @return size_t 返回链表中的真实节点数量。
 * @example
 * size_t ready_count = MRT_ListGetCount(&ready_list);
 */
size_t MRT_ListGetCount(const MRT_List *list);

/**
 * @brief 将节点插入链表尾部。
 * @param list 目标链表指针，不能为空。
 * @param node 待插入节点指针，不能为空且必须处于未入链状态。
 * @return void 无返回值。
 * @example
 * MRT_ListInsertTail(&ready_list, &task->ready_node);
 */
void MRT_ListInsertTail(MRT_List *list, MRT_ListNode *node);

/**
 * @brief 按 value 从小到大将节点插入链表。
 * @param list 目标链表指针，不能为空。
 * @param node 待插入节点指针，不能为空且必须处于未入链状态。
 * @return void 无返回值。
 * @example
 * MRT_ListInsertOrdered(&delay_list, &task->delay_node);
 */
void MRT_ListInsertOrdered(MRT_List *list, MRT_ListNode *node);

/**
 * @brief 从所属链表移除节点。
 * @param node 待移除节点指针，不能为空且必须已经入链。
 * @return void 无返回值。
 * @example
 * MRT_ListRemove(&task->ready_node);
 */
void MRT_ListRemove(MRT_ListNode *node);

/**
 * @brief 获取链表头部真实节点。
 * @param list 待查询的链表指针，不能为空。
 * @return MRT_ListNode* 返回头部节点；链表为空时返回空指针。
 * @example
 * MRT_ListNode *node = MRT_ListGetHead(&ready_list);
 */
MRT_ListNode *MRT_ListGetHead(MRT_List *list);

/**
 * @brief 获取指定节点的后继节点。
 * @param node 当前节点指针，不能为空。
 * @return MRT_ListNode* 返回后继节点；如果当前节点是尾节点，则返回所属链表的哨兵节点。
 * @example
 * MRT_ListNode *next = MRT_ListGetNext(current);
 */
MRT_ListNode *MRT_ListGetNext(MRT_ListNode *node);

/**
 * @brief 判断节点是否已经加入某个链表。
 * @param node 待检查的节点指针，不能为空。
 * @return bool 返回 true 表示节点已入链，返回 false 表示节点未入链。
 * @example
 * if (!MRT_ListNodeIsLinked(&task->ready_node)) { MRT_ListInsertTail(&ready_list, &task->ready_node); }
 */
bool MRT_ListNodeIsLinked(const MRT_ListNode *node);

#endif
