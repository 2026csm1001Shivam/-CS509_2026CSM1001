# CS509 Assignment 3 — Minimum Spanning Tree (Individual Task)

Kruskal's algorithm and Prim's algorithm on weighted undirected graphs in
CSR format, per the assignment spec. The adjacency-list input file is
converted to CSR (via the CSR helper from the previous assignment, **not
copied**) before the timed algorithm call.

Author: 2026CSM1001 (individual task; Gradient Descent and Maxflow-Mincut
are the buddy-task algorithms and are not part of this submission).

## Build & run

```
mingw32-make                # or: make -> builds mst.exe, gen_mst.exe
./mst both mst_example.txt  # both algorithms, average of 3 runs
./mst kruskal mst_100.txt   # Kruskal only
./mst prim mst_100.txt      # Prim only
./mst both mst_100000.txt -runs 1   # single run (no averaging)
./mst                       # interactive menu (algorithm + file)
./mst kruskal missing.txt   # -> Error: cannot open input file: missing.txt
make clean                  # (del /q on Windows, rm -f on Linux)
```

`mst.exe` accepts `kruskal`, `prim` and `both`. With no arguments it falls
back to an interactive menu. `-runs N` repeats the algorithm N times and
reports the average execution time (default 3, documented in every output
line).

## Files

| File | Role |
|---|---|
| `main.c` | Driver: args/menu, validation, adjacency-list→CSR prep (calls `AdjListToCSR` from `../assignment2/graph.c`), timing, output |
| `mst_graph.c/h` | MST adjacency-list reader + full validation (order, degree, range, sumdeg=2E, symmetry, connectivity) |
| `kruskal.c/h` | `kruskal_mst()` on CSR: edge extraction + sort + DSU inside the timed call |
| `prim.c/h` | `prim_mst()` on CSR: binary min-heap with lazy deletion, tree grown from vertex 0 |
| `gen_mst.c` | Random connected weighted undirected graph generator (Section 4.2) |
| `Makefile` | Compiles the CSR helper from the previous assignment (`../assignment2/graph.c` → `graph_a2.o`) |
| `mst_example.txt` | The Section 5.2 worked example (expected weight 16) |
| `mst_10.txt` … `mst_100000.txt` | Required test inputs (Section 10 naming) |
| `mst_bad_*.txt` | Invalid inputs used to exercise the validation errors |
| `README.md` | This file, with the required result tables (Section 9.1) |

`graph.h`, `timer.h` and `AdjListToCSR()` come from the previous
assignment (`../assignment2`) via `-I$(ASSIGN2)` and a direct compile of
`../assignment2/graph.c`; none of that code is copied into this folder.

## CSR conversion (Section 4.1)

The driver reads the weighted undirected adjacency list into the
`AdjList` structure from the previous assignment and calls
`AdjListToCSR()` on it. The conversion is preprocessing: its execution
time is **not** included in the reported algorithm runtime.

## Algorithms

- **Kruskal** (`kruskal.c`): the sortable edge list is extracted from the
  already-prepared CSR inside the timed call (each undirected edge appears
  once via the `u < v` direction), sorted by weight with a deterministic
  (u,v) tie-break, and processed with a Disjoint Set Union (path
  compression + union by rank) until V−1 edges are selected.
- **Prim** (`prim.c`): starts from vertex 0 and grows one tree using a
  binary min-heap with lazy deletion (stale entries skipped). Works with
  negative and zero edge weights. Stops when all vertices are in the tree.

Both algorithms handle the full allowed weight range: positive, zero and
negative integers (Section 3).

## Timing methodology (Section 8)

- The timer (`timer.h`, `QueryPerformanceCounter`) starts immediately
  before the algorithm call and stops right after it returns. Edge
  extraction + sorting + DSU (Kruskal) and heap operation (Prim) are
  inside the timed call, as Section 8 requires.
- File reading, parsing, validation, adjacency-list→CSR conversion and
  printing are never timed.
- Every table time is the average of **3 runs** (`-runs 3`, documented in
  the output as `(average of 3 runs)`), reported in ms (same unit
  throughout). Values below 0.01 ms print as `0.00`.
- Environment: Windows, MinGW gcc 4.8.3, `-std=c11 -O2`. All required
  sizes completed; no core dumps or excessive runtime.

## Random graph generator (Section 4.2)

```
gen_mst V E seed outfile
```

Generates a connected weighted undirected graph with V vertices and E
distinct edges: a random spanning tree (vertex i joins a random earlier
vertex) plus random extra edges, deduplicated with a hash set. Weights are
drawn from a mix of positive (70%), negative (25%) and zero (5%) values in
[−1000, 1000] so that the full allowed weight range is exercised. The
generator also computes the MST weight of the produced graph with its own
internal Kruskal implementation and prints it — this is the `Exp. Wt.`
column below, cross-checked independently (see next section).

## 9.1 MST results table

| File | V | E | Exp. Wt. | Kruskal Wt. | Prim Wt. | Kruskal Time | Prim Time | Equal? | Status |
|---|---|---|---|---|---|---|---|---|---|
| mst_example.txt | 5 | 7 | 16 | 16 | 16 | 0.00 ms | 0.00 ms | Yes | Pass |
| mst_10.txt | 10 | 16 | -900 | -900 | -900 | 0.00 ms | 0.00 ms | Yes | Pass |
| mst_100.txt | 100 | 230 | -20869 | -20869 | -20869 | 0.02 ms | 0.01 ms | Yes | Pass |
| mst_10000.txt | 10000 | 25000 | -2475252 | -2475252 | -2475252 | 5.10 ms | 3.73 ms | Yes | Pass |
| mst_50000.txt | 50000 | 130000 | -13261310 | -13261310 | -13261310 | 28.77 ms | 21.52 ms | Yes | Pass |
| mst_100000.txt | 100000 | 260000 | -26451968 | -26451968 | -26451968 | 58.89 ms | 48.25 ms | Yes | Pass |

Graph type: weighted undirected, sparse (E ≈ 1.6V–2.6V per Section 4.2),
connected, weights include positive/zero/negative values. Times are the
average of 3 runs; Kruskal and Prim used the identical input files for
every row. Results are printed to the terminal; the table above is the
recorded output of those runs.

## Cross-checks

1. `Exp. Wt.` was produced at generation time by an independent Kruskal
   implementation inside `gen_mst.c`; `Kruskal Wt.` and `Prim Wt.` (the
   submitted implementations) match it on all six graphs.
2. A third, independent Python Prim implementation was run on
   `mst_example.txt`, `mst_10.txt` and `mst_100.txt` and returned the same
   totals (16, −900, −20869).
3. `mst_example.txt` reproduces the Section 5.3 worked example exactly:
   edges `0 1 2`, `1 2 3`, `1 4 5`, `0 3 6`, total weight **16**.

## Input validation (Section 11)

The driver rejects, with a clear `Error:` message on stderr and non-zero
exit: missing/unreadable file; vertex lines out of order; bad degree;
neighbour out of `[0,V)`; self-loops; duplicate neighbours; weight tokens
missing or malformed; `sum(degrees) != 2E`; asymmetric edge (mirrored
weight mismatch or missing mirror); trailing garbage after the vertex
list; disconnected graph (Section 3 requires connected MST inputs);
unknown algorithm; invalid `-runs`. The following errors were observed
directly in the terminal (stderr):

| Input | Error produced |
|---|---|
| missing.txt | `Error: cannot open input file: missing.txt` |
| mst_bad_asym.txt | `Error: file mst_bad_asym.txt: edge (0,2) not mirrored in vertex 2` |
| mst_bad_weightsum.txt | `Error: file mst_bad_weightsum.txt: declared E=2 but degrees sum to 3 (must be 2E)` |
| mst_bad_disconnected.txt | `Error: file mst_bad_disconnected.txt: graph is not connected (MST requires connectivity)` |
| `mst bogus mst_10.txt` | `Error: unknown algorithm 'bogus'` (+ usage) |

## Notes

1. Language is C11 to stay on the exact toolchain of Assignment 2
   (MinGW gcc 4.8.3, `-std=c11 -O2`), which also makes the CSR helper
   reuse a plain compile of the previous assignment's source.
2. `mst_10.txt` … `mst_100000.txt` use the Section 10 naming; the
   generator seeds are 101–105 (documented in `gen_mst` output).
3. Negative total weights are expected because the generator deliberately
   includes negative edges to exercise the allowed weight range.