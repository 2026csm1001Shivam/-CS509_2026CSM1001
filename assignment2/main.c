#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bellman.h"
#include "floyd.h"
#include "graph.h"
#include "timer.h"

static void usage(const char *prog) {
    printf("Usage: %s <algorithm> <input-file> [-s source]\n", prog);
    printf("  algorithms: bf | bellmanford | fw | floydwarshall\n");
    printf("  -s N  : override the SOURCE vertex (bf only, for cross-checking)\n");
    printf("With no arguments the program runs an interactive menu.\n");
}

static void run_bf(const char *file, int src_override) {
    char err[256];
    AdjList adj;
    int src = -1;

    if (read_weighted_adjlist(file, &adj, &src, err, sizeof(err)) != 0) {
        fprintf(stderr, "Error: %s\n", err);
        exit(1);
    }
    if (src_override >= 0) {
        if (src_override >= adj.V) {
            fprintf(stderr, "Error: -s %d out of range [0,%d)\n",
                    src_override, adj.V);
            free_adjlist(&adj);
            exit(1);
        }
        src = src_override;
    }

    CSRGraph csr;
    AdjListToCSR(&adj, &csr);
    free_adjlist(&adj);

    int64_t *dist = (int64_t *)malloc((size_t)csr.V * sizeof(int64_t));
    if (!dist) {
        fprintf(stderr, "Error: out of memory\n");
        free_csr(&csr);
        exit(1);
    }

    Stopwatch sw;
    stopwatch_start(&sw);
    int neg_cycle = bellman_ford(&csr, src, dist);
    double ms = stopwatch_ms(&sw);

    printf("Algorithm: Bellman-Ford\n");
    printf("Source: %d\n", src);
    if (neg_cycle) {
        printf("Negative cycle: true\n");
    } else {
        printf("Vertex Distance\n");
        for (int v = 0; v < csr.V; v++) {
            printf("%d ", v);
            fprint_i64(stdout, dist[v]);
            printf("\n");
        }
        printf("Negative cycle: none\n");
    }
    printf("Execution time: %.2f ms\n", ms);

    free(dist);
    free_csr(&csr);
}

static void run_fw(const char *file) {
    char err[256];
    int V = -1;
    int64_t *mat = NULL;

    if (read_dense_matrix(file, &V, &mat, err, sizeof(err)) != 0) {
        fprintf(stderr, "Error: %s\n", err);
        exit(1);
    }

    Stopwatch sw;
    stopwatch_start(&sw);
    int neg_cycle = 0;
    floyd_warshall(mat, V, &neg_cycle);
    double ms = stopwatch_ms(&sw);

    printf("Algorithm: Floyd-Warshall\n");
    if (neg_cycle) {
        printf("Negative cycle: true\n");
    } else {
        printf("Distance matrix:\n");
        for (int i = 0; i < V; i++) {
            for (int j = 0; j < V; j++) {
                if (j) printf(" ");
                fprint_i64(stdout, mat[(size_t)i * V + j]);
            }
            printf("\n");
        }
        printf("Negative cycle: none\n");
    }
    printf("Execution time: %.2f ms\n", ms);

    free(mat);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        usage(argv[0]);
        if (argc == 1) {
            char alg[64], file[512];
            printf("\nInteractive mode.\n");
            printf("Algorithm (bf/fw): ");
            if (!fgets(alg, sizeof(alg), stdin)) return 1;
            printf("Input file: ");
            if (!fgets(file, sizeof(file), stdin)) return 1;
            alg[strcspn(alg, "\r\n")] = '\0';
            file[strcspn(file, "\r\n")] = '\0';
            if (strcmp(alg, "bf") == 0 || strcmp(alg, "bellmanford") == 0)
                run_bf(file, -1);
            else if (strcmp(alg, "fw") == 0 || strcmp(alg, "floydwarshall") == 0)
                run_fw(file);
            else {
                fprintf(stderr, "Error: unknown algorithm '%s'\n", alg);
                return 1;
            }
        }
        return 0;
    }

    const char *alg = argv[1];
    const char *file = argv[2];
    int src_override = -1;

    for (int i = 3; i < argc; i++) {
        if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            src_override = atoi(argv[++i]);
        } else {
            fprintf(stderr, "Error: unknown option '%s'\n", argv[i]);
            usage(argv[0]);
            return 1;
        }
    }

    if (strcmp(alg, "bf") == 0 || strcmp(alg, "bellmanford") == 0) {
        run_bf(file, src_override);
    } else if (strcmp(alg, "fw") == 0 || strcmp(alg, "floydwarshall") == 0) {
        run_fw(file);
    } else {
        fprintf(stderr, "Error: unknown algorithm '%s'\n", alg);
        usage(argv[0]);
        return 1;
    }
    return 0;
}