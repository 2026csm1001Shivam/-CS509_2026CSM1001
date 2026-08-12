#!/usr/bin/env python3

import os
import random

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
GEMM_DIR = os.path.join(REPO_ROOT, "matmul_assignment", "assignment_01", "tests")
CSR_DIR = os.path.join(REPO_ROOT, "csr_assignment", "tests")

rng = random.Random(12345)

GEMM_CASES = [
    ("gemm_test_01.txt", 2, 3, 2, 1,
     [[[1, 2, 3], [4, 5, 6]], [[7, 8], [9, 10], [11, 12]]]),
    ("gemm_test_02.txt", 8, 8, 8, 2, None),
    ("gemm_test_03.txt", 16, 32, 24, 4, None),
    ("gemm_test_04.txt", 64, 64, 64, 8, None),
    ("gemm_test_05.txt", 128, 128, 128, 16, None),
    ("gemm_test_06.txt", 128, 256, 192, 32, None),
    ("gemm_test_07.txt", 256, 256, 256, 32, None),
    ("gemm_test_08.txt", 512, 512, 512, 64, None),
    ("gemm_test_09.txt", 1024, 1024, 1024, 64, None),
]


def rand_matrix(rows, cols):
    return [[rng.randint(0, 9) for _ in range(cols)] for _ in range(rows)]


def write_gemm_file(name, M, K, N, block, matrices):
    path = os.path.join(GEMM_DIR, name)
    with open(path, "w") as f:
        f.write("# {} - GEMM test case: A = {}x{}, B = {}x{}, C = {}x{}\n".format(
            name, M, K, K, N, M, N))
        f.write("# BLOCK {}\n".format(block))
        f.write("{} {} {}\n".format(M, K, N))
        if matrices is None:
            matrices = [rand_matrix(M, K), rand_matrix(K, N)]
        for row in matrices[0]:
            f.write(" ".join(str(x) for x in row) + "\n")
        for row in matrices[1]:
            f.write(" ".join(str(x) for x in row) + "\n")


def undirected_edges(V, weighted=False):
    edges = set()
    for u in range(V):
        deg = rng.randint(1, 4)
        for _ in range(deg):
            v = rng.randrange(0, V)
            if v == u:
                v = (v + 1) % V
            edges.add((min(u, v), max(u, v)))
    out = {}
    for (u, v) in sorted(edges):
        w = rng.randint(1, 10) if weighted else None
        out[(u, v)] = w
    return out


def write_csr_file(name, V, weighted=False):
    path = os.path.join(CSR_DIR, name)
    edges = undirected_edges(V, weighted)
    E = len(edges)

    neighbors = {u: [] for u in range(V)}
    weights = {u: [] for u in range(V)}
    for (u, v), w in edges.items():
        neighbors[u].append(v)
        weights[u].append(w if w is not None else 0)
        neighbors[v].append(u)
        weights[v].append(w if w is not None else 0)

    with open(path, "w") as f:
        f.write("# {} - {} adjacency list, V = {}, E = {}\n".format(
            name, "weighted" if weighted else "unweighted", V, E))
        f.write("{} {}\n".format(V, E))
        for u in range(V):
            deg = len(neighbors[u])
            if weighted:
                parts = []
                for v, w in zip(neighbors[u], weights[u]):
                    parts.append("{} {}".format(v, w))
                f.write("{} {} {}\n".format(u, deg, " ".join(parts)))
            else:
                f.write("{} {} {}\n".format(u, deg,
                        " ".join(str(v) for v in neighbors[u])))
        f.write("SOURCE 0\n")


def main():
    for d in (GEMM_DIR, CSR_DIR):
        if not os.path.isdir(d):
            raise SystemExit("tests dir not found: " + d)

    for (name, M, K, N, block, matrices) in GEMM_CASES:
        write_gemm_file(name, M, K, N, block, matrices)
        print("wrote {:24s}  A: {}x{}  B: {}x{}  block {}".format(
            name, M, K, K, N, block))

    write_csr_file("csr_10.txt", 10)
    write_csr_file("csr_100.txt", 100)
    write_csr_file("csr_10000.txt", 10000)
    write_csr_file("csr_50000.txt", 50000)
    write_csr_file("csr_100000.txt", 100000)
    write_csr_file("csr_weighted_10.txt", 10, weighted=True)
    write_csr_file("csr_weighted_100.txt", 100, weighted=True)

    for d in (GEMM_DIR, CSR_DIR):
        print("{}:".format(d))
        for f in sorted(os.listdir(d)):
            if f.endswith(".txt"):
                size = os.path.getsize(os.path.join(d, f))
                print("  {:24s} {:>10} bytes".format(f, size))


if __name__ == "__main__":
    main()
