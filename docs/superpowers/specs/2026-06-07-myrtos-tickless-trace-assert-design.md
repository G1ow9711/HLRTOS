# MyRTOS Tickless/Trace/Assert Design

## 目标
本阶段补齐低功耗 tickless idle、内核 trace hook、统一断言 hook 三类可观测与调试能力。实现必须保持原创，只借鉴 FreeRTOS 的模块职责划分思想，不复制其源码、宏名或手册表述。

## 方案
1. Tickless idle 由内核提供 `MRT_TicklessGetExpectedIdleTicks` 与 `MRT_TicklessEnterIdle`。前者综合任务延时链表与软件定时器活动链表，返回当前 tick 到最近唤醒点的距离；后者调用移植层睡眠接口，由移植层回报真实睡眠 tick 数，再用内核 tick 补偿函数逐 tick 推进任务与定时器。
2. Trace hook 由独立 `mrt_trace` 模块管理事件下沉回调。任务切换、队列发送、队列接收等路径只发布轻量事件，不依赖具体日志后端，便于 host 测试、串口日志或嵌入式 Trace 工具接入。
3. Assert hook 由独立 `mrt_assert` 模块管理失败回调。`MRT_ASSERT(expr)` 记录表达式、文件、行号，默认不陷入死循环；真实移植时可在 hook 中关中断、打印、断点或复位。

## 边界
- Tickless 只在 host/kernel 层验证补偿语义，不实现 STM32/DSP 的真实低功耗寄存器代码；真实端口步骤留给后续 port/manual 阶段。
- Trace sink 允许为空；未设置 sink 时事件被静默丢弃，避免给小型 MCU 增加强依赖。
- Assert hook 只统一断言入口，不批量改造所有历史 API 的参数检查。

## 测试
- `test_tickless_expected_idle` 验证空闲距离、任务延时、定时器到期和 tick 回绕。
- `test_tickless_timer_compensation` 验证 mock 端口睡眠后内核 tick 增加、任务超时唤醒、定时器回调执行。
- `test_trace_task_switch` 验证任务切换 trace。
- `test_trace_queue` 验证队列发送/接收 trace。
- `test_assert_hook` 验证断言 hook 收到表达式、文件和行号。

## 验收
- `python tools\run_host_tests.py` 全量通过。
- 验证矩阵更新 `C-019`、`C-025`、`C-026`、`C-027`。
- 本分支提交并推送到 `origin/feature/tickless-trace-hooks`。
