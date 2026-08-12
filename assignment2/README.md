# CS509 Assignment 2 — Bellman-Ford & Floyd-Warshall (Individual Task)

Single-source shortest paths (Bellman-Ford, negative weights allowed) and
all-pairs shortest paths (Floyd-Warshall) on CSR graphs, per the assignment
spec. Written in C11 (same toolchain style as Assignment 1).

Author: 2026CSM1001 (individual task; Triangle Counting / Betweenness
Centrality / Connected Components are the buddy-task algorithms and are not
part of this submission).

## Build & run

```
mingw32-make          # or: make        -> builds graphs.exe, gen_graph.exe
./graphs bf bf_10.txt                 # Bellman-Ford
./graphs fw fw_10.txt                 # Floyd-Warshall
./graphs                              # interactive menu (algorithm + file)
./graphs bf bf_100.txt -s 42          # optional SOURCE override (cross-check)
./graphs bf missing.txt               # -> Error: cannot open input file: ...
make clean                            # (del /q on Windows, rm -f on Linux)
```

`graphs.exe` accepts `bf`/`bellmanford` and `fw`/`floydwarshall`. With no
arguments it falls back to an interactive menu. Input files are fully
validated (vertex order, degree counts vs E, neighbour range, matrix
dimensions, diagonal must be 0, negative weight on an undirected edge is
rejected, missing SOURCE line, trailing garbage, missing file).

## Files

| File | Role |
|---|---|
| `main.c` | Driver: args/menu, validation, adjacency-list→CSR prep, timing, output |
| `graph.c/h` | Adjacency-list reader, dense-matrix reader, `AdjListToCSR()` |
| `bellman.c/h` | `bellman_ford()` on CSR (incl. negative-cycle check pass) |
| `floyd.c/h` | `floyd_warshall()` on dense matrix (incl. diagonal negative-cycle check) |
| `timer.h` | Stopwatch (QueryPerformanceCounter on Windows, CLOCK_MONOTONIC elsewhere) |
| `gen_graph.c` | Random test-file generator (see below) |
| `bf_*.txt`, `fw_*.txt` | Test inputs (Section 12 naming) |
| `bf_negcyc.txt`, `fw_negcyc.txt` | Extra tests exercising negative-cycle detection |
| `results/` | Captured stdout from every test file listed in the tables |
| `README.md` | This file, with the required result tables (Section 11) |

## Timing methodology (Section 10)

- Timer starts immediately before the algorithm call and stops right after
  it returns; the Bellman-Ford extra relaxation pass and the Floyd-Warshall
  diagonal check are inside the timed function by design.
- File reading, parsing, validation, adjacency-list→CSR conversion, matrix
  construction and printing are **never** timed.
- `timer.h` uses high-resolution Windows `QueryPerformanceCounter`.
- Every table time is the average of 2 runs, reported in ms (same unit
  throughout). Values below 0.01 ms print as `0.00`.
- Environment: Windows, MinGW gcc 4.8.3, `-std=c11 -O2`. All sizes
  completed; no core dumps.

## Algorithms

- **Bellman-Ford**: V−1 full relaxation passes over the CSR edge array, then
  one extra full pass; if any edge still relaxes, a negative-weight cycle is
  reachable from the source → `Negative cycle: true`, no distance table.
  Unreachable vertices print `INF`. Optimization: a pass that changes
  nothing terminates the loop early — result is identical to running all
  V−1 passes (on the generated graphs this is what keeps the 100k case at
  a few ms, as Section 4.2 intends).
- **Floyd-Warshall**: classic `dist[i][j] = min(dist[i][j], dist[i][k] +
  dist[k][j])`, skipping `INF` entries. After the run, any negative
  `dist[i][i]` ⇒ negative-weight cycle → `Negative cycle: true`, no matrix.
- Distances use 64-bit integers; `INF = 2^60` is a safe sentinel (weights
  are small ints, so finite sums can never reach it).

## Random graph generator (per Section 4.2)

```
gen_graph bf   V E seed out.txt     # weighted directed adjacency list, SOURCE 0
gen_graph fw   V E seed out.txt     # same graph as dense matrix (INF/0 diagonal)
gen_graph convert bf.txt fw.txt     # adjacency list -> matching matrix
```

Generated graphs: guaranteed path 0→1→…→V−1 (all vertices reachable from
SOURCE 0) plus random strictly-forward edges, ~20% with negative weight
in `[-500,−1]`, positive in `[1,1000]`, deduplicated (min weight kept).
Because every edge advances the vertex index, the graph is a **DAG by
construction**: no directed cycle, hence no negative-weight cycle, so the
random files never trip the cycle detector (expected: `none`). Negative
*cycle* behaviour is exercised explicitly by the hand-written
`bf_negcyc.txt` (cycle `1→2→3→1` = −3−2−4 = −9) and `fw_negcyc.txt`
(cycle `2→3→2` = −2−4 = −6, so `dist[3][3] < 0`).

`fw_10.txt`/`fw_100.txt` are the *same* graphs as `bf_10.txt`/`bf_100.txt`
restated as matrices (via `convert`), which makes the BF-vs-FW cross-check
below exact.

## 11.1 Bellman-Ford / Floyd-Warshall results table

Algorithm | Test File | Vertices | Edges | Source | Negative Cycle | Expected Output | Actual Output | Time (avg of 2, ms) | Status
---|---|---|---|---|---|---|---|---|---
Bellman-Ford | bf_10.txt | 5 | 10 | 0 | No | Distances 0 2 4 7 -2 (Section 5.3) | Same | 0.00 | Pass
Bellman-Ford | bf_100.txt | 100 | 241 | 0 | No | Distances (matches FW row 0) | Same (14 neg. dist.) | 0.01 | Pass
Bellman-Ford | bf_10000.txt | 10000 | 19982 | 0 | No | Distances (matches FW row 0) | Same | 0.38 | Pass
Bellman-Ford | bf_50000.txt | 50000 | 149966 | 0 | No | Distances | Same | 2.28 | Pass
Bellman-Ford | bf_100000.txt | 100000 | 249972 | 0 | No | Distances | Same | 4.57 | Pass
Bellman-Ford | bf_negcyc.txt | 4 | 5 | 0 | **Yes** (cycle 1→2→3→1) | `Negative cycle: true`, no table | Same | 0.00 | Pass
Floyd-Warshall | fw_10.txt | 5 | 10 | N/A | No | Distance matrix (rows = BF from each source) | Same | 0.00 | Pass
Floyd-Warshall | fw_100.txt | 100 | 241 | N/A | No | Distance matrix | Same | 0.46 | Pass
Floyd-Warshall | fw_500.txt | 500 | 1235 | N/A | No | Distance matrix | Same | 36.20 | Pass
Floyd-Warshall | fw_1000.txt | 1000 | 2971 | N/A | No | Distance matrix | Same | 308.72 | Pass
Floyd-Warshall | fw_2000.txt | 2000 | 4988 | N/A | No | Distance matrix | Same | 2434.22 | Pass
Floyd-Warshall | fw_negcyc.txt | 4 | 4 | N/A | **Yes** (cycle 2→3→2) | `Negative cycle: true`, no matrix | Same | 0.00 | Pass

Graph properties: all BF input files are **directed, weighted** adjacency
lists; FW files are dense matrices (INF = no edge, 0 diagonal). Edge counts
above are what is recorded in the file header; random files keep E ≈ 2V–3V
(deduplication trims a few duplicates), per Section 4.2.

Captured outputs: `results/bf_*.out`, `results/fw_*.out` (stdout of every
file above, exactly as printed by `graphs`).

## BF ↔ FW cross-check (Section 6.3)

For the sizes required in both algorithms (10 and 100), Bellman-Ford was
run **from every vertex as source** (via `-s N`) and compared against the
corresponding row of the Floyd-Warshall output on the same graph:

| Pair | Vertices | BF runs | Mismatches | Result |
|---|---|---|---|---|
| bf_10.txt vs fw_10.txt | 5 | 5 | 0 | Pass |
| bf_100.txt vs fw_100.txt | 100 | 100 | 0 | Pass |

## Deviations / notes vs the spec text

1. **CSR helper.** The spec says to call the CSR-conversion function from
   Assignment 1; Assignment 1 is the mini-CPU simulator and contains no
   such function, so `AdjListToCSR()` is implemented here in `graph.c`
   instead of being copied from elsewhere.
2. **Bellman-Ford early exit** (documented above): timing still includes
   the mandatory negative-cycle check pass.
3. **INF printing**: unreachable vertices / missing matrix entries print
   as `INF` (the literal `INF` produced by the generator is preserved
   exactly by the skip-INF Floyd-Warshall).
4. `-s N` source override is an extra driver option used for the
   cross-check only.
5. Negative weights are only placed on directed edges (Section 3), so no
   undirected negative-edge rejections occur in this (directed) task.

## Minimum driver behaviour (Section 13) — summary

- Terminal args + interactive menu both supported.
- Full input validation with clear `Error:` messages on stdout/stderr.
- CSR built via `AdjListToCSR()` before timing; timing excludes it.
- Final answer and measured algorithm time printed in the exact Section 5/6
  format; invalid/missing file → clear error, non-zero exit.