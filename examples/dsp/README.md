# MyRTOS DSP Smoke Model

## 目的
这个目录提供一个 DSP C28x 风格的 host smoke/model 工程，用来验证：

- DSP helper 层可以和 MyRTOS 内核一起编译和运行。
- 中断嵌套、软件中断式切换请求、队列 ISR、定时器和任务创建的耦合语义清晰。
- 真实 TI DSP 工程后续只需把 `mrt_port_dsp_model.c` 替换为目标芯片的汇编/启动文件。

## 目录说明

- `main.c`：host smoke 入口，执行 DSP helper、任务、队列和定时器验证。
- `mrt_port_dsp_model.c`：DSP 公共 port 接口的 host 模型实现。
- `../../src/portable/dsp_c28x/mrt_port_dsp_c28x_context.asm`：TI C28x 风格上下文切换汇编骨架，用于记录首任务启动、软件中断 yield、寄存器保存/恢复和 C 钩子交接顺序；它不参与当前 host 编译，也不替代真实 DSP 板级 smoke。

## 构建方式

当前仓库使用 `tools/verify/check_embedded_smoke_projects.py` 作为统一验证入口。脚本会：

1. 检查 `examples/dsp/` 是否齐全。
2. 在 host 上编译并运行 DSP smoke/model。
3. 让 DSP 的真实 TI 工具链缺口保持可见，而不是被忽略。

## DSP 迁移步骤

1. 先运行 host model，确认 API 接线、调度请求和 ISR 嵌套计数逻辑正确。
2. 以 `mrt_port_dsp_c28x_context.asm` 为审计模板，把 `mrt_port_dsp_model.c` 中的模型实现替换为目标 DSP 的中断屏蔽、上下文切换和睡眠代码。
3. 用目标工具链替换当前 host `gcc`。
4. 把 `MRT_PortDspC28xInitializeStack()` 的契约映射到目标 ABI 的真实保存槽位。
5. 在 `CpuTimer0Isr`、`AdcIsr`、软件中断和低功耗入口中接入真实硬件寄存器操作。

## smoke 关注点

- 任务栈必须满足目标 ABI 的对齐规则。
- ISR 中只能做短时间复制、状态记录和延迟切换请求。
- 定时器/队列/任务的耦合优先在 host model 上验证。
- 真实 TI 工具链和板级 smoke 仍是后续落地项。
- 汇编骨架只证明保存/恢复顺序已经形成可审计模板，不替代真实 DSP 板级 smoke；真实验收仍必须提交 `dsp_board_smoke.md` 和匹配 raw log。
