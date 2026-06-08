# MyRTOS STM32 MPU Helper Plan

## Goal
Add a host-verifiable STM32 MPU region layout helper that normalizes arbitrary memory ranges into MPU-compatible power-of-two regions, with tests and manual coverage.

## Why
The current preview still leaves MPU as a reservation. A layout helper makes the STM32 port story more concrete without pretending to run on hardware we do not have.

## Phase 1: RED
- Add a test that exercises the helper API before it exists.
- Confirm the build fails on missing declarations.

## Phase 2: API
- Add a new STM32 portable helper API for MPU layout normalization.
- Keep the API original, C-only, and host-testable.
- Add Chinese function comments and examples.

## Phase 3: Tests
- Cover invalid arguments.
- Cover exact power-of-two regions.
- Cover ranges that must round up.

## Phase 4: Docs
- Update the API catalog, manual STM32 porting chapter, and verification matrices.

## Phase 5: Verify
- Re-run host tests, static checks, and diff checks.
