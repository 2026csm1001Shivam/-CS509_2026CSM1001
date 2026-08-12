#include "bellman.h"

int bellman_ford(const CSRGraph *g, int src, int64_t *dist) {
    int V = g->V;

    for (int i = 0; i < V; i++) dist[i] = GRAPH_INF;
    dist[src] = 0;

    for (int pass = 0; pass < V - 1; pass++) {
        int changed = 0;
        for (int u = 0; u < V; u++) {
            int64_t du = dist[u];
            if (du == GRAPH_INF) continue;
            for (int e = g->row_ptr[u]; e < g->row_ptr[u + 1]; e++) {
                int v = g->col_idx[e];
                int64_t nd = du + g->values[e];
                if (nd < dist[v]) {
                    dist[v] = nd;
                    changed = 1;
                }
            }
        }
        if (!changed) break;
    }

    for (int u = 0; u < V; u++) {
        int64_t du = dist[u];
        if (du == GRAPH_INF) continue;
        for (int e = g->row_ptr[u]; e < g->row_ptr[u + 1]; e++) {
            int v = g->col_idx[e];
            if (du + g->values[e] < dist[v]) return 1;
        }
    }
    return 0;
}