#include <stdint.h>

extern int main(void);
extern uint32_t _estack;
extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;

/**
 * @brief 默认中断处理函数。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * Default_Handler();
 */
void Default_Handler(void)
{
    /* 默认中断处理函数只需停在原地，便于调试器停机分析。 */
    for (;;) {
        /* 空转等待更具体的 handler 覆盖该弱符号。 */
    }
}

/**
 * @brief 复位入口，负责初始化 data/bss 并跳转 main。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * Reset_Handler();
 */
void Reset_Handler(void)
{
    /* 把 .data 段从 flash 复制到 RAM。 */
    uint32_t *src = &_sidata;
    uint32_t *dst = &_sdata;
    while (dst < &_edata) {
        /* 逐字复制已初始化变量。 */
        *dst++ = *src++;
    }

    /* 清零 .bss 段。 */
    dst = &_sbss;
    while (dst < &_ebss) {
        /* 逐字清空未初始化变量。 */
        *dst++ = 0u;
    }

    /* 进入应用主函数。 */
    (void)main();

    /* 若 main 返回，停在这里等待调试器或看门狗处理。 */
    for (;;) {
        /* 维持复位后最小停机状态。 */
    }
}

/**
 * @brief NMI 中断占位处理函数。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * NMI_Handler();
 */
void NMI_Handler(void)
{
    /* NMI 作为最高优先级异常，默认直接停机。 */
    Default_Handler();
}

/**
 * @brief HardFault 中断占位处理函数。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * HardFault_Handler();
 */
void HardFault_Handler(void)
{
    /* HardFault 默认停机，等待调试器读取现场。 */
    Default_Handler();
}

/**
 * @brief MemManage 中断占位处理函数。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * MemManage_Handler();
 */
void MemManage_Handler(void)
{
    /* 内存管理异常默认停机。 */
    Default_Handler();
}

/**
 * @brief BusFault 中断占位处理函数。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * BusFault_Handler();
 */
void BusFault_Handler(void)
{
    /* 总线异常默认停机。 */
    Default_Handler();
}

/**
 * @brief UsageFault 中断占位处理函数。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * UsageFault_Handler();
 */
void UsageFault_Handler(void)
{
    /* 使用错误默认停机。 */
    Default_Handler();
}

/**
 * @brief 保留异常占位处理函数。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * Reserved_Handler();
 */
void Reserved_Handler(void)
{
    /* 保留向量直接停机。 */
    Default_Handler();
}

/**
 * @brief SVC 中断弱处理函数。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * SVC_Handler();
 */
__attribute__((weak)) void SVC_Handler(void)
{
    /* 未接入真实 SVC 逻辑时复用默认停机路径。 */
    Default_Handler();
}

/**
 * @brief DebugMon 中断占位处理函数。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * DebugMon_Handler();
 */
void DebugMon_Handler(void)
{
    /* 调试监控异常默认停机。 */
    Default_Handler();
}

/**
 * @brief PendSV 中断弱处理函数。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * PendSV_Handler();
 */
__attribute__((weak)) void PendSV_Handler(void)
{
    /* 未接入真实上下文切换汇编时复用默认停机路径。 */
    Default_Handler();
}

/**
 * @brief SysTick 中断弱处理函数。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * SysTick_Handler();
 */
__attribute__((weak)) void SysTick_Handler(void)
{
    /* 默认 SysTick 处理函数停机，实际工程会由 main.c 覆盖。 */
    Default_Handler();
}

typedef void (*MRT_VectorEntry)(void);

/**
 * @brief STM32 Cortex-M 中断向量表。
 *
 * 第一项为初始栈顶，后续项为各异常/中断入口。
 */
__attribute__((section(".isr_vector"), used))
const MRT_VectorEntry g_vector_table[] = {
    (MRT_VectorEntry)((uintptr_t)&_estack),
    Reset_Handler,
    NMI_Handler,
    HardFault_Handler,
    MemManage_Handler,
    BusFault_Handler,
    UsageFault_Handler,
    Reserved_Handler,
    Reserved_Handler,
    Reserved_Handler,
    Reserved_Handler,
    SVC_Handler,
    DebugMon_Handler,
    Reserved_Handler,
    PendSV_Handler,
    SysTick_Handler
};
