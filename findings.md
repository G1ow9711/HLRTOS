# Findings & Decisions

## Requirements
- Build an embedded RTOS similar in spirit to FreeRTOS.
- Support STM32 and DSP targets.
- Code must include professional, detailed Chinese comments.
- Required comment coverage:
  - every function purpose
  - input parameters
  - return value
  - call example
  - function-internal line-by-line comments
- Write a Chinese official-style user manual.
- Manual should follow a FreeRTOS-like usage-manual structure, but must be original text and layout expression.
- All features must be tested.
- Coupling and interaction cases must be tested clearly.

## Project Discovery
- Project root: `F:\My_RTOS`
- Existing contents: `.codex-local` only.
- Git status: not a Git repository.
- No existing source, docs, tests, build files, or FreeRTOS source found in project root.

## Research Findings
- Official FreeRTOS documentation groups public API material around task/scheduler, queues, semaphores/mutexes, software timers, event groups, direct task notifications, stream buffers, message buffers, and memory management.
- Official FreeRTOS source organization is intentionally small at kernel core level: common kernel logic is centered around task scheduling, queues, and list management, with optional modules for timers, event groups, co-routines, stream/message buffers, and portable memory managers.
- FreeRTOS-Kernel GitHub repository separates common source, public headers, portable CPU/compiler code, and memory-management implementations.
- FreeRTOS-Kernel GitHub page showed latest release `V11.3.0` dated 2026-03-30 during this session.
- FreeRTOS reference manual structure includes an "About this manual" section, API usage restrictions, chapterized API groups, prototypes, summaries, parameters, return values, notes, examples, and appendices for types/macros.
- FreeRTOS may be used as architectural inspiration, but direct copying of source code or manual text is not acceptable.
- FreeRTOS `tasks.c` uses per-priority ready lists, a current TCB pointer, delayed task lists, pending-ready handling while scheduling is suspended, and delayed-list overflow handling. MyRTOS scheduler will borrow the concepts of priority-indexed ready queues and explicit blocked/ready transitions, but will use original `MRT_` APIs, original structures, and a signed tick-difference wake check rather than copying FreeRTOS implementation text or macros.
- FreeRTOS source repository confirms common kernel files include task, queue, and list modules. MyRTOS keeps the same broad separation of concerns but uses original file names and interfaces.
- FreeRTOS `queue.c` uses queue objects that hold storage pointers, item size/count metadata, send/receive positions, message counters, and separate task wait lists for senders/receivers. MyRTOS queue will borrow the high-level concepts of fixed-size copy queues, circular storage, and separate blocked sender/receiver lists, but will use original `MRT_Queue` fields, result codes, and API behavior.

## Technical Decisions
| Decision | Rationale |
|----------|-----------|
| Treat this as greenfield RTOS project | Workspace is empty except `.codex-local`. |
| Use broad C scope | User selected C: implement a broad FreeRTOS-like feature set rather than a compact first release. |
| Still require staged delivery inside C | Full kernel, ports, manual, and exhaustive coupling tests need module phases to keep verification meaningful. |
| Use TDD for implementation | User requires all functions and coupling cases tested; TDD skill also requires failing tests before production code. |
| Provisional target platform | No exact chip/toolchain answer yet; proceed with STM32 Cortex-M4/M7 + ARM GCC/CMake + TI C2000-style DSP abstraction as reasonable default. |
| API prefix `MRT_` proposed | Avoid confusion with FreeRTOS symbols and preserve originality. |
| Autonomous decision mode | User instructed best-direction decisions without stopping for routine clarification; FreeRTOS may be consulted as reference only. |
| Host build fallback | Current machine has GCC 13.1.0 but no `cmake`; use Python script to compile/run host tests until CMake is installed. |

## Issues Encountered
| Issue | Resolution |
|-------|------------|
| Not a Git repository | Skip worktree/commit requirements until user initializes Git or approves repo init. |
| Scope very large | Decompose into approved releases before implementation. |

## Resources
- Local planning files:
  - `task_plan.md`
  - `findings.md`
  - `progress.md`
- Approved design:
  - `docs/superpowers/specs/2026-06-07-myrtos-c-scope-design.md`
- Verification specs:
  - `docs/verification/requirements_traceability_matrix.md`
  - `docs/verification/coupling_test_matrix.md`
  - `docs/verification/test_suite_plan.md`
- API specs:
  - `docs/api/myrtos_api_catalog.md`
- Implementation plans:
  - `docs/superpowers/plans/2026-06-07-myrtos-master-implementation.md`
  - `docs/superpowers/plans/2026-06-07-myrtos-foundation-kernel.md`
- FreeRTOS official documentation overview: https://www.freertos.org/Documentation/00-Overview
- FreeRTOS source organization: https://www.freertos.org/Documentation/02-Kernel/06-Coding-guidelines/01-Source-code-organization
- FreeRTOS-Kernel GitHub repository: https://github.com/FreeRTOS/FreeRTOS-Kernel
- FreeRTOS-Kernel `tasks.c`: https://raw.githubusercontent.com/FreeRTOS/FreeRTOS-Kernel/main/tasks.c
- FreeRTOS-Kernel `list.c`: https://raw.githubusercontent.com/FreeRTOS/FreeRTOS-Kernel/main/list.c
- FreeRTOS-Kernel `queue.c`: https://raw.githubusercontent.com/FreeRTOS/FreeRTOS-Kernel/main/queue.c
- FreeRTOS Reference Manual V10.0.0 PDF: https://www.freertos.org/media/2018/FreeRTOS_Reference_Manual_V10.0.0.pdf

## Visual/Browser Findings
- None yet.

## Open Confirmations
- User approved platform default, API prefix, manual originality constraint, and Git repository initialization.
- User approved autonomous technical decision-making; only major irreversible scope changes should require asking.
- User requires detailed STM32/DSP porting steps in the final Chinese manual.

## Verification Findings
- Requirement traceability now has 11 top-level requirements (`R-001` through `R-011`); `R-011` requires the final manual to include detailed STM32 and DSP porting steps.
- Coupling matrix now has 33 coverage rows (`C-001` through `C-033`) spanning scheduler, tick, queues, ISR APIs, semaphores, mutexes, event groups, task notifications, timers, stream/message buffers, heap behavior, trace, assertions, STM32 port, DSP port, manual, and source comments.
- Implementation plan self-review placeholder scan found no `TBD`, `TODO`, `implement later`, `fill in details`, or stale draft-design path strings.
- Queue Task 2 non-blocking FIFO send/receive is implemented with caller-provided storage, circular byte-copy semantics, empty/full status returns, and temporary nonzero-timeout handling through `MRT_RESULT_TIMEOUT` until queue blocking coupling is implemented.
- Queue Task 3 queue variants are implemented: `MRT_QueuePeek` preserves queue state, `MRT_QueueSendFront` inserts before existing head, `MRT_QueueOverwrite` is intentionally restricted to one-slot queues, and `MRT_QueueReset` clears count/read/write indexes without clearing backing bytes.
- Queue Task 4 ISR queue APIs validate ISR context through the port layer, never block, and currently write `should_yield=false` because queue wait-list wakeups are scheduled for the later blocking-coupling tasks.
- Queue Task 5 uses two task list nodes: `state_node` for ready/delay scheduling and `wait_node` for object wait lists. This lets a queue receive timeout remove the task from both delay and queue wait lists without corrupting either list.
- Queue Task 6 wakes the highest-priority receiver when a send succeeds. Task-context send immediately reschedules; ISR send only marks the receiver ready and sets `should_yield=true` so the port layer can request a deferred switch.
- Queue verification matrix now marks `C-004`, `C-005`, and `C-006` verified. `C-007` remains explicitly not implemented because dynamic queue creation and heap failure behavior belong to the later memory-management module.
- Plan 4 starts semaphore and mutex work on top of the queue object-wait infrastructure. Semaphores can reuse task wait lists directly; mutexes need additional task priority inheritance helpers.
- Semaphore Task 1 adds static binary/counting semaphores with no heap dependency. Binary semaphores use max count 1 and an explicit initial availability flag; counting semaphores reject max count 0 and initial count above max.
- Semaphore Task 2 implements only nonblocking take/give. Empty semaphore with nonzero timeout returns `MRT_RESULT_TIMEOUT` outside scheduler context; actual task blocking and wakeup are reserved for Task 3 coupling tests.
- Semaphore Task 3 reuses the queue-era object wait helpers. A give to a nonempty taker wait list transfers the token directly to the highest-priority waiter, so semaphore count does not increase.
- Semaphore Task 4 mirrors queue ISR wake semantics: ISR give never switches immediately; it makes the waiter ready and reports `should_yield=true` for the port layer.
- Mutex Task 5 implements ownership-only mutex behavior. Lock/unlock require a running task; non-owner unlock returns `MRT_RESULT_OWNER_ERROR`; contention blocking and priority inheritance remain for Task 6.
- Mutex Task 6 implements priority inheritance by adjusting the owner's effective priority while preserving `base_priority`. Unlock restores the old owner and transfers ownership directly to the highest-priority waiter when one exists.
- Mutex Task 7 adds recursive mutex creation. The existing lock-depth path now has coverage: recursive owners can relock, partial unlock keeps ownership, and final unlock clears ownership.
- Synchronization Task 8 adds direct recursive mutex non-owner release coverage. `test_mutex_recursive` now checks `MRT_RESULT_OWNER_ERROR`, unchanged owner, and unchanged recursive depth when a non-owner attempts to unlock.
- Synchronization matrix update marks semaphore ISR, counting semaphore full, mutex priority inheritance, and recursive mutex ownership verified. Mutex timeout priority rollback, task deletion while holding a mutex, and unified assertion-hook behavior remain intentionally marked partial or pending.
- Plan 5 events/notifications will add `MRT_EventGroupWaitBits` with wait-any, wait-all, clear-on-exit, and timeout behavior, plus task notification actions with set bits, increment, overwrite, and no-overwrite.
- Event groups need a new internal wake-specific-task helper because existing object helper wakes only the first waiter, while event groups can wake multiple matching waiters from one bit set operation.
- Task notifications need per-task state fields and a block-current helper without an object wait list because notification waits are attached to the current task rather than a separate kernel object.
- Final manual requirement remains unchanged: STM32 and DSP sections must include detailed migration steps for toolchain, startup/vector table, tick, context switch, stack layout, critical sections, low power, examples, and troubleshooting.
- Event Group Task 1 implements only static creation plus task-context set/clear/get bit operations. Wait matching, blocking, multi-waiter wake, and ISR set are intentionally separated into later Plan 5 tasks.
- Event Group Task 2 defines `MRT_EventGroupWaitBits` immediate semantics: wait-all requires all requested bits, wait-any requires at least one requested bit, `out_bits` receives the pre-clear event snapshot, and clear-on-exit removes matched requested bits after a successful wait.
- Event Group Task 3 adds `MRT_TASK_WAIT_REASON_EVENT_BITS` and stores event wait metadata inside `MRT_Task`. Timeout waits now use the existing object wait helper, so tick expiry removes the task from both the delay list and the event group's `waiting_tasks` list.
- Event Group Task 4 adds `MRT_TaskKernelWakeTask` so event groups can wake multiple specific waiters in one set operation. Matching uses the post-set, pre-clear snapshot; clear-on-exit is applied once after all eligible waiters have been selected.
- Event Group Task 5 adds `MRT_EventGroupSetBitsFromISR`. It validates ISR context through the port mock, never switches immediately, and reports `should_yield=true` when a waiting task is moved to ready.
- Task Notify Task 6 adds one notification slot per task with `notify_value` and `notify_pending`. `MRT_TaskNotify` supports set-bits, increment, overwrite, and no-overwrite; no-overwrite rejects pending notifications without changing the old value.
- Task Notify Task 7 adds `MRT_TaskNotifyWait`, `MRT_TaskNotifyTake`, and `MRT_TaskKernelBlockCurrent`. Notification waits block without an object wait list; task-context notify wakes blocked tasks and can immediately reschedule. In the current host model, a blocked wait API returns timeout immediately, so a later notify leaves the value pending for future real resume semantics.
- Task Notify Task 8 adds `MRT_TaskNotifyFromISR`. It validates ISR context, reuses the same notification action helper, wakes notification waiters with deferred scheduling, and sets `should_yield=true` only when a task is made ready.
- Events/Notifications Task 9 updates `R-002`, `R-008`, `R-009`, `C-014`, `C-015`, `C-016`, and `C-027`. Event groups and task notifications are now evidenced by 33 passing host targets; timers, stream/message buffers, memory, tickless/trace/assert, ports, manual, static verification, and final report remain future plans.
- Plan 6 timers will implement deterministic host timer behavior with static timers, direct control APIs, tick-driven expiry, and pending function FIFO. Dedicated timer service task and asynchronous command queue will be reported as partial coverage because the current host scheduler does not execute task entry functions yet.
- Timer Task 1 adds static software timer creation with caller-provided control block storage. `MRT_TimerCreateStatic` rejects zero periods, null callbacks, null storage, and null output handles; new timers preserve name/period/reload/callback/argument fields and start inactive.
- Timer Task 2 adds direct timer control APIs. `MRT_TimerStart` and `MRT_TimerReset` compute `expiry_tick = MRT_KernelGetTick() + period_ticks`, insert the timer into a global active list ordered by expiry, and keep callback execution deferred for tick processing. `MRT_TimerStop` unlinks active timers, and `MRT_TimerChangePeriod` updates inactive timers without starting them while rearming active timers from the current tick.
- Timer Task 3 couples software timers to the kernel tick path. `MRT_KernelInitialize` now clears timer internal state, and `MRT_KernelTick` calls `MRT_TimerKernelTick` after task delay processing. One-shot timers stop after callback, auto-reload timers rearm before callback so callback code can stop or adjust them, and callbacks run outside the critical section.
- Timer Task 4 adds pending function support through a fixed-size ring buffer controlled by `MRT_CFG_TIMER_PENDING_FUNCTION_QUEUE_LENGTH` (default 8). `MRT_TimerPendFunctionCall` validates null functions and reports `MRT_RESULT_OBJECT_FULL` on capacity exhaustion; `MRT_TimerServiceRunPending` drains FIFO entries outside the critical section.
- Timer Task 5 updates verification evidence. `R-002`, `R-008`, and `R-009` now include timer sources and tests; `C-018` is verified for host tick expiry callbacks; `C-017` is partial because deterministic service-shim behavior is implemented but a dedicated service task and asynchronous command queue remain future work; `C-019` stays pending for tickless idle.
- Plan 7 stream/message buffer reference check: FreeRTOS `stream_buffer.c` uses one shared byte-buffer engine for stream buffers and message buffers; message buffers preserve packet boundaries by storing a length field before each message, while stream buffers expose raw byte FIFO semantics. MyRTOS will borrow these concepts only, keeping original `MRT_` APIs, original data structures, explicit `MRT_Result` returns, and Chinese comments.
- Stream Buffer Task 1 adds static stream buffer creation with caller-provided control block and byte storage. Creation rejects zero capacity, zero trigger level, trigger level greater than capacity, null byte storage, null control block, and null output handle. Query APIs report bytes used and free spaces through `MRT_Result`-based calls.
- Stream Buffer Task 2 adds nonblocking byte FIFO send/receive and reset. The stream buffer supports wrap-around, partial sends when free space is smaller than requested bytes, empty receive returning `MRT_RESULT_OBJECT_EMPTY`, full send returning `MRT_RESULT_OBJECT_FULL`, and reset clearing read/write indexes plus used byte count.
- Stream Buffer Task 3 adds ISR send/receive and reader wake coupling. Empty receive with nonzero timeout blocks the current task on `waiting_readers`; task-context send and ISR send wake a reader when `bytes_used >= trigger_level`. ISR send never switches immediately and reports `should_yield=true` when it wakes a reader.
- Message Buffer Task 4 adds static message buffer creation with caller-provided control block and byte storage. Capacity must hold a 4-byte length header plus at least one payload byte; query APIs report used bytes and free spaces.
- Message Buffer Task 5 adds packet-preserving send/receive with a 32-bit little-endian length header. Sends require enough free space for both header and payload, receives reject undersized output buffers without removing the pending message, and reset clears read/write indexes plus used-byte state.
- Message Buffer Task 6 adds ISR send/receive APIs plus reader wake coupling. Empty task-context receive with nonzero timeout blocks on `waiting_readers`; task-context send and ISR send wake one reader only after a complete message record is present; ISR send reports `should_yield=true` without switching immediately.
- Stream/Message Buffer Task 7 updates requirement evidence for `R-002`, `R-008`, `R-009`, and `R-010`, and marks coupling rows `C-020`, `C-021`, and `C-022` verified with 44 passing host test targets.
- Plan 8 memory management starts from `feature/stream-message-buffers` with 44 passing host targets. The plan will add three heap modes, a fixed block memory pool, and dynamic queue allocation to verify `C-007`, `C-023`, and `C-024`.
- Memory Task 1 adds heap initialization and query APIs. Heap regions are aligned to `MRT_CFG_HEAP_ALIGNMENT`, invalid modes and too-small regions are rejected, and repeated initialization resets free/minimum-free statistics.
- Memory Task 2 adds linear heap allocation. `MRT_Malloc` returns aligned blocks, updates current and minimum-ever free bytes, returns null for zero-size or exhausted allocations, and `MRT_Free` treats null as success while rejecting linear-heap block release with `MRT_RESULT_OBJECT_BUSY`.
- Memory Task 3 RED confirms the current heap still routes `MRT_HEAP_MODE_FREE_LIST` releases through the linear no-free policy. `test_heap_free_list` fails at `MRT_Free(large)` with `MRT_RESULT_OBJECT_BUSY`, so first-fit block metadata and free-list release handling are required.
- Memory Task 3 adds address-ordered heap block metadata for reusable heap modes. Free-list allocation uses first-fit scanning, splits only when the remainder can hold a block header plus an aligned payload, rejects heap-external pointers, rejects double free, and intentionally does not merge adjacent free blocks yet so Task 4 can validate coalescing separately.
- Memory Task 4 RED confirms `MRT_HEAP_MODE_COALESCING` currently behaves like the non-coalescing free-list mode. After freeing two adjacent 64-byte blocks and exhausting tail space, `MRT_Malloc(96)` fails to return the first block because neighbor merging is missing.
- Memory Task 4 adds release-time coalescing only for `MRT_HEAP_MODE_COALESCING`. The implementation first merges with a free predecessor, then merges with a free successor; `MRT_HEAP_MODE_FREE_LIST` remains intentionally non-coalescing and is covered by the same coalescing test target.
- Memory Pool Task 5 RED adds fixed-block pool coverage and currently fails because `myrtos/mrt_memory_pool.h` does not exist. The expected API is static-only creation with caller-provided control block and storage, allocation until empty, free-count query, invalid pointer rejection, and double-free rejection.
- Memory Pool Task 5 implements static fixed-block pools without using the global heap. Blocks are aligned to at least pointer size, the free-list pointer is stored inside free blocks, allocation returns `MRT_RESULT_OBJECT_EMPTY` when empty, and release validates range, block boundary, and duplicate free before relinking the block.
- Dynamic Queue Task 6 RED adds heap/queue coupling coverage and currently fails because `MRT_QueueCreate` and `MRT_QueueDelete` are not declared. The test requires successful dynamic creation/FIFO/delete, allocation failure returning `MRT_RESULT_NO_MEMORY`, unchanged heap free size on failed creation, static queue deletion rejection, and null-argument protection.
- Dynamic Queue Task 6 implements heap-backed queue creation as one allocation containing an aligned `MRT_Queue` control block followed by item storage. Failed allocation returns `MRT_RESULT_NO_MEMORY` and leaves heap free size unchanged; `MRT_QueueDelete` returns dynamic queues through `MRT_Free` and rejects static queues with `MRT_RESULT_OBJECT_BUSY`.
- Memory Task 7 updates requirement evidence for `R-002`, `R-008`, `R-009`, and `R-010`, and marks coupling rows `C-007`, `C-023`, and `C-024` verified with 50 passing host test targets.
- Tickless/Trace/Assert branch starts from `feature/memory-management` with 50 passing host test targets.
- Tickless design will query the task delayed list and software timer active list for nearest deadlines, then compensate kernel time after port sleep instead of directly jumping observable module state.
- Trace design uses a sink callback and compact event records so scheduler/queue paths remain backend-agnostic.
- Assert design uses one hook dispatcher and `MRT_ASSERT(expr)` macro; existing API parameter validation is not bulk-rewritten in this branch.
- Tickless sleep compensation uses repeated `MRT_KernelTick()` calls after the port reports actual slept ticks. This preserves existing task delay and software timer expiry order while keeping the implementation small.
- During compensation testing, a crash was traced to a test leaving an active stack-allocated timer in the global timer list before reinitializing the kernel. The test now stops that timer before returning.
- Trace module stores one optional sink callback and user pointer. Events are dropped when no sink is set, so enabled trace APIs do not force a logging backend.
- Task switch trace needs special handling for blocking paths because the scheduler clears `g_current_task` before choosing the next task. Blocking helpers explicitly publish `blocked_task -> new_current` after selection.
- Queue trace records successful send/receive only, with `value` set to the queue element count after the operation.
- Assert hook module stores one optional failure callback and user pointer. `MRT_ASSERT(expr)` stringifies the expression and dispatches file/line information, while default no-hook behavior returns without halting so host tests remain deterministic.
- Portable branch starts from `feature/tickless-trace-hooks` at `cee5b2d` with a clean worktree and a 55-target host-test baseline from the previous session.
- New port plan file: `docs/superpowers/plans/2026-06-08-myrtos-portable-stm32-dsp.md`.
- STM32/DSP code will first implement host-testable port-contract helpers rather than direct chip register writes. Real vector-table, SysTick/PendSV/SVC, low-power, and DSP interrupt hook-up steps remain mandatory in the final manual.
- STM32 helper scope: Cortex-M automatic exception frame layout, callee-saved register reserve area, stack alignment, SysTick reload calculation, and BASEPRI priority encoding.
- DSP helper scope: C28x-style downward-growing stack model, entry/argument/status/register save fields, alignment, software-interrupt context switch request, nesting counter, and acknowledgement behavior.
- STM32 stack helper is now implemented and tested by `test_port_stm32_stack`: initial frame uses 16 MRT_StackType words, 8-byte aligned stack top, R4-R11 debug placeholders, R0 argument, LR task-exit handler, PC task entry, and xPSR Thumb bit.
- STM32 tick/priority helper is now implemented and tested by `test_port_stm32_tick_priority`: SysTick reload rejects zero/too-fast/24-bit-overflow inputs, and BASEPRI encoding rejects invalid priority bit widths, logical priority 0, and out-of-range priorities.
- DSP stack helper is now implemented and tested by `test_port_dsp_stack`: frame is host-verifiable, 8-byte aligned, downward-growing, and records status, entry argument, exit handler, entry PC, and XAR4-XAR7 placeholders.
- DSP software-interrupt context model is now implemented and tested by `test_port_dsp_context`: requests remain pending until acknowledged, nested ISR exits defer switching until outermost exit, and underflow/null-output errors are rejected.
- Requirement matrix now records STM32/DSP port-contract evidence under `R-003` and `R-004`; real board smoke tests and detailed final manual porting steps remain pending.
- Manual/static verification branch starts from `feature/portable-stm32-dsp` at `eb0c965` with a 59-target host-test baseline passing.
- Official reference-manual structure observed from FreeRTOS reference material: opening scope/about section, API usage restrictions, chapterized API groups, function prototype, summary/parameters/return/notes/examples, and appendices. MyRTOS manual must use this structure only as a template and keep all wording/examples/API names original.
- New manual/static plan file: `docs/superpowers/plans/2026-06-08-myrtos-manual-static-verification.md`.
- Static verification scripts added under `tools/verify/`: API manual coverage, Chinese function comments, and originality/symbol scan.
- API manual coverage script verifies every API table entry in `docs/api/myrtos_api_catalog.md` has a matching `### API` manual section with function prototype, purpose, parameters, return value, calling context, blocking behavior, ISR limits, config dependencies, example, and common errors.
- Chinese comment script initially found 13 gaps, mostly internal task scheduler prototypes plus two public function definitions. These were fixed in `src/kernel/mrt_task_internal.h`, `src/kernel/mrt_queue.c`, and `src/kernel/mrt_task.c`.
- Manual file `docs/manual/MyRTOS_Reference_Manual_zh.md` now covers 125 API sections and includes detailed STM32 Cortex-M and DSP porting steps with toolchain, startup/vector table, tick, context switch, stack layout, critical section, low power, example, and troubleshooting content.
- Static checks currently pass: manual coverage, Chinese comment coverage, and original-symbol scan.
- Final verification report branch starts from `feature/manual-static-verification` at `cdb4b39`.
- Final report records that the project is a verified preview rather than a fully complete final RTOS: host/static evidence is strong, but real STM32/DSP board smoke tests and several preview API implementations remain future work.

---
*Update this file after every 2 view/browser/search operations.*

## Task Lifecycle API Gap Closure
- Branch `feature/task-lifecycle-apis` starts from `feature/final-verification-report` at `e6995d0`.
- API catalog/source audit found 21 catalog APIs without C declarations/definitions. The first closure batch targets 8 task APIs: `MRT_TaskCreate`, `MRT_TaskDelete`, `MRT_TaskSuspend`, `MRT_TaskResume`, `MRT_TaskResumeFromISR`, `MRT_TaskDelayUntil`, `MRT_TaskSetPriority`, and `MRT_TaskGetStackHighWaterMark`.
- Existing task internals already provide ready lists, delay list, object wait links, wake helpers, effective-priority helper, and notification wait state. New lifecycle APIs should reuse these helpers and add unlink/delete guards instead of copying FreeRTOS internals.
- Existing dynamic queue implementation provides the preferred pattern for heap-backed dynamic objects: validate, allocate one aligned block, initialize through the static API, mark `static_storage=false`, and free only dynamic storage on delete.
- Manual already includes detailed STM32 and DSP porting chapters. This branch should keep those sections and update task lifecycle sections from preview/gap language to implemented/tested behavior.
- Task lifecycle implementation is now covered by 61 passing host targets. The original 21 catalog/source gaps are reduced by 8; remaining known gaps are the 13 non-task dynamic/statistics APIs: dynamic semaphore, mutex, event group, timer, stream/message buffer create/delete APIs and `MRT_StatsGetTaskRuntime`.
- `MRT_TaskGetStackHighWaterMark` currently returns configured stack capacity in the host model because stack painting and real stack consumption belong to architecture-specific ports. The manual documents this limit explicitly.
- `MRT_TaskDelete` now handles scheduler/list cleanup and dynamic task heap release, but deletion while holding a mutex remains a separate coupling policy gap under `C-012`.
