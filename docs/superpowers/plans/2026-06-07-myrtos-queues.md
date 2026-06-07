# MyRTOS Queues Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add MyRTOS fixed-size queues with static creation, send/receive/peek/reset, ISR variants, and first scheduler coupling tests for queue blocking and timeout behavior.

**Architecture:** Queue code lives in `include/myrtos/mrt_queue.h` and `src/kernel/mrt_queue.c`. It uses caller-provided storage for static queues, circular copy semantics for items, and task wait lists that will later be reused by semaphores and mutexes.

**Tech Stack:** C99, GCC-backed `tools/run_host_tests.py`, CMake metadata kept current, host tests under `tests/unit/` and `tests/coupling/`.

---

## Task 1: Static Queue Creation

**Files:**
- Create: `include/myrtos/mrt_queue.h`
- Create: `src/kernel/mrt_queue.c`
- Create: `tests/unit/test_queue_create_static.c`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`
- Modify: `tools/run_host_tests.py`

Steps:

- [ ] Write failing test including `myrtos/mrt_queue.h`; expect missing header.
- [ ] Implement `MRT_Queue` and `MRT_QueueCreateStatic`.
- [ ] Reject null queue storage, null buffer when capacity > 0, zero item size, zero capacity, and null output handle.
- [ ] Verify created queue has zero messages and full free space.
- [ ] Run `python tools\run_host_tests.py`; expect all tests pass.
- [ ] Commit `feat: add static queue creation`.

## Task 2: Non-Blocking Send and Receive

**Files:**
- Create: `tests/unit/test_queue_send_receive.c`
- Modify: `include/myrtos/mrt_queue.h`
- Modify: `src/kernel/mrt_queue.c`
- Modify: `tests/CMakeLists.txt`
- Modify: `tools/run_host_tests.py`

Steps:

- [ ] Write failing tests for FIFO send/receive, full queue send, empty queue receive.
- [ ] Implement `MRT_QueueSend`, `MRT_QueueReceive`, `MRT_QueueMessagesWaiting`, and `MRT_QueueSpacesAvailable`.
- [ ] Keep timeout parameter accepted but only support `timeout == 0` in this task; nonzero timeout returns `MRT_RESULT_TIMEOUT` until blocking task is implemented.
- [ ] Run `python tools\run_host_tests.py`; expect all tests pass.
- [ ] Commit `feat: add nonblocking queue send receive`.

## Task 3: Peek, Send Front, Overwrite, Reset

**Files:**
- Create: `tests/unit/test_queue_variants.c`
- Modify: `include/myrtos/mrt_queue.h`
- Modify: `src/kernel/mrt_queue.c`
- Modify: `tests/CMakeLists.txt`
- Modify: `tools/run_host_tests.py`

Steps:

- [ ] Write failing tests for peek preserving item, send-front order, one-slot overwrite, multi-slot overwrite rejection, and reset clearing count.
- [ ] Implement `MRT_QueuePeek`, `MRT_QueueSendFront`, `MRT_QueueOverwrite`, and `MRT_QueueReset`.
- [ ] Run `python tools\run_host_tests.py`; expect all tests pass.
- [ ] Commit `feat: add queue variants`.

## Task 4: ISR Variants

**Files:**
- Create: `tests/unit/test_queue_isr.c`
- Modify: `include/myrtos/mrt_queue.h`
- Modify: `src/kernel/mrt_queue.c`
- Modify: `tests/CMakeLists.txt`
- Modify: `tools/run_host_tests.py`

Steps:

- [ ] Write failing tests for `MRT_QueueSendFromISR` and `MRT_QueueReceiveFromISR`.
- [ ] Implement ISR send/receive without blocking.
- [ ] Set `should_yield` only when a receiver/sender wait list is affected; until blocking waits exist, keep false.
- [ ] Run `python tools\run_host_tests.py`; expect all tests pass.
- [ ] Commit `feat: add queue ISR variants`.

## Task 5: Queue Blocking and Timeout Coupling

**Files:**
- Create: `tests/coupling/test_queue_task_timeout.c`
- Modify: `src/kernel/mrt_task.c`
- Modify: `src/kernel/mrt_task_internal.h`
- Modify: `include/myrtos/mrt_queue.h`
- Modify: `src/kernel/mrt_queue.c`
- Modify: `tests/CMakeLists.txt`
- Modify: `tools/run_host_tests.py`

Steps:

- [ ] Write failing test for current task receiving empty queue with 3 tick timeout, then waking with `MRT_RESULT_TIMEOUT`.
- [ ] Add task wait reason and wait result fields.
- [ ] Add queue receive wait list using `MRT_List`.
- [ ] On timeout, remove task from queue wait list and return timeout result through task wait result.
- [ ] Run `python tools\run_host_tests.py`; expect all tests pass.
- [ ] Commit `feat: add queue receive timeout coupling`.

## Task 6: Queue Send Wakes Receiver

**Files:**
- Create: `tests/coupling/test_queue_send_wakes_receiver.c`
- Modify: `src/kernel/mrt_queue.c`
- Modify: `src/kernel/mrt_task.c`
- Modify: `tests/CMakeLists.txt`
- Modify: `tools/run_host_tests.py`

Steps:

- [ ] Write failing test where high-priority task blocks on empty queue and low-priority task sends item.
- [ ] Wake highest-priority waiting receiver when send succeeds.
- [ ] Verify receiver becomes current task and receives item.
- [ ] Run `python tools\run_host_tests.py`; expect all tests pass.
- [ ] Commit `feat: wake receiver on queue send`.

## Task 7: Verification Matrix Update

**Files:**
- Modify: `docs/verification/requirements_traceability_matrix.md`
- Modify: `docs/verification/coupling_test_matrix.md`
- Modify: `progress.md`

Steps:

- [ ] Run `python tools\run_host_tests.py`; expect queue, scheduler, and foundation tests pass.
- [ ] Mark `C-004`, `C-005`, `C-006`, and `C-007` according to implemented coverage.
- [ ] Update `R-002`, `R-008`, and `R-009` evidence with queue source and tests.
- [ ] Record commit hashes and test output in `progress.md`.
- [ ] Commit `docs: record queue verification evidence`.

