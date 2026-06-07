# MyRTOS Dynamic Object APIs Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 补齐 API 目录中除运行统计外的动态同步对象、定时器、流缓冲和消息缓冲创建/删除接口，使剩余实现缺口从 13 个降到 1 个。

**Architecture:** 复用各模块现有静态创建函数作为唯一初始化路径，动态 API 只负责参数预校验、堆布局、失败清理和 `static_storage=false` 标记。控制块单独分配的对象通过 `MRT_Free(handle)` 释放；带数据区的流/消息缓冲使用一个对齐堆块保存控制块和字节存储；删除静态对象返回 `MRT_RESULT_OBJECT_BUSY`，删除动态对象先清理模块状态再归还堆内存。

**Tech Stack:** C99、MyRTOS heap、host GCC test runner `python tools\run_host_tests.py`、静态验证脚本 `tools\verify\*.py`。

---

### Task 1: Dynamic Sync Object RED Tests

**Files:**
- Create: `tests/coupling/test_sync_dynamic_allocation.c`
- Modify: `tools/run_host_tests.py`
- Modify: `tests/CMakeLists.txt`

- [x] **Step 1: Write failing semaphore dynamic tests**

Add coverage for `MRT_SemaphoreCreateBinary`、`MRT_SemaphoreCreateCounting`、`MRT_SemaphoreDelete`:
- dynamic binary create/take/give/delete restores heap free size;
- dynamic counting create validates `max_count` and `initial_count`;
- allocation failure returns `MRT_RESULT_NO_MEMORY`, clears output handle, and preserves heap free size;
- deleting a static semaphore returns `MRT_RESULT_OBJECT_BUSY`;
- null output/delete arguments return `MRT_RESULT_INVALID_ARGUMENT`.

- [x] **Step 2: Write failing mutex dynamic tests**

Add coverage for `MRT_MutexCreate`、`MRT_MutexCreateRecursive`、`MRT_MutexDelete`:
- dynamic normal mutex can lock/unlock/delete and restores heap free size;
- dynamic recursive mutex can lock twice/unlock twice/delete;
- allocation failure clears output handle and preserves heap free size;
- deleting a locked dynamic mutex returns `MRT_RESULT_OBJECT_BUSY`;
- deleting a static mutex returns `MRT_RESULT_OBJECT_BUSY`;
- null output/delete arguments return `MRT_RESULT_INVALID_ARGUMENT`.

- [x] **Step 3: Verify RED**

Run:

```powershell
python tools\run_host_tests.py
```

Expected: `test_sync_dynamic_allocation` build fails with implicit declarations for missing semaphore and mutex dynamic APIs.

### Task 2: Dynamic Event/Timer RED Tests

**Files:**
- Create: `tests/coupling/test_event_timer_dynamic_allocation.c`
- Modify: `tools/run_host_tests.py`
- Modify: `tests/CMakeLists.txt`

- [x] **Step 1: Write failing event group dynamic tests**

Add coverage for `MRT_EventGroupCreate` and `MRT_EventGroupDelete`:
- dynamic event group can set/get/delete and restores heap free size;
- deleting a group with waiting tasks returns `MRT_RESULT_OBJECT_BUSY`;
- allocation failure clears output handle and preserves heap free size;
- deleting a static group returns `MRT_RESULT_OBJECT_BUSY`;
- null output/delete arguments return `MRT_RESULT_INVALID_ARGUMENT`.

- [x] **Step 2: Write failing timer dynamic tests**

Add coverage for `MRT_TimerCreate` and `MRT_TimerDelete`:
- dynamic timer can start/stop/delete and restores heap free size;
- deleting an active timer stops it before freeing;
- invalid period/callback/out args are rejected;
- allocation failure clears output handle and preserves heap free size;
- deleting a static timer returns `MRT_RESULT_OBJECT_BUSY`.

- [x] **Step 3: Verify RED**

Run:

```powershell
python tools\run_host_tests.py
```

Expected: missing declarations for event group and timer dynamic APIs.

### Task 3: Dynamic Buffer RED Tests

**Files:**
- Create: `tests/coupling/test_buffer_dynamic_allocation.c`
- Modify: `tools/run_host_tests.py`
- Modify: `tests/CMakeLists.txt`

- [x] **Step 1: Write failing stream buffer dynamic tests**

Add coverage for `MRT_StreamBufferCreate`:
- dynamic stream buffer can send/receive bytes;
- allocation failure clears output handle and preserves heap free size;
- invalid capacity, trigger level, and output args are rejected.

- [x] **Step 2: Write failing message buffer dynamic tests**

Add coverage for `MRT_MessageBufferCreate`:
- dynamic message buffer can send/receive one complete message;
- allocation failure clears output handle and preserves heap free size;
- invalid capacity and output args are rejected.

- [x] **Step 3: Verify RED**

Run:

```powershell
python tools\run_host_tests.py
```

Expected: missing declarations for stream/message buffer dynamic APIs.

### Task 4: Implement Dynamic APIs

**Files:**
- Modify: `include/myrtos/mrt_semaphore.h`
- Modify: `src/kernel/mrt_semaphore.c`
- Modify: `include/myrtos/mrt_mutex.h`
- Modify: `src/kernel/mrt_mutex.c`
- Modify: `include/myrtos/mrt_event_group.h`
- Modify: `src/kernel/mrt_event_group.c`
- Modify: `include/myrtos/mrt_timer.h`
- Modify: `src/kernel/mrt_timer.c`
- Modify: `include/myrtos/mrt_stream_buffer.h`
- Modify: `src/kernel/mrt_stream_buffer.c`
- Modify: `include/myrtos/mrt_message_buffer.h`
- Modify: `src/kernel/mrt_message_buffer.c`

- [x] **Step 1: Add public prototypes and Chinese comments**

Each new API must include `@brief`、`@param`、`@return`、`@example` in headers and source definitions.

- [x] **Step 2: Implement control-block-only dynamic objects**

For semaphore, mutex, event group, and timer, allocate `sizeof(MRT_*)`, call the matching static create function, set `static_storage=false`, and free on initialization failure.

- [x] **Step 3: Implement buffer dynamic objects**

For stream/message buffers, allocate one block containing aligned control block plus byte storage. Call the matching static create function with the buffer pointer, set `static_storage=false`, and free on initialization failure.

- [x] **Step 4: Implement delete APIs**

Semaphore/event delete must reject objects with waiting tasks. Mutex delete must reject locked objects or waiting lockers. Timer delete must stop active timers before freeing. Static objects are not heap-owned and return `MRT_RESULT_OBJECT_BUSY`.

- [x] **Step 5: Verify GREEN**

Run:

```powershell
python tools\run_host_tests.py
```

Expected: all host targets pass with three new dynamic-object test targets.

### Task 5: Documentation and Evidence

**Files:**
- Modify: `docs/manual/MyRTOS_Reference_Manual_zh.md`
- Modify: `docs/verification/requirements_traceability_matrix.md`
- Modify: `docs/verification/coupling_test_matrix.md`
- Modify: `docs/verification/final_verification_report.md`
- Modify: `task_plan.md`
- Modify: `findings.md`
- Modify: `progress.md`

- [x] **Step 1: Manual**

Update dynamic object sections to describe exact prototypes, static-delete rejection, waiters/locked-object delete behavior, and buffer heap layout.

- [x] **Step 2: Matrices and report**

Update evidence to show dynamic object API gap closure. Remaining known source/catalog gap should be only `MRT_StatsGetTaskRuntime`.

- [x] **Step 3: Static verification**

Run:

```powershell
python tools\verify\check_api_manual_coverage.py
python tools\verify\check_chinese_comments.py
python tools\verify\check_original_symbols.py
```

Expected: all static checks pass.

### Task 6: Commit and Push

- [x] **Step 1: Full verification**

Run:

```powershell
python tools\run_host_tests.py
python tools\verify\check_api_manual_coverage.py
python tools\verify\check_chinese_comments.py
python tools\verify\check_original_symbols.py
git diff --check
```

Expected: host/static checks pass. `git diff --check` may print LF-to-CRLF warnings, but `git diff --cached --check` must be clean before commit.

- [x] **Step 2: Commit**

```powershell
git add include src tests tools docs task_plan.md findings.md progress.md
git commit -m "feat: add dynamic object APIs"
```

- [ ] **Step 3: Push**

```powershell
git push -u origin feature/dynamic-object-apis
```

---

## Self-Review

- Spec coverage: covers 12 dynamic object APIs listed in the catalog as missing after task lifecycle closure.
- Placeholder scan: no `TBD`, `TODO`, or `implement later` placeholders are present.
- Type consistency: signatures match current manual/catalog entries, with `MRT_Result` return values and existing handle types.
- Scope control: `MRT_StatsGetTaskRuntime` remains a separate runtime-statistics branch because it requires scheduler accounting fields.
