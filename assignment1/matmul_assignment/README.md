# CS509 Assignment 1 — GEMM (Individual Task)

Single-task implementation of the **GEMM** part of Assignment 1 (CS509):

- **GEMM - General Matrix Multiplication**: simple (direct nested-loop) and
  blocking (tiled) implementations of `C = A x B` for `A` of size `M x K` and
  `B` of size `K x N`.

The CSR graph implementation lives in its own self-contained assignment
folder, `../csr_assignment` (same repository, same layout style). The common
wrapper in `common_wrapper/` manages both folders.

Author: 2026CSM1001 (individual task; the CSR graph conversion, `../csr_assignment`,
is the other half of this assignment and is covered by its own README).

## Build & run

Requires `g++` with C++11 support.

```
mingw32-make                       # or: make  -> builds driver + wrapper
mingw32-make run-gemm TEST=gemm_test_01.txt
mingw32-make run-all-gemm          # all 9 test cases
mingw32-make csr                   # build the CSR assignment driver (../csr_assignment)
```

or directly:

```
g++ -O2 -Wall -Wextra -std=c++11 -I assignment_01/src \
    assignment_01/src/matrix.cpp assignment_01/src/gemm.cpp \
    assignment_01/driver/driver.cpp -o assignment_01/driver/driver

# usage: driver gemm <input_file|--all>
assignment_01/driver/driver gemm assignment_01/tests/gemm_test_01.txt
assignment_01/driver/driver gemm --all
```

Interactive menu (compile / run one test / run all tests, per assignment and
algorithm):

```
common_wrapper/wrapper   (or: make wrapper && common_wrapper/wrapper)
```

A clear error message is printed for a missing or invalid input file.

## Files

| File | Role |
|---|---|
| `Makefile` | Build / run targets |
| `common_wrapper/wrapper.cpp` | Interactive menu (assignments 1 + 2, run one/all tests) |
| `tools/gen_tests.py` | Deterministic test-data generator for both assignments |
| `assignment_01/src/matrix.h/cpp` | `Matrix` class (dense row-major container) |
| `assignment_01/src/gemm.h/cpp` | GEMM simple + blocking implementations |
| `assignment_01/driver/driver.cpp` | Driver: reads input file, times only the algorithm |
| `assignment_01/tests/` | One test case per `.txt` file (`gemm_test_01.txt` .. `gemm_test_09.txt`) |
| `assignment_01/outputs/` | Saved outputs (`output_<test>.txt`) |
| `README.md` | This file, with the required result tables |

Sibling folder (same repository): `../csr_assignment` — CSR conversion
(`src/csr.h`, `src/csr.cpp`), CSR driver, `tests/csr_*.txt`, `outputs/`.

## GEMM input format (assignment rule 5)

Both implementations read the **same** input file:

```
M K N
A row 0 values ...       # K integers
A row 1 values ...
...
A row M-1 values ...
B row 0 values ...       # N integers
B row 1 values ...
...
B row K-1 values ...
```

A `# BLOCK n` comment line (before `M K N`) sets the block size (default 32).

## Algorithms

- **Simple** (`matmul_simple()`, `gemm.cpp`): textbook i-j-k triple loop,
  `C[i][j] += A[i][k] * B[k][j]`.
- **Blocked** (`matmul_blocked()`, `gemm.cpp`): tiles the loops into
  `bs x bs` blocks using i-k-j loop order so the inner j-loop reuses the
  cached `A(i,k)` element, improving cache locality for large matrices.
- **Correctness**: `matrices_equal()` compares the two C matrices with
  tolerance 1e-6; `matrix_checksum()` computes a checksum of the result.
  `render_matrix()` prints the full matrix when it has at most 1024 elements,
  otherwise an 8x8 sample plus the checksum.

## Timing methodology (assignment rule 8)

- The timer starts immediately before the algorithm call and stops immediately
  after it finishes.
- File reading, input parsing, matrix allocation, output printing and file
  writing are **not** included.
- For very fast inputs the measurement is repeated and the **average** is
  reported; the number of runs is printed with every measurement
  (R = max(1, min(100000, floor(5 ms / single-run time)))).
- Unit: milliseconds (ms) throughout; timer is `QueryPerformanceCounter` on
  Windows (high resolution), otherwise `std::chrono::steady_clock`.
- Environment: Windows 10, MinGW g++ 4.8.3, `-std=c++11 -O2`.

## Index 9.1 GEMM results table

Both implementations run on the same input file and must produce the same
result matrix. `gemm_test_01.txt` is the example from the assignment and its
result matrix is `58 64 / 139 154`, as required.

| Test File | Input Type / Size | Expected Output | Actual Output | Simple Time | Blocking Time | Block Size | Status |
|---|---|---|---|---|---|---|---|
| gemm_test_01.txt | A: 2x3, B: 3x2 | Result matrix 58 64 / 139 154 | Result matrix 58 64 / 139 154 | ~0.000 ms | ~0.000 ms | 1 | Pass |
| gemm_test_02.txt | A: 8x8, B: 8x8 | Result matrix | Result matrix (full, saved to output file) | ~0.000 ms | ~0.001 ms | 2 | Pass |
| gemm_test_03.txt | A: 16x32, B: 32x24 | Result matrix | Result matrix (full) | 0.012 ms | 0.012 ms | 4 | Pass |
| gemm_test_04.txt | A: 64x64, B: 64x64 | Result matrix | Result matrix (checksum 5221703) | 0.294 ms | 0.181 ms | 8 | Pass |
| gemm_test_05.txt | A: 128x128, B: 128x128 | Result matrix | Result matrix (checksum 42458609) | 3.380 ms | 1.252 ms | 16 | Pass |
| gemm_test_06.txt | A: 128x256, B: 256x192 | Result matrix | Result matrix (checksum 127497561) | 9.332 ms | 4.034 ms | 32 | Pass |
| gemm_test_07.txt | A: 256x256, B: 256x256 | Result matrix | Result matrix (checksum 338852981) | 29.989 ms | 9.462 ms | 32 | Pass |
| gemm_test_08.txt | A: 512x512, B: 512x512 | Result matrix | Result matrix (checksum 2712761970) | 688.117 ms | 78.740 ms | 64 | Pass |
| gemm_test_09.txt | A: 1024x1024, B: 1024x1024 | Result matrix | Result matrix (checksum 21777642836) | 5791.067 ms | 630.095 ms | 64 | Pass |

Full result matrices are printed for tests with at most 1024 elements; larger
results print the first 8x8 sample plus a checksum (complete matrices are
reproducible from the committed input files; inputs are small integers so all
arithmetic is exact in `double`).

Timings are averages over R runs (R printed by the driver); for the two
smallest GEMM tests R ~ 10000-25000 and the average is below the 0.001 ms
printing resolution, shown as 0.000-0.001 ms.

## Driver behaviour — summary

- Terminal args: `driver gemm <input_file|--all>`; usage error otherwise.
- `--all` mode lists every `gemm_test_*.txt` in the tests directory, runs
  each, and returns a failure exit code if any test fails.
- With no arguments the interactive wrapper (compiled by `make`) offers
  compile / run one / run all for both assignments.
- Missing or invalid input file -> clear `Error:` message and non-zero exit.
- For every test the driver prints the report to the console and saves the
  same report to `assignment_01/outputs/output_<test>.txt`.

## Deviations / notes vs the spec text

1. **Full vs. sampled output.** Result matrices with more than 1024 elements
   print an 8x8 sample plus a checksum instead of the full matrix (rule 5
   output is otherwise reproduced exactly).
2. **Average-of-R timing.** For sub-5 ms runs the measurement is repeated and
   the average reported; the single-run time and R are printed with each
   result (assignment rule 8).
3. **Block sizes per test.** `# BLOCK n` in each input file sets the tile
   size (1..64 across the tests); the default is 32.

## Regenerating test data

The committed `.txt` files (for both assignments) were produced by
`tools/gen_tests.py` (fixed-seed deterministic generator):

```
python tools/gen_tests.py
```