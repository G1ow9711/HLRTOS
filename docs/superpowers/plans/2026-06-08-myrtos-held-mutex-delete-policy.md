# MyRTOS Held Mutex Delete Policy Plan

## Objective
Close coupling item `C-012`: deleting a task that still owns a mutex must leave mutex state and waiting tasks in a defined, tested state.

## Policy
MyRTOS rejects deletion of a task that owns one or more mutexes. The API returns `MRT_RESULT_OBJECT_BUSY`; the task remains scheduled, mutex ownership remains unchanged, and waiting tasks remain queued.

## TDD Evidence
| Step | Command | Expected | Actual | Status |
|------|---------|----------|--------|--------|
| RED | `python tools\run_host_tests.py` | New held-mutex delete test fails because delete currently succeeds | `test_mutex_task_delete_policy.c:96: expected 5 got 0` | Pass |
| GREEN | `python tools\run_host_tests.py` | All host tests pass after delete guard | `[summary] 67 test target(s) passed` | Pass |

## Implementation Notes
- Mutex objects are registered in an internal list when created and unregistered when dynamic mutexes are deleted.
- `MRT_KernelInitialize` resets the mutex registry so repeated host tests and system restarts do not retain stale objects.
- `MRT_TaskDelete` calls `MRT_MutexKernelCanDeleteTask` before unlinking or freeing a task.
