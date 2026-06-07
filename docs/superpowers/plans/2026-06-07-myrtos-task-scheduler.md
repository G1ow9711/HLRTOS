# MyRTOS Task Scheduler Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add MyRTOS static task creation and host-simulated scheduler behavior: highest-priority selection, round-robin yield, task delay, delay wakeup, and tick overflow-safe wake checks.

**Architecture:** Keep scheduler logic in `src/kernel/mrt_task.c` and public task API in `include/myrtos/mrt_task.h`. Existing `mrt_kernel.c` owns kernel-level entry points and delegates ready/delay work to scheduler internals through non-public functions.

**Tech Stack:** C99, GCC-backed `tools/run_host_tests.py`, CMake files kept for standard environments, host simulation tests under `tests/sim/`.

---

## Task 1: Static Task Creation

**Files:**
- Create: `include/myrtos/mrt_task.h`
- Create: `src/kernel/mrt_task.c`
- Create: `tests/sim/test_task_create_static.c`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`
- Modify: `tools/run_host_tests.py`

Steps:

- [ ] Write failing test including `myrtos/mrt_task.h`; expect missing header.
- [ ] Implement `MRT_Task`, `MRT_TaskState`, `MRT_TaskEntry`, and `MRT_TaskCreateStatic`.
- [ ] Validate argument errors: null entry, null stack, zero stack words, null storage, out-of-range priority.
- [ ] Verify created task starts in ready state and returns a handle.
- [ ] Run `python tools\run_host_tests.py`; expect all tests pass.
- [ ] Commit `feat: add static task creation`.

## Task 2: Kernel Start Selects Highest Ready Task

**Files:**
- Create: `tests/sim/test_scheduler_start.c`
- Modify: `src/kernel/mrt_kernel.c`
- Modify: `src/kernel/mrt_task.c`
- Modify: `include/myrtos/mrt_task.h`
- Modify: `tests/CMakeLists.txt`
- Modify: `tools/run_host_tests.py`

Steps:

- [ ] Write failing test creating low and high priority tasks, then calling `MRT_KernelStart`.
- [ ] Expect current task to be the high priority task and kernel running state true.
- [ ] Implement scheduler ready lists using `MRT_List` per priority and `MRT_PriorityBitmap`.
- [ ] Implement `MRT_TaskGetCurrent`, `MRT_TaskGetState`, `MRT_TaskGetPriority`.
- [ ] Update `MRT_KernelStart` to select highest ready task and call `MRT_PortStartFirstTask`.
- [ ] Run `python tools\run_host_tests.py`; expect all tests pass.
- [ ] Commit `feat: start scheduler with highest priority task`.

## Task 3: Round-Robin Yield

**Files:**
- Create: `tests/sim/test_scheduler_round_robin.c`
- Modify: `src/kernel/mrt_kernel.c`
- Modify: `src/kernel/mrt_task.c`
- Modify: `tests/CMakeLists.txt`
- Modify: `tools/run_host_tests.py`

Steps:

- [ ] Write failing test creating two same-priority tasks, starting scheduler, then calling `MRT_KernelYield`.
- [ ] Expect current task rotates from first task to second task.
- [ ] Implement ready-list tail rotation for current task when same-priority peers exist.
- [ ] Keep higher-priority task selection dominant after each yield.
- [ ] Run `python tools\run_host_tests.py`; expect all tests pass.
- [ ] Commit `feat: add round-robin scheduler yield`.

## Task 4: Task Delay and Wakeup

**Files:**
- Create: `tests/sim/test_task_delay.c`
- Modify: `src/kernel/mrt_kernel.c`
- Modify: `src/kernel/mrt_task.c`
- Modify: `include/myrtos/mrt_task.h`
- Modify: `tests/CMakeLists.txt`
- Modify: `tools/run_host_tests.py`

Steps:

- [ ] Write failing test where current high-priority task delays for 3 ticks and lower-priority task runs.
- [ ] Expect delayed task state blocked until third tick, then ready/current again.
- [ ] Implement `MRT_TaskDelay`.
- [ ] Add ordered delayed list keyed by wake tick.
- [ ] On `MRT_KernelTick`, move due delayed tasks back to ready list and reschedule.
- [ ] Run `python tools\run_host_tests.py`; expect all tests pass.
- [ ] Commit `feat: add task delay wakeup`.

## Task 5: Tick Overflow Simulation

**Files:**
- Create: `tests/sim/test_task_delay_overflow.c`
- Modify: `include/myrtos/mrt_kernel.h`
- Modify: `src/kernel/mrt_kernel.c`
- Modify: `src/kernel/mrt_task.c`
- Modify: `tests/CMakeLists.txt`
- Modify: `tools/run_host_tests.py`

Steps:

- [ ] Write failing test enabling `MRT_TESTING` and setting tick near `UINT32_MAX`.
- [ ] Delay current task across tick wrap and verify wake after correct number of ticks.
- [ ] Add `MRT_KernelTestSetTick` under `#if MRT_TESTING`.
- [ ] Implement due check with signed tick difference: due when `(int32_t)(now - wake_tick) >= 0`.
- [ ] Run `python tools\run_host_tests.py`; expect all tests pass.
- [ ] Commit `test: verify scheduler delay across tick overflow`.

## Task 6: Verification Matrix Update

**Files:**
- Modify: `docs/verification/requirements_traceability_matrix.md`
- Modify: `docs/verification/coupling_test_matrix.md`
- Modify: `progress.md`

Steps:

- [ ] Run `python tools\run_host_tests.py`; expect scheduler and foundation tests pass.
- [ ] Mark `C-001`, `C-002`, and `C-003` as verified with exact test names.
- [ ] Update `R-002`, `R-008`, and `R-009` evidence with scheduler source and tests.
- [ ] Record commit hashes and test output in `progress.md`.
- [ ] Commit `docs: record scheduler verification evidence`.

