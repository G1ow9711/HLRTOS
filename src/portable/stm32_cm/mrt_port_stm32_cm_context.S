/*
 * @file mrt_port_stm32_cm_context.S
 * @brief STM32 Cortex-M SVC/PendSV 上下文切换汇编骨架。
 *
 * 本文件只提供板级端口需要覆盖向量表的汇编入口，并把真正的调度决策交给 C 钩子。
 * 当前 MyRTOS host/smoke 任务模型尚未在 TCB 中保存运行期 PSP，因此这里是可编译、
 * 可审查、可接入真板验证的骨架，不伪称已经完成完整任务切换。
 */

    .syntax unified
    .cpu cortex-m4
    .thumb

    .extern MRT_PortStm32CmSvcHook
    .extern MRT_PortStm32CmPendSvHook

/*
 * @brief 以给定 PSP 请求启动首任务。
 * @param r0 初始 PSP 栈顶，通常来自 MRT_PortStm32CmInitializeStack()。
 * @return 不返回；真实端口会在 SVC 中恢复首任务异常帧。
 * @example
 * MRT_PortStm32CmStartFirstTaskAsm(first_stack_top);
 */
    .global MRT_PortStm32CmStartFirstTaskAsm
    .type MRT_PortStm32CmStartFirstTaskAsm, %function
MRT_PortStm32CmStartFirstTaskAsm:
    /* 把调用者传入的任务栈顶写入 PSP，作为后续异常返回的任务栈。 */
    msr psp, r0

    /* 设置 CONTROL.SPSEL=1，使线程模式后续使用 PSP。 */
    movs r0, #2
    msr control, r0

    /* 确保 CONTROL/PSP 写入在触发 SVC 前已经对内核可见。 */
    isb

    /* 触发 SVC，交给 SVC_Handler 进入首任务启动路径。 */
    svc 0

    /* 若调试骨架中的 SVC 钩子返回，则返回调用点，便于 smoke 工程继续诊断。 */
    bx lr

/*
 * @brief SVC 异常入口。
 * @param lr EXC_RETURN，指示异常前使用 MSP 还是 PSP。
 * @return 异常返回到调用方；真实端口可在 C 钩子内扩展首任务恢复。
 * @example
 * SVC_Handler();
 */
    .global SVC_Handler
    .type SVC_Handler, %function
SVC_Handler:
    /* 根据 EXC_RETURN bit[2] 判断异常帧来自 MSP 还是 PSP。 */
    tst lr, #4

    /* bit[2]=0 表示异常前使用 MSP，bit[2]=1 表示异常前使用 PSP。 */
    ite eq
    mrseq r0, msp
    mrsne r0, psp

    /* 把 EXC_RETURN 作为第二个参数传给 C 钩子。 */
    mov r1, lr

    /* 进入 C 级 SVC 钩子，smoke 端只记录现场，真实端口可恢复首任务。 */
    bl MRT_PortStm32CmSvcHook

    /* 返回异常入口，由硬件按 EXC_RETURN 弹出自动异常帧。 */
    bx lr

/*
 * @brief PendSV 异常入口。
 * @param lr EXC_RETURN，指示异常返回路径。
 * @return 异常返回到调度后的任务；当前 smoke 骨架返回原 PSP。
 * @example
 * PendSV_Handler();
 */
    .global PendSV_Handler
    .type PendSV_Handler, %function
PendSV_Handler:
    /* 读取当前 PSP；没有首任务时 PSP 可能为 0，需走安全诊断路径。 */
    mrs r0, psp

    /* smoke 阶段若尚未进入任务栈，避免向 0 地址压入 R4-R11。 */
    cbz r0, .Lmrt_pendsv_no_psp

    /* 保存 R4-R11，Cortex-M 硬件不会自动保存这些被调用者保存寄存器。 */
    stmdb r0!, {r4-r11}

    /* 把 EXC_RETURN 传给 C 钩子，钩子返回要恢复的任务 PSP。 */
    mov r1, lr

    /* 调用 C 钩子；完整端口会在这里选择下一个任务并返回其栈顶。 */
    bl MRT_PortStm32CmPendSvHook

    /* 从返回的 PSP 恢复 R4-R11，形成 PendSV 上下文恢复路径。 */
    ldmia r0!, {r4-r11}

    /* 写回恢复后的 PSP，使异常返回使用正确任务栈。 */
    msr psp, r0

    /* 异常返回，硬件继续弹出 R0-R3/R12/LR/PC/xPSR 自动异常帧。 */
    bx lr

.Lmrt_pendsv_no_psp:
    /* PSP 为空时仍调用 C 钩子记录诊断信息，但不保存/恢复寄存器。 */
    mov r1, lr
    bl MRT_PortStm32CmPendSvHook

    /* 返回当前异常，避免 smoke 骨架在未启动首任务前访问非法 PSP。 */
    bx lr

    .size MRT_PortStm32CmStartFirstTaskAsm, .-MRT_PortStm32CmStartFirstTaskAsm
    .size SVC_Handler, .-SVC_Handler
    .size PendSV_Handler, .-PendSV_Handler
