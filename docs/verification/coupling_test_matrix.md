# MyRTOS 耦合测试矩阵

> 目的：定义“所有耦合情况要测试清楚”的最小可执行边界。  
> 状态：草案，后续实现时每行变成一个或多个自动化测试。

## 覆盖原则

- 每个模块至少覆盖：正常路径、阻塞路径、超时路径、非法上下文、资源不足。
- 每个 ISR API 至少覆盖：不触发切换、触发切换、参数错误。
- 每个等待对象至少覆盖：单任务等待、多任务等待、超时与事件同时发生。
- 每个动态创建 API 至少覆盖：分配成功、分配失败、删除释放。
- 每个 trace hook 至少覆盖：开启后调用、关闭后无副作用。

## 矩阵

| 编号 | 耦合模块 | 场景 | 预期行为 | 测试层级 | 当前状态 |
|------|----------|------|----------|----------|----------|
| C-001 | 任务 + 调度器 | 高优先级任务变为就绪 | 立即抢占低优先级任务 | 调度仿真 | 已验证：`test_scheduler_start` 验证启动选择最高优先级；`test_task_delay` 验证高优先级延时到期后抢占低优先级；`test_task_lifecycle` 验证恢复挂起任务、动态修改优先级后重排 ready list 并抢占 |
| C-002 | 任务 + tick | 任务延时到期 | 从延时链表进入就绪链表 | 调度仿真 | 已验证：`test_task_delay` 验证 3 tick 延时到期后 blocked -> running；`test_task_lifecycle` 验证 `MRT_TaskDelayUntil` 推进周期基准并在绝对 tick 到期后唤醒 |
| C-003 | 任务 + tick 溢出 | tick 计数翻转前后延时 | 延时到期顺序仍正确 | 调度仿真 | 已验证：`test_task_delay_overflow` 验证 UINT32_MAX 附近延时跨回绕后正确唤醒 |
| C-004 | 队列 + 任务阻塞 | 接收空队列并等待 | 任务阻塞，发送后唤醒 | host 单测 | 已验证：`test_queue_send_wakes_receiver` 验证高优先级接收任务阻塞后由低优先级发送唤醒并抢占 |
| C-005 | 队列 + 超时 | 接收空队列直到超时 | 返回超时，任务恢复就绪 | host 单测 | 已验证：`test_queue_task_timeout` 验证 3 tick timeout、等待链表清理、任务恢复 running |
| C-006 | 队列 + ISR | ISR 发送到空队列 | 等待任务唤醒，按需请求切换 | host/mock ISR | 已验证：`test_queue_isr` 覆盖 ISR 非阻塞收发；`test_queue_send_wakes_receiver` 覆盖 ISR 发送唤醒接收任务并设置 `should_yield=true` |
| C-007 | 队列 + 内存 | 动态创建队列时堆不足 | 返回资源不足，不泄漏 | host 单测 | 已验证：`test_queue_dynamic_allocation` 覆盖小堆上动态队列创建失败返回 `MRT_RESULT_NO_MEMORY`、输出句柄清空、`MRT_HeapGetFreeSize` 前后不变 |
| C-008 | 二值信号量 + ISR | ISR give 信号量 | 等待任务唤醒 | host/mock ISR | 已验证：`test_semaphore_isr` 覆盖无等待者 give 后计数增加且不切换、满信号量返回 `MRT_RESULT_OBJECT_FULL`、任务上下文调用返回 `MRT_RESULT_INVALID_CONTEXT`、唤醒等待任务并设置 `should_yield=true` |
| C-009 | 计数信号量 + 边界 | give 超过最大计数 | 返回对象状态错误或饱和策略结果 | host 单测 | 已验证：`test_semaphore_take_give` 覆盖计数信号量 give 成功增加计数和满计数返回 `MRT_RESULT_OBJECT_FULL`；`test_semaphore_create_static` 覆盖初始计数大于最大计数时拒绝创建 |
| C-010 | 互斥锁 + 优先级继承 | 低优先级持锁，高优先级等待 | 持锁任务继承高优先级 | 调度仿真 | 已验证：`test_mutex_priority_inheritance` 覆盖低优先级持锁、高优先级等待、拥有者有效优先级提升、解锁后所有权转交和基础优先级恢复 |
| C-011 | 互斥锁 + 超时 | 高优先级等待互斥锁超时 | 持锁任务优先级正确回滚 | 调度仿真 | 已验证：`test_mutex_timeout_rollback` 覆盖低优先级任务持锁、高优先级任务等待并触发优先级继承、等待者 3 tick 超时离开等待链表、拥有者有效优先级回落到基础值且互斥锁所有权保持不变 |
| C-012 | 互斥锁 + 任务删除 | 删除持锁任务 | 互斥锁状态和等待任务处理符合规则 | 调度仿真 | 已验证：`test_mutex_task_delete_policy` 覆盖低优先级任务持锁、高优先级任务等待并触发继承后，删除持锁任务返回 `MRT_RESULT_OBJECT_BUSY`，当前任务、任务状态、互斥锁拥有者和等待链表均保持不变 |
| C-013 | 递归互斥锁 + 所有权 | 非拥有者释放 | 返回非法状态，不改变计数 | host 单测 | 已验证：`test_mutex_recursive` 覆盖递归互斥锁非拥有者释放返回 `MRT_RESULT_OWNER_ERROR`，拥有者保持不变，递归深度保持 2 |
| C-014 | 事件组 + 多等待者 | set bits 满足多个任务条件 | 所有满足条件任务被唤醒 | host 单测 | 已验证：`test_event_group_set_wakes_tasks` 覆盖同一事件组上 wait-any 与 wait-all 多等待者，`MRT_EventGroupSetBits` 基于置位后快照唤醒所有匹配任务，并让最高优先级等待者抢占 |
| C-015 | 事件组 + 清位 | wait-any 且退出清位 | 返回原始事件值后清除指定 bit | host 单测 | 已验证：`test_event_group_wait_immediate` 覆盖 wait-any clear-on-exit 返回清位前快照并清除匹配 bit；`test_event_group_set_wakes_tasks` 覆盖多等待者匹配后统一清除匹配 bit 并保留无关 bit |
| C-016 | 任务通知 + 覆盖策略 | no-overwrite 遇到未读通知 | 返回对象忙，不覆盖旧值 | host 单测 | 已验证：`test_task_notify_actions` 覆盖 pending 通知下 `MRT_NOTIFY_NO_OVERWRITE` 返回 `MRT_RESULT_OBJECT_BUSY` 且旧值不变；`test_task_notify_isr` 覆盖 ISR no-overwrite busy 同样不改旧值 |
| C-017 | 软件定时器 + 命令队列 | 启动/停止/复位命令排队 | 服务任务按序处理命令 | host 单测 | 部分验证：`test_timer_control` 覆盖启动/停止/复位/改周期控制语义，`test_timer_pending_function` 覆盖 deterministic service-shim pending FIFO、参数传递、满队列和 drain；`test_event_timer_dynamic_allocation` 覆盖动态定时器启动后删除会先停止再释放，删除后 tick 推进不再触发回调；真正独立 timer service task 与异步命令队列将在后续 scheduler/service 增强计划补测 |
| C-018 | 软件定时器 + 调度 | 定时器到期 | 回调在服务任务上下文执行 | 调度仿真 | 已验证：`test_timer_tick_expiry` 覆盖 `MRT_KernelTick` 驱动单次定时器到期一次、自动重载定时器在 tick 2/tick 4 触发并保持活动、多个定时器按到期 tick 顺序执行；当前 host 模型使用 timer service shim，真实服务任务上下文待后续增强 |
| C-019 | 软件定时器 + tickless | 睡眠期间定时器到期 | 唤醒后补偿 tick 并执行回调 | 端口 mock | 已验证：`test_tickless_expected_idle` 覆盖任务/定时器最近 deadline 估算；`test_tickless_timer_compensation` 覆盖 mock 端口睡眠后补偿 tick、执行软件定时器回调、唤醒延时任务、遵守最大睡眠 tick 限制和无 deadline 跳过睡眠 |
| C-020 | 流缓冲 + 环绕 | 写指针环绕后读取 | 数据顺序保持正确 | host 单测 | 已验证：`test_stream_buffer_send_receive` 覆盖写入、读取、再写入触发环形回绕后仍按 FIFO 顺序读出；`test_buffer_dynamic_allocation` 覆盖动态流缓冲创建后可正常写入和读取字节流 |
| C-021 | 流缓冲 + ISR | ISR 写入，任务阻塞读 | 任务被唤醒并读到数据 | host/mock ISR | 已验证：`test_stream_buffer_isr_wakes_reader` 覆盖高优先级读者阻塞、ISR 写入达到触发水位、读者回到 ready、`should_yield=true`；`test_stream_buffer_isr` 覆盖 ISR 非阻塞收发和非法上下文 |
| C-022 | 消息缓冲 + 容量 | 剩余空间不足以放完整消息 | 写入失败，不产生半包 | host 单测 | 已验证：`test_message_buffer_send_receive` 覆盖整包边界、小输出不移除消息、剩余空间不足不写半包；`test_message_buffer_isr` 覆盖 ISR 容量拒绝、小输出保持消息和读者唤醒；`test_buffer_dynamic_allocation` 覆盖动态消息缓冲创建后可正常收发完整消息 |
| C-023 | 内存堆 + 对象创建 | heap 分配失败 | API 返回资源不足，内部状态不变 | host 单测 | 已验证：`test_heap_linear` 覆盖堆耗尽分配返回 NULL；`test_queue_dynamic_allocation` 覆盖动态队列创建失败返回 `MRT_RESULT_NO_MEMORY` 且堆空闲水位不变；`test_task_dynamic_allocation` 覆盖动态任务创建成功释放、创建失败清空句柄且堆空闲水位不变；`test_sync_dynamic_allocation` 覆盖动态信号量/互斥锁成功创建、失败清空句柄、静态删除拒绝、忙删除保护和释放后 heap 水位恢复；`test_event_timer_dynamic_allocation` 覆盖动态事件组/定时器成功创建、失败清空句柄、静态删除拒绝、事件等待者忙保护、活动定时器删除先停止且释放 heap；`test_buffer_dynamic_allocation` 覆盖动态流/消息缓冲创建成功和创建失败 heap 水位不变 |
| C-024 | 内存堆 + 释放合并 | 释放相邻块 | 空闲块合并，碎片减少 | host 单测 | 已验证：`test_heap_coalescing` 覆盖释放两个相邻块后分配大于任一单块的请求成功，并验证普通 free-list 模式仍不合并 |
| C-025 | trace + 任务切换 | trace 开启 | hook 收到切换事件，不改变调度结果 | host 单测 | 已验证：`test_trace_task_switch` 覆盖高优先级任务延时切到低优先级任务、tick 到期后低优先级切回高优先级，并验证 trace 事件中的旧任务、新任务和 tick |
| C-026 | trace + 队列 | 队列 send/receive | hook 收到事件，不改变队列数据 | host 单测 | 已验证：`test_trace_queue` 覆盖队列 send/receive 后 trace sink 收到事件，队列数据 FIFO 语义保持不变，事件 value 记录操作后队列水位 |
| C-027 | 断言 + 非法上下文 | ISR 调用禁止 API | 触发断言或返回非法上下文 | host/mock ISR | 部分验证：`test_semaphore_isr`、`test_event_group_isr`、`test_task_notify_isr`、`test_task_lifecycle` 覆盖任务上下文调用 FromISR API 返回 `MRT_RESULT_INVALID_CONTEXT`；`test_mutex_create_lock` 覆盖无当前任务调用互斥锁 API 返回 `MRT_RESULT_INVALID_CONTEXT`；`test_assert_hook` 覆盖 `MRT_ASSERT` 与 `MRT_AssertFailed` 会把表达式、文件、行号分发给统一 hook。ISR 调用非 ISR-safe API 后触发断言的强制策略仍待后续 API 约束收敛 |
| C-028 | STM32 端口 + tick | SysTick 调用内核 tick | tick 推进并按需触发 PendSV | 端口 mock/smoke | 部分验证：`test_kernel_tick` 已验证 tick 推进与 kernel yield 转发；`test_port_stm32_tick_priority` 验证 SysTick reload 计算、24 位上限和参数校验；真实 SysTick_Handler/PendSV_Handler/SVC_Handler 接入待 STM32 手册和板级 smoke test 补证 |
| C-029 | STM32 端口 + 临界区 | 嵌套进入临界区 | 中断屏蔽状态可恢复 | 端口 mock | 部分验证：`test_port_mock` 已验证 mock 临界区嵌套恢复；`test_port_stm32_tick_priority` 验证 BASEPRI 左对齐编码和非法 0 优先级拒绝；真实 PRIMASK/BASEPRI 读写待 STM32 手册和板级 smoke test 补证 |
| C-030 | DSP 端口 + 栈初始化 | 创建任务栈帧 | 栈顶满足对齐和入口参数规则 | 端口 mock | 已验证：`test_port_dsp_stack` 覆盖 DSP C28x 风格向下增长栈、8 字节对齐、入口 PC、入口参数、退出处理函数、状态字和 XAR4-XAR7 保存槽占位 |
| C-031 | DSP 端口 + 上下文切换 | 触发软件中断切换 | 保存/恢复接口调用顺序正确 | 端口 mock | 已验证：`test_port_dsp_context` 覆盖任务上下文请求/确认、ISR 嵌套期间延迟切换、最外层 ISR 退出提示切换、退出下溢和空输出参数拒绝；真实 DSP 汇编保存/恢复顺序待具体芯片端口补证 |
| C-032 | 手册 + API | 每个 public API | 手册有原型、参数、返回值、示例、上下文限制 | 文档检查 | 已验证：`python tools\verify\check_api_manual_coverage.py` 覆盖 `docs/api/myrtos_api_catalog.md` 中 125 个 API 条目，并检查每个条目包含函数原型、功能说明、参数、返回值、调用上下文、阻塞行为、ISR 限制、配置宏、调用示例、常见错误；同脚本验证 STM32/DSP 移植章节包含工具链、启动文件、向量表、tick、上下文切换、栈布局、临界区、低功耗、示例、排错；本阶段手册补充动态对象精确原型、静态删除拒绝、等待者/持锁忙删除、活动定时器删除先停止、动态缓冲单堆块布局和当前无缓冲删除 API 的限制 |
| C-033 | 注释 + 源码 | 每个函数 | 有中文函数头说明和内部步骤注释 | 静态扫描 | 已验证：`python tools\verify\check_chinese_comments.py` 覆盖 `include/` 与 `src/` 函数头中文 Doxygen 字段和函数体附近中文步骤注释；`python tools\verify\check_original_symbols.py` 验证源码和手册未出现 banned FreeRTOS-style public symbols |
| C-034 | 运行统计 + tick + 调度 | 当前任务跨 tick 运行，随后阻塞并切换到其他任务 | 每个 kernel tick 归属到 tick 到来前的当前运行任务；任务删除后拒绝统计查询 | host 耦合测试 | 已验证：`test_runtime_stats` 覆盖高优先级任务连续运行两个 tick 后累计 2，阻塞期间低优先级任务累计 2，高优先级任务唤醒后继续累计；同时覆盖空任务、空输出和已删除任务查询返回 `MRT_RESULT_INVALID_ARGUMENT` |

## 后续落地

- `tests/unit/`：基础算法与单模块。
- `tests/sim/`：调度仿真与任务交互。
- `tests/coupling/`：矩阵中的跨模块场景。
- `tests/port_mock/`：STM32/DSP 端口抽象。
- `examples/stm32/`：板级 smoke test。
- `examples/dsp/`：DSP mock/真实端口示例。

