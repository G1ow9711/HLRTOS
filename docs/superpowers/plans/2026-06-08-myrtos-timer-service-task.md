# MyRTOS Timer Service Command Queue Plan

## Goal

Close the software timer service-task gap by replacing direct timer control/callback behavior with an original FIFO service command queue. The public host-verifiable service entry is `MRT_TimerServiceRunPending`; real STM32/DSP ports can bind that entry to a dedicated timer service task.

## Scope

- Convert `MRT_TimerStart`, `MRT_TimerStop`, `MRT_TimerReset`, and `MRT_TimerChangePeriod` into asynchronous service commands.
- Convert tick expiry into queued callback events so `MRT_TimerKernelTick` never runs user callbacks directly.
- Keep pending function calls in the same FIFO service queue.
- Purge queued commands/events for a dynamic timer before delete frees its control block.
- Update tests, manual, and verification matrices to describe service-queue semantics.

## TDD Evidence

| Step | Command | Expected | Actual |
|------|---------|----------|--------|
| RED | `python tools\run_host_tests.py` after adding `test_timer_service_task` | Existing direct-start behavior fails new async-start assertion | `test_timer_service_task.c:84: assertion failed: !active` |
| First GREEN check | `python tools\run_host_tests.py` after service queue implementation | Compile succeeds; old tests reveal stale direct semantics | 4 old timer/tickless targets failed |
| GREEN | `python tools\run_host_tests.py` after updating affected tests | All host tests pass | `[summary] 68 test target(s) passed` |

## Implementation Checklist

- [x] Add `MRT_CFG_TIMER_COMMAND_QUEUE_LENGTH`.
- [x] Add internal `MRT_TimerServiceEntry` queue.
- [x] Queue timer start/stop/reset/change-period commands.
- [x] Queue expiry callback events from tick path.
- [x] Run callbacks and pending functions only from `MRT_TimerServiceRunPending`.
- [x] Purge deleted timer commands/events before heap release.
- [x] Add coupling test `test_timer_service_task`.
- [x] Update legacy timer/tickless tests to drain the service queue explicitly.
- [x] Update Chinese manual, API catalog, verification matrices, final report, and planning logs.

## Remaining Notes

- `MRT_TimerServiceRunPending` is the deterministic service entry for host tests and portable integrations.
- A later board integration may create an actual always-running scheduler task whose body repeatedly calls this service entry.
- Real STM32/DSP smoke evidence remains outside this branch.
