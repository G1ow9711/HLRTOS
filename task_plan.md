# Task Plan: My_RTOS

## Goal
设计并实现一个原创的类 FreeRTOS 嵌入式 RTOS：适配 STM32 与 DSP，代码含详细中文注释，配套原创中文使用手册，并建立功能与耦合测试。

## Current Phase
Phase 26 repo-side complete: hardware smoke capture/preflight supports target-scoped validation; overall goal still awaits real STM32/DSP board logs

## Phases

### Phase 1: Requirements & Discovery
- [x] Capture user goal
- [x] Check project state
- [x] Clarify scope direction: user selected C, broad FreeRTOS-like feature scope
- [x] Set provisional target chips/toolchains and port priority
- [x] Document FreeRTOS reference findings without copying protected text/code
- **Status:** complete

### Phase 2: Architecture Spec
- [x] Define RTOS module set and kernel contract
- [x] Define portable CPU abstraction for STM32 Cortex-M and DSP
- [x] Define tests: host unit tests, kernel simulation tests, coupling tests, embedded smoke tests
- [x] Write approved design spec under docs/
- **Status:** complete

### Phase 3: Implementation Plan
- [x] Create detailed TDD implementation plan
- [x] Define exact files, APIs, and test cases
- [x] Confirm user approval before implementation
- **Status:** complete

### Phase 4: TDD Implementation
- [x] Write failing tests before production code
- [x] Implement foundation kernel core incrementally
- [x] Implement task scheduler core incrementally
- [x] Implement portable layers and demos
  - [x] STM32 Cortex-M stack/tick/priority helper contract
  - [x] DSP C28x-style stack/software-interrupt helper contract
- [x] Add detailed Chinese comments to every public and internal function
- [ ] Close preview API implementation gaps
  - [x] Task lifecycle APIs: dynamic create/delete, suspend/resume, delay-until, priority set, stack water mark
  - [x] Dynamic synchronization/buffer APIs
  - [x] Runtime stats API: task runtime tick accounting
- **Status:** complete

### Phase 5: Documentation
- [x] Write original Chinese user manual in FreeRTOS-like structure
  - [x] API reference manual coverage
  - [x] Detailed STM32 porting steps, minimal integration skeleton, and acceptance checklist
  - [x] Detailed DSP porting steps, minimal integration skeleton, and acceptance checklist
- [x] Document API, examples, detailed STM32/DSP porting steps, configuration, troubleshooting
- [x] Avoid verbatim FreeRTOS manual/template copying
- **Status:** complete

### Phase 6: Verification & Delivery
- [x] Run all host tests
- [x] Run coupling tests
- [x] Run static checks where available
- [x] Produce verification report
- [x] Close mutex timeout priority rollback coupling (`C-011`)
- [x] Close held-mutex task deletion policy coupling (`C-012`)
- [x] Close timer service command queue and callback service coupling (`C-017`, `C-018`)
- **Status:** complete

### Phase 7: Embedded Smoke Projects
- [x] Add a failing smoke/build check for missing `examples/stm32/` and `examples/dsp/`
- [x] Add STM32 smoke project scaffold and ARM GCC build path
- [x] Add DSP smoke/model project scaffold and host-verifiable build path
- [x] Expand manual porting steps with explicit example wiring and build commands
- [x] Update requirement/coupling matrices and final report for smoke evidence
- **Status:** complete

### Phase 8: STM32 MPU Helper
- [x] Add a failing test for STM32 MPU region layout helper
- [x] Implement host-verifiable MPU layout normalization API
- [x] Update manual/API catalog and verification matrices
- [x] Re-run host/static/smoke verification
- **Status:** complete

### Phase 9: Dynamic Buffer Delete Lifecycle
- [x] Add failing coverage for dynamic stream/message buffer delete APIs
- [x] Implement `MRT_StreamBufferDelete` and `MRT_MessageBufferDelete`
- [x] Update API catalog and Chinese manual sections
- [x] Re-run host/static/smoke verification
- **Status:** complete

### Phase 10: Buffer Writer Wait Coupling and API Prototype Audit
- [x] Add failing coverage for stream/message buffer writer wait paths that must block dynamic deletion while waiters exist
- [x] Implement `MRT_TASK_WAIT_REASON_STREAM_SEND`, `MRT_TASK_WAIT_REASON_MESSAGE_SEND`, and per-task `object_wait_bytes`
- [x] Wake blocked buffer writers when receive/reset releases enough space
- [x] Add `tools/verify/check_api_catalog_prototypes.py` to verify API catalog, headers, and source definitions align
- [x] Add missing catalog/manual coverage for `MRT_TimerGetName` and `MRT_MemoryPoolGetFreeCount`
- [x] Update requirement/coupling/final audit evidence and re-run host/static/smoke verification
- **Status:** complete

### Phase 11: Hardware Smoke Evidence Gate
- [x] Validate `tests/static/test_hardware_smoke_evidence_checker.py`
- [x] Improve `tools/verify/check_hardware_smoke_evidence.py` so missing required fields do not hide present-but-failing status/numeric fields
- [x] Add `docs/verification/hardware_smoke/README.md`
- [x] Add STM32 and DSP real-board evidence templates under `docs/verification/hardware_smoke/`
- [x] Expand manual STM32/DSP porting chapters with real-board evidence archival steps
- [x] Update verification plan, final report, completion audit, and requirement matrix to reference the hardware evidence gate
- [x] Re-run host/static/smoke verification
- [ ] Replace templates with real `stm32_board_smoke.md` and `dsp_board_smoke.md` after actual board runs
- **Status:** complete for repo-side gate/template work; real hardware evidence remains pending

### Phase 12: Hardware Smoke Capture Checklist
- [x] Add `docs/verification/hardware_smoke/collection_checklist.md`
- [x] Document STM32 evidence capture steps: build, flash, tick, PendSV/SVC, ISR queue, critical section, timer, tickless, heap, assert
- [x] Document DSP evidence capture steps: ABI, stack, timer tick, software interrupt switch, ISR nesting, queue/pool, timer, tickless, heap, assert
- [x] Link the checklist from the manual, verification plan, completion audit, final report, and requirement matrix
- [x] Re-run manual/static checks after documentation updates
- [ ] Replace templates with real `stm32_board_smoke.md` and `dsp_board_smoke.md` after actual board runs
- **Status:** complete for repo-side guide/docs; real hardware evidence remains pending

### Phase 13: Unified Release Verification Runner
- [x] Add `tools/verify/run_release_verification.py`
- [x] Add `tests/static/test_release_verification_runner.py`
- [x] Make the default release path run host, static, embedded smoke, and hardware checker self-test steps
- [x] Make `--require-hardware` append the real STM32/DSP board evidence gate
- [x] Update the manual and verification docs to point at the unified release entrypoint
- [x] Run the default release verification successfully
- [x] Run the hardware-required release verification and confirm it fails only on missing real board logs
- **Status:** complete for repo-side wrapper/docs; real hardware evidence remains pending

## Key Questions
1. Which first target should drive the port: STM32 Cortex-M3/M4/M7, Cortex-M0/M0+, or a specific DSP family?
2. Should first release be a compact teaching/industrial kernel, or a broad FreeRTOS-like feature clone?
3. Which toolchains must build: GCC/ARM GNU, Keil MDK, IAR, TI C2000, ADI, or others?
4. Which FreeRTOS feature families are mandatory: tasks, queues, semaphores, timers, event groups, stream/message buffers, memory heaps, tickless idle, trace hooks, MPU/SMP?
5. What test environment is available for embedded smoke tests: QEMU, STM32 board, DSP board, or host-only first?

## Decisions Made
| Decision | Rationale |
|----------|-----------|
| Original implementation only | FreeRTOS code and manuals are copyrighted/open-source licensed; project can learn architecture but must not copy protected source/manual text. |
| Use planning files | Task is large and must persist across many phases. |
| No git worktree now | `F:\My_RTOS` is not a Git repository. |
| Use broad C scope | User selected C: broad FreeRTOS-like functionality including advanced modules and wider verification. |
| Provisional platform | Until user gives exact hardware, use STM32 Cortex-M4/M7 + ARM GCC/CMake + TI C2000-style DSP abstraction. |
| Add verification matrices before code | User requires all features and coupling cases tested; matrices make completion auditable. |
| User approved design | User replied "批准"; proceed to implementation planning and Git baseline. |
| Split C implementation into plan series | Full C scope covers many subsystems; master plan plus subsystem plans keeps verification auditable. |
| Autonomous technical decisions | User instructed Codex to choose the best direction, consult FreeRTOS source when unclear, and redesign rather than copy. |
| Host test fallback | Local environment has GCC but no CMake; keep CMake files for standard environments and use `tools/run_host_tests.py` for current host verification. |
| Task lifecycle gap first | API catalog vs C source audit found 21 missing catalog APIs; start with 8 task lifecycle APIs because they unblock scheduler, heap, ISR, and manual consistency evidence. |
| Dynamic object APIs second | After task lifecycle closure, 13 gaps remain. This branch targets 12 dynamic object APIs and leaves runtime statistics as the final source/catalog API gap. |
| Heap exhaustion test helper | Free-list/coalescing heaps need block headers, so failure-path tests must exhaust heap by repeated smaller allocations instead of requesting the full reported free size at once. |
| Runtime stats tick model | Current portable preview records task runtime in kernel ticks; high-resolution STM32/DSP counters remain a port enhancement without changing `MRT_StatsGetTaskRuntime`. |
| Porting manual detail | STM32/DSP manual chapters now include concrete migration steps, handler skeletons, smoke-test guidance, troubleshooting, and acceptance checklists. |
| Mutex timeout rollback | When a mutex waiter times out, the owner effective priority is recalculated from remaining waiters and restored to base priority if no higher waiter remains. |
| Held mutex deletion policy | `MRT_TaskDelete` rejects deletion of a task that still owns a mutex, preserving owner, waiters, and task state until the application releases the lock explicitly. |
| Timer service command queue | Timer control APIs enqueue service commands; tick expiry enqueues callback events; `MRT_TimerServiceRunPending` drains commands, callbacks, and pending functions in FIFO order. |
| Buffer writer wait model | Stream buffer write wait records 1 byte because partial send is allowed; message buffer write wait records full record length (`4 + payload`) so wakeup only happens when a whole message can fit. |
| API prototype verifier | Public API catalog must match public headers and source definitions; test-only `MRT_PortMock*`/`MRT_KernelTest*`, list primitives, priority bitmap helpers, and `MRT_ASSERT` macro are handled explicitly. |
| Hardware evidence templates | Do not commit fake PASS board logs. Keep templates under `docs/verification/hardware_smoke/` and require real `stm32_board_smoke.md` / `dsp_board_smoke.md` before final hardware completion. |
| Hardware capture checklist | Keep real-board collection steps separate from templates so users capture UART/trace proof first, then fill evidence files. |
| TCB stack-top contract | Task creation initializes `stack_top`, STM32 smoke task setup writes the port-initialized PSP back into each TCB, scheduler records the task being switched out, and STM32 PendSV smoke hook uses `MRT_TaskKernelSwitchStackTop()` to save old PSP and return current PSP. |
| Hardware raw-log schema contract | The generator constants, `raw_log_schema.md`, `mrt_hardware_smoke_log_schema.h`, capture guides, and release runner must stay aligned so real STM32/DSP board logs do not miss required fields. |
| Hardware smoke log output helper | Board code may use `mrt_hardware_smoke_log.h` with a single-character UART/SWO/trace callback to output schema-compliant `Key: Value` lines without `printf`; the helper formats logs only and never decides PASS/FAIL. |
| Hardware smoke complete report emitter | Board code may use `mrt_hardware_smoke_report.h` to validate and output STM32/DSP required field sets in one call, reducing real-board raw-log omissions while still leaving PASS/FAIL decisions to board tests. |
| Report emitter schema coverage | `check_hardware_smoke_raw_log_schema.py` must inspect `mrt_hardware_smoke_report.h` so generator field additions cannot leave the complete report emitter stale. |
| Target-scoped hardware preflight | Single-target smoke capture should validate only the selected STM32 or DSP target so one board can be brought up while the other target configuration is still incomplete; `all` remains strict across both targets. |

## Errors Encountered
| Error | Attempt | Resolution |
|-------|---------|------------|
| `git status` failed: not a git repository | 1 | Logged repository state; proceed directly in project directory. |
| `git log` failed: not a git repository | 1 | Logged repository state; skip commit/worktree steps until repo exists. |
| PowerShell rejected `&&` command separator | 1 | Re-run git add and git commit as separate PowerShell commands. |
| `cmake` command not found | 1 | Verified GCC exists; added project-local Python host test runner as fallback while preserving CMake build files. |
| Dynamic object GREEN first run failed: `filler != 0` | 1 | Replaced full-free-size allocation with a repeated heap exhaustion helper in dynamic allocation tests. |
| Timer service GREEN first run failed four legacy tests | 1 | Updated timer control, timer expiry, and tickless tests to drain `MRT_TimerServiceRunPending` before expecting queued commands or callbacks to take effect. |
| Parallel full verification timed out on host tests | 1 | Static/smoke checks completed; reran `python tools\run_host_tests.py` alone with longer timeout and confirmed `[summary] 69 test target(s) passed`. |
| Hardware evidence checker static test failed because early missing-field return hid `Evidence-Status: FAIL` | 1 | Removed the early return after required-field checks so present failing fields and numeric bounds are still reported. |

## Notes
- Re-read this file before major design decisions.
- Update `findings.md` after research or discoveries.
- Update `progress.md` after meaningful work.
- Design approval required before implementation because brainstorming workflow hard-gates code/scaffold work.

## Phase 14: Manual Porting Detail Refresh and Evidence Generator Integration
- [x] Expand STM32 Cortex-M migration steps with vendor-project bring-up order, handler wiring, and first-board validation sequence
- [x] Expand DSP migration steps with ABI, startup, timer ISR, software-interrupt, and nested-ISR bring-up order
- [x] Add raw-log-to-evidence generator usage to the manual and hardware smoke docs
- [x] Add generator test to the unified release verification runner
- [x] Re-run manual coverage, generator test, release verification, and hardware-required release verification
- **Status:** complete for repo-side docs/tooling; real STM32/DSP evidence remains pending

## Phase 15: STM32 Cortex-M SVC/PendSV Assembly Scaffold
- [x] Add failing static coverage for missing STM32 context assembly scaffold
- [x] Add `src/portable/stm32_cm/mrt_port_stm32_cm_context.S` with SVC, PendSV, and first-task-start symbols
- [x] Add smoke C hooks for SVC/PendSV diagnostic handoff
- [x] Include the assembly file in the ARM GCC embedded smoke build
- [x] Add `stm32-context-scaffold` to the default release verification runner
- [x] Update STM32 manual porting steps and verification evidence docs
- [ ] Replace scaffold proof with real STM32 board runtime evidence after actual hardware smoke
- **Status:** complete for compile/static scaffold; real STM32 board evidence remains pending

## Phase 16: Task Runtime Stack-Top Contract and STM32 PendSV Hook Wiring
- [x] Add failing host coverage for task `stack_top` initialization, invalid helper inputs, and scheduler/PendSV save-return contract
- [x] Add `MRT_Task.stack_top` plus internal `MRT_TaskKernelGetStackTop`, `MRT_TaskKernelSetStackTop`, and `MRT_TaskKernelSwitchStackTop`
- [x] Record the task being switched out so PendSV can save the old PSP into the correct TCB
- [x] Wire `examples/stm32/MRT_PortStm32CmPendSvHook()` to `MRT_TaskKernelSwitchStackTop()`
- [x] Wire STM32 smoke task creation through `MRT_PortStm32CmInitializeStack()` and `MRT_TaskKernelSetStackTop()`
- [x] Add static coverage requiring the STM32 hook and embedded smoke build to include the internal stack-top contract
- [x] Update STM32 manual porting steps and verification docs
- [ ] Replace cross-build PSP/TCB proof with real STM32 board runtime evidence after actual hardware smoke
- **Status:** complete for host/static/cross-build contract; real STM32 board evidence remains pending

## Phase 17: Hardware Smoke Preflight Configuration
- [x] Add failing static coverage for hardware smoke preflight config validation
- [x] Add `tools/verify/check_hardware_smoke_preflight.py`
- [x] Add `docs/verification/hardware_smoke/hardware_smoke_preflight.json`
- [x] Validate STM32/DSP targets, board/chip metadata, command fields, runtime duration, expected log fields, and target-specific STM32/DSP settings
- [x] Add optional `--check-tools` executable lookup without making repo-side release verification depend on local TI/OpenOCD tools
- [x] Add `hardware-smoke-preflight` to the default release verification runner
- [x] Update manual STM32/DSP evidence sections, hardware smoke docs, verification report, completion audit, requirement matrix, coupling matrix, and test suite plan
- [x] Re-run full release verification after documentation sync
- [ ] Replace templates with real `stm32_board_smoke.md` and `dsp_board_smoke.md` after actual board runs
- **Status:** complete for repo-side preflight tooling; real hardware evidence remains pending

## Phase 18: Hardware Evidence Raw-Log Traceability
- [x] Add failing generator coverage requiring final evidence to include `Raw-Log-Path` and `Raw-Log-SHA256`
- [x] Add failing checker coverage requiring SHA-256 mismatch to be rejected
- [x] Update `tools/verify/generate_hardware_smoke_evidence.py` to derive raw log path and SHA-256 from `--input`
- [x] Update `tools/verify/check_hardware_smoke_evidence.py` to require raw log traceability fields and compare SHA-256 against the retained raw log
- [x] Update STM32/DSP hardware evidence templates and docs to require raw log path/hash
- [x] Re-run release verification and expected hardware-gate failure after documentation sync
- [ ] Replace templates with real `stm32_board_smoke.md`, `dsp_board_smoke.md`, and matching raw logs after actual board runs
- **Status:** complete for repo-side raw-log traceability; real hardware evidence remains pending

## Phase 19: Hardware Smoke Capture Runner
- [x] Add failing static coverage for a hardware smoke capture runner and release-runner integration
- [x] Add `tools/verify/run_hardware_smoke_capture.py` with default dry-run and explicit `--execute`
- [x] Orchestrate preflight, compiler/build/flash/capture commands, raw-log evidence generation, and target-level evidence validation
- [x] Add `hardware-smoke-capture-runner` to the default release verification runner
- [x] Update manual, hardware smoke docs, requirements matrix, coupling matrix, test-suite plan, completion audit, and final report
- [x] Re-run focused/static/full release verification after documentation sync
- [ ] Replace templates with real `stm32_board_smoke.md`, `dsp_board_smoke.md`, and matching raw logs after actual board runs
- **Status:** complete for repo-side capture runner; real hardware evidence remains pending

## Phase 20: DSP C28x Context Assembly Scaffold
- [x] Add failing static coverage for missing DSP C28x context assembly scaffold
- [x] Add `src/portable/dsp_c28x/mrt_port_dsp_c28x_context.asm` with first-task, yield, software-interrupt switch, save/restore markers, and C hook handoff symbols
- [x] Add `dsp-context-scaffold` to the default release verification runner
- [x] Update DSP README, manual, requirement matrix, coupling matrix, test-suite plan, completion audit, and final report
- [x] Re-run focused/static/full release verification after documentation sync
- [ ] Replace scaffold proof with real DSP board runtime evidence after actual hardware smoke
- **Status:** complete for repo-side DSP scaffold; real DSP board evidence remains pending

## Phase 21: DSP C2000 Board Smoke Project Scaffold
- [x] Add failing static coverage for C2000 startup, board glue, linker command, README boundary, and release-runner integration
- [x] Add `examples/dsp/startup_c28x.c`, `examples/dsp/mrt_port_dsp_c2000_smoke.c`, and `examples/dsp/linker_c28x.cmd`
- [x] Add `dsp-c2000-project-scaffold` to the default release verification runner
- [x] Add C2000 scaffold files to embedded smoke required-path checks
- [x] Update manual, requirement matrix, coupling matrix, test-suite plan, completion audit, final report, findings, and progress
- [x] Re-run focused/static/full release verification after documentation sync
- [ ] Replace scaffold proof with real DSP board runtime evidence after actual hardware smoke
- **Status:** complete for repo-side C2000 scaffold; real DSP board evidence remains pending

## Phase 22: Hardware Smoke Raw-Log Schema Contract
- [x] Add failing static coverage for missing raw-log schema docs, example C field constants, schema checker, guide links, and release-runner integration
- [x] Add `docs/verification/hardware_smoke/raw_log_schema.md`
- [x] Add `examples/hardware_smoke/mrt_hardware_smoke_log_schema.h`
- [x] Add `tools/verify/check_hardware_smoke_raw_log_schema.py`
- [x] Add `hardware-smoke-raw-log-schema` to the default release verification runner
- [x] Update hardware smoke README/checklist, STM32/DSP READMEs, manual, verification report, requirement matrix, coupling matrix, test-suite plan, completion audit, findings, and progress
- [x] Re-run focused/static/full release verification after documentation sync
- [ ] Replace templates with real `stm32_board_smoke.md`, `dsp_board_smoke.md`, and matching raw logs after actual board runs
- **Status:** complete for repo-side schema contract; real hardware evidence remains pending

## Phase 23: Hardware Smoke Log Output Helper
- [x] Add failing host coverage for a board-side `Key: Value` log output helper
- [x] Add `examples/hardware_smoke/mrt_hardware_smoke_log.h`
- [x] Add `tests/unit/test_hardware_smoke_log.c` and wire it into `tools/run_host_tests.py` plus `tests/CMakeLists.txt`
- [x] Verify the helper writes exact string fields, decimal `uint32_t` fields, and no partial line on invalid arguments
- [x] Update manual, hardware smoke docs, STM32/DSP READMEs, requirement matrix, coupling matrix, test-suite plan, completion audit, final report, findings, and progress
- [x] Re-run focused/static/full release verification after documentation sync
- [ ] Replace templates with real `stm32_board_smoke.md`, `dsp_board_smoke.md`, and matching raw logs after actual board runs
- **Status:** complete for repo-side helper; real hardware evidence remains pending

## Phase 24: Hardware Smoke Complete Report Emitter
- [x] Add failing host coverage for STM32/DSP complete raw-log report output
- [x] Add `examples/hardware_smoke/mrt_hardware_smoke_report.h`
- [x] Add `tests/unit/test_hardware_smoke_report.c` and wire it into `tools/run_host_tests.py` plus `tests/CMakeLists.txt`
- [x] Verify STM32/DSP required field sets are emitted and invalid arguments produce no partial report
- [x] Update manual, hardware smoke docs, STM32/DSP READMEs, requirement matrix, coupling matrix, test-suite plan, completion audit, final report, findings, and progress
- [x] Re-run focused/static/full release verification after documentation sync
- [ ] Replace templates with real `stm32_board_smoke.md`, `dsp_board_smoke.md`, and matching raw logs after actual board runs
- **Status:** complete for repo-side report emitter; real hardware evidence remains pending

## Phase 25: Hardware Smoke Report Schema Drift Coverage
- [x] Add failing static coverage requiring `test_hardware_smoke_raw_log_schema.py` to include `mrt_hardware_smoke_report.h`
- [x] Update `tools/verify/check_hardware_smoke_raw_log_schema.py` to validate report header field constants against generator-required STM32/DSP fields
- [x] Update verification docs, requirement matrix, coupling matrix, test-suite plan, completion audit, final report, findings, and progress
- [x] Re-run focused/static/full release verification after documentation sync
- [ ] Replace templates with real `stm32_board_smoke.md`, `dsp_board_smoke.md`, and matching raw logs after actual board runs
- **Status:** complete for repo-side schema/report drift coverage; real hardware evidence remains pending

## Phase 26: Target-Scoped Hardware Smoke Preflight
- [x] Add failing capture-runner coverage proving `--target STM32 --execute` ignores invalid DSP config, while `--target all` still rejects it
- [x] Add target filtering to `check_hardware_smoke_preflight.py`
- [x] Wire capture runner preflight steps to pass the selected target into the preflight checker and dry-run command text
- [x] Update manual, hardware smoke docs, requirement matrix, coupling matrix, test-suite plan, completion audit, final report, findings, and progress
- [x] Re-run focused/static/full release verification after documentation sync
- [ ] Replace templates with real `stm32_board_smoke.md`, `dsp_board_smoke.md`, and matching raw logs after actual board runs
- **Status:** complete for repo-side target-scoped capture/preflight; real hardware evidence remains pending
