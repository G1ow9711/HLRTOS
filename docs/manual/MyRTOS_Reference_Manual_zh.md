# MyRTOS 中文参考手册

版本：0.1-preview
适用对象：嵌入式 C 工程师、STM32 移植工程师、DSP 移植工程师、测试与安全审查人员。

## 1. 关于本手册

本手册采用嵌入式 RTOS 常见参考手册结构：先说明适用范围、调用限制和配置方法，再按 API 族分章列出函数原型、功能说明、参数、返回值、调用上下文、阻塞行为、ISR 限制、配置宏、调用示例和常见错误。本文档为 MyRTOS 原创说明，不复制其他 RTOS 的源码、注释或手册文字。

MyRTOS 当前处于 preview 阶段。已经通过 host 自动化测试的模块包括：基础类型、配置、链表、优先级位图、mock 端口、任务调度、队列、信号量、互斥锁、事件组、任务通知、软件定时器、流缓冲、消息缓冲、堆、固定块内存池、tickless、trace、运行统计、assert、STM32 Cortex-M 端口契约 helper、STM32 MPU 区域布局 helper、DSP C28x 风格端口契约 helper、DSP C28x 汇编骨架和 C2000 启动/链接/ISR glue 骨架。真实 STM32/DSP 板级 smoke test 需要按本手册移植章节接入具体硬件后补证。

## 2. API 使用规则

1. 所有返回 `MRT_Result` 的 API 都使用统一错误码。调用方必须检查返回值，不应把非 `MRT_RESULT_OK` 当作成功。
2. 带 `FromISR` 后缀的 API 只能在 ISR 上下文调用；普通可能阻塞的 API 不能在 ISR 中调用。
3. 静态创建 API 由调用方提供控制块和存储区，适合安全关键、内存确定性项目。
4. 动态创建 API 依赖 MyRTOS heap。使用前必须初始化 heap，并测试分配失败路径。
5. 任务、队列、事件、定时器等对象句柄是不透明指针，应用层不得直接改内部字段。
6. tick 单位由 `MRT_CFG_TICK_RATE_HZ` 决定。所有 timeout 参数均以 tick 为单位。
7. STM32 和 DSP 真实端口必须保持 `MRT_Port*` 公共语义：临界区可嵌套、ISR 切换延迟到中断退出、tick 只推进一次内核节拍。

## 3. 快速上手

```c
#include "myrtos/mrt_kernel.h"
#include "myrtos/mrt_task.h"

static MRT_Task app_task_storage;
static MRT_StackType app_stack[128];

static void app_task(void *arg)
{
    (void)arg;
    for (;;) {
        MRT_TaskDelay(1000u);
    }
}

int main(void)
{
    MRT_KernelInitialize();
    MRT_TaskCreateStatic("app", app_task, 0, 3u, app_stack, 128u, &app_task_storage, 0);
    MRT_KernelStart();
    for (;;) {
    }
}
```

## 4. API 参考

### MRT_KernelInitialize
- 函数原型：`MRT_Result MRT_KernelInitialize(void);`
- 功能说明：初始化内核全局状态、端口 mock 状态、任务调度器和定时器内部状态。
- 参数：无。
- 返回值：成功返回 `MRT_RESULT_OK`；重复初始化用于测试时应回到干净状态。
- 调用上下文：任务调度启动前的普通上下文。
- 阻塞行为：不阻塞。
- ISR 限制：不能在 ISR 中作为系统初始化入口调用。
- 配置宏：受 `MRT_CFG_MAX_PRIORITIES`、`MRT_CFG_TICK_RATE_HZ` 等默认配置影响。
- 调用示例：`MRT_KernelInitialize();`
- 常见错误：创建任务前未初始化内核会让调试状态不清晰；建议应用显式初始化。

### MRT_KernelStart
- 函数原型：`MRT_Result MRT_KernelStart(void);`
- 功能说明：启动调度器并请求端口层启动第一个任务。
- 参数：无。
- 返回值：成功返回 `MRT_RESULT_OK`；没有就绪任务或重复启动按实现阶段返回相应错误。
- 调用上下文：任务调度启动前。
- 阻塞行为：真实端口通常不返回；host 测试端口记录启动请求后返回。
- ISR 限制：禁止在 ISR 中调用。
- 配置宏：受抢占和时间片配置影响。
- 调用示例：`MRT_KernelStart();`
- 常见错误：未创建任何任务就启动内核。

### MRT_KernelIsRunning
- 函数原型：`bool MRT_KernelIsRunning(void);`
- 功能说明：查询调度器是否已经进入运行状态。
- 参数：无。
- 返回值：运行中返回 `true`，未运行返回 `false`。
- 调用上下文：任务上下文和测试代码均可调用。
- 阻塞行为：不阻塞。
- ISR 限制：ISR 中只建议用于诊断，不应改变调度状态。
- 配置宏：无特殊依赖。
- 调用示例：`if (MRT_KernelIsRunning()) { MRT_KernelYield(); }`
- 常见错误：把“端口启动首任务请求已记录”和“真实 CPU 已切换”混为一谈。

### MRT_KernelGetTick
- 函数原型：`MRT_Tick MRT_KernelGetTick(void);`
- 功能说明：读取当前系统 tick 计数。
- 参数：无。
- 返回值：返回当前 tick。
- 调用上下文：任务上下文、ISR 诊断和测试代码。
- 阻塞行为：不阻塞。
- ISR 限制：只读安全；不要在 ISR 中基于旧 tick 执行长逻辑。
- 配置宏：tick 单位由 `MRT_CFG_TICK_RATE_HZ` 决定。
- 调用示例：`MRT_Tick now = MRT_KernelGetTick();`
- 常见错误：用有符号整数直接比较 tick 溢出前后的大小。

### MRT_KernelTick
- 函数原型：`void MRT_KernelTick(void);`
- 功能说明：推进一个系统节拍，处理任务延时到期和软件定时器到期。
- 参数：无。
- 返回值：无。
- 调用上下文：通常由 SysTick、DSP timer ISR 或 host 仿真器调用。
- 阻塞行为：不阻塞。
- ISR 限制：真实端口应保持 ISR 短小，只做 tick 推进和必要的延迟切换请求。
- 配置宏：`MRT_CFG_TICK_RATE_HZ`。
- 调用示例：`void SysTick_Handler(void) { MRT_KernelTick(); }`
- 常见错误：在一个硬件 tick 中多次调用导致时间加速。

### MRT_KernelYield
- 函数原型：`void MRT_KernelYield(void);`
- 功能说明：请求当前任务让出 CPU，由调度器重新选择可运行任务。
- 参数：无。
- 返回值：无。
- 调用上下文：任务上下文。
- 阻塞行为：不阻塞，但可能触发上下文切换。
- ISR 限制：ISR 中应使用 `MRT_PortYieldFromISR` 或对应 FromISR API 的 `should_yield` 输出。
- 配置宏：`MRT_CFG_USE_TIME_SLICING`。
- 调用示例：`MRT_KernelYield();`
- 常见错误：在临界区内频繁 yield。

### MRT_KernelSuspendAll
- 函数原型：`MRT_Result MRT_KernelSuspendAll(void);`
- 功能说明：挂起调度器，延迟任务切换处理。
- 参数：无。
- 返回值：成功返回 `MRT_RESULT_OK`。
- 调用上下文：任务上下文。
- 阻塞行为：不阻塞。
- ISR 限制：禁止在 ISR 中调用。
- 配置宏：抢占配置会影响恢复后的切换行为。
- 调用示例：`MRT_KernelSuspendAll();`
- 常见错误：挂起后忘记恢复导致高优先级任务不能及时运行。

### MRT_KernelResumeAll
- 函数原型：`MRT_Result MRT_KernelResumeAll(void);`
- 功能说明：恢复调度器，并处理挂起期间积累的 ready 状态。
- 参数：无。
- 返回值：成功返回 `MRT_RESULT_OK`；未挂起时按实现返回状态。
- 调用上下文：任务上下文。
- 阻塞行为：不阻塞，但可能触发调度。
- ISR 限制：禁止在 ISR 中调用。
- 配置宏：抢占配置会影响是否立即切换。
- 调用示例：`MRT_KernelResumeAll();`
- 常见错误：挂起/恢复不配对。

### MRT_TaskCreateStatic
- 函数原型：`MRT_Result MRT_TaskCreateStatic(const char *name, MRT_TaskEntry entry, void *arg, MRT_Priority priority, MRT_StackType *stack, size_t stack_words, MRT_Task *storage, MRT_TaskHandle *out_task);`
- 功能说明：使用调用者提供的 TCB 和栈创建任务。
- 参数：`name` 可为空；`entry`、`stack`、`storage` 必须有效；`priority` 必须小于最大优先级。
- 返回值：成功返回 `MRT_RESULT_OK`；参数错误返回 `MRT_RESULT_INVALID_ARGUMENT`。
- 调用上下文：调度启动前或任务上下文。
- 阻塞行为：不阻塞。
- ISR 限制：禁止在 ISR 中创建任务。
- 配置宏：`MRT_CFG_SUPPORT_STATIC_ALLOCATION`、`MRT_CFG_MAX_PRIORITIES`。
- 调用示例：`MRT_TaskCreateStatic("worker", worker, 0, 4u, stack, 128u, &tcb, &task);`
- 常见错误：栈数组生命周期短于任务生命周期。

### MRT_TaskCreate
- 函数原型：`MRT_Result MRT_TaskCreate(const char *name, MRT_TaskEntry entry, void *arg, MRT_Priority priority, size_t stack_words, MRT_TaskHandle *out_task);`
- 功能说明：从 MyRTOS heap 动态分配 TCB 和栈后创建任务。
- 参数：`name` 可为空；`entry` 不能为空；`arg` 可为空；`priority` 必须小于 `MRT_CFG_MAX_PRIORITIES`；`stack_words` 必须大于 0；`out_task` 不能为空。
- 返回值：成功返回 `MRT_RESULT_OK`；动态分配关闭、heap 未初始化或空间不足返回 `MRT_RESULT_NO_MEMORY`；参数错误返回 `MRT_RESULT_INVALID_ARGUMENT`。
- 调用上下文：任务上下文或调度启动前。
- 阻塞行为：不等待任务运行；可能在 heap 内部进入短临界区。
- ISR 限制：禁止在 ISR 中调用。
- 配置宏：`MRT_CFG_SUPPORT_DYNAMIC_ALLOCATION`、`MRT_CFG_HEAP_ALIGNMENT`、`MRT_CFG_MAX_PRIORITIES`。
- 调用示例：`MRT_TaskCreate("worker", worker, 0, 4u, 256u, &task);`
- 常见错误：未初始化 heap 就调用动态创建；创建失败后继续使用旧任务句柄。

### MRT_TaskDelete
- 函数原型：`MRT_Result MRT_TaskDelete(MRT_TaskHandle task);`
- 功能说明：删除指定任务；若任务未持有互斥锁，则将其从 ready/delay/object wait 链表摘除；动态任务会释放 TCB 和栈所在堆块，静态任务只改变调度状态。若目标任务仍持有任意互斥锁，删除会被拒绝，避免互斥锁变成无有效拥有者的上锁状态。
- 参数：`task` 为目标任务句柄，不能为空。
- 返回值：成功返回 `MRT_RESULT_OK`；空句柄返回 `MRT_RESULT_INVALID_ARGUMENT`；重复删除或目标任务仍持有互斥锁时返回 `MRT_RESULT_OBJECT_BUSY`；ISR 上下文返回 `MRT_RESULT_INVALID_CONTEXT`。
- 调用上下文：任务上下文。
- 阻塞行为：删除当前任务会触发调度。
- ISR 限制：禁止在 ISR 中调用。
- 配置宏：动态释放依赖 heap 配置。
- 调用示例：`MRT_TaskDelete(task);`
- 常见错误：未先释放互斥锁就删除任务；正确流程是先让任务解锁或进入可清理状态，再调用 `MRT_TaskDelete`。

### MRT_TaskSuspend
- 函数原型：`MRT_Result MRT_TaskSuspend(MRT_TaskHandle task);`
- 功能说明：挂起目标任务，使其不参与调度；若任务正在延时或等待对象，会同步移出 delay/object wait 链表。
- 参数：目标任务句柄，不能为空。
- 返回值：成功返回 `MRT_RESULT_OK`；空句柄或已删除任务返回 `MRT_RESULT_INVALID_ARGUMENT`；ISR 上下文返回 `MRT_RESULT_INVALID_CONTEXT`。
- 调用上下文：任务上下文。
- 阻塞行为：挂起当前任务会触发调度。
- ISR 限制：禁止在 ISR 中调用。
- 配置宏：无特殊依赖。
- 调用示例：`MRT_TaskSuspend(task);`
- 常见错误：把 suspend/resume 当作事件同步工具使用。

### MRT_TaskResume
- 函数原型：`MRT_Result MRT_TaskResume(MRT_TaskHandle task);`
- 功能说明：恢复被挂起任务，使其重新参与调度。
- 参数：目标任务句柄，不能为空，且目标必须处于 `MRT_TASK_STATE_SUSPENDED`。
- 返回值：成功返回 `MRT_RESULT_OK`；空句柄返回 `MRT_RESULT_INVALID_ARGUMENT`；目标未挂起返回 `MRT_RESULT_OBJECT_BUSY`；ISR 上下文返回 `MRT_RESULT_INVALID_CONTEXT`。
- 调用上下文：任务上下文。
- 阻塞行为：不阻塞，但可能触发抢占。
- ISR 限制：ISR 中使用 `MRT_TaskResumeFromISR`。
- 配置宏：抢占配置影响恢复后是否立即切换。
- 调用示例：`MRT_TaskResume(task);`
- 常见错误：恢复一个并未挂起的任务却期望产生事件。

### MRT_TaskResumeFromISR
- 函数原型：`MRT_Result MRT_TaskResumeFromISR(MRT_TaskHandle task, bool *should_yield);`
- 功能说明：在 ISR 中恢复任务，并报告是否需要在 ISR 退出后切换。
- 参数：`task` 为目标挂起任务，不能为空；`should_yield` 可为空，非空时写出切换建议。
- 返回值：成功返回 `MRT_RESULT_OK`；空句柄返回 `MRT_RESULT_INVALID_ARGUMENT`；任务上下文调用返回 `MRT_RESULT_INVALID_CONTEXT`；目标未挂起返回 `MRT_RESULT_OBJECT_BUSY`。
- 调用上下文：ISR 上下文。
- 阻塞行为：不阻塞。
- ISR 限制：这是 ISR 专用 API。
- 配置宏：抢占配置影响 `should_yield`。
- 调用示例：`MRT_TaskResumeFromISR(task, &yield);`
- 常见错误：在任务上下文调用 FromISR 版本。

### MRT_TaskDelay
- 函数原型：`MRT_Result MRT_TaskDelay(MRT_Tick ticks);`
- 功能说明：让当前任务延时指定 tick 数。
- 参数：`ticks` 为相对延时 tick 数。
- 返回值：成功进入延时或执行 0 tick yield 返回 `MRT_RESULT_OK`；无当前任务返回 `MRT_RESULT_INVALID_CONTEXT`。
- 调用上下文：任务上下文。
- 阻塞行为：`ticks > 0` 时阻塞当前任务直到 tick 到期。
- ISR 限制：禁止在 ISR 中调用。
- 配置宏：tick 单位由 `MRT_CFG_TICK_RATE_HZ` 决定。
- 调用示例：`MRT_TaskDelay(10u);`
- 常见错误：用 `MRT_TaskDelay(0)` 代替 yield。

### MRT_TaskDelayUntil
- 函数原型：`MRT_Result MRT_TaskDelayUntil(MRT_Tick *previous, MRT_Tick period);`
- 功能说明：按固定周期延时，降低周期任务漂移。
- 参数：`previous` 保存上一周期基准 tick；`period` 为周期 tick。
- 返回值：成功等待到下一周期或周期已到期时返回 `MRT_RESULT_OK`；空指针或 0 周期返回 `MRT_RESULT_INVALID_ARGUMENT`；无当前任务或 ISR 上下文返回 `MRT_RESULT_INVALID_CONTEXT`。
- 调用上下文：任务上下文。
- 阻塞行为：到达下一周期前阻塞。
- ISR 限制：禁止在 ISR 中调用。
- 配置宏：`MRT_CFG_TICK_RATE_HZ`。
- 调用示例：`MRT_TaskDelayUntil(&last, 100u);`
- 常见错误：每次循环重新初始化 `previous`。

### MRT_TaskSetPriority
- 函数原型：`MRT_Result MRT_TaskSetPriority(MRT_TaskHandle task, MRT_Priority priority);`
- 功能说明：修改任务基础优先级并重排就绪队列。
- 参数：目标任务和新优先级。
- 返回值：成功返回 `MRT_RESULT_OK`；空句柄、已删除任务或优先级越界返回 `MRT_RESULT_INVALID_ARGUMENT`；ISR 上下文返回 `MRT_RESULT_INVALID_CONTEXT`。
- 调用上下文：任务上下文。
- 阻塞行为：不阻塞，但可能触发调度。
- ISR 限制：禁止在 ISR 中调用。
- 配置宏：`MRT_CFG_MAX_PRIORITIES`。
- 调用示例：`MRT_TaskSetPriority(task, 5u);`
- 常见错误：运行时频繁改优先级掩盖设计问题。

### MRT_TaskGetPriority
- 函数原型：`MRT_Result MRT_TaskGetPriority(MRT_TaskHandle task, MRT_Priority *out_priority);`
- 功能说明：读取任务当前有效优先级。
- 参数：`task` 可为当前任务约定值；`out_priority` 不能为空。
- 返回值：成功返回 `MRT_RESULT_OK`；参数错误返回 `MRT_RESULT_INVALID_ARGUMENT`。
- 调用上下文：任务上下文或诊断代码。
- 阻塞行为：不阻塞。
- ISR 限制：不建议在 ISR 中调用。
- 配置宏：无特殊依赖。
- 调用示例：`MRT_TaskGetPriority(task, &priority);`
- 常见错误：忽略互斥锁优先级继承导致的有效优先级变化。

### MRT_TaskGetCurrent
- 函数原型：`MRT_TaskHandle MRT_TaskGetCurrent(void);`
- 功能说明：返回当前运行任务句柄。
- 参数：无。
- 返回值：返回当前任务；调度未启动或无运行任务时返回空。
- 调用上下文：任务上下文和测试代码。
- 阻塞行为：不阻塞。
- ISR 限制：ISR 中返回值只用于诊断。
- 配置宏：无特殊依赖。
- 调用示例：`MRT_TaskHandle self = MRT_TaskGetCurrent();`
- 常见错误：在调度启动前假定当前任务非空。

### MRT_TaskGetState
- 函数原型：`MRT_Result MRT_TaskGetState(MRT_TaskHandle task, MRT_TaskState *out_state);`
- 功能说明：读取任务状态。
- 参数：任务句柄和输出状态指针。
- 返回值：成功返回 `MRT_RESULT_OK`；参数错误返回 `MRT_RESULT_INVALID_ARGUMENT`。
- 调用上下文：任务上下文或诊断代码。
- 阻塞行为：不阻塞。
- ISR 限制：不建议在 ISR 中调用。
- 配置宏：无特殊依赖。
- 调用示例：`MRT_TaskGetState(task, &state);`
- 常见错误：用状态轮询替代同步对象。

### MRT_TaskGetName
- 函数原型：`const char *MRT_TaskGetName(MRT_TaskHandle task);`
- 功能说明：读取创建任务时保存的名称指针。
- 参数：任务句柄。
- 返回值：返回名称指针；无名称或参数非法时按实现返回空。
- 调用上下文：任务上下文和诊断代码。
- 阻塞行为：不阻塞。
- ISR 限制：不建议在 ISR 中调用。
- 配置宏：无特殊依赖。
- 调用示例：`const char *name = MRT_TaskGetName(task);`
- 常见错误：传入栈上临时字符串作为任务名。

### MRT_TaskGetStackHighWaterMark
- 函数原型：`MRT_Result MRT_TaskGetStackHighWaterMark(MRT_TaskHandle task, size_t *out_words);`
- 功能说明：查询任务栈剩余水位；当前 host 模型返回任务创建时的栈容量，真实端口可通过栈涂色扩展为实际剩余水位。
- 参数：任务句柄和输出剩余栈字数。
- 返回值：成功返回 `MRT_RESULT_OK`；空句柄、已删除任务或空输出指针返回 `MRT_RESULT_INVALID_ARGUMENT`。
- 调用上下文：任务上下文或诊断任务。
- 阻塞行为：不阻塞。
- ISR 限制：不建议在 ISR 中调用。
- 配置宏：栈填充策略由端口实现决定；host 测试模型不消耗任务栈。
- 调用示例：`MRT_TaskGetStackHighWaterMark(task, &words);`
- 常见错误：未启用栈填充却依赖水位结果。

### MRT_QueueCreateStatic
- 函数原型：`MRT_Result MRT_QueueCreateStatic(size_t item_size, size_t capacity, void *buffer, size_t buffer_size, MRT_Queue *storage, MRT_QueueHandle *out_queue);`
- 功能说明：使用外部存储创建固定长度复制队列。
- 参数：元素大小、容量、元素缓冲区、控制块和输出句柄。
- 返回值：成功返回 `MRT_RESULT_OK`；参数或容量非法返回 `MRT_RESULT_INVALID_ARGUMENT`。
- 调用上下文：调度启动前或任务上下文。
- 阻塞行为：不阻塞。
- ISR 限制：禁止在 ISR 中创建队列。
- 配置宏：`MRT_CFG_SUPPORT_STATIC_ALLOCATION`。
- 调用示例：`MRT_QueueCreateStatic(sizeof(msg), 8u, buf, sizeof(buf), &queue_storage, &queue);`
- 常见错误：缓冲区字节数小于 `item_size * capacity`。

### MRT_QueueCreate
- 函数原型：`MRT_Result MRT_QueueCreate(size_t item_size, size_t capacity, MRT_QueueHandle *out_queue);`
- 功能说明：从 heap 动态创建复制队列。
- 参数：元素大小、容量和输出句柄。
- 返回值：成功返回 `MRT_RESULT_OK`；内存不足返回 `MRT_RESULT_NO_MEMORY`。
- 调用上下文：任务上下文或调度启动前。
- 阻塞行为：不阻塞。
- ISR 限制：禁止在 ISR 中调用。
- 配置宏：`MRT_CFG_SUPPORT_DYNAMIC_ALLOCATION`。
- 调用示例：`MRT_QueueCreate(sizeof(msg), 8u, &queue);`
- 常见错误：未初始化 heap 或容量为 0。

### MRT_QueueDelete
- 函数原型：`MRT_Result MRT_QueueDelete(MRT_QueueHandle queue);`
- 功能说明：删除动态队列或拒绝删除静态队列。
- 参数：队列句柄。
- 返回值：动态队列删除成功返回 `MRT_RESULT_OK`；静态队列返回对象忙或相应错误。
- 调用上下文：任务上下文。
- 阻塞行为：不阻塞。
- ISR 限制：禁止在 ISR 中调用。
- 配置宏：动态队列依赖 heap。
- 调用示例：`MRT_QueueDelete(queue);`
- 常见错误：删除仍有任务等待的队列而未定义唤醒策略。

### MRT_QueueSend
- 函数原型：`MRT_Result MRT_QueueSend(MRT_QueueHandle queue, const void *item, MRT_Timeout timeout);`
- 功能说明：把一个元素复制到队列尾部。
- 参数：队列句柄、元素地址和等待 tick。
- 返回值：成功返回 `MRT_RESULT_OK`；满队列且不等待返回 `MRT_RESULT_OBJECT_FULL`；超时返回 `MRT_RESULT_TIMEOUT`。
- 调用上下文：任务上下文。
- 阻塞行为：队列满且 timeout 非 0 时可阻塞。
- ISR 限制：ISR 中使用 `MRT_QueueSendFromISR`。
- 配置宏：tick 频率影响 timeout。
- 调用示例：`MRT_QueueSend(queue, &msg, 10u);`
- 常见错误：发送栈对象指针却把队列元素大小配置为指针大小之外的值。

### MRT_QueueSendFront
- 函数原型：`MRT_Result MRT_QueueSendFront(MRT_QueueHandle queue, const void *item, MRT_Timeout timeout);`
- 功能说明：把元素复制到队列头部。
- 参数：队列句柄、元素地址和等待 tick。
- 返回值：同 `MRT_QueueSend`。
- 调用上下文：任务上下文。
- 阻塞行为：队列满且 timeout 非 0 时可阻塞。
- ISR 限制：不用于 ISR。
- 配置宏：无特殊依赖。
- 调用示例：`MRT_QueueSendFront(queue, &urgent, 0u);`
- 常见错误：滥用队头插入破坏消息公平性。

### MRT_QueueOverwrite
- 函数原型：`MRT_Result MRT_QueueOverwrite(MRT_QueueHandle queue, const void *item);`
- 功能说明：覆盖单槽队列中的元素。
- 参数：队列句柄和元素地址。
- 返回值：成功返回 `MRT_RESULT_OK`；非单槽队列返回参数或对象错误。
- 调用上下文：任务上下文。
- 阻塞行为：不阻塞。
- ISR 限制：不用于 ISR。
- 配置宏：无特殊依赖。
- 调用示例：`MRT_QueueOverwrite(latest_value_queue, &sample);`
- 常见错误：对多槽队列使用 overwrite。

### MRT_QueueReceive
- 函数原型：`MRT_Result MRT_QueueReceive(MRT_QueueHandle queue, void *out_item, MRT_Timeout timeout);`
- 功能说明：从队列头部复制并移除一个元素。
- 参数：队列句柄、输出缓冲区和等待 tick。
- 返回值：成功返回 `MRT_RESULT_OK`；空队列不等待返回 `MRT_RESULT_OBJECT_EMPTY`；超时返回 `MRT_RESULT_TIMEOUT`。
- 调用上下文：任务上下文。
- 阻塞行为：队列空且 timeout 非 0 时可阻塞。
- ISR 限制：ISR 中使用 `MRT_QueueReceiveFromISR`。
- 配置宏：tick 频率影响 timeout。
- 调用示例：`MRT_QueueReceive(queue, &msg, 20u);`
- 常见错误：输出缓冲区大小小于队列元素大小。

### MRT_QueuePeek
- 函数原型：`MRT_Result MRT_QueuePeek(MRT_QueueHandle queue, void *out_item, MRT_Timeout timeout);`
- 功能说明：复制队列头部元素但不移除。
- 参数：队列句柄、输出缓冲区和等待 tick。
- 返回值：同接收 API。
- 调用上下文：任务上下文。
- 阻塞行为：队列空且 timeout 非 0 时可阻塞。
- ISR 限制：不用于 ISR。
- 配置宏：无特殊依赖。
- 调用示例：`MRT_QueuePeek(queue, &msg, 0u);`
- 常见错误：peek 后误以为元素已被消费。

### MRT_QueueSendFromISR
- 函数原型：`MRT_Result MRT_QueueSendFromISR(MRT_QueueHandle queue, const void *item, bool *should_yield);`
- 功能说明：在 ISR 中非阻塞发送队列元素。
- 参数：队列句柄、元素地址和可选切换输出。
- 返回值：成功返回 `MRT_RESULT_OK`；队列满返回 `MRT_RESULT_OBJECT_FULL`；上下文错误返回 `MRT_RESULT_INVALID_CONTEXT`。
- 调用上下文：ISR 上下文。
- 阻塞行为：不阻塞。
- ISR 限制：这是 ISR 专用 API。
- 配置宏：抢占配置影响 `should_yield`。
- 调用示例：`MRT_QueueSendFromISR(queue, &msg, &yield);`
- 常见错误：在任务上下文调用 FromISR 版本。

### MRT_QueueReceiveFromISR
- 函数原型：`MRT_Result MRT_QueueReceiveFromISR(MRT_QueueHandle queue, void *out_item, bool *should_yield);`
- 功能说明：在 ISR 中非阻塞接收队列元素。
- 参数：队列句柄、输出缓冲区和可选切换输出。
- 返回值：成功返回 `MRT_RESULT_OK`；队列空返回 `MRT_RESULT_OBJECT_EMPTY`。
- 调用上下文：ISR 上下文。
- 阻塞行为：不阻塞。
- ISR 限制：这是 ISR 专用 API。
- 配置宏：无特殊依赖。
- 调用示例：`MRT_QueueReceiveFromISR(queue, &msg, &yield);`
- 常见错误：在 ISR 中等待队列数据。

### MRT_QueueMessagesWaiting
- 函数原型：`size_t MRT_QueueMessagesWaiting(MRT_QueueHandle queue);`
- 功能说明：查询队列已有元素数量。
- 参数：队列句柄。
- 返回值：返回消息数量；空句柄返回 0。
- 调用上下文：任务上下文或诊断代码。
- 阻塞行为：不阻塞。
- ISR 限制：ISR 中仅建议用于诊断。
- 配置宏：无特殊依赖。
- 调用示例：`size_t used = MRT_QueueMessagesWaiting(queue);`
- 常见错误：用轮询数量代替阻塞接收。

### MRT_QueueSpacesAvailable
- 函数原型：`size_t MRT_QueueSpacesAvailable(MRT_QueueHandle queue);`
- 功能说明：查询队列剩余可写槽位数量。
- 参数：队列句柄。
- 返回值：返回剩余槽位；空句柄返回 0。
- 调用上下文：任务上下文或诊断代码。
- 阻塞行为：不阻塞。
- ISR 限制：ISR 中仅建议用于诊断。
- 配置宏：无特殊依赖。
- 调用示例：`size_t free_slots = MRT_QueueSpacesAvailable(queue);`
- 常见错误：查询到空间后假定后续发送一定成功。

### MRT_QueueReset
- 函数原型：`MRT_Result MRT_QueueReset(MRT_QueueHandle queue);`
- 功能说明：清空队列读写索引和元素计数。
- 参数：队列句柄。
- 返回值：成功返回 `MRT_RESULT_OK`；参数错误返回 `MRT_RESULT_INVALID_ARGUMENT`。
- 调用上下文：任务上下文。
- 阻塞行为：不阻塞。
- ISR 限制：不建议在 ISR 中重置。
- 配置宏：无特殊依赖。
- 调用示例：`MRT_QueueReset(queue);`
- 常见错误：重置仍有等待任务的队列。

### MRT_SemaphoreCreateBinaryStatic
- 函数原型：`MRT_Result MRT_SemaphoreCreateBinaryStatic(bool initially_available, MRT_Semaphore *storage, MRT_SemaphoreHandle *out_semaphore);`
- 功能说明：使用调用方提供的控制块创建静态二值信号量。
- 参数：初始可用标志、控制块和输出句柄。
- 返回值：成功返回 `MRT_RESULT_OK`；参数错误返回 `MRT_RESULT_INVALID_ARGUMENT`。
- 调用上下文：调度启动前或任务上下文。
- 阻塞行为：不阻塞。
- ISR 限制：禁止在 ISR 中创建。
- 配置宏：静态分配支持。
- 调用示例：`MRT_SemaphoreCreateBinaryStatic(false, &sem_storage, &sem);`
- 常见错误：把二值信号量当互斥锁使用。

### MRT_SemaphoreCreateCountingStatic
- 函数原型：`MRT_Result MRT_SemaphoreCreateCountingStatic(size_t max_count, size_t initial_count, MRT_Semaphore *storage, MRT_SemaphoreHandle *out_semaphore);`
- 功能说明：创建静态计数信号量。
- 参数：控制块、最大计数、初始计数和输出句柄。
- 返回值：成功返回 `MRT_RESULT_OK`；计数非法返回 `MRT_RESULT_INVALID_ARGUMENT`。
- 调用上下文：调度启动前或任务上下文。
- 阻塞行为：不阻塞。
- ISR 限制：禁止在 ISR 中创建。
- 配置宏：静态分配支持。
- 调用示例：`MRT_SemaphoreCreateCountingStatic(4u, 0u, &sem_storage, &sem);`
- 常见错误：初始计数大于最大计数。

### MRT_SemaphoreCreateBinary
- 函数原型：`MRT_Result MRT_SemaphoreCreateBinary(bool initially_available, MRT_SemaphoreHandle *out_semaphore);`
- 功能说明：从 MyRTOS heap 动态分配一个信号量控制块，并初始化为二值信号量。
- 参数：初始可用标志和输出句柄；创建失败时输出句柄会被清空。
- 返回值：成功返回 `MRT_RESULT_OK`；参数错误返回 `MRT_RESULT_INVALID_ARGUMENT`；动态分配关闭、heap 未初始化或空间不足返回 `MRT_RESULT_NO_MEMORY`。
- 调用上下文：任务上下文。
- 阻塞行为：不阻塞。
- ISR 限制：禁止在 ISR 中创建。
- 配置宏：动态分配支持和 heap。
- 调用示例：`MRT_SemaphoreCreateBinary(false, &sem);`
- 常见错误：未初始化 heap 就创建动态对象；失败后继续使用旧句柄。

### MRT_SemaphoreCreateCounting
- 函数原型：`MRT_Result MRT_SemaphoreCreateCounting(size_t max_count, size_t initial_count, MRT_SemaphoreHandle *out_semaphore);`
- 功能说明：从 MyRTOS heap 动态分配一个信号量控制块，并初始化为计数信号量。
- 参数：最大计数、初始计数和输出句柄；`max_count` 必须大于 0，`initial_count` 不得大于 `max_count`。
- 返回值：成功返回 `MRT_RESULT_OK`；参数错误返回 `MRT_RESULT_INVALID_ARGUMENT`；动态分配关闭、heap 未初始化或空间不足返回 `MRT_RESULT_NO_MEMORY`。
- 调用上下文：任务上下文。
- 阻塞行为：不阻塞。
- ISR 限制：禁止在 ISR 中创建。
- 配置宏：动态分配支持和 heap。
- 调用示例：`MRT_SemaphoreCreateCounting(8u, 0u, &sem);`
- 常见错误：把计数信号量用于需要优先级继承的资源保护。

### MRT_SemaphoreDelete
- 函数原型：`MRT_Result MRT_SemaphoreDelete(MRT_SemaphoreHandle semaphore);`
- 功能说明：删除动态信号量或拒绝删除静态信号量。
- 参数：信号量句柄。
- 返回值：动态信号量释放成功返回 `MRT_RESULT_OK`；空句柄返回 `MRT_RESULT_INVALID_ARGUMENT`；静态信号量或仍有等待任务时返回 `MRT_RESULT_OBJECT_BUSY`。
- 调用上下文：任务上下文。
- 阻塞行为：不阻塞。
- ISR 限制：禁止在 ISR 中调用。
- 配置宏：动态分配支持。
- 调用示例：`MRT_SemaphoreDelete(sem);`
- 常见错误：删除仍有任务等待的信号量。

### MRT_SemaphoreTake
- 函数原型：`MRT_Result MRT_SemaphoreTake(MRT_SemaphoreHandle semaphore, MRT_Timeout timeout);`
- 功能说明：获取一个信号量计数。
- 参数：信号量句柄和等待 tick。
- 返回值：成功返回 `MRT_RESULT_OK`；无计数且不等待返回 `MRT_RESULT_OBJECT_EMPTY`；超时返回 `MRT_RESULT_TIMEOUT`。
- 调用上下文：任务上下文。
- 阻塞行为：计数为 0 且 timeout 非 0 时可阻塞。
- ISR 限制：ISR 中不应 take。
- 配置宏：tick 频率影响 timeout。
- 调用示例：`MRT_SemaphoreTake(sem, 100u);`
- 常见错误：在 ISR 中等待信号量。

### MRT_SemaphoreGive
- 函数原型：`MRT_Result MRT_SemaphoreGive(MRT_SemaphoreHandle semaphore);`
- 功能说明：释放一个信号量计数并唤醒等待任务。
- 参数：信号量句柄。
- 返回值：成功返回 `MRT_RESULT_OK`；计数已满返回 `MRT_RESULT_OBJECT_FULL`。
- 调用上下文：任务上下文。
- 阻塞行为：不阻塞，但可能触发调度。
- ISR 限制：ISR 中使用 `MRT_SemaphoreGiveFromISR`。
- 配置宏：抢占配置影响唤醒后切换。
- 调用示例：`MRT_SemaphoreGive(sem);`
- 常见错误：give 次数超过最大计数。

### MRT_SemaphoreGiveFromISR
- 函数原型：`MRT_Result MRT_SemaphoreGiveFromISR(MRT_SemaphoreHandle semaphore, bool *should_yield);`
- 功能说明：在 ISR 中释放信号量并报告延迟切换建议。
- 参数：信号量句柄和可选切换输出。
- 返回值：成功返回 `MRT_RESULT_OK`；上下文错误返回 `MRT_RESULT_INVALID_CONTEXT`。
- 调用上下文：ISR 上下文。
- 阻塞行为：不阻塞。
- ISR 限制：这是 ISR 专用 API。
- 配置宏：抢占配置影响 `should_yield`。
- 调用示例：`MRT_SemaphoreGiveFromISR(sem, &yield);`
- 常见错误：在任务上下文调用 FromISR 版本。

### MRT_SemaphoreGetCount
- 函数原型：`uint32_t MRT_SemaphoreGetCount(MRT_SemaphoreHandle semaphore);`
- 功能说明：读取当前信号量计数。
- 参数：信号量句柄。
- 返回值：返回当前计数；空句柄返回 0。
- 调用上下文：任务上下文或诊断代码。
- 阻塞行为：不阻塞。
- ISR 限制：ISR 中仅建议用于诊断。
- 配置宏：无特殊依赖。
- 调用示例：`uint32_t count = MRT_SemaphoreGetCount(sem);`
- 常见错误：用计数轮询替代阻塞等待。

### MRT_MutexCreateStatic
- 函数原型：`MRT_Result MRT_MutexCreateStatic(MRT_Mutex *storage, MRT_MutexHandle *out_mutex);`
- 功能说明：创建静态普通互斥锁。
- 参数：互斥锁控制块和输出句柄。
- 返回值：成功返回 `MRT_RESULT_OK`；参数错误返回 `MRT_RESULT_INVALID_ARGUMENT`。
- 调用上下文：调度启动前或任务上下文。
- 阻塞行为：不阻塞。
- ISR 限制：禁止在 ISR 中创建。
- 配置宏：静态分配支持。
- 调用示例：`MRT_MutexCreateStatic(&mutex_storage, &mutex);`
- 常见错误：在 ISR 中使用互斥锁。

### MRT_MutexCreate
- 函数原型：`MRT_Result MRT_MutexCreate(MRT_MutexHandle *out_mutex);`
- 功能说明：从 MyRTOS heap 动态分配一个互斥锁控制块，并初始化为普通互斥锁。
- 参数：输出句柄；创建失败时输出句柄会被清空。
- 返回值：成功返回 `MRT_RESULT_OK`；参数错误返回 `MRT_RESULT_INVALID_ARGUMENT`；动态分配关闭、heap 未初始化或空间不足返回 `MRT_RESULT_NO_MEMORY`。
- 调用上下文：任务上下文。
- 阻塞行为：不阻塞。
- ISR 限制：禁止在 ISR 中调用。
- 配置宏：动态分配支持和 heap。
- 调用示例：`MRT_MutexCreate(&mutex);`
- 常见错误：未处理动态分配失败。

### MRT_MutexCreateRecursiveStatic
- 函数原型：`MRT_Result MRT_MutexCreateRecursiveStatic(MRT_Mutex *storage, MRT_MutexHandle *out_mutex);`
- 功能说明：创建静态递归互斥锁。
- 参数：控制块和输出句柄。
- 返回值：成功返回 `MRT_RESULT_OK`；参数错误返回 `MRT_RESULT_INVALID_ARGUMENT`。
- 调用上下文：任务上下文或调度启动前。
- 阻塞行为：不阻塞。
- ISR 限制：禁止在 ISR 中创建。
- 配置宏：静态分配支持。
- 调用示例：`MRT_MutexCreateRecursiveStatic(&storage, &mutex);`
- 常见错误：递归加锁后释放次数不足。

### MRT_MutexCreateRecursive
- 函数原型：`MRT_Result MRT_MutexCreateRecursive(MRT_MutexHandle *out_mutex);`
- 功能说明：从 MyRTOS heap 动态分配一个互斥锁控制块，并初始化为递归互斥锁。
- 参数：输出句柄；创建失败时输出句柄会被清空。
- 返回值：成功返回 `MRT_RESULT_OK`；参数错误返回 `MRT_RESULT_INVALID_ARGUMENT`；动态分配关闭、heap 未初始化或空间不足返回 `MRT_RESULT_NO_MEMORY`。
- 调用上下文：任务上下文。
- 阻塞行为：不阻塞。
- ISR 限制：禁止在 ISR 中调用。
- 配置宏：动态分配支持。
- 调用示例：`MRT_MutexCreateRecursive(&mutex);`
- 常见错误：递归互斥锁掩盖模块边界设计问题。

### MRT_MutexDelete
- 函数原型：`MRT_Result MRT_MutexDelete(MRT_MutexHandle mutex);`
- 功能说明：删除动态互斥锁或拒绝删除静态互斥锁。
- 参数：互斥锁句柄。
- 返回值：动态互斥锁释放成功返回 `MRT_RESULT_OK`；空句柄返回 `MRT_RESULT_INVALID_ARGUMENT`；静态互斥锁、已被持有的互斥锁或仍有等待任务时返回 `MRT_RESULT_OBJECT_BUSY`。
- 调用上下文：任务上下文。
- 阻塞行为：不阻塞。
- ISR 限制：禁止在 ISR 中调用。
- 配置宏：动态分配支持。
- 调用示例：`MRT_MutexDelete(mutex);`
- 常见错误：删除仍被持有的互斥锁。

### MRT_MutexLock
- 函数原型：`MRT_Result MRT_MutexLock(MRT_MutexHandle mutex, MRT_Timeout timeout);`
- 功能说明：获取互斥锁，必要时对拥有者执行优先级继承；等待者因 tick 超时离开等待链表后，拥有者有效优先级会按剩余等待者重新计算，若无更高优先级等待者则回落到基础优先级。
- 参数：互斥锁句柄和等待 tick。
- 返回值：成功返回 `MRT_RESULT_OK`；超时、上下文非法或所有权错误返回相应错误。
- 调用上下文：任务上下文。
- 阻塞行为：锁被占用且 timeout 非 0 时可阻塞。
- ISR 限制：禁止在 ISR 中调用。
- 配置宏：抢占配置影响继承后的调度。
- 调用示例：`MRT_MutexLock(mutex, 50u);`
- 常见错误：持锁期间调用不可控阻塞 API。

### MRT_MutexUnlock
- 函数原型：`MRT_Result MRT_MutexUnlock(MRT_MutexHandle mutex);`
- 功能说明：释放互斥锁并恢复或转交所有权。
- 参数：互斥锁句柄。
- 返回值：成功返回 `MRT_RESULT_OK`；非拥有者释放返回 `MRT_RESULT_OWNER_ERROR`。
- 调用上下文：任务上下文。
- 阻塞行为：不阻塞，但可能唤醒等待任务。
- ISR 限制：禁止在 ISR 中调用。
- 配置宏：抢占配置影响唤醒后切换。
- 调用示例：`MRT_MutexUnlock(mutex);`
- 常见错误：由非拥有任务释放互斥锁。

### MRT_MutexGetOwner
- 函数原型：`MRT_Result MRT_MutexGetOwner(MRT_MutexHandle mutex, MRT_TaskHandle *out_owner);`
- 功能说明：查询当前互斥锁拥有者。
- 参数：互斥锁句柄和输出拥有者指针。
- 返回值：查询成功返回 `MRT_RESULT_OK`，并通过 `out_owner` 写出拥有者任务；无拥有者时写出空；参数非法返回 `MRT_RESULT_INVALID_ARGUMENT`。
- 调用上下文：任务上下文或诊断代码。
- 阻塞行为：不阻塞。
- ISR 限制：ISR 中仅建议用于诊断。
- 配置宏：无特殊依赖。
- 调用示例：`MRT_TaskHandle owner; MRT_MutexGetOwner(mutex, &owner);`
- 常见错误：用 owner 轮询替代锁操作。

### MRT_EventGroupCreateStatic
- 函数原型：`MRT_Result MRT_EventGroupCreateStatic(MRT_EventGroup *storage, MRT_EventGroupHandle *out_group);`
- 功能说明：创建静态事件组。
- 参数：事件组控制块和输出句柄。
- 返回值：成功返回 `MRT_RESULT_OK`；参数错误返回 `MRT_RESULT_INVALID_ARGUMENT`。
- 调用上下文：任务上下文或调度启动前。
- 阻塞行为：不阻塞。
- ISR 限制：禁止在 ISR 中创建。
- 配置宏：静态分配支持。
- 调用示例：`MRT_EventGroupCreateStatic(&storage, &group);`
- 常见错误：把事件组 bit 当作计数信号量。

### MRT_EventGroupCreate
- 函数原型：`MRT_Result MRT_EventGroupCreate(MRT_EventGroupHandle *out_group);`
- 功能说明：从 MyRTOS heap 动态分配一个事件组控制块，并初始化 bit 集合为 0。
- 参数：输出句柄；创建失败时输出句柄会被清空。
- 返回值：成功返回 `MRT_RESULT_OK`；参数错误返回 `MRT_RESULT_INVALID_ARGUMENT`；动态分配关闭、heap 未初始化或空间不足返回 `MRT_RESULT_NO_MEMORY`。
- 调用上下文：任务上下文。
- 阻塞行为：不阻塞。
- ISR 限制：禁止在 ISR 中调用。
- 配置宏：动态分配支持。
- 调用示例：`MRT_EventGroupCreate(&group);`
- 常见错误：未处理 heap 失败。

### MRT_EventGroupDelete
- 函数原型：`MRT_Result MRT_EventGroupDelete(MRT_EventGroupHandle group);`
- 功能说明：删除动态事件组并归还控制块；静态事件组不由该 API 释放。
- 参数：事件组句柄。
- 返回值：动态事件组释放成功返回 `MRT_RESULT_OK`；空句柄返回 `MRT_RESULT_INVALID_ARGUMENT`；静态事件组或仍有等待任务时返回 `MRT_RESULT_OBJECT_BUSY`。
- 调用上下文：任务上下文。
- 阻塞行为：不阻塞。
- ISR 限制：禁止在 ISR 中调用。
- 配置宏：动态分配支持。
- 调用示例：`MRT_EventGroupDelete(group);`
- 常见错误：删除仍有任务等待的事件组。

### MRT_EventGroupSetBits
- 函数原型：`MRT_Result MRT_EventGroupSetBits(MRT_EventGroupHandle group, MRT_EventBits bits_to_set, MRT_EventBits *out_bits);`
- 功能说明：置位事件 bit，并唤醒满足条件的等待任务。
- 参数：事件组句柄、要置位的 bit 和可选的置位后 bit 快照输出。
- 返回值：成功返回 `MRT_RESULT_OK`；参数错误返回 `MRT_RESULT_INVALID_ARGUMENT`。
- 调用上下文：任务上下文。
- 阻塞行为：不阻塞，但可能触发调度。
- ISR 限制：ISR 中使用 `MRT_EventGroupSetBitsFromISR`。
- 配置宏：抢占配置影响唤醒后切换。
- 调用示例：`MRT_EventBits bits; MRT_EventGroupSetBits(group, READY_BIT, &bits);`
- 常见错误：用同一个 bit 表示多个独立事件。

### MRT_EventGroupClearBits
- 函数原型：`MRT_Result MRT_EventGroupClearBits(MRT_EventGroupHandle group, MRT_EventBits bits_to_clear, MRT_EventBits *out_bits);`
- 功能说明：清除事件组中的指定 bit。
- 参数：事件组句柄、要清除的 bit 和可选的清除后 bit 快照输出。
- 返回值：成功返回 `MRT_RESULT_OK`；参数错误返回 `MRT_RESULT_INVALID_ARGUMENT`。
- 调用上下文：任务上下文。
- 阻塞行为：不阻塞。
- ISR 限制：不建议在 ISR 中清位。
- 配置宏：无特殊依赖。
- 调用示例：`MRT_EventBits bits; MRT_EventGroupClearBits(group, READY_BIT, &bits);`
- 常见错误：清除其他任务仍需要观察的 bit。

### MRT_EventGroupWaitBits
- 函数原型：`MRT_Result MRT_EventGroupWaitBits(MRT_EventGroupHandle group, MRT_EventBits bits, bool wait_all, bool clear_on_exit, MRT_Timeout timeout, MRT_EventBits *out_bits);`
- 功能说明：等待事件组 bit 满足 wait-any 或 wait-all 条件。
- 参数：事件组、目标 bit、等待模式、退出清位标志、timeout 和输出快照。
- 返回值：满足条件返回 `MRT_RESULT_OK`；超时返回 `MRT_RESULT_TIMEOUT`。
- 调用上下文：任务上下文。
- 阻塞行为：条件不满足且 timeout 非 0 时阻塞。
- ISR 限制：禁止在 ISR 中等待。
- 配置宏：tick 频率影响 timeout。
- 调用示例：`MRT_EventGroupWaitBits(group, mask, true, true, 50u, &snapshot);`
- 常见错误：wait-all mask 中包含永远不会置位的 bit。

### MRT_EventGroupSetBitsFromISR
- 函数原型：`MRT_Result MRT_EventGroupSetBitsFromISR(MRT_EventGroupHandle group, MRT_EventBits bits, bool *should_yield);`
- 功能说明：在 ISR 中置位事件 bit，并报告是否应延迟切换。
- 参数：事件组、要置位的 bit 和可选切换输出。
- 返回值：成功返回 `MRT_RESULT_OK`；上下文错误返回 `MRT_RESULT_INVALID_CONTEXT`。
- 调用上下文：ISR 上下文。
- 阻塞行为：不阻塞。
- ISR 限制：这是 ISR 专用 API。
- 配置宏：抢占配置影响 `should_yield`。
- 调用示例：`MRT_EventGroupSetBitsFromISR(group, RX_BIT, &yield);`
- 常见错误：在任务上下文调用 FromISR 版本。

### MRT_EventGroupGetBits
- 函数原型：`MRT_EventBits MRT_EventGroupGetBits(MRT_EventGroupHandle group);`
- 功能说明：读取事件组当前 bit。
- 参数：事件组句柄。
- 返回值：返回当前 bit；空句柄返回 0。
- 调用上下文：任务上下文或诊断代码。
- 阻塞行为：不阻塞。
- ISR 限制：ISR 中仅建议用于诊断。
- 配置宏：无特殊依赖。
- 调用示例：`MRT_EventBits bits = MRT_EventGroupGetBits(group);`
- 常见错误：读取后假定 bit 不会被其他任务改变。

### MRT_TaskNotify
- 函数原型：`MRT_Result MRT_TaskNotify(MRT_TaskHandle task, MRT_NotifyValue value, MRT_TaskNotifyAction action);`
- 功能说明：向目标任务发送轻量通知。
- 参数：任务句柄、通知值和动作。
- 返回值：成功返回 `MRT_RESULT_OK`；no-overwrite 遇到未读通知返回 `MRT_RESULT_OBJECT_BUSY`。
- 调用上下文：任务上下文。
- 阻塞行为：不阻塞，但可能唤醒任务。
- ISR 限制：ISR 中使用 `MRT_TaskNotifyFromISR`。
- 配置宏：抢占配置影响唤醒后切换。
- 调用示例：`MRT_TaskNotify(task, BIT0, MRT_NOTIFY_SET_BITS);`
- 常见错误：多个生产者混用 overwrite 导致事件丢失。

### MRT_TaskNotifyFromISR
- 函数原型：`MRT_Result MRT_TaskNotifyFromISR(MRT_TaskHandle task, MRT_NotifyValue value, MRT_TaskNotifyAction action, bool *should_yield);`
- 功能说明：在 ISR 中发送任务通知。
- 参数：任务句柄、通知值、动作和可选切换输出。
- 返回值：成功返回 `MRT_RESULT_OK`；上下文错误返回 `MRT_RESULT_INVALID_CONTEXT`。
- 调用上下文：ISR 上下文。
- 阻塞行为：不阻塞。
- ISR 限制：这是 ISR 专用 API。
- 配置宏：抢占配置影响 `should_yield`。
- 调用示例：`MRT_TaskNotifyFromISR(task, 1u, MRT_NOTIFY_INCREMENT, &yield);`
- 常见错误：在任务上下文调用 FromISR 版本。

### MRT_TaskNotifyWait
- 函数原型：`MRT_Result MRT_TaskNotifyWait(MRT_NotifyValue clear_on_entry, MRT_NotifyValue clear_on_exit, MRT_Timeout timeout, MRT_NotifyValue *out_value);`
- 功能说明：等待当前任务收到通知并读取通知值。
- 参数：进入清位 mask、退出清位 mask、timeout 和输出值。
- 返回值：收到通知返回 `MRT_RESULT_OK`；超时返回 `MRT_RESULT_TIMEOUT`。
- 调用上下文：任务上下文。
- 阻塞行为：无 pending 通知且 timeout 非 0 时阻塞。
- ISR 限制：禁止在 ISR 中等待。
- 配置宏：tick 频率影响 timeout。
- 调用示例：`MRT_TaskNotifyWait(0u, UINT32_MAX, 100u, &value);`
- 常见错误：退出清位 mask 设置过宽导致未处理 bit 被清除。

### MRT_TaskNotifyTake
- 函数原型：`MRT_Result MRT_TaskNotifyTake(bool clear_count_on_exit, MRT_Timeout timeout, MRT_NotifyValue *out_value);`
- 功能说明：把任务通知值当作轻量计数信号量获取。
- 参数：退出时是否清零、timeout 和输出值。
- 返回值：成功返回 `MRT_RESULT_OK`；超时返回 `MRT_RESULT_TIMEOUT`。
- 调用上下文：任务上下文。
- 阻塞行为：计数为 0 且 timeout 非 0 时阻塞。
- ISR 限制：禁止在 ISR 中等待。
- 配置宏：tick 频率影响 timeout。
- 调用示例：`MRT_TaskNotifyTake(true, 20u, &count);`
- 常见错误：同时把通知值当 bit 和计数使用。

### MRT_TaskNotifyStateClear
- 函数原型：`MRT_Result MRT_TaskNotifyStateClear(MRT_TaskHandle task);`
- 功能说明：清除目标任务的通知 pending 状态。
- 参数：任务句柄。
- 返回值：成功返回 `MRT_RESULT_OK`；参数错误返回 `MRT_RESULT_INVALID_ARGUMENT`。
- 调用上下文：任务上下文。
- 阻塞行为：不阻塞。
- ISR 限制：不建议在 ISR 中调用。
- 配置宏：无特殊依赖。
- 调用示例：`MRT_TaskNotifyStateClear(task);`
- 常见错误：清除 pending 后遗漏还未处理的通知值。

### MRT_TaskNotifyValueClear
- 函数原型：`MRT_Result MRT_TaskNotifyValueClear(MRT_TaskHandle task, MRT_NotifyValue bits);`
- 功能说明：清除目标任务通知值中的指定 bit。
- 参数：任务句柄和清除 mask。
- 返回值：成功返回 `MRT_RESULT_OK`；参数错误返回 `MRT_RESULT_INVALID_ARGUMENT`。
- 调用上下文：任务上下文。
- 阻塞行为：不阻塞。
- ISR 限制：不建议在 ISR 中调用。
- 配置宏：无特殊依赖。
- 调用示例：`MRT_TaskNotifyValueClear(task, DONE_BIT);`
- 常见错误：把清位 mask 写成保留 bit。

### MRT_TimerCreateStatic
- 函数原型：`MRT_Result MRT_TimerCreateStatic(const char *name, MRT_Tick period_ticks, bool auto_reload, void *arg, MRT_TimerCallback callback, MRT_Timer *storage, MRT_TimerHandle *out_timer);`
- 功能说明：创建静态软件定时器。
- 参数：名称、周期、自动重载标志、用户参数、回调、控制块和输出句柄。
- 返回值：成功返回 `MRT_RESULT_OK`；周期为 0 或回调为空返回 `MRT_RESULT_INVALID_ARGUMENT`。
- 调用上下文：任务上下文或调度启动前。
- 阻塞行为：不阻塞。
- ISR 限制：禁止在 ISR 中创建。
- 配置宏：静态分配支持。
- 调用示例：`MRT_TimerCreateStatic("blink", 100u, true, 0, cb, &storage, &timer);`
- 常见错误：在回调中执行长时间阻塞操作。

### MRT_TimerCreate
- 函数原型：`MRT_Result MRT_TimerCreate(const char *name, MRT_Tick period_ticks, bool auto_reload, void *arg, MRT_TimerCallback callback, MRT_TimerHandle *out_timer);`
- 功能说明：从 MyRTOS heap 动态分配一个软件定时器控制块，并初始化定时器参数。
- 参数：名称、周期、自动重载标志、用户参数、回调和输出句柄；周期必须大于 0，回调和输出句柄不能为空，创建失败时输出句柄会被清空。
- 返回值：成功返回 `MRT_RESULT_OK`；参数错误返回 `MRT_RESULT_INVALID_ARGUMENT`；动态分配关闭、heap 未初始化或空间不足返回 `MRT_RESULT_NO_MEMORY`。
- 调用上下文：任务上下文。
- 阻塞行为：不阻塞。
- ISR 限制：禁止在 ISR 中创建。
- 配置宏：动态分配支持和 heap。
- 调用示例：`MRT_TimerCreate("periodic", 10u, true, 0, cb, &timer);`
- 常见错误：把 `arg` 和 `callback` 参数顺序写反；未处理动态分配失败。

### MRT_TimerDelete
- 函数原型：`MRT_Result MRT_TimerDelete(MRT_TimerHandle timer);`
- 功能说明：删除动态定时器；删除前会从活动列表移除该定时器，并清除服务命令队列中尚未处理的该定时器控制命令或到期事件，避免释放后残留悬空引用。
- 参数：定时器句柄。
- 返回值：动态定时器释放成功返回 `MRT_RESULT_OK`；空句柄返回 `MRT_RESULT_INVALID_ARGUMENT`；静态定时器返回 `MRT_RESULT_OBJECT_BUSY`。
- 调用上下文：任务上下文。
- 阻塞行为：不阻塞。
- ISR 限制：禁止在 ISR 中调用。
- 配置宏：动态分配支持；`MRT_CFG_TIMER_COMMAND_QUEUE_LENGTH` 影响可清理的服务队列容量。
- 调用示例：`MRT_TimerDelete(timer);`
- 常见错误：删除静态定时器；删除动态定时器后继续保存旧句柄并再次访问。

### MRT_TimerStart
- 函数原型：`MRT_Result MRT_TimerStart(MRT_TimerHandle timer, MRT_Timeout timeout);`
- 功能说明：把启动命令投递到定时器服务命令队列；服务任务处理该命令时，才会按当前 tick 计算到期时间并加入活动列表。
- 参数：定时器句柄和等待内部控制资源的 tick 数；当前实现不阻塞等待队列空位，`timeout` 为兼容参数。
- 返回值：命令入队成功返回 `MRT_RESULT_OK`；参数错误返回 `MRT_RESULT_INVALID_ARGUMENT`；服务命令队列满返回 `MRT_RESULT_OBJECT_FULL`。
- 调用上下文：任务上下文。
- 阻塞行为：不阻塞；调用返回成功只表示命令已经入队，不表示定时器已经活动。
- ISR 限制：不建议在 ISR 中调用。
- 配置宏：tick 频率影响周期；`MRT_CFG_TIMER_COMMAND_QUEUE_LENGTH` 控制命令队列容量。
- 调用示例：`MRT_TimerStart(timer, 0u); MRT_TimerServiceRunPending();`
- 常见错误：启动后立即查询活动状态却未运行定时器服务任务。

### MRT_TimerStop
- 函数原型：`MRT_Result MRT_TimerStop(MRT_TimerHandle timer, MRT_Timeout timeout);`
- 功能说明：把停止命令投递到定时器服务命令队列；服务任务处理后，定时器才会从活动列表移除。
- 参数：定时器句柄和等待内部控制资源的 tick 数；当前实现不阻塞等待队列空位，`timeout` 为兼容参数。
- 返回值：命令入队成功返回 `MRT_RESULT_OK`；参数错误返回 `MRT_RESULT_INVALID_ARGUMENT`；服务命令队列满返回 `MRT_RESULT_OBJECT_FULL`。
- 调用上下文：任务上下文。
- 阻塞行为：不阻塞；调用返回成功只表示停止命令已经入队。
- ISR 限制：不建议在 ISR 中调用。
- 配置宏：`MRT_CFG_TIMER_COMMAND_QUEUE_LENGTH`。
- 调用示例：`MRT_TimerStop(timer, 0u); MRT_TimerServiceRunPending();`
- 常见错误：排队停止后未运行服务任务，导致定时器仍按旧状态参与 tickless deadline 计算。

### MRT_TimerReset
- 函数原型：`MRT_Result MRT_TimerReset(MRT_TimerHandle timer, MRT_Timeout timeout);`
- 功能说明：把重新装载命令投递到定时器服务命令队列；服务任务处理后，定时器按当时 tick 重新计算到期时间。
- 参数：定时器句柄和等待内部控制资源的 tick 数；当前实现不阻塞等待队列空位，`timeout` 为兼容参数。
- 返回值：命令入队成功返回 `MRT_RESULT_OK`；参数错误返回 `MRT_RESULT_INVALID_ARGUMENT`；服务命令队列满返回 `MRT_RESULT_OBJECT_FULL`。
- 调用上下文：任务上下文。
- 阻塞行为：不阻塞；调用返回成功只表示 reset 命令已经入队。
- ISR 限制：不建议在 ISR 中调用。
- 配置宏：tick 频率影响周期；`MRT_CFG_TIMER_COMMAND_QUEUE_LENGTH`。
- 调用示例：`MRT_TimerReset(timer, 0u); MRT_TimerServiceRunPending();`
- 常见错误：忘记 reset 生效点是服务任务运行时刻，而不是 API 调用时刻。

### MRT_TimerChangePeriod
- 函数原型：`MRT_Result MRT_TimerChangePeriod(MRT_TimerHandle timer, MRT_Tick new_period_ticks, MRT_Timeout timeout);`
- 功能说明：把改周期命令投递到定时器服务命令队列；服务任务处理后更新周期，若定时器已经活动，则按服务任务运行时 tick 重算到期点。
- 参数：定时器句柄、新周期和等待内部控制资源的 tick 数；新周期必须大于 0，当前实现不阻塞等待队列空位。
- 返回值：命令入队成功返回 `MRT_RESULT_OK`；空句柄或 0 周期返回 `MRT_RESULT_INVALID_ARGUMENT`；服务命令队列满返回 `MRT_RESULT_OBJECT_FULL`。
- 调用上下文：任务上下文。
- 阻塞行为：不阻塞；调用返回成功只表示改周期命令已经入队。
- ISR 限制：不建议在 ISR 中调用。
- 配置宏：tick 频率影响周期；`MRT_CFG_TIMER_COMMAND_QUEUE_LENGTH`。
- 调用示例：`MRT_TimerChangePeriod(timer, 250u, 0u); MRT_TimerServiceRunPending();`
- 常见错误：以为周期字段会在 API 返回前立即改变。

### MRT_TimerIsActive
- 函数原型：`MRT_Result MRT_TimerIsActive(MRT_TimerHandle timer, bool *out_active);`
- 功能说明：查询定时器是否处于活动列表中；结果只反映已经被服务任务处理过的控制命令。
- 参数：定时器句柄和活动状态输出指针，二者均不能为空。
- 返回值：成功返回 `MRT_RESULT_OK` 并写入 `out_active`；参数错误返回 `MRT_RESULT_INVALID_ARGUMENT`。
- 调用上下文：任务上下文或诊断代码。
- 阻塞行为：不阻塞。
- ISR 限制：ISR 中仅建议用于诊断。
- 配置宏：无特殊依赖。
- 调用示例：`bool active = false; MRT_TimerIsActive(timer, &active);`
- 常见错误：查询到非活动后忽略队列中尚未处理的启动命令。

### MRT_TimerGetName
- 函数原型：`const char *MRT_TimerGetName(MRT_TimerHandle timer);`
- 功能说明：返回创建定时器时保存的名称指针，供 trace、日志和调试界面显示。
- 参数：定时器句柄，不能为空。
- 返回值：定时器有效时返回名称指针；空句柄返回空指针；创建时名称为空则返回空指针。
- 调用上下文：任务上下文、诊断代码或调度启动前。
- 阻塞行为：不阻塞。
- ISR 限制：ISR 中仅建议用于调试读取，不应在 ISR 中执行复杂字符串处理。
- 配置宏：无特殊依赖。
- 调用示例：`const char *name = MRT_TimerGetName(timer);`
- 常见错误：把返回的名称指针当作内核复制出的字符串；MyRTOS 只保存调用方传入的指针。

### MRT_TimerPendFunctionCall
- 函数原型：`MRT_Result MRT_TimerPendFunctionCall(MRT_TimerPendingFunction function, void *arg, uint32_t value, MRT_Timeout timeout);`
- 功能说明：把轻量函数投递到定时器服务命令队列；服务任务按 FIFO 顺序在临界区外调用 `function(arg, value)`。
- 参数：函数指针、用户指针参数、用户整数参数和等待队列空位的 tick 数；当前实现不阻塞等待队列空位。
- 返回值：成功返回 `MRT_RESULT_OK`；函数为空返回 `MRT_RESULT_INVALID_ARGUMENT`；服务命令队列满返回 `MRT_RESULT_OBJECT_FULL`。
- 调用上下文：任务上下文；ISR 支持取决于端口策略。
- 阻塞行为：不阻塞。
- ISR 限制：ISR 中应避免投递重负载函数。
- 配置宏：`MRT_CFG_TIMER_COMMAND_QUEUE_LENGTH`；旧的 `MRT_CFG_TIMER_PENDING_FUNCTION_QUEUE_LENGTH` 仅作为默认容量来源。
- 调用示例：`MRT_TimerPendFunctionCall(deferred_fn, arg, value, 0u);`
- 常见错误：pending function 中执行阻塞等待；把 pending function 容量误认为独立于定时器控制命令。

### MRT_TimerServiceRunPending
- 函数原型：`void MRT_TimerServiceRunPending(void);`
- 功能说明：运行定时器服务入口，按 FIFO 顺序处理启动、停止、复位、改周期、到期回调和 pending function 命令；用户回调和 pending function 在临界区外执行。
- 参数：无。
- 返回值：无。
- 调用上下文：任务上下文；host 测试中可显式调用，真实移植中通常由定时器服务任务循环调用。
- 阻塞行为：不阻塞；持续处理直到当前服务命令队列为空。
- ISR 限制：禁止在 ISR 中执行服务入口，避免在中断中运行用户回调。
- 配置宏：`MRT_CFG_TIMER_COMMAND_QUEUE_LENGTH`。
- 调用示例：`MRT_TimerServiceRunPending();`
- 常见错误：只投递控制命令而从不运行服务入口，导致定时器状态、到期回调和 pending function 都不推进。

### MRT_StreamBufferCreateStatic
- 函数原型：`MRT_Result MRT_StreamBufferCreateStatic(size_t capacity, size_t trigger_level, void *buffer, MRT_StreamBuffer *storage, MRT_StreamBufferHandle *out_stream);`
- 功能说明：创建静态字节流缓冲。
- 参数：容量、触发水位、字节缓冲、控制块和输出句柄。
- 返回值：成功返回 `MRT_RESULT_OK`；容量或水位非法返回参数错误。
- 调用上下文：任务上下文或调度启动前。
- 阻塞行为：不阻塞。
- ISR 限制：禁止在 ISR 中创建。
- 配置宏：静态分配支持。
- 调用示例：`MRT_StreamBufferCreateStatic(sizeof(buf), 8u, buf, &storage, &stream);`
- 常见错误：触发水位大于容量。

### MRT_StreamBufferCreate
- 函数原型：`MRT_Result MRT_StreamBufferCreate(size_t capacity, size_t trigger_level, MRT_StreamBufferHandle *out_stream);`
- 功能说明：从 MyRTOS heap 动态创建字节流缓冲；内部使用一个堆块保存对齐后的控制块和字节存储区。
- 参数：容量、触发水位和输出句柄；`capacity` 必须大于 0，`trigger_level` 必须在 1 到 `capacity` 之间，创建失败时输出句柄会被清空。
- 返回值：成功返回 `MRT_RESULT_OK`；参数错误返回 `MRT_RESULT_INVALID_ARGUMENT`；动态分配关闭、heap 未初始化或空间不足返回 `MRT_RESULT_NO_MEMORY`。
- 调用上下文：任务上下文。
- 阻塞行为：不阻塞。
- ISR 限制：禁止在 ISR 中调用。
- 配置宏：动态分配支持和 heap。
- 调用示例：`MRT_StreamBufferCreate(128u, 16u, &stream);`
- 常见错误：动态创建后忘记在对象生命周期结束时调用 `MRT_StreamBufferDelete`，导致 heap 长期占用。

### MRT_StreamBufferDelete
- 函数原型：`MRT_Result MRT_StreamBufferDelete(MRT_StreamBufferHandle stream);`
- 功能说明：删除动态创建的流缓冲，并把控制块和字节存储所在堆块归还给 MyRTOS heap。
- 参数：流缓冲句柄；不能为空，且必须来自 `MRT_StreamBufferCreate`。
- 返回值：成功返回 `MRT_RESULT_OK`；空句柄返回 `MRT_RESULT_INVALID_ARGUMENT`；静态对象或仍有等待任务时返回 `MRT_RESULT_OBJECT_BUSY`。
- 调用上下文：任务上下文。
- 阻塞行为：不阻塞。
- ISR 限制：禁止在 ISR 中删除。
- 配置宏：动态分配支持和 heap。
- 调用示例：`MRT_StreamBufferDelete(stream);`
- 常见错误：对静态创建的流缓冲调用动态删除；删除前没有确保读者/写者等待链表为空。

### MRT_StreamBufferSend
- 函数原型：`MRT_Result MRT_StreamBufferSend(MRT_StreamBufferHandle stream, const void *data, size_t length, MRT_Timeout timeout, size_t *out_sent);`
- 功能说明：向流缓冲写入字节序列。
- 参数：缓冲句柄、数据地址、长度、等待 tick 和可选实际写入字节数输出。
- 返回值：成功返回 `MRT_RESULT_OK`；无空间且 `timeout` 为 0 返回 `MRT_RESULT_OBJECT_FULL`；无空间且 `timeout` 非 0 时进入写等待并在当前 host 模型中返回 `MRT_RESULT_TIMEOUT`；参数错误返回 `MRT_RESULT_INVALID_ARGUMENT`。
- 调用上下文：任务上下文。
- 阻塞行为：空间不足且 timeout 非 0 时可阻塞。
- ISR 限制：ISR 中使用 `MRT_StreamBufferSendFromISR`。
- 配置宏：tick 频率影响 timeout。
- 调用示例：`size_t sent; MRT_StreamBufferSend(stream, data, len, 10u, &sent);`
- 常见错误：期望一次写入一定完整。

### MRT_StreamBufferReceive
- 函数原型：`MRT_Result MRT_StreamBufferReceive(MRT_StreamBufferHandle stream, void *out_data, size_t length, MRT_Timeout timeout, size_t *out_received);`
- 功能说明：从流缓冲读取字节序列。
- 参数：缓冲句柄、输出地址、最大长度、等待 tick 和可选实际读取字节数输出。
- 返回值：成功返回 `MRT_RESULT_OK`；无数据返回 `MRT_RESULT_OBJECT_EMPTY`；参数错误返回 `MRT_RESULT_INVALID_ARGUMENT`。
- 调用上下文：任务上下文。
- 阻塞行为：无数据且 timeout 非 0 时可阻塞。
- ISR 限制：ISR 中使用 `MRT_StreamBufferReceiveFromISR`。
- 配置宏：tick 频率影响 timeout。
- 调用示例：`size_t received; MRT_StreamBufferReceive(stream, buf, sizeof(buf), 20u, &received);`
- 常见错误：把流缓冲当作保留消息边界的消息队列。

### MRT_StreamBufferSendFromISR
- 函数原型：`MRT_Result MRT_StreamBufferSendFromISR(MRT_StreamBufferHandle stream, const void *data, size_t length, size_t *out_sent, bool *should_yield);`
- 功能说明：在 ISR 中非阻塞写入流缓冲。
- 参数：缓冲句柄、数据、长度、可选实际写入字节数输出和可选切换输出。
- 返回值：成功返回 `MRT_RESULT_OK`；空间不足返回 `MRT_RESULT_OBJECT_FULL`；参数错误或上下文错误返回相应错误。
- 调用上下文：ISR 上下文。
- 阻塞行为：不阻塞。
- ISR 限制：这是 ISR 专用 API。
- 配置宏：抢占配置影响 `should_yield`。
- 调用示例：`size_t sent; MRT_StreamBufferSendFromISR(stream, rx, n, &sent, &yield);`
- 常见错误：忽略部分写入。

### MRT_StreamBufferReceiveFromISR
- 函数原型：`MRT_Result MRT_StreamBufferReceiveFromISR(MRT_StreamBufferHandle stream, void *out_data, size_t length, size_t *out_received);`
- 功能说明：在 ISR 中非阻塞读取流缓冲。
- 参数：缓冲句柄、输出地址、最大长度和可选实际读取字节数输出。
- 返回值：成功返回 `MRT_RESULT_OK`；无数据返回 `MRT_RESULT_OBJECT_EMPTY`；参数错误或上下文错误返回相应错误。
- 调用上下文：ISR 上下文。
- 阻塞行为：不阻塞。
- ISR 限制：这是 ISR 专用 API。
- 配置宏：无特殊依赖。
- 调用示例：`size_t received; MRT_StreamBufferReceiveFromISR(stream, buf, sizeof(buf), &received);`
- 常见错误：在 ISR 中等待数据。

### MRT_StreamBufferBytesAvailable
- 函数原型：`MRT_Result MRT_StreamBufferBytesAvailable(MRT_StreamBufferHandle stream, size_t *out_bytes);`
- 功能说明：查询流缓冲当前可读字节数。
- 参数：缓冲句柄和输出字节数。
- 返回值：成功返回 `MRT_RESULT_OK`；参数错误返回 `MRT_RESULT_INVALID_ARGUMENT`。
- 调用上下文：任务上下文或诊断代码。
- 阻塞行为：不阻塞。
- ISR 限制：ISR 中仅建议用于诊断。
- 配置宏：无特殊依赖。
- 调用示例：`MRT_StreamBufferBytesAvailable(stream, &bytes);`
- 常见错误：查询后假定后续读取数量不变。

### MRT_StreamBufferSpacesAvailable
- 函数原型：`MRT_Result MRT_StreamBufferSpacesAvailable(MRT_StreamBufferHandle stream, size_t *out_spaces);`
- 功能说明：查询流缓冲剩余可写空间。
- 参数：缓冲句柄和输出空间。
- 返回值：成功返回 `MRT_RESULT_OK`；参数错误返回 `MRT_RESULT_INVALID_ARGUMENT`。
- 调用上下文：任务上下文或诊断代码。
- 阻塞行为：不阻塞。
- ISR 限制：ISR 中仅建议用于诊断。
- 配置宏：无特殊依赖。
- 调用示例：`MRT_StreamBufferSpacesAvailable(stream, &spaces);`
- 常见错误：查询后假定后续写入一定完整。

### MRT_StreamBufferReset
- 函数原型：`MRT_Result MRT_StreamBufferReset(MRT_StreamBufferHandle stream);`
- 功能说明：清空流缓冲并复位读写索引。
- 参数：流缓冲句柄，不能为空。
- 返回值：成功返回 `MRT_RESULT_OK`；参数错误返回 `MRT_RESULT_INVALID_ARGUMENT`。
- 调用上下文：任务上下文。
- 阻塞行为：不阻塞。
- ISR 限制：不建议在 ISR 中复位共享流缓冲。
- 配置宏：无特殊依赖。
- 调用示例：`MRT_StreamBufferReset(stream);`
- 常见错误：复位仍有任务等待的流缓冲，造成应用层协议丢包。

### MRT_MessageBufferCreateStatic
- 函数原型：`MRT_Result MRT_MessageBufferCreateStatic(size_t capacity, void *buffer, MRT_MessageBuffer *storage, MRT_MessageBufferHandle *out_message_buffer);`
- 功能说明：创建静态消息缓冲，保留消息边界。
- 参数：容量、字节缓冲、控制块和输出句柄。
- 返回值：成功返回 `MRT_RESULT_OK`；容量不足以容纳长度字段时返回参数错误。
- 调用上下文：任务上下文或调度启动前。
- 阻塞行为：不阻塞。
- ISR 限制：禁止在 ISR 中创建。
- 配置宏：静态分配支持。
- 调用示例：`MRT_MessageBufferCreateStatic(sizeof(buf), buf, &storage, &mb);`
- 常见错误：容量没有预留长度字段空间。

### MRT_MessageBufferCreate
- 函数原型：`MRT_Result MRT_MessageBufferCreate(size_t capacity, MRT_MessageBufferHandle *out_message_buffer);`
- 功能说明：从 MyRTOS heap 动态创建消息缓冲；内部使用一个堆块保存对齐后的控制块和字节存储区。
- 参数：容量和输出句柄；容量必须至少能容纳长度字段和 1 字节消息，创建失败时输出句柄会被清空。
- 返回值：成功返回 `MRT_RESULT_OK`；参数错误返回 `MRT_RESULT_INVALID_ARGUMENT`；动态分配关闭、heap 未初始化或空间不足返回 `MRT_RESULT_NO_MEMORY`。
- 调用上下文：任务上下文。
- 阻塞行为：不阻塞。
- ISR 限制：禁止在 ISR 中调用。
- 配置宏：动态分配支持和 heap。
- 调用示例：`MRT_MessageBufferCreate(256u, &mb);`
- 常见错误：动态创建后忘记在对象生命周期结束时调用 `MRT_MessageBufferDelete`，导致 heap 长期占用。

### MRT_MessageBufferDelete
- 函数原型：`MRT_Result MRT_MessageBufferDelete(MRT_MessageBufferHandle message_buffer);`
- 功能说明：删除动态创建的消息缓冲，并把控制块和字节存储所在堆块归还给 MyRTOS heap。
- 参数：消息缓冲句柄；不能为空，且必须来自 `MRT_MessageBufferCreate`。
- 返回值：成功返回 `MRT_RESULT_OK`；空句柄返回 `MRT_RESULT_INVALID_ARGUMENT`；静态对象或仍有等待任务时返回 `MRT_RESULT_OBJECT_BUSY`。
- 调用上下文：任务上下文。
- 阻塞行为：不阻塞。
- ISR 限制：禁止在 ISR 中删除。
- 配置宏：动态分配支持和 heap。
- 调用示例：`MRT_MessageBufferDelete(message_buffer);`
- 常见错误：对静态创建的消息缓冲调用动态删除；删除前没有确保读者/写者等待链表为空。

### MRT_MessageBufferSend
- 函数原型：`MRT_Result MRT_MessageBufferSend(MRT_MessageBufferHandle message_buffer, const void *message, size_t length, MRT_Timeout timeout, size_t *out_sent);`
- 功能说明：写入一条完整消息。
- 参数：消息缓冲句柄、消息地址、长度、等待 tick 和可选实际写入字节数输出。
- 返回值：成功返回 `MRT_RESULT_OK`；空间不足且 `timeout` 为 0 返回 `MRT_RESULT_OBJECT_FULL`；空间不足且 `timeout` 非 0 时进入写等待并在当前 host 模型中返回 `MRT_RESULT_TIMEOUT`；参数错误返回 `MRT_RESULT_INVALID_ARGUMENT`。
- 调用上下文：任务上下文。
- 阻塞行为：空间不足且 timeout 非 0 时可阻塞。
- ISR 限制：ISR 中使用 `MRT_MessageBufferSendFromISR`。
- 配置宏：tick 频率影响 timeout。
- 调用示例：`size_t sent; MRT_MessageBufferSend(mb, msg, len, 20u, &sent);`
- 常见错误：以为消息缓冲允许半包写入。

### MRT_MessageBufferReceive
- 函数原型：`MRT_Result MRT_MessageBufferReceive(MRT_MessageBufferHandle message_buffer, void *out_message, size_t output_capacity, MRT_Timeout timeout, size_t *out_received);`
- 功能说明：读取一条完整消息。
- 参数：消息缓冲句柄、输出缓冲、输出容量、等待 tick 和可选实际读取字节数输出。
- 返回值：成功返回 `MRT_RESULT_OK`；无消息返回 `MRT_RESULT_OBJECT_EMPTY`；输出容量不足返回 `MRT_RESULT_OBJECT_FULL` 且不移除消息；参数错误返回 `MRT_RESULT_INVALID_ARGUMENT`。
- 调用上下文：任务上下文。
- 阻塞行为：无消息且 timeout 非 0 时可阻塞。
- ISR 限制：ISR 中使用 `MRT_MessageBufferReceiveFromISR`。
- 配置宏：tick 频率影响 timeout。
- 调用示例：`size_t received; MRT_MessageBufferReceive(mb, buf, sizeof(buf), 50u, &received);`
- 常见错误：输出缓冲小于下一条消息导致消息保留。

### MRT_MessageBufferSendFromISR
- 函数原型：`MRT_Result MRT_MessageBufferSendFromISR(MRT_MessageBufferHandle message_buffer, const void *message, size_t length, size_t *out_sent, bool *should_yield);`
- 功能说明：在 ISR 中非阻塞写入完整消息。
- 参数：消息缓冲、消息地址、长度、可选实际写入字节数输出和可选切换输出。
- 返回值：成功返回 `MRT_RESULT_OK`；空间不足返回 `MRT_RESULT_OBJECT_FULL`；参数错误或上下文错误返回相应错误。
- 调用上下文：ISR 上下文。
- 阻塞行为：不阻塞。
- ISR 限制：这是 ISR 专用 API。
- 配置宏：抢占配置影响 `should_yield`。
- 调用示例：`size_t sent; MRT_MessageBufferSendFromISR(mb, msg, len, &sent, &yield);`
- 常见错误：忽略 0 返回值导致消息丢失未统计。

### MRT_MessageBufferReceiveFromISR
- 函数原型：`MRT_Result MRT_MessageBufferReceiveFromISR(MRT_MessageBufferHandle message_buffer, void *out_message, size_t output_capacity, size_t *out_received);`
- 功能说明：在 ISR 中非阻塞读取完整消息。
- 参数：消息缓冲、输出缓冲、输出容量和可选实际读取字节数输出。
- 返回值：成功返回 `MRT_RESULT_OK`；无消息返回 `MRT_RESULT_OBJECT_EMPTY`；输出容量不足返回 `MRT_RESULT_OBJECT_FULL`；参数错误或上下文错误返回相应错误。
- 调用上下文：ISR 上下文。
- 阻塞行为：不阻塞。
- ISR 限制：这是 ISR 专用 API。
- 配置宏：无特殊依赖。
- 调用示例：`size_t received; MRT_MessageBufferReceiveFromISR(mb, buf, sizeof(buf), &received);`
- 常见错误：在 ISR 中等待消息。

### MRT_MessageBufferBytesAvailable
- 函数原型：`MRT_Result MRT_MessageBufferBytesAvailable(MRT_MessageBufferHandle message_buffer, size_t *out_bytes);`
- 功能说明：查询消息缓冲当前已使用字节数，包含消息长度头和消息载荷。
- 参数：消息缓冲句柄和输出字节数指针；二者都不能为空。
- 返回值：成功返回 `MRT_RESULT_OK`；参数错误返回 `MRT_RESULT_INVALID_ARGUMENT`。
- 调用上下文：任务上下文或诊断代码。
- 阻塞行为：不阻塞。
- ISR 限制：ISR 中仅建议用于诊断。
- 配置宏：无特殊依赖。
- 调用示例：`MRT_MessageBufferBytesAvailable(message_buffer, &bytes);`
- 常见错误：把已使用字节数误认为下一条消息的载荷长度。

### MRT_MessageBufferSpacesAvailable
- 函数原型：`MRT_Result MRT_MessageBufferSpacesAvailable(MRT_MessageBufferHandle message_buffer, size_t *out_spaces);`
- 功能说明：查询消息缓冲当前可写空闲字节数。
- 参数：消息缓冲句柄和输出空间指针；二者都不能为空。
- 返回值：成功返回 `MRT_RESULT_OK`；参数错误返回 `MRT_RESULT_INVALID_ARGUMENT`。
- 调用上下文：任务上下文或诊断代码。
- 阻塞行为：不阻塞。
- ISR 限制：ISR 中仅建议用于诊断。
- 配置宏：无特殊依赖。
- 调用示例：`MRT_MessageBufferSpacesAvailable(message_buffer, &spaces);`
- 常见错误：只比较载荷长度，忘记消息缓冲还要保存 4 字节长度头。

### MRT_MessageBufferReset
- 函数原型：`MRT_Result MRT_MessageBufferReset(MRT_MessageBufferHandle message_buffer);`
- 功能说明：清空消息缓冲并复位读写索引，丢弃所有未读消息。
- 参数：消息缓冲句柄，不能为空。
- 返回值：成功返回 `MRT_RESULT_OK`；参数错误返回 `MRT_RESULT_INVALID_ARGUMENT`。
- 调用上下文：任务上下文。
- 阻塞行为：不阻塞。
- ISR 限制：不建议在 ISR 中复位共享消息缓冲。
- 配置宏：无特殊依赖。
- 调用示例：`MRT_MessageBufferReset(message_buffer);`
- 常见错误：复位后还期望旧消息可以继续读取。

### MRT_HeapInitialize
- 函数原型：`MRT_Result MRT_HeapInitialize(void *memory, size_t bytes, MRT_HeapMode mode);`
- 功能说明：初始化 MyRTOS heap 区域和分配策略。
- 参数：堆内存地址、字节数和 heap 模式。
- 返回值：成功返回 `MRT_RESULT_OK`；地址、大小或模式非法返回参数错误。
- 调用上下文：调度启动前推荐。
- 阻塞行为：不阻塞。
- ISR 限制：禁止在 ISR 中初始化 heap。
- 配置宏：`MRT_CFG_HEAP_ALIGNMENT`。
- 调用示例：`MRT_HeapInitialize(heap_area, sizeof(heap_area), MRT_HEAP_MODE_COALESCING);`
- 常见错误：堆区域未按平台要求对齐。

### MRT_Malloc
- 函数原型：`void *MRT_Malloc(size_t size);`
- 功能说明：从 MyRTOS heap 分配内存。
- 参数：请求字节数。
- 返回值：成功返回对齐内存地址；失败返回空。
- 调用上下文：任务上下文。
- 阻塞行为：不阻塞，内部可进入短临界区。
- ISR 限制：禁止在 ISR 中分配。
- 配置宏：heap 模式和对齐配置。
- 调用示例：`void *ptr = MRT_Malloc(64u);`
- 常见错误：在线性 heap 模式下期望释放复用。

### MRT_Free
- 函数原型：`MRT_Result MRT_Free(void *ptr);`
- 功能说明：释放 heap 分配的内存。
- 参数：要释放的地址；空指针允许。
- 返回值：成功返回 `MRT_RESULT_OK`；非法指针或重复释放返回相应错误。
- 调用上下文：任务上下文。
- 阻塞行为：不阻塞。
- ISR 限制：禁止在 ISR 中释放。
- 配置宏：heap 模式决定是否支持释放和合并。
- 调用示例：`MRT_Free(ptr);`
- 常见错误：释放非 MyRTOS heap 指针。

### MRT_HeapGetFreeSize
- 函数原型：`MRT_Result MRT_HeapGetFreeSize(size_t *out_free_size);`
- 功能说明：查询当前 heap 剩余字节数。
- 参数：输出当前空闲字节数的指针，不能为空。
- 返回值：成功返回 `MRT_RESULT_OK`；堆未初始化返回 `MRT_RESULT_NOT_STARTED`；输出指针为空返回 `MRT_RESULT_INVALID_ARGUMENT`。
- 调用上下文：任务上下文或诊断代码。
- 阻塞行为：不阻塞。
- ISR 限制：ISR 中仅建议用于诊断。
- 配置宏：无特殊依赖。
- 调用示例：`size_t free_bytes = 0u; MRT_HeapGetFreeSize(&free_bytes);`
- 常见错误：把剩余总量当作最大连续块大小。

### MRT_HeapGetMinimumEverFreeSize
- 函数原型：`MRT_Result MRT_HeapGetMinimumEverFreeSize(size_t *out_minimum_free_size);`
- 功能说明：查询 heap 历史最低剩余字节数。
- 参数：输出历史最低剩余字节数的指针，不能为空。
- 返回值：成功返回 `MRT_RESULT_OK`；堆未初始化返回 `MRT_RESULT_NOT_STARTED`；输出指针为空返回 `MRT_RESULT_INVALID_ARGUMENT`。
- 调用上下文：任务上下文或诊断代码。
- 阻塞行为：不阻塞。
- ISR 限制：ISR 中仅建议用于诊断。
- 配置宏：无特殊依赖。
- 调用示例：`size_t low = 0u; MRT_HeapGetMinimumEverFreeSize(&low);`
- 常见错误：重新初始化 heap 后仍使用旧水位数据。

### MRT_MemoryPoolCreateStatic
- 函数原型：`MRT_Result MRT_MemoryPoolCreateStatic(MRT_MemoryPool *storage, void *memory, size_t memory_bytes, size_t block_size, MRT_MemoryPoolHandle *out_pool);`
- 功能说明：创建固定块内存池。
- 参数：控制块、池内存、总字节数、块大小和输出句柄。
- 返回值：成功返回 `MRT_RESULT_OK`；参数非法返回 `MRT_RESULT_INVALID_ARGUMENT`。
- 调用上下文：任务上下文或调度启动前。
- 阻塞行为：不阻塞。
- ISR 限制：禁止在 ISR 中创建。
- 配置宏：块对齐遵循指针和 heap 对齐要求。
- 调用示例：`MRT_MemoryPoolCreateStatic(&pool_storage, mem, sizeof(mem), 32u, &pool);`
- 常见错误：块大小小于内部 free-list 指针大小。

### MRT_MemoryPoolAlloc
- 函数原型：`MRT_Result MRT_MemoryPoolAlloc(MRT_MemoryPoolHandle pool, void **out_block);`
- 功能说明：从固定块池取出一个空闲块。
- 参数：内存池句柄和输出块指针。
- 返回值：成功返回 `MRT_RESULT_OK`；池空返回 `MRT_RESULT_OBJECT_EMPTY`。
- 调用上下文：任务上下文。
- 阻塞行为：不阻塞。
- ISR 限制：ISR 中使用前需确认端口临界区策略。
- 配置宏：无特殊依赖。
- 调用示例：`MRT_MemoryPoolAlloc(pool, &block);`
- 常见错误：忘记释放导致池耗尽。

### MRT_MemoryPoolFree
- 函数原型：`MRT_Result MRT_MemoryPoolFree(MRT_MemoryPoolHandle pool, void *block);`
- 功能说明：把固定块归还给内存池。
- 参数：内存池句柄和块地址。
- 返回值：成功返回 `MRT_RESULT_OK`；非法指针或重复释放返回错误。
- 调用上下文：任务上下文。
- 阻塞行为：不阻塞。
- ISR 限制：ISR 中使用前需确认端口临界区策略。
- 配置宏：无特殊依赖。
- 调用示例：`MRT_MemoryPoolFree(pool, block);`
- 常见错误：归还不属于该池的指针。

### MRT_MemoryPoolGetFreeCount
- 函数原型：`MRT_Result MRT_MemoryPoolGetFreeCount(MRT_MemoryPoolHandle pool, size_t *out_free_count);`
- 功能说明：查询固定块内存池当前仍可分配的空闲块数量。
- 参数：内存池句柄和输出空闲块数量指针，二者均不能为空。
- 返回值：成功返回 `MRT_RESULT_OK` 并写入 `out_free_count`；参数错误返回 `MRT_RESULT_INVALID_ARGUMENT`。
- 调用上下文：任务上下文、初始化诊断或调试统计路径。
- 阻塞行为：不阻塞。
- ISR 限制：ISR 中仅建议用于诊断读取；若与任务同时访问同一池，应由端口临界区或上层策略保护。
- 配置宏：无特殊依赖。
- 调用示例：`size_t free_count = 0u; MRT_MemoryPoolGetFreeCount(pool, &free_count);`
- 常见错误：把空闲块数量当作剩余字节数；剩余字节数等于 `free_count * block_size`。

### MRT_TicklessGetExpectedIdleTicks
- 函数原型：`MRT_Result MRT_TicklessGetExpectedIdleTicks(MRT_Tick *out_ticks);`
- 功能说明：计算当前最近任务/定时器 deadline 前可睡眠 tick 数。
- 参数：输出 tick 指针。
- 返回值：成功返回 `MRT_RESULT_OK`；没有明确 deadline 时输出 0。
- 调用上下文：空闲任务或低功耗管理任务。
- 阻塞行为：不阻塞。
- ISR 限制：禁止在 ISR 中调用。
- 配置宏：`MRT_CFG_USE_TICKLESS_IDLE`。
- 调用示例：`MRT_Tick idle; MRT_TicklessGetExpectedIdleTicks(&idle);`
- 常见错误：无 deadline 时进入无界睡眠。

### MRT_TicklessEnterIdle
- 函数原型：`MRT_Result MRT_TicklessEnterIdle(MRT_Tick max_sleep_ticks, MRT_Tick *out_slept_ticks);`
- 功能说明：请求端口层抑制 tick 并睡眠，再按实际睡眠 tick 补偿内核时间。
- 参数：最大允许睡眠 tick 和实际睡眠输出。
- 返回值：成功返回 `MRT_RESULT_OK`；端口失败时透传端口结果。
- 调用上下文：空闲任务或低功耗管理任务。
- 阻塞行为：会进入端口低功耗等待。
- ISR 限制：禁止在 ISR 中调用。
- 配置宏：`MRT_CFG_USE_TICKLESS_IDLE`。
- 调用示例：`MRT_TicklessEnterIdle(100u, &slept);`
- 常见错误：端口回报睡眠 tick 超过请求上限。

### MRT_TraceSetSink
- 函数原型：`MRT_Result MRT_TraceSetSink(MRT_TraceSink sink, void *user);`
- 功能说明：设置 trace 事件接收函数。
- 参数：sink 回调和用户指针。
- 返回值：成功返回 `MRT_RESULT_OK`。
- 调用上下文：初始化阶段或任务上下文。
- 阻塞行为：不阻塞。
- ISR 限制：sink 若在 ISR 路径被调用必须足够短。
- 配置宏：`MRT_CFG_USE_TRACE`。
- 调用示例：`MRT_TraceSetSink(trace_sink, 0);`
- 常见错误：trace sink 中调用阻塞 API。

### MRT_TraceEmit
- 函数原型：`void MRT_TraceEmit(const MRT_TraceEvent *event);`
- 功能说明：发布一条 trace 事件到当前 sink。
- 参数：事件指针。
- 返回值：无；未设置 sink 时丢弃事件。
- 调用上下文：内核内部、任务上下文或 ISR 路径。
- 阻塞行为：不阻塞。
- ISR 限制：ISR 事件 sink 必须可重入或受保护。
- 配置宏：`MRT_CFG_USE_TRACE`。
- 调用示例：`MRT_TraceEmit(&event);`
- 常见错误：在关闭 trace 时仍假定事件被保存。

### MRT_StatsGetTaskRuntime
- 函数原型：`MRT_Result MRT_StatsGetTaskRuntime(MRT_TaskHandle task, uint64_t *out_runtime);`
- 功能说明：读取任务累计运行时间统计；当前 portable preview 以 kernel tick 为单位，在每次 `MRT_KernelTick()` 到来时把刚结束的一个 tick 归属到当前运行任务。
- 参数：任务句柄和输出计数；任务不能为空，且不能处于 deleted 状态，输出指针不能为空。
- 返回值：成功返回 `MRT_RESULT_OK`；参数错误或任务已删除返回 `MRT_RESULT_INVALID_ARGUMENT`。
- 调用上下文：诊断任务。
- 阻塞行为：不阻塞。
- ISR 限制：不建议在 ISR 中调用。
- 配置宏：当前实现不需要额外宏；tick 单位受 `MRT_CFG_TICK_RATE_HZ` 影响。
- 调用示例：`MRT_StatsGetTaskRuntime(task, &ticks);`
- 常见错误：把当前 tick 级统计当作 CPU cycle 精度；STM32/DSP 高分辨率计数源属于后续端口增强。

### MRT_AssertSetHook
- 函数原型：`MRT_Result MRT_AssertSetHook(MRT_AssertHook hook, void *user);`
- 功能说明：设置断言失败回调。
- 参数：hook 回调和用户指针。
- 返回值：成功返回 `MRT_RESULT_OK`。
- 调用上下文：初始化阶段或任务上下文。
- 阻塞行为：不阻塞。
- ISR 限制：hook 可能在 ISR 错误路径执行，必须简短。
- 配置宏：断言配置。
- 调用示例：`MRT_AssertSetHook(assert_sink, 0);`
- 常见错误：hook 中执行复杂日志或动态分配。

### MRT_AssertFailed
- 函数原型：`void MRT_AssertFailed(const char *expr, const char *file, uint32_t line);`
- 功能说明：分发断言失败信息到 hook。
- 参数：表达式、文件名和行号。
- 返回值：无。
- 调用上下文：任意上下文的错误路径。
- 阻塞行为：默认不阻塞。
- ISR 限制：ISR 中应只记录最小信息。
- 配置宏：断言配置。
- 调用示例：`MRT_AssertFailed("ptr != 0", __FILE__, __LINE__);`
- 常见错误：默认 host 行为不会停机，真实端口需要在 hook 中停机或复位。

### MRT_ASSERT
- 函数原型：`MRT_ASSERT(expr);`
- 功能说明：断言宏，表达式为 false 时记录表达式、文件和行号。
- 参数：`expr` 为待检查表达式。
- 返回值：宏无返回值。
- 调用上下文：任务、ISR 或初始化错误检查路径。
- 阻塞行为：取决于 hook。
- ISR 限制：ISR 中的 hook 必须短小。
- 配置宏：断言配置。
- 调用示例：`MRT_ASSERT(queue != 0);`
- 常见错误：把断言当作运行时参数校验唯一手段。

### MRT_PortInitialize
- 函数原型：`void MRT_PortInitialize(void);`
- 功能说明：初始化公共端口层状态。
- 参数：无。
- 返回值：无。
- 调用上下文：内核初始化阶段。
- 阻塞行为：不阻塞。
- ISR 限制：禁止在 ISR 中初始化端口。
- 配置宏：端口相关配置。
- 调用示例：`MRT_PortInitialize();`
- 常见错误：用户应用重复直接调用破坏端口状态。

### MRT_PortStartFirstTask
- 函数原型：`void MRT_PortStartFirstTask(void);`
- 功能说明：由端口层启动第一个任务上下文。
- 参数：无。
- 返回值：真实端口通常不返回。
- 调用上下文：调度器启动路径。
- 阻塞行为：进入任务运行。
- ISR 限制：禁止在 ISR 中调用。
- 配置宏：端口实现决定。
- 调用示例：`MRT_PortStartFirstTask();`
- 常见错误：在应用层直接调用。

### MRT_PortYield
- 函数原型：`void MRT_PortYield(void);`
- 功能说明：请求任务上下文切换。
- 参数：无。
- 返回值：无。
- 调用上下文：任务上下文。
- 阻塞行为：不阻塞，但触发调度请求。
- ISR 限制：ISR 中使用 `MRT_PortYieldFromISR`。
- 配置宏：端口实现决定。
- 调用示例：`MRT_PortYield();`
- 常见错误：在临界区内反复触发切换。

### MRT_PortYieldFromISR
- 函数原型：`void MRT_PortYieldFromISR(bool should_yield);`
- 功能说明：在 ISR 末尾按需请求延迟上下文切换。
- 参数：`should_yield` 为 FromISR API 输出的切换建议。
- 返回值：无。
- 调用上下文：ISR 上下文。
- 阻塞行为：不阻塞。
- ISR 限制：这是 ISR 末尾专用路径。
- 配置宏：端口实现决定。
- 调用示例：`MRT_PortYieldFromISR(yield);`
- 常见错误：不检查 `should_yield` 就无条件切换。

### MRT_PortEnterCritical
- 函数原型：`MRT_IntState MRT_PortEnterCritical(void);`
- 功能说明：进入临界区并保存旧中断状态。
- 参数：无。
- 返回值：返回可传给退出函数的旧状态。
- 调用上下文：任务上下文；部分端口允许 ISR 内短临界区。
- 阻塞行为：不阻塞。
- ISR 限制：ISR 内使用需遵循端口规则。
- 配置宏：STM32 可能使用 BASEPRI 或 PRIMASK。
- 调用示例：`MRT_IntState s = MRT_PortEnterCritical();`
- 常见错误：未保存返回值导致退出时恢复错误状态。

### MRT_PortExitCritical
- 函数原型：`void MRT_PortExitCritical(MRT_IntState state);`
- 功能说明：退出临界区并恢复进入前中断状态。
- 参数：`state` 为进入函数返回值。
- 返回值：无。
- 调用上下文：与进入临界区配对。
- 阻塞行为：不阻塞。
- ISR 限制：必须与同一上下文的进入配对。
- 配置宏：端口实现决定。
- 调用示例：`MRT_PortExitCritical(s);`
- 常见错误：嵌套临界区退出顺序错误。

### MRT_PortIsInsideISR
- 函数原型：`bool MRT_PortIsInsideISR(void);`
- 功能说明：查询当前是否处于 ISR 上下文。
- 参数：无。
- 返回值：ISR 中返回 `true`，任务上下文返回 `false`。
- 调用上下文：任意上下文。
- 阻塞行为：不阻塞。
- ISR 限制：无。
- 配置宏：端口实现决定。
- 调用示例：`if (MRT_PortIsInsideISR()) { ... }`
- 常见错误：host mock 状态未设置导致测试误判。

### MRT_PortSuppressTicksAndSleep
- 函数原型：`MRT_Result MRT_PortSuppressTicksAndSleep(MRT_Tick expected_idle_ticks, MRT_Tick *out_slept_ticks);`
- 功能说明：端口层抑制周期 tick 并进入低功耗睡眠。
- 参数：预计可睡眠 tick 和实际睡眠输出。
- 返回值：成功返回 `MRT_RESULT_OK`；参数错误返回 `MRT_RESULT_INVALID_ARGUMENT`。
- 调用上下文：tickless idle 路径。
- 阻塞行为：会进入低功耗等待。
- ISR 限制：禁止在 ISR 中调用。
- 配置宏：`MRT_CFG_USE_TICKLESS_IDLE`。
- 调用示例：`MRT_PortSuppressTicksAndSleep(10u, &slept);`
- 常见错误：端口没有处理提前唤醒。

### MRT_PortStm32CmInitializeStack
- 函数原型：`MRT_Result MRT_PortStm32CmInitializeStack(MRT_StackType *stack_memory, size_t stack_words, void (*entry)(void *), void *argument, void (*task_exit)(void), MRT_StackType **out_stack_top);`
- 功能说明：构造 STM32 Cortex-M 初始任务栈帧。
- 参数：栈内存、栈长度、入口函数、入口参数、退出处理函数和输出栈顶。
- 返回值：成功返回 `MRT_RESULT_OK`；参数或栈空间非法返回 `MRT_RESULT_INVALID_ARGUMENT`。
- 调用上下文：任务创建路径。
- 阻塞行为：不阻塞。
- ISR 限制：禁止在 ISR 中调用。
- 配置宏：`MRT_CFG_MINIMAL_STACK_WORDS` 影响推荐栈深。
- 调用示例：`MRT_PortStm32CmInitializeStack(stack, 128u, task, arg, task_exit, &top);`
- 常见错误：任务入口不是 Thumb 可执行地址或栈不足。

### MRT_PortStm32CmCalculateSysTickReload
- 函数原型：`MRT_Result MRT_PortStm32CmCalculateSysTickReload(uint32_t cpu_clock_hz, uint32_t tick_rate_hz, uint32_t *out_reload);`
- 功能说明：计算 STM32 SysTick LOAD 寄存器装载值。
- 参数：SysTick 输入时钟、目标 tick 频率和输出 reload。
- 返回值：成功返回 `MRT_RESULT_OK`；频率非法或超过 24 位范围返回 `MRT_RESULT_INVALID_ARGUMENT`。
- 调用上下文：板级端口初始化。
- 阻塞行为：不阻塞。
- ISR 限制：禁止在 ISR 中配置。
- 配置宏：`MRT_CFG_TICK_RATE_HZ`。
- 调用示例：`MRT_PortStm32CmCalculateSysTickReload(SystemCoreClock, 1000u, &reload);`
- 常见错误：低 tick 频率配高时钟导致 reload 超过 24 位。

### MRT_PortStm32CmEncodeBasepri
- 函数原型：`MRT_Result MRT_PortStm32CmEncodeBasepri(uint32_t nvic_priority_bits, uint32_t logical_priority, uint32_t *out_encoded_priority);`
- 功能说明：把逻辑中断优先级左对齐编码成 BASEPRI 值。
- 参数：NVIC 实现优先级 bit 数、逻辑优先级和输出编码。
- 返回值：成功返回 `MRT_RESULT_OK`；位宽或优先级非法返回 `MRT_RESULT_INVALID_ARGUMENT`。
- 调用上下文：板级端口初始化。
- 阻塞行为：不阻塞。
- ISR 限制：不在 ISR 中计算配置。
- 配置宏：端口中断优先级策略。
- 调用示例：`MRT_PortStm32CmEncodeBasepri(4u, 5u, &basepri);`
- 常见错误：把逻辑优先级 0 用作可屏蔽内核临界区阈值。

### MRT_PortStm32CmNormalizeMpuRegion
- 函数原型：`MRT_Result MRT_PortStm32CmNormalizeMpuRegion(uintptr_t base_address, size_t size_bytes, uintptr_t *out_region_base, size_t *out_region_size, uint32_t *out_region_shift);`
- 功能说明：把任意 STM32 Cortex-M 内存范围规整为 MPU 可表达的 power-of-two 区域，输出向下对齐后的区域基址、规整后区域大小和 `log2(size)`。
- 参数：原始内存起始地址、原始字节数、输出区域基址、输出区域大小和输出区域 shift。
- 返回值：成功返回 `MRT_RESULT_OK`；原始大小为 0、输出指针为空、地址范围溢出或无法规整时返回 `MRT_RESULT_INVALID_ARGUMENT`。
- 调用上下文：板级 MPU 初始化、任务栈保护区计算、DMA/外设缓冲区属性规划路径。
- 阻塞行为：不阻塞。
- ISR 限制：禁止在 ISR 中重新计算并改写 MPU；ISR 只能使用已经配置好的内存属性。
- 配置宏：`MRT_PORT_STM32_CM_MPU_MIN_REGION_BYTES` 定义最小区域 32 字节。
- 调用示例：`MRT_PortStm32CmNormalizeMpuRegion(0x20001234u, 6000u, &base, &size, &shift);`
- 常见错误：把规整后的区域误认为等于原始范围；忽略起始偏移会导致区域从 8 KiB 继续扩大到 16 KiB；真实硬件仍需要把 `shift` 转换成目标 MPU 寄存器的 size 编码。

### MRT_PortDspC28xInitializeStack
- 函数原型：`MRT_Result MRT_PortDspC28xInitializeStack(MRT_StackType *stack_memory, size_t stack_words, void (*entry)(void *), void *argument, void (*task_exit)(void), MRT_StackType **out_stack_top);`
- 功能说明：构造 DSP C28x 风格初始任务栈帧。
- 参数：栈内存、栈长度、入口、参数、退出处理函数和输出栈顶。
- 返回值：成功返回 `MRT_RESULT_OK`；参数或空间非法返回 `MRT_RESULT_INVALID_ARGUMENT`。
- 调用上下文：任务创建路径或 DSP 端口测试。
- 阻塞行为：不阻塞。
- ISR 限制：禁止在 ISR 中调用。
- 配置宏：栈对齐由端口 ABI 决定。
- 调用示例：`MRT_PortDspC28xInitializeStack(stack, 128u, task, arg, task_exit, &top);`
- 常见错误：具体 DSP ABI 与本契约槽位未同步。

### MRT_PortDspC28xContextModelReset
- 函数原型：`void MRT_PortDspC28xContextModelReset(void);`
- 功能说明：复位 DSP 软件中断上下文切换模型。
- 参数：无。
- 返回值：无。
- 调用上下文：测试初始化或端口初始化。
- 阻塞行为：不阻塞。
- ISR 限制：不在真实 ISR 中调用。
- 配置宏：无特殊依赖。
- 调用示例：`MRT_PortDspC28xContextModelReset();`
- 常见错误：测试之间不复位导致状态串扰。

### MRT_PortDspC28xRequestContextSwitch
- 函数原型：`MRT_Result MRT_PortDspC28xRequestContextSwitch(void);`
- 功能说明：请求一次 DSP 软件中断式上下文切换。
- 参数：无。
- 返回值：成功返回 `MRT_RESULT_OK`；计数溢出返回内部错误。
- 调用上下文：任务上下文或 ISR 尾部模型。
- 阻塞行为：不阻塞。
- ISR 限制：ISR 中只置位请求，不直接切换。
- 配置宏：DSP 端口软件中断策略。
- 调用示例：`MRT_PortDspC28xRequestContextSwitch();`
- 常见错误：嵌套 ISR 中立即切换。

### MRT_PortDspC28xAcknowledgeContextSwitch
- 函数原型：`MRT_Result MRT_PortDspC28xAcknowledgeContextSwitch(void);`
- 功能说明：确认挂起的 DSP 切换请求已经被服务。
- 参数：无。
- 返回值：返回 `MRT_RESULT_OK`。
- 调用上下文：软件中断服务路径。
- 阻塞行为：不阻塞。
- ISR 限制：应在真实切换完成后调用。
- 配置宏：DSP 端口软件中断策略。
- 调用示例：`MRT_PortDspC28xAcknowledgeContextSwitch();`
- 常见错误：确认过早导致丢失切换请求。

### MRT_PortDspC28xEnterInterrupt
- 函数原型：`MRT_Result MRT_PortDspC28xEnterInterrupt(void);`
- 功能说明：记录进入一层 DSP ISR 嵌套。
- 参数：无。
- 返回值：成功返回 `MRT_RESULT_OK`；计数溢出返回内部错误。
- 调用上下文：DSP ISR 入口。
- 阻塞行为：不阻塞。
- ISR 限制：这是 ISR 入口模型函数。
- 配置宏：DSP 嵌套中断策略。
- 调用示例：`MRT_PortDspC28xEnterInterrupt();`
- 常见错误：进入/退出不配对。

### MRT_PortDspC28xExitInterrupt
- 函数原型：`MRT_Result MRT_PortDspC28xExitInterrupt(bool *out_should_switch);`
- 功能说明：记录退出一层 DSP ISR，并在最外层退出时报告是否应切换。
- 参数：输出是否应切换。
- 返回值：成功返回 `MRT_RESULT_OK`；下溢返回 `MRT_RESULT_INVALID_CONTEXT`。
- 调用上下文：DSP ISR 退出。
- 阻塞行为：不阻塞。
- ISR 限制：这是 ISR 退出模型函数。
- 配置宏：DSP 嵌套中断策略。
- 调用示例：`MRT_PortDspC28xExitInterrupt(&should_switch);`
- 常见错误：内层 ISR 退出时执行上下文切换。

### MRT_PortDspC28xIsContextSwitchPending
- 函数原型：`bool MRT_PortDspC28xIsContextSwitchPending(void);`
- 功能说明：查询 DSP 切换请求是否挂起。
- 参数：无。
- 返回值：挂起返回 `true`。
- 调用上下文：测试、诊断或软件中断路径。
- 阻塞行为：不阻塞。
- ISR 限制：ISR 中只读安全。
- 配置宏：无特殊依赖。
- 调用示例：`bool pending = MRT_PortDspC28xIsContextSwitchPending();`
- 常见错误：查询后不确认切换请求。

### MRT_PortDspC28xGetContextSwitchRequestCount
- 函数原型：`uint32_t MRT_PortDspC28xGetContextSwitchRequestCount(void);`
- 功能说明：读取 DSP 切换请求累计次数。
- 参数：无。
- 返回值：累计请求次数。
- 调用上下文：测试或诊断代码。
- 阻塞行为：不阻塞。
- ISR 限制：ISR 中只读安全。
- 配置宏：无特殊依赖。
- 调用示例：`uint32_t count = MRT_PortDspC28xGetContextSwitchRequestCount();`
- 常见错误：把累计次数当作当前 pending 数量。

### MRT_PortDspC28xGetInterruptNesting
- 函数原型：`uint32_t MRT_PortDspC28xGetInterruptNesting(void);`
- 功能说明：读取 DSP ISR 嵌套深度。
- 参数：无。
- 返回值：当前嵌套深度，0 表示任务上下文。
- 调用上下文：测试、诊断或端口路径。
- 阻塞行为：不阻塞。
- ISR 限制：ISR 中只读安全。
- 配置宏：DSP 嵌套中断策略。
- 调用示例：`uint32_t depth = MRT_PortDspC28xGetInterruptNesting();`
- 常见错误：退出 ISR 后未恢复到 0。

## 5. STM32 Cortex-M 移植步骤

### STM32 Cortex-M 移植步骤

本节面向 Cortex-M4/M7，M3 也可按同样流程处理。M0/M0+ 没有 BASEPRI 或能力不同，需要把临界区策略改为 PRIMASK 并重新验证。

### 5.1 移植前准备

- 确认目标芯片的 Cortex-M 版本、是否带 FPU、NVIC 优先级位宽、SysTick 时钟源、FLASH/RAM 容量，以及是否启用 MPU。
- 准备 ARM GNU Toolchain、烧录器、仿真器和串口日志工具，并确认 `arm-none-eabi-gcc`、`arm-none-eabi-objcopy`、`arm-none-eabi-size` 可用。
- 先决定工程是基于 HAL 还是 CMSIS 直写寄存器；MyRTOS 只要求启动、tick、临界区、上下文切换和低功耗语义一致。
- 先保留一份可工作的厂商工程备份，避免第一次接入时把向量表和链接脚本改乱。
- 先把 `examples/stm32/` 当作参考模板，而不是从零拼接端口。

### 5.2 工程分层

- `main.c` 只放应用对象创建、外设初始化和 `MRT_KernelStart()`。
- `startup_stm32cm.c` 只负责向量表、Reset_Handler 和 `.data`/`.bss` 启动流程。
- `linker.ld` 只负责 FLASH/RAM 布局、主栈、任务栈、heap 和 DMA/MPU 保留区。
- `mrt_port_stm32_smoke.c` 只负责公共端口语义：临界区、PendSV、SVC、tickless sleep 和必要的板级 glue。
- `src/portable/stm32_cm/mrt_port_stm32_cm_context.S` 提供可交叉编译的 `SVC_Handler`、`PendSV_Handler` 和 `MRT_PortStm32CmStartFirstTaskAsm` 骨架；当前 smoke 端的 `MRT_PortStm32CmPendSvHook()` 已通过内部 TCB 栈顶契约保存旧 PSP 并返回当前任务 PSP，但仍需要真实板级运行日志证明 PendSV/SVC 时序、异常返回和任务切换行为。
- `runtime_stubs.c` 或厂商库文件只补齐启动和调试所需的弱符号，不要把调度逻辑塞回启动文件。

### 5.3 关键接入顺序

1. 工具链：安装 ARM GNU Toolchain，确认 `arm-none-eabi-gcc`、`arm-none-eabi-objcopy`、`arm-none-eabi-size` 可用。CMake 工程中把 MyRTOS `include/` 加入 include path，把 `src/kernel/*.c`、目标端口 C 文件和 PendSV/SVC 汇编文件加入目标。
2. 启动文件：在 STM32 startup 文件中保留厂商默认 Reset_Handler、栈顶符号和 `.data`/`.bss` 初始化流程。不要在 Reset_Handler 中启动调度器，应用应先初始化时钟、GPIO、外设、heap 和任务，再调用 `MRT_KernelStart()`。
3. 向量表：把 `SysTick_Handler` 指向 MyRTOS tick 入口，把 `PendSV_Handler` 指向上下文切换入口，把 `SVC_Handler` 指向首任务启动入口。若项目已有同名 handler，必须合并逻辑，不能重复定义。
4. tick 配置：读取 `SystemCoreClock`，调用 `MRT_PortStm32CmCalculateSysTickReload(SystemCoreClock, MRT_CFG_TICK_RATE_HZ, &reload)`，将 reload 写入 SysTick LOAD，清零 VAL，开启 CLKSOURCE、TICKINT、ENABLE。若 reload 超出 24 位，降低 SysTick 输入时钟、降低 tick 频率或改用通用定时器。
5. 上下文切换：PendSV 设置为最低抢占优先级。任务上下文 `MRT_PortYield()` 只置 PendSV pending，不直接保存/恢复寄存器。ISR 路径根据 `should_yield` 在退出前置 PendSV pending。仓库中的 `mrt_port_stm32_cm_context.S` 已交叉编译验证 SVC/PendSV 入口、PSP 读取、R4-R11 保存/恢复和 C 钩子转交；`tests/sim/test_task_stack_top.c` 验证任务 TCB 的 `stack_top` 初始化、保存和切换返回契约；`examples/stm32/mrt_port_stm32_smoke.c` 的 PendSV C 钩子已调用 `MRT_TaskKernelSwitchStackTop()`；`examples/stm32/main.c` 已在任务创建后把端口初始化出的初始 PSP 写回 TCB。真实板级端口仍必须通过 UART/trace 证明异常返回后确实恢复到目标任务。
6. 栈布局：任务创建后调用 `MRT_PortStm32CmInitializeStack()`，再调用 `MRT_TaskKernelSetStackTop()` 把返回的初始 PSP 写入任务 TCB；初始栈帧包含 R4-R11、R0 参数、LR 退出兜底函数、PC 入口函数和 xPSR Thumb bit。栈顶必须 8 字节对齐。带 FPU 的 M4F/M7F 需要明确是否启用 lazy stacking，并按 FPU ABI 增加浮点寄存器保存策略。
7. 临界区：M3/M4/M7 推荐使用 BASEPRI 屏蔽不高于内核阈值的中断。使用 `MRT_PortStm32CmEncodeBasepri(nvic_bits, logical_priority, &basepri)` 生成左对齐值。优先级 0 不得被 MyRTOS 屏蔽，应留给最高紧急中断。M0/M0+ 使用 PRIMASK 时会屏蔽全部可屏蔽中断，需评估实时性。
8. 中断优先级：所有调用 MyRTOS FromISR API 的外设中断，其抢占优先级必须低于或等于内核可屏蔽阈值。高于阈值的中断不得调用任何 MyRTOS API，只能写硬件寄存器或置原子标志。
9. 低功耗：tickless idle 中由 `MRT_TicklessEnterIdle()` 调用 `MRT_PortSuppressTicksAndSleep()`。STM32 端口应先关闭 SysTick 或改写下一次唤醒比较值，执行 `WFI`，唤醒后读取实际睡眠 tick 并回报。进入 STOP/STANDBY 前必须确认时钟恢复后 SysTick 时钟源仍正确。
10. MPU 布局：若目标 MCU 启用 MPU，用 `MRT_PortStm32CmNormalizeMpuRegion()` 先把任务栈、heap、外设寄存器窗口、DMA 缓冲和只读表规整为 power-of-two 区域，再按芯片 MPU 寄存器格式写入属性。规整区域可能大于原始范围，必须检查额外覆盖的 RAM 是否允许同样权限。
11. 链接脚本：为 `.bss`、`.data`、heap、主栈和任务栈保留足够 RAM。静态任务栈可以放在普通 `.bss`；DMA 相关栈或缓冲需要按 MCU cache/MPU 规则放入非缓存区或执行 cache clean/invalidate。
12. 示例 smoke test：创建两个任务，一个 500 ms 翻转 LED，一个通过队列接收 UART RX ISR 发来的字节；创建一个周期软件定时器翻转第二个 GPIO；在 SysTick 中调用 `MRT_KernelTick()`；在 UART ISR 中调用 `MRT_QueueSendFromISR()` 并把 `should_yield` 传给 `MRT_PortYieldFromISR()`。
13. 排错：若首任务不运行，检查 SVC/PendSV 向量和 PSP 初始化；若 HardFault，检查 PC 是否为 Thumb 地址、栈 8 字节对齐、任务栈是否溢出；若 ISR API 无效，检查 `MRT_PortIsInsideISR()` 和 NVIC 优先级；若 tick 不准，检查 reload、时钟源和 `SystemCoreClock` 更新；若启用 MPU 后异常，检查规整区域是否覆盖了不该共享权限的相邻对象。

### 5.3.1 从厂商裸机工程迁入 MyRTOS 的实际顺序

1. 先保留一份能独立烧录的厂商裸机工程，不在原工程上直接改内核。先记录芯片型号、封装、晶振、FLASH/RAM、FPU、MPU 和 NVIC 位宽。
2. 先只接入 MyRTOS 头文件、`src/kernel/*.c` 和目标端口 C 文件，不马上改应用逻辑。第一轮目标只要通过编译，并能在 `main()` 中调用 `MRT_KernelInitialize()`、`MRT_HeapInitialize()`、任务创建和 `MRT_KernelStart()`。
3. 先把启动文件改到“只保留一个向量表”的状态。厂商 `startup` 里若已定义 `SysTick_Handler`、`PendSV_Handler`、`SVC_Handler`，必须合并到同一套入口，并确认 Reset 后先完成 `.data`/`.bss`，再进入 `main()`。
4. 先验证最小运行链路：`Reset_Handler -> main -> 首任务 -> 任务延时 -> SysTick -> 回到调度`。这一阶段先不要启用 DMA、串口、USB、复杂中断和 MPU。
5. 再接入 `SysTick_Handler`。先只做 `MRT_KernelTick()`，确认节拍递增正确，再接任务唤醒和 `should_yield` 路径。若 `MRT_PortStm32CmCalculateSysTickReload()` 返回失败，先修时钟树和 tick 频率，不要先改内核代码。
6. 再接入 `PendSV_Handler` 和 `SVC_Handler`。`PendSV` 只做延迟切换，`SVC` 只做首任务启动；不要把上下文保存恢复、任务创建和启动入口混在一个文件里。
7. 再接入第一个外设 ISR，例如 UART RX。先用队列或任务通知完成“ISR -> 任务”的闭环，再逐步扩展到 ADC/DMA、软件定时器和事件组。每加一个 ISR，都要单独确认 `MRT_PortYieldFromISR()` 只在 `should_yield=true` 时触发。
8. 再启用低功耗、tickless 和 MPU。顺序必须是“先 tick 正常 -> 再 ISR 唤醒正常 -> 再 tickless -> 最后 MPU”，不要反过来。
9. 最后才收口成板级证据：编译命令、ELF/map 路径、首任务启动、tick 计数、ISR 唤醒、临界区嵌套、低功耗恢复和 30 分钟 smoke 结果。

### 5.4 首次联调

1. 先只验证编译和链接，确认 `include/`、`src/kernel/`、端口源码和启动文件都进入最终镜像。
2. 再确认 `Reset_Handler -> main()` 路径可达，且 `MRT_KernelInitialize()`、`MRT_HeapInitialize()`、任务创建、队列创建和定时器启动命令都能成功。
3. 再验证 `SysTick_Handler` 只负责推进 `MRT_KernelTick()`，`PendSV_Handler` 只负责延迟切换，`SVC_Handler` 只负责首任务启动。
4. 再验证 UART、DMA 或 GPIO 中断中的 FromISR API 只在 `should_yield=true` 时请求延迟切换，而不是在 ISR 内直接切任务。
5. 若启用 MPU，先只保护任务栈和 heap，稳定后再扩展到外设窗口、DMA buffer 和只读表。
6. 每次只改一个接入点，避免把 startup、tick、临界区和低功耗同时改动后无法定位故障。

STM32 最小接入骨架如下，真实工程中可把寄存器写入替换为厂商 HAL 或 CMSIS 调用，但必须保持 tick、PendSV 和 ISR 延迟切换语义一致。

```c
void SysTick_Handler(void)
{
    MRT_KernelTick();
}

void USARTx_IRQHandler(void)
{
    bool should_yield = false;
    uint8_t byte = USARTx->DR;

    (void)MRT_QueueSendFromISR(rx_queue, &byte, &should_yield);
    if (should_yield)
    {
        MRT_PortYieldFromISR(should_yield);
    }
}

int main(void)
{
    uint32_t reload = 0u;

    SystemInit();
    BoardPeripheralInit();
    MRT_KernelInitialize();
    MRT_HeapInitialize(heap_buffer, sizeof(heap_buffer), MRT_HEAP_MODE_COALESCING);
    CreateApplicationTasks();
    MRT_PortStm32CmCalculateSysTickReload(SystemCoreClock, MRT_CFG_TICK_RATE_HZ, &reload);
    ConfigureSysTickReload(reload);
    MRT_KernelStart();

    for (;;)
    {
    }
}
```

STM32 smoke 工程落地步骤：

1. 在仓库根目录运行 `python tools\verify\check_embedded_smoke_projects.py`。该命令会检查 `examples/stm32/` 文件是否齐全，并使用 `arm-none-eabi-gcc` 编译链接 `build/embedded-smoke/stm32_smoke.elf`。
2. 打开 `examples/stm32/README.md`，确认工程角色划分：`main.c` 负责任务、队列、定时器和 ISR 连接；`startup_stm32cm.c` 负责向量表和 Reset_Handler；`linker.ld` 负责 FLASH/RAM 布局；`mrt_port_stm32_smoke.c` 负责公共端口接口；`src/portable/stm32_cm/mrt_port_stm32_cm_context.S` 负责 SVC/PendSV 汇编入口骨架。
3. 第一轮板级移植先保留 `main.c` 的对象创建顺序，只替换 `mrt_port_stm32_smoke.c` 中的临界区、PendSV、SVC 和 tickless sleep 实现。这样可以把“应用对象是否创建成功”和“CPU 上下文切换是否正确”分开排错。
4. 把 `startup_stm32cm.c` 与厂商 startup 文件合并时，只保留一个向量表。若厂商工程已经定义 `SysTick_Handler`、`PendSV_Handler` 或 `SVC_Handler`，必须把 MyRTOS 调用合并进去，不能产生重复符号。
5. 链接脚本迁移时，先按 `examples/stm32/linker.ld` 检查 `.isr_vector`、`.text`、`.data`、`.bss`、`_estack`、`_sidata`、`_sdata`、`_edata`、`_sbss`、`_ebss` 是否齐全，再替换为目标芯片真实 FLASH/RAM 容量。
6. 首次烧录后按顺序验证：Reset_Handler 能进入 `main()`；`MRT_HeapInitialize()` 返回成功；两个静态任务创建成功；`MRT_PortStm32CmInitializeStack()` 为两个任务写入初始异常帧；`MRT_TaskKernelSetStackTop()` 把初始 PSP 写回 TCB；UART 队列创建成功；软件定时器启动命令经 `MRT_TimerServiceRunPending()` 生效；SysTick 能推进 `MRT_KernelTick()`。
7. 接入真实板级验证前，不要把 smoke 现象解释为完整调度成功。`examples/stm32/main.c`、`examples/stm32/mrt_port_stm32_smoke.c` 和 `src/portable/stm32_cm/mrt_port_stm32_cm_context.S` 已经具备初始栈帧、PSP 写入/读出 TCB 的可编译契约，真实任务切换还必须在目标芯片上证明 R4-R11、LR/EXC_RETURN、异常返回路径以及可选 FPU 状态保存恢复正确。
8. 板级记录应至少包含：编译命令、ELF/map 路径、芯片型号、系统时钟、tick 频率、NVIC 优先级位宽、临界区策略、PendSV/SVC 汇编来源、LED/UART/timer smoke 结果。

### 5.5 板级验收

- 记录编译命令、ELF/map 路径、芯片型号、系统时钟、tick 频率、NVIC 优先级位宽、临界区策略、PendSV/SVC 汇编来源、LED/UART/timer smoke 结果。
- 验证项至少包括：编译通过、首任务运行、SysTick 计数推进、ISR 唤醒延迟切换、临界区嵌套恢复、MPU 规整区域生效、tickless 睡眠后顺序不乱。

STM32 移植验收清单：

- 编译检查：`arm-none-eabi-gcc` 无未定义 handler，map 文件中 MyRTOS 内核、端口汇编、任务栈和 heap 均已链接。
- 启动检查：`MRT_KernelStart()` 后首个最高优先级任务运行，空闲路径不会回到 `main()`。
- tick 检查：1 秒内 `MRT_KernelGetTick()` 增量等于 `MRT_CFG_TICK_RATE_HZ`，误差只来自晶振和测量工具。
- 切换检查：高优先级任务被 ISR 唤醒后，在 ISR 退出后抢占低优先级任务，而不是在 ISR 内直接切换。
- 临界区检查：嵌套进入/退出临界区后中断屏蔽状态恢复到进入前状态，高紧急中断不调用 MyRTOS API。
  - MPU 检查：任务栈、heap、DMA buffer 的 MPU 规整区域覆盖原始范围，额外覆盖字节已经在链接脚本中隔离或允许相同权限。
  - 低功耗检查：tickless 睡眠前后任务延时和软件定时器到期顺序保持一致。
  - 故障检查：开启断言 hook、trace hook 和栈水位查询，至少覆盖任务创建失败、队列满、ISR 唤醒任务、tickless 唤醒四类场景。

### 5.6 真实板级证据归档

1. 采集前先看 `docs/verification/hardware_smoke/raw_log_schema.md` 和 `docs/verification/hardware_smoke/collection_checklist.md`，按字段契约输出 `Key: Value` 原始 UART/trace 日志，建议命名为 `docs/verification/hardware_smoke/stm32_uart_raw.log`。
2. 修改或确认 `docs/verification/hardware_smoke/hardware_smoke_preflight.json` 中的 STM32 节点：`chip`、`board`、`compiler_command`、`build_command`、`flash_command`、`capture_command`、`raw_log`、`evidence_output`、`minimum_runtime_minutes` 和 `expected_fields` 都必须是当前板卡的真实信息。然后运行 `python tools\verify\check_hardware_smoke_preflight.py`；若要同时检查本机工具链、烧录器和采集命令是否在 PATH 中，再运行 `python tools\verify\check_hardware_smoke_preflight.py --check-tools`。
3. 运行 `python tools\verify\check_hardware_smoke_raw_log_schema.py`，确认生成器常量、`raw_log_schema.md`、`examples/hardware_smoke/mrt_hardware_smoke_log_schema.h` 和采集指南中的字段契约没有漂移。
4. 运行 `python tools\verify\run_hardware_smoke_capture.py --target STM32` 预演完整顺序；确认 `capture_command` 会保留原始 UART/trace 日志后，再追加 `--execute` 真正执行编译、烧录、采集、证据生成和目标级证据校验。若 `capture_command` 本身已经调用 `generate_hardware_smoke_evidence.py`，执行器不会重复生成证据。
5. 若原始日志已经包含 `Key: Value` 格式字段，也可单独运行 `python tools\verify\generate_hardware_smoke_evidence.py --target STM32 --input docs\verification\hardware_smoke\stm32_uart_raw.log --output docs\verification\hardware_smoke\stm32_board_smoke.md`，由生成器裁掉启动噪声并输出最终证据文件。
6. 若不使用生成器，先从 `docs/verification/hardware_smoke/stm32_board_smoke.template.md` 复制出 `docs/verification/hardware_smoke/stm32_board_smoke.md`，再把 `PENDING`、`FAIL` 和 `TODO` 全部替换为真实板级结果。
7. 证据至少要记录：芯片型号、板卡型号、编译器版本、系统时钟、tick 频率、NVIC 优先级位宽、临界区策略、上下文切换来源、SysTick、PendSV/SVC、ISR 队列、软件定时器、tickless、运行时长、断言次数、heap 最小剩余量、UART/trace 摘要、原始日志路径和原始日志 SHA-256。
8. 真实验收时，必须能从日志里看出 `Reset -> main -> 首任务 -> SysTick -> ISR 唤醒 -> 延迟切换 -> 退出` 这条链路闭环，而不是只有“能编译”。
9. 完成后运行 `python tools\verify\check_hardware_smoke_evidence.py`，它会检查 `docs/verification/hardware_smoke/stm32_board_smoke.md` 是否满足最终格式。
10. 只有在 `Evidence-Status: PASS`、`Runtime-Minutes >= 30`、`Assert-Failures = 0` 且 `Heap-Min-Free-Bytes > 0` 时，STM32 板级 smoke 才算最终可交付。

## 6. DSP 移植步骤

### DSP 移植步骤

本节以 TI C2000/C28x 风格为模型。其他 DSP 族应保留 MyRTOS 公共语义，但按实际 ABI 替换寄存器保存列表、栈方向和软件中断机制。

### 6.1 移植前准备

- 确认目标 DSP 编译器、ABI 文档、链接脚本格式、栈元素宽度、可用 RAM 分区和软件中断资源。
- 明确寄存器分类：调用者保存、被调用者保存、状态寄存器、返回地址寄存器、栈指针、参数传递寄存器，以及必要的扩展寄存器。
- 先决定 tick 源是 CPU timer、ePWM timer 还是片上通用 timer，并确认它能稳定触发 `MRT_KernelTick()`。
- 先决定软件中断或 trap 作为上下文切换触发路径，避免后面再改 ISR 嵌套模型。
- 先准备一份可跑的 host model，确保公共端口语义先验收，再替换成真实汇编。

### 6.2 工程分层

- `main.c` 只负责时钟、外设、对象创建和 `MRT_KernelStart()`。
- `portable/dsp/<family>/` 只放公共 C helper、上下文切换汇编、timer ISR glue 和启动 glue。
- `linker.cmd` 或等价脚本只负责任务栈、heap、DMA buffer、采样帧和中断栈分区。
- `mrt_port_dsp_model.c` 或 host model 只表达调用顺序和耦合关系，不要把真实 ABI 假装成已完成。
- `src/portable/dsp_c28x/mrt_port_dsp_c28x_context.asm` 提供 TI C28x 风格首任务启动、软件中断 yield、寄存器保存/恢复和 C 钩子交接顺序的汇编骨架；该文件用于移植审计和静态检查，当前不参与 host 编译，也不替代真实 DSP 板级 smoke。
- `examples/dsp/startup_c28x.c`、`examples/dsp/mrt_port_dsp_c2000_smoke.c` 和 `examples/dsp/linker_c28x.cmd` 提供 C2000 启动向量、CPU Timer0 tick、软件中断、ADC/DMA ISR、任务栈、heap、DMA buffer 和 trace buffer 分区骨架；这些文件用于移植审计，不替代 TI 工具链编译或真实 DSP 板级 smoke。

### 6.3 关键接入顺序

1. 工具链：确认目标 DSP 编译器、ABI 文档和链接脚本格式。建立独立 `portable/dsp/<family>/` 目录，把公共 C helper、真实汇编上下文切换文件、timer ISR 文件和启动 glue 文件分开。
2. ABI 确认：记录寄存器分类：调用者保存、被调用者保存、状态寄存器、返回地址寄存器、栈指针、参数传递寄存器。若栈元素不是 32 bit，需要在端口头文件中重新定义栈槽宽度或适配 `MRT_StackType`。
3. 栈布局：以 `MRT_PortDspC28xInitializeStack()` 的契约为起点，确认栈向下增长、8 字节对齐、入口 PC、入口参数、退出处理函数、状态字和 XAR4-XAR7 保存槽。真实端口若需要更多寄存器，必须扩展槽位并补测试。
4. tick 定时器：选择 CPU timer、ePWM timer 或片上通用 timer 作为 RTOS tick 源。ISR 中只清中断标志、调用 `MRT_KernelTick()`、根据端口策略请求软件中断切换。不要在 tick ISR 中执行长回调。
5. 上下文切换：选择一个软件中断、trap 或低优先级可挂起中断作为 PendSV 等价物。任务上下文 yield 只置位该软件中断。ISR 中的 `should_yield` 只设置挂起请求，最外层 ISR 退出后再切换。可先按 `mrt_port_dsp_c28x_context.asm` 的保存顺序审计 XAR4-XAR7、ST0/ST1、ACC、P、XT 等寄存器，再按目标 ABI 增删槽位。
6. 嵌套中断：在 ISR 入口调用端口进入钩子，在 ISR 退出调用端口退出钩子。`MRT_PortDspC28xExitInterrupt()` 的 `out_should_switch` 为 true 时，触发软件中断或直接跳转到尾链切换路径。内层 ISR 退出不得切换。
7. C2000 启动与链接：按 `startup_c28x.c` 的向量占位把 CPU Timer0、软件中断、ADC/DMA ISR 接入 PIE 向量表；按 `linker_c28x.cmd` 把 `.mrtos_heap`、`.mrtos_tasks`、`.mrtos_dma` 和 `.mrtos_trace` 映射到目标 RAM，并用 map 文件确认地址不与 CLA、DMA、采样缓冲或 boot ROM 保留区冲突。
8. 临界区：根据 DSP 中断控制器选择全局中断屏蔽位、分组中断屏蔽寄存器或优先级阈值。`MRT_PortEnterCritical()` 必须返回旧状态，`MRT_PortExitCritical()` 必须完整恢复旧状态，支持嵌套。
9. 低功耗：tickless idle 端口应停止或重装 tick timer，进入 idle/standby 指令，唤醒后读取硬件计数或低功耗 timer 估算实际睡眠 tick。若低功耗会关闭主时钟，必须在恢复时重新同步 tick timer。
9. 链接脚本：为任务栈、heap、DMA 缓冲、采样帧和中断栈分区。DSP 项目常有快 RAM、共享 RAM、外部 RAM，任务栈应放在访问延迟可预测的区域。
10. 示例 smoke test：创建一个采样处理任务和一个通信任务；timer ISR 每 1 ms 推进 tick；ADC ISR 把采样块指针写入固定块内存池或队列；软件中断执行上下文切换；低优先级任务用事件组等待处理完成。
11. 排错：若任务入口参数错误，检查 ABI 参数槽；若切换后状态寄存器异常，检查 ST0/ST1 保存恢复；若 ISR 嵌套后不切换，检查进入/退出计数是否回到 0；若随机崩溃，检查栈对齐、双字访问和链接脚本栈区大小。

### 6.3.1 从厂商裸机工程迁入 MyRTOS 的实际顺序

1. 先把 ABI、编译器版本、栈元素宽度、interrupt/trap 入口和 timer 源记到工程说明里，不在未确认 ABI 时写汇编上下文切换。
2. 先只接入 MyRTOS 头文件、kernel 源、DSP helper 和 host model 文件，第一轮只验证编译/链接和 `MRT_PortDspC28xInitializeStack()` 栈帧。
3. 再整理启动文件和链接脚本：保留唯一向量表，拆出任务栈、heap、DMA buffer、采样 buffer 和中断栈分区，先让 `main()` 能完成初始化并进入 `MRT_KernelStart()`。
4. 再接入 CPU timer ISR，只做清标志、`MRT_KernelTick()` 和延迟切换请求，不在 timer ISR 内执行算法、打印或长回调。
5. 再接入软件中断或尾链切换路径，确认 `MRT_PortDspC28xRequestContextSwitch()` 和 `MRT_PortDspC28xExitInterrupt()` 的先后关系正确，且只有最外层 ISR 退出时才切换。
6. 再接入第一个数据 ISR，例如 ADC/DMA。先用队列、事件组或固定块内存池把数据送到任务，再确认任务能在预期 tick 内被唤醒。
7. 再扩展到嵌套中断、tickless 和低功耗。每一步都先确认嵌套计数不下溢、软件中断不丢、恢复后 tick 连续。
8. 最后再接入真实板级 smoke 记录：记录 ABI、栈方向、编译器、timer 精度、上下文切换来源、ISR 深度、队列峰值和 heap 最小剩余量。

### 6.4 首次联调

1. 先只验证编译和链接，确认 DSP 端口源码、启动 glue、timer ISR 和上下文切换文件都进入镜像。
2. 再确认 `MRT_PortDspC28xInitializeStack()` 的栈帧位置、返回地址和参数槽位与目标 ABI 一致。
3. 再验证 CPU timer ISR 只推进 `MRT_KernelTick()`，软件中断只负责延迟切换，不在 ISR 中执行长算法。
4. 再验证 ADC/DMA ISR 通过队列、事件组或固定块内存池把数据交给任务，然后再请求切换。
5. 若启用多层 ISR 嵌套，先确认进入/退出计数不会下溢，再看最外层退出后是否触发切换。
6. 每次只改一个寄存器组或一个 ISR 路径，避免同时改 ABI、tick 和中断控制器。

DSP 最小接入骨架如下。不同 DSP 的寄存器名称不同，示例只表达调用顺序和耦合关系。

```c
interrupt void CpuTimer0Isr(void)
{
    ClearCpuTimer0InterruptFlag();
    MRT_KernelTick();
    MRT_PortDspC28xRequestContextSwitch();
    AcknowledgeTimerInterruptGroup();
}

interrupt void AdcIsr(void)
{
    bool should_switch = false;
    SampleBlock *block = AcquireSampleBlockFromIsr();

    MRT_PortDspC28xEnterInterrupt();
    (void)MRT_QueueSendFromISR(sample_queue, &block, &should_switch);
    if (should_switch)
    {
        MRT_PortDspC28xRequestContextSwitch();
    }
    ClearAdcInterruptFlag();
    MRT_PortDspC28xExitInterrupt(&should_switch);
}

int main(void)
{
    DspClockInit();
    DspInterruptControllerInit();
    MRT_KernelInitialize();
    MRT_HeapInitialize(heap_buffer, sizeof(heap_buffer), MRT_HEAP_MODE_COALESCING);
    CreateDspApplicationTasks();
    ConfigureCpuTimer0ForTick(MRT_CFG_TICK_RATE_HZ);
    EnableSoftwareContextSwitchInterrupt();
    MRT_KernelStart();

    for (;;)
    {
    }
}
```

DSP smoke/model 落地步骤：

1. 在仓库根目录运行 `python tools\verify\check_embedded_smoke_projects.py`。当前环境没有 TI DSP 工具链，因此脚本会用 host `gcc` 编译并运行 `examples/dsp/` 的 C28x 风格模型。
2. `examples/dsp/main.c` 会执行 DSP 栈帧 helper、任务创建、队列创建、定时器启动、ISR 模型进入/退出、软件中断式切换请求和 tickless sleep 模型检查。它用于证明公共语义和调用顺序，而不是替代真实 DSP 汇编。
3. 真实 TI C2000/C28x 工程中，把 `examples/dsp/mrt_port_dsp_model.c` 拆成目标端口文件：中断屏蔽/恢复 C 文件、上下文保存恢复汇编文件、启动/向量表文件、硬件 timer/软件中断/ADC glue 文件和链接命令文件。上下文保存恢复文件可从 `src/portable/dsp_c28x/mrt_port_dsp_c28x_context.asm` 的骨架开始，启动和链接可从 `examples/dsp/startup_c28x.c`、`examples/dsp/mrt_port_dsp_c2000_smoke.c`、`examples/dsp/linker_c28x.cmd` 的骨架开始，但必须按目标编译器 ABI、芯片寄存器集、PIE 向量表和 RAM/flash 分区校正。
4. 按目标 ABI 校验 `MRT_PortDspC28xInitializeStack()` 的槽位。若真实 ABI 需要保存更多寄存器，应扩展槽位枚举、修改端口 helper，并补充对应 host 测试。
5. 把 CPU timer ISR 写成短路径：清硬件中断标志、调用 `MRT_KernelTick()`、按 `should_yield` 请求软件中断、确认中断控制器分组。不要在 timer ISR 中运行算法或定时器回调。
6. ADC/DMA ISR 先通过固定块内存池或队列把数据交给任务，再通过软件中断请求延迟切换。多层 ISR 嵌套时只允许最外层退出后执行任务切换。
7. 链接脚本要把任务栈、heap、采样 buffer、DMA buffer、通信 buffer 放入确定 RAM 区域，并记录 cache/共享 RAM 同步策略。
8. 真实板级 smoke 结果应记录：DSP 型号、编译器版本、ABI 模式、栈方向、任务栈地址范围、链接 map 中 `.mrtos_heap`/`.mrtos_tasks`/`.mrtos_dma`/`.mrtos_trace` 地址、timer tick 精度、上下文汇编来源、ISR 嵌套深度峰值、队列峰值、heap 最小剩余量和 30 分钟混合负载结果。

### 6.5 板级验收

- 记录 DSP 型号、编译器版本、ABI 模式、栈方向、任务栈地址范围、timer tick 精度、ISR 嵌套深度峰值、队列峰值、heap 最小剩余量和 30 分钟混合负载结果。
- 验证项至少包括：编译通过、首任务运行、tick 推进、软件中断切换、嵌套 ISR 退栈正确、数据通路能唤醒任务、缓存或共享 RAM 策略可复现。

DSP 移植验收清单：

- ABI 检查：上下文切换汇编保存并恢复所有被调用者保存寄存器、状态寄存器、返回地址、栈指针和必要的扩展寄存器。
- 栈检查：任务栈满足目标 ABI 对齐要求，入口参数能被任务函数稳定读取，任务误返回时进入统一退出兜底函数。
- tick 检查：硬件 timer ISR 只推进内核 tick 并请求延迟切换，不执行长时间 DSP 算法。
- 嵌套检查：多层 ISR 中只有最外层退出后允许切换，`MRT_PortDspC28xGetInterruptNesting()` 不出现下溢。
  - 数据通路检查：ADC/DMA ISR 通过队列、事件组或固定块内存池把数据交给任务，任务侧能在预期 tick 内被唤醒。
  - 内存检查：任务栈、heap、DMA buffer、采样 buffer 放在确定的 RAM 区域，cache 或共享 RAM 同步策略已写入端口说明。
  - smoke 检查：至少运行 30 分钟采样/通信/定时器混合负载，记录 tick 计数、队列峰值、内存最小剩余量和断言 hook 触发次数。

### 6.6 真实板级证据归档

1. 采集前先看 `docs/verification/hardware_smoke/raw_log_schema.md` 和 `docs/verification/hardware_smoke/collection_checklist.md`，按字段契约输出 `Key: Value` 原始 UART/trace 日志，建议命名为 `docs/verification/hardware_smoke/dsp_uart_raw.log`。
2. 修改或确认 `docs/verification/hardware_smoke/hardware_smoke_preflight.json` 中的 DSP 节点：`chip`、`board`、`compiler_command`、`build_command`、`flash_command`、`capture_command`、`raw_log`、`evidence_output`、`minimum_runtime_minutes`、`expected_fields`、`abi`、`stack_direction`、`timer_source` 和 `context_switch` 都必须是当前 DSP 工程的真实信息。然后运行 `python tools\verify\check_hardware_smoke_preflight.py`；若要同时检查本机 TI/ADI/目标 DSP 工具链和烧录采集工具是否在 PATH 中，再运行 `python tools\verify\check_hardware_smoke_preflight.py --check-tools`。
3. 运行 `python tools\verify\check_hardware_smoke_raw_log_schema.py`，确认生成器常量、`raw_log_schema.md`、`examples/hardware_smoke/mrt_hardware_smoke_log_schema.h` 和采集指南中的字段契约没有漂移。
4. 运行 `python tools\verify\run_hardware_smoke_capture.py --target DSP` 预演完整顺序；确认 `capture_command` 会保留原始 UART/trace 日志后，再追加 `--execute` 真正执行编译、烧录、采集、证据生成和目标级证据校验。若 `capture_command` 本身已经调用 `generate_hardware_smoke_evidence.py`，执行器不会重复生成证据。
5. 若原始日志已经包含 `Key: Value` 格式字段，也可单独运行 `python tools\verify\generate_hardware_smoke_evidence.py --target DSP --input docs\verification\hardware_smoke\dsp_uart_raw.log --output docs\verification\hardware_smoke\dsp_board_smoke.md`，由生成器裁掉启动噪声并输出最终证据文件。
6. 若不使用生成器，先从 `docs/verification/hardware_smoke/dsp_board_smoke.template.md` 复制出 `docs/verification/hardware_smoke/dsp_board_smoke.md`，再把 `PENDING`、`FAIL` 和 `TODO` 全部替换为真实板级结果。
7. 证据至少要记录：DSP 型号、板卡型号、编译器版本、ABI 模式、栈方向、上下文切换来源、timer tick 精度、软件中断切换、ISR 嵌套深度峰值、队列峰值、heap 最小剩余量、运行时长、UART/trace 摘要、原始日志路径和原始日志 SHA-256。
8. 实机联调时，必须能从日志里看出 `timer ISR -> MRT_KernelTick() -> 软件中断请求 -> 最外层 ISR 退出 -> 切换发生` 这条链路闭合。
9. 完成后运行 `python tools\verify\check_hardware_smoke_evidence.py`，它会检查 `docs/verification/hardware_smoke/dsp_board_smoke.md` 是否满足最终格式。
10. 只有在 `Evidence-Status: PASS`、`Runtime-Minutes >= 30`、`Assert-Failures = 0` 且 `Heap-Min-Free-Bytes > 0` 时，DSP 板级 smoke 才算最终可交付。

## 7. 附录

### 7.1 基础类型

- `MRT_Tick`：系统节拍计数类型。
- `MRT_Timeout`：阻塞等待 tick 数。
- `MRT_Priority`：任务优先级，数值越大优先级越高。
- `MRT_StackType`：任务栈元素类型。
- `MRT_IntState`：临界区旧中断状态保存类型。

### 7.2 返回值

- `MRT_RESULT_OK`：操作成功。
- `MRT_RESULT_TIMEOUT`：等待超时。
- `MRT_RESULT_INVALID_ARGUMENT`：参数非法。
- `MRT_RESULT_NO_MEMORY`：内存不足。
- `MRT_RESULT_INVALID_CONTEXT`：调用上下文非法。
- `MRT_RESULT_OBJECT_BUSY`：对象忙。
- `MRT_RESULT_OBJECT_EMPTY`：对象空。
- `MRT_RESULT_OBJECT_FULL`：对象满。
- `MRT_RESULT_OWNER_ERROR`：所有权错误。
- `MRT_RESULT_NOT_STARTED`：对象或内核未启动。
- `MRT_RESULT_ALREADY_STARTED`：对象或内核已启动。
- `MRT_RESULT_INTERNAL_ERROR`：内部保护性错误。

### 7.3 常用配置宏

- `MRT_CFG_MAX_PRIORITIES`：最大优先级数量。
- `MRT_CFG_TICK_RATE_HZ`：系统 tick 频率。
- `MRT_CFG_MINIMAL_STACK_WORDS`：最小任务栈元素数。
- `MRT_CFG_USE_PREEMPTION`：是否启用抢占。
- `MRT_CFG_USE_TIME_SLICING`：是否启用同优先级时间片。
- `MRT_CFG_SUPPORT_STATIC_ALLOCATION`：是否支持静态对象。
- `MRT_CFG_SUPPORT_DYNAMIC_ALLOCATION`：是否支持动态对象。
- `MRT_CFG_HEAP_ALIGNMENT`：heap 对齐字节数。
- `MRT_CFG_USE_TRACE`：是否启用 trace。
- `MRT_CFG_USE_TICKLESS_IDLE`：是否启用 tickless idle。
- `MRT_CFG_TIMER_COMMAND_QUEUE_LENGTH`：软件定时器服务命令队列长度。
- `MRT_CFG_TIMER_PENDING_FUNCTION_QUEUE_LENGTH`：pending function 历史默认容量，当前用于给 `MRT_CFG_TIMER_COMMAND_QUEUE_LENGTH` 提供默认值。

### 7.4 验证命令

  ```powershell
  python tools\verify\run_release_verification.py
  python tools\verify\check_hardware_smoke_preflight.py
  python tools\verify\check_hardware_smoke_raw_log_schema.py
  python tools\verify\run_hardware_smoke_capture.py --target STM32
  python tools\verify\run_release_verification.py --require-hardware
  ```
