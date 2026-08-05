# CS509 Assignment 1 - Individual Task: GEMM

Single-task implementation for **Assignment 1** (CS509), GEMM part:

- **GEMM - General Matrix Multiplication**: simple (direct nested-loop) and
  blocking (tiled) implementations of `C = A x B` for `A` of size `M x K` and
  `B` of size `K x N`.

The CSR graph implementation lives in its own assignment folder, `../csr_assignment`
(same repository root, same layout style). The common wrapper in
`common_wrapper/` manages both folders.

## Repository layout

```
matmul_assignment/
├── Makefile                     # build / run targets
├── common_wrapper/
│   ├── wrapper.cpp              # interactive menu (assignments 1 + 2, run one/all tests)
│   └── wrapper.exe
├── tools/
│   └── gen_tests.py             # deterministic test-data generator for both assignments
└── assignment_01/
    ├── src/
    │   ├── matrix.h / matrix.cpp    # Matrix class (dense row-major container)
    │   └── gemm.h / gemm.cpp        # GEMM simple + blocking implementations
    ├── driver/
    │   ├── driver.cpp               # driver: reads input file, times only the algorithm
    │   └── driver.exe
    ├── tests/                       # one test case per .txt file
    │   └── gemm_test_01.txt .. gemm_test_09.txt
    └── outputs/                     # saved outputs (output_<test>.txt)
```

Sibling folder (same repository):

```
csr_assignment/
├── Makefile
├── src/csr.h, csr.cpp          # adjacency list -> CSR conversion
├── driver/driver.cpp           # CSR-only driver (conversion + timed scan)
├── tests/csr_*.txt             # graph sizes 10 .. 100,000 vertices
└── outputs/
```

## Building and running

Requires `g++` with C++11 support.

```sh
make                 # build driver + wrapper
make run-gemm TEST=gemm_test_01.txt
make run-all-gemm
make csr             # build the CSR assignment driver (../csr_assignment)
```

or directly:

```sh
g++ -O2 -Wall -Wextra -std=c++11 -I assignment_01/src \
    assignment_01/src/matrix.cpp assignment_01/src/gemm.cpp \
    assignment_01/driver/driver.cpp -o assignment_01/driver/driver

# usage: driver gemm <input_file|--all>
assignment_01/driver/driver gemm assignment_01/tests/gemm_test_01.txt
assignment_01/driver/driver gemm --all
```

Interactive menu (compile / run one test / run all tests, per assignment and
algorithm):

```sh
common_wrapper/wrapper   (or: make wrapper && common_wrapper/wrapper)
```

A clear error message is printed for a missing or invalid input file.

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
- Hardware used for the numbers below: Windows 10, g++ 4.8.3 (MinGW), -O2.

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

## GEMM results

Both implementations run on the same input file and must produce the same
result matrix. `gemm_test_01.txt` is the example from the assignment and its
result matrix is `58 64 / 139 154`, as required.

### 9.1 GEMM results table

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

## Regenerating test data

The committed `.txt` files (for both assignments) were produced by
`tools/gen_tests.py` (fixed-seed deterministic generator):

```sh
python tools/gen_tests.py
```
