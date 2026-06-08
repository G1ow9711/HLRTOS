# Progress Log

## Session: 2026-06-08 Hardware Smoke Evidence Gate
- RED/GREEN: `python tests\static\test_hardware_smoke_evidence_checker.py` first failed because the checker stopped after missing required fields and did not report present failing fields such as `Evidence-Status: FAIL`.
- Fixed `tools/verify/check_hardware_smoke_evidence.py` to keep validating target, PASS fields, date, runtime, assert count, and heap minimum after missing-field collection.
- GREEN: `python tests\static\test_hardware_smoke_evidence_checker.py` now passes.
- Added `docs/verification/hardware_smoke/README.md`, `stm32_board_smoke.template.md`, and `dsp_board_smoke.template.md`.
- Updated manual sections 5.6 and 6.6 with real-board evidence archival steps; updated section 7.4 with `python tools\verify\check_hardware_smoke_evidence.py`.
- Updated `docs/verification/test_suite_plan.md`, `docs/verification/final_verification_report.md`, `docs/verification/completion_audit.md`, and `docs/verification/requirements_traceability_matrix.md` to reference the hardware evidence gate.
- Fresh verification passed:
  - `python tools\run_host_tests.py` -> `[summary] 69 test target(s) passed`
  - `python tools\verify\check_api_manual_coverage.py` -> `[manual-coverage] 135 API section(s) covered`
  - `python tools\verify\check_api_catalog_prototypes.py` -> `[api-catalog] 135 API prototype(s) aligned`
  - `python tools\verify\check_chinese_comments.py` -> `[chinese-comments] include/src/examples/tests function comments covered`
  - `python tools\verify\check_original_symbols.py` -> `[original-symbols] no banned FreeRTOS-style public symbols found`
  - `python tools\verify\check_embedded_smoke_projects.py` -> `[embedded-smoke] STM32 cross build and DSP model smoke passed`
  - `git diff --check` -> exit 0 with expected CRLF warnings only
- Expected remaining failure: `python tools\verify\check_hardware_smoke_evidence.py` reports missing `stm32_board_smoke.md` and `dsp_board_smoke.md` because no real STM32/DSP board logs have been produced in this environment.
- Committed verified repo-side smoke/evidence work as `59b154f` with message `feat: add embedded smoke verification`.
- Pushed branch `feature/embedded-smoke-projects` to `origin`; GitHub PR URL suggested by remote: `https://github.com/G1ow9711/HLRTOS/pull/new/feature/embedded-smoke-projects`.

## Session: 2026-06-08 Hardware Smoke Capture Checklist
- Added `docs/verification/hardware_smoke/collection_checklist.md` with real STM32 and DSP log collection steps.
- Expanded `docs/verification/hardware_smoke/README.md` so users collect raw UART/trace logs before filling final evidence files.
- Updated manual sections 5.6 and 6.6 to point at the collection checklist before `stm32_board_smoke.md` and `dsp_board_smoke.md` are filled.
- Updated manual section 7.4 to list the full repo-side verification commands plus the hardware evidence gate.
- Updated `docs/verification/test_suite_plan.md`, `docs/verification/final_verification_report.md`, `docs/verification/completion_audit.md`, and `docs/verification/requirements_traceability_matrix.md` to reference the collection checklist.
- Verification after checklist update:
  - `python tools\verify\check_api_manual_coverage.py` -> `[manual-coverage] 135 API section(s) covered`
  - `python tools\verify\check_api_catalog_prototypes.py` -> `[api-catalog] 135 API prototype(s) aligned`
  - `python tools\verify\check_embedded_smoke_projects.py` -> `[embedded-smoke] STM32 cross build and DSP model smoke passed`
  - `python tests\static\test_hardware_smoke_evidence_checker.py` -> pass
  - `git diff --check` -> exit 0 with expected CRLF warnings only
  - `python tools\verify\check_hardware_smoke_evidence.py` -> expected failure because real `stm32_board_smoke.md` and `dsp_board_smoke.md` are still absent
- Committed checklist/doc updates as `2ea379b` with message `docs: add hardware smoke capture checklist`.

## Session: 2026-06-08 Unified Release Verification Runner
- Added `tools/verify/run_release_verification.py` as the single release gate entrypoint.
- Added `tests/static/test_release_verification_runner.py` to verify the default step list and the hardware-required append behavior.
- Default release run passed: `python tools\verify\run_release_verification.py` -> `[release] 8 step(s) passed`.
- Hardware-required release run failed only at the real board log gate: `python tools\verify\run_release_verification.py --require-hardware` -> `[release] 1 step(s) failed` with missing `stm32_board_smoke.md` and `dsp_board_smoke.md`.
- Manual/test-suite/final-report/completion-audit/requirements docs now point at the unified release entrypoint.
- Final report summary now records the default pass and hardware-required failure, and `hardware_smoke/README.md` now points users at the unified release runner first.
- Committed unified release runner work as `3f169e6` with message `feat: add unified release verification runner`, then pushed to `origin/feature/embedded-smoke-projects`.

## Session: 2026-06-08 Embedded Smoke Projects
- Added dynamic `MRT_StreamBufferDelete` and `MRT_MessageBufferDelete` lifecycle APIs with heap-release, null-argument, and static-object rejection coverage.
- RED: direct compile of `test_buffer_dynamic_allocation` failed on implicit declarations for `MRT_StreamBufferDelete` and `MRT_MessageBufferDelete`.
- GREEN: direct compile/run of `test_buffer_dynamic_allocation` passed after adding delete APIs; full verification later stayed green.
- Added `docs/verification/completion_audit.md` to map the original user objective to current evidence and the remaining real-hardware smoke gap.
- Added detailed STM32/DSP migration subsections to the manual: `移植前准备`、`工程分层`、`关键接入顺序`、`首次联调`、`板级验收`.
- Extended `tools/verify/check_api_manual_coverage.py` to require the new migration terms so the manual detail stays locked.
- Added MPU helper evidence to the verification set: `test_port_stm32_mpu` now has its own coupling row `C-035`, and STM32 port plan/docs mention it explicitly.
- Resumed worktree `F:\My_RTOS\.worktrees\embedded-smoke-projects` on branch `feature/embedded-smoke-projects`.
- Updated `task_plan.md` and added branch plan `docs/superpowers/plans/2026-06-08-myrtos-embedded-smoke-projects.md`.
- Discovery: `examples/`, `examples/stm32/`, and `examples/dsp/` are missing.
- Discovery: `arm-none-eabi-gcc` is installed and can support STM32 cross-compile smoke verification.
- Discovery: `tiarmclang` is not installed, so DSP real hardware/toolchain verification remains pending; use a host model and document the boundary.
- Next step: add a RED smoke verification script that fails because the example projects are missing.
- Added `tools/verify/check_embedded_smoke_projects.py`.
- RED run: `python tools\verify\check_embedded_smoke_projects.py` failed with 11 missing required paths under `examples/stm32` and `examples/dsp`, proving the smoke project gap.
- Added `examples/stm32/` with README, minimal startup, linker script, runtime stubs, smoke main, and STM32 port smoke implementation.
- Added `examples/dsp/` with README, DSP port host model, and smoke main.
- GREEN run: `python tools\verify\check_embedded_smoke_projects.py` passed; STM32 ELF cross-build succeeded and DSP model executable compiled and ran successfully.
- Expanded `tools\verify\check_chinese_comments.py` to include `examples/` and `tests/`; after documenting `tests/unit/test_types_contract.c` `main`, run passed with `[chinese-comments] include/src/examples/tests function comments covered`.
- Expanded `tools\verify\check_original_symbols.py` to include `examples/` and `tests/`; run passed with `[original-symbols] no banned FreeRTOS-style public symbols found`.
- Updated manual STM32/DSP porting chapters with smoke project layout, build commands, handler wiring, acceptance evidence, and real hardware boundaries.
- Marked Phase 4 and Phase 7 complete in `task_plan.md` after all code/example/comment deliverables reached the documented target.
- Updated requirement matrix, coupling matrix, test suite plan, and final verification report with embedded smoke evidence.
- Error logged: first attempt to patch `docs/verification/test_suite_plan.md` used coupling-matrix-only context and failed; resolved by re-reading the file and applying a narrower patch.
- Latest verification passed:
  - `python tools\verify\check_api_manual_coverage.py` -> `[manual-coverage] 135 API section(s) covered`
  - `python tools\verify\check_api_catalog_prototypes.py` -> `[api-catalog] 135 API prototype(s) aligned`
  - `python tools\verify\check_chinese_comments.py` -> `[chinese-comments] include/src/examples/tests function comments covered`
  - `python tools\verify\check_original_symbols.py` -> `[original-symbols] no banned FreeRTOS-style public symbols found`
  - `python tools\verify\check_embedded_smoke_projects.py` -> `[embedded-smoke] STM32 cross build and DSP model smoke passed`
  - `python tools\run_host_tests.py` -> `[summary] 69 test target(s) passed`
  - `git diff --check` -> exit 0 with expected CRLF warnings only

## Session: 2026-06-07

### Held Mutex Delete Policy Branch
- Created worktree `F:\My_RTOS\.worktrees\held-mutex-delete-policy` on branch `feature/held-mutex-delete-policy` from `feature/mutex-timeout-rollback`.
- Added `tests/coupling/test_mutex_task_delete_policy.c` and registered it in CMake plus `tools/run_host_tests.py`.
- RED run: `python tools\run_host_tests.py` failed in `test_mutex_task_delete_policy.c` because deleting a task that still owned a mutex returned `MRT_RESULT_OK` instead of `MRT_RESULT_OBJECT_BUSY`.
- Implemented mutex object registry, kernel registry reset, dynamic mutex unregister, and `MRT_TaskDelete` guard through `MRT_MutexKernelCanDeleteTask`.
- GREEN run: `python tools\run_host_tests.py` reported `[summary] 67 test target(s) passed`.
- Updated manual, coupling matrix, requirement matrix, final report, and branch plan for `C-012`.

### Timer Service Command Queue Branch
- Continued worktree `F:\My_RTOS\.worktrees\timer-service-task` on branch `feature/timer-service-task` from `feature/held-mutex-delete-policy`.
- Added `tests/coupling/test_timer_service_task.c` and registered it in CMake plus `tools/run_host_tests.py`.
- RED evidence from this branch: `test_timer_service_task.c:84` failed because `MRT_TimerStart` activated the timer before the service task drained the start command.
- Implemented one FIFO timer service queue for start/stop/reset/change-period commands, expiry callback events, and pending functions.
- Changed `MRT_TimerKernelTick` to queue expiry events instead of running user callbacks directly.
- Changed dynamic timer delete to purge queued commands/events for that timer before freeing memory.
- First full GREEN attempt after implementation exposed 4 stale tests still assuming direct timer activation/callback behavior.
- Updated `test_timer_control`, `test_timer_tick_expiry`, `test_tickless_expected_idle`, and `test_tickless_timer_compensation` to drain `MRT_TimerServiceRunPending()` before expecting queued commands or callbacks to take effect.
- GREEN run: `python tools\run_host_tests.py` reported `[summary] 68 test target(s) passed`.
- Updated `mrt_timer.h` comments, API catalog, Chinese manual timer sections, configuration macro appendix, coupling matrix, requirement matrix, final verification report, and added branch plan `docs/superpowers/plans/2026-06-08-myrtos-timer-service-task.md`.

### Mutex Timeout Rollback Branch Resume
- Created worktree `F:\My_RTOS\.worktrees\mutex-timeout-rollback` on branch `feature/mutex-timeout-rollback` from `feature/runtime-stats-api`.
- Added coupling test `tests/coupling/test_mutex_timeout_rollback.c` and registered it in `tests/CMakeLists.txt` and `tools/run_host_tests.py`.
- RED run: first failed because test setup started the higher-priority waiter before the owner held the mutex; fixed by creating the waiter after the owner locked the mutex.
- RED run 2: `test_mutex_timeout_rollback` failed with `expected 1 got 5`, proving owner priority did not roll back after timeout.
- Implemented mutex timeout rollback by preserving timeout wait reason in `MRT_TaskKernelTick`, adding internal mutex cleanup helper, and recalculating owner effective priority from remaining waiters.
- Updated manual and verification matrices to mark `C-011` verified.
- Final verification: `python tools\run_host_tests.py` reported `[summary] 66 test target(s) passed`.

### Runtime Stats Branch Resume Verification
- Recovered active worktree `F:\My_RTOS\.worktrees\runtime-stats-api` on branch `feature/runtime-stats-api`.
- Static verification passed: `check_api_manual_coverage.py` covered 125 API sections, `check_chinese_comments.py` covered include/src function comments, and `check_original_symbols.py` found no banned FreeRTOS-style public symbols.
- First parallel `python tools\run_host_tests.py` attempt timed out after 124 seconds while static checks ran concurrently; reran host tests alone with a longer timeout.
- Host verification passed: `python tools\run_host_tests.py` reported `[summary] 65 test target(s) passed`.

### Tickless/Trace/Assert Branch
- Created design document `docs/superpowers/specs/2026-06-07-myrtos-tickless-trace-assert-design.md`.
- Created implementation plan `docs/superpowers/plans/2026-06-07-myrtos-tickless-trace-hooks.md`.
- Re-read kernel, task, timer, port, queue, config, and host test runner boundaries.
- Key discovery: task and timer modules already keep ordered deadline lists, so tickless can be implemented by exporting minimal internal next-deadline queries.
- Tickless Task 1 RED: `python tools\run_host_tests.py` failed on `test_tickless_expected_idle` because `myrtos/mrt_tickless.h` was missing.
- Tickless Task 1 GREEN: added `MRT_TicklessGetExpectedIdleTicks`, task next-wake query, timer next-expiry query, CMake/runner entries, and `tests/unit/test_tickless_expected_idle.c`.
- Tickless Task 1 verification: `python tools\run_host_tests.py` passed 51 test targets.
- Tickless Task 2 RED: `python tools\run_host_tests.py` failed on `test_tickless_timer_compensation` because `MRT_TicklessEnterIdle` and mock port sleep APIs were undeclared.
- Tickless Task 2 first GREEN attempt compiled but crashed with access violation. GDB showed `MRT_TimerKernelInitialize -> MRT_ListRemove`; root cause was the test leaving a stack-allocated active timer in the global active list before the next `MRT_KernelInitialize`.
- Tickless Task 2 fix: stopped the still-active timer at the end of the max-sleep-limit test case.
- Tickless Task 2 GREEN: added `MRT_TicklessEnterIdle`, `MRT_PortSuppressTicksAndSleep`, mock sleep controls/observers, and `tests/coupling/test_tickless_timer_compensation.c`.
- Tickless Task 2 verification: `python tools\run_host_tests.py` passed 52 test targets.
- Trace Task 3 RED: `python tools\run_host_tests.py` failed on `test_trace_task_switch` and `test_trace_queue` because `myrtos/mrt_trace.h` was missing.
- Trace Task 3 GREEN: added `MRT_TraceSetSink`, `MRT_TraceEmit`, trace event structures, task-switch hooks, and queue send/receive hooks.
- Trace Task 3 verification: `python tools\run_host_tests.py` passed 54 test targets.
- Assert Task 4 RED: `python tools\run_host_tests.py` failed on `test_assert_hook` because `myrtos/mrt_assert.h` was missing.
- Assert Task 4 GREEN: added `MRT_AssertSetHook`, `MRT_AssertFailed`, `MRT_ASSERT(expr)`, and `tests/unit/test_assert_hook.c`.
- Assert Task 4 verification: `python tools\run_host_tests.py` passed 55 test targets.
- Verification Task 5 updated API catalog and verification matrices for tickless, trace, and assert evidence.
- Verification Task 5 fresh run: `python tools\run_host_tests.py` passed 55 test targets.

### Phase 1: Requirements & Discovery
- **Status:** in_progress
- **Started:** 2026-06-07
- Actions taken:
  - Loaded caveman-mode instructions because user requested global caveman mode.
  - Loaded planning-with-files workflow because task is complex.
  - Loaded brainstorming, writing-plans, TDD, and git-worktree workflows relevant to design and implementation.
  - Checked project root.
  - Found project is effectively empty except `.codex-local`.
  - Checked Git status and log; both failed because project is not a Git repository.
  - Created persistent planning files.
  - Read official FreeRTOS documentation overview, source organization notes, kernel repository page, and reference manual structure.
  - Determined full request must be decomposed into releases before implementation.
  - User selected option C: broad FreeRTOS-like functionality.
  - Updated planning files to reflect C scope.
  - Assumed provisional platform: STM32 Cortex-M4/M7 + ARM GCC/CMake + TI C2000-style DSP abstraction.
  - Created C-scope architecture design draft.
  - Added requirement traceability matrix.
  - Added coupling test matrix.
  - Linked verification matrices from the design draft.
  - Added API catalog design.
  - Added test suite plan.
  - User approved the design.
  - Initialized Git repository.
  - Added `.gitignore`.
  - Renamed design draft to approved design file.
  - Committed design baseline as `cd4555b`.
  - Created master implementation plan.
  - Created foundation kernel TDD implementation plan.
  - Ran implementation-plan self-review scans.
  - User instructed autonomous best-direction decisions and reiterated: reference FreeRTOS only as design input, do not copy.
  - User added requirement: final Chinese manual must include detailed porting steps.
  - Created foundation worktree `feature/foundation-kernel`.
  - Found `cmake` command missing in local environment.
  - Confirmed GCC 13.1.0 is available.
  - Added fallback host test runner `tools/run_host_tests.py`.
  - Task 1 RED: `python tools\run_host_tests.py` failed because `myrtos/mrt_types.h` was missing.
  - Task 1 GREEN: added `include/myrtos/mrt_types.h`; same command passed 1 test target.
  - Committed Task 1 as `4122e94`.
  - Task 2 RED: `python tools\run_host_tests.py` failed because `myrtos/mrt_config.h` was missing.
  - Task 2 GREEN: added `include/myrtos/mrt_config.h`; same command passed 2 test targets.
  - Committed Task 2 as `1674397`.
  - Task 3 RED: `python tools\run_host_tests.py` failed because `myrtos/mrt_list.h` was missing.
  - Task 3 GREEN: added `include/myrtos/mrt_list.h` and `src/kernel/mrt_list.c`; same command passed 3 test targets.
  - Committed Task 3 as `62202ee`.
  - Task 4 RED: `python tools\run_host_tests.py` failed because `myrtos/mrt_priority.h` was missing.
  - Task 4 GREEN: added `include/myrtos/mrt_priority.h` and `src/kernel/mrt_priority.c`; same command passed 4 test targets.
  - Committed Task 4 as `dbd0f92`.
  - Task 5 RED: `python tools\run_host_tests.py` failed because `myrtos/mrt_port.h` was missing.
  - Task 5 GREEN: added `include/myrtos/mrt_port.h` and `src/portable/mock/mrt_port_mock.c`; same command passed 5 test targets.
  - Committed Task 5 as `3be37e7`.
  - Task 6 RED: `python tools\run_host_tests.py` failed because `myrtos/mrt_kernel.h` was missing.
  - Task 6 GREEN: added `include/myrtos/mrt_kernel.h` and `src/kernel/mrt_kernel.c`; same command passed 6 test targets.
  - Committed Task 6 as `034deb6`.
  - Foundation full verification: `python tools\run_host_tests.py` passed 6 test targets.
  - Updated requirement and coupling matrices with foundation evidence.
  - Created task scheduler worktree `feature/task-scheduler`.
  - Added scheduler implementation plan.
  - Consulted FreeRTOS `tasks.c` for concepts: ready lists per priority, current TCB, delayed lists, overflow handling; MyRTOS keeps original APIs and implementation.
  - Task scheduler Task 1 RED: `python tools\run_host_tests.py` failed because `myrtos/mrt_task.h` was missing.
  - Task scheduler Task 1 GREEN: added `include/myrtos/mrt_task.h`, `src/kernel/mrt_task.c`, and internal scheduler init; same command passed 7 test targets.
  - Committed scheduler Task 1 as `e9a15da`.
  - Task scheduler Task 2 RED: `python tools\run_host_tests.py` failed in `test_scheduler_start` because `MRT_KernelStart` returned `MRT_RESULT_NOT_STARTED`.
  - Task scheduler Task 2 GREEN: implemented highest ready task selection; same command passed 8 test targets.
  - Committed scheduler Task 2 as `68120e7`.
  - Task scheduler Task 3 RED: `python tools\run_host_tests.py` failed in `test_scheduler_round_robin` because yield did not rotate to the second same-priority task.
  - Task scheduler Task 3 GREEN: implemented ready list tail rotation on yield; same command passed 9 test targets.
  - Committed scheduler Task 3 as `ac3eda3`.
  - Task scheduler Task 4 RED: `python tools\run_host_tests.py` failed because `MRT_TaskDelay` was not declared.
  - Task scheduler Task 4 GREEN: implemented delayed list, `MRT_TaskDelay`, tick wakeup, and wake preemption; same command passed 10 test targets.
  - Committed scheduler Task 4 as `6f347c6`.
  - Task scheduler Task 5 RED: `python tools\run_host_tests.py` failed because `MRT_KernelTestSetTick` was not declared.
  - Task scheduler Task 5 GREEN: added `MRT_KernelTestSetTick` under `MRT_TESTING`; tick overflow delay test passed; full command passed 11 test targets.
  - Committed scheduler Task 5 as `8d06412`.
  - Task scheduler full verification: `python tools\run_host_tests.py` passed 11 test targets.
  - Updated requirement matrix rows `R-002`, `R-008`, `R-009`.
  - Updated coupling matrix rows `C-001`, `C-002`, `C-003`.
  - Created queues worktree `feature/queues`.
  - Added queue implementation plan.
  - Consulted FreeRTOS `queue.c` for concepts: fixed-size copy queue, circular storage, separate sender/receiver wait lists; MyRTOS keeps original APIs and implementation.
  - Queue Task 1 RED: `python tools\run_host_tests.py` failed because `myrtos/mrt_queue.h` was missing.
  - Queue Task 1 GREEN: added `include/myrtos/mrt_queue.h` and `src/kernel/mrt_queue.c`; same command passed 12 test targets.
  - Committed Queue Task 1 as `8d7ff8c`.
  - Queue Task 2 RED: `python tools\run_host_tests.py` failed because `MRT_QueueSend` and `MRT_QueueReceive` were not declared.
  - Queue Task 2 GREEN: implemented FIFO non-blocking `MRT_QueueSend` and `MRT_QueueReceive`; same command passed 13 test targets.
  - Committed Queue Task 2 with message `feat: add nonblocking queue send receive`.
  - Queue Task 3 RED: `python tools\run_host_tests.py` failed because `MRT_QueuePeek`, `MRT_QueueSendFront`, `MRT_QueueOverwrite`, and `MRT_QueueReset` were not declared.
  - Queue Task 3 GREEN: implemented peek, send-front, single-slot overwrite, and reset; same command passed 14 test targets.
  - Committed Queue Task 3 with message `feat: add queue variants`.
  - Queue Task 4 RED: `python tools\run_host_tests.py` failed because `MRT_QueueSendFromISR` and `MRT_QueueReceiveFromISR` were not declared.
  - Queue Task 4 GREEN: implemented ISR send/receive wrappers with ISR-context validation and conservative `should_yield=false`; same command passed 15 test targets.
  - Committed Queue Task 4 with message `feat: add queue ISR variants`.
  - Queue Task 5 RED: `python tools\run_host_tests.py` failed in `test_queue_task_timeout` because `MRT_QueueReceive(queue, out, 3)` did not block the current task or switch to the low-priority task.
  - Queue Task 5 GREEN: added task object-wait node/result/reason fields, queue receive waiting list coupling, and timeout cleanup on tick wake; same command passed 16 test targets.
  - Queue Task 5 commit message: `feat: add queue receive timeout coupling`.
  - Queue Task 6 RED: `python tools\run_host_tests.py` failed in `test_queue_send_wakes_receiver` because queue send did not wake the blocked receiver.
  - Queue Task 6 GREEN: added object-wait wakeup helper, normal send immediate reschedule, and ISR send delayed-yield signaling; same command passed 17 test targets.
  - Queue Task 6 commit message: `feat: wake receiver on queue send`.
  - Queue Task 7 verification: `python tools\run_host_tests.py` passed 17 test targets.
  - Queue Task 7 updated requirement matrix rows `R-002`, `R-008`, and `R-009`.
  - Queue Task 7 updated coupling matrix rows `C-004`, `C-005`, `C-006`, and `C-007`.
  - Queue branch commits: `8d7ff8c` static creation, `44e2d9f` nonblocking send/receive, `ffe8fc7` variants, `11eca1d` ISR variants, `4f7386e` receive timeout coupling, `96bb674` send wakes receiver.
  - Queue Task 7 commit message: `docs: record queue verification evidence`.
  - Created semaphore/mutex worktree `feature/semaphore-mutex` from queue verification baseline.
  - Created semaphore/mutex implementation plan at `docs/superpowers/plans/2026-06-07-myrtos-semaphore-mutex.md`.
  - Synchronization baseline verification: `python tools\run_host_tests.py` passed 17 test targets.
  - Semaphore Task 1 RED: `python tools\run_host_tests.py` failed because `myrtos/mrt_semaphore.h` was missing.
  - Semaphore Task 1 GREEN: added static binary/counting semaphore creation and count query; same command passed 18 test targets.
  - Semaphore Task 1 commit message: `feat: add static semaphore creation`.
  - Semaphore Task 2 RED: `python tools\run_host_tests.py` failed because `MRT_SemaphoreTake` and `MRT_SemaphoreGive` were not declared.
  - Semaphore Task 2 GREEN: implemented nonblocking take/give, empty/full results, null-handle validation, and no-current timeout result; same command passed 19 test targets.
  - Semaphore Task 2 commit message: `feat: add semaphore take give`.
  - Semaphore Task 3 RED: `python tools\run_host_tests.py` failed in semaphore timeout/wake coupling because take did not block the current high-priority task.
  - Semaphore Task 3 GREEN: added `MRT_TASK_WAIT_REASON_SEMAPHORE_TAKE`, semaphore take object-wait blocking, and give wakeup transfer; same command passed 21 test targets.
  - Semaphore Task 3 commit message: `feat: add semaphore scheduler coupling`.
  - Semaphore Task 4 RED: `python tools\run_host_tests.py` failed because `MRT_SemaphoreGiveFromISR` was not declared.
  - Semaphore Task 4 GREEN: implemented ISR give, ISR context validation, full-count handling, and delayed-yield wakeup; same command passed 22 test targets.
  - Semaphore Task 4 commit message: `feat: add semaphore ISR give`.
  - Mutex Task 5 RED: `python tools\run_host_tests.py` failed because `myrtos/mrt_mutex.h` was missing.
  - Mutex Task 5 first GREEN attempt failed because `test_mutex_create_lock` reused current task state from an earlier test case; fixed by reinitializing the kernel inside the invalid-context test.
  - Mutex Task 5 GREEN: added static mutex creation, lock/unlock ownership, owner query, owner-error handling, and invalid-context checks; same command passed 23 test targets.
  - Mutex Task 5 commit message: `feat: add mutex ownership`.
  - Mutex Task 6 RED: `python tools\run_host_tests.py` failed because high-priority mutex wait did not block and did not boost the low-priority owner.
  - Mutex Task 6 GREEN: added task effective-priority helpers, mutex wait blocking, priority inheritance, owner priority restore, and ownership transfer to the highest-priority waiter; same command passed 24 test targets.
  - Mutex Task 6 commit message: `feat: add mutex priority inheritance`.
  - Mutex Task 7 RED: `python tools\run_host_tests.py` failed because `MRT_MutexCreateRecursiveStatic` was not declared.
  - Mutex Task 7 GREEN: added recursive mutex creation and verified recursive lock depth, partial unlock, final unlock, and plain mutex relock busy result; same command passed 25 test targets.
  - Mutex Task 7 commit message: `feat: add recursive mutex`.
- Files created/modified:
  - `task_plan.md` created.
  - `findings.md` created and updated with FreeRTOS reference findings.
  - `progress.md` created.
  - `docs/superpowers/specs/2026-06-07-myrtos-c-scope-design.md` created.
  - `docs/verification/requirements_traceability_matrix.md` created.
  - `docs/verification/coupling_test_matrix.md` created.
  - `docs/api/myrtos_api_catalog.md` created.
  - `docs/verification/test_suite_plan.md` created.
  - `.gitignore` created.
  - `docs/superpowers/plans/2026-06-07-myrtos-master-implementation.md` created.
  - `docs/superpowers/plans/2026-06-07-myrtos-foundation-kernel.md` created.
  - `CMakeLists.txt` created in foundation worktree.
  - `tests/CMakeLists.txt` created.
  - `tests/support/mrt_test.h` created.
  - `tests/unit/test_types_contract.c` created.
  - `tools/run_host_tests.py` created.
  - `include/myrtos/mrt_types.h` created.
  - `tests/unit/test_config_defaults.c` created.
  - `include/myrtos/mrt_config.h` created.
  - `tests/unit/test_list.c` created.
  - `include/myrtos/mrt_list.h` created.
  - `src/kernel/mrt_list.c` created.
  - `tests/unit/test_priority_bitmap.c` created.
  - `include/myrtos/mrt_priority.h` created.
  - `src/kernel/mrt_priority.c` created.
  - `tests/port_mock/test_port_mock.c` created.
  - `include/myrtos/mrt_port.h` created.
  - `src/portable/mock/mrt_port_mock.c` created.
  - `tests/unit/test_kernel_tick.c` created.
  - `include/myrtos/mrt_kernel.h` created.
  - `src/kernel/mrt_kernel.c` created.
  - `docs/superpowers/plans/2026-06-07-myrtos-task-scheduler.md` created.
  - `tests/sim/test_task_create_static.c` created.
  - `include/myrtos/mrt_task.h` created.
  - `src/kernel/mrt_task.c` created.
  - `src/kernel/mrt_task_internal.h` created.
  - `tests/sim/test_scheduler_start.c` created.
  - `tests/sim/test_scheduler_round_robin.c` created.
  - `tests/sim/test_task_delay.c` created.
  - `tests/sim/test_task_delay_overflow.c` created.
  - `docs/superpowers/plans/2026-06-07-myrtos-queues.md` created.
  - `tests/unit/test_queue_create_static.c` created.
  - `include/myrtos/mrt_queue.h` created.
  - `src/kernel/mrt_queue.c` created.
  - `tests/unit/test_queue_send_receive.c` created.
  - `tests/CMakeLists.txt` updated for queue send/receive test.
  - `tools/run_host_tests.py` updated for queue send/receive test.
  - `tests/unit/test_queue_variants.c` created.
  - `include/myrtos/mrt_queue.h` updated with queue variant API declarations.
  - `src/kernel/mrt_queue.c` updated with queue variant implementations.
  - `tests/unit/test_queue_isr.c` created.
  - `include/myrtos/mrt_queue.h` updated with ISR queue API declarations.
  - `src/kernel/mrt_queue.c` updated with ISR queue API implementations.
  - `tests/coupling/test_queue_task_timeout.c` created.
  - `include/myrtos/mrt_task.h` updated with task wait reason/result fields.
  - `src/kernel/mrt_task_internal.h` updated with object-wait blocking API.
  - `src/kernel/mrt_task.c` updated with object-wait timeout coupling.
  - `tests/coupling/test_queue_send_wakes_receiver.c` created.
  - `src/kernel/mrt_task_internal.h` updated with object-wait wakeup API.
  - `src/kernel/mrt_queue.c` updated so task-context send wakes receivers and ISR send sets delayed-yield output when a receiver wakes.

## Test Results
| Test | Input | Expected | Actual | Status |
|------|-------|----------|--------|--------|
| Repository detection | `git status --short --branch` | Git status or clear failure | `fatal: not a git repository` | Logged |
| Repository history detection | `git log --oneline -5` | Git log or clear failure | `fatal: not a git repository` | Logged |

## Error Log
| Timestamp | Error | Attempt | Resolution |
|-----------|-------|---------|------------|
| 2026-06-07 | `git status` failed: not a Git repository | 1 | Logged state; continue without worktree. |
| 2026-06-07 | `git log` failed: not a Git repository | 1 | Logged state; continue without commit history. |
| 2026-06-07 | PowerShell rejected `&&` command separator | 1 | Switched to separate git commands. |
| 2026-06-07 | `cmake` command not found | 1 | Used GCC-backed project-local Python runner for host tests and kept CMake files for standard environments. |
| 2026-06-07 | `apply_patch` targeted the main worktree instead of `.worktrees\queues` | 1 | Re-ran the patch using the queue worktree path. |
| 2026-06-07 | `test_mutex_create_lock` invalid-context case reused current task from a previous test case | 1 | Added `MRT_KernelInitialize()` at the start of that test case to isolate scheduler state. |
| 2026-06-07 | `python tools\run_host_tests.py` timed out at 124 seconds after adding `test_message_buffer_isr` | 1 | Avoided repeating the same failing command; compiled the new test target directly to capture the expected RED error, then used a 240-second timeout for full verification. |

## Foundation Kernel Test Results
| Test | Input | Expected | Actual | Status |
|------|-------|----------|--------|--------|
| Task 1 RED | `python tools\run_host_tests.py` before `mrt_types.h` exists | Build fails due to missing header | `fatal error: myrtos/mrt_types.h: No such file or directory` | Pass |
| Task 1 GREEN | `python tools\run_host_tests.py` after adding `mrt_types.h` | 1 test target passes | `[summary] 1 test target(s) passed` | Pass |
| Task 2 RED | `python tools\run_host_tests.py` before `mrt_config.h` exists | Build fails due to missing header | `fatal error: myrtos/mrt_config.h: No such file or directory` | Pass |
| Task 2 GREEN | `python tools\run_host_tests.py` after adding `mrt_config.h` | 2 test targets pass | `[summary] 2 test target(s) passed` | Pass |
| Task 3 RED | `python tools\run_host_tests.py` before `mrt_list.h` exists | Build fails due to missing header | `fatal error: myrtos/mrt_list.h: No such file or directory` | Pass |
| Task 3 GREEN | `python tools\run_host_tests.py` after adding list module | 3 test targets pass | `[summary] 3 test target(s) passed` | Pass |
| Task 4 RED | `python tools\run_host_tests.py` before `mrt_priority.h` exists | Build fails due to missing header | `fatal error: myrtos/mrt_priority.h: No such file or directory` | Pass |
| Task 4 GREEN | `python tools\run_host_tests.py` after adding priority bitmap module | 4 test targets pass | `[summary] 4 test target(s) passed` | Pass |
| Task 5 RED | `python tools\run_host_tests.py` before `mrt_port.h` exists | Build fails due to missing header | `fatal error: myrtos/mrt_port.h: No such file or directory` | Pass |
| Task 5 GREEN | `python tools\run_host_tests.py` after adding host mock port | 5 test targets pass | `[summary] 5 test target(s) passed` | Pass |
| Task 6 RED | `python tools\run_host_tests.py` before `mrt_kernel.h` exists | Build fails due to missing header | `fatal error: myrtos/mrt_kernel.h: No such file or directory` | Pass |
| Task 6 GREEN | `python tools\run_host_tests.py` after adding kernel tick shell | 6 test targets pass | `[summary] 6 test target(s) passed` | Pass |
| Foundation full verification | `python tools\run_host_tests.py` | 6 test targets pass | `[summary] 6 test target(s) passed` | Pass |
| Scheduler Task 1 RED | `python tools\run_host_tests.py` before `mrt_task.h` exists | Build fails due to missing header | `fatal error: myrtos/mrt_task.h: No such file or directory` | Pass |
| Scheduler Task 1 GREEN | `python tools\run_host_tests.py` after static task creation | 7 test targets pass | `[summary] 7 test target(s) passed` | Pass |
| Scheduler Task 2 RED | `python tools\run_host_tests.py` before scheduler start selection | Start test fails | `expected 0 got 9` | Pass |
| Scheduler Task 2 GREEN | `python tools\run_host_tests.py` after highest ready selection | 8 test targets pass | `[summary] 8 test target(s) passed` | Pass |
| Scheduler Task 3 RED | `python tools\run_host_tests.py` before round-robin yield | Round-robin test fails | `assertion failed: MRT_TaskGetCurrent() == second_task` | Pass |
| Scheduler Task 3 GREEN | `python tools\run_host_tests.py` after ready list tail rotation | 9 test targets pass | `[summary] 9 test target(s) passed` | Pass |
| Scheduler Task 4 RED | `python tools\run_host_tests.py` before `MRT_TaskDelay` | Build fails due to missing function declaration | `implicit declaration of function 'MRT_TaskDelay'` | Pass |
| Scheduler Task 4 GREEN | `python tools\run_host_tests.py` after delay list and tick wakeup | 10 test targets pass | `[summary] 10 test target(s) passed` | Pass |
| Scheduler Task 5 RED | `python tools\run_host_tests.py` before tick test hook | Build fails due to missing function declaration | `implicit declaration of function 'MRT_KernelTestSetTick'` | Pass |
| Scheduler Task 5 GREEN | `python tools\run_host_tests.py` after tick overflow test hook | 11 test targets pass | `[summary] 11 test target(s) passed` | Pass |
| Scheduler full verification | `python tools\run_host_tests.py` | 11 test targets pass | `[summary] 11 test target(s) passed` | Pass |
| Queue Task 1 RED | `python tools\run_host_tests.py` before `mrt_queue.h` exists | Build fails due to missing header | `fatal error: myrtos/mrt_queue.h: No such file or directory` | Pass |
| Queue Task 1 GREEN | `python tools\run_host_tests.py` after static queue creation | 12 test targets pass | `[summary] 12 test target(s) passed` | Pass |
| Queue Task 2 RED | `python tools\run_host_tests.py` before send/receive declarations | Build fails due to missing function declarations | `implicit declaration of function 'MRT_QueueSend'`, `implicit declaration of function 'MRT_QueueReceive'` | Pass |
| Queue Task 2 GREEN | `python tools\run_host_tests.py` after FIFO non-blocking send/receive | 13 test targets pass | `[summary] 13 test target(s) passed` | Pass |
| Queue Task 3 RED | `python tools\run_host_tests.py` before queue variant declarations | Build fails due to missing function declarations | `implicit declaration of function 'MRT_QueuePeek'`, `MRT_QueueSendFront`, `MRT_QueueOverwrite`, `MRT_QueueReset` | Pass |
| Queue Task 3 GREEN | `python tools\run_host_tests.py` after queue variants | 14 test targets pass | `[summary] 14 test target(s) passed` | Pass |
| Queue Task 4 RED | `python tools\run_host_tests.py` before ISR declarations | Build fails due to missing function declarations | `implicit declaration of function 'MRT_QueueSendFromISR'`, `implicit declaration of function 'MRT_QueueReceiveFromISR'` | Pass |
| Queue Task 4 GREEN | `python tools\run_host_tests.py` after ISR queue APIs | 15 test targets pass | `[summary] 15 test target(s) passed` | Pass |
| Queue Task 5 RED | `python tools\run_host_tests.py` before queue receive blocking | Coupling test fails because current task does not block | `assertion failed: MRT_TaskGetCurrent() == low_task` | Pass |
| Queue Task 5 GREEN | `python tools\run_host_tests.py` after queue receive timeout coupling | 16 test targets pass | `[summary] 16 test target(s) passed` | Pass |
| Queue Task 6 RED | `python tools\run_host_tests.py` before send wakeup | Coupling test fails because sender does not wake receiver | `assertion failed: MRT_TaskGetCurrent() == receiver_task` | Pass |
| Queue Task 6 GREEN | `python tools\run_host_tests.py` after send wakeup | 17 test targets pass | `[summary] 17 test target(s) passed` | Pass |
| Queue final verification | `python tools\run_host_tests.py` after matrix updates | 17 test targets pass | `[summary] 17 test target(s) passed` | Pass |
| Semaphore Task 1 RED | `python tools\run_host_tests.py` before semaphore header | Build fails due to missing header | `fatal error: myrtos/mrt_semaphore.h: No such file or directory` | Pass |
| Semaphore Task 1 GREEN | `python tools\run_host_tests.py` after static semaphore creation | 18 test targets pass | `[summary] 18 test target(s) passed` | Pass |
| Semaphore Task 2 RED | `python tools\run_host_tests.py` before take/give declarations | Build fails due to missing declarations | `implicit declaration of function 'MRT_SemaphoreTake'`, `implicit declaration of function 'MRT_SemaphoreGive'` | Pass |
| Semaphore Task 2 GREEN | `python tools\run_host_tests.py` after take/give | 19 test targets pass | `[summary] 19 test target(s) passed` | Pass |
| Semaphore Task 3 RED | `python tools\run_host_tests.py` before semaphore scheduler coupling | Coupling tests fail because take does not block current task | `assertion failed: MRT_TaskGetCurrent() == low_task`, `assertion failed: MRT_TaskGetCurrent() == giver_task` | Pass |
| Semaphore Task 3 GREEN | `python tools\run_host_tests.py` after semaphore scheduler coupling | 21 test targets pass | `[summary] 21 test target(s) passed` | Pass |
| Semaphore Task 4 RED | `python tools\run_host_tests.py` before ISR give declaration | Build fails due to missing declaration | `implicit declaration of function 'MRT_SemaphoreGiveFromISR'` | Pass |
| Semaphore Task 4 GREEN | `python tools\run_host_tests.py` after ISR give | 22 test targets pass | `[summary] 22 test target(s) passed` | Pass |
| Mutex Task 5 RED | `python tools\run_host_tests.py` before mutex header | Build fails due to missing header | `fatal error: myrtos/mrt_mutex.h: No such file or directory` | Pass |
| Mutex Task 5 GREEN | `python tools\run_host_tests.py` after mutex ownership | 23 test targets pass | `[summary] 23 test target(s) passed` | Pass |
| Mutex Task 6 RED | `python tools\run_host_tests.py` before priority inheritance | Coupling test fails because high-priority waiter does not block | `assertion failed: MRT_TaskGetCurrent() == low_task` | Pass |
| Mutex Task 6 GREEN | `python tools\run_host_tests.py` after priority inheritance | 24 test targets pass | `[summary] 24 test target(s) passed` | Pass |
| Mutex Task 7 RED | `python tools\run_host_tests.py` before recursive mutex declaration | Build fails due to missing declaration | `implicit declaration of function 'MRT_MutexCreateRecursiveStatic'` | Pass |
| Mutex Task 7 GREEN | `python tools\run_host_tests.py` after recursive mutex | 25 test targets pass | `[summary] 25 test target(s) passed` | Pass |
| Sync Task 8 coverage add | `python tools\run_host_tests.py` after recursive non-owner release test | 25 test targets pass | `[summary] 25 test target(s) passed` | Pass |

## Semaphore/Mutex Commit Evidence
| Commit | Scope |
|--------|-------|
| `fc72b2e` | Static binary/counting semaphore creation |
| `451357c` | Semaphore take/give |
| `be339bb` | Semaphore scheduler timeout/wake coupling |
| `b9bb2c1` | Semaphore ISR give |
| `2632175` | Mutex ownership |
| `32b38f8` | Mutex priority inheritance |
| `06aad08` | Recursive mutex |

## Synchronization Verification Notes
- `docs/verification/requirements_traceability_matrix.md` now records semaphore/mutex implementation evidence for `R-002`, `R-008`, and `R-009`.
- `docs/verification/coupling_test_matrix.md` now marks `C-008`, `C-009`, `C-010`, and `C-013` verified; `C-011`, `C-012`, and `C-027` remain explicitly partial or pending where implementation is incomplete.
- User requirement that the final manual include detailed STM32/DSP porting steps remains captured as `R-011`.

## Events/Notifications Plan Results
| Check | Command | Expected | Actual | Status |
|-------|---------|----------|--------|--------|
| Baseline | `python tools\run_host_tests.py` in `.worktrees\events-notifications` | 25 test targets pass | `[summary] 25 test target(s) passed` | Pass |
| Plan placeholder scan | `rg -n "TBD|TODO|implement later|fill in details|appropriate error handling|add validation|Similar to Task" docs\superpowers\plans\2026-06-07-myrtos-events-notifications.md` | No matches | No matches, exit code 1 | Pass |
| Plan file | Create `docs/superpowers/plans/2026-06-07-myrtos-events-notifications.md` | Plan 5 tasks defined | 9 tasks defined for event groups, task notifications, and verification matrix update | Pass |
| Event Group Task 1 RED | `python tools\run_host_tests.py` before event group header | Build fails due to missing header | `fatal error: myrtos/mrt_event_group.h: No such file or directory` | Pass |
| Event Group Task 1 GREEN | `python tools\run_host_tests.py` after static event group bit APIs | 26 test targets pass | `[summary] 26 test target(s) passed` | Pass |
| Event Group Task 2 RED | `python tools\run_host_tests.py` before wait bits declaration | Build fails due to missing declaration | `implicit declaration of function 'MRT_EventGroupWaitBits'` | Pass |
| Event Group Task 2 GREEN | `python tools\run_host_tests.py` after immediate wait implementation | 27 test targets pass | `[summary] 27 test target(s) passed` | Pass |
| Event Group Task 3 RED | `python tools\run_host_tests.py` before event wait blocking | Coupling test fails because current task does not block | `assertion failed: MRT_TaskGetCurrent() == low_task` | Pass |
| Event Group Task 3 GREEN | `python tools\run_host_tests.py` after event wait timeout coupling | 28 test targets pass | `[summary] 28 test target(s) passed` | Pass |
| Event Group Task 4 RED | `python tools\run_host_tests.py` before set-wake coupling | Coupling test fails because set bits does not wake waiters | `assertion failed: MRT_TaskGetCurrent() == high_task` | Pass |
| Event Group Task 4 GREEN | `python tools\run_host_tests.py` after set-wake coupling | 29 test targets pass | `[summary] 29 test target(s) passed` | Pass |
| Event Group Task 5 RED | `python tools\run_host_tests.py` before ISR set declaration | Build fails due to missing declaration | `implicit declaration of function 'MRT_EventGroupSetBitsFromISR'` | Pass |
| Event Group Task 5 GREEN | `python tools\run_host_tests.py` after ISR set implementation | 30 test targets pass | `[summary] 30 test target(s) passed` | Pass |
| Task Notify Task 6 RED | `python tools\run_host_tests.py` before notify declarations | Build fails due to missing declarations and task fields | `implicit declaration of function 'MRT_TaskNotify'`, missing `notify_value`/`notify_pending` | Pass |
| Task Notify Task 6 GREEN | `python tools\run_host_tests.py` after notify actions | 31 test targets pass | `[summary] 31 test target(s) passed` | Pass |
| Task Notify Task 7 RED | `python tools\run_host_tests.py` before notify wait/take declarations | Build fails due to missing declarations | `implicit declaration of function 'MRT_TaskNotifyWait'`, `implicit declaration of function 'MRT_TaskNotifyTake'` | Pass |
| Task Notify Task 7 GREEN | `python tools\run_host_tests.py` after notify wait/take coupling | 32 test targets pass | `[summary] 32 test target(s) passed` | Pass |
| Task Notify Task 8 RED | `python tools\run_host_tests.py` before notify ISR declaration | Build fails due to missing declaration | `implicit declaration of function 'MRT_TaskNotifyFromISR'` | Pass |
| Task Notify Task 8 GREEN | `python tools\run_host_tests.py` after notify ISR implementation | 33 test targets pass | `[summary] 33 test target(s) passed` | Pass |
| Events/Notifications Task 9 final verification | `python tools\run_host_tests.py` after matrix updates | 33 test targets pass | `[summary] 33 test target(s) passed` | Pass |

## Events/Notifications Commit Evidence
| Commit | Scope |
|--------|-------|
| `7c8564a` | Events/notifications implementation plan |
| `7e77add` | Static event group bit operations |
| `156eac0` | Event group immediate wait |
| `8c6a10c` | Event group timeout wait |
| `ac6fa44` | Event group multi-waiter wake |
| `5bffb09` | Event group ISR set |
| `683efd7` | Task notification actions |
| `9b1a470` | Task notification wait/take |
| `d7cccea` | Task notification ISR |

## Events/Notifications Verification Notes
- `docs/verification/requirements_traceability_matrix.md` now records event group and task notification implementation/test evidence for `R-002`, `R-008`, and `R-009`.
- `docs/verification/coupling_test_matrix.md` now marks `C-014`, `C-015`, and `C-016` verified, and expands `C-027` partial illegal-context evidence with event group and task notification FromISR tests.
- Remaining unimplemented high-level modules are timers, stream/message buffers, memory managers, tickless/trace/assertion hooks, STM32/DSP ports, manual/static verification, and final report.

## Timers Plan Results
| Check | Command | Expected | Actual | Status |
|-------|---------|----------|--------|--------|
| Baseline | `python tools\run_host_tests.py` in `.worktrees\timers` | 33 test targets pass | `[summary] 33 test target(s) passed` | Pass |
| Plan placeholder scan | `rg -n "TBD|TODO|implement later|fill in details|appropriate error handling|add validation|Similar to Task" docs\superpowers\plans\2026-06-07-myrtos-timers.md` | No matches | No matches, exit code 1 | Pass |
| Plan file | Create `docs/superpowers/plans/2026-06-07-myrtos-timers.md` | Plan 6 tasks defined | 5 tasks defined for static timers, control APIs, tick expiry, pending function calls, and matrix update | Pass |
| Timer Task 1 RED | `python tools\run_host_tests.py` before timer header | Build fails due to missing header | `fatal error: myrtos/mrt_timer.h: No such file or directory` | Pass |
| Timer Task 1 GREEN | `python tools\run_host_tests.py` after static timer creation | 34 test targets pass | `[summary] 34 test target(s) passed` | Pass |
| Timer Task 1 pre-commit verification | `python tools\run_host_tests.py` | 34 test targets pass | `[summary] 34 test target(s) passed` | Pass |
| Timer Task 2 RED | `python tools\run_host_tests.py` before timer control declarations | Build fails due to missing declarations | `implicit declaration of function 'MRT_TimerStart'`, `MRT_TimerStop`, `MRT_TimerReset`, `MRT_TimerChangePeriod` | Pass |
| Timer Task 2 GREEN | `python tools\run_host_tests.py` after timer control APIs | 35 test targets pass | `[summary] 35 test target(s) passed` | Pass |
| Timer Task 3 RED | `python tools\run_host_tests.py` before timer tick processing | Coupling test fails because callbacks are not run on tick expiry | `test_timer_tick_expiry.c:143: expected 1 got 0` | Pass |
| Timer Task 3 GREEN | `python tools\run_host_tests.py` after kernel tick timer processing | 36 test targets pass | `[summary] 36 test target(s) passed` | Pass |
| Timer Task 4 RED | `python tools\run_host_tests.py` before pending function API/default config | Build fails due to missing config macro and declarations | `MRT_CFG_TIMER_PENDING_FUNCTION_QUEUE_LENGTH undeclared`, `implicit declaration of function 'MRT_TimerPendFunctionCall'`, `MRT_TimerServiceRunPending` | Pass |
| Timer Task 4 GREEN | `python tools\run_host_tests.py` after pending function FIFO | 37 test targets pass | `[summary] 37 test target(s) passed` | Pass |
| Timer Task 5 final verification | `python tools\run_host_tests.py` after timer implementation and before matrix update | 37 test targets pass | `[summary] 37 test target(s) passed` | Pass |

## Timer Commit Evidence
| Commit | Scope |
|--------|-------|
| `960a882` | Timers implementation plan |
| `e87352e` | Static timer creation |
| `1074ce6` | Timer control APIs |
| `9ef6071` | Kernel tick timer expiry processing |
| `dd782a3` | Timer pending function FIFO |

## Timer Verification Notes
- `docs/verification/requirements_traceability_matrix.md` now records software timer implementation and tests for `R-002`, `R-008`, and `R-009`.
- `docs/verification/coupling_test_matrix.md` now marks `C-018` verified for host tick expiry behavior and `C-017` partial for deterministic service-shim pending FIFO/control coverage.
- `C-019` remains pending because tickless idle compensation belongs to the later low-power/tickless plan.
- User requirement that the final manual include detailed STM32/DSP porting steps remains captured as `R-011`.

## Stream/Message Buffer Plan Results
| Check | Command | Expected | Actual | Status |
|-------|---------|----------|--------|--------|
| Worktree creation | `git worktree add .worktrees\stream-message-buffers -b feature/stream-message-buffers feature/timers` | New branch from timer baseline | Worktree created at `F:\My_RTOS\.worktrees\stream-message-buffers` | Pass |
| Baseline | `python tools\run_host_tests.py` in `.worktrees\stream-message-buffers` | 37 test targets pass | `[summary] 37 test target(s) passed` | Pass |
| Plan placeholder scan | `rg -n "TBD|TODO|implement later|fill in details|appropriate error handling|add validation|Similar to Task|待定" docs\superpowers\plans\2026-06-07-myrtos-stream-message-buffers.md` | No matches | No matches, exit code 1 | Pass |
| Plan file | Create `docs/superpowers/plans/2026-06-07-myrtos-stream-message-buffers.md` | Plan 7 tasks defined | 7 tasks defined for stream creation, stream FIFO, stream ISR coupling, message creation, message packet IO, message ISR, and matrix update | Pass |
| Stream Buffer Task 1 RED | `python tools\run_host_tests.py` before stream buffer header | Build fails due to missing header | `fatal error: myrtos/mrt_stream_buffer.h: No such file or directory` | Pass |
| Stream Buffer Task 1 GREEN | `python tools\run_host_tests.py` after static stream buffer creation | 38 test targets pass | `[summary] 38 test target(s) passed` | Pass |
| Stream Buffer Task 2 RED | `python tools\run_host_tests.py` before stream send/receive/reset declarations | Build fails due to missing declarations | `implicit declaration of function 'MRT_StreamBufferSend'`, `MRT_StreamBufferReceive`, `MRT_StreamBufferReset` | Pass |
| Stream Buffer Task 2 GREEN | `python tools\run_host_tests.py` after stream send/receive/reset | 39 test targets pass | `[summary] 39 test target(s) passed` | Pass |
| Stream Buffer Task 3 RED | `python tools\run_host_tests.py` before stream ISR API declarations | Build fails due to missing declarations | `implicit declaration of function 'MRT_StreamBufferSendFromISR'`, `MRT_StreamBufferReceiveFromISR` | Pass |
| Stream Buffer Task 3 GREEN | `python tools\run_host_tests.py` after stream ISR and reader wake coupling | 41 test targets pass | `[summary] 41 test target(s) passed` | Pass |
| Message Buffer Task 4 RED | `python tools\run_host_tests.py` before message buffer header | Build fails due to missing header | `fatal error: myrtos/mrt_message_buffer.h: No such file or directory` | Pass |
| Message Buffer Task 4 GREEN | `python tools\run_host_tests.py` after static message buffer creation | 42 test targets pass | `[summary] 42 test target(s) passed` | Pass |
| Message Buffer Task 5 RED | `python tools\run_host_tests.py` before message send/receive/reset declarations | Build fails due to missing declarations | `implicit declaration of function 'MRT_MessageBufferSend'`, `MRT_MessageBufferReceive`, `MRT_MessageBufferReset` | Pass |
| Message Buffer Task 5 GREEN | `python tools\run_host_tests.py` after packet send/receive/reset | 43 test targets pass | `[summary] 43 test target(s) passed` | Pass |
| Message Buffer Task 6 RED | Single-target GCC compile for `tests\unit\test_message_buffer_isr.c` before ISR APIs and wait reason | Build fails due to missing declarations and wait reason | `implicit declaration of function 'MRT_MessageBufferSendFromISR'`, `MRT_MessageBufferReceiveFromISR`, `MRT_TASK_WAIT_REASON_MESSAGE_RECEIVE undeclared` | Pass |
| Message Buffer Task 6 single-target GREEN | Compile and run `build\host-tests\test_message_buffer_isr.exe` after ISR implementation | New ISR test target passes | Exit code 0 | Pass |
| Message Buffer Task 6 GREEN | `python tools\run_host_tests.py` after message ISR APIs and reader wake coupling | 44 test targets pass | `[summary] 44 test target(s) passed` | Pass |
| Stream/Message Buffer Task 7 final verification | `python tools\run_host_tests.py` before matrix update | Stream/message buffers plus prior modules pass | `[summary] 44 test target(s) passed` | Pass |

## Stream/Message Buffer Commit Evidence
| Commit | Scope |
|--------|-------|
| `79f122b` | Stream/message buffer implementation plan |
| `1af8d79` | Static stream buffer creation |
| `2647d02` | Stream buffer send/receive |
| `153fc6a` | Stream buffer ISR wake coupling |
| `4b1ca30` | Static message buffer creation |
| `69e6858` | Message buffer send/receive |
| `d4df611` | Message buffer ISR APIs |

## Stream/Message Buffer Verification Notes
- `docs/verification/requirements_traceability_matrix.md` now records stream/message buffer implementation and tests for `R-002`, `R-008`, `R-009`, and `R-010`.
- `docs/verification/coupling_test_matrix.md` now marks `C-020`, `C-021`, and `C-022` verified.
- Remaining large modules are memory managers, tickless/low-power, trace/assertion hooks, STM32/DSP ports, final manual with detailed porting steps, static comment verification, and final report.

## Memory Management Plan Results
| Check | Command | Expected | Actual | Status |
|-------|---------|----------|--------|--------|
| Worktree creation | `git worktree add .worktrees\memory-management -b feature/memory-management feature/stream-message-buffers` | New branch from stream/message buffer baseline | Worktree created at `F:\My_RTOS\.worktrees\memory-management` | Pass |
| Baseline | `python tools\run_host_tests.py` in `.worktrees\memory-management` | 44 test targets pass | `[summary] 44 test target(s) passed` | Pass |
| Plan file | Create `docs/superpowers/plans/2026-06-07-myrtos-memory.md` | Plan 8 tasks defined | 7 tasks defined for heap initialization, linear heap, free-list heap, coalescing heap, fixed block pool, dynamic queue allocation, and verification matrix update | Pass |
| Heap Task 1 RED | `python tools\run_host_tests.py` before heap header exists | Build fails due to missing header | `fatal error: myrtos/mrt_heap.h: No such file or directory` | Pass |
| Heap Task 1 GREEN | `python tools\run_host_tests.py` after heap initialization/query implementation | 45 test targets pass | `[summary] 45 test target(s) passed` | Pass |
| Heap Task 2 RED | `python tools\run_host_tests.py` before heap allocation APIs | Build fails due to missing declarations | `implicit declaration of function 'MRT_Malloc'`, `implicit declaration of function 'MRT_Free'` | Pass |
| Heap Task 2 GREEN | `python tools\run_host_tests.py` after linear heap allocation | 46 test targets pass | `[summary] 46 test target(s) passed` | Pass |
| Heap Task 3 RED | `python tools\run_host_tests.py` after adding `test_heap_free_list` | `test_heap_free_list` fails because free-list release/reuse is missing | `[summary] 1 test target(s) failed`; `MRT_Free(large)` expected `MRT_RESULT_OK` but got `MRT_RESULT_OBJECT_BUSY` | Pass |
| Heap Task 3 GREEN | `python tools\run_host_tests.py` after first-fit free-list implementation | 47 test targets pass | `[summary] 47 test target(s) passed` | Pass |
| Heap Task 4 RED | `python tools\run_host_tests.py` after adding `test_heap_coalescing` | `test_heap_coalescing` fails because adjacent free blocks are not merged | `[summary] 1 test target(s) failed`; assertion `merged == first` failed | Pass |
| Heap Task 4 GREEN | `python tools\run_host_tests.py` after release-time coalescing implementation | 48 test targets pass | `[summary] 48 test target(s) passed` | Pass |
| Memory Pool Task 5 RED | `python tools\run_host_tests.py` after adding `test_memory_pool` | Build fails because memory pool public header is missing | `fatal error: myrtos/mrt_memory_pool.h: No such file or directory`; `[summary] 1 test target(s) failed` | Pass |
| Memory Pool Task 5 GREEN | `python tools\run_host_tests.py` after fixed-block pool implementation | 49 test targets pass | `[summary] 49 test target(s) passed` | Pass |
| Dynamic Queue Task 6 RED | `python tools\run_host_tests.py` after adding `test_queue_dynamic_allocation` | Build fails because dynamic queue APIs are missing | implicit declaration of `MRT_QueueCreate` and `MRT_QueueDelete`; `[summary] 1 test target(s) failed` | Pass |
| Dynamic Queue Task 6 GREEN | `python tools\run_host_tests.py` after dynamic queue create/delete implementation | 50 test targets pass | `[summary] 50 test target(s) passed` | Pass |
| Memory Task 7 final verification | `python tools\run_host_tests.py` before matrix update | Foundation through dynamic queue tests pass | `[summary] 50 test target(s) passed` | Pass |

## Memory Management Commit Evidence
| Commit | Scope |
|--------|-------|
| `e370ab3` | Memory management implementation plan |
| `e69144d` | Heap initialization and query APIs |
| `4850364` | Linear heap allocation |
| `47b3018` | Free-list heap allocation and reuse |
| `c3b8934` | Coalescing heap release |
| `5a3fa45` | Fixed block memory pool |
| `549f751` | Dynamic queue allocation |

## Memory Management Verification Notes
- `docs/verification/requirements_traceability_matrix.md` now records heap, memory pool, and dynamic queue implementation/test evidence for `R-002`, `R-008`, `R-009`, and `R-010`.
- `docs/verification/coupling_test_matrix.md` now marks `C-007`, `C-023`, and `C-024` verified.
- Latest full verification command: `python tools\run_host_tests.py`.
- Latest full verification output: `[summary] 50 test target(s) passed`.

## Plan Self-Review Results
| Check | Command | Expected | Actual | Status |
|-------|---------|----------|--------|--------|
| Placeholder scan | `rg -n "TBD|TODO|implement later|fill in details|appropriate error handling|add validation|Similar to Task|myrtos-c-scope-design-draft" docs\superpowers\plans docs\superpowers\specs docs\verification docs\api` | No matches | No matches, exit code 1 | Pass |
| Key API consistency scan | `rg -n "MRT_PriorityBitmap|MRT_KernelInitialize|MRT_PortInitialize|MRT_ListInitialize|MRT_RESULT_NOT_STARTED" docs\superpowers\plans\2026-06-07-myrtos-foundation-kernel.md` | Expected symbols present | Symbols present | Pass |

## 5-Question Reboot Check
| Question | Answer |
|----------|--------|
| Where am I? | Plan 8: memory management implementation plan |
| Where am I going? | Commit the Plan 8 implementation plan, then execute TDD tasks for heap and memory pool |
| What's the goal? | Build original STM32/DSP-capable RTOS with detailed Chinese comments, manual, and tests |
| What have I learned? | Memory plan should verify dynamic allocation failure (`C-007`, `C-023`) and adjacent free block coalescing (`C-024`) without copying FreeRTOS heap implementations |
| What have I done? | Created `.worktrees\memory-management`, verified the 44-target baseline, and drafted Plan 8 |

---
*Update after completing each phase or encountering errors.*

## Task Lifecycle API Gap Closure Results
| Check | Command | Expected | Actual | Status |
|-------|---------|----------|--------|--------|
| Worktree baseline | `git status --short --branch` in `.worktrees\task-lifecycle-apis` | Clean feature branch | `## feature/task-lifecycle-apis` | Pass |
| Planning files | Create/update task lifecycle plan and root planning logs | Plan, findings, and progress record scope | `docs/superpowers/plans/2026-06-08-myrtos-task-lifecycle-apis.md` added; root files updated | Pass |
| Task lifecycle RED | `python tools\run_host_tests.py` after adding two lifecycle tests | Build fails because task lifecycle APIs are missing | `test_task_lifecycle` and `test_task_dynamic_allocation` fail with implicit declarations for the eight target APIs | Pass |
| Task lifecycle GREEN | `python tools\run_host_tests.py` after task API implementation | All existing and new host tests pass | `[summary] 61 test target(s) passed` | Pass |
| Documentation evidence | Update manual, requirement matrix, coupling matrix, final report | Task lifecycle APIs documented as implemented; remaining gaps stated accurately | Manual task sections updated; R-002/R-008/R-009/C-001/C-002/C-012/C-023/C-027/final report updated | Pass |
| Static checks | Run three `tools\verify` scripts | Manual/comments/originality pass | API 125 covered; comments covered; originality clean | Pass |
| Final verification | `python tools\run_host_tests.py` plus three static scripts | Host and static checks pass | `[summary] 61 test target(s) passed`; API 125 covered; comments covered; originality clean | Pass |
| Whitespace check | `git diff --check` | No real whitespace errors | Exit 0; only expected LF-to-CRLF warnings | Pass |

## Dynamic Object API Gap Closure Results
| Check | Command | Expected | Actual | Status |
|-------|---------|----------|--------|--------|
| Worktree creation | `git worktree add .worktrees\dynamic-object-apis -b feature/dynamic-object-apis feature/task-lifecycle-apis` | New branch from task lifecycle baseline | Worktree created at `F:\My_RTOS\.worktrees\dynamic-object-apis` | Pass |
| Baseline host verification | `python tools\run_host_tests.py` | 61 host test targets pass | `[summary] 61 test target(s) passed` | Pass |
| Plan file | Create `docs/superpowers/plans/2026-06-08-myrtos-dynamic-object-apis.md` | Dynamic object API plan recorded | Plan covers 12 dynamic object APIs and leaves runtime stats for next branch | Pass |
| Dynamic object RED | `python tools\run_host_tests.py` after adding three dynamic tests | Build fails because dynamic object APIs are missing | `test_sync_dynamic_allocation`, `test_event_timer_dynamic_allocation`, and `test_buffer_dynamic_allocation` fail with implicit declarations | Pass |
| Dynamic object first GREEN attempt | `python tools\run_host_tests.py` after implementation | All tests pass or reveal implementation/test defect | 2 targets failed because tests tried `MRT_Malloc(free_before)` on a coalescing heap where block header overhead makes that request impossible | Logged |
| Dynamic object failure-path test fix | Replace full-free-size allocation with repeated heap exhaustion helper | Failure-path tests reliably force `MRT_RESULT_NO_MEMORY` without relying on impossible full-size allocation | `test_sync_dynamic_allocation` and `test_event_timer_dynamic_allocation` updated | Pass |
| Dynamic object GREEN | `python tools\run_host_tests.py` | Existing and new dynamic object tests pass | `[summary] 64 test target(s) passed` | Pass |
| Documentation evidence | Update manual, requirement matrix, coupling matrix, final report | Dynamic object APIs documented as implemented; remaining gap stated accurately | Manual dynamic sections updated; R-002/R-003/R-004/R-008/R-009/R-010/C-017/C-020/C-022/C-023/C-032/final report updated | Pass |
| Static checks | Run three `tools\verify` scripts | Manual/comments/originality pass | API 125 covered; comments covered; originality clean | Pass |
| Final verification | `python tools\run_host_tests.py` plus three static scripts and `git diff --check` | Host/static checks pass; no real whitespace errors | `[summary] 64 test target(s) passed`; API 125 covered; comments covered; originality clean; `git diff --check` exit 0 with expected CRLF warnings only | Pass |

## Runtime Stats API Gap Closure Results
| Check | Command | Expected | Actual | Status |
|-------|---------|----------|--------|--------|
| Worktree creation | `git worktree add .worktrees\runtime-stats-api -b feature/runtime-stats-api feature/dynamic-object-apis` | New branch from dynamic object baseline | Worktree created at `F:\My_RTOS\.worktrees\runtime-stats-api` | Pass |
| Baseline host verification | `python tools\run_host_tests.py` | 64 host test targets pass | `[summary] 64 test target(s) passed` | Pass |
| Plan file | Create `docs/superpowers/plans/2026-06-08-myrtos-runtime-stats-api.md` | Runtime stats API plan recorded | Plan covers tick-level runtime accounting and final catalog/source gap closure | Pass |
| Runtime stats RED | `python tools\run_host_tests.py` after adding runtime stats test | Build fails because stats API is missing | `test_runtime_stats` fails with `fatal error: myrtos/mrt_stats.h: No such file or directory`; all prior targets pass | Pass |
| Runtime stats GREEN | `python tools\run_host_tests.py` after implementation | Existing and new runtime stats tests pass | `[summary] 65 test target(s) passed` | Pass |
| Documentation evidence | Update manual, requirement matrix, coupling matrix, final report | Runtime stats documented as implemented; catalog/source gap count recorded as 0 | Manual `MRT_StatsGetTaskRuntime` updated; R-002/R-008/R-009/R-010/C-034/final report updated | Pass |
| Static checks | Run three `tools\verify` scripts | Manual/comments/originality pass | API 125 covered; comments covered; originality clean | Pass |
| Porting manual detail refresh | Strengthen manual STM32/DSP porting chapters | Manual includes detailed steps plus concrete integration and acceptance guidance | Added STM32/DSP minimal skeletons and acceptance checklists | Pass |
| Runtime stats final verification | `python tools\run_host_tests.py` plus three static scripts | Host/static checks pass | `[summary] 65 test target(s) passed`; API 125 covered; comments covered; originality clean | Pass |

## Final Verification Report Results
| Check | Command | Expected | Actual | Status |
|-------|---------|----------|--------|--------|
| Worktree creation | `git worktree add F:\My_RTOS\.worktrees\final-verification-report -b feature/final-verification-report feature/manual-static-verification` | New branch from manual/static baseline | Worktree created at `F:\My_RTOS\.worktrees\final-verification-report` | Pass |
| Baseline host verification | `python tools\run_host_tests.py` | 59 host test targets pass | `[summary] 59 test target(s) passed` | Pass |
| Baseline static verification | Three `tools\verify` scripts | Manual/comments/originality pass | API 125 covered; comments covered; originality clean | Pass |
| Report file | Create `docs/verification/final_verification_report.md` | Report with evidence and gaps | Report created with current verified scope and remaining work | Pass |
| Final verification | `python tools\run_host_tests.py` plus three static scripts | Host and static checks pass | `[summary] 59 test target(s) passed`; API 125 covered; comments covered; originality clean | Pass |

## Manual/Static Verification Plan Results
| Check | Command | Expected | Actual | Status |
|-------|---------|----------|--------|--------|
| Worktree creation | `git worktree add F:\My_RTOS\.worktrees\manual-static-verification -b feature/manual-static-verification feature/portable-stm32-dsp` | New branch from portable helper baseline | Worktree created at `F:\My_RTOS\.worktrees\manual-static-verification` | Pass |
| Baseline | `python tools\run_host_tests.py` in `.worktrees\manual-static-verification` | 59 test targets pass | `[summary] 59 test target(s) passed` | Pass |
| Plan file | Create `docs/superpowers/plans/2026-06-08-myrtos-manual-static-verification.md` | Plan 11 tasks defined | 4 tasks defined for static scripts, manual writing, static GREEN, and matrix evidence | Pass |
| Static scripts RED | `python tools\verify\check_api_manual_coverage.py`; `python tools\verify\check_chinese_comments.py`; `python tools\verify\check_original_symbols.py` | Manual missing and comment gaps detected; originality scan clean | Manual missing; 13 comment failures; originality scan passed | Pass |
| Comment coverage fix | `python tools\verify\check_chinese_comments.py` after adding missing comments | include/src comment check passes | `[chinese-comments] include/src function comments covered` | Pass |
| Manual coverage GREEN | `python tools\verify\check_api_manual_coverage.py` after writing manual | Every catalog API has manual section | `[manual-coverage] 125 API section(s) covered` | Pass |
| Static verification GREEN | Run all three `tools\verify` scripts | Manual, comments, originality all pass | API 125 covered; comments covered; no banned FreeRTOS-style public symbols | Pass |
| Manual/static full verification | `python tools\run_host_tests.py` plus three `tools\verify` scripts | Host and static checks pass | `[summary] 59 test target(s) passed`; API 125 covered; comments covered; originality clean | Pass |

## Portable STM32/DSP Plan Results
| Check | Command | Expected | Actual | Status |
|-------|---------|----------|--------|--------|
| Worktree baseline | `git -C F:\My_RTOS\.worktrees\portable-stm32-dsp status --short --branch` | Clean feature branch | `## feature/portable-stm32-dsp` | Pass |
| Plan file | Create `docs/superpowers/plans/2026-06-08-myrtos-portable-stm32-dsp.md` | Plan 10 tasks defined | 5 tasks defined for STM32 stack, STM32 tick/priority, DSP stack, DSP context model, and evidence update | Pass |
| STM32 Stack Task 1 RED | `python tools\run_host_tests.py` before STM32 port header | Build fails due to missing header | `fatal error: myrtos/portable/mrt_port_stm32_cm.h: No such file or directory`; `[summary] 1 test target(s) failed` | Pass |
| STM32 Stack Task 1 GREEN | `python tools\run_host_tests.py` after STM32 stack helper | New stack test plus prior modules pass | `[summary] 56 test target(s) passed` | Pass |
| STM32 Tick/Priority Task 2 RED | `python tools\run_host_tests.py` before tick/priority helpers | Build fails due to missing declarations | implicit declaration of `MRT_PortStm32CmCalculateSysTickReload` and `MRT_PortStm32CmEncodeBasepri`; `[summary] 1 test target(s) failed` | Pass |
| STM32 Tick/Priority Task 2 GREEN | `python tools\run_host_tests.py` after SysTick/BASEPRI helpers | STM32 port helpers plus prior modules pass | `[summary] 57 test target(s) passed` | Pass |
| DSP Stack Task 3 RED | `python tools\run_host_tests.py` before DSP port header | Build fails due to missing header | `fatal error: myrtos/portable/mrt_port_dsp_c28x.h: No such file or directory`; `[summary] 1 test target(s) failed` | Pass |
| DSP Stack Task 3 GREEN | `python tools\run_host_tests.py` after DSP stack helper | DSP stack helper plus prior modules pass | `[summary] 58 test target(s) passed` | Pass |
| DSP Context Task 4 RED | `python tools\run_host_tests.py` before DSP context model APIs | Build fails due to missing declarations | implicit declarations for context reset/request/ack/nesting APIs; `[summary] 1 test target(s) failed` | Pass |
| DSP Context Task 4 GREEN | `python tools\run_host_tests.py` after DSP context model | DSP context model plus prior modules pass | `[summary] 59 test target(s) passed` | Pass |
| Portable Task 5 verification | `python tools\run_host_tests.py` after build/docs/matrix updates and test comments | STM32/DSP port helpers plus prior modules pass | `[summary] 59 test target(s) passed` | Pass |
| Portable implementation commit | `git commit -m "feat: add STM32 and DSP portable helpers"` | Commit port helper implementation | `f24502a` | Pass |
| Portable branch push | `git push -u origin feature/portable-stm32-dsp` | Push branch to origin | Branch tracks `origin/feature/portable-stm32-dsp` | Pass |

## Buffer Writer Wait and API Prototype Audit Results
| Check | Command | Expected | Actual | Status |
|-------|---------|----------|--------|--------|
| Stream/message writer RED | Direct compile of `tests/coupling/test_buffer_dynamic_allocation.c` after adding writer-wait assertions | Build fails because send wait reasons and per-task requested-byte tracking are missing | Compile failed on missing `MRT_TASK_WAIT_REASON_STREAM_SEND`, `MRT_TASK_WAIT_REASON_MESSAGE_SEND`, and `MRT_Task.object_wait_bytes` | Pass |
| Stream/message writer GREEN | Direct compile/run of `test_buffer_dynamic_allocation` | Dynamic stream/message full-buffer writer waits enter waiting list, dynamic delete returns busy, receive wakes writer | Target compiled and ran successfully with exit code 0 | Pass |
| API prototype audit RED | `python tools\verify\check_api_catalog_prototypes.py` after adding verifier | Script reveals API catalog/header/source drift | Initial failures reduced to real public omissions: `MRT_MemoryPoolGetFreeCount` and `MRT_TimerGetName` missing from catalog/manual | Pass |
| API prototype audit GREEN | `python tools\verify\check_api_catalog_prototypes.py` | Catalog, public headers, and source definitions align | `[api-catalog] 135 API prototype(s) aligned` | Pass |
| Manual coverage GREEN | `python tools\verify\check_api_manual_coverage.py` | Manual covers all catalog APIs | `[manual-coverage] 135 API section(s) covered` | Pass |
| Full host verification | `python tools\run_host_tests.py` | All host targets pass | `[summary] 69 test target(s) passed` | Pass |
| Static/smoke verification | `python tools\verify\check_chinese_comments.py`; `python tools\verify\check_original_symbols.py`; `python tools\verify\check_embedded_smoke_projects.py`; `git diff --check` | Static and smoke checks pass; no real whitespace errors | Chinese comments covered; originality clean; STM32 cross build + DSP model smoke passed; `git diff --check` exit 0 with expected CRLF warnings | Pass |
| Parallel verification timeout | Parallel run including host tests | All checks finish | Host runner timed out at 184 s while static/smoke checks passed; reran host tests alone with longer timeout and passed | Logged |

## Manual Porting Detail Refresh
- User asked for even more detailed porting steps in the manual.
- Manual already has STM32/DSP sections; next edit will add vendor-project migration order, file-by-file integration sequence, first-board bring-up order, and board evidence checklist wording.
- Error logged: first attempt to append this progress entry used stale context and failed; resolved by appending after the latest table tail.

## Hardware Evidence Generator Integration
- Added `tests/static/test_hardware_smoke_evidence_generator.py` and `tools/verify/generate_hardware_smoke_evidence.py`.
- Added generator commands to `docs/manual/MyRTOS_Reference_Manual_zh.md`, `docs/verification/hardware_smoke/README.md`, and `docs/verification/hardware_smoke/collection_checklist.md`.
- Added `hardware-evidence-generator` to `tools/verify/run_release_verification.py` and the corresponding release-runner expectation test.
- Updated `docs/verification/final_verification_report.md`, `docs/verification/completion_audit.md`, `docs/verification/test_suite_plan.md`, and `docs/verification/requirements_traceability_matrix.md` so release evidence and hardware evidence flow mention the generator and the current 9-step default release gate.
- Verification passed:
  - `python tests\static\test_hardware_smoke_evidence_generator.py`
  - `python tests\static\test_release_verification_runner.py`
  - `python tools\verify\check_api_manual_coverage.py`
  - `python tools\verify\run_release_verification.py` -> `[release] 9 step(s) passed`
- `python tools\verify\run_release_verification.py --require-hardware` still fails only on missing real `stm32_board_smoke.md` and `dsp_board_smoke.md`.
- Follow-up verification after report sync:
  - `python tests\static\test_hardware_smoke_evidence_generator.py` -> pass
  - `python tests\static\test_release_verification_runner.py` -> pass
  - `python tools\verify\check_api_manual_coverage.py` -> `[manual-coverage] 135 API section(s) covered`
  - `git diff --check` -> exit 0 with expected CRLF warnings only
  - first `python tools\verify\run_release_verification.py` attempt timed out at 244 seconds with no failure output; reran with longer timeout and got `[release] 9 step(s) passed`
  - `python tools\verify\check_hardware_smoke_evidence.py` -> expected failure on missing `stm32_board_smoke.md` and `dsp_board_smoke.md`

## STM32 Context Assembly Scaffold
- Added RED static checks:
  - `python tests\static\test_stm32_context_scaffold.py` failed on missing `src\portable\stm32_cm\mrt_port_stm32_cm_context.S`
  - `python tests\static\test_release_verification_runner.py` failed because the new release step was absent
- Added `src\portable\stm32_cm\mrt_port_stm32_cm_context.S` with `SVC_Handler`, `PendSV_Handler`, `MRT_PortStm32CmStartFirstTaskAsm`, PSP handling, R4-R11 save/restore markers, and C hook calls.
- Added STM32 smoke C hooks `MRT_PortStm32CmSvcHook()` and `MRT_PortStm32CmPendSvHook()`.
- Updated `tools\verify\check_embedded_smoke_projects.py` so ARM GCC cross-build includes the `.S` file.
- Added `stm32-context-scaffold` to `tools\verify\run_release_verification.py` and updated `tests\static\test_release_verification_runner.py`.
- Updated STM32 manual porting steps, `examples/stm32/README.md`, final verification report, completion audit, test suite plan, requirement matrix, and coupling matrix.
- GREEN checks passed:
  - `python tests\static\test_stm32_context_scaffold.py`
  - `python tests\static\test_release_verification_runner.py`
  - `python tools\verify\check_embedded_smoke_projects.py`
  - `python tools\verify\check_chinese_comments.py`
  - `python tools\verify\check_api_manual_coverage.py`
  - `python tools\verify\check_api_catalog_prototypes.py`
  - `python tools\verify\check_original_symbols.py`
  - `python tools\verify\run_release_verification.py` -> `[release] 10 step(s) passed`
  - `git diff --check` -> exit 0 with expected CRLF warnings only
- Hardware evidence gate remains expected-failing:
  - `python tools\verify\check_hardware_smoke_evidence.py` -> missing `stm32_board_smoke.md` and `dsp_board_smoke.md`

## Task Runtime Stack-Top Contract
- Added RED host test `tests\sim\test_task_stack_top.c`; first `python tools\run_host_tests.py` run failed only because `MRT_TaskKernelGetStackTop`, `MRT_TaskKernelSetStackTop`, and `MRT_TaskKernelSwitchStackTop` were missing.
- Added `MRT_Task.stack_top`, initialized it for static and dynamic tasks, and implemented internal stack-top helpers in `src\kernel\mrt_task.c` / `src\kernel\mrt_task_internal.h`.
- Updated scheduler paths so the task being switched out is remembered before yield, blocking wait, delay, or current-task suspend; PendSV can then save the old PSP into the correct TCB.
- Added static RED coverage requiring `examples/stm32/mrt_port_stm32_smoke.c` to call `MRT_TaskKernelSwitchStackTop()` and requiring embedded smoke to include `-Isrc/kernel`.
- Wired `MRT_PortStm32CmPendSvHook()` to `MRT_TaskKernelSwitchStackTop()` and updated STM32 manual/README plus verification matrices.
- Follow-up sanity check found STM32 smoke task creation still returned raw empty stack ends to TCBs. Added RED static coverage requiring `examples\stm32\main.c` to call `MRT_PortStm32CmInitializeStack()` and `MRT_TaskKernelSetStackTop()`.
- Updated `examples\stm32\main.c` so LED/UART tasks build Cortex-M initial exception frames and write the initialized PSP into each TCB before `MRT_KernelStart()`.
- Rechecked `python tests\static\test_stm32_context_scaffold.py` and `python tools\verify\check_embedded_smoke_projects.py`; both passed after the STM32 initial-stack wiring.
- First follow-up `python tools\verify\check_chinese_comments.py` failed because the new helper had been inserted between `SmokeConfigureClock` and its Doxygen block; moved the Doxygen block back above `SmokeConfigureClock`, then comment check passed.
- Full verification passed: `python tools\verify\run_release_verification.py` -> `[release] 10 step(s) passed` after 70 host test targets.
- Whitespace check passed: `git diff --check` exited 0 with only expected CRLF conversion warnings.
- Hardware evidence gate remains expected-failing: `python tools\verify\check_hardware_smoke_evidence.py` reports missing `stm32_board_smoke.md` and `dsp_board_smoke.md`.
- GREEN checks passed so far:
  - `python tools\run_host_tests.py` -> `[summary] 70 test target(s) passed`
  - `python tests\static\test_stm32_context_scaffold.py`
  - `python tools\verify\check_embedded_smoke_projects.py` -> `[embedded-smoke] STM32 cross build and DSP model smoke passed`
- Hardware evidence gap remains unchanged: no real `stm32_board_smoke.md` or `dsp_board_smoke.md` PASS logs have been produced.

## Hardware Smoke Preflight Configuration
- Verified RED: `python tests\static\test_hardware_smoke_preflight.py` failed on missing `tools\verify\check_hardware_smoke_preflight.py`.
- Added `tools\verify\check_hardware_smoke_preflight.py` with schema validation, placeholder rejection, required STM32/DSP expected-field checks, runtime minimum checks, target-specific STM32/DSP checks, and optional `--check-tools`.
- Added `docs\verification\hardware_smoke\hardware_smoke_preflight.json` with STM32 and DSP default board smoke capture configuration.
- Verified GREEN for first cycle:
  - `python tests\static\test_hardware_smoke_preflight.py`
  - `python tools\verify\check_hardware_smoke_preflight.py`
- Verified RED for release runner wiring: `python tests\static\test_release_verification_runner.py` failed after adding `hardware-smoke-preflight` to the expected default step list.
- Added `hardware-smoke-preflight` to `tools\verify\run_release_verification.py`.
- Verified GREEN for runner wiring:
  - `python tests\static\test_release_verification_runner.py`
  - `python tests\static\test_hardware_smoke_preflight.py`
  - `python tools\verify\check_hardware_smoke_preflight.py`
- Updated STM32/DSP manual evidence sections, hardware smoke README/checklist, final verification report, completion audit, test suite plan, requirements matrix, and coupling matrix to mention preflight before real board evidence generation.
- Verification after docs/tooling sync:
  - `python tests\static\test_hardware_smoke_preflight.py` -> pass
  - `python tests\static\test_release_verification_runner.py` -> pass
  - `python tools\verify\check_hardware_smoke_preflight.py` -> accepted
  - `python tools\verify\check_api_manual_coverage.py` -> `[manual-coverage] 135 API section(s) covered`
  - `python tools\verify\check_chinese_comments.py` -> pass
  - `python tools\verify\check_api_catalog_prototypes.py` -> `[api-catalog] 135 API prototype(s) aligned`
  - `python tools\verify\run_release_verification.py` -> `[release] 11 step(s) passed`
  - `git diff --check` -> exit 0 with expected CRLF conversion warnings only
  - `python tools\verify\check_hardware_smoke_evidence.py` -> expected failure on missing real `stm32_board_smoke.md` and `dsp_board_smoke.md`
  - `python tools\verify\run_release_verification.py --require-hardware` -> expected `[release] 1 step(s) failed` only at `hardware-smoke-evidence`
- Remaining: collect real STM32 and DSP board logs, generate/fill final evidence files, and pass the hardware evidence gate.

## Hardware Evidence Raw-Log Traceability
- Added RED assertions in `tests\static\test_hardware_smoke_evidence_generator.py` requiring generated evidence to contain `Raw-Log-Path` and `Raw-Log-SHA256`.
- Added RED checker case in `tests\static\test_hardware_smoke_evidence_checker.py` requiring SHA-256 mismatch against the retained raw log to fail.
- First RED runs:
  - `python tests\static\test_hardware_smoke_evidence_generator.py` failed at missing `Raw-Log-Path`
  - `python tests\static\test_hardware_smoke_evidence_checker.py` failed because `Raw-Log-SHA256 mismatch` was not reported
- Implemented SHA-256 derivation in `tools\verify\generate_hardware_smoke_evidence.py`.
- Implemented `Raw-Log-Path` resolution and SHA-256 comparison in `tools\verify\check_hardware_smoke_evidence.py`.
- GREEN checks:
  - `python tests\static\test_hardware_smoke_evidence_generator.py`
  - `python tests\static\test_hardware_smoke_evidence_checker.py`
- Updated STM32/DSP evidence templates, manual evidence sections, hardware smoke README/checklist, final verification report, completion audit, test suite plan, requirements matrix, coupling matrix, and task plan.
- Follow-up focused checks passed:
  - `python tools\verify\check_api_catalog_prototypes.py` -> `[api-catalog] 135 API prototype(s) aligned`
  - `python tools\verify\check_original_symbols.py` -> no banned FreeRTOS-style public symbols
  - `python tools\verify\check_hardware_smoke_preflight.py` -> hardware smoke preflight config accepted
  - `git diff --check` -> exit 0 with expected CRLF conversion warnings only
- Phase 20 default release verification passed before adding the C2000 project scaffold step:
  - `python tools\verify\run_release_verification.py` -> `[summary] 70 test target(s) passed`; `[release] 11 step(s) passed`
- Hardware-required gate remains intentionally failing until real boards are run:
  - `python tools\verify\check_hardware_smoke_evidence.py` -> missing `stm32_board_smoke.md` and `dsp_board_smoke.md`
  - `python tools\verify\run_release_verification.py --require-hardware` -> `[release] 1 step(s) failed` only at `hardware-smoke-evidence`
- Remaining: collect real STM32 and DSP board logs, retain matching raw logs, generate/fill final evidence files, and pass the hardware evidence gate.

## DSP C2000 Board Smoke Project Scaffold
- Added RED static coverage:
  - `python tests\static\test_dsp_c2000_project_scaffold.py` failed on missing `examples\dsp\startup_c28x.c`.
  - `python tests\static\test_release_verification_runner.py` failed because `dsp-c2000-project-scaffold` was absent from the default release chain.
- Added `examples\dsp\startup_c28x.c` for C2000 timer/software-interrupt/ADC vector placeholders and startup install order.
- Added `examples\dsp\mrt_port_dsp_c2000_smoke.c` for C2000 `MRT_Port*` glue, CPU Timer0 tick ISR, ADC FromISR queue path, C28x software interrupt handoff, and stack-top switch hook.
- Added `examples\dsp\linker_c28x.cmd` for C2000 `.mrtos_heap`, `.mrtos_tasks`, `.mrtos_dma`, and `.mrtos_trace` linker section placeholders.
- Added `dsp-c2000-project-scaffold` to `tools\verify\run_release_verification.py` and required the C2000 files in `tools\verify\check_embedded_smoke_projects.py`.
- Initial GREEN checks:
  - `python tests\static\test_dsp_c2000_project_scaffold.py`
  - `python tests\static\test_release_verification_runner.py`
  - `python tools\verify\check_embedded_smoke_projects.py` -> `[embedded-smoke] STM32 cross build and DSP model smoke passed`
- Follow-up review found `startup_c28x.c` should not define weak ISR placeholders because some TI compiler configurations may not treat the macro as weak and could collide with board glue. Added RED assertions requiring `extern` ISR declarations and no `MRT_DSP_C2000_WEAK`, then changed startup to let `mrt_port_dsp_c2000_smoke.c` own the ISR definitions.
- Updated manual, README, requirement matrix, coupling matrix, test-suite plan, completion audit, final report, findings, progress, and task plan for the C2000 scaffold and the continued real-board boundary.
- Focused/static checks after docs sync:
  - `python tests\static\test_dsp_c2000_project_scaffold.py`
  - `python tests\static\test_release_verification_runner.py`
  - `python tools\verify\check_api_manual_coverage.py` -> `[manual-coverage] 135 API section(s) covered`
  - `python tools\verify\check_api_catalog_prototypes.py` -> `[api-catalog] 135 API prototype(s) aligned`
  - `python tools\verify\check_chinese_comments.py` -> `[chinese-comments] include/src/examples/tests function comments covered`
  - `python tools\verify\check_original_symbols.py` -> `[original-symbols] no banned FreeRTOS-style public symbols found`
  - `git diff --check` -> exit 0 with expected CRLF conversion warnings only
- Full default release verification passed:
  - `python tools\verify\run_release_verification.py` -> `[summary] 70 test target(s) passed`; `[release] 14 step(s) passed`
- Hardware-required gate remains intentionally failing until real boards are run:
  - `python tools\verify\check_hardware_smoke_evidence.py` -> missing `stm32_board_smoke.md` and `dsp_board_smoke.md`
  - `python tools\verify\run_release_verification.py --require-hardware` -> `[release] 1 step(s) failed` only at `hardware-smoke-evidence`
- Note: one temporary Python one-liner used only for file inspection failed with a PowerShell newline escaping `SyntaxError`; replaced by direct `Get-Content` inspection and no production files were written by that command.
- After the follow-up startup/glue ownership fix, reran `python tests\static\test_dsp_c2000_project_scaffold.py`, `python tests\static\test_release_verification_runner.py`, `python tools\verify\check_chinese_comments.py`, `git diff --check`, `python tools\verify\run_release_verification.py`, and `python tools\verify\run_release_verification.py --require-hardware`; results stayed the same: default release passed 14 steps, hardware-required release failed only at `hardware-smoke-evidence`.
- Remaining: collect real STM32 and DSP board logs, retain matching raw logs, generate/fill final evidence files, and pass the hardware evidence gate.

## Hardware Smoke Capture Runner
- Added RED static coverage:
  - `python tests\static\test_hardware_smoke_capture_runner.py` failed on missing `tools\verify\run_hardware_smoke_capture.py`
  - `python tests\static\test_release_verification_runner.py` failed because `hardware-smoke-capture-runner` was absent from the default release chain
- Added `tools\verify\run_hardware_smoke_capture.py`.
  - Default mode is dry-run only.
  - `--execute` runs the configured compiler/build/flash/capture commands.
  - The runner invokes preflight validation, raw-log evidence generation, and target-level evidence validation.
- Added `hardware-smoke-capture-runner` to `tools\verify\run_release_verification.py`.
- Added a follow-up RED case for generator duplication when `capture_command` already calls `generate_hardware_smoke_evidence.py`; fixed the runner to skip the extra `generate-evidence` step in that case.
- Updated manual, hardware smoke README/checklist, final verification report, completion audit, requirements matrix, coupling matrix, test suite plan, and task plan.
- Focused checks passed:
  - `python tests\static\test_hardware_smoke_capture_runner.py`
  - `python tests\static\test_release_verification_runner.py`
  - `python tools\verify\check_api_manual_coverage.py` -> `[manual-coverage] 135 API section(s) covered`
  - `python tools\verify\check_hardware_smoke_preflight.py` -> hardware smoke preflight config accepted
  - `python tools\verify\run_hardware_smoke_capture.py --target STM32` -> dry-run printed preflight/compiler/build/flash/capture/verify plan without executing hardware commands
- Phase 19 default release verification passed before adding the DSP scaffold step:
  - `python tools\verify\run_release_verification.py` -> `[summary] 70 test target(s) passed`; `[release] 12 step(s) passed`
- Hardware-required gate remains intentionally failing until real boards are run:
  - `python tools\verify\check_hardware_smoke_evidence.py` -> missing `stm32_board_smoke.md` and `dsp_board_smoke.md`
  - `python tools\verify\run_release_verification.py --require-hardware` -> `[release] 1 step(s) failed` only at `hardware-smoke-evidence`
- Remaining: collect real STM32 and DSP board logs, retain matching raw logs, generate/fill final evidence files, and pass the hardware evidence gate.

## DSP C28x Context Assembly Scaffold
- Added RED static coverage:
  - `python tests\static\test_dsp_context_scaffold.py` requires `src\portable\dsp_c28x\mrt_port_dsp_c28x_context.asm`, first-task/yield/software-interrupt symbols, save/restore markers, and C hook handoff.
  - `python tests\static\test_release_verification_runner.py` requires the default release chain to include `dsp-context-scaffold`.
- Added `src\portable\dsp_c28x\mrt_port_dsp_c28x_context.asm` as a TI C28x-style context switch audit scaffold. It documents first-task start, software interrupt yield, register save/restore order, hook calls, and the real-board boundary.
- Added `dsp-context-scaffold` to `tools\verify\run_release_verification.py`.
- Updated `examples\dsp\README.md`, manual DSP porting steps, requirement matrix, coupling matrix, test suite plan, completion audit, final report, and task plan.
- Focused/static checks passed:
  - `python tests\static\test_dsp_context_scaffold.py`
  - `python tests\static\test_release_verification_runner.py`
  - `python tools\verify\check_api_manual_coverage.py` -> `[manual-coverage] 135 API section(s) covered`
  - `python tools\verify\check_api_catalog_prototypes.py` -> `[api-catalog] 135 API prototype(s) aligned`
  - `python tools\verify\check_chinese_comments.py` -> `[chinese-comments] include/src/examples/tests function comments covered`
  - `python tools\verify\check_original_symbols.py` -> `[original-symbols] no banned FreeRTOS-style public symbols found`
  - `git diff --check` -> exit 0 with expected CRLF conversion warnings only
- Full default release verification passed:
  - `python tools\verify\run_release_verification.py` -> `[summary] 70 test target(s) passed`; `[release] 13 step(s) passed`
- Hardware-required gate remains intentionally failing until real boards are run:
  - `python tools\verify\check_hardware_smoke_evidence.py` -> missing `stm32_board_smoke.md` and `dsp_board_smoke.md`
  - `python tools\verify\run_release_verification.py --require-hardware` -> `[release] 1 step(s) failed` only at `hardware-smoke-evidence`
- Remaining: collect real STM32 and DSP board logs, retain matching raw logs, generate/fill final evidence files, and pass the hardware evidence gate.

## Hardware Smoke Raw-Log Schema
- Added RED static coverage in `tests\static\test_hardware_smoke_raw_log_schema.py`.
  - `python tests\static\test_hardware_smoke_raw_log_schema.py` failed on missing `tools\verify\check_hardware_smoke_raw_log_schema.py`.
  - `python tests\static\test_release_verification_runner.py` failed because `hardware-smoke-raw-log-schema` was absent from the default release chain.
- Added `docs\verification\hardware_smoke\raw_log_schema.md` for common, STM32, and DSP raw UART/trace `Key: Value` fields and examples.
- Added `examples\hardware_smoke\mrt_hardware_smoke_log_schema.h` with `MRT_SMOKE_FIELD_*` constants and target X-macro required-field lists.
- Added `tools\verify\check_hardware_smoke_raw_log_schema.py` and wired `hardware-smoke-raw-log-schema` into `tools\verify\run_release_verification.py`.
- Updated hardware smoke README/checklist and STM32/DSP example READMEs to point at `raw_log_schema.md`.
- Focused checks passed:
  - `python tests\static\test_hardware_smoke_raw_log_schema.py`
  - `python tests\static\test_release_verification_runner.py`
  - `python tools\verify\check_hardware_smoke_raw_log_schema.py` -> `[hardware-raw-log-schema] schema fields aligned`
- Updated manual, final verification report, requirement matrix, coupling matrix, test-suite plan, completion audit, findings, progress, and task plan for the schema contract and continued real-board boundary.
- Focused/static checks after docs sync passed:
  - `python tools\verify\check_api_manual_coverage.py` -> `[manual-coverage] 135 API section(s) covered`
  - `python tools\verify\check_api_catalog_prototypes.py` -> `[api-catalog] 135 API prototype(s) aligned`
  - `python tools\verify\check_chinese_comments.py` -> `[chinese-comments] include/src/examples/tests function comments covered`
  - `python tools\verify\check_original_symbols.py` -> `[original-symbols] no banned FreeRTOS-style public symbols found`
  - `python tools\verify\check_hardware_smoke_preflight.py` -> hardware smoke preflight config accepted
  - `python tools\verify\check_embedded_smoke_projects.py` -> `[embedded-smoke] STM32 cross build and DSP model smoke passed`
  - `git diff --check` -> exit 0 with expected CRLF conversion warnings only
- Full default release verification passed:
  - `python tools\verify\run_release_verification.py` -> `[summary] 70 test target(s) passed`; `[release] 15 step(s) passed`
- Hardware-required gate remains intentionally failing until real boards are run:
  - `python tools\verify\check_hardware_smoke_evidence.py` -> missing `stm32_board_smoke.md` and `dsp_board_smoke.md`
  - `python tools\verify\run_release_verification.py --require-hardware` -> `[release] 1 step(s) failed` only at `hardware-smoke-evidence`

## Hardware Smoke Log Output Helper
- Added RED host coverage:
  - `tests\unit\test_hardware_smoke_log.c` first failed because `examples\hardware_smoke\mrt_hardware_smoke_log.h` was missing.
- Added `examples\hardware_smoke\mrt_hardware_smoke_log.h`.
  - `MRT_SmokeLogWriter` binds a board single-character callback.
  - `MRT_SmokeLogWritePair()` writes `Key: Value\n` string fields.
  - `MRT_SmokeLogWriteU32()` writes decimal `uint32_t` fields without `printf`.
  - Invalid arguments return `MRT_SMOKE_LOG_INVALID_ARGUMENT` before any partial output.
- Wired `test_hardware_smoke_log` into `tools\run_host_tests.py` and `tests\CMakeLists.txt`.
- Initial GREEN checks before final doc sync:
  - direct `test_hardware_smoke_log` compile/run passed.
  - `python tools\run_host_tests.py` -> `[summary] 71 test target(s) passed`.
- Updated manual sections 5.6/6.6, hardware smoke README, raw-log schema, STM32/DSP READMEs, final report, requirement matrix, coupling matrix, test-suite plan, completion audit, findings, progress, and task plan so the helper is documented as a formatting helper only, not real-board evidence.
- Fresh focused checks passed:
  - `python tools\run_host_tests.py` -> `[summary] 71 test target(s) passed`
  - `python tools\verify\check_chinese_comments.py` -> `[chinese-comments] include/src/examples/tests function comments covered`
  - `python tools\verify\check_api_manual_coverage.py` -> `[manual-coverage] 135 API section(s) covered`
  - `python tools\verify\check_api_catalog_prototypes.py` -> `[api-catalog] 135 API prototype(s) aligned`
  - `python tools\verify\check_original_symbols.py` -> `[original-symbols] no banned FreeRTOS-style public symbols found`
  - `python tests\static\test_hardware_smoke_raw_log_schema.py` -> pass
  - `python tools\verify\check_hardware_smoke_raw_log_schema.py` -> `[hardware-raw-log-schema] schema fields aligned`
  - `python tools\verify\check_hardware_smoke_preflight.py` -> `[hardware-preflight] hardware smoke preflight config accepted`
  - `python tools\verify\check_embedded_smoke_projects.py` -> `[embedded-smoke] STM32 cross build and DSP model smoke passed`
  - `git diff --check` -> exit 0 with expected CRLF conversion warnings only
- Full default release verification passed:
  - `python tools\verify\run_release_verification.py` -> `[summary] 71 test target(s) passed`; `[release] 15 step(s) passed`
- Hardware-required gate remains intentionally failing until real boards are run:
  - `python tools\verify\check_hardware_smoke_evidence.py` -> missing `stm32_board_smoke.md` and `dsp_board_smoke.md`
  - `python tools\verify\run_release_verification.py --require-hardware` -> `[release] 1 step(s) failed` only at `hardware-smoke-evidence`
- Remaining: commit/push repo-side helper, then collect real STM32 and DSP board logs, retain matching raw logs, generate/fill final evidence files, and pass the hardware evidence gate.

## Hardware Smoke Complete Report Emitter
- Added RED host coverage:
  - `tests\unit\test_hardware_smoke_report.c` first failed because `examples\hardware_smoke\mrt_hardware_smoke_report.h` was missing.
- Added `examples\hardware_smoke\mrt_hardware_smoke_report.h`.
  - `MRT_SmokeCommonReport` captures common raw-log fields.
  - `MRT_SmokeStm32Report` adds STM32 fields.
  - `MRT_SmokeDspReport` adds DSP fields.
  - `MRT_SmokeEmitStm32Report()` and `MRT_SmokeEmitDspReport()` validate all required strings before output, then write the field set through the existing single-character writer.
- Wired `test_hardware_smoke_report` into `tools\run_host_tests.py` and `tests\CMakeLists.txt`.
- Initial GREEN check:
  - `python tools\run_host_tests.py` -> `[summary] 72 test target(s) passed`
- Updated manual, hardware smoke README, raw-log schema, STM32/DSP READMEs, final report, requirement matrix, coupling matrix, test-suite plan, completion audit, findings, progress, and task plan so the emitter is documented as field-set serialization only, not real-board evidence.
- Fresh focused checks passed:
  - `python tools\run_host_tests.py` -> `[summary] 72 test target(s) passed`
  - `python tools\verify\check_chinese_comments.py` -> `[chinese-comments] include/src/examples/tests function comments covered`
  - `python tools\verify\check_api_manual_coverage.py` -> `[manual-coverage] 135 API section(s) covered`
  - `python tools\verify\check_api_catalog_prototypes.py` -> `[api-catalog] 135 API prototype(s) aligned`
  - `python tools\verify\check_original_symbols.py` -> `[original-symbols] no banned FreeRTOS-style public symbols found`
  - `python tests\static\test_hardware_smoke_raw_log_schema.py` -> pass
  - `python tools\verify\check_hardware_smoke_raw_log_schema.py` -> `[hardware-raw-log-schema] schema fields aligned`
  - `python tools\verify\check_hardware_smoke_preflight.py` -> `[hardware-preflight] hardware smoke preflight config accepted`
  - `python tools\verify\check_embedded_smoke_projects.py` -> `[embedded-smoke] STM32 cross build and DSP model smoke passed`
  - `git diff --check` -> exit 0 with expected CRLF conversion warnings only
- Full default release verification passed:
  - `python tools\verify\run_release_verification.py` -> `[summary] 72 test target(s) passed`; `[release] 15 step(s) passed`
- Hardware-required gate remains intentionally failing until real boards are run:
  - `python tools\verify\check_hardware_smoke_evidence.py` -> missing `stm32_board_smoke.md` and `dsp_board_smoke.md`
  - `python tools\verify\run_release_verification.py --require-hardware` -> `[release] 1 step(s) failed` only at `hardware-smoke-evidence`
- Remaining: commit/push repo-side report emitter, then collect real STM32 and DSP board logs, retain matching raw logs, generate/fill final evidence files, and pass the hardware evidence gate.

## Hardware Smoke Report Schema Drift Coverage
- Started Phase 25 from a clean worktree on `feature/embedded-smoke-projects`.
- First `apply_patch` attempt targeted the parent `F:\My_RTOS` path instead of the active worktree and failed with file-not-found; repeated the patch against `.worktrees/embedded-smoke-projects`.
- RED test added to `tests\static\test_hardware_smoke_raw_log_schema.py`:
  - `REPORT_HEADER` must exist.
  - Every generator-required STM32/DSP field must map to a `MRT_SMOKE_FIELD_*` token referenced by `mrt_hardware_smoke_report.h`.
  - The checker itself must mention `mrt_hardware_smoke_report.h`.
- RED run:
  - `python tests\static\test_hardware_smoke_raw_log_schema.py` -> failed at `assert "mrt_hardware_smoke_report.h" in checker_text`.
- GREEN implementation:
  - `tools\verify\check_hardware_smoke_raw_log_schema.py` now loads `REPORT_HEADER`, checks artifact existence, derives field tokens with `field_macro_token()`, and rejects missing report emitter field tokens or missing STM32/DSP emit functions.
- Focused GREEN checks:
  - `python tests\static\test_hardware_smoke_raw_log_schema.py` -> pass.
  - `python tools\verify\check_hardware_smoke_raw_log_schema.py` -> `[hardware-raw-log-schema] schema fields aligned`.
- Follow-up RED/GREEN for release runner description:
  - `python tests\static\test_release_verification_runner.py` failed until `hardware-smoke-raw-log-schema` step description mentioned `完整报告 emitter`.
  - Updated `tools\verify\run_release_verification.py` description to say the schema step checks generator, docs, schema C header, and complete report emitter alignment.
  - `python tests\static\test_release_verification_runner.py` -> pass.
- Documentation sync completed for coupling matrix, test suite plan, requirements traceability, final report, completion audit, findings, progress, and task plan.
- Focused/static checks after docs sync passed:
  - `python tests\static\test_hardware_smoke_raw_log_schema.py`
  - `python tools\verify\check_hardware_smoke_raw_log_schema.py` -> `[hardware-raw-log-schema] schema fields aligned`
  - `python tools\verify\check_chinese_comments.py` -> `[chinese-comments] include/src/examples/tests function comments covered`
  - `python tools\verify\check_api_manual_coverage.py` -> `[manual-coverage] 135 API section(s) covered`
  - `python tools\verify\check_api_catalog_prototypes.py` -> `[api-catalog] 135 API prototype(s) aligned`
  - `python tools\verify\check_original_symbols.py` -> `[original-symbols] no banned FreeRTOS-style public symbols found`
  - `python tools\verify\check_hardware_smoke_preflight.py` -> `[hardware-preflight] hardware smoke preflight config accepted`
  - `git diff --check` -> exit 0 with expected CRLF conversion warnings only
- `python tools\run_host_tests.py` timed out once while running in parallel with embedded smoke at 244 seconds; reran it alone with a longer timeout and got `[summary] 72 test target(s) passed`.
- `python tools\verify\check_embedded_smoke_projects.py` -> `[embedded-smoke] STM32 cross build and DSP model smoke passed`.
- Full default release verification passed after the release runner description update:
  - `python tools\verify\run_release_verification.py` -> `[summary] 72 test target(s) passed`; `[release] 15 step(s) passed`.
- Hardware-required verification remains correctly gated:
  - `python tools\verify\check_hardware_smoke_evidence.py` -> missing `stm32_board_smoke.md` and `dsp_board_smoke.md`.
  - `python tools\verify\run_release_verification.py --require-hardware` -> `[release] 1 step(s) failed` only at `hardware-smoke-evidence`.

## Target-Scoped Hardware Smoke Preflight
- Continued Phase 26 on `feature/embedded-smoke-projects` with existing uncommitted code/docs changes.
- RED coverage already added to `tests\static\test_hardware_smoke_capture_runner.py`:
  - single-target execute with `STM32` must ignore a bad unselected DSP config.
  - `all` target execute must reject the same bad DSP config.
- GREEN implementation already present:
  - `tools\verify\check_hardware_smoke_preflight.py` supports target filtering and CLI `--target all|STM32|DSP`.
  - `tools\verify\run_hardware_smoke_capture.py` builds and runs target-specific preflight commands.
- Documentation sync updated manual, hardware smoke README/checklist, coupling matrix, test suite plan, requirements matrix, final verification report, and completion audit.
- Planning sync updated `task_plan.md`, `findings.md`, and this progress log.
- Next verification batch:
  - `python tests\static\test_hardware_smoke_capture_runner.py`
  - `python tools\verify\check_hardware_smoke_preflight.py`
  - `python tools\verify\check_hardware_smoke_preflight.py --target STM32`
  - `python tools\verify\check_hardware_smoke_preflight.py --target DSP`
  - `python tools\verify\run_hardware_smoke_capture.py --target STM32`
  - `python tools\verify\run_hardware_smoke_capture.py --target DSP`
  - `python tools\verify\run_release_verification.py`
  - hardware-required checks are expected to fail only because real STM32/DSP board evidence files are still absent.
- Focused checks passed:
  - `python tests\static\test_hardware_smoke_capture_runner.py`
  - `python tools\verify\check_hardware_smoke_preflight.py`
  - `python tools\verify\check_hardware_smoke_preflight.py --target STM32`
  - `python tools\verify\check_hardware_smoke_preflight.py --target DSP`
  - `python tools\verify\run_hardware_smoke_capture.py --target STM32`
  - `python tools\verify\run_hardware_smoke_capture.py --target DSP`
- Static checks passed:
  - `python tests\static\test_release_verification_runner.py`
  - `python tools\verify\check_api_manual_coverage.py` -> `[manual-coverage] 135 API section(s) covered`
  - `python tools\verify\check_chinese_comments.py` -> `[chinese-comments] include/src/examples/tests function comments covered`
  - `git diff --check` -> exit 0 with expected CRLF conversion warnings only
- Full default release verification passed:
  - `python tools\verify\run_release_verification.py` -> `[summary] 72 test target(s) passed`; `[release] 15 step(s) passed`
- Hardware-required gate remains intentionally failing until real boards are run:
  - `python tools\verify\check_hardware_smoke_evidence.py` -> missing `stm32_board_smoke.md` and `dsp_board_smoke.md`
  - `python tools\verify\run_release_verification.py --require-hardware` -> `[release] 1 step(s) failed` only at `hardware-smoke-evidence`

## Hardware Smoke Raw-Log Direct Checker
- Started Phase 27 from a clean worktree on `feature/embedded-smoke-projects`.
- RED tests added:
  - `tests\static\test_hardware_smoke_raw_log_checker.py` expects a raw UART/trace checker that validates raw logs before final evidence generation.
  - `tests\static\test_hardware_smoke_capture_runner.py` expects a `STM32.check-raw-log` step after capture.
  - `tests\static\test_release_verification_runner.py` expects a `hardware-smoke-raw-log-checker` release step.
- RED runs:
  - `python tests\static\test_hardware_smoke_raw_log_checker.py` -> failed with missing `check_hardware_smoke_raw_log.py`.
  - `python tests\static\test_hardware_smoke_capture_runner.py` -> failed on missing `check-raw-log` plan step.
  - `python tests\static\test_release_verification_runner.py` -> failed on missing release step.
- GREEN implementation:
  - Added `tools\verify\check_hardware_smoke_raw_log.py`.
  - It derives `Raw-Log-Path` and `Raw-Log-SHA256` from the raw log path, then reuses final evidence checker required-field, PASS, date, and numeric rules.
  - Updated `tools\verify\run_hardware_smoke_capture.py` so capture pipelines run `check-raw-log` after capture and before `generate-evidence`.
  - Added `hardware-smoke-raw-log-checker` to `tools\verify\run_release_verification.py`.
- Focused GREEN checks passed:
  - `python tests\static\test_hardware_smoke_raw_log_checker.py`
  - `python tests\static\test_hardware_smoke_capture_runner.py`
  - `python tests\static\test_release_verification_runner.py`
- Documentation sync updated manual STM32/DSP evidence sections, hardware smoke README/checklist, coupling matrix, test suite plan, requirements matrix, final verification report, completion audit, findings, progress, and task plan.
- Follow-up RED/GREEN:
  - `python tests\static\test_hardware_smoke_raw_log_schema.py` failed until `tools\verify\check_hardware_smoke_raw_log_schema.py` checked `check_hardware_smoke_raw_log.py` field coverage.
  - `tools\verify\check_hardware_smoke_raw_log.py` now lists common raw-log fields explicitly, and the schema checker validates those fields against generator requirements.
  - `python tests\static\test_release_verification_runner.py` failed until the raw-log schema release step description explicitly named `check_hardware_smoke_raw_log.py`.
- Focused/static verification passed:
  - `python tests\static\test_hardware_smoke_raw_log_schema.py`
  - `python tools\verify\check_hardware_smoke_raw_log_schema.py` -> `[hardware-raw-log-schema] schema fields aligned`
  - `python tests\static\test_hardware_smoke_raw_log_checker.py`
  - `python tests\static\test_hardware_smoke_capture_runner.py`
  - `python tests\static\test_release_verification_runner.py`
  - `python tools\verify\check_api_manual_coverage.py` -> `[manual-coverage] 135 API section(s) covered`
  - `python tools\verify\check_chinese_comments.py` -> `[chinese-comments] include/src/examples/tests function comments covered`
  - `python tools\verify\check_api_catalog_prototypes.py` -> `[api-catalog] 135 API prototype(s) aligned`
  - `python tools\verify\check_original_symbols.py` -> `[original-symbols] no banned FreeRTOS-style public symbols found`
  - `python tools\verify\check_hardware_smoke_preflight.py` -> `[hardware-preflight] hardware smoke preflight config accepted`
  - `python tools\verify\check_embedded_smoke_projects.py` -> `[embedded-smoke] STM32 cross build and DSP model smoke passed`
  - `git diff --check` -> exit 0 with expected CRLF conversion warnings only
- Full release verification passed:
  - `python tools\verify\run_release_verification.py` -> `[summary] 72 test target(s) passed`; `[release] 16 step(s) passed`
- Hardware-required gate remains intentionally failing until real boards are run:
  - `python tools\verify\check_hardware_smoke_evidence.py` -> missing `stm32_board_smoke.md` and `dsp_board_smoke.md`
  - `python tools\verify\run_release_verification.py --require-hardware` -> `[release] 1 step(s) failed` only at `hardware-smoke-evidence`
