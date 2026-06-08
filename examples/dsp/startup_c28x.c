#include <stdint.h>

typedef void (*MRT_DspC2000VectorEntry)(void);

extern void CpuTimer0Isr(void);
extern void MRT_DspC2000SoftwareInterruptIsr(void);
extern void MRT_DspC2000AdcIsr(void);

/** @brief 当前已安装的 C2000 PIE/向量表指针，仅作为 smoke 诊断状态。 */
static volatile const MRT_DspC2000VectorEntry *g_mrt_dsp_c2000_installed_vectors;

/**
 * @brief 默认 DSP 中断占位处理函数。
 * @param void 无输入参数。
 * @return void 无返回值；真实板级工程通常停在此处等待调试器或复位。
 * @example
 * MRT_DspC2000DefaultIsr();
 */
void MRT_DspC2000DefaultIsr(void)
{
    /* 未接入真实 ISR 时停机，避免未知中断被静默吞掉。 */
    for (;;) {
        /* 保持在可调试位置，真实工程可在此输出故障码或触发看门狗复位。 */
    }
}

/**
 * @brief C2000 smoke 向量表骨架。
 *
 * 真实 TI C2000 工程需要把这些入口复制到 PIE 向量表或芯片启动文件指定的向量区域。
 * 当前数组只记录 MyRTOS smoke 关心的最小入口，不替代目标芯片完整向量表。
 */
const MRT_DspC2000VectorEntry g_mrt_dsp_c2000_vector_table[] = {
    CpuTimer0Isr,
    MRT_DspC2000SoftwareInterruptIsr,
    MRT_DspC2000AdcIsr
};

/**
 * @brief 安装 C2000 smoke 向量表占位。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * MRT_DspC2000InstallVectors();
 */
void MRT_DspC2000InstallVectors(void)
{
    /* 记录待安装向量表地址，真实工程应在此写入 PIE 向量表 RAM。 */
    g_mrt_dsp_c2000_installed_vectors = g_mrt_dsp_c2000_vector_table;
}

/**
 * @brief C2000 smoke 启动占位入口。
 * @param void 无输入参数。
 * @return void 无返回值。
 * @example
 * MRT_DspC2000Startup();
 */
void MRT_DspC2000Startup(void)
{
    /* 第一步安装 MyRTOS smoke 需要的 timer、软件中断和外设 ISR 入口。 */
    MRT_DspC2000InstallVectors();
}
