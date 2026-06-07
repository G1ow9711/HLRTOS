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
| C-001 | 任务 + 调度器 | 高优先级任务变为就绪 | 立即抢占低优先级任务 | 调度仿真 | 已验证：`test_scheduler_start` 验证启动选择最高优先级；`test_task_delay` 验证高优先级延时到期后抢占低优先级 |
| C-002 | 任务 + tick | 任务延时到期 | 从延时链表进入就绪链表 | 调度仿真 | 已验证：`test_task_delay` 验证 3 tick 延时到期后 blocked -> running |
| C-003 | 任务 + tick 溢出 | tick 计数翻转前后延时 | 延时到期顺序仍正确 | 调度仿真 | 已验证：`test_task_delay_overflow` 验证 UINT32_MAX 附近延时跨回绕后正确唤醒 |
| C-004 | 队列 + 任务阻塞 | 接收空队列并等待 | 任务阻塞，发送后唤醒 | host 单测 | 已验证：`test_queue_send_wakes_receiver` 验证高优先级接收任务阻塞后由低优先级发送唤醒并抢占 |
| C-005 | 队列 + 超时 | 接收空队列直到超时 | 返回超时，任务恢复就绪 | host 单测 | 已验证：`test_queue_task_timeout` 验证 3 tick timeout、等待链表清理、任务恢复 running |
| C-006 | 队列 + ISR | ISR 发送到空队列 | 等待任务唤醒，按需请求切换 | host/mock ISR | 已验证：`test_queue_isr` 覆盖 ISR 非阻塞收发；`test_queue_send_wakes_receiver` 覆盖 ISR 发送唤醒接收任务并设置 `should_yield=true` |
| C-007 | 队列 + 内存 | 动态创建队列时堆不足 | 返回资源不足，不泄漏 | host 单测 | 未实现：当前队列阶段仅支持静态创建；动态 heap 路径等待内存管理模块实现后补测 |
| C-008 | 二值信号量 + ISR | ISR give 信号量 | 等待任务唤醒 | host/mock ISR | 已验证：`test_semaphore_isr` 覆盖无等待者 give 后计数增加且不切换、满信号量返回 `MRT_RESULT_OBJECT_FULL`、任务上下文调用返回 `MRT_RESULT_INVALID_CONTEXT`、唤醒等待任务并设置 `should_yield=true` |
| C-009 | 计数信号量 + 边界 | give 超过最大计数 | 返回对象状态错误或饱和策略结果 | host 单测 | 已验证：`test_semaphore_take_give` 覆盖计数信号量 give 成功增加计数和满计数返回 `MRT_RESULT_OBJECT_FULL`；`test_semaphore_create_static` 覆盖初始计数大于最大计数时拒绝创建 |
| C-010 | 互斥锁 + 优先级继承 | 低优先级持锁，高优先级等待 | 持锁任务继承高优先级 | 调度仿真 | 已验证：`test_mutex_priority_inheritance` 覆盖低优先级持锁、高优先级等待、拥有者有效优先级提升、解锁后所有权转交和基础优先级恢复 |
| C-011 | 互斥锁 + 超时 | 高优先级等待互斥锁超时 | 持锁任务优先级正确回滚 | 调度仿真 | 部分覆盖：`test_mutex_priority_inheritance` 覆盖等待者阻塞和解锁恢复；等待者 tick 超时后拥有者优先级回滚尚未实现，后续需补 `mutex timeout rollback` 测试 |
| C-012 | 互斥锁 + 任务删除 | 删除持锁任务 | 互斥锁状态和等待任务处理符合规则 | 调度仿真 | 未实现：任务删除 API 尚未实现；持锁任务删除时互斥锁释放、等待者处理和优先级恢复规则待任务生命周期模块补测 |
| C-013 | 递归互斥锁 + 所有权 | 非拥有者释放 | 返回非法状态，不改变计数 | host 单测 | 已验证：`test_mutex_recursive` 覆盖递归互斥锁非拥有者释放返回 `MRT_RESULT_OWNER_ERROR`，拥有者保持不变，递归深度保持 2 |
| C-014 | 事件组 + 多等待者 | set bits 满足多个任务条件 | 所有满足条件任务被唤醒 | host 单测 | 已验证：`test_event_group_set_wakes_tasks` 覆盖同一事件组上 wait-any 与 wait-all 多等待者，`MRT_EventGroupSetBits` 基于置位后快照唤醒所有匹配任务，并让最高优先级等待者抢占 |
| C-015 | 事件组 + 清位 | wait-any 且退出清位 | 返回原始事件值后清除指定 bit | host 单测 | 已验证：`test_event_group_wait_immediate` 覆盖 wait-any clear-on-exit 返回清位前快照并清除匹配 bit；`test_event_group_set_wakes_tasks` 覆盖多等待者匹配后统一清除匹配 bit 并保留无关 bit |
| C-016 | 任务通知 + 覆盖策略 | no-overwrite 遇到未读通知 | 返回对象忙，不覆盖旧值 | host 单测 | 已验证：`test_task_notify_actions` 覆盖 pending 通知下 `MRT_NOTIFY_NO_OVERWRITE` 返回 `MRT_RESULT_OBJECT_BUSY` 且旧值不变；`test_task_notify_isr` 覆盖 ISR no-overwrite busy 同样不改旧值 |
| C-017 | 软件定时器 + 命令队列 | 启动/停止/复位命令排队 | 服务任务按序处理命令 | host 单测 | 待实现 |
| C-018 | 软件定时器 + 调度 | 定时器到期 | 回调在服务任务上下文执行 | 调度仿真 | 待实现 |
| C-019 | 软件定时器 + tickless | 睡眠期间定时器到期 | 唤醒后补偿 tick 并执行回调 | 端口 mock | 待实现 |
| C-020 | 流缓冲 + 环绕 | 写指针环绕后读取 | 数据顺序保持正确 | host 单测 | 待实现 |
| C-021 | 流缓冲 + ISR | ISR 写入，任务阻塞读 | 任务被唤醒并读到数据 | host/mock ISR | 待实现 |
| C-022 | 消息缓冲 + 容量 | 剩余空间不足以放完整消息 | 写入失败，不产生半包 | host 单测 | 待实现 |
| C-023 | 内存堆 + 对象创建 | heap 分配失败 | API 返回资源不足，内部状态不变 | host 单测 | 待实现 |
| C-024 | 内存堆 + 释放合并 | 释放相邻块 | 空闲块合并，碎片减少 | host 单测 | 待实现 |
| C-025 | trace + 任务切换 | trace 开启 | hook 收到切换事件，不改变调度结果 | host 单测 | 待实现 |
| C-026 | trace + 队列 | 队列 send/receive | hook 收到事件，不改变队列数据 | host 单测 | 待实现 |
| C-027 | 断言 + 非法上下文 | ISR 调用禁止 API | 触发断言或返回非法上下文 | host/mock ISR | 部分验证：`test_semaphore_isr`、`test_event_group_isr`、`test_task_notify_isr` 覆盖任务上下文调用 FromISR API 返回 `MRT_RESULT_INVALID_CONTEXT`；`test_mutex_create_lock` 覆盖无当前任务调用互斥锁 API 返回 `MRT_RESULT_INVALID_CONTEXT`；统一断言 hook 和 ISR 调用非 ISR-safe API 的断言路径待 trace/assert 模块补测 |
| C-028 | STM32 端口 + tick | SysTick 调用内核 tick | tick 推进并按需触发 PendSV | 端口 mock/smoke | 部分验证：`test_kernel_tick` 已验证 tick 推进与 kernel yield 转发；真实 SysTick/PendSV 待 STM32 端口计划 |
| C-029 | STM32 端口 + 临界区 | 嵌套进入临界区 | 中断屏蔽状态可恢复 | 端口 mock | 部分验证：`test_port_mock` 已验证 mock 临界区嵌套恢复；真实 PRIMASK/BASEPRI 待 STM32 端口计划 |
| C-030 | DSP 端口 + 栈初始化 | 创建任务栈帧 | 栈顶满足对齐和入口参数规则 | 端口 mock | 待实现 |
| C-031 | DSP 端口 + 上下文切换 | 触发软件中断切换 | 保存/恢复接口调用顺序正确 | 端口 mock | 待实现 |
| C-032 | 手册 + API | 每个 public API | 手册有原型、参数、返回值、示例、上下文限制 | 文档检查 | 待实现 |
| C-033 | 注释 + 源码 | 每个函数 | 有中文函数头说明和内部步骤注释 | 静态扫描 | 待实现 |

## 后续落地

- `tests/unit/`：基础算法与单模块。
- `tests/sim/`：调度仿真与任务交互。
- `tests/coupling/`：矩阵中的跨模块场景。
- `tests/port_mock/`：STM32/DSP 端口抽象。
- `examples/stm32/`：板级 smoke test。
- `examples/dsp/`：DSP mock/真实端口示例。

