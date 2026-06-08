# MyRTOS 测试套件落地计划

> 状态：设计草案。  
> 目标：把需求追踪矩阵和耦合测试矩阵转为可执行测试目录、测试类别和验收命令。

## 1. 测试目录规划

| 目录 | 作用 | 覆盖对象 |
|------|------|----------|
| `tests/unit/` | 单模块与基础算法测试 | 链表、位图、heap、队列、事件组、缓冲、硬件 smoke 日志输出 helper |
| `tests/sim/` | 调度仿真测试 | 任务切换、tick、阻塞、超时、优先级继承、TCB 运行期栈顶保存契约 |
| `tests/coupling/` | 跨模块耦合测试 | 矩阵 `C-001` 到 `C-041` |
| `tests/port_mock/` | 端口抽象 mock 测试 | STM32/DSP 栈、临界区、tickless、ISR |
| `tests/static/` | 静态检查 | 注释覆盖、API 手册覆盖、符号原创性、STM32 汇编骨架接线 |
| `examples/stm32/` | STM32 smoke test | LED、UART、ISR 队列、定时器 |
| `examples/dsp/` | DSP smoke/mock test | tick、软件中断、栈初始化 |
| `docs/verification/hardware_smoke/` | 真实板级 smoke 证据 | STM32/DSP 实机日志、raw-log schema、采集前置配置、采集清单与最终验收模板 |
| `examples/hardware_smoke/` | 板级 smoke 日志字段常量与日志格式 helper | STM32/DSP UART/trace 输出端可复用字段名，并可用单字符回调输出 `Key: Value` 行 |

## 2. 测试框架建议

- Host C 测试：Unity 或 CMocka 二选一；默认建议 Unity，体积小，适合嵌入式。
- 构建：CMake + CTest。
- 静态脚本：Python，放在 `tools/verify/`，覆盖 `include/`、`src/`、`examples/`、`tests/`。
- STM32：先提供 ARM GCC 可编译 smoke 示例；真实板级运行结果后续由用户硬件环境补证。
- DSP：先以端口 mock 证明端口契约；真实 DSP 型号确认后补 ABI 级测试。
- 真实硬件验收：先按目标板修改 `docs/verification/hardware_smoke/hardware_smoke_preflight.json` 并运行 `tools/verify/check_hardware_smoke_preflight.py`，再用 `tools/verify/check_hardware_smoke_raw_log_schema.py` 检查 `raw_log_schema.md`、生成器字段常量、示例 C 头文件和采集指南一致；随后用 `tools/verify/run_hardware_smoke_capture.py --target STM32|DSP` 预演采集流水线；确认命令会连接真实板卡并产出 raw log 后追加 `--execute`，或按 `docs/verification/hardware_smoke/raw_log_schema.md` 和 `docs/verification/hardware_smoke/collection_checklist.md` 手动采集原始日志，用 `tools/verify/generate_hardware_smoke_evidence.py` 生成含 raw-log SHA-256 的 `stm32_board_smoke.md` 和 `dsp_board_smoke.md`，最后运行 `tools/verify/check_hardware_smoke_evidence.py`。

## 3. 验收命令设计

后续实现完成后，验收命令应包含：

```powershell
cmake -S . -B build -DMRT_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
python tools/verify/run_release_verification.py
python tools/verify/check_hardware_smoke_preflight.py
python tools/verify/check_hardware_smoke_raw_log_schema.py
python tools/verify/run_hardware_smoke_capture.py --target STM32
python tools/verify/run_release_verification.py --require-hardware
```

`run_release_verification.py` 作为最终验收入口，默认执行 host、静态、原型、STM32 汇编骨架、DSP 汇编骨架、DSP C2000 工程骨架、embedded smoke、硬件 smoke 预检配置、硬件 raw-log schema 对齐、硬件日志输出 helper 单测、硬件采集执行器自测、硬件证据校验脚本自测和原始日志生成器自测；`--require-hardware` 会把真实 STM32/DSP 板级证据 gate 纳入同一条链路。

## 4. 测试分层策略

### 4.1 RED-GREEN 节奏

每个 API 实现前必须先有失败测试：

1. 写目标行为测试。
2. 运行并确认失败原因正确。
3. 写最小实现。
4. 运行并确认通过。
5. 补边界测试。
6. 更新需求矩阵状态。

### 4.2 基础算法先行

优先实现并测试：

- `MRT_List`
- `MRT_PriorityBitmap`
- `MRT_Heap`
- `MRT_PortMock`

理由：调度、队列、定时器、事件组都会依赖这些基础部件。

### 4.3 调度仿真优先于真实端口

调度正确性先在 host 仿真验证：

- 不依赖真实 CPU 上下文切换。
- 可重复制造 tick 溢出、ISR、超时。
- 能验证复杂耦合路径。

真实 STM32/DSP 端口之后只验证端口契约与 smoke 行为。

## 5. 静态验证规则

### 5.1 中文注释覆盖

脚本检查：

- 每个函数前有 Doxygen 风格中文注释。
- 注释包含 `@brief`、`@param`、`@return`、`@example`。
- 函数体内存在中文步骤注释。
- public API 注释和手册 API 名一致。

### 5.2 手册覆盖

脚本检查：

- `docs/api/myrtos_api_catalog.md` 中每个 public API 在手册出现。
- 每个 API 条目有“函数原型、参数、返回值、示例、调用上下文”。
- API 目录、公共头文件和源文件定义中的 public API 原型保持一致。
- STM32 和 DSP 移植章节有“工具链、启动文件、向量表、tick、上下文切换、栈布局、临界区、低功耗、示例、排错”。
- 手册不出现 FreeRTOS API 名作为 MyRTOS API 名。

### 5.3 原创性扫描

脚本检查：

- public 符号不使用 `xTask`、`vTask`、`xQueue`、`vQueue`、`port` 等 FreeRTOS 风格命名作为 API。
- 源码不包含 FreeRTOS 版权头。
- 手册不包含大段 FreeRTOS 官方原文。

## 6. 矩阵到测试套件映射

| 范围 | 测试文件模式 | 覆盖矩阵 |
|------|--------------|----------|
| 调度 | `tests/sim/test_scheduler_*.c` | `C-001` 到 `C-003` |
| 队列 | `tests/unit/test_queue_*.c`、`tests/coupling/test_queue_task_*.c` | `C-004` 到 `C-007` |
| 信号量 | `tests/unit/test_semaphore_*.c` | `C-008` 到 `C-009` |
| 互斥锁 | `tests/sim/test_mutex_*.c` | `C-010` 到 `C-013` |
| 事件组 | `tests/unit/test_event_group_*.c` | `C-014` 到 `C-015` |
| 任务通知 | `tests/unit/test_task_notify_*.c` | `C-016` |
| 定时器 | `tests/unit/test_timer_*.c`、`tests/sim/test_timer_scheduler_*.c` | `C-017` 到 `C-019` |
| 缓冲 | `tests/unit/test_stream_buffer_*.c`、`tests/unit/test_message_buffer_*.c`、`tests/coupling/test_buffer_dynamic_allocation.c` | `C-020` 到 `C-023`、`C-036` 到 `C-037` |
| 内存 | `tests/unit/test_heap_*.c`、`tests/coupling/test_buffer_dynamic_allocation.c` | `C-023` 到 `C-024` |
| trace/断言 | `tests/unit/test_trace_*.c`、`tests/port_mock/test_assert_context_*.c` | `C-025` 到 `C-027` |
| STM32 端口 | `tests/port_mock/test_port_stm32_*.c`、`tests/port_mock/test_port_stm32_mpu.c`、`tests/sim/test_task_stack_top.c`、`tests/static/test_stm32_context_scaffold.py`、`tools/verify/check_embedded_smoke_projects.py` | `C-028` 到 `C-029`、`C-035`；包含初始 PSP 写回 TCB 和 PendSV helper 接线 |
| DSP 端口 | `tests/port_mock/test_port_dsp_*.c`、`tests/static/test_dsp_context_scaffold.py`、`tests/static/test_dsp_c2000_project_scaffold.py` | `C-030` 到 `C-031`、`C-039`；包含 C28x 汇编骨架和 C2000 启动/链接/ISR glue 骨架静态检查 |
| 手册/注释 | `tests/static/*.py` | `C-032` 到 `C-033` |
| 烟雾工程 | `examples/stm32/`、`examples/dsp/`、`tools/verify/check_embedded_smoke_projects.py` | `C-028` 到 `C-031`、`C-039` |
| 真实板级证据 | `docs/verification/hardware_smoke/`、`examples/hardware_smoke/mrt_hardware_smoke_log_schema.h`、`examples/hardware_smoke/mrt_hardware_smoke_log.h`、`tools/verify/check_hardware_smoke_preflight.py`、`tools/verify/check_hardware_smoke_raw_log_schema.py`、`tools/verify/run_hardware_smoke_capture.py`、`tools/verify/generate_hardware_smoke_evidence.py`、`tools/verify/check_hardware_smoke_evidence.py`、`tests/unit/test_hardware_smoke_log.c`、`tests/static/test_hardware_smoke_capture_runner.py`、`tests/static/test_hardware_smoke_raw_log_schema.py`、`tests/static/test_hardware_smoke_evidence_checker.py`、`tests/static/test_hardware_smoke_evidence_generator.py` | `R-003`、`R-004`、`R-011`、`C-038`、`C-040` 到 `C-041`；包含 dry-run/execute 采集顺序、raw-log 字段 schema 对齐、板级输出 helper、raw-log SHA-256 派生与错配拒绝 |

## 7. 验收报告

最终需要生成：

- `build/test-results/ctest.log`
- `build/test-results/coverage-summary.txt`
- `docs/verification/final_verification_report.md`

验收报告必须按 `R-*` 和 `C-*` 编号列出证据。

