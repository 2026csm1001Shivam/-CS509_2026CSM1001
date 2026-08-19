#ifndef PRIM_H
#define PRIM_H

#include "graph.h"
#include "kruskal.h" /* MstEdge */

/*
 * Prim's MST on a CSR graph of a connected, weighted, undirected graph.
 * Grows the tree from vertex 0 using a binary min-heap with lazy
 * deletion; works with negative and zero edge weights.
 *
 * Everything inside this call is timed (Section 8).
 *
 * Returns the total MST weight and fills `out` (capacity V-1) with the
 * selected edges; `n_out` receives the number of selected edges (V-1).
 */
int64_t prim_mst(const CSRGraph *g, MstEdge *out, int *n_out);

#endif
