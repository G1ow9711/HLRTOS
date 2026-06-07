# Task Plan: My_RTOS

## Goal
设计并实现一个原创的类 FreeRTOS 嵌入式 RTOS：适配 STM32 与 DSP，代码含详细中文注释，配套原创中文使用手册，并建立功能与耦合测试。

## Current Phase
Phase 4 extension: Task lifecycle API gap closure

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
- [ ] Implement portable layers and demos
  - [x] STM32 Cortex-M stack/tick/priority helper contract
  - [x] DSP C28x-style stack/software-interrupt helper contract
- [ ] Add detailed Chinese comments to every public and internal function
- [ ] Close preview API implementation gaps
  - [x] Task lifecycle APIs: dynamic create/delete, suspend/resume, delay-until, priority set, stack water mark
  - [ ] Dynamic synchronization/buffer APIs and runtime stats
- **Status:** in_progress

### Phase 5: Documentation
- [x] Write original Chinese user manual in FreeRTOS-like structure
  - [x] API reference manual coverage
  - [x] Detailed STM32 porting steps
  - [x] Detailed DSP porting steps
- [x] Document API, examples, detailed STM32/DSP porting steps, configuration, troubleshooting
- [x] Avoid verbatim FreeRTOS manual/template copying
- **Status:** complete

### Phase 6: Verification & Delivery
- [x] Run all host tests
- [x] Run coupling tests
- [x] Run static checks where available
- [x] Produce verification report
- **Status:** complete

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

## Errors Encountered
| Error | Attempt | Resolution |
|-------|---------|------------|
| `git status` failed: not a git repository | 1 | Logged repository state; proceed directly in project directory. |
| `git log` failed: not a git repository | 1 | Logged repository state; skip commit/worktree steps until repo exists. |
| PowerShell rejected `&&` command separator | 1 | Re-run git add and git commit as separate PowerShell commands. |
| `cmake` command not found | 1 | Verified GCC exists; added project-local Python host test runner as fallback while preserving CMake build files. |

## Notes
- Re-read this file before major design decisions.
- Update `findings.md` after research or discoveries.
- Update `progress.md` after meaningful work.
- Design approval required before implementation because brainstorming workflow hard-gates code/scaffold work.
