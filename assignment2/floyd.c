#include "floyd.h"

#include "graph.h"

void floyd_warshall(int64_t *D, int V, int *neg_cycle) {
    for (int k = 0; k < V; k++) {
        const int64_t *rowk = D + (size_t)k * V;
        for (int i = 0; i < V; i++) {
            int64_t dik = D[(size_t)i * V + k];
            if (dik == GRAPH_INF) continue;
            int64_t *rowi = D + (size_t)i * V;
            for (int j = 0; j < V; j++) {
                if (rowk[j] == GRAPH_INF) continue;
                int64_t nd = dik + rowk[j];
                if (nd < rowi[j]) rowi[j] = nd;
            }
        }
    }

    *neg_cycle = 0;
    for (int i = 0; i < V; i++) {
        if (D[(size_t)i * V + i] < 0) {
            *neg_cycle = 1;
            return;
        }
    }
}