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

---
*Update this file after every 2 view/browser/search operations.*
