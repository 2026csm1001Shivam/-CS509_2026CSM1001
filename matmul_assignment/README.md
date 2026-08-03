# CS509 Laboratory Repository - Matrix Multiplication Assignment

## Repository Overview

This repository contains the CS509 laboratory assignments of the student,
submitted in **Individual (Single) mode**. Currently it holds **Assignment 01 -
Matrix Multiplication (Simple vs Blocked/Tiled)** implemented in **C++**.

The repository follows the recommended structure from the CS509 Lab Work
Guidelines:

- Core algorithm implementations are kept in `assignment_01/src/`
- A dedicated driver per assignment lives in `assignment_01/driver/`
- Test files (one test case each) live in `assignment_01/tests/`
- Per-assignment outputs are written to `assignment_01/outputs/`
- A common wrapper (menu interface) is provided in `common_wrapper/`

## Student / Pair Details

| Field | Value |
|---|---|
| Name | *(fill in your name)* |
| Entry Number | 2026CSM1001 |
| Assignment Mode | Single (Individual) |
| Course | CS509 - Lab Work, First-Year M.Tech CSE, 2026 |

## Language and Environment

- **Programming language:** C++ (used consistently for all assignments)
- **Compiler:** `g++` (GCC), C++11 standard (`-std=c++11`)
- **Build tool:** `make` (optional; direct `g++` commands also documented)
- **Compilation flags:** `-O2 -Wall -Wextra -std=c++11`
- **Timing method:** `std::chrono::steady_clock`, measuring **only** the
  algorithm execution time (see "General Conventions" below)
- **Machine used for the measurements below:** *(fill in: CPU model, RAM, OS,
  compiler version - important so the numbers can be interpreted)*

## Directory Structure

```
matmul_assignment/                  <- repository root
|-- README.md                       <- this file (full documentation)
|-- Makefile                        <- top-level build file
|-- common_wrapper/
|   `-- wrapper.cpp                 <- common wrapper: menu interface
`-- assignment_01/                  <- Assignment 01 (Matrix Multiplication)
    |-- src/
    |   |-- matmul.h                <- core algorithm header
    |   `-- matmul.cpp              <- core algorithms (simple + blocked)
    |-- driver/
    |   `-- driver.cpp              <- dedicated driver (reads test files,
    |                                   times algorithms, writes outputs)
    |-- tests/
    |   |-- test_01.txt             <- one test case per file
    |   |-- test_02.txt
    |   |-- ...
    |   `-- test_06.txt
    `-- outputs/                    <- per-test output files written here
        `-- output_test_XX.txt
```

## Quick Start - Terminal Commands

Run all commands from the repository root (`matmul_assignment/`):

```bash
# 1) Build everything (driver + wrapper) with make
make

# 2) Run ONE test file (results printed + saved to outputs/)
make run-one TEST=test_01.txt
./assignment_01/driver/driver assignment_01/tests/test_01.txt

# 3) Run ALL test files
make run-all
./assignment_01/driver/driver --all

# 4) Interactive menu (common wrapper)
make wrapper
./common_wrapper/wrapper

# 5) Clean up binaries
make clean
```

Without `make` (e.g. plain Windows terminal), build and run directly:

```bash
g++ -O2 -Wall -Wextra -std=c++11 -I assignment_01/src \
    assignment_01/src/matmul.cpp assignment_01/driver/driver.cpp \
    -o assignment_01/driver/driver.exe

assignment_01\driver\driver.exe assignment_01\tests\test_01.txt   # one test
assignment_01\driver\driver.exe --all                              # all tests
```

## Common Wrapper: Build and Usage

The common wrapper is the repository-level interface. It **invokes the
dedicated driver** of the selected assignment; it does not replace it.

Build (from the repository root):

```bash
g++ -O2 -Wall -Wextra -std=c++11 -o common_wrapper/wrapper common_wrapper/wrapper.cpp
```

Run:

```bash
./common_wrapper/wrapper        # Windows: .\common_wrapper\wrapper.exe
```

Menu options:

1. **List available assignments** - shows registered assignments and whether
   their directories are present.
2. **Compile an assignment** - compiles the core library + driver of the
   selected assignment.
3. **Run one test file of an assignment** - lists the available test files,
   asks for one, compiles the driver if needed, and runs it on that file.
4. **Run all test files of an assignment** - runs every `test_XX.txt`.
5. **Compile and run all assignments** - compiles everything, then runs all
   tests of all assignments.
6. **Exit**

Clear error messages are printed when a source file, test file, or executable
is unavailable.

## General Conventions

### Test files

- Every assignment has multiple test files with sequential names
  (`test_01.txt`, `test_02.txt`, ...).
- **Each test file contains exactly one test case.**
- For this (matrix) assignment, a test case is the matrix dimension `N` and
  the block/tile size `BLOCK`. The matrices themselves are generated
  reproducibly by the driver (fixed seeds, `A` seeded with 42, `B` with 7),
  so results are deterministic across runs and machines.
- Test file syntax: `KEY value` lines, keys are case-insensitive (`N`,
  `BLOCK`); lines starting with `#` are comments.

### Outputs

- For every test case the driver prints the **computed result** (checksum of
  the result matrix `C`, plus a PASSED/FAILED correctness check) and the
  **algorithm execution time in milliseconds (ms)** - the unit is always
  stated explicitly.
- A copy of the output is saved to `assignment_01/outputs/output_test_XX.txt`.

### Runtime measurement

- **"Track time" = execution time of the algorithm only.**
- The timer starts immediately before the algorithm call and stops
  immediately after it finishes.
- Input reading, file handling, parsing, matrix generation (input
  preparation), correctness verification, and output formatting/printing are
  all **outside** the timed region.
- Two separate timed regions are used: one around `matmul_simple`, one around
  `matmul_blocked`.
- The reported unit is milliseconds (ms).

---

# Assignment 01 - Matrix Multiplication: Simple vs Blocked/Tiled

## Assignment Mode

Single (Individual)

## Objective

Implement dense square matrix multiplication `C = A x B` in two ways, compare
their performance, and understand **why** one is faster even though both
perform the same number of arithmetic operations:

1. **Simple (naive) multiplication** - the standard triple-nested loop.
2. **Blocked (tiled) multiplication** - the same computation reorganized onto
   small `BS x BS` sub-matrices ("tiles") that stay resident in cache.

Both algorithms are **O(n^3)** in arithmetic operations. The point is that
*algorithmic complexity is not the whole story* - **memory access patterns**
matter enormously on real hardware.

## Algorithm / Approach

### Why the naive version is slow for large N

```c
for i in 0..n:
  for j in 0..n:
    for k in 0..n:
      C[i][j] += A[i][k] * B[k][j]
```

- `A[i][k]` is accessed row-wise -> cache-friendly (contiguous memory).
- `B[k][j]` is accessed column-wise -> each access jumps `n` elements ahead ->
  poor spatial locality -> frequent cache misses once the matrix no longer
  fits in cache.

### Why blocking/tiling helps

Instead of computing the full `C` in one pass, the matrices are split into
`BS x BS` blocks and multiplied block-by-block (`ii`, `jj`, `kk` loops), so
the data needed for one block's worth of work stays resident in cache while it
is reused, trading a little loop overhead for far fewer cache misses. A good
`BS` makes each block comfortably fit in L1/L2 cache.

## Input Format

Each test file contains **exactly one test case**, e.g. `test_01.txt`:

```
# Test case 01 - Matrix Multiplication (Simple vs Blocked)
N 128
BLOCK 16
```

- `N`: matrix dimension (square `N x N` matrix of doubles)
- `BLOCK`: tile size `BS` used by the blocked version
- Assumptions/constraints: positive integers; matrices are generated
  reproducibly (seeds 42 and 7, values in [0,9]) so both versions see
  identical inputs.

## Helper Functions / File Structure

| File | Purpose |
|---|---|
| `src/matmul.h` | Declares `Matrix`, `matmul_simple`, `matmul_blocked`, `matrices_equal`, `matrix_checksum` |
| `src/matmul.cpp` | Core algorithm library (no I/O, no timing) |
| `driver/driver.cpp` | Dedicated driver: parses the test file, prepares input, times each algorithm call, verifies, prints and saves the result |
| `tests/test_01..06.txt` | One test case each |
| `outputs/output_test_XX.txt` | Saved output of each run |

## Compilation and Execution (terminal commands)

Run all commands from the repository root (`matmul_assignment/`).

**Step 1 - Build** (needs `g++` and `make`):

```bash
make                 # builds driver + wrapper
make driver          # builds only the assignment_01 driver
make wrapper         # builds only the common wrapper
```

**Step 2 - Run one test file** (results printed and saved to `outputs/`):

```bash
make run-one TEST=test_01.txt
# or directly:
./assignment_01/driver/driver assignment_01/tests/test_01.txt
```

**Step 3 - Run all test files:**

```bash
make run-all
./assignment_01/driver/driver --all
```

**Step 4 - Interactive menu (common wrapper):**

```bash
./common_wrapper/wrapper
```

**Step 5 - Clean up binaries:**

```bash
make clean
```

Without `make` (e.g. plain Windows terminal), build and run directly:

```bash
g++ -O2 -Wall -Wextra -std=c++11 -I assignment_01/src \
    assignment_01/src/matmul.cpp assignment_01/driver/driver.cpp \
    -o assignment_01/driver/driver.exe

assignment_01\driver\driver.exe assignment_01\tests\test_01.txt   # one test
assignment_01\driver\driver.exe --all                              # all tests
```

For `N <= 8` the driver additionally prints matrices `A`, `B`, and `C` so a
small case can be verified by hand.

## Test Cases and Result Table

Input type: **random square matrix (reproducible)**, generated by the driver.
"Expected output" is the checksum (sum of all elements) of `C = A x B`,
verified against the naive implementation; "Actual output" is the checksum
produced by the (verified) blocked implementation.

| Mode | Test File | Input Type | Input Size / Dimensions | Expected Output (checksum of C) | Actual Output (checksum of C) | Algorithm Time - Simple | Algorithm Time - Blocked | Speedup |
|---|---|---|---|---|---|---|---|---|
| Single | test_01.txt | Random matrix (seeded) | N = 128, BS = 16 | 42282354 | 42282354 | 3.519 ms | 2.020 ms | 1.74x |
| Single | test_02.txt | Random matrix (seeded) | N = 256, BS = 32 | 340101676 | 340101676 | 35.370 ms | 32.480 ms | 1.09x |
| Single | test_03.txt | Random matrix (seeded) | N = 512, BS = 32 | 2720561445 | 2720561445 | 819.858 ms | 365.791 ms | 2.24x |
| Single | test_04.txt | Random matrix (seeded) | N = 512, BS = 64 | 2720561445 | 2720561445 | 820.929 ms | 415.801 ms | 1.97x |
| Single | test_05.txt | Random matrix (seeded) | N = 1024, BS = 64 | 21759139504 | 21759139504 | 5661.053 ms | 2820.937 ms | 2.01x |
| Single | test_06.txt | Random matrix (seeded) | N = 1024, BS = 128 | 21759139504 | 21759139504 | 5602.604 ms | 3599.048 ms | 1.56x |

All correctness checks: **PASSED** (simple == blocked).

> Note: measurements above were taken with `g++ -O2` on the machine described
> in "Language and Environment". Re-measure on the machine used for the
> report and fill in its details.

## Experiments (for the report)

Run at least the sizes `128, 256, 512, 1024` and block sizes `8, 16, 32, 64,
128`, then produce at least two charts:

- Execution time vs. matrix size `N` (one line for simple, one for blocked)
- Execution time (or speedup) vs. block size, for a fixed `N`

Questions to answer in the report:

- At what matrix size does the blocked version start clearly outperforming the
  simple version? Why might small matrices show little/no difference?
- What happens if the block size is too small (e.g. 2)? Too large (e.g. equal
  to N)? Explain in terms of caching.
- Both algorithms do the exact same number of floating-point operations. Where
  does the speedup actually come from?
- (Bonus) What is your machine's L1 cache size? What block size would you
  predict to be near-optimal, and does your data support that?

## Complexity

- **Time:** O(n^3) for both versions (same arithmetic operations).
- **Space:** O(n^2) for the three stored matrices (contiguous flat `vector`).
- Blocking changes only the memory-access pattern, not the complexity.

## References

- (Fill in any references used, e.g. *Computer Architecture: A Quantitative
  Approach* (Hennessy & Patterson), CSAPP (Bryant & O'Hallaron), cppreference
  for `std::chrono` and `std::mt19937`, etc.)

---

## Optional extensions (bonus)

- `-O3 -march=native` build vs. `-O2`
- Loop reordering (`i-k-j`) in the simple version without blocking
- OpenMP parallelization (`#pragma omp parallel for`)
- Rectangular (non-square) matrix multiplication
- `std::vector<std::vector<double>>` layout vs. the flat contiguous layout
