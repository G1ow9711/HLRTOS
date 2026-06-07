# MyRTOS Stream and Message Buffers Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add original MyRTOS stream buffers and message buffers with static allocation, byte FIFO behavior, packet-preserving messages, ISR variants, scheduler wake coupling, Chinese comments, and verification evidence.

**Architecture:** Stream buffers use caller-provided byte storage as a circular FIFO with one writer and one reader as the intended embedded usage model. Message buffers reuse the same storage style but add a fixed 32-bit length header before each message so receives preserve packet boundaries and never return half a message. Dynamic creation remains reserved for the memory-management plan because heap behavior must be tested with the heap module.

**Tech Stack:** C99, GCC-backed `tools/run_host_tests.py`, CMake metadata, host unit tests under `tests/unit/`, coupling tests under `tests/coupling/`, existing task object wait helpers for reader/writer blocking.

---

## File Structure

- `include/myrtos/mrt_stream_buffer.h`: stream buffer control block, public stream APIs, ISR variants, and query APIs.
- `src/kernel/mrt_stream_buffer.c`: circular byte FIFO implementation, task wait lists, ISR wrappers, trigger-level wake logic.
- `include/myrtos/mrt_message_buffer.h`: message buffer control block and public packet APIs.
- `src/kernel/mrt_message_buffer.c`: packet send/receive using a 32-bit length header and no-partial-message policy.
- `CMakeLists.txt`: add stream/message buffer source files to `myrtos_kernel`.
- `tests/CMakeLists.txt`: add new unit and coupling tests.
- `tools/run_host_tests.py`: add new source files and test targets.
- `tests/unit/test_stream_buffer_create_static.c`: stream static creation, trigger-level validation, query defaults.
- `tests/unit/test_stream_buffer_send_receive.c`: byte FIFO send/receive, wrap-around, reset, partial nonblocking behavior.
- `tests/unit/test_stream_buffer_isr.c`: ISR send/receive validation and no-block behavior.
- `tests/coupling/test_stream_buffer_isr_wakes_reader.c`: ISR send wakes a blocked reader and sets `should_yield`.
- `tests/unit/test_message_buffer_create_static.c`: message static creation and capacity validation.
- `tests/unit/test_message_buffer_send_receive.c`: packet boundaries, output buffer too small, capacity edge, no half packet.
- `tests/unit/test_message_buffer_isr.c`: ISR send/receive message integrity and invalid context checks.
- `docs/verification/requirements_traceability_matrix.md`: update `R-002`, `R-008`, `R-009`, `R-010`.
- `docs/verification/coupling_test_matrix.md`: update `C-020`, `C-021`, `C-022`.
- `progress.md`, `findings.md`: record baseline, RED/GREEN evidence, commit hashes, and remaining gaps.

## API Signatures

```c
MRT_Result MRT_StreamBufferCreateStatic(size_t capacity,
                                        size_t trigger_level,
                                        void *buffer,
                                        MRT_StreamBuffer *storage,
                                        MRT_StreamBufferHandle *out_stream);
MRT_Result MRT_StreamBufferSend(MRT_StreamBufferHandle stream,
                                const void *data,
                                size_t length,
                                MRT_Timeout timeout,
                                size_t *out_sent);
MRT_Result MRT_StreamBufferReceive(MRT_StreamBufferHandle stream,
                                   void *out_data,
                                   size_t length,
                                   MRT_Timeout timeout,
                                   size_t *out_received);
MRT_Result MRT_StreamBufferSendFromISR(MRT_StreamBufferHandle stream,
                                       const void *data,
                                       size_t length,
                                       size_t *out_sent,
                                       bool *should_yield);
MRT_Result MRT_StreamBufferReceiveFromISR(MRT_StreamBufferHandle stream,
                                          void *out_data,
                                          size_t length,
                                          size_t *out_received);
MRT_Result MRT_StreamBufferBytesAvailable(MRT_StreamBufferHandle stream, size_t *out_bytes);
MRT_Result MRT_StreamBufferSpacesAvailable(MRT_StreamBufferHandle stream, size_t *out_spaces);
MRT_Result MRT_StreamBufferReset(MRT_StreamBufferHandle stream);

MRT_Result MRT_MessageBufferCreateStatic(size_t capacity,
                                         void *buffer,
                                         MRT_MessageBuffer *storage,
                                         MRT_MessageBufferHandle *out_message_buffer);
MRT_Result MRT_MessageBufferSend(MRT_MessageBufferHandle message_buffer,
                                 const void *message,
                                 size_t length,
                                 MRT_Timeout timeout,
                                 size_t *out_sent);
MRT_Result MRT_MessageBufferReceive(MRT_MessageBufferHandle message_buffer,
                                    void *out_message,
                                    size_t output_capacity,
                                    MRT_Timeout timeout,
                                    size_t *out_received);
MRT_Result MRT_MessageBufferSendFromISR(MRT_MessageBufferHandle message_buffer,
                                        const void *message,
                                        size_t length,
                                        size_t *out_sent,
                                        bool *should_yield);
MRT_Result MRT_MessageBufferReceiveFromISR(MRT_MessageBufferHandle message_buffer,
                                           void *out_message,
                                           size_t output_capacity,
                                           size_t *out_received);
MRT_Result MRT_MessageBufferBytesAvailable(MRT_MessageBufferHandle message_buffer, size_t *out_bytes);
MRT_Result MRT_MessageBufferSpacesAvailable(MRT_MessageBufferHandle message_buffer, size_t *out_spaces);
MRT_Result MRT_MessageBufferReset(MRT_MessageBufferHandle message_buffer);
```

## Task 1: Static Stream Buffer Creation

**Files:**
- Create: `include/myrtos/mrt_stream_buffer.h`
- Create: `src/kernel/mrt_stream_buffer.c`
- Create: `tests/unit/test_stream_buffer_create_static.c`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`
- Modify: `tools/run_host_tests.py`

Steps:

- [x] Write failing tests for `MRT_StreamBufferCreateStatic`, `MRT_StreamBufferBytesAvailable`, `MRT_StreamBufferSpacesAvailable`, null storage, null byte buffer, null output handle, zero capacity, zero trigger level, and trigger level greater than capacity.
- [x] Run `python tools\run_host_tests.py`; expected failure is missing `myrtos/mrt_stream_buffer.h`.
- [x] Implement `MRT_StreamBuffer` with byte storage pointer, capacity, read index, write index, byte count, trigger level, waiting reader list, waiting writer list, and static-storage flag.
- [x] Implement static creation and query APIs.
- [x] Add stream buffer source to CMake and host runner.
- [x] Run `python tools\run_host_tests.py`; expect 38 test targets pass.
- [x] Commit `feat: add static stream buffer creation`.

## Task 2: Stream Buffer Send/Receive

**Files:**
- Create: `tests/unit/test_stream_buffer_send_receive.c`
- Modify: `include/myrtos/mrt_stream_buffer.h`
- Modify: `src/kernel/mrt_stream_buffer.c`
- Modify: `tests/CMakeLists.txt`
- Modify: `tools/run_host_tests.py`

Steps:

- [x] Write failing tests for nonblocking send, nonblocking receive, byte order preservation, write/read wrap-around, partial send when free space is smaller than requested length, empty receive returning `MRT_RESULT_OBJECT_EMPTY`, and reset clearing indexes/count.
- [x] Run `python tools\run_host_tests.py`; expected failure is missing stream send/receive/reset declarations.
- [x] Implement circular byte copy helpers for write, read, and peek-free-space queries.
- [x] Implement `MRT_StreamBufferSend`, `MRT_StreamBufferReceive`, and `MRT_StreamBufferReset`.
- [x] Run `python tools\run_host_tests.py`; expect 39 test targets pass.
- [x] Commit `feat: add stream buffer send receive`.

## Task 3: Stream Buffer ISR and Reader Wake Coupling

**Files:**
- Create: `tests/unit/test_stream_buffer_isr.c`
- Create: `tests/coupling/test_stream_buffer_isr_wakes_reader.c`
- Modify: `include/myrtos/mrt_stream_buffer.h`
- Modify: `src/kernel/mrt_stream_buffer.c`
- Modify: `src/kernel/mrt_task.h`
- Modify: `tests/CMakeLists.txt`
- Modify: `tools/run_host_tests.py`

Steps:

- [x] Write failing unit tests for `MRT_StreamBufferSendFromISR`, `MRT_StreamBufferReceiveFromISR`, ISR context validation, task-context misuse returning `MRT_RESULT_INVALID_CONTEXT`, and conservative no-wait behavior.
- [x] Write failing coupling test where a high-priority task blocks in `MRT_StreamBufferReceive(stream, out, len, timeout, &received)` and an ISR send writes enough bytes to meet trigger level, wakes the reader, and sets `should_yield=true`.
- [x] Run `python tools\run_host_tests.py`; expected failure is missing ISR APIs and reader wait reason.
- [x] Add `MRT_TASK_WAIT_REASON_STREAM_RECEIVE` to task wait reasons.
- [x] Implement stream receive blocking on empty buffer with timeout using the existing object-wait helper.
- [x] Implement task-context send and ISR send wake logic when available bytes reach trigger level.
- [x] Implement ISR receive with zero-timeout semantics and no writer wake behavior.
- [x] Run `python tools\run_host_tests.py`; expect 41 test targets pass.
- [x] Commit `feat: add stream buffer ISR wake coupling`.

## Task 4: Static Message Buffer Creation

**Files:**
- Create: `include/myrtos/mrt_message_buffer.h`
- Create: `src/kernel/mrt_message_buffer.c`
- Create: `tests/unit/test_message_buffer_create_static.c`
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`
- Modify: `tools/run_host_tests.py`

Steps:

- [x] Write failing tests for `MRT_MessageBufferCreateStatic`, bytes/spaces queries, null storage, null byte buffer, null output handle, and capacity smaller than one length header plus one byte.
- [x] Run `python tools\run_host_tests.py`; expected failure is missing `myrtos/mrt_message_buffer.h`.
- [x] Implement `MRT_MessageBuffer` with byte storage pointer, capacity, read index, write index, used bytes, waiting reader list, waiting writer list, and static-storage flag.
- [x] Implement static creation and query APIs.
- [x] Add message buffer source to CMake and host runner.
- [x] Run `python tools\run_host_tests.py`; expect 42 test targets pass.
- [x] Commit `feat: add static message buffer creation`.

## Task 5: Message Buffer Send/Receive

**Files:**
- Create: `tests/unit/test_message_buffer_send_receive.c`
- Modify: `include/myrtos/mrt_message_buffer.h`
- Modify: `src/kernel/mrt_message_buffer.c`
- Modify: `tests/CMakeLists.txt`
- Modify: `tools/run_host_tests.py`

Steps:

- [x] Write failing tests for sending two messages, receiving them as complete packets, preserving packet boundaries, rejecting a message too large for total capacity, rejecting send when remaining space cannot hold length header plus payload, output buffer too small returning `MRT_RESULT_OBJECT_FULL` without removing the message, and reset clearing stored messages.
- [x] Run `python tools\run_host_tests.py`; expected failure is missing message send/receive/reset declarations.
- [x] Implement 32-bit little-endian length header write/read helpers.
- [x] Implement packet send only when full header plus payload fits.
- [x] Implement packet receive only when the output buffer can hold the whole next message.
- [x] Implement `MRT_MessageBufferReset`.
- [x] Run `python tools\run_host_tests.py`; expect 43 test targets pass.
- [x] Commit `feat: add message buffer send receive`.

## Task 6: Message Buffer ISR APIs

**Files:**
- Create: `tests/unit/test_message_buffer_isr.c`
- Modify: `include/myrtos/mrt_message_buffer.h`
- Modify: `src/kernel/mrt_message_buffer.c`
- Modify: `src/kernel/mrt_task.h`
- Modify: `tests/CMakeLists.txt`
- Modify: `tools/run_host_tests.py`

Steps:

- [x] Write failing tests for `MRT_MessageBufferSendFromISR`, `MRT_MessageBufferReceiveFromISR`, ISR context validation, task-context misuse returning `MRT_RESULT_INVALID_CONTEXT`, packet integrity, capacity rejection, and `should_yield=true` when a reader is woken.
- [x] Run `python tools\run_host_tests.py`; expected failure is missing ISR APIs and message wait reason.
- [x] Add `MRT_TASK_WAIT_REASON_MESSAGE_RECEIVE` to task wait reasons.
- [x] Implement message receive blocking on empty buffer with timeout using object-wait helper.
- [x] Implement task-context send and ISR send wake logic when at least one complete message is available.
- [x] Implement ISR receive with zero-timeout semantics.
- [x] Run `python tools\run_host_tests.py`; expect 44 test targets pass.
- [x] Commit `feat: add message buffer ISR APIs`.

## Task 7: Verification Matrix Update

**Files:**
- Modify: `docs/verification/requirements_traceability_matrix.md`
- Modify: `docs/verification/coupling_test_matrix.md`
- Modify: `progress.md`
- Modify: `findings.md`

Steps:

- [ ] Run `python tools\run_host_tests.py`; expect foundation, scheduler, queues, semaphores, mutexes, event groups, task notifications, timers, stream buffers, and message buffers pass.
- [ ] Update `R-002`, `R-008`, `R-009`, and `R-010` evidence with stream/message buffer sources and tests.
- [ ] Mark `C-020` verified for stream buffer wrap-around.
- [ ] Mark `C-021` verified for ISR send waking a blocked stream reader.
- [ ] Mark `C-022` verified for message buffer capacity and no-half-packet behavior.
- [ ] Record commit hashes and final test output in `progress.md`.
- [ ] Commit `docs: record stream message buffer verification evidence`.
