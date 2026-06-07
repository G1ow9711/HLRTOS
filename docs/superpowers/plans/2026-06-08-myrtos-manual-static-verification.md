# MyRTOS Manual And Static Verification Plan

> **For agentic workers:** Use TDD for verification scripts. The manual must be original Chinese text while following an official reference-manual style: scope, usage restrictions, chapterized API groups, function prototype, summary, parameters, return values, notes, examples, appendices, and porting chapters.

**Goal:** Write the Chinese MyRTOS reference manual, include detailed STM32/DSP porting steps, and add static checks for manual API coverage, Chinese function comments, and originality/symbol hygiene.

**Reference Structure:** FreeRTOS official reference material uses an "About This Manual" opening, API usage restrictions, chapterized API groups, function prototypes, summaries, parameters, return values, notes, examples, and appendices for data types/macros. MyRTOS will follow that structure at a template level only; all text, API names, examples, and porting steps remain original.

**Tech Stack:** Markdown manual, Python verification scripts, host GCC runner for regression, project-local docs.

---

### Task 1: Static Verification Script RED

**Files:**
- Create: `tools/verify/check_api_manual_coverage.py`
- Create: `tools/verify/check_chinese_comments.py`
- Create: `tools/verify/check_original_symbols.py`

- [x] **Step 1: Add scripts**
- [x] **Step 2: Run scripts and confirm expected failure because manual is missing/incomplete**
- [x] **Step 3: Record RED evidence**

### Task 2: Chinese Reference Manual

**Files:**
- Create: `docs/manual/MyRTOS_Reference_Manual_zh.md`

- [x] **Step 1: Write original manual chapters**
- [x] **Step 2: Include every API listed in `docs/api/myrtos_api_catalog.md`**
- [x] **Step 3: Add detailed STM32 porting steps**
- [x] **Step 4: Add detailed DSP porting steps**
- [x] **Step 5: Add appendices for types, result codes, macros, and verification commands**

### Task 3: Static Verification GREEN

**Files:**
- Modify as needed based on script findings.

- [x] **Step 1: Run API manual coverage script**
- [x] **Step 2: Run Chinese comment coverage script**
- [x] **Step 3: Run originality/symbol script**
- [x] **Step 4: Fix any failures and re-run**

### Task 4: Matrices And Evidence

**Files:**
- Modify: `docs/verification/requirements_traceability_matrix.md`
- Modify: `docs/verification/coupling_test_matrix.md`
- Modify: `docs/api/myrtos_api_catalog.md`
- Modify: `findings.md`
- Modify: `progress.md`
- Modify: `task_plan.md`

- [x] **Step 1: Update R-005, R-006, R-007, R-011 evidence**
- [x] **Step 2: Update C-032 and C-033 evidence**
- [x] **Step 3: Run `python tools\run_host_tests.py`**
- [ ] **Step 4: Commit and push `feature/manual-static-verification`**
