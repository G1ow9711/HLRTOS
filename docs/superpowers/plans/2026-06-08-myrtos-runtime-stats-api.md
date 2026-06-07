# MyRTOS Runtime Stats API Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 实现 `MRT_StatsGetTaskRuntime`，关闭当前 API 目录中最后一个已知源码缺口，并用 host 测试验证任务运行 tick 统计。

**Architecture:** 当前 release 不引入芯片专用 cycle counter，先提供可移植的 tick 级运行统计：每次 `MRT_KernelTick()` 先把 1 个 runtime tick 累加到当前运行任务，再处理延时唤醒和定时器。任务控制块新增 `runtime_ticks` 字段，创建/内核初始化时清零；查询 API 只读该字段，已删除任务和空参数返回参数错误。后续 STM32/DSP 可把累加源替换成高分辨率计数器，但公共 API 语义保持不变。

**Tech Stack:** C99、MyRTOS task/kernel 模块、host GCC test runner `python tools\run_host_tests.py`、静态验证脚本 `tools\verify\*.py`。

---

### Task 1: Runtime Stats RED Test

**Files:**
- Create: `tests/coupling/test_runtime_stats.c`
- Modify: `tools/run_host_tests.py`
- Modify: `tests/CMakeLists.txt`

- [x] **Step 1: Write failing runtime accumulation test**

Create a test that:
- includes `myrtos/mrt_stats.h`;
- creates high and low priority tasks;
- starts with high task running;
- advances two ticks while high is running and expects high runtime to become 2;
- delays high for two ticks, advances one tick with low running and expects low runtime to become 1;
- advances another tick so high wakes and expects low runtime to become 2 while high remains 2;
- checks null task/null output/deleted task argument handling.

Representative assertions:

```c
uint64_t high_runtime = 0u;
MRT_TEST_ASSERT_EQ_U32((unsigned)MRT_RESULT_OK,
                       (unsigned)MRT_StatsGetTaskRuntime(high_task, &high_runtime));
MRT_TEST_ASSERT_EQ_U32(2u, (unsigned)high_runtime);
```

- [x] **Step 2: Register test target**

Add `test_runtime_stats` to `tools/run_host_tests.py` and `tests/CMakeLists.txt`.

- [x] **Step 3: Verify RED**

Run:

```powershell
python tools\run_host_tests.py
```

Expected: `test_runtime_stats` fails to build because `myrtos/mrt_stats.h` is missing or `MRT_StatsGetTaskRuntime` is undeclared.

### Task 2: Implement Runtime Stats API

**Files:**
- Create: `include/myrtos/mrt_stats.h`
- Create: `src/kernel/mrt_stats.c`
- Modify: `include/myrtos/mrt_task.h`
- Modify: `src/kernel/mrt_task_internal.h`
- Modify: `src/kernel/mrt_task.c`
- Modify: `src/kernel/mrt_kernel.c`
- Modify: `tools/run_host_tests.py`

- [x] **Step 1: Add public header**

Create `include/myrtos/mrt_stats.h` with Chinese Doxygen comment and:

```c
MRT_Result MRT_StatsGetTaskRuntime(MRT_TaskHandle task, uint64_t *out_runtime);
```

- [x] **Step 2: Add task runtime field**

Add `uint64_t runtime_ticks;` to `struct MRT_Task` with Chinese field comment. Initialize it to `0u` in `MRT_TaskCreateStatic`.

- [x] **Step 3: Add internal accumulator**

Add internal function:

```c
void MRT_TaskKernelAccumulateCurrentRuntime(MRT_Tick elapsed_ticks);
```

Implementation should no-op when no current task or `elapsed_ticks == 0`, otherwise add `elapsed_ticks` to `g_current_task->runtime_ticks`.

- [x] **Step 4: Accumulate on kernel tick**

In `MRT_KernelTick`, call `MRT_TaskKernelAccumulateCurrentRuntime(1u)` before `g_kernel_tick++`. This attributes the elapsed tick to the task that was running during the interval ending at the tick interrupt.

- [x] **Step 5: Implement query API**

`MRT_StatsGetTaskRuntime` validates `task` and `out_runtime`, rejects `MRT_TASK_STATE_DELETED`, writes `task->runtime_ticks`, and returns `MRT_RESULT_OK`.

- [x] **Step 6: Verify GREEN**

Run:

```powershell
python tools\run_host_tests.py
```

Expected: all host targets pass with `test_runtime_stats` included.

### Task 3: Documentation and Evidence

**Files:**
- Modify: `docs/manual/MyRTOS_Reference_Manual_zh.md`
- Modify: `docs/verification/requirements_traceability_matrix.md`
- Modify: `docs/verification/coupling_test_matrix.md`
- Modify: `docs/verification/final_verification_report.md`
- Modify: `task_plan.md`
- Modify: `findings.md`
- Modify: `progress.md`

- [x] **Step 1: Manual**

Update `MRT_StatsGetTaskRuntime` to state it is implemented as tick-level runtime accounting in the current portable host model, not a high-resolution CPU cycle counter.

- [x] **Step 2: Matrices and report**

Update evidence so the known source/API catalog gap count is zero for current catalog scope. Keep real STM32/DSP board smoke tests, mutex timeout rollback, held-mutex task deletion, and timer service task as remaining system-level work.

- [x] **Step 3: Static verification**

Run:

```powershell
python tools\verify\check_api_manual_coverage.py
python tools\verify\check_chinese_comments.py
python tools\verify\check_original_symbols.py
```

Expected: all static checks pass.

### Task 4: Commit and Push

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

- [ ] **Step 2: Commit**

```powershell
git add include src tests tools docs task_plan.md findings.md progress.md
git commit -m "feat: add runtime stats API"
```

- [ ] **Step 3: Push**

```powershell
git push -u origin feature/runtime-stats-api
```

---

## Self-Review

- Spec coverage: implements the final known catalog/source API gap, `MRT_StatsGetTaskRuntime`.
- Placeholder scan: no `TBD`, `TODO`, `implement later`, or vague error-handling placeholders.
- Type consistency: public signature matches API catalog and manual section, with `uint64_t *out_runtime`.
- Scope control: runtime stats are tick-level for this branch; high-resolution STM32/DSP counters remain a future port enhancement, not a public API change.
