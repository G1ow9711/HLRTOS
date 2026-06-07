# MyRTOS API 目录设计

> 状态：设计草案。  
> 用途：约束后续源码、测试、手册三者的公共接口一致性。  
> 注意：本文件只定义原创 API 形态，不复用 FreeRTOS 函数名、宏名或手册表达。

## 1. 命名与句柄

### 1.1 公共前缀

- 类型：`MRT_` 前缀，例如 `MRT_TaskHandle`。
- 函数：`MRT_模块动作`，例如 `MRT_TaskCreate`。
- ISR 版本：统一后缀 `FromISR`。
- 配置宏：`MRT_CFG_` 前缀。
- trace 宏：`MRT_TRACE_` 前缀。

### 1.2 基础类型

```c
typedef uint32_t MRT_Tick;
typedef uint32_t MRT_Timeout;
typedef uint32_t MRT_Priority;
typedef uint32_t MRT_StackType;
typedef uint32_t MRT_EventBits;
typedef uint32_t MRT_NotifyValue;
typedef uintptr_t MRT_IntState;
typedef struct MRT_Task *MRT_TaskHandle;
typedef struct MRT_Queue *MRT_QueueHandle;
typedef struct MRT_Semaphore *MRT_SemaphoreHandle;
typedef struct MRT_Mutex *MRT_MutexHandle;
typedef struct MRT_EventGroup *MRT_EventGroupHandle;
typedef struct MRT_Timer *MRT_TimerHandle;
typedef struct MRT_StreamBuffer *MRT_StreamBufferHandle;
typedef struct MRT_MessageBuffer *MRT_MessageBufferHandle;
```

### 1.3 统一返回值

```c
typedef enum MRT_Result {
    MRT_RESULT_OK = 0,
    MRT_RESULT_TIMEOUT,
    MRT_RESULT_INVALID_ARGUMENT,
    MRT_RESULT_NO_MEMORY,
    MRT_RESULT_INVALID_CONTEXT,
    MRT_RESULT_OBJECT_BUSY,
    MRT_RESULT_OBJECT_EMPTY,
    MRT_RESULT_OBJECT_FULL,
    MRT_RESULT_OWNER_ERROR,
    MRT_RESULT_NOT_STARTED,
    MRT_RESULT_ALREADY_STARTED,
    MRT_RESULT_INTERNAL_ERROR
} MRT_Result;
```

## 2. 内核与调度 API

| API | 功能 | 关键测试 |
|-----|------|----------|
| `MRT_KernelInitialize(void)` | 初始化内核全局状态 | 重复初始化、空对象状态 |
| `MRT_KernelStart(void)` | 启动调度器 | 首任务启动、未创建任务 |
| `MRT_KernelIsRunning(void)` | 查询调度器状态 | 启动前/后状态 |
| `MRT_KernelGetTick(void)` | 获取当前 tick | tick 增长与溢出 |
| `MRT_KernelTick(void)` | 由 tick 中断或仿真器推进内核时间 | 延时到期、定时器到期 |
| `MRT_KernelYield(void)` | 主动让出 CPU | 同优先级轮转 |
| `MRT_KernelSuspendAll(void)` | 挂起调度器 | 嵌套挂起、pending ready |
| `MRT_KernelResumeAll(void)` | 恢复调度器 | pending ready 触发切换 |

## 3. 任务 API

| API | 功能 | 关键测试 |
|-----|------|----------|
| `MRT_TaskCreateStatic(...)` | 使用外部 TCB/栈创建任务 | 参数校验、栈对齐 |
| `MRT_TaskCreate(...)` | 动态创建任务 | 分配失败、释放路径 |
| `MRT_TaskDelete(MRT_TaskHandle task)` | 删除任务 | 删除当前任务、删除持锁任务 |
| `MRT_TaskSuspend(MRT_TaskHandle task)` | 挂起任务 | 就绪/阻塞任务挂起 |
| `MRT_TaskResume(MRT_TaskHandle task)` | 恢复任务 | 恢复后抢占 |
| `MRT_TaskResumeFromISR(MRT_TaskHandle task, bool *should_yield)` | ISR 恢复任务 | 触发/不触发切换 |
| `MRT_TaskDelay(MRT_Tick ticks)` | 相对延时 | 0 tick、溢出 |
| `MRT_TaskDelayUntil(MRT_Tick *previous, MRT_Tick period)` | 周期延时 | 周期漂移控制 |
| `MRT_TaskSetPriority(MRT_TaskHandle task, MRT_Priority priority)` | 设置优先级 | 优先级改变后重排 |
| `MRT_TaskGetPriority(MRT_TaskHandle task, MRT_Priority *out_priority)` | 获取优先级 | 空句柄、当前任务 |
| `MRT_TaskGetCurrent(void)` | 获取当前任务 | 调度前/后 |
| `MRT_TaskGetState(MRT_TaskHandle task, MRT_TaskState *out_state)` | 获取任务状态 | 全状态覆盖 |
| `MRT_TaskGetName(MRT_TaskHandle task)` | 获取任务名 | 空名、长名截断 |
| `MRT_TaskGetStackHighWaterMark(MRT_TaskHandle task, size_t *out_words)` | 获取栈剩余水位 | mock 栈填充 |

## 4. 队列 API

| API | 功能 | 关键测试 |
|-----|------|----------|
| `MRT_QueueCreateStatic(...)` | 静态创建队列 | 缓冲区大小校验 |
| `MRT_QueueCreate(...)` | 动态创建队列 | 分配失败 |
| `MRT_QueueDelete(MRT_QueueHandle queue)` | 删除队列 | 唤醒等待者策略 |
| `MRT_QueueSend(MRT_QueueHandle queue, const void *item, MRT_Timeout timeout)` | 队尾发送 | 满队列阻塞/超时 |
| `MRT_QueueSendFront(...)` | 队头发送 | 顺序验证 |
| `MRT_QueueOverwrite(...)` | 单槽覆盖写入 | 非单槽拒绝 |
| `MRT_QueueReceive(MRT_QueueHandle queue, void *out_item, MRT_Timeout timeout)` | 接收元素 | 空队列阻塞/超时 |
| `MRT_QueuePeek(...)` | 查看不移除 | 多次 peek 一致 |
| `MRT_QueueSendFromISR(...)` | ISR 发送 | should_yield |
| `MRT_QueueReceiveFromISR(...)` | ISR 接收 | 空队列返回 |
| `MRT_QueueMessagesWaiting(...)` | 查询已有元素数 | 边界计数 |
| `MRT_QueueSpacesAvailable(...)` | 查询剩余空间 | 边界计数 |
| `MRT_QueueReset(...)` | 清空队列 | 等待者状态 |

## 5. 信号量 API

| API | 功能 | 关键测试 |
|-----|------|----------|
| `MRT_SemaphoreCreateBinaryStatic(...)` | 静态创建二值信号量 | 初始空/满 |
| `MRT_SemaphoreCreateCountingStatic(...)` | 静态创建计数信号量 | 最大计数校验 |
| `MRT_SemaphoreCreateBinary(...)` | 动态创建二值信号量 | 分配失败 |
| `MRT_SemaphoreCreateCounting(...)` | 动态创建计数信号量 | 分配失败 |
| `MRT_SemaphoreDelete(...)` | 删除信号量 | 等待任务处理 |
| `MRT_SemaphoreTake(...)` | 获取信号量 | 阻塞/超时 |
| `MRT_SemaphoreGive(...)` | 释放信号量 | 计数上限 |
| `MRT_SemaphoreGiveFromISR(...)` | ISR 释放信号量 | 唤醒高优先级任务 |
| `MRT_SemaphoreGetCount(...)` | 获取当前计数 | 边界计数 |

## 6. 互斥锁 API

| API | 功能 | 关键测试 |
|-----|------|----------|
| `MRT_MutexCreateStatic(...)` | 静态创建互斥锁 | 初始所有权 |
| `MRT_MutexCreate(...)` | 动态创建互斥锁 | 分配失败 |
| `MRT_MutexCreateRecursiveStatic(...)` | 静态创建递归互斥锁 | 递归计数 |
| `MRT_MutexCreateRecursive(...)` | 动态创建递归互斥锁 | 分配失败 |
| `MRT_MutexDelete(...)` | 删除互斥锁 | 持锁删除策略 |
| `MRT_MutexLock(...)` | 加锁 | 优先级继承 |
| `MRT_MutexUnlock(...)` | 解锁 | 优先级恢复 |
| `MRT_MutexGetOwner(...)` | 查询拥有者 | 无主/有主 |

## 7. 事件组 API

| API | 功能 | 关键测试 |
|-----|------|----------|
| `MRT_EventGroupCreateStatic(...)` | 静态创建事件组 | 初始 bit 为 0 |
| `MRT_EventGroupCreate(...)` | 动态创建事件组 | 分配失败 |
| `MRT_EventGroupDelete(...)` | 删除事件组 | 等待者唤醒 |
| `MRT_EventGroupSetBits(...)` | 设置事件 bit | 多任务唤醒 |
| `MRT_EventGroupClearBits(...)` | 清除事件 bit | 清位准确 |
| `MRT_EventGroupWaitBits(...)` | 等待事件 bit | wait-any/wait-all/超时 |
| `MRT_EventGroupSetBitsFromISR(...)` | ISR 设置事件 bit | 延迟处理/唤醒 |
| `MRT_EventGroupGetBits(...)` | 读取事件 bit | 并发一致性 |

## 8. 任务通知 API

| API | 功能 | 关键测试 |
|-----|------|----------|
| `MRT_TaskNotify(...)` | 向任务发送通知 | set bits/increment/overwrite |
| `MRT_TaskNotifyFromISR(...)` | ISR 发送通知 | should_yield |
| `MRT_TaskNotifyWait(...)` | 等待通知 | 进入清位/退出清位 |
| `MRT_TaskNotifyTake(...)` | 以计数信号量方式等待 | clear/count 模式 |
| `MRT_TaskNotifyStateClear(...)` | 清除通知状态 | 未读状态 |
| `MRT_TaskNotifyValueClear(...)` | 清除通知值 bit | bit 操作准确 |

## 9. 软件定时器 API

| API | 功能 | 关键测试 |
|-----|------|----------|
| `MRT_TimerCreateStatic(...)` | 静态创建定时器 | 周期参数校验 |
| `MRT_TimerCreate(...)` | 动态创建定时器 | 分配失败 |
| `MRT_TimerDelete(...)` | 删除定时器 | 命令队列路径 |
| `MRT_TimerStart(...)` | 启动定时器 | 单次/周期 |
| `MRT_TimerStop(...)` | 停止定时器 | 未启动停止 |
| `MRT_TimerReset(...)` | 复位定时器 | 到期时间重算 |
| `MRT_TimerChangePeriod(...)` | 修改周期 | 运行中修改 |
| `MRT_TimerIsActive(...)` | 查询活动状态 | 命令处理前后 |
| `MRT_TimerPendFunctionCall(...)` | 投递延迟函数调用 | 服务任务执行 |

## 10. 流缓冲与消息缓冲 API

| API | 功能 | 关键测试 |
|-----|------|----------|
| `MRT_StreamBufferCreateStatic(...)` | 静态创建流缓冲 | 触发水位校验 |
| `MRT_StreamBufferCreate(...)` | 动态创建流缓冲 | 分配失败 |
| `MRT_StreamBufferSend(...)` | 写入字节流 | 环绕、阻塞 |
| `MRT_StreamBufferReceive(...)` | 读取字节流 | 水位唤醒 |
| `MRT_StreamBufferSendFromISR(...)` | ISR 写入字节流 | should_yield |
| `MRT_StreamBufferReceiveFromISR(...)` | ISR 读取字节流 | 空缓冲 |
| `MRT_StreamBufferBytesAvailable(...)` | 查询可读字节 | 环绕计数 |
| `MRT_StreamBufferSpacesAvailable(...)` | 查询可写空间 | 环绕计数 |
| `MRT_MessageBufferCreateStatic(...)` | 静态创建消息缓冲 | 长度字段空间 |
| `MRT_MessageBufferCreate(...)` | 动态创建消息缓冲 | 分配失败 |
| `MRT_MessageBufferSend(...)` | 写入完整消息 | 不允许半包 |
| `MRT_MessageBufferReceive(...)` | 读取完整消息 | 输出缓冲不足 |
| `MRT_MessageBufferSendFromISR(...)` | ISR 写入消息 | 完整性 |
| `MRT_MessageBufferReceiveFromISR(...)` | ISR 读取消息 | 空消息 |

## 11. 内存管理 API

| API | 功能 | 关键测试 |
|-----|------|----------|
| `MRT_HeapInitialize(...)` | 初始化堆区域 | 对齐、重复初始化 |
| `MRT_Malloc(size_t size)` | 分配内存 | 边界、失败 |
| `MRT_Free(void *ptr)` | 释放内存 | 空指针、合并 |
| `MRT_HeapGetFreeSize(...)` | 查询剩余堆 | 统计准确 |
| `MRT_HeapGetMinimumEverFreeSize(...)` | 查询历史最低剩余堆 | 水位准确 |
| `MRT_MemoryPoolCreateStatic(...)` | 静态创建固定块池 | 块大小/对齐 |
| `MRT_MemoryPoolAlloc(...)` | 分配固定块 | 池空 |
| `MRT_MemoryPoolFree(...)` | 释放固定块 | 重复释放检测 |

## 12. Tickless、trace、断言 API

| API | 功能 | 关键测试 |
|-----|------|----------|
| `MRT_TicklessGetExpectedIdleTicks(...)` | 获取可睡眠 tick 数 | 定时器/延时约束 |
| `MRT_TicklessEnterIdle(...)` | 进入 tickless idle | tick 补偿 |
| `MRT_TraceSetSink(...)` | 设置 trace 接收器 | 开关无副作用 |
| `MRT_StatsGetTaskRuntime(...)` | 获取任务运行统计 | 切换统计 |
| `MRT_AssertFailed(...)` | 断言失败处理 | 可替换 hook |

## 13. 移植层接口

| API | 功能 | 关键测试 |
|-----|------|----------|
| `MRT_PortInitialize(void)` | 初始化端口层 | 重复初始化 |
| `MRT_PortStartFirstTask(void)` | 启动首任务 | 调用顺序 |
| `MRT_PortYield(void)` | 请求上下文切换 | PendSV/软件中断触发 |
| `MRT_PortYieldFromISR(bool should_yield)` | ISR 末尾按需切换 | true/false 分支 |
| `MRT_PortEnterCritical(void)` | 进入临界区 | 嵌套保存 |
| `MRT_PortExitCritical(MRT_IntState state)` | 退出临界区 | 状态恢复 |
| `MRT_PortIsInsideISR(void)` | 判断 ISR 上下文 | mock 切换 |
| `MRT_PortSetupTimerInterrupt(MRT_Tick tick_hz)` | 配置 tick 中断 | 参数校验 |
| `MRT_PortInitializeStack(...)` | 初始化任务栈帧 | STM32/DSP 栈布局 |
| `MRT_PortSuppressTicksAndSleep(...)` | 端口低功耗睡眠 | tickless 补偿 |

## 14. 手册覆盖规则

每个 public API 必须在最终手册中有同名条目，并包含：

- 函数原型。
- 功能说明。
- 参数表。
- 返回值表。
- 调用上下文。
- 阻塞行为。
- ISR 限制。
- 配置宏依赖。
- 调用示例。
- 常见错误。

移植章节必须额外覆盖：

- STM32 Cortex-M4/M7 的 ARM GCC/CMake 工程接入步骤。
- STM32 启动文件、向量表、SysTick、PendSV、SVC 配置步骤。
- STM32 中断优先级、临界区、BASEPRI/PRIMASK 选择规则。
- STM32 链接脚本、任务栈、堆区与 `.bss`/`.data` 注意事项。
- DSP 工具链、ABI、寄存器保存、栈增长方向、对齐要求。
- DSP tick 定时器、软件中断上下文切换、嵌套中断策略。
- STM32 和 DSP smoke test 逐步验收流程。

