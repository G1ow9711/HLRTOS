/* MyRTOS C2000/C28x smoke linker command scaffold.
 *
 * 本文件只提供分区命名和 RTOS 关键段位的落点模板。真实 TI C2000 工程必须按
 * 具体芯片 datasheet、boot mode、flash sector、RAM block、CLA/DMA 共享区和编译器 ABI
 * 重新校正 origin/length。不要把这里的示例地址当作已验证板级地址。
 */

MEMORY
{
    /* codestart 入口，真实工程应匹配 boot ROM 跳转地址。 */
    BEGIN       : origin = 0x080000, length = 0x000002

    /* 示例 flash 区域，用于 .text/.cinit/.const。 */
    FLASH       : origin = 0x080002, length = 0x03FFFE

    /* 小 RAM 区域，通常放启动栈或短生命周期数据。 */
    RAMM0       : origin = 0x000122, length = 0x0002DE

    /* 本地 RAM 区域，示例放 bss、任务栈和 TCB。 */
    RAMLS       : origin = 0x008000, length = 0x004000

    /* 全局 RAM 区域，示例放 RTOS heap、DMA buffer 和 trace buffer。 */
    RAMGS       : origin = 0x00C000, length = 0x004000
}

SECTIONS
{
    /* 启动入口段，真实工程可映射到 codestart 或厂商启动文件。 */
    codestart       : > BEGIN

    /* 代码、常量和 C 初始化表放入 flash。 */
    .text           : > FLASH
    .cinit          : > FLASH
    .const          : > FLASH

    /* C 栈和普通未初始化数据放入片上 RAM。 */
    .stack          : > RAMM0
    .bss            : > RAMLS
    .ebss           : > RAMLS
    .data           : > RAMLS

    /* MyRTOS heap 独立成段，便于 smoke 记录 heap 最小剩余量。 */
    .mrtos_heap     : > RAMGS

    /* MyRTOS 任务栈/TCB 独立成段，便于检查栈边界和水位。 */
    .mrtos_tasks    : > RAMLS

    /* DMA/ADC 采样缓冲区独立成段，真实工程应按 cache/共享 RAM 策略校正。 */
    .mrtos_dma      : > RAMGS

    /* trace 事件缓冲区独立成段，便于实机 smoke 导出调度和 ISR 事件。 */
    .mrtos_trace    : > RAMGS
}
