#ifndef MYRTOS_MRT_TYPES_H
#define MYRTOS_MRT_TYPES_H

/**
 * @file mrt_types.h
 * @brief MyRTOS 基础公共类型定义。
 *
 * 本文件只定义所有模块共享的基础类型、对象句柄和统一返回值。
 * 这些类型保持小而稳定，便于 STM32、DSP 和 host 测试环境共用同一套 API。
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** @brief 系统节拍计数类型，单位由 MRT_CFG_TICK_RATE_HZ 决定。 */
typedef uint32_t MRT_Tick;

/** @brief API 阻塞等待时间类型，单位为系统 tick。 */
typedef uint32_t MRT_Timeout;

/** @brief 任务优先级类型，数值越大表示优先级越高。 */
typedef uint32_t MRT_Priority;

/** @brief 任务栈元素类型，端口层可根据 CPU 字长调整。 */
typedef uint32_t MRT_StackType;

/** @brief 事件组 bit 集合类型。 */
typedef uint32_t MRT_EventBits;

/** @brief 任务通知值类型。 */
typedef uint32_t MRT_NotifyValue;

/** @brief 临界区入口状态保存类型，用于恢复进入临界区前的中断状态。 */
typedef uintptr_t MRT_IntState;

/** @brief 任务对象句柄，用户只能通过 API 操作该不透明指针。 */
typedef struct MRT_Task *MRT_TaskHandle;

/** @brief 队列对象句柄，用户只能通过 API 操作该不透明指针。 */
typedef struct MRT_Queue *MRT_QueueHandle;

/** @brief 信号量对象句柄，用户只能通过 API 操作该不透明指针。 */
typedef struct MRT_Semaphore *MRT_SemaphoreHandle;

/** @brief 互斥锁对象句柄，用户只能通过 API 操作该不透明指针。 */
typedef struct MRT_Mutex *MRT_MutexHandle;

/** @brief 事件组对象句柄，用户只能通过 API 操作该不透明指针。 */
typedef struct MRT_EventGroup *MRT_EventGroupHandle;

/** @brief 软件定时器对象句柄，用户只能通过 API 操作该不透明指针。 */
typedef struct MRT_Timer *MRT_TimerHandle;

/** @brief 流缓冲对象句柄，用户只能通过 API 操作该不透明指针。 */
typedef struct MRT_StreamBuffer *MRT_StreamBufferHandle;

/** @brief 消息缓冲对象句柄，用户只能通过 API 操作该不透明指针。 */
typedef struct MRT_MessageBuffer *MRT_MessageBufferHandle;

/**
 * @brief MyRTOS 公共 API 统一返回值。
 *
 * 所有返回 MRT_Result 的函数都使用该枚举表达成功、超时、参数错误、
 * 内存不足、上下文非法和对象状态错误等结果，避免不同模块重复定义状态码。
 */
typedef enum MRT_Result {
    /** @brief 操作成功完成。 */
    MRT_RESULT_OK = 0,
    /** @brief 操作在指定等待时间内未完成。 */
    MRT_RESULT_TIMEOUT,
    /** @brief 输入参数非法，例如空指针或越界配置。 */
    MRT_RESULT_INVALID_ARGUMENT,
    /** @brief 内存或对象池空间不足。 */
    MRT_RESULT_NO_MEMORY,
    /** @brief 当前调用上下文非法，例如在 ISR 中调用阻塞 API。 */
    MRT_RESULT_INVALID_CONTEXT,
    /** @brief 对象正忙，当前操作不能立即执行。 */
    MRT_RESULT_OBJECT_BUSY,
    /** @brief 对象为空，例如空队列或空缓冲。 */
    MRT_RESULT_OBJECT_EMPTY,
    /** @brief 对象已满，例如满队列或满缓冲。 */
    MRT_RESULT_OBJECT_FULL,
    /** @brief 所有权错误，例如非持有者释放互斥锁。 */
    MRT_RESULT_OWNER_ERROR,
    /** @brief 内核或对象尚未启动。 */
    MRT_RESULT_NOT_STARTED,
    /** @brief 内核或对象已经启动，不能重复启动。 */
    MRT_RESULT_ALREADY_STARTED,
    /** @brief 内核内部错误，通常表示断言前的保护性返回。 */
    MRT_RESULT_INTERNAL_ERROR
} MRT_Result;

#endif
