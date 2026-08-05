# CS509 Assignment 1 - CSR Graph (adjacency list -> CSR)

Part of the individual task for **Assignment 1** (CS509). This folder mirrors
the `matmul_assignment/` layout and contains the CSR graph implementation as a
separate, self-contained assignment folder.

- **CSR (Compressed Sparse Row)** conversion: reads a graph in adjacency-list
  form (unweighted or positive-weighted) and converts it into the three CSR
  arrays `row_ptr`, `col_idx`, `values`.
- **Validation**: rebuilds the adjacency list from the CSR arrays and checks
  that it matches the input exactly (length, order, and weights).
- **Timed CSR scan**: one full scan of the CSR structure (the neighbour
  iteration every CSR-based graph algorithm performs). Per the assignment
  timing rule, the adjacency-list-to-CSR conversion is preprocessing and its
  time is **not** included in the reported algorithm time.
- The BFS / DFS / SSSP buddy tasks build on this file and reuse the
  adjacency-list test files in `tests/`.

## Repository layout

```
csr_assignment/
├── Makefile                  # build / run targets
├── src/
│   ├── csr.h                 # AdjacencyList, CSRGraph, build_csr, csr_verify, csr_scan_checksum
│   └── csr.cpp
├── driver/
│   ├── driver.cpp            # parses adjacency-list file, builds CSR, times only the scan
│   └── driver.exe
├── tests/                    # one test case per .txt file
│   ├── csr_10.txt, csr_100.txt, csr_10000.txt, csr_50000.txt, csr_100000.txt
│   └── csr_weighted_10.txt, csr_weighted_100.txt
└── outputs/                  # saved outputs (output_<test>.txt)
```

## Building and running

Requires `g++` with C++11 support.

```sh
make
make run-one TEST=csr_10.txt
make run-all
```

or directly:

```sh
g++ -O2 -Wall -Wextra -std=c++11 -I src src/csr.cpp driver/driver.cpp -o driver/driver

# usage: driver <input_file|--all>
driver tests/csr_10.txt
driver --all
```

A clear error message is printed for a missing or invalid input file. The
common wrapper (in `matmul_assignment/common_wrapper/`) lists this as
Assignment 2 and can compile and run it.

## Graph input format (assignment rules 3, 6, 7)

Unweighted adjacency list (used by BFS/DFS):

```
V E
u0 degree neighbor1 neighbor2 ...
u1 degree neighbor1 neighbor2 ...
...
SOURCE s
```

Positive-weighted adjacency list (used by SSSP): each neighbour is followed by
its positive weight:

```
V E
u0 degree neighbor1 weight1 neighbor2 weight2 ...
...
SOURCE s
```

- `V` = number of vertices, numbered `0 .. V-1`; `E` = number of edges (each
  undirected edge counted once, even though it appears in both adjacency
  lists).
- A vertex with no neighbours is written as `u 0`.
- For an undirected graph each edge appears in the lists of both endpoints.

## Timing methodology (assignment rule 8)

- The timer starts immediately before the algorithm call and stops immediately
  after it finishes.
- File reading, parsing, adjacency-list-to-CSR conversion, output printing and
  file writing are **not** included. The conversion time is printed separately
  as "Preprocessing (adj-list -> CSR) time: ... ms (not counted)".
- For very fast inputs the measurement is repeated and the **average** is
  reported; the number of runs is printed with every measurement (R = max(1,
  min(100000, floor(5 ms / single-run time)))).
- Unit: milliseconds (ms); timer is `QueryPerformanceCounter` on Windows.
- Hardware used for the numbers below: Windows 10, g++ 4.8.3 (MinGW), -O2.

## Results (graph result table, assignment 9.2)

| Algorithm | Test File | Vertices | Edges | Input Type | Source | Expected Output | Actual Output | Time | Status |
|---|---|---|---|---|---|---|---|---|---|
| CSR | csr_10.txt | 10 | 23 | Unweighted adjacency list | 0 | row_ptr/col_idx = adjacency list | CSR validation PASSED; scan checksum 216 | 0.000 ms | Pass |
| CSR | csr_100.txt | 100 | 240 | Unweighted adjacency list | 0 | row_ptr/col_idx = adjacency list | CSR validation PASSED; scan checksum 23836 | 0.001 ms | Pass |
| CSR | csr_10000.txt | 10,000 | 24,929 | Unweighted adjacency list | 0 | CSR equals adjacency list | CSR validation PASSED; scan checksum 249327523 | 0.084 ms | Pass |
| CSR | csr_50000.txt | 50,000 | 125,139 | Unweighted adjacency list | 0 | CSR equals adjacency list | CSR validation PASSED; scan checksum 6259941822 | 0.443 ms | Pass |
| CSR | csr_100000.txt | 100,000 | 249,813 | Unweighted adjacency list | 0 | CSR equals adjacency list | CSR validation PASSED; scan checksum 24976630616 | 0.882 ms | Pass |
| CSR | csr_weighted_10.txt | 10 | 22 | Positive weighted adjacency list | 0 | row_ptr/col_idx/values = adjacency list | CSR validation PASSED; scan checksum 410 | 0.000 ms | Pass |
| CSR | csr_weighted_100.txt | 100 | 231 | Positive weighted adjacency list | 0 | row_ptr/col_idx/values = adjacency list | CSR validation PASSED; scan checksum 24206 | 0.001 ms | Pass |

CSR conversion (preprocessing) times, **not** counted in the algorithm time:
csr_10: 0.002 ms, csr_100: 0.008 ms, csr_10000: 0.254 ms, csr_50000:
1.378 ms, csr_100000: 2.741 ms.

All 5 required graph sizes (10, 100, 10,000, 50,000, 100,000 vertices) are
covered. The test files are generated deterministically by
`matmul_assignment/tools/gen_tests.py` (fixed seed), so every run is
reproducible.
