#ifndef MST_GRAPH_H
#define MST_GRAPH_H

#include "graph.h"

/*
 * Reads a weighted undirected adjacency-list file (MST format, Section 5.1):
 *
 *   V E
 *   u degree neighbor1 weight1 neighbor2 weight2 ...
 *   ...
 *   u(V-1) degree ...
 *
 * into the AdjList structure from the previous assignment.  Validates:
 * vertex order, degree counts, neighbour range, weight tokens, sum of
 * degrees == 2E, no self-loops, no duplicate neighbours, edge symmetry
 * (each edge mirrored in both endpoint lists with the same weight), and
 * graph connectivity.  Negative/zero weights are legal (Section 3).
 */
int read_mst_adjlist(const char *path, AdjList *g, char *err, size_t errsz);

#endif
