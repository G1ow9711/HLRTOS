; @file mrt_port_dsp_c28x_context.asm
; @brief DSP C28x style context-switch assembly scaffold.
;
; This file is an original, auditable scaffold for a future TI C2000/C28x
; target port. It documents the save/restore order expected by MyRTOS but is
; not compiled by the current host smoke model because this workspace does not
; contain a TI DSP toolchain. Board acceptance still requires real DSP ABI
; assembly, flashing, raw UART/trace logs, and hardware smoke evidence.

        .def    MRT_PortDspC28xStartFirstTaskAsm
        .def    MRT_PortDspC28xSoftwareInterruptHandler
        .def    MRT_PortDspC28xYieldAsm
        .ref    MRT_PortDspC28xStartFirstTaskHook
        .ref    MRT_PortDspC28xSwitchHook

; Static verifiers search for these GNU-style markers so the required public
; entry points are visible even when the target assembler is unavailable.
.global MRT_PortDspC28xStartFirstTaskAsm
.global MRT_PortDspC28xSoftwareInterruptHandler
.global MRT_PortDspC28xYieldAsm

; @brief Start the first task from an already prepared task stack.
; @param SP Target-specific stack pointer loaded by board startup code.
; @return Does not return on a complete target port.
; @example
; MRT_PortDspC28xStartFirstTaskAsm();
MRT_PortDspC28xStartFirstTaskAsm:
        ; 中文注释：调用 C 钩子读取首任务 TCB 和初始栈顶。
        LCR     MRT_PortDspC28xStartFirstTaskHook

        ; 中文注释：真实端口应在此处把返回值写入目标 DSP 栈指针寄存器。
        ; MOV     SP, AL

        ; 中文注释：按初始栈帧恢复任务入口所需状态，当前骨架只保留审计标记。
        POP     XT
        POP     P
        POP     ACC
        POP     ST1
        POP     ST0
        POP     XAR7
        POP     XAR6
        POP     XAR5
        POP     XAR4

        ; 中文注释：真实端口应跳转到初始 PC，本骨架用 LRETR 表示从任务入口返回路径。
        LRETR

; @brief Request a deferred software-interrupt context switch.
; @param void No explicit argument.
; @return Returns to caller after setting the software interrupt request.
; @example
; MRT_PortDspC28xYieldAsm();
MRT_PortDspC28xYieldAsm:
        ; 中文注释：真实端口应在此处置位 PIE/IFR 或目标软件中断触发位。
        ; OR      IFR, #MYRTOS_SOFTWARE_INTERRUPT_MASK

        ; 中文注释：当前骨架只保留可审计入口，实际请求由板级端口替换。
        LRETR

; @brief Software interrupt entry used to switch from one task stack to another.
; @param SP Current task stack pointer as defined by the target ABI.
; @return Returns through the restored task context.
; @example
; MRT_PortDspC28xSoftwareInterruptHandler();
MRT_PortDspC28xSoftwareInterruptHandler:
        ; 中文注释：保存 C28x 被调用者保存扩展地址寄存器。
        PUSH    XAR4
        PUSH    XAR5
        PUSH    XAR6
        PUSH    XAR7

        ; 中文注释：保存任务状态寄存器，避免切换后状态位串扰。
        PUSH    ST0
        PUSH    ST1

        ; 中文注释：保存计算相关寄存器，保护 DSP 算法任务现场。
        PUSH    ACC
        PUSH    P
        PUSH    XT

        ; 中文注释：真实端口应把当前 SP 传给 C 钩子，并接收下一任务 SP。
        LCR     MRT_PortDspC28xSwitchHook

        ; 中文注释：真实端口应在此处把 C 钩子返回的新栈顶写入 SP。
        ; MOV     SP, AL

        ; 中文注释：按保存的相反顺序恢复下一任务计算寄存器。
        POP     XT
        POP     P
        POP     ACC

        ; 中文注释：恢复下一任务状态寄存器。
        POP     ST1
        POP     ST0

        ; 中文注释：恢复下一任务扩展地址寄存器。
        POP     XAR7
        POP     XAR6
        POP     XAR5
        POP     XAR4

        ; 中文注释：返回到恢复后的任务或中断尾链路径。
        LRETR
