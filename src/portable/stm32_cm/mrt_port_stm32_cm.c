#include "myrtos/portable/mrt_port_stm32_cm.h"

#include <stdint.h>

/**
 * @brief 将地址向下对齐到指定边界。
 * @param value 待对齐的地址数值。
 * @param alignment 对齐边界，必须为 2 的幂。
 * @return uintptr_t 返回向下对齐后的地址数值。
 * @example
 * uintptr_t aligned = mrt_stm32_cm_align_down(raw_end, 8u);
 */
static uintptr_t mrt_stm32_cm_align_down(uintptr_t value, uintptr_t alignment)
{
    /* 生成对齐掩码，alignment 为 8 时低 3 bit 会被清零。 */
    uintptr_t mask = alignment - 1u;

    /* 清除低位未对齐 bit，得到向下取整后的地址。 */
    return value & ~mask;
}

/**
 * @brief 初始化 STM32 Cortex-M 任务初始栈帧。
 * @param stack_memory 调用者提供的任务栈首地址，元素类型必须为 MRT_StackType。
 * @param stack_words 任务栈元素数量，必须至少容纳 MRT_PORT_STM32_CM_INITIAL_FRAME_WORDS。
 * @param entry 任务入口函数，参数通过 R0 传入，不能为空。
 * @param argument 传给任务入口函数的用户参数，可为空。
 * @param task_exit 任务函数意外返回时跳转的处理函数，不能为空。
 * @param out_stack_top 输出初始化后的栈顶指针，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示栈帧已写入；参数非法或空间不足时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * MRT_StackType stack[128];
 * MRT_StackType *top;
 * MRT_PortStm32CmInitializeStack(stack, 128, app_task, arg, task_exit, &top);
 */
MRT_Result MRT_PortStm32CmInitializeStack(MRT_StackType *stack_memory,
                                          size_t stack_words,
                                          void (*entry)(void *argument),
                                          void *argument,
                                          void (*task_exit)(void),
                                          MRT_StackType **out_stack_top)
{
    /* 输出指针为空时无法回传栈顶，直接拒绝。 */
    if (out_stack_top == 0) {
        /* 返回统一参数错误码。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 先清空输出，保证失败路径不会留下旧栈顶。 */
    *out_stack_top = 0;

    /* 栈内存、任务入口和任务退出处理函数都必须有效。 */
    if ((stack_memory == 0) || (entry == 0) || (task_exit == 0)) {
        /* 返回统一参数错误码。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 栈空间必须至少能容纳完整初始帧。 */
    if (stack_words < MRT_PORT_STM32_CM_INITIAL_FRAME_WORDS) {
        /* 返回统一参数错误码。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 读取栈起始地址，后续用整数形式做对齐计算。 */
    uintptr_t stack_start = (uintptr_t)stack_memory;

    /* 栈起始地址必须满足 MRT_StackType 自身对齐，否则无法安全写 32 位槽。 */
    if ((stack_start & (uintptr_t)(sizeof(MRT_StackType) - 1u)) != 0u) {
        /* 返回统一参数错误码。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 防御乘加溢出，避免极端参数绕回地址空间。 */
    if (stack_words > ((UINTPTR_MAX - stack_start) / sizeof(MRT_StackType))) {
        /* 返回统一参数错误码。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 计算调用者提供栈区的末尾地址，末尾地址指向最后一个元素之后。 */
    uintptr_t stack_end = stack_start + (stack_words * sizeof(MRT_StackType));

    /* Cortex-M ABI 要求异常帧入口保持 8 字节对齐。 */
    uintptr_t aligned_end = mrt_stm32_cm_align_down(stack_end, 8u);

    /* 对齐后如果已经落到起始地址之前或等于起始地址，说明无可用空间。 */
    if (aligned_end <= stack_start) {
        /* 返回统一参数错误码。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 计算对齐后仍可使用的栈元素数量。 */
    size_t usable_words = (size_t)((aligned_end - stack_start) / sizeof(MRT_StackType));

    /* 对齐损失后仍必须能容纳完整初始帧。 */
    if (usable_words < MRT_PORT_STM32_CM_INITIAL_FRAME_WORDS) {
        /* 返回统一参数错误码。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 初始栈帧从对齐末尾向低地址方向预留 16 个 32 位槽。 */
    MRT_StackType *frame = ((MRT_StackType *)aligned_end) - MRT_PORT_STM32_CM_INITIAL_FRAME_WORDS;

    /* 依次写入 R4-R11 的调试占位值，真实任务首次运行前这些寄存器无业务含义。 */
    for (uint32_t reg = 4u; reg <= 11u; reg++) {
        /* 根据寄存器号换算到初始帧内的软件保存槽。 */
        size_t slot = (size_t)MRT_PORT_STM32_CM_FRAME_R4 + (size_t)(reg - 4u);

        /* 写入可识别模式，便于调试器观察任务尚未运行的初始上下文。 */
        frame[slot] = (MRT_StackType)(MRT_PORT_STM32_CM_CALLEE_PATTERN_BASE + reg);
    }

    /* R0 保存任务入口参数。 */
    frame[MRT_PORT_STM32_CM_FRAME_R0] = (MRT_StackType)(uintptr_t)argument;

    /* R1 初始无参数，清零。 */
    frame[MRT_PORT_STM32_CM_FRAME_R1] = 0u;

    /* R2 初始无参数，清零。 */
    frame[MRT_PORT_STM32_CM_FRAME_R2] = 0u;

    /* R3 初始无参数，清零。 */
    frame[MRT_PORT_STM32_CM_FRAME_R3] = 0u;

    /* R12 初始无业务值，清零。 */
    frame[MRT_PORT_STM32_CM_FRAME_R12] = 0u;

    /* LR 保存任务函数意外返回后的兜底处理入口。 */
    frame[MRT_PORT_STM32_CM_FRAME_LR] = (MRT_StackType)(uintptr_t)task_exit;

    /* PC 保存任务入口函数地址，异常返回后从这里开始执行。 */
    frame[MRT_PORT_STM32_CM_FRAME_PC] = (MRT_StackType)(uintptr_t)entry;

    /* xPSR 必须设置 Thumb bit，否则 Cortex-M 不能合法进入 C 函数。 */
    frame[MRT_PORT_STM32_CM_FRAME_XPSR] = MRT_PORT_STM32_CM_XPSR_THUMB_BIT;

    /* 回传新栈顶，调度器后续把它保存进任务控制块。 */
    *out_stack_top = frame;

    /* 栈帧构造完成。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 计算 STM32 Cortex-M SysTick reload 装载值。
 * @param cpu_clock_hz SysTick 输入时钟频率，单位 Hz，通常为内核时钟或其分频时钟。
 * @param tick_rate_hz 目标 RTOS tick 频率，单位 Hz，必须大于 0。
 * @param out_reload 输出 reload 值，即写入 SysTick LOAD 的数值，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示计算成功；参数非法、每 tick 时钟数为 0 或超出 24 位范围时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * uint32_t reload;
 * MRT_PortStm32CmCalculateSysTickReload(168000000u, 1000u, &reload);
 */
MRT_Result MRT_PortStm32CmCalculateSysTickReload(uint32_t cpu_clock_hz,
                                                 uint32_t tick_rate_hz,
                                                 uint32_t *out_reload)
{
    /* 输出指针为空时无法回传 reload 值，直接拒绝。 */
    if (out_reload == 0) {
        /* 返回统一参数错误码。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* CPU 时钟和 tick 频率都必须为非零值。 */
    if ((cpu_clock_hz == 0u) || (tick_rate_hz == 0u)) {
        /* 保持调用者原输出值不变并返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 计算一个 RTOS tick 内包含的 SysTick 输入时钟数。 */
    uint32_t clocks_per_tick = cpu_clock_hz / tick_rate_hz;

    /* 当 tick 频率高于输入时钟时，无法形成至少 1 个计数周期。 */
    if (clocks_per_tick == 0u) {
        /* 保持调用者原输出值不变并返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* SysTick LOAD 写入的是“周期计数减 1”。 */
    uint32_t reload = clocks_per_tick - 1u;

    /* Cortex-M SysTick LOAD 只有 24 bit，超出时必须改用分频或其他定时器。 */
    if (reload > MRT_PORT_STM32_CM_SYSTICK_RELOAD_MAX) {
        /* 保持调用者原输出值不变并返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 回传可直接写入 SysTick LOAD 的装载值。 */
    *out_reload = reload;

    /* reload 计算完成。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 按 NVIC 实现优先级位数计算 BASEPRI 编码值。
 * @param nvic_priority_bits MCU 实际实现的 NVIC 优先级 bit 数，常见 STM32 为 4。
 * @param logical_priority 未左移的逻辑优先级，0 表示最高优先级且不能用于 BASEPRI 屏蔽。
 * @param out_encoded_priority 输出左对齐到 8 bit 优先级字段后的 BASEPRI 数值，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示编码成功；位宽、优先级或输出指针非法时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * uint32_t basepri;
 * MRT_PortStm32CmEncodeBasepri(4u, 5u, &basepri);
 */
MRT_Result MRT_PortStm32CmEncodeBasepri(uint32_t nvic_priority_bits,
                                        uint32_t logical_priority,
                                        uint32_t *out_encoded_priority)
{
    /* 输出指针为空时无法回传 BASEPRI 编码值。 */
    if (out_encoded_priority == 0) {
        /* 返回统一参数错误码。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* Cortex-M 优先级字段最多 8 bit，且至少要实现 1 bit 才能编码。 */
    if ((nvic_priority_bits == 0u) || (nvic_priority_bits > 8u)) {
        /* 保持调用者原输出值不变并返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 计算逻辑优先级的取值上限，位宽为 4 时范围是 0..15。 */
    uint32_t logical_priority_count = 1u << nvic_priority_bits;

    /* BASEPRI 写 0 表示不屏蔽中断，因此内核临界区屏蔽优先级不能为 0。 */
    if ((logical_priority == 0u) || (logical_priority >= logical_priority_count)) {
        /* 保持调用者原输出值不变并返回参数错误。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* NVIC 优先级字段左对齐存放在 8 bit 字段的高位。 */
    *out_encoded_priority = logical_priority << (8u - nvic_priority_bits);

    /* BASEPRI 编码完成。 */
    return MRT_RESULT_OK;
}

/**
 * @brief 将任意 STM32 Cortex-M 内存范围规整为 MPU 兼容区域。
 * @param base_address 待保护或映射的内存起始地址。
 * @param size_bytes 待保护或映射的原始字节数，必须大于 0。
 * @param out_region_base 输出规整后的区域基址，不能为空。
 * @param out_region_size 输出规整后的区域大小，单位字节，不能为空。
 * @param out_region_shift 输出规整后区域大小的 log2 值，例如 8192 字节对应 13，不能为空。
 * @return MRT_Result 返回 MRT_RESULT_OK 表示规整成功；参数非法、地址范围溢出或无法规整时返回 MRT_RESULT_INVALID_ARGUMENT。
 * @example
 * uintptr_t region_base;
 * size_t region_size;
 * uint32_t region_shift;
 * MRT_PortStm32CmNormalizeMpuRegion(0x20001234u, 6000u, &region_base, &region_size, &region_shift);
 */
MRT_Result MRT_PortStm32CmNormalizeMpuRegion(uintptr_t base_address,
                                             size_t size_bytes,
                                             uintptr_t *out_region_base,
                                             size_t *out_region_size,
                                             uint32_t *out_region_shift)
{
    /* 输出参数只要有一个为空，就无法完整回传 MPU 布局结果。 */
    if ((out_region_base == 0) || (out_region_size == 0) || (out_region_shift == 0)) {
        /* 返回统一参数错误码。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 先把输出清零，避免失败路径遗留旧值。 */
    *out_region_base = 0u;
    *out_region_size = 0u;
    *out_region_shift = 0u;

    /* 原始大小必须大于 0。 */
    if (size_bytes == 0u) {
        /* 返回统一参数错误码。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* MPU 区域至少为 32 字节。 */
    size_t region_size = MRT_PORT_STM32_CM_MPU_MIN_REGION_BYTES;

    /* 计算原始范围末尾，若发生回绕则说明参数组合不合法。 */
    uintptr_t requested_end = base_address + (uintptr_t)size_bytes;
    if (requested_end < base_address) {
        /* 返回统一参数错误码。 */
        return MRT_RESULT_INVALID_ARGUMENT;
    }

    /* 逐步翻倍区域大小，直到某个 power-of-two 区域能够完整覆盖原始范围。 */
    for (;;) {
        /* 生成当前候选区域的向下对齐基址。 */
        uintptr_t region_base = base_address & ~((uintptr_t)region_size - 1u);

        /* 计算当前候选区域的末尾地址，注意这里是末尾之后的地址。 */
        uintptr_t region_end = region_base + (uintptr_t)region_size;

        /* 若末尾回绕，说明区域大小过大或平台地址空间不支持。 */
        if (region_end < region_base) {
            /* 返回统一参数错误码。 */
            return MRT_RESULT_INVALID_ARGUMENT;
        }

        /* 候选区域必须同时覆盖起点和终点。 */
        if ((region_base <= base_address) && (region_end >= requested_end)) {
            /* 写回规整后的基址。 */
            *out_region_base = region_base;

            /* 写回规整后的区域大小。 */
            *out_region_size = region_size;

            /* 计算区域大小对应的 log2 值。 */
            uint32_t shift = 0u;
            size_t temp = region_size;
            while (temp > 1u) {
                /* 逐步右移得到幂次。 */
                temp >>= 1u;
                shift++;
            }

            /* 写回 shift，供后续硬件寄存器编码使用。 */
            *out_region_shift = shift;

            /* 规整成功。 */
            return MRT_RESULT_OK;
        }

        /* 若已无法继续扩大，则返回参数错误。 */
        if (region_size > (SIZE_MAX / 2u)) {
            /* 返回统一参数错误码。 */
            return MRT_RESULT_INVALID_ARGUMENT;
        }

        /* 区域翻倍后重新尝试。 */
        region_size <<= 1u;
    }
}
