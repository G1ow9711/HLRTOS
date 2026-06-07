#ifndef MYRTOS_MRT_TRACE_H
#define MYRTOS_MRT_TRACE_H

/**
 * @file mrt_trace.h
 * @brief MyRTOS 内核 trace hook 公共接口。
 *
 * trace hook 用于把任务切换、队列操作等内核事件转交给用户提供的轻量回调。该模块不规定日志、
 * 串口、SWO、ETM 或文件输出方式，应用可根据 STM32、DSP 或 host 环境自行选择 sink。
 */

#include "myrtos/mrt_types.h"

/**
 * @brief trace 事件类型。
 */
typedef enum MRT_TraceEventKind {
    /** @brief 无事件，用于初始化测试结构或占位。 */
    MRT_TRACE_EVENT_NONE = 0,
    /** @brief 调度器已经从一个任务切换到另一个任务。 */
    MRT_TRACE_EVENT_TASK_SWITCH,
    /** @brief 队列成功发送一个元素。 */
    MRT_TRACE_EVENT_QUEUE_SEND,
    /** @brief 队列成功接收一个元素。 */
    MRT_TRACE_EVENT_QUEUE_RECEIVE
} MRT_TraceEventKind;

/**
 * @brief trace 事件快照。
 */
typedef struct MRT_TraceEvent {
    /** @brief 事件类型。 */
    MRT_TraceEventKind kind;
    /** @brief 事件发生时的系统 tick。 */
    MRT_Tick tick;
    /** @brief 事件主任务；任务切换时表示旧任务，队列事件时表示当前任务。 */
    MRT_TaskHandle task;
    /** @brief 事件关联任务；任务切换时表示新任务，其他事件通常为空。 */
    MRT_TaskHandle related_task;
    /** @brief 事件关联内核对象；队列事件中保存队列句柄。 */
    void *object;
    /** @brief 事件附加数值；队列事件中保存操作后的元素数量。 */
    uint32_t value;
    /** @brief 事件关联结果；成功事件通常为 MRT_RESULT_OK。 */
    MRT_Result result;
} MRT_TraceEvent;

/**
 * @brief trace 事件下沉回调类型。
 * @param event trace 事件快照，回调内部只应读取，不应保存该指针。
 * @param user 设置 sink 时传入的用户指针。
 * @return void 无返回值。
 * @example
 * static void TraceSink(const MRT_TraceEvent *event, void *user) { (void)event; (void)user; }
 */
typedef void (*MRT_TraceSink)(const MRT_TraceEvent *event, void *user);

/**
 * @brief 设置 trace 事件下沉回调。
 * @param sink 事件回调；传入空指针表示关闭 trace 输出。
 * @param user 传递给 sink 的用户指针，允许为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示设置成功。
 * @example
 * MRT_TraceSetSink(TraceSink, NULL);
 */
MRT_Result MRT_TraceSetSink(MRT_TraceSink sink, void *user);

/**
 * @brief 由内核模块发布一个 trace 事件。
 * @param event 待发布的事件快照，不能为空。
 * @return void 无返回值。
 * @example
 * MRT_TraceEvent event = {0};
 * event.kind = MRT_TRACE_EVENT_QUEUE_SEND;
 * MRT_TraceEmit(&event);
 */
void MRT_TraceEmit(const MRT_TraceEvent *event);

#endif
