# MyRTOS Completion Audit

生成日期：2026-06-08
范围：当前工作树 `feature/embedded-smoke-projects`

## 结论

当前项目已经满足大部分核心交付：

- 原创 `MRT_` RTOS 代码已实现。
- STM32 / DSP 端口契约 helper 已实现。
- 中文注释覆盖、手册 API 覆盖、原始符号扫描、host 测试和 embedded smoke 脚本都已通过。
- 手册已补充详细 STM32 / DSP 移植步骤。
- 真实板级 smoke 证据模板、raw-log schema、采集预检配置、采集执行器、原始日志生成器和校验脚本已补充到 `docs/verification/hardware_smoke/`、`examples/hardware_smoke/mrt_hardware_smoke_log_schema.h`、`tools/verify/check_hardware_smoke_preflight.py`、`tools/verify/check_hardware_smoke_raw_log_schema.py`、`tools/verify/run_hardware_smoke_capture.py`、`tools/verify/generate_hardware_smoke_evidence.py` 与 `tools/verify/check_hardware_smoke_evidence.py`；最终证据还要求 `Raw-Log-Path` 与 `Raw-Log-SHA256` 回溯到原始 UART/trace 日志。
- 统一 release 验证入口 `tools/verify/run_release_verification.py` 已就位：默认模式可跑 repo-side 验证，`--require-hardware` 会把真实板级 gate 纳入同一链路。

但**仍有一项硬缺口未被证明**：

- 真实 STM32 板级 smoke test。
- 真实 DSP 板级 smoke test。

因此，当前状态是 **high-confidence preview**，不是“已被真实硬件完全证实的最终完结态”。

## 需求审计

| 需求 | 证据 | 状态 |
|------|------|------|
| 原创实现，不照抄 FreeRTOS | `check_original_symbols.py` 通过；源码和手册使用原创 `MRT_` 命名 | 已验证 |
| 适配 STM32 | `test_port_stm32_stack`、`test_port_stm32_tick_priority`、`test_port_stm32_mpu`，以及 `examples/stm32` 交叉编译通过 | 部分验证 |
| 适配 DSP | `test_port_dsp_stack`、`test_port_dsp_context`、`test_dsp_context_scaffold.py`、`test_dsp_c2000_project_scaffold.py`，以及 `examples/dsp` host model 通过 | 部分验证 |
| 专业详细中文注释 | `check_chinese_comments.py` 通过，覆盖 `include/`、`src/`、`examples/`、`tests/` | 已验证 |
| 类 FreeRTOS 官方风格中文手册 | `check_api_manual_coverage.py` 通过，135 个 API 条目覆盖；`check_api_catalog_prototypes.py` 通过，135 个 API 原型对齐 | 已验证 |
| 手册含详细移植步骤 | 手册第 5、6 节包含移植前准备、工程分层、关键接入顺序、从厂商裸机工程迁入 MyRTOS 的实际顺序、首次联调、板级验收、raw-log schema 和证据生成命令 | 已验证 |
| 所有功能必须测试 | 70 个 host test target 通过，embedded smoke 脚本通过 | 大部分已验证 |
| 所有耦合情况测试清楚 | `docs/verification/coupling_test_matrix.md` 已扩展到 `C-040`，对应 host / smoke / 静态证据已落表 | 大部分已验证 |
| 真实 STM32/DSP 板级 smoke | 目前仅有交叉编译与 host model 证据；`raw_log_schema.md`、`mrt_hardware_smoke_log_schema.h`、`hardware_smoke_preflight.json`、`collection_checklist.md`、模板、采集执行器、raw-log SHA-256 校验、`check_hardware_smoke_preflight.py`、`check_hardware_smoke_raw_log_schema.py` 和 `check_hardware_smoke_evidence.py` 已就位，但真实 `stm32_board_smoke.md`、`dsp_board_smoke.md` 未提交 | 未验证 |

## 当前可证明的完成面

- Kernel / scheduler / queue / semaphore / mutex / event group / task notify / timer / stream buffer / message buffer / heap / memory pool / tickless / trace / runtime stats / assert 已有 host 验证。
- STM32 helper 与 MPU helper 已有 host 验证。
- DSP helper 已有 host 验证。
- 手册、注释和原创性静态检查已通过。

## 最新增量

- STM32 端口新增 `src/portable/stm32_cm/mrt_port_stm32_cm_context.S`，提供 SVC/PendSV/首任务启动汇编入口骨架；任务 TCB 现在保存运行期 `stack_top`，`examples/stm32/main.c` 会把 `MRT_PortStm32CmInitializeStack()` 返回的初始 PSP 写入 TCB，`MRT_PortStm32CmPendSvHook()` 已接入 `MRT_TaskKernelSwitchStackTop()` 的 PSP 保存/恢复契约。
- 新增 `tests/static/test_stm32_context_scaffold.py`，并纳入 `tools/verify/run_release_verification.py` 默认 release 链路。
- `tools/verify/check_embedded_smoke_projects.py` 已把 `.S` 和 STM32 初始栈帧写回 TCB 的 C 接线纳入 ARM GCC 交叉编译；该证据证明入口骨架可构建，不证明真实板级上下文切换已运行。
- DSP 端口新增 `src/portable/dsp_c28x/mrt_port_dsp_c28x_context.asm`，提供 C28x 风格首任务启动、软件中断 yield、软件中断切换入口、寄存器保存/恢复和 C 钩子交接骨架；新增 `tests/static/test_dsp_context_scaffold.py` 并纳入默认 release 链路。该证据证明 DSP 汇编保存/恢复顺序已有可审计模板，不证明具体 DSP 工具链编译或真实板级上下文切换已运行。
- DSP C2000 工程新增 `examples/dsp/startup_c28x.c`、`examples/dsp/mrt_port_dsp_c2000_smoke.c` 和 `examples/dsp/linker_c28x.cmd`，提供启动向量、CPU Timer0 tick、软件中断、ADC/DMA ISR、RTOS heap、任务栈、DMA buffer 和 trace buffer 分区骨架；新增 `tests/static/test_dsp_c2000_project_scaffold.py` 并纳入默认 release 链路。该证据证明 C2000 工程接线清单可审计，不证明 TI 工具链编译或真实 DSP 板级 smoke 已运行。
- 新增 `tools/verify/check_hardware_smoke_preflight.py`、`tests/static/test_hardware_smoke_preflight.py` 和 `docs/verification/hardware_smoke/hardware_smoke_preflight.json`，用于在采集真实板级日志前检查 STM32/DSP 目标配置、运行时长、必需字段和可选工具链可用性。
- 新增 `tools/verify/run_hardware_smoke_capture.py` 和 `tests/static/test_hardware_smoke_capture_runner.py`，用于 dry-run 或执行预检、编译、构建、烧录、采集、证据生成和目标级证据校验；默认不执行硬件命令。
- 新增 raw log 追溯规则：`tools/verify/generate_hardware_smoke_evidence.py` 会自动写入 `Raw-Log-Path` 和 `Raw-Log-SHA256`，`tools/verify/check_hardware_smoke_evidence.py` 会读取原始日志并拒绝 SHA-256 错配证据。
- 新增 raw-log schema 契约：`docs/verification/hardware_smoke/raw_log_schema.md` 集中列出 STM32/DSP 原始日志字段，`examples/hardware_smoke/mrt_hardware_smoke_log_schema.h` 提供 C 输出端字段常量，`tools/verify/check_hardware_smoke_raw_log_schema.py` 和 `tests/static/test_hardware_smoke_raw_log_schema.py` 防止生成器、文档、示例头文件和采集指南字段漂移。

## 仍待补证

1. 真实 STM32 板卡运行 `examples/stm32` 派生工程。
2. 真实 DSP 板卡运行 `examples/dsp` 派生工程。
3. 采集前按真实板卡修改 `hardware_smoke_preflight.json` 并运行 `python tools\verify\check_hardware_smoke_preflight.py`；在实机采集环境上可追加 `--check-tools`。
4. 运行 `python tools\verify\run_hardware_smoke_capture.py --target STM32` 或 `--target DSP` 预演采集流水线；确认命令会连接真实板卡并产出 raw log 后，再追加 `--execute`。
5. 按 `docs/verification/hardware_smoke/raw_log_schema.md` 输出或整理原始 UART/trace 字段，必要时复用 `examples/hardware_smoke/mrt_hardware_smoke_log_schema.h` 中的字段常量。
6. 保留原始 UART/trace 日志，并用 `tools/verify/generate_hardware_smoke_evidence.py` 生成最终证据文件，或按 `docs/verification/hardware_smoke/*.template.md` 手动填写真实板级日志、编译器版本、芯片型号、时钟、tick 频率、PendSV/SVC 或软件中断汇编证据；最终文件必须含匹配的 `Raw-Log-Path` 与 `Raw-Log-SHA256`。
7. 运行 `python tools\verify\check_hardware_smoke_evidence.py`，让真实硬件证据 gate 通过。

## 建议下一步

- 若提供 STM32 板卡或 QEMU/RENODE 类环境，优先补 STM32 板级 smoke。
- 若提供 DSP 实机或对应工具链，补真实 ABI 汇编端口与板级 smoke。
- 补齐后，再把 `R-003`、`R-004`、`R-008`、`R-009`、`R-011` 的“部分验证”状态升级为最终验证。
