# MyRTOS Tickless/Trace/Assert Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add tickless idle, trace hooks, and assertion hooks with host tests and verification evidence.

**Architecture:** Tickless is a small kernel service that asks task/timer modules for next deadlines and delegates sleep to the port layer. Trace and assert are independent hook modules so production builds can keep them lightweight while tests observe events.

**Tech Stack:** C99, MyRTOS kernel modules, host GCC test runner, mock portable layer.

---

### Task 1: Tickless Expected Idle

**Files:**
- Create: `include/myrtos/mrt_tickless.h`
- Create: `src/kernel/mrt_tickless.c`
- Modify: `src/kernel/mrt_task_internal.h`
- Modify: `src/kernel/mrt_task.c`
- Modify: `src/kernel/mrt_timer_internal.h`
- Modify: `src/kernel/mrt_timer.c`
- Test: `tests/unit/test_tickless_expected_idle.c`

- [ ] **Step 1: Write failing test**
- [ ] **Step 2: Run `python tools\run_host_tests.py` and confirm missing API/header failure**
- [ ] **Step 3: Add next wake/expiry internal queries and expected-idle API**
- [ ] **Step 4: Re-run full host tests**
- [ ] **Step 5: Commit**

### Task 2: Tickless Sleep Compensation

**Files:**
- Modify: `include/myrtos/mrt_port.h`
- Modify: `src/portable/mock/mrt_port_mock.c`
- Modify: `include/myrtos/mrt_kernel.h`
- Modify: `src/kernel/mrt_kernel.c`
- Modify: `src/kernel/mrt_tickless.c`
- Test: `tests/coupling/test_tickless_timer_compensation.c`

- [ ] **Step 1: Write failing compensation test**
- [ ] **Step 2: Run full host tests and confirm missing port/tickless behavior failure**
- [ ] **Step 3: Add mock sleep controls, kernel tick compensation, and `MRT_TicklessEnterIdle`**
- [ ] **Step 4: Re-run full host tests**
- [ ] **Step 5: Commit**

### Task 3: Trace Hooks

**Files:**
- Create: `include/myrtos/mrt_trace.h`
- Create: `src/kernel/mrt_trace.c`
- Modify: `src/kernel/mrt_task.c`
- Modify: `src/kernel/mrt_queue.c`
- Test: `tests/coupling/test_trace_task_switch.c`
- Test: `tests/coupling/test_trace_queue.c`

- [ ] **Step 1: Write failing trace tests**
- [ ] **Step 2: Run full host tests and confirm trace API failure**
- [ ] **Step 3: Add trace sink and emit task switch / queue send / queue receive events**
- [ ] **Step 4: Re-run full host tests**
- [ ] **Step 5: Commit**

### Task 4: Assertion Hook

**Files:**
- Create: `include/myrtos/mrt_assert.h`
- Create: `src/kernel/mrt_assert.c`
- Test: `tests/unit/test_assert_hook.c`

- [ ] **Step 1: Write failing assertion hook test**
- [ ] **Step 2: Run full host tests and confirm missing assert API failure**
- [ ] **Step 3: Add hook setter, failure dispatcher, and `MRT_ASSERT` macro**
- [ ] **Step 4: Re-run full host tests**
- [ ] **Step 5: Commit**

### Task 5: Verification Evidence

**Files:**
- Modify: `docs/verification/requirements_traceability_matrix.md`
- Modify: `docs/verification/coupling_test_matrix.md`
- Modify: `docs/api/myrtos_api_catalog.md`
- Modify: `findings.md`
- Modify: `progress.md`

- [ ] **Step 1: Update API catalog and matrices**
- [ ] **Step 2: Run `python tools\run_host_tests.py`**
- [ ] **Step 3: Commit docs evidence**
- [ ] **Step 4: Push `feature/tickless-trace-hooks`**
