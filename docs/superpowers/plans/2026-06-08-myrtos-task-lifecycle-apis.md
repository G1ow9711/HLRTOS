# MyRTOS Task Lifecycle APIs Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 补齐任务生命周期相关的预览 API，使 API 目录中的任务创建、删除、挂起、恢复、周期延时、优先级调整和栈水位查询均有 C 实现、中文注释、手册和测试证据。

**Architecture:** 复用现有 `MRT_TaskCreateStatic()`、ready list、delay list、对象等待链表和 `MRT_Malloc()`/`MRT_Free()`，不引入 FreeRTOS 源码结构。动态任务使用一个堆块保存 TCB 和栈；删除/挂起会清理 ready/delay/object 链表，防止陈旧等待节点再次唤醒；FromISR 恢复只置 ready 并通过 `should_yield` 把切换请求交给端口层。

**Tech Stack:** C99、MyRTOS kernel、host GCC test runner `python tools\run_host_tests.py`、静态脚本 `tools\verify\*.py`。

---

### Task 1: Add Lifecycle RED Tests

**Files:**
- Create: `tests/sim/test_task_lifecycle.c`
- Create: `tests/coupling/test_task_dynamic_allocation.c`
- Modify: `tools/run_host_tests.py`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Write the failing lifecycle test**

`tests/sim/test_task_lifecycle.c` must cover:
- static task deletion marks a task deleted and removes it from scheduling;
- suspending and resuming a ready task updates state and scheduling;
- `MRT_TaskResumeFromISR()` rejects task context and sets `should_yield` in ISR context;
- `MRT_TaskDelayUntil()` updates the previous wake tick and blocks the current task until the absolute period;
- `MRT_TaskSetPriority()` updates base/effective priority and affects scheduling;
- `MRT_TaskGetStackHighWaterMark()` reports the configured host-model stack capacity.

- [ ] **Step 2: Write the failing dynamic task test**

`tests/coupling/test_task_dynamic_allocation.c` must cover:
- `MRT_TaskCreate()` allocates a heap-backed TCB+stack, creates a ready task, and `MRT_TaskDelete()` releases heap space;
- allocation failure returns `MRT_RESULT_NO_MEMORY`, clears the output handle, and leaves heap free size unchanged;
- null arguments and invalid priorities are rejected.

- [ ] **Step 3: Register tests**

Add both targets to `tools/run_host_tests.py` and `tests/CMakeLists.txt`.

- [ ] **Step 4: Verify RED**

Run:

```powershell
python tools\run_host_tests.py
```

Expected: build failure caused by missing declarations for `MRT_TaskCreate`, `MRT_TaskDelete`, `MRT_TaskSuspend`, `MRT_TaskResume`, `MRT_TaskResumeFromISR`, `MRT_TaskDelayUntil`, `MRT_TaskSetPriority`, and `MRT_TaskGetStackHighWaterMark`.

### Task 2: Implement Task Lifecycle APIs

**Files:**
- Modify: `include/myrtos/mrt_task.h`
- Modify: `src/kernel/mrt_task.c`

- [ ] **Step 1: Add public prototypes and Chinese API comments**

Declare the eight missing task APIs with `@brief`、`@param`、`@return`、`@example` comments.

- [ ] **Step 2: Add internal unlink helper**

Add a local helper in `src/kernel/mrt_task.c` that removes a task from `state_node` and `wait_node`, clears wait metadata, and updates ready bitmap when the node belonged to a ready list.

- [ ] **Step 3: Add dynamic create/delete**

`MRT_TaskCreate()` validates parameters, checks dynamic allocation support, allocates one aligned heap block, calls `MRT_TaskCreateStatic()`, marks the task dynamic, and frees on failure. `MRT_TaskDelete()` accepts static and dynamic tasks: it marks both deleted, unlinks them from scheduler lists, clears current task if needed, and frees only dynamic storage.

- [ ] **Step 4: Add suspend/resume APIs**

`MRT_TaskSuspend()` moves ready/running/blocked tasks to `MRT_TASK_STATE_SUSPENDED` and removes pending wait links. `MRT_TaskResume()` moves suspended tasks to ready and reschedules in task context. `MRT_TaskResumeFromISR()` requires `MRT_PortIsInsideISR()`, moves suspended tasks to ready without immediate switch, and reports `should_yield=true` when a task was resumed.

- [ ] **Step 5: Add periodic delay, priority set, and stack water mark**

`MRT_TaskDelayUntil()` updates `*previous_wake_tick += period_ticks`, delays until that absolute tick with wrap-safe comparison, and yields on already-due periods. `MRT_TaskSetPriority()` updates base/effective priority through the existing priority helper and reschedules if needed. `MRT_TaskGetStackHighWaterMark()` returns `stack_words` in the current host model because stack painting/consumption is not yet simulated.

- [ ] **Step 6: Verify GREEN**

Run:

```powershell
python tools\run_host_tests.py
```

Expected: all host targets pass, with the new count increased by 2.

### Task 3: Update Documentation and Evidence

**Files:**
- Modify: `docs/manual/MyRTOS_Reference_Manual_zh.md`
- Modify: `docs/verification/requirements_traceability_matrix.md`
- Modify: `docs/verification/coupling_test_matrix.md`
- Modify: `docs/verification/final_verification_report.md`
- Modify: `findings.md`
- Modify: `progress.md`

- [ ] **Step 1: Manual**

Update the task API sections so the manual no longer describes the eight lifecycle APIs as preview-only. Keep the existing official-style section structure and include detailed STM32/DSP porting notes already requested by the user.

- [ ] **Step 2: Matrices**

Record the new task lifecycle tests under task, memory, ISR resume, and scheduler coupling rows. Leave real board smoke rows as pending until hardware evidence exists.

- [ ] **Step 3: Report**

Update the final report’s gap list from 21 missing catalog APIs to 13 remaining non-task dynamic/statistics APIs.

- [ ] **Step 4: Static verification**

Run:

```powershell
python tools\verify\check_api_manual_coverage.py
python tools\verify\check_chinese_comments.py
python tools\verify\check_original_symbols.py
```

Expected: manual coverage, Chinese comment coverage, and originality scan pass.

### Task 4: Commit and Push

**Files:**
- All changed files from Tasks 1-3.

- [ ] **Step 1: Full verification**

Run:

```powershell
python tools\run_host_tests.py
python tools\verify\check_api_manual_coverage.py
python tools\verify\check_chinese_comments.py
python tools\verify\check_original_symbols.py
git diff --check
```

Expected: tests/static scripts pass. `git diff --check` may report repository-wide CRLF warnings, but must not report newly introduced real whitespace errors.

- [ ] **Step 2: Commit**

```powershell
git add include src tests tools docs task_plan.md findings.md progress.md
git commit -m "feat: add task lifecycle APIs"
```

- [ ] **Step 3: Push**

```powershell
git push -u origin feature/task-lifecycle-apis
```

---

## Self-Review

- Spec coverage: covers all eight task APIs identified as declared in the API catalog but missing from C headers/source.
- Placeholder scan: no `TBD`, `TODO`, or `implement later` placeholders are present.
- Type consistency: uses existing `MRT_TaskHandle`, `MRT_TaskEntry`, `MRT_Tick`, `MRT_Result`, `MRT_StackType`, and heap APIs.
- Scope control: dynamic semaphore/mutex/event/timer/stream/message/statistics APIs remain separate follow-up branches.
