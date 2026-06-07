# Task Plan: My_RTOS

## Goal
设计并实现一个原创的类 FreeRTOS 嵌入式 RTOS：适配 STM32 与 DSP，代码含详细中文注释，配套原创中文使用手册，并建立功能与耦合测试。

## Current Phase
Phase 3: Implementation Plan

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
- [ ] Create detailed TDD implementation plan
- [ ] Define exact files, APIs, and test cases
- [ ] Confirm user approval before implementation
- **Status:** in_progress

### Phase 4: TDD Implementation
- [ ] Write failing tests before production code
- [ ] Implement kernel core incrementally
- [ ] Implement portable layers and demos
- [ ] Add detailed Chinese comments to every public and internal function
- **Status:** pending

### Phase 5: Documentation
- [ ] Write original Chinese user manual in FreeRTOS-like structure
- [ ] Document API, examples, porting guide, configuration, troubleshooting
- [ ] Avoid verbatim FreeRTOS manual/template copying
- **Status:** pending

### Phase 6: Verification & Delivery
- [ ] Run all host tests
- [ ] Run coupling tests
- [ ] Run static checks where available
- [ ] Produce verification report
- **Status:** pending

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

## Errors Encountered
| Error | Attempt | Resolution |
|-------|---------|------------|
| `git status` failed: not a git repository | 1 | Logged repository state; proceed directly in project directory. |
| `git log` failed: not a git repository | 1 | Logged repository state; skip commit/worktree steps until repo exists. |
| PowerShell rejected `&&` command separator | 1 | Re-run git add and git commit as separate PowerShell commands. |

## Notes
- Re-read this file before major design decisions.
- Update `findings.md` after research or discoveries.
- Update `progress.md` after meaningful work.
- Design approval required before implementation because brainstorming workflow hard-gates code/scaffold work.
