# MyRTOS C-Scope Master Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement the full approved C-scope MyRTOS objective: original FreeRTOS-like embedded RTOS for STM32 and DSP, with Chinese-documented C source, official-style Chinese manual, and full feature/coupling tests.

**Architecture:** Work proceeds as a series of independently verifiable subsystem plans. Each subsystem must update the requirement matrix, coupling matrix, API manual coverage, and final verification evidence before the next subsystem expands behavior.

**Tech Stack:** C99/C11, CMake, CTest, lightweight C test harness, ARM GNU Toolchain for STM32 examples, host port mock for scheduler and DSP abstraction, Python static verification scripts.

---

## Approved Inputs

- Design: `docs/superpowers/specs/2026-06-07-myrtos-c-scope-design.md`
- API catalog: `docs/api/myrtos_api_catalog.md`
- Requirement matrix: `docs/verification/requirements_traceability_matrix.md`
- Coupling matrix: `docs/verification/coupling_test_matrix.md`
- Test suite plan: `docs/verification/test_suite_plan.md`

## Plan Series

### Plan 1: Foundation Kernel Infrastructure

**Plan file:** `docs/superpowers/plans/2026-06-07-myrtos-foundation-kernel.md`

**Deliverables:**

- CMake/CTest host test harness.
- Core public types and result codes.
- Config defaults.
- Intrusive list.
- Priority bitmap.
- Mock port abstraction.
- Kernel tick counter shell.

**Requirement coverage:** Starts `R-002`, `R-008`, `R-009`, and enables `R-005`/`R-006` static scanning later.

**Coupling coverage enabled:** Prepares `C-001` through `C-003`, `C-028` through `C-031`.

### Plan 2: Task Scheduler Core

**Plan file to create after Plan 1 passes:** `docs/superpowers/plans/2026-06-07-myrtos-task-scheduler.md`

**Deliverables:**

- `MRT_TaskCreateStatic`, `MRT_TaskDelay`, `MRT_KernelStart`, `MRT_KernelYield`.
- Ready lists, delayed lists, task states, priority-based selection.
- Scheduler simulation tests for preemption, round-robin, delay expiry, tick overflow.

**Requirement coverage:** Expands `R-002`, `R-008`, `R-009`.

**Coupling coverage:** `C-001`, `C-002`, `C-003`.

### Plan 3: Queues and Blocking Objects

**Plan file to create after Plan 2 passes:** `docs/superpowers/plans/2026-06-07-myrtos-queues.md`

**Deliverables:**

- Static and dynamic queue creation.
- Send/receive/peek/overwrite/reset.
- Task and ISR variants.
- Queue wait lists integrated with scheduler.

**Coupling coverage:** `C-004`, `C-005`, `C-006`, `C-007`.

### Plan 4: Semaphores and Mutexes

**Plan file to create after Plan 3 passes:** `docs/superpowers/plans/2026-06-07-myrtos-semaphore-mutex.md`

**Deliverables:**

- Binary semaphore, counting semaphore.
- Mutex and recursive mutex.
- Priority inheritance and rollback.
- Illegal ISR context enforcement.

**Coupling coverage:** `C-008` through `C-013`, `C-027`.

### Plan 5: Events and Task Notifications

**Plan file to create after Plan 4 passes:** `docs/superpowers/plans/2026-06-07-myrtos-events-notifications.md`

**Deliverables:**

- Event groups with wait-any, wait-all, exit clear.
- Task notification actions: set bits, increment, overwrite, no-overwrite.
- ISR notification paths.

**Coupling coverage:** `C-014`, `C-015`, `C-016`.

### Plan 6: Timers

**Plan file to create after Plan 5 passes:** `docs/superpowers/plans/2026-06-07-myrtos-timers.md`

**Deliverables:**

- Timer service task.
- Timer command queue.
- One-shot and periodic timers.
- Pending function calls.

**Coupling coverage:** `C-017`, `C-018`.

### Plan 7: Stream and Message Buffers

**Plan file to create after Plan 6 passes:** `docs/superpowers/plans/2026-06-07-myrtos-stream-message-buffers.md`

**Deliverables:**

- Stream buffer with trigger level.
- Message buffer preserving packet boundaries.
- ISR send/receive variants.

**Coupling coverage:** `C-020`, `C-021`, `C-022`.

### Plan 8: Memory Managers

**Plan file to create after Plan 7 passes:** `docs/superpowers/plans/2026-06-07-myrtos-memory.md`

**Deliverables:**

- Static allocation patterns for every object type.
- Linear heap, free-list heap, coalescing heap, fixed block pool.
- Allocation failure tests across object creation.

**Coupling coverage:** `C-007`, `C-023`, `C-024`.

### Plan 9: Tickless, Trace, Stats, Assertions

**Plan file to create after Plan 8 passes:** `docs/superpowers/plans/2026-06-07-myrtos-tickless-trace-stats.md`

**Deliverables:**

- Tickless idle decision and compensation.
- Trace hook sink.
- Runtime stats.
- Assertion hook and illegal context behavior.

**Coupling coverage:** `C-019`, `C-025`, `C-026`, `C-027`.

### Plan 10: STM32 and DSP Ports

**Plan file to create after Plan 9 passes:** `docs/superpowers/plans/2026-06-07-myrtos-ports.md`

**Deliverables:**

- STM32 Cortex-M4/M7 port skeleton with SysTick, PendSV, SVC, stack frame initialization.
- DSP C2000-style abstraction with mock and documented ABI assumptions.
- Buildable examples.

**Coupling coverage:** `C-028`, `C-029`, `C-030`, `C-031`.

### Plan 11: Manual and Static Verification

**Plan file to create after Plan 10 passes:** `docs/superpowers/plans/2026-06-07-myrtos-manual-verification.md`

**Deliverables:**

- `docs/manual/MyRTOS_Reference_Manual_zh.md`.
- API manual coverage script.
- Chinese comment coverage script.
- Originality symbol scan script.

**Coupling coverage:** `C-032`, `C-033`.

### Plan 12: Final Verification Report

**Plan file to create after Plan 11 passes:** `docs/superpowers/plans/2026-06-07-myrtos-final-verification.md`

**Deliverables:**

- `docs/verification/final_verification_report.md`.
- Requirement matrix status changed from design/in-progress to verified where evidence exists.
- Coupling matrix status changed to verified where tests pass.
- Full command output references.

## Execution Rule

Implement one plan at a time. Do not start a later plan until the previous plan has:

- Fresh passing build/test output.
- Updated requirement matrix evidence.
- Updated coupling matrix evidence.
- Updated manual/API coverage where relevant.
- Git commit containing only that plan's work.

