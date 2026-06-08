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
| C-017 | 软件定时器 + 命令队列 | 启动/停止/复位命令排队 | 服务任务按序处理命令 | host 单测 | 已验证：`test_timer_service_task` 覆盖启动命令只入队、服务入口运行后才激活定时器、到期事件只入队且回调由服务入口执行、启动后改周期按 FIFO 生效；`test_timer_control` 覆盖启动/停止/复位/改周期命令处理前后状态；`test_timer_pending_function` 覆盖共享服务队列 pending function FIFO、参数传递、满队列和 drain；`test_event_timer_dynamic_allocation` 覆盖动态定时器删除会清除该定时器未处理命令/事件，删除后 tick 推进不再触发回调 |
| C-018 | 软件定时器 + 调度 | 定时器到期 | 回调在服务任务上下文执行 | 调度仿真 | 已验证：`test_timer_tick_expiry` 覆盖 `MRT_KernelTick` 只投递单次/自动重载/多定时器到期事件，不在 tick 路径直接运行用户回调；`MRT_TimerServiceRunPending` 才按 FIFO 执行回调，并保持自动重载下一次到期点正确 |
| C-019 | 软件定时器 + tickless | 睡眠期间定时器到期 | 唤醒后补偿 tick 并投递回调事件 | 端口 mock | 已验证：`test_tickless_expected_idle` 覆盖任务/定时器最近 deadline 估算需先处理服务启动命令；`test_tickless_timer_compensation` 覆盖 mock 端口睡眠后补偿 tick、到期事件先入服务队列、服务入口运行后执行软件定时器回调、唤醒延时任务、遵守最大睡眠 tick 限制和无 deadline 跳过睡眠 |
| C-020 | 流缓冲 + 环绕 | 写指针环绕后读取 | 数据顺序保持正确 | host 单测 | 已验证：`test_stream_buffer_send_receive` 覆盖写入、读取、再写入触发环形回绕后仍按 FIFO 顺序读出；`test_buffer_dynamic_allocation` 覆盖动态流缓冲创建后可正常写入和读取字节流，并覆盖满缓冲写者等待、删除 busy 和读出释放空间后写者唤醒 |
| C-021 | 流缓冲 + ISR | ISR 写入，任务阻塞读 | 任务被唤醒并读到数据 | host/mock ISR | 已验证：`test_stream_buffer_isr_wakes_reader` 覆盖高优先级读者阻塞、ISR 写入达到触发水位、读者回到 ready、`should_yield=true`；`test_stream_buffer_isr` 覆盖 ISR 非阻塞收发和非法上下文 |
| C-022 | 消息缓冲 + 容量 | 剩余空间不足以放完整消息 | 写入失败，不产生半包 | host 单测 | 已验证：`test_message_buffer_send_receive` 覆盖整包边界、小输出不移除消息、剩余空间不足不写半包；`test_message_buffer_isr` 覆盖 ISR 容量拒绝、小输出保持消息和读者唤醒；`test_buffer_dynamic_allocation` 覆盖动态消息缓冲创建后可正常收发完整消息，并覆盖满缓冲写者等待、删除 busy 和读出完整消息后写者唤醒 |
| C-023 | 内存堆 + 对象创建 | heap 分配失败 | API 返回资源不足，内部状态不变 | host 单测 | 已验证：`test_heap_linear` 覆盖堆耗尽分配返回 NULL；`test_queue_dynamic_allocation` 覆盖动态队列创建失败返回 `MRT_RESULT_NO_MEMORY` 且堆空闲水位不变；`test_task_dynamic_allocation` 覆盖动态任务创建成功释放、创建失败清空句柄且堆空闲水位不变；`test_sync_dynamic_allocation` 覆盖动态信号量/互斥锁成功创建、失败清空句柄、静态删除拒绝、忙删除保护和释放后 heap 水位恢复；`test_event_timer_dynamic_allocation` 覆盖动态事件组/定时器成功创建、失败清空句柄、静态删除拒绝、事件等待者忙保护、活动定时器删除先停止且释放 heap；`test_buffer_dynamic_allocation` 覆盖动态流/消息缓冲创建成功、创建失败 heap 水位不变、动态删除释放 heap、空删除参数拒绝、静态对象删除拒绝和存在等待写者时拒绝删除 |
| C-024 | 内存堆 + 释放合并 | 释放相邻块 | 空闲块合并，碎片减少 | host 单测 | 已验证：`test_heap_coalescing` 覆盖释放两个相邻块后分配大于任一单块的请求成功，并验证普通 free-list 模式仍不合并 |
| C-025 | trace + 任务切换 | trace 开启 | hook 收到切换事件，不改变调度结果 | host 单测 | 已验证：`test_trace_task_switch` 覆盖高优先级任务延时切到低优先级任务、tick 到期后低优先级切回高优先级，并验证 trace 事件中的旧任务、新任务和 tick |
| C-026 | trace + 队列 | 队列 send/receive | hook 收到事件，不改变队列数据 | host 单测 | 已验证：`test_trace_queue` 覆盖队列 send/receive 后 trace sink 收到事件，队列数据 FIFO 语义保持不变，事件 value 记录操作后队列水位 |
| C-027 | 断言 + 非法上下文 | ISR 调用禁止 API | 触发断言或返回非法上下文 | host/mock ISR | 部分验证：`test_semaphore_isr`、`test_event_group_isr`、`test_task_notify_isr`、`test_task_lifecycle` 覆盖任务上下文调用 FromISR API 返回 `MRT_RESULT_INVALID_CONTEXT`；`test_mutex_create_lock` 覆盖无当前任务调用互斥锁 API 返回 `MRT_RESULT_INVALID_CONTEXT`；`test_assert_hook` 覆盖 `MRT_ASSERT` 与 `MRT_AssertFailed` 会把表达式、文件、行号分发给统一 hook。ISR 调用非 ISR-safe API 后触发断言的强制策略仍待后续 API 约束收敛 |
| C-028 | STM32 端口 + tick | SysTick 调用内核 tick | tick 推进并按需触发 PendSV | 端口 mock/smoke | 部分验证：`test_kernel_tick` 已验证 tick 推进与 kernel yield 转发；`test_port_stm32_tick_priority` 验证 SysTick reload 计算、24 位上限和参数校验；`test_task_stack_top` 验证调度切换前后的 TCB 栈顶保存/返回契约；`python tools\verify\check_embedded_smoke_projects.py` 已使用 ARM GCC 交叉编译 `examples/stm32`，其中 `SysTick_Handler` 调用 `MRT_KernelTick()`、`USART1_IRQHandler` 使用 `MRT_QueueSendFromISR()` 并把 `should_yield` 交给 `MRT_PortYieldFromISR(should_yield)`，`main.c` 使用 `MRT_PortStm32CmInitializeStack()` 与 `MRT_TaskKernelSetStackTop()` 写入初始 PSP；真实 PendSV/SVC 汇编运行待板级 smoke 补证 |
| C-029 | STM32 端口 + 临界区 | 嵌套进入临界区 | 中断屏蔽状态可恢复 | 端口 mock/smoke | 部分验证：`test_port_mock` 已验证 mock 临界区嵌套恢复；`test_port_stm32_tick_priority` 验证 BASEPRI 左对齐编码和非法 0 优先级拒绝；`examples/stm32/mrt_port_stm32_smoke.c` 通过 ARM GCC 编译，包含 PRIMASK 保存/恢复、PendSV pending、tickless WFI scaffold 和 `MRT_TaskKernelSwitchStackTop()` PSP/TCB 接线；`examples/stm32/main.c` 通过 ARM GCC 编译，包含初始栈帧写回 TCB 接线；真实 PRIMASK/BASEPRI 硬件行为仍待板级 smoke 补证 |
| C-030 | DSP 端口 + 栈初始化 | 创建任务栈帧 | 栈顶满足对齐和入口参数规则 | 端口 mock/smoke | 已验证：`test_port_dsp_stack` 覆盖 DSP C28x 风格向下增长栈、8 字节对齐、入口 PC、入口参数、退出处理函数、状态字和 XAR4-XAR7 保存槽占位；`python tools\verify\check_embedded_smoke_projects.py` 编译并运行 `examples/dsp`，再次调用 `MRT_PortDspC28xInitializeStack()` 验证 smoke 入口 |
| C-031 | DSP 端口 + 上下文切换 | 触发软件中断切换 | 保存/恢复接口调用顺序正确 | 端口 mock/smoke | 已验证：`test_port_dsp_context` 覆盖任务上下文请求/确认、ISR 嵌套期间延迟切换、最外层 ISR 退出提示切换、退出下溢和空输出参数拒绝；`examples/dsp/mrt_port_dsp_model.c` 通过 host smoke model 验证 ISR enter/exit、`MRT_PortYieldFromISR(true)`、软件中断请求和 tickless sleep 模型；真实 DSP 汇编保存/恢复顺序待具体芯片端口补证 |
| C-032 | 手册 + API | 每个 public API | 手册有原型、参数、返回值、示例、上下文限制 | 文档检查 | 已验证：`python tools\verify\check_api_manual_coverage.py` 覆盖 `docs/api/myrtos_api_catalog.md` 中 135 个 API 条目，并检查每个条目包含函数原型、功能说明、参数、返回值、调用上下文、阻塞行为、ISR 限制、配置宏、调用示例、常见错误；`python tools\verify\check_api_catalog_prototypes.py` 验证 API 目录、公共头文件和源文件定义三方 135 个原型对齐；同手册脚本验证 STM32/DSP 移植章节包含移植前准备、工程分层、关键接入顺序、首次联调、板级验收、工具链、启动文件、向量表、tick、上下文切换、栈布局、临界区、低功耗、示例、排错；本阶段手册补充 `MRT_TimerGetName` 与 `MRT_MemoryPoolGetFreeCount` 章节，并保留 `examples/stm32`、`examples/dsp` smoke 工程落地步骤、构建命令、板级证据要求和真实硬件边界 |
| C-033 | 注释 + 源码 | 每个函数 | 有中文函数头说明和内部步骤注释 | 静态扫描 | 已验证：`python tools\verify\check_chinese_comments.py` 覆盖 `include/`、`src/`、`examples/` 与 `tests/` 函数头中文 Doxygen 字段和函数体附近中文步骤注释；`python tools\verify\check_original_symbols.py` 验证源码和手册未出现 banned FreeRTOS-style public symbols |
| C-034 | 运行统计 + tick + 调度 | 当前任务跨 tick 运行，随后阻塞并切换到其他任务 | 每个 kernel tick 归属到 tick 到来前的当前运行任务；任务删除后拒绝统计查询 | host 耦合测试 | 已验证：`test_runtime_stats` 覆盖高优先级任务连续运行两个 tick 后累计 2，阻塞期间低优先级任务累计 2，高优先级任务唤醒后继续累计；同时覆盖空任务、空输出和已删除任务查询返回 `MRT_RESULT_INVALID_ARGUMENT` |
| C-035 | STM32 端口 + MPU 布局 | 任意内存范围需要转换为 MPU 可表达区域 | 规整结果为覆盖原始范围的 power-of-two 区域，且基址按区域大小向下对齐 | 端口 mock | 已验证：`test_port_stm32_mpu` 覆盖非法参数、最小 32 字节区域、已对齐 power-of-two 区域保持不变，以及 `0x20001234 + 6000` 字节范围规整为 `0x20000000 + 16384` 字节且 shift 为 14 |
| C-036 | 流缓冲 + 写者等待 + 动态删除 | 满流缓冲上高优先级写者带 timeout 写入，随后删除对象 | 写者进入 `waiting_writers`，删除返回 `MRT_RESULT_OBJECT_BUSY`；读出释放至少 1 字节后唤醒写者并清空等待字节记录 | host 耦合测试 | 已验证：`test_buffer_dynamic_allocation` 覆盖动态流缓冲满载后 `MRT_StreamBufferSend(..., timeout>0)` 进入 `MRT_TASK_WAIT_REASON_STREAM_SEND`，`object_wait_bytes=1`，删除 busy；后台任务读出 1 字节后 waiting writer 被唤醒并允许后续动态删除释放 heap |
| C-037 | 消息缓冲 + 写者等待 + 动态删除 | 满消息缓冲上高优先级写者带 timeout 写入完整消息，随后删除对象 | 写者进入 `waiting_writers`，删除返回 `MRT_RESULT_OBJECT_BUSY`；读出完整消息后按完整记录长度唤醒写者 | host 耦合测试 | 已验证：`test_buffer_dynamic_allocation` 覆盖动态消息缓冲满载后 `MRT_MessageBufferSend(..., timeout>0)` 进入 `MRT_TASK_WAIT_REASON_MESSAGE_SEND`，`object_wait_bytes=4+payload`，删除 busy；后台任务读出完整消息后 waiting writer 被唤醒并允许后续动态删除释放 heap |
| C-038 | 硬件 smoke 采集流水线 + 证据生成 | 预检、构建、烧录、采集、raw log 生成最终证据和目标级校验 | 默认 dry-run 不执行硬件命令；显式 `--execute` 才运行真实命令；生成器和目标级校验按顺序执行 | 静态工具测试 | 已验证：`test_hardware_smoke_capture_runner.py` 覆盖 dry-run 顺序、无害执行、生成器去重和目标级证据校验；真实 STM32/DSP 板级 PASS 仍待真实硬件补证 |
| C-039 | DSP C2000 smoke 工程 + 启动/链接/ISR glue | 从 host DSP model 迁移到真实 C2000 板级工程 | 启动向量、CPU Timer0 tick、软件中断上下文切换、ADC/DMA FromISR 队列、RTOS heap/任务栈/DMA/trace 分区均有可审计骨架，且明确不替代真实板级 smoke | 静态检查/smoke 路径检查 | 已验证：`test_dsp_c2000_project_scaffold.py` 检查 `startup_c28x.c`、`mrt_port_dsp_c2000_smoke.c`、`linker_c28x.cmd` 的入口符号、tick/FromISR/软件中断接线和 RTOS 分区标记；`check_embedded_smoke_projects.py` 把这些文件纳入 required path 检查；真实 TI 工具链编译和 DSP 板级运行仍待补证 |
| C-040 | 硬件 smoke 原始日志 schema + 证据生成器 + 示例 C 头文件 | 真实板级 UART/trace 字段由文档、生成器和板级输出端共同使用 | `COMMON_REQUIRED_FIELDS`、STM32/DSP 专属字段、`raw_log_schema.md`、`mrt_hardware_smoke_log_schema.h` 和采集指南必须保持一致；release 默认链路必须包含该检查 | 静态检查 | 已验证：`test_hardware_smoke_raw_log_schema.py` 和 `check_hardware_smoke_raw_log_schema.py` 覆盖字段存在性、指南链接和 release runner 步骤；该检查只证明字段契约一致，不替代真实板级 smoke |
| C-041 | 硬件 smoke 日志输出 helper + raw-log schema | STM32/DSP 板级代码需要输出 `Key: Value` 字符串字段和十进制整数字段 | helper 输出必须精确为 `Key: Value\n`；十进制整数不带单位；非法参数必须返回错误且不留下半行日志，避免误导证据生成器 | host 单测 | 已验证：`test_hardware_smoke_log` 覆盖 `MRT_SmokeLogWritePair()`、`MRT_SmokeLogWriteU32()`、字段常量联用和非法参数无输出；该 helper 只格式化日志，不判断 PASS/FAIL，也不替代真实 STM32/DSP 板级 smoke |
| C-042 | 硬件 smoke 完整报告 emitter + raw-log schema | STM32/DSP 板级代码一次性输出 common 字段和目标专属字段 | emitter 必须先完整校验所有字符串字段，再输出通用字段和 STM32/DSP 专属字段；缺字段或空 writer 时必须返回错误且不留下半份 raw log | host 单测 | 已验证：`test_hardware_smoke_report` 覆盖 `MRT_SmokeEmitStm32Report()` 输出 STM32 全部必填字段、`MRT_SmokeEmitDspReport()` 输出 DSP 全部必填字段、非法参数无输出；该 emitter 只序列化调用方提供的结果，不生成真实板级 PASS |

## 最新增量证据：STM32 上下文切换入口

- `C-028`：新增 `test_stm32_context_scaffold.py`，检查 `SVC_Handler`、`PendSV_Handler`、`MRT_PortStm32CmStartFirstTaskAsm`、PSP 读取、R4-R11 保存/恢复、C 钩子转交、`MRT_TaskKernelSwitchStackTop()` 接线，以及 `main.c` 的初始 PSP 写回 TCB 接线；`check_embedded_smoke_projects.py` 已把 `mrt_port_stm32_cm_context.S` 纳入 ARM GCC 交叉编译。
- `C-029`：`examples/stm32/mrt_port_stm32_smoke.c` 新增 `MRT_PortStm32CmSvcHook()` 和 `MRT_PortStm32CmPendSvHook()` 记录异常帧、PSP 和 EXC_RETURN，PendSV C 钩子现在通过 TCB 栈顶契约保存旧 PSP 并返回当前任务 PSP；`examples/stm32/main.c` 现在为 LED/UART 任务构造 Cortex-M 初始异常帧并写回 TCB；`test_task_stack_top` 覆盖静态/动态任务初始栈顶、非法输入和调度切换保存契约。当前仍不替代真实 PRIMASK/BASEPRI 和 PendSV 运行验证。

## 最新增量证据：硬件 smoke 预检配置

- `C-032`：新增 `tools/verify/check_hardware_smoke_preflight.py`、`tests/static/test_hardware_smoke_preflight.py` 和 `docs/verification/hardware_smoke/hardware_smoke_preflight.json`，用于在真实 STM32/DSP 采集前检查目标配置、采集命令、最短运行时长、必需日志字段和可选工具链可用性；该检查只证明采集前置配置完整，不替代真实板级运行证据。
- `C-032`：`tools/verify/generate_hardware_smoke_evidence.py` 现在从原始日志派生 `Raw-Log-Path` 和 `Raw-Log-SHA256`，`tools/verify/check_hardware_smoke_evidence.py` 会读取原始日志并拒绝 SHA-256 错配；`tests/static/test_hardware_smoke_evidence_generator.py` 与 `tests/static/test_hardware_smoke_evidence_checker.py` 覆盖该追溯链路。
- `C-040`：`docs/verification/hardware_smoke/raw_log_schema.md` 给出 STM32/DSP 原始日志字段契约，`examples/hardware_smoke/mrt_hardware_smoke_log_schema.h` 给出 C 输出端字段常量，`tools/verify/check_hardware_smoke_raw_log_schema.py` 防止字段漂移。

## 后续落地

- `tests/unit/`：基础算法与单模块。
- `tests/sim/`：调度仿真与任务交互。
- `tests/coupling/`：矩阵中的跨模块场景。
- `tests/port_mock/`：STM32/DSP 端口抽象。
- `examples/stm32/`：板级 smoke test。
- `examples/dsp/`：DSP mock/真实端口示例。

## 最新增量证据：硬件 smoke 采集执行器

- `C-038`：新增 `tools/verify/run_hardware_smoke_capture.py` 和 `tests/static/test_hardware_smoke_capture_runner.py`，把硬件 smoke 预检、编译器检查、构建、烧录、采集、raw log 到最终证据生成、目标级证据校验连成流水线。
- `C-038`：默认模式只执行 dry-run，打印步骤顺序，不运行真实硬件命令；只有显式追加 `--execute` 才会调用 `hardware_smoke_preflight.json` 中的命令。
- `C-038`：静态测试使用临时无害命令写入 raw log，再由 `generate_hardware_smoke_evidence.py` 生成带 `Raw-Log-Path` 和 `Raw-Log-SHA256` 的证据，最后调用目标级证据校验；该测试证明流水线契约，不替代真实 STM32/DSP 板级 PASS。

## 最新增量证据：DSP C28x 汇编骨架

- `C-031`：新增 `src/portable/dsp_c28x/mrt_port_dsp_c28x_context.asm`，记录 DSP 首任务启动、软件中断 yield、软件中断切换入口、XAR4-XAR7、ST0/ST1、ACC、P、XT 保存和反向恢复顺序。
- `C-031`：新增 `tests/static/test_dsp_context_scaffold.py`，检查汇编骨架入口符号、C 钩子交接、保存/恢复标记和 `examples/dsp/README.md` 中的真实板级边界说明。
- `C-031`：当前证据证明 DSP 上下文汇编已经有可审计模板，不证明具体 TI/ADI DSP 工具链编译通过，也不证明真实板级上下文切换已经运行。

## 最新增量证据：DSP C2000 工程骨架

- `C-039`：新增 `examples/dsp/startup_c28x.c`，记录 C2000 timer、软件中断和 ADC/DMA ISR 向量占位与安装顺序。
- `C-039`：新增 `examples/dsp/mrt_port_dsp_c2000_smoke.c`，记录 `MRT_Port*` 公共端口、`MRT_KernelTick()`、`MRT_QueueSendFromISR()`、`MRT_PortYieldFromISR()`、C28x 汇编钩子和任务栈顶切换契约。
- `C-039`：新增 `examples/dsp/linker_c28x.cmd`，记录 `.mrtos_heap`、`.mrtos_tasks`、`.mrtos_dma`、`.mrtos_trace` 分区名称和真实 map 文件检查点。
- `C-039`：新增 `tests/static/test_dsp_c2000_project_scaffold.py` 并纳入 release runner；当前证据证明 C2000 工程文件清单和接线顺序可审计，不证明 TI 工具链编译或真实 DSP 板级 smoke 已通过。

## 最新增量证据：硬件 smoke raw-log schema

- `C-040`：新增 `docs/verification/hardware_smoke/raw_log_schema.md`，集中列出通用字段、STM32 字段、DSP 字段、示例日志和字段填写规则。
- `C-040`：新增 `examples/hardware_smoke/mrt_hardware_smoke_log_schema.h`，让真实板级 UART/trace 输出端复用 `MRT_SMOKE_FIELD_*` 字段常量。
- `C-040`：新增 `tools/verify/check_hardware_smoke_raw_log_schema.py` 和 `tests/static/test_hardware_smoke_raw_log_schema.py`，并纳入默认 release runner；该检查证明字段契约一致，但不生成或接受真实板级 PASS 证据。

## 最新增量证据：硬件 smoke 日志输出 helper

- `C-041`：新增 `examples/hardware_smoke/mrt_hardware_smoke_log.h`，提供不依赖 `printf` 的轻量字符输出 helper，板级 UART/SWO/trace 回调只需实现单字符写入。
- `C-041`：新增 `tests/unit/test_hardware_smoke_log.c` 并纳入 host test runner 与 CMake，验证字符串字段、十进制整数和非法参数路径，确保失败路径不会写出半行 `Key: Value` 日志。
- `C-041`：该 helper 与 `mrt_hardware_smoke_log_schema.h` 联用，只负责格式化字段；真实 `Evidence-Status: PASS` 仍必须来自实际 STM32/DSP 板级运行和硬件证据 gate。

## 最新增量证据：硬件 smoke 完整报告 emitter

- `C-042`：新增 `examples/hardware_smoke/mrt_hardware_smoke_report.h`，把 common 字段、STM32 专属字段和 DSP 专属字段组织成可复用结构体，并复用日志 helper 输出完整 raw log。
- `C-042`：新增 `tests/unit/test_hardware_smoke_report.c` 并纳入 host test runner 与 CMake，验证 STM32/DSP 必填字段全集输出和非法参数无半份报告输出。
- `C-042`：该 emitter 只减少真机采集漏字段风险，不判断 `Evidence-Status`，不替代真实 `stm32_board_smoke.md`、`dsp_board_smoke.md` 和匹配 raw log。

