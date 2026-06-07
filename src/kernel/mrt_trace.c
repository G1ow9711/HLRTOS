#include "myrtos/mrt_trace.h"

/** @brief 当前 trace 事件下沉回调；为空表示关闭 trace 输出。 */
static MRT_TraceSink g_trace_sink;

/** @brief 传递给 trace sink 的用户指针。 */
static void *g_trace_user;

/**
 * @brief 设置 trace 事件下沉回调。
 * @param sink 事件回调；传入空指针表示关闭 trace 输出。
 * @param user 传递给 sink 的用户指针，允许为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示设置成功。
 * @example
 * MRT_TraceSetSink(TraceSink, NULL);
 */
MRT_Result MRT_TraceSetSink(MRT_TraceSink sink, void *user)
{
    /* 保存调用方提供的事件下沉回调；为空时表示禁用 trace。 */
    g_trace_sink = sink;

    /* 保存用户指针，后续事件发布时原样传回。 */
    g_trace_user = user;

    /* 设置过程不分配资源，因此总是成功。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 由内核模块发布一个 trace 事件。
 * @param event 待发布的事件快照，不能为空。
 * @return void 无返回值。
 * @example
 * MRT_TraceEvent event = {0};
 * event.kind = MRT_TRACE_EVENT_QUEUE_SEND;
 * MRT_TraceEmit(&event);
 */
void MRT_TraceEmit(const MRT_TraceEvent *event)
{
    /* 未设置 sink 时静默丢弃事件，保持最小系统零外设依赖。 */
    if (g_trace_sink == 0) {
        /* 没有输出后端，直接返回。 */
        return;
    }

    /* 事件指针不能为空，否则 sink 无法读取事件内容。 */
    if (event == 0) {
        /* 非法事件不发布。 */
        return;
    }

    /* 调用用户 sink，并传回设置时保存的用户指针。 */
    g_trace_sink(event, g_trace_user);
}
