# MyRTOS Foundation Kernel Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the first testable MyRTOS foundation: host test harness, core types, config defaults, intrusive list, priority bitmap, mock port, and kernel tick shell.

**Architecture:** This plan creates only low-level infrastructure needed by the scheduler and later RTOS objects. Production code is limited to small, focused C modules under `include/myrtos/` and `src/`; tests drive each API before implementation.

**Tech Stack:** C99, CMake, CTest, custom lightweight C test macros, PowerShell commands on Windows.

---

## File Structure

- Create: `CMakeLists.txt` - root CMake project and test toggle.
- Create: `tests/CMakeLists.txt` - host test executables.
- Create: `tests/support/mrt_test.h` - tiny assertion macros for C tests.
- Create: `tests/unit/test_types_contract.c` - core type/result tests.
- Create: `tests/unit/test_config_defaults.c` - config default tests.
- Create: `tests/unit/test_list.c` - intrusive list tests.
- Create: `tests/unit/test_priority_bitmap.c` - priority bitmap tests.
- Create: `tests/port_mock/test_port_mock.c` - port mock tests.
- Create: `tests/unit/test_kernel_tick.c` - kernel tick tests.
- Create: `include/myrtos/mrt_types.h` - public base types and result enum.
- Create: `include/myrtos/mrt_config.h` - config default macros.
- Create: `include/myrtos/mrt_list.h` - intrusive list API.
- Create: `include/myrtos/mrt_priority.h` - priority bitmap API.
- Create: `include/myrtos/mrt_port.h` - port interface API.
- Create: `include/myrtos/mrt_kernel.h` - kernel tick/init API.
- Create: `src/kernel/mrt_list.c` - intrusive list implementation.
- Create: `src/kernel/mrt_priority.c` - priority bitmap implementation.
- Create: `src/kernel/mrt_kernel.c` - kernel tick/init shell.
- Create: `src/portable/mock/mrt_port_mock.c` - host mock port.
- Modify: `docs/verification/requirements_traceability_matrix.md` - add foundation evidence after tests pass.
- Modify: `docs/verification/coupling_test_matrix.md` - mark port mock foundation rows as enabled after tests pass.
- Modify: `progress.md` - log plan execution and test results.

## Task 1: Test Harness and Core Types

**Files:**
- Create: `CMakeLists.txt`
- Create: `tests/CMakeLists.txt`
- Create: `tests/support/mrt_test.h`
- Create: `tests/unit/test_types_contract.c`
- Create: `include/myrtos/mrt_types.h`

- [ ] **Step 1: Write the failing test and harness**

Create `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.20)
project(MyRTOS C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(MRT_BUILD_TESTS "Build MyRTOS host tests" ON)

add_library(myrtos_kernel INTERFACE)
target_include_directories(myrtos_kernel INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(MRT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
```

Create `tests/CMakeLists.txt`:

```cmake
function(mrt_add_test test_name test_source)
    add_executable(${test_name} ${test_source})
    target_link_libraries(${test_name} PRIVATE myrtos_kernel)
    target_include_directories(${test_name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/support
    )
    add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()

mrt_add_test(test_types_contract unit/test_types_contract.c)
```

Create `tests/support/mrt_test.h`:

```c
#ifndef MRT_TEST_H
#define MRT_TEST_H

#include <stdio.h>
#include <stdlib.h>

#define MRT_TEST_ASSERT_TRUE(expr)                                                     \
    do {                                                                               \
        if (!(expr)) {                                                                 \
            fprintf(stderr, "%s:%d: assertion failed: %s\n", __FILE__, __LINE__, #expr); \
            exit(1);                                                                   \
        }                                                                              \
    } while (0)

#define MRT_TEST_ASSERT_EQ_U32(expected, actual)                                       \
    do {                                                                               \
        unsigned long mrt_expected_value = (unsigned long)(expected);                  \
        unsigned long mrt_actual_value = (unsigned long)(actual);                      \
        if (mrt_expected_value != mrt_actual_value) {                                  \
            fprintf(stderr, "%s:%d: expected %lu got %lu\n",                          \
                    __FILE__, __LINE__, mrt_expected_value, mrt_actual_value);         \
            exit(1);                                                                   \
        }                                                                              \
    } while (0)

#endif
```

Create `tests/unit/test_types_contract.c`:

```c
#include "mrt_test.h"
#include "myrtos/mrt_types.h"

int main(void)
{
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)MRT_RESULT_OK);
    MRT_TEST_ASSERT_TRUE(sizeof(MRT_Tick) == 4u);
    MRT_TEST_ASSERT_TRUE(sizeof(MRT_StackType) >= 4u);
    MRT_TEST_ASSERT_TRUE(sizeof(MRT_TaskHandle) == sizeof(void *));
    return 0;
}
```

- [ ] **Step 2: Run test to verify it fails**

Run:

```powershell
cmake -S . -B build -DMRT_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: build fails because `myrtos/mrt_types.h` does not exist.

- [ ] **Step 3: Write minimal implementation**

Create `include/myrtos/mrt_types.h`:

```c
#ifndef MYRTOS_MRT_TYPES_H
#define MYRTOS_MRT_TYPES_H

/**
 * @file mrt_types.h
 * @brief MyRTOS 基础类型定义。
 *
 * 本文件定义内核公共 API 使用的整数类型、句柄类型和统一返回值。
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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

#endif
```

- [ ] **Step 4: Run test to verify it passes**

Run:

```powershell
cmake -S . -B build -DMRT_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: `test_types_contract` passes.

- [ ] **Step 5: Commit**

Run:

```powershell
git add CMakeLists.txt tests/CMakeLists.txt tests/support/mrt_test.h tests/unit/test_types_contract.c include/myrtos/mrt_types.h
git commit -m "test: establish MyRTOS type contract"
```

## Task 2: Config Defaults

**Files:**
- Modify: `tests/CMakeLists.txt`
- Create: `tests/unit/test_config_defaults.c`
- Create: `include/myrtos/mrt_config.h`

- [ ] **Step 1: Write the failing test**

Modify `tests/CMakeLists.txt` by adding:

```cmake
mrt_add_test(test_config_defaults unit/test_config_defaults.c)
```

Create `tests/unit/test_config_defaults.c`:

```c
#include "mrt_test.h"
#include "myrtos/mrt_config.h"

int main(void)
{
    MRT_TEST_ASSERT_TRUE(MRT_CFG_MAX_PRIORITIES >= 8u);
    MRT_TEST_ASSERT_TRUE(MRT_CFG_TICK_RATE_HZ >= 100u);
    MRT_TEST_ASSERT_TRUE(MRT_CFG_MINIMAL_STACK_WORDS >= 64u);
    MRT_TEST_ASSERT_EQ_U32(1u, MRT_CFG_USE_PREEMPTION);
    MRT_TEST_ASSERT_EQ_U32(1u, MRT_CFG_USE_TIME_SLICING);
    MRT_TEST_ASSERT_EQ_U32(1u, MRT_CFG_SUPPORT_STATIC_ALLOCATION);
    return 0;
}
```

- [ ] **Step 2: Run test to verify it fails**

Run:

```powershell
cmake -S . -B build -DMRT_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: build fails because `myrtos/mrt_config.h` does not exist.

- [ ] **Step 3: Write minimal implementation**

Create `include/myrtos/mrt_config.h`:

```c
#ifndef MYRTOS_MRT_CONFIG_H
#define MYRTOS_MRT_CONFIG_H

/**
 * @file mrt_config.h
 * @brief MyRTOS 默认配置项。
 *
 * 用户工程可以在编译选项中预定义这些宏，从而覆盖默认值。
 */

#ifndef MRT_CFG_MAX_PRIORITIES
#define MRT_CFG_MAX_PRIORITIES 32u
#endif

#ifndef MRT_CFG_TICK_RATE_HZ
#define MRT_CFG_TICK_RATE_HZ 1000u
#endif

#ifndef MRT_CFG_MINIMAL_STACK_WORDS
#define MRT_CFG_MINIMAL_STACK_WORDS 128u
#endif

#ifndef MRT_CFG_USE_PREEMPTION
#define MRT_CFG_USE_PREEMPTION 1u
#endif

#ifndef MRT_CFG_USE_TIME_SLICING
#define MRT_CFG_USE_TIME_SLICING 1u
#endif

#ifndef MRT_CFG_SUPPORT_STATIC_ALLOCATION
#define MRT_CFG_SUPPORT_STATIC_ALLOCATION 1u
#endif

#ifndef MRT_CFG_SUPPORT_DYNAMIC_ALLOCATION
#define MRT_CFG_SUPPORT_DYNAMIC_ALLOCATION 1u
#endif

#ifndef MRT_CFG_USE_TRACE
#define MRT_CFG_USE_TRACE 0u
#endif

#ifndef MRT_CFG_USE_TICKLESS_IDLE
#define MRT_CFG_USE_TICKLESS_IDLE 1u
#endif

#endif
```

- [ ] **Step 4: Run test to verify it passes**

Run:

```powershell
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: `test_types_contract` and `test_config_defaults` pass.

- [ ] **Step 5: Commit**

Run:

```powershell
git add tests/CMakeLists.txt tests/unit/test_config_defaults.c include/myrtos/mrt_config.h
git commit -m "feat: add MyRTOS configuration defaults"
```

## Task 3: Intrusive List

**Files:**
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`
- Create: `tests/unit/test_list.c`
- Create: `include/myrtos/mrt_list.h`
- Create: `src/kernel/mrt_list.c`

- [ ] **Step 1: Write the failing tests**

Modify `tests/CMakeLists.txt` by adding:

```cmake
mrt_add_test(test_list unit/test_list.c)
```

Create `tests/unit/test_list.c`:

```c
#include "mrt_test.h"
#include "myrtos/mrt_list.h"

static void assert_empty_list_has_sentinel_links(void)
{
    MRT_List list;
    MRT_ListInitialize(&list);
    MRT_TEST_ASSERT_TRUE(MRT_ListIsEmpty(&list));
    MRT_TEST_ASSERT_EQ_U32(0u, (unsigned)MRT_ListGetCount(&list));
}

static void assert_insert_tail_preserves_fifo_order(void)
{
    MRT_List list;
    MRT_ListNode first;
    MRT_ListNode second;
    MRT_ListInitialize(&list);
    MRT_ListNodeInitialize(&first, (void *)1u, 10u);
    MRT_ListNodeInitialize(&second, (void *)2u, 20u);
    MRT_ListInsertTail(&list, &first);
    MRT_ListInsertTail(&list, &second);
    MRT_TEST_ASSERT_TRUE(MRT_ListGetHead(&list) == &first);
    MRT_TEST_ASSERT_TRUE(MRT_ListGetNext(&first) == &second);
    MRT_TEST_ASSERT_EQ_U32(2u, (unsigned)MRT_ListGetCount(&list));
}

static void assert_ordered_insert_sorts_by_value(void)
{
    MRT_List list;
    MRT_ListNode high;
    MRT_ListNode low;
    MRT_ListInitialize(&list);
    MRT_ListNodeInitialize(&high, (void *)1u, 50u);
    MRT_ListNodeInitialize(&low, (void *)2u, 10u);
    MRT_ListInsertOrdered(&list, &high);
    MRT_ListInsertOrdered(&list, &low);
    MRT_TEST_ASSERT_TRUE(MRT_ListGetHead(&list) == &low);
}

static void assert_remove_detaches_node(void)
{
    MRT_List list;
    MRT_ListNode node;
    MRT_ListInitialize(&list);
    MRT_ListNodeInitialize(&node, (void *)1u, 10u);
    MRT_ListInsertTail(&list, &node);
    MRT_ListRemove(&node);
    MRT_TEST_ASSERT_TRUE(MRT_ListIsEmpty(&list));
    MRT_TEST_ASSERT_TRUE(!MRT_ListNodeIsLinked(&node));
}

int main(void)
{
    assert_empty_list_has_sentinel_links();
    assert_insert_tail_preserves_fifo_order();
    assert_ordered_insert_sorts_by_value();
    assert_remove_detaches_node();
    return 0;
}
```

- [ ] **Step 2: Run test to verify it fails**

Run:

```powershell
cmake -S . -B build -DMRT_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: build fails because `myrtos/mrt_list.h` does not exist.

- [ ] **Step 3: Write minimal implementation**

Modify `CMakeLists.txt` by replacing the `INTERFACE` target with a static target:

```cmake
add_library(myrtos_kernel STATIC
    src/kernel/mrt_list.c
)
target_include_directories(myrtos_kernel PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
```

Create `include/myrtos/mrt_list.h`:

```c
#ifndef MYRTOS_MRT_LIST_H
#define MYRTOS_MRT_LIST_H

#include "myrtos/mrt_types.h"

typedef struct MRT_List MRT_List;

typedef struct MRT_ListNode {
    struct MRT_ListNode *prev;
    struct MRT_ListNode *next;
    MRT_List *owner;
    void *item;
    MRT_Tick value;
} MRT_ListNode;

struct MRT_List {
    MRT_ListNode sentinel;
    size_t count;
};

void MRT_ListInitialize(MRT_List *list);
void MRT_ListNodeInitialize(MRT_ListNode *node, void *item, MRT_Tick value);
bool MRT_ListIsEmpty(const MRT_List *list);
size_t MRT_ListGetCount(const MRT_List *list);
void MRT_ListInsertTail(MRT_List *list, MRT_ListNode *node);
void MRT_ListInsertOrdered(MRT_List *list, MRT_ListNode *node);
void MRT_ListRemove(MRT_ListNode *node);
MRT_ListNode *MRT_ListGetHead(MRT_List *list);
MRT_ListNode *MRT_ListGetNext(MRT_ListNode *node);
bool MRT_ListNodeIsLinked(const MRT_ListNode *node);

#endif
```

Create `src/kernel/mrt_list.c`:

```c
#include "myrtos/mrt_list.h"

void MRT_ListInitialize(MRT_List *list)
{
    list->sentinel.prev = &list->sentinel;
    list->sentinel.next = &list->sentinel;
    list->sentinel.owner = list;
    list->sentinel.item = 0;
    list->sentinel.value = UINT32_MAX;
    list->count = 0u;
}

void MRT_ListNodeInitialize(MRT_ListNode *node, void *item, MRT_Tick value)
{
    node->prev = 0;
    node->next = 0;
    node->owner = 0;
    node->item = item;
    node->value = value;
}

bool MRT_ListIsEmpty(const MRT_List *list)
{
    return list->count == 0u;
}

size_t MRT_ListGetCount(const MRT_List *list)
{
    return list->count;
}

void MRT_ListInsertTail(MRT_List *list, MRT_ListNode *node)
{
    MRT_ListNode *tail = list->sentinel.prev;
    node->next = &list->sentinel;
    node->prev = tail;
    node->owner = list;
    tail->next = node;
    list->sentinel.prev = node;
    list->count++;
}

void MRT_ListInsertOrdered(MRT_List *list, MRT_ListNode *node)
{
    MRT_ListNode *cursor = list->sentinel.next;
    while ((cursor != &list->sentinel) && (cursor->value <= node->value)) {
        cursor = cursor->next;
    }
    node->next = cursor;
    node->prev = cursor->prev;
    node->owner = list;
    cursor->prev->next = node;
    cursor->prev = node;
    list->count++;
}

void MRT_ListRemove(MRT_ListNode *node)
{
    MRT_List *owner = node->owner;
    node->prev->next = node->next;
    node->next->prev = node->prev;
    owner->count--;
    node->prev = 0;
    node->next = 0;
    node->owner = 0;
}

MRT_ListNode *MRT_ListGetHead(MRT_List *list)
{
    return (list->count == 0u) ? 0 : list->sentinel.next;
}

MRT_ListNode *MRT_ListGetNext(MRT_ListNode *node)
{
    return node->next;
}

bool MRT_ListNodeIsLinked(const MRT_ListNode *node)
{
    return node->owner != 0;
}
```

- [ ] **Step 4: Run test to verify it passes**

Run:

```powershell
cmake -S . -B build -DMRT_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: type, config, and list tests pass.

- [ ] **Step 5: Refactor for comment standard**

Add Chinese Doxygen comments to every function in `include/myrtos/mrt_list.h` and add internal Chinese step comments to `src/kernel/mrt_list.c` while keeping behavior unchanged.

Run:

```powershell
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: all existing tests still pass.

- [ ] **Step 6: Commit**

Run:

```powershell
git add CMakeLists.txt tests/CMakeLists.txt tests/unit/test_list.c include/myrtos/mrt_list.h src/kernel/mrt_list.c
git commit -m "feat: add intrusive kernel list"
```

## Task 4: Priority Bitmap

**Files:**
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`
- Create: `tests/unit/test_priority_bitmap.c`
- Create: `include/myrtos/mrt_priority.h`
- Create: `src/kernel/mrt_priority.c`

- [ ] **Step 1: Write the failing tests**

Add to `tests/CMakeLists.txt`:

```cmake
mrt_add_test(test_priority_bitmap unit/test_priority_bitmap.c)
```

Create `tests/unit/test_priority_bitmap.c`:

```c
#include "mrt_test.h"
#include "myrtos/mrt_priority.h"

int main(void)
{
    MRT_PriorityBitmap bitmap;
    MRT_PriorityBitmapInitialize(&bitmap);
    MRT_TEST_ASSERT_TRUE(!MRT_PriorityBitmapFindHighest(&bitmap, 0));

    MRT_PriorityBitmapSet(&bitmap, 3u);
    MRT_PriorityBitmapSet(&bitmap, 7u);
    MRT_Priority highest = 0u;
    MRT_TEST_ASSERT_TRUE(MRT_PriorityBitmapFindHighest(&bitmap, &highest));
    MRT_TEST_ASSERT_EQ_U32(7u, highest);

    MRT_PriorityBitmapClear(&bitmap, 7u);
    MRT_TEST_ASSERT_TRUE(MRT_PriorityBitmapFindHighest(&bitmap, &highest));
    MRT_TEST_ASSERT_EQ_U32(3u, highest);

    MRT_PriorityBitmapClear(&bitmap, 3u);
    MRT_TEST_ASSERT_TRUE(!MRT_PriorityBitmapFindHighest(&bitmap, &highest));
    return 0;
}
```

- [ ] **Step 2: Run test to verify it fails**

Run:

```powershell
cmake -S . -B build -DMRT_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: build fails because `myrtos/mrt_priority.h` does not exist.

- [ ] **Step 3: Write minimal implementation**

Add `src/kernel/mrt_priority.c` to `myrtos_kernel` sources in `CMakeLists.txt`.

Create `include/myrtos/mrt_priority.h`:

```c
#ifndef MYRTOS_MRT_PRIORITY_H
#define MYRTOS_MRT_PRIORITY_H

#include "myrtos/mrt_config.h"
#include "myrtos/mrt_types.h"

typedef struct MRT_PriorityBitmap {
    uint32_t words[(MRT_CFG_MAX_PRIORITIES + 31u) / 32u];
} MRT_PriorityBitmap;

void MRT_PriorityBitmapInitialize(MRT_PriorityBitmap *bitmap);
void MRT_PriorityBitmapSet(MRT_PriorityBitmap *bitmap, MRT_Priority priority);
void MRT_PriorityBitmapClear(MRT_PriorityBitmap *bitmap, MRT_Priority priority);
bool MRT_PriorityBitmapFindHighest(const MRT_PriorityBitmap *bitmap, MRT_Priority *out_priority);

#endif
```

Create `src/kernel/mrt_priority.c`:

```c
#include "myrtos/mrt_priority.h"

void MRT_PriorityBitmapInitialize(MRT_PriorityBitmap *bitmap)
{
    for (size_t index = 0u; index < ((MRT_CFG_MAX_PRIORITIES + 31u) / 32u); index++) {
        bitmap->words[index] = 0u;
    }
}

void MRT_PriorityBitmapSet(MRT_PriorityBitmap *bitmap, MRT_Priority priority)
{
    bitmap->words[priority / 32u] |= (uint32_t)(1u << (priority % 32u));
}

void MRT_PriorityBitmapClear(MRT_PriorityBitmap *bitmap, MRT_Priority priority)
{
    bitmap->words[priority / 32u] &= (uint32_t)~(1u << (priority % 32u));
}

bool MRT_PriorityBitmapFindHighest(const MRT_PriorityBitmap *bitmap, MRT_Priority *out_priority)
{
    for (MRT_Priority priority = MRT_CFG_MAX_PRIORITIES; priority > 0u; priority--) {
        MRT_Priority candidate = priority - 1u;
        uint32_t mask = (uint32_t)(1u << (candidate % 32u));
        if ((bitmap->words[candidate / 32u] & mask) != 0u) {
            if (out_priority != 0) {
                *out_priority = candidate;
            }
            return true;
        }
    }
    return false;
}
```

- [ ] **Step 4: Run test to verify it passes**

Run:

```powershell
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: all current tests pass.

- [ ] **Step 5: Refactor for comment standard and invalid priority behavior**

Add Chinese function comments and internal comments. Add one failing test that setting or clearing `MRT_CFG_MAX_PRIORITIES` does not corrupt valid bits, then add bounds checks in implementation.

Run:

```powershell
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: all current tests pass.

- [ ] **Step 6: Commit**

Run:

```powershell
git add CMakeLists.txt tests/CMakeLists.txt tests/unit/test_priority_bitmap.c include/myrtos/mrt_priority.h src/kernel/mrt_priority.c
git commit -m "feat: add scheduler priority bitmap"
```

## Task 5: Mock Port

**Files:**
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`
- Create: `tests/port_mock/test_port_mock.c`
- Create: `include/myrtos/mrt_port.h`
- Create: `src/portable/mock/mrt_port_mock.c`

- [ ] **Step 1: Write the failing tests**

Add to `tests/CMakeLists.txt`:

```cmake
mrt_add_test(test_port_mock port_mock/test_port_mock.c)
```

Create `tests/port_mock/test_port_mock.c`:

```c
#include "mrt_test.h"
#include "myrtos/mrt_port.h"

int main(void)
{
    MRT_PortInitialize();
    MRT_TEST_ASSERT_TRUE(!MRT_PortIsInsideISR());
    MRT_TEST_ASSERT_TRUE(!MRT_PortMockWasYieldRequested());

    MRT_PortYield();
    MRT_TEST_ASSERT_TRUE(MRT_PortMockWasYieldRequested());

    MRT_IntState state = MRT_PortEnterCritical();
    MRT_TEST_ASSERT_TRUE(MRT_PortMockGetCriticalDepth() == 1u);
    MRT_PortExitCritical(state);
    MRT_TEST_ASSERT_TRUE(MRT_PortMockGetCriticalDepth() == 0u);
    return 0;
}
```

- [ ] **Step 2: Run test to verify it fails**

Run:

```powershell
cmake -S . -B build -DMRT_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: build fails because `myrtos/mrt_port.h` does not exist.

- [ ] **Step 3: Write minimal implementation**

Add `src/portable/mock/mrt_port_mock.c` to `myrtos_kernel` sources in `CMakeLists.txt`.

Create `include/myrtos/mrt_port.h`:

```c
#ifndef MYRTOS_MRT_PORT_H
#define MYRTOS_MRT_PORT_H

#include "myrtos/mrt_types.h"

void MRT_PortInitialize(void);
void MRT_PortStartFirstTask(void);
void MRT_PortYield(void);
void MRT_PortYieldFromISR(bool should_yield);
MRT_IntState MRT_PortEnterCritical(void);
void MRT_PortExitCritical(MRT_IntState state);
bool MRT_PortIsInsideISR(void);
void MRT_PortMockSetInsideISR(bool inside_isr);
bool MRT_PortMockWasYieldRequested(void);
uint32_t MRT_PortMockGetCriticalDepth(void);

#endif
```

Create `src/portable/mock/mrt_port_mock.c`:

```c
#include "myrtos/mrt_port.h"

static bool g_inside_isr;
static bool g_yield_requested;
static uint32_t g_critical_depth;

void MRT_PortInitialize(void)
{
    g_inside_isr = false;
    g_yield_requested = false;
    g_critical_depth = 0u;
}

void MRT_PortStartFirstTask(void)
{
    g_yield_requested = true;
}

void MRT_PortYield(void)
{
    g_yield_requested = true;
}

void MRT_PortYieldFromISR(bool should_yield)
{
    if (should_yield) {
        g_yield_requested = true;
    }
}

MRT_IntState MRT_PortEnterCritical(void)
{
    MRT_IntState previous_depth = (MRT_IntState)g_critical_depth;
    g_critical_depth++;
    return previous_depth;
}

void MRT_PortExitCritical(MRT_IntState state)
{
    g_critical_depth = (uint32_t)state;
}

bool MRT_PortIsInsideISR(void)
{
    return g_inside_isr;
}

void MRT_PortMockSetInsideISR(bool inside_isr)
{
    g_inside_isr = inside_isr;
}

bool MRT_PortMockWasYieldRequested(void)
{
    return g_yield_requested;
}

uint32_t MRT_PortMockGetCriticalDepth(void)
{
    return g_critical_depth;
}
```

- [ ] **Step 4: Run test to verify it passes**

Run:

```powershell
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: all current tests pass.

- [ ] **Step 5: Refactor for comment standard**

Add Chinese comments to every port function and internal state transition while keeping tests green.

Run:

```powershell
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: all current tests pass.

- [ ] **Step 6: Commit**

Run:

```powershell
git add CMakeLists.txt tests/CMakeLists.txt tests/port_mock/test_port_mock.c include/myrtos/mrt_port.h src/portable/mock/mrt_port_mock.c
git commit -m "feat: add host mock port"
```

## Task 6: Kernel Tick Shell

**Files:**
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`
- Create: `tests/unit/test_kernel_tick.c`
- Create: `include/myrtos/mrt_kernel.h`
- Create: `src/kernel/mrt_kernel.c`

- [ ] **Step 1: Write the failing tests**

Add to `tests/CMakeLists.txt`:

```cmake
mrt_add_test(test_kernel_tick unit/test_kernel_tick.c)
```

Create `tests/unit/test_kernel_tick.c`:

```c
#include "mrt_test.h"
#include "myrtos/mrt_kernel.h"

int main(void)
{
    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK, (unsigned)MRT_KernelInitialize());
    MRT_TEST_ASSERT_TRUE(!MRT_KernelIsRunning());
    MRT_TEST_ASSERT_EQ_U32(0u, MRT_KernelGetTick());

    MRT_KernelTick();
    MRT_TEST_ASSERT_EQ_U32(1u, MRT_KernelGetTick());

    MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_NOT_STARTED, (unsigned)MRT_KernelStart());
    return 0;
}
```

- [ ] **Step 2: Run test to verify it fails**

Run:

```powershell
cmake -S . -B build -DMRT_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: build fails because `myrtos/mrt_kernel.h` does not exist.

- [ ] **Step 3: Write minimal implementation**

Add `src/kernel/mrt_kernel.c` to `myrtos_kernel` sources in `CMakeLists.txt`.

Create `include/myrtos/mrt_kernel.h`:

```c
#ifndef MYRTOS_MRT_KERNEL_H
#define MYRTOS_MRT_KERNEL_H

#include "myrtos/mrt_types.h"

MRT_Result MRT_KernelInitialize(void);
MRT_Result MRT_KernelStart(void);
bool MRT_KernelIsRunning(void);
MRT_Tick MRT_KernelGetTick(void);
void MRT_KernelTick(void);
void MRT_KernelYield(void);
void MRT_KernelSuspendAll(void);
MRT_Result MRT_KernelResumeAll(void);

#endif
```

Create `src/kernel/mrt_kernel.c`:

```c
#include "myrtos/mrt_kernel.h"
#include "myrtos/mrt_port.h"

static MRT_Tick g_kernel_tick;
static bool g_kernel_running;
static uint32_t g_scheduler_suspend_depth;

MRT_Result MRT_KernelInitialize(void)
{
    g_kernel_tick = 0u;
    g_kernel_running = false;
    g_scheduler_suspend_depth = 0u;
    MRT_PortInitialize();
    return MRT_RESULT_OK;
}

MRT_Result MRT_KernelStart(void)
{
    if (!g_kernel_running) {
        return MRT_RESULT_NOT_STARTED;
    }
    return MRT_RESULT_OK;
}

bool MRT_KernelIsRunning(void)
{
    return g_kernel_running;
}

MRT_Tick MRT_KernelGetTick(void)
{
    return g_kernel_tick;
}

void MRT_KernelTick(void)
{
    g_kernel_tick++;
}

void MRT_KernelYield(void)
{
    MRT_PortYield();
}

void MRT_KernelSuspendAll(void)
{
    g_scheduler_suspend_depth++;
}

MRT_Result MRT_KernelResumeAll(void)
{
    if (g_scheduler_suspend_depth == 0u) {
        return MRT_RESULT_INVALID_CONTEXT;
    }
    g_scheduler_suspend_depth--;
    return MRT_RESULT_OK;
}
```

- [ ] **Step 4: Run test to verify it passes**

Run:

```powershell
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: all current tests pass.

- [ ] **Step 5: Refactor for comment standard**

Add Chinese Doxygen comments and internal step comments to kernel functions while preserving behavior.

Run:

```powershell
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: all current tests pass.

- [ ] **Step 6: Commit**

Run:

```powershell
git add CMakeLists.txt tests/CMakeLists.txt tests/unit/test_kernel_tick.c include/myrtos/mrt_kernel.h src/kernel/mrt_kernel.c
git commit -m "feat: add kernel tick shell"
```

## Task 7: Update Verification Evidence

**Files:**
- Modify: `docs/verification/requirements_traceability_matrix.md`
- Modify: `docs/verification/coupling_test_matrix.md`
- Modify: `progress.md`

- [ ] **Step 1: Run full verification**

Run:

```powershell
cmake -S . -B build -DMRT_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: all foundation tests pass.

- [ ] **Step 2: Update requirement matrix**

Update these rows:

- `R-002`: implementation evidence lists foundation files, status remains “部分实现”.
- `R-008`: test evidence lists CTest command, status remains “部分验证”.
- `R-009`: coupling evidence says foundation enables scheduler/port coupling rows, status remains “部分验证”.

- [ ] **Step 3: Update coupling matrix**

Update these rows:

- `C-028`: status becomes “端口 mock 基础已验证” after port mock tests pass.
- `C-029`: status becomes “端口 mock 基础已验证” after critical nesting tests pass.
- `C-030`: status remains “待实现” because DSP stack frame is not implemented in this plan.
- `C-031`: status remains “待实现” because DSP context switch is not implemented in this plan.

- [ ] **Step 4: Update progress log**

Record:

- Commands run.
- Number of tests.
- Commit hashes for foundation tasks.
- Remaining next plan: task scheduler core.

- [ ] **Step 5: Commit**

Run:

```powershell
git add docs/verification/requirements_traceability_matrix.md docs/verification/coupling_test_matrix.md progress.md
git commit -m "docs: record foundation verification evidence"
```

## Self-Review Checklist

- [ ] Each production API in this plan has a failing test before implementation.
- [ ] Each test command specifies expected failure or pass condition.
- [ ] Each task has exact files.
- [ ] Each implementation step has concrete code.
- [ ] No FreeRTOS API names are introduced as MyRTOS public symbols.
- [ ] Comment standard is introduced before committing each production module.
- [ ] Verification matrices are updated only after fresh test output exists.

