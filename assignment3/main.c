#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "graph.h"
#include "kruskal.h"
#include "mst_graph.h"
#include "prim.h"
#include "timer.h"

static void usage(const char *prog) {
    printf("Usage: %s <algorithm> <input-file> [-runs N]\n", prog);
    printf("  algorithms: kruskal | prim | both\n");
    printf("  -runs N : repeat the algorithm N times and report the\n");
    printf("            average execution time (default 3)\n");
    printf("With no arguments the program runs an interactive menu.\n");
}

static int run_algorithm(const char *name, const CSRGraph *csr,
                         int64_t (*fn)(const CSRGraph *, MstEdge *, int *),
                         int runs) {
    int V = csr->V;
    MstEdge *edges = (MstEdge *)malloc((size_t)(V > 1 ? V - 1 : 1) *
                                       sizeof(MstEdge));
    if (!edges) {
        fprintf(stderr, "Error: out of memory\n");
        return 1;
    }

    int64_t total = 0;
    int n = 0;
    double sum_ms = 0.0;
    for (int r = 0; r < runs; r++) {
        Stopwatch sw;
        stopwatch_start(&sw);
        total = fn(csr, edges, &n);
        sum_ms += stopwatch_ms(&sw);
    }
    double avg_ms = sum_ms / (double)runs;

    printf("Algorithm: %s\n", name);
    printf("MST edges:\n");
    for (int i = 0; i < n; i++) {
        printf("%d %d ", edges[i].u, edges[i].v);
        fprint_i64(stdout, edges[i].w);
        printf("\n");
    }
    printf("Total MST weight: ");
    fprint_i64(stdout, total);
    printf("\n");
    if (n != V - 1) {
        fprintf(stderr, "Error: selected %d edges, expected %d (graph must "
                        "be connected)\n", n, V - 1);
    }
    if (runs > 1)
        printf("Execution time: %.2f ms (average of %d runs)\n", avg_ms, runs);
    else
        printf("Execution time: %.2f ms\n", avg_ms);

    free(edges);
    return 0;
}

static int run_mst_file(const char *file, const char *alg, int runs) {
    char err[256];
    AdjList adj;

    if (read_mst_adjlist(file, &adj, err, sizeof(err)) != 0) {
        fprintf(stderr, "Error: %s\n", err);
        return 1;
    }

    CSRGraph csr;
    AdjListToCSR(&adj, &csr);
    free_adjlist(&adj);

    printf("Graph: %s (V=%d, E=%d)\n", file, csr.V, csr.E / 2);

    int rc = 0;
    if (strcmp(alg, "kruskal") == 0) {
        rc = run_algorithm("Kruskal's MST", &csr, kruskal_mst, runs);
    } else if (strcmp(alg, "prim") == 0) {
        rc = run_algorithm("Prim's MST", &csr, prim_mst, runs);
    } else { /* both */
        rc = run_algorithm("Kruskal's MST", &csr, kruskal_mst, runs);
        if (rc) goto done;
        printf("\n");
        rc = run_algorithm("Prim's MST", &csr, prim_mst, runs);
    }

done:
    free_csr(&csr);
    return rc;
}

static int menu(void) {
    char alg[64], file[512];
    printf("\nInteractive mode.\n");
    printf("Algorithm (kruskal/prim/both): ");
    if (!fgets(alg, sizeof(alg), stdin)) return 1;
    printf("Input file: ");
    if (!fgets(file, sizeof(file), stdin)) return 1;
    alg[strcspn(alg, "\r\n")] = '\0';
    file[strcspn(file, "\r\n")] = '\0';

    if (strcmp(alg, "kruskal") != 0 && strcmp(alg, "prim") != 0 &&
        strcmp(alg, "both") != 0) {
        fprintf(stderr, "Error: unknown algorithm '%s'\n", alg);
        return 1;
    }
    return run_mst_file(file, alg, 3);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        usage(argv[0]);
        if (argc == 1) return menu();
        return 1;
    }

    const char *alg = argv[1];
    const char *file = argv[2];
    int runs = 3;

    for (int i = 3; i < argc; i++) {
        if (strcmp(argv[i], "-runs") == 0 && i + 1 < argc) {
            int v = atoi(argv[++i]);
            if (v < 1) {
                fprintf(stderr, "Error: -runs must be >= 1\n");
                usage(argv[0]);
                return 1;
            }
            runs = v;
        } else {
            fprintf(stderr, "Error: unknown option '%s'\n", argv[i]);
            usage(argv[0]);
            return 1;
        }
    }

    if (strcmp(alg, "kruskal") != 0 && strcmp(alg, "prim") != 0 &&
        strcmp(alg, "both") != 0) {
        fprintf(stderr, "Error: unknown algorithm '%s'\n", alg);
        usage(argv[0]);
        return 1;
    }
    return run_mst_file(file, alg, runs);
}
