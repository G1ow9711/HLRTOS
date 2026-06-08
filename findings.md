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
- 新增 `docs/verification/completion_audit.md`，把原始目标拆成逐项证据审计；当前唯一硬缺口仍是真实 STM32 / DSP 板级 smoke 证据。
- Latest verification: `python tools\verify\check_api_manual_coverage.py` now reports 135 API sections covered, `python tools\verify\check_api_catalog_prototypes.py` reports 135 aligned prototypes, and `python tools\run_host_tests.py` now reports 70 passed host test targets.
- Dynamic stream/message buffer lifecycle is now closed for current API scope: `MRT_StreamBufferDelete` and `MRT_MessageBufferDelete` release heap-backed buffers and reject null/static objects; `test_buffer_dynamic_allocation` covers the paths.
- STM32/DSP 手册移植章已加细粒度骨架：`移植前准备`、`工程分层`、`关键接入顺序`、`首次联调`、`板级验收`；`check_api_manual_coverage.py` 也把这些词纳入必检项。
- `test_port_stm32_mpu` 已单独纳入 `C-035`，把 STM32 MPU 区域规整 helper 从“附带证据”变成独立耦合项。
- Requirement traceability now has 11 top-level requirements (`R-001` through `R-011`); `R-011` requires the final manual to include detailed STM32 and DSP porting steps.
- Coupling matrix now has 40 coverage rows (`C-001` through `C-040`) spanning scheduler, tick, queues, ISR APIs, semaphores, mutexes, event groups, task notifications, timers, stream/message buffers, heap behavior, trace, assertions, STM32 port, DSP port, manual, source comments, runtime stats, STM32 MPU layout, buffer writer wait/delete coupling, hardware capture flow, DSP C2000 scaffold, and hardware raw-log schema alignment.
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
- Manual file `docs/manual/MyRTOS_Reference_Manual_zh.md` now covers 126 API sections and includes detailed STM32 Cortex-M and DSP porting steps with toolchain, startup/vector table, tick, context switch, stack layout, critical section, low power, example, and troubleshooting content.
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

## Dynamic Object API Gap Closure
- Branch `feature/dynamic-object-apis` starts from `feature/task-lifecycle-apis` at `ca7da08`.
- Baseline verification in the new worktree passes: `python tools\run_host_tests.py` reports `[summary] 61 test target(s) passed`.
- Remaining known source/catalog gaps after task lifecycle closure: 12 dynamic object APIs plus `MRT_StatsGetTaskRuntime`.
- Dynamic object branch targets: `MRT_SemaphoreCreateBinary`, `MRT_SemaphoreCreateCounting`, `MRT_SemaphoreDelete`, `MRT_MutexCreate`, `MRT_MutexCreateRecursive`, `MRT_MutexDelete`, `MRT_EventGroupCreate`, `MRT_EventGroupDelete`, `MRT_TimerCreate`, `MRT_TimerDelete`, `MRT_StreamBufferCreate`, and `MRT_MessageBufferCreate`.
- Preferred implementation pattern follows dynamic queue/task: validate arguments, allocate heap memory, call static create, mark `static_storage=false`, free on failure, and reject static objects in delete APIs with `MRT_RESULT_OBJECT_BUSY`.
- Stream/message buffers need one heap block containing an aligned control block plus byte storage. Semaphore, mutex, event group, and timer need only control-block-sized heap allocations.
- First GREEN run exposed a test assumption bug: `MRT_Malloc(free_before)` fails on coalescing heap because `free_before` includes the heap block header overhead. Failure-path tests now use a helper that repeatedly allocates smaller blocks until even the minimum payload cannot be allocated, then compares heap free size before/after the target dynamic API failure.
- Dynamic object implementation is now covered by 64 passing host targets. The 12 dynamic object source/catalog gaps are closed; the remaining known source/catalog gap is `MRT_StatsGetTaskRuntime`.
- Manual updates now document exact dynamic object prototypes, failed-create handle clearing, static-delete rejection, waiters/locked-object busy deletion, active timer delete-stop-free behavior, dynamic stream/message buffer single-block heap layout, and stream/message buffer dynamic delete APIs.

## Runtime Stats API Gap Closure
- Branch `feature/runtime-stats-api` starts from `feature/dynamic-object-apis` at `dfe4838`.
- Baseline verification in the new worktree passes: `python tools\run_host_tests.py` reports `[summary] 64 test target(s) passed`.
- `MRT_StatsGetTaskRuntime` is the remaining known API catalog/source gap after dynamic object APIs.
- Runtime stats will use portable tick-level accounting for this branch: each `MRT_KernelTick()` attributes one runtime tick to the task that was current before the tick interrupt processed wakeups and timers. This is deterministic on host and maps to real ports; STM32/DSP cycle-accurate counters can later feed the same public query API.
- Runtime stats implementation is now covered by 65 passing host targets. `MRT_StatsGetTaskRuntime` has public header/source, task TCB storage, kernel tick accumulation, and coupling tests for high/low task runtime attribution plus invalid/deleted-task arguments.
- Current API catalog/source implementation gap count is 0 for the 125 public API entries in `docs/api/myrtos_api_catalog.md`. Remaining work is system-level: real STM32/DSP smoke, mutex timeout rollback, held-mutex task deletion policy, timer service task, and broader comment scanning.
- Manual porting chapters have been strengthened beyond API reference coverage: STM32 and DSP sections now include minimum handler/application skeletons, smoke-test expectations, troubleshooting points, and explicit acceptance checklists for build, tick, context switch, ISR wakeup, low power, ABI, stack, and memory placement.

## Mutex Timeout Rollback Findings
- Branch `feature/mutex-timeout-rollback` starts from `feature/runtime-stats-api` at `37281c5`.
- Baseline verification before the fix: `python tools\run_host_tests.py` showed the new `test_mutex_timeout_rollback` failing with `expected 1 got 5`, proving owner priority was not restored after waiter timeout.
- Implementation approach: `MRT_TaskKernelTick` now preserves timeout wait reason and object wait list before removing the waiter node, and mutex timeout cleanup recalculates owner effective priority from the remaining mutex waiters.
- Current verification: `python tools\run_host_tests.py` reports `[summary] 66 test target(s) passed`.
- Manual update: mutex lock section now states timeout-driven waiter removal triggers owner priority recalculation, and `MRT_MutexGetOwner` prototype in the manual now matches source.

## Held Mutex Delete Policy Findings
- Branch `feature/held-mutex-delete-policy` starts from `feature/mutex-timeout-rollback` at `680e4ae`.
- RED evidence: `test_mutex_task_delete_policy` failed because `MRT_TaskDelete(owner_task)` returned `MRT_RESULT_OK` while the task still owned a mutex; failure line showed `expected 5 got 0`.
- Policy decision: deleting a task that owns a mutex is rejected with `MRT_RESULT_OBJECT_BUSY`; MyRTOS does not silently release application locks during task deletion.
- Implementation approach: mutex objects are tracked in an internal registry; `MRT_TaskDelete` checks the registry before unlinking/freeing a task; `MRT_KernelInitialize` resets the registry for deterministic test/system restart behavior.
- Current verification: `python tools\run_host_tests.py` reports `[summary] 67 test target(s) passed`.

## Timer Service Command Queue Findings
- Branch `feature/timer-service-task` starts from `feature/held-mutex-delete-policy` at `d56cb0e`.
- RED evidence: `test_timer_service_task` failed because `MRT_TimerStart(timer, 0u)` made the timer active immediately; the expected service-queue behavior is that the timer remains inactive until `MRT_TimerServiceRunPending()` drains the start command.
- Design decision: use `MRT_TimerServiceRunPending()` as the host-testable timer service entry. Real STM32/DSP ports may run that entry from a dedicated service task, but host tests can call it directly for deterministic proof.
- Implementation approach: one FIFO service queue now carries timer control commands, timer expiry callback events, and pending functions. `MRT_TimerKernelTick()` only queues expiry events; user callbacks execute from the service entry outside the critical section.
- Dynamic timer deletion now purges queued commands and expiry events for the deleted timer before freeing memory, preventing service-queue dangling references.
- Current verification: `python tools\run_host_tests.py` reports `[summary] 68 test target(s) passed`.

## Embedded Smoke Project Findings
- Branch `feature/embedded-smoke-projects` starts from `feature/timer-service-task` at `3071d34`; baseline state is expected to have 68 host test targets passing.
- Current project has no `examples/`, `examples/stm32/`, or `examples/dsp/` directories, while `docs/verification/test_suite_plan.md` already lists those directories as required smoke layers.
- `arm-none-eabi-gcc` is available: Arm GNU Toolchain 14.2.Rel1. This makes an STM32 Cortex-M cross-compile smoke build feasible in the current environment.
- `tiarmclang` is not available in the current environment. DSP real-toolchain verification cannot be claimed here; use a host-verifiable DSP C28x-style model and document the real TI/toolchain smoke gap.
- The manual already has STM32/DSP porting chapters, but this branch should add explicit smoke project layout, build commands, handler wiring, and acceptance steps so the manual porting path matches the new examples.
- RED smoke verification script added at `tools/verify/check_embedded_smoke_projects.py`; first run failed only because the required example directories and files do not exist yet.
- GREEN smoke verification now passes: `python tools\verify\check_embedded_smoke_projects.py` builds `examples/stm32` with `arm-none-eabi-gcc`, builds `examples/dsp` with host `gcc`, and runs the DSP smoke model executable successfully.
- STM32 smoke project is a cross-compile/link scaffold and does not claim real board execution. DSP smoke is a host-verifiable C28x-style model because the TI DSP toolchain is absent.
- Chinese comment scanning was first extended to `examples/`, proving the new STM32/DSP smoke files have Chinese function and step comments.
- `python tools\verify\check_chinese_comments.py` now scans `include/`, `src/`, `examples/`, and `tests/`; the only discovered gap was `tests/unit/test_types_contract.c` `main`, which is now documented.
- `python tools\verify\check_original_symbols.py` now scans `include/`, `src/`, `examples/`, `tests/`, and the manual, and still reports no banned FreeRTOS-style public symbols.
- Final verification snapshot: 68 host targets passed; API manual coverage, Chinese comments, original-symbol scan, embedded smoke verification, and `git diff --check` all passed with only expected CRLF warnings.

## Buffer Writer Wait and API Prototype Audit Findings
- Dynamic stream/message delete already checked `waiting_writers`, but public send paths previously never populated those lists. New tests prove the missing coupling by expecting full-buffer nonzero-timeout send to block the writer and make delete return `MRT_RESULT_OBJECT_BUSY`.
- Stream buffer writer wait records `object_wait_bytes=1` because stream buffers permit partial writes; any freed byte lets the writer retry. Message buffer writer wait records `4 + payload` because a message buffer must not wake a writer until a complete record can fit.
- `MRT_TASK_WAIT_REASON_STREAM_SEND` and `MRT_TASK_WAIT_REASON_MESSAGE_SEND` are now public task wait reasons for diagnostics. `MRT_Task.object_wait_bytes` is cleared on object wake, timeout, pure delay, task unlink, and task creation so stale byte requests cannot affect later waits.
- Receive/reset paths now wake one highest-priority waiting writer when enough space exists. `MRT_StreamBufferReceiveFromISR` and `MRT_MessageBufferReceiveFromISR` wake writers without immediate task switch because those APIs have no `should_yield` output.
- New verifier `tools/verify/check_api_catalog_prototypes.py` checks catalog, public headers, and source definitions. It treats `MRT_ASSERT` as a macro and excludes test-only mock hooks plus low-level list/priority bitmap primitives from the official catalog contract.
- Prototype verifier RED found two real public omissions: `MRT_MemoryPoolGetFreeCount` and `MRT_TimerGetName` were declared/implemented but not in the API catalog/manual. Catalog and manual now cover both, raising manual/prototype coverage to 135 APIs.
- Latest full verification: `python tools\run_host_tests.py` reports `[summary] 70 test target(s) passed`; manual coverage reports 135; API prototype verifier reports 135; comments, originality, embedded smoke, and `git diff --check` pass with only expected CRLF warnings.
- Real STM32 and DSP board smoke evidence remains the only hard completion gap; current STM32 evidence is cross-compile/link and current DSP evidence is host model smoke.

## Hardware Smoke Evidence Gate Findings
- `tools/verify/check_hardware_smoke_evidence.py` now defines the machine-checkable final evidence contract for real STM32 and DSP boards.
- The checker expects `docs/verification/hardware_smoke/stm32_board_smoke.md` and `docs/verification/hardware_smoke/dsp_board_smoke.md`.
- `docs/verification/hardware_smoke/collection_checklist.md` now describes the real STM32/DSP log collection flow, raw log retention, and field filling rules before the final evidence file is produced.
- `tests/static/test_hardware_smoke_evidence_checker.py` covers valid STM32/DSP evidence plus missing/invalid evidence. The first run failed because the checker returned immediately after missing required fields and hid present-but-failing `Evidence-Status`, runtime, assert, and heap fields.
- Checker behavior is now improved: missing required fields are reported, but present failing status and numeric fields are still validated.
- Added templates only: `docs/verification/hardware_smoke/stm32_board_smoke.template.md` and `docs/verification/hardware_smoke/dsp_board_smoke.template.md`. No fake PASS logs are committed.
- Manual sections 5.6 and 6.6 now describe real-board evidence archival steps and point at the collection checklist. Section 7.4 lists the verification commands, including `python tools\verify\check_hardware_smoke_evidence.py`.
- Latest repo-side recheck after checklist update: manual coverage 135, API prototype alignment 135, embedded smoke passed, hardware checker unit test passed, and `git diff --check` stayed clean apart from expected CRLF warnings.
- `tools/verify/run_release_verification.py` is now the unified entrypoint: default mode passed all repo-side checks, and `--require-hardware` failed only because the real STM32/DSP evidence files are still absent.
- `python tools\verify\run_release_verification.py --list` shows the default repo-side chain only; hardware gate is appended only with `--require-hardware`.
- Current hardware evidence gate intentionally fails with missing `stm32_board_smoke.md` and `dsp_board_smoke.md`; this is the remaining real-hardware proof gap, not a software test failure.
- Latest repo-side verification: host tests 70 passed; manual coverage 135; API prototype alignment 135; Chinese comments, originality, embedded smoke, and hardware checker unit test pass; `git diff --check` exits 0 with expected CRLF warnings only.

## Manual Porting Detail Refresh Findings
- User explicitly asked that the final manual include detailed porting steps.
- Existing manual sections 5 and 6 already cover STM32 Cortex-M and DSP porting, smoke projects, acceptance, and hardware evidence archival.
- Detail gap to close: add a stricter migration playbook for moving from a vendor bare-metal project into MyRTOS, including file copy order, startup/vector merge, linker placement, interrupt/tick wiring, first-board validation order, and real-board evidence fields.

## Hardware Evidence Generator Findings
- `tests/static/test_hardware_smoke_evidence_generator.py` now passes and proves the raw-log normalizer strips noise and preserves required STM32/DSP evidence fields.
- `tools/verify/run_release_verification.py` now includes the generator test as a default release step, so hardware evidence tooling is covered by the unified gate.
- `python tools\\verify\\run_release_verification.py` passed with 9 step(s).
- `python tools\\verify\\run_release_verification.py --require-hardware` still fails only because `stm32_board_smoke.md` and `dsp_board_smoke.md` are not present yet; no new software regression showed up.
- Verification docs now mention the generator in the final report, completion audit, test suite plan, and requirement traceability matrix, replacing stale `[release] 8 step(s) passed` evidence with the current 9-step default release result.

## STM32 Context Assembly Scaffold Findings
- RED evidence: `python tests\\static\\test_stm32_context_scaffold.py` failed because `src/portable/stm32_cm/mrt_port_stm32_cm_context.S` did not exist.
- RED evidence: `python tests\\static\\test_release_verification_runner.py` failed because the release runner did not include the new `stm32-context-scaffold` step.
- Implementation adds a Cortex-M4 GNU assembly scaffold with `SVC_Handler`, `PendSV_Handler`, and `MRT_PortStm32CmStartFirstTaskAsm`. PendSV reads PSP, safely skips R4-R11 save/restore if PSP is zero, otherwise saves R4-R11, calls a C hook, restores R4-R11, writes PSP, and returns through EXC_RETURN.
- `examples/stm32/mrt_port_stm32_smoke.c` now provides `MRT_PortStm32CmSvcHook()` and `MRT_PortStm32CmPendSvHook()` for diagnostic handoff. These hooks record exception frame, PSP, EXC_RETURN, and call counts, but intentionally do not claim full TCB/PSP task switching.
- `tools/verify/check_embedded_smoke_projects.py` now cross-compiles the `.S` file with ARM GCC. This proves symbol and build wiring only; real STM32 board runtime evidence is still required.
- `python tools\\verify\\run_release_verification.py` now passes with `[release] 10 step(s) passed`.
- `python tools\\verify\\check_hardware_smoke_evidence.py` still fails only because `stm32_board_smoke.md` and `dsp_board_smoke.md` are missing; no fake board evidence was generated.

## Task Runtime Stack-Top Contract Findings
- RED evidence: `python tools\\run_host_tests.py` failed only on `test_task_stack_top` because `MRT_TaskKernelGetStackTop`, `MRT_TaskKernelSetStackTop`, and `MRT_TaskKernelSwitchStackTop` did not exist.
- Implementation adds `MRT_Task.stack_top` and initializes it to the end of static/dynamic task stacks. Internal helpers reject null/deleted tasks, save the switched-out task PSP, and return the current task PSP.
- `MRT_TaskSwitchToHighestReady`, blocking waits, delays, and current-task suspend now remember the task being switched out so the PendSV hook saves PSP into the correct TCB.
- `examples/stm32/MRT_PortStm32CmPendSvHook()` now calls `MRT_TaskKernelSwitchStackTop()`; `check_embedded_smoke_projects.py` includes `-Isrc/kernel` so the ARM GCC smoke build verifies the hook wiring.
- Follow-up RED evidence: `python tests\\static\\test_stm32_context_scaffold.py` failed until `examples/stm32/main.c` used `MRT_PortStm32CmInitializeStack()` and `MRT_TaskKernelSetStackTop()` to write each port-initialized initial PSP into the task TCB.
- STM32 smoke now constructs initial Cortex-M exception frames for the LED and UART tasks before `MRT_KernelStart()`, then PendSV can return a TCB stack top that points at an actual initial frame instead of the raw empty stack end.
- Current verification after this increment: `python tools\\verify\\run_release_verification.py` reports `[release] 10 step(s) passed` after 70 host targets, manual/API/comment/originality checks, STM32 context scaffold, embedded smoke, and hardware evidence tooling tests.
- `python tools\\verify\\check_hardware_smoke_evidence.py` still fails only because `stm32_board_smoke.md` and `dsp_board_smoke.md` are missing. This is still not real STM32/DSP board runtime evidence; the hardware gate remains pending until both files contain real PASS logs.
## Hardware Smoke Preflight Findings
- RED evidence: `python tests\\static\\test_hardware_smoke_preflight.py` first failed because `tools\\verify\\check_hardware_smoke_preflight.py` was missing.
- Implementation adds `check_config_file(config_path, check_tools)` plus CLI `--config` and `--check-tools`. Default mode validates schema and placeholders only; `--check-tools` additionally checks command executables with `shutil.which`.
- Default config `docs/verification/hardware_smoke/hardware_smoke_preflight.json` covers STM32F407VG/STM32F4DISCOVERY and TMS320F28379D/LAUNCHXL-F28379D example targets, 30-minute minimum runtime, required STM32/DSP evidence fields, and raw-log/evidence output paths.
- Release runner now includes `hardware-smoke-preflight` as a default repo-side step before final evidence checker self-tests. This does not fabricate or accept real board evidence.
- Manual sections 5.6, 6.6, and 7.4 now instruct users to run the preflight check before raw-log normalization and final evidence validation.
- Latest default release verification passed with `[release] 11 step(s) passed`; `--require-hardware` still fails only at the real hardware evidence gate.
- Hardware evidence gap remains unchanged: no real `stm32_board_smoke.md` or `dsp_board_smoke.md` PASS logs exist yet.

## Hardware Evidence Raw-Log Traceability Findings
- RED evidence: `python tests\\static\\test_hardware_smoke_evidence_generator.py` failed because generated final evidence did not include `Raw-Log-Path` and `Raw-Log-SHA256`.
- RED evidence: `python tests\\static\\test_hardware_smoke_evidence_checker.py` failed because a final evidence file with mismatched raw-log SHA-256 was accepted.
- Implementation adds SHA-256 derivation in `tools/verify/generate_hardware_smoke_evidence.py` and SHA-256 comparison in `tools/verify/check_hardware_smoke_evidence.py`.
- Final real board evidence now must preserve raw UART/trace logs and include a matching `Raw-Log-SHA256`; this strengthens traceability but still does not create real STM32/DSP board evidence.
- Latest repo-side verification passed: API catalog prototype audit, original-symbol scan, hardware smoke preflight, whitespace check, and `python tools\\verify\\run_release_verification.py` with `[release] 11 step(s) passed`.
- Hardware-required verification remains correctly gated: `python tools\\verify\\check_hardware_smoke_evidence.py` reports missing real `stm32_board_smoke.md` and `dsp_board_smoke.md`, and `python tools\\verify\\run_release_verification.py --require-hardware` fails only at `hardware-smoke-evidence`.

## Hardware Smoke Capture Runner Findings
- RED evidence: `python tests\\static\\test_hardware_smoke_capture_runner.py` first failed because `tools\\verify\\run_hardware_smoke_capture.py` was missing; `python tests\\static\\test_release_verification_runner.py` then failed because the default release chain did not include `hardware-smoke-capture-runner`.
- Implementation adds `run_hardware_smoke_capture.py` with default dry-run, explicit `--execute`, target selection, preflight validation, configured compiler/build/flash/capture command execution, raw-log evidence generation, and target-level evidence validation.
- Follow-up RED evidence found default dry-run duplicated evidence generation when `capture_command` already invoked `generate_hardware_smoke_evidence.py`; the runner now detects that command and skips the extra `generate-evidence` step.
- Phase 19 repo-side verification passed: capture runner static test, release-runner static test, manual coverage, hardware preflight, `git diff --check`, and `python tools\\verify\\run_release_verification.py` with `[release] 12 step(s) passed` before the DSP context scaffold step was added.
- Hardware-required verification remains correctly gated: real `stm32_board_smoke.md`, `dsp_board_smoke.md`, and matching raw logs are still absent, so `python tools\\verify\\run_release_verification.py --require-hardware` fails only at `hardware-smoke-evidence`.

## DSP C28x Context Assembly Scaffold Findings
- RED evidence existed for missing DSP context assembly: `tests/static/test_dsp_context_scaffold.py` requires a C28x-style scaffold file, first-task/yield/software-interrupt symbols, save/restore markers, and explicit C hook handoff symbols.
- Implementation adds `src/portable/dsp_c28x/mrt_port_dsp_c28x_context.asm` as an audit scaffold for first-task start, software interrupt yield, context switch save/restore order, and hook handoff. It is intentionally not treated as target-toolchain or real-board proof.
- Phase 20 default release chain added `dsp-context-scaffold` after `stm32-context-scaffold` and passed with `[release] 13 step(s) passed` before the C2000 project scaffold step was added.
- Manual section 6 and `examples/dsp/README.md` now tell users to use the scaffold only as a migration/audit template, then adjust it for the exact DSP compiler ABI, register set, interrupt controller, and board smoke evidence.
- Hardware-required verification remains correctly gated: `python tools\\verify\\check_hardware_smoke_evidence.py` reports missing real `stm32_board_smoke.md` and `dsp_board_smoke.md`, and `python tools\\verify\\run_release_verification.py --require-hardware` fails only at `hardware-smoke-evidence`.

## DSP C2000 Board Smoke Project Scaffold Findings
- RED evidence: `python tests\\static\\test_dsp_c2000_project_scaffold.py` first failed because `examples\\dsp\\startup_c28x.c` and related C2000 scaffold files were missing.
- RED evidence: `python tests\\static\\test_release_verification_runner.py` failed because the default release chain did not include `dsp-c2000-project-scaffold`.
- Implementation adds `startup_c28x.c`, `mrt_port_dsp_c2000_smoke.c`, and `linker_c28x.cmd` under `examples/dsp/`. These files record C2000 vector/timer/software-interrupt/ADC glue plus `.mrtos_heap`, `.mrtos_tasks`, `.mrtos_dma`, and `.mrtos_trace` linker sections.
- Follow-up review changed `startup_c28x.c` to declare ISR entry points as `extern` and let `mrt_port_dsp_c2000_smoke.c` define them, avoiding weak-placeholder collisions in real target builds.
- `tools/verify/check_embedded_smoke_projects.py` now requires the C2000 scaffold files to exist while still compiling/running only the host-verifiable DSP model in the current environment.
- The new scaffold improves DSP portability evidence but still does not prove TI toolchain compilation or real DSP board execution. Final completion still needs real `dsp_board_smoke.md` and matching raw log.

## Hardware Smoke Raw-Log Schema Findings
- RED evidence: `python tests\\static\\test_hardware_smoke_raw_log_schema.py` failed because `tools\\verify\\check_hardware_smoke_raw_log_schema.py` was missing.
- RED evidence: `python tests\\static\\test_release_verification_runner.py` failed because `hardware-smoke-raw-log-schema` was not in the default release chain.
- Implementation adds `docs/verification/hardware_smoke/raw_log_schema.md`, listing common, STM32, and DSP required `Key: Value` fields plus STM32/DSP example raw logs.
- Implementation adds `examples/hardware_smoke/mrt_hardware_smoke_log_schema.h` with `MRT_SMOKE_FIELD_*` constants and X-macro required-field lists for real board UART/trace output code to reuse without header-level unused objects.
- Implementation adds `tools/verify/check_hardware_smoke_raw_log_schema.py`, which loads `COMMON_REQUIRED_FIELDS`, `STM32_REQUIRED_FIELDS`, and `DSP_REQUIRED_FIELDS` from `generate_hardware_smoke_evidence.py` and checks the schema doc, C header, capture guides, and example READMEs for drift.
- Focused GREEN checks now pass: `python tests\\static\\test_hardware_smoke_raw_log_schema.py`, `python tests\\static\\test_release_verification_runner.py`, and `python tools\\verify\\check_hardware_smoke_raw_log_schema.py`.
- Full default release verification now passes with `[release] 15 step(s) passed`; `--require-hardware` still fails only at `hardware-smoke-evidence` because real STM32/DSP board evidence files are absent.
- This phase improves real-board readiness and field consistency, but still does not create or replace real `stm32_board_smoke.md` or `dsp_board_smoke.md` evidence.

## Hardware Smoke Log Output Helper Findings
- RED evidence: direct compile of `tests/unit/test_hardware_smoke_log.c` failed first because `examples/hardware_smoke/mrt_hardware_smoke_log.h` did not exist.
- Implementation adds `mrt_hardware_smoke_log.h` as a header-only helper with `MRT_SmokeLogWriter`, `MRT_SmokeLogWritePair()`, and `MRT_SmokeLogWriteU32()`.
- Design boundary: helper writes schema-compliant `Key: Value\n` and decimal integer lines through a board-provided single-character callback; it does not use `printf`, does not judge PASS/FAIL, and does not generate final evidence.
- `test_hardware_smoke_log` covers string field output, decimal `uint32_t` output, and invalid arguments returning `MRT_SMOKE_LOG_INVALID_ARGUMENT` without leaving partial log bytes.
- Fresh verification after documentation sync passed: `python tools\run_host_tests.py` reported `[summary] 71 test target(s) passed`; Chinese comment, API manual coverage, API prototype, original-symbol, raw-log schema, hardware preflight, embedded smoke, and `git diff --check` all passed.
- Full default release verification passed with `[release] 15 step(s) passed`.
- Hardware-required verification remains correctly gated: `python tools\verify\check_hardware_smoke_evidence.py` reports missing `stm32_board_smoke.md` and `dsp_board_smoke.md`, and `python tools\verify\run_release_verification.py --require-hardware` fails only at `hardware-smoke-evidence`.

## Hardware Smoke Complete Report Emitter Findings
- RED evidence: `python tools\run_host_tests.py` failed at `test_hardware_smoke_report` because `examples/hardware_smoke/mrt_hardware_smoke_report.h` did not exist.
- Implementation adds `mrt_hardware_smoke_report.h` with `MRT_SmokeCommonReport`, `MRT_SmokeStm32Report`, `MRT_SmokeDspReport`, validation helpers, `MRT_SmokeEmitCommonReport()`, `MRT_SmokeEmitStm32Report()`, and `MRT_SmokeEmitDspReport()`.
- Design boundary: the emitter validates strings and writes all required raw-log fields, but does not compute PASS/FAIL, does not hash logs, and does not generate final evidence files.
- `test_hardware_smoke_report` covers STM32 required field output, DSP required field output, and invalid arguments returning `MRT_SMOKE_LOG_INVALID_ARGUMENT` without partial report output.
- Fresh verification after documentation sync passed: `python tools\run_host_tests.py` reported `[summary] 72 test target(s) passed`; Chinese comment, API manual coverage, API prototype, original-symbol, raw-log schema, hardware preflight, embedded smoke, and `git diff --check` all passed.
- Full default release verification passed with `[release] 15 step(s) passed`.
- Hardware-required verification remains correctly gated: `python tools\verify\check_hardware_smoke_evidence.py` reports missing `stm32_board_smoke.md` and `dsp_board_smoke.md`, and `python tools\verify\run_release_verification.py --require-hardware` fails only at `hardware-smoke-evidence`.

## Hardware Smoke Report Schema Drift Findings
- RED evidence: after adding `REPORT_HEADER` coverage to `tests/static/test_hardware_smoke_raw_log_schema.py`, `python tests\static\test_hardware_smoke_raw_log_schema.py` failed because `tools/verify/check_hardware_smoke_raw_log_schema.py` did not mention `mrt_hardware_smoke_report.h`.
- Implementation adds `REPORT_HEADER`, `field_macro_token()`, and `check_report_header_coverage()` to the schema checker. It now verifies every generator-required STM32/DSP raw-log field maps to a `MRT_SMOKE_FIELD_*` token used by `mrt_hardware_smoke_report.h`.
- Focused GREEN checks passed: `python tests\static\test_hardware_smoke_raw_log_schema.py` and `python tools\verify\check_hardware_smoke_raw_log_schema.py`.
- Follow-up RED evidence caught stale release-runner wording: `python tests\static\test_release_verification_runner.py` failed until the raw-log schema step description mentioned `完整报告 emitter`; the description now matches the checker scope.
- Design boundary unchanged: this proves schema/report-header alignment only; it does not create real STM32/DSP board evidence or accept fake `Evidence-Status: PASS`.
