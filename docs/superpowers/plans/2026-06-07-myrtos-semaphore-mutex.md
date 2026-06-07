# MyRTOS Semaphore and Mutex Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add MyRTOS binary/counting semaphores plus normal/recursive mutexes with scheduler coupling, ISR give paths, priority inheritance, Chinese comments, and verification evidence.

**Architecture:** Semaphores and mutexes are separate kernel objects. Semaphores use a count and a priority-ordered waiting-taker list. Mutexes track owner, lock depth, waiting lockers, and owner priority inheritance through task-kernel internal helpers.

**Tech Stack:** C99, GCC-backed `tools/run_host_tests.py`, CMake metadata, host unit tests under `tests/unit/`, scheduler coupling tests under `tests/coupling/`.

---

## Task 1: Static Semaphore Creation

**Files:**
- Create: `include/myrtos/mrt_semaphore.h`
- Create: `src/kernel/mrt_semaphore.c`
- Create: `tests/unit/test_semaphore_create_static.c`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`
- Modify: `tools/run_host_tests.py`

Steps:

- [x] Write failing tests for `MRT_SemaphoreCreateBinaryStatic`, `MRT_SemaphoreCreateCountingStatic`, and `MRT_SemaphoreGetCount`.
- [x] Implement `MRT_Semaphore` with `max_count`, `count`, `waiting_takers`, and `static_storage`.
- [x] Reject null storage/output, zero max count, and initial count greater than max count.
- [x] Run `python tools\run_host_tests.py`; expect all tests pass.
- [x] Commit `feat: add static semaphore creation`.

## Task 2: Semaphore Take and Give

**Files:**
- Create: `tests/unit/test_semaphore_take_give.c`
- Modify: `include/myrtos/mrt_semaphore.h`
- Modify: `src/kernel/mrt_semaphore.c`
- Modify: `tests/CMakeLists.txt`
- Modify: `tools/run_host_tests.py`

Steps:

- [x] Write failing tests for nonblocking take success, empty take, give success, and give at max count.
- [x] Implement `MRT_SemaphoreTake` and `MRT_SemaphoreGive` for nonblocking paths.
- [x] Keep nonzero timeout returning `MRT_RESULT_TIMEOUT` until scheduler coupling is introduced in Task 3.
- [x] Run `python tools\run_host_tests.py`; expect all tests pass.
- [x] Commit `feat: add semaphore take give`.

## Task 3: Semaphore Timeout and Wake Coupling

**Files:**
- Create: `tests/coupling/test_semaphore_task_timeout.c`
- Create: `tests/coupling/test_semaphore_give_wakes_task.c`
- Modify: `src/kernel/mrt_task.c`
- Modify: `src/kernel/mrt_task_internal.h`
- Modify: `src/kernel/mrt_semaphore.c`
- Modify: `tests/CMakeLists.txt`
- Modify: `tools/run_host_tests.py`

Steps:

- [x] Write failing test where high-priority task blocks on empty semaphore with 3 tick timeout and wakes with timeout.
- [x] Write failing test where low-priority task gives semaphore and wakes high-priority taker.
- [x] Add semaphore wait reason to `MRT_TaskWaitReason`.
- [x] Use `MRT_TaskKernelBlockCurrentOnObject` and `MRT_TaskKernelWakeFirstObjectWaiter` for semaphore takers.
- [x] Run `python tools\run_host_tests.py`; expect all tests pass.
- [x] Commit `feat: add semaphore scheduler coupling`.

## Task 4: Semaphore ISR Give

**Files:**
- Create: `tests/unit/test_semaphore_isr.c`
- Modify: `include/myrtos/mrt_semaphore.h`
- Modify: `src/kernel/mrt_semaphore.c`
- Modify: `tests/CMakeLists.txt`
- Modify: `tools/run_host_tests.py`

Steps:

- [x] Write failing tests for `MRT_SemaphoreGiveFromISR`: no waiter keeps `should_yield=false`, waiter wakes with `should_yield=true`, full semaphore returns `MRT_RESULT_OBJECT_FULL`, task context returns `MRT_RESULT_INVALID_CONTEXT`.
- [x] Implement ISR give without blocking and without immediate task switch.
- [x] Run `python tools\run_host_tests.py`; expect all tests pass.
- [x] Commit `feat: add semaphore ISR give`.

## Task 5: Static Mutex Creation and Lock Ownership

**Files:**
- Create: `include/myrtos/mrt_mutex.h`
- Create: `src/kernel/mrt_mutex.c`
- Create: `tests/unit/test_mutex_create_lock.c`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`
- Modify: `tools/run_host_tests.py`

Steps:

- [ ] Write failing tests for `MRT_MutexCreateStatic`, first lock, owner query, unlock by owner, and unlock by non-owner.
- [ ] Implement `MRT_Mutex` with owner, lock count, recursive flag, waiting lockers, and static storage.
- [ ] Implement `MRT_MutexLock`, `MRT_MutexUnlock`, and `MRT_MutexGetOwner` for uncontended non-recursive paths.
- [ ] Run `python tools\run_host_tests.py`; expect all tests pass.
- [ ] Commit `feat: add mutex ownership`.

## Task 6: Mutex Priority Inheritance

**Files:**
- Create: `tests/coupling/test_mutex_priority_inheritance.c`
- Modify: `src/kernel/mrt_task.c`
- Modify: `src/kernel/mrt_task_internal.h`
- Modify: `src/kernel/mrt_mutex.c`
- Modify: `tests/CMakeLists.txt`
- Modify: `tools/run_host_tests.py`

Steps:

- [ ] Write failing test where low-priority owner is boosted when high-priority task waits on the mutex.
- [ ] Add task internal helper to raise current effective priority while preserving base priority.
- [ ] Add task internal helper to restore owner priority when mutex unlocks.
- [ ] On mutex contention, block waiter in priority-ordered wait list and boost owner.
- [ ] On unlock, transfer ownership to highest-priority waiter or clear owner and restore priority.
- [ ] Run `python tools\run_host_tests.py`; expect all tests pass.
- [ ] Commit `feat: add mutex priority inheritance`.

## Task 7: Recursive Mutex

**Files:**
- Create: `tests/unit/test_mutex_recursive.c`
- Modify: `include/myrtos/mrt_mutex.h`
- Modify: `src/kernel/mrt_mutex.c`
- Modify: `tests/CMakeLists.txt`
- Modify: `tools/run_host_tests.py`

Steps:

- [ ] Write failing tests for recursive create, same-owner relock, partial unlock, final unlock, and non-recursive relock returning `MRT_RESULT_OBJECT_BUSY`.
- [ ] Implement `MRT_MutexCreateRecursiveStatic` and recursive lock-depth behavior.
- [ ] Run `python tools\run_host_tests.py`; expect all tests pass.
- [ ] Commit `feat: add recursive mutex`.

## Task 8: Synchronization Verification Matrix Update

**Files:**
- Modify: `docs/verification/requirements_traceability_matrix.md`
- Modify: `docs/verification/coupling_test_matrix.md`
- Modify: `progress.md`
- Modify: `findings.md`

Steps:

- [ ] Run `python tools\run_host_tests.py`; expect foundation, scheduler, queues, semaphores, and mutex tests pass.
- [ ] Mark `C-008` through `C-013` and `C-027` according to implemented coverage.
- [ ] Update `R-002`, `R-008`, and `R-009` evidence with synchronization source and tests.
- [ ] Record commit hashes and final test output in `progress.md`.
- [ ] Commit `docs: record synchronization verification evidence`.
