# MyRTOS Mutex Timeout Rollback Plan

## Objective
Close coupling item `C-011`: when a high-priority task waits on a mutex and times out, the mutex owner must no longer keep the inherited priority from that timed-out waiter.

## Scope
- Add a coupling test for mutex wait timeout priority rollback.
- Implement minimal kernel cleanup on object timeout for mutex waiters.
- Update verification/manual evidence.

## TDD Evidence
| Step | Command | Expected | Actual | Status |
|------|---------|----------|--------|--------|
| RED attempt 1 | `python tools\run_host_tests.py` | New mutex timeout test fails for missing rollback | Failed earlier due incorrect test setup: high-priority task ran before owner locked mutex | Fixed test setup |
| RED attempt 2 | `python tools\run_host_tests.py` | New mutex timeout test fails because owner remains inherited priority 5 | `expected 1 got 5` in `test_mutex_timeout_rollback.c` | Pass |
| GREEN | `python tools\run_host_tests.py` | All tests pass after rollback implementation | `[summary] 66 test target(s) passed` | Pass |

## Implementation Notes
- `MRT_TaskKernelTick` stores timeout wait reason and wait-list pointer before removing the waiter node.
- `MRT_MutexKernelHandleLockTimeout` recalculates owner effective priority from remaining mutex waiters.
- If no remaining waiter has priority above owner base priority, owner returns to base priority.

## Remaining Out Of Scope
- Full multi-mutex inheritance stacking remains a later policy enhancement if the project adds held-mutex ownership tracking.
- Held-mutex task deletion policy remains `C-012`.
