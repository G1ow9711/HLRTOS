# MyRTOS Events and Task Notifications Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add MyRTOS event groups and task notifications with wait-any, wait-all, clear-on-exit, task/ISR wake coupling, Chinese comments, and verification matrix evidence.

**Architecture:** Event groups are standalone static kernel objects that hold a bit set and a priority-ordered waiter list. Task notifications are per-task lightweight state fields stored in `MRT_Task`, with notify actions and wait/take helpers using scheduler blocking and direct task wake helpers.

**Tech Stack:** C99, GCC-backed `tools/run_host_tests.py`, CMake metadata, host unit tests under `tests/unit/`, scheduler coupling tests under `tests/coupling/`.

---

## File Structure

- `include/myrtos/mrt_event_group.h`: public event group control block, constants, and APIs.
- `src/kernel/mrt_event_group.c`: event group bit operations, wait matching, task wake, ISR set.
- `include/myrtos/mrt_task.h`: task wait reason additions, event wait metadata, notification fields, notify action enum, notification APIs.
- `src/kernel/mrt_task.c`: initialize new task fields and implement notification APIs.
- `src/kernel/mrt_task_internal.h`: scheduler internal helpers for blocking current task without an object list and waking a specific task.
- `tests/unit/test_event_group_create_bits.c`: static create, set, clear, get, argument validation.
- `tests/unit/test_event_group_wait_immediate.c`: wait-any, wait-all, clear-on-exit, no-match nonblocking behavior.
- `tests/coupling/test_event_group_task_timeout.c`: event wait timeout coupling and wait-list cleanup.
- `tests/coupling/test_event_group_set_wakes_tasks.c`: set bits wakes one or more matching waiters and applies clear-on-exit after matching.
- `tests/unit/test_event_group_isr.c`: ISR set bits, task-context rejection, deferred yield flag.
- `tests/unit/test_task_notify_actions.c`: set bits, increment, overwrite, no-overwrite, state/value clear.
- `tests/coupling/test_task_notify_wait_take.c`: notification wait/take blocking, clear-on-entry, clear-on-exit, count take behavior.
- `tests/unit/test_task_notify_isr.c`: ISR notification action paths and `should_yield`.
- `CMakeLists.txt`, `tests/CMakeLists.txt`, `tools/run_host_tests.py`: include new sources and tests.
- `docs/verification/requirements_traceability_matrix.md`: update `R-002`, `R-008`, and `R-009`.
- `docs/verification/coupling_test_matrix.md`: update `C-014`, `C-015`, `C-016`, and related illegal-context evidence.
- `progress.md`, `findings.md`: record test output, design notes, commit hashes, and remaining gaps.

## API Signatures

```c
MRT_Result MRT_EventGroupCreateStatic(MRT_EventGroup *storage, MRT_EventGroupHandle *out_group);
MRT_Result MRT_EventGroupSetBits(MRT_EventGroupHandle group, MRT_EventBits bits_to_set, MRT_EventBits *out_bits);
MRT_Result MRT_EventGroupClearBits(MRT_EventGroupHandle group, MRT_EventBits bits_to_clear, MRT_EventBits *out_bits);
MRT_Result MRT_EventGroupWaitBits(MRT_EventGroupHandle group,
                                  MRT_EventBits bits_to_wait,
                                  bool wait_all,
                                  bool clear_on_exit,
                                  MRT_Timeout timeout,
                                  MRT_EventBits *out_bits);
MRT_Result MRT_EventGroupSetBitsFromISR(MRT_EventGroupHandle group,
                                        MRT_EventBits bits_to_set,
                                        bool *should_yield);
MRT_EventBits MRT_EventGroupGetBits(MRT_EventGroupHandle group);

typedef enum MRT_NotifyAction {
    MRT_NOTIFY_SET_BITS = 0,
    MRT_NOTIFY_INCREMENT,
    MRT_NOTIFY_OVERWRITE,
    MRT_NOTIFY_NO_OVERWRITE
} MRT_NotifyAction;

MRT_Result MRT_TaskNotify(MRT_TaskHandle task, MRT_NotifyValue value, MRT_NotifyAction action);
MRT_Result MRT_TaskNotifyFromISR(MRT_TaskHandle task, MRT_NotifyValue value, MRT_NotifyAction action, bool *should_yield);
MRT_Result MRT_TaskNotifyWait(MRT_NotifyValue clear_on_entry,
                              MRT_NotifyValue clear_on_exit,
                              MRT_Timeout timeout,
                              MRT_NotifyValue *out_value);
MRT_Result MRT_TaskNotifyTake(bool clear_count_on_exit, MRT_Timeout timeout, MRT_NotifyValue *out_count);
MRT_Result MRT_TaskNotifyStateClear(MRT_TaskHandle task);
MRT_Result MRT_TaskNotifyValueClear(MRT_TaskHandle task, MRT_NotifyValue bits_to_clear);
```

## Task 1: Static Event Group and Bit Operations

**Files:**
- Create: `include/myrtos/mrt_event_group.h`
- Create: `src/kernel/mrt_event_group.c`
- Create: `tests/unit/test_event_group_create_bits.c`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`
- Modify: `tools/run_host_tests.py`

Steps:

- [ ] Write failing tests for `MRT_EventGroupCreateStatic`, `MRT_EventGroupGetBits`, `MRT_EventGroupSetBits`, `MRT_EventGroupClearBits`, null storage/output, zero `bits_to_set`, and zero `bits_to_clear`.
- [ ] Run `python tools\run_host_tests.py`; expected failure is missing `myrtos/mrt_event_group.h`.
- [ ] Implement `MRT_EventGroup` with `bits`, `waiting_tasks`, and `static_storage`.
- [ ] Implement set/clear/get for task context without blocking or wake logic.
- [ ] Add event group source to CMake and host runner.
- [ ] Run `python tools\run_host_tests.py`; expect 26 test targets pass.
- [ ] Commit `feat: add static event group bits`.

## Task 2: Event Group Immediate Wait

**Files:**
- Create: `tests/unit/test_event_group_wait_immediate.c`
- Modify: `include/myrtos/mrt_event_group.h`
- Modify: `src/kernel/mrt_event_group.c`
- Modify: `tests/CMakeLists.txt`
- Modify: `tools/run_host_tests.py`

Steps:

- [ ] Write failing tests for wait-any immediate success, wait-all immediate success, clear-on-exit clearing only matched bits, no clear preserving bits, zero wait mask rejection, and no-match `timeout == 0` returning `MRT_RESULT_OBJECT_EMPTY`.
- [ ] Run `python tools\run_host_tests.py`; expected failure is missing `MRT_EventGroupWaitBits`.
- [ ] Implement bit matching helper: wait-all requires every requested bit, wait-any requires at least one requested bit.
- [ ] Implement immediate wait path and `out_bits` writeback.
- [ ] Run `python tools\run_host_tests.py`; expect 27 test targets pass.
- [ ] Commit `feat: add event group immediate wait`.

## Task 3: Event Group Timeout Coupling

**Files:**
- Create: `tests/coupling/test_event_group_task_timeout.c`
- Modify: `include/myrtos/mrt_task.h`
- Modify: `src/kernel/mrt_task.c`
- Modify: `src/kernel/mrt_task_internal.h`
- Modify: `src/kernel/mrt_event_group.c`
- Modify: `tests/CMakeLists.txt`
- Modify: `tools/run_host_tests.py`

Steps:

- [ ] Write failing test where a high-priority task waits for event bits with 3 tick timeout, blocks, low-priority task runs, tick expiry removes the waiter from both delay list and event wait list, and event bits remain unchanged.
- [ ] Run `python tools\run_host_tests.py`; expected failure is that event wait does not block current task.
- [ ] Add `MRT_TASK_WAIT_REASON_EVENT_BITS`.
- [ ] Add event wait metadata to `MRT_Task`: requested bits, wait-all flag, clear-on-exit flag, and matched bits.
- [ ] Initialize event wait metadata in `MRT_TaskCreateStatic`.
- [ ] Reuse `MRT_TaskKernelBlockCurrentOnObject` for event wait timeout.
- [ ] Run `python tools\run_host_tests.py`; expect 28 test targets pass.
- [ ] Commit `feat: add event group timeout wait`.

## Task 4: Event Group Set Wakes Matching Tasks

**Files:**
- Create: `tests/coupling/test_event_group_set_wakes_tasks.c`
- Modify: `src/kernel/mrt_task.c`
- Modify: `src/kernel/mrt_task_internal.h`
- Modify: `src/kernel/mrt_event_group.c`
- Modify: `tests/CMakeLists.txt`
- Modify: `tools/run_host_tests.py`

Steps:

- [ ] Write failing test where two tasks wait on the same event group, `MRT_EventGroupSetBits` wakes every waiter whose condition matches the post-set bit snapshot, and the highest-priority awakened task runs.
- [ ] Write failing test where clear-on-exit clears the union of matched bits after all eligible waiters are selected.
- [ ] Run `python tools\run_host_tests.py`; expected failure is that set bits changes only the stored bits and does not wake waiters.
- [ ] Add `MRT_TaskKernelWakeTask(MRT_TaskHandle task, MRT_Result wait_result, bool switch_now)` to remove a task from object and delay lists, mark it ready, and optionally reschedule.
- [ ] Implement event group waiter iteration using the post-set snapshot so one clear-on-exit waiter cannot hide bits from a later matching waiter in the same set operation.
- [ ] Run `python tools\run_host_tests.py`; expect 29 test targets pass.
- [ ] Commit `feat: wake event group waiters`.

## Task 5: Event Group ISR Set

**Files:**
- Create: `tests/unit/test_event_group_isr.c`
- Modify: `include/myrtos/mrt_event_group.h`
- Modify: `src/kernel/mrt_event_group.c`
- Modify: `tests/CMakeLists.txt`
- Modify: `tools/run_host_tests.py`

Steps:

- [ ] Write failing tests for `MRT_EventGroupSetBitsFromISR`: no waiter keeps `should_yield=false`, matching waiter sets `should_yield=true`, task context returns `MRT_RESULT_INVALID_CONTEXT`, null group returns `MRT_RESULT_INVALID_ARGUMENT`, zero bits returns `MRT_RESULT_INVALID_ARGUMENT`.
- [ ] Run `python tools\run_host_tests.py`; expected failure is missing ISR API declaration.
- [ ] Implement ISR set without immediate task switch and with deferred yield flag.
- [ ] Run `python tools\run_host_tests.py`; expect 30 test targets pass.
- [ ] Commit `feat: add event group ISR set`.

## Task 6: Task Notification Actions

**Files:**
- Create: `tests/unit/test_task_notify_actions.c`
- Modify: `include/myrtos/mrt_task.h`
- Modify: `src/kernel/mrt_task.c`
- Modify: `tests/CMakeLists.txt`
- Modify: `tools/run_host_tests.py`

Steps:

- [ ] Write failing tests for `MRT_TaskNotify` set-bits, increment, overwrite, no-overwrite success on empty notification, no-overwrite busy on pending notification, `MRT_TaskNotifyStateClear`, `MRT_TaskNotifyValueClear`, null task, and invalid action.
- [ ] Run `python tools\run_host_tests.py`; expected failure is missing notify API declarations.
- [ ] Add `MRT_NotifyAction` and per-task notification fields: `notify_value` and `notify_pending`.
- [ ] Initialize notification fields in `MRT_TaskCreateStatic`.
- [ ] Implement task-context notify actions and state/value clear helpers.
- [ ] Run `python tools\run_host_tests.py`; expect 31 test targets pass.
- [ ] Commit `feat: add task notification actions`.

## Task 7: Task Notification Wait and Take Coupling

**Files:**
- Create: `tests/coupling/test_task_notify_wait_take.c`
- Modify: `include/myrtos/mrt_task.h`
- Modify: `src/kernel/mrt_task.c`
- Modify: `src/kernel/mrt_task_internal.h`
- Modify: `tests/CMakeLists.txt`
- Modify: `tools/run_host_tests.py`

Steps:

- [ ] Write failing tests where `MRT_TaskNotifyWait` immediately returns pending value, applies clear-on-entry before waiting, applies clear-on-exit before returning, and returns `MRT_RESULT_OBJECT_EMPTY` for no pending notification with `timeout == 0`.
- [ ] Write failing tests where `MRT_TaskNotifyTake` returns count, either clears count to zero or decrements by one, and blocks with timeout when count is zero.
- [ ] Write failing coupling test where a high-priority task blocks in notify wait, a low-priority task notifies it, and the high-priority task becomes current.
- [ ] Run `python tools\run_host_tests.py`; expected failure is missing wait/take implementations.
- [ ] Add `MRT_TASK_WAIT_REASON_NOTIFY_WAIT` and `MRT_TaskKernelBlockCurrent(ticks, wait_reason, wait_result)` for per-task notification waits that do not use an object wait list.
- [ ] Implement notify wait/take immediate and blocking paths.
- [ ] Make `MRT_TaskNotify` wake blocked tasks waiting for notification and reschedule in task context.
- [ ] Run `python tools\run_host_tests.py`; expect 32 test targets pass.
- [ ] Commit `feat: add task notification waits`.

## Task 8: Task Notification ISR

**Files:**
- Create: `tests/unit/test_task_notify_isr.c`
- Modify: `include/myrtos/mrt_task.h`
- Modify: `src/kernel/mrt_task.c`
- Modify: `tests/CMakeLists.txt`
- Modify: `tools/run_host_tests.py`

Steps:

- [ ] Write failing tests for `MRT_TaskNotifyFromISR`: no blocked waiter keeps `should_yield=false`, blocked waiter sets `should_yield=true` and becomes ready without immediate switch, task context returns `MRT_RESULT_INVALID_CONTEXT`, null task returns `MRT_RESULT_INVALID_ARGUMENT`, no-overwrite busy returns `MRT_RESULT_OBJECT_BUSY`.
- [ ] Run `python tools\run_host_tests.py`; expected failure is missing ISR notify API declaration.
- [ ] Implement ISR notification using the same action helper as task-context notify.
- [ ] Wake notification waiters with `switch_now=false` and set `should_yield` when a task is made ready.
- [ ] Run `python tools\run_host_tests.py`; expect 33 test targets pass.
- [ ] Commit `feat: add task notification ISR`.

## Task 9: Events and Notifications Verification Matrix Update

**Files:**
- Modify: `docs/verification/requirements_traceability_matrix.md`
- Modify: `docs/verification/coupling_test_matrix.md`
- Modify: `progress.md`
- Modify: `findings.md`

Steps:

- [ ] Run `python tools\run_host_tests.py`; expect foundation, scheduler, queues, semaphores, mutexes, event groups, and task notifications pass.
- [ ] Update `R-002`, `R-008`, and `R-009` evidence with event group and task notification source/tests.
- [ ] Mark `C-014`, `C-015`, and `C-016` according to implemented coverage.
- [ ] Record commit hashes and final test output in `progress.md`.
- [ ] Commit `docs: record events notifications verification evidence`.
