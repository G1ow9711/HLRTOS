# MyRTOS Embedded Smoke Projects Plan

## Goal
Build minimal embedded smoke projects for STM32 and DSP, keep them original, and extend the Chinese manual with concrete porting steps and build flow.

## Scope
- Add a failing verification step for missing smoke project directories.
- Create `examples/stm32/` as a compilable STM32 Cortex-M smoke project.
- Create `examples/dsp/` as a DSP C28x-style smoke/model project.
- Update the manual with step-by-step porting guidance and build commands.
- Update requirement and coupling matrices.
- Record what is verified on host and what still needs real hardware.

## Phase 1: RED
- Add a verification script that expects both smoke project directories and minimal build files.
- Run it before implementation.
- Confirm failure is caused by missing smoke scaffolding.

## Phase 2: STM32 Smoke
- Add minimal STM32 example source, startup glue, linker script, and build recipe.
- Use ARM GCC as the compile target.
- Keep the code board-neutral and free of vendor HAL dependence where possible.
- Exercise task creation, tick, queue ISR, and timer service paths.

## Phase 3: DSP Smoke/Model
- Add DSP example source and a host-verifiable model build.
- Keep the public port contract aligned with the existing DSP helper layer.
- Provide a TI toolchain template and a host model for verification.

## Phase 4: Manual and Traceability
- Expand the STM32 and DSP porting chapters with explicit migration steps.
- Add example tree layout, build commands, and troubleshooting notes.
- Update requirement and coupling matrices with new evidence.

## Phase 5: Verification
- Re-run host tests and the new smoke verification script.
- Record real hardware smoke as pending if the board/toolchain is still unavailable.
- Update the final verification report with the current boundary.
