# MyRTOS Timers Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add MyRTOS software timers with static creation, start/stop/reset/change-period controls, one-shot/periodic expiry, pending function calls, Chinese comments, and verification evidence.

**Architecture:** Timers are static kernel objects managed by a timer list sorted by expiry tick. Host tests execute callbacks from the kernel tick path through a timer service shim; a true dedicated timer service task and asynchronous command queue are documented as a later scheduler/service enhancement while this plan still gives deterministic timer behavior and tests.

**Tech Stack:** C99, GCC-backed `tools/run_host_tests.py`, CMake metadata, host unit tests under `tests/unit/`, scheduler/tick coupling tests under `tests/coupling/`.

---

## File Structure

- `include/myrtos/mrt_timer.h`: timer control block, callback types, pending function type, and public timer APIs.
- `src/kernel/mrt_timer.c`: timer list, start/stop/reset/change-period logic, tick expiry processing, pending function FIFO.
- `src/kernel/mrt_kernel.c`: call timer tick processing after kernel tick increments and task tick processing runs.
- `CMakeLists.txt`, `tests/CMakeLists.txt`, `tools/run_host_tests.py`: include timer source and tests.
- `tests/unit/test_timer_create_static.c`: static creation, query APIs, parameter validation.
- `tests/unit/test_timer_control.c`: start, stop, reset, change period, active state, expiry tick recalculation.
- `tests/coupling/test_timer_tick_expiry.c`: one-shot expiry, periodic reload, tick ordering, callback arguments.
- `tests/unit/test_timer_pending_function.c`: pending function FIFO, capacity edge, service drain.
- `docs/verification/requirements_traceability_matrix.md`: update `R-002`, `R-008`, `R-009`.
- `docs/verification/coupling_test_matrix.md`: update `C-017`, `C-018`, and keep `C-019` pending for tickless.
- `progress.md`, `findings.md`: record baseline, RED/GREEN evidence, commit hashes, and remaining gaps.

## API Signatures

```c
typedef void (*MRT_TimerCallback)(MRT_TimerHandle timer, void *arg);
typedef void (*MRT_TimerPendingFunction)(void *arg, uint32_t value);

MRT_Result MRT_TimerCreateStatic(const char *name,
                                 MRT_Tick period_ticks,
                                 bool auto_reload,
                                 void *arg,
                                 MRT_TimerCallback callback,
                                 MRT_Timer *storage,
                                 MRT_TimerHandle *out_timer);
MRT_Result MRT_TimerStart(MRT_TimerHandle timer, MRT_Timeout timeout);
MRT_Result MRT_TimerStop(MRT_TimerHandle timer, MRT_Timeout timeout);
MRT_Result MRT_TimerReset(MRT_TimerHandle timer, MRT_Timeout timeout);
MRT_Result MRT_TimerChangePeriod(MRT_TimerHandle timer, MRT_Tick new_period_ticks, MRT_Timeout timeout);
MRT_Result MRT_TimerIsActive(MRT_TimerHandle timer, bool *out_active);
const char *MRT_TimerGetName(MRT_TimerHandle timer);
MRT_Result MRT_TimerPendFunctionCall(MRT_TimerPendingFunction function, void *arg, uint32_t value, MRT_Timeout timeout);
void MRT_TimerServiceRunPending(void);
```

## Task 1: Static Timer Creation

**Files:**
- Create: `include/myrtos/mrt_timer.h`
- Create: `src/kernel/mrt_timer.c`
- Create: `tests/unit/test_timer_create_static.c`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`
- Modify: `tools/run_host_tests.py`

Steps:

- [ ] Write failing tests for `MRT_TimerCreateStatic`, `MRT_TimerIsActive`, `MRT_TimerGetName`, null callback, null storage, null output handle, and zero period.
- [ ] Run `python tools\run_host_tests.py`; expected failure is missing `myrtos/mrt_timer.h`.
- [ ] Implement `MRT_Timer` with name, period, auto-reload flag, callback, callback arg, active flag, expiry tick, list node, and static-storage flag.
- [ ] Implement static create, active query, and name query.
- [ ] Add timer source to CMake and host runner.
- [ ] Run `python tools\run_host_tests.py`; expect 34 test targets pass.
- [ ] Commit `feat: add static timer creation`.

## Task 2: Timer Control APIs

**Files:**
- Create: `tests/unit/test_timer_control.c`
- Modify: `include/myrtos/mrt_timer.h`
- Modify: `src/kernel/mrt_timer.c`
- Modify: `tests/CMakeLists.txt`
- Modify: `tools/run_host_tests.py`

Steps:

- [ ] Write failing tests for start making a timer active, stop making it inactive, reset recalculating expiry from current tick, change period updating period and expiry for active timers, invalid timer handles, and zero new period rejection.
- [ ] Run `python tools\run_host_tests.py`; expected failure is missing control API implementation.
- [ ] Add a global active timer list sorted by expiry tick.
- [ ] Implement start/stop/reset/change-period using direct host service semantics and ignore timeout except for API compatibility.
- [ ] Run `python tools\run_host_tests.py`; expect 35 test targets pass.
- [ ] Commit `feat: add timer control APIs`.

## Task 3: Timer Tick Expiry

**Files:**
- Create: `tests/coupling/test_timer_tick_expiry.c`
- Modify: `src/kernel/mrt_kernel.c`
- Modify: `src/kernel/mrt_timer.c`
- Modify: `src/kernel/mrt_timer_internal.h`
- Modify: `tests/CMakeLists.txt`
- Modify: `tools/run_host_tests.py`

Steps:

- [ ] Write failing test where a one-shot timer started for 3 ticks fires exactly once when `MRT_KernelTick` reaches the expiry tick.
- [ ] Write failing test where an auto-reload timer with period 2 fires at tick 2 and tick 4 and remains active.
- [ ] Write failing test where two timers expiring on different ticks fire in expiry order.
- [ ] Run `python tools\run_host_tests.py`; expected failure is that kernel tick does not process timers.
- [ ] Add `MRT_TimerKernelInitialize` and `MRT_TimerKernelTick(MRT_Tick now)` internal functions.
- [ ] Call timer initialization from `MRT_KernelInitialize` and timer tick processing from `MRT_KernelTick`.
- [ ] Run `python tools\run_host_tests.py`; expect 36 test targets pass.
- [ ] Commit `feat: process software timers on tick`.

## Task 4: Pending Function Calls

**Files:**
- Create: `tests/unit/test_timer_pending_function.c`
- Modify: `include/myrtos/mrt_timer.h`
- Modify: `src/kernel/mrt_timer.c`
- Modify: `tests/CMakeLists.txt`
- Modify: `tools/run_host_tests.py`

Steps:

- [ ] Write failing tests for `MRT_TimerPendFunctionCall` FIFO ordering, argument/value delivery, null function rejection, pending queue full returning `MRT_RESULT_OBJECT_FULL`, and `MRT_TimerServiceRunPending` draining queued functions.
- [ ] Run `python tools\run_host_tests.py`; expected failure is missing pending function API implementation.
- [ ] Implement a fixed-size pending-function ring buffer controlled by `MRT_CFG_TIMER_PENDING_FUNCTION_QUEUE_LENGTH`.
- [ ] Add a config default for `MRT_CFG_TIMER_PENDING_FUNCTION_QUEUE_LENGTH` if it is not already defined.
- [ ] Run `python tools\run_host_tests.py`; expect 37 test targets pass.
- [ ] Commit `feat: add timer pending function calls`.

## Task 5: Timers Verification Matrix Update

**Files:**
- Modify: `docs/verification/requirements_traceability_matrix.md`
- Modify: `docs/verification/coupling_test_matrix.md`
- Modify: `progress.md`
- Modify: `findings.md`

Steps:

- [ ] Run `python tools\run_host_tests.py`; expect foundation, scheduler, queues, semaphores, mutexes, event groups, task notifications, and timers pass.
- [ ] Update `R-002`, `R-008`, and `R-009` evidence with timer source and tests.
- [ ] Mark `C-018` verified for host tick expiry callbacks.
- [ ] Mark `C-017` partial because this plan implements deterministic service-shim processing and pending function FIFO, while a dedicated timer service task with asynchronous command queue remains a later scheduler/service enhancement.
- [ ] Keep `C-019` pending because tickless idle is Plan 9.
- [ ] Record commit hashes and final test output in `progress.md`.
- [ ] Commit `docs: record timer verification evidence`.
