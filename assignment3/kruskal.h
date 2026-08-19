#ifndef KRUSKAL_H
#define KRUSKAL_H

#include "graph.h"

typedef struct {
    int u, v;
    int64_t w;
} MstEdge;

/*
 * Kruskal's MST on a CSR graph of a connected, weighted, undirected graph.
 * The CSR is expected to hold every undirected edge twice (once per
 * endpoint), which is what the adjacency-list format of Section 5.1
 * produces.
 *
 * Everything inside this call is timed: extraction of the sortable edge
 * list from the CSR, sorting, DSU, and edge selection (Section 8).
 *
 * Returns the total MST weight and fills `out` (capacity V-1) with the
 * selected edges; `n_out` receives the number of selected edges (V-1).
 */
int64_t kruskal_mst(const CSRGraph *g, MstEdge *out, int *n_out);

#endif
