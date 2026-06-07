# MyRTOS 需求追踪矩阵

> 目的：把用户原始目标拆成可验证需求，防止后续实现缩水。  
> 状态：草案，随实现推进更新证据列。  
> 规则：每项需求必须有设计位置、实现文件、测试证据、文档位置四类证据；未齐全前不能声明目标完成。

| 编号 | 原始需求 | 可验证要求 | 设计证据 | 实现证据 | 测试证据 | 文档证据 | 当前状态 |
|------|----------|------------|----------|----------|----------|----------|----------|
| R-001 | 参考 FreeRTOS 源码 | 只参考公开架构和模块划分，不复制源码、注释、手册原文 | `docs/superpowers/specs/2026-06-07-myrtos-c-scope-design.md` 第 1、10 节 | 源码和手册使用原创 `MRT_` API 与原创中文说明；未使用 FreeRTOS 风格 public API 名 | `python tools\verify\check_original_symbols.py` 通过：未发现 banned FreeRTOS-style public symbols | `docs/manual/MyRTOS_Reference_Manual_zh.md` 第 1 节说明原创边界 | 部分验证 |
| R-002 | 复现类似嵌入式 RTOS | 提供任务、调度、同步、通信、定时、内存、低功耗、trace、移植层 | 设计草案第 3、4 节 | Foundation + scheduler + task lifecycle APIs + queues + semaphores + mutexes + event groups + task notifications + software timers + stream/message buffers + heap + fixed memory pool + dynamic queue/task/sync/event/timer/buffer allocation + tickless idle + trace hook + runtime stats + assert hook 已实现；STM32/DSP 端口契约 helper 已实现；新增端口源码见 `include/myrtos/portable/mrt_port_stm32_cm.h`、`src/portable/stm32_cm/mrt_port_stm32_cm.c`、`include/myrtos/portable/mrt_port_dsp_c28x.h`、`src/portable/dsp_c28x/mrt_port_dsp_c28x.c` | `python tools\run_host_tests.py`：65 个 host 测试目标通过 | `docs/manual/MyRTOS_Reference_Manual_zh.md` 覆盖任务 API、同步通信 API、内存、tickless/trace/runtime stats/assert、STM32/DSP 详细移植步骤和动态对象生命周期 | 部分实现 |
| R-003 | 适应 STM32 | 提供 Cortex-M4/M7 ARM GCC/CMake 移植层与示例 | 设计草案第 2、6、7.4 节 | 已实现 host-testable Cortex-M 栈帧、SysTick reload、BASEPRI 编码 helper；真实启动文件、向量表、PendSV/SVC 汇编和板级示例待硬件 smoke 补齐 | `test_port_stm32_stack`、`test_port_stm32_tick_priority` 通过；STM32 板级 smoke test 待真实硬件或交叉工具链补证 | `docs/manual/MyRTOS_Reference_Manual_zh.md` 第 5 节包含 STM32 工具链、启动文件、向量表、SysTick、PendSV/SVC、优先级、链接脚本、示例和排错步骤 | 部分实现 |
| R-004 | 适应 DSP | 提供 DSP 抽象端口、mock 测试，后续可替换真实 DSP 汇编端口 | 设计草案第 2、6、7.4 节 | 已实现 DSP C28x 风格栈帧 helper 与软件中断上下文切换模型；真实 DSP 汇编保存/恢复和芯片定时器 hook 待具体型号补齐 | `test_port_dsp_stack`、`test_port_dsp_context` 通过；真实 DSP smoke test 待具体 DSP 型号和工具链补证 | `docs/manual/MyRTOS_Reference_Manual_zh.md` 第 6 节包含 DSP 工具链、ABI、栈帧、tick、软件中断切换、嵌套中断、示例和排错步骤 | 部分实现 |
| R-005 | 中文专业详细注释 | 每个函数含功能、参数、返回值、调用示例、注意事项 | 设计草案第 8 节 | `include/` 与 `src/` 函数注释已补齐；本阶段补充 `src/kernel/mrt_task_internal.h`、`src/kernel/mrt_queue.c`、`src/kernel/mrt_task.c` 缺口 | `python tools\verify\check_chinese_comments.py` 通过：include/src function comments covered | `docs/manual/MyRTOS_Reference_Manual_zh.md` 第 7.4 节列出注释/验证命令 | 部分验证 |
| R-006 | 函数内部逐行注释 | 每个函数关键语句有中文步骤注释；复杂分支逐行解释 | 设计草案第 8 节 | `include/` 与 `src/` 函数体附近中文步骤注释通过静态扫描；历史测试文件不纳入本阶段静态范围 | `python tools\verify\check_chinese_comments.py` 通过 | `docs/manual/MyRTOS_Reference_Manual_zh.md` 第 7.4 节列出注释/验证命令 | 部分验证 |
| R-007 | 类 FreeRTOS 官方说明手册 | 提供原创中文参考手册，API 按章节列原型/参数/返回值/示例/限制 | 设计草案第 9、10 节 | 不适用 | `python tools\verify\check_api_manual_coverage.py` 通过：125 API section(s) covered | `docs/manual/MyRTOS_Reference_Manual_zh.md` 已写，包含关于本手册、API 使用规则、API 参考、移植步骤、附录 | 部分验证 |
| R-008 | 所有功能必须测试 | 每个 public API 和核心内部算法有自动化测试 | 设计草案第 7 节 | Foundation、scheduler、task lifecycle、queue、semaphore、mutex、event group、task notification、software timer、stream buffer、message buffer、heap、memory pool、dynamic queue/task/sync/event/timer/buffer allocation、tickless、trace、runtime stats、assert、STM32 port helper、DSP port helper public API 已有 host/sim/coupling 测试 | `python tools\run_host_tests.py`：65 个 host 测试目标通过；本阶段新增 `test_runtime_stats` | `docs/verification/final_verification_report.md` 记录最新 host/static 结果 | 部分验证 |
| R-009 | 所有耦合情况测试清楚 | 用耦合矩阵覆盖模块交互、ISR、超时、内存失败、trace、tickless | 设计草案第 7.3 节；耦合矩阵文件 | 已验证 scheduler + task lifecycle + tick + ready/delay list、task + heap dynamic allocation、task ResumeFromISR、queue + task/ISR、queue + heap dynamic allocation、dynamic semaphore/mutex/event/timer/buffer heap failure and delete semantics、semaphore + task/ISR、mutex + priority inheritance/ownership、event group + multi-waiter/clear/ISR、task notification + overwrite/wait/ISR、software timer + kernel tick/pending FIFO、software timer + tickless compensation、stream buffer + wrap/ISR reader wake、message buffer + packet capacity/ISR reader wake、heap allocation failure、heap coalescing、trace + task switch、trace + queue、runtime stats + current task/tick/scheduler switch、assert hook、STM32 stack/tick/BASEPRI helper、DSP stack/context helper；真实板级端口仍待后续模块 | `test_runtime_stats` 已加入运行统计耦合证据；`python tools\run_host_tests.py`：65 个 host 测试目标通过 | `docs/verification/coupling_test_matrix.md` 更新 C-034 | 部分验证 |
| R-010 | C 版大而全范围 | 包含 stream/message buffer、tickless、trace、MPU 预留、多 heap 策略等高级模块 | 设计草案第 4 节 | stream/message buffer 与动态创建路径已实现；多 heap 策略已实现线性、free-list、coalescing 三种模式；固定块 memory pool、动态队列、动态任务和动态同步/定时对象堆分配已实现；tickless idle、trace hook、runtime stats、assert hook 已实现；MPU 预留仍待后续计划 | `test_stream_buffer_create_static`、`test_stream_buffer_send_receive`、`test_stream_buffer_isr`、`test_stream_buffer_isr_wakes_reader`、`test_message_buffer_create_static`、`test_message_buffer_send_receive`、`test_message_buffer_isr`、`test_buffer_dynamic_allocation`、`test_heap_linear`、`test_heap_free_list`、`test_heap_coalescing`、`test_memory_pool`、`test_queue_dynamic_allocation`、`test_tickless_expected_idle`、`test_tickless_timer_compensation`、`test_trace_task_switch`、`test_trace_queue`、`test_runtime_stats`、`test_assert_hook` 通过 | `docs/manual/MyRTOS_Reference_Manual_zh.md` 覆盖高级模块 API、运行统计和动态对象限制 | 部分实现 |
| R-011 | 手册包含详细移植步骤 | STM32 与 DSP 移植章节必须覆盖工具链、启动文件、向量表、tick、上下文切换、栈布局、临界区、低功耗、示例和排错 | 设计草案第 9 节；`docs/api/myrtos_api_catalog.md` 第 14 节 | 不适用 | `python tools\verify\check_api_manual_coverage.py` 验证 STM32/DSP 移植关键词覆盖 | `docs/manual/MyRTOS_Reference_Manual_zh.md` 第 5 节“STM32 Cortex-M 移植步骤”和第 6 节“DSP 移植步骤”包含详细迁移流程、工程接入点、示例和排错 | 部分验证 |

## 完成判定

目标完成必须同时满足：

- 所有 `R-*` 行状态为“已验证”。
- 每行实现证据指向具体源码文件。
- 每行测试证据指向通过的测试命令和结果。
- 每行文档证据指向手册具体章节。
- 测试报告记录 host、耦合、端口 mock、嵌入式 smoke test 结果。

## 关联规格

- API 目录：`docs/api/myrtos_api_catalog.md`
- 测试套件计划：`docs/verification/test_suite_plan.md`
- 耦合测试矩阵：`docs/verification/coupling_test_matrix.md`
