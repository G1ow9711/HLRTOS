# MyRTOS Memory Managers Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add original MyRTOS dynamic memory services: configurable heap modes, fixed block memory pools, dynamic queue creation/deletion, and verification evidence for allocation failure and block coalescing.

**Architecture:** The kernel gets one active heap region initialized from caller-provided storage. `MRT_HEAP_MODE_LINEAR` provides bump-only allocation, `MRT_HEAP_MODE_FREE_LIST` reuses freed blocks without neighbor merging, and `MRT_HEAP_MODE_COALESCING` merges adjacent free blocks on release. Fixed block pools stay separate from the heap so embedded users can allocate deterministic same-size buffers without general heap fragmentation.

**Tech Stack:** C99, GCC-backed `tools/run_host_tests.py`, CMake metadata, host unit tests under `tests/unit/`, coupling tests under `tests/coupling/`, existing queue static creation internals for dynamic queue allocation.

---

## File Structure

- `include/myrtos/mrt_heap.h`: heap mode enum, heap block/control types, public heap APIs, and query APIs.
- `src/kernel/mrt_heap.c`: global heap initialization, aligned allocation, free-list management, optional coalescing, and usage statistics.
- `include/myrtos/mrt_memory_pool.h`: fixed block pool control block and public pool APIs.
- `src/kernel/mrt_memory_pool.c`: static fixed-size block pool creation, allocation, free, ownership validation, and double-free detection.
- `include/myrtos/mrt_types.h`: add `MRT_MemoryPoolHandle`.
- `include/myrtos/mrt_config.h`: add `MRT_CFG_HEAP_ALIGNMENT`.
- `include/myrtos/mrt_queue.h`: add dynamic queue create/delete declarations.
- `src/kernel/mrt_queue.c`: implement dynamic queue allocation with MyRTOS heap and deletion cleanup.
- `CMakeLists.txt`: add heap and memory pool sources to `myrtos_kernel`.
- `tests/CMakeLists.txt`: add heap, memory pool, and dynamic queue tests.
- `tools/run_host_tests.py`: add new source files and test targets.
- `tests/unit/test_heap_initialize.c`: heap creation, alignment, query defaults, invalid arguments.
- `tests/unit/test_heap_linear.c`: bump allocation, alignment, exhaustion, no-free policy.
- `tests/unit/test_heap_free_list.c`: free/reuse behavior without adjacent coalescing.
- `tests/unit/test_heap_coalescing.c`: adjacent free block merge behavior.
- `tests/unit/test_memory_pool.c`: fixed block allocation, free, pool empty, invalid pointer, double free.
- `tests/coupling/test_queue_dynamic_allocation.c`: queue dynamic creation success, heap failure, state unchanged, deletion frees heap.
- `docs/verification/requirements_traceability_matrix.md`: update `R-002`, `R-008`, `R-009`, and `R-010`.
- `docs/verification/coupling_test_matrix.md`: update `C-007`, `C-023`, and `C-024`.
- `progress.md`, `findings.md`: record baseline, RED/GREEN evidence, commit hashes, and remaining gaps.

## API Signatures

```c
typedef enum MRT_HeapMode {
    MRT_HEAP_MODE_LINEAR = 0,
    MRT_HEAP_MODE_FREE_LIST,
    MRT_HEAP_MODE_COALESCING
} MRT_HeapMode;

MRT_Result MRT_HeapInitialize(void *buffer, size_t size, MRT_HeapMode mode);
void *MRT_Malloc(size_t size);
MRT_Result MRT_Free(void *ptr);
MRT_Result MRT_HeapGetFreeSize(size_t *out_free_size);
MRT_Result MRT_HeapGetMinimumEverFreeSize(size_t *out_minimum_free_size);

MRT_Result MRT_MemoryPoolCreateStatic(size_t block_size,
                                      size_t block_count,
                                      void *buffer,
                                      MRT_MemoryPool *storage,
                                      MRT_MemoryPoolHandle *out_pool);
MRT_Result MRT_MemoryPoolAlloc(MRT_MemoryPoolHandle pool, void **out_block);
MRT_Result MRT_MemoryPoolFree(MRT_MemoryPoolHandle pool, void *block);
MRT_Result MRT_MemoryPoolGetFreeCount(MRT_MemoryPoolHandle pool, size_t *out_free_count);

MRT_Result MRT_QueueCreate(size_t item_size,
                           size_t capacity,
                           MRT_QueueHandle *out_queue);
MRT_Result MRT_QueueDelete(MRT_QueueHandle queue);
```

## Task 1: Heap Initialization and Queries

**Files:**
- Create: `include/myrtos/mrt_heap.h`
- Create: `src/kernel/mrt_heap.c`
- Create: `tests/unit/test_heap_initialize.c`
- Modify: `include/myrtos/mrt_config.h`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`
- Modify: `tools/run_host_tests.py`

Steps:

- [x] Write failing tests for `MRT_HeapInitialize`, `MRT_HeapGetFreeSize`, `MRT_HeapGetMinimumEverFreeSize`, null heap buffer, too-small heap region, invalid heap mode, and repeated initialization resetting statistics.
- [x] Run `python tools\run_host_tests.py`; expected failure is missing `myrtos/mrt_heap.h`.
- [x] Add `MRT_CFG_HEAP_ALIGNMENT` defaulting to 8 bytes in `mrt_config.h`.
- [x] Implement heap control state, pointer alignment helpers, mode validation, initialization, free-size query, and minimum-ever-free query.
- [x] Add heap source to CMake and the host test runner.
- [x] Run `python tools\run_host_tests.py`; expect 45 test targets pass.
- [x] Commit `feat: add heap initialization`.

## Task 2: Linear Heap Allocation

**Files:**
- Create: `tests/unit/test_heap_linear.c`
- Modify: `include/myrtos/mrt_heap.h`
- Modify: `src/kernel/mrt_heap.c`
- Modify: `tests/CMakeLists.txt`
- Modify: `tools/run_host_tests.py`

Steps:

- [ ] Write failing tests for aligned `MRT_Malloc`, zero-size allocation returning null, exhaustion returning null, free-size/minimum-ever tracking, `MRT_Free(NULL)` as a no-op success, and freeing a linear-heap allocation returning `MRT_RESULT_OBJECT_BUSY`.
- [ ] Run `python tools\run_host_tests.py`; expected failure is missing `MRT_Malloc` and `MRT_Free`.
- [ ] Implement bump-pointer allocation for `MRT_HEAP_MODE_LINEAR`, aligned request sizing, free-size reduction, minimum-ever update, and linear free rejection.
- [ ] Run `python tools\run_host_tests.py`; expect 46 test targets pass.
- [ ] Commit `feat: add linear heap allocation`.

## Task 3: Free-List Heap Allocation and Reuse

**Files:**
- Create: `tests/unit/test_heap_free_list.c`
- Modify: `src/kernel/mrt_heap.c`
- Modify: `tests/CMakeLists.txt`
- Modify: `tools/run_host_tests.py`

Steps:

- [ ] Write failing tests for allocating two blocks, freeing the first block, reusing that freed block for a smaller allocation, rejecting a pointer outside the heap, rejecting a duplicate free, and reporting free bytes after reuse.
- [ ] Run `python tools\run_host_tests.py`; expected failure is that `MRT_HEAP_MODE_FREE_LIST` does not reuse freed blocks.
- [ ] Implement heap block headers, first-fit allocation, block splitting when the remainder can hold a header plus aligned payload, allocated/free state tracking, and free-list reuse without neighbor coalescing.
- [ ] Run `python tools\run_host_tests.py`; expect 47 test targets pass.
- [ ] Commit `feat: add free list heap allocation`.

## Task 4: Coalescing Heap Release

**Files:**
- Create: `tests/unit/test_heap_coalescing.c`
- Modify: `src/kernel/mrt_heap.c`
- Modify: `tests/CMakeLists.txt`
- Modify: `tools/run_host_tests.py`

Steps:

- [ ] Write failing tests for allocating three adjacent blocks, freeing two neighbors, allocating a block larger than either single freed block, and verifying `MRT_HeapGetFreeSize` reflects merged free space.
- [ ] Run `python tools\run_host_tests.py`; expected failure is that adjacent free blocks do not merge.
- [ ] Implement previous/next block links and coalescing for `MRT_HEAP_MODE_COALESCING` while preserving non-coalescing behavior in `MRT_HEAP_MODE_FREE_LIST`.
- [ ] Run `python tools\run_host_tests.py`; expect 48 test targets pass.
- [ ] Commit `feat: add coalescing heap release`.

## Task 5: Fixed Block Memory Pool

**Files:**
- Create: `include/myrtos/mrt_memory_pool.h`
- Create: `src/kernel/mrt_memory_pool.c`
- Create: `tests/unit/test_memory_pool.c`
- Modify: `include/myrtos/mrt_types.h`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`
- Modify: `tools/run_host_tests.py`

Steps:

- [ ] Write failing tests for static pool creation, null arguments, block size smaller than a pointer, zero block count, allocation until empty, free count query, freeing a block back to the pool, invalid pointer rejection, and double-free rejection.
- [ ] Run `python tools\run_host_tests.py`; expected failure is missing `myrtos/mrt_memory_pool.h`.
- [ ] Implement `MRT_MemoryPool` with aligned block size, caller-provided byte storage, singly-linked free list stored inside free blocks, free count, block-range validation, and free-list scan for double-free detection.
- [ ] Add memory pool source to CMake and the host test runner.
- [ ] Run `python tools\run_host_tests.py`; expect 49 test targets pass.
- [ ] Commit `feat: add fixed block memory pool`.

## Task 6: Dynamic Queue Creation Coupled to Heap

**Files:**
- Create: `tests/coupling/test_queue_dynamic_allocation.c`
- Modify: `include/myrtos/mrt_queue.h`
- Modify: `src/kernel/mrt_queue.c`
- Modify: `tests/CMakeLists.txt`
- Modify: `tools/run_host_tests.py`

Steps:

- [ ] Write failing tests for `MRT_QueueCreate` successful dynamic allocation, FIFO send/receive on the dynamic queue, `MRT_QueueDelete` returning heap memory, allocation failure returning `MRT_RESULT_NO_MEMORY`, and failed creation leaving heap free-size unchanged.
- [ ] Run `python tools\run_host_tests.py`; expected failure is missing dynamic queue APIs.
- [ ] Implement dynamic queue allocation as one heap block that contains `MRT_Queue` followed by aligned item storage, mark `static_storage=false`, and reject dynamic creation when `MRT_CFG_SUPPORT_DYNAMIC_ALLOCATION` is 0.
- [ ] Implement `MRT_QueueDelete` so dynamic queues are freed through `MRT_Free`, while static queues reject deletion with `MRT_RESULT_OBJECT_BUSY`.
- [ ] Run `python tools\run_host_tests.py`; expect 50 test targets pass.
- [ ] Commit `feat: add dynamic queue allocation`.

## Task 7: Verification Matrix Update

**Files:**
- Modify: `docs/verification/requirements_traceability_matrix.md`
- Modify: `docs/verification/coupling_test_matrix.md`
- Modify: `progress.md`
- Modify: `findings.md`

Steps:

- [ ] Run `python tools\run_host_tests.py`; expect foundation, scheduler, queues, semaphores, mutexes, event groups, task notifications, timers, stream/message buffers, heap, memory pool, and dynamic queue tests pass.
- [ ] Update `R-002`, `R-008`, `R-009`, and `R-010` evidence with heap, memory pool, and dynamic queue source/tests.
- [ ] Mark `C-007` verified for queue dynamic allocation failure.
- [ ] Mark `C-023` verified for heap allocation failure and object creation failure state.
- [ ] Mark `C-024` verified for adjacent free block coalescing.
- [ ] Record commit hashes and final test output in `progress.md`.
- [ ] Commit `docs: record memory management verification evidence`.
