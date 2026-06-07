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
- Requirement traceability now has 10 top-level requirements (`R-001` through `R-010`).
- Coupling matrix now has 33 coverage rows (`C-001` through `C-033`) spanning scheduler, tick, queues, ISR APIs, semaphores, mutexes, event groups, task notifications, timers, stream/message buffers, heap behavior, trace, assertions, STM32 port, DSP port, manual, and source comments.
- Implementation plan self-review placeholder scan found no `TBD`, `TODO`, `implement later`, `fill in details`, or stale draft-design path strings.
- Queue Task 2 non-blocking FIFO send/receive is implemented with caller-provided storage, circular byte-copy semantics, empty/full status returns, and temporary nonzero-timeout handling through `MRT_RESULT_TIMEOUT` until queue blocking coupling is implemented.
- Queue Task 3 queue variants are implemented: `MRT_QueuePeek` preserves queue state, `MRT_QueueSendFront` inserts before existing head, `MRT_QueueOverwrite` is intentionally restricted to one-slot queues, and `MRT_QueueReset` clears count/read/write indexes without clearing backing bytes.
- Queue Task 4 ISR queue APIs validate ISR context through the port layer, never block, and currently write `should_yield=false` because queue wait-list wakeups are scheduled for the later blocking-coupling tasks.
- Queue Task 5 uses two task list nodes: `state_node` for ready/delay scheduling and `wait_node` for object wait lists. This lets a queue receive timeout remove the task from both delay and queue wait lists without corrupting either list.

---
*Update this file after every 2 view/browser/search operations.*
