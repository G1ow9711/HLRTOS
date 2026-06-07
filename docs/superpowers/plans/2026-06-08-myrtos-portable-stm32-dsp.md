# MyRTOS STM32/DSP Portable Layer Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:test-driven-development for every task. Each task must start with a failing host test and end with a fresh full `python tools\run_host_tests.py` run.

**Goal:** Add original, host-testable STM32 Cortex-M and DSP C28x-style portable-layer contract helpers, then update API and verification evidence.

**Architecture:** Keep common kernel code independent from chip registers. The new port helpers describe stack layout, tick setup math, interrupt priority masking, and DSP software-interrupt request state in pure C so they can be tested on the host. Real vector-table, SysTick, PendSV/SVC, and DSP interrupt hook-up steps will be documented in the final manual.

**Tech Stack:** C99, MyRTOS public types, host GCC test runner, CMake source lists, original Chinese comments.

---

### Task 1: STM32 Cortex-M Stack Frame Contract

**Files:**
- Create: `include/myrtos/portable/mrt_port_stm32_cm.h`
- Create: `src/portable/stm32_cm/mrt_port_stm32_cm.c`
- Test: `tests/port_mock/test_port_stm32_stack.c`

- [x] **Step 1: Write failing stack-frame test**
- [x] **Step 2: Run `python tools\run_host_tests.py` and confirm missing header/API failure**
- [x] **Step 3: Implement host-testable stack initialization helper**
- [x] **Step 4: Re-run full host tests**
- [x] **Step 5: Record evidence**

### Task 2: STM32 Tick And Interrupt Priority Helpers

**Files:**
- Modify: `include/myrtos/portable/mrt_port_stm32_cm.h`
- Modify: `src/portable/stm32_cm/mrt_port_stm32_cm.c`
- Test: `tests/port_mock/test_port_stm32_tick_priority.c`

- [x] **Step 1: Write failing tick/priority helper test**
- [x] **Step 2: Run full host tests and confirm missing API failure**
- [x] **Step 3: Implement SysTick reload calculation and BASEPRI priority encoding helpers**
- [x] **Step 4: Re-run full host tests**
- [x] **Step 5: Record evidence**

### Task 3: DSP C28x-Style Stack Frame Contract

**Files:**
- Create: `include/myrtos/portable/mrt_port_dsp_c28x.h`
- Create: `src/portable/dsp_c28x/mrt_port_dsp_c28x.c`
- Test: `tests/port_mock/test_port_dsp_stack.c`

- [x] **Step 1: Write failing DSP stack-frame test**
- [x] **Step 2: Run full host tests and confirm missing header/API failure**
- [x] **Step 3: Implement host-testable DSP stack initialization helper**
- [x] **Step 4: Re-run full host tests**
- [x] **Step 5: Record evidence**

### Task 4: DSP Software Interrupt Context Switch Model

**Files:**
- Modify: `include/myrtos/portable/mrt_port_dsp_c28x.h`
- Modify: `src/portable/dsp_c28x/mrt_port_dsp_c28x.c`
- Test: `tests/port_mock/test_port_dsp_context.c`

- [x] **Step 1: Write failing context-switch request test**
- [x] **Step 2: Run full host tests and confirm missing API/behavior failure**
- [x] **Step 3: Implement software interrupt request, nesting, and acknowledgement helpers**
- [x] **Step 4: Re-run full host tests**
- [x] **Step 5: Record evidence**

### Task 5: Build, API, And Verification Evidence

**Files:**
- Modify: `CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`
- Modify: `tools/run_host_tests.py`
- Modify: `docs/api/myrtos_api_catalog.md`
- Modify: `docs/verification/requirements_traceability_matrix.md`
- Modify: `docs/verification/coupling_test_matrix.md`
- Modify: `findings.md`
- Modify: `progress.md`

- [x] **Step 1: Add new sources/tests to build systems**
- [x] **Step 2: Update API catalog and matrices**
- [x] **Step 3: Run `python tools\run_host_tests.py`**
- [ ] **Step 4: Commit and push `feature/portable-stm32-dsp`**
